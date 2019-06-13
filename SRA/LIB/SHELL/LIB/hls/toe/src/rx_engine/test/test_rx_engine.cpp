/*****************************************************************************
 * @file       : test_rx_engine.cpp
 * @brief      : Testbench for Rx Engine (RXe) of TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../../../../toe/test/test_toe_utils.hpp"
#include "../../../../toe/src/rx_engine/src/rx_engine.hpp"
#include "../../../../toe/test/dummy_memory/dummy_memory.hpp"

#include <iostream>
#include <map>
#include <unistd.h>

using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_EVE    1 <<  1
#define TRACE_IPRX   1 <<  2
#define TRACE_PRT    1 <<  3
#define TRACE_RAI    1 <<  4
#define TRACE_RXMEM  1 <<  5
#define TRACE_RST    1 <<  6
#define TRACE_SLC    1 <<  7
#define TRACE_STT    1 <<  8
#define TRACE_TIM    1 <<  9
#define TRACE_TST    1 << 10
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//---------------------------------------------------------
#define MAX_SIM_CYCLES 2500000
#define TB_GRACE_TIME        0
#define STARTUP_DELAY        0  // This one might be required for C/RTL CoSim

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_FPGA_TCP_PORT   0x0057      // TOE listens on port     = 87 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_TCP_PORT   0x8000      // TB listens on port      = 32768 (dynamic ports must be 32768..65535)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  Some of these variables might be updated/overwritten
//--  by the content of a test-vector file.
//---------------------------------------------------------
unsigned int    gMaxSimCycles   = 100;
SockAddr        gLocalSocket(DEFAULT_FPGA_IP4_ADDR, DEFAULT_FPGA_TCP_PORT);
SockAddr        gForeignSocket(DEFAULT_HOST_IP4_ADDR, DEFAULT_HOST_TCP_PORT);
bool            gTraceEvent     = false;

static sessionState currentState = CLOSED;
static rxSarEntry   currRxEntry;
static txSarEntry   currTxEntry;


/*****************************************************************************
 * @brief Emulates the behavior of the Session Lookup Controller (SLc).
 *
 * @param[in]  siRXe_SessLookupReq, Session lookup request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep, Session lookup reply to RXe.
 *
 * @details
 *  The [SLc] maps a four-tuple information {{IP4_SA,TCP_SA},{IP4_DA,TCP_DP}} of
 *   a socket pair to a so-called 'SessionId'. This session ID represents the
 *   TCP connection and is used as an index into the various data structures of
 *   the TOE.
 *****************************************************************************/
void pEmulateSessionLookupController(
        stream<sessionLookupQuery>  &siRXe_SessLookupReq,
        stream<sessionLookupReply>  &soRXe_SessLookupRep)
{
    if (!siRXe_SessLookupReq.empty()) {
        siRXe_SessLookupReq.read();
        soRXe_SessLookupRep.write(sessionLookupReply(1, SESSION_EXISTS));
    }
}

/*****************************************************************************
 * @brief Emulates behavior of the Port Table (PRt).
 *
 * @param[in]  siRXe_PortStateReq, Request from the Rx Engine [RXe].
 * @param[out] soRxe_PortStateRep, Reply to [RXe].
 *
 * @details:
 *  The [PRt] keeps track of the TCP port numbers which are in use and which
 *  are therefore opened.
 ******************************************************************************/
void pEmulatePortTable(
        stream<AxiTcpPort>  &siRXe_PortStateReq,
        stream<RepBit>      &soRXe_PortStateRep)
{
    if (!siRXe_PortStateReq.empty()) {
        siRXe_PortStateReq.read();
        soRXe_PortStateRep.write(PORT_IS_OPENED);
    }
}

/*****************************************************************************
 * @brief Emulates behavior of the TCP State Table (STt).
 *
 * @param[in]  siRXe_SessStateReq, Request from the Rx Engine [RXe].
 * @param[out] soRXe_SessStateRep, Reply to [RXe].
 *
 * @details:
 *  The [STt] Stores the TCP connection state of each session.
 ******************************************************************************/
void pEmulateStateTable(
        stream<stateQuery>      &siRXe_SessStateReq,
        stream<sessionState>    &soRXe_SessStateRep)
{
    stateQuery query;
    if (!siRXe_SessStateReq.empty()) {
        siRXe_SessStateReq.read(query);
        if (query.write) {
            currentState = query.state;
        }
        else {
            soRXe_SessStateRep.write(currentState);
        }
    }
}

/*****************************************************************************
 * @brief Emulates the behavior of the Rx SAR Table (RSt).
 *
 * @param[in]  siRXe_RxSarUpdReq, Update request from Rx Engine (RXe).
 * @param[out] soRXe_RxSarUpdRep, Update reply to [RXe].
 *
 * @details
 *  The [RSt] stores the Rx sliding window information.
 *****************************************************************************/
