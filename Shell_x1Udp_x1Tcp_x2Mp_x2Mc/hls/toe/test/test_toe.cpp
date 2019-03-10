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

#include <iostream>
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
#define TB_GRACE_TIME       10

//---------------------------------------------------------
//-- DEFAULT LOCAL AND FOREIGN SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_LOCAL_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_LOCAL_TCP_PORT   0x0057      // TOE listens on port     = 87 (static  ports must be     0..32767)
#define DEFAULT_FOREIGN_IP4_ADDR 0x0A0A0A0A  // TB's foreign IP Address = 10.10.10.10
#define DEFAULT_FOREIGN_TCP_PORT 0x8000      // TB listens on port      = 32768 (dynamic ports must be 32768..65535)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gMaxSimCycles   = 100;
TbSockAddr      gLocalSocket(DEFAULT_LOCAL_IP4_ADDR, DEFAULT_LOCAL_TCP_PORT);
TbSockAddr      gForeignSocket(DEFAULT_FOREIGN_IP4_ADDR, DEFAULT_FOREIGN_TCP_PORT);
bool            gTraceEvent     = false;


/*****************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,     the socket address to display.
 *****************************************************************************/
void printTbSockAddr(const char *callerName, TbSockAddr sockAddr)
{
    printInfo(callerName, "SocketAddr {Addr,Port} = {%3.3d.%3.3d.%3.3d.%3.3d, %5.5d} \n",
        (sockAddr.addr & 0xFF000000) >> 24,
        (sockAddr.addr & 0x00FF0000) >> 16,
        (sockAddr.addr & 0x0000FF00) >>  8,
        (sockAddr.addr & 0x000000FF) >>  0, sockAddr.port);
}

/*****************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,     the socket pair to display.
 *****************************************************************************/
