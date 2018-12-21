/*****************************************************************************
 * @file       : test_toe.cpp
 * @brief      : Testbench for the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "./test_toe.hpp"
#include "../src/toe.hpp"
#include "../src/dummy_memory/dummy_memory.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"

#include <map>
#include <string>
#include <unistd.h>

using namespace std;


//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_IPRX   1 <<  1
#define TRACE_L3MUX  1 <<  2
#define TRACE_TRIF   1 <<  3
#define TRACE_CAM    1 <<  4
#define TRACE_TXMEM  1 <<  5
#define TRACE_RXMEM  1 <<  6
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//---------------------------------------------------------
#define MAX_SIM_CYCLES 2500000

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
unsigned int    gMaxSimCycles = 100;    // Might be updated by content of the test vector file.
bool            gTraceEvent   = false;


/*****************************************************************************
 * @brief Print the socket pair association of a data segment.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,     the socket pair to display.
 *****************************************************************************/
void printTbSockPair(const char *callerName, TbSocketPair sockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        sockPair.src.addr, sockPair.src.port,
        sockPair.dst.addr, sockPair.dst.port);
}

/*****************************************************************************
 * @brief Converts an UINT64 into a string of 16 HEX characters.
 *
 * @param[in]   inputNumber, the UINT64 to convert.
 *
 * @ingroup test_toe
 ******************************************************************************/
string decodeApUint64(
        ap_uint<64> inputNumber)
{
    string                    outputString    = "0000000000000000";
    unsigned short int        tempValue       = 16;
    static const char* const  lut             = "0123456789ABCDEF";

    for (int i = 15;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0;k<4;++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[15-i] = lut[tempValue];
    }
    return outputString;
}

/*****************************************************************************
 * @brief Converts an UINT8 into a string of 2 HEX characters.
 *
 * @param[in]   inputNumber, the UINT8 to convert.
 *
 * @ingroup test_toe
 ******************************************************************************/
string decodeApUint8(
        ap_uint<8> inputNumber)
{
    string                      outputString    = "00";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";

    for (int i = 1;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0; k<4; ++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[1-i] = lut[tempValue];
    }
    return outputString;
}

/*****************************************************************************
 * @brief Converts a string of 16 HEX characters into an UINT64.
 *
 * @param[in]   inputNumber, the string to convert.
 *
 * @ingroup test_toe
 ******************************************************************************/