void pEmulateRxSarTable(
        stream<rxSarRecvd>  &siRXe_RxSarUpdReq,
        stream<rxSarEntry>  &soRXe_RxSarUpdRep)
{
    rxSarRecvd query;
    if (!siRXe_RxSarUpdReq.empty()) {
        siRXe_RxSarUpdReq.read(query);
        if (query.write) {
            currRxEntry.recvd = query.recvd;
        }
        else {
            soRXe_RxSarUpdRep.write(currRxEntry);
        }
    }
}

/*****************************************************************************
 * @brief Emulates the behavior of the Tx SAR Table (TSt).
 *
 * @param[in]  siRXe_TxSarRdReq, Read request from Rx Engine (RXe).
 * @param[out] soRXe_TxSarRdRep, Read reply to [RXe].
 *
 * @details
 *  The [TSt] stores the Tx sliding window information.
 *****************************************************************************/
void pEmulateTxSarTable(
        stream<rxTxSarQuery>    &siRXe_TxSarRdReq,
        stream<rxTxSarReply>    &soRXe_TxSarRdRep)
{
    rxTxSarQuery query;

    if (!siRXe_TxSarRdReq.empty()) {
        siRXe_TxSarRdReq.read(query);
        if (query.write) {
            currTxEntry.ackd        = query.ackd;
            currTxEntry.recv_window = query.recv_window;
        }
        else {
            soRXe_TxSarRdRep.write(rxTxSarReply(currTxEntry.ackd,
                                                currTxEntry.ackd+100,
                                                0xFF00, 0x00FF, 0));
        }
    }
}

/*****************************************************************************
 * @brief Emulates the behavior of the Timers (TIm).
 *
 * @param[in]  siRXe_ReTxTimerCmd,  Retransmission timer command from [RxEngine].
 * @param[in]  siRXe_ClrProbeTimer, Clear probe timer from [RXe].
 * @param[in]  siRXe_CloseTimer,    Close timer from[RXe].
 *
 * @details
 *  The [TIm] contains the retransmit, probe and close timers.
 *****************************************************************************/
void pEmulateTimers(
        stream<ReTxTimerCmd>      &siRXe_ReTxTimerCmd)
{
    ReTxTimerCmd    timerCmd;
    const char *myName = concat3(THIS_NAME, "/", "TIm");

    if(!siRXe_ReTxTimerCmd.empty()) {
        siRXe_ReTxTimerCmd.read(timerCmd);
        if (DEBUG_LEVEL & TRACE_TIM) {
            printInfo(myName, "Received timer command from RXe: sessionID=%d | command=%d\n",
                              timerCmd.sessionID.to_uint(), timerCmd.command);
        }
    }

}

/*****************************************************************************
 * @brief Emulates the behavior of the Event Engine (EVe).
 *
 * @param[in]  siRXe_SetEvent,   Event from Rx Engine (RXe).
 *
 * @details
 *  The [EVe] arbitrates the incoming events and forwards them event to [TXe].
 *****************************************************************************/
void pEmulateEventEngine(
        stream<extendedEvent>   &siRXe_SetEvent)
{
    extendedEvent ev;

    const char *myName = concat3(THIS_NAME, "/", "EVe");

    if (!siRXe_SetEvent.empty()) {
        siRXe_SetEvent.read(ev);
        if (DEBUG_LEVEL & TRACE_EVE) {
            printInfo(myName, "Received event from RXe: sessionID=%d | EventType=%d\n",
                              ev.sessionID.to_uint(), ev.type);
        }
    }
}

/*****************************************************************************
 * @brief Emulates the behavior of the Rx Application Interface (RAi).
 *
 * @param[in]  siRXe_RxNotif, Notification from Rx Engine.
 *
 *****************************************************************************/
void pEmulateRxAppInterface(
        stream<AppNotif>    &siRXe_Notif)
{
    AppNotif    notif;

    const char *myName = concat3(THIS_NAME, "/", "RAi");

    if (!siRXe_Notif.empty()) {
    	siRXe_Notif.read(notif);
        if (DEBUG_LEVEL & TRACE_RAI) {
            printInfo(myName, "Received data notification from RXe: sessionID=%d | tcpSegLen=%d | ip4SrcAddr=0x%8.8X | tcpDstPort=%d\n",
                              notif.sessionID.to_uint(), notif.tcpSegLen.to_uint(),
                              notif.ip4SrcAddr.to_uint(), notif.tcpDstPort.to_uint());
        }
    }
}

/*****************************************************************************
 * @brief Emulate the behavior of the Receive DDR4 Buffer Memory (RXMEM).
 *
 * @param[in]  memWrFile,       A ref to the Rx buffer memory file to write.
 * @param[in]  rxMemCounterWr   A ref to the counter for TCP byte written in RxMem.
 * @param[in]  errCounter       A ref to the main error counter.
 * @param[in]  siTOE_RxP_WrCmd, A ref to the write command stream from TOE.
 * @param[out] soTOE_RxP_WrSts, A ref to the write status stream to TOE.
 * @param[in]  siTOE_RxP_RdCmd, A ref to the read command stream from TOE.
 * @param[in]  siTOE_RxP_Data,  A ref to the data stream from TOE.
 * @param[out] soTOE_RxP_Data,  A ref to the data stream to TOE.
 *
 * @details
 *
 * @ingroup toe
 ******************************************************************************/