void printTbSockPair(const char *callerName, TbSocketPair sockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {{%3.3d.%3.3d.%3.3d.%3.3d,%5.5d},{%3.3d.%3.3d.%3.3d.%3.3d,%5.5d}} \n",
        (sockPair.src.addr & 0xFF000000) >> 24,
        (sockPair.src.addr & 0x00FF0000) >> 16,
        (sockPair.src.addr & 0x0000FF00) >>  8,
        (sockPair.src.addr & 0x000000FF) >>  0, sockPair.src.port,
        (sockPair.dst.addr & 0xFF000000) >> 24,
        (sockPair.dst.addr & 0x00FF0000) >> 16,
        (sockPair.dst.addr & 0x0000FF00) >>  8,
        (sockPair.dst.addr & 0x000000FF) >>  0, sockPair.dst.port);
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
            if (lut[j] == toupper(dataString[i])) {
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
            if (lut[j] == toupper(keepString[i])) {
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
    int              tokenCounter = 0;
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

    // Search for space characters delimiting the different data words
    while (strBuff.find(" ") != string::npos) {
        // Split the string in two parts
        string temp = strBuff.substr(0, strBuff.find(" "));
        // Remove first element from 'strBuff'
        strBuff = strBuff.substr(strBuff.find(" ")+1, strBuff.length());
        // Store the new part into the vector.
        if (temp != "")
            tmpBuff.push_back(temp);
        // Quit if the current line is a comment
        if ((tokenCounter == 0) && (temp =="#"))
            break;
    }

    // Push the final part of the string into the vector when no more spaces are present.
    tmpBuff.push_back(strBuff);
    return tmpBuff;
}


/*****************************************************************************
 * @brief Parse the input test file and set the global parameters of the TB.
 *
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
 *
 * @ingroup toe
 ******************************************************************************/
bool setGlobalParameters(ifstream &inputFile)
{
    const char *myName  = concat3(THIS_NAME, "/TRIF_Send/", "setGlobalParameters");

    string              rxStringBuffer;
    vector<string>      stringVector;

    do {
        //-- READ ONE LINE AT A TIME FROM INPUT FILE ---------------
        getline(inputFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer);

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            // By convention, a global parameter must start with a single 'G' character.
            if ((stringVector[0] == "G") && (stringVector[1] == "PARAM")) {
                if (stringVector[2] == "SimCycles") {
                    // The test vector file is specifying a minimum number of simulation cycles.
                    int noSimCycles = atoi(stringVector[3].c_str());
                    if (noSimCycles > gMaxSimCycles)
                        gMaxSimCycles = noSimCycles;
                    printInfo(myName, "Requesting the simulation to last for %d cycles. \n", gMaxSimCycles);
                }
                else if (stringVector[2] == "LocalSocket") {
                    char * ptr;
                    unsigned int ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                    gLocalSocket.addr = ip4Addr;
                    unsigned int tcpPort = strtoul(stringVector[4].c_str(), &ptr, 16);
                    gLocalSocket.port = tcpPort;
                    printInfo(myName, "Creating local socket <0x%8.8X, 0x%4.4X>.\n", ip4Addr, tcpPort);
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

    // Seek back to the start of stream
    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    return true;

} // End of: setGlopbalParameters


/*****************************************************************************
 * @brief Write the TCP data part of an IP packet into a file.
 *
 * @param[in]   outFile,  a ref to the gold file to write.
 * @param[in]   ipPacket, a ref to an IP RX packet.
 *
 * @ingroup test_toe
 ******************************************************************************/
void writeTcpDataToFile(
        ofstream    &outFile,
        IpPacket    &ipPacket)
{
    if(ipPacket.sizeOfTcpData() > 0) {
        string tcpData = ipPacket.getTcpData();
        if (tcpData.size() > 0)
            outFile << tcpData << endl;
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
    apTx_TcpBytCntr++;
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
int pIPRX_InjectAckNumber(
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
                printTbSockPair(myName, newSockPair);
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
            //OBSOLETE-20190130 ipRxPacket.setTcpChecksum(newTcpCsum);
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
 *  chunks intended for the IPRX interface of the TOE. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX_FeedTOE(
        deque<IpPacket>               &ipRxPacketizer,
        int                           &ipRxPktCounter,
        stream<Ip4overAxi>            &soTOE_Data,
        map<TbSocketPair, TcpAckNum>  &sessionList)
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
 * @param[in]  appTxGold,      A ref to the [TODO]
 * @param[in]  testRxPath,      Indicates if the test of the Rx path is enabled.
 * @param[i/o] ipRxPktCounter, A ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
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
        ifstream                      &ipRxFile,
        ofstream                      &appTxGold,
        bool                          &testRxPath,
        int                           &ipRxPktCounter,
        int                           &ipRx_TcpBytCntr,
        deque<IpPacket>               &ipRxPacketizer,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        stream<Ip4overAxi>            &soTOE_Data)
{
    static bool         globParseDone  = false;
    static bool         ipRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int ipRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int ipRxIdleCycCnt = 0;     // The count of idle cycles

    // Keep track of the current active local socket
     static TbSockAddr   currLocalSocket(DEFAULT_LOCAL_IP4_ADDR,
                                         DEFAULT_LOCAL_TCP_PORT);

    string              rxStringBuffer;
    vector<string>      stringVector;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    //-------------------------------------------------------------------------
    //-- STEP-0: PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(ipRxFile);
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
    //-- STEP-3: QUIT HERE IF RX TEST MODE IS DISABLED OR EOF IS REACHED
    //-------------------------------------------------------------------------
    if ((not testRxPath) || (ipRxFile.eof()))
        return;

    //------------------------------------------------------
    //-- STEP-? : [TODO] CHECK IF CURRENT LOCAL SOCKET IS LISTENING
    //------------------------------------------------------
    // TbSocketPair  currSocketPair(gLocalSocket, currForeignSocket);
    // isOpen = pTRIF_OpenSess(currSocketPair, openSessList, soTOE_OpnReq, siTOE_OpnSts);
    // if (!isOpen)
    //     return;

    //-----------------------------------------------------
    //-- STEP-4 : READ THE IP RX FILE
    //-----------------------------------------------------
    do {
        //-- READ A LINE FROM IPRX INPUT FILE -------------
        getline(ipRxFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer);

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
            ipRx_TcpBytCntr += ipRxPacket.sizeOfTcpData();

            // Write to the Application Tx Gold file
            writeTcpDataToFile(appTxGold, ipRxPacket);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessionList); // [FIXME-Can be removed?]

            return;
        }

    } while(!ipRxFile.eof());

} // End of: pIPRX


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
bool pL3MUX_Parse(
        IpPacket                      &ipTxPacket,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        deque<IpPacket>               &ipRxPacketizer)
{
    bool        returnValue    = false;
    bool        isFinAck       = false;
    bool        isSynAck       = false;
    //OBSOLETE-20190104 static int  ipTxPktCounter = 0;
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

        IpPacket synAckPacket;
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

            // Set the Window size
            ackPacket.setTcpWindow(7777);

            // Recalculate the Checksum
            int newTcpCsum = ackPacket.recalculateChecksum();
            ackPacket.setTcpChecksum(newTcpCsum);

            // Add the created ACK packet to the ipRxPacketizer
            ipRxPacketizer.push_back(ackPacket);

            //OBSOLETE-20190104 // Increment the packet counter
            //OBSOLETE-20190104 if (currAckNum != nextAckNum) {
            //OBSOLETE-20190104     ipTxPktCounter++;
            //OBSOLETE-20190104     if (DEBUG_LEVEL & TRACE_L3MUX) {
            //OBSOLETE-20190104         printInfo(myName, "IP Tx Packet Counter = %d \n", ipTxPktCounter);
            //OBSOLETE-20190104     }
            //OBSOLETE-20190104 }
            currAckNum = nextAckNum;
        }
    }


    return returnValue;

} // End of: parseL3MuxPacket()


/*****************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]  siTOE_Data,      A reference to the data stream from TOE.
 * @param[in]  ipTxFile1,       The output file to write.
 * @param[in]  ipTxFile2,       The output file to write.
 * @param[in]  sessionList,     A ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[i/o] ipTx_PktCounter, A ref to the packet counter on the IP Tx I/F.
 *                               (counts all kinds and from all sessions).
 * @param[i/o] ipTx_TcpBytCntr, A ref to the TCP byte counter on the IP Tx I/F.
 * @param[out] ipRxPacketizer,  A ref to the IPv4 Rx packetizer.
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
        ofstream                      &ipTxFile1,
        ofstream                      &ipTxFile2,
        map<TbSocketPair, TcpAckNum>  &sessionList,
        int                           &ipTx_PktCounter,
        int                           &ipTx_TcpBytCntr,
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
            if (pL3MUX_Parse(ipTxPacket, sessionList, ipRxPacketizer) == true) {
                // Found an ACK
                ipTx_PktCounter++;
                int tcpPayloadSize = ipTxPacket.sizeOfTcpData();
                if (tcpPayloadSize) {
                    ipTx_TcpBytCntr += tcpPayloadSize;
                    // Write to the IP Tx Gold file
                    writeTcpDataToFile(ipTxFile2, ipTxPacket);
                }
            }
            // Clear the word counter and the received IP packet
            ipTxWordCounter = 0;
            ipTxPacket.clear();
        }
        else
            ipTxWordCounter++;

        //-- STEP-4 : Write to file ------------- ---------
        string dataOutput = decodeApUint64(ipTxWord.tdata);
        string keepOutput = decodeApUint8(ipTxWord.tkeep);
        ipTxFile1 << dataOutput << " " << ipTxWord.tlast << " " << keepOutput << endl;
    }
} // End of: pL3MUX


/*****************************************************************************
 * @brief Write a TCP AXI word into a file.
 *
 * @param[in]  outFile, a ref to the file to write.
 * @param[in]  tcpWord, a ref to the AXI word to write.
 *
 * @return the number of bytes written into the file.
 *
 * @ingroup test_toe
 ******************************************************************************/
int writeTcpWordToFile(
        ofstream    &outFile,
        AxiWord     &tcpWord)
{
    string tdataToFile = "";
    int writtenBytes = 0;

    for (int bytNum=0; bytNum<8; bytNum++) {
        if (tcpWord.tkeep.bit(bytNum)) {
            int hi = ((bytNum*8) + 7);
            int lo = ((bytNum*8) + 0);
            ap_uint<8>  octet = tcpWord.tdata.range(hi, lo);
            tdataToFile += toHexString(octet);
            writtenBytes++;
        }
    }

    if (tcpWord.tlast == 1)
        outFile << tdataToFile << endl;
    else
        outFile << tdataToFile;

    return writtenBytes;
}


/*****************************************************************************
 * @brief Start listening on new port(s). The port numbers correspond to the
 *         ones opened by the TOE.
 *
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 *
 * @return true if listening was successful, otherwise false.
 *
 * @ingroup test_toe
 ******************************************************************************/
bool pTRIF_Listen(
        stream<AppLsnReq>  &soTOE_LsnReq,
        stream<AppLsnAck>  &siTOE_LsnAck)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF/Listen()");

    static ap_uint<1> listenFsm     =   0;
    static int        watchDogTimer = 100;

    bool              listenDone  = false;

    TcpPort portNum = gLocalSocket.port;  // The port # to listen to.

    switch (listenFsm) {
    case 0:
        soTOE_LsnReq.write(portNum);
        if (DEBUG_LEVEL & TRACE_TRIF) {
            printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                      portNum.to_uint(), portNum.to_uint());
        }
        watchDogTimer = 100;
        listenFsm++;
        break;

    case 1:
        watchDogTimer--;
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
            listenFsm = 0;
        }
        else {
            if (watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to listen on port %d %d (0x%4.4X).\n",
                           portNum.to_uint(), portNum.to_uint());
                listenFsm = 0;
            }
        }
        break;
    }
    return listenDone;
}


