/************************************************
Copyright (c) 2016-2019, IBM Research.
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

/*****************************************************************************
 * @file       : uoe.cpp
 * @brief      : UDP Offload Engine (UOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "uoe.hpp"

using namespace hls;

#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPd | TRACE_IBUF)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "UOE"

#define TRACE_OFF  0x0000
#define TRACE_IHA  1 << 1
#define TRACE_IHS  1 << 2
#define TRACE_RPH  1 << 3
#define TRACE_TAI  1 << 4
#define TRACE_TDH  1 << 5
#define TRACE_UCC  1 << 6
#define TRACE_UHA  1 << 7
#define TRACE_UPT  1 << 8
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*** RXe PROCESSES ************************************************************/

/******************************************************************************
 * IPv4 Header Stripper (Ihs)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[out] soUcc_UdpDgrm, UDP datagram stream to UdpChecksumChecker (Ucc).
 * @param[out] soUcc_PsdHdrSum, Sum of the pseudo header information to [Ucc].
 * @param[out] soRph_Ip4Hdr,  The header part of the IPv4 packet as a stream to [Rph].
 *
 * @details
 *  This process extracts the UDP pseudo header and the IP header from the
 *  incoming IPv4 packet. The IP header is forwarded to RxPacketHandler (Rph)
 *  for further processing while the UDP datagram and the UDP pseudo-header
 *  are forwarded to the UDP checksum checker.
 *
 *****************************************************************************/
