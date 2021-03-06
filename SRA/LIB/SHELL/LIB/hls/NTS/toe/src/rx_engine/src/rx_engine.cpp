/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/************************************************
Copyright (c) 2015, Xilinx, Inc.

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

/*******************************************************************************
 * @file       : rx_engine.hpp
 * @brief      : Rx Engine (RXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "rx_engine.hpp"

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_CSA | TRACE_IPH)
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

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief TCP Length Extraction (Tle)
 *
 * @param[in]  siIPRX_Data     IP4 packet stream from IpRxHandler (IPRX).
 * @param[out] soIph_Data      A custom-made pseudo packet to InsertPseudoHeader (Iph).
 * @param[out] soIph_TcpSegLen The length of the incoming TCP segment to [Iph].
 *
 * @details
 *   This process receives the incoming IPv4 data stream from IpRxHandler (IPRX).
 *   It computes the TCP length field from the IP header, removes that IP header
 *   but keeps the IP source and destination addresses in front of the TCP
 *   segment in order for that output stream to be used by the next process to
 *   populate the rest of the TCP pseudo header.
 *   The length of the IPv4 data (.i.e. the TCP segment length) is computed from
 *   the IPv4 total length and the IPv4 header length.
 *
 *   The data received from [IPRX] are logically divided into lane #0 (7:0) to
 *   lane #7 (63:56). The format of an incoming IPv4 packet is defined in
 *   (@see AxisIp4.hpp) and is as follows:
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
 *    The format of the outgoing segment is custom-made as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
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
 *******************************************************************************/
void pTcpLengthExtractor(
        stream<AxisIp4>      &siIPRX_Data,
        stream<AxisRaw>      &soIph_Data,
        stream<TcpSegLen>    &soIph_TcpSegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tle");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<4>           tle_chunkCount=0;
    #pragma HLS RESET  variable=tle_chunkCount
    static bool                 tle_residue=false;
    #pragma HLS RESET  variable=tle_residue

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static bool         tle_shift;   // Keeps track of the chunk alignment
    static Ip4HdrLen    tle_ip4HdrLen;
    static Ip4TotalLen  tle_ip4TotLen;
    static Ip4DatLen    tle_ipDataLen;
    static AxisIp4      tle_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisRaw             sendChunk;

    //-- MAIN PROCESS
    if (!siIPRX_Data.empty() and !soIph_Data.full() and !tle_residue) {
        AxisIp4 currChunk = siIPRX_Data.read();
        switch (tle_chunkCount) {
        case CHUNK_0:
            tle_ip4HdrLen = currChunk.getIp4HdrLen();
            tle_ip4TotLen = currChunk.getIp4TotalLen();
            // Compute length of IPv4 data (.i.e. the TCP segment length)
            tle_ipDataLen  = tle_ip4TotLen - (tle_ip4HdrLen * 4);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_chunkCount++;
            break;
        case CHUNK_1:
            // Forward length of IPv4 data (.i.e. the TCP segment length)
            soIph_TcpSegLen.write(tle_ipDataLen);
            tle_ip4HdrLen -= 2; // We just processed 8 bytes
            tle_chunkCount++;
            break;
        case CHUNK_2:
            // Forward destination IP address
            // Warning, half of this address is now in 'prevChunk'
            sendChunk.setLE_TData(tle_prevChunk.getLE_TData(63, 32), 31,  0);
            sendChunk.setLE_TKeep(tle_prevChunk.getLE_TKeep( 7,  4),  3,  0);
            sendChunk.setLE_TData(    currChunk.getLE_TData(31,  0), 63, 32);
            sendChunk.setLE_TKeep(    currChunk.getLE_TKeep( 3,  0),  7,  4);
            sendChunk.setLE_TLast(    currChunk.getLE_TKeep()[4] == 0);
            soIph_Data.write(sendChunk);
            tle_ip4HdrLen -= 1;  // We just processed the last 4 bytes of a standard IP4 header
            tle_chunkCount++;
            if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
            break;
        case CHUNK_3:
            switch (tle_ip4HdrLen) {
            case 0: // No or end of IP option(s) - Forward half of 'prevChunk' and half of 'currChunk'.
                sendChunk.setLE_TData(tle_prevChunk.getLE_TData(63, 32), 31,  0);
                sendChunk.setLE_TKeep(tle_prevChunk.getLE_TKeep( 7,  4),  3,  0);
                sendChunk.setLE_TData(    currChunk.getLE_TData(31,  0), 63, 32);
                sendChunk.setLE_TKeep(    currChunk.getLE_TKeep( 3,  0),  7,  4);
                sendChunk.setLE_TLast(    currChunk.getLE_TKeep()[4] == 0);
                soIph_Data.write(sendChunk);
                tle_shift = true;
                tle_chunkCount++;
                if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
                break;
            case 1: // Drop IP option located in 'prevChunk' and forward entire 'currChunk'
                sendChunk = AxisRaw(currChunk.getLE_TData(), currChunk.getLE_TKeep(), currChunk.getLE_TLast());
                soIph_Data.write(sendChunk);
                tle_shift = false;
                tle_ip4HdrLen = 0;
                tle_chunkCount++;
                if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
                break;
            default: // Drop two option chunks located in half of 'prevChunk' and half of 'currChunk'.
                tle_ip4HdrLen -= 2;
                break;
            }
            break;
        default:
            if (tle_shift) {
                // The 'currChunk' is not aligned with the outgoing 'sendChunk'
                sendChunk.setLE_TData(tle_prevChunk.getLE_TData(63, 32), 31,  0);
                sendChunk.setLE_TKeep(tle_prevChunk.getLE_TKeep( 7,  4),  3,  0);
                sendChunk.setLE_TData(    currChunk.getLE_TData(31,  0), 63, 32);
                sendChunk.setLE_TKeep(    currChunk.getLE_TKeep( 3,  0),  7,  4);
                sendChunk.setLE_TLast(    currChunk.getLE_TKeep()[4] == 0);
                soIph_Data.write(sendChunk);
                if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
                if (currChunk.getTLast()) {
                    tle_chunkCount = 0;
                    tle_residue = (currChunk.getLE_TKeep()[4] != 0);
                }
            }
            else {
                // The 'currChunk' is aligned with the outgoing 'sendChunk'
                sendChunk = AxisIp4(currChunk.getLE_TData(), currChunk.getLE_TKeep(), currChunk.getLE_TLast());
                soIph_Data.write(sendChunk);
                if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
                if (currChunk.getTLast()) {
                    tle_chunkCount = 0;
                }
            }
            break;
        } // End of: switch (tle_chunkCount)
        // Always copy 'currChunk' into 'prevChunk'
        tle_prevChunk = currChunk;
    }
    else if (tle_residue) {
        // Send remaining data
        AxisRaw sendChunk = AxisRaw(0, 0, TLAST);
        sendChunk.setLE_TData(tle_prevChunk.getLE_TData(63, 32), 31, 0);
        sendChunk.setLE_TKeep(tle_prevChunk.getLE_TKeep( 7,  4),  3, 0);
        soIph_Data.write(sendChunk);
        tle_residue = false;
        if (DEBUG_LEVEL & TRACE_TLE) { printAxisRaw(myName, "soIph_Data =", sendChunk); }
    }
}

/*******************************************************************************
 * @brief Insert pseudo header (Iph)
 *
 * @param[in]  siTle_Data      A custom-made pseudo packet from TcpLengthExtractor (Tle).
 * @param[in]  siTle_TcpSegLen The length of the incoming TCP segment from [Tle].
 * @param[out] soCsa_PseudoPkt Pseudo TCP/IP packet to CheckSumAccumulator (Csa).
 *
 * @details
 *  Constructs a TCP pseudo header and prepends it to the TCP payload. The
 *  result is a TCP pseudo packet (@see AxisPsd4.hpp).
 *
 *  The format of the incoming custom-made pseudo packet is as follows (see [TLe]):
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
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
 *    The format of the outgoing pseudo TCP segment is as follows (see also AxisPsd4.hpp):
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
 *  |               |               |               |               |               |               |C|E|U|A|P|R|S|F|  Data |     |N|
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |W|C|R|C|S|S|Y|I| Offset| Res |S|
 *  |               |               |               |               |               |               |R|E|G|K|H|T|N|N|       |     | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pInsertPseudoHeader(
        stream<AxisRaw>     &siTle_Data,
        stream<TcpSegLen>   &siTle_TcpSegLen,
        stream<AxisPsd4>    &soCsa_PseudoPkt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Iph");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                iph_residue=false;
    #pragma HLS RESET variable=iph_residue
    static ap_uint<2>          iph_chunkCount=0;
    #pragma HLS RESET variable=iph_chunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisRaw  iph_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    ValBit          valid;
    TcpSegLen       tcpSegLen;
    AxisRaw         currChunk;
    AxisPsd4        sendChunk;

    if (iph_residue) {
        sendChunk.setLE_TData(iph_prevChunk.getLE_TData(63, 32), 31,  0);
        sendChunk.setLE_TKeep(iph_prevChunk.getLE_TKeep( 7,  4),  3,  0);
        sendChunk.setLE_TData(                       0x00000000, 63, 32);
        sendChunk.setLE_TKeep(                              0x0,  7,  4);
        sendChunk.setLE_TLast(TLAST);
        soCsa_PseudoPkt.write(sendChunk);
        iph_residue = false;
        if (DEBUG_LEVEL & TRACE_IPH) { printAxisRaw(myName, "soCsa_PseudoPkt =", sendChunk); }
    }
    else if(!siTle_Data.empty() and !soCsa_PseudoPkt.full()) {
        switch (iph_chunkCount) {
        case CHUNK_0:
            // Forward [IP_DA|IP_SA]
            siTle_Data.read(currChunk);
            soCsa_PseudoPkt.write(currChunk);
            iph_chunkCount++;
            if (DEBUG_LEVEL & TRACE_IPH) { printAxisRaw(myName, "soCsa_PseudoPkt =", currChunk); }
            break;
        case CHUNK_1:
            // Forward [TCP_DP|TCP_SP|TCP_LEN|PROT|0x00]
            if (!siTle_TcpSegLen.empty()) {
                siTle_TcpSegLen.read(tcpSegLen);
                siTle_Data.read(currChunk);
                // Assemble outgoing chunk
                sendChunk.setPsd4ResBits(0x00);
                sendChunk.setPsd4Prot(IP4_PROT_TCP);
                sendChunk.setPsd4Len(tcpSegLen);
                sendChunk.setTcpSrcPort(byteSwap16(currChunk.getLE_TData(15,  0)));
                sendChunk.setTcpDstPort(byteSwap16(currChunk.getLE_TData(31, 16)));
                sendChunk.setLE_TKeep(0xFF);
                sendChunk.setLE_TLast(0);
                soCsa_PseudoPkt.write(sendChunk);
                iph_chunkCount++;
                if (DEBUG_LEVEL & TRACE_IPH) printAxisRaw(myName, "soCsa_PseudoPkt =", sendChunk);
            }
            break;
        default:
            siTle_Data.read(currChunk);
            // Forward [    ACK_NUM    |    SEQ_NUM    ] or
            //         [URG_PTR|CHEKSUM|WINSIZE| FLAGS ] or
            //         [             DATA              ]
            sendChunk.setLE_TData(iph_prevChunk.getLE_TData(63, 32), 31,  0);
            sendChunk.setLE_TKeep(iph_prevChunk.getLE_TKeep( 7,  4),  3,  0);
            sendChunk.setLE_TData(    currChunk.getLE_TData(31,  0), 63, 32);
            sendChunk.setLE_TKeep(    currChunk.getLE_TKeep( 3,  0),  7,  4);
            sendChunk.setLE_TLast(currChunk.getLE_TKeep()[4] == 0);
            soCsa_PseudoPkt.write(sendChunk);
            if (DEBUG_LEVEL & TRACE_IPH) printAxisRaw(myName, "soCsa_PseudoPkt =", sendChunk);
            break;
        }
        // Always copy 'currChunk' into 'prevChunk'
        iph_prevChunk = currChunk;
        if (currChunk.getTLast()) {
            iph_chunkCount = 0;
            iph_residue = currChunk.getLE_TKeep()[4] != 0;
        }
    }
}

/*******************************************************************************
 * @brief TCP checksum accumulator (Csa)
 *
 * @param[in]  siIph_PseudoPkt A pseudo TCP packet from InsertPseudoHeader (Iph).
 * @param[out] soTid_Data      TCP data stream to TcpInvalidDropper (Tid).
 * @param[out] soTid_DataVal   TCP data valid to [Tid].
 * @param[out] soMdh_Meta      TCP metadata to MetaDataHandler (Mdh).
 * @param[out] soMdh_SockPair  TCP socket pair to [Mdh].
 * @param[out] soPRt_GetState  Req state of the TCP DestPort to PortTable (PRt).
 *
 * @details
 *  This process extracts the data section from the incoming pseudo IP packet
 *   and forwards it to the TcpInvalidDropper (Tid) together with a valid bit
 *   indicating the result of the checksum validation.
 *  It also extracts the socket pair information and some metadata information
 *   from the TCP segment and forwards them to the MetaDataHandler (Mdh).
 *  Next, the TCP destination port number is extracted and forwarded to the
 *   PortTable (PRt) process to check if the port is open.
 *
 *  The format of the incoming pseudo TCP segment is as follows (@see AxisPsd4.hpp):
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
 *******************************************************************************/