void pEmulateRxBufMem(
        ofstream            &memWrFile,
        int                 &rxMemCounterWr,
        int                 &errCounter,
        stream<DmCmd>       &siTOE_RxP_WrCmd,
        stream<DmSts>       &soTOE_RxP_WrSts,
        stream<DmCmd>       &siTOE_RxP_RdCmd,
        stream<AxiWord>     &siTOE_RxP_Data,
        stream<AxiWord>     &soTOE_RxP_Data)
{
    const char *myName  = concat3(THIS_NAME, "/", "RXMEM");

    static DummyMemory  rxMemory;   // A dummy model of the DDR4 memory.
    static uint32_t     rxMemCounterRd = 0;
    static bool         stx_read       = false;
    static bool         stx_readCmd    = false;
    static int          noBytesToWrite = 0;
    static int          noBytesWritten = 0;
    static ap_uint<16>  noBytesToRead  = 0;
    static int          mwrIdle1       = 0;
    static int          mwrIdle2       = 0;

    static enum MwrFsmStates {MWR_CMD=0, MWR_DATA, MWR_STS} mwrFsmState;

    DummyMemory *memory = &rxMemory;

    static DmCmd dmCmd;     // Data Mover Command
    DmSts        dmSts;     // Data Mover Status
    AxiWord      tmpInWord  = AxiWord(0, 0, 0);
    AxiWord      inWord     = AxiWord(0, 0, 0);
    AxiWord      outWord    = AxiWord(0, 0, 0);
    AxiWord      tmpOutWord = AxiWord(0, 0, 0);

    //-----------------------------------------------------
    //-- MEMORY WRITE PROCESS
    //-----------------------------------------------------
    switch (mwrFsmState) {

    case MWR_CMD:
        if (!siTOE_RxP_WrCmd.empty()) {
            //-- Memory Write Command ---------------------
            siTOE_RxP_WrCmd.read(dmCmd);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                printInfo(myName, "Received memory write command from RXe: (addr=0x%x, bbt=%d).\n",
                                  dmCmd.saddr.to_uint64(), dmCmd.bbt.to_uint());
            }
            memory->setWriteCmd(dmCmd);
            noBytesToWrite = dmCmd.bbt.to_int();
            mwrIdle1    = 0;
            mwrFsmState = MWR_DATA;
        }
        break;

    case MWR_DATA:
        //-- Wait some cycles to match the co-simulation --
        if (mwrIdle1 > 0)
            mwrIdle1--;
        else if (!siTOE_RxP_Data.empty()) {
            //-- Data Memory Write Transfer ---------------
            siTOE_RxP_Data.read(tmpInWord);
            inWord = tmpInWord;
            memory->writeWord(inWord);
            rxMemCounterWr += writeTcpWordToFile(memWrFile, inWord);
            noBytesWritten += keepToLen(inWord.tkeep);
            if ((inWord.tlast) || (noBytesWritten == noBytesToWrite)) {
                mwrIdle2    = 2;
                mwrFsmState = MWR_STS;
            }
        }
        break;

    case MWR_STS:
        //-- Wait some cycles to match the co-simulation --
        if (mwrIdle2 > 0)
            mwrIdle2--;
        else if (!soTOE_RxP_WrSts.full()) {
            //-- Data Memory Write Status -----------------
            if (noBytesToWrite != noBytesWritten) {
                printError(myName, "The number of bytes received from RXe (%d) does not match the expected number specified by the command (%d)!\n",
                                   noBytesWritten, noBytesToWrite);
                errCounter++;
                dmSts.okay = 0;
            }
            else {
                dmSts.okay = 1;
            }
            soTOE_RxP_WrSts.write(dmSts);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                printInfo(myName, "Sending memory status back to RXe: (OK=%d).\n",
                                  dmSts.okay.to_int());
            }
            mwrFsmState = MWR_CMD;
        }
        break;

    } // End-of: switch

    //-----------------------------------------------------
    //-- MEMORY READ PROCESS
    //-----------------------------------------------------
    /***  rxBufferWriteData.read(outData);
          outputFile << std::hex << std::noshowbase;
          outputFile << std::setfill('0');
          outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
          outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
          outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
          outputFile << std::setw(1) << ((uint32_t) outData.last);
          outputFile << std::endl;
    ***/
    if (!siTOE_RxP_RdCmd.empty() && !stx_read) {
        // Memory Read Command
        siTOE_RxP_RdCmd.read(dmCmd);
        memory->setReadCmd(dmCmd);
        noBytesToRead = dmCmd.bbt;
        stx_read      = true;
    }
    else if(stx_read) {
        // Data Memory Read Transfer
        memory->readWord(outWord);
        tmpOutWord = outWord;
        soTOE_RxP_Data.write(tmpOutWord);
        cerr << dec << rxMemCounterRd << " - " << hex << outWord.tdata << " " << outWord.tkeep << " " << outWord.tlast << endl;
        rxMemCounterRd++;
        if (noBytesToRead < 9)
            stx_read = false;
        else
            noBytesToRead -= 8;
    }

} // End of: pEmulateRxBufMem