void pIpHeaderStripper(
          stream<AxisIp4>      &siIPRX_Data,
          stream<AxisUdp>      &soUcc_UdpDgrm,
          stream<UdpCsum>      &soUcc_PsdHdrSum,
          stream<AxisIp4>      &soRph_Ip4Hdr)
  {
      //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
      #pragma HLS INLINE off
      #pragma HLS pipeline II=1 enable_flush

      const char *myName  = concat3(THIS_NAME, "/Rxe/", "Ihs");

      //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
      static enum FsmStates {FSM_IHS_IPW0=0, FSM_IHS_IPW1, FSM_IHS_IPW2, FSM_IHS_OPT,
                             FSM_IHS_UDP_HEADER, FSM_IHS_UDP_HEADER_ALIGNED,
                             FSM_IHS_FORWARD,    FSM_IHS_FORWARD_ALIGNED,
                             FSM_IHS_RESIDUE,    FSM_IHS_DROP } ihs_fsmState=FSM_IHS_IPW0;
      #pragma HLS RESET                            variable=ihs_fsmState

      //-- STATIC DATAFLOW VARIABLES --------------------------------------------
      static ap_uint<4>    ihs_bitCount;
      static ap_uint<3>    ihs_ipHdrCnt;
      static AxisIp4       ihs_prevWord;
      static Ip4HdrLen     ihs_ip4HdrLen;
      static ap_uint<17>   ihs_psdHdrSum;

      //-- DYNAMIC VARIABLES ----------------------------------------------------
      AxisIp4     currIp4Word;

      switch(ihs_fsmState) {
      case FSM_IHS_IPW0:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 1st AXI-WORD (Frag|Flags|Id|TotLen|ToS|Ver|IHL) ---------
              siIPRX_Data.read(currIp4Word);
              ihs_ip4HdrLen = currIp4Word.getIp4HdrLen();
              if (ihs_ip4HdrLen < 5) {
                  printWarn(myName, "FSM_IHS_IPW0 - Received an IPv4 packet with invalid IHL. This packet will be dropped.\n");
                  ihs_fsmState  = FSM_IHS_DROP;
              }
              else {
                  soRph_Ip4Hdr.write(currIp4Word);
                  if (DEBUG_LEVEL & TRACE_IHS) {
                      printInfo(myName, "FSM_IHS_IPW0 - Received a new IPv4 packet (IHL=%d|TotLen=%d)\n",
                                ihs_ip4HdrLen.to_uint(), currIp4Word.getIp4TotalLen().to_ushort());
                  }
                  ihs_ip4HdrLen -= 2;
                  ihs_fsmState  = FSM_IHS_IPW1;
              }
          }
          break;
      case FSM_IHS_IPW1:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 2nd AXI_WORD (SA|HdrCsum|Prot|TTL)
              siIPRX_Data.read(currIp4Word);
              soRph_Ip4Hdr.write(currIp4Word);
              //-- Csum accumulate (SA+Prot)
              ihs_psdHdrSum  = (0x00, currIp4Word.getIp4Prot());
              ihs_psdHdrSum += currIp4Word.getIp4SrcAddr().range(31,16);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_psdHdrSum += currIp4Word.getIp4SrcAddr().range(15, 0);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_ip4HdrLen -= 2;
              ihs_fsmState = FSM_IHS_IPW2;
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_IPW1 - \n"); }
          break;
      case FSM_IHS_IPW2:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 3rd AXI-WORD (DP|SP|DA) or (Opt|DA)
              siIPRX_Data.read(currIp4Word);
              //-- Csum accumulate (DA)
              ihs_psdHdrSum += currIp4Word.getIp4DstAddr().range(31,16);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_psdHdrSum += currIp4Word.getIp4DstAddr().range(15, 0);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              if (ihs_ip4HdrLen == 1) {
                  // This a typical IPv4 header with a length of 20 bytes (5*4).
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_Ip4DstAddr(), 0x0F, TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER;
              }
              else if (ihs_ip4HdrLen == 2 ) {
                  printWarn(myName, "This IPv4 packet contains 1 option word! FYI, IPV4 options are not supported.\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_TData(), currIp4Word.getLE_TKeep(), TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
              }
              else {  // ihs_ip4HdrLen > 2
                  printWarn(myName, "This IPv4 packet contains 2+ option words! FYI, IPV4 options are not supported.\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_TData(), currIp4Word.getLE_TKeep(), 0));
                  ihs_ip4HdrLen -= 2;
                  ihs_fsmState = FSM_IHS_OPT;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_IPW2 - \n"); }
          break;
      case FSM_IHS_OPT:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              printWarn(myName, "This IPv4 packet contains options! FYI, IPV4 options are not supported and will be dropped.\n");
              //-- READ more Options (OPT|Opt) and/or Data (Data|Opt)
              siIPRX_Data.read(currIp4Word);
              if (ihs_ip4HdrLen == 1) {
                  printWarn(myName, "This IPv4 packet contains 3 option words!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_TDataHi(), 0x0F, TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER;
              }
              else if (ihs_ip4HdrLen == 2 ) {
                  printWarn(myName, "This IPv4 packet contains 4 option words!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_TData(), currIp4Word.getLE_TKeep(), TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
              }
              else {  // ihs_ip4HdrLen > 2
                  printWarn(myName, "This IPv4 packet contains 4+ option words!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_TData(), currIp4Word.getLE_TKeep(), 0));
                  ihs_ip4HdrLen -= 2;
                  ihs_fsmState = FSM_IHS_OPT;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_OPT - \n"); }
          break;
      case FSM_IHS_UDP_HEADER:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- Read end of un-aligned UDP header (Data|Csum|Len) ----------
              siIPRX_Data.read(currIp4Word);
              //-- Csum accumulate (UdpLen)
              ihs_psdHdrSum += currIp4Word.getUdpLen();
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              //-- Forward UDP PseudoHeaderCsum to [Ucc]
              soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
              //-- Forward the UDP Header (Csum|Len|DP|SP)
              AxisUdp sendWord(0, 0xFF, 0);
              sendWord.setTDataHi(ihs_prevWord.getTDataLo());
              sendWord.setTDataLo(currIp4Word.getTDataHi());
              if (currIp4Word.getTLast()) {
                  if (currIp4Word.getTKeep() == 0xF0) {
                      printWarn(myName, "Received a UDP datagram of length = 0!\n");
                      sendWord.setTKeep(0xFF);
                      ihs_fsmState = FSM_IHS_IPW0;
                  }
                  else {
                      sendWord.setTKeepHi(ihs_prevWord.getTKeepLo());
                      sendWord.setTKeepLo(currIp4Word.getTKeepHi());
                      ihs_fsmState = FSM_IHS_RESIDUE;
                  }
              }
              else {
                  ihs_fsmState = FSM_IHS_FORWARD;
              }
              soUcc_UdpDgrm.write(sendWord);
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER - \n"); }
          break;
      case FSM_IHS_UDP_HEADER_ALIGNED:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- READ Aligned UDP header (Csum|Len|DP|SP) --------
              siIPRX_Data.read(currIp4Word);
              //-- Cast incoming word into an AxisUdp word
              AxisUdp  currUdpWord(currIp4Word);
              //-- Csum accumulate (UdpLen)
              ihs_psdHdrSum += currUdpWord.getUdpLen();
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              //-- Forward UDP PseudoHeaderCsum to [Ucc]
              soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
              //-- Forward the UDP Header (Csum|Len|DP|SP)
              soUcc_UdpDgrm.write(currUdpWord);
              if (currUdpWord.getTLast()) {
                  printWarn(myName, "Received a UDP datagram of length = 0!\n");
                  ihs_fsmState = FSM_IHS_IPW0;
              }
              else {
                  ihs_fsmState = FSM_IHS_FORWARD_ALIGNED;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER_ALIGNED - \n"); }
          break;
      case FSM_IHS_FORWARD:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- READ n-th AXI-WORD (Data) ----------------
              siIPRX_Data.read(currIp4Word);
              //-- Forward 1/2 of previous word and 1/2 of current word)
              AxisUdp sendWord(0,0,0);
              sendWord.setTDataHi(ihs_prevWord.getTDataLo());
              sendWord.setTKeepHi(ihs_prevWord.getTKeepLo());
              sendWord.setTDataLo(currIp4Word.getTDataHi());
              sendWord.setTKeepLo(currIp4Word.getTKeepHi());
              if (currIp4Word.getTLast()) {
                   if (currIp4Word.getTKeep() <= 0xF0) {
                      sendWord.setTLast(TLAST);
                      ihs_fsmState = FSM_IHS_IPW0;
                  }
                  else {
                      sendWord.setTLast(0);
                      ihs_fsmState = FSM_IHS_RESIDUE;
                  }
              }
              else {
                  sendWord.setTLast(0);
                  ihs_fsmState = FSM_IHS_FORWARD;
              }
              soUcc_UdpDgrm.write(sendWord);
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_FORWARD - \n"); }
          break;
      case FSM_IHS_FORWARD_ALIGNED:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- READ UDP ALIGNED AXI-WORD --------------
              siIPRX_Data.read(currIp4Word);
              //-- Cast incoming word into an AxisUdp word
              AxisUdp  currUdpWord(currIp4Word);
              soUcc_UdpDgrm.write(currUdpWord);
              if (currUdpWord.getTLast()) {
                  ihs_fsmState = FSM_IHS_IPW0;
              }
              else {
                  ihs_fsmState = FSM_IHS_FORWARD_ALIGNED;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_FORWARD_ALIGNED - \n"); }
          break;
      case FSM_IHS_RESIDUE:
          if (!soUcc_UdpDgrm.full()) {
              //-- Forward the very last bytes of the current word
              AxisUdp sendWord(0,0,0);
              sendWord.setTDataHi(ihs_prevWord.getTDataLo());
              sendWord.setTKeepHi(ihs_prevWord.getTKeepLo());
              sendWord.setTLast(TLAST);
             soUcc_UdpDgrm.write(sendWord);
             ihs_fsmState = FSM_IHS_IPW0;
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_RESIDUE - \n"); }
          break;
      case FSM_IHS_DROP:
          if (!siIPRX_Data.empty()) {
              //-- READ and DRAIN all AXI-WORDS -------------
              siIPRX_Data.read(currIp4Word);
              if (currIp4Word.getTLast()) {
                  ihs_fsmState = FSM_IHS_IPW0;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_DROP - \n"); }
          break;
      } // End-of: switch

      ihs_prevWord = currIp4Word;

  } // End-of: pIpHeaderStripper

/******************************************************************************
 * UDP Checksum Checker (Ucc)
 *
 * @param[in]  siUcc_UdpDgrm, UDP datagram stream from IpHeaderStripper (Ihs).
 * @param[in]  siUcc_PsdHdrSum, Pseudo header sum (SA+DA+Prot+Len) from [Ihs].
 * @param[out] soRph_UdpDgrm, UDP datagram stream to RxPacketHandler (Rph).
 * @param[out] soRph_CsumVal, Checksum valid information to [Rph].
 *
 * @details
 *  This process accumulates the checksum over the UDP header and the UDP data.
 *  The computed checksum is compared with the embedded checksum when 'TLAST'
 *  is reached, and the result is forwarded to the RxPacketHandler (Rph).
 *
 *****************************************************************************/
void pUdpChecksumChecker(
        stream<AxisUdp>      &siIhs_UdpDgrm,
        stream<UdpCsum>      &siIhs_PsdHdrSum,
        stream<AxisUdp>      &soRph_UdpDgrm,
        stream<ValBool>      &soRph_CsumVal)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/RXe/", "Ucc");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_UCC_IDLE=0, FSM_UCC_ACCUMULATE,
                            FSM_UCC_CHK0,   FSM_UCC_CHK1,
                            FSM_UCC_STREAM } ucc_fsmState=FSM_UCC_IDLE;
    #pragma HLS RESET               variable=ucc_fsmState
    static ap_uint<10>                       ucc_wordCount=0;
    #pragma HLS RESET               variable=ucc_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<17>  ucc_csum[4];
    static UdpCsum      ucc_psdHdrCsum;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisUdp     currWord;

    switch (ucc_fsmState) {
    case FSM_UCC_IDLE:
        if (!siIhs_UdpDgrm.empty() && !siIhs_PsdHdrSum.empty()) {
            //-- READ 1st DTGM_WORD (CSUM|LEN|DP|SP)
            siIhs_UdpDgrm.read(currWord);
            //-- READ the checksum of the pseudo header
            siIhs_PsdHdrSum.read(ucc_psdHdrCsum);
            // Always forward datagram to [Rph]
            soRph_UdpDgrm.write(currWord);
            if (currWord.getUdpCsum() == 0x0000) {
            // An all zero transmitted checksum  value means that the
            // transmitter generated no checksum.
            soRph_CsumVal.write(true);
            ucc_fsmState = FSM_UCC_STREAM;
        }
        else {
            // Accumulate e the UDP header
            ucc_csum[0]  = 0x00000 | currWord.getUdpSrcPort();
            ucc_csum[0] += ucc_psdHdrCsum;
            ucc_csum[0]  = (ucc_csum[0] & 0xFFFF) + (ucc_csum[0] >> 16);
            ucc_csum[1] = 0x00000 | currWord.getUdpDstPort();
            ucc_csum[2] = 0x00000 | currWord.getUdpLen();
            ucc_csum[3] = 0x00000 | currWord.getUdpCsum();
            if (currWord.getUdpLen() == 8) {
                // Payload is empty
                ucc_fsmState = FSM_UCC_CHK0;
            }
            else {
                ucc_fsmState = FSM_UCC_ACCUMULATE;
            }
        }
        if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_IDLE       - ", currWord); }
        }
        break;
    case FSM_UCC_STREAM:
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currWord);
            soRph_UdpDgrm.write(currWord);
            if (currWord.getTLast()) {
                ucc_fsmState = FSM_UCC_IDLE;
            }
            if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_STREAM     - ", currWord); }
        }
        break;
    case FSM_UCC_ACCUMULATE:
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currWord);
            // Always set the disabled bytes to zero
            LE_tData cleanChunk = 0;
            if (currWord.getLE_TKeep() & 0x01)
                cleanChunk.range( 7, 0) = (currWord.getLE_TData()).range( 7, 0);
            if (currWord.getLE_TKeep() & 0x02)
                cleanChunk.range(15, 8) = (currWord.getLE_TData()).range(15, 8);
            if (currWord.getLE_TKeep() & 0x04)
                cleanChunk.range(23,16) = (currWord.getLE_TData()).range(23,16);
            if (currWord.getLE_TKeep() & 0x08)
                cleanChunk.range(31,24) = (currWord.getLE_TData()).range(31,24);
            if (currWord.getLE_TKeep() & 0x10)
                cleanChunk.range(39,32) = (currWord.getLE_TData()).range(39,32);
            if (currWord.getLE_TKeep() & 0x20)
                cleanChunk.range(47,40) = (currWord.getLE_TData()).range(47,40);
            if (currWord.getLE_TKeep() & 0x40)
                cleanChunk.range(55,48) = (currWord.getLE_TData()).range(55,48);
            if (currWord.getLE_TKeep() & 0x80)
                cleanChunk.range(63,56) = (currWord.getLE_TData()).range(63,56);
            soRph_UdpDgrm.write(AxisUdp(cleanChunk, currWord.getLE_TKeep(), currWord.getLE_TLast()));

            ucc_csum[0] += byteSwap16(cleanChunk.range(63, 48));
            ucc_csum[0]  = (ucc_csum[0] & 0xFFFF) + (ucc_csum[0] >> 16);
            ucc_csum[1] += byteSwap16(cleanChunk.range(47, 32));
            ucc_csum[1]  = (ucc_csum[1] & 0xFFFF) + (ucc_csum[1] >> 16);
            ucc_csum[2] += byteSwap16(cleanChunk.range(31, 16));
            ucc_csum[2]  = (ucc_csum[2] & 0xFFFF) + (ucc_csum[2] >> 16);
            ucc_csum[3] += byteSwap16(cleanChunk.range(15,  0));
            ucc_csum[3]  = (ucc_csum[3] & 0xFFFF) + (ucc_csum[3] >> 16);

            if (currWord.getTLast()) {
              ucc_fsmState = FSM_UCC_CHK0;
          }
            if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_ACCUMULATE - ", currWord); }
        }
        break;
    case FSM_UCC_CHK0:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_CHK0 - \n"); }
        ucc_csum[0] += ucc_csum[2];
        ucc_csum[0]  = (ucc_csum[0] & 0xFFFF) + (ucc_csum[0] >> 16);
        ucc_csum[1] += ucc_csum[3];
        ucc_csum[1]  = (ucc_csum[1] & 0xFFFF) + (ucc_csum[1] >> 16);
        ucc_fsmState = FSM_UCC_CHK1;
        break;
    case FSM_UCC_CHK1:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_CHK1 - \n"); }
        ucc_csum[0] += ucc_csum[1];
        ucc_csum[0]  = (ucc_csum[0] & 0xFFFF) + (ucc_csum[0] >> 16);
        UdpCsum csumChk = ~(ucc_csum[0](15, 0));
        if (csumChk == 0) {
		  // The checksum is correct. UDP datagram is valid.
		  soRph_CsumVal.write(true);
	  }
        else {
            soRph_CsumVal.write(false);
            if (DEBUG_LEVEL & TRACE_UCC) {
                printWarn(myName, "The current UDP datagram will be dropped because:\n");
                printWarn(myName, "  csum = 0x%4.4X instead of 0x0000\n", csumChk.to_ushort());
            }
        }
        ucc_fsmState = FSM_UCC_IDLE;
        break;
    } // End-of: switch

} // End-of: pUdpChecksumChecker

/*****************************************************************************
 * Rx Packet Handler (Rph)
 *
 * @param[in]  siIhs_UdpDgrm, UDP datagram stream from IpHeaderStripper (Ihs).
 * @param[in]  siIhs_Ip4Hdr,  The header part of the IPv4 packet from [Ihs].
 * @param[in]  siUcc_CsumVal, Checksum valid information from UdpChecksumChecker (Ucc).
 * @param[out] soUpt_PortStateReq, Request for the state of port to UdpPortTable (Upt).
 * @param[in]  siUpt_PortStateRep, Port state reply from [Upt].
 * @param[out] soUAIF_Data, UDP data stream to UDP Application Interface (UAIF).
 * @param[out] soUAIF_Meta, UDP metadata stream to [UAIF].
 * @param[out] soICMP_Data, Control message to InternetControlMessageProtocol[ICMP] engine.

 * @details
 *  This process handles the payload of the incoming IP4 packet and forwards it
 *  the UdpAppInterface (UAIF).
 *
 *****************************************************************************/
void pRxPacketHandler(
        stream<AxisUdp>     &siUcc_UdpDgrm,
        stream<ValBool>     &siUcc_CsumVal,
        stream<AxisIp4>     &siIhs_Ip4Hdr,
        stream<UdpPort>     &soUpt_PortStateReq,
        stream<StsBool>     &siUpt_PortStateRep,
        stream<AxisApp>     &soUAIF_Data,
        stream<SocketPair>  &soUAIF_Meta,
        stream<AxisIcmp>    &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "Rph");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_RPH_IDLE=0,               FSM_RPH_PORT_LOOKUP,
    	                    FSM_RPH_STREAM,               FSM_RPH_STREAM_FIRST,
							FSM_RPH_DRAIN_DATAGRAM_STREAM,FSM_RPH_DRAIN_IP4HDR_STREAM,
							FSM_RPH_PORT_UNREACHABLE_1ST, FSM_RPH_PORT_UNREACHABLE_2ND,
							FSM_RPH_PORT_UNREACHABLE_STREAM,
							FSM_RPH_PORT_UNREACHABLE_LAST } rph_fsmState=FSM_RPH_IDLE;
    #pragma HLS RESET                              variable=rph_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIp4      rph_1stIp4HdrWord;
    static AxisIp4      rph_2ndIp4HdrWord;
    static AxisUdp      rph_udpHeaderWord;
    static FlagBit      rph_emptyPayloadFlag;
    static FlagBool     rph_doneWithIpHdrStream;

    static SocketPair   rph_udpMeta = SocketPair(SockAddr(0, 0), SockAddr(0, 0));

    switch(rph_fsmState) {
    case FSM_RPH_IDLE:
        if (!siUcc_UdpDgrm.empty() && !siIhs_Ip4Hdr.empty() &&
            !soUpt_PortStateReq.full()) {
            rph_doneWithIpHdrStream = false;
            // Read the 1st IPv4 header word and retrieve the IHL
            siIhs_Ip4Hdr.read(rph_1stIp4HdrWord);
            // Read the the 1st datagram word
            siUcc_UdpDgrm.read(rph_udpHeaderWord);
            // Request the state of this port to UdpPortTable (Upt)
            soUpt_PortStateReq.write(rph_udpHeaderWord.getUdpDstPort());
            // Check if payload of datagram is empty
            if (rph_udpHeaderWord.getUdpLen() > 8) {
                rph_emptyPayloadFlag = 0;
                // Update the UDP metadata
                rph_udpMeta.src.port = rph_udpHeaderWord.getUdpSrcPort();
                rph_udpMeta.dst.port = rph_udpHeaderWord.getUdpDstPort();
            }
            else {
                rph_emptyPayloadFlag = 1;
            }
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_IDLE - Receive new datagram (UdpLen=%d)\n",
                          rph_udpHeaderWord.getUdpLen().to_ushort());
            }
            rph_fsmState = FSM_RPH_PORT_LOOKUP;
        }
        break;
    case FSM_RPH_PORT_LOOKUP:
        if (!siUpt_PortStateRep.empty() && !siUcc_CsumVal.empty() && !siIhs_Ip4Hdr.empty()) {
            bool csumResult = siUcc_CsumVal.read();
            bool portLkpRes = siUpt_PortStateRep.read();
            // Read the 2nd IPv4 header word and update the metadata structure
            siIhs_Ip4Hdr.read(rph_2ndIp4HdrWord);
            rph_udpMeta.src.addr = rph_2ndIp4HdrWord.getIp4SrcAddr();
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_LOOKUP - CsumValid=%d and portLkpRes=%d.\n",
                          csumResult, portLkpRes);
            }
            if(portLkpRes && csumResult) {
                rph_fsmState = FSM_RPH_STREAM_FIRST;
            }
            else if (not csumResult) {
                rph_fsmState = FSM_RPH_DRAIN_DATAGRAM_STREAM;
            }
            else {
                rph_fsmState = FSM_RPH_PORT_UNREACHABLE_1ST;
            }
        }
        break;
    case FSM_RPH_STREAM_FIRST:
        if (DEBUG_LEVEL & TRACE_RPH) { printInfo(myName, "FSM_RPH_STREAM_FIRST \n"); }
        if (!siUcc_UdpDgrm.empty() && !siIhs_Ip4Hdr.empty() &&
            !soUAIF_Data.full()    && !soUAIF_Meta.full()) {
            // Read the 3rd IPv4 header word, update and forward the metadata
            AxisIp4 thirdIp4HdrWord;
            siIhs_Ip4Hdr.read(thirdIp4HdrWord);
            rph_udpMeta.dst.addr = thirdIp4HdrWord.getIp4DstAddr();
            if (thirdIp4HdrWord.getTLast()) {
                rph_doneWithIpHdrStream = true;
            }
            AxisUdp dgrmWord(0, 0, 0);
            if (not rph_emptyPayloadFlag) {
                soUAIF_Meta.write(rph_udpMeta);
                // Read the 1st datagram word and forward to [UAIF]
                siUcc_UdpDgrm.read(dgrmWord);
                soUAIF_Data.write(AxisApp(dgrmWord));
            }
            if (dgrmWord.getTLast() or rph_emptyPayloadFlag) {
                if (thirdIp4HdrWord.getTLast()) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_RPH_IDLE;
                 }
                else {
                    // They were options words that remain to be drained
                    rph_fsmState = FSM_RPH_DRAIN_IP4HDR_STREAM;
                }
            }
            else {
                rph_fsmState = FSM_RPH_STREAM;
            }
        }
        break;
    case FSM_RPH_STREAM:
        if (!siUcc_UdpDgrm.empty() && !soUAIF_Data.full() && !soUAIF_Meta.full()) {
            // Forward datagram word
            AxisUdp dgrmWord;
            siUcc_UdpDgrm.read(dgrmWord);
            soUAIF_Data.write(AxisApp(dgrmWord));
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_STREAM\n");
            }
            if (dgrmWord.getTLast()) {
                if (rph_doneWithIpHdrStream) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_RPH_IDLE;
                }
                else {
                    // They are IPv4 header words that remain to be drained
                    rph_fsmState = FSM_RPH_DRAIN_IP4HDR_STREAM;
                }
            }
            else if (!siIhs_Ip4Hdr.empty() && not rph_doneWithIpHdrStream) {
                // Drain any pending IPv4 header word
                AxisIp4 currIp4HdrWord;
                siIhs_Ip4Hdr.read(currIp4HdrWord);
                if (currIp4HdrWord.getTLast()) {
                    rph_doneWithIpHdrStream = true;
                    // Both incoming stream are empty. We are done.
                    // rph_fsmState = FSM_RPH_IDLE;
                }
            }
        }
        break;
    case FSM_RPH_DRAIN_DATAGRAM_STREAM:
        //-- Drop and drain the entire datagram
        if (!siUcc_UdpDgrm.empty()) {
            AxisUdp currWord;
            siUcc_UdpDgrm.read(currWord);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_DRAIN_DATAGRAM_STREAM -\n");
            }
            if (currWord.getTLast()) {
                if (rph_doneWithIpHdrStream) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_RPH_IDLE;
                }
                else {
                    // Now go and drain the corresponding IPV4 header stream
                    rph_fsmState = FSM_RPH_DRAIN_IP4HDR_STREAM;
                }
            }
        }
        break;
    case FSM_RPH_DRAIN_IP4HDR_STREAM :
        //-- Drain the IPv4 Header Stream
        if (!siIhs_Ip4Hdr.empty()) {
            AxisIp4 currWord;
            siIhs_Ip4Hdr.read(currWord);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_DRAIN_IP4HDR_STREAM -\n");
            }
            if (currWord.getTLast()) {
                rph_fsmState = FSM_RPH_IDLE;
            }
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_1ST:
        if (!soICMP_Data.full()) {
            // Forward the 1st word of the IPv4 header
            soICMP_Data.write(rph_1stIp4HdrWord);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_1ST -\n");
            }
            rph_fsmState = FSM_RPH_PORT_UNREACHABLE_2ND;
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_2ND:
        if (!soICMP_Data.full()) {
            // Forward the 2nd word of the IPv4 header
            soICMP_Data.write(rph_2ndIp4HdrWord);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_2ND -\n");
            }
            rph_fsmState = FSM_RPH_PORT_UNREACHABLE_STREAM;
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_STREAM:
        if (!siIhs_Ip4Hdr.empty() && !soICMP_Data.full()) {
            // Forward remaining of the IPv4 header words
            AxisIp4 ip4Word;
            siIhs_Ip4Hdr.read(ip4Word);
            // Always clear the LAST bit because the UDP header will follow
            soICMP_Data.write(AxisIcmp(ip4Word.getLE_TData(), ip4Word.getLE_TKeep(), 0));
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_STREAM -\n");
            }
            if (ip4Word.getTLast()) {
                rph_doneWithIpHdrStream = true;
                rph_fsmState = FSM_RPH_PORT_UNREACHABLE_LAST;
            }
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_LAST:
        if (DEBUG_LEVEL & TRACE_RPH) { printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_LAST -\n"); }
        if (!soICMP_Data.full()) {
            // Forward the first 8 bytes of the datagram (.i.e, the UDP header)
            soICMP_Data.write(AxisIcmp(rph_udpHeaderWord.getLE_TData(), rph_udpHeaderWord.getLE_TKeep(), TLAST));
            rph_fsmState = FSM_RPH_DRAIN_DATAGRAM_STREAM;
        }
        break;
    } // End-of: switch()
}