void pCheckSumAccumulator(
        stream<AxisPsd4>          &siIph_PseudoPkt,
        stream<AxisApp>           &soTid_Data,
        stream<ValBit>            &soTid_DataVal,
        stream<RXeMeta>           &soMdh_Meta,
        stream<SocketPair>        &soMdh_SockPair,
        stream<TcpPort>           &soPRt_GetState)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Csa");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<3>           csa_chunkCount=0;
    #pragma HLS RESET  variable=csa_chunkCount
    static bool                 csa_doCSumVerif=false;
    #pragma HLS RESET  variable=csa_doCSumVerif
    static bool                 csa_residue=false;
    #pragma HLS RESET  variable=csa_residue
    static ap_uint<3>           csa_cc_state=0;
    #pragma HLS RESET  variable=csa_cc_state
    static ap_uint<17>          csa_tcp_sums[4]={0, 0, 0, 0};
    #pragma HLS RESET  variable=csa_tcp_sums
    #pragma HLS ARRAY_PARTITION \
                       variable=csa_tcp_sums complete dim=1

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TcpDataOff       csa_dataOffset;
    static bool             csa_doShift;
    static SocketPair       csa_socketPair;
    static RXeMeta          csa_meta;
    static TcpDstPort       csa_tcpDstPort;
    static TcpChecksum      csa_tcpCSum;
    static ap_uint<32>      csa_half_tdata;
    static ap_uint<4>       csa_half_tkeep;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisPsd4                currChunk(0,0,0);
    AxisApp                 sendChunk(0,0,0);

    if (!siIph_PseudoPkt.empty() and !soTid_Data.full() and !csa_doCSumVerif) {
        currChunk = siIph_PseudoPkt.read();

        switch (csa_chunkCount) {
        case CHUNK_0:
            csa_doShift = false;
            // Get IP-SA & IP-DA
            csa_socketPair.src.addr = currChunk.getPsd4SrcAddr();
            csa_socketPair.dst.addr = currChunk.getPsd4DstAddr();
            sendChunk.setTLast(currChunk.getTLast());
            csa_chunkCount++;
            break;
        case CHUNK_1:
            // Get Segment length
            csa_meta.length = currChunk.getPsd4Len();
            // Get TCP-SP & TCP-DP
            csa_socketPair.src.port = currChunk.getTcpSrcPort();
            csa_socketPair.dst.port = currChunk.getTcpDstPort();
            csa_tcpDstPort          = currChunk.getTcpDstPort();
            sendChunk.setTLast(currChunk.getTLast());
            csa_chunkCount++;
            break;
        case CHUNK_2:
            // Get Sequence and Acknowledgment Numbers
            csa_meta.seqNumb = currChunk.getTcpSeqNum();
            csa_meta.ackNumb = currChunk.getTcpAckNum();
            sendChunk.setTLast(currChunk.getTLast());
            csa_chunkCount++;
            break;
        case CHUNK_3:
            // Get Data Offset
            csa_dataOffset   = currChunk.getTcpDataOff();
            csa_meta.length -= (csa_dataOffset * 4);
            // Get Control Bits
            //  [ 8] == FIN | [ 9] == SYN | [10] == RST
            //  [11] == PSH | [12] == ACK | [13] == URG
            csa_meta.ack = currChunk.getTcpCtrlAck();
            csa_meta.rst = currChunk.getTcpCtrlRst();
            csa_meta.syn = currChunk.getTcpCtrlSyn();
            csa_meta.fin = currChunk.getTcpCtrlFin();
            // Get Window Size
            csa_meta.winSize = currChunk.getTcpWindow();
            // Get the checksum of the pseudo-header (only for debug purposes)
            csa_tcpCSum  = currChunk.getTcpChecksum();
            sendChunk.setTLast(currChunk.getTLast());
            csa_chunkCount++;
            break;
        default:
            if (csa_dataOffset >= 6) {
                // Handle TCP options.
                //  Warning: At this point, we only support one TCP option and
                //   we further assume that this option is always 32-bit aligned.
                //  [TODO] Must add support for Window Scaling and unaligned options.
                TcpOptKind optionKind = currChunk.getTcpOptKind();
                if (optionKind == TCP_OPT_KIND_MSS) {
                    // Extract the MSS value
                    TcpOptMss theirMSS = currChunk.getTcpOptMss();
                    if (DEBUG_LEVEL & TRACE_CSA) {
                        printInfo(myName, "TCP segment includes 4 option bytes (OptKind=2, OptLen=4, MSS=%d)\n",
                                  theirMSS.to_uint());
                    }
                }
                else {
                    printWarn(myName, "The TCP option %d is not yet supported and will be dropped.\n", optionKind.to_uchar());
                }
                if (csa_dataOffset == 6) {
                    csa_dataOffset -= 1;
                     csa_doShift = true;
                     csa_half_tdata = currChunk.getLE_TData(63, 32);
                     csa_half_tkeep = currChunk.getLE_TKeep( 7,  4);
                }
                else {
                    csa_dataOffset -= 2;
                }
            }
            else {
                // The DataOffset == 5 (or less)
                if (!csa_doShift) {
                    sendChunk = currChunk;
                    soTid_Data.write(sendChunk);
                }
                else {
                    sendChunk.setLE_TData(  csa_half_tdata, 31,  0);
                    sendChunk.setLE_TKeep(  csa_half_tkeep,  3,  0);
                    sendChunk.setLE_TData(currChunk.getLE_TData(31,  0), 63, 32);
                    sendChunk.setLE_TKeep(currChunk.getLE_TKeep( 3,  0),  7,  4);
                    sendChunk.setTLast(   currChunk.getLE_TKeep()[4] == 0);
                    soTid_Data.write(sendChunk);
                    if (DEBUG_LEVEL & TRACE_CSA) { printAxisRaw(myName, "soTid_Data() =", sendChunk); }
                    csa_half_tdata = currChunk.getLE_TData(63, 32);
                    csa_half_tkeep = currChunk.getLE_TKeep( 7,  4);
                }
            }
            break;
        } // End of: switch

        // Accumulate TCP checksum
        for (int i = 0; i < 4; i++) {
            #pragma HLS UNROLL
            TcpCsum temp;
            if (currChunk.getLE_TKeep(i*2+1, i*2) == 0x3) {
                temp( 7, 0) = currChunk.getLE_TData(i*16+15, i*16+8);
                temp(15, 8) = currChunk.getLE_TData(i*16+ 7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
            else if (currChunk.getLE_TKeep()[i*2] == 0x1) {
                temp( 7, 0) = 0;
                temp(15, 8) = currChunk.getLE_TData(i*16+7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
        }

        // Handle last chunk
        if(currChunk.getTLast()) {
            csa_chunkCount = 0;
            csa_residue = !sendChunk.getTLast();
            csa_doCSumVerif = true;
        }
    } // End of: if (!siIph_Data.empty() && !csa_doCSumVerif)
    else if(csa_residue and !soTid_Data.full()) {
        if (csa_meta.length != 0) {
            sendChunk.setLE_TData(csa_half_tdata, 31,  0);
            sendChunk.setLE_TData(    0x00000000, 63, 32);
            sendChunk.setLE_TKeep(csa_half_tkeep,  3,  0);
            sendChunk.setLE_TKeep(           0x0,  7,  4);
            sendChunk.setLE_TLast(TLAST);
            soTid_Data.write(sendChunk);
            if (DEBUG_LEVEL & TRACE_CSA) { printAxisRaw(myName, "soTid_Data() =", currChunk); }
        }
        csa_residue = false;
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
                    // Forward to metadata to MetaDataHandler
                    soMdh_Meta.write(csa_meta);
                    soMdh_SockPair.write(csa_socketPair);
                    if (csa_meta.length != 0) {
                        // Forward valid checksum info to TcpInvalidDropper
                        soTid_DataVal.write(OK);
                        if (DEBUG_LEVEL & TRACE_CSA) {
                            printInfo(myName, "Received end-of-packet. Checksum is correct.\n");
                        }
                    }
                    // Request state of TCP_DP
                    soPRt_GetState.write(csa_tcpDstPort);
                }
                else {
                    printWarn(myName, "RECEIVED BAD CHECKSUM (0x%4.4X - Delta= 0x%4.4X).\n",
                                csa_tcpCSum.to_uint(), (~csa_tcp_sums[0](15, 0).to_uint() & 0xFFFF));
                    if(csa_meta.length != 0) {
                        // Packet has some TCP payload
                        soTid_DataVal.write(KO);
                    }
                    if (DEBUG_LEVEL & TRACE_CSA) {
                        printSockPair(myName, csa_socketPair);
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

/*******************************************************************************
 * @brief TCP Invalid checksum Dropper (Tid)
 *
 * @param[in]  siCsa_Data    TCP data stream from CheckSumAccumulator (Csa).
 * @param[in]  siCsa_DataVal TCP data segment valid.
 * @param[out] soTsd_Data    TCP data stream to TcpSegmentDropper (Tsd).
 * @param[out] soMMIO_CrcDropCnt The value of the CRC drop counter.
 *
 * @details
 *  This process drops the incoming TCP segment when it is flagged with an
 *   invalid checksum. Otherwise, the TCP segment is passed on.
 *
 *******************************************************************************/
void pTcpInvalidDropper(
        stream<AxisApp>     &siCsa_Data,
        stream<ValBit>      &siCsa_DataVal,
        stream<AxisApp>     &soTsd_Data,
        stream<ap_uint<8> > &soMMIO_CrcDropCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tid");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { TSD_IDLE=0, TSD_FWD, TSD_DROP } \
                               tid_fsmState=TSD_IDLE;
    #pragma HLS RESET variable=tid_fsmState
    static ap_uint<8 >         tid_crcDropCounter=0;
    #pragma HLS reset variable=tid_crcDropCounter


    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisApp     currChunk;
    ValBit      validBit;

    switch (tid_fsmState) {
    case TSD_IDLE:
        if (!siCsa_DataVal.empty()) {
            siCsa_DataVal.read(validBit);
            if (validBit == OK) {
                tid_fsmState = TSD_FWD;
            }
            else {
                tid_fsmState = TSD_DROP;
                tid_crcDropCounter++;
                printWarn(myName, "Bad checksum: Dropping payload for this packet!\n");
            }
        }
        break;
    case TSD_FWD:
        if(!siCsa_Data.empty() && !soTsd_Data.full()) {
            siCsa_Data.read(currChunk);
            soTsd_Data.write(currChunk);
            if (currChunk.getTLast()) {
                tid_fsmState = TSD_IDLE;
            }
            if (DEBUG_LEVEL & TRACE_TID) { printAxisRaw(myName, "soTsd_Data =", currChunk); }
        }
        break;
    case TSD_DROP:
        if(!siCsa_Data.empty()) {
            siCsa_Data.read(currChunk);
            if (currChunk.getTLast()) {
                tid_fsmState = TSD_IDLE;
            }
        }
        break;
    } // End of: switch

    //-- ALWAYS
    if (!soMMIO_CrcDropCnt.full()) {
        soMMIO_CrcDropCnt.write(tid_crcDropCounter);
    }
    else {
        printFatal(myName, "Cannot write soMMIO_CrcDropCnt stream...");
    }

} // End of: pTcpInvalidDropper

/*******************************************************************************
 * @brief TCP Segment Dropper (Tsd)
 *
 * @param[in]  siTid_Data     TCP data stream from TcpInvalidDropper (Tid).
 * @param[in]  siMdh_DropCmd  Drop command from MetaDataHandler (Mdh).
 * @param[in]  siFsm_DropCmd  Drop command from FiniteStateMachine (Fsm).
 * @param[out] soMwr_Data     TCP data stream to MemoryWriter (MWr).
 *
 * @details
 *  This process drops the TCP segment which metadata did not match and/or
 *   which is invalid.
 *******************************************************************************/
void pTcpSegmentDropper(
        stream<AxisApp>     &siTid_Data,
        stream<CmdBit>      &siMdh_DropCmd,
        stream<CmdBit>      &siFsm_DropCmd,
        stream<AxisApp>     &soMwr_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Tsd");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { TSD_RD_DROP_CMD1=0, TSD_RD_DROP_CMD2, TSD_FWD, TSD_DROP } \
                               tsd_fsmState=TSD_RD_DROP_CMD1;
    #pragma HLS RESET variable=tsd_fsmState

    switch (tsd_fsmState) {
    case TSD_RD_DROP_CMD1:
        if (!siMdh_DropCmd.empty()) {
            CmdBit dropCmd = siMdh_DropCmd.read();
            if (dropCmd) {
                tsd_fsmState = TSD_DROP;
                printWarn(myName, "[Mdh] is requesting to drop this packet.\n");
            }
            else {
                tsd_fsmState = TSD_RD_DROP_CMD2;
            }
        }
        break;
    case TSD_RD_DROP_CMD2:
        if (!siFsm_DropCmd.empty()) {
            CmdBit dropCmd = siFsm_DropCmd.read();
            if (dropCmd) {
                tsd_fsmState = TSD_DROP;
                printWarn(myName, "[Fsm] is requesting to drop this packet.\n");
            }
            else {
                tsd_fsmState = TSD_FWD;
            }
        }
        break;
    case TSD_FWD:
        if(!siTid_Data.empty() && !soMwr_Data.full()) {
            AxisApp currChunk = siTid_Data.read();
            if (currChunk.getTLast()) {
                tsd_fsmState = TSD_RD_DROP_CMD1;
            }
            soMwr_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_TID) { printAxisRaw(myName, "soMwr_Data =", currChunk); }
        }
        break;
    case TSD_DROP:
        if(!siTid_Data.empty()) {
            AxisApp currChunk = siTid_Data.read();
            if (currChunk.getTLast()) {
                tsd_fsmState = TSD_RD_DROP_CMD1;
            }
        }
        break;
    } // End of: switch
}

/*******************************************************************************
 * @brief Rx Memory Writer (Mwr)
 *
 * @param[in]  siTsd_Data      Tcp data stream from the Tcp SegmentDropper (Tid).
 * @param[in]  siFsm_MemWrCmd  Memory write command from the FiniteStateMachine (Fsm).
 * @param[out] soMEM_WrCmd     Memory write command to the data mover of Memory sub-system (MEM).
 * @param[out] soMEM_WrData    Memory data write stream to [MEM].
 * @param[out] soRan_SplitSeg  Split segment flag to RxAppNotifier (Ran).
 *
 * @details
 *  This is the front end memory controller process for writing the received TCP
 *   data segments into the external DRAM. Memory write commands are received
 *   from the FiniteStetMachine (Fsm) and are transferred to the data-mover of
 *   the memory sub-system (MEM).
 *  The Rx buffer memory is organized and managed as a circular buffer, and it
 *   may happen that the received segment does not fit into remain memory buffer
 *   space because the memory pointer needs to wrap around. In such a case, the
 *   incoming segment is broken down and written into physical DRAM as two
 *   memory buffers, and the follow-on RxAppNotifier (Ran) process is notified
 *   about this splitted segment
 *******************************************************************************/
void pRxMemoryWriter(
        stream<AxisApp>     &siTsd_Data,
        stream<DmCmd>       &siFsm_MemWrCmd,
        stream<DmCmd>       &soMEM_WrCmd,
        stream<AxisApp>     &soMEM_WrData,
        stream<FlagBool>    &soRan_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Mwr");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { MWR_IDLE=0,
                            MWR_FWD_ALIGNED, MWR_SPLIT_1ST_CMD,
                            MWR_FWD_1ST_BUF, MWR_FWD_2ND_BUF, MWR_RESIDUE } \
                                mwr_fsmState=MWR_IDLE;
    #pragma HLS RESET  variable=mwr_fsmState
    static ap_uint<3>           mwr_residueLen=0;
    #pragma HLS RESET  variable=mwr_residueLen
    static bool                 mwr_accessBreakdown=false;
    #pragma HLS RESET  variable=mwr_accessBreakdown

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd       mwr_memWrCmd;
    static AxisApp     mwr_currChunk;
    static RxBufPtr    mwr_firstAccLen;
    static TcpSegLen   mwr_nrBytesToWr;
    static ap_uint<4>  mwr_splitOffset;
    static uint16_t    mwr_debugCounter=3; // To align with # in the DAT file

    switch (mwr_fsmState) {
    case MWR_IDLE:
        if (!siFsm_MemWrCmd.empty() and !soRan_SplitSeg.full() and !soMEM_WrCmd.full()) {
            siFsm_MemWrCmd.read(mwr_memWrCmd);
            if ((mwr_memWrCmd.saddr.range(TOE_WINDOW_BITS-1, 0) + mwr_memWrCmd.btt) > TOE_RX_BUFFER_SIZE) {
                //-- Break this segment in two memory accesses because TCP Rx memory buffer wraps around
                soRan_SplitSeg.write(true);

                if (DEBUG_LEVEL & TRACE_MWR) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This segment will be broken in two memory buffers.\n");
                }
                mwr_fsmState = MWR_SPLIT_1ST_CMD;
            }
            else {
                //-- Don't split and issue the write command
                soRan_SplitSeg.write(false);
                soMEM_WrCmd.write(mwr_memWrCmd);

                if (DEBUG_LEVEL & TRACE_MWR) {
                    printInfo(myName, "Issuing memory write command #%d - SADDR=0x%9.9x - BTT=%d\n",
                              mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.btt.to_uint());
                    mwr_debugCounter++;
                }
                mwr_fsmState = MWR_FWD_ALIGNED;
            }
        }
        break;
    case MWR_SPLIT_1ST_CMD:
        if (!soMEM_WrCmd.full()) {
            mwr_firstAccLen   = TOE_RX_BUFFER_SIZE - mwr_memWrCmd.saddr;
            mwr_nrBytesToWr   = mwr_firstAccLen;
            soMEM_WrCmd.write(DmCmd(mwr_memWrCmd.saddr, mwr_firstAccLen));

            if (DEBUG_LEVEL & TRACE_MWR) {
                printInfo(myName, "Issuing 1st memory write command #%d - SADDR=0x%9.9x - BTT=%d\n",
                    mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_firstAccLen.to_uint());
            }
            mwr_fsmState = MWR_FWD_1ST_BUF;
        }
        break;
    case MWR_FWD_ALIGNED:
        if (!siTsd_Data.empty() and !soMEM_WrData.full()) {
            //-- Default streaming state used to forward segments or splitted buffers
            //-- that are aligned with the Axis raw width
            AxisApp memChunk = siTsd_Data.read();
            soMEM_WrData.write(memChunk);
            if (memChunk.getTLast()) {
                 mwr_fsmState = MWR_IDLE;
            }

            if (DEBUG_LEVEL & TRACE_MWR) { printAxisRaw(myName, "FWD_ALIGNED: soMEM_WrData =", memChunk); }
         }
        break;
    case MWR_FWD_1ST_BUF:
        if (!siTsd_Data.empty() and !soMEM_WrData.full()) {
            //-- Create 1st splitted data buffer and stream it to memory
            siTsd_Data.read(mwr_currChunk);
            AxisApp memChunk = mwr_currChunk;
            if (mwr_nrBytesToWr > (ARW/8)) {
                mwr_nrBytesToWr -= (ARW/8);
            }
            else if (!soMEM_WrCmd.full()) {
                if (mwr_nrBytesToWr == (ARW/8)) {
                    //-- End of 1st segment and begin of 2nd segment are aligned
                    memChunk.setLE_TLast(TLAST);

                    //-- Prepare and issue 2nd command
                    mwr_memWrCmd.saddr(TOE_WINDOW_BITS-1, 0) = 0;
                    mwr_memWrCmd.btt -= mwr_firstAccLen;
                    soMEM_WrCmd.write(mwr_memWrCmd);

                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printInfo(myName, "Issuing 2nd memory write command #%d - SADDR=0x%9.9x - BTT=%d\n",
                                  mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.btt.to_uint());
                        mwr_debugCounter++;
                    }
                    mwr_fsmState = MWR_FWD_ALIGNED;
                }
                else {
                    //-- End of 1st segment and begin of 2nd segment not aligned
                    memChunk.setLE_TLast(TLAST);
                    memChunk.setLE_TKeep(lenToLE_tKeep(mwr_nrBytesToWr));
                    #ifndef __SYNTHESIS__
                    memChunk.setLE_TData(0, (ARW-1), ((int)mwr_nrBytesToWr*8));
                    #endif

                    //-- Prepare and issue 2nd command
                    mwr_memWrCmd.saddr(TOE_WINDOW_BITS-1, 0) = 0;
                    mwr_memWrCmd.btt -= mwr_firstAccLen;
                    soMEM_WrCmd.write(mwr_memWrCmd);

                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printInfo(myName, "Issuing 2nd memory write command #%d - SADDR=0x%9.9x - BTT=%d\n",
                                  mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.btt.to_uint());
                        mwr_debugCounter++;
                    }
                    mwr_splitOffset = (ARW/8) - mwr_nrBytesToWr;
                    if (mwr_currChunk.getLE_TLast()) {
                        mwr_fsmState = MWR_RESIDUE;
                    }
                    else {
                        mwr_fsmState = MWR_FWD_2ND_BUF;
                    }
                }
            }

            soMEM_WrData.write(memChunk);
            if (DEBUG_LEVEL & TRACE_MWR) { printAxisRaw(myName, "FWD_1ST_BUF: soMEM_WrData =", memChunk); }
        }
        break;
    case MWR_FWD_2ND_BUF:
        if (!siTsd_Data.empty() && !soMEM_WrData.full()) {
            //-- Alternate streaming state used to re-align a splitted second buffer
            AxisApp prevChunk = mwr_currChunk;
            mwr_currChunk = siTsd_Data.read();

            AxisApp joinedChunk(0,0,0);  // [FIXME-Create a join method in AxisRaw]
            // Set lower-part of the joined chunk with the last bytes of the previous chunk
            joinedChunk.setLE_TData(prevChunk.getLE_TData((ARW  )-1, ((ARW  )-((int)mwr_splitOffset*8))),
                                                                              ((int)mwr_splitOffset*8)-1, 0);
            joinedChunk.setLE_TKeep(prevChunk.getLE_TKeep((ARW/8)-1, ((ARW/8)-((int)mwr_splitOffset  ))),
                                                                              ((int)mwr_splitOffset  )-1, 0);
            // Set higher part of the joined chunk with the first bytes of the current chunk
            joinedChunk.setLE_TData(mwr_currChunk.getLE_TData((ARW  )-((int)mwr_splitOffset*8)-1, 0),
                                                              (ARW  )                         -1, ((int)mwr_splitOffset*8));
            joinedChunk.setLE_TKeep(mwr_currChunk.getLE_TKeep((ARW/8)-((int)mwr_splitOffset  )-1, 0),
                                                              (ARW/8)                         -1, ((int)mwr_splitOffset  ));
            if (mwr_currChunk.getLE_TLast()) {
                if (mwr_currChunk.getLen() > mwr_nrBytesToWr) {
                    // This cannot be the last chunk because the current one plus
                    // the remainder of the previous one do not fit into a single chunk.
                    // Goto the 'MWR_RESIDUE' and handle the remainder of that chunk.
                    mwr_fsmState = MWR_RESIDUE;
                }
                else {
                    // The entire current chunk and the remainder of the previous chunk
                    // fit into a single chunk. We are done with this 2nd memory buffer.
                    joinedChunk.setLE_TLast(TLAST);
                    mwr_fsmState = MWR_IDLE;
                }
            }
            soMEM_WrData.write(joinedChunk);
            if (DEBUG_LEVEL & TRACE_MWR) { printAxisRaw(myName, "FWD_2ND_BUF: soMEM_WrData =", joinedChunk); }
        }
        break;
    case MWR_RESIDUE:
        if (!soMEM_WrData.full()) {
            //-- Output the very last unaligned chunk
            AxisApp prevChunk = mwr_currChunk;
            AxisApp lastChunk(0,0,0);

            // Set lower-part of the last chunk with the last bytes of the previous chunk
            lastChunk.setLE_TData(prevChunk.getLE_TData((ARW  )-1, ((ARW  )-((int)mwr_splitOffset*8))),
                                                                            ((int)mwr_splitOffset*8)-1, 0);
            lastChunk.setLE_TKeep(prevChunk.getLE_TKeep((ARW/8)-1, ((ARW/8)-((int)mwr_splitOffset  ))),
                                                                            ((int)mwr_splitOffset  )-1, 0);
            lastChunk.setLE_TLast(TLAST);
            soMEM_WrData.write(lastChunk);

            if (DEBUG_LEVEL & TRACE_MWR) { printAxisRaw(myName, "RESIDUE    : soMEM_WrData =", lastChunk); }
            mwr_fsmState = MWR_IDLE;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Rx Application Notifier (Ran)
 *
 * @param[in]  siMEM_WrSts     The memory write status from the AXI data mover [MEM].
 * @param[in]  siFsm_Notif     Rx data notification from FiniteStateMachine (Fsm).
 * @param[out] soRAi_RxNotif   Rx data notification to RxApplicationInterface (RAi).
 * @param[in]  siMwr_SplitSeg  Split segment flag from MemoryWriter (Mwr).
 * @param[out] soMMIO_MemWrErr Reports a memory write error to MMIO.
 *
 * @details
 *  Delays the notifications to the application until the TCP segment is
 *   actually written into the physical DRAM memory.
 *  If the segment was split in two memory accesses, the current process will
 *   wait until both segments are written into memory before issuing the
 *   notification to the application.
 *  Any error reported by the AXI data mover (MEM) is forwarded to MIMO and
 *   remains set until [TOE] is reset.
 *******************************************************************************/
void pRxAppNotifier(
        stream<DmSts>         &siMEM_WrSts,
        stream<TcpAppNotif>   &siFsm_Notif,
        stream<TcpAppNotif>   &soRAi_RxNotif,
        stream<FlagBool>      &siMwr_SplitSeg,
        stream<StsBit>        &soMMIO_MemWrErr)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Ran");

    //-- LOCAL STREAMS ---------------------------------------------------------
    static stream<TcpAppNotif>     ssRxNotifFifo("ssRxNotifFifo");
    #pragma HLS STREAM    variable=ssRxNotifFifo depth=32 // WARNING: [FIXME] Depends on the memory delay !!!
    #pragma HLS DATA_PACK variable=ssRxNotifFifo

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static FlagBool            ran_doubleAccessFlag=FLAG_OFF;
    #pragma HLS RESET variable=ran_doubleAccessFlag
    static StsBit              ran_dataMoverError=STS_NO_ERR;
    #pragma HLS RESET variable=ran_dataMoverError

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmSts        ran_dmStatus1;
    static DmSts        ran_dmStatus2;
    static TcpAppNotif  ran_appNotification;

    if (ran_doubleAccessFlag == FLAG_ON) {
        // The segment was splitted and notification will only go out now
        if(!siMEM_WrSts.empty()) {
            siMEM_WrSts.read(ran_dmStatus2);
            if (ran_dmStatus1.okay and ran_dmStatus2.okay) {
                soRAi_RxNotif.write(ran_appNotification);
                if (DEBUG_LEVEL & TRACE_RAN) {
                    printInfo(myName, "Sending APP notification to [RAi]. This was a double access.\n");
                }
            }
            else {
                ran_dataMoverError = ran_dmStatus1.slverr | ran_dmStatus1.decerr | ran_dmStatus1.interr | \
                                     ran_dmStatus2.slverr | ran_dmStatus2.decerr | ran_dmStatus2.interr;;
                if (DEBUG_LEVEL & TRACE_RAN) {
                    printError(myName, "The previous splitted mem-write command failed (OKAY=0).\n");
                }
            }
            ran_doubleAccessFlag = FLAG_OFF;
        }
    }
    else {
        //-- We don't know yet about a possible double memory access
        if (!siMEM_WrSts.empty() and !siMwr_SplitSeg.empty() and !ssRxNotifFifo.empty()) {
            siMEM_WrSts.read(ran_dmStatus1);
            siMwr_SplitSeg.read(ran_doubleAccessFlag);
            ssRxNotifFifo.read(ran_appNotification);
            if (ran_doubleAccessFlag == FLAG_OFF) {
                // This segment consists of a single memory access
                if (ran_dmStatus1.okay) {
                    // Output the notification now
                    soRAi_RxNotif.write(ran_appNotification);
                    if (DEBUG_LEVEL & TRACE_RAN) {
                        printInfo(myName, "Sending APP notification to [RAi].\n");
                    }
                }
                else {
                    ran_dataMoverError = ran_dmStatus1.slverr | ran_dmStatus1.decerr | ran_dmStatus1.interr;
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
        else if (!siFsm_Notif.empty() and !ssRxNotifFifo.full()) {
            siFsm_Notif.read(ran_appNotification);
            if (ran_appNotification.tcpDatLen != 0) {
                ssRxNotifFifo.write(ran_appNotification);
            }
            else {
                // Do not send forward the notification to [APP]
                printInfo(myName, "Received an RxNotif with LENGTH=0.\n");
            }
        }
    }

    //-- ALWAYS
    if (!soMMIO_MemWrErr.full()) {
        soMMIO_MemWrErr.write(ran_dataMoverError);
    }
    else {
        printFatal(myName, "Cannot write soMMIO_MemWrerr stream...");
    }
}

/*******************************************************************************
 * @brief MetaData Handler (Mdh)
 *
 * @param[in]  siCsa_Meta       TCP metadata from CheckSumAccumulator (Csa).
 * @param[in]  siCsa_SockPair   TCP socket pair from [Csa].
 * @param[out] soSLc_SessLkpReq Session lookup request to Session Lookup Controller (SLc).
 * @param[in]  siSLc_SessLkpRep Session Lookup reply from [SLc].
 * @param[in]  siPRt_PortSts    Port state (opened/closed) from PortTable (PRt).
 * @param[out] soEVe_Event      Event to EventEngine (EVe).
 * @param[out] soTsd_DropCmd    Drop command to Tcp Segment Dropper (Tsd).
 * @param[out] soFsm_Meta       Metadata to RXe's Finite State Machine (Fsm).
 * @param[out] soMMIO_SessDrop  The value of the session drop counter.
 *
 * @details
 *  This process waits until it gets a response from the PortTable (PRt).
 *   It then loads the metadata and socket pair generated by the Checksum-
 *   Accumulator (Csa) process and evaluates them. Next, if the destination
 *   port is opened, it requests the SessionLookupController (SLc) to perform
 *   a session lookup and waits for its reply. If a session is opened for this
 *   socket pair, a new metadata structure is generated and is forwarded to the
 *   FiniteStateMachine (FSm) of the RxEngine (RXe).
 *  If the target destination port is not opened, the process creates an event
 *   requesting a 'RST+ACK' TCP segment to be sent back to the initiating host.
 *******************************************************************************/
void pMetaDataHandler(
        stream<RXeMeta>             &siCsa_Meta,
        stream<SocketPair>          &siCsa_SockPair,
        stream<SessionLookupQuery>  &soSLc_SessLkpReq,
        stream<SessionLookupReply>  &siSLc_SessLkpRep,
        stream<StsBit>              &siPRt_PortSts,
        stream<ExtendedEvent>       &soEVe_Event,
        stream<CmdBit>              &soTsd_DropCmd,
        stream<RXeFsmMeta>          &soFsm_Meta,
        stream<ap_uint<8> >         &soMMIO_SessDropCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Mdh");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { MDH_META=0, \
                            MDH_LOOKUP } mdh_fsmState;
    #pragma HLS RESET           variable=mdh_fsmState
    static ap_uint<8 >                   mdh_SessDropCounter=0;
    #pragma HLS reset           variable=mdh_SessDropCounter

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static RXeMeta              mdh_meta;
    static SessionLookupReply   mdh_sessLookupReply;
    static Ip4Address           mdh_ip4SrcAddr;
    static TcpPort              mdh_tcpSrcPort;
    static TcpPort              mdh_tcpDstPort;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    SocketPair                  socketPair;
    StsBit                      dstPortStatus;

    switch (mdh_fsmState) {
    case MDH_META:
        // Wait until we get a reply from the PortTable (PRt)
        if (!siPRt_PortSts.empty()) {
            //  Read metadata and socket pair
            if (!siCsa_Meta.empty() && !siCsa_SockPair.empty()) {
                siPRt_PortSts.read(dstPortStatus);
                siCsa_Meta.read(mdh_meta);
                siCsa_SockPair.read(socketPair);
                mdh_ip4SrcAddr = socketPair.src.addr;
                mdh_tcpSrcPort = socketPair.src.port;
                mdh_tcpDstPort = socketPair.dst.port;
                if (dstPortStatus == STS_CLOSED) {
                    // The destination port is closed
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printWarn(myName, "Port 0x%4.4X (%d) is not open.\n",
                                  mdh_tcpDstPort.to_uint(), mdh_tcpDstPort.to_uint());
                    }
                    if (!mdh_meta.rst) {
                        // Reply with 'RST+ACK' and send necessary socket-pair through event
                        LE_SocketPair  switchedTuple; // [FIXME]
                        switchedTuple.src.addr = byteSwap32(socketPair.dst.addr);
                        switchedTuple.dst.addr = byteSwap32(socketPair.src.addr);
                        switchedTuple.src.port = byteSwap16(socketPair.dst.port);
                        switchedTuple.dst.port = byteSwap16(socketPair.src.port);
                        if (mdh_meta.syn || mdh_meta.fin) {
                            soEVe_Event.write(ExtendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length+1),
                                                           switchedTuple)); //always 0
                        }
                        else {
                            soEVe_Event.write(ExtendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length),
                                                           switchedTuple));
                        }
                    }
                    else {
                        // The RST bit is set. Ignore => do nothing
                    }
                    if (mdh_meta.length != 0) {
                        soTsd_DropCmd.write(CMD_DROP);
                        mdh_SessDropCounter++;
                    }
                }
                else {
                    // Destination Port is opened
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printInfo(myName, "Destination port 0x%4.4X (%d) is open.\n",
                                  mdh_tcpDstPort.to_uint(), mdh_tcpDstPort.to_uint());
                    }
                    // Query a session lookup. Only allow creation of a new entry when SYN or SYN_ACK
                    LE_SocketPair  leSocketPair(LE_SockAddr(byteSwap32(socketPair.src.addr),byteSwap16(socketPair.src.port)),
                                                LE_SockAddr(byteSwap32(socketPair.dst.addr),byteSwap16(socketPair.dst.port)));
                    soSLc_SessLkpReq.write(SessionLookupQuery(leSocketPair,
                                          (mdh_meta.syn && !mdh_meta.rst && !mdh_meta.fin))); // [FIXME - Endianess
                    if (DEBUG_LEVEL & TRACE_MDH) {
                        printInfo(myName, "Request the SLc to lookup the following session:\n");
                        printSockPair(myName, socketPair);
                    }
                    mdh_fsmState = MDH_LOOKUP;
                }
            }
        }
        break;
    case MDH_LOOKUP:
        // Wait until we get a reply from the SessionLookupController (SLc)
        //  Warning: There may be a large delay for the lookup to complete
        if (!siSLc_SessLkpRep.empty()) {
            siSLc_SessLkpRep.read(mdh_sessLookupReply);
            if (mdh_sessLookupReply.hit) {
                // Forward metadata to the TCP FiniteStateMachine (Fsm)
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
                if (!mdh_sessLookupReply.hit) {
                    mdh_SessDropCounter++;
                }
            }
            mdh_fsmState = MDH_META;
        }
        break;
    } // End of: switch

    //-- ALWAYS
    if (!soMMIO_SessDropCnt.full()) {
        soMMIO_SessDropCnt.write(mdh_SessDropCounter);
    }
    else {
        printFatal(myName, "Cannot write soMMIO_SessDropCnt stream...");
    }
} // End of: pMetaDataHandler