/*****************************************************************************
 * @brief Parse the input test file and set the global parameters of the TB.
 *
 * @param[in]  callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in]  inputFile,    A ref to the input file to parse.
 *
 * @details:
 *  A global parameter specifies a general property of the testbench such as
 *  the minimum number of simulation cycles, the default IP address of the TOE
 *  or the default port to listen to. Such a parameter is passed to the TB via
 *  the test vector file. The line containing such a parameter must start with
 *  the single upper character 'G' followed by a space character.
 *  Examples:
 *    G PARAM SimCycles     <NUM>
 *    G PARAM LocalSocket   <ADDR> <PORT>
 ******************************************************************************/
bool setGlobalParameters(const char *callerName, ifstream &inputFile)
{
    char myName[120];
    strcpy(myName, callerName);
    strcat(myName, "/setGlobalParameters");

    string              rxStringBuffer;
    vector<string>      stringVector;

    do {
        //-- READ ONE LINE AT A TIME FROM INPUT FILE ---------------
        getline(inputFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer, ' ');

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            // By convention, a global parameter must start with a single 'G' character.
            if ((stringVector[0] == "G") && (stringVector[1] == "PARAM")) {
                if (stringVector[2] == "SimCycles") {
                    // The test vector file is specifying a minimum number of simulation cycles.
                    int noSimCycles = atoi(stringVector[3].c_str());
                    noSimCycles += STARTUP_DELAY;
                    if (noSimCycles > gMaxSimCycles)
                        gMaxSimCycles = noSimCycles;
                    printInfo(myName, "Requesting the simulation to last for %d cycles. \n", gMaxSimCycles);
                }
                else if (stringVector[2] == "LocalSocket") {
                    char * ptr;
                    // Retrieve the IPv4 address to set
                    unsigned int ip4Addr;
                    if (isDottedDecimal(stringVector[3]))
                        ip4Addr = myDottedDecimalIpToUint32(stringVector[3]);
                    else if (isHexString(stringVector[3]))
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                    else
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 10);
                    gLocalSocket.addr = ip4Addr;
                    // Retrieve the TCP-Port to set
                    unsigned int tcpPort;
                    if (isHexString(stringVector[4]))
                        tcpPort = strtoul(stringVector[4].c_str(), &ptr, 16);
                    else
                        tcpPort = strtoul(stringVector[4].c_str(), &ptr, 10);
                    gLocalSocket.port = tcpPort;
                    printInfo(myName, "Redefining the default FPGA socket to be <0x%8.8X, 0x%4.4X>.\n", ip4Addr, tcpPort);
                }
                else {
                    printError(myName, "Unknown parameter \'%s\'.\n", stringVector[2].c_str());
                    return false;
                }
            }
            else
                continue;
        }
    } while(!inputFile.eof());

    printInfo(myName, "Done with the parsing of the input test file.\n\n");

    // Seek back to the start of stream
    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    return true;

} // End of: setGlopbalParameters

/*****************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *        number field of the current packet.
 *
 * @param[in]   ipRxPacket,  a ref to an IP packet.
 * @param[in]   sessionList, a ref to an associative container which holds
 *                            the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 ******************************************************************************/
