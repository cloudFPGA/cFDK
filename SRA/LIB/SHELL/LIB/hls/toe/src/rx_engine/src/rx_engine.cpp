/************************************************
Copyright (c) 2015, Xilinx, Inc.
Copyright (c) 2016-2019, IBM Research.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : rx_engine.cpp
 * @brief      : Rx Engine (RXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
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

#define DEBUG_LEVEL (TRACE_MDH | TRACE_FSM | TRACE_MWR | TRACE_RAN)

enum DropCmd {KEEP_CMD=false, DROP_CMD=true};


/*****************************************************************************
 * @brief TCP length extraction (Tle).
 *
 * @param[in]  siIPRX_Pkt,    IP4 packet stream form IPRX.
 * @param[out] soPseudoPkt,   A pseudo TCP/IP packet (.i.e, IP-SA + IP-DA + TCP)
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
 ******************************************************************************/
void pTcpLengthExtract(
        stream<Ip4overMac>     &siIPRX_Pkt,
        stream<Ip4overMac>     &soPseudoPkt,
        stream<TcpSegLen>      &soTcpSegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tle");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<4>          tle_wordCount=0;
    #pragma HLS RESET variable=tle_wordCount
    static bool                tle_insertWord=false;
    #pragma HLS RESET variable=tle_insertWord
    static bool                tle_wasLast=false;
    #pragma HLS RESET variable=tle_wasLast

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static bool         tle_shift;
    static Ip4HdrLen    tle_ip4HdrLen;
    static Ip4TotalLen  tle_ip4TotalLen;
    static Ip4DatLen    tle_ipDataLen;
    static Ip4overMac   tle_prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    Ip4overMac          sendWord;

    if (tle_insertWord) {
        sendWord = Ip4overMac(0, 0xFF, 0);
        soPseudoPkt.write(sendWord);
        tle_insertWord = false;
        if (DEBUG_LEVEL & TRACE_TLE) {
            printAxiWord(myName, sendWord);
        }
    }
    else if (!siIPRX_Pkt.empty() && !tle_wasLast) {
        Ip4overMac currWord = siIPRX_Pkt.read();

        switch (tle_wordCount) {
        case WORD_0:
            tle_ip4HdrLen  = currWord.tdata(3, 0);
            //OBSOLETE_20191118 tle_ip4TotalLen = byteSwap16(currWord.tdata(31, 16));
            tle_ip4TotalLen = currWord.getIp4TotalLen();
            // Compute length of IPv4 data (.i.e. the TCP segment length)
            tle_ipDataLen  = tle_ip4TotalLen - (tle_ip4HdrLen * 4);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case WORD_1:
            // Forward length of IPv4 data
            soTcpSegLen.write(tle_ipDataLen);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case WORD_2:
            // Forward destination IP address
            // Warning, half of this address is now in 'prevWord'
            sendWord = Ip4overMac((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                  (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                  (currWord.tkeep[4] == 0));
            soPseudoPkt.write(sendWord);
            tle_ip4HdrLen -= 1;  // We just processed the last 4 bytes of the IP header
            tle_insertWord = true;
            tle_wordCount++;
            if (DEBUG_LEVEL & TRACE_TLE) {
                printAxiWord(myName, sendWord);
            }
            break;
        case WORD_3:
            switch (tle_ip4HdrLen) {
            case 0: // Half of prevWord contains valuable data and currWord is full of valuable
                sendWord = Ip4overMac((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                      (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                      (currWord.tkeep[4] == 0));
                soPseudoPkt.write(sendWord);
                tle_shift = true;
                tle_ip4HdrLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL & TRACE_TLE) {
                	printAxiWord(myName, sendWord);
                }
                break;
            case 1: // The prevWord contains garbage data, but currWord is valuable
                sendWord = Ip4overMac(currWord.tdata, currWord.tkeep, currWord.tlast);
                soPseudoPkt.write(sendWord);
                tle_shift = false;
                tle_ip4HdrLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL & TRACE_TLE) {
                	printAxiWord(myName, sendWord);
                }
                break;
            default: // The prevWord contains garbage data, currWord at least half garbage
                // Drop everything else
                tle_ip4HdrLen -= 2;
                break;
            }
            break;
        default:
            if (tle_shift) {
                sendWord = Ip4overMac((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                      (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                      (currWord.tkeep[4] == 0));
                soPseudoPkt.write(sendWord);
                if (DEBUG_LEVEL & TRACE_TLE) {
                	printAxiWord(myName, sendWord);
                }
            }
            else {
                sendWord = Ip4overMac(currWord.tdata, currWord.tkeep, currWord.tlast);
                soPseudoPkt.write(sendWord);
                if (DEBUG_LEVEL & TRACE_TLE) {
                	printAxiWord(myName, sendWord);
                }
            }
            break;

        } // End of: switch (tle_wordCount)

        tle_prevWord = currWord;
        if (currWord.tlast) {
            tle_wordCount = 0;
            tle_wasLast = !sendWord.tlast;
        }

    } // End of: else if (!siIPRX_Data.empty() && !tle_wasLast) {
    else if (tle_wasLast) { // Assumption has to be shift
        // Send remaining data
        Ip4overMac sendWord = Ip4overMac(0, 0, 1);
        sendWord.tdata(31, 0) = tle_prevWord.tdata(63, 32);
        sendWord.tkeep( 3, 0) = tle_prevWord.tkeep( 7,  4);
        soPseudoPkt.write(sendWord);
        tle_wasLast = false;
        if (DEBUG_LEVEL & TRACE_TLE) {
        	printAxiWord(myName, sendWord);
        }
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
 ******************************************************************************/
void pInsertPseudoHeader(
        stream<Ip4overMac>  &siTle_PseudoPkt,
        stream<TcpSegLen>   &siTle_TcpSegLen,
        stream<Ip4overMac>  &soCsa_PseudoPkt)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Iph");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                iph_wasLast=false;
    #pragma HLS RESET variable=iph_wasLast
    static ap_uint<2>          iph_wordCount=0;
    #pragma HLS RESET variable=iph_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static Ip4overMac   iph_prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    ValBit              valid;
    TcpSegLen           tcpSegLen;
    Ip4overMac          currWord;
    Ip4overMac          sendWord;

    if (iph_wasLast) {
        sendWord.tdata(31, 0) = iph_prevWord.tdata(63,32);
        sendWord.tkeep( 3, 0) = iph_prevWord.tkeep( 7, 4);
        sendWord.tkeep( 7, 4) = 0x0;
        sendWord.tlast        = 0x1;
        soCsa_PseudoPkt.write(sendWord);
        iph_wasLast = false;
        if (DEBUG_LEVEL & TRACE_IPH) {
            printAxiWord(myName, sendWord);
        }
    }
    else if(!siTle_PseudoPkt.empty()) {
        switch (iph_wordCount) {
        case WORD_0:
            siTle_PseudoPkt.read(currWord);
            iph_wordCount++;
            break;
        case WORD_1:
            siTle_PseudoPkt.read(currWord);
            sendWord = iph_prevWord;
            // Forward IP-DA & IP-SA
            soCsa_PseudoPkt.write(sendWord);
            iph_wordCount++;
            if (DEBUG_LEVEL & TRACE_IPH) {
                printAxiWord(myName, sendWord);
            }
            break;
        case WORD_2:
            // [TODO - Use the Ip4overMac methods here]
            if (!siTle_TcpSegLen.empty()) {
                siTle_PseudoPkt.read(currWord);
                siTle_TcpSegLen.read(tcpSegLen);
                // Forward Protocol and Segment length
                sendWord.tdata(15,  0) = 0x0600;        // 06 is for TCP
                sendWord.tdata(31, 16) = byteSwap16(tcpSegLen);
                // Forward TCP-SP & TCP-DP
                sendWord.tdata(63, 32) = currWord.tdata(31, 0);
                sendWord.tkeep         = 0xFF;
                sendWord.tlast         = 0;
                soCsa_PseudoPkt.write(sendWord);
                iph_wordCount++;
                if (DEBUG_LEVEL & TRACE_IPH) printAxiWord(myName, sendWord);
            }
            break;
        default:
            siTle_PseudoPkt.read(currWord);
            // Forward { Sequence Number, Acknowledgment Number } or
            //         { Flags, Window, Checksum, UrgentPointer } or
            //         { Data }
            sendWord.tdata.range(31,  0) = iph_prevWord.tdata.range(63, 32);
            sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
            sendWord.tkeep.range( 3,  0) = iph_prevWord.tkeep.range(7, 4);
            sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
            sendWord.tlast               = (currWord.tkeep[4] == 0); // see format of the incoming segment
            soCsa_PseudoPkt.write(sendWord);
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
 *  This process extracts the data section from the incoming segment and
 *   forwards them to the TCP Invalid Dropper (Tid) together with a valid
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
 ******************************************************************************/
void pCheckSumAccumulator(
        stream<Ip4overMac>        &siCsa_PseudoPkt,
        stream<AxiWord>           &soTid_Data,
        stream<ValBit>            &soTid_DataVal,
        stream<RXeMeta>           &soMdh_Meta,
        stream<LE_SocketPair>     &soMdh_SockPair,
        stream<TcpPort>           &soPRt_GetState)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Csa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    //OBSOLETE_20191118 static ap_uint<16>           csa_wordCount   = 0;
    static ap_uint<3>			csa_wordCount=0;
    #pragma HLS RESET  variable=csa_wordCount
    static bool                 csa_doCSumVerif=false;
    #pragma HLS RESET  variable=csa_doCSumVerif
    static bool                 csa_wasLast=false;
    #pragma HLS RESET  variable=csa_wasLast
    static ap_uint<3>           csa_cc_state=0;
    #pragma HLS RESET  variable=csa_cc_state
    static ap_uint<17>          csa_tcp_sums[4]={0, 0, 0, 0};
	#pragma HLS RESET  variable=csa_tcp_sums
	#pragma HLS ARRAY_PARTITION \
					   variable=csa_tcp_sums complete dim=1

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpDataOff       csa_dataOffset;
    static bool             csa_doShift;
    static LE_SocketPair    csa_leSessTuple;
    static RXeMeta          csa_meta;
    static LE_TcpDstPort    csa_leTcpDstPort;
    static LE_TcpChecksum   csa_leTcpCSum;
    static ap_uint<36>      csa_halfWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    Ip4overMac              currWord;
    Ip4overMac              sendWord;

    if (!siCsa_PseudoPkt.empty() && !csa_doCSumVerif) {
        siCsa_PseudoPkt.read(currWord);
        switch (csa_wordCount) {
        // [TODO - Use the Ip4overMac methods here]
        case WORD_0:
            csa_dataOffset = 0xF;
            csa_doShift = false;
            // Get IP-SA & IP-DA
            //  Warning: Remember that IP addresses come in little-endian order
            csa_leSessTuple.src.addr = currWord.tdata(31,  0);
            csa_leSessTuple.dst.addr = currWord.tdata(63, 32);
            sendWord.tlast           = currWord.tlast;
            csa_wordCount++;
            break;
        case WORD_1:
            // Get Segment length
            csa_meta.length = byteSwap16(currWord.tdata(31, 16));
            // Get TCP-SP & TCP-DP
            //  Warning: Remember that TCP ports come in little-endian order
            csa_leSessTuple.src.port = currWord.tdata(47, 32);
            csa_leSessTuple.dst.port = currWord.tdata(63, 48);
            csa_leTcpDstPort      = currWord.tdata(63, 48);
            sendWord.tlast         = currWord.tlast;
            csa_wordCount++;
            break;
        case WORD_2:
            // Get Sequence and Acknowledgment Numbers
            csa_meta.seqNumb = byteSwap32(currWord.tdata(31, 0));
            csa_meta.ackNumb = byteSwap32(currWord.tdata(63, 32));
            sendWord.tlast   = currWord.tlast;
            csa_wordCount++;
            break;
        case WORD_3:
            // Get Data Offset
            csa_dataOffset   = currWord.tdata.range(7, 4);
            csa_meta.length -= (csa_dataOffset * 4);
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
            csa_leTcpCSum  = currWord.tdata(47, 32);
            sendWord.tlast = currWord.tlast;
            csa_wordCount++;
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
                csa_halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                csa_halfWord.range(35, 32) = currWord.tkeep.range( 7,  4);
                sendWord.tlast = (currWord.tkeep[4] == 0);
            }
            else {    // csa_dataOffset == 5 (or less)
                if (!csa_doShift) {
                    sendWord = currWord;
                    soTid_Data.write(sendWord);
                }
                else {
                    sendWord.tdata.range(31,  0) = csa_halfWord.range(31, 0);
                    sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
                    sendWord.tkeep.range( 3,  0) = csa_halfWord.range(35, 32);
                    sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
                    sendWord.tlast = (currWord.tkeep[4] == 0);
                    soTid_Data.write(sendWord);
                    csa_halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                    csa_halfWord.range(35, 32) = currWord.tkeep.range(7, 4);
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

        //OBSOLETE_20191118 csa_wordCount++;

        if(currWord.tlast == 1) {
            csa_wordCount = 0;
            csa_wasLast = !sendWord.tlast;
            csa_doCSumVerif = true;
            if (DEBUG_LEVEL & TRACE_CSA)
                printAxiWord(myName, currWord);
        }

    } // End of: if (!siIph_Data.empty() && !csa_doCSumVerif)

    else if(csa_wasLast) {
        if (csa_meta.length != 0) {
            sendWord.tdata.range(31,  0) = csa_halfWord.range(31,  0);
            sendWord.tdata.range(63, 32) = 0;
            sendWord.tkeep.range( 3,  0) = csa_halfWord.range(35, 32);
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
                    soMdh_SockPair.write(csa_leSessTuple);
                    // Forward to TcpInvalidDropper
                    if (csa_meta.length != 0) {
                        soTid_DataVal.write(OK);
                        if (DEBUG_LEVEL & TRACE_CSA) {
                            printInfo(myName, "Received end-of-packet. Checksum is correct.\n");
                        }
                    }
                    // Request state of TCP_DP
                    soPRt_GetState.write(byteSwap16(csa_leTcpDstPort));
                }
                else {
                    if(csa_meta.length != 0) {
                        // Packet has some TCP payload
                        soTid_DataVal.write(KO);
                    }
                    if (DEBUG_LEVEL & TRACE_CSA) {
                        printWarn(myName, "RECEIVED BAD CHECKSUM (0x%4.4X - Delta= 0x%4.4X).\n",
                                    csa_leTcpCSum.to_uint(),
                                    byteSwap16(~csa_tcp_sums[0](15, 0) & 0xFFFF).to_uint());
                        printSockPair(myName, csa_leSessTuple);
                        //printInfo(myName, "SocketPair={{0x%8.8X, 0x%4.4X},{0x%8.8X, 0x%4.4X}.\n",
                        //        csa_sessTuple.src.addr.to_uint(), csa_sessTuple.src.port.to_uint(),
                        //        csa_sessTuple.dst.addr.to_uint(), csa_sessTuple.dst.port.to_uint());
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
 *****************************************************************************/
void pTcpInvalidDropper(
        stream<AxiWord>     &siCsa_Data,
        stream<ValBit>      &siCsa_DataVal,
        stream<AxiWord>     &soTsd_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tid");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmState {GET_VALID=0, FWD, DROP} tid_fsmState=GET_VALID;
    #pragma HLS RESET                    variable=tid_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     currWord;
    ValBit      validBit;

    switch (tid_fsmState) {

    case GET_VALID:
        if (!siCsa_DataVal.empty()) {
            siCsa_DataVal.read(validBit);
            if (validBit == OK) {
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
            if (DEBUG_LEVEL & TRACE_TID)
                printAxiWord(myName, currWord);
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
 *****************************************************************************/
void pMetaDataHandler(
        stream<RXeMeta>             &siCsa_Meta,
        stream<LE_SocketPair>       &siCsa_SockPair,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<StsBit>              &siPRt_PortSts,
        stream<sessionLookupQuery>  &soSLc_SessLkpReq,
        stream<extendedEvent>       &soEVe_Event,
        stream<CmdBool>             &soTsd_DropCmd,
        stream<RXeFsmMeta>          &soFsm_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Mdh");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmState {META=0, LOOKUP} mdh_fsmState;
    #pragma HLS RESET            variable=mdh_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static RXeMeta              mdh_meta;
    static sessionLookupReply   mdh_sessLookupReply;
    static Ip4Address           mdh_ip4SrcAddr;
    static TcpPort              mdh_tcpSrcPort;
    static TcpPort              mdh_tcpDstPort;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    LE_SocketPair               tuple;
    StsBit                      dstPortStatus;

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
                mdh_tcpSrcPort = byteSwap16(tuple.src.port);
                mdh_tcpDstPort = byteSwap16(tuple.dst.port);

                if (dstPortStatus == STS_CLOSED) {
                    // The destination port is closed
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printWarn(myName, "Port 0x%4.4X (%d) is not open.\n",
                                  mdh_tcpDstPort.to_uint(), mdh_tcpDstPort.to_uint());
                    }
                    if (!mdh_meta.rst) {
                        // Reply with RST+ACK and send necessary socket-pair through event
                        LE_SocketPair  switchedTuple;
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
                soFsm_Meta.write(RXeFsmMeta(mdh_sessLookupReply.sessionID,
                                            mdh_ip4SrcAddr,  mdh_tcpSrcPort,
                                            mdh_tcpDstPort,  mdh_meta));
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
 * @param[in]  siMdh_FsmMeta,     FSM metadata from MetaDataHandler (Mdh).
 * @param[out] soSTt_StateQry,    State query to StateTable (STt).
 * @param[in]  siSTt_StateRep,    State reply from [STt].
 * @param[out] soRSt_RxSarQry,    Query to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep,    Reply from [RSt].
 * @param[out] soTSt_TxSarQry,    Query to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRep,    Reply from [TSt].
 *
 * @param[out] soTIm_ReTxTimerCmd,Command for a retransmit timer to Timers (TIm).
 * @param[out] soTIm_ClearProbeTimer,Clear the probing timer to [TIm].
 * @param[out] soTIm_CloseTimer,  Close session timer to [TIm].
 * @param[out] soTAi_SessOpnSts,  Open status of the session to TxAppInterface (TAi).
 * @param[out] soEVe_Event,       Event to EventEngine (EVe).
 * @param[out] soTsd_DropCmd,     Drop command to TcpSegmentDropper (Tsd).
 * @param[out] soMwr_WrCmd,       Memory write command to MemoryWriter (Mwr).
 * @param[out] soRan_RxNotif,     Rx data notification to RxAppNotifier (Ran).
 *
 * @details
 *  This process implements the typical TCP state and metadata management. It
 *   contains all the logic that updates the metadata and keeps track of the
 *   events related to the reception of segments and their handshaking. This is
 *   the key central part of the Rx engine.
 *
 *****************************************************************************/
void pFiniteStateMachine(
        stream<RXeFsmMeta>                  &siMdh_FsmMeta,
        stream<StateQuery>                  &soSTt_StateQry,
        stream<SessionState>                &siSTt_StateRep,
        stream<RXeRxSarQuery>               &soRSt_RxSarQry,
        stream<RxSarEntry>                  &siRSt_RxSarRep,
        stream<RXeTxSarQuery>               &soTSt_TxSarQry,
        stream<RXeTxSarReply>               &siTSt_TxSarRep,
        stream<RXeReTransTimerCmd>          &soTIm_ReTxTimerCmd,
        stream<ap_uint<16> >                &soTIm_ClearProbeTimer,
        stream<ap_uint<16> >                &soTIm_CloseTimer,
        stream<OpenStatus>                  &soTAi_SessOpnSts, //TODO merge with eventEngine
        stream<event>                       &soEVe_Event,
        stream<CmdBool>                     &soTsd_DropCmd,
        stream<DmCmd>                       &soMwr_WrCmd,
        stream<AppNotif>                    &soRan_RxNotif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Fsm");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { LOAD=0, TRANSITION } \
								 fsm_fsmState=LOAD;
    #pragma HLS RESET   variable=fsm_fsmState
    static bool                  fsm_txSarRequest=false;
    #pragma HLS RESET	variable=fsm_txSarRequest

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static RXeFsmMeta	fsm_Meta;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    ap_uint<4>			control_bits;
    SessionState		tcpState;
    RxSarEntry          rxSar;
    RXeTxSarReply       txSar;

    switch(fsm_fsmState) {

    case LOAD:
        if (!siMdh_FsmMeta.empty()) {
            siMdh_FsmMeta.read(fsm_Meta);
            // Request the current state of the session
            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId));
            // Always request the RxSarTable, even though not required for SYN-ACK
            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId));
            if (fsm_Meta.meta.ack) {
                // Only request the txSar when (ACK+ANYTHING); not for SYN
                soTSt_TxSarQry.write(RXeTxSarQuery(fsm_Meta.sessionId));
                fsm_txSarRequest = true;
            }
            fsm_fsmState = TRANSITION;
        }
        break;

    case TRANSITION:
        // Check if transition to LOAD occurs
        if (!siSTt_StateRep.empty() && !siRSt_RxSarRep.empty() &&
            !(fsm_txSarRequest && siTSt_TxSarRep.empty())) {
            fsm_fsmState = LOAD;
            fsm_txSarRequest = false;
        }

        control_bits[0] = fsm_Meta.meta.ack;
        control_bits[1] = fsm_Meta.meta.syn;
        control_bits[2] = fsm_Meta.meta.fin;
        control_bits[3] = fsm_Meta.meta.rst;

        switch (control_bits) {

        case 1:
            //--------------------------------------
            //-- ACK
            //--------------------------------------
            if (fsm_fsmState == LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                TimerCmd timerCmd = (fsm_Meta.meta.ackNumb == txSar.nextByte) ? STOP_TIMER : LOAD_TIMER;
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));
                if ( (tcpState == ESTABLISHED) || (tcpState == SYN_RECEIVED) ||
                     (tcpState == FIN_WAIT_1)  || (tcpState == CLOSING)      ||
                     (tcpState == LAST_ACK) ) {
                    // Check if new ACK arrived
                    if ( (fsm_Meta.meta.ackNumb == txSar.prevAck) &&
                         (txSar.prevAck != txSar.nextByte) ) {
                        // Not new ACK; increase counter but only if it does not contain data
                        if (fsm_Meta.meta.length == 0) {
                            txSar.count++;
                        }
                    }
                    else {
                        // Notify probeTimer about new ACK
                        soTIm_ClearProbeTimer.write(fsm_Meta.sessionId);

                        // Check for SlowStart & Increase Congestion Window
                        if (txSar.cong_window <= (txSar.slowstart_threshold-MSS)) {
                            txSar.cong_window += MSS;
                        }
                        else if (txSar.cong_window <= 0xF7FF) {
                            txSar.cong_window += 365; //TODO replace by approx. of (MSS x MSS) / cong_window
                        }
                        txSar.count = 0;
                        txSar.fastRetransmitted = false;
                    }

                    // Update TxSarTable (only if count or retransmit)
                    //  [FIXME - 'txSar.count' must be compared to a DEFINE constant]
                    if ( (  (txSar.prevAck <= fsm_Meta.meta.ackNumb) && (fsm_Meta.meta.ackNumb <= txSar.nextByte) ) ||
                         ( ((txSar.prevAck <= fsm_Meta.meta.ackNumb) || (fsm_Meta.meta.ackNumb <= txSar.nextByte) ) &&
                            (txSar.nextByte < txSar.prevAck) ) ) {
                        soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId,
                                                            fsm_Meta.meta.ackNumb,
                                                            fsm_Meta.meta.winSize,
                                                            txSar.cong_window,
                                                            txSar.count,
                                                          ((txSar.count == 3) || txSar.fastRetransmitted))));
                    }

                    // Check if packet contains payload
                    if (fsm_Meta.meta.length != 0) {
                        RxSeqNum rcvNext = fsm_Meta.meta.seqNumb + fsm_Meta.meta.length;
                        // Second part makes sure that 'appd' pointer is not overtaken
#if !(RX_DDR_BYPASS)
                        RxBufPtr free_space = ((rxSar.appd - rxSar.rcvd(15, 0)) - 1);
                        // Check if segment in order and if enough free space is available
                        if ( (fsm_Meta.meta.seqNumb == rxSar.rcvd) &&
                             (free_space > fsm_Meta.meta.length) ) {
#else
                        if ( (fsm_meta.meta.seqNumb == rxSar.rcvd) &&
                             (rxbuffer_max_data_count - rxbuffer_data_count) > 375) { // [FIXME - Why 375?]
#endif
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, rcvNext, QUERY_WR));
                            // Build memory address for this segment in the lower 4GB
                            RxMemPtr memSegAddr;
                            memSegAddr(31, 30) = 0x0;
                            memSegAddr(29, 16) = fsm_Meta.sessionId(13, 0);
                            memSegAddr(15,  0) = fsm_Meta.meta.seqNumb.range(15, 0);
#if !(RX_DDR_BYPASS)
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
#endif
                            // Only notify about new data available
                            soRan_RxNotif.write(AppNotif(fsm_Meta.sessionId,  fsm_Meta.meta.length, fsm_Meta.ip4SrcAddr,
                                                         fsm_Meta.tcpSrcPort, fsm_Meta.tcpDstPort));
                            soTsd_DropCmd.write(KEEP_CMD);
                        }
                        else {
                            soTsd_DropCmd.write(DROP_CMD);
                            if (fsm_Meta.meta.seqNumb != rxSar.rcvd) {
                                printWarn(myName, "The received sequence number (%d) is not the expected one (%d).\n",
                                        fsm_Meta.meta.seqNumb.to_uint(), rxSar.rcvd.to_uint());
                            }
                            else if (free_space < fsm_Meta.meta.length) {
                                printWarn(myName, "There is not enough space left to store the received segment in the Rx ring buffer.\n");
                            }
                        }
                    }

# if FAST_RETRANSMIT
                    if (txSar.count == 3 && !txSar.fastRetransmitted) {  // [FIXME - Use a constant here]
                        soEVe_Event.write(event(RT, fsm_Meta.sessionId));
                    }
                    else if (fsm_meta.meta.length != 0) {
#else
                    // OBSOLETE-20190705 if (txSar.count == 3 && !txSar.fastRetransmitted) {
                    if (fsm_Meta.meta.length != 0) {
#endif
                        soEVe_Event.write(event(ACK, fsm_Meta.sessionId));
                    }

                    // Reset Retransmit Timer
                    if (fsm_Meta.meta.ackNumb == txSar.nextByte) {
                        switch (tcpState) {
                        case SYN_RECEIVED:  //TODO MAYBE REARRANGE
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, ESTABLISHED, QUERY_WR));
                            break;
                        case CLOSING:
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, TIME_WAIT, QUERY_WR));
                            soTIm_CloseTimer.write(fsm_Meta.sessionId); // [TDOD - rename]
                            break;
                        case LAST_ACK:
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                            break;
                        default:
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                            break;
                        }
                    }
                    else { // we have to release the lock
                        // Reset rtTimer
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR)); // or ESTABLISHED
                    }
                } // End of : if ( (tcpState == ...

                // TODO if timewait just send ACK, can it be time wait??
                else { // state == (CLOSED || SYN_SENT || CLOSE_WAIT || FIN_WAIT_2 || TIME_WAIT)
                    // SENT RST, RFC 793: fig.11
                    soEVe_Event.write(rstEvent(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+fsm_Meta.meta.length)); // noACK ?
                    // if data is in the pipe it needs to be droppped
                    if (fsm_Meta.meta.length != 0) {
                        soTsd_DropCmd.write(DROP_CMD);
                    }
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                }

            } // End of: if (fsmState == LOAD) {
            break;

        case 2:
            //--------------------------------------
            //-- SYN
            //--------------------------------------
            if (DEBUG_LEVEL & TRACE_FSM) printInfo(myName, "Segment is SYN.\n");
            if (fsm_fsmState == LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                if (tcpState == CLOSED || tcpState == SYN_SENT) {
                    // Actually this is LISTEN || SYN_SENT
                    // Initialize rxSar, SEQ + phantom byte, last '1' for makes sure appd is initialized
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb + 1, QUERY_WR, QUERY_INIT));
                    // Initialize receive window ([TODO - maybe include count check])
                    soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId, 0, fsm_Meta.meta.winSize, txSar.cong_window, 0, false)));
                    // Set SYN_ACK event
                    soEVe_Event.write(event(SYN_ACK, fsm_Meta.sessionId));
                    if (DEBUG_LEVEL & TRACE_FSM) printInfo(myName, "Set event SYN_ACK for sessionID %d.\n", fsm_Meta.sessionId.to_uint());
                    // Change State to SYN_RECEIVED
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, SYN_RECEIVED, QUERY_WR));
                }
                else if (tcpState == SYN_RECEIVED) { // && mdh_meta.seqNumb+1 == rxSar.recvd) // Maybe Check for seq
                    // If it is the same SYN, we resent SYN-ACK, almost like quick RT, we could also wait for RT timer
                    if (fsm_Meta.meta.seqNumb+1 == rxSar.rcvd) {
                        // Retransmit SYN_ACK
                        ap_uint<3> rtCount = 1;
                        soEVe_Event.write(event(SYN_ACK, fsm_Meta.sessionId, rtCount));
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                    }
                    else { // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                        soEVe_Event.write(rstEvent(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+1)); //length == 0
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                    }
                }
                else { // Any synchronized state
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(event(ACK_NODELAY, fsm_Meta.sessionId));
                    // TODo send RST, has no ACK??
                    // Respond with RST, no ACK, seq ==
                    //eventEngine.write(rstEvent(mdh_meta.seqNumb, mh_meta.length, true));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                }
            }
            break;

        case 3:
            //--------------------------------------
            //-- SYN_ACK
            //--------------------------------------
            if (fsm_fsmState == LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                TimerCmd timerCmd = (fsm_Meta.meta.ackNumb == txSar.nextByte) ? STOP_TIMER : LOAD_TIMER;
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));

                if ( (tcpState == SYN_SENT) && (fsm_Meta.meta.ackNumb == txSar.nextByte) ) { // && !mh_lup.created)
                    // Initialize rx_sar, SEQ + phantom byte, last '1' for appd init
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId,
                                                       fsm_Meta.meta.seqNumb + 1, QUERY_WR, QUERY_INIT));
                    soTSt_TxSarQry.write(RXeTxSarQuery(fsm_Meta.sessionId,
                                                       fsm_Meta.meta.ackNumb,
                                                       fsm_Meta.meta.winSize,
                                                       txSar.cong_window, 0, false)); // [TODO - maybe include count check]
                    // Set ACK event
                    soEVe_Event.write(event(ACK_NODELAY, fsm_Meta.sessionId));

                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, ESTABLISHED, QUERY_WR));
                    soTAi_SessOpnSts.write(OpenStatus(fsm_Meta.sessionId, SESS_IS_OPENED));
                }
                else if (tcpState == SYN_SENT) { //TODO correct answer?
                    // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                    soEVe_Event.write(rstEvent(fsm_Meta.sessionId,
                                               fsm_Meta.meta.seqNumb+fsm_Meta.meta.length+1));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                }
                else {
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(event(ACK_NODELAY, fsm_Meta.sessionId));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                }
            }
            break;

        case 5:
            //--------------------------------------
            //-- FIN (_ACK)
            //--------------------------------------
            if (fsm_fsmState == LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                TimerCmd timerCmd = (fsm_Meta.meta.ackNumb == txSar.nextByte) ? STOP_TIMER : LOAD_TIMER;
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));
                // Check state and if FIN in order, Current out of order FINs are not accepted
                if ( (tcpState == ESTABLISHED || tcpState == FIN_WAIT_1 ||
                      tcpState == FIN_WAIT_2) && (rxSar.rcvd == fsm_Meta.meta.seqNumb) ) {
                    soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId,
                                          fsm_Meta.meta.ackNumb, fsm_Meta.meta.winSize,
                                          txSar.cong_window, txSar.count,
                                          ~QUERY_FAST_RETRANSMIT))); //TODO include count check

                    // +1 for phantom byte, there might be data too
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId,
                                         fsm_Meta.meta.seqNumb+fsm_Meta.meta.length+1,
                                         QUERY_WR)); // diff to ACK

                    // Clear the probe timer
                    soTIm_ClearProbeTimer.write(fsm_Meta.sessionId);

                    // Check if there is payload
                    if (fsm_Meta.meta.length != 0) {
                        RxMemPtr    memSegAddr;
                        memSegAddr(31, 30) = 0x0;
                        memSegAddr(29, 16) = fsm_Meta.sessionId(13, 0);
                        memSegAddr(15,  0) = fsm_Meta.meta.seqNumb(15, 0);
#if !(RX_DDR_BYPASS)
                        soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
#endif
                        // Tell Application new data is available and connection got closed
                        soRan_RxNotif.write(AppNotif(fsm_Meta.sessionId,  fsm_Meta.meta.length, fsm_Meta.ip4SrcAddr,
                                                     fsm_Meta.tcpSrcPort, fsm_Meta.tcpDstPort,  true)); //CLOSE
                        soTsd_DropCmd.write(KEEP_CMD);
                    }
                    else if (tcpState == ESTABLISHED) {
                        // Tell Application connection got closed
                        soRan_RxNotif.write(AppNotif(fsm_Meta.sessionId,  0,                   fsm_Meta.ip4SrcAddr,
                                                     fsm_Meta.tcpSrcPort, fsm_Meta.tcpDstPort,  true)); //CLOSE
                    }

                    // Update state
                    if (tcpState == ESTABLISHED) {
                        soEVe_Event.write(event(FIN, fsm_Meta.sessionId));
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, LAST_ACK, QUERY_WR));
                    }
                    else { //FIN_WAIT_1 || FIN_WAIT_2
                        if (fsm_Meta.meta.ackNumb == txSar.nextByte) {
                            // Check if final FIN is ACK'd -> LAST_ACK
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, TIME_WAIT, QUERY_WR));
                            soTIm_CloseTimer.write(fsm_Meta.sessionId);
                        }
                        else {
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSING, QUERY_WR));
                        }
                        soEVe_Event.write(event(ACK, fsm_Meta.sessionId));
                    }
                }
                else { // NOT (ESTABLISHED || FIN_WAIT_1 || FIN_WAIT_2)
                    soEVe_Event.write(event(ACK, fsm_Meta.sessionId));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                    // If there is payload we need to drop it
                    if (fsm_Meta.meta.length != 0) {
                        soTsd_DropCmd.write(DROP_CMD);
                    }
                }
            }
            break;

        default: //TODO MAYBE load everything all the time
            // stateTable is locked, make sure it is released in at the end
            // If there is an ACK we read txSar
            // We always read rxSar
            if (fsm_fsmState == LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar); //TODO not sure nb works
                siTSt_TxSarRep.read_nb(txSar);
            }
            if (fsm_fsmState == LOAD) {
                // Handle if RST
                if (fsm_Meta.meta.rst) {
                    if (tcpState == SYN_SENT) {
                        // [TODO this would be a RST,ACK i think]
                        // Check if matching SYN
                        if (fsm_Meta.meta.ackNumb == txSar.nextByte) {
                            // Tell application, could not open connection
                            soTAi_SessOpnSts.write(OpenStatus(fsm_Meta.sessionId, FAILED_TO_OPEN_SESS));
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                            soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, STOP_TIMER));
                        }
                        else {
                            // Ignore since not matching
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                        }
                    }
                    else {
                        // Check if in window
                        if (fsm_Meta.meta.seqNumb == rxSar.rcvd) {
                            // Tell application, RST occurred, abort
                            soRan_RxNotif.write(AppNotif(fsm_Meta.sessionId, 0, fsm_Meta.ip4SrcAddr,
                                                         fsm_Meta.tcpSrcPort,   fsm_Meta.tcpDstPort, true)); // RESET-CLOSED
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR)); //TODO maybe some TIME_WAIT state
                            soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, STOP_TIMER));
                        }
                        else {
                            // Ignore since not matching window
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                        }
                    }
                }
                else { // Handle non RST bogus packages
                    //TODO maybe sent RST ourselves, or simply ignore
                    // For now ignore, sent ACK??
                    //eventsOut.write(rstEvent(mh_meta.seqNumb, 0, true));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                } // End of: if (fsm_meta.meta.rst)
            } // if fsm_stat
            break;

        } // End of: switch control_bits

        break;

    } // End of: switch state

} // End of: pFiniteStateMachine(

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
 *****************************************************************************/
