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
    IP4Hdr_Version  ip4Hdr_Ver     = pktChunk[0].tdata.range( 3,  0);
    Ip4Hdr_HdrLen   ip4Hdr_HdrLen  = pktChunk[0].tdata.range( 7,  4);
    Ip4Hdr_ToS      ip4Hdr_ToS     = pktChunk[0].tdata.range(15,  8);
    Ip4Hdr_TotalLen ip4Hdr_TotLen  = pktChunk[0].tdata.range(31, 16);
    Ip4Hdr_SrcAddr  ip4Hdr_SrcAddr = pktChunk[1].tdata.range(63, 32);
    Ip4Hdr_DstAddr  ip4Hdr_DstAddr = pktChunk[2].tdata.range(31,  0);

    TcpHdr_SrcPort  tcpHdr_SrcPort = pktChunk[2].tdata.range(47, 32);
    TcpHdr_DstPort  tcpHdr_DstPort = pktChunk[2].tdata.range(63, 48);
    TcpHdr_SeqNum   tcpHdr_SeqNum  = pktChunk[3].tdata.range(31,  0);
    TcpHdr_AckNum   tcpHdr_AckNum  = pktChunk[3].tdata.range(63, 32);
    TcpHdr_DataOff  tcpHdr_DatOff  = pktChunk[4].tdata.range( 7,  4);
    TcpHdr_CtrlBits tcpHdr_Contol  = pktChunk[4].tdata.range(13,  8);
    TcpHdr_Window   tcpHdr_Window  = pktChunk[4].tdata.range(31, 16);
    TcpHdr_Checksum tcpHdr_CSum    = pktChunk[4].tdata.range(47, 32);
    TcpHdr_UrgPtr   tcpHdr_UrgPtr  = pktChunk[4].tdata.range(63, 48);

    printf("[%s] IP PACKET HEADER (HEX numbers are in network byte order): \n", callerName);
    printf("\t IP4 Source Address      = 0x%8.8X (%3d.%3d.%3d.%3d) \n",
            ip4Hdr_SrcAddr.to_uint(),
            ip4Hdr_SrcAddr.to_uint() & 0xFF000000 >> 24,
            ip4Hdr_SrcAddr.to_uint() & 0x00FF0000 >> 16,
            ip4Hdr_SrcAddr.to_uint() & 0x0000FF00 >>  8,
            ip4Hdr_SrcAddr.to_uint() & 0x000000FF >>  0);
    printf("\t IP4 Destination Address = 0x%8.8X (%3d.%3d.%3d.%3d) \n",
            ip4Hdr_DstAddr.to_uint(),
            ip4Hdr_DstAddr.to_uint() & 0xFF000000 >> 24,
            ip4Hdr_DstAddr.to_uint() & 0x00FF0000 >> 16,
            ip4Hdr_DstAddr.to_uint() & 0x0000FF00 >>  8,
            ip4Hdr_DstAddr.to_uint() & 0x000000FF >>  0);
    printf("\t TCP Source Port         = 0x%4.4X     (%5u) \n",
            tcpHdr_SrcPort.to_uint(), swapWord(tcpHdr_SrcPort).to_uint());
    printf("\t TCP Destination Port    = 0x%4.4X     (%5u) \n",
            tcpHdr_DstPort.to_uint(), swapWord(tcpHdr_DstPort).to_uint());
    printf("\t TCP Sequence Number     = 0x%8.8X (%10u) \n",
            tcpHdr_SeqNum.to_uint(), swapDWord(tcpHdr_SeqNum).to_uint());
    printf("\t TCP Acknowledge Number  = 0x%8.8X (%10u) \n",
            tcpHdr_AckNum.to_uint(), swapDWord(tcpHdr_AckNum).to_uint());
    printf("\t TCP Data Offset         = 0x%1.1X        (%d) \n",
            tcpHdr_DatOff.to_uint(), tcpHdr_DatOff.to_uint());

    printf("\t TCP Control Bits        = ");
    printf("%s", tcpHdr_Contol[0] ? "FIN |" : "");
    printf("%s", tcpHdr_Contol[1] ? "SYN |" : "");
    printf("%s", tcpHdr_Contol[2] ? "RST |" : "");
    printf("%s", tcpHdr_Contol[3] ? "PSH |" : "");
    printf("%s", tcpHdr_Contol[4] ? "ACK |" : "");
    printf("%s", tcpHdr_Contol[5] ? "URG |" : "");
    printf("\n");

    printf("\t TCP Window              = 0x%4.4X     (%u) \n",
            tcpHdr_Window.to_uint(), swapWord(tcpHdr_Window).to_uint());
    printf("\t TCP Checksum            = 0x%4.4X     (%u) \n",
            tcpHdr_CSum.to_uint(), tcpHdr_CSum.to_uint());
    printf("\t TCP Urgent Pointer      = 0x%4.4X     (%u) \n",
            tcpHdr_UrgPtr.to_uint(), swapWord(tcpHdr_UrgPtr).to_uint());
}

/*****************************************************************************
 * @brief Prints the socket pair association of a data segment (for debug).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,     the socket pair to display.
 *****************************************************************************/
void printSockPair(const char *callerName, SocketPair sockPair)
{
    unsigned int srcAddr = sockPair.src.addr;
    unsigned int srcPort = sockPair.src.port;
    unsigned int dstAddr = sockPair.dst.addr;
    unsigned int dstPort = sockPair.dst.port;

    printf("[%s] SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        callerName,
        (unsigned int)sockPair.src.addr, (unsigned int)sockPair.src.port,
        (unsigned int)sockPair.dst.addr, (unsigned int)sockPair.dst.port);
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