int pIPRX_InjectAckNumber(
        IpPacket                       &ipRxPacket,
        map<SocketPair, TcpAckNum>     &sessionList)
{
    const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");

    SockAddr   srcSock = SockAddr(ipRxPacket.getIpSourceAddress(),
                                  ipRxPacket.getTcpSourcePort());
    SockAddr   dstSock = SockAddr(ipRxPacket.getIpDestinationAddress(),
                                  ipRxPacket.getTcpDestinationPort());
    SocketPair newSockPair = SocketPair(srcSock, dstSock);

    if (ipRxPacket.isSYN()) {

        // This packet is a SYN and there's no need to inject anything
        if (sessionList.find(newSockPair) != sessionList.end()) {
            printWarn(myName, "Trying to open an existing session (%d)!\n", (sessionList.find(newSockPair)->second).to_uint());
            printSockPair(myName, newSockPair);
            return -1;
        }
        else {
            // Let's check the pseudo-header checksum of the packet
            int computedCsum = ipRxPacket.recalculateChecksum();
            int embeddedCsum = ipRxPacket.getTcpChecksum();
            if (computedCsum != embeddedCsum) {
                printError(myName, "WRONG PSEUDO-HEADER CHECKSUM (0x%4.4X) - Expected 0x%4.4X \n",
                           embeddedCsum, byteSwap16(computedCsum).to_uint());
                return -1;
            }
            else {
                sessionList[newSockPair] = 0;
                printInfo(myName, "Successfully opened a new session (%d).\n", (sessionList.find(newSockPair)->second).to_uint());
                printSockPair(myName, newSockPair);
                return 0;
            }
        }
    }
    else if (ipRxPacket.isACK()) {
        // This packet is an ACK and we must update the its acknowledgment number
        if (sessionList.find(newSockPair) != sessionList.end()) {
            // Inject the oldest acknowledgment number in the ACK number field
            TcpAckNum newAckNum = sessionList[newSockPair];
            ipRxPacket.setTcpAcknowledgeNumber(newAckNum);

            if (DEBUG_LEVEL & TRACE_IPRX)
                printInfo(myName, "Setting the TCP Acknowledge of this segment to: %u (0x%8.8X) \n",
                          newAckNum.to_uint(), byteSwap32(newAckNum).to_uint());


            // Recalculate and update the checksum
            int oldCsum = ipRxPacket.getTcpChecksum();
            int newCsum = ipRxPacket.recalculateChecksum();
            if (DEBUG_LEVEL & TRACE_IPRX)
                printInfo(myName, "Updating the checksum of this packet from 0x%4.4X to 0x%4.4X\n",
                          oldCsum, newCsum);

            if (DEBUG_LEVEL & TRACE_IPRX) {
                ipRxPacket.printRaw(myName);
            }
            return 1;
        }
        else {
            printWarn(myName, "Trying to send data to a non-existing session! \n");
            return -1;
        }
    }
    return -1;
} // End of: injectAckNumber()