void pTcpSegmentDropper(
        stream<AxiWord>     &siTid_Data,
        stream<CmdBool>     &siMdh_DropCmd,
        stream<CmdBool>     &siFsm_DropCmd,
        stream<AxiWord>     &soMwr_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tsd");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmState {FSM_RD_DROP_CMD1=0, FSM_RD_DROP_CMD2, FSM_FWD, FSM_DROP} \
	                           tsd_fsmState=FSM_RD_DROP_CMD1;
    #pragma HLS RESET variable=tsd_fsmState


    switch (tsd_fsmState) {
    case FSM_RD_DROP_CMD1:
        if (!siMdh_DropCmd.empty()) {
            CmdBit dropCmd = siMdh_DropCmd.read();
            if (dropCmd) {
                tsd_fsmState = FSM_DROP;
                printWarn(myName, "[Mdh] is requesting to drop this packet.\n");
            }
            else {
                tsd_fsmState = FSM_RD_DROP_CMD2;
            }
        }
        break;
    case FSM_RD_DROP_CMD2:
        if (!siFsm_DropCmd.empty()) {
            CmdBit dropCmd = siFsm_DropCmd.read();
            if (dropCmd) {
                tsd_fsmState = FSM_DROP;
                printWarn(myName, "[Fsm] is requesting to drop this packet.\n");
            }
            else {
                tsd_fsmState = FSM_FWD;
            }
        }
        break;
    case FSM_FWD:
        if(!siTid_Data.empty() && !soMwr_Data.full()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast) {
                tsd_fsmState = FSM_RD_DROP_CMD1;
            }
            soMwr_Data.write(currWord);
        }
        break;
    case FSM_DROP:
        if(!siTid_Data.empty()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast) {
                tsd_fsmState = FSM_RD_DROP_CMD1;
            }
        }
        break;

    } // End of: switch
}


