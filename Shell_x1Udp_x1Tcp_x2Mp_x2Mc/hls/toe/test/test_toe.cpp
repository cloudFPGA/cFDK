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

#include "../src/toe.hpp"
#include "../src/toe_utils.hpp"
#include "../src/dummy_memory/dummy_memory.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"
#include <map>
#include <string>
#include <unistd.h>

using namespace std;

#define DEBUG_LEVEL    2
#define MAX_SIM_CYCLES 2500000

#define THIS_NAME "TB"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//---------------------------------------------------------
unsigned int    gSimCycCnt       = 0;
unsigned int    gMaxSimCycles = 100;    // Might be updated by content of the test vector file.



/*****************************************************************************
 * @brief Returns true if packet is a FIN.
 *****************************************************************************/
bool isFIN(AxiWord axiWord) {
    if (axiWord.tdata.bit(8))
        return true;
    else
        return false;
}

/*****************************************************************************
 * @brief Returns true if packet is a SYN.
 *****************************************************************************/
bool isSYN(AxiWord axiWord) {
    if (axiWord.tdata.bit(9))
        return true;
    else
        return false;
}

/*****************************************************************************
 * @brief Returns true if packet is an ACK.
 *****************************************************************************/
bool isACK(AxiWord axiWord) {
    if (axiWord.tdata.bit(12))
        return true;
    else
        return false;
}


