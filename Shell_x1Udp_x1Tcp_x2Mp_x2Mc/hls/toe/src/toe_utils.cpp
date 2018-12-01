/*****************************************************************************
 * @file       : toe_uitls.cpp
 * @brief      : Utilities for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

// OBSOLETE #include <queue>
// OBSOLET #include "toe_utils.hpp"
#include "toe.hpp"
#include "toe_utils.hpp"


/*****************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord, the 16-bit unsigned data to swap.
 *
 * @return a 16-bit unsigned data.
 *****************************************************************************/
ap_uint<16> swapWord(ap_uint<16> inpWord)
{
    return (inpWord.range(7,0), inpWord(15, 8));
}
ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8));
}

/*****************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord, a 32-bit unsigned data.
 *
 * @return a 32-bit unsigned data.
 *****************************************************************************/
ap_uint<32> swapDWord(ap_uint<32> inpDWord)
{
    return (inpDWord.range( 7, 0), inpDWord(15,  8),
            inpDWord.range(23,16), inpDWord(31, 24));
}
ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
    return (inputVector.range( 7, 0), inputVector(15,  8),
        inputVector.range(23,16), inputVector(31, 24));
}

/*****************************************************************************
 * @brief Returns the number of valid bytes in an AxiWord.
 * @param[in] The 'tkeep' field of the AxiWord.
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepValue) {
    ap_uint<4> count = 0;
    switch(keepValue){
        case 0x01: count = 1; break;
        case 0x03: count = 2; break;
        case 0x07: count = 3; break;
        case 0x0F: count = 4; break;
        case 0x1F: count = 5; break;
        case 0x3F: count = 6; break;
        case 0x7F: count = 7; break;
        case 0xFF: count = 8; break;
    }
    return count;
}

/*****************************************************************************
 * @brief A function to count the number of 1s in an 8-bit value.
 *        [FIXME - To replace by keepToLen]
 *****************************************************************************/
ap_uint<4> keepMapping(ap_uint<8> keepValue) {
    ap_uint<4> counter = 0;

    switch(keepValue){
        case 0x01: counter = 1; break;
        case 0x03: counter = 2; break;
        case 0x07: counter = 3; break;
        case 0x0F: counter = 4; break;
        case 0x1F: counter = 5; break;
        case 0x3F: counter = 6; break;
        case 0x7F: counter = 7; break;
        case 0xFF: counter = 8; break;
    }
    return counter;
}

/*****************************************************************************
 * @brief Returns the 'tkeep' field of an AxiWord as a function of the number
 *  of valid bytes in that word.
 * @param[in] The number of valid bytes in an AxiWord.
 *****************************************************************************/