/*****************************************************************************
 * @brief Open a new session.
 *
 * @param[in]  aSocketPair,   The socket pair of the session to open.
 * @param[in]  openSessList,  A ref to an associative container that holds the
 *                             IDs of the opened sessions.
 * @param[out] soTOE_OpnReq,  TCP open connection request to TOE.
 * @param[in]  siTOE_OpnSts,  TCP open connection status from TOE.
 *
 * @return true if the session is or was successfully opened, otherwise false.
 *
 * @details:
 *  The max number of connections that can be opened is given by 'noTxSessions'.
 *
 * @ingroup test_toe
 ******************************************************************************/
bool pTRIF_OpenSess(
        TbSocketPair                  &aSocketPair,
        map<TbSocketPair, SessionId>  &openSessList,
        stream<AxiSockAddr>           &soTOE_OpnReq,
        stream<OpenStatus>            &siTOE_OpnSts)
{
    static int noOpenSess    =   0;
    static int watchDogTimer = 100;

    bool rc = false;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF/OpenSess()");

    // Check if a session exists for this socket-pair
    if (openSessList.find(aSocketPair) != openSessList.end()) {
        // A session exists;
        return true;
    }

    // Check maximum number of opened sessions
    if (noOpenSess >= NO_TX_SESSIONS) {
        printError(myName, "Trying to open too many sessions. Max. is %d.\n", NO_TX_SESSIONS);
        exit(1);
    }

    // Assess that the port number falls in the dynamic port range
    if (aSocketPair.dst.port < 0x8000) {
        printError(myName, "Port #%d is not a dynamic port (.i.e, in the range 32768..65535).\n", aSocketPair.dst.port);
        exit(1);
    }

    // Prepare to open connection
    AxiSockAddr axiForeignSockAddr(AxiSockAddr(byteSwap32(aSocketPair.dst.addr),
                                               byteSwap16(aSocketPair.dst.port)));
    static int openFsm = 0;

    switch (openFsm) {

    case 0:
        soTOE_OpnReq.write(axiForeignSockAddr);
        if (DEBUG_LEVEL & TRACE_TRIF) {
            printInfo(myName, "Request to open the following socket: \n");
            printTbSockPair(myName, aSocketPair);
        }
        watchDogTimer = 100;
        openFsm++;
        break;

    case 1:
        watchDogTimer--;
        if (!siTOE_OpnSts.empty()) {
            OpenStatus openConStatus = siTOE_OpnSts.read();
            if(openConStatus.success) {
                // Update the list of opened sessions with the new ID
                openSessList[aSocketPair] = openConStatus.sessionID;
                if (DEBUG_LEVEL & TRACE_TRIF) {
                    printInfo(myName, "Session #%d is now opened.\n", openConStatus.sessionID.to_uint());
                    printTbSockPair(myName, aSocketPair);
                }
                noOpenSess++;
                rc = true;
            }
            else {
                printError(myName, "Failed to to open session #%d.\n", openConStatus.sessionID.to_uint());
            }
            openFsm = 0;
        }
        else {
            if (watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to open the following socket:\n");
                printTbSockPair(myName, aSocketPair);
                openFsm = 0;
            }
        }
        break;

    } // End Of: switch

    return rc;

}


/*****************************************************************************
 * @brief Open new socket(s) and setup connection(s).
 *
 * @param[in]  toeSockAddr,   The local socket address of the TOE.
 * @param[out] txSessIdVector,A vector containing the generated Tx session IDs.
 * @param[out] soTOE_OpnReq,  TCP open connection request to TOE.
 * @param[in]  siTOE_OpnSts,  TCP open connection status from TOE.
 *
 * @return true if a socket was successfully opened, otherwise false. The
 *
 * @details:
 *  The number of connections to open is given by 'noTxSessions'.
 *
 * @ingroup test_toe
 ******************************************************************************/