string decodeApUint64(ap_uint<64> inputNumber) {
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

string decodeApUint8(ap_uint<8> inputNumber) {
    string                      outputString    = "00";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";
    for (int i = 1;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0;k<4;++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[1-i] = lut[tempValue];
    }
    return outputString;
}

ap_uint<64> encodeApUint64(string dataString){
    ap_uint<64> tempOutput          = 0;
    unsigned short int  tempValue   = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<dataString.size();++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == dataString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(63-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}

ap_uint<8> encodeApUint8(string keepString){
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

// Use Dummy Memory
void pEmulateRxBufMem(dummyMemory* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
                    stream<axiWord>& BufferIn, stream<axiWord>& BufferOut) {
    mmCmd cmd;
    mmStatus status;
    axiWord inWord = axiWord(0, 0, 0);
    axiWord outWord = axiWord(0, 0, 0);
    static uint32_t rxMemCounter = 0;
    static uint32_t rxMemCounterRd = 0;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;
    static ap_uint<16> wrBufferWriteCounter = 0;
    static ap_uint<16> wrBufferReadCounter = 0;

    const char *myName  = concat3(THIS_NAME, "/", "RXMEM");

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        memory->setWriteCmd(cmd);
        wrBufferWriteCounter = cmd.bbt;
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(inWord);
        //cerr << dec << rxMemCounter << " - " << hex << inWord.data << " " << inWord.keep << " " << inWord.last << endl;
        //rxMemCounter++;;
        memory->writeWord(inWord);
        if (wrBufferWriteCounter < 9) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
        else
            wrBufferWriteCounter -= 8;
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        memory->setReadCmd(cmd);
        wrBufferReadCounter = cmd.bbt;
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        BufferOut.write(outWord);
        //cerr << dec << rxMemCounterRd << " - " << hex << outWord.data << " " << outWord.keep << " " << outWord.last << endl;
        rxMemCounterRd++;;
        if (wrBufferReadCounter < 9)
            stx_read = false;
        else
            wrBufferReadCounter -= 8;
    }
}


void pEmulateTxBufMem(dummyMemory* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
                    stream<axiWord>& BufferIn, stream<axiWord>& BufferOut) {
    mmCmd cmd;
    mmStatus status;
    axiWord inWord;
    axiWord outWord;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;

    const char *myName  = concat3(THIS_NAME, "/", "TXMEM");

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        //cerr << "WR: " << dec << cycleCounter << hex << " - " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setWriteCmd(cmd);
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(inWord);
        //cerr << "Data: " << dec << cycleCounter << hex << inWord.data << " - " << inWord.keep << " - " << inWord.last << endl;
        memory->writeWord(inWord);
        if (inWord.last) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        //cerr << "RD: " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setReadCmd(cmd);
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        //cerr << inWord.data << " " << inWord.last << " - ";
        BufferOut.write(outWord);
        if (outWord.last)
            stx_read = false;
    }
}


/*****************************************************************************
 * @brief A function to recompute the TCP checksum of a packet after it has
 *     been modified.
 *
 * @param[in]  pseudoHeader,    a double-ended queue w/ one pseudo header.

 * @return the new checksum.
 *
 * @ingroup toe
 ******************************************************************************/
ap_uint<16> checksumComputation(deque<AxiWord>  pseudoHeader) {
    ap_uint<32> tcpChecksum = 0;

    for (uint8_t i=0;i<pseudoHeader.size();++i) {
        ap_uint<64> tempInput = (pseudoHeader[i].tdata.range( 7,  0),
                                 pseudoHeader[i].tdata.range(15,  8),
                                 pseudoHeader[i].tdata.range(23, 16),
                                 pseudoHeader[i].tdata.range(31, 24),
                                 pseudoHeader[i].tdata.range(39, 32),
                                 pseudoHeader[i].tdata.range(47, 40),
                                 pseudoHeader[i].tdata.range(55, 48),
                                 pseudoHeader[i].tdata.range(63, 56));
        //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
        tcpChecksum = ((((tcpChecksum +
                        tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                        tempInput.range(31, 16)) + tempInput.range(15, 0));
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
    }
//  tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
    tcpChecksum = ~tcpChecksum;         // Reverse the bits of the result
    return tcpChecksum.range(15, 0);    // and write it into the output
}


//// This version does not work for TCP segments that are too long... overflow happens
// ap_uint<16> checksumComputation(deque<axiWord>    pseudoHeader) {
//  ap_uint<20> tcpChecksum = 0;
//  for (uint8_t i=0;i<pseudoHeader.size();++i) {
//      ap_uint<64> tempInput = (pseudoHeader[i].data.range(7, 0), pseudoHeader[i].data.range(15, 8), pseudoHeader[i].data.range(23, 16), pseudoHeader[i].data.range(31, 24), pseudoHeader[i].data.range(39, 32), pseudoHeader[i].data.range(47, 40), pseudoHeader[i].data.range(55, 48), pseudoHeader[i].data.range(63, 56));
//      //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
//      tcpChecksum = ((((tcpChecksum + tempInput.range(63, 48)) + tempInput.range(47, 32)) + tempInput.range(31, 16)) + tempInput.range(15, 0));
//  }
//  tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
//  tcpChecksum = ~tcpChecksum;             // Reverse the bits of the result
//  return tcpChecksum.range(15, 0);    // and write it into the output
//}

/*****************************************************************************
 * @brief A function to recalculate the TCP checksum of a packet after it has
 *     been modified.
 *
 * @param[in]  ipPcktQueue,    a double-ended queue w/ one IP packet.

 * @return the new checksum.
 *
 * @ingroup toe
 ******************************************************************************/
TcpCSum recalculateChecksum(
        deque<AxiWord> ipPktQueue)
{
    TcpCSum    newChecksum = 0;

    AxiIp4TotalLen  ip4Hdr_TotLen = ipPktQueue[0].tdata.range(31, 16);
    TcpDatLen        tcpDatLen     = swapWord(ip4Hdr_TotLen) - 20;

    // Create the pseudo-header
    ipPktQueue[0].tdata               = (ipPktQueue[2].tdata.range(31,  0), ipPktQueue[1].tdata.range(63, 32));
    ipPktQueue[1].tdata.range(15, 0)  = 0x0600;
    ipPktQueue[1].tdata.range(31, 16) = swapWord(tcpDatLen);
    ipPktQueue[4].tdata.range(47, 32) = 0x0;
    ipPktQueue[1].tdata.range(63, 32) = ipPktQueue[2].tdata.range(63, 32);

    for (uint8_t i=2; i<ipPktQueue.size()-1; ++i)
        ipPktQueue[i]= ipPktQueue[i+1];
    ipPktQueue.pop_back();

    return checksumComputation(ipPktQueue);

    //OBSOLETE ap_uint<16> newChecksum = 0;
    //OBSOLETE // Create the pseudo-header
    //OBSOLETE ap_uint<16> tcpLength        = (inputPacketizer[0].tdata.range(23, 16), inputPacketizer[0].tdata.range(31, 24)) - 20;
    //OBSOLETE inputPacketizer[0].tdata                = (inputPacketizer[2].tdata.range(31,  0), inputPacketizer[1].tdata.range(63, 32));
    //OBSOLETE inputPacketizer[1].tdata.range(15, 0)   = 0x0600;
    //OBSOLETE inputPacketizer[1].tdata.range(31, 16)  = (tcpLength.range(7, 0), tcpLength(15, 8));
    //OBSOLETE inputPacketizer[4].tdata.range(47, 32)    = 0x0;
    //OBSOLETE inputPacketizer[1].tdata.range(63, 32)   = inputPacketizer[2].tdata.range(63, 32);
    //OBSOLETE for (uint8_t i=2;i<inputPacketizer.size() -1;++i)
    //OBSOLETE     inputPacketizer[i]= inputPacketizer[i+1];
    //OBSOLETE inputPacketizer.pop_back();
    //OBSOLETE return checksumComputation(inputPacketizer);
}

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
 * @brief Take the ACK number of a session and inject it into the sequence
 *            number field of the current packet.
 *
 * @param[in]   ipRxPacketizer, a ref to a double-ended queue w/ packets.
 * @param[in]   sessionList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 *
 * @ingroup toe
 ******************************************************************************/
short int injectAckNumber(
        deque<AxiWord>                  &ipRxPacketizer,
        map<SocketPair, AxiTcpSeqNum>   &sessionList)
{

    SockAddr   srcSock = SockAddr(ipRxPacketizer[1].tdata.range(63, 32),
                                  ipRxPacketizer[2].tdata.range(47, 32));
    SockAddr   dstSock = SockAddr(ipRxPacketizer[2].tdata.range(31,  0),
                                  ipRxPacketizer[2].tdata.range(63, 48));
    SocketPair newSockPair = SocketPair(srcSock, dstSock);

    if (isSYN(ipRxPacketizer[4])) {
        // This packet is a SYN and there's no need to inject anything
        if (sessionList.find(newSockPair) != sessionList.end()) {
            const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");
            printWarn(myName, "Trying to open an existing session %d.\n", 1999);
            printSockPair(myName, newSockPair);
            return -1;
        }
        else {
            sessionList[newSockPair] = 0;
            const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");
            printInfo(myName, "Successfully opened a new session.\n");
            printSockPair(myName, newSockPair);
            return 0;
        }
    }
    else {
        // Packet is not a SYN
        if (sessionList.find(newSockPair) != sessionList.end()) {
            // Inject the oldest acknowledgment number in the ACK number deque
            AxiTcpAckNum newTcpHdr_AckNum = sessionList[newSockPair];
            ipRxPacketizer[3].tdata.range(63, 32) = newTcpHdr_AckNum;
            if (DEBUG_LEVEL >= 1)
                printf("<D1> Setting the sequence number of this segment to: %u \n",
                        ipRxPacketizer[3].tdata.range(63, 32).to_uint());

            // Recalculate and update the checksum
            TcpCSum         newTcpCSum = recalculateChecksum(ipRxPacketizer);
            AxiTcpChecksum newHdrCSum = swapWord(newTcpCSum);
            ipRxPacketizer[4].tdata.range(47, 32) = newHdrCSum;
            if (DEBUG_LEVEL >= 2) {
                printf("<D2> [injectAckNumber] Current packet is : ");
                for (uint8_t i=0; i<ipRxPacketizer.size(); ++i)
                    printf("%16.16lX ", ipRxPacketizer[i].tdata.to_uint());
                printf("\n");
            }
            return 1;
        }
        else {
            printf("<D0> WARNING: Trying to send data to a non-existing session! \n");
            return -1;
        }
    }
}

/*****************************************************************************
 * @brief Feed TOE with IP an Rx packet.
 *
 * @param[in]  ipRxPacketizer, a ref to the dqueue w/ an IP Rx packet.
 * @param[out] sIPRX_Toe_Data, a ref to the data stream to write.
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
void feedTOE(
        deque<Ip4Word>                  &ipRxPacketizer,
        stream<Ip4Word>                 &sIPRX_Toe_Data,
        map<SocketPair, AxiTcpSeqNum>   &sessionList)
{
    const char *myName = concat3(THIS_NAME, "/", "IPRX/FeedToe");

    if (ipRxPacketizer.size() != 0) {
        // Insert proper ACK Number in packet
        injectAckNumber(ipRxPacketizer, sessionList);
        if (DEBUG_LEVEL >= 1) {
            printIpPktStream(myName, ipRxPacketizer);
        }
        // Write stream IPRX->TOE
        uint8_t inputPacketizerSize = ipRxPacketizer.size();
        for (uint8_t i=0; i<inputPacketizerSize; ++i) {
            Ip4Word temp = ipRxPacketizer.front();
            sIPRX_Toe_Data.write(temp);
            ipRxPacketizer.pop_front();
        }
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the IP Rx Path (IPRX).
 *
 * @param[in]     iprxFile,       the input file stream to read from.
 * @param[in/out] idlingReq,      a ref to the request to idle.
 * @param[in/out] idleCycReq,     a ref to the no cycles to idle.
 * @param[in/out] ipRxPacketizer, a ref to the IPv4 Rx double-ended queue.
 * @param[in]     sessionList,    a ref to an associative container which holds
 *                                  the sessions as socket pair associations.
 * @param[out]    sIPRX_Toe_Data, a reference to the data stream between this
 *                                  process and the TOE.
 * @details
 *  Reads in new IPv4 packets from the Rx input file and stores them into the
 *   the IPv4 Rx Packetizer (ipRxPacketizer). This ipRxPacketizer is a
 *   double-ended queue that is also be fed by the process 'pEmulateL3Mux' when
 *   it wants to generate ACK packets.
 *  If packets are stored in the 'ipRxPacketizer', the get forwarded to the TOE
 *   over the 'sIRPX_Toe_Data' stream at the pace of one chunk per clock cycle.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX(
        ifstream                        &iprxFile,
        bool                            &idlingReq,
        unsigned int                    &idleCycReq,
        deque<Ip4Word>                  &ipRxPacketizer,
        map<SocketPair, AxiTcpSeqNum>   &sessionList,
        stream<Ip4Word>                 &sIPRX_Toe_Data)
{
    unsigned short int  temp;
    string              rxStringBuffer;
    vector<string>      stringVector;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    // Note: The IPv4 Rx Packetizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, L3Mux).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the file.
    feedTOE(ipRxPacketizer, sIPRX_Toe_Data, sessionList);

    // Check for EOF
    if (iprxFile.eof())
        return;

    //-- READ A LINE FROM IPRX INPUT FILE -------------------------------------
    do {
        getline(iprxFile, rxStringBuffer);

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
            // The test vector is is willing to request idle wait cycles on the before providing
            // TOE with a new IPRX packet. Warning, if Rx and Tx wait cycles coincide, the Tx
            // delay of the Tx side takes precedence (only in case of bidirectional testing).
            idleCycReq = atoi(stringVector[1].c_str());
            idlingReq = true;
            return;
        }
        else if (iprxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;;
        }
        else {
            // Send data from file to the IP Rx PAcketizer

            bool     firstWordFlag = true; // AXI-word is first chunk of packet
            Ip4Word    ipRxData;

            do {
                if (firstWordFlag == false) {
                    getline(iprxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer);
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                ipRxData = Ip4Word(encodeApUint64(stringVector[0]), \
                                   encodeApUint8(stringVector[2]),  \
                                   atoi(stringVector[1].c_str()));
                ipRxPacketizer.push_back(ipRxData);
            } while (ipRxData.tlast != 1);

            feedTOE(ipRxPacketizer, sIPRX_Toe_Data, sessionList);

            return;
        }

    } while(!iprxFile.eof());

}

// parseL3MuxPacket(ipTxPacketizer, sessionList, ipRxPacketizer);

/*****************************************************************************
 * @brief Parse the TCP packets generated by the TOE. 
 *
 * @param[in]  ipTxPacketizer,  a ref to a dqueue w/ packets from TOE.
 * @param[in]  sessionList,     a ref to an associative container which holds
 *                                the sessions as socket pair associations.
 * @param[out] ipRxPacketizer,  a ref to dqueue w/ packets for IPRX.
 *
 * @return true if a an ACK was found .
 *
 * @details
 *     Looks for an ACK packet in the output stream and when found if stores the
 *     ackNumber from that packet into the seqNumbers deque of the input stream
 *     and clears the deque containing the output packet.
 *
 *  @ingroup toe
 ******************************************************************************/
bool parseL3MuxPacket(
        deque<AxiWord>                  &ipTxPacketizer,
        map<SocketPair, AxiTcpSeqNum>   &sessionList,
        deque<AxiWord>                  &ipRxPacketizer)
{

    bool                returnValue       = false;
    bool                finPacket         = false;
    static int          ipTxPktCounter    = 0;
    static AxiTcpSeqNum prevTcpHdr_SeqNum = 0;

    const char *myName = concat3(THIS_NAME, "/", "L3MUX/Parse");

    if (DEBUG_LEVEL >= 1) {
        printIpPktStream(myName, ipTxPacketizer);
    }

    if (isSYN(ipTxPacketizer[4]) && !isACK(ipTxPacketizer[4])) {

        // The SYN bit is set but without the ACK bit being set.
        // This is a SYN packet. Rely with a SYN-ACK packet.
        //--------------------------------------------------
        ipRxPacketizer.push_back(ipTxPacketizer[0]);

        AxiIp4Address savedIp4Hdr_Addr = ipTxPacketizer[1].tdata.range(63, 32);
        AxiTcpPort    savedTcpHdr_Port = ipTxPacketizer[2].tdata.range(47, 32);

        // Replace in-going IPv4 Source w/ out-coming IPv4 Destination Address
        ipTxPacketizer[1].tdata.range(63, 32) = ipTxPacketizer[2].tdata.range(31, 0);
        ipRxPacketizer.push_back(ipTxPacketizer[1]);

        // Replace in-going IPv4 Destination w/ out-coming IPv4 Source Address
        // Also swap in-going and out-coming TCP Ports.
        ipTxPacketizer[2].tdata.range(31, 0) = savedIp4Hdr_Addr;
        ipTxPacketizer[2].tdata.range(47, 32) = ipTxPacketizer[2].tdata.range(63, 48);
        ipTxPacketizer[2].tdata.range(63, 48) = savedTcpHdr_Port;
        ipRxPacketizer.push_back(ipTxPacketizer[2]);

        // Swap the SEQ and ACK NUmbers while incrementing the ACK
        AxiTcpSeqNum tcpHdr_SeqNum = ipTxPacketizer[3].tdata.range(31, 0);
        TcpSeqNum        tcpSeqNum     = swapDWord(tcpHdr_SeqNum) + 1;
        AxiTcpAckNum tcpHdr_AckNum = swapDWord(tcpSeqNum);

        ipTxPacketizer[3].tdata.range(31,  0) = ipTxPacketizer[3].tdata.range(63, 32);
        ipTxPacketizer[3].tdata.range(63, 32) = tcpHdr_AckNum;
        ipRxPacketizer.push_back(ipTxPacketizer[3]);

        // Set the ACK bit and Recalculate the Checksum
        ipTxPacketizer[4].tdata.bit(12) = 1;
        TcpCSum         tempChecksum    = recalculateChecksum(ipTxPacketizer);
        AxiTcpChecksum tcpHdr_Checksum = swapWord(tempChecksum);
        ipTxPacketizer[4].tdata.range(47, 32) = tcpHdr_Checksum;
        ipRxPacketizer.push_back(ipTxPacketizer[4]);

        if (DEBUG_LEVEL >= 2)
            printf("<D2> Got a SYN from TOE. Replied with a SYN-ACK.\n");
    }

    else if (isFIN(ipTxPacketizer[4]) && !isACK(ipTxPacketizer[4])) {

        // The FIN bit is set but without the ACK bit being set at the same time.
        // Erase the socket pair for this session from the map.
        //------------------------------------------------------------------------
        SockAddr    srcAddr = SockAddr(ipTxPacketizer[1].tdata.range(63, 32),
                                       ipTxPacketizer[2].tdata.range(47, 32));
        SockAddr    dstSock = SockAddr(ipTxPacketizer[2].tdata.range(31,  0),
                                       ipTxPacketizer[2].tdata.range(63, 48));
        sessionList.erase(SocketPair(srcAddr, dstSock));

        if (DEBUG_LEVEL >= 2)
            printf("<D2> Got a FIN from TOE.\n");

    }

    else if (isACK(ipTxPacketizer[4])) {

        // The ACK bit is set
        //-----------------------------------------
        AxiIp4TotalLen  ip4Hdr_TotLen = ipTxPacketizer[0].tdata.range(31, 16);
        Ip4PktLen        ip4PktLen     = swapWord(ip4Hdr_TotLen);

        AxiTcpSeqNum    tcpHdr_SeqNum = ipTxPacketizer[3].tdata.range(31, 0);
        TcpSeqNum        tcpSeqNum     = swapDWord(tcpHdr_SeqNum);

        if (isFIN(ipTxPacketizer[4])) {
            // The FIN bit is set
            tcpSeqNum++;
            if (DEBUG_LEVEL >= 2)
                printf("<D2> Got an FIN+ACK from TOE.\n");
        }
        else if (isSYN(ipTxPacketizer[4])) {
            // The SYN
            tcpSeqNum++;
            if (DEBUG_LEVEL >= 2)
                printf("<D2> Got an SYN+ACK from TOE.\n");
        }

        if (ip4PktLen >= 40) {
            // Decrement by 40B (.i.e, 20B of IP Header + 20B of TCP Header since we never generate options)
            ip4PktLen -= 40;
            tcpSeqNum += ip4PktLen;
        }

        tcpHdr_SeqNum = swapDWord(tcpSeqNum);

        SockAddr   srcSock = SockAddr(ipTxPacketizer[1].tdata.range(63, 32),
                                      ipTxPacketizer[2].tdata.range(47, 32));
        SockAddr   dstSock = SockAddr(ipTxPacketizer[2].tdata.range(31,  0),
                                      ipTxPacketizer[2].tdata.range(63, 48));

        // Note how we call the constructor with source & destination swapped.
        // Destination is now the former source and vice-versa
        SocketPair socketPair(SocketPair(dstSock, srcSock));

        sessionList[socketPair] = tcpHdr_SeqNum;
        returnValue = true;

        if (isFIN(ipTxPacketizer[4])) {
            // This might be a FIN segment at the same time.
            // In this case erase the session from the list
            //------------------------------------------------
            SockAddr srcSock = SockAddr(ipTxPacketizer[1].tdata.range(63, 32),
                                        ipTxPacketizer[2].tdata.range(47, 32));
            SockAddr dstSock = SockAddr(ipTxPacketizer[2].tdata.range(31,  0),
                                        ipTxPacketizer[2].tdata.range(63, 48));

            // Note how we call to the erase method with source & destination swapped.
            // Destination is now the former source and vice-versa
            uint8_t itemsErased = sessionList.erase(SocketPair(dstSock, srcSock));
            finPacket = true;
            //cerr << "Close Tuple: " << hex << outputPacketizer[2].data.range(31, 0) << " - " << outputPacketizer[1].data.range(63, 32) << " - " << inputPacketizer[2].data.range(63, 48) << " - " << outputPacketizer[2].data.range(47, 32) << endl;

            if (DEBUG_LEVEL >= 2)
                printf("<D2> Got an ACK+FIN from TOE.\n");

            if (itemsErased != 1)
                cerr << "WARNING: Received FIN segment for a non-existing session - " << gSimCycCnt << endl;
            else
                cerr << "INFO: Session closed successfully - " << gSimCycCnt << endl;
        }

        if (ip4PktLen > 0 || finPacket == true) {

            // The ACK packet also contains data.
              // If it does generate an ACK for. Look into the IP header length for this.
            finPacket = false;
            // Update the IPv4 Total Length
            AxiIp4TotalLen ip4Hdr_TotalLen = 0x2800;
            ipTxPacketizer[0].tdata.range(31, 16) = ip4Hdr_TotalLen;
            ipRxPacketizer.push_back(ipTxPacketizer[0]);

            AxiIp4Address savedIp4Hdr_Addr = ipTxPacketizer[1].tdata.range(63, 32);
            AxiTcpPort    savedTcpHdr_Port = ipTxPacketizer[2].tdata.range(47, 32);

            // Replace in-going IPv4 Source w/ out-coming IPv4 Destination Address
            ipTxPacketizer[1].tdata.range(63, 32) = ipTxPacketizer[2].tdata.range(31, 0);
            ipRxPacketizer.push_back(ipTxPacketizer[1]);

            // Replace in-going IPv4 Destination w/ out-coming IPv4 Source Address
            // Also swap in-going and out-coming TCP Ports.
            ipTxPacketizer[2].tdata.range(31, 0) = savedIp4Hdr_Addr;
            ipTxPacketizer[2].tdata.range(47, 32) = ipTxPacketizer[2].tdata.range(63, 48);
            ipTxPacketizer[2].tdata.range(63, 48) = savedTcpHdr_Port;
            ipRxPacketizer.push_back(ipTxPacketizer[2]);

            // Swap the SEQ and ACK Numbers
            ipTxPacketizer[3].tdata.range(31,  0) = ipTxPacketizer[3].tdata.range(63, 32);
            ipTxPacketizer[3].tdata.range(63, 32) = tcpHdr_SeqNum;
            if (DEBUG_LEVEL >= 2)
                printf("<D2> tcpHdr_SeqNum = 0x%8.8X \n", tcpHdr_SeqNum.to_uint());
            ipRxPacketizer.push_back(ipTxPacketizer[3]);

            // Set the ACK bit and Recalculate the Checksum
            ipTxPacketizer[4].tdata.bit(12) = 1;
            ipTxPacketizer[4].tdata.bit(8)  = 0;  // Unset the FIN bit
            TcpSeqNum tempChecksum = recalculateChecksum(ipTxPacketizer);
            ipTxPacketizer[4].tdata.range(47, 32) = swapWord(tempChecksum);
            ipTxPacketizer[4].tkeep = 0x3F;
            ipTxPacketizer[4].tlast = 1;
            ipRxPacketizer.push_back(ipTxPacketizer[4]);

            if (prevTcpHdr_SeqNum != tcpHdr_SeqNum) {
                ipTxPktCounter++;
                if (DEBUG_LEVEL >= 1) {
                    printf("<D1> IP Tx Packet Counter = %d \n", ipTxPktCounter);
                    //cerr << "ACK cnt: " << dec << pOpacketCounter << hex << " - " << outputPacketizer[3].data.range(63, 32) << endl;
                }
            }
            prevTcpHdr_SeqNum = tcpHdr_SeqNum;
        }
        //cerr << hex << "Output: " << outputPacketizer[3].data.range(31, 0) << endl;
    }
    ipTxPacketizer.clear();
    return returnValue;
}


/*****************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]   sTOE_L3mux_Data,a reference to the data stream between TOE
 *                               and this process.
 * @param[in]   iptxFile,       the output file stream to write.
 * @param[in]   sessionList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out]  ipTxPktCount,   a ref to the IP Tx packet counter.
 *                               (counts all kinds and from all sessions).
 * @param[out]  ipRxPacketizer, a ref to the IPv4 Rx packetizer.
 *
 * @details
 *  Drains the data from the L3MUX interface of the TOE and stores them into the
 *   the IPv4 Tx Packetizer (ipTxPacketizer). This ipTxPacketizer is a double-
 *   ended queue used to accumulate all the data chunks until a whole packet is
 *   received. This queue is further read by a packet parser which either
 *   forwards the packets to an output file, or which generates an ACK packet
 *   that is further forwarded to the 'ipRxPacketizer' (see process 'pIPRX').
 *
 * @ingroup toe
 ******************************************************************************/
void pL3MUX(
        stream<Ip4Word>                 &sTOE_L3mux_Data,
        ofstream                        &iptxFile,
        map<SocketPair, AxiTcpSeqNum>   &sessionList,
        int                             &ipTxPktCounter,
        deque<Ip4Word>                  &ipRxPacketizer)
{
    Ip4Word        ipTxWord;        // An IP4 chunk
    deque<AxiWord> ipTxPacketizer;  // A double-ended queue

    uint16_t       ipTxWordCounter = 0;

    //const char *myName  = concat3(THIS_NAME, "/", "L3MUX");

    if (!sTOE_L3mux_Data.empty()) {

        //-- STEP-1 : Drain the TOE -----------------------
        sTOE_L3mux_Data.read(ipTxWord);
        string dataOutput = decodeApUint64(ipTxWord.tdata);
        string keepOutput = decodeApUint8(ipTxWord.tkeep);

        //-- STEP-2 : Write to file and to packetizer -----
        iptxFile << dataOutput << " " << ipTxWord.tlast << " " << keepOutput << endl;
        ipTxPacketizer.push_back(ipTxWord);

        //-- STEP-3 : Parse the received packet------------
        if (ipTxWord.tlast == 1) {
            // The whole packet has been written into the deque.
            parseL3MuxPacket(ipTxPacketizer, sessionList, ipRxPacketizer);
            ipTxWordCounter = 0;
            ipTxPktCounter++;
        }
        else
            ipTxWordCounter++;
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the TCP Role Interface (TRIF).
 *             This process implements Iperf.
 *
 * @param[out] soTOE_LsnReq,    TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck,    TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,     TCP notification from TOE.
 * @param[out] soTOE_DReq,      TCP data request to TOE.
 * @param[in]  siTOE_Meta,      TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,      TCP data stream from TOE.
 * @param[out] soTOE_Data,      TCP data stream to TOE.
 * @param[out] soTOE_OpnReq,    TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts,    TCP open port status from TOE.
 * @param[out] soTOE_ClsReq,    TCP close connection request to TOE.
 * @param[out] txSessionIDs,    TCP metadata (i.e. the Tx session ID) to TOE.
 *
 * @remark    The metadata from TRIF (i.e., the Tx session ID) is not directly
 *             sent to TOE. Instead, it is pushed into a vector that is used by
 *             the main process when it feeds the input data flows [FIXME].
 *
 * @details By default, the Iperf client connects to the Iperf server on the
 *             TCP port 5001 and the bandwidth displayed by Iperf is the bandwidth
 *             from the client to the server.
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF(
        stream<TcpPort>         &soTOE_LsnReq,
        stream<bool>            &siTOE_LsnAck,
        stream<appNotification> &siTOE_Notif,
        stream<appReadRequest>  &soTOE_DReq,
        stream<ap_uint<16> >    &siTOE_Meta,
        stream<axiWord>         &siTOE_Data,
        stream<axiWord>         &soTOE_Data,
        stream<ipTuple>         &soTOE_OpnReq,
        stream<openStatus>      &siTOE_OpnSts,
        stream<ap_uint<16> >    &soTOE_ClsReq,
        vector<ap_uint<16> >    &txSessionIDs)
{
    static bool listenDone        = false;
    static bool runningExperiment = false;
    static ap_uint<1> listenFsm   = 0;

    openStatus      newConStatus;
    appNotification notification;
    ipTuple         tuple;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    //-- Request to listen on a port number
    if (!listenDone) {
        TcpPort listeningPort = 0x0057;   // #87
        switch (listenFsm) {
        case 0:
            soTOE_LsnReq.write(listeningPort);
            if (DEBUG_LEVEL > 0) {
                printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                          listeningPort.to_uint(), listeningPort.to_uint());
                listenFsm++;
            }
            break;
        case 1:
            if (!siTOE_LsnAck.empty()) {
                siTOE_LsnAck.read(listenDone);
                if (listenDone) {
                    printInfo(myName, "TOE is now listening on port %d (0x%4.4X).\n",
                              listeningPort.to_uint(), listeningPort.to_uint());
                }
                else {
                    printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                            listeningPort.to_uint(), listeningPort.to_uint());
                }
                listenFsm++;
            }
            break;
        }
    }

    //OBSOLETE-20181017 axiWord transmitWord;

    // In case we are connecting back
    if (!siTOE_OpnSts.empty()) {
        openStatus tempStatus = siTOE_OpnSts.read();
        if(tempStatus.success)
            txSessionIDs.push_back(tempStatus.sessionID);
    }

    if (!siTOE_Notif.empty())     {
        siTOE_Notif.read(notification);

        if (notification.length != 0)
            soTOE_DReq.write(appReadRequest(notification.sessionID,
                                            notification.length));
        else // closed
            runningExperiment = false;
    }

    //-- IPERF PROCESSING
    enum   consumeFsmStateType {WAIT_PKG, CONSUME, HEADER_2, HEADER_3};
    static consumeFsmStateType  serverFsmState = WAIT_PKG;

    ap_uint<16>         sessionID;
    axiWord             currWord;
    static bool         dualTest = false;
    static ap_uint<32>     mAmount = 0;

    currWord.last = 0;

    switch (serverFsmState) {
    case WAIT_PKG: // Read 1st chunk: [IP-SA|IP-DA]
        if (!siTOE_Meta.empty() && !siTOE_Data.empty()) {
            siTOE_Meta.read(sessionID);
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (!runningExperiment) {
                // Check if a bidirectional test is requested (i.e. dualtest)
                if (currWord.data(31, 0) == 0x00000080)
                    dualTest = true;
                else
                    dualTest = false;
                runningExperiment = true;
                serverFsmState = HEADER_2;
            }
            else
                serverFsmState = CONSUME;
        }
        break;
    case HEADER_2: // Read 2nd chunk: [SP,DP|SeqNum]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (dualTest) {
                tuple.ip_address = 0x0a010101;  // FIXME
                tuple.ip_port = currWord.data(31, 16);
                soTOE_OpnReq.write(tuple);
            }
            serverFsmState = HEADER_3;
        }
        break;
    case HEADER_3: // Read 3rd chunk: [AckNum|Off,Flags,Window]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            mAmount = currWord.data(63, 32);
            serverFsmState = CONSUME;
        }
        break;
    case CONSUME: // Read all remain chunks
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
        }
        break;
    }
    if (currWord.last == 1)
        serverFsmState = WAIT_PKG;
}