ap_uint<64> encodeApUint64(
        string dataString)
{
    ap_uint<64> tempOutput          = 0;
    unsigned short int  tempValue   = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<dataString.size(); ++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == dataString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3; k>=0; --k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(63-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}

/*****************************************************************************
 * @brief Converts a string of 2 HEX characters into an UINT8.
 *
 * @param[in]   inputNumber, the string to convert.
 *
 * @ingroup test_toe
 ******************************************************************************/
ap_uint<8> encodeApUint8(
        string keepString)
{
    ap_uint<8>               tempOutput = 0;
    unsigned short int       tempValue  = 16;
    static const char* const lut        = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<2;++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == keepString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(7-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}


/*****************************************************************************
 * @brief Emulate the behavior of the Content Addressable Memory (CAM).
 *
 * @param[TODO]
 *
 * @details
 *
 * @ingroup test_toe
 ******************************************************************************/
void pEmulateCam(
        stream<rtlSessionLookupRequest>  &lup_req,
        stream<rtlSessionLookupReply>    &lup_rsp,
        stream<rtlSessionUpdateRequest>  &upd_req,
        stream<rtlSessionUpdateReply>    &upd_rsp)
{
    //stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
    static map<fourTupleInternal, ap_uint<14> > lookupTable;

    rtlSessionLookupRequest request;
    rtlSessionUpdateRequest update;

    map<fourTupleInternal, ap_uint<14> >::const_iterator findPos;

    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    if (!lup_req.empty()) {
        lup_req.read(request);
        findPos = lookupTable.find(request.key);
        if (findPos != lookupTable.end()) //hit
            lup_rsp.write(rtlSessionLookupReply(true, findPos->second, request.source));
        else
            lup_rsp.write(rtlSessionLookupReply(false, request.source));
    }

    if (!upd_req.empty()) {     //TODO what if element does not exist
        upd_req.read(update);
        if (update.op == INSERT) {  //Is there a check if it already exists?
            // Read free id
            //new_id.read(update.value);
            lookupTable[update.key] = update.value;
            upd_rsp.write(rtlSessionUpdateReply(update.value, INSERT, update.source));

        }
        else {  // DELETE
            //fin_id.write(update.value);
            lookupTable.erase(update.key);
            upd_rsp.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
        }
    }
}

/*****************************************************************************
 * @brief Emulate the behavior of the Receive DDR4 Buffer Memory (RXMEM).
 *
 * @param[in/out] *memory,         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     siTOE_RxP_WrCmd, A ref to the write command stream from TOE.
 * @param[out]    soTOE_RxP_WrSts, A ref to the write status stream to TOE.
 * @param[in]     siTOE_RxP_RdCmd, A ref to the read command stream from TOE.
 * @param[in]     siTOE_RxP_Data,  A ref to the data stream from TOE.
 * @param[out]    soTOE_RxP_Data,  A ref to the data stream to TOE.
 *
 * @details
 *
 * @ingroup toe
 ******************************************************************************/
void pEmulateRxBufMem(
        DummyMemory         *memory,
        stream<DmCmd>       &siTOE_RxP_WrCmd,
        stream<DmSts>       &soTOE_RxP_WrSts,
        stream<DmCmd>       &siTOE_RxP_RdCmd,
        stream<AxiWord>     &siTOE_RxP_Data,
        stream<AxiWord>     &soTOE_RxP_Data)
{
    DmCmd    dmCmd;     // Data Mover Command
    DmSts    dmSts;     // Data Mover Status

    AxiWord  tmpInWord  = AxiWord(0, 0, 0);
    AxiWord  inWord     = AxiWord(0, 0, 0);
    AxiWord  outWord    = AxiWord(0, 0, 0);
    AxiWord  tmpOutWord = AxiWord(0, 0, 0);

    //OBSOLETE static uint32_t rxMemCounter   = 0;
    static uint32_t rxMemCounterRd = 0;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool         stx_readCmd = false;
    static ap_uint<16>  noBytesToWrite = 0;
    static ap_uint<16>  noBytesToRead  = 0;

    const char *myName  = concat3(THIS_NAME, "/", "RXMEM");

    if (!siTOE_RxP_WrCmd.empty() && !stx_write) {
        // Memory Write Command
        siTOE_RxP_WrCmd.read(dmCmd);
        memory->setWriteCmd(dmCmd);
        noBytesToWrite = dmCmd.bbt;
        stx_write = true;
    }
    else if (!siTOE_RxP_Data.empty() && stx_write) {
        // Data Memory Write Transfer
        siTOE_RxP_Data.read(tmpInWord);
        inWord = tmpInWord;
        //cerr << dec << rxMemCounter << " - " << hex << inWord.data << " " << inWord.keep << " " << inWord.last << endl;
        //rxMemCounter++;;
        memory->writeWord(inWord);
        if (noBytesToWrite < 9) {
            // We are done
            stx_write  = false;
            dmSts.okay = 1;
            soTOE_RxP_WrSts.write(dmSts);
        }
        else
            noBytesToWrite -= 8;
    }

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
        //cerr << dec << rxMemCounterRd << " - " << hex << outWord.data << " " << outWord.keep << " " << outWord.last << endl;
        rxMemCounterRd++;
        if (noBytesToRead < 9)
            stx_read = false;
        else
            noBytesToRead -= 8;
    }

} // End of: pEmulateRxBufMem


/*****************************************************************************
 * @brief Emulate the behavior of the Transmit DDR4 Buffer Memory (TXMEM).
 *
 * @param[in/out] *memory,         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     siTOE_TxP_WrCmd, A ref to the write command stream from TOE.
 * @param[out]    soTOE_TxP_WrSts, A ref to the write status stream to TOE.
 * @param[in]     siTOE_TxP_RdCmd, A ref to the read command stream from TOE.
 * @param[in]     siTOE_TxP_Data,  A ref to the data stream from TOE.
 * @param[out]    soTOE_TxP_Data,  A ref to the data stream to TOE.
 *
 * @details
 *
 * @ingroup toe
 ******************************************************************************/
void pEmulateTxBufMem(
        DummyMemory         *memory,
        stream<DmCmd>       &siTOE_TxP_WrCmd,
        stream<DmSts>       &soTOE_TxP_WrSts,
        stream<DmCmd>       &siTOE_TxP_RdCmd,
        stream<AxiWord>     &siTOE_TxP_Data,
        stream<AxiWord>     &soTOE_TxP_Data)
{
    DmCmd    dmCmd;     // Data Mover Command
    DmSts    dmSts;     // Data Mover Status
    AxiWord  inWord;
    AxiWord  outWord;

    static bool stx_write = false;
    static bool stx_read  = false;

    static bool stx_readCmd = false;

    const char *myName  = concat3(THIS_NAME, "/", "TXMEM");

    if (!siTOE_TxP_WrCmd.empty() && !stx_write) {
        // Memory Write Command
        siTOE_TxP_WrCmd.read(dmCmd);
        memory->setWriteCmd(dmCmd);
        stx_write = true;
    }
    else if (!siTOE_TxP_Data.empty() && stx_write) {
        // Data Memory Write Transfer
        siTOE_TxP_Data.read(inWord);
        memory->writeWord(inWord);
        if (inWord.tlast) {
            stx_write = false;
            dmSts.okay = 1;
            soTOE_TxP_WrSts.write(dmSts);
        }
    }

    if (!siTOE_TxP_RdCmd.empty() && !stx_read) {
        // Memory Read Command
        siTOE_TxP_RdCmd.read(dmCmd);
        memory->setReadCmd(dmCmd);
        stx_read = true;
    }
    else if(stx_read) {
        // Data Memory Read Transfer
        memory->readWord(outWord);
        soTOE_TxP_Data.write(outWord);
        if (outWord.tlast)
            stx_read = false;
    }
} // End of: pEmulateTxBufMem


/*****************************************************************************
 * @brief Brakes a string into tokens by using the 'space' delimiter.
 *
 * @param[in]  stringBuffer, the string to tokenize.
 * @return a vector of strings.
 *
 * @ingroup toe
 ******************************************************************************/
vector<string> myTokenizer(string strBuff) {
    vector<string>   tmpBuff;
    bool             found = false;

    if (strBuff.empty()) {
        tmpBuff.push_back(strBuff);
        return tmpBuff;
    }
    else {
        // Substitute the "\r" with nothing
        if (strBuff[strBuff.size() - 1] == '\r')
            strBuff.erase(strBuff.size() - 1);
    }

    // Search for spaces delimiting the different data words
    while (strBuff.find(" ") != string::npos) {
        // while (!string::npos) {
        //if (strBuff.find(" ")) {
            // Split the string in two parts
            string temp  = strBuff.substr(0, strBuff.find(" "));
            strBuff = strBuff.substr(strBuff.find(" ")+1,
                                       strBuff.length());
            // Store the new part into the vector.
            tmpBuff.push_back(temp);
        //}
        // Continue searching until no more spaces are found.
    }

    // Push the final part of the string into the vector when no more spaces are present.
    tmpBuff.push_back(strBuff);
    return tmpBuff;
}

/*****************************************************************************
 * @brief Write the TCP data part of an IP packet into the Tx Application Gold
 *         file. This file will latter be compared with the 'TxAppFile'.
 *
 * @param[in]     appTxGold,  a ref to the gold file to write.
 * @param[in]     ipRxPacket, a ref to an IP RX packet.
 *
 * @ingroup test_toe
 ******************************************************************************/
void writeAppTxGoldFile(
        ofstream    &appTxGold,
        IpPacket    &ipPacket)
{
    if(ipPacket.sizeOfTcpData() > 0) {
        string tcpData = ipPacket.getTcpData();
        if (tcpData.size() > 0)
            appTxGold << tcpData << endl;
    }
}


/************
string tdataToFile = decodeApUint64(tcpWord.tdata);
string tkeepToFile = decodeApUint8 (tcpWord.tkeep);

for (int i = 0; i<8; ++i) {
    // Delete the data not to be kept by "keep" - for Golden comparison
    if(tcpWord.tkeep[7-i] == 0) {
        tdataToFile.replace(i*2, 2, "00");
    }
    appTxBytCounter++;
}

if (tcpWord.tlast == 1)
    appTxFile << tdataToFile << endl;
else
    appTxFile << tdataToFile;
***************/


/*****************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *            number field of the current packet.
 *
 * @param[in]   ipRxPacket,  a ref to an IP packet.
 * @param[in]   sessionList, a ref to an associative container which holds
 *                            the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 *
 * @ingroup toe
 ******************************************************************************/
int injectAckNumber(
        IpPacket                         &ipRxPacket,
        map<TbSocketPair, TcpAckNum>     &sessionList)
{
    const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");

    TbSockAddr  srcSock = TbSockAddr(ipRxPacket.getIpSourceAddress(),
                                     ipRxPacket.getTcpSourcePort());
    TbSockAddr  dstSock = TbSockAddr(ipRxPacket.getIpDestinationAddress(),
                                     ipRxPacket.getTcpDestinationPort());
    TbSocketPair newSockPair = TbSocketPair(srcSock, dstSock);

    if (ipRxPacket.isSYN()) {
        // This packet is a SYN and there's no need to inject anything
        if (sessionList.find(newSockPair) != sessionList.end()) {
            printWarn(myName, "Trying to open an existing session (%d)!\n", (sessionList.find(newSockPair)->second).to_uint());
            printTbSockPair(myName, newSockPair);
            return -1;
        }
        else {
            sessionList[newSockPair] = 0;
            printInfo(myName, "Successfully opened a new session (%d).\n", (sessionList.find(newSockPair)->second).to_uint());
            printTbSockPair(myName, newSockPair);
            return 0;
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
            int newTcpCsum = ipRxPacket.recalculateChecksum();
            ipRxPacket.setTcpChecksum(newTcpCsum);

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
 * @param[in]  ipRxPacketizer,    a ref to the dqueue w/ an IP Rx packets.
 * @param[in/out] ipRxPktCounter, a ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
 * @param[out] soTOE_Data,        A reference to the data stream to TOE.
 * @param[in]  sessionList,       a ref to an associative container that
 *                                 holds the sessions as socket pair associations.
 *
 * @details:
 *  Empties the double-ended packetizer queue which contains the IPv4 packet
 *  chunks intended for the IPRX interface of the TOE. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *
 * @ingroup toe
 ******************************************************************************/
void feedTOE(
        deque<IpPacket>               &ipRxPacketizer,
        int                           &ipRxPktCounter,
        stream<Ip4overAxi>            &soTOE_Data,
        map<TbSocketPair, TcpAckNum>  &sessionList)
{
    const char *myName = concat3(THIS_NAME, "/", "IPRX/FeedToe");

    if (ipRxPacketizer.size() != 0) {
        // Insert proper ACK Number in packet at the head of the queue
        injectAckNumber(ipRxPacketizer[0], sessionList);
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
 * @param[in]     ipRxFile,       A ref to the input file w/ IP Rx packets.
 * @param[in/out] ipRxPktCounter, A ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
 * @param[in/out] tcpRxBytCounter,A ref to the IP Rx byte counter.
 *                                 (counts all kinds and from all sessions).
 * @param[in/out] ipRxPacketizer, A ref to the RxPacketizer (double-ended queue).
 * @param[in]     sessionList,    A ref to an associative container which holds
 *                                  the sessions as socket pair associations.
 * @param[out]    soTOE_Data,     A reference to the data stream to TOE.
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
        ifstream                      &ipRxFile,
        ofstream                      &appTxGold,
        int                           &ipRxPktCounter,
        int                           &tcpRxBytCounter,
        deque<IpPacket>               &ipRxPacketizer,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        stream<Ip4overAxi>            &soTOE_Data)
{
    static bool         ipRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int ipRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int ipRxIdleCycCnt = 0;     // The count of idle cycles

    string              rxStringBuffer;
    vector<string>      stringVector;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    //-----------------------------------------------------
    //-- STEP-1 : RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (ipRxIdlingReq == true) {
        if (ipRxIdleCycCnt >= ipRxIdleCycReq) {
            ipRxIdleCycCnt = 0;
            ipRxIdlingReq = false;
        }
        else
            ipRxIdleCycCnt++;
    }

    //-----------------------------------------------------
    //-- STEP-2 : FEED the IP Rx INTERFACE
    //-----------------------------------------------------
    // Note: The IPv4 RxPacketizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, L3Mux).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the file.
    feedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessionList);

    //-----------------------------------------------------
    //-- STEP-3 : READ THE IP RX FILE
    //-----------------------------------------------------
    // Check for EOF
    if (ipRxFile.eof())
        return;

    do {
        //-- READ A LINE FROM IPRX INPUT FILE -------------
        getline(ipRxFile, rxStringBuffer);

        stringVector = myTokenizer(rxStringBuffer);

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0] == "#") {
            // WARNING: A comment must start with a hash symbol followed by a space character
            for (int t=0; t<stringVector.size(); t++)
                printf("%s ", stringVector[t].c_str());
            printf("\n");
            continue;
        }
        else if (stringVector[0] == "C") {
            // The test vector file is specifying a minimum number of simulation cycles.
            int noSimCycles = atoi(stringVector[1].c_str());
            if (noSimCycles > gMaxSimCycles)
                gMaxSimCycles = noSimCycles;
            return;
        }
        else if (stringVector[0] == "W") {
            // The test vector is is willing to request idle wait cycles before providing
            // TOE with a new IPRX packet.
            ipRxIdleCycReq = atoi(stringVector[1].c_str());
            ipRxIdlingReq = true;
            return;
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
                    stringVector = myTokenizer(rxStringBuffer);
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                ipRxData = Ip4overAxi(encodeApUint64(stringVector[0]), \
                                      encodeApUint8(stringVector[2]),  \
                                      atoi(stringVector[1].c_str()));
                ipRxPacket.push_back(ipRxData);
            } while (ipRxData.tlast != 1);

            // Count the number of data bytes contained in the TCP payload
            tcpRxBytCounter += ipRxPacket.sizeOfTcpData();

            // Write to the Application Tx Gold file
            writeAppTxGoldFile(appTxGold, ipRxPacket);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            feedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessionList); // [FIXME-Can be removed?]

            return;
        }

    } while(!ipRxFile.eof());

}