/*** OBSOLETE-20190125 *************************
bool pTRIF_OpenConOld(
        TbSockAddr             &toeSockAddr,
        vector<SessionId>      &txSessIdVector,
        stream<AxiSockAddr>    &soTOE_OpnReq,
        stream<OpenStatus>     &siTOE_OpnSts)
{
    static int noOpenCon = 0;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF/OpenCon()");

    if (noOpenCon < NO_TX_SESSIONS) {
        // Define the socket pair involved in the connection
        TbSockAddr   tbSockAddr(gForeignSocket.addr + noOpenCon,
                                gForeignSocket.port + noOpenCon);
        // Assess that the port number falls in the dynamic ports range
        if (tbSockAddr.port < 0x8000) {
            printError(myName, "Port #%d is not a dynamic port (.i.e, in the range 32768..65535).\n");
            return false;
        }

        TbSocketPair socketPair(toeSockAddr, tbSockAddr);
        AxiSockAddr axiForeignSockAddr(AxiSockAddr(byteSwap32(tbSockAddr.addr),
                                                   byteSwap16(tbSockAddr.port)));
        static int openFsm = 0;

        switch (openFsm) {

        case 0:
            soTOE_OpnReq.write(axiForeignSockAddr);
            if (DEBUG_LEVEL & TRACE_TRIF) {
                printInfo(myName, "Request to open the following socket: \n");
                //OBSOLETE-20190115 printAxiSockAddr(myName, axiForeignSockAddr);
                printTbSockAddr(myName, tbSockAddr);
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
                noOpenCon++;
                openFsm++;
            }
            break;
        }
        return false;
    }
    else {
        return true;
    }
}
************************************************/

/*****************************************************************************
 * @brief Emulates behavior of the receive half of TCP Role Interface (TRIF).
 *
 * @param[in]  appTxFile,    A ref to the output Tx application file to write.
 * @param[i/o] apTx_TcpBytCntr, A ref to the counter of bytes on the APP Tx I/F.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,  TCP notification from TOE.
 * @param[out] soTOE_DReq,   TCP data request to TOE.
 * @param[in]  siTOE_Meta,   TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,   TCP data stream from TOE.
 *
 * @details:
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF_Recv(
        ofstream                &appTxFile,
        int                     &apTx_TcpBytCntr,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<AppMeta>         &siTOE_Meta,
        stream<AppData>         &siTOE_Data)
{
    static bool         listenDone         = false;
    static bool         openDone           = false;
    static bool         runningExperiment  = false;
    static SessionId    currTxSessionID    = 0;  // The current Tx session ID

    static bool         appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int appRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int appRxIdleCycCnt = 0;     // The count of idle cycles

    static vector<SessionId> txSessIdVector;     // A vector containing the Tx session IDs to be send from TRIF/Meta to TOE/Meta
    static int        startupDelay = TB_GRACE_TIME;

    string              rxStringBuffer;
    vector<string>      stringVector;

    OpenStatus          newConStatus;
    AppNotif     notification;
    ipTuple             tuple;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Recv");

    //---------------------------------------------------------------
    //-- STEP-0 : Give this process some grace time before starting.
    //---------------------------------------------------------------
    if (startupDelay) {
        startupDelay--;
        return;
    }

    //------------------------------------------------
    //-- STEP-1 : REQUEST TO LISTEN ON A PORT
    //------------------------------------------------
    if (!listenDone) {
        listenDone = pTRIF_Listen(soTOE_LsnReq, siTOE_LsnAck);
        return;
    }

    //------------------------------------------------
    //-- STEP-2 : READ NOTIFICATION
    //------------------------------------------------
    if (!siTOE_Notif.empty()) {
        siTOE_Notif.read(notification);
        if (notification.tcpSegLen != 0)
            soTOE_DReq.write(AppRdReq(notification.sessionID,
                                      notification.tcpSegLen));
        else // closed
            runningExperiment = false;
    }

    //------------------------------------------------
    //-- STEP-3 : DRAIN TOE-->TRIF
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
            apTx_TcpBytCntr += writeTcpWordToFile(appTxFile, currWord);

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
            writeApplicationTxFile(appTxFile, currWord, apTx_TcpBytCntr);

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
            writeApplicationTxFile(appTxFile, currWord, apTx_TcpBytCntr);

            mAmount = currWord.tdata(63, 32);
            fsmState = CONSUME;
        }
        break;
    **************/

    case CONSUME:
        if (!siTOE_Data.empty()) {
            // Read all the remaining TCP data chunks
            siTOE_Data.read(currWord);
            apTx_TcpBytCntr += writeTcpWordToFile(appTxFile, currWord);
        }
        break;
    }

    // Intercept the cases where TCP payload in less that 4 chunks
    if (currWord.tlast == 1)
        fsmState = WAIT_SEG;

} // End of: pTRIF_Recv