ap_uint<8> lenToKeep(ap_uint<4> noValidBytes) {
    ap_uint<8> keep = 0;

    switch(noValidBytes) {
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}


/*****************************************************************************
 * @brief A function to set a number of 1s in an 8-bit value.
 *         [FIXME - To replace by lenToKeep]
 *****************************************************************************/
ap_uint<8> returnKeep(ap_uint<4> count) {
    ap_uint<8> keep = 0;

    switch(count){
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}


#ifndef __SYNTHESIS__

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 ************************************************/
extern bool gTraceEvent;

/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk)
{
    printInfo(callerName, "AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}


/*****************************************************************************
 * @brief Prints the content of an IP streamed packet (for debugging).
 *
 * @param[in] callerName,     the name of the calling function or process.
 * @param[in] pktChunk,       a ref to a dqueue containing the IP packet chunks.
 *
 * @details:
 *  The format of an IPv4 packet streamed over an AxiWord interface is:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Fragment Offset    |Flags|         Identification        |          Total Length         |Type of Service|  IHL  |Version|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Destination Port        |          Source Port          |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Acknowledgment Number                      |                        Sequence Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               |                               |                               |   |U|A|P|R|S|F|  Data |       |
 *  |         Urgent Pointer        |           Checksum            |            Window             |Res|R|C|S|S|Y|I| Offset|  Res  |
 *  |                               |                               |                               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************************/
void printIpPktStream(const char *callerName, std::deque<Ip4Word> &pktChunk)
{
    AxiIp4Version  axiIp4Version = pktChunk[0].tdata.range( 3,  0);
    AxiIp4HdrLen   axiIp4HdrLen  = pktChunk[0].tdata.range( 7,  4);
    AxiIp4ToS      axiIp4ToS     = pktChunk[0].tdata.range(15,  8);
    AxiIp4TotalLen axiIp4TotLen  = pktChunk[0].tdata.range(31, 16);
    AxiIp4SrcAddr  axiIp4SrcAddr = pktChunk[1].tdata.range(63, 32);
    AxiIp4DstAddr  axiIp4DstAddr = pktChunk[2].tdata.range(31,  0);

    AxiTcpSrcPort  axiTcpSrcPort = pktChunk[2].tdata.range(47, 32);
    AxiTcpDstPort  axiTcpDstPort = pktChunk[2].tdata.range(63, 48);
    AxiTcpSeqNum   axiTcpSeqNum  = pktChunk[3].tdata.range(31,  0);
    AxiTcpAckNum   axiTcpAckNum  = pktChunk[3].tdata.range(63, 32);
    AxiTcpDataOff  axiTcpDatOff  = pktChunk[4].tdata.range( 7,  4);
    AxiTcpCtrlBits axiTcpContol  = pktChunk[4].tdata.range(13,  8);
    AxiTcpWindow   axiTcpWindow  = pktChunk[4].tdata.range(31, 16);
    AxiTcpChecksum axiTcpCSum    = pktChunk[4].tdata.range(47, 32);
    AxiTcpUrgPtr   axiTcpUrgPtr  = pktChunk[4].tdata.range(63, 48);

    printInfo(callerName, "IP PACKET HEADER (HEX numbers are in LITTLE-ENDIAN order): \n");
    printf("\t IP4 Source Address      = 0x%8.8X (%3d.%3d.%3d.%3d) \n",
            axiIp4SrcAddr.to_uint(),
            axiIp4SrcAddr.to_uint() & 0xFF000000 >> 24,
            axiIp4SrcAddr.to_uint() & 0x00FF0000 >> 16,
            axiIp4SrcAddr.to_uint() & 0x0000FF00 >>  8,
            axiIp4SrcAddr.to_uint() & 0x000000FF >>  0);
    printf("\t IP4 Destination Address = 0x%8.8X (%3d.%3d.%3d.%3d) \n",
            axiIp4DstAddr.to_uint(),
            axiIp4DstAddr.to_uint() & 0xFF000000 >> 24,
            axiIp4DstAddr.to_uint() & 0x00FF0000 >> 16,
            axiIp4DstAddr.to_uint() & 0x0000FF00 >>  8,
            axiIp4DstAddr.to_uint() & 0x000000FF >>  0);
    printf("\t TCP Source Port         = 0x%4.4X     (%u) \n",
            axiTcpSrcPort.to_uint(), swapWord(axiTcpSrcPort).to_uint());
    printf("\t TCP Destination Port    = 0x%4.4X     (%u) \n",
            axiTcpDstPort.to_uint(), swapWord(axiTcpDstPort).to_uint());
    printf("\t TCP Sequence Number     = 0x%8.8X (%u) \n",
            axiTcpSeqNum.to_uint(), swapDWord(axiTcpSeqNum).to_uint());
    printf("\t TCP Acknowledge Number  = 0x%8.8X (%u) \n",
            axiTcpAckNum.to_uint(), swapDWord(axiTcpAckNum).to_uint());
    printf("\t TCP Data Offset         = 0x%1.1X        (%d) \n",
            axiTcpDatOff.to_uint(), axiTcpDatOff.to_uint());

    printf("\t TCP Control Bits        = ");
    printf("%s", axiTcpContol[0] ? "FIN |" : "");
    printf("%s", axiTcpContol[1] ? "SYN |" : "");
    printf("%s", axiTcpContol[2] ? "RST |" : "");
    printf("%s", axiTcpContol[3] ? "PSH |" : "");
    printf("%s", axiTcpContol[4] ? "ACK |" : "");
    printf("%s", axiTcpContol[5] ? "URG |" : "");
    printf("\n");

    printf("\t TCP Window              = 0x%4.4X     (%u) \n",
            axiTcpWindow.to_uint(), swapWord(axiTcpWindow).to_uint());
    printf("\t TCP Checksum            = 0x%4.4X     (%u) \n",
            axiTcpCSum.to_uint(), axiTcpCSum.to_uint());
    printf("\t TCP Urgent Pointer      = 0x%4.4X     (%u) \n",
            axiTcpUrgPtr.to_uint(), swapWord(axiTcpUrgPtr).to_uint());
}


/*****************************************************************************
 * @brief Prints the socket pair association of a data segment (for debug).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,     the socket pair to display.
 *
 * @note
 *  The the socket pair addresses and port are stored in little-endian order.
 *****************************************************************************/
void printSockPair(const char *callerName, SocketPair sockPair)
{
    AxiIp4Address srcAddr = sockPair.src.addr;
    AxiTcpPort    srcPort = sockPair.src.port;
    AxiIp4Address dstAddr = sockPair.dst.addr;
    AxiTcpPort    dstPort = sockPair.dst.port;

    printInfo(callerName, "SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(),
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}


#endif