/******************************************************************************
 * UDP Port Table (Upt)
 *
 * param[in]  siRph_PortStateReq, Port state request from RxPacketHandler (Rph).
 * param[out] soRph_PortStateRep, Port state reply to [Rph].
 * param[in]  siUAIF_LsnReq,      Request to open a port from [UAIF].
 * param[out] soUAIF_LsnRep,      Open port status reply to [UAIF].
 * param[in]  siUAIF_ClsReq,      Request to close a port from [UAIF].
 *
 * @details
 *  The UDP Port Table (Upt) keeps track of the opened ports. A port is opened
 *  if its state is 'true' and closed othewise.
 *
 *****************************************************************************/
void pUdpPortTable(
        stream<UdpPort>     &siRph_PortStateReq,
        stream<StsBool>     &soRph_PortStateRep,
        stream<UdpPort>     &siUAIF_LsnReq,
        stream<StsBool>     &soUAIF_LsnRep,
        stream<UdpPort>     &siUAIF_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/RXe/", "Upt");

    //-- STATIC ARRAYS --------------------------------------------------------
    static ValBool                  PORT_TABLE[65536];
    #pragma HLS RESOURCE   variable=PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=PORT_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { UPT_WAIT4REQ=0, UPT_RPH_LKP, UPT_LSN_REP,
                            UPT_CLS_REP } upt_fsmState=UPT_WAIT4REQ;
    #pragma HLS RESET            variable=upt_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static UdpPort upt_portNum;

    switch (upt_fsmState) {
    case UPT_WAIT4REQ:
        if (!siRph_PortStateReq.empty()) {
            // Request to lookup the table from [RPh]
            siRph_PortStateReq.read(upt_portNum);
            upt_fsmState = UPT_RPH_LKP;
        }
        else if (!siUAIF_LsnReq.empty()) {
            // Request to open a port from [UAIF]
            siUAIF_LsnReq.read(upt_portNum);
            upt_fsmState = UPT_LSN_REP;
        }
        else if (!siUAIF_ClsReq.empty()) {
            // Request to close a port from [UAIF]
            siUAIF_ClsReq.read(upt_portNum);
            upt_fsmState = UPT_CLS_REP;
        }
        break;
    case UPT_RPH_LKP: // Lookup Reply
        if (!soRph_PortStateRep.full()) {
            soRph_PortStateRep.write(PORT_TABLE[upt_portNum]);
            upt_fsmState = UPT_WAIT4REQ;
        }
        break;
    case UPT_LSN_REP: // Listen Reply
        if (!soUAIF_LsnRep.full()) {
            PORT_TABLE[upt_portNum] = STS_OPENED;
            soUAIF_LsnRep.write(STS_OPENED);
            upt_fsmState = UPT_WAIT4REQ;
        }
        break;
    case UPT_CLS_REP: // Close Reply
        // [FIXME:MissingReplyChannel] if (!soUAIF_ClsRep.full()) {
        PORT_TABLE[upt_portNum] = STS_CLOSED;
        // [FIXME:MissingReplyChannel] soUAIF_ClsRep.write(STS_OCLOSED);
        upt_fsmState = UPT_WAIT4REQ;
        break;
    }
}