/*****************************************************************************
 * @brief Emulates behavior of the send half of the TCP Role Interface (TRIF).
 *
 * @param[in]  testTxPath,   Indicates if the Tx path is to be tested.
 * @param[in]  myIpAddress,  The local IP address used by the TOE.
 * @param[in]  appRxFile,    A ref to the input Rx application file to read.
 * @param[in]  ipTxGoldFile, A ref to the output IP Tx gold file to write.
 * @param[i/o] apRx_TcpBytCntr, A ref to the counter of bytes on the APP Rx I/F.
 * @param[out] soTOE_OpnReq, TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts, TCP open port status from TOE.
 * @param[out] soTOE_Meta,   TCP metadata stream to TOE.
 * @param[out] soTOE_Data,   TCP data stream to TOE.
 * @param[out] soTOE_ClsReq, TCP close connection request to TOE.
 *
 * @details:
 *  This process maintains a list of opened foreign sockets.
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF_Send(
        bool                    &testTxPath,
        Ip4Address              &myIpAddress,
        ifstream                &appRxFile,
        ofstream                &ipTxGoldFile,
        int                     &apRx_TcpBytCntr,
        stream<AxiSockAddr>     &soTOE_OpnReq,
        stream<OpenStatus>      &siTOE_OpnSts,
        stream<SessionId>       &soTOE_Meta,
        stream<AxiWord>         &soTOE_Data,
        stream<ap_uint<16> >    &soTOE_ClsReq)
{
    //OBSOLETE-20190123 static bool         openDone          = false;
    static bool         globParseDone     = false;
    //OBSOLETE-20190123 static SessionId    currTxSessionID   = 0;  // The current Tx session ID

    // Keep track of the current active foreign socket
    static TbSockAddr   currForeignSocket(DEFAULT_FOREIGN_IP4_ADDR,
                                          DEFAULT_FOREIGN_TCP_PORT);
    // Keep track of the opened sessions
    static map<TbSocketPair, SessionId>  openSessList;

    static bool         appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int appRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int appRxIdleCycCnt = 0;     // The count of idle cycles

    static vector<SessionId> txSessIdVector;     // A vector containing the Tx session IDs to be send from TRIF/Meta to TOE/Meta

    string              rxStringBuffer;
    vector<string>      stringVector;
    OpenStatus          newConStatus;
    bool                isOpen;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Send");

    //-------------------------------------------------------------------------
    //-- STEP-0 : IMMEDIATELY QUIT IF TX TEST MODE IS NOT ENABLED
    //-------------------------------------------------------------------------
    if (not testTxPath)
        return;

    //-------------------------------------------------------------------------
    //-- STEP-1 : PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(appRxFile);
        if (globParseDone == false) {
            printInfo(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (appRxIdlingReq == true) {
        if (appRxIdleCycCnt >= appRxIdleCycReq) {
            appRxIdleCycCnt = 0;
            appRxIdlingReq = false;
            printInfo(myName, "End of APP Rx idling phase. \n");
        }
        else {
            appRxIdleCycCnt++;
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-3 : RETURN IF END OF FILE IS REACHED
    //-----------------------------------------------------
    if (appRxFile.eof())
        return;

    //------------------------------------------------------
    //-- STEP-4 : CHECK IF CURRENT FOREIGN SOCKET IS OPENED
    //------------------------------------------------------
    TbSocketPair  currSocketPair(gLocalSocket, currForeignSocket);
    isOpen = pTRIF_OpenSess(currSocketPair, openSessList, soTOE_OpnReq, siTOE_OpnSts);
    if (!isOpen)
        return;

    //-----------------------------------------------------
    //-- STEP-4 : READ THE APP RX FILE AND FEED THE TOE
    //-----------------------------------------------------
    do {
        //-- READ A LINE FROM APP RX FILE -------------
        getline(appRxFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer);

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
                // This is a global parameter. It can be skipped because it was already parsed.
                continue;
            }
            else if (stringVector[0] == ">") {
                // The test vector is issuing a command.
                //  FYI, don't forget to return at the end of command execution.
                if (stringVector[1] == "IDLE") {
                    // Cmd = Request to idle for <NUM> cycles.
                    appRxIdleCycReq = atoi(stringVector[2].c_str());
                    appRxIdlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", appRxIdleCycReq);
                    return;
                }
                if (stringVector[1] == "SET") {
                    // Cmd = Set a new foreign socket.
                    if (stringVector[2] == "ForeignSocket") {
                        char * ptr;
                        unsigned int ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                        currForeignSocket.addr = ip4Addr;
                        unsigned int tcpPort = strtoul(stringVector[4].c_str(), &ptr, 16);
                        currForeignSocket.port = tcpPort;
                        printInfo(myName, "Setting foreign socket to <0x%8.8X, 0x%4.4X>.\n", ip4Addr, tcpPort);
                        return;
                    }
                }
            }
            else {
                printError(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[0].c_str());
                exit(1);
            }
        }
        else if (appRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            //-------------------------------------
            //-- Feed the TOE with data from file
            //-------------------------------------
            AxiWord appRxData;
            bool    firstWordFlag = true; // AXI-word is first data chunk of segment

            do {
                if (firstWordFlag == false) {
                    getline(appRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer);
                }
                else {
                    // A Tx data request (i.e. a metadata) must be sent by TRIF to TOE
                    soTOE_Meta.write(openSessList[currSocketPair]);
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                appRxData = AxiWord(encodeApUint64(stringVector[0]), \
                                    encodeApUint8(stringVector[2]),  \
                                    atoi(stringVector[1].c_str()));
                soTOE_Data.write(appRxData);

                // Write current word to the gold file
                apRx_TcpBytCntr += writeTcpWordToFile(ipTxGoldFile, appRxData);

            } while (appRxData.tlast != 1);

        } // End of: else

    } while(!appRxFile.eof());


/*** OBSOLETE-20190123 *********************************************
     //------------------------------------------------
    //-- STEP-2 : REQUEST TO OPEN CONNECTION(S)
    //------------------------------------------------
    //OBSOLETE-20190123 TcpPort    listeningPort = gLocalTcpOpn;
    //OBSOLETE-20190123 TbSockAddr toeSockAddr(myIpAddress, listeningPort);
    if (!openDone) {
        openDone = pTRIF_OpenCon(gLocalSocket, txSessIdVector, soTOE_OpnReq, siTOE_OpnSts);
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : READ THE APP RX FILE AND FEED THE TOE
    //-----------------------------------------------------
    // Only if a session has been opened on the Tx Side
    if (txSessIdVector.size() > 0) {

        //-- STEP-2.0 : RETURN IF IDLING IS REQUESTED -----
        if (appRxIdlingReq == true) {
            if (appRxIdleCycCnt >= appRxIdleCycReq) {
                appRxIdleCycCnt = 0;
                appRxIdlingReq = false;
                printInfo(myName, "End of APP Rx idling phase. \n");
            }
            else {
                appRxIdleCycCnt++;
            }
            return;
        }

        //-- STEP-2.1 : QUIT HERE IF TX TEST MODE IS NOT ENABLED
        if (not testTxPath)
            return;

        //-- STEP-2.2 : QUIT HERE IF END OF FILE IS REACHED
        if (appRxFile.eof())
             return;

        //-- STEP-2.2 : READ THE APP RX FILE --------------
        do {
            //-- READ A LINE FROM APP RX FILE -------------
            getline(appRxFile, rxStringBuffer);

            stringVector = myTokenizer(rxStringBuffer);

            if (stringVector[0] == "") {
                continue;
            }
            else if (stringVector[0].length() == 1) {
                // WARNING:
                //  A comment must start with a hash symbol followed by a space character.
                //  A command must start with an upper character followed by a space character.
                if (stringVector[0] == "#") {
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
                    if (stringVector[1] == "IDLE") {
                        // Cmd = Request to idle for <NUM> cycles.
                        appRxIdleCycReq = atoi(stringVector[1].c_str());
                        appRxIdlingReq = true;
                        printInfo(myName, "Request to idle for %d cycles. \n", appRxIdleCycReq);
                        return;
                    }
                }
                else {
                    printError(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[0].c_str());
                    exit(1);
                }
            }
            else if (appRxFile.fail() == 1 || rxStringBuffer.empty()) {
                return;
            }
            else {
                //-- Feed the TOE with data from file
                AxiWord appRxData;
                bool    firstWordFlag = true; // AXI-word is first data chunk of segment

                do {
                    if (firstWordFlag == false) {
                        getline(appRxFile, rxStringBuffer);
                        stringVector = myTokenizer(rxStringBuffer);
                    }
                    else {
                        // This is the first chunk of a frame.
                        // A Tx data request (i.e. a metadata) must be sent by TRIF to TOE
                        soTOE_Meta.write(txSessIdVector[currTxSessionID]);
                        if (currTxSessionID == (NO_TX_SESSIOSNS - 1))
                            currTxSessionID = 0;
                        else
                            currTxSessionID++;
                    }
                    firstWordFlag = false;
                    string tempString = "0000000000000000";
                    appRxData = AxiWord(encodeApUint64(stringVector[0]), \
                                        encodeApUint8(stringVector[2]),  \
                                        atoi(stringVector[1].c_str()));
                    soTOE_Data.write(appRxData);

                    // Write current word to the gold file
                    apRx_TcpBytCntr += writeTcpWordToFile(ipTxGoldFile, appRxData);

                } while (appRxData.tlast != 1);

            } // End of: else

        } while(!appRxFile.eof());

    } // End of: if (txSessIdVector.size() > 0) {
 *******************************************************************/

} // End of: pTRIF_Send