/*****************************************************************************
 * @brief Parse the TCP/IP packets generated by the TOE.
 *
 * @param[in]  ipTxPacket,     a ref to the packet received from the TOE.
 * @param[in]  sessionList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out] ipRxPacketizer, a ref to dequeue w/ packets for IPRX.
 *
 * @return true if an ACK was found [FIXME].
 *
 * @details
 *  Looks for an ACK in the IP packet. If found, stores the 'ackNumber' from
 *  that packet into the 'seqNumber' deque of the Rx input stream and clears
 *  the deque containing the IP Tx packet.
 *
 *  @ingroup toe
 ******************************************************************************/
bool parseL3MuxPacket(
        IpPacket                      &ipTxPacket,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        deque<IpPacket>               &ipRxPacketizer)
{
    bool        returnValue    = false;
    bool        isFinAck       = false;
    bool        isSynAck       = false;
    static int  ipTxPktCounter = 0;
    static int  currAckNum     = 0;

    const char *myName = concat3(THIS_NAME, "/", "L3MUX/Parse");

    if (DEBUG_LEVEL & TRACE_L3MUX) {
        ipTxPacket.printHdr(myName);
    }

    if (ipTxPacket.isSYN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a SYN segment. Reply with a SYN+ACK packet.
        //------------------------------------------------------
        if (DEBUG_LEVEL & TRACE_L3MUX)
            printInfo(myName, "Got a SYN from TOE. Replying with a SYN+ACK.\n");

        IpPacket synAckPacket(40);
        synAckPacket.clone(ipTxPacket);

        // Swap IP_SA and IP_DA
        synAckPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
        synAckPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

        // Swap TCP_SP and TCP_DP
        synAckPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
        synAckPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

        // Swap the SEQ and ACK Numbers while incrementing the ACK
        synAckPacket.setTcpSequenceNumber(0);
        synAckPacket.setTcpAcknowledgeNumber(ipTxPacket.getTcpSequenceNumber() + 1);

        // Set the ACK bit and Recalculate the Checksum
        synAckPacket.setTcpControlAck(1);
        int newTcpCsum = synAckPacket.recalculateChecksum();
        synAckPacket.setTcpChecksum(newTcpCsum);

        // Add the created SYN+ACK packet to the ipRxPacketizer
        ipRxPacketizer.push_back(synAckPacket);

    }

    else if (ipTxPacket.isFIN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a FIN segment. Close the connection.
        //------------------------------------------------------

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        TbSockAddr  srcSock = TbSockAddr(ipTxPacket.getIpSourceAddress(),
                                         ipTxPacket.getTcpSourcePort());
        TbSockAddr  dstSock = TbSockAddr(ipTxPacket.getIpDestinationAddress(),
                                         ipTxPacket.getTcpDestinationPort());
        TbSocketPair sockPair(dstSock, srcSock);

        if (DEBUG_LEVEL & TRACE_L3MUX) {
            printInfo(myName, "Got a FIN from TOE. Closing the following connection:\n");
            printTbSockPair(myName, sockPair);
        }

        // Erase the socket pair for this session from the map.
        sessionList.erase(sockPair);

    }

    else if (ipTxPacket.isACK()) {
        //---------------------------------------
        // This is an ACK segment.
        //---------------------------------------
        returnValue = true;

        // Retrieve IP packet length and TCP sequence numbers
        int ip4PktLen  = ipTxPacket.getIpTotalLength();
        int nextAckNum = ipTxPacket.getTcpSequenceNumber();
        currAckNum     = ipTxPacket.getTcpAcknowledgeNumber();

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        TbSockAddr  srcSock = TbSockAddr(ipTxPacket.getIpSourceAddress(),
                                         ipTxPacket.getTcpSourcePort());
        TbSockAddr  dstSock = TbSockAddr(ipTxPacket.getIpDestinationAddress(),
                                         ipTxPacket.getTcpDestinationPort());
        TbSocketPair sockPair(dstSock, srcSock);

        if (ipTxPacket.isFIN() && !ipTxPacket.isSYN()) {
            // The FIN bit is also set
            nextAckNum++;  // A FIN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
               printInfo(myName, "Got a FIN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isSYN() && !ipTxPacket.isFIN()) {
            // The SYN bit is also set
            nextAckNum++;  // A SYN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got a SYN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isFIN() && ipTxPacket.isSYN()) {
             printError(myName, "Got a SYN+FIN+ACK from TOE.\n");
             // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
        }
        else if (ip4PktLen >= 40) {
            // Decrement by 40B (.i.e, 20B of IP Header + 20B of TCP Header
            // [FIXME - What if we add options???]
            // [TODO - Print the TCP options ]
            ip4PktLen -= 40;
            nextAckNum += ip4PktLen;
        }

        // Update the Session List with the new sequence number
        sessionList[sockPair] = nextAckNum;

        if (ipTxPacket.isFIN()) {
            //------------------------------------------------
            // This is an ACK+FIN segment.
            //------------------------------------------------
            isFinAck = true;
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got an ACK+FIN from TOE.\n");

            // Erase this session from the list
            int itemsErased = sessionList.erase(sockPair);
            if (itemsErased != 1) {
                printError(myName, "Received a ACK+FIN segment for a non-existing session. \n");
                printTbSockPair(myName, sockPair);
                // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
            }
            else {
                if (DEBUG_LEVEL & TRACE_L3MUX) {
                    printInfo(myName, "Connection was successfully closed.\n");
                    printTbSockPair(myName, sockPair);
                }
            }
        } // End of: isFIN

        if (ip4PktLen > 0 || isFinAck == true) {
            //--------------------------------------------------------
            // The ACK segment contains more data (.e.g, TCP options),
            // and/or the segment is a FIN+ACK segment.
            // In both cases, reply with an empty ACK packet.
            //--------------------------------------------------------
            IpPacket ackPacket(40);  // [FIXME - What if we generate options ???]

            // [TODO - Add TCP Window option]

            // Swap IP_SA and IP_DA
            ackPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
            ackPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

            // Swap TCP_SP and TCP_DP
            ackPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
            ackPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

            // Swap the SEQ and ACK Numbers while incrementing the ACK
            ackPacket.setTcpSequenceNumber(currAckNum);
            ackPacket.setTcpAcknowledgeNumber(nextAckNum);

            // Set the ACK bit and unset the FIN bit
            ackPacket.setTcpControlAck(1);
            ackPacket.setTcpControlFin(0);

            // Recalculate the Checksum
            int newTcpCsum = ackPacket.recalculateChecksum();
            ackPacket.setTcpChecksum(newTcpCsum);

            // Add the created ACK packet to the ipRxPacketizer
            ipRxPacketizer.push_back(ackPacket);

            // Increment the packet counter
            if (currAckNum != nextAckNum) {
                ipTxPktCounter++;
                if (DEBUG_LEVEL & TRACE_L3MUX) {
                    printInfo(myName, "IP Tx Packet Counter = %d \n", ipTxPktCounter);
                }
            }
            currAckNum = nextAckNum;
        }
    }

    // Clear the received Tx packet
    ipTxPacket.clear();

    return returnValue;

} // End of: parseL3MuxPacket()


/*****************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]     siTOE_Data,      A reference to the data stream from TOE.
 * @param[in]     iptxFile,        The output file stream to write.
 * @param[in]     sessionList,     A ref to an associative container which holds the sessions as socket pair associations.
 * @param[in/out] ipTxPktCount,    A ref to the IP Tx packet counter (counts all kinds and from all sessions).
 * @param[in/out] tcpTxBytCounter, A ref to the TCP Tx byte counter (counts all kinds and from all sessions).
 * @param[out]    ipRxPacketizer,  A ref to the IPv4 Rx packetizer.
 *
 * @details
 *  Drains the data from the L3MUX interface of the TOE and stores them into
 *   an IPv4 Tx Packet (ipTxPacket). This ipTxPacket is a double-ended queue
 *   used to accumulate all the data chunks until a whole packet is received.
 *  This queue is further read by a packet parser which either forwards the
 *   packets to an output file, or which generates an ACK packet that is
 *   injected into the 'ipRxPacketizer' (see process 'pIPRX').
 *
 * @ingroup toe
 ******************************************************************************/
void pL3MUX(
        stream<Ip4overAxi>            &siTOE_Data,
        ofstream                      &iptxFile,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        int                           &ipTxPktCounter,
        int                           &tcpTxBytCounter,
        deque<IpPacket>               &ipRxPacketizer)
{
    const char *myName  = concat3(THIS_NAME, "/", "L3MUX");

    static IpPacket ipTxPacket;

    Ip4overAxi  ipTxWord;  // An IP4 chunk
    uint16_t    ipTxWordCounter = 0;

    if (!siTOE_Data.empty()) {

        //-- STEP-1 : Drain the TOE -----------------------
        siTOE_Data.read(ipTxWord);

        //-- STEP-2 : Write to packet --------------------
        ipTxPacket.push_back(ipTxWord);

        //-- STEP-3 : Parse the received packet------------
        if (ipTxWord.tlast == 1) {
            // The whole packet is now into the deque.
            if (parseL3MuxPacket(ipTxPacket, sessionList, ipRxPacketizer) == true) {
                ipTxPktCounter++;
                tcpTxBytCounter += ipTxPacket.sizeOfTcpData();
                ipTxWordCounter = 0;
            }
        }
        else
            ipTxWordCounter++;

        //-- STEP-4 : Write to file ------------- ---------
        string dataOutput = decodeApUint64(ipTxWord.tdata);
        string keepOutput = decodeApUint8(ipTxWord.tkeep);
        iptxFile << dataOutput << " " << ipTxWord.tlast << " " << keepOutput << endl;
    }
}


/*****************************************************************************
 * @brief Write a TCP piece of data into the Tx Application file.
 *
 * @param[in]     appTxFile, a ref to the file to write.
 * @param[in]     tcpWord,   a ref to the AXI word to write.
 * @param[in/out] appTxBytCounter, A ref to the counter of bytes from TOE to APP.
 *
 * @ingroup test_toe
 ******************************************************************************/
void writeAppTxDataFile(
        ofstream    &appTxFile,
        AxiWord     &tcpWord,
        int         &appTxBytCounter)
{
    string tdataToFile = "";

    for (int bytNum=0; bytNum<8; bytNum++) {
        if (tcpWord.tkeep.bit(bytNum)) {
            int hi = ((bytNum*8) + 7);
            int lo = ((bytNum*8) + 0);
            ap_uint<8>  octet = tcpWord.tdata.range(hi, lo);
            tdataToFile += toHexString(octet);
            appTxBytCounter++;
        }
    }

    if (tcpWord.tlast == 1)
        appTxFile << tdataToFile << endl;
    else
        appTxFile << tdataToFile;

}

/*****************************************************************************
 * @brief Start listening on a given port number.
 *
 * @param[in]  portNum,      the port number to listen.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 *
 * @return true if listening was successful, otherwise false.
 *
 * @ingroup test_toe
 ******************************************************************************/
bool listen(
        TcpPort          &portNum,
        stream<TcpPort>  &soTOE_LsnReq,
        stream<bool>     &siTOE_LsnAck)
{
    static ap_uint<1> listenFsm   = 0;
    bool              listenDone  = false;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF/listen()");

    switch (listenFsm) {
    case 0:
        soTOE_LsnReq.write(portNum);
        if (DEBUG_LEVEL & TRACE_TRIF) {
            printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                      portNum.to_uint(), portNum.to_uint());
        }
        listenFsm++;
        break;

    case 1:
        if (!siTOE_LsnAck.empty()) {
            siTOE_LsnAck.read(listenDone);
            if (listenDone) {
                printInfo(myName, "TOE is now listening on port %d (0x%4.4X).\n",
                          portNum.to_uint(), portNum.to_uint());
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          portNum.to_uint(), portNum.to_uint());
            }
            listenFsm++;
        }
        break;
    }
    return listenDone;
}