/*****************************************************************************
 * @brief Rx Application Notifier (Ran)
 *
 * @param[in]  siMEM_WrSts,    The memory write status from memory data mover [MEM].
 * @param[in]  siFsm_Notif,    Rx data notification from [FiniteStateMachine].
 * @param[out] soRAi_RxNotif,  Rx data notification to RxApplicationInterface (RAi).
 * @param[in]  siMwr_SplitSeg, This TCP data segment is split in two Rx memory buffers.
 *
 * @details
 *  Delays the notifications to the application until the data is actually
 *   written into the physical DRAM memory.
 *
 * @todo
 *  Handle unsuccessful write to memory.
 *
 *****************************************************************************/
void pRxAppNotifier(
        stream<DmSts>         &siMEM_WrSts,
        stream<AppNotif>      &siFsm_Notif,
        stream<AppNotif>      &soRAi_RxNotif,
        stream<StsBit>        &siMwr_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Ran");

    //-- LOCAL STREAMS
    static stream<AppNotif>        ssRxNotifFifo("ssRxNotifFifo");
    #pragma HLS STREAM    variable=ssRxNotifFifo depth=32 // WARNING: Depends on the memory delay !!!
    #pragma HLS DATA_PACK variable=ssRxNotifFifo

    //-- STATIC CONTROL VARIABLES (with RESET)
    static FlagBit             ran_doubleAccessFlag=FLAG_OFF;
    #pragma HLS RESET variable=ran_doubleAccessFlag
    //OBSOLETE_20191118 static ap_uint<5>          ran_fifoCount = 0;
    //OBSOLETE_20191118 #pragma HLS RESET variable=ran_fifoCount

    //-- STATIC DATAFLOW VARIABLES
    static DmSts        ran_dmStatus1;
    static DmSts		ran_dmStatus2;
    static AppNotif     ran_appNotification;

    if (ran_doubleAccessFlag == FLAG_ON) {
        // The segment was splitted and notification will only go out now
        if(!siMEM_WrSts.empty()) {
            siMEM_WrSts.read(ran_dmStatus2);
            //OBSOLETE_20191118 ran_fifoCount--;
            if (ran_dmStatus1.okay && ran_dmStatus2.okay) {
                soRAi_RxNotif.write(ran_appNotification);
                if (DEBUG_LEVEL & TRACE_RAN) {
                    printInfo(myName, "Sending APP notification to [RAi]. This was a double access.\n");
                }
            }
            else {
                if (DEBUG_LEVEL & TRACE_RAN) {
                    printError(myName, "The previous splitted mem-write command failed (OKAY=0).\n");
                }
            }
            ran_doubleAccessFlag = FLAG_OFF;
        }
    }
    else {
        //-- We don't know yet about a possible double memory access
        if(!siMEM_WrSts.empty() && !siMwr_SplitSeg.empty() && !ssRxNotifFifo.empty()) {
            siMEM_WrSts.read(ran_dmStatus1);
            siMwr_SplitSeg.read(ran_doubleAccessFlag);
            ssRxNotifFifo.read(ran_appNotification);
            if (ran_doubleAccessFlag == FLAG_OFF) {
                // This segment consists of a single memory access
            	//OBSOLETE_20191118 ran_fifoCount--;
                if (ran_dmStatus1.okay) {
                    // Output the notification now
                    soRAi_RxNotif.write(ran_appNotification);
                    if (DEBUG_LEVEL & TRACE_RAN) {
                        printInfo(myName, "Sending APP notification to [RAi].\n");
                    }
                }
                else {
                    if (DEBUG_LEVEL & TRACE_RAN) {
                        printError(myName, "The previous memory write command failed (OKAY=0).\n");
                    }
                }
            }
            else {
                if (DEBUG_LEVEL & TRACE_RAN) {
                    printInfo(myName, "The memory access was broken down in two parts.\n");
                }
            }
        }
        //OBSOLETE_20191118 else if (!siFsm_Notif.empty() && (ran_fifoCount < 31)) {
        else if (!siFsm_Notif.empty() && !ssRxNotifFifo.full()) {
            siFsm_Notif.read(ran_appNotification);
            if (ran_appNotification.tcpSegLen != 0) {
                ssRxNotifFifo.write(ran_appNotification);
                //OBSOLETE_20191118 ran_fifoCount++;
            }
            else {
                soRAi_RxNotif.write(ran_appNotification);
            }
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
 *****************************************************************************/
void pEventMultiplexer(
        stream<extendedEvent>    &siMdh_Event,
        stream<event>            &siFsm_Event,
        stream<extendedEvent>    &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Evm");

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
 *****************************************************************************/
void pMemWriter(
        stream<AxiWord>     &siTsd_MemWrData,
        stream<DmCmd>       &siFsm_MemWrCmd,
        stream<DmCmd>       &soMEM_WrCmd,
        stream<AxiWord>     &soMEM_WrData,
        stream<StsBit>      &soRan_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Mwr");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmState {IDLE=0, WRFIRST, EVALSECOND, WRSECONDSTR, ALIGNED, RESIDUE} \
								mwr_fsmState=IDLE;
    #pragma HLS RESET  variable=mwr_fsmState
    static ap_uint<3>			mwr_residueLen=0;
    #pragma HLS RESET  variable=mwr_residueLen
    static bool                 mwr_accessBreakdown=false;
    #pragma HLS RESET  variable=mwr_accessBreakdown

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static DmCmd       mwr_memWrCmd;
    static RxBufPtr    mwr_curAccLen;  //OBSOLETE_20191118 rxEngBreakTemp;
    static uint8_t     mwr_bufferLen;  //OBSOLETE_20191118 lengthBuffer;
    static AxiWord     mwr_pushWord;   //OBSOLETE_20191118 = AxiWord(0, 0xFF, 0);

    switch (mwr_fsmState) {

    case IDLE:
        if (!siFsm_MemWrCmd.empty() && !soMEM_WrCmd.full() && !soRan_SplitSeg.full()) {
            mwr_memWrCmd = siFsm_MemWrCmd.read();
            DmCmd memWrCmd = mwr_memWrCmd;
            if ((mwr_memWrCmd.saddr.range(15, 0) + mwr_memWrCmd.bbt) > RXMEMBUF) {
                // Break this segment in two memory accesses because TCP Rx memory buffer wraps around
                mwr_curAccLen = RXMEMBUF - mwr_memWrCmd.saddr;
                mwr_memWrCmd.bbt -= mwr_curAccLen;
                memWrCmd = DmCmd(mwr_memWrCmd.saddr, mwr_curAccLen);
                mwr_accessBreakdown = true;
                if (DEBUG_LEVEL & TRACE_MWR)
                    printInfo(myName, "TCP memory buffer wraps around: This segment will be broken in two memory buffers.\n");
            }
            else {
                mwr_curAccLen = mwr_memWrCmd.bbt;
            }
            soMEM_WrCmd.write(memWrCmd);
            soRan_SplitSeg.write(mwr_accessBreakdown);
            if (DEBUG_LEVEL & TRACE_MWR)
                printDmCmd(myName, memWrCmd);
            mwr_fsmState = WRFIRST;
        }
        break;

    case WRFIRST:
        if (!siTsd_MemWrData.empty() && !soMEM_WrData.full()) {
            siTsd_MemWrData.read(mwr_pushWord);
            AxiWord outputWord = mwr_pushWord;
            ap_uint<4> byteCount = keepToLen(mwr_pushWord.tkeep);
            if (mwr_curAccLen > 8) {
                mwr_curAccLen -= 8;
            }
            else {
                if (mwr_accessBreakdown == true) {
                    // Changes are to go in here
                    // If the word is not perfectly aligned then there is some magic to be worked.
                    if (mwr_memWrCmd.saddr.range(15, 0) % 8 != 0) {
                        outputWord.tkeep = lenToKeep(mwr_curAccLen);
                    }
                    outputWord.tlast = 1;
                    mwr_residueLen = byteCount - mwr_curAccLen;
                    // Buffer the number of consumed bytes
                    mwr_bufferLen = mwr_curAccLen;
                    mwr_fsmState = EVALSECOND;
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
            if (mwr_memWrCmd.saddr.range(15, 0) % 8 == 0)
                mwr_fsmState = ALIGNED;
            //else if (rxMemWriterCmd.bbt + rxEngAccessResidue > 8 || rxEngAccessResidue > 0)
            else if (mwr_memWrCmd.bbt - mwr_residueLen > 0)
                mwr_fsmState = WRSECONDSTR;
            else
                mwr_fsmState = RESIDUE;
            mwr_memWrCmd.saddr.range(15, 0) = 0;
            mwr_curAccLen = mwr_memWrCmd.bbt;
            DmCmd memWrCmd = DmCmd(mwr_memWrCmd.saddr, mwr_curAccLen);
            soMEM_WrCmd.write(memWrCmd);
            if (DEBUG_LEVEL & TRACE_MWR) printDmCmd(myName, memWrCmd);
            mwr_accessBreakdown = false;
        }
        break;

    case ALIGNED:   // This is the non-realignment state
        if (!siTsd_MemWrData.empty() & !soMEM_WrData.full()) {
            siTsd_MemWrData.read(mwr_pushWord);
            soMEM_WrData.write(mwr_pushWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, mwr_pushWord);
            if (mwr_pushWord.tlast == 1)
                mwr_fsmState = IDLE;
        }
        break;

    case WRSECONDSTR: // We go into this state when we need to realign things
        if (!siTsd_MemWrData.empty() && !soMEM_WrData.full()) {
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range(((8-mwr_bufferLen)*8) - 1, 0) = mwr_pushWord.tdata.range(63, mwr_bufferLen*8);
            mwr_pushWord = siTsd_MemWrData.read();
            outputWord.tdata.range(63, (8-mwr_bufferLen)*8) = mwr_pushWord.tdata.range((mwr_bufferLen * 8), 0 );

            if (mwr_pushWord.tlast == 1) {
                if (mwr_curAccLen - mwr_residueLen > mwr_bufferLen) {
                    // In this case there's residue to be handled
                    mwr_curAccLen -= 8;
                    mwr_fsmState = RESIDUE;
                }
                else {
                    outputWord.tkeep = returnKeep(mwr_curAccLen);
                    outputWord.tlast = 1;
                    mwr_fsmState = IDLE;
                }
            }
            else {
                mwr_curAccLen -= 8;
            }
            soMEM_WrData.write(outputWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, outputWord);
        }
        break;

    case RESIDUE:
        if (!soMEM_WrData.full()) {
            AxiWord outputWord = AxiWord(0, returnKeep(mwr_curAccLen), 1);
            outputWord.tdata.range(((8-mwr_bufferLen)*8) - 1, 0) = mwr_pushWord.tdata.range(63, mwr_bufferLen*8);
            soMEM_WrData.write(outputWord);
            if (DEBUG_LEVEL & TRACE_MWR) printAxiWord(myName, outputWord);
            mwr_fsmState = IDLE;
        }
        break;

    }
}

/*****************************************************************************
 * @brief The rx_engine (RXe) processes the data packets received from IPRX.
 *
 * @param[in]  siIPRX_Data,         IP4 data stream form IPRX.
 * @param[out] soSLc_SessLookupReq, Session lookup request to SessionLookupController (SLc).
 * @param[in]  siSLc_SessLookupRep, Session lookup reply from [SLc].
 * @param[out] soSTt_StateQry,      State query to StateTable (STt).
 * @param[in]  siSTt_StateRep,      State reply from [STt].
 * @param[out] soPRt_PortStateReq,  Port state request to PortTable (PRt).
 * @param[in]  siPRt_PortStateRep,  Port state reply from [PRt].
 * @param[out] soRSt_RxSarQry,      Query to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep       Reply from [RSt].
 * @param[out] soTSt_TxSarQry,      Query to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRep       Reply from [TSt].
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
 ******************************************************************************/
void rx_engine(
		// IP Rx Interface
        stream<Ip4overMac>              &siIPRX_Pkt,
		//-- Session Lookup Controller Interface
		stream<sessionLookupQuery>      &soSLc_SessLookupReq,
        stream<sessionLookupReply>      &siSLc_SessLookupRep,
        //-- State Table Interface
		stream<StateQuery>              &soSTt_StateQry,
        stream<SessionState>            &siSTt_StateRep,
		//-- Port Table Interface
        stream<TcpPort>                 &soPRt_PortStateReq,
        stream<RepBit>                  &siPRt_PortStateRep,
		//-- Rx SAR Table Interface
        stream<RXeRxSarQuery>           &soRSt_RxSarQry,
        stream<RxSarEntry>              &siRSt_RxSarRep,
		//-- Tx SAR Table Interface
        stream<RXeTxSarQuery>           &soTSt_TxSarQry,
        stream<RXeTxSarReply>           &siTSt_TxSarRep,
		//-- Timers Interface
        stream<RXeReTransTimerCmd>      &soTIm_ReTxTimerCmd,
        stream<ap_uint<16> >            &soTIm_ClearProbeTimer,
        stream<ap_uint<16> >            &soTIm_CloseTimer,
		//-- Event Engine Interface
        stream<extendedEvent>           &soEVe_SetEvent,
		//-- Tx Application Interface
        stream<OpenStatus>              &soTAi_SessOpnSts,
		//-- Rx Application Interface
        stream<AppNotif>                &soRAi_RxNotif,
		//-- MEM / Rx Write Path Interface
        stream<DmCmd>                   &soMEM_WrCmd,
        stream<AxiWord>                 &soMEM_WrData,
        stream<DmSts>                   &siMEM_WrSts)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
	#pragma HLS DATAFLOW
	#pragma HLS INTERFACE ap_ctrl_none port=return

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tcp Length Extract (Tle) ---------------------------------------------
    static stream<Ip4overMac>       ssTleToIph_TcpSeg       ("ssTleToIph_TcpSeg");
    #pragma HLS stream     variable=ssTleToIph_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=ssTleToIph_TcpSeg

    static stream<TcpSegLen>        ssTleToIph_TcpSegLen    ("ssTleToIph_TcpSegLen");
    #pragma HLS stream     variable=ssTleToIph_TcpSegLen    depth=2

    //-- Insert Pseudo Header (Iph) -------------------------------------------
    static stream<Ip4overMac>       ssIphToCsa_TcpSeg       ("ssIphToCsa_TcpSeg");
    #pragma    HLS stream  variable=ssIphToCsa_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=ssIphToCsa_TcpSeg

    //-- CheckSum Accumulator (Csa) -------------------------------------------
    static stream<AxiWord>          ssCsaToTid_Data         ("ssCsaToTid_Data");
    #pragma HLS stream     variable=ssCsaToTid_Data         depth=256 //critical, tcp checksum computation
    #pragma HLS DATA_PACK  variable=ssCsaToTid_Data

    static stream<ValBit>           ssCsaToTid_DataValid	("ssCsaToTid_DataValid");
    #pragma HLS stream     variable=ssCsaToTid_DataValid	depth=2

    static stream<RXeMeta>          ssCsaToMdh_Meta         ("ssCsaToMdh_Meta");
    #pragma HLS stream     variable=ssCsaToMdh_Meta         depth=2
    #pragma HLS DATA_PACK  variable=ssCsaToMdh_Meta

    static stream<LE_SocketPair>    ssCsaToMdh_SockPair     ("ssCsaToMdh_SockPair");
    #pragma HLS stream     variable=ssCsaToMdh_SockPair     depth=2
    #pragma HLS DATA_PACK  variable=ssCsaToMdh_SockPair

    static stream<AxiWord>          ssTidToTsd_Data         ("ssTidToTsd_Data");
    #pragma HLS stream     variable=ssTidToTsd_Data         depth=8
    #pragma HLS DATA_PACK  variable=ssTidToTsd_Data

    //-- Tcp Invalid dropper (Tid) --------------------------------------------
    static stream<AxiWord>          ssTsdToMwr_Data         ("ssTsdToMwr_Data");
    #pragma HLS stream     variable=ssTsdToMwr_Data         depth=16
    #pragma HLS DATA_PACK  variable=ssTsdToMwr_Data

    //-- MetaData Handler (Mdh) -----------------------------------------------
    static stream<extendedEvent>    ssMdhToEvm_Event        ("ssMdhToEvm_Event");
    #pragma HLS stream     variable=ssMdhToEvm_Event        depth=2
    #pragma HLS DATA_PACK  variable=ssMdhToEvm_Event

    static stream<CmdBool>          ssMdhToTsd_DropCmd      ("ssMdhToTsd_DropCmd");
    #pragma HLS stream     variable=ssMdhToTsd_DropCmd      depth=2

    static stream<RXeFsmMeta>       ssMdhToFsm_Meta			("ssMdhToFsm_Meta");
    #pragma HLS stream     variable=ssMdhToFsm_Meta			depth=2
    #pragma HLS DATA_PACK  variable=ssMdhToFsm_Meta

    //-- Finite State Machine (Fsm) -------------------------------------------
    static stream<CmdBool>          ssFsmToTsd_DropCmd      ("ssFsmToTsd_DropCmd");
    #pragma HLS stream     variable=ssFsmToTsd_DropCmd      depth=2

    static stream<AppNotif>         ssFsmToRan_Notif        ("ssFsmToRan_Notif");
    #pragma HLS stream     variable=ssFsmToRan_Notif        depth=8  // This depends on the memory delay
    #pragma HLS DATA_PACK  variable=ssFsmToRan_Notif

    static stream<event>            ssFsmToEvm_Event        ("ssFsmToEvm_Event");
    #pragma HLS stream     variable=ssFsmToEvm_Event        depth=2
    #pragma HLS DATA_PACK  variable=ssFsmToEvm_Event

    static stream<DmCmd>            ssFsmToMwr_WrCmd        ("ssFsmToMwr_WrCmd");
    #pragma HLS stream     variable=ssFsmToMwr_WrCmd        depth=8
    #pragma HLS DATA_PACK  variable=ssFsmToMwr_WrCmd

    //-- Memory Writer (Mwr) --------------------------------------------------
    static stream<ap_uint<1> >      ssMwrToRan_SplitSeg     ("ssMwrToRan_SplitSeg");
    #pragma HLS stream     variable=ssMwrToRan_SplitSeg     depth=8

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTcpLengthExtract(
            siIPRX_Pkt,
            ssTleToIph_TcpSeg,
            ssTleToIph_TcpSegLen);

    pInsertPseudoHeader(
            ssTleToIph_TcpSeg,
            ssTleToIph_TcpSegLen,
            ssIphToCsa_TcpSeg);

    pCheckSumAccumulator(
            ssIphToCsa_TcpSeg,
            ssCsaToTid_Data,
            ssCsaToTid_DataValid,
            ssCsaToMdh_Meta,
            ssCsaToMdh_SockPair,
            soPRt_PortStateReq);

    pTcpInvalidDropper(
            ssCsaToTid_Data,
            ssCsaToTid_DataValid,
            ssTidToTsd_Data);

    pMetaDataHandler(
            ssCsaToMdh_Meta,
            ssCsaToMdh_SockPair,
            siSLc_SessLookupRep,
            siPRt_PortStateRep,
            soSLc_SessLookupReq,
            ssMdhToEvm_Event,
            ssMdhToTsd_DropCmd,
            ssMdhToFsm_Meta);

    pFiniteStateMachine(
            ssMdhToFsm_Meta,
            soSTt_StateQry,
            siSTt_StateRep,
            soRSt_RxSarQry,
            siRSt_RxSarRep,
            soTSt_TxSarQry,
            siTSt_TxSarRep,
            soTIm_ReTxTimerCmd,
            soTIm_ClearProbeTimer,
            soTIm_CloseTimer,
            soTAi_SessOpnSts,
            ssFsmToEvm_Event,
            ssFsmToTsd_DropCmd,
            ssFsmToMwr_WrCmd,
            ssFsmToRan_Notif);

    pTcpSegmentDropper(
            ssTidToTsd_Data,
            ssMdhToTsd_DropCmd,
            ssFsmToTsd_DropCmd,
            ssTsdToMwr_Data);

    pMemWriter(
            ssTsdToMwr_Data,
            ssFsmToMwr_WrCmd,
            soMEM_WrCmd,
            soMEM_WrData,
            ssMwrToRan_SplitSeg);

    pRxAppNotifier(
            siMEM_WrSts,
            ssFsmToRan_Notif,
            soRAi_RxNotif,
            ssMwrToRan_SplitSeg);

    pEventMultiplexer(
            ssMdhToEvm_Event,
            ssFsmToEvm_Event,
            soEVe_SetEvent);

}
