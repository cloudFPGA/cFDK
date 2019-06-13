/*****************************************************************************
 * @file       : rx_engine.cpp
 * @brief      : Rx Engine (RXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "rx_engine.hpp"
#include "../../../test/test_toe_utils.hpp"

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/RXe"

#define TRACE_OFF  0x0000
#define TRACE_TLE 1 <<  1
#define TRACE_IPH 1 <<  2
#define TRACE_CSA 1 <<  3
#define TRACE_MDH 1 <<  4
#define TRACE_TID 1 <<  5
#define TRACE_TSD 1 <<  6
#define TRACE_EVM 1 <<  7
#define TRACE_FSM 1 <<  8
#define TRACE_MWR 1 <<  9
#define TRACE_RAN 1 << 10
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};


/*****************************************************************************
 * @brief TCP length extraction (Tle).
 *
 * @param[in]  siIPRX_Pkt,    IP4 packet stream form IPRX.
 * @param[out] soTcpSeg,      A pseudo TCP segment (.i.e, IP-SA + IP-DA + TCP)
 * @param[out] soTcpSegLen,   The length of the pseudo TCP segment.
 *
 * @details
 *   This is the process that handles the incoming data stream from the IPRX.
 *   It extracts the TCP length field from the IP header, removes that IP
 *   header but keeps the IP source and destination addresses in front of the
 *   TCP segment so that the output can be used by the next process to build
 *   the 12-byte TCP pseudo header.
 *   The length of the IPv4 data (.i.e. the TCP segment length) is also
 *   computed from the IPv4 total length and the IPv4 header length.
 *
 *   The data received from the Ethernet MAC are logically divided into lane #0
 *   (7:0) to lane #7 (63:56). The format of an incoming IPv4 packet is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |   Ident (L)   |   Ident (H)   | Total Len (L) | Total Len (H) |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    | Hd Chksum (L) | Hd Chksum (H) |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *    The format of the outgoing segment is the following:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                      0x0000000000000000                                                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |   |U|A|P|R|S|F|  Data |       |               |               |               |               |
 *  |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |
 *  |               |               |   |G|K|H|T|N|N|       |       |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |               |               |
 *  |    Data 3     |    Data 2     |    Data 1     |    Data 0     |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |
 *  |               |               |               |               |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               NU                              |    Data 7     |    Data 6     |    Data 5     |    Data 4     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pTcpLengthExtract(
        stream<Ip4overAxi>     &siIPRX_Pkt,
        stream<TcpWord>        &soTcpSeg,
        stream<TcpSegLen>      &soTcpSegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tle");

    static Ip4HdrLen        tle_ip4HdrLen   = 0;
    static Ip4TotalLen      tle_ip4TotalLen = 0;
    static Ip4DatLen        tle_ipDataLen   = 0;
    static ap_uint<4>       tle_wordCount   = 0;
    static bool             tle_insertWord  = false;
    static bool             tle_wasLast     = false;
    static bool             tle_shift       = true;
    static Ip4Word          tle_prevWord;
    TcpWord                 sendWord;

    if (tle_insertWord) {
        sendWord = TcpWord(0, 0xFF, 0);
        soTcpSeg.write(sendWord);
        tle_insertWord = false;
        if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
    }
    else if (!siIPRX_Pkt.empty() && !tle_wasLast) {
        Ip4Word currWord = siIPRX_Pkt.read();
        switch (tle_wordCount) {
        case 0:
            tle_ip4HdrLen   = currWord.tdata(3, 0);
            tle_ip4TotalLen = byteSwap16(currWord.tdata(31, 16));
            // Compute length of IPv4 data (.i.e. the TCP segment length)
            tle_ipDataLen  = tle_ip4TotalLen - (tle_ip4HdrLen * 4);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case 1:
            // Forward length of IPv4 data
            soTcpSegLen.write(tle_ipDataLen);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case 2:
            // Forward destination IP address
            // Warning, half of this address is now in 'prevWord'
            sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                               (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                               (currWord.tkeep[4] == 0));
            soTcpSeg.write(sendWord);
            tle_ip4HdrLen -= 1;  // We just processed the last 8 bytes of the IP header
            tle_insertWord = true;
            tle_wordCount++;
            if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
            break;
        case 3:
            switch (tle_ip4HdrLen) {
            case 0: // Half of prevWord contains valuable data and currWord is full of valuable
                sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                   (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                   (currWord.tkeep[4] == 0));
                soTcpSeg.write(sendWord);
                tle_shift = true;
                tle_ip4HdrLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
                break;
            case 1: // The prevWord contains garbage data, but currWord is valuable
                sendWord = TcpWord(currWord.tdata, currWord.tkeep, currWord.tlast);
                soTcpSeg.write(sendWord);
                tle_shift = false;
                tle_ip4HdrLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
                break;
            default: // The prevWord contains garbage data, currWord at least half garbage
                //Drop this shit
                tle_ip4HdrLen -= 2;
                break;
            }
            break;
        default:
            if (tle_shift) {
                sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                   (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                   (currWord.tkeep[4] == 0));
                soTcpSeg.write(sendWord);
                if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
            }
            else {
                sendWord = TcpWord(currWord.tdata, currWord.tkeep, currWord.tlast);
                soTcpSeg.write(sendWord);
                if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
            }
            break;

        } // End of: switch (tle_wordCount)

        tle_prevWord = currWord;
        if (currWord.tlast) {
            tle_wordCount = 0;
            tle_wasLast = !sendWord.tlast;
        }

    } // End of: else if (!siIPRX_Data.empty() && !tle_wasLast) {

    else if (tle_wasLast) { //Assumption has to be shift
        // Send remaining data
        TcpWord sendWord = TcpWord(0, 0, 1);
        sendWord.tdata(31, 0) = tle_prevWord.tdata(63, 32);
        sendWord.tkeep( 3, 0) = tle_prevWord.tkeep( 7,  4);
        soTcpSeg.write(sendWord);
        tle_wasLast = false;
        if (DEBUG_LEVEL & TRACE_TLE) printAxiWord(myName, sendWord);
    }
}


/*****************************************************************************
 * @brief Insert pseudo header (Iph).
 *
 * @param[in]  siTle_TcpSeg,    A pseudo TCP segment from TCP Length Extraction.
 * @param[in]  siTle_TcpSegLen, The length of the incoming pseudo TCP segment.
 * @param[out] soTcpSeg,        An updated pseudo TCP segment.
 *
 * @details
 *  Constructs a TCP pseudo header and prepends it to the TCP payload.
 *
 *  The format of the incoming segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                      0x0000000000000000                                                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |   |U|A|P|R|S|F|  Data |       |               |               |               |               |
 *  |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |
 *  |               |               |   |G|K|H|T|N|N|       |       |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |               |               |
 *  |    Data 3     |    Data 2     |    Data 1     |    Data 0     |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |
 *  |               |               |               |               |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               NU                              |    Data 7     |    Data 6     |    Data 5     |    Data 4     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 *    The format of the outgoing pseudo TCP segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |         Segment Len           |      0x06     |    0x00       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pInsertPseudoHeader(
        stream<TcpWord>     &siTle_TcpSeg,
        stream<TcpSegLen>   &siTle_TcpSegLen,
        stream<TcpWord>     &soTcpSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static bool         iph_wasLast = false;
    static ap_uint<2>   iph_wordCount = 0;
    ap_uint<1>          valid;
    TcpSegLen           tcpSegLen;
    TcpWord             currWord;
    TcpWord             sendWord;
    static TcpWord      iph_prevWord;

    const char *myName  = concat3(THIS_NAME, "/", "Iph");

    currWord.tlast = 0;

    if (iph_wasLast) {
        sendWord.tdata(31, 0) = iph_prevWord.tdata(63,32);
        sendWord.tkeep( 3, 0) = iph_prevWord.tkeep( 7, 4);
        sendWord.tkeep( 7, 4) = 0x0;
        sendWord.tlast        = 0x1;
        soTcpSeg.write(sendWord);
        iph_wasLast = false;
        if (DEBUG_LEVEL & TRACE_IPH) printAxiWord(myName, sendWord);
    }
    else if(!siTle_TcpSeg.empty()) {
        switch (iph_wordCount) {
        case 0:
            siTle_TcpSeg.read(currWord);
            iph_wordCount++;
            break;
        case 1:
            siTle_TcpSeg.read(currWord);
            sendWord = iph_prevWord;
            // Forward IP-DA & IP-SA
            soTcpSeg.write(sendWord);
            iph_wordCount++;
            if (DEBUG_LEVEL & TRACE_IPH) printAxiWord(myName, sendWord);
            break;
        case 2:
            if (!siTle_TcpSegLen.empty()) {
                siTle_TcpSeg.read(currWord);
                siTle_TcpSegLen.read(tcpSegLen);
                // Forward Protocol and Segment length
                sendWord.tdata(15,  0) = 0x0600;        // 06 is for TCP
                //OBSOLETE-20181120 sendWord.tdata(23, 16) = tcpSegLen(15, 8);
                //OBSOLETE-20181120 sendWord.tdata(31, 24) = tcpSegLen( 7, 0);
                sendWord.tdata(31, 16) = byteSwap16(tcpSegLen);
                // Forward TCP-SP & TCP-DP
                sendWord.tdata(63, 32) = currWord.tdata(31, 0);
                sendWord.tkeep         = 0xFF;
                sendWord.tlast         = 0;
                soTcpSeg.write(sendWord);
                iph_wordCount++;
                if (DEBUG_LEVEL & TRACE_IPH) printAxiWord(myName, sendWord);
            }
            break;
        default:
            siTle_TcpSeg.read(currWord);
            // Forward { Sequence Number, Acknowledgment Number } or
            //         { Flags, Window, Checksum, UrgentPointer } or
            //         { Data }
            sendWord.tdata.range(31,  0) = iph_prevWord.tdata.range(63, 32);
            sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
            sendWord.tkeep.range( 3,  0) = iph_prevWord.tkeep.range(7, 4);
            sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
            sendWord.tlast               = (currWord.tkeep[4] == 0); // see format of the incoming segment
            soTcpSeg.write(sendWord);
            if (DEBUG_LEVEL & TRACE_IPH) printAxiWord(myName, sendWord);
            break;
        }
        iph_prevWord = currWord;
        if (currWord.tlast == 1) {
            iph_wordCount = 0;
            iph_wasLast = !sendWord.tlast;
        }
    }
}


/*****************************************************************************
 * @brief TCP checksum accumulator (csa).
 *
 * @param[in]  siIph_TcpSeg,   A pseudo TCP segment from InsertPseudoHeader (Iph).
 * @param[out] soTid_Data,     TCP data stream to TcpInvalidDropper (Tid).
 * @param[out] soTid_DataVal,  TCP data valid to [Tid].
 * @param[out] soMdh_Meta,,    TCP metadata to MetaDataHandler (Mdh).
 * @param[out] soMdh_SockPair, TCP socket pair to [Mdh].
 * @param[out] soPRt_GetState, Req state of TCP_DP to PortTable (PRt).
 *
 * @details
 *  This process extracts the data (i.e. text octets) from the incoming segment
 *   and forwards them to the TCP Invalid Dropper (Tid) together with a valid
 *   bit indicating the result of the checksum validation.
 *  It also extracts the socket pair information and some metadata information
 *   from the TCP segment and forwards them to the MetaData Handler (Mdh).
 *  Next, the TCP destination port number is extracted and forwarded to the
 *   Port Table (PRt) process to check if the port is open.
 *
 *  The format of the incoming pseudo TCP segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |         Segment Len           |      0x06     |    0x00       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pCheckSumAccumulator(
        stream<TcpWord>           &siIph_TcpSeg,
        stream<AxiWord>           &soTid_Data,
        stream<ValBit>            &soTid_DataVal,
        stream<rxEngineMetaData>  &soMdh_Meta,
        stream<AxiSocketPair>     &soMdh_SockPair,
        stream<TcpPort>           &soPRt_GetState)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Csa");

    static ap_uint<17>      csa_tcp_sums[4] = {0, 0, 0, 0};
    static ap_uint<8>       csa_dataOffset = 0xFF; // [FIXME-Why not make it of type AxiTcpDataOff]
    static ap_uint<16>      csa_wordCount = 0;
    static AxiSocketPair    csa_sessTuple;
    static ap_uint<36>      halfWord;
    TcpWord                 currWord;
    TcpWord                 sendWord;
    static rxEngineMetaData csa_meta;
    static AxiTcpDstPort    csa_axiTcpDstPort;
    static AxiTcpChecksum   csa_axiTcpCSum;

    static bool             csa_doShift     = false;
    static bool             csa_wasLast     = false;
    static bool             csa_doCSumVerif = false;

    static ap_uint<3>       csa_cc_state = 0;

    if (!siIph_TcpSeg.empty() && !csa_doCSumVerif) {
        siIph_TcpSeg.read(currWord);
        switch (csa_wordCount) {
        case 0:
            csa_dataOffset = 0xFF;
            csa_doShift = false;
                // Get IP-SA & IP-DA
                //  Warning: Remember that IP addresses come in little-endian order
                csa_sessTuple.src.addr = currWord.tdata(31,  0);
                csa_sessTuple.dst.addr = currWord.tdata(63, 32);
                sendWord.tlast         = currWord.tlast;
            break;
        case 1:
            // Get Segment length
            csa_meta.length = byteSwap16(currWord.tdata(31, 16));
            // Get TCP-SP & TCP-DP
            //  Warning: Remember that TCP ports come in little-endian order
            csa_sessTuple.src.port = currWord.tdata(47, 32);
            csa_sessTuple.dst.port = currWord.tdata(63, 48);
            csa_axiTcpDstPort      = currWord.tdata(63, 48);
            sendWord.tlast         = currWord.tlast;
            break;
        case 2:
            // Get Sequence and Acknowledgment Numbers
            csa_meta.seqNumb = byteSwap32(currWord.tdata(31, 0));
            csa_meta.ackNumb = byteSwap32(currWord.tdata(63, 32));
            sendWord.tlast   = currWord.tlast;
            break;
        case 3:
            // Get Data Offset
            csa_dataOffset   = currWord.tdata.range(7, 4);
            csa_meta.length -= (csa_dataOffset * 4);
            //OBSOLETE-20190511 csa_dataOffset -= 5; //FIXME, do -5
            // Get Control Bits
            /*  [ 8] == FIN
             *  [ 9] == SYN
             *  [10] == RST
             *  [11] == PSH
             *  [12] == ACK
             *  [13] == URG
             */
            csa_meta.ack = currWord.tdata[12];
            csa_meta.rst = currWord.tdata[10];
            csa_meta.syn = currWord.tdata[ 9];
            csa_meta.fin = currWord.tdata[ 8];
            // Get Window Size
            csa_meta.winSize = byteSwap16(currWord.tdata(31, 16));
            // Get the checksum of the pseudo-header (only for debug purposes)
            csa_axiTcpCSum = currWord.tdata(47, 32);
            sendWord.tlast = currWord.tlast;
            break;
        default:
            if (csa_dataOffset > 6) {
                // Drain the unsupported options
                csa_dataOffset -= 2;
            }
            else if (csa_dataOffset == 6) {
                // The header has 32 bits of options and padding out of 64 bits
                // Get the first four Data bytes
                csa_dataOffset = 5;
                csa_doShift = true;
                halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                halfWord.range(35, 32) = currWord.tkeep.range( 7,  4);
                sendWord.tlast = (currWord.tkeep[4] == 0);
            }
            else {    // csa_dataOffset == 5 (or less)
                if (!csa_doShift) {
                    sendWord = currWord;
                    soTid_Data.write(sendWord);
                }
                else {
                    sendWord.tdata.range(31,  0) = halfWord.range(31, 0);
                    sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
                    sendWord.tkeep.range( 3,  0) = halfWord.range(35, 32);
                    sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
                    sendWord.tlast = (currWord.tkeep[4] == 0);
                    soTid_Data.write(sendWord);
                    halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                    halfWord.range(35, 32) = currWord.tkeep.range(7, 4);
                }
            }
            break;
        } // End of: switch

        // Accumulate TCP checksum
        for (int i = 0; i < 4; i++) {
            #pragma HLS UNROLL
        	TcpCSum temp;
            if (currWord.tkeep.range(i*2+1, i*2) == 0x3) {
                temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                temp(15, 8) = currWord.tdata.range(i*16+ 7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
            else if (currWord.tkeep[i*2] == 0x1) {
                temp( 7, 0) = 0;
                temp(15, 8) = currWord.tdata.range(i*16+7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
        }

        csa_wordCount++;

        if(currWord.tlast == 1) {
            csa_wordCount = 0;
            csa_wasLast = !sendWord.tlast;
            csa_doCSumVerif = true;
        }

    } // End of: if (!siIph_Data.empty() && !csa_doCSumVerif)

    else if(csa_wasLast) {
        if (csa_meta.length != 0) {
            sendWord.tdata.range(31,  0) = halfWord.range(31,  0);
            sendWord.tdata.range(63, 32) = 0;
            sendWord.tkeep.range( 3,  0) = halfWord.range(35, 32);
            sendWord.tkeep.range( 7,  4) = 0;
            sendWord.tlast               = 1;
            soTid_Data.write(sendWord);
        }
        csa_wasLast = false;
    }
    
    else if (csa_doCSumVerif) {
        switch (csa_cc_state) {
            case 0:
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_tcp_sums[1] = (csa_tcp_sums[1] + (csa_tcp_sums[1] >> 16)) & 0xFFFF;
                csa_tcp_sums[2] = (csa_tcp_sums[2] + (csa_tcp_sums[2] >> 16)) & 0xFFFF;
                csa_tcp_sums[3] = (csa_tcp_sums[3] + (csa_tcp_sums[3] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 1:
                csa_tcp_sums[0] += csa_tcp_sums[2];
                csa_tcp_sums[1] += csa_tcp_sums[3];
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_tcp_sums[1] = (csa_tcp_sums[1] + (csa_tcp_sums[1] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 2:
                csa_tcp_sums[0] += csa_tcp_sums[1];
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 3:
                csa_tcp_sums[0] = ~csa_tcp_sums[0];
                csa_cc_state++;
                break;
            case 4:
                if (csa_tcp_sums[0](15, 0) == 0) {
                    // The checksum is correct. TCP segment is valid.
                    // Forward to MetaDataHandler
                    soMdh_Meta.write(csa_meta);
                    soMdh_SockPair.write(csa_sessTuple);
                    // Forward to TcpInvalidDropper
                    if (csa_meta.length != 0) {
                        soTid_DataVal.write(true);
                    }
                    // Request state of TCP_DP
                    soPRt_GetState.write(byteSwap16(csa_axiTcpDstPort));
                }
                else {
                    if(csa_meta.length != 0) {
                        // Packet has some TCP payload
                        soTid_DataVal.write(false);
                    }
                    if (DEBUG_LEVEL & TRACE_CSA) {
                        printWarn(myName, "RECEIVED BAD CHECKSUM (0x%4.4X - Delta= 0x%4.4X).\n",
                                    csa_axiTcpCSum.to_uint(),
                                    byteSwap16(~csa_tcp_sums[0](15, 0) & 0xFFFF).to_uint());
                        printInfo(myName, "SocketPair={{0x%8.8X, 0x%4.4X},{0x%8.8X, 0x%4.4X}.\n",
                                csa_sessTuple.src.addr.to_uint(), csa_sessTuple.src.port.to_uint(),
                                csa_sessTuple.dst.addr.to_uint(), csa_sessTuple.dst.port.to_uint());
                    }
                }
                csa_doCSumVerif = false;
                csa_tcp_sums[0] = 0;
                csa_tcp_sums[1] = 0;
                csa_tcp_sums[2] = 0;
                csa_tcp_sums[3] = 0;
                csa_cc_state = 0;
                break;

        } // End of: switch
    }
} // End of: pCheckSumAccumulator


/*****************************************************************************
 * @brief TCP Invalid checksum Dropper (Tid)
 *
 * @param[in]  siCsa_TcpData,   TCP data stream from Checksum Accumulator (Csa).
 * @param[in]  siCsa_TcpDataVal,TCP data valid.
 * @param[out] soTsd_Data,      TCP data stream to Tcp Segment Dropper.
 *
 * @details
 *  Drops the TCP data when they are flagged with an invalid checksum by
 *   'siDataValid'. Otherwise, the TCP data is passed on.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pTcpInvalidDropper(
        stream<TcpWord>     &siCsa_Data,
        stream<ValBit>      &siCsa_DataVal,
        stream<AxiWord>     &soTsd_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tid");

    /*** OBSOLETE-20181212 ***********
    static bool tid_isFirstWord = true;
    static bool tid_doDrop      = false;

    AxiWord currWord;
    bool     isValid;

    currWord.tlast = 0;

    if (tid_doDrop) {
        if(!siCsa_Data.empty())
            siCsa_Data.read(currWord);
    }
    else if (tid_isFirstWord) {
        if (!siCsa_DataVal.empty() && !siCsa_Data.empty()) {
            siCsa_DataVal.read(isValid);
            siCsa_Data.read(currWord);
            if (!isValid)
                tid_doDrop = true;
            else
                soTsd_Data.write(currWord);
            tid_isFirstWord = false;
        }
    }
    else if(!siCsa_Data.empty()) {
        siCsa_Data.read(currWord);
        soTsd_Data.write(currWord);
    }

    if (currWord.tlast == 1) {
        tid_doDrop      = false;
        tid_isFirstWord = true;
    }
    **********************************/

    static enum FsmState {GET_VALID=0, FWD, DROP} tid_fsmState=GET_VALID;

    AxiWord currWord;
    bool    isValid;

    switch (tid_fsmState) {

    case GET_VALID:
        if (!siCsa_DataVal.empty()) {
            siCsa_DataVal.read(isValid);
            if (isValid) {
                tid_fsmState = FWD;
            }
            else {
                tid_fsmState = DROP;
                printWarn(myName, "Bad checksum: Dropping payload for this packet!\n");
            }
        }
        break;

    case FWD:
        if(!siCsa_Data.empty() && !soTsd_Data.full()) {
            siCsa_Data.read(currWord);
            soTsd_Data.write(currWord);
            if (currWord.tlast)
                tid_fsmState = GET_VALID;
            if (DEBUG_LEVEL & TRACE_TID) printAxiWord(myName, currWord);
        }
        break;

    case DROP:
        if(!siCsa_Data.empty()) {
            siCsa_Data.read(currWord);
            if (currWord.tlast) {
                tid_fsmState = GET_VALID;
            }
        }
        break;

    } // End of: switch

} // End of: pTcpInvalidDropper


/*****************************************************************************
 * @brief MetaData Handler (Mdh)
 *
 * @param[in]  siCsa_Meta,      TCP metadata from CheckSum Accumulator (Csa).
 * @param[in]  siCsa_SockPair,  TCP socket pair from [Csa].
 * @param[in]  siSLc_SessLookupRep, Lookup reply from Session Lookup Controller (SLc).
 * @param[in]  siPRt_PortSts,   Port state (open/close) from Port Table (PRt).
 * @param[out] soSLc_SessLkpReq,Session lookup request to [SLc].
 * @param[out] soEVe_Event,     Event to EVent Engine (EVe).
 * @param[out] soTsd_DropCmd,   Drop command to Tcp Segment Dropper (Tsd).
 * @param[out] soFsm_Meta,      Metadata to RXe's Finite State Machine (Fsm).
 *
 * @details
 *  This process waits until it gets a response from the Port Table (PRt).
 *   It then loads the metadata and socket pair generated by the Checksum
 *   Accumulator (Csa) process and evaluates them. Next, if the destination
 *   port is open, it requests the Session Lookup Controller (SLc) to
 *   perform a session lookup and waits for its reply. If a session is open
 *   for this socket pair, a new metadata structure is generated and is
 *   forwarded to the Finite State Machine (FSm) of the Rx engine.
 *   If the target destination port is not open, the process creates an
 *    event requesting a RST+ACK message to be sent back to the initiating
 *    host.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pMetaDataHandler(
        stream<rxEngineMetaData>    &siCsa_Meta,
        stream<AxiSocketPair>       &siCsa_SockPair,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<StsBit>              &siPRt_PortSts,
        stream<sessionLookupQuery>  &soSLc_SessLkpReq,
        stream<extendedEvent>       &soEVe_Event,
        stream<CmdBit>              &soTsd_DropCmd,
        stream<rxFsmMetaData>       &soFsm_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName = concat3(THIS_NAME, "/", "Mdh");

    static rxEngineMetaData     mdh_meta;
    static sessionLookupReply   mdh_sessLookupReply;
    static Ip4Address           mdh_ip4SrcAddr;
    static TcpPort              mdh_tcpDstPort;

    AxiSocketPair tuple;
    StsBit        dstPortStatus;

    static enum FsmState {META=0, LOOKUP} mdh_fsmState;

    switch (mdh_fsmState) {

    case META:
        // Wait until we get a reply from the [PortTable]
        if (!siPRt_PortSts.empty()) {
            //  Read metadata and socket pair
            if (!siCsa_Meta.empty() && !siCsa_SockPair.empty()) {
                siPRt_PortSts.read(dstPortStatus);
                siCsa_Meta.read(mdh_meta);
                siCsa_SockPair.read(tuple);

                mdh_ip4SrcAddr = byteSwap32(tuple.src.addr);
                mdh_tcpDstPort = byteSwap16(tuple.dst.port);

                if (dstPortStatus == false) {
                    // The destination port is closed
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printWarn(myName, "Port 0x%4.4X (%d) is not open.\n",
                                  mdh_tcpDstPort.to_uint(), mdh_tcpDstPort.to_uint());
                    }
                    if (!mdh_meta.rst) {
                        // Reply with RST+ACK and send necessary socket-pair through event
                        AxiSocketPair  switchedTuple;
                        switchedTuple.src.addr = tuple.dst.addr;
                        switchedTuple.dst.addr = tuple.src.addr;
                        switchedTuple.src.port = tuple.dst.port;
                        switchedTuple.dst.port = tuple.src.port;
                        if (mdh_meta.syn || mdh_meta.fin) {
                            soEVe_Event.write(extendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length+1),
                                                           switchedTuple)); //always 0
                        }
                        else {
                            soEVe_Event.write(extendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length),
                                                           switchedTuple));
                        }
                    }
                    else {
                        // The RST bit is set. Ignore => do nothing
                    }

                    if (mdh_meta.length != 0) {
                        soTsd_DropCmd.write(DROP_CMD);
                    }
                }
                else {
                    // Destination Port is opened
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printInfo(myName, "Destination port 0x%4.4X (%d) is open.\n",
                                  mdh_tcpDstPort.to_uint(), mdh_tcpDstPort.to_uint());
                    }
                    // Query a session lookup. Only allow creation of a new entry when SYN or SYN_ACK
                    soSLc_SessLkpReq.write(sessionLookupQuery(tuple,
                                          (mdh_meta.syn && !mdh_meta.rst && !mdh_meta.fin)));
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printInfo(myName, "Request the SLc to lookup the following session:\n");
                        //OBSOLETE-20190524 printAxiSockPair(myName, tuple);
                        printSockPair(myName, tuple);
                    }
                    mdh_fsmState = LOOKUP;
                }
            }
        }
        break;

    case LOOKUP:
        // Wait until we get a reply from the [SessionLookupController]
        //  Warning: There may be a large delay for the lookup to complete
        if (!siSLc_SessLookupRep.empty()) {
            siSLc_SessLookupRep.read(mdh_sessLookupReply);
            if (mdh_sessLookupReply.hit) {
                // Forward metadata to the TCP Finite State Machine
                soFsm_Meta.write(rxFsmMetaData(mdh_sessLookupReply.sessionID,
                                               mdh_ip4SrcAddr,
                                               mdh_tcpDstPort,
                                               mdh_meta));
                if (DEBUG_LEVEL & TRACE_MDH)
                    printInfo(myName, "Successful session lookup. \n");
            }
            else {
                // [TODO - Port is Open, but we have no sessionID for it]
                if (DEBUG_LEVEL & TRACE_MDH)
                    printWarn(myName, "Session lookup failed! \n");
            }
            if (mdh_meta.length != 0) {
                soTsd_DropCmd.write(!mdh_sessLookupReply.hit);
            }
            mdh_fsmState = META;
        }
        break;

    } // End of: switch

} // End of: pMetaDataHandler


/*****************************************************************************
 * @brief Finite State machine (Fsm)
 *
 * @param[in]  siMdh_Meta,        Metadata from [MetDataHandler].
 * @param[in]  siSTt_SessStateRep,Session state reply from [StateTable].
 * @param[in]  siRSt_RxSarUpdRep, Update reply from [RxSarTable].
 * @param[in]  siTSt_TxSarRdRep,  Read reply from [TxSarTable].
 * @param[out] soSTt_SessStateReq,Request to read the session state to [StateTable]].
 * @param[out] soRSt_RxSarUpdReq, Request to update the session Rx SAR to [RxSarTable].
 * @param[out] soTSt_TxSarRdReq,  Request to read the session Tx SAR to [TxSarTable].
 * @param[out] soTIm_ReTxTimerCmd,Command for a retransmit timer of [Timers].
 * @param[out] soTIm_ClearProbeTimer,Clear the probing timer to [Timers].
 * @param[out] soTIm_CloseTimer,  Close session timer to [Timers].
 * @param[out] soTAi_SessOpnSts,  Open status of the session to [TxAppInterface].
 * @param[out] soEVe_Event,       Event to [EventEngine].
 * @param[out] soTsd_DropCmd,     Drop command to [TcpSegmentDropper].
 * @param[out] soMwr_WrCmd,       Memory write command to [MemoryWriter].
 * @param[out] soRan_RxNotif,     Rx data notification to [RxAppNotifier].
 *
 * @details
 *  This process implements the typical TCP state and metadata management. It
 *   contains all the logic that updates the metadata and keeps track of the
 *   events related to the reception of segments and their handshaking. This is
 *   the key central part of the Rx engine.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pFiniteStateMachine(
        stream<rxFsmMetaData>               &siMdh_Meta,
        stream<sessionState>                &siSTt_SessStateRep,
        stream<rxSarEntry>                  &siRSt_RxSarUpdRep,
        stream<rxTxSarReply>                &siTSt_TxSarRdRep,
        stream<stateQuery>                  &soSTt_SessStateReq,
        stream<rxSarRecvd>                  &soRSt_RxSarUpdReq,
        stream<rxTxSarQuery>                &soTSt_TxSarRdReq,
        //OBSOLETE-20190118 stream<rxRetransmitTimerUpdate>     &soTIm_ReTxTimerCmd,
        stream<ReTxTimerCmd>                &soTIm_ReTxTimerCmd,
        stream<ap_uint<16> >                &soTIm_ClearProbeTimer,
        stream<ap_uint<16> >                &soTIm_CloseTimer,
        stream<OpenStatus>                  &soTAi_SessOpnSts, //TODO merge with eventEngine
        stream<event>                       &soEVe_Event,
        stream<CmdBit>                      &soTsd_DropCmd,
        stream<DmCmd>                       &soMwr_WrCmd,
        stream<AppNotif>                    &soRan_RxNotif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Fsm");

    static rxFsmMetaData fsm_meta;
    static bool          fsm_txSarRequest = false;

    static uint16_t rxEngSynCounter = 0;

    ap_uint<4>      control_bits = 0;
    sessionState    tcpState;
    rxSarEntry      rxSar;
    rxTxSarReply    txSar;

    static enum FsmState {LOAD=0, TRANSITION} fsmState=LOAD;

    switch(fsmState) {

    case LOAD:
        if (!siMdh_Meta.empty()) {
            siMdh_Meta.read(fsm_meta);
            // Request the current state of the session
            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId));
            // Always request the rxSar, even though not required for SYN-ACK
            soRSt_RxSarUpdReq.write(rxSarRecvd(fsm_meta.sessionId));

            if (fsm_meta.meta.ack) {
                // Only request the txSar when (ACK+ANYTHING); not for SYN
                soTSt_TxSarRdReq.write(rxTxSarQuery(fsm_meta.sessionId));
                fsm_txSarRequest = true;
            }
            fsmState = TRANSITION;
        }
        break;

    case TRANSITION:
        // Check if transition to LOAD occurs
        if (!siSTt_SessStateRep.empty() && !siRSt_RxSarUpdRep.empty() &&
            !(fsm_txSarRequest && siTSt_TxSarRdRep.empty())) {
            fsmState = LOAD;
            fsm_txSarRequest = false;
        }

        control_bits[0] = fsm_meta.meta.ack;
        control_bits[1] = fsm_meta.meta.syn;
        control_bits[2] = fsm_meta.meta.fin;
        control_bits[3] = fsm_meta.meta.rst;

        switch (control_bits) {

        case 1:
            //--------------------------------------
            //-- ACK
            //--------------------------------------
            if (fsmState == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_RxSarUpdRep.read(rxSar);
                siTSt_TxSarRdRep.read(txSar);
                //OBSOLETE-20190181 soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID,
                //OBSOLETE-20190181                                                 (fsm_meta.meta.ackNumb == txSar.nextByte)));
                soTIm_ReTxTimerCmd.write(ReTxTimerCmd(fsm_meta.sessionId,
                                                     (fsm_meta.meta.ackNumb == txSar.nextByte)));
                if ( (tcpState == ESTABLISHED) || (tcpState == SYN_RECEIVED) ||
                     (tcpState == FIN_WAIT_1)  || (tcpState == CLOSING)      ||
                     (tcpState == LAST_ACK) ) {
                    // Check if new ACK arrived
                    if ( (fsm_meta.meta.ackNumb == txSar.prevAck) &&
                         (txSar.prevAck != txSar.nextByte) ) {
                        // Not new ACK increase counter
                        txSar.count++;
                    }
                    else {
                        // Notify probeTimer about new ACK
                        soTIm_ClearProbeTimer.write(fsm_meta.sessionId);

                        // Check for SlowStart & Increase Congestion Window (TODO-CheckDiff)
                        if (txSar.cong_window <= (txSar.slowstart_threshold-MMS)) {
                            txSar.cong_window += MMS;
                        }
                        else if (txSar.cong_window <= 0xF7FF) {
                            txSar.cong_window += 365; //TODO replace by approx. of (MSS x MSS) / cong_window
                        }
                        txSar.count = 0;
                    }

                    // TX SAR (TODO-CheckDiff)
                    if ( (  (txSar.prevAck <= fsm_meta.meta.ackNumb) && (fsm_meta.meta.ackNumb <= txSar.nextByte) ) ||
                         ( ((txSar.prevAck <= fsm_meta.meta.ackNumb) || (fsm_meta.meta.ackNumb <= txSar.nextByte) ) && (txSar.nextByte < txSar.prevAck) ) ) {
                        soTSt_TxSarRdReq.write((rxTxSarQuery(fsm_meta.sessionId,
                                                 fsm_meta.meta.ackNumb,
                                                 fsm_meta.meta.winSize,
                                                 txSar.cong_window,
                                                 txSar.count,
                                                 0)));
                    }

                    // Check if packet contains payload
                    if (fsm_meta.meta.length != 0) {
                        ap_uint<32> newRecvd = fsm_meta.meta.seqNumb + fsm_meta.meta.length;
                        // Second part makes sure that app pointer is not overtaken
                        ap_uint<16> free_space = ((rxSar.appd - rxSar.recvd(15, 0)) - 1);
                        // Check if segment in order and if enough free space is available
                        if ( (fsm_meta.meta.seqNumb == rxSar.recvd) &&
                             (free_space > fsm_meta.meta.length) ) {
                            soRSt_RxSarUpdReq.write(rxSarRecvd(fsm_meta.sessionId, newRecvd, 1));
                            // Build memory address for this segment
                            ap_uint<32> memSegAddr;
                            memSegAddr(31, 30) = 0x0;
                            memSegAddr(29, 16) = fsm_meta.sessionId(13, 0);
                            memSegAddr(15,  0) = fsm_meta.meta.seqNumb.range(15, 0);
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_meta.meta.length));
                            // Only notify about new data available
                            soRan_RxNotif.write(AppNotif(fsm_meta.sessionId,  fsm_meta.meta.length,
                                                         fsm_meta.ip4SrcAddr, fsm_meta.tcpDstPort));
                            soTsd_DropCmd.write(KEEP_CMD);
                        }
                        else {
                            soTsd_DropCmd.write(DROP_CMD);
                        }

                        // OBSOLETE-Sent ACK
                        // OBSOLETE-soSetEvent.write(event(ACK, fsm_meta.sessionID));
                    }
                    if (txSar.count == 3) {
                        soEVe_Event.write(event(RT, fsm_meta.sessionId));
                    }
                    else if (fsm_meta.meta.length != 0) {
                        soEVe_Event.write(event(ACK, fsm_meta.sessionId));
                    }

                    // Reset Retransmit Timer
                    // OBSOLETE soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, (mdh_meta.ackNumb == txSarNextByte)));
                    if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                        switch (tcpState) {
                        case SYN_RECEIVED:  //TODO MAYBE REARRANGE
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, ESTABLISHED, 1));
                            break;
                        case CLOSING:
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, TIME_WAIT, 1));
                            soTIm_CloseTimer.write(fsm_meta.sessionId);
                            break;
                        case LAST_ACK:
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSED, 1));
                            break;
                        default:
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                            break;
                        }
                    }
                    else { // we have to release the lock
                        // reset rtTimer
                        // OBSOLETE rtTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID));
                        soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1)); // or ESTABLISHED
                    }
                } // End of : if ( (tcpState...

                // TODO if timewait just send ACK, can it be time wait??
                else { // state == (CLOSED || SYN_SENT || CLOSE_WAIT || FIN_WAIT_2 || TIME_WAIT)
                    // SENT RST, RFC 793: fig.11
                    soEVe_Event.write(rstEvent(fsm_meta.sessionId, fsm_meta.meta.seqNumb+fsm_meta.meta.length)); // noACK ?
                    // if data is in the pipe it needs to be droppped
                    if (fsm_meta.meta.length != 0) {
                        soTsd_DropCmd.write(DROP_CMD);
                    }
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, QUERY_WR));
                }
                //fsmState = LOAD;
            }
            break;

        case 2:
            //--------------------------------------
            //-- SYN
            //--------------------------------------
            if (DEBUG_LEVEL & TRACE_FSM) printInfo(myName, "Segment is SYN.\n");
            // OBBSOLETE if (!siSTt_SessStateRep.empty())
            if (fsmState == LOAD) {
                rxEngSynCounter++;
                //std::cerr << "SYN Counter: " << rxEngSynCounter << std::endl;
                siSTt_SessStateRep.read(tcpState);
                siRSt_RxSarUpdRep.read(rxSar);
                if (tcpState == CLOSED || tcpState == SYN_SENT) {
                    // Actually this is LISTEN || SYN_SENT
                    // Initialize rxSar, SEQ + phantom byte, last '1' for makes sure appd is initialized
                    soRSt_RxSarUpdReq.write(rxSarRecvd(fsm_meta.sessionId, fsm_meta.meta.seqNumb + 1, 1, 1));
                    // Initialize receive window
                    soTSt_TxSarRdReq.write((rxTxSarQuery(fsm_meta.sessionId, 0, fsm_meta.meta.winSize,
                                              txSar.cong_window, 0, 1))); //TODO maybe include count check
                    // Set SYN_ACK event
                    soEVe_Event.write(event(SYN_ACK, fsm_meta.sessionId));
                    if (DEBUG_LEVEL & TRACE_FSM) printInfo(myName, "Set event SYN_ACK for sessionID %d.\n", fsm_meta.sessionId.to_uint());
                    // Change State to SYN_RECEIVED
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, SYN_RECEIVED, 1));
                }
                else if (tcpState == SYN_RECEIVED) { // && mdh_meta.seqNumb+1 == rxSar.recvd) // Maybe Check for seq
                    // If it is the same SYN, we resent SYN-ACK, almost like quick RT, we could also wait for RT timer
                    if (fsm_meta.meta.seqNumb+1 == rxSar.recvd) {
                        // Retransmit SYN_ACK
                        soEVe_Event.write(event(SYN_ACK, fsm_meta.sessionId, 1));
                        soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                    }
                    else { // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                        soEVe_Event.write(rstEvent(fsm_meta.sessionId, fsm_meta.meta.seqNumb+1)); //length == 0
                        soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSED, 1));
                    }
                }
                else { // Any synchronized state
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(event(ACK_NODELAY, fsm_meta.sessionId));
                    // TODo send RST, has no ACK??
                    // Respond with RST, no ACK, seq ==
                    //eventEngine.write(rstEvent(mdh_meta.seqNumb, mh_meta.length, true));
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                }
            }
            break;

        case 3:
            //--------------------------------------
            //-- SYN_ACK
            //--------------------------------------
            //OBSOLETE if (!siSTt_SessStateRep.empty() && !siTSt_TxSarRdRep.empty())
            if (fsmState == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_RxSarUpdRep.read(rxSar);
                siTSt_TxSarRdRep.read(txSar);
                //OBSOLETE-20190181 soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID,
                //OBSOLETE-20190181                                                (fsm_meta.meta.ackNumb == txSar.nextByte)));
                soTIm_ReTxTimerCmd.write(ReTxTimerCmd(fsm_meta.sessionId,
                                                     (fsm_meta.meta.ackNumb == txSar.nextByte)));
                if ( (tcpState == SYN_SENT) && (fsm_meta.meta.ackNumb == txSar.nextByte) ) { // && !mh_lup.created)
                    //initialize rx_sar, SEQ + phantom byte, last '1' for appd init
                    soRSt_RxSarUpdReq.write(rxSarRecvd(fsm_meta.sessionId,
                                                    fsm_meta.meta.seqNumb + 1, 1, 1));
                    soTSt_TxSarRdReq.write((rxTxSarQuery(fsm_meta.sessionId,
                                                           fsm_meta.meta.ackNumb,
                                                           fsm_meta.meta.winSize,
                                                           txSar.cong_window, 0, 1))); //CHANGE this was added //TODO maybe include count check
                    // Set ACK event
                    soEVe_Event.write(event(ACK_NODELAY, fsm_meta.sessionId));

                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, ESTABLISHED, 1));
                    soTAi_SessOpnSts.write(OpenStatus(fsm_meta.sessionId, true));
                }
                else if (tcpState == SYN_SENT) { //TODO correct answer?
                    // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                    soEVe_Event.write(rstEvent(fsm_meta.sessionId,
                                              fsm_meta.meta.seqNumb+fsm_meta.meta.length+1));
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSED, 1));
                }
                else {
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(event(ACK_NODELAY, fsm_meta.sessionId));
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                }
            }
            break;

        case 5:
            //--------------------------------------
            //-- FIN (_ACK)
            //--------------------------------------
            //OBSOLETE if (!siRSt_RxSarUpdRep.empty() && !siSTt_SessStateRep.empty() && !siTSt_TxSarRdRep.empty())
            if (fsmState == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_RxSarUpdRep.read(rxSar);
                siTSt_TxSarRdRep.read(txSar);
                //OBSOLETE-20190118 soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID,
                //OBSOLETE-20190118                         (fsm_meta.meta.ackNumb == txSar.nextByte)));
                soTIm_ReTxTimerCmd.write(ReTxTimerCmd(fsm_meta.sessionId,
                                                     (fsm_meta.meta.ackNumb == txSar.nextByte)));
                // Check state and if FIN in order, Current out of order FINs are not accepted
                if ( (tcpState == ESTABLISHED || tcpState == FIN_WAIT_1 ||
                      tcpState == FIN_WAIT_2) && (rxSar.recvd == fsm_meta.meta.seqNumb) ) {
                    soTSt_TxSarRdReq.write((rxTxSarQuery(fsm_meta.sessionId,
                                              fsm_meta.meta.ackNumb, fsm_meta.meta.winSize,
                                              txSar.cong_window, txSar.count, 0))); //TODO include count check

                    // +1 for phantom byte, there might be data too
                    soRSt_RxSarUpdReq.write(rxSarRecvd(fsm_meta.sessionId, fsm_meta.meta.seqNumb+fsm_meta.meta.length+1, 1)); //diff to ACK

                    // Clear the probe timer
                    soTIm_ClearProbeTimer.write(fsm_meta.sessionId);

                    // Check if there is payload
                    if (fsm_meta.meta.length != 0) {
                        ap_uint<32> pkgAddr;
                        pkgAddr(31, 30) = 0x0;
                        pkgAddr(29, 16) = fsm_meta.sessionId(13, 0);
                        pkgAddr(15,  0) = fsm_meta.meta.seqNumb(15, 0);
                        soMwr_WrCmd.write(DmCmd(pkgAddr, fsm_meta.meta.length));
                        // Tell Application new data is available and connection got closed
                        soRan_RxNotif.write(AppNotif(fsm_meta.sessionId,    fsm_meta.meta.length,
                                                     fsm_meta.ip4SrcAddr, fsm_meta.tcpDstPort, true));
                        soTsd_DropCmd.write(KEEP_CMD);
                    }
                    else if (tcpState == ESTABLISHED) {
                        // Tell Application connection got closed
                        soRan_RxNotif.write(AppNotif(fsm_meta.sessionId,  fsm_meta.ip4SrcAddr,
                                                     fsm_meta.tcpDstPort, true)); //CLOSE
                    }

                    // Update state
                    if (tcpState == ESTABLISHED) {
                        soEVe_Event.write(event(FIN, fsm_meta.sessionId));
                        soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, LAST_ACK, 1));
                    }
                    else { //FIN_WAIT_1 || FIN_WAIT_2
                        if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                            // check if final FIN is ACK'd -> LAST_ACK
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, TIME_WAIT, 1));
                            soTIm_CloseTimer.write(fsm_meta.sessionId);
                        }
                        else {
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSING, 1));
                        }
                        soEVe_Event.write(event(ACK, fsm_meta.sessionId));
                    }
                }
                else { // NOT (ESTABLISHED || FIN_WAIT_1 || FIN_WAIT_2)
                    soEVe_Event.write(event(ACK, fsm_meta.sessionId));
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                    // If there is payload we need to drop it
                    if (fsm_meta.meta.length != 0) {
                        soTsd_DropCmd.write(DROP_CMD);
                    }
                }
            }
            break;

        default: //TODO MAYBE load everything all the time
            // stateTable is locked, make sure it is released in at the end
            // If there is an ACK we read txSar
            // We always read rxSar
            if (fsmState == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_RxSarUpdRep.read(rxSar); //TODO not sure nb works
                siTSt_TxSarRdRep.read_nb(txSar);
            }
            if (fsmState == LOAD) {
                // Handle if RST
                if (fsm_meta.meta.rst) {
                    if (tcpState == SYN_SENT) {
                        // [TODO this would be a RST,ACK i think]
                        // Check if matching SYN
                        if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                            // Tell application, could not open connection
                            soTAi_SessOpnSts.write(OpenStatus(fsm_meta.sessionId, false));
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSED, 1));
                            //OBSOLETE-20180118 soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, true));
                            soTIm_ReTxTimerCmd.write(ReTxTimerCmd(fsm_meta.sessionId,
                                                                  STOP_TIMER));
                        }
                        else {
                            // Ignore since not matching
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                        }
                    }
                    else {
                        // Check if in window
                        if (fsm_meta.meta.seqNumb == rxSar.recvd) {
                            // Tell application, RST occurred, abort
                            soRan_RxNotif.write(AppNotif(fsm_meta.sessionId, fsm_meta.ip4SrcAddr, fsm_meta.tcpDstPort, true)); //RESET
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, CLOSED, 1)); //TODO maybe some TIME_WAIT state
                            //OBSOLETE-20190181 soTIm_ReTxTimerCmd.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, true));
                            soTIm_ReTxTimerCmd.write(ReTxTimerCmd(fsm_meta.sessionId,
                                                                  STOP_TIMER));
                        }
                        else {
                            // Ignore since not matching window
                            soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                        }
                    }
                }
                else { // Handle non RST bogus packages
                    //TODO maybe sent RST ourselves, or simply ignore
                    // For now ignore, sent ACK??
                    //eventsOut.write(rstEvent(mh_meta.seqNumb, 0, true));
                    soSTt_SessStateReq.write(stateQuery(fsm_meta.sessionId, tcpState, 1));
                } // End of: if (fsm_meta.meta.rst)
            } // if fsm_stat
            break;

        } // End of: switch control_bits

        break;

    } // End of: switch state
}


/*****************************************************************************
 * @brief TCP Segment Dropper (Tsd)
 *
 * @param[in]  siTid_Data,      TCP data stream from Tcp Invalid Dropper (Tid).
 * @param[in]  siMdh_DropCmd,   Drop command from MetaData Handler (Mdh).
 * @param[in]  siFsm_DropCmd,   Drop command from FiniteState Machine (Fsm).
 * @param[out] soMwr_Data,      TCP data stream to Memory Writer (MWr).
 *
 * @details
 *  Drops TCP segments when their metadata did not match and/or is invalid.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pTcpSegmentDropper(
        stream<AxiWord>     &siTid_Data,
        stream<CmdBit>      &siMdh_DropCmd,
        stream<CmdBit>      &siFsm_DropCmd,
        stream<AxiWord>     &soMwr_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tsd");

    static enum FsmState {FSM_RD_DROP_CMD1=0, FSM_RD_DROP_CMD2, FSM_FWD, FSM_DROP} tsd_fsmState = FSM_RD_DROP_CMD1;

    static ap_uint<10>  rxBuffWrAccessLength = 0;
    static ap_uint<1>   rxBuffWriteDoubleAccess = 0;

    switch (tsd_fsmState) {

    case FSM_RD_DROP_CMD1:
        if (!siMdh_DropCmd.empty()) {
            CmdBit dropCmd = siMdh_DropCmd.read();
            //OBSOLETE-20190130 (dropCmd) ? tsd_fsmState = FSM_DROP : tsd_fsmState = FSM_RD_DROP_CMD2;
            if (dropCmd) {
                tsd_fsmState = FSM_DROP;
                printWarn(myName, "[Mdh] is requesting to drop this packet.\n");
            }
            else
                tsd_fsmState = FSM_RD_DROP_CMD2;
        }
        break;
    case FSM_RD_DROP_CMD2:
        if (!siFsm_DropCmd.empty()) {
            CmdBit dropCmd = siFsm_DropCmd.read();
            //OBSOLETE-20190130 (dropCmd) ? tsd_fsmState = FSM_DROP : tsd_fsmState = FSM_FWD;
            if (dropCmd) {
                tsd_fsmState = FSM_DROP;
                printWarn(myName, "[Fsm] is requesting to drop this packet.\n");
            }
            else
                tsd_fsmState = FSM_FWD;
        }
        break;
    case FSM_FWD:
        if(!siTid_Data.empty() && !soMwr_Data.full()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast)
                tsd_fsmState = FSM_RD_DROP_CMD1;
            soMwr_Data.write(currWord);
        }
        break;
    case FSM_DROP:
        if(!siTid_Data.empty()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast)
                tsd_fsmState = FSM_RD_DROP_CMD1;
        }
        break;

    } // End of: switch
}


/*****************************************************************************
 * @brief Rx Application Notifier (Ran)
 *
 * @param[in]  siMEM_WrSts,    The memory write status from memory data mover [MEM].
 * @param[in]  siFsm_Notif,    Rx data notification from [FiniteStateMachine].
 * @param[out] soRxNotif,      Outgoing Rx notification for the application.
 * @param[in]  siMwr_SplitSeg, This TCP data segment is split in two Rx memory buffers.
 *
 * @details
 *  Delays the notifications to the application until the data is actually
 *   written into the physical DRAM memory.
 *
 * @todo
 *  Handle unsuccessful write to memory.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pRxAppNotifier(
        stream<DmSts>         &siMEM_WrSts,
        stream<AppNotif>      &siFsm_Notif,
        stream<AppNotif>      &soRxNotif,
        stream<ap_uint<1> >   &siMwr_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Ran");

    //-- LOCAL STREAMS
    static stream<AppNotif>        sRxNotifFifo("sRxNotifFifo");
    #pragma HLS STREAM    variable=sRxNotifFifo depth=32 // Depends on memory delay
    #pragma HLS DATA_PACK variable=sRxNotifFifo

    static ap_uint<1>       rxAppNotificationDoubleAccessFlag = false;
    static ap_uint<5>       rand_fifoCount = 0;
    static DmSts            rxAppNotificationStatus1, rxAppNotificationStatus2;
    static AppNotif         rxAppNotification;

    if (rxAppNotificationDoubleAccessFlag == true) {
        if(!siMEM_WrSts.empty()) {
            siMEM_WrSts.read(rxAppNotificationStatus2);
            rand_fifoCount--;
            if (rxAppNotificationStatus1.okay && rxAppNotificationStatus2.okay)
                soRxNotif.write(rxAppNotification);
            rxAppNotificationDoubleAccessFlag = false;
            if (DEBUG_LEVEL & TRACE_RAN) {
                printInfo(myName, "Reading memory write status: (DoubleAccess=1; OK=%d).\n",
                                   rxAppNotificationStatus2.okay.to_int());
            }
        }
    }
    else {  // if (rxAppNotificationDoubleAccessFlag == false) {
        if(!siMEM_WrSts.empty() && !sRxNotifFifo.empty() && !siMwr_SplitSeg.empty()) {
            siMEM_WrSts.read(rxAppNotificationStatus1);
            sRxNotifFifo.read(rxAppNotification);
            if (DEBUG_LEVEL & TRACE_RAN) {
                printInfo(myName, "Reading memory write status: (DoubleAccess=0; OK=%d).\n",
                                   rxAppNotificationStatus1.okay.to_int());
            }
            // Read the double notification flag. If one then go and w8 for the second status
            rxAppNotificationDoubleAccessFlag = siMwr_SplitSeg.read();
            if (rxAppNotificationDoubleAccessFlag == 0) {
                // The memory access was not broken down in two for this segment
                rand_fifoCount--;
                if (rxAppNotificationStatus1.okay) {
                    // Output the notification
                    soRxNotif.write(rxAppNotification);
                }
            }
            //TODO else, we are screwed since the ACK is already sent
        }
        else if (!siFsm_Notif.empty() && (rand_fifoCount < 31)) {
            siFsm_Notif.read(rxAppNotification);
            if (rxAppNotification.tcpSegLen != 0) {
                sRxNotifFifo.write(rxAppNotification);
                rand_fifoCount++;
            }
            else
                soRxNotif.write(rxAppNotification);
        }
    }
}

/*****************************************************************************
 * @brief Event Multiplexer (Evm)
 *
 * @param[in]  siMdh_Event, Event from [MetaDataHandler].
 * @param[in]  siFsm_Event, Event from [FiniteStateMachine].
 * @param[out] soEVe_Event, Event to   [EVentEngine].
 *
 * @details
 *  Takes two events as inputs and muxes them on a single output. Note
 *   that the first input has priority over the second one.
 *
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pEventMultiplexer(
        stream<extendedEvent>    &siMdh_Event,
        stream<event>            &siFsm_Event,
        stream<extendedEvent>    &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE

    const char *myName  = concat3(THIS_NAME, "/", "Evm");

    if (!siMdh_Event.empty()) {
        soEVe_Event.write(siMdh_Event.read());
    }
    else if (!siFsm_Event.empty()) {
        soEVe_Event.write(siFsm_Event.read());
    }
}

/*****************************************************************************
 * @brief Memory Writer (Mwr)
 *
 * @param[in]  siTsd_MemWrData, Memory data write from the Tcp Segment Dropper (Tid).
 * @param[in]  siFsm_MemWrCmd,  Memory write command from the Finite State Machine (Fsm).
 * @param[out] soMEM_WrCmd,     Memory write command to the data mover of memory sub-system (MEM).
 * @param[out] soMEM_WrData,    Memory data write stream [MEM].
 * @param[out] soRan_SplitSeg,  Segment is broken in two Rx memory buffers.
 *
 * @details
 *  Front memory controller process for writing data into the external DRAM.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pMemWriter(
        stream<AxiWord>     &siTsd_MemWrData,
        stream<DmCmd>       &siFsm_MemWrCmd,
        stream<DmCmd>       &soMEM_WrCmd,
        stream<AxiWord>     &soMEM_WrData,
        stream<ap_uint<1> > &soRan_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Mwr");

    static DmCmd       rxMemWriterCmd = DmCmd(0, 0);
    static ap_uint<16> rxEngBreakTemp = 0;
    static uint8_t     lengthBuffer   = 0;
    static ap_uint<3>  rxEngAccessResidue = 0;
    static bool        txAppBreakdown = false;

    static AxiWord     pushWord = AxiWord(0, 0xFF, 0);

    static enum FsmState {IDLE,       WRFIRST,
                          EVALSECOND, WRSECONDSTR,
                          ALIGNED,    RESIDUE} mwr_fsmState;

    switch (mwr_fsmState) {

    case IDLE:
        if (!siFsm_MemWrCmd.empty() && !soMEM_WrCmd.full() && !soRan_SplitSeg.full()) {
            rxMemWriterCmd = siFsm_MemWrCmd.read();
            DmCmd tempCmd = rxMemWriterCmd;
            if ((rxMemWriterCmd.saddr.range(15, 0) + rxMemWriterCmd.bbt) > RXMEMBUF) {
                // Break this segment in two memory accesses because TCP Rx memory buffer wraps around
                rxEngBreakTemp = RXMEMBUF - rxMemWriterCmd.saddr;
                rxMemWriterCmd.bbt -= rxEngBreakTemp;
                tempCmd = DmCmd(rxMemWriterCmd.saddr, rxEngBreakTemp);
                txAppBreakdown = true;
                if (DEBUG_LEVEL & TRACE_MWR)
                    printInfo(myName, "TCP memory buffer wraps around: This segment will be broken in two memory buffers.\n");
            }
            else {
                rxEngBreakTemp = rxMemWriterCmd.bbt;
            }
            soMEM_WrCmd.write(tempCmd);
            soRan_SplitSeg.write(txAppBreakdown);
            if (DEBUG_LEVEL & TRACE_MWR)
                printDmCmd(myName, tempCmd);
            mwr_fsmState = WRFIRST;
        }
        break;

    case WRFIRST:
        if (!siTsd_MemWrData.empty() && !soMEM_WrData.full()) {
            siTsd_MemWrData.read(pushWord);
            AxiWord outputWord = pushWord;
            //OBSOLETE-20181213 ap_uint<4> byteCount = keepMapping(pushWord.tkeep);
            ap_uint<4> byteCount = keepToLen(pushWord.tkeep);
            if (rxEngBreakTemp > 8) {
                rxEngBreakTemp -= 8;
            }
            else {
                if (txAppBreakdown == true) {
                    // Changes are to go in here
                    // If the word is not perfectly aligned then there is some magic to be worked.
                    if (rxMemWriterCmd.saddr.range(15, 0) % 8 != 0) {
                        outputWord.tkeep = lenToKeep(rxEngBreakTemp);
                    }
                    outputWord.tlast = 1;
                    mwr_fsmState = EVALSECOND;
                    rxEngAccessResidue = byteCount - rxEngBreakTemp;
                    lengthBuffer = rxEngBreakTemp;  // Buffer the number of bits consumed.
                }
                else {
                    mwr_fsmState = IDLE;
                }
            }
            soMEM_WrData.write(outputWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, outputWord);
        }
        break;

    case EVALSECOND:
        if (!soMEM_WrCmd.full()) {
            if (rxMemWriterCmd.saddr.range(15, 0) % 8 == 0)
                mwr_fsmState = ALIGNED;
            //else if (rxMemWriterCmd.bbt + rxEngAccessResidue > 8 || rxEngAccessResidue > 0)
            else if (rxMemWriterCmd.bbt - rxEngAccessResidue > 0)
                mwr_fsmState = WRSECONDSTR;
            else
                mwr_fsmState = RESIDUE;
            rxMemWriterCmd.saddr.range(15, 0) = 0;
            rxEngBreakTemp = rxMemWriterCmd.bbt;
            DmCmd tempCmd = DmCmd(rxMemWriterCmd.saddr, rxEngBreakTemp);
            soMEM_WrCmd.write(tempCmd);
            if (DEBUG_LEVEL & TRACE_MWR) printDmCmd(myName, tempCmd);
            txAppBreakdown = false;
        }
        break;

    case ALIGNED:   // This is the non-realignment state
        if (!siTsd_MemWrData.empty() & !soMEM_WrData.full()) {
            siTsd_MemWrData.read(pushWord);
            soMEM_WrData.write(pushWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, pushWord);
            if (pushWord.tlast == 1)
                mwr_fsmState = IDLE;
        }
        break;

    case WRSECONDSTR: // We go into this state when we need to realign things
        if (!siTsd_MemWrData.empty() && !soMEM_WrData.full()) {
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            pushWord = siTsd_MemWrData.read();
            outputWord.tdata.range(63, (8-lengthBuffer)*8) = pushWord.tdata.range((lengthBuffer * 8), 0 );

            if (pushWord.tlast == 1) {
                if (rxEngBreakTemp - rxEngAccessResidue > lengthBuffer) {
                    // In this case there's residue to be handled
                    rxEngBreakTemp -= 8;
                    mwr_fsmState = RESIDUE;
                }
                else {
                    outputWord.tkeep = returnKeep(rxEngBreakTemp);
                    outputWord.tlast = 1;
                    mwr_fsmState = IDLE;
                }
            }
            else
                rxEngBreakTemp -= 8;
            soMEM_WrData.write(outputWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, outputWord);
        }
        break;

    case RESIDUE:
        if (!soMEM_WrData.full()) {
            AxiWord outputWord = AxiWord(0, returnKeep(rxEngBreakTemp), 1);
            outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            soMEM_WrData.write(outputWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, outputWord);
            mwr_fsmState = IDLE;
        }
        break;

    }
}

/*****************************************************************************
 * @brief Debug process that keeps track an internal counter.
 *
 * @param[out]  poDBG_CycCnt, a reference to RXe's cycle counter.
 *
 * @ingroup rx_engine
 *****************************************************************************/
/*** OBSOLETE-20190505 ******
void pDebug(
        ap_uint<32>     &poDBG_CycCnt)
{
    static ap_uint<32> rxeCycCnt = 0;

    if (DEBUG_LEVEL) {
        if ((rxeCycCnt % 100) == 0) {
            printInfo(THIS_NAME, "-- [@%4.4u] -----------------------------\n", rxeCycCnt.to_uint());
        }
    }
    rxeCycCnt++;
    //OBSOLETE-20190506 poDBG_CycCnt = rxeCycCnt;
}
*****************************/

/*****************************************************************************
 * @brief The rx_engine (RXe) processes the data packets received from IPRX.
 *
 * @param[in]  siIPRX_Data,         IP4 data stream form IPRX.
 * @param[out] soSLc_SessLookupReq, Session lookup request to SessionLookupController (SLc).
 * @param[in]  siSLc_SessLookupRep, Session lookup reply from [SLc].
 * @param[out] soSTt_SessStateReq,  Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep,  Session state reply from [STt].
 * @param[out] soPRt_PortStateReq,  Port state request to PortTable (PRt).
 * @param[in]  siPRt_PortStateRep,  Port state reply from [PRt].
 * @param[out] soRSt_RxSarUpdReq,   Rx session SAR update request to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarUpdRep    Rx session SAR update reply from [RSt].
 * @param[out] soTSt_TxSarRdReq,    Tx session SAR read request to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRdRep     Tx session SAR read peply from [TSt].
 * @param[out] soTIm_ReTxTimerCmd,  Command for a retransmit timer to Timers (TIm).
 * @param[out] soTIm_ClearProbeTimer,Clear the probe timer command to [TIm]..
 * @param[out] soTIm_CloseTimer,    Close session timer command to [TIm].
 * @param[out] soEVe_SetEvent,      Event forward to EventEngine (EVe).
 * @param[out] soTAi_SessOpnSts,    Open status of the session to TxAppInterface (TAi).
 * @param[out] soRAi_RxNotif,       Rx data notification to RxAppInterface (RAi).
 * @param[out] soMEM_WrCmd,         Memory write command to MemorySubSystem (MEM).
 * @param[out] soMEM_WrData,        Memory data write stream to [MEM].
 * @param[in]  siMEM_WrSts,         Memory write status from [MEM].
 *
 * @details
 *  When a new packet enters the engine its TCP checksum is tested, the header
 *   is parsed and some more checks are done. Next, it is evaluated by the main
 *   TCP state machine which triggers events and updates the data structures
 *   according to the type of received packet. Finally, if the packet contains
 *   valid payload, it is stored in external DDR4 memory and the application is
 *   notified about the arrival of new data.
 *
 * @return Nothing.
 *
 * @ingroup rx_engine
 ******************************************************************************/
void rx_engine(
        stream<Ip4overAxi>              &siIPRX_Pkt,
        stream<sessionLookupQuery>      &soSLc_SessLookupReq,
        stream<sessionLookupReply>      &siSLc_SessLookupRep,
        stream<stateQuery>              &soSTt_SessStateReq,
        stream<sessionState>            &siSTt_SessStateRep,
        stream<TcpPort>                 &soPRt_PortStateReq,
        stream<StsBit>                  &siPRt_PortStateRep,
        stream<rxSarRecvd>              &soRSt_RxSarUpdReq,
        stream<rxSarEntry>              &siRSt_RxSarUpdRep,
        stream<rxTxSarQuery>            &soTSt_TxSarRdReq,
        stream<rxTxSarReply>            &siTSt_TxSarRdRep,
        stream<ReTxTimerCmd>            &soTIm_ReTxTimerCmd,
		//OBSOLETE-20190118 stream<rxRetransmitTimerUpdate> &soTIm_ReTxTimerCmd,
        stream<ap_uint<16> >            &soTIm_ClearProbeTimer,
        stream<ap_uint<16> >            &soTIm_CloseTimer,
        stream<extendedEvent>           &soEVe_SetEvent,
        stream<OpenStatus>              &soTAi_SessOpnSts,
        stream<AppNotif>                &soRAi_RxNotif,
        stream<DmCmd>                   &soMEM_WrCmd,
        stream<AxiWord>                 &soMEM_WrData,
        stream<DmSts>                   &siMEM_WrSts)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tcp Length Extract (Tle) ---------------------------------------------
    static stream<TcpWord>          sTleToIph_TcpSeg       ("sTleToIph_TcpSeg");
    #pragma HLS stream     variable=sTleToIph_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=sTleToIph_TcpSeg

    static stream<TcpSegLen>        sTleToIph_TcpSegLen    ("sTleToIph_TcpSegLen");
    #pragma HLS stream     variable=sTleToIph_TcpSegLen    depth=2

    //-- Insert Pseudo Header (Iph) -------------------------------------------
    static stream<TcpWord>          sIphToCsa_TcpSeg       ("sIphToCsa_TcpSeg");
    #pragma    HLS stream  variable=sIphToCsa_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=sIphToCsa_TcpSeg

    //-- CheckSum Accumulator (Csa) -------------------------------------------
    static stream<TcpWord>          sCsaToTid_Data         ("sCsaToTid_Data");
    #pragma HLS stream     variable=sCsaToTid_Data         depth=256 //critical, tcp checksum computation
    #pragma HLS DATA_PACK  variable=sCsaToTid_Data

    static stream<bool>             sCsaToTid_DataValid    ("sCsaToTid_DataValid");
    #pragma HLS stream     variable=sCsaToTid_DataValid    depth=2

    static stream<rxEngineMetaData> sCsaToMdh_Meta         ("sCsaToTid_Meta");
    #pragma HLS stream     variable=sCsaToMdh_Meta         depth=2
    #pragma HLS DATA_PACK  variable=sCsaToMdh_Meta

    static stream<AxiSocketPair>    sCsaToMdh_SockPair     ("sCsaToMdh_SockPair");
    #pragma HLS stream     variable=sCsaToMdh_SockPair     depth=2
    #pragma HLS DATA_PACK  variable=sCsaToMdh_SockPair

    static stream<AxiWord>          sTidToTsd_Data         ("sTidToTsd_Data");
    #pragma HLS stream     variable=sTidToTsd_Data         depth=8
    #pragma HLS DATA_PACK  variable=sTidToTsd_Data

    //-- Tcp Invalid dropper (Tid) --------------------------------------------
    static stream<AxiWord>          sTsdToMwr_Data         ("sTsdToMwr_Data");
    #pragma HLS stream     variable=sTsdToMwr_Data         depth=16
    #pragma HLS DATA_PACK  variable=sTsdToMwr_Data

    //-- MetaData Handler (Mdh) -----------------------------------------------
    static stream<extendedEvent>    sMdhToEvm_Event        ("sMdhToEvm_Event");
    #pragma HLS stream     variable=sMdhToEvm_Event        depth=2
    #pragma HLS DATA_PACK  variable=sMdhToEvm_Event

    static stream<CmdBit>           sMdhToTsd_DropCmd      ("sMdhToTsd_DropCmd");
    #pragma HLS stream     variable=sMdhToTsd_DropCmd      depth=2
    //OBSOLETE-20190426 #pragma HLS DATA_PACK  variable=sMdhToTsd_DropCmd

    static stream<rxFsmMetaData>    sMdhToFsm_Meta         ("sMdhToFsm_Meta");
    #pragma HLS stream     variable=sMdhToFsm_Meta         depth=2
    #pragma HLS DATA_PACK  variable=sMdhToFsm_Meta

    //-- Finite State Machine (Fsm) -------------------------------------------
    static stream<CmdBit>           sFsmToTsd_DropCmd      ("sFsmToTsd_DropCmd");
    #pragma HLS stream     variable=sFsmToTsd_DropCmd      depth=2
    //OBSOLETE-20190426 #pragma HLS DATA_PACK  variable=sFsmToTsd_DropCmd

    static stream<AppNotif>         sFsmToRan_Notif        ("sFsmToRan_Notif");
    #pragma HLS stream     variable=sFsmToRan_Notif        depth=8  // This depends on the memory delay
    #pragma HLS DATA_PACK  variable=sFsmToRan_Notif

    static stream<event>            sFsmToEvm_Event        ("sFsmToEvm_Event");
    #pragma HLS stream     variable=sFsmToEvm_Event        depth=2
    #pragma HLS DATA_PACK  variable=sFsmToEvm_Event

    static stream<DmCmd>            sFsmToMwr_WrCmd        ("sFsmToMwr_WrCmd");
    #pragma HLS stream     variable=sFsmToMwr_WrCmd        depth=8
    #pragma HLS DATA_PACK  variable=sFsmToMwr_WrCmd

    //-- Memory Writer (Mwr) --------------------------------------------------
    static stream<ap_uint<1> >      sMwrToRan_SplitSeg     ("sMwrToRan_SplitSeg");
    #pragma HLS stream     variable=sMwrToRan_SplitSeg     depth=8

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTcpLengthExtract(
            siIPRX_Pkt,
            sTleToIph_TcpSeg,
            sTleToIph_TcpSegLen);

    pInsertPseudoHeader(
            sTleToIph_TcpSeg,
            sTleToIph_TcpSegLen,
            sIphToCsa_TcpSeg);

    pCheckSumAccumulator(
            sIphToCsa_TcpSeg,
            sCsaToTid_Data,
            sCsaToTid_DataValid,
            sCsaToMdh_Meta,
            sCsaToMdh_SockPair,
            soPRt_PortStateReq);

    pTcpInvalidDropper(
            sCsaToTid_Data,
            sCsaToTid_DataValid,
            sTidToTsd_Data);

    pMetaDataHandler(
            sCsaToMdh_Meta,
            sCsaToMdh_SockPair,
            siSLc_SessLookupRep,
            siPRt_PortStateRep,
            soSLc_SessLookupReq,
            sMdhToEvm_Event,
            sMdhToTsd_DropCmd,
            sMdhToFsm_Meta);

    pFiniteStateMachine(
            sMdhToFsm_Meta,
            siSTt_SessStateRep,
            siRSt_RxSarUpdRep,
            siTSt_TxSarRdRep,
            soSTt_SessStateReq,
            soRSt_RxSarUpdReq,
            soTSt_TxSarRdReq,
            soTIm_ReTxTimerCmd,
            soTIm_ClearProbeTimer,
            soTIm_CloseTimer,
            soTAi_SessOpnSts,
            sFsmToEvm_Event,
            sFsmToTsd_DropCmd,
            sFsmToMwr_WrCmd,
            sFsmToRan_Notif);

    pTcpSegmentDropper(
            sTidToTsd_Data,
            sMdhToTsd_DropCmd,
            sFsmToTsd_DropCmd,
            sTsdToMwr_Data);

    pMemWriter(
            sTsdToMwr_Data,
            sFsmToMwr_WrCmd,
            soMEM_WrCmd,
            soMEM_WrData,
            sMwrToRan_SplitSeg);

    pRxAppNotifier(
            siMEM_WrSts,
            sFsmToRan_Notif,
            soRAi_RxNotif,
            sMwrToRan_SplitSeg);

    pEventMultiplexer(
            sMdhToEvm_Event,
            sFsmToEvm_Event,
            soEVe_SetEvent);

}