/*****************************************************************************
 * @brief Open a new socket and setup connection.
 *
 * @param[in]  localSockAddr, The local socket address .
 * @param[out] txSessIdVector,A vector containing the generated Tx session IDs.
 * @param[out] soTOE_OpnReq,  TCP open connection request to TOE.
 * @param[in]  siTOE_OpnSts,  TCP open connection status from TOE.
 *
 * @return true if a socket was successfully opened, otherwise false. The
 *
 * @ingroup test_toe
 ******************************************************************************/
bool openCon(
        TbSockAddr             &localSockAddr,
        vector<SessionId>      &txSessIdVector,
        stream<AxiSockAddr>    &soTOE_OpnReq,
        stream<OpenStatus>     &siTOE_OpnSts)
{

    const char *myName  = concat3(THIS_NAME, "/", "TRIF/socket()");

    for (static int8_t i=0; i<noTxSessions; ++i) {
        // Define the socket pair involved in the connection
        TbSockAddr   foreignSockAddr(150*(i+65355), 10*(i+65355));
        TbSocketPair socketPair(localSockAddr, foreignSockAddr);


        //OBSOLETE-20181221 AxiSocketPair axiSocketPair(AxiSockAddr(byteSwap32(localSockAddr.addr),   byteSwap16(localSockAddr.port)),
        //OBSOLETE-20181221                            AxiSockAddr(byteSwap32(foreignSockAddr.addr), byteSwap16(foreignSockAddr.port)));

        AxiSockAddr axiForeignSockAddr(AxiSockAddr(byteSwap32(foreignSockAddr.addr),
                                                   byteSwap16(foreignSockAddr.port)));

        static int openFsm = 0;

        switch (openFsm) {

        case 0:
            soTOE_OpnReq.write(axiForeignSockAddr);
            if (DEBUG_LEVEL & TRACE_TRIF) {
                printInfo(myName, "Request to open the following socket: \n");
                printSockAddr(myName, axiForeignSockAddr);
            }
            openFsm++;
            break;

        case 1:
            if (!siTOE_OpnSts.empty()) {
                OpenStatus openConStatus = siTOE_OpnSts.read();
                if(openConStatus.success) {
                    txSessIdVector.push_back(openConStatus.sessionID);
                    if (DEBUG_LEVEL & TRACE_TRIF)
                        printInfo(myName, "Session #%d is now opened.\n", openConStatus.sessionID.to_uint());
                }
                else {
                    printWarn(myName, "Session #%d is not yet opened.\n", openConStatus.sessionID.to_uint());
                }
                openFsm++;
            }
            break;
        }
    }

    return true;
}