/******************************************************************************
 * Rx Engine (RXe)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[in]  siUAIF_LsnReq, UDP open port request from UdpAppInterface (UAIF).
 * @param[out] soUAIF_LsnRep, UDP open port reply to [UAIF].
 * @param[in]  siUAIF_ClsReq, UDP close port request from [UAIF].
 * @param[out] soUAIF_Data,   UDP data stream to [UAIF].
 * @param[out] soUAIF_Meta,   UDP metadata stream to [UAIF].
 * @param[out] soICMP_Data,   Control message to InternetControlMessageProtocol[ICMP] engine.
 *
 * @details
 *  The Rx path of the UdpOffloadEngine (UOE). This is the path from [IPRX]
 *  to the UdpAppInterface (UAIF).
 *****************************************************************************/
void pRxEngine(
        stream<AxisIp4>         &siIPRX_Data,
        stream<UdpPort>         &siUAIF_LsnReq,
        stream<StsBool>         &siUAIF_OpnRep,
        stream<UdpPort>         &siUAIF_ClsReq,
        stream<AxisApp>         &soUAIF_Data,
        stream<SocketPair>      &soUAIF_Meta,
        stream<AxisIcmp>        &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- IP Header Stripper (Ihs)
    static stream<AxisIp4>      ssIhsToRph_Ip4Hdr       ("ssIhsToRph_Ip4Hdr");
    #pragma HLS STREAM variable=ssIhsToRph_Ip4Hdr       depth=32
    static stream<AxisUdp>      ssIhsToUcc_UdpDgrm      ("ssIhsToUcc_UdpDgrm");
    #pragma HLS STREAM variable=ssIhsToUcc_UdpDgrm      depth=8
    static stream<UdpCsum>      ssIhsToUcc_PsdHdrSum    ("ssIhsToUcc_PsdHdrSum");
    #pragma HLS STREAM variable=ssIhsToUcc_PsdHdrSum    depth=8
    //-- UDP Checksum Checker (Ucc)
    static stream<AxisUdp>      ssUccToRph_UdpDgrm      ("ssUccToRph_UdpDgrm");
    #pragma HLS STREAM variable=ssUccToRph_UdpDgrm      depth=4096
    static stream<ValBool>      ssUccToRph_CsumVal      ("ssUccToRph_CsumVal");
    #pragma HLS STREAM variable=ssUccToRph_CsumVal      depth=8

    //-- UDP Packet Handler (UPh)
    static stream<UdpPort>      ssRphToUpt_PortStateReq ("ssRphToUpt_PortStateReq");
    #pragma HLS STREAM variable=ssRphToUpt_PortStateReq depth=2

    //-- UDP Port Table (Upt)
    static stream<StsBool>      ssUptToRph_PortStateRep ("ssUptToRph_PortStateRep");
    #pragma HLS STREAM variable=ssUptToRph_PortStateRep depth=2

    pIpHeaderStripper(
            siIPRX_Data,
            ssIhsToUcc_UdpDgrm,
            ssIhsToUcc_PsdHdrSum,
            ssIhsToRph_Ip4Hdr);

    pUdpChecksumChecker(
            ssIhsToUcc_UdpDgrm,
            ssIhsToUcc_PsdHdrSum,
            ssUccToRph_UdpDgrm,
            ssUccToRph_CsumVal);

    pRxPacketHandler(
            ssUccToRph_UdpDgrm,
            ssUccToRph_CsumVal,
            ssIhsToRph_Ip4Hdr,
            ssRphToUpt_PortStateReq,
            ssUptToRph_PortStateRep,
            soUAIF_Data,
            soUAIF_Meta,
            soICMP_Data);

    pUdpPortTable(
            ssRphToUpt_PortStateReq,
            ssUptToRph_PortStateRep,
            siUAIF_LsnReq,
            siUAIF_OpnRep,
            siUAIF_ClsReq);
}

/*** TXe PROCESSES   **********************************************************/

/******************************************************************************
 * Tx Application Interface (Tai)
 *
 * @param[in]  piMMIO_En    Enable signal from [SHELL/MMIO].
 * @param[in]  siUAIF_Data  Data stream from UserAppInterface (UAIF).
 * @param[in]  siUAIF_Meta  Metadata from [UAIF].
 * @param[in]  siUAIF_DLen  Data payload length from [UAIF].
 * @param[out] soTdh_Data   Data stream to UdpHeaderAdder (Uha).
 * @param[out] soTdh_Meta   Metadata stream to [Uha].
 * @param[out] soTdh_DLen   Data payload length to [Uha].
 *
 * @details
 *  This process is the front-end interface to the Tx part of the Udp Application
 *  Interface (UAIF). The APP must provide the data as a stream of 'AxisApp'
 *  chunks, and every stream must be accompanied by a metadata and payload
 *  length information. The metadata specifies the socket-pair that the stream
 *  belongs to, while the payload length specifies its length.
 *  Two modes of operations are supported by the UDP application interface:
 *  1) DATAGRAM_MODE: If the 'DLen' field is loaded with a length != 0, this
 *     length is used as reference for handling the corresponding stream. If the
 *     length is larger than 1472 bytes (.i.e, MTU-IP_HEADER_LEN-UDP_HEADER_LEN),
 *     the this process will split the incoming datagram and generate as many
 *     sub-datagrams as required to transport all 'DLen' bytes over Ethernet
 *     frames.
 *  2) STREAMING_MODE: If the 'DLen' field is configured with a length == 0, the
 *     corresponding stream stream will be forwarded based on the same metadata
 *     information until the 'TLAST' bit of the data stream is set. In this mode,
 *     the UOE will wait for the reception of 1472 bytes before generating a new
 *     UDP-over-IPv4 packet, unless the 'TLAST' bit of the data stream is set.
 *  In DATAGRAM_MODE, the setting of the 'TLAST' bit of the data stream is not
 *  required but highly recommended.
 *****************************************************************************/