/*****************************************************************************
 * @brief Feed TOE with IP an Rx packet.
 *
 * @param[in]  ipRxPacketizer, a ref to the deque w/ an IP Rx packets.
 * @param[i/o] ipRxPktCounter, a ref to the IP Rx packet counter.
 *                              (counts all kinds and from all sessions).
 * @param[out] soTOE_Data,     A reference to the data stream to TOE.
 * @param[in]  sessionList,    a ref to an associative container that
 *                              holds the sessions as socket pair associations.
 *
 * @details:
 *  Empties the double-ended packetizer queue which contains the IPv4 packet
 *  chunks intended for the IPRX interface of the Rxe. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX_FeedTOE(
        deque<IpPacket>             &ipRxPacketizer,
        int                         &ipRxPktCounter,
        stream<Ip4overAxi>          &soTOE_Data,
        map<SocketPair, TcpAckNum>  &sessionList)
{
    const char *myName = concat3(THIS_NAME, "/", "IPRX/FeedToe");

    if (ipRxPacketizer.size() != 0) {
        // Insert proper ACK Number in packet at the head of the queue
        pIPRX_InjectAckNumber(ipRxPacketizer[0], sessionList);
        if (DEBUG_LEVEL & TRACE_IPRX) {
            ipRxPacketizer[0].printHdr(myName);
        }
        // Write stream IPRX->TOE
        int noPackets= ipRxPacketizer.size();
        for (int p=0; p<noPackets; p++) {
            IpPacket ipRxPacket = ipRxPacketizer.front();
            Ip4overAxi axiWord;
            do {
                axiWord = ipRxPacket.front();
                soTOE_Data.write(axiWord);
                ipRxPacket.pop_front();
            } while (!axiWord.tlast);
            ipRxPktCounter++;
            ipRxPacketizer.pop_front();
        }
    }
}

/*****************************************************************************
 * @brief Emulate the behavior of the IP Rx Path (IPRX).
 *
 * @param[in]  ipRxFile,       A ref to the input file w/ IP Rx packets.
 * @param[in]  memTxGold,      A ref to the [TODO]
 * @param[i/o] ipRxPktCounter, A ref to the IP Rx packet counter.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipRx_TcpBytCntr,A ref to the counter of TCP data bytes received
 *                               on the IP Rx interface.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipRxPacketizer, A ref to the RxPacketizer (double-ended queue).
 * @param[in]  sessionList,    A ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out] soTOE_Data,     A reference to the data stream to TOE.
 *
 * @details
 *  Reads in new IPv4 packets from the Rx input file and stores them into the
 *   the IPv4 RxPacketizer (ipRxPacketizer). This ipRxPacketizer is a
 *   double-ended queue that is also fed by the process 'pEmulateL3Mux' when
 *   it wants to generate ACK packets.
 *  If packets are stored in the 'ipRxPacketizer', they will be forwarded to
 *   the TOE over the 'sIRPX_Toe_Data' stream at the pace of one chunk per
 *   clock cycle.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX(
        ifstream                    &ipRxFile,
        ofstream                    &memWrGold,
        int                         &ipRxPktCounter,
        int                         &ipRx_TcpBytCntr,
        deque<IpPacket>             &ipRxPacketizer,
        map<SocketPair, TcpAckNum>  &sessionList,
        stream<Ip4overAxi>          &soTOE_Data)
{
    static bool         globParseDone  = false;
    static bool         ipRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int ipRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int ipRxIdleCycCnt = 0;     // The count of idle cycles

    // Keep track of the current active local socket
     static SockAddr    currLocalSocket(DEFAULT_FPGA_IP4_ADDR,
                                        DEFAULT_FPGA_TCP_PORT);

    string              rxStringBuffer;
    vector<string>      stringVector;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    //-------------------------------------------------------------------------
    //-- STEP-0: PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(myName, ipRxFile);
        if (globParseDone == false) {
            printInfo(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-1 : RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (ipRxIdlingReq == true) {
        if (ipRxIdleCycCnt >= ipRxIdleCycReq) {
            ipRxIdleCycCnt = 0;
            ipRxIdlingReq = false;
        }
        else {
            ipRxIdleCycCnt++;
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : ALWAYS FEED the IP Rx INTERFACE
    //-----------------------------------------------------
    // Note: The IPv4 RxPacketizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, L3Mux).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the IP Rx input file.
    pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessionList);

    //-------------------------------------------------------------------------
    //-- STEP-3: QUIT HERE IF EOF IS REACHED
    //-------------------------------------------------------------------------
    if (ipRxFile.eof())
        return;

    //-----------------------------------------------------
    //-- STEP-4 : READ THE IP RX FILE
    //-----------------------------------------------------
    do {
        //-- READ A LINE FROM IPRX INPUT FILE -------------
        getline(ipRxFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer, ' ');

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            //------------------------------------------------------
            //-- Process the command and comment lines
            //--  FYI: A command or a comment start with a single
            //--       character followed by a space character.
            //------------------------------------------------------
            if (stringVector[0] == "#") {
                // This is a comment line.
                for (int t=0; t<stringVector.size(); t++)
                    printf("%s ", stringVector[t].c_str());
                printf("\n");
                continue;
            }
            else if (stringVector[0] == "G") {
                // We can skip all the global parameters as they were already parsed.
                continue;
            }
            else if (stringVector[0] == ">") {
                // The test vector is issuing a command
                //  FYI, don't forget to return at the end of command execution.
                if (stringVector[1] == "IDLE") {
                    // Cmd = Request to idle for <NUM> cycles.
                    ipRxIdleCycReq = atoi(stringVector[2].c_str());
                    ipRxIdlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", ipRxIdleCycReq);
                    return;
                }
            }
            else {
                printError(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[1].c_str());
                exit(1);
            }
        }
        else if (ipRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            // Build a new packet from data file
            IpPacket   ipRxPacket;
            Ip4overAxi ipRxData;
            bool       firstWordFlag = true; // AXI-word is first chunk of packet

            do {
                if (firstWordFlag == false) {
                    getline(ipRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer, ' ');
                }
                if (stringVector[0] == "#") {
                    // This is a comment line.
                    for (int t=0; t<stringVector.size(); t++) {
                        printf("%s ", stringVector[t].c_str());
                        printf("\n");
                    }
                    continue;
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                ipRxData = Ip4overAxi(myStrHexToUint64(stringVector[0]), \
                                      myStrHexToUint8(stringVector[2]),  \
                                      atoi(stringVector[1].c_str()));
                ipRxPacket.push_back(ipRxData);
            } while (ipRxData.tlast != 1);

            // Count the number of data bytes contained in the TCP payload
            ipRx_TcpBytCntr += ipRxPacket.sizeOfTcpData();

            // Write to the RxBufferMemory Gold file
            writeTcpDataToFile(memWrGold, ipRxPacket);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessionList); // [FIXME-Can be removed?]

            return;
        }

    } while(!ipRxFile.eof());

} // End of: pIPRX


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  inpFile, the pathname of the input file containing the test
 *                      vectors to be fed to the RXe.
 *
 * @remark:
 *  The input test vector files are stores in ./testVectors.
 *   Example (see also file '../run_hls.tcl'):
 *    csim_design -argv "../../../../test/testVectors/ipRx_OneSynPkt.dat"
 *
 * @ingroup test_rx_engine
 ******************************************************************************/
