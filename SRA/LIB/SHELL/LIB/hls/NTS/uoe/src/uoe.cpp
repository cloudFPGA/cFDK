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
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_UOE
 * \{
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
#define TRACE_UCA  1 << 6
#define TRACE_UCC  1 << 7
#define TRACE_UHA  1 << 8
#define TRACE_UPT  1 << 9
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*** RXe PROCESSES *************************************************************/

/*******************************************************************************
 * IPv4 Header Stripper (Ihs)
 *
 * @param[in]  siIPRX_Data     IP4 data stream from IpRxHAndler (IPRX).
 * @param[out] soUcc_UdpDgrm   UDP datagram stream to UdpChecksumChecker (Ucc).
 * @param[out] soUcc_PsdHdrSum Sum of the pseudo header information to [Ucc].
 * @param[out] soRph_Ip4Hdr    The header part of the IPv4 packet as a stream to [Rph].
 *
 * @details
 *  This process extracts the UDP pseudo header and the IP header from the
 *  incoming IPv4 packet. The IP header is forwarded to RxPacketHandler (Rph)
 *  for further processing while the UDP datagram and the UDP pseudo-header
 *  are forwarded to the UDP checksum checker.
 *
 *******************************************************************************/
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
      static AxisIp4       ihs_prevChunk;
      static Ip4HdrLen     ihs_ip4HdrLen;
      static ap_uint<17>   ihs_psdHdrSum;

      //-- DYNAMIC VARIABLES ----------------------------------------------------
      AxisIp4     currIp4Chunk;

      switch(ihs_fsmState) {
      case FSM_IHS_IPW0:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 1st AXI-CHUNK (Frag|Flags|Id|TotLen|ToS|Ver|IHL) ---------
              siIPRX_Data.read(currIp4Chunk);
              ihs_ip4HdrLen = currIp4Chunk.getIp4HdrLen();
              if (ihs_ip4HdrLen < 5) {
                  printWarn(myName, "FSM_IHS_IPW0 - Received an IPv4 packet with invalid IHL. This packet will be dropped.\n");
                  ihs_fsmState  = FSM_IHS_DROP;
              }
              else {
                  soRph_Ip4Hdr.write(currIp4Chunk);
                  if (DEBUG_LEVEL & TRACE_IHS) {
                      printInfo(myName, "FSM_IHS_IPW0 - Received a new IPv4 packet (IHL=%d|TotLen=%d)\n",
                                ihs_ip4HdrLen.to_uint(), currIp4Chunk.getIp4TotalLen().to_ushort());
                  }
                  ihs_ip4HdrLen -= 2;
                  ihs_fsmState  = FSM_IHS_IPW1;
              }
          }
          break;
      case FSM_IHS_IPW1:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 2nd AXI-CHUNK (SA|HdrCsum|Prot|TTL)
              siIPRX_Data.read(currIp4Chunk);
              soRph_Ip4Hdr.write(currIp4Chunk);
              //-- Csum accumulate (SA+Prot)
              ihs_psdHdrSum  = (0x00, currIp4Chunk.getIp4Prot());
              ihs_psdHdrSum += currIp4Chunk.getIp4SrcAddr().range(31,16);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_psdHdrSum += currIp4Chunk.getIp4SrcAddr().range(15, 0);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_ip4HdrLen -= 2;
              ihs_fsmState = FSM_IHS_IPW2;
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_IPW1 - \n"); }
          break;
      case FSM_IHS_IPW2:
          if (!siIPRX_Data.empty() && !soRph_Ip4Hdr.full()) {
              //-- READ 3rd AXI-CHUNK (DP|SP|DA) or (Opt|DA)
              siIPRX_Data.read(currIp4Chunk);
              //-- Csum accumulate (DA)
              ihs_psdHdrSum += currIp4Chunk.getIp4DstAddr().range(31,16);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              ihs_psdHdrSum += currIp4Chunk.getIp4DstAddr().range(15, 0);
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              if (ihs_ip4HdrLen == 1) {
                  // This a typical IPv4 header with a length of 20 bytes (5*4).
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_Ip4DstAddr(), 0x0F, TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER;
              }
              else if (ihs_ip4HdrLen == 2 ) {
                  printWarn(myName, "This IPv4 packet contains one 32-bit option! FYI, IPV4 options are not supported.\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_TData(), currIp4Chunk.getLE_TKeep(), TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
              }
              else {  // ihs_ip4HdrLen > 2
                  printWarn(myName, "This IPv4 packet contains two+ 32-bit options! FYI, IPV4 options are not supported.\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_TData(), currIp4Chunk.getLE_TKeep(), 0));
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
              siIPRX_Data.read(currIp4Chunk);
              if (ihs_ip4HdrLen == 1) {
                  printWarn(myName, "This IPv4 packet contains three 32-bit options!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_TDataHi(), 0x0F, TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER;
              }
              else if (ihs_ip4HdrLen == 2 ) {
                  printWarn(myName, "This IPv4 packet contains four 32-bit options!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_TData(), currIp4Chunk.getLE_TKeep(), TLAST));
                  ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
              }
              else {  // ihs_ip4HdrLen > 2
                  printWarn(myName, "This IPv4 packet contains four+ 32-bit options!\n");
                  soRph_Ip4Hdr.write(AxisIp4(currIp4Chunk.getLE_TData(), currIp4Chunk.getLE_TKeep(), 0));
                  ihs_ip4HdrLen -= 2;
                  ihs_fsmState = FSM_IHS_OPT;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_OPT - \n"); }
          break;
      case FSM_IHS_UDP_HEADER:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- Read end of un-aligned UDP header (Data|Csum|Len) ----------
              siIPRX_Data.read(currIp4Chunk);
              //-- Csum accumulate (UdpLen)
              ihs_psdHdrSum += currIp4Chunk.getUdpLen();
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              //-- Forward UDP PseudoHeaderCsum to [Ucc]
              soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
              //-- Forward the UDP Header (Csum|Len|DP|SP)
              AxisUdp sendChunk(0, 0xFF, 0);
              sendChunk.setTDataHi(ihs_prevChunk.getTDataLo());
              sendChunk.setTDataLo(currIp4Chunk.getTDataHi());
              if (currIp4Chunk.getTLast()) {
                  if (currIp4Chunk.getTKeep() == 0xF0) {
                      printWarn(myName, "Received a UDP datagram of length = 0!\n");
                      sendChunk.setTKeep(0xFF);
                      ihs_fsmState = FSM_IHS_IPW0;
                  }
                  else {
                      sendChunk.setTKeepHi(ihs_prevChunk.getTKeepLo());
                      sendChunk.setTKeepLo(currIp4Chunk.getTKeepHi());
                      ihs_fsmState = FSM_IHS_RESIDUE;
                  }
              }
              else {
                  ihs_fsmState = FSM_IHS_FORWARD;
              }
              soUcc_UdpDgrm.write(sendChunk);
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER - \n"); }
          break;
      case FSM_IHS_UDP_HEADER_ALIGNED:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- READ Aligned UDP header (Csum|Len|DP|SP) --------
              siIPRX_Data.read(currIp4Chunk);
              //-- Cast incoming chunk into an AxisUdp chunk
              AxisUdp  currUdpChunk(currIp4Chunk);
              //-- Csum accumulate (UdpLen)
              ihs_psdHdrSum += currUdpChunk.getUdpLen();
              ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
              //-- Forward UDP PseudoHeaderCsum to [Ucc]
              soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
              //-- Forward the UDP Header (Csum|Len|DP|SP)
              soUcc_UdpDgrm.write(currUdpChunk);
              if (currUdpChunk.getTLast()) {
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
              //-- READ n-th AXI-CHUNK (Data) ----------------
              siIPRX_Data.read(currIp4Chunk);
              //-- Forward 1/2 of previous chunk and 1/2 of current chunk)
              AxisUdp sendChunk(0,0,0);
              sendChunk.setTDataHi(ihs_prevChunk.getTDataLo());
              sendChunk.setTKeepHi(ihs_prevChunk.getTKeepLo());
              sendChunk.setTDataLo(currIp4Chunk.getTDataHi());
              sendChunk.setTKeepLo(currIp4Chunk.getTKeepHi());
              if (currIp4Chunk.getTLast()) {
                   if (currIp4Chunk.getTKeep() <= 0xF0) {
                      sendChunk.setTLast(TLAST);
                      ihs_fsmState = FSM_IHS_IPW0;
                  }
                  else {
                      sendChunk.setTLast(0);
                      ihs_fsmState = FSM_IHS_RESIDUE;
                  }
              }
              else {
                  sendChunk.setTLast(0);
                  ihs_fsmState = FSM_IHS_FORWARD;
              }
              soUcc_UdpDgrm.write(sendChunk);
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_FORWARD - \n"); }
          break;
      case FSM_IHS_FORWARD_ALIGNED:
          if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
              //-- READ UDP ALIGNED AXI-CHUNK --------------
              siIPRX_Data.read(currIp4Chunk);
              //-- Cast incoming chunk into an AxisUdp chunk
              AxisUdp  currUdpChunk(currIp4Chunk);
              soUcc_UdpDgrm.write(currUdpChunk);
              if (currUdpChunk.getTLast()) {
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
              //-- Forward the very last bytes of the current chunk
              AxisUdp sendChunk(0,0,0);
              sendChunk.setTDataHi(ihs_prevChunk.getTDataLo());
              sendChunk.setTKeepHi(ihs_prevChunk.getTKeepLo());
              sendChunk.setTLast(TLAST);
             soUcc_UdpDgrm.write(sendChunk);
             ihs_fsmState = FSM_IHS_IPW0;
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_RESIDUE - \n"); }
          break;
      case FSM_IHS_DROP:
          if (!siIPRX_Data.empty()) {
              //-- READ and DRAIN all AXI-CHUNK -------------
              siIPRX_Data.read(currIp4Chunk);
              if (currIp4Chunk.getTLast()) {
                  ihs_fsmState = FSM_IHS_IPW0;
              }
          }
          if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_DROP - \n"); }
          break;
      } // End-of: switch

      ihs_prevChunk = currIp4Chunk;

  } // End-of: pIpHeaderStripper

/*******************************************************************************
 * UDP Checksum Checker (Ucc)
 *
 * @param[in]  siUcc_UdpDgrm   UDP datagram stream from IpHeaderStripper (Ihs).
 * @param[in]  siUcc_PsdHdrSum Pseudo header sum (SA+DA+Prot+Len) from [Ihs].
 * @param[out] soRph_UdpDgrm   UDP datagram stream to RxPacketHandler (Rph).
 * @param[out] soRph_CsumVal   Checksum valid information to [Rph].
 *
 * @details
 *  This process accumulates the checksum over the UDP header and the UDP data.
 *  The computed checksum is compared with the embedded checksum when 'TLAST'
 *  is reached, and the result is forwarded to the RxPacketHandler (Rph).
 *
 *******************************************************************************/
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
    static ap_uint<10>                       ucc_chunkCount=0;
    #pragma HLS RESET               variable=ucc_chunkCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<17>  ucc_csum0;
    static ap_uint<17>  ucc_csum1;
    static ap_uint<17>  ucc_csum2;
    static ap_uint<17>  ucc_csum3;
    static UdpCsum      ucc_psdHdrCsum;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisUdp     currChunk;

    switch (ucc_fsmState) {
    case FSM_UCC_IDLE:
        if (!siIhs_UdpDgrm.empty() && !siIhs_PsdHdrSum.empty()) {
            //-- READ 1st DTGM-CHUNK (CSUM|LEN|DP|SP)
            siIhs_UdpDgrm.read(currChunk);
            //-- READ the checksum of the pseudo header
            siIhs_PsdHdrSum.read(ucc_psdHdrCsum);
            // Always forward datagram to [Rph]
            soRph_UdpDgrm.write(currChunk);
            if (currChunk.getUdpCsum() == 0x0000) {
            // An all zero transmitted checksum  value means that the
            // transmitter generated no checksum.
            soRph_CsumVal.write(true);
            ucc_fsmState = FSM_UCC_STREAM;
        }
        else {
            // Accumulate e the UDP header
            ucc_csum0  = 0x00000 | currChunk.getUdpSrcPort();
            ucc_csum0 += ucc_psdHdrCsum;
            ucc_csum0  = (ucc_csum0 & 0xFFFF) + (ucc_csum0 >> 16);
            ucc_csum1 = 0x00000 | currChunk.getUdpDstPort();
            ucc_csum2 = 0x00000 | currChunk.getUdpLen();
            ucc_csum3 = 0x00000 | currChunk.getUdpCsum();
            if (currChunk.getUdpLen() == 8) {
                // Payload is empty
                ucc_fsmState = FSM_UCC_CHK0;
            }
            else {
                ucc_fsmState = FSM_UCC_ACCUMULATE;
            }
        }
        if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_IDLE       - ", currChunk); }
        }
        break;
    case FSM_UCC_STREAM:
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currChunk);
            soRph_UdpDgrm.write(currChunk);
            if (currChunk.getTLast()) {
                ucc_fsmState = FSM_UCC_IDLE;
            }
            if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_STREAM     - ", currChunk); }
        }
        break;
    case FSM_UCC_ACCUMULATE:
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currChunk);
            // Always set the disabled bytes to zero
            LE_tData cleanChunk = 0;
            if (currChunk.getLE_TKeep() & 0x01)
                cleanChunk.range( 7, 0) = (currChunk.getLE_TData()).range( 7, 0);
            if (currChunk.getLE_TKeep() & 0x02)
                cleanChunk.range(15, 8) = (currChunk.getLE_TData()).range(15, 8);
            if (currChunk.getLE_TKeep() & 0x04)
                cleanChunk.range(23,16) = (currChunk.getLE_TData()).range(23,16);
            if (currChunk.getLE_TKeep() & 0x08)
                cleanChunk.range(31,24) = (currChunk.getLE_TData()).range(31,24);
            if (currChunk.getLE_TKeep() & 0x10)
                cleanChunk.range(39,32) = (currChunk.getLE_TData()).range(39,32);
            if (currChunk.getLE_TKeep() & 0x20)
                cleanChunk.range(47,40) = (currChunk.getLE_TData()).range(47,40);
            if (currChunk.getLE_TKeep() & 0x40)
                cleanChunk.range(55,48) = (currChunk.getLE_TData()).range(55,48);
            if (currChunk.getLE_TKeep() & 0x80)
                cleanChunk.range(63,56) = (currChunk.getLE_TData()).range(63,56);
            soRph_UdpDgrm.write(AxisUdp(cleanChunk, currChunk.getLE_TKeep(), currChunk.getLE_TLast()));

            ucc_csum0 += byteSwap16(cleanChunk.range(63, 48));
            ucc_csum0  = (ucc_csum0 & 0xFFFF) + (ucc_csum0 >> 16);
            ucc_csum1 += byteSwap16(cleanChunk.range(47, 32));
            ucc_csum1  = (ucc_csum1 & 0xFFFF) + (ucc_csum1 >> 16);
            ucc_csum2 += byteSwap16(cleanChunk.range(31, 16));
            ucc_csum2  = (ucc_csum2 & 0xFFFF) + (ucc_csum2 >> 16);
            ucc_csum3 += byteSwap16(cleanChunk.range(15,  0));
            ucc_csum3  = (ucc_csum3 & 0xFFFF) + (ucc_csum3 >> 16);

            if (currChunk.getTLast()) {
              ucc_fsmState = FSM_UCC_CHK0;
          }
            if (DEBUG_LEVEL & TRACE_UCC) { printAxisRaw(myName,"FSM_UCC_ACCUMULATE - ", currChunk); }
        }
        break;
    case FSM_UCC_CHK0:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_CHK0 - \n"); }
        ucc_csum0 += ucc_csum2;
        ucc_csum0  = (ucc_csum0 & 0xFFFF) + (ucc_csum0 >> 16);
        ucc_csum1 += ucc_csum3;
        ucc_csum1  = (ucc_csum1 & 0xFFFF) + (ucc_csum1 >> 16);
        ucc_fsmState = FSM_UCC_CHK1;
        break;
    case FSM_UCC_CHK1:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_CHK1 - \n"); }
        ucc_csum0 += ucc_csum1;
        ucc_csum0  = (ucc_csum0 & 0xFFFF) + (ucc_csum0 >> 16);
        UdpCsum csumChk = ~(ucc_csum0(15, 0));
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

/*******************************************************************************
 * Rx Packet Handler (Rph)
 *
 * @param[in]  siIhs_UdpDgrm UDP datagram stream from IpHeaderStripper (Ihs).
 * @param[in]  siIhs_Ip4Hdr  The header part of the IPv4 packet from [Ihs].
 * @param[in]  siUcc_CsumVal Checksum valid information from UdpChecksumChecker (Ucc).
 * @param[out] soUpt_PortStateReq  Request for the state of port to UdpPortTable (Upt).
 * @param[in]  siUpt_PortStateRep  Port state reply from [Upt].
 * @param[out] soUAIF_Data   UDP data stream to UDP Application Interface (UAIF).
 * @param[out] soUAIF_Meta   UDP metadata stream to [UAIF].
 * @param[out] soICMP_Data   Control message to InternetControlMessageProtocol[ICMP] engine.

 * @details
 *  This process handles the payload of the incoming IP4 packet and forwards it
 *  the UdpAppInterface (UAIF).
 *  If the UDP checksum of incoming datagram is wrong, the datagram is dropped.
 *  If the destination UDP port is not opened, the incoming IP header and the
 *  first 8 bytes of the datagram are forwarded to the Internet Control Message
 *  Protocol (ICMP) Server which will build a 'Destination Unreachable' message.
 *
 *******************************************************************************/
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
    static AxisIp4      rph_1stIp4HdrChunk;
    static AxisIp4      rph_2ndIp4HdrChunk;
    static AxisUdp      rph_udpHeaderChunk;
    static FlagBit      rph_emptyPayloadFlag;
    static FlagBool     rph_doneWithIpHdrStream;

    static SocketPair   rph_udpMeta = SocketPair(SockAddr(0, 0), SockAddr(0, 0));

    switch(rph_fsmState) {
    case FSM_RPH_IDLE:
        if (!siUcc_UdpDgrm.empty() && !siIhs_Ip4Hdr.empty() &&
            !soUpt_PortStateReq.full()) {
            rph_doneWithIpHdrStream = false;
            // Read the 1st IPv4 header chunk retrieve the IHL
            siIhs_Ip4Hdr.read(rph_1stIp4HdrChunk);
            // Read the the 1st datagram chunk
            siUcc_UdpDgrm.read(rph_udpHeaderChunk);
            // Request the state of this port to UdpPortTable (Upt)
            soUpt_PortStateReq.write(rph_udpHeaderChunk.getUdpDstPort());
            // Check if payload of datagram is empty
            if (rph_udpHeaderChunk.getUdpLen() > 8) {
                rph_emptyPayloadFlag = 0;
                // Update the UDP metadata
                rph_udpMeta.src.port = rph_udpHeaderChunk.getUdpSrcPort();
                rph_udpMeta.dst.port = rph_udpHeaderChunk.getUdpDstPort();
            }
            else {
                rph_emptyPayloadFlag = 1;
            }
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_IDLE - Receive new datagram (UdpLen=%d)\n",
                          rph_udpHeaderChunk.getUdpLen().to_ushort());
            }
            rph_fsmState = FSM_RPH_PORT_LOOKUP;
        }
        break;
    case FSM_RPH_PORT_LOOKUP:
        if (!siUpt_PortStateRep.empty() && !siUcc_CsumVal.empty() && !siIhs_Ip4Hdr.empty()) {
            bool csumResult = siUcc_CsumVal.read();
            bool portLkpRes = siUpt_PortStateRep.read();
            // Read the 2nd IPv4 header chunk and update the metadata structure
            siIhs_Ip4Hdr.read(rph_2ndIp4HdrChunk);
            rph_udpMeta.src.addr = rph_2ndIp4HdrChunk.getIp4SrcAddr();
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
            // Read the 3rd IPv4 header chunk, update and forward the metadata
            AxisIp4 thirdIp4HdrChunk;
            siIhs_Ip4Hdr.read(thirdIp4HdrChunk);
            rph_udpMeta.dst.addr = thirdIp4HdrChunk.getIp4DstAddr();
            if (thirdIp4HdrChunk.getTLast()) {
                rph_doneWithIpHdrStream = true;
            }
            AxisUdp dgrmChunk(0, 0, 0);
            if (not rph_emptyPayloadFlag) {
                soUAIF_Meta.write(rph_udpMeta);
                // Read the 1st datagram chunk and forward to [UAIF]
                siUcc_UdpDgrm.read(dgrmChunk);
                soUAIF_Data.write(AxisApp(dgrmChunk));
            }
            if (dgrmChunk.getTLast() or rph_emptyPayloadFlag) {
                if (thirdIp4HdrChunk.getTLast()) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_RPH_IDLE;
                 }
                else {
                    // They were options chunk that remain to be drained
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
            // Forward datagram chunk
            AxisUdp dgrmChunk;
            siUcc_UdpDgrm.read(dgrmChunk);
            soUAIF_Data.write(AxisApp(dgrmChunk));
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_STREAM\n");
            }
            if (dgrmChunk.getTLast()) {
                if (rph_doneWithIpHdrStream) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_RPH_IDLE;
                }
                else {
                    // They are IPv4 header chunk that remain to be drained
                    rph_fsmState = FSM_RPH_DRAIN_IP4HDR_STREAM;
                }
            }
            else if (!siIhs_Ip4Hdr.empty() && not rph_doneWithIpHdrStream) {
                // Drain any pending IPv4 header chunk
                AxisIp4 currIp4HdrChunk;
                siIhs_Ip4Hdr.read(currIp4HdrChunk);
                if (currIp4HdrChunk.getTLast()) {
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
            AxisUdp currChunk;
            siUcc_UdpDgrm.read(currChunk);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_DRAIN_DATAGRAM_STREAM -\n");
            }
            if (currChunk.getTLast()) {
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
            AxisIp4 currChunk;
            siIhs_Ip4Hdr.read(currChunk);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_DRAIN_IP4HDR_STREAM -\n");
            }
            if (currChunk.getTLast()) {
                rph_fsmState = FSM_RPH_IDLE;
            }
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_1ST:
        if (!soICMP_Data.full()) {
            // Forward the 1st chunk of the IPv4 header
            soICMP_Data.write(rph_1stIp4HdrChunk);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_1ST -\n");
            }
            rph_fsmState = FSM_RPH_PORT_UNREACHABLE_2ND;
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_2ND:
        if (!soICMP_Data.full()) {
            // Forward the 2nd chunk of the IPv4 header
            soICMP_Data.write(rph_2ndIp4HdrChunk);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_2ND -\n");
            }
            rph_fsmState = FSM_RPH_PORT_UNREACHABLE_STREAM;
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_STREAM:
        if (!siIhs_Ip4Hdr.empty() && !soICMP_Data.full()) {
            // Forward remaining of the IPv4 header chunks
            AxisIp4 ip4Chunk;
            siIhs_Ip4Hdr.read(ip4Chunk);
            // Always clear the LAST bit because the UDP header will follow
            soICMP_Data.write(AxisIcmp(ip4Chunk.getLE_TData(), ip4Chunk.getLE_TKeep(), 0));
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_STREAM -\n");
            }
            if (ip4Chunk.getTLast()) {
                rph_doneWithIpHdrStream = true;
                rph_fsmState = FSM_RPH_PORT_UNREACHABLE_LAST;
            }
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_LAST:
        if (DEBUG_LEVEL & TRACE_RPH) { printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_LAST -\n"); }
        if (!soICMP_Data.full()) {
            // Forward the first 8 bytes of the datagram (.i.e, the UDP header)
            soICMP_Data.write(AxisIcmp(rph_udpHeaderChunk.getLE_TData(), rph_udpHeaderChunk.getLE_TKeep(), TLAST));
            rph_fsmState = FSM_RPH_DRAIN_DATAGRAM_STREAM;
        }
        break;
    } // End-of: switch()
}

/*******************************************************************************
 * UDP Port Table (Upt)
 *
 * param[out] soMMIO_Ready        Process ready signal.
 * param[in]  siRph_PortStateReq  Port state request from RxPacketHandler (Rph).
 * param[out] soRph_PortStateRep  Port state reply to [Rph].
 * param[in]  siUAIF_LsnReq       Listen port request from [UAIF].
 * param[out] soUAIF_LsnRep       Listen port reply to [UAIF] (0=closed/1=opened).
 * param[in]  siUAIF_ClsReq       Close  port request from [UAIF].
 * param[out] soUAIF_ClsRep       Close  port reply to [UAIF] (0=closed/1=opened).
 *
 * @details
 *  The UDP Port Table (Upt) keeps track of the opened ports. A port is opened
 *  if its state is 'true' and closed otherwise.
 *
 * @note: We are using a stream to signal that UOE is ready because the C/RTL
 *  co-simulation only only supports the following 'ap_ctrl_none' designs:
 *  (1) combinational designs; (2) pipelined design with task interval of 1;
 *  (3) designs with array streaming or hls_stream or AXI4 stream ports.
 *******************************************************************************/
void pUdpPortTable(
        stream<StsBool>     &soMMIO_Ready,
        stream<UdpPort>     &siRph_PortStateReq,
        stream<StsBool>     &soRph_PortStateRep,
        stream<UdpPort>     &siUAIF_LsnReq,
        stream<StsBool>     &soUAIF_LsnRep,
        stream<UdpPort>     &siUAIF_ClsReq,
        stream<StsBool>     &soUAIF_ClsRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //OBSOLETE_20200902 #pragma HLS pipeline II=2 enable_flush

    const char *myName = concat3(THIS_NAME, "/RXe/", "Upt");

    //-- STATIC ARRAYS --------------------------------------------------------
    static ValBool                  PORT_TABLE[0x10000];
    #pragma HLS RESOURCE   variable=PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=PORT_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { UPT_WAIT4REQ=0, UPT_RPH_LKP, UPT_LSN_REP,
                            UPT_CLS_REP } upt_fsmState=UPT_WAIT4REQ;
    #pragma HLS RESET            variable=upt_fsmState
    static bool                           upt_isInit=false;
    #pragma HLS reset            variable=upt_isInit
    static  UdpPort                       upt_initPtr=(0x10000-1);
    #pragma HLS reset            variable=upt_initPtr

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static UdpPort upt_portNum;

    // The PORT_TABLE must be initialized upon reset
    if (!upt_isInit) {
        PORT_TABLE[upt_initPtr] = STS_CLOSED;
        if (upt_initPtr == 0) {
            if (!soMMIO_Ready.full()) {
                soMMIO_Ready.write(true);
                upt_isInit = true;
                if (DEBUG_LEVEL & TRACE_UPT) {
                    printInfo(myName, "Done with initialization of the PORT_TABLE.\n");
                }
            }
            else {
                printWarn(myName, "Cannot signal INIT_DONE because HLS stream is not empty.\n");
            }
         }
        else {
            upt_initPtr = upt_initPtr - 1;
        }
        return;
    }

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
        if (!soUAIF_ClsRep.full()) {
            PORT_TABLE[upt_portNum] = STS_CLOSED;
            soUAIF_ClsRep.write(STS_CLOSED);
            upt_fsmState = UPT_WAIT4REQ;
        }
        break;
    }
}

/*******************************************************************************
 * Rx Engine (RXe)
 *
 * @param[out] soMMIO_Ready   Process ready signal.
 * @param[in]  siIPRX_Data    IP4 data stream from IpRxHAndler (IPRX).
 * @param[in]  siUAIF_LsnReq  UDP open port request from UdpAppInterface (UAIF).
 * @param[out] soUAIF_LsnRep  UDP open port reply to [UAIF].
 * @param[in]  siUAIF_ClsReq  UDP close port request from [UAIF].
 * @param[out] soUAIF_ClsRep  UDP close port reply to [UAIF].
 * @param[out] soUAIF_Data    UDP data stream to [UAIF].
 * @param[out] soUAIF_Meta    UDP metadata stream to [UAIF].
 * @param[out] soICMP_Data    Control message to InternetControlMessageProtocol[ICMP] engine.
 *
 * @details
 *  The Rx path of the UdpOffloadEngine (UOE). This is the path from [IPRX]
 *  to the UdpAppInterface (UAIF).
 *******************************************************************************/
void pRxEngine(
        stream<StsBool>         &soMMIO_Ready,
        stream<AxisIp4>         &siIPRX_Data,
        stream<UdpPort>         &siUAIF_LsnReq,
        stream<StsBool>         &soUAIF_LsnRep,
        stream<UdpPort>         &siUAIF_ClsReq,
        stream<StsBool>         &soUAIF_ClsRep,
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
    #pragma HLS STREAM variable=ssIhsToUcc_UdpDgrm      depth=1024
    static stream<UdpCsum>      ssIhsToUcc_PsdHdrSum    ("ssIhsToUcc_PsdHdrSum");
    #pragma HLS STREAM variable=ssIhsToUcc_PsdHdrSum    depth=16
    //-- UDP Checksum Checker (Ucc)
    static stream<AxisUdp>      ssUccToRph_UdpDgrm      ("ssUccToRph_UdpDgrm");
    #pragma HLS STREAM variable=ssUccToRph_UdpDgrm      depth=1024
    static stream<ValBool>      ssUccToRph_CsumVal      ("ssUccToRph_CsumVal");
    #pragma HLS STREAM variable=ssUccToRph_CsumVal      depth=16

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
            soMMIO_Ready,
            ssRphToUpt_PortStateReq,
            ssUptToRph_PortStateRep,
            siUAIF_LsnReq,
            soUAIF_LsnRep,
            siUAIF_ClsReq,
            soUAIF_ClsRep);
}

/*** TXe PROCESSES   ***********************************************************/

/*******************************************************************************
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
 *     length is larger than UDP_MDS bytes (.i.e, MTU_ZYC2-IP_HEADER_LEN-UDP_HEADER_LEN),
 *     this process will split the incoming datagram and generate as many
 *     sub-datagrams as required to transport all 'DLen' bytes over Ethernet
 *     frames.
 *  2) STREAMING_MODE: If the 'DLen' field is configured with a length == 0, the
 *     corresponding stream will be forwarded based on the same metadata
 *     information until the 'TLAST' bit of the data stream is set. In this mode,
 *     the UOE will wait for the reception of UDP_MDS bytes before generating a
 *     new UDP-over-IPv4 packet, unless the 'TLAST' bit of the data stream is set.
 *
 * @warning
 *  In DATAGRAM_MODE, the setting of the 'TLAST' bit of the data stream is not
 *   required but highly recommended.
 *  In STREAMING_MODE, it is the responsibility of the application to set the
 *   'TLAST' bit to avoid a connection from monopolizing the UOE indefinitely.
 *******************************************************************************/
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
    static UdpAppMeta  tai_appMeta;  // The socket-pair information
    static UdpAppDLen  tai_appDLen;  // Application's datagram length (0 to 2^16)
    static UdpAppDLen  tai_splitCnt; // Split counter (from 0 to 1422-1)

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

/*******************************************************************************
 * Tx Datagram Handler (Tdh)
 *
 * @param[in]  siUAIF_Data  Data stream from UserAppInterface (UAIF).
 * @param[in]  siUAIF_Meta  Metadata stream from [UAIF].
 * @param[in]  siUAIF_DLen  Data payload length from [UAIF].
 * @param[out] soUha_Data   Data stream to UdpHeaderAdder (Uha).
 * @param[out] soUha_Meta   Metadata stream to [Uha].
 * @param[out] soUha_DLen   Data payload length to [Uha].
 * @param[out] soUca_Data   UDP data stream to UdpChecksumAccumulator (Uca).
 *
 * @details
 *  This process handles the raw data coming from the UdpAppInterface (UAIF).
 *  Data are received as a stream from the application layer. They come with a
 *  metadata information that specifies the connection the data belong to, as
 *  well as a data-length information.
 *  Two main tasks are performed by this process:
 *  1) a UDP pseudo-packet is generated and forwarded to the process
 *     UdpChecksumAccumulator (Uca) which will compute the UDP checksum.
 *  2) the length of the incoming data stream is measured while the AppData
 *     is streamed forward to the process UdpHeaderAdder (Uha).
 *
 *******************************************************************************/
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
    //OBSOLETE_20200902 #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Tdh");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_TDH_PSD_PKT1=0, FSM_TDH_PSD_PKT2, FSM_TDH_PSD_PKT3,
                            FSM_TDH_PSD_PKT4,   FSM_TDH_STREAM,   FSM_TDH_PSD_RESIDUE,
                            FSM_TDH_LAST } tdh_fsmState=FSM_TDH_PSD_PKT1;
    #pragma HLS RESET             variable=tdh_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisApp    tdh_currChunk;
    static UdpAppMeta tdh_udpMeta;
    static UdpAppDLen tdh_appLen; // The length of the application data stream
    static UdpLen     tdh_udpLen; // the length of the UDP datagram stream

    switch(tdh_fsmState) {
        case FSM_TDH_PSD_PKT1:
            if (!siUAIF_Meta.empty() and
                !soUha_Meta.full() and !soUca_Data.full()) {
                siUAIF_Meta.read(tdh_udpMeta);
                // Generate 1st pseudo-packet chunk [DA|SA]
                AxisPsd4 firstPseudoPktChunk(0, 0xFF, 0);
                firstPseudoPktChunk.setPsd4SrcAddr(tdh_udpMeta.src.addr);
                firstPseudoPktChunk.setPsd4DstAddr(tdh_udpMeta.dst.addr);
                // Forward & next
                soUca_Data.write(firstPseudoPktChunk);
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
                // Generate 2nd pseudo-packet chunk [DP|SP|Len|Prot|0x00]
                AxisPsd4 secondPseudoPktChunk(0, 0xFF, 0);
                secondPseudoPktChunk.setPsd4Prot(IP4_PROT_UDP);
                secondPseudoPktChunk.setPsd4Len(tdh_udpLen);
                secondPseudoPktChunk.setUdpSrcPort(tdh_udpMeta.src.port);
                secondPseudoPktChunk.setUdpDstPort(tdh_udpMeta.dst.port);
                // Forward & next
                soUha_DLen.write(tdh_appLen);
                soUca_Data.write(secondPseudoPktChunk);
                tdh_fsmState = FSM_TDH_PSD_PKT3;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT2\n"); }
            break;
        case FSM_TDH_PSD_PKT3:
            if (!siUAIF_Data.empty() and
                !soUca_Data.full() and !soUha_Data.full()) {
                // Read 1st data payload chunk
                siUAIF_Data.read(tdh_currChunk);
                // Generate 3rd pseudo-packet chunk [Data|Csum|Len]
                AxisPsd4 thirdPseudoPktChunk(0, 0xFF, 0);
                thirdPseudoPktChunk.setUdpLen(tdh_udpLen);
                thirdPseudoPktChunk.setUdpCsum(0x0000);
                thirdPseudoPktChunk.setTDataLo(tdh_currChunk.getTDataHi());
                if (tdh_currChunk.getTLast()) {
                    tdh_currChunk.clearUnusedBytes();
                    soUha_Data.write(tdh_currChunk);
                    if (tdh_currChunk.getTKeepLo() == 0) {
                        // Done. Payload <= 4 bytes fits into current pseudo-pkt
                        tdh_fsmState = FSM_TDH_PSD_PKT1;
                        thirdPseudoPktChunk.setTKeepLo(tdh_currChunk.getTKeepHi());
                        thirdPseudoPktChunk.setTLast(TLAST);
                    }
                    else {
                        // Needs an additional pseudo-pkt-chunk for remaining 1-4 bytes
                        tdh_fsmState = FSM_TDH_PSD_PKT4;
                    }
                }
                else {
                    tdh_fsmState = FSM_TDH_STREAM;
                }
                soUca_Data.write(thirdPseudoPktChunk);
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT3\n"); }
            break;
        case FSM_TDH_PSD_PKT4:
            if (!soUca_Data.full()) {
                // Generate 4th and last pseudo-packet chunk to hold remaining 1-4 bytes
                AxisPsd4 fourthPseudoPktChunk(0, 0x00, TLAST);
                fourthPseudoPktChunk.setTDataHi(tdh_currChunk.getTDataLo());
                fourthPseudoPktChunk.setTKeepHi(tdh_currChunk.getTKeepLo());
                soUca_Data.write(fourthPseudoPktChunk);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT4\n"); }
            break;
        case FSM_TDH_STREAM:
            // This state streams both the pseudo-packet chunks into the checksum
            // calculation stage and the next stage
            if (!siUAIF_Data.empty() and
                !soUha_Data.full() && !soUca_Data.full()) {
                // Always forward the current AppData chunk to next-stage [Uha]
                soUha_Data.write(tdh_currChunk);
                // Save previous AppData and read a new one from [UAIF]
                AxisPsd4 currPseudoChunk(0, 0xFF, 0);
                AxisApp  prevAppDataChunk(tdh_currChunk);
                siUAIF_Data.read(tdh_currChunk);
                // Generate new pseudo-packet chunk
                currPseudoChunk.setTDataHi(prevAppDataChunk.getTDataLo());
                currPseudoChunk.setTDataLo(tdh_currChunk.getTDataHi());
                if (tdh_currChunk.getTLast()) {
                    if (tdh_currChunk.getTKeepLo() != 0) {
                        // Last AppData chunk won't fit in current pseudo-chunk.
                        tdh_fsmState = FSM_TDH_PSD_RESIDUE;
                    }
                    else {
                        // Done. Payload <= 4 bytes fits into current pseudo-pkt
                        tdh_fsmState = FSM_TDH_LAST;
                        currPseudoChunk.setTKeepLo(tdh_currChunk.getTKeepHi());
                        currPseudoChunk.setTLast(TLAST);
                    }
                }
                soUca_Data.write(currPseudoChunk);
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_STREAM\n"); }
            break;
        case FSM_TDH_LAST:
            if (!soUha_Data.full()) {
                soUha_Data.write(tdh_currChunk);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_LAST\n"); }
            break;
        case FSM_TDH_PSD_RESIDUE:
            if (!soUha_Data.full() && !soUca_Data.full()) {
                // Always forward the last AppData chunk
                soUha_Data.write(tdh_currChunk);
                // Generate the last pseudo-packet chunk and forward it to [Uca]
                AxisPsd4 lastPseudoPktChunk(0, 0x00, TLAST);
                lastPseudoPktChunk.setTDataHi(tdh_currChunk.getTDataLo());
                lastPseudoPktChunk.setTKeepHi(tdh_currChunk.getTKeepLo());
                soUca_Data.write(lastPseudoPktChunk);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_RESIDUE\n"); }
            break;
    }
}

/*******************************************************************************
 * UDP Checksum Accumulator (Uca)
 *
 * @param[in]  siTdh_Data  Pseudo IPv4 packet from TxDatagramHandler (Tdh).
 * @param[out] soUha_Csum  The checksum of the pseudo IPv4 packet to UdpHeaderAdder (Uha).
 *
 * @details
 *  This process accumulates the checksum over the pseudo header of an IPv4
 *  packet and its embedded UDP datagram. The computed checksum is forwarded to
 *  the [Uha] when 'TLAST' is reached.
 *
 *******************************************************************************/
void pUdpChecksumAccumulator(
        stream<AxisPsd4>    &siTdh_Data,
        stream<UdpCsum>     &soUha_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Uca");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<17>           uca_csum0;
    #pragma HLS RESET   variable=uca_csum0
    static ap_uint<17>           uca_csum1;
    #pragma HLS RESET   variable=uca_csum1
    static ap_uint<17>           uca_csum2;
    #pragma HLS RESET   variable=uca_csum2
    static ap_uint<17>           uca_csum3;
    #pragma HLS RESET   variable=uca_csum3

    if (!siTdh_Data.empty() and !soUha_Csum.full()) {
        AxisPsd4 currChunk = siTdh_Data.read();
         currChunk.clearUnusedBytes();
        if (DEBUG_LEVEL & TRACE_UCA) {
            printAxisRaw(myName, "Received a new pseudo-header chunk: ", currChunk);
        }
        uca_csum0 += byteSwap16(currChunk.getLE_TData().range(63, 48));
        uca_csum0  = (uca_csum0 & 0xFFFF) + (uca_csum0 >> 16);
        uca_csum1 += byteSwap16(currChunk.getLE_TData().range(47, 32));
        uca_csum1  = (uca_csum1 & 0xFFFF) + (uca_csum1 >> 16);
        uca_csum2 += byteSwap16(currChunk.getLE_TData().range(31, 16));
        uca_csum2  = (uca_csum2 & 0xFFFF) + (uca_csum2 >> 16);
        uca_csum3 += byteSwap16(currChunk.getLE_TData().range(15,  0));
        uca_csum3  = (uca_csum3 & 0xFFFF) + (uca_csum3 >> 16);
        if (currChunk.getTLast()) {
            ap_uint<17> csum01, csum23, csum0123;
            csum01 = uca_csum0 + uca_csum1;
            csum01 = (csum01 & 0xFFFF) + (csum01 >> 16);
            csum23 = uca_csum2 + uca_csum3;
            csum23 = (csum23 & 0xFFFF) + (csum23 >> 16);
            csum0123 = csum01 + csum23;
            csum0123 = (csum0123 & 0xFFFF) + (csum0123 >> 16);
            csum0123 = ~csum0123;
            soUha_Csum.write(csum0123.range(15, 0));
            //-- Clear the csum accumulators
            uca_csum0 = 0;
            uca_csum1 = 0;
            uca_csum2 = 0;
            uca_csum3 = 0;
            if (DEBUG_LEVEL & TRACE_UCA) {
                printInfo(myName, "End of pseudo-header packet.\n");
            }
        }
    }
}

/*******************************************************************************
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
 *******************************************************************************/
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
                AxisUdp udpHdrChunk = AxisUdp(0, 0xFF, 0);
                udpHdrChunk.setUdpSrcPort(udpAppMeta.src.port);
                udpHdrChunk.setUdpDstPort(udpAppMeta.dst.port);
                udpHdrChunk.setUdpLen(udpLen);
                udpHdrChunk.setUdpCsum(siUca_Csum.read());
                soIha_Data.write(udpHdrChunk);
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

/*******************************************************************************
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
 *******************************************************************************/
void pIp4HeaderAdder(
        stream<AxisUdp>     &siUha_Data,
        stream<IpAddrPair>  &siUha_IpPair,
        stream<UdpLen>      &siUha_UdpLen,
        stream<AxisIp4>     &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    //OBSOLETE_20200902 #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/TXe/", "Iha");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { IPH_IP1=0, IPH_IP2, IPH_IP3, IPH_FORWARD,
                            IPH_RESIDUE} iha_fsmState;
    #pragma HLS RESET           variable=iha_fsmState


    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static IpAddrPair iha_ipPair;
    static AxisUdp    iha_prevUdpChunk;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    static AxisUdp    currUdpChunk;

    switch(iha_fsmState) {
    case IPH_IP1:
        if(!siUha_UdpLen.empty() and !siUha_Data.empty() and !siUha_IpPair.empty() and
           !soIPTX_Data.full()) {
            UdpLen    udpLen = siUha_UdpLen.read();
            Ip4PktLen ip4Len = udpLen + IP4_HEADER_LEN;
            AxisIp4 firstIp4Chunk = AxisIp4(0, 0xFF, 0);
            firstIp4Chunk.setIp4HdrLen(5);
            firstIp4Chunk.setIp4Version(4);
            firstIp4Chunk.setIp4ToS(0);
            firstIp4Chunk.setIp4TotalLen(ip4Len);
            firstIp4Chunk.setIp4Ident(0);
            firstIp4Chunk.setIp4Flags(0);
            firstIp4Chunk.setIp4FragOff(0);
            soIPTX_Data.write(firstIp4Chunk);
            iha_fsmState = IPH_IP2;
            if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP1\n"); }
        }
    break;
    case IPH_IP2:
        if(!siUha_IpPair.empty() and !siUha_Data.empty() and
           !soIPTX_Data.full()) {
            iha_ipPair = siUha_IpPair.read();
            AxisIp4 secondIp4Chunk = AxisIp4(0, 0xFF, 0);
            secondIp4Chunk.setIp4TtL(0xFF);
            secondIp4Chunk.setIp4Prot(IP4_PROT_UDP);
            secondIp4Chunk.setIp4HdrCsum(0);  // FYI-HeaderCsum will be generated by [IPTX]
            secondIp4Chunk.setIp4SrcAddr(iha_ipPair.ipSa);  // [FIXME-Replace by piMMIO_IpAddress]
            soIPTX_Data.write(secondIp4Chunk);
            iha_fsmState = IPH_IP3;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP2\n"); }
        break;
    case IPH_IP3:
        if(!siUha_Data.empty() and !soIPTX_Data.full()) {
            currUdpChunk = siUha_Data.read();
            AxisIp4 thirdIp4Chunk = AxisIp4(0x0, 0xFF, 0);
            thirdIp4Chunk.setIp4DstAddr(iha_ipPair.ipDa);
            thirdIp4Chunk.setUdpSrcPort(currUdpChunk.getUdpSrcPort());
            thirdIp4Chunk.setUdpDstPort(currUdpChunk.getUdpDstPort());
            soIPTX_Data.write(thirdIp4Chunk);
            iha_fsmState = IPH_FORWARD;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_IP3\n"); }
        break;
    case IPH_FORWARD:
        if(!siUha_Data.empty() && !soIPTX_Data.full()) {
            currUdpChunk = siUha_Data.read();
            AxisIp4 forwardIp4Chunk = AxisIp4(0x0, 0xFF, 0);
            forwardIp4Chunk.setTDataHi(iha_prevUdpChunk.getTDataLo());
            forwardIp4Chunk.setTDataLo(    currUdpChunk.getTDataHi());
            if(currUdpChunk.getTLast()) {
                if (currUdpChunk.getTKeepLo() != 0) {
                    iha_fsmState = IPH_RESIDUE;
                }
                else {
                    forwardIp4Chunk.setTKeepHi(iha_prevUdpChunk.getTKeepLo());
                    forwardIp4Chunk.setTKeepLo(currUdpChunk.getTKeepHi());
                    forwardIp4Chunk.setTLast(TLAST);
                    iha_fsmState = IPH_IP1;
                }
            }
            soIPTX_Data.write(forwardIp4Chunk);
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_FORWARD\n"); }
        break;
    case IPH_RESIDUE:
        if (!soIPTX_Data.full()) {
            AxisIp4 forwardIp4Chunk = AxisIp4(0x0, 0x00, 0);
            forwardIp4Chunk.setTDataHi(iha_prevUdpChunk.getTDataLo());
            forwardIp4Chunk.setTKeepHi(iha_prevUdpChunk.getTKeepLo());
            forwardIp4Chunk.setTLast(TLAST);
            soIPTX_Data.write(forwardIp4Chunk);
            iha_fsmState = IPH_IP1;
        }
        if (DEBUG_LEVEL & TRACE_IHA) { printInfo(myName, "IPH_RESIDUE\n"); }
        break;
    }
    iha_prevUdpChunk = currUdpChunk;
}

/*******************************************************************************
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
 *******************************************************************************/
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


/*******************************************************************************
 * @brief  Main process of the UDP Offload Engine (UOE).
 *
 * -- MMIO Interface
 * @param[in]  piMMIO_En      Enable signal from [SHELL/MMIO].
 * @param[out] soMMIO_Ready   UOE ready stream to [SHELL/MMIO].
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data    IP4 data stream from IpRxHAndler (IPRX).
 * -- IPTX / IP Tx / Data Interface
 * @param[out] soIPTX_Data    IP4 data stream to IpTxHandler (IPTX).
 * -- UAIF / Control Port Interfaces
 * @param[in]  siUAIF_LsnReq  UDP open  port request from UdpAppInterface (UAIF).
 * @param[out] soUAIF_LsnRep  UDP open  port reply   to   [UAIF] (0=closed/1=opened).
 * @param[in]  siUAIF_ClsReq  UDP close port request from [UAIF].
 * @param[out] soUAIF_ClsRep  UDP close port reply   to   [UAIF] (0=closed/1=opened).
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
 *******************************************************************************/
void uoe(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                           piMMIO_En,
        stream<StsBool>                 &soMMIO_Ready,
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
        stream<UdpAppLsnReq>            &siUAIF_LsnReq,
        stream<UdpAppLsnRep>            &soUAIF_LsnRep,
        stream<UdpAppClsReq>            &siUAIF_ClsReq,
        stream<UdpAppClsRep>            &soUAIF_ClsRep,
        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &soUAIF_Data,
        stream<UdpAppMeta>              &soUAIF_Meta,
        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &siUAIF_Data,
        stream<UdpAppMeta>              &siUAIF_Meta,
        stream<UdpAppDLen>              &siUAIF_DLen,
        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisIcmp>                &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- PROCESS FUNCTIONS -----------------------------------------------------

    pRxEngine(
            soMMIO_Ready,
            siIPRX_Data,
            siUAIF_LsnReq,
            soUAIF_LsnRep,
            siUAIF_ClsReq,
            soUAIF_ClsRep,
            soUAIF_Data,
            soUAIF_Meta,
            soICMP_Data);

    pTxEngine(
            piMMIO_En,
            siUAIF_Data,
            siUAIF_Meta,
            siUAIF_DLen,
            soIPTX_Data);

}

/*******************************************************************************
 * @brief Top of UDP Offload Engine (UOE)
 *
 * -- MMIO Interface
 * @param[in]  piMMIO_En      Enable signal from [SHELL/MMIO].
 * @param[out] soMMIO_Ready   UOE ready stream to [SHELL/MMIO].
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data    IP4 data stream from IpRxHAndler (IPRX).
 * -- IPTX / IP Tx / Data Interface
 * @param[out] soIPTX_Data    IP4 data stream to IpTxHandler (IPTX).
 * -- UAIF / Control Port Interfaces
 * @param[in]  siUAIF_LsnReq  UDP open  port request from UdpAppInterface (UAIF).
 * @param[out] soUAIF_LsnRep  UDP open  port reply   to   [UAIF] (0=closed/1=opened).
 * @param[in]  siUAIF_ClsReq  UDP close port request from [UAIF].
 * @param[out] soUAIF_ClsRep  UDP close port reply   to   [UAIF] (0=closed/1=opened).
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
 *******************************************************************************/
#if HLS_VERSION == 2017
    void uoe_top(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                           piMMIO_En,
        stream<StsBool>                 &soMMIO_Ready,
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
        stream<UdpAppLsnReq>            &siUAIF_LsnReq,
        stream<UdpAppLsnRep>            &soUAIF_LsnRep,
        stream<UdpAppClsReq>            &siUAIF_ClsReq,
        stream<UdpAppClsRep>            &soUAIF_ClsRep,
        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &soUAIF_Data,
        stream<UdpAppMeta>              &soUAIF_Meta,
        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &siUAIF_Data,
        stream<UdpAppMeta>              &siUAIF_Meta,
        stream<UdpAppDLen>              &siUAIF_DLen,
        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisIcmp>                &soICMP_Data)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable          port=piMMIO_En         name=piMMIO_En

    #pragma HLS RESOURCE core=AXI4Stream variable=soMMIO_Ready      metadata="-bus_bundle soMMIO_Ready"

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data       metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_Data       metadata="-bus_bundle soIPTX_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_LsnReq     metadata="-bus_bundle siUAIF_LsnReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_LsnRep     metadata="-bus_bundle soUAIF_LsnRep"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_ClsReq     metadata="-bus_bundle siUAIF_ClsReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_ClsRep     metadata="-bus_bundle soUAIF_ClsRep"

    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_Data       metadata="-bus_bundle soUAIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soUAIF_Meta       metadata="-bus_bundle soUAIF_Meta"
    #pragma HLS DATA_PACK                variable=soUAIF_Meta

    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_Data       metadata="-bus_bundle siUAIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_Meta       metadata="-bus_bundle siUAIF_Meta"
    #pragma HLS DATA_PACK                variable=siUAIF_Meta
    #pragma HLS RESOURCE core=AXI4Stream variable=siUAIF_DLen       metadata="-bus_bundle siUAIF_DLen"

    #pragma HLS RESOURCE core=AXI4Stream variable=soICMP_Data       metadata="-bus_bundle soICMP_Data"

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- MAIN IPRX PROCESS -----------------------------------------------------
    uoe(
        //-- MMIO Interface
        piMMIO_En,
        soMMIO_Ready,
        //-- IPRX / IP Rx / Data Interface
        siIPRX_Data,
        //-- IPTX / IP Tx / Data Interface
        soIPTX_Data,
        //-- UAIF / Control Port Interfaces
        siUAIF_LsnReq,
        soUAIF_LsnRep,
        siUAIF_ClsReq,
        soUAIF_ClsRep,
        //-- UAIF / Rx Data Interfaces
        soUAIF_Data,
        soUAIF_Meta,
        //-- UAIF / Tx Data Interfaces
        siUAIF_Data,
        siUAIF_Meta,
        siUAIF_DLen,
        //-- ICMP / Message Data Interface (Port Unreachable)
        soICMP_Data);

}
#else
    void uoe_top(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                           piMMIO_En,
        stream<StsBool>                 &soMMIO_Ready,
        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisRaw>                 &siIPRX_Data,
        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisRaw>                 &soIPTX_Data,
        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>            &siUAIF_LsnReq,
        stream<UdpAppLsnRep>            &soUAIF_LsnRep,
        stream<UdpAppClsReq>            &siUAIF_ClsReq,
        stream<UdpAppClsRep>            &soUAIF_ClsRep,
        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &soUAIF_Data,
        stream<UdpAppMeta>              &soUAIF_Meta,
        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>              &siUAIF_Data,
        stream<UdpAppMeta>              &siUAIF_Meta,
        stream<UdpAppDLen>              &siUAIF_DLen,
        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisRaw>                 &soICMP_Data)
{

    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable             port=piMMIO_En         name=piMMIO_En

    #pragma HLS INTERFACE axis register both    port=soMMIO_Ready      name=soMMIO_Ready

    #pragma HLS INTERFACE axis off              port=siIPRX_Data       name=siIPRX_Data
    #pragma HLS INTERFACE axis register forward port=soIPTX_Data       name=soIPTX_Data

    #pragma HLS INTERFACE axis off              port=siUAIF_LsnReq     name=siUAIF_LsnReq
    #pragma HLS INTERFACE axis off              port=soUAIF_LsnRep     name=soUAIF_LsnRep
    #pragma HLS INTERFACE axis off              port=siUAIF_ClsReq     name=siUAIF_ClsReq
    #pragma HLS INTERFACE axis off              port=soUAIF_ClsRep     name=soUAIF_ClsRep

    #pragma HLS INTERFACE axis off              port=soUAIF_Data       name=soUAIF_Data
    #pragma HLS INTERFACE axis off              port=soUAIF_Meta       name=soUAIF_Meta
    #pragma HLS DATA_PACK                   variable=soUAIF_Meta   instance=soUAIF_Meta

    #pragma HLS INTERFACE axis off              port=siUAIF_Data       name=siUAIF_Data
    #pragma HLS INTERFACE axis off              port=siUAIF_Meta       name=siUAIF_Meta
    #pragma HLS DATA_PACK                   variable=siUAIF_Meta   instance=siUAIF_Meta
    #pragma HLS INTERFACE axis off              port=siUAIF_DLen       name=siUAIF_DLen

    #pragma HLS INTERFACE axis register forward port=soICMP_Data       name=soICMP_Data

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW disable_start_propagation

    //-- LOCAL INPUT and OUTPUT STREAMS ----------------------------------------
    static stream<AxisIp4>            ssiIPRX_Data;
    #pragma HLS STREAM       variable=ssiIPRX_Data  depth=32
    static stream<AxisIp4>            ssoIPTX_Data;
    static stream<AxisIcmp>           ssoICMP_Data;

    //-- INPUT STREAM CASTING --------------------------------------------------
    pAxisRawCast(siIPRX_Data, ssiIPRX_Data);

    //-- MAIN IPRX PROCESS -----------------------------------------------------
    uoe(
        //-- MMIO Interface
        piMMIO_En,
        soMMIO_Ready,
        //-- IPRX / IP Rx / Data Interface
        ssiIPRX_Data,
        //-- IPTX / IP Tx / Data Interface
        ssoIPTX_Data,
        //-- UAIF / Control Port Interfaces
        siUAIF_LsnReq,
        soUAIF_LsnRep,
        siUAIF_ClsReq,
        soUAIF_ClsRep,
        //-- UAIF / Rx Data Interfaces
        soUAIF_Data,
        soUAIF_Meta,
        //-- UAIF / Tx Data Interfaces
        siUAIF_Data,
        siUAIF_Meta,
        siUAIF_DLen,
        //-- ICMP / Message Data Interface (Port Unreachable)
        ssoICMP_Data);

    //-- OUTPUT STREAM CASTING ----------------------------
    pAxisRawCast(ssoIPTX_Data, soIPTX_Data);
    pAxisRawCast(ssoICMP_Data, soICMP_Data);

}
#endif  // HLS_VERSION

/*! \} */