void pTxApplicationInterface(
        CmdBit                   piMMIO_En,
        stream<AxisApp>         &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,
        stream<AxisApp>         &soTdh_Data,
        stream<UdpAppMeta>      &soTdh_Meta,
        stream<UdpAppDLen>      &soTdh_DLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Tai");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_TAI_IDLE=0,
                            FSM_TAI_DRGM_DATA, FSM_TAI_DRGM_META,
                            FSM_TAI_STRM_DATA, FSM_TAI_STRM_META,
                          } tai_fsmState=FSM_TAI_IDLE;
    #pragma HLS RESET   variable=tai_fsmState
    static FlagBool              tai_streamMode=false;
    #pragma HLS RESET   variable=tai_streamMode

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    //static AxisApp    tai_currWord;
    static UdpAppMeta  tai_appMeta;  // The socket-pair information
    static UdpAppDLen  tai_appDLen;  // Application's datagram length (0 to 2^16)
    static UdpAppDLen  tai_splitCnt; // Split counter (from 0 to 1472-1)

    switch(tai_fsmState) {
    case FSM_TAI_IDLE:
        if (!siUAIF_Meta.empty() and !siUAIF_DLen.empty() and (piMMIO_En == CMD_ENABLE)) {
            siUAIF_Meta.read(tai_appMeta);
            siUAIF_DLen.read(tai_appDLen);
            if (tai_appDLen == 0) {
                tai_streamMode = true;
                tai_fsmState = FSM_TAI_STRM_DATA;
            }
            else {
                tai_streamMode = false;
                tai_fsmState = FSM_TAI_DRGM_DATA;
            }
            tai_splitCnt = 0;
            if (DEBUG_LEVEL & TRACE_TAI) { printInfo(myName, "FSM_TAI_IDLE\n"); }
        }
        //[TODO] else if (!siUAIF_Data.empty() and soTdh_Data.full() ) {
            // In streaming-mode, we may accept up to the depth of 'ssTaiToTdh_Data'
            // bytes to be received ahead of the pair {Meta, DLen}
        //    siUAIF_Meta.read(tai_appMeta);
        //}
        break;
    case FSM_TAI_DRGM_DATA:
        if (!siUAIF_Data.empty() and !soTdh_Data.full() ) {
            //-- Forward data in 'datagram' mode
            AxisApp currChunk = siUAIF_Data.read();
            tai_appDLen  -= currChunk.getLen();
            tai_splitCnt += currChunk.getLen();
            if ((tai_appDLen == 0) or (tai_splitCnt == UDP_MDS)) {
                // Always enforce TLAST
                currChunk.setTLast(TLAST);
                tai_fsmState = FSM_TAI_DRGM_META;
            }
            soTdh_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_TAI) {
                printInfo(myName, "FSM_TAI_DRGM_DATA - DLen=%4d - Remainder=%5d \n",
                          tai_splitCnt.to_ushort(), tai_appDLen.to_ushort());
            }
        }
        break;
    case FSM_TAI_DRGM_META:
        if (!soTdh_Meta.full() and !soTdh_DLen.full()) {
            //-- Forward metadata and length in 'datagram' mode
            soTdh_Meta.write(tai_appMeta);
            soTdh_DLen.write(tai_splitCnt);
            if (tai_appDLen == 0) {
                tai_fsmState = FSM_TAI_IDLE;
            }
            else {
                tai_fsmState = FSM_TAI_DRGM_DATA;
                tai_splitCnt = 0;
            }
            if (DEBUG_LEVEL & TRACE_TAI) {
                printInfo(myName, "FSM_TAI_DRGM_META - DLen=%4d - Remainder=0 \n",
                        tai_splitCnt.to_ushort(), tai_appDLen.to_ushort());
            }
        }
        break;
    case FSM_TAI_STRM_DATA:
        if (!siUAIF_Data.empty() and !soTdh_Data.full() ) {
            //-- Forward data in 'streaming' mode
            AxisApp currChunk = siUAIF_Data.read();
            tai_appDLen  -= currChunk.getLen();
            tai_splitCnt += currChunk.getLen();
            if (currChunk.getTLast()) {
                tai_streamMode = false;
                tai_fsmState = FSM_TAI_STRM_META;
            }
            else if (tai_splitCnt == UDP_MDS) {
                // Always enforce TLAST
                currChunk.setTLast(TLAST);
                tai_fsmState = FSM_TAI_STRM_META;
            }
            soTdh_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_TAI) {
                printInfo(myName, "FSM_TAI_STRM_DATA - DLen=%4d \n",
                          tai_splitCnt.to_ushort());
            }
        }
        break;
    case FSM_TAI_STRM_META:
        if (!soTdh_Meta.full() and !soTdh_DLen.full()) {
            //-- Forward metadata and length in 'datagram' mode
            soTdh_Meta.write(tai_appMeta);
            soTdh_DLen.write(tai_splitCnt);
            if (tai_streamMode == false) {
                tai_fsmState = FSM_TAI_IDLE;
            }
            else {
                tai_fsmState = FSM_TAI_STRM_DATA;
                tai_splitCnt = 0;
            }
            if (DEBUG_LEVEL & TRACE_TAI) {
                printInfo(myName, "FSM_TAI_STRM_META - DLen=%4d - Remainder=0 \n",
                        tai_splitCnt.to_ushort(), tai_appDLen.to_ushort());
            }
        }
        break;
    }
}

/******************************************************************************
 * Tx Datagram Handler (Tdh)
 *
 * @param[in]  siUAIF_Data Data stream from UserAppInterface (UAIF).
 * @param[in]  siUAIF_Meta  Metadata stream from [UAIF].
 * @param[in]  siUAIF_DLen  Data payload length from [UAIF].
 * @param[out] soUha_Data   Data stream to UdpHeaderAdder (Uha).
 * @param[out] soUha_Meta   Metadata stream to [Uha].
 * @param[out] soUha_DLen   Data payload length to [Uha].
 * @param[out] soUca_Data   UDP data stream to UdpChecksumAccumulator (Uca).
 *
 * @details
 *  This process handles the raw data coming from the UdpAppInterface (UAIF).
 *  Data are receieved as a stream from the application layer. They come with a
 *  metadata information that specifies the connection the data belong to, as
 *  well as a data-length information.
 *  Two main tasks are perfomed by this process:
 *  1) a UDP pseudo-packet is generated and forwarded to the process
 *     UdpChecksumAccumulator (Uca) which will compute the UDP checksum.
 *  2) the length of the incoming data stream is measured while the AppData
 *     is streamed forward to the process UdpHeaderAdder (Uha).
 *
 *****************************************************************************/