/*******************************************************************************
 * @brief Finite State machine (Fsm)
 *
 * @param[in]  siMdh_FsmMeta     FSM metadata from MetaDataHandler (Mdh).
 * @param[out] soSTt_StateQry    State query to StateTable (STt).
 * @param[in]  siSTt_StateRep    State reply from [STt].
 * @param[out] soRSt_RxSarQry    Query to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep    Reply from [RSt].
 * @param[out] soTSt_TxSarQry    Query to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRep    Reply from [TSt].
 * @param[out] soTIm_ReTxTimerCmd Command for a retransmit timer to Timers (TIm).
 * @param[out] soTIm_ClearProbeTimer Clear the probing timer to [TIm].
 * @param[out] soTIm_CloseTimer  Close session timer to [TIm].
 * @param[out] soTAi_SessOpnSts  Open status of the session to TxAppInterface (TAi).
 * @param[out] soEVe_Event       Event to EventEngine (EVe).
 * @param[out] soTsd_DropCmd     Drop command to TcpSegmentDropper (Tsd).
 * @param[out] soMwr_WrCmd       Memory write command to MemoryWriter (Mwr).
 * @param[out] soRan_RxNotif     Rx data notification to RxAppNotifier (Ran).
 * @param[out] soMMIO_OooDropCnt The value of the out-of-order drop counter.
 *
 * @details
 *  This process implements the typical TCP state and metadata management. It
 *   contains all the logic that updates the metadata and keeps track of the
 *   events related to the reception of segments and their handshaking. This is
 *   the key central part of the Rx engine.
 *****************************************************************************/
