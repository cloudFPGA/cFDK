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
 *----------------------------------------------------------------------------
 *
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

// OBSOLETE #include <queue>
// OBSOLET #include "toe_utils.hpp"
#include "toe.hpp"


//*** [ FIXME - Consider adding byteSwap16 and byteSwap32 here]  

/*****************************************************************************
 * @brief Swap the two bytes of a word.
 *
 * @param[in] inpWord, a 16-bit unsigned data.
 *
 * @return a 16-bit unsigned data.
 *****************************************************************************/
ap_uint<16> swapWord(ap_uint<16> inpWord)
{
    return (inpWord.range(7,0), inpWord(15, 8));
}

/*****************************************************************************
 * @brief Swap the four bytes of a double-word.
 *
 * @param[in] inpDord, a 32-bit unsigned data.
 *
 * @return a 32-bit unsigned data.
 *****************************************************************************/
ap_uint<32> swapDWord(ap_uint<32> inpDWord)
{
    return (inpDWord.range( 7, 0), inpDWord(15,  8),
            inpDWord.range(23,16), inpDWord(31, 24));
}

/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk)
{
    printf("[%s] AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
           callerName, chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}

/*****************************************************************************
 * @brief Prints an information message.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 *****************************************************************************/
void printInfo(const char *callerName, const char *message)
{
    printf("[%s] INFO - %s \n", callerName, message);
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

    printf("[%s] IP PACKET HEADER (HEX numbers are in LITTLE-ENDIAN order): \n", callerName);
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

    printf("[%s] SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        callerName,
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(),
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*****************************************************************************
 * @brief Prints a warning message.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 *****************************************************************************/
void printWarn(const char *callerName, const char *message)
{
    printf("[%s] WARNING - %s \n", callerName, message);
}