int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<Ip4Word>                     sIPRX_Toe_Data      ("sIPRX_Toe_Data");

    stream<Ip4Word>                     sTOE_L3mux_Data     ("sTOE_L3mux_Data");

    stream<axiWord>                     sTRIF_Toe_Data      ("sTRIF_Toe_Data");
    stream<ap_uint<16> >                sTRIF_Toe_Meta      ("sTRIF_Toe_Meta");
    stream<ap_int<17> >                 sTOE_Trif_DSts      ("sTOE_Trif_DSts");

    stream<appReadRequest>              sTRIF_Toe_DReq      ("sTRIF_Toe_DReq");
    stream<axiWord>                     sTOE_Trif_Data      ("sTOE_Trif_Data");
    stream<ap_uint<16> >                sTOE_Trif_Meta      ("sTOE_Trif_Meta");

    stream<TcpPort>                     sTRIF_Toe_LsnReq    ("sTRIF_Toe_LsnReq");
    stream<bool>                        sTOE_Trif_LsnAck    ("sTOE_Trif_LsnAck");

    stream<ipTuple>                     sTRIF_Toe_OpnReq    ("sTRIF_Toe_OpnReq");
    stream<openStatus>                  sTOE_Trif_OpnSts    ("sTOE_Trif_OpnSts");

    stream<appNotification>             sTOE_Trif_Notif     ("sTOE_Trif_Notif");

    stream<ap_uint<16> >                sTRIF_Toe_ClsReq    ("sTRIF_Toe_ClsReq");

    stream<mmCmd>                       sTOE_Mem_RxP_RdCmd    ("sTOE_Mem_RxP_RdCmd");
    stream<axiWord>                     sMEM_Toe_RxP_Data   ("sMEM_Toe_RxP_Data");
    stream<mmStatus>                    sMEM_Toe_RxP_WrSts  ("sMEM_Toe_RxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_RxP_WrCmd  ("sTOE_Mem_RxP_WrCmd");
    stream<axiWord>                     sTOE_Mem_RxP_Data   ("sTOE_Mem_RxP_Data");

    stream<mmCmd>                       sTOE_Mem_TxP_RdCmd  ("sTOE_Mem_TxP_RdCmd");
    stream<axiWord>                     sMEM_Toe_TxP_Data   ("sMEM_Toe_TxP_Data");
    stream<mmStatus>                    sMEM_Toe_TxP_WrSts  ("sMEM_Toe_TxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_TxP_WrCmd  ("sTOE_Mem_TxP_WrCmd");
    stream<axiWord>                     sTOE_Mem_TxP_Data   ("sTOE_Mem_TxP_Data");

    stream<rtlSessionLookupReply>       sCAM_This_SssLkpRpl ("sCAM_This_SssLkpRpl");
    stream<rtlSessionUpdateReply>       sCAM_This_SssUpdRpl ("sCAM_This_SssUpdRpl");
    stream<rtlSessionLookupRequest>     sTHIS_Cam_SssLkpReq ("sTHIS_Cam_SssLkpReq");
    stream<rtlSessionUpdateRequest>     sTHIS_Cam_SssUpdReq ("sTHIS_Cam_SssUpdReq");

    stream<axiWord>                     sTRIF_Role_Data     ("sTRIF_Role_Data");


    //-- TESTBENCH MODES OF OPERATION ---------------------
    enum TestingMode {RX_MODE='0', TX_MODE='1', BIDIR_MODE='2'};

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    int             nrErr;
    bool            idlingReq;
    unsigned int    idleCycReq;
    unsigned int    idleCycCnt;

    bool            firstWordFlag;

    Ip4Word         ipRxData;    // An IP4 chunk

    axiWord         tcpTxData;    // A  TCP chunk

    ap_uint<32>     mmioIpAddr = 0x01010101;
    ap_uint<16>     regSessionCount;
    ap_uint<16>     relSessionCount;

    axiWord         rxDataOut_Data;         // This variable is where the data read from the stream above is temporarily stored before output

    dummyMemory     rxMemory;
    dummyMemory     txMemory;

    map<SocketPair, AxiTcpSeqNum>    sessionList;

    //-- Double-ended queues ------------------------------
    deque<AxiWord>  ipRxPacketizer;        // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        iprxFile;            // Packets for the IPRX I/F of TOE.
    ifstream        txInputFile;
    ofstream        rxOutputile;
    ofstream        iptxFile;        // Packets from the L3MUX I/F of TOE.
    ifstream        rxGoldFile;
    ifstream        txGoldFile;

    unsigned int    myCounter   = 0;

    string          dataString, keepString;
    vector<string>  stringVector;
    vector<string>  txStringVector;

    vector<ap_uint<16> > txSessionIDs;        // The Tx session ID that is sent from TRIF/Meta to TOE/Meta
    uint16_t        currTxSessionID = 0;    // The current Tx session ID

    int             rxGoldCompare       = 0;
    int             txGoldCompare       = 0; // not used yet
    int             returnValue         = 0;
    uint16_t        rxPayloadCounter    = 0;    // Counts the #packets output during the simulation on the Rx side
    int             ipTxPktCounter      = 0;    // Counts the # IP Tx packets send by the TOE (all kinds and from all sessions).
    bool            testRxPath          = true; // Indicates if the Rx path is to be tested, thus it remains true in case of Rx only or bidirectional testing
    bool            testTxPath          = true; // Same but for the Tx path.

    char            mode        = *argv[1];
    char            cCurrPath[FILENAME_MAX];


    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc > 7 || argc < 4) {
        printf("## [TB-ERROR] Expected 4 or 5 parameters with one of the the following synopsis:\n");
        printf("\t mode(0) rxInputFileName rxOutputFileName iptxFileName [rxGoldFileName] \n");
        printf("\t mode(1) txInputFileName iptxFileName [txGoldFileName] \n");
        printf("\t mode(2) rxInputFileName txInputFileName rxOutputFileName iptxFileName");
        return -1;
    }

    printf("INFO: This TB-run executes in mode \'%c\'.\n", mode);

    if (mode == RX_MODE)  {
        //-------------------------------------------------
        //-- Test the Rx side only                       --
        //-------------------------------------------------
        iprxFile.open(argv[2]);
        if (!iprxFile) {
            printf("## [TB-ERROR] Cannot open input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[3]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        iptxFile.open(argv[4]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 6) {
            rxGoldFile.open(argv[5]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
        }
        testTxPath = false;
    }
    else if (mode == TX_MODE) {
        //-- Test the Tx side only ----
        txInputFile.open(argv[2]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        iptxFile.open(argv[3]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 5){
            txGoldFile.open(argv[4]);
            if (!txGoldFile) {
                cout << " Error accessing Gold Tx file!" << endl;
                return -1;
            }
        }
        testRxPath = false;
    }
    else if (mode == BIDIR_MODE) {
        //-------------------------------------------------
        //-- Test both Rx & Tx sides                     --
        //-------------------------------------------------
        iprxFile.open(argv[2]);
        if (!iprxFile) {
            printf("## [TB-ERROR] Cannot open input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        txInputFile.open(argv[3]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[4]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        iptxFile.open(argv[5]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc >= 7) {
            rxGoldFile.open(argv[6]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
            if (argc != 8) {
                printf("## [TB-ERROR] Bi-directional testing requires two golden output files to be passed!");
                return -1;
            }
            txGoldFile.open(argv[7]);
            if (!txGoldFile) {
                printf("## [TB-ERROR] Error accessing Tx gold file!");
                return -1;
            }
        }
    }
    else {
        //-- Other cases are not allowed, exit
        printf("## [TB-ERROR] First argument can be: \n \
                \t 0 - Rx path testing only,         \n \
                \t 1 - Tx path testing only,         \n \
                \t 2 - Bi-directional testing. ");
        return -1;
    }


    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt   = 0;        // Simulation cycle counter as a global variable
    nrErr        = 0;        // Total number of testbench errors
    idlingReq    = false;    // Request to idle (.i.e, do not feed TOE's input streams)
    idleCycReq   = 0;        // The requested number of idle cycles
    idleCycCnt   = 0;        // The count of idle cycles

    firstWordFlag = true;    // AXI-word is first chunk of packet

    if (testTxPath == true) {
        //-- If the Tx Path will be tested then open a session for testing.
        for (uint8_t i=0; i<noTxSessions; ++i) {
            ipTuple newTuple = {150*(i+65355), 10*(i+65355)};   // IP address and port to open
            sTRIF_Toe_OpnReq.write(newTuple);                   // Write into TOE Tx I/F queue
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-----------------------------------------------------
        //-- STEP-1.0 : SKIP FEEDING THE INPUT DATA FLOWS
        //-----------------------------------------------------
        if (idlingReq == true) {
            if (idleCycCnt >= idleCycReq) {
                idleCycCnt = 0;
                idlingReq = false;
            }
            else
                idleCycCnt++;
        }
        else {

            if (testRxPath == true) {
                //-------------------------------------------------
                //-- STEP-1.1 : Emulate the IPv4 Rx Path
                //-------------------------------------------------
                pIPRX(iprxFile,       idlingReq,   idleCycReq,
                      ipRxPacketizer, sessionList, sIPRX_Toe_Data);
            }

            /*** OBSOLETE-2018115 ******************************************************************
            unsigned short int temp;
            string     rxStringBuffer;


            // Before processing the input file data words, write any packets generated from the TB itself
            flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);

            //-- FEED RX INPUT PATH -----------------------
            if (testRxPath == true) {
                getline(rxInputFile, rxStringBuffer);
                stringVector = tokenize(rxStringBuffer);
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
                    if (noSimCycles > maxSimCycles)
                        maxSimCycles = noSimCycles;
                }
                else if (stringVector[0] == "W") {
                    // Take into account idle wait cycles in the Rx input files.
                    // Warning, if they coincide with wait cycles in the Tx input files (only in case of bidirectional testing),
                    // then the Tx side ones takes precedence.
                    idleCycReq = atoi(stringVector[1].c_str());
                    idlingReq = true;
                }
                else if (rxInputFile.fail() == 1 || rxStringBuffer.empty()) {
                    //break;
                }
                else {
                    // Send data to the IPRX side
                    do {
                        if (firstWordFlag == false) {
                            getline(rxInputFile, rxStringBuffer);
                            stringVector = tokenize(rxStringBuffer);
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        ipRxData = Ip4Word(encodeApUint64(stringVector[0]), \
                                           encodeApUint8(stringVector[2]),  \
                                           atoi(stringVector[1].c_str()));
                        inputPacketizer.push_back(ipRxData);
                    } while (ipRxData.tlast != 1);
                    firstWordFlag = true;
                    flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);
                }
            }
            *** OBSOLETE-2018115 ******************************************************************/

            //-- FEED TX INPUT PATH -----------------------
            if (testTxPath == true && txSessionIDs.size() > 0) {

                string     txStringBuffer;

                getline(txInputFile, txStringBuffer);
                txStringVector = myTokenizer(txStringBuffer);

                if (stringVector[0] == "#") {
                    printf("%s", txStringBuffer.c_str());
                    continue;
                }
                else if (txStringVector[0] == "W") {
                    // Info: In case of bidirectional testing, the Tx wait cycles takes precedence on Rx cycles.
                    idleCycReq = atoi(txStringVector[1].c_str());
                    idlingReq = true;
                }
                else if(txInputFile.fail() || txStringBuffer.empty()){
                    //break;
                }
                else {
                    // Send data only after a session has been opened on the Tx Side
                    do {
                        if (firstWordFlag == false) {
                            getline(txInputFile, txStringBuffer);
                            txStringVector = myTokenizer(txStringBuffer);
                        }
                        else {
                            // This is the first chunk of a frame.
                            // A Tx data request (i.e. a metadata) must sent by the TRIF to the TOE
                            sTRIF_Toe_Meta.write(txSessionIDs[currTxSessionID]);
                            currTxSessionID == noTxSessions - 1 ? currTxSessionID = 0 : currTxSessionID++;
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        tcpTxData = axiWord(encodeApUint64(txStringVector[0]), \
                                            encodeApUint8(txStringVector[2]),     \
                                            atoi(txStringVector[1].c_str()));
                        sTRIF_Toe_Data.write(tcpTxData);
                    } while (tcpTxData.last != 1);

                    firstWordFlag = true;
                }
            }
        }  // End-of: FEED THE INPUT DATA FLOWS

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
            relSessionCount, regSessionCount);

        //-------------------------------------------------
        //-- STEP-3 : Emulate DRAM & CAM Interfaces
        //-------------------------------------------------
        pEmulateRxBufMem(
            &rxMemory,
            sTOE_Mem_RxP_WrCmd, sMEM_Toe_RxP_WrSts,
            sTOE_Mem_RxP_RdCmd, sTOE_Mem_TxP_Data, sMEM_Toe_RxP_Data);

        pEmulateTxBufMem(
            &txMemory,
            sTOE_Mem_TxP_WrCmd, sMEM_Toe_TxP_WrSts,
            sTOE_Mem_TxP_RdCmd, sTOE_Mem_RxP_Data, sMEM_Toe_TxP_Data);

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
            sTOE_L3mux_Data, iptxFile, sessionList,
            ipTxPktCounter,  ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Role Interface
        //-------------------------------------------------
        pTRIF(
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            sTOE_Trif_Notif,  sTRIF_Toe_DReq,
            sTOE_Trif_Meta,   sTOE_Trif_Data, sTRIF_Role_Data,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
            sTRIF_Toe_ClsReq, txSessionIDs);

        //-- STEP-4.2 : DRAIN TRIF-->ROLE -----------------
        if (!sTRIF_Role_Data.empty()) {
            sTRIF_Role_Data.read(rxDataOut_Data);
            string dataOutput = decodeApUint64(rxDataOut_Data.data);
            string keepOutput = decodeApUint8(rxDataOut_Data.keep);
            // cout << rxDataOut_Data.keep << endl;
            for (unsigned short int i = 0; i<8; ++i) {
                // Delete the data not to be kept by "keep" - for Golden comparison
                if(rxDataOut_Data.keep[7-i] == 0) {
                    // cout << "rxDataOut_Data.keep[" << i << "] = " << rxDataOut_Data.keep[i] << endl;
                    // cout << "dataOutput " << dataOutput[i*2] << dataOutput[(i*2)+1] << " is replaced by 00" << endl;
                    dataOutput.replace(i*2, 2, "00");
                }
            }
            if (rxDataOut_Data.last == 1)
                rxPayloadCounter++;
            // Write fo file
            rxOutputile << dataOutput << " " << rxDataOut_Data.last << " " << keepOutput << endl;
        }
        if (!sTOE_Trif_DSts.empty()) {
            ap_uint<17> tempResp = sTOE_Trif_DSts.read();
            if (tempResp == -1 || tempResp == -2)
                cerr << endl << "Warning: Attempt to write data into the Tx App I/F of the TOE was unsuccesfull. Returned error code: " << tempResp << endl;
        }

        //------------------------------------------------------
        //-- STEP-6 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (DEBUG_LEVEL)
            printf("@%4.4d STEP DUT \n", gSimCycCnt);

        //cerr << simCycleCounter << " - Number of Sessions opened: " <<  dec << regSessionCount << endl << "Number of Sessions closed: " << relSessionCount << endl;

    } while (gSimCycCnt < gMaxSimCycles);



    // while (!ipTxData.empty() || !ipRxData.empty() || !sTRIF_Role_Data.empty());
    /*while (!txBufferWriteCmd.empty()) {
        mmCmd tempMemCmd = txBufferWriteCmd.read();
        std::cerr <<  "Left-over Cmd: " << std::hex << tempMemCmd.saddr << " - " << tempMemCmd.bbt << std::endl;
    }*/


    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------

    // Only RX Gold supported for now
    float          rxDividedPacketCounter = static_cast <float>(rxPayloadCounter) / 2;
    unsigned int roundedRxPayloadCounter = rxPayloadCounter / 2;

    if (rxDividedPacketCounter > roundedRxPayloadCounter)
        roundedRxPayloadCounter++;

    if (roundedRxPayloadCounter != (ipTxPktCounter - 1))
        cout << "WARNING: Number of received packets (" << rxPayloadCounter << ") is not equal to the number of Tx Packets (" << ipTxPktCounter << ")!" << endl;

    // Output Number of Sessions
    cerr << "Number of Sessions opened: " << dec << regSessionCount << endl;
    cerr << "Number of Sessions closed: " << dec << relSessionCount << endl;

    // Convert command line arguments to strings
    if(argc == 5) {
        vector<string> args(argc);
        for (int i=1; i<argc; ++i)
            args[i] = argv[i];

        rxGoldCompare = system(("diff --brief -w "+args[2]+" " + args[4]+" ").c_str());
        //  txGoldCompare = system(("diff --brief -w "+args[3]+" " + args[5]+" ").c_str()); // uncomment when TX Golden comparison is supported

        if (rxGoldCompare != 0){
            cout << "RX Output != Golden RX Output. Simulation FAILED." << endl;
            returnValue = 0;
        }
        else cout << "RX Output == Golden RX Output" << endl;

        //  if (txGoldCompare != 0){
        //      cout << "TX Output != Golden TX Output" << endl;
        //      returnValue = 1;
        //  }
        //  else cout << "TX Output == Golden TX Output" << endl;

        if(rxGoldCompare == 0 && txGoldCompare == 0){
            cout << "Test Passed! (Both Golden files match)" << endl;
            returnValue = 0; // Return 0 if the test passes
        }
    }

    if (mode == RX_MODE) {
        // Rx side testing only
        iprxFile.close();
        rxOutputile.close();
        iptxFile.close();
        if(argc == 6)
            rxGoldFile.close();
    }
    else if (mode == TX_MODE) {
        // Tx side testing only
        txInputFile.close();
        iptxFile.close();
        if(argc == 5)
            txGoldFile.close();
    }
    else if (mode == BIDIR_MODE) {
        // Bi-directional testing
        iprxFile.close();
        txInputFile.close();
        rxOutputile.close();
        iptxFile.close();
        if(argc == 7){
            rxGoldFile.close();
            txGoldFile.close();
        }
    }

    return 0;  // [FIXME] return returnValue;
}