/*****************************************************************************
 * @brief Emulates the behavior of the TCP Role Interface (TRIF).
 *             This process implements Iperf [FIXME].
 *
 * @param[in]  testTxPath,   Indicates if the Tx path is to be tested.
 * @param[in]  myIpAddress,  The local IP address used by the TOE.
 * @param[in]  appRxFile,    A ref to the input Rx application file to read.
 * @param[in]  appTxFile,    A ref to the output Tx application file to write.
 * @param[in]  ipTxGoldFile, A ref to the output IP Tx gold file to write.
 * @param[i/o] apRx_TcpBytCntr, A ref to the counter of bytes on the APP Rx I/F.
 * @param[i/o] apTx_TcpBytCntr, A ref to the counter of bytes on the APP Tx I/F.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,  TCP notification from TOE.
 * @param[out] soTOE_DReq,   TCP data request to TOE.
 * @param[in]  siTOE_Meta,   TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,   TCP data stream from TOE.
 * @param[out] soTOE_OpnReq, TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts, TCP open port status from TOE.
 * @param[out] soTOE_Meta,   TCP metadata stream to TOE.
 * @param[out] soTOE_Data,   TCP data stream to TOE.
 * @param[out] soTOE_ClsReq, TCP close connection request to TOE.
 *
 * @details:
 *
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
        ofstream                &ipTxGoldFile,
        int                     &apRx_TcpBytCntr,
        int                     &apTx_TcpBytCntr,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<AppMeta>         &siTOE_Meta,
        stream<AppData>         &siTOE_Data,
        stream<AppOpnReq>       &soTOE_OpnReq,
        stream<AppOpnSts>       &siTOE_OpnSts,
        stream<AppMeta>         &soTOE_Meta,
        stream<AppData>         &soTOE_Data,
        stream<AppClsReq>       &soTOE_ClsReq)
{

    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    pTRIF_Recv(
            appTxFile,
            apTx_TcpBytCntr,
            soTOE_LsnReq, siTOE_LsnAck,
            siTOE_Notif,  soTOE_DReq,
            siTOE_Meta,   siTOE_Data);

    pTRIF_Send(
            testTxPath,
            myIpAddress,
            appRxFile,
            ipTxGoldFile,
            apRx_TcpBytCntr,
            soTOE_OpnReq, siTOE_OpnSts,
            soTOE_Meta,
            soTOE_Data,
            soTOE_ClsReq);

} // End of: pTRIF


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode,       the test mode (0=RX_MODE, 1=TX_MODE, 2=BIDIR_MODE).
 * @param[in]  inpFile1,   the pathname of the input file containing the test
 *                          vectors to be fed to the TOE:
 *                          If (mode==0 || mode=2)
 *                            inpFile1 = ipRxFile
 *                          Else
 *                            inpFile1 = appRxFile.
 * @param[in]  inpFile2,   the pathname of the second input file containing the
 *                         test vectors to be fed to the TOE:
 *                            inpFile2 == appRxFile.
 * @remark:
 *  The number of input parameters is variable and depends on the testing mode.
 *   Example (see also file '../run_hls.tcl'):
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

    stream<AppData>                     sTRIF_Toe_Data      ("sTRIF_Toe_Data");
    stream<AppMeta>                     sTRIF_Toe_Meta      ("sTRIF_Toe_Meta");
    stream<AppWrSts>                    sTOE_Trif_DSts      ("sTOE_Trif_DSts");

    stream<AppRdReq>                    sTRIF_Toe_DReq      ("sTRIF_Toe_DReq");
    stream<AppData>                     sTOE_Trif_Data      ("sTOE_Trif_Data");
    stream<AppMeta>                     sTOE_Trif_Meta      ("sTOE_Trif_Meta");

    stream<AppLsnReq>                   sTRIF_Toe_LsnReq    ("sTRIF_Toe_LsnReq");
    stream<AppLsnAck>                   sTOE_Trif_LsnAck    ("sTOE_Trif_LsnAck");

    stream<AppOpnReq>                   sTRIF_Toe_OpnReq    ("sTRIF_Toe_OpnReq");
    stream<AppOpnSts>                   sTOE_Trif_OpnSts    ("sTOE_Trif_OpnSts");

    stream<AppNotif>                    sTOE_Trif_Notif     ("sTOE_Trif_Notif");

    stream<AppClsReq>                   sTRIF_Toe_ClsReq    ("sTRIF_Toe_ClsReq");

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

    stream<rtlSessionLookupRequest>     sTHIS_Cam_SssLkpReq ("sTHIS_Cam_SssLkpReq");
    stream<rtlSessionLookupReply>       sCAM_This_SssLkpRep ("sCAM_This_SssLkpRep");
    stream<rtlSessionUpdateRequest>     sTHIS_Cam_SssUpdReq ("sTHIS_Cam_SssUpdReq");
    stream<rtlSessionUpdateReply>       sCAM_This_SssUpdRep ("sCAM_This_SssUpdRep");

    //-- TESTBENCH MODES OF OPERATION ---------------------
    enum TestingMode {RX_MODE='0', TX_MODE='1', BIDIR_MODE='2'};

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    unsigned int    simCycCnt      = 0;
    int             nrErr;

    Ip4Word         ipRxData;    // An IP4 chunk
    AxiWord         tcpTxData;   // A  TCP chunk

    ap_uint<16>     opnSessionCount;
    ap_uint<16>     clsSessionCount;

    DummyMemory     rxMemory;
    DummyMemory     txMemory;

    map<TbSocketPair, TcpAckNum>    sessionList;

    //-- Double-ended queue of packets --------------------
    deque<IpPacket>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        ipRxFile;   // IP packets to         IPRX  I/F of TOE.
    ifstream        appRxFile;  // APP data streams to   TRIF  I/F of TOE.

    ofstream        appTxFile;  // APP byte streams from TRIF  I/F of TOE.
    ofstream        appTxGold;  // Gold reference file for 'appTxFile'
    ofstream        ipTxFile1;  // Raw IP packets from L3MUX I/F of TOE.
    ofstream        ipTxGold1;  // Gold reference file for 'ipTxFile1' (not used)
    ofstream        ipTxFile2;  // Tcp payload of the IP packets from L3MUX I/F of TOE.
    ofstream        ipTxGold2;  // Gold reference file for 'ipTxFile2'

    const char      *appTxFileName = "../../../../test/appTx_TOE.strm";
    const char      *appTxGoldName = "../../../../test/appTx_TOE.gold";
    const char      *ipTxFileName1 = "../../../../test/ipTx_TOE.dat";
    const char      *ipTxGoldName1 = "../../../../test/ipTx_TOE.gold";
    const char      *ipTxFileName2 = "../../../../test/ipTx_TOE_TcpData.strm";
    const char      *ipTxGoldName2 = "../../../../test/ipTx_TOE_TcpData.gold";

    //NotUsed int             rxGoldCompare  = 0;
    //NotUsed int             returnValue    = 0;
    //NotUsed unsigned int    myCounter      = 0;

    string          dataString, keepString;

    int             ipRx_PktCounter = 0;  // Counts the # IP packets rcvd by TOE/IpRx (all kinds and from all sessions).
    int             ipRx_TcpBytCntr = 0;  // Counts the # TCP bytes  rcvd by TOE/IpRx.

    int             ipTx_PktCounter = 0;  // Counts the # IP packets sent by TOE/IpTx (all kinds and from all sessions).
    int             ipTx_TcpBytCntr = 0;  // Counts the # TCP bytes  sent by TOE/IpTx.

    int             apRx_TcpBytCntr = 0;  // Counts the # TCP bytes  rcvd by TOE/AppRx.

    int             apTx_TcpBytCntr = 0;  // Counts the # TCP bytes  sent by TOE/AppTx.

    bool            testRxPath      = false; // Indicates if the Rx path is to be tested.
    bool            testTxPath      = false; // Indicates if the Tx path is to be tested.

    char            mode            = *argv[1];
    char            cCurrPath[FILENAME_MAX];

    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printf("## [TB-ERROR] Expected a minimum of 2 or 3 parameters with one of the the following synopsis:\n");
        printf("\t mode(0) ipRxFileName\n");
        printf("\t mode(1) appRxFileName\n");
        printf("\t mode(2) ipRxFileName appRxFileName\n");
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

        ipTxFile1.open(ipTxFileName1);
        if (!ipTxFile1) {
            printf("## [TB-ERROR] Cannot open the IP Tx file:  \n\t %s \n", ipTxFileName1);
            return -1;
        }
        ipTxFile2.open(ipTxFileName2);
        if (!ipTxFile2) {
            printf("## [TB-ERROR] Cannot open the IP Tx file:  \n\t %s \n", ipTxFileName2);
            return -1;
        }

        ipTxGold2.open(ipTxGoldName2);
        if (!ipTxGold2) {
            printf("## [TB-ERROR] Cannot open the IP Tx gold file:  \n\t %s \n", ipTxGoldName2);
            return -1;
        }
    }

    printf("############################################################################\n");
    printf("## TESTBENCH STARTS HERE                                                  ##\n");
    printf("############################################################################\n");
    printf("   FYI - LocalSocket = { 0x%8.8X, 0x%4.4X} \n\n", gLocalSocket.addr, gLocalSocket.port);
    simCycCnt = 0;     // Simulation cycle counter as a global variable
    nrErr     = 0;     // Total number of testbench errors

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- STEP-1 : Emulate the IPv4 Rx Path
        //-------------------------------------------------
        pIPRX(ipRxFile,       appTxGold,
              testRxPath,     ipRx_PktCounter, ipRx_TcpBytCntr,
              ipRxPacketizer, sessionList,     sIPRX_Toe_Data);

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        if (simCycCnt > TB_GRACE_TIME) {
            // Give the TB some grace time to do some of the required initializations
            toe(
                //-- From MMIO Interfaces
                (AxiIp4Addr)(byteSwap32(gLocalSocket.addr)),
                //-- IPv4 / Rx & Tx Interfaces
                sIPRX_Toe_Data,   sTOE_L3mux_Data,
                //-- TRIF / Tx Data Interfaces
                sTOE_Trif_Notif,  sTRIF_Toe_DReq,
                sTOE_Trif_Data,   sTOE_Trif_Meta,
                //-- TRIF / Listen Interfaces
                sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
                //-- TRIF / Rx Data Interfaces
                sTRIF_Toe_Data,   sTRIF_Toe_Meta,
                sTOE_Trif_DSts,
                //-- TRIF / Open Interfaces
                sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
                //-- TRIF / Close Interfaces
                sTRIF_Toe_ClsReq,
                //-- MEM / Rx PATH / S2MM Interface
                sTOE_Mem_RxP_RdCmd, sMEM_Toe_RxP_Data, sMEM_Toe_RxP_WrSts, sTOE_Mem_RxP_WrCmd, sTOE_Mem_RxP_Data,
                sTOE_Mem_TxP_RdCmd, sMEM_Toe_TxP_Data, sMEM_Toe_TxP_WrSts, sTOE_Mem_TxP_WrCmd, sTOE_Mem_TxP_Data,
                //-- CAM / This / Session Lookup & Update Interfaces
                sTHIS_Cam_SssLkpReq, sCAM_This_SssLkpRep,
                sTHIS_Cam_SssUpdReq, sCAM_This_SssUpdRep,
                //-- DEBUG / Session Statistics Interfaces
                clsSessionCount, opnSessionCount);
        }

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
            sTHIS_Cam_SssLkpReq, sCAM_This_SssLkpRep,
            sTHIS_Cam_SssUpdReq, sCAM_This_SssUpdRep);

        //------------------------------------------------------
        //-- STEP-4 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------

        //-------------------------------------------------
        //-- STEP-4.0 : Emulate Layer-3 Multiplexer
        //-------------------------------------------------
        pL3MUX(
            sTOE_L3mux_Data,
            ipTxFile1,       ipTxFile2,
            sessionList,
            ipTx_PktCounter, ipTx_TcpBytCntr,
            ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Role Interface
        //-------------------------------------------------
        Ip4Address myIpAddress = gLocalSocket.addr;
        pTRIF(
            testTxPath,       myIpAddress,
            appRxFile,        appTxFile,
            ipTxGold2,
            apRx_TcpBytCntr,  apTx_TcpBytCntr,
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            sTOE_Trif_Notif,  sTRIF_Toe_DReq,
            sTOE_Trif_Meta,   sTOE_Trif_Data,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
            sTRIF_Toe_Meta,   sTRIF_Toe_Data,
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
        simCycCnt++;
        if (gTraceEvent || ((simCycCnt % 100) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", simCycCnt);
            gTraceEvent = false;
        }

    } while (simCycCnt < gMaxSimCycles);

    printf("-- [@%4.4d] -----------------------------\n", simCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n");

    //---------------------------------------------------------------
    //-- PRINT AN OVERALL TESTBENCH STATUS
    //---------------------------------------------------------------
    printInfo(THIS_NAME, "Number of sessions opened by TOE       : %6d \n", opnSessionCount.to_uint());
    printInfo(THIS_NAME, "Number of sessions closed by TOE       : %6d \n", clsSessionCount.to_uint());

    printInfo(THIS_NAME, "Number of IP  Packets rcvd by TOE/IpRx : %6d \n", ipRx_PktCounter);

    printInfo(THIS_NAME, "Number of IP  Packets sent by TOE/IpTx : %6d \n", ipTx_PktCounter);

    printInfo(THIS_NAME, "Number of TCP Bytes   rcvd by TOE/IpRx : %6d \n", ipRx_TcpBytCntr);
    printInfo(THIS_NAME, "Number of TCP Bytes   sent by TOE/AppTx: %6d \n", apTx_TcpBytCntr);

    printInfo(THIS_NAME, "Number of TCP Bytes   rcvd by TOE/AppRx: %6d \n", apRx_TcpBytCntr);
    printInfo(THIS_NAME, "Number of TCP Bytes   sent by TOE/IpTx : %6d \n", ipTx_TcpBytCntr);

    printf("\n");
    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------
    if (mode == RX_MODE) {
        if (ipRx_TcpBytCntr != apTx_TcpBytCntr) {
            printError(THIS_NAME, "The number of TCP bytes received by TOE on its IP interface (%d) does not match the number TCP bytes forwarded by TOE to the application over its TRIF interface (%d). \n", ipRx_TcpBytCntr, apTx_TcpBytCntr);
            nrErr++;
        }

        int appTxCompare = system(("diff --brief -w " + std::string(appTxFileName) + " " + std::string(appTxGoldName) + " ").c_str());
        if (appTxCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", appTxFileName, appTxGoldName);
            nrErr++;
        }

        if ((opnSessionCount == 0) && (ipRx_PktCounter > 0)) {
            printWarn(THIS_NAME, "No session was opened by the TOE during this run. Please double check!!!\n");
            nrErr++;
        }
    }

    if (mode == TX_MODE) {
        if (ipTx_TcpBytCntr != apRx_TcpBytCntr) {
            printError(THIS_NAME, "The number of TCP bytes forwarded by TOE on its IP interface (%d) does not match the number TCP bytes received by TOE from the application over its TRIF interface (%d). \n", ipTx_TcpBytCntr, apRx_TcpBytCntr);
            nrErr++;
        }

        int ipTx_TcpDataCompare = system(("diff --brief -w " + std::string(ipTxFileName2) + " " + std::string(ipTxGoldName2) + " ").c_str());
        if (ipTx_TcpDataCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", ipTxFileName2, ipTxGoldName2);
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
        ipTxFile1.close();
        ipTxFile2.close();
        ipTxGold2.close();
    }

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