void pFiniteStateMachine(
        stream<RXeFsmMeta>          &siMdh_FsmMeta,
        stream<StateQuery>          &soSTt_StateQry,
        stream<TcpState>            &siSTt_StateRep,
        stream<RXeRxSarQuery>       &soRSt_RxSarQry,
        stream<RxSarReply>          &siRSt_RxSarRep,
        stream<RXeTxSarQuery>       &soTSt_TxSarQry,
        stream<RXeTxSarReply>       &siTSt_TxSarRep,
        stream<RXeReTransTimerCmd>  &soTIm_ReTxTimerCmd,
        stream<SessionId>           &soTIm_ClearProbeTimer,
        stream<SessionId>           &soTIm_CloseTimer,
        stream<SessState>           &soTAi_SessOpnSts, // [TODO -Merge with eventEngine]
        stream<Event>               &soEVe_Event,
        stream<CmdBit>              &soTsd_DropCmd,
        stream<DmCmd>               &soMwr_WrCmd,
        stream<TcpAppNotif>         &soRan_RxNotif,
        stream<ap_uint<8> >         &soMMIO_OooDropCnt,
        stream<RxBufPtr>            &soDBG_RxFreeSpace,
        stream<ap_uint<32> >        &soDBG_TcpIpRxByteCnt,
        stream<ap_uint<8> >         &soDBG_OooDebug)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Fsm");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { FSM_LOAD=0, FSM_TRANSITION } \
                                 fsm_fsmState=FSM_LOAD;
    #pragma HLS RESET   variable=fsm_fsmState
    static bool                  fsm_txSarRequest=false;
    #pragma HLS RESET   variable=fsm_txSarRequest
    static ap_uint<8 >           fsm_oooDropCounter=0;
    #pragma HLS RESET   variable=fsm_oooDropCounter
    static RxBufPtr              fsm_freeSpace=0;
    #pragma HLS RESET   variable=fsm_freeSpace
    static ap_uint<32>           fsm_rxByteCounter;
    #pragma HLS RESET   variable=fsm_rxByteCounter
    static ap_uint<8 >           fsm_oooDebugState=0;
    #pragma HLS RESET   variable=fsm_oooDebugState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static RXeFsmMeta   fsm_Meta;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    ap_uint<4>          control_bits;
    TcpState            tcpState;
    RxSarReply          rxSar;
    RXeTxSarReply       txSar;

    fsm_oooDebugState = 0;

    switch(fsm_fsmState) {
    case FSM_LOAD:
        if (!siMdh_FsmMeta.empty()) {
            siMdh_FsmMeta.read(fsm_Meta);
            // Request the current state of this session
            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, QUERY_RD));
            // Always request the RxSarTable, even though not required for SYN-ACK
            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, QUERY_RD));
            if (fsm_Meta.meta.ack) {
                // Only request the txSar when (ACK+ANYTHING); not for SYN
                soTSt_TxSarQry.write(RXeTxSarQuery(fsm_Meta.sessionId, QUERY_RD));
                fsm_txSarRequest = true;
            }
            fsm_fsmState = FSM_TRANSITION;
            fsm_oooDebugState = 1;
        }
        break;
    case FSM_TRANSITION:
        // Check if transition to FSM_LOAD occurs
        if (!siSTt_StateRep.empty() and !siRSt_RxSarRep.empty() and
            !(fsm_txSarRequest and siTSt_TxSarRep.empty())) {
            fsm_fsmState = FSM_LOAD;
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
            if (fsm_fsmState == FSM_LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                //-- ALWAYS - Compute free space to ensure that 'appd' pointer is not overtaken
                fsm_freeSpace = ((rxSar.appd - rxSar.oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
                soDBG_RxFreeSpace.write(fsm_freeSpace);

                if (DEBUG_LEVEL & TRACE_FSM) {
                    printInfo(myName, "Entering 'ACK' processing.\n \
                              \t\t Meta.seqNum  =0x%8.8x \n \
                              \t\t RxSar.appd   =0x%8.8x \n \
                              \t\t RxSar.oooHead=0x%8.8x \n \
                              \t\t RxSar.oooTail=0x%8.8x \n \
                              \t\t FreeSpace    =  %8d\n",
                              fsm_Meta.meta.seqNumb.to_uint(),
                              rxSar.oooHead.to_uint(),
                              rxSar.oooTail.to_uint(),
                              fsm_freeSpace.to_uint());
                }

                TimerCmd timerCmd;
                if (fsm_Meta.meta.ackNumb == txSar.prevUnak) {
                    timerCmd = STOP_TIMER;
                }
                else {
                    timerCmd = LOAD_TIMER;
                }
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));
                if ( (tcpState == ESTABLISHED) || (tcpState == SYN_RECEIVED) ||
                     (tcpState == FIN_WAIT_1)  || (tcpState == CLOSING)      ||
                     (tcpState == LAST_ACK) ) {
                    // Check if new ACK arrived
                    if ( (fsm_Meta.meta.ackNumb == txSar.prevAckd) and
                         (txSar.prevAckd != txSar.prevUnak) ) {
                        // Not new ACK; increase counter but only if it does not contain data
                        if (fsm_Meta.meta.length == 0) {
                            txSar.count++;
                        }
                    }
                    else {
                        // Notify probeTimer about new ACK
                        soTIm_ClearProbeTimer.write(fsm_Meta.sessionId);
                        // Check for SlowStart & Increase Congestion Window
                        if (txSar.cong_window <= (txSar.slowstart_threshold-ZYC2_MSS)) {
                            txSar.cong_window += ZYC2_MSS;
                        }
                        else if (txSar.cong_window <= TOE_MAX_CONGESTION_WINDOW) { // 0xF7FF
                            txSar.cong_window += 365; //TODO replace by approx. of (MSS x MSS) / cong_window
                        }
                        txSar.count = 0;
                        txSar.fastRetransmitted = false;
                    }
                    // Update TxSarTable (only if count or retransmit)
                    if ( (  (fsm_Meta.meta.ackNumb >= txSar.prevAckd) and (fsm_Meta.meta.ackNumb <= txSar.prevUnak)) or
                          (((fsm_Meta.meta.ackNumb >= txSar.prevAckd) or  (fsm_Meta.meta.ackNumb <= txSar.prevUnak)) and (txSar.prevUnak < txSar.prevAckd))) {
                        soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId,
                                                            fsm_Meta.meta.ackNumb,
                                                            fsm_Meta.meta.winSize,
                                                            txSar.cong_window,
                                                            txSar.count,
                                                          ((txSar.count == 3) || txSar.fastRetransmitted))));
                    }

                    // If packet contains payload
                    //  We must handle Out-Of-Order delivered segments
                    if (fsm_Meta.meta.length != 0) {

                        // Build a DDR memory address for this segment
                        //  FYI - The TCP Rx buffers use up to 1GB (16Kx64KB).
                        RxMemPtr memSegAddr = TOE_RX_MEMORY_BASE;
                        memSegAddr(29, TOE_WINDOW_BITS) = fsm_Meta.sessionId(13, 0);
                        memSegAddr(TOE_WINDOW_BITS-1, 0) = fsm_Meta.meta.seqNumb.range(TOE_WINDOW_BITS-1, 0);

                        // Increment the Rx byte counter
                        fsm_rxByteCounter += fsm_Meta.meta.length;

                        RxSeqNum    newRcvd    = 0;
                        RxSeqNum    newOooHead = 0;
                        RxSeqNum    newOooTail = 0;

                        //-- DEFAULT : No OOO and Rx segment is in expected sequence order
                        if (!rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.rcvd) and
                                           (fsm_freeSpace > fsm_Meta.meta.length)) {
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "OOO-DEFAULT : Rx segment is in-order.\n"); }
                            // Generate new received pointer
                            newRcvd = fsm_Meta.meta.seqNumb + fsm_Meta.meta.length;
                            // Update RxSar pointers
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, newRcvd, QUERY_WR));
                            // Send memory write command
                            assessSize(myName, soMwr_WrCmd, "soMwr_WrCmd", cDepth_FsmToMwr_WrCmd);
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
                            // Send Rx data notify to [APP]
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId,  fsm_Meta.meta.length,
                                                            fsm_Meta.ip4SrcAddr, fsm_Meta.tcpSrcPort,
                                                            fsm_Meta.tcpDstPort));
                            // Send keep command
                            soTsd_DropCmd.write(CMD_KEEP);
                            fsm_oooDebugState = 2;
                        }
                        //-- START   : No OOO but Rx segment arrived out-of-order
                        else if (!rxSar.ooo and (fsm_Meta.meta.seqNumb > rxSar.rcvd) and
                                                (fsm_freeSpace > (fsm_Meta.meta.seqNumb+fsm_Meta.meta.length-rxSar.oooHead)(TOE_WINDOW_BITS-1, 0))) {
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "OOO-START   : Rx segment is out-of-order.\n"); }
                            // Generate new oooHead and oooTail pointers
                            newOooHead = fsm_Meta.meta.seqNumb + fsm_Meta.meta.length;
                            newOooTail = fsm_Meta.meta.seqNumb;
                            // Update RxSar pointers
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, rxSar.rcvd, FLAG_OOO, newOooHead, newOooTail, QUERY_WR));
                            // Send memory write command
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
                            // Prevent [Ran] to send Rx data notify to [APP] by setting LENGTH=0 !!!
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId,  0,
                                                            fsm_Meta.ip4SrcAddr, fsm_Meta.tcpSrcPort,
                                                            fsm_Meta.tcpDstPort));
                            // Send keep command
                            soTsd_DropCmd.write(CMD_KEEP);
                            fsm_oooDebugState = 3;
                        }
                        //-- CONTINUE: OOO exists and Rx segment is in-order with respect to 'oooHead' pointer
                        else if (rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.oooHead) and
                                               (fsm_freeSpace > fsm_Meta.meta.length)) {
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "OOO-CONTINUE: Rx segment is in-order with respect to 'oooHead'.\n"); }
                            // Generate new oooHead pointer
                            newOooHead = fsm_Meta.meta.seqNumb + fsm_Meta.meta.length;
                            // Update RxSar pointers
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, rxSar.rcvd, FLAG_OOO, newOooHead, rxSar.oooTail, QUERY_WR));
                            // Send memory write command
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
                            // Prevent [Ran] to send Rx data notify to [APP] by setting LENGTH=0 !!!
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId,  0,
                                                            fsm_Meta.ip4SrcAddr, fsm_Meta.tcpSrcPort,
                                                            fsm_Meta.tcpDstPort));
                            // Send keep command
                            soTsd_DropCmd.write(CMD_KEEP);
                            fsm_oooDebugState = 4;
                        }
                        //-- CONTINUE: OOO exists, Rx segment is in-order with respect to 'rcvd' pointer but it does not fill the gap
                        else if (rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.rcvd) and
                                               ((fsm_Meta.meta.seqNumb + fsm_Meta.meta.length) < rxSar.oooTail)) {
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "OOO-CONTINUE: Rx segment is in-order with respect to 'rcvd' but it does not fill the gap.\n"); }
                            // Generate new received pointer
                            newRcvd = fsm_Meta.meta.seqNumb + fsm_Meta.meta.length;
                            // Update RxSar pointers
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, newRcvd, FLAG_OOO, rxSar.oooHead, rxSar.oooTail, QUERY_WR));
                            // Send memory write command
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
                            // Send Rx data notify to [APP]
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId, fsm_Meta.meta.length,
                                                            fsm_Meta.ip4SrcAddr, fsm_Meta.tcpSrcPort,
                                                            fsm_Meta.tcpDstPort));
                            // Send keep command
                            soTsd_DropCmd.write(CMD_KEEP);
                            fsm_oooDebugState = 5;
                        }
                        //-- END       : OOO exists, Rx segment is in-order with respect to 'rcvd' pointer and it fills the gap
                        else if (rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.rcvd) and
                                               ((fsm_Meta.meta.seqNumb + fsm_Meta.meta.length) == rxSar.oooTail)) {
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "OOO-END     : Rx segment is in-order with respect to 'rcvd' and it fills the gap.\n"); }
                            // Generate new received pointer
                            newRcvd = rxSar.oooHead;
                            // Update RxSar pointers
                            soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, newRcvd, FLAG_INO, rxSar.oooHead, rxSar.oooTail, QUERY_WR));
                            // Send memory write command
                            soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
                            // Send Rx data notify to [APP]
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId, (rxSar.oooHead - rxSar.rcvd),
                                                            fsm_Meta.ip4SrcAddr, fsm_Meta.tcpSrcPort,
                                                            fsm_Meta.tcpDstPort));
                            // Send keep command
                            soTsd_DropCmd.write(CMD_KEEP);
                            fsm_oooDebugState = 6;
                        }
                        //-- OOO-DROP  : Always drop segment in all other cases
                        else {
                            soTsd_DropCmd.write(CMD_DROP);
                            fsm_oooDropCounter++;
                            fsm_oooDebugState = 10;
                            if ( (fsm_Meta.meta.seqNumb < rxSar.rcvd)) {
                                printInfo(myName, "OOO-Dropping Rx segment (Frame is a retransmission because of a lost or delayed ACK). \n");
                                fsm_oooDebugState = 11;
                            }
                            else if ( (!rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.rcvd) and (fsm_freeSpace <  fsm_Meta.meta.length)) or
                                      (!rxSar.ooo and (fsm_Meta.meta.seqNumb >  rxSar.rcvd) and (fsm_freeSpace > (fsm_Meta.meta.seqNumb+fsm_Meta.meta.length-rxSar.rcvd)(TOE_WINDOW_BITS-1, 0))) ) {
                                printInfo(myName, "OOO-Dropping Rx segment (Not enough space left in the Rx ring buffer).\n");
                                fsm_oooDebugState = 12;
                            }
                            else if (fsm_Meta.meta.seqNumb+fsm_Meta.meta.length == rxSar.rcvd) {
                                printInfo(myName, "OOO-Dropping Rx segment (Segment is a duplicate).\n");
                                fsm_oooDebugState = 13;
                            }
                            else {
                                printFatal(myName, "OOO-Dropping Rx segment (Reason is unknown).\n");
                                fsm_oooDebugState = 14;
                            }
                        }
                    }

                   //-- Generate ACK Event -------------------------------------