/*****************************************************************************
 * @brief Emulates the behavior of the TCP Role Interface (TRIF).
 *             This process implements Iperf.
 *
 * @param[in]  testTxPath,   Indicates if the Tx path is to be tested.
 * @param[in]  myIpAddress,  The local IP address used by the TOE.
 * @param[in]  appRxFile,    A ref to the input Rx application file to read.
 * @param[in]  appTxFile,    A ref to the output Tx application file to write.
 * @param[in/out] appTxBytCounter, A ref to the counter of bytes from TOE to APP.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,  TCP notification from TOE.
 * @param[out] soTOE_DReq,   TCP data request to TOE.
 * @param[in]  siTOE_Meta,   TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,   TCP data stream from TOE.
 * @param[out] soTOE_OpnReq, TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts, TCP open port status from TOE.
 * @param[out] soTOE_ClsReq, TCP close connection request to TOE.
 *
 * @details:
 *
 * *
 * @remark: [FIXME - Not accurate. To be updated]
 *  By default, the Iperf client connects to the Iperf server on the TCP port
 *   5001 and the bandwidth displayed by Iperf is the bandwidth from the client
 *   to the server.
 *  The metadata from TRIF (i.e., the Tx session ID) is not directly sent to
 *   TOE. Instead, it is pushed into a vector that is used by the main process
 *   when it feeds the input data flows [FIXME].
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF(
        bool                    &testTxPath,
        Ip4Address              &myIpAddress,
        ifstream                &appRxFile,
        ofstream                &appTxFile,
        int                     &appTxBytCounter,
        stream<TcpPort>         &soTOE_LsnReq,
        stream<bool>            &siTOE_LsnAck,
        stream<appNotification> &siTOE_Notif,
        stream<appReadRequest>  &soTOE_DReq,
        stream<SessionId>       &siTOE_Meta,
        stream<AxiWord>         &siTOE_Data,
        stream<AxiSockAddr>     &soTOE_OpnReq,
        stream<OpenStatus>      &siTOE_OpnSts,
        stream<ap_uint<16> >    &soTOE_ClsReq)
{
    static bool         listenDone         = false;
    static bool         openDone           = false;
    static bool         runningExperiment  = false;

    static bool         appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int appRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int appRxIdleCycCnt = 0;     // The count of idle cycles

    OpenStatus          newConStatus;
    appNotification     notification;
    ipTuple             tuple;
    vector<SessionId>   txSessIdVector;  // A vector containing the Tx session IDs to be send from TRIF/Meta to TOE/Meta

    //Ip4Addr  openAddr = 0x0A0A0A0A;   // 10.10.10.10
    //TcpPort  openPort = 0x89;         // #137
    //TbSockAddr(openAddr, openPort);

    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    //------------------------------------------------
    //-- STEP-1 : REQUEST TO LISTEN ON A PORT
    //------------------------------------------------
    TcpPort listeningPort = 0x0057;   // #87
    if (!listenDone) {
        listenDone = listen(listeningPort, soTOE_LsnReq, siTOE_LsnAck);
    }

    //------------------------------------------------
    //-- STEP-2 : REQUEST TO OPEN CONNECTION(S)
    //------------------------------------------------
    TbSockAddr  localTbSockAddr(myIpAddress, listeningPort);
    if (!openDone) {
        openDone = openCon(localTbSockAddr, txSessIdVector, soTOE_OpnReq, siTOE_OpnSts);
    }

    //-----------------------------------------------------
    //-- STEP-3 : READ THE APP RX FILE AND FEED THE TOE
    //-----------------------------------------------------
    // Only if Tx test mode is enabled and after a session has been opened on the Tx Side
    if (testTxPath == true && txSessIdVector.size() > 0) {

        //-- STEP-3.0 : RETURN IF IDLING IS REQUESTED -----
        if (appRxIdlingReq == true) {
            if (appRxIdleCycCnt >= appRxIdleCycReq) {
                appRxIdleCycCnt = 0;
                appRxIdlingReq = false;
            }
            else
                appRxIdleCycCnt++;
        }

        //-- STEP-3.1 : FEED the APP Rx INTERFACE ---------
        //TODO-TODAY feedTOE(appRxPacketizer, appRxPktCounter, soTOE_Data, sessionList);

         //-- STEP-2.2 : READ THE APP RX FILE -------------
        while(!appRxFile.eof()) {

            string          rxStringBuffer;
            vector<string>  stringVector;

            getline(appRxFile, rxStringBuffer);

            stringVector = myTokenizer(rxStringBuffer);

            if (stringVector[0] == "") {
                continue;
            }
            else if (stringVector[0] == "#") {
                // WARNING: A comment must start with a hash symbol followed by a space character
                for (int t=0; t<stringVector.size(); t++)
                    printf("%s ", stringVector[t].c_str());
                printf("\n");
                continue;
            }
            else if (stringVector[0] == "C") {
                // The test vector file is specifying a minimum number of simulation cycles.
                int noSimCycles = atoi(stringVector[1].c_str());
                if (noSimCycles > gMaxSimCycles)
                    gMaxSimCycles = noSimCycles;
                return;
            }
            else if (stringVector[0] == "W") {
                // The test vector is is willing to request idle wait cycles before providing
                // TOE with a new APP segment.
                appRxIdleCycReq = atoi(stringVector[1].c_str());
                appRxIdlingReq = true;
                return;
            }
            else if (appRxFile.fail() == 1 || rxStringBuffer.empty()) {
                return;
            }
            else {
                // Build a new segment from data file
            	//TODO-TODAY TcpSegment  appRxSegment;
            	//TODO-TODAY TcpOverAxi  tcpRxData;
                bool        firstWordFlag = true; // AXI-word is first chunk of segment

                do {
                    if (firstWordFlag == false) {
                        getline(appRxFile, rxStringBuffer);
                        stringVector = myTokenizer(rxStringBuffer);
                    }
                    else {
                        // This is the first chunk of a frame.
                        // A Tx data request (i.e. a metadata) must be sent by TRIF to TOE
                    	//TODO-TODAY soTOE_Meta.write(txSessIdVector[currTxSessionID]);
                    	//TODO-TODAY currTxSessionID == noTxSessions - 1 ? currTxSessionID = 0 : currTxSessionID++;
					}
                    firstWordFlag = false;
                    string tempString = "0000000000000000";
                    //TODO-TODAY tcpRxData = AxiWord(encodeApUint64(stringVector[0]),\
                                        encodeApUint8(stringVector[2]),\
                                        atoi(stringVector[1].c_str()));
                    //TODO-TODAY tcpRxSegment.push_back(appRxData);
                } while (1); //TODO-TODAY  tcpRxData.tlast != 1);

                // Count the number of data bytes contained in the TCP segment
                //TODO-TODAY tcpRxBytCounter += appRxSegment.sizeOfTcpData();

                // Write to the TCP Tx Gold file
                //TODO-TODAY writeAppTxGoldFile(tcpTxGold, ipTxPacket);

                // Push that packet into the packetizer
                //TODO-TODAY appRxSegmentizer.push_back(appRxSegment);

            }
        }
    }

    //------------------------------------------------
    //-- STEP-4 : READ NOTIFICATION
    //------------------------------------------------
    if (!siTOE_Notif.empty()) {
        siTOE_Notif.read(notification);
        if (notification.length != 0)
            soTOE_DReq.write(appReadRequest(notification.sessionID,
                                            notification.length));
        else // closed
            runningExperiment = false;
    }

    //------------------------------------------------
    //-- STEP-5 : DRAIN TOE-->TRIF
    //------------------------------------------------
    //-- IPERF PROCESSING
    static enum FsmState {WAIT_SEG=0, CONSUME, HEADER_2, HEADER_3} fsmState = WAIT_SEG;

    SessionId           tcpSessId;
    AxiWord             currWord;
    static bool         dualTest = false;
    static ap_uint<32>  mAmount = 0;

    currWord.tlast = 0;

    switch (fsmState) {

    case WAIT_SEG:
        if (!siTOE_Meta.empty() && !siTOE_Data.empty()) {
            // Read the TCP session ID and the 1st TCP data chunk
            siTOE_Meta.read(tcpSessId);
            siTOE_Data.read(currWord);
            // Write TCP data chunk to file
            writeAppTxDataFile(appTxFile, currWord, appTxBytCounter);

            //FIXME if (!runningExperiment) {
            //FIXME     // Check if a bidirectional test is requested (i.e. dualtest)
            //FIXME     if (currWord.tdata(31, 0) == 0x00000080)
            //FIXME         dualTest = true;
            //FIXME     else
            //FIXME         dualTest = false;
            //FIXME     runningExperiment = true;
            //FIXME     fsmState = HEADER_2;
            //FIXME }
            //FIXME else
            fsmState = CONSUME;
        }
        break;

    /*** FIXME ***
    case HEADER_2:
        if (!siTOE_Data.empty()) {
        	 // Read the 2nd TCP data chunk
            siTOE_Data.read(currWord);
            writeApplicationTxFile(appTxFile, currWord, appTxBytCounter);

            if (dualTest) {
                tuple.ip_address = 0x0a010101;  // FIXME
                tuple.ip_port    = currWord.tdata(31, 16);
                soTOE_OpnReq.write(tuple);
            }    // Check for EOF
    if (ipRxFile.eof())
        return;
            fsmState = HEADER_3;
        }
        break;

    case HEADER_3:
        if (!siTOE_Data.empty()) {
            // Read 3rd TCP data chunk
            siTOE_Data.read(currWord);
            writeApplicationTxFile(appTxFile, currWord, appTxBytCounter);

            mAmount = currWord.tdata(63, 32);
            fsmState = CONSUME;
        }
        break;
    **************/

    case CONSUME:
        if (!siTOE_Data.empty()) {
            // Read all the remaining TCP data chunks
            siTOE_Data.read(currWord);
            writeAppTxDataFile(appTxFile, currWord, appTxBytCounter);
        }
        break;
    }

    // Intercept the cases where TCP payload in less that 4 chunks
    if (currWord.tlast == 1)
        fsmState = WAIT_SEG;

}


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode,         the testing mode (0=RX_MODE, 1=TX_MODE, 2=BIDIR_MODE).
 * @param[in]  ipRxFileName, the pathname of the input file containing the IP
 *                            packets to be fed to the TOE.
 *
 * @details:
 *
 * @remark:
 *  The number of input parameters is variable and depends on the testing mode.
 *   Examples:
 *    csim_design -argv "0 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
 *
 * @ingroup test_toe
 ******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<Ip4overAxi>                  sIPRX_Toe_Data      ("sIPRX_Toe_Data");

    stream<Ip4overAxi>                  sTOE_L3mux_Data     ("sTOE_L3mux_Data");

    stream<AxiWord>                     sTRIF_Toe_Data      ("sTRIF_Toe_Data");
    stream<ap_uint<16> >                sTRIF_Toe_Meta      ("sTRIF_Toe_Meta");
    stream<ap_int<17> >                 sTOE_Trif_DSts      ("sTOE_Trif_DSts");

    stream<appReadRequest>              sTRIF_Toe_DReq      ("sTRIF_Toe_DReq");
    stream<AxiWord>                     sTOE_Trif_Data      ("sTOE_Trif_Data");
    stream<SessionId>                   sTOE_Trif_Meta      ("sTOE_Trif_Meta");

    stream<TcpPort>                     sTRIF_Toe_LsnReq    ("sTRIF_Toe_LsnReq");
    stream<bool>                        sTOE_Trif_LsnAck    ("sTOE_Trif_LsnAck");

    stream<AxiSockAddr>                 sTRIF_Toe_OpnReq    ("sTRIF_Toe_OpnReq");
    stream<OpenStatus>                  sTOE_Trif_OpnSts    ("sTOE_Trif_OpnSts");

    stream<appNotification>             sTOE_Trif_Notif     ("sTOE_Trif_Notif");

    stream<ap_uint<16> >                sTRIF_Toe_ClsReq    ("sTRIF_Toe_ClsReq");

    stream<DmCmd>                       sTOE_Mem_RxP_RdCmd  ("sTOE_Mem_RxP_RdCmd");
    stream<AxiWord>                     sMEM_Toe_RxP_Data   ("sMEM_Toe_RxP_Data");
    stream<DmSts>                       sMEM_Toe_RxP_WrSts  ("sMEM_Toe_RxP_WrSts");
    stream<DmCmd>                       sTOE_Mem_RxP_WrCmd  ("sTOE_Mem_RxP_WrCmd");
    stream<AxiWord>                     sTOE_Mem_RxP_Data   ("sTOE_Mem_RxP_Data");

    stream<DmCmd>                       sTOE_Mem_TxP_RdCmd  ("sTOE_Mem_TxP_RdCmd");
    stream<AxiWord>                     sMEM_Toe_TxP_Data   ("sMEM_Toe_TxP_Data");
    stream<DmSts>                       sMEM_Toe_TxP_WrSts  ("sMEM_Toe_TxP_WrSts");
    stream<DmCmd>                       sTOE_Mem_TxP_WrCmd  ("sTOE_Mem_TxP_WrCmd");
    stream<AxiWord>                     sTOE_Mem_TxP_Data   ("sTOE_Mem_TxP_Data");

    stream<rtlSessionLookupReply>       sCAM_This_SssLkpRpl ("sCAM_This_SssLkpRpl");
    stream<rtlSessionUpdateReply>       sCAM_This_SssUpdRpl ("sCAM_This_SssUpdRpl");
    stream<rtlSessionLookupRequest>     sTHIS_Cam_SssLkpReq ("sTHIS_Cam_SssLkpReq");
    stream<rtlSessionUpdateRequest>     sTHIS_Cam_SssUpdReq ("sTHIS_Cam_SssUpdReq");

    //OBSOLETE-20181207 stream<AxiWord>                     sTRIF_Role_Data     ("sTRIF_Role_Data");


    //-- TESTBENCH MODES OF OPERATION ---------------------
    enum TestingMode {RX_MODE='0', TX_MODE='1', BIDIR_MODE='2'};

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    int             nrErr;

    //OBSOLETE-20181219 bool            firstWordFlag;

    Ip4Word         ipRxData;    // An IP4 chunk
    AxiWord         tcpTxData;    // A  TCP chunk

    Ip4Address      myIpAddress = 0x01010101;  // [TODO - Make this a parameter of the testbench]

    ap_uint<16>     opnSessionCount;
    ap_uint<16>     clsSessionCount;

    AxiWord         rxDataOut_Data;         // This variable is where the data read from the stream above is temporarily stored before output

    DummyMemory     rxMemory;
    DummyMemory     txMemory;

    map<TbSocketPair, TcpAckNum>    sessionList;

    //-- Double-ended queue of packets --------------------
    deque<IpPacket>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        ipRxFile;   // IP packets to         IPRX  I/F of TOE.
    ofstream        appTxFile;  // APP byte streams from TRIF  I/F of TOE.
    ofstream        appTxGold;  // Gold reference file for 'appTxFile'

    ifstream        appRxFile;  // APP data streams to   TRIF  I/F of TOE.
    ofstream        ipTxFile;   // IP packets from       L3MUX I/F of TOE.
    ofstream        ipTxGold;   // Gold reference file for 'ipTxFile'


    //Not USed ifstream        txInputFile; // FIXME
    //Not USed ifstream        rxGoldFile;  // FIXME
    //Not USed ifstream        txGoldFile;  // FIXME

    int             rxGoldCompare       = 0;
    int             returnValue         = 0;

    const char      *ipTxFileName  = "../../../../test/ipTx_TOE.dat";
    const char      *ipTxGoldName  = "../../../../test/ipTx_TOE.gold";
    const char      *appTxFileName = "../../../../test/appTx_TOE.dat";
    const char      *appTxGoldName = "../../../../test/appTx_TOE.gold";


    unsigned int    myCounter   = 0;

    string          dataString, keepString;
    uint16_t        currTxSessionID = 0;    // The current Tx session ID

    int             ipRxPktCounter      = 0;    // Counts the # IP packets rcvd by the TOE (all kinds and from all sessions).
    int             tcpRxBytCounter     = 0;    // Counts the # TCP bytes  rcvd by the TOE.

    int             ipTxPktCounter      = 0;    // Counts the # IP packets sent by the TOE (all kinds and from all sessions).
    int             appTxBytCounter     = 0;    // Counts the # APP bytes  sent by the TOE.
    int             tcpTxBytCounter     = 0;    // Counts the # TCP bytes  sent by the TOE.

    bool            testRxPath          = false; // Indicates if the Rx path is to be tested.
    bool            testTxPath          = false; // Indicates if the Tx path is to be tested.

    char            mode        = *argv[1];
    char            cCurrPath[FILENAME_MAX];

    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printf("## [TB-ERROR] Expected a minimum of 2 or 3 parameters with one of the the following synopsis:\n");
        printf("\t mode(0) ipRxFileName\n");
        printf("\t mode(1) apRxFileName\n");
        printf("\t mode(2) ipRxFileName apRxFileName\n");
        return -1;
    }

    printf("INFO: This TB-run executes in mode \'%c\'.\n", mode);

    if ((mode == RX_MODE) || (mode == BIDIR_MODE)) {
        testRxPath = true;
        //-------------------------------------------------
        //-- Files used for the test of the Rx side
        //-------------------------------------------------
        ipRxFile.open(argv[2]);
        if (!ipRxFile) {
            printf("## [TB-ERROR] Cannot open the IP Rx file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }

        appTxFile.open(appTxFileName);
        if (!appTxFile) {
            printf("## [TB-ERROR] Cannot open the Application Tx file:  \n\t %s \n", appTxFileName);
            return -1;
        }

        appTxGold.open(appTxGoldName);
        if (!appTxGold) {
            printf("## [TB-ERROR] Cannot open the Application Tx gold file:  \n\t %s \n", appTxGoldName);
            return -1;
        }
    }

    if ((mode == TX_MODE) || (mode == BIDIR_MODE)) {
        testTxPath = true;
        //-------------------------------------------------
        //-- Files used for the test of the Tx side
        //-------------------------------------------------
        switch (mode) {
        case TX_MODE:
            appRxFile.open(argv[2]);
            if (!appRxFile) {
                printf("## [TB-ERROR] Cannot open the APP Rx file: \n\t %s \n", argv[2]);
                if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                    return -1;
                }
                printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
                return -1;
            }
            break;
        case BIDIR_MODE:
            appRxFile.open(argv[3]);
            if (!appRxFile) {
                printf("## [TB-ERROR] Cannot open the APP Rx file: \n\t %s \n", argv[3]);
                if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                    return -1;
                }
                printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
                return -1;
            }
            break;
        }

        ipTxFile.open(ipTxFileName);
        if (!ipTxFile) {
            printf("## [TB-ERROR] Cannot open the IP Tx gold file:  \n\t %s \n", ipTxGoldName);
            return -1;
        }

        ipTxGold.open(ipTxGoldName);
        if (!ipTxGold) {
            printf("## [TB-ERROR] Cannot open the IP Tx file:  \n\t %s \n", ipTxGoldName);
            return -1;
        }
    }

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt = 0;     // Simulation cycle counter as a global variable
    nrErr      = 0;     // Total number of testbench errors

    AxiIp4Addr mmioIpAddr = (AxiIp4Addr)(byteSwap32(myIpAddress));

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        if (testRxPath == true) {
            //-------------------------------------------------
            //-- STEP-1 : Emulate the IPv4 Rx Path
            //-------------------------------------------------
            pIPRX(ipRxFile,       appTxGold,
                  ipRxPktCounter, tcpRxBytCounter,
                  ipRxPacketizer, sessionList, sIPRX_Toe_Data);
        }

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        toe(
            //-- From MMIO Interfaces
            mmioIpAddr,
            //-- IPv4 / Rx & Tx Interfaces
            sIPRX_Toe_Data,   sTOE_L3mux_Data,
            //-- TRIF / Rx Interfaces
            sTRIF_Toe_DReq,   sTOE_Trif_Notif,  sTOE_Trif_Data,   sTOE_Trif_Meta,
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            //-- TRIF / Tx Interfaces
            sTRIF_Toe_Data,   sTRIF_Toe_Meta,   sTOE_Trif_DSts,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts, sTRIF_Toe_ClsReq,
            //-- MEM / Rx PATH / S2MM Interface
            sTOE_Mem_RxP_RdCmd, sMEM_Toe_RxP_Data, sMEM_Toe_RxP_WrSts, sTOE_Mem_RxP_WrCmd, sTOE_Mem_RxP_Data,
            sTOE_Mem_TxP_RdCmd, sMEM_Toe_TxP_Data, sMEM_Toe_TxP_WrSts, sTOE_Mem_TxP_WrCmd, sTOE_Mem_TxP_Data,
            //-- CAM / This / Session Lookup & Update Interfaces
            sCAM_This_SssLkpRpl, sCAM_This_SssUpdRpl,
            sTHIS_Cam_SssLkpReq, sTHIS_Cam_SssUpdReq,
            //-- DEBUG / Session Statistics Interfaces
            clsSessionCount, opnSessionCount);

        //-------------------------------------------------
        //-- STEP-3 : Emulate DRAM & CAM Interfaces
        //-------------------------------------------------
        pEmulateRxBufMem(
            &rxMemory,
            sTOE_Mem_RxP_WrCmd, sMEM_Toe_RxP_WrSts,
            sTOE_Mem_RxP_RdCmd, sTOE_Mem_RxP_Data, sMEM_Toe_RxP_Data);

        pEmulateTxBufMem(
            &txMemory,
            sTOE_Mem_TxP_WrCmd, sMEM_Toe_TxP_WrSts,
            sTOE_Mem_TxP_RdCmd, sTOE_Mem_TxP_Data, sMEM_Toe_TxP_Data);

        pEmulateCam(
            sTHIS_Cam_SssLkpReq, sCAM_This_SssLkpRpl,
            sTHIS_Cam_SssUpdReq, sCAM_This_SssUpdRpl);

        //------------------------------------------------------
        //-- STEP-4 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------

        //-------------------------------------------------
        //-- STEP-4.0 : Emulate Layer-3 Multiplexer
        //-------------------------------------------------
        pL3MUX(
            sTOE_L3mux_Data, ipTxFile,       sessionList,
            ipTxPktCounter,  tcpTxBytCounter, ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Role Interface
        //-------------------------------------------------
        pTRIF(
            testTxPath,       myIpAddress,
            appRxFile,        appTxFile,
            appTxBytCounter,
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            sTOE_Trif_Notif,  sTRIF_Toe_DReq,
            sTOE_Trif_Meta,   sTOE_Trif_Data,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
            sTRIF_Toe_ClsReq);

        // TODO
        if (!sTOE_Trif_DSts.empty()) {
            ap_uint<17> tempResp = sTOE_Trif_DSts.read();
            if (tempResp == -1 || tempResp == -2)
                cerr << endl << "Warning: Attempt to write data into the Tx App I/F of the TOE was unsuccesfull. Returned error code: " << tempResp << endl;
        }

        //------------------------------------------------------
        //-- STEP-6 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (gTraceEvent) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

    } while (gSimCycCnt < gMaxSimCycles);


    printf("#####################################################\n");
    printf("## TESTBENCH ENDS HERE                             ##\n");
    printf("#####################################################\n");

    //---------------------------------------------------------------
    //-- PRINT AN OVERALL TESTBENCH STATUS
    //---------------------------------------------------------------
    printInfo(THIS_NAME, "Number of sessions opened by TOE  : %6d \n", opnSessionCount.to_uint());
    printInfo(THIS_NAME, "Number of sessions closed by TOE  : %6d \n", clsSessionCount.to_uint());

    printInfo(THIS_NAME, "Number of IP  Packets rcvd by TOE : %6d \n", ipRxPktCounter);
    printInfo(THIS_NAME, "Number of IP  Packets sent by TOE : %6d \n", ipRxPktCounter);

    printInfo(THIS_NAME, "Number of TCP Bytes   rcvd by TOE : %6d \n", tcpRxBytCounter);
    printInfo(THIS_NAME, "Number of APP Bytes   sent by TOE : %6d \n", appTxBytCounter);

    printInfo(THIS_NAME, "Number of TCP Bytes   sent by TOE : %6d \n", tcpTxBytCounter);

    printf("\n");
    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------
    if (mode == RX_MODE) {
        if (tcpRxBytCounter != appTxBytCounter) {
            printError(THIS_NAME, "The number of TCP bytes received by TOE on its IP interface (%d) does not match the number TCP bytes forwarded by TOE to the application over its TRIF interface (%d). \n", tcpRxBytCounter, appTxBytCounter);
            nrErr++;
        }

        int appTxCompare = system(("diff --brief -w " + std::string(appTxFileName) + " " + std::string(appTxGoldName) + " ").c_str());
        if (appTxCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", appTxFileName, appTxGoldName);
            nrErr++;
        }
    }

    if ((mode == RX_MODE) || (mode == BIDIR_MODE)) {
        // Rx side testing only
        ipRxFile.close();
        appTxFile.close();
        appTxGold.close();
    }

    if ((mode == TX_MODE) || (mode == BIDIR_MODE)) {
        // Tx side testing only
        appRxFile.close();
        ipTxFile.close();
        ipTxGold.close();
    }

    if (nrErr)
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %d ####\n\n", nrErr);
    else {
        printInfo(THIS_NAME, "########################################\n");
        printInfo(THIS_NAME, "####     SUCCESSFUL END OF TEST     ####\n");
        printInfo(THIS_NAME, "########################################\n");
    }

    return nrErr;
}