void pTxDatagramHandler(
        stream<AxisApp>         &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,
        stream<AxisApp>         &soUha_Data,
        stream<UdpAppMeta>      &soUha_Meta,
        stream<UdpAppDLen>      &soUha_DLen,
        stream<AxisPsd4>        &soUca_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Tdh");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_TDH_PSD_PKT1=0, FSM_TDH_PSD_PKT2, FSM_TDH_PSD_PKT3,
                            FSM_TDH_PSD_PKT4,   FSM_TDH_STREAM,   FSM_TDH_PSD_RESIDUE,
                            FSM_TDH_LAST } tdh_fsmState=FSM_TDH_PSD_PKT1;
    #pragma HLS RESET             variable=tdh_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisApp    tdh_currWord;
    static UdpAppMeta tdh_udpMeta;
    static UdpAppDLen tdh_appLen; // The length of the application data stream
    static UdpLen     tdh_udpLen; // the length of the UDP datagram stream

    switch(tdh_fsmState) {
        case FSM_TDH_PSD_PKT1:
            if (!siUAIF_Meta.empty() and
                !soUha_Meta.full() and !soUca_Data.full()) {
                siUAIF_Meta.read(tdh_udpMeta);
                // Generate 1st pseudo-packet word [DA|SA]
                AxisPsd4 firstPseudoPktWord(0, 0xFF, 0);
                firstPseudoPktWord.setPsd4SrcAddr(tdh_udpMeta.src.addr);
                firstPseudoPktWord.setPsd4DstAddr(tdh_udpMeta.dst.addr);
                // Forward & next
                soUca_Data.write(firstPseudoPktWord);
                soUha_Meta.write(tdh_udpMeta);
                tdh_fsmState = FSM_TDH_PSD_PKT2;
                if (DEBUG_LEVEL & TRACE_TDH) {
                    printInfo(myName, "FSM_TDH_PSD_PKT1 - Receive new metadata information.\n");
                    printSockPair(myName, tdh_udpMeta);
                }
            }
        break;
        case FSM_TDH_PSD_PKT2:
            if (!siUAIF_DLen.empty() and
                !soUha_DLen.full() and !soUca_Data.full()) {
                siUAIF_DLen.read(tdh_appLen);
                // Generate UDP length from incoming payload length
                tdh_udpLen = tdh_appLen + UDP_HEADER_LEN;
                // Generate 2nd pseudo-packet word [DP|SP|Len|Prot|0x00]
                AxisPsd4 secondPseudoPktWord(0, 0xFF, 0);
                secondPseudoPktWord.setPsd4Prot(UDP_PROTOCOL);
                secondPseudoPktWord.setPsd4Len(tdh_udpLen);
                secondPseudoPktWord.setUdpSrcPort(tdh_udpMeta.src.port);
                secondPseudoPktWord.setUdpDstPort(tdh_udpMeta.dst.port);
                // Forward & next
                soUha_DLen.write(tdh_appLen);
                soUca_Data.write(secondPseudoPktWord);
                tdh_fsmState = FSM_TDH_PSD_PKT3;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT2\n"); }
            break;
        case FSM_TDH_PSD_PKT3:
            if (!siUAIF_Data.empty() and
                !soUca_Data.full() and !soUha_Data.full()) {
                // Read 1st data payload word
                siUAIF_Data.read(tdh_currWord);
                // Generate 3rd pseudo-packet word [Data|Csum|Len]
                AxisPsd4 thirdPseudoPktWord(0, 0xFF, 0);
                thirdPseudoPktWord.setUdpLen(tdh_udpLen);
                thirdPseudoPktWord.setUdpCsum(0x0000);
                thirdPseudoPktWord.setTDataLo(tdh_currWord.getTDataHi());
                if (tdh_currWord.getTLast()) {
                    soUha_Data.write(tdh_currWord);
                    if (tdh_currWord.getTKeepLo() == 0) {
                        // Done. Payload <= 4 bytes fits into current pseudo-pkt
                        tdh_fsmState = FSM_TDH_PSD_PKT1;
                        thirdPseudoPktWord.setTLast(TLAST);
                    }
                    else {
                        // Needs an additional pseudo-pkt-word for remaining 1-4 bytes
                        tdh_fsmState = FSM_TDH_PSD_PKT4;
                    }
                }
                else {
                    tdh_fsmState = FSM_TDH_STREAM;
                }
                soUca_Data.write(thirdPseudoPktWord);
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT3\n"); }
            break;
        case FSM_TDH_PSD_PKT4:
            if (!soUca_Data.full()) {
                // Generate 4th and last pseudo-packet word to hold remaining 1-4 bytes
                AxisPsd4 fourthPseudoPktWord(0, 0x00, TLAST);
                fourthPseudoPktWord.setTDataHi(tdh_currWord.getTDataLo());
                fourthPseudoPktWord.setTKeepHi(tdh_currWord.getTKeepLo());
                soUca_Data.write(fourthPseudoPktWord);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT4\n"); }
            break;
        case FSM_TDH_STREAM:
            // This state streams both the pseudo-packet chunks into the checksum
            // calculation stage and the next stage
            if (!siUAIF_Data.empty() and
                !soUha_Data.full() && !soUca_Data.full()) {
                // Always forward the current AppData word to next-stage [Uha]
                soUha_Data.write(tdh_currWord);
                // Save previous AppData and read a new one from [UAIF]
                AxisPsd4 currPseudoWord(0, 0xFF, 0);
                AxisApp  prevAppDataWord(tdh_currWord);
                siUAIF_Data.read(tdh_currWord);
                // Generate new pseudo-packet word
                currPseudoWord.setTDataHi(prevAppDataWord.getTDataLo());
                currPseudoWord.setTDataLo(tdh_currWord.getTDataHi());
                if (tdh_currWord.getTLast()) {
                    if (tdh_currWord.getTKeepLo() != 0) {
                        // Last AppData word won't fit in current pseudo-word.
                        tdh_fsmState = FSM_TDH_PSD_RESIDUE;
                    }
                    else {
                        currPseudoWord.setTLast(TLAST);
                        tdh_fsmState = FSM_TDH_LAST;
                    }
                }
                soUca_Data.write(currPseudoWord);
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_STREAM\n"); }
            break;
        case FSM_TDH_LAST:
            if (!soUha_Data.full()) {
                soUha_Data.write(tdh_currWord);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_LAST\n"); }
            break;
        case FSM_TDH_PSD_RESIDUE:
            if (!soUha_Data.full() && !soUca_Data.full()) {
                // Always forward the last AppData word
                soUha_Data.write(tdh_currWord);
                // Generate the last pseudo-packet word and forward it to [Uca]
                AxisPsd4 lastPseudoPktWord(0, 0x00, TLAST);
                lastPseudoPktWord.setTDataHi(tdh_currWord.getTDataLo());
                lastPseudoPktWord.setTKeepHi(tdh_currWord.getTKeepLo());
                soUca_Data.write(lastPseudoPktWord);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_RESIDUE\n"); }
            break;
    }
}

/******************************************************************************
 * UDP Checksum Accumulator (Uca)
 *
 * @param[in]  siTdh_Data,   Pseudo IPv4 packet from TxDatagramHandler (Tdh).
 * @param[out] soUha_Csum,   The checksum of the pseudo IPv4 packet to UdpHeaderAdder (Uha).
 *
 * @details
 *  This process accumulates the checksum over the pseudo header of an IPv4
 *  packet and its embeded UDP datagram. The computed checksum is forwarded to
 *  the [Uha] when 'TLAST' is reached.
 *
 *****************************************************************************/
void pUdpChecksumAccumulator(
        stream<AxisPsd4>    &siTdh_Data,
        stream<UdpCsum>     &soUha_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Uca");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<17>           uca_csum[4]={0,0,0,0};
    #pragma HLS RESET   variable=uca_csum

    if (!siTdh_Data.empty()) {
        AxisPsd4 currWord = siTdh_Data.read();
        uca_csum[0] += byteSwap16(currWord.getLE_TData().range(63, 48));
        uca_csum[0]  = (uca_csum[0] & 0xFFFF) + (uca_csum[0] >> 16);
        uca_csum[1] += byteSwap16(currWord.getLE_TData().range(47, 32));
        uca_csum[1]  = (uca_csum[1] & 0xFFFF) + (uca_csum[1] >> 16);
        uca_csum[2] += byteSwap16(currWord.getLE_TData().range(31, 16));
        uca_csum[2]  = (uca_csum[2] & 0xFFFF) + (uca_csum[2] >> 16);
        uca_csum[3] += byteSwap16(currWord.getLE_TData().range(15,  0));
        uca_csum[3]  = (uca_csum[3] & 0xFFFF) + (uca_csum[3] >> 16);
        if (currWord.getTLast() and !soUha_Csum.full()) {
            ap_uint<17> csum01, csum23, csum0123;
            csum01 = uca_csum[0] + uca_csum[1];
            csum01 = (csum01 & 0xFFFF) + (csum01 >> 16);
            csum23 = uca_csum[2] + uca_csum[3];
            csum23 = (csum23 & 0xFFFF) + (csum23 >> 16);
            csum0123 = csum01 + csum23;
            csum0123 = (csum0123 & 0xFFFF) + (csum0123 >> 16);
            csum0123 = ~csum0123;
            soUha_Csum.write(csum0123.range(15, 0));
            //-- Clear the csum accumulators
            uca_csum[0] = 0;
            uca_csum[1] = 0;
            uca_csum[2] = 0;
            uca_csum[3] = 0;
        }
    }
}

/******************************************************************************
 * UDP Header Adder (Uha)
 *
 * @param[in]  siTdh_Data   AppData stream from TxDatagramHandler (Tdh).
 * @param[in]  siTdh_DLen   Data payload length from [Tdh].
 * @param[in]  siTdh_Meta   Metadata from [tdH].
 * @param[in]  siUca_Csum   The UDP checksum from UdpChecksumAccumaulato (Uca).
 * @param[out] soIha_Data   UdpData strem to IpHeaderAdder (Iha).
 * @param[out] soIha_IpPair The IP_SA and IP_DA for this datagram.
 * @param[out] soIha_UdpLen The length of the UDP datagram to [Iha].
 *
 * @details
 *  This process creates a UDP header and prepends it to the AppData stream
 *  coming from the TxDatagramHandler (Tdh). The UDP checksum is read from the
 *  process UdpChecksumAccumulator(Uca).
 *****************************************************************************/
void pUdpHeaderAdder(
        stream<AxisApp>     &siTdh_Data,
        stream<UdpAppDLen>  &siTdh_DLen,
        stream<UdpAppMeta>  &siTdh_Meta,
        stream<UdpCsum>     &siUca_Csum,
        stream<AxisUdp>     &soIha_Data,
        stream<IpAddrPair>  &soIha_IpPair,
        stream<UdpLen>      &soIha_UdpLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Uha");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates{ UHA_IDLE=0, UHA_STREAM} uha_fsmState;
    #pragma HLS RESET                     variable=uha_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static UdpAppDLen   uha_appDLen;

    switch(uha_fsmState) {
    case UHA_IDLE:
        if (!siTdh_DLen.empty() and !siTdh_Meta.empty() and !siUca_Csum.empty() and
            !soIha_UdpLen.full() and !soIha_Data.full() and !soIha_IpPair.full()) {
                // Read data payload length
                siTdh_DLen.read(uha_appDLen);
                UdpLen  udpLen = uha_appDLen + UDP_HEADER_LEN;
                soIha_UdpLen.write(udpLen);
                // Read metadata and generate UDP header
                UdpAppMeta udpAppMeta = siTdh_Meta.read();
                AxisUdp udpHdrWord = AxisUdp(0, 0xFF, 0);
                udpHdrWord.setUdpSrcPort(udpAppMeta.src.port);
                udpHdrWord.setUdpDstPort(udpAppMeta.dst.port);
                udpHdrWord.setUdpLen(udpLen);
                udpHdrWord.setUdpCsum(siUca_Csum.read());
                soIha_Data.write(udpHdrWord);
                IpAddrPair ipAddrPair = IpAddrPair(udpAppMeta.src.addr, udpAppMeta.dst.addr);
                soIha_IpPair.write(ipAddrPair);
                uha_fsmState = UHA_STREAM;
                if (DEBUG_LEVEL & TRACE_UHA) { printInfo(myName, "UHA_IDLE\n"); }
        }
        break;
    case UHA_STREAM:
        if (!siTdh_Data.empty() and !soIha_Data.full()) {
            AxisApp currAppData = siTdh_Data.read();
            if (uha_appDLen > 8) {
                uha_appDLen -= 8;
                if (currAppData.getTLast() == TLAST) {
                    printWarn(myName, "Malformed - TLAST bit is set but end of Axis stream is not reached !!!\n");
                    // [FIXME - Must go to state that drains remaining chunks]
                    uha_fsmState = UHA_IDLE;
                }
            }
            else {
                if (currAppData.getTLast() != TLAST) {
                    printWarn(myName, "Malformed - End of Axis stream is reached but TLAST bit is not set!!!\n");
                    // [FIXME - Must go to state that drains remaining chunks]
                    currAppData.setTLast(TLAST);
                }
                uha_appDLen = 0;
                uha_fsmState = UHA_IDLE;
            }
            soIha_Data.write(AxisUdp(currAppData.getLE_TData(), currAppData.getLE_TKeep(), currAppData.getLE_TLast()));
        }
        if (DEBUG_LEVEL & TRACE_UHA) { printInfo(myName, "UHA_STREAM\n"); }
        break;
    }
}

/******************************************************************************
 * IPv4 Header Adder (Iha)
 *
 * @param[in]  siUha_Data    UDM datagram stream from UdpHeaderAdder (Uha).
 * @param[in]  siUha_IpPair  The IP_SA and IP_DA of this this datagram from [Uha].
 * @param[in]  siUha_UdpLen  The length of the UDP datagram fro [Uha].
 * @param[out] soIPTX_Data   The IPv4 packet to IpTxHandler (IPTX).
 *
 * @details
 *  This process creates an IPv4 header and prepends it to the UDP datagram
 *  stream coming from the UdpHeaderAdder (Uha).
 *****************************************************************************/
void pIp4HeaderAdder(
        stream<AxisUdp>     &siUha_Data,
        stream<IpAddrPair>  &siUha_IpPair,
        stream<UdpLen>      &siUha_UdpLen,
        stream<AxisIp4>     &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Iha");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { IPH_IP1=0, IPH_IP2, IPH_IP3, IPH_FORWARD,
                            IPH_RESIDUE} iha_fsmState;

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static IpAddrPair iha_ipPair;
    static AxisUdp    iha_prevUdpWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    static AxisUdp    currUdpWord;

    switch(iha_fsmState) {
    case IPH_IP1:
        if(!siUha_UdpLen.empty() and !siUha_Data.empty() and !siUha_IpPair.empty() and
           !soIPTX_Data.full()) {
            UdpLen    udpLen = siUha_UdpLen.read();
            Ip4PktLen ip4Len = udpLen + IP4_HEADER_LEN;
            AxisIp4 firstIp4Word = AxisIp4(0, 0xFF, 0);
            firstIp4Word.setIp4HdrLen(5);
            firstIp4Word.setIp4Version(4);
            firstIp4Word.setIp4ToS(0);
            firstIp4Word.setIp4TotalLen(ip4Len);
            firstIp4Word.setIp4Ident(0);
            firstIp4Word.setIp4Flags(0);
            firstIp4Word.setIp4FragOff(0);
            soIPTX_Data.write(firstIp4Word);
            iha_fsmState = IPH_IP2;
            if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP1\n"); }
        }
    break;
    case IPH_IP2:
        if(!siUha_IpPair.empty() and !siUha_Data.empty() and
           !soIPTX_Data.full()) {
            iha_ipPair = siUha_IpPair.read();
            AxisIp4 secondIp4Word = AxisIp4(0, 0xFF, 0);
            secondIp4Word.setIp4TtL(0xFF);
            secondIp4Word.setIp4Prot(UDP_PROTOCOL);
            secondIp4Word.setIp4HdrCsum(0);  // FYI-HeaderCsum will be generated by [IPTX]
            secondIp4Word.setIp4SrcAddr(iha_ipPair.ipSa);  // [FIXME-Replace by piMMIO_IpAddress]
            soIPTX_Data.write(secondIp4Word);
            iha_fsmState = IPH_IP3;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP2\n"); }
        break;
    case IPH_IP3:
        if(!siUha_Data.empty() and !soIPTX_Data.full()) {
            currUdpWord = siUha_Data.read();
            AxisIp4 thirdIp4Word = AxisIp4(0x0, 0xFF, 0);
            thirdIp4Word.setIp4DstAddr(iha_ipPair.ipDa);
            thirdIp4Word.setUdpSrcPort(currUdpWord.getUdpSrcPort());
            thirdIp4Word.setUdpDstPort(currUdpWord.getUdpDstPort());
            soIPTX_Data.write(thirdIp4Word);
            iha_fsmState = IPH_FORWARD;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP3\n"); }
        break;
    case IPH_FORWARD:
        if(!siUha_Data.empty() && !soIPTX_Data.full()) {
            currUdpWord = siUha_Data.read();
            AxisIp4 forwardIp4Word = AxisIp4(0x0, 0xFF, 0);
            forwardIp4Word.setTDataHi(iha_prevUdpWord.getTDataLo());
            forwardIp4Word.setTDataLo(    currUdpWord.getTDataHi());
            if(currUdpWord.getTLast()) {
                if (currUdpWord.getTKeepLo() != 0) {
                    iha_fsmState = IPH_RESIDUE;
                }
                else {
                    forwardIp4Word.setTKeepHi(iha_prevUdpWord.getTKeepLo());
                    forwardIp4Word.setTKeepLo(currUdpWord.getTKeepHi());
                    forwardIp4Word.setTLast(TLAST);
                    iha_fsmState = IPH_IP1;
                }
            }
            soIPTX_Data.write(forwardIp4Word);
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_FORWARD\n"); }
        break;
    case IPH_RESIDUE:
        if (!soIPTX_Data.full()) {
            AxisIp4 forwardIp4Word = AxisIp4(0x0, 0x00, TLAST);
            forwardIp4Word.setTDataHi(iha_prevUdpWord.getTDataLo());
            forwardIp4Word.setTKeepHi(iha_prevUdpWord.getTKeepLo());
            soIPTX_Data.write(forwardIp4Word);
            iha_fsmState = IPH_IP1;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_RESIDUE\n"); }
        break;
    }
    iha_prevUdpWord = currUdpWord;
}

/******************************************************************************
 * Tx Engine (TXe)
 *
 * @param[in]  piMMIO_En    Enable signal from [SHELL/MMIO].
 * @param[in]  siUAIF_Data  Data stream from UserAppInterface (UAIF).
 * @param[in]  siUAIF_Meta  Metadata stream from [UAIF].
 * @param[in]  siUAIF_DLen  Data length from [UAIF].
 * @param[out] soIPTX_Data  Data stream to IpTxHandler (IPTX).
 *
 * @details
 *  The Tx path of the UdpOffloadEngine (UOE). This is the path from the
 *  UdpAppInterface (UAIF).
 *****************************************************************************/
void pTxEngine(
        CmdBit                   piMMIO_En,
        stream<AxisApp>         &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,
        stream<AxisIp4>         &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    const char *myName  = concat3(THIS_NAME, "/", "TXe");

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tx Application Interface (Tai)
    static stream<AxisApp>         ssTaiToTdh_Data    ("ssTaiToTdh_Data");
    #pragma HLS STREAM    variable=ssTaiToTdh_Data    depth=4096
    static stream<UdpAppMeta>      ssTaiToTdh_Meta    ("ssTaiToTdh_Meta");
    #pragma HLS STREAM    variable=ssTaiToTdh_Meta    depth=4
    #pragma HLS DATA_PACK variable=ssTaiToTdh_Meta
    static stream<UdpAppDLen>      ssTaiToTdh_DLen    ("ssTaiToTdh_DLen");
    #pragma HLS STREAM    variable=ssTaiToTdh_DLen    depth=4

    //-- Tx Datagram Handler (Tdh)
    static stream<AxisApp>         ssTdhToUha_Data    ("ssTdhToUha_Data");
    #pragma HLS STREAM    variable=ssTdhToUha_Data    depth=2048
    static stream<UdpAppMeta>      ssTdhToUha_Meta    ("ssTdhToUha_Meta");
    #pragma HLS STREAM    variable=ssTdhToUha_Meta    depth=4
    #pragma HLS DATA_PACK variable=ssTdhToUha_Meta
    static stream<UdpAppDLen>      ssTdhToUha_DLen    ("ssTdhToUha_DLen");
    #pragma HLS STREAM    variable=ssTdhToUha_DLen    depth=4
    static stream<AxisPsd4>        ssTdhToUca_Data    ("ssTdhToUca_Data");
    #pragma HLS STREAM    variable=ssTdhToUca_Data    depth=32

    // UdpChecksumAccumulator (Uca)
    static stream<UdpCsum>         ssUcaToUha_Csum    ("ssUcaToUha_Csum");
    #pragma HLS STREAM    variable=ssUcaToUha_Csum    depth=8

    // UdpHeaderAdder (Uha)
    static stream<AxisUdp>         ssUhaToIha_Data    ("ssUhaToIha_Data");
    #pragma HLS STREAM    variable=ssUhaToIha_Data    depth=2048
    static stream<UdpLen>          ssUhaToIha_UdpLen  ("ssUhaToIha_UdpLen");
    #pragma HLS STREAM    variable=ssUhaToIha_UdpLen  depth=4
    static stream<IpAddrPair>      ssUhaToIha_IpPair  ("ssUhaToIha_IpPair");
    #pragma HLS STREAM    variable=ssUhaToIha_IpPair  depth=4
    #pragma HLS DATA_PACK variable=ssUhaToIha_IpPair

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTxApplicationInterface(
            piMMIO_En,
            siUAIF_Data,
            siUAIF_Meta,
            siUAIF_DLen,
            ssTaiToTdh_Data,
            ssTaiToTdh_Meta,
            ssTaiToTdh_DLen);

    pTxDatagramHandler(
            ssTaiToTdh_Data,
            ssTaiToTdh_Meta,
            ssTaiToTdh_DLen,
            ssTdhToUha_Data,
            ssTdhToUha_Meta,
            ssTdhToUha_DLen,
            ssTdhToUca_Data);

    pUdpChecksumAccumulator(
            ssTdhToUca_Data,
            ssUcaToUha_Csum);

    pUdpHeaderAdder(
            ssTdhToUha_Data,
            ssTdhToUha_DLen,
            ssTdhToUha_Meta,
            ssUcaToUha_Csum,
            ssUhaToIha_Data,
            ssUhaToIha_IpPair,
            ssUhaToIha_UdpLen);


    pIp4HeaderAdder(
            ssUhaToIha_Data,
            ssUhaToIha_IpPair,
            ssUhaToIha_UdpLen,
            soIPTX_Data);

}


/******************************************************************************
 * @brief 	Main process of the UDP Offload Engine (UOE).
 *
 * -- MMIO Interface
 * @param[in]  piMMIO_En      Enable signal from [SHELL/MMIO].
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data    IP4 data stream from IpRxHAndler (IPRX).
 * -- IPTX / IP Tx / Data Interface
 * @param[out] soIPTX_Data    IP4 data stream to IpTxHandler (IPTX).
 * -- UAIF / Control Port Interfaces
 * @param[in]  siUAIF_LsnReq  UDP open port request from UdpAppInterface (UAIF).
 * @param[out] soUAIF_LsnRep  UDP open port reply to [UAIF].
 * @param[in]  siUAIF_ClsReq  UDP close port request from [UAIF].
 * -- UAIF / Rx Data Interfaces
 * @param[out] soUAIF_Data    UDP data stream to [UAIF].
 * @param[out] soUAIF_Meta    UDP metadata stream to [UAIF].
 * -- UAIF / Tx Data Interfaces
 * @param[in]  siUAIF_Data    UDP data stream from [UAIF].
 * @param[in]  siUAIF_Meta    UDP metadata stream from [UAIF].
 * @param[in]  siUAIF_DLen    UDP data length form [UAIF].
 * -- ICMP / Message Data Interface
 * @param[out] soICMP_Data    Data stream to [ICMP].
 *
 ******************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                           piMMIO_En,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                 &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                 &soIPTX_Data,

        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>                 &siUAIF_LsnReq,
        stream<StsBool>                 &soUAIF_LsnRep,
        stream<UdpPort>                 &siUAIF_ClsReq,

        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &soUAIF_Data,
        stream<UdpAppMeta>              &soUAIF_Meta,

        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &siUAIF_Data,
        stream<UdpAppMeta>              &siUAIF_Meta,
        stream<UdpAppDLen>              &siUAIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisIcmp>                &soICMP_Data)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable          port=piMMIO_En         name=piMMIO_En

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data       metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_Data       metadata="-bus_bundle soIPTX_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_LsnReq     metadata="-bus_bundle siUAIF_LsnReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_LsnRep     metadata="-bus_bundle soUAIF_LsnRep"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_ClsReq     metadata="-bus_bundle siUAIF_ClsReq"

    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_Data       metadata="-bus_bundle soUAIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_Meta       metadata="-bus_bundle soUAIF_Meta"
    #pragma HLS DATA_PACK                variable=soUAIF_Meta

    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_Data       metadata="-bus_bundle siUAIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_Meta       metadata="-bus_bundle siUAIF_Meta"
    #pragma HLS DATA_PACK                variable=siUAIF_Meta
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_DLen       metadata="-bus_bundle siUAIF_DLen"

    #pragma HLS RESOURCE core=AXI4Stream variable=soICMP_Data       metadata="-bus_bundle soICMP_Data"

#else
    #pragma HLS INTERFACE ap_stable          port=piMMIO_En         name=piMMIO_En

    #pragma HLS INTERFACE axis register both port=siIPRX_Data       name=siIPRX_Data
    #pragma HLS INTERFACE axis register both port=soIPTX_Data       name=soIPTX_Data
    #pragma HLS DATA_PACK                variable=soIPTX_Data   instance=soIPTX_Data

    #pragma HLS INTERFACE axis register both port=siUAIF_LsnReq     name=siUAIF_LsnReq
    #pragma HLS INTERFACE axis register both port=soUAIF_LsnRep     name=soUAIF_LsnRep
    #pragma HLS INTERFACE axis register both port=siUAIF_ClsReq     name=siUAIF_ClsReq

    #pragma HLS INTERFACE axis register both port=soUAIF_Data       name=soUAIF_Data
    #pragma HLS INTERFACE axis register both port=soUAIF_Meta       name=soUAIF_Meta
    #pragma HLS DATA_PACK                variable=soUAIF_Meta   instance=soUAIF_Meta

    #pragma HLS INTERFACE axis register both port=siUAIF_Data       name=siUAIF_Data
    #pragma HLS INTERFACE axis  register both port=siUAIF_Meta       name=siUAIF_Meta
    #pragma HLS DATA_PACK                variable=siUAIF_Meta   instance=siUAIF_Meta
    #pragma HLS INTERFACE axis register both port=siUAIF_DLen       name=siUAIF_DLen

    #pragma HLS INTERFACE axis register both port=soICMP_Data       name=soICMP_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    pRxEngine(
            siIPRX_Data,
            siUAIF_LsnReq,
            soUAIF_LsnRep,
            siUAIF_ClsReq,
            soUAIF_Data,
            soUAIF_Meta,
            soICMP_Data);

    pTxEngine(
            piMMIO_En,
            siUAIF_Data,
            siUAIF_Meta,
            siUAIF_DLen,
            soIPTX_Data);

	/******************************************************************************
	 * Rx Engine (RXe)
	 *
	 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
	 * @param[in]  siUAIF_LsnReq, UDP open port request from UdpAppInterface (UAIF).
	 * @param[out] soUAIF_LsnRep, UDP open port reply to [UAIF].
	 * @param[in]  siUAIF_ClsReq, UDP close port request from [UAIF].
	 * @param[out] soUAIF_Data,   UDP data stream to [UAIF].
	 * @param[out] soUAIF_Meta,   UDP metadata stream to [UAIF].
	 * @param[out] soICMP_Data,   Control message to InternetControlMessageProtocol[ICMP] engine.
	 *
	 * @details
	 *  The Rx path of the UdpOffloadEngine (UOE). This is the path from [IPRX]
	 *  to the UdpAppInterface (UAIF).
	 *****************************************************************************/
/*** OBSOLETE_20200425 ************

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- IP Header Stripper (Ihs)
    static stream<AxisIp4>      ssIhsToRph_Ip4Hdr       ("ssIhsToRph_Ip4Hdr");
    #pragma HLS STREAM variable=ssIhsToRph_Ip4Hdr       depth=32
    static stream<AxisUdp>      ssIhsToUcc_UdpDgrm      ("ssIhsToUcc_UdpDgrm");
    #pragma HLS STREAM variable=ssIhsToUcc_UdpDgrm      depth=8
    static stream<UdpCsum>      ssIhsToUcc_PsdHdrSum    ("ssIhsToUcc_PsdHdrSum");
    #pragma HLS STREAM variable=ssIhsToUcc_PsdHdrSum    depth=8
    //-- UDP Checksum Checker (Ucc)
    static stream<AxisUdp>      ssUccToRph_UdpDgrm      ("ssUccToRph_UdpDgrm");
    #pragma HLS STREAM variable=ssUccToRph_UdpDgrm      depth=4096
    static stream<ValBool>      ssUccToRph_CsumVal      ("ssUccToRph_CsumVal");
    #pragma HLS STREAM variable=ssUccToRph_CsumVal      depth=8

    //-- UDP Packet Handler (UPh)
    static stream<UdpPort>      ssRphToUpt_PortStateReq ("ssRphToUpt_PortStateReq");
    #pragma HLS STREAM variable=ssRphToUpt_PortStateReq depth=2

    //-- UDP Port Table (Upt)
    static stream<StsBool>      ssUptToRph_PortStateRep ("ssUptToRph_PortStateRep");
    #pragma HLS STREAM variable=ssUptToRph_PortStateRep depth=2

    pIpHeaderStripper(
            siIPRX_Data,
            ssIhsToUcc_UdpDgrm,
            ssIhsToUcc_PsdHdrSum,
            ssIhsToRph_Ip4Hdr);

    pUdpChecksumChecker(
            ssIhsToUcc_UdpDgrm,
            ssIhsToUcc_PsdHdrSum,
            ssUccToRph_UdpDgrm,
            ssUccToRph_CsumVal);

    pRxPacketHandler(
            ssUccToRph_UdpDgrm,
            ssUccToRph_CsumVal,
            ssIhsToRph_Ip4Hdr,
            ssRphToUpt_PortStateReq,
            ssUptToRph_PortStateRep,
            soUAIF_Data,
            soUAIF_Meta,
            soICMP_Data);

    pUdpPortTable(
            ssRphToUpt_PortStateReq,
            ssUptToRph_PortStateRep,
            siUAIF_LsnReq,
            soUAIF_LsnRep,
            siUAIF_ClsReq);
***************************************/

    /******************************************************************************
     * Tx Engine (TXe)
     *
     * @param[in]  piMMIO_En    Enable signal from [SHELL/MMIO].
     * @param[in]  siUAIF_Data  Data stream from UserAppInterface (UAIF).
     * @param[in]  siUAIF_Meta  Metadata stream from [UAIF].
     * @param[in]  siUAIF_DLen  Data length from [UAIF].
     * @param[out] soIPTX_Data  Data stream to IpTxHandler (IPTX).
     *
     * @details
     *  The Tx path of the UdpOffloadEngine (UOE). This is the path from the
     *  UdpAppInterface (UAIF).
     *****************************************************************************/
/*** OBSOLETE_20200425 ************
    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tx Application Interface (Tai)
    static stream<AxisApp>         ssTaiToTdh_Data    ("ssTaiToTdh_Data");
    #pragma HLS STREAM    variable=ssTaiToTdh_Data    depth=4096
    static stream<UdpAppMeta>      ssTaiToTdh_Meta    ("ssTaiToTdh_Meta");
    #pragma HLS STREAM    variable=ssTaiToTdh_Meta    depth=4
    #pragma HLS DATA_PACK variable=ssTaiToTdh_Meta
    static stream<UdpAppDLen>      ssTaiToTdh_DLen    ("ssTaiToTdh_DLen");
    #pragma HLS STREAM    variable=ssTaiToTdh_DLen    depth=4

    //-- Tx Datagram Handler (Tdh)
    static stream<AxisApp>         ssTdhToUha_Data    ("ssTdhToUha_Data");
    #pragma HLS STREAM    variable=ssTdhToUha_Data    depth=2048
    static stream<UdpAppMeta>      ssTdhToUha_Meta    ("ssTdhToUha_Meta");
    #pragma HLS STREAM    variable=ssTdhToUha_Meta    depth=4
    #pragma HLS DATA_PACK variable=ssTdhToUha_Meta
    static stream<UdpAppDLen>      ssTdhToUha_DLen    ("ssTdhToUha_DLen");
    #pragma HLS STREAM    variable=ssTdhToUha_DLen    depth=4
    static stream<AxisPsd4>        ssTdhToUca_Data    ("ssTdhToUca_Data");
    #pragma HLS STREAM    variable=ssTdhToUca_Data    depth=32

    // UdpChecksumAccumulator (Uca)
    static stream<UdpCsum>         ssUcaToUha_Csum    ("ssUcaToUha_Csum");
    #pragma HLS STREAM    variable=ssUcaToUha_Csum    depth=8

    // UdpHeaderAdder (Uha)
    static stream<AxisUdp>         ssUhaToIha_Data    ("ssUhaToIha_Data");
    #pragma HLS STREAM    variable=ssUhaToIha_Data    depth=2048
    static stream<UdpLen>          ssUhaToIha_UdpLen  ("ssUhaToIha_UdpLen");
    #pragma HLS STREAM    variable=ssUhaToIha_UdpLen  depth=4
    static stream<IpAddrPair>      ssUhaToIha_IpPair  ("ssUhaToIha_IpPair");
    #pragma HLS STREAM    variable=ssUhaToIha_IpPair  depth=4
    #pragma HLS DATA_PACK variable=ssUhaToIha_IpPair

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTxApplicationInterface(
            piMMIO_En,
            siUAIF_Data,
            siUAIF_Meta,
            siUAIF_DLen,
            ssTaiToTdh_Data,
            ssTaiToTdh_Meta,
            ssTaiToTdh_DLen);

    pTxDatagramHandler(
            ssTaiToTdh_Data,
            ssTaiToTdh_Meta,
            ssTaiToTdh_DLen,
            ssTdhToUha_Data,
            ssTdhToUha_Meta,
            ssTdhToUha_DLen,
            ssTdhToUca_Data);

    pUdpChecksumAccumulator(
            ssTdhToUca_Data,
            ssUcaToUha_Csum);

    pUdpHeaderAdder(
            ssTdhToUha_Data,
            ssTdhToUha_DLen,
            ssTdhToUha_Meta,
            ssUcaToUha_Csum,
            ssUhaToIha_Data,
            ssUhaToIha_IpPair,
            ssUhaToIha_UdpLen);

    pIp4HeaderAdder(
            ssUhaToIha_Data,
            ssUhaToIha_IpPair,
            ssUhaToIha_UdpLen,
            soIPTX_Data);

*********************/

}