# if FAST_RETRANSMIT
                    if (txSar.count == 3 && !txSar.fastRetransmitted) {  // [FIXME - Use a constant here]
                        soEVe_Event.write(event(RT, fsm_Meta.sessionId));
                    }
                    else if (fsm_meta.meta.length != 0) {
#else
                    if (fsm_Meta.meta.length != 0) {
#endif
                        if (!rxSar.ooo and (fsm_Meta.meta.seqNumb == rxSar.rcvd) and
                                           (fsm_freeSpace > fsm_Meta.meta.length)) {
                            // No OOO and Rx segment is in expected sequence order
                            soEVe_Event.write(Event(ACK_EVENT, fsm_Meta.sessionId));
                        }
                        else {
                            // TCP retransmission frame
                            soEVe_Event.write(Event(ACK_NODELAY_EVENT, fsm_Meta.sessionId));
                            fsm_oooDebugState += 100;
                        }
                    }

                    //-- Reset Retransmit Timer --------------------------------
                    if (fsm_Meta.meta.ackNumb == txSar.prevUnak) {
                        switch (tcpState) {
                        case SYN_RECEIVED:
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, ESTABLISHED, QUERY_WR));
                            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "Session[%d] - TCP State = ESTABISHED.\n", fsm_Meta.sessionId.to_uint()); }
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
                        soTsd_DropCmd.write(CMD_DROP);
                    }
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                }
            } // End of: ACK processing
            break;
        case 2:
            //--------------------------------------
            //-- SYN
            //--------------------------------------
            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "Entering 'SYN' processing.\n"); }
            if (fsm_fsmState == FSM_LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                if (tcpState == CLOSED or tcpState == SYN_SENT) {
                    // Initialize RxSar with received SeqNum
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+1,
                                                       QUERY_WR, QUERY_INIT));
                    // Initialize TxSar with received WindowSize
                    //  All other parameters are zero or false; they will be initialized by [TXe]
                    soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId, 0, fsm_Meta.meta.winSize,
                                                        0, 0, false)));
                    // Post a SYN_ACK event request
                    soEVe_Event.write(Event(SYN_ACK_EVENT, fsm_Meta.sessionId));
                    if (DEBUG_LEVEL & TRACE_FSM) printInfo(myName, "Requesting [TXe] to send a [SYN,ACK] for SessId %d.\n", fsm_Meta.sessionId.to_uint());
                    // Change TcpState to SYN_RECEIVED
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, SYN_RECEIVED, QUERY_WR));
                }
                else if (tcpState == SYN_RECEIVED) { // && mdh_meta.seqNumb+1 == rxSar.recvd) // Maybe Check for seq
                    // If it is the same SYN, we resent SYN-ACK, almost like quick RT, we could also wait for RT timer
                    if (fsm_Meta.meta.seqNumb+1 == rxSar.rcvd) {
                        // Retransmit SYN_ACK
                        ap_uint<3> rtCount = 1;
                        soEVe_Event.write(Event(SYN_ACK_EVENT, fsm_Meta.sessionId, rtCount));
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                    }
                    else { // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                        soEVe_Event.write(rstEvent(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+1)); //length == 0
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                    }
                }
                else { // Any synchronized state
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(Event(ACK_NODELAY_EVENT, fsm_Meta.sessionId));
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
            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "Entering 'SYN_ACK' processing.\n"); }
            if (fsm_fsmState == FSM_LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);
                TimerCmd timerCmd = (fsm_Meta.meta.ackNumb == txSar.prevUnak) ? STOP_TIMER : LOAD_TIMER;
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));
                if ( (tcpState == SYN_SENT) and (fsm_Meta.meta.ackNumb == txSar.prevUnak) ) { // && !mh_lup.created)
                    // Initialize RxSar with received SeqNum
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+1,
                                                       QUERY_WR, QUERY_INIT));
                    // Update TxSar with received AckNum and WindowSize
                    soTSt_TxSarQry.write(RXeTxSarQuery(fsm_Meta.sessionId,
                                                       fsm_Meta.meta.ackNumb,
                                                       fsm_Meta.meta.winSize,
                                                       txSar.cong_window, 0, false)); // [TODO - maybe include count check]
                    // Set ACK event
                    soEVe_Event.write(Event(ACK_NODELAY_EVENT, fsm_Meta.sessionId));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, ESTABLISHED, QUERY_WR));
                    // Signal [TAi] that the active connection was successfully established
                    soTAi_SessOpnSts.write(SessState(fsm_Meta.sessionId, ESTABLISHED));
                }
                else if (tcpState == SYN_SENT) { //TODO correct answer?
                    // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                    soEVe_Event.write(rstEvent(fsm_Meta.sessionId,
                                               fsm_Meta.meta.seqNumb+fsm_Meta.meta.length+1));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSED, QUERY_WR));
                }
                else {
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soEVe_Event.write(Event(ACK_NODELAY_EVENT, fsm_Meta.sessionId));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                }
            }
            break;
        case 5:
            //--------------------------------------
            //-- FIN (_ACK)
            //--------------------------------------
            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "Entering 'FIN_ACK' processing.\n"); }
            if (fsm_fsmState == FSM_LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);
                TimerCmd timerCmd = (fsm_Meta.meta.ackNumb == txSar.prevUnak) ? STOP_TIMER : LOAD_TIMER;
                soTIm_ReTxTimerCmd.write(RXeReTransTimerCmd(fsm_Meta.sessionId, timerCmd));
                // Check state and if FIN in order, Current out of order FINs are not accepted
                if ( (tcpState == ESTABLISHED or  tcpState == FIN_WAIT_1 or tcpState == FIN_WAIT_2) and (rxSar.rcvd == fsm_Meta.meta.seqNumb) ) {
                    soTSt_TxSarQry.write((RXeTxSarQuery(fsm_Meta.sessionId,
                                          fsm_Meta.meta.ackNumb, fsm_Meta.meta.winSize,
                                          txSar.cong_window, txSar.count,
                                          ~QUERY_FAST_RETRANSMIT))); //TODO include count check
                    // +1 for phantom byte, there might be data too
                    soRSt_RxSarQry.write(RXeRxSarQuery(fsm_Meta.sessionId, fsm_Meta.meta.seqNumb+fsm_Meta.meta.length+1,
                                         QUERY_WR)); // diff to ACK
                    // Clear the probe timer
                    soTIm_ClearProbeTimer.write(fsm_Meta.sessionId);
                    // Check if there is payload
                    if (fsm_Meta.meta.length != 0) {
                        // Build a DDR memory address for this segment
                        RxMemPtr memSegAddr = TOE_RX_MEMORY_BASE;
                        memSegAddr(29, 16) = fsm_Meta.sessionId(13, 0);
                        memSegAddr(15,  0) = fsm_Meta.meta.seqNumb(15, 0);
#if !(RX_DDR_BYPASS)
                        soMwr_WrCmd.write(DmCmd(memSegAddr, fsm_Meta.meta.length));
#endif
                        // Tell Application new data is available and connection got closed
                        soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId,  fsm_Meta.meta.length, fsm_Meta.ip4SrcAddr,
                                                     fsm_Meta.tcpSrcPort, fsm_Meta.tcpDstPort,  CLOSED));
                        soTsd_DropCmd.write(CMD_KEEP);
                    }
                    else if (tcpState == ESTABLISHED) {
                        // Tell Application connection got closed
                        soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId,  0,                   fsm_Meta.ip4SrcAddr,
                                                     fsm_Meta.tcpSrcPort, fsm_Meta.tcpDstPort,  CLOSED));
                    }
                    // Update state
                    if (tcpState == ESTABLISHED) {
                        soEVe_Event.write(Event(FIN_EVENT, fsm_Meta.sessionId));
                        soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, LAST_ACK, QUERY_WR));
                    }
                    else { //FIN_WAIT_1 || FIN_WAIT_2
                        if (fsm_Meta.meta.ackNumb == txSar.prevUnak) {
                            // Check if final FIN is ACK'd -> LAST_ACK
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, TIME_WAIT, QUERY_WR));
                            soTIm_CloseTimer.write(fsm_Meta.sessionId);
                        }
                        else {
                            soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, CLOSING, QUERY_WR));
                        }
                        soEVe_Event.write(Event(ACK_EVENT, fsm_Meta.sessionId));
                    }
                }
                else { // NOT (ESTABLISHED || FIN_WAIT_1 || FIN_WAIT_2)
                    soEVe_Event.write(Event(ACK_EVENT, fsm_Meta.sessionId));
                    soSTt_StateQry.write(StateQuery(fsm_Meta.sessionId, tcpState, QUERY_WR));
                    // If there is payload we need to drop it
                    if (fsm_Meta.meta.length != 0) {
                        soTsd_DropCmd.write(CMD_DROP);
                    }
                }
            }
            break;
        default: //TODO MAYBE load everything all the time
            if (DEBUG_LEVEL & TRACE_FSM) { printInfo(myName, "Received segment. CtrlBits=0x%X\n", control_bits.to_uchar()); }
            // stateTable is locked, make sure it is released in at the end
            // If there is an ACK we read txSar
            // We always read rxSar
            if (fsm_fsmState == FSM_LOAD) {
                siSTt_StateRep.read(tcpState);
                siRSt_RxSarRep.read(rxSar); //TODO not sure nb works
                siTSt_TxSarRep.read_nb(txSar);
            }
            if (fsm_fsmState == FSM_LOAD) {
                // Handle if RST
                if (fsm_Meta.meta.rst) {
                    if (tcpState == SYN_SENT) {
                        // [TODO this would be a RST,ACK i think]
                        // Check if matching SYN
                        if (fsm_Meta.meta.ackNumb == txSar.prevUnak) {
                            // The connection culd not be established
                            soTAi_SessOpnSts.write(SessState(fsm_Meta.sessionId, CLOSED));
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
                            soRan_RxNotif.write(TcpAppNotif(fsm_Meta.sessionId, 0, fsm_Meta.ip4SrcAddr,
                                                         fsm_Meta.tcpSrcPort,   fsm_Meta.tcpDstPort, CLOSED)); // RESET-CLOSED
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
        if (DEBUG_LEVEL & TRACE_FSM) {
            printInfo(myName, "Flags - [0x%X] \n", control_bits.to_uint());
        }
        break;
    } // End of: switch state

    //-- ALWAYS -------------------------------------------
    if (!soMMIO_OooDropCnt.full()) {
        soMMIO_OooDropCnt.write(fsm_oooDropCounter);
    }
    else {
        printFatal(myName, "Cannot write soMMIO_OooDropCnt stream...");
    }
    if (!soDBG_TcpIpRxByteCnt.full()) {
        soDBG_TcpIpRxByteCnt.write(fsm_rxByteCounter);
    }
    else {
        printFatal(myName, "Cannot write soDBG_TcpIpRxByteCnt stream...");
    }
    if (!soDBG_OooDebug.full()) {
        soDBG_OooDebug.write(fsm_oooDebugState);
     }
     else {
         printFatal(myName, "Cannot write soDBG_OooDebug stream...");
     }

} // End of: pFiniteStateMachine(

/*******************************************************************************
 * @brief Event Multiplexer (Evm)
 *
 * @param[in]  siMdh_Event  Event from MetaDataHandler (Mdh).
 * @param[in]  siFsm_Event  Event from FiniteStateMachine (Fsm).
 * @param[out] soEVe_Event  Event to EVentEngine (EVe).
 *
 * @details
 *  This process takes two events as inputs and muxes them on a single output.
 *   Note that the first input has priority over the second one.
 *
 *******************************************************************************/
void pEventMultiplexer(
        stream<ExtendedEvent>    &siMdh_Event,
        stream<Event>            &siFsm_Event,
        stream<ExtendedEvent>    &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE

    const char *myName = concat3(THIS_NAME, "/", "Evm");

    if (!siMdh_Event.empty()) {
        soEVe_Event.write(siMdh_Event.read());
    }
    else if (!siFsm_Event.empty()) {
        soEVe_Event.write(siFsm_Event.read());
    }
}


/*****************************************************************************
 * @brief Receive Engine (RXe)
 *
 * @param[in]  siIPRX_Data         IP4 data stream form [IPRX].
 * @param[out] soSLc_SessLkReq     Session lookup request to SessionLookupController (SLc).
 * @param[in]  siSLc_SessLkRep     Session lookup reply from [SLc].
 * @param[out] soSTt_StateQry      State query to StateTable (STt).
 * @param[in]  siSTt_StateRep      State reply from [STt].
 * @param[out] soPRt_PortStateReq  Port state request to PortTable (PRt).
 * @param[in]  siPRt_PortStateRep  Port state reply from [PRt].
 * @param[out] soRSt_RxSarQry      Query to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep      Reply from [RSt].
 * @param[out] soTSt_TxSarQry      Query to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRep      Reply from [TSt].
 * @param[out] soTIm_ReTxTimerCmd  Command for a retransmit timer to Timers (TIm).
 * @param[out] soTIm_ClearProbeTimer Clear the probe timer command to [TIm]..
 * @param[out] soTIm_CloseTimer    Close session timer command to [TIm].
 * @param[out] soEVe_SetEvent      Event forward to EventEngine (EVe).
 * @param[out] soTAi_SessOpnSts    Open status of the session to TxAppInterface (TAi).
 * @param[out] soRAi_RxNotif       Rx data notification to RxAppInterface (RAi).
 * @param[out] soMEM_WrCmd         Memory write command to MemorySubSystem (MEM).
 * @param[out] soMEM_WrData        Memory data write stream to [MEM].
 * @param[in]  siMEM_WrSts         Memory write status from [MEM].
 * @param[out] soMMIO_RxMemWrErr   Reports an Rx memory write error.
 * @param[out] soMMIO_CrcDropCnt   The value of the CRC drop counter.
 * @param[out] soMMIO_SessDropCnt  The value of the session drop counter.
 * @param[out] soMMIO_OooDropCnt   The value of the out-of-order drop counter.
 *
 * @details
 *  The RxEngine (RXe) processes the TCP/IP packets received from the IpRxHandler
 *   (IPRX). When a new TCP/IP packet enters the RXe, its TCP checksum is tested,
 *   the TCP header is parsed and some more checks are done. Next, the TCP segment
 *   is evaluated by the TCP state machine which triggers events and updates the
 *   data structures according to the type of received packet. Finally, if the
 *   packet contains a valid data payload, the TCP segment is stored in external
 *   DDR4 memory and the application is notified about the arrival of new data.
 *******************************************************************************/
void rx_engine(
        // IP Rx Interface
        stream<AxisIp4>                 &siIPRX_Data,
        //-- Session Lookup Controller Interface
        stream<SessionLookupQuery>      &soSLc_SessLkReq,
        stream<SessionLookupReply>      &siSLc_SessLkRep,
        //-- State Table Interface
        stream<StateQuery>              &soSTt_StateQry,
        stream<TcpState>                &siSTt_StateRep,
        //-- Port Table Interface
        stream<TcpPort>                 &soPRt_PortStateReq,
        stream<RepBit>                  &siPRt_PortStateRep,
        //-- Rx SAR Table Interface
        stream<RXeRxSarQuery>           &soRSt_RxSarQry,
        stream<RxSarReply>              &siRSt_RxSarRep,
        //-- Tx SAR Table Interface
        stream<RXeTxSarQuery>           &soTSt_TxSarQry,
        stream<RXeTxSarReply>           &siTSt_TxSarRep,
            //-- Timers Interface
        stream<RXeReTransTimerCmd>      &soTIm_ReTxTimerCmd,
        stream<SessionId>               &soTIm_ClearProbeTimer,
        stream<SessionId>               &soTIm_CloseTimer,
        //-- Event Engine Interface
        stream<ExtendedEvent>           &soEVe_SetEvent,
        //-- Tx Application Interface
        stream<SessState>               &soTAi_SessOpnSts,
        //-- Rx Application Interface
        stream<TcpAppNotif>             &soRAi_RxNotif,
        //-- MEM / Rx Write Path Interface
        stream<DmCmd>                   &soMEM_WrCmd,
        stream<AxisApp>                 &soMEM_WrData,
        stream<DmSts>                   &siMEM_WrSts,
        //--MMIO Interfaces
        stream<StsBit>                  &soMMIO_RxMemWrErr,
        stream<ap_uint<8> >             &soMMIO_CrcDropCnt,
        stream<ap_uint<8> >             &soMMIO_SessDropCnt,
        stream<ap_uint<8> >             &soMMIO_OooDropCnt,
        //-- DEBUG Interfaces
        stream<RxBufPtr>                &soDBG_RxFreeSpace,
        stream<ap_uint<32> >            &soDBG_TcpIpRxByteCnt,
        stream<ap_uint<8> >             &soDBG_OooDebug)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tcp Length Extract (Tle) ---------------------------------------------
    static stream<AxisRaw>          ssTleToIph_Data         ("ssTleToIph_Data");
    #pragma HLS stream     variable=ssTleToIph_Data         depth=8
    #pragma HLS DATA_PACK  variable=ssTleToIph_Data

    static stream<TcpSegLen>        ssTleToIph_TcpSegLen    ("ssTleToIph_TcpSegLen");
    #pragma HLS stream     variable=ssTleToIph_TcpSegLen    depth=2

    //-- Insert Pseudo Header (Iph) -------------------------------------------
    static stream<AxisPsd4>         ssIphToCsa_PseudoPkt    ("ssIphToCsa_PseudoPkt");
    #pragma    HLS stream  variable=ssIphToCsa_PseudoPkt    depth=8
    #pragma HLS DATA_PACK  variable=ssIphToCsa_PseudoPkt

    //-- CheckSum Accumulator (Csa) -------------------------------------------
    static stream<AxisApp>          ssCsaToTid_Data         ("ssCsaToTid_Data");
    #pragma HLS stream     variable=ssCsaToTid_Data         depth=256 //critical, tcp checksum computation
    #pragma HLS DATA_PACK  variable=ssCsaToTid_Data

    static stream<ValBit>           ssCsaToTid_DataValid    ("ssCsaToTid_DataValid");
    #pragma HLS stream     variable=ssCsaToTid_DataValid    depth=2

    static stream<RXeMeta>          ssCsaToMdh_Meta         ("ssCsaToMdh_Meta");
    #pragma HLS stream     variable=ssCsaToMdh_Meta         depth=2
    #pragma HLS DATA_PACK  variable=ssCsaToMdh_Meta

    static stream<SocketPair>       ssCsaToMdh_SockPair     ("ssCsaToMdh_SockPair");
    #pragma HLS stream     variable=ssCsaToMdh_SockPair     depth=2
    #pragma HLS DATA_PACK  variable=ssCsaToMdh_SockPair

    //-- Tcp Invalid dropper (Tid) --------------------------------------------
    static stream<AxisApp>          ssTidToTsd_Data         ("ssTidToTsd_Data");
    #pragma HLS stream     variable=ssTidToTsd_Data         depth=8
    #pragma HLS DATA_PACK  variable=ssTidToTsd_Data

    //-- Tcp Tcp Segment Dropper (Tsd) -----------------------------------------
    static stream<AxisApp>          ssTsdToMwr_Data         ("ssTsdToMwr_Data");
    #pragma HLS stream     variable=ssTsdToMwr_Data         depth=16
    #pragma HLS DATA_PACK  variable=ssTsdToMwr_Data

    //-- MetaData Handler (Mdh) -----------------------------------------------
    static stream<ExtendedEvent>    ssMdhToEvm_Event        ("ssMdhToEvm_Event");
    #pragma HLS stream     variable=ssMdhToEvm_Event        depth=cDepth_MdhToEvm_Event
    #pragma HLS DATA_PACK  variable=ssMdhToEvm_Event

    static stream<CmdBit>           ssMdhToTsd_DropCmd      ("ssMdhToTsd_DropCmd");
    #pragma HLS stream     variable=ssMdhToTsd_DropCmd      depth=cDepth_MdhToTsd_DropCmd

    static stream<RXeFsmMeta>       ssMdhToFsm_Meta         ("ssMdhToFsm_Meta");
    #pragma HLS stream     variable=ssMdhToFsm_Meta         depth=2
    #pragma HLS DATA_PACK  variable=ssMdhToFsm_Meta

    //-- Finite State Machine (Fsm) -------------------------------------------
    static stream<CmdBit>           ssFsmToTsd_DropCmd      ("ssFsmToTsd_DropCmd");
    #pragma HLS stream     variable=ssFsmToTsd_DropCmd      depth=cDepth_FsmToTsd_DropCmd

    static stream<TcpAppNotif>      ssFsmToRan_Notif        ("ssFsmToRan_Notif");
    #pragma HLS stream     variable=ssFsmToRan_Notif        depth=cDepth_FsmToRan_Notif
    #pragma HLS DATA_PACK  variable=ssFsmToRan_Notif

    static stream<Event>            ssFsmToEvm_Event        ("ssFsmToEvm_Event");
    #pragma HLS stream     variable=ssFsmToEvm_Event        depth=cDepth_FsmToEve_Event
    #pragma HLS DATA_PACK  variable=ssFsmToEvm_Event

    static stream<DmCmd>            ssFsmToMwr_WrCmd        ("ssFsmToMwr_WrCmd");
    #pragma HLS stream     variable=ssFsmToMwr_WrCmd        depth=cDepth_FsmToMwr_WrCmd
    #pragma HLS DATA_PACK  variable=ssFsmToMwr_WrCmd

    //-- Memory Writer (Mwr) --------------------------------------------------
    static stream<FlagBool>         ssMwrToRan_SplitSeg     ("ssMwrToRan_SplitSeg");
    #pragma HLS stream     variable=ssMwrToRan_SplitSeg     depth=cDepth_MwrToRan_SplitSeg

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTcpLengthExtractor(
            siIPRX_Data,
            ssTleToIph_Data,
            ssTleToIph_TcpSegLen);


    pInsertPseudoHeader(
            ssTleToIph_Data,
            ssTleToIph_TcpSegLen,
            ssIphToCsa_PseudoPkt);

    pCheckSumAccumulator(
            ssIphToCsa_PseudoPkt,
            ssCsaToTid_Data,
            ssCsaToTid_DataValid,
            ssCsaToMdh_Meta,
            ssCsaToMdh_SockPair,
            soPRt_PortStateReq);

    pTcpInvalidDropper(
            ssCsaToTid_Data,
            ssCsaToTid_DataValid,
            ssTidToTsd_Data,
            soMMIO_CrcDropCnt);

    pMetaDataHandler(
            ssCsaToMdh_Meta,
            ssCsaToMdh_SockPair,
            soSLc_SessLkReq,
            siSLc_SessLkRep,
            siPRt_PortStateRep,
            ssMdhToEvm_Event,
            ssMdhToTsd_DropCmd,
            ssMdhToFsm_Meta,
            soMMIO_SessDropCnt);

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
            ssFsmToRan_Notif,
            soMMIO_OooDropCnt,
            soDBG_RxFreeSpace,
            soDBG_TcpIpRxByteCnt,
            soDBG_OooDebug);

    pTcpSegmentDropper(
            ssTidToTsd_Data,
            ssMdhToTsd_DropCmd,
            ssFsmToTsd_DropCmd,
            ssTsdToMwr_Data);

    pRxMemoryWriter(
            ssTsdToMwr_Data,
            ssFsmToMwr_WrCmd,
            soMEM_WrCmd,
            soMEM_WrData,
            ssMwrToRan_SplitSeg);

    pRxAppNotifier(
            siMEM_WrSts,
            ssFsmToRan_Notif,
            soRAi_RxNotif,
            ssMwrToRan_SplitSeg,
            soMMIO_RxMemWrErr);

    pEventMultiplexer(
            ssMdhToEvm_Event,
            ssFsmToEvm_Event,
            soEVe_SetEvent);

}

/*! \} */