int main(int argc, char* argv[]) {

    //-----------------------------------------------------
    //-- DUT INTERFACE SIGNALS
    //-----------------------------------------------------
    stream<Ip4overAxi>              sIPRX_RXe_Pkt           ("sIPRX_RXe_Pkt");
    stream<sessionLookupQuery>      sRXe_SLc_SessLookupReq  ("sRXe_SLc_SessLookupReq");
    stream<sessionLookupReply>      sSLc_RXe_SessLookupRep  ("sSLc_RXe_SessLookupRep");
    stream<stateQuery>              sRXe_STt_SessStateReq   ("sRXe_STt_SessStateReq");
    stream<sessionState>            sSTt_RXe_SessStateRep   ("sSTt_RXe_SessStateRep");
    stream<AxiTcpPort>              sRXe_PRt_GetPortState   ("sRXe_PRt_GetPortState");
    stream<StsBit>                  sPRt_RXe_PortSts        ("sPRt_RXe_PortSts");
    stream<rxSarRecvd>              sRXe_RSt_RxSarUpdReq    ("sRXe_RSt_RxSarUpdReq");
    stream<rxSarEntry>              sRSt_RXe_RxSarUpdRep    ("sRSt_RXe_RxSarUpdRep");
    stream<rxTxSarQuery>            sRXe_TSt_TxSarRdReq     ("sRXe_TSt_TxSarRdReq");
    stream<rxTxSarReply>            sTSt_RXe_TxSarRdRep     ("sTSt_RXe_TxSarRdRep");
    stream<ReTxTimerCmd>            sRXe_TIm_ReTxTimerCmd   ("sRXe_TIm_ReTxTimerCmd");
    stream<ap_uint<16> >            sRXe_TIm_ClearProbeTimer("sRXe_TIm_ClearProbeTimer");
    stream<ap_uint<16> >            sRXe_TIm_CloseTimer     ("sRXe_TIm_CloseTimer");
    stream<extendedEvent>           sRXe_EVe_SetEvent       ("sRXe_EVe_SetEvent");
    stream<OpenStatus>              sRXe_TAi_SessOpnSts     ("sRXe_TAi_SessOpnSts");
    stream<AppNotif>                sRXe_RAi_RxNotif        ("sRXe_RAi_RxNotif");
    stream<DmCmd>                   sRXe_MEM_WrCmd          ("sRXe_MEM_WrCmd");
    stream<AxiWord>                 sRXe_MEM_WrData         ("sRXe_MEM_WrData");
    stream<DmSts>                   sMEM_RXe_WrSts          ("sMEM_RXe_WrSts");

    // Unused memory streams
    stream<DmCmd>                   sRXe_MEM_RdCmd;
    stream<AxiWord>                 sMEM_RXe_RdData;

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    static ap_uint<32> simCycCnt = 0;
    int             nrErr;

    Ip4Word         ipRxData;    // An IP4 chunk
    AxiWord         tcpTxData;   // A  TCP chunk

    // [TODO] ap_uint<16>     opnSessionCount;
    // [TODO] ap_uint<16>     clsSessionCount;

    map<SocketPair, TcpAckNum>  sessionList;

    //-- Double-ended queue of packets --------------------
    deque<IpPacket>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        ipRxFile;   // IP4 packets      from [IPRX] to [RXe].
    ofstream        memWrFile;  // MEM byte streams from [RXe]  to [MEM].
    ofstream        memWrGold;  // Gold reference file for 'memWrFile'

    const char      *memWrFileName = "../../../../test/memWr_RXe.strm";
    const char      *memWrGoldName = "../../../../test/memWr_RXe.gold";

    string          dataString, keepString;

    int             ipRx_PktCounter  = 0; // Counts the # IP packets rcvd by RXe/IpRx (all kinds and from all sessions).

    int             ipRx_TcpBytCntr  = 0; // Counts the # TCP bytes received by RXe/IpRx.
    int             memWr_TcpBytCntr = 0; // Counts the # TCP bytes written  by RXe/MemWr.

    char            cCurrPath[FILENAME_MAX];


    printf("\n\n\n\n");
    printf("############################################################################\n");
    printf("## TESTBENCH STARTS HERE                                                  ##\n");
    printf("############################################################################\n");
    simCycCnt = 0;     // Simulation cycle counter as a global variable
    nrErr     = 0;     // Total number of testbench errors

    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 2) {
        printError(THIS_NAME, "Missing argument. Expecting the path name of the Rx test vectors.\n");
        return -1;
    }
    else {
        ipRxFile.open(argv[1]);
        if (!ipRxFile) {
            printError(THIS_NAME, "Cannot open the IP Rx file: \n\t %s \n", argv[1]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printInfo(THIS_NAME, "\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        printInfo(THIS_NAME, "This run executes with IP Rx file  : %s.\n", argv[1]);

        memWrFile.open(memWrFileName);
        if (!memWrFile) {
            printError(THIS_NAME, "Cannot open the Rx Memory Write file:  \n\t %s \n", memWrFileName);
            return -1;
        }

        memWrGold.open(memWrGoldName);
        if (!memWrGold) {
            printInfo(THIS_NAME, "Cannot open the Rx Memory Write gold file:  \n\t %s \n", memWrGoldName);
            return -1;
        }
    }

    printInfo(THIS_NAME, "This run executes with a STARTUP_DELAY = %d \n", STARTUP_DELAY);

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- STEP-1 : Emulate the IPv4 Rx Path
        //-------------------------------------------------
        if (simCycCnt >= STARTUP_DELAY) {
            pIPRX(ipRxFile,        memWrGold,
                  ipRx_PktCounter, ipRx_TcpBytCntr,
                  ipRxPacketizer,  sessionList, sIPRX_RXe_Pkt);
        }

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        if (simCycCnt >= TB_GRACE_TIME) {
            // Give the TB some grace time to do some of the required initializations
            rx_engine(
                sIPRX_RXe_Pkt,
                sRXe_SLc_SessLookupReq,
                sSLc_RXe_SessLookupRep,
                sRXe_STt_SessStateReq,
                sSTt_RXe_SessStateRep,
                sRXe_PRt_GetPortState,
                sPRt_RXe_PortSts,
                sRXe_RSt_RxSarUpdReq,
                sRSt_RXe_RxSarUpdRep,
                sRXe_TSt_TxSarRdReq,
                sTSt_RXe_TxSarRdRep,
                sRXe_TIm_ReTxTimerCmd,
                sRXe_TIm_ClearProbeTimer,
                sRXe_TIm_CloseTimer,
                sRXe_EVe_SetEvent,
                sRXe_TAi_SessOpnSts,
                sRXe_RAi_RxNotif,
                sRXe_MEM_WrCmd,
                sRXe_MEM_WrData,
                sMEM_RXe_WrSts);
        }

        //-------------------------------------------------
        //-- STEP-3 : Emulate EVe,PRt,RAi,SLc,STt,RSt,TSt
        //-------------------------------------------------
        if (simCycCnt >= STARTUP_DELAY) {
            pEmulateEventEngine(
                sRXe_EVe_SetEvent);
            pEmulatePortTable(
                sRXe_PRt_GetPortState,
                sPRt_RXe_PortSts);
            pEmulateRxAppInterface(
                sRXe_RAi_RxNotif);
            pEmulateSessionLookupController(
                sRXe_SLc_SessLookupReq,
                sSLc_RXe_SessLookupRep);
            pEmulateStateTable(
                sRXe_STt_SessStateReq,
                sSTt_RXe_SessStateRep);
            pEmulateRxSarTable(
                sRXe_RSt_RxSarUpdReq,
                sRSt_RXe_RxSarUpdRep);
            pEmulateTimers(
                sRXe_TIm_ReTxTimerCmd);
            pEmulateTxSarTable(
                sRXe_TSt_TxSarRdReq,
                sTSt_RXe_TxSarRdRep);

        }

        //-------------------------------------------------
        //-- STEP-4 : Emulate the MEM Process
        //-------------------------------------------------
        if (simCycCnt >= STARTUP_DELAY) {
            pEmulateRxBufMem(
                memWrFile,
                memWr_TcpBytCntr,
                nrErr,
                sRXe_MEM_WrCmd,
                sMEM_RXe_WrSts,
                sRXe_MEM_RdCmd,
                sRXe_MEM_WrData,
                sMEM_RXe_RdData);
        }

        //------------------------------------------------------
        //-- STEP-5 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        if (gTraceEvent || ((simCycCnt % 25) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", simCycCnt.to_uint());
            gTraceEvent = false;
        }
        simCycCnt++;

    } while (simCycCnt < gMaxSimCycles);

    printf("-- [@%4.4d] -----------------------------\n", simCycCnt.to_uint());
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n");

    //---------------------------------------------------------------
    //-- PRINT AN OVERALL TESTBENCH STATUS
    //---------------------------------------------------------------
    //TODO printInfo(THIS_NAME, "Number of sessions opened by TOE         : %6d \n", opnSessionCount.to_uint());
    //TODO printInfo(THIS_NAME, "Number of sessions closed by TOE         : %6d \n", clsSessionCount.to_uint());

    printInfo(THIS_NAME, "Number of IP  Packets rcvd by RXe/IpRx   : %6d \n", ipRx_PktCounter);

    printInfo(THIS_NAME, "Number of TCP Bytes received by RXe/IpRx : %6d \n", ipRx_TcpBytCntr);
    printInfo(THIS_NAME, "Number of TCP Bytes written  by RXe/MemWr: %6d \n", memWr_TcpBytCntr);

    printf("\n");
    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------
    if (ipRx_TcpBytCntr != memWr_TcpBytCntr) {
        printError(THIS_NAME, "The number of TCP bytes received by RXe on its IP interface (%d) does not match the number TCP bytes written by RXe to the Rx Buffer Memory (%d). \n", ipRx_TcpBytCntr, memWr_TcpBytCntr);
        nrErr++;
     }

     int memWrCompare = system(("diff --brief -w " + std::string(memWrFileName) + " " + std::string(memWrGoldName) + " ").c_str());
     if (memWrCompare != 0) {
         printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", memWrFileName, memWrGoldName);
         nrErr++;
     }

     // [TODO] if ((opnSessionCount == 0) && (ipRx_PktCounter > 0)) {
     // [TODO]     printWarn(THIS_NAME, "No session was opened by the TOE during this run. Please double check!!!\n");
     // [TODO]     nrErr++;
     // [TODO] }

     // Close the files
     ipRxFile.close();
     memWrFile.close();
     memWrGold.close();

       if (nrErr) {
           printError(THIS_NAME, "###########################################################\n");
           printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
           printError(THIS_NAME, "###########################################################\n");
       }
           else {
           printInfo(THIS_NAME, "#############################################################\n");
           printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
           printInfo(THIS_NAME, "#############################################################\n");
       }

       return nrErr;
}
