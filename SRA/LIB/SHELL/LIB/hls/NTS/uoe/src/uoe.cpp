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
 * @file       : udp.cpp
 * @brief      : UDP Offload Engine (UOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "uoe.hpp"
#include "../../toe/test/test_toe_utils.hpp"
#include "../../AxisPsd4.hpp"

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
#define TRACE_TDH  1 << 4
#define TRACE_UCC  1 << 5
#define TRACE_UPT  1 << 6
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


#define packetSize 16

ap_uint<4> countBits(ap_uint<8> bitVector) {
	ap_uint<4> bitCounter = 0;
	for (uint8_t i=0;i<8;++i) {
		if (bitVector.bit(i) == 1)
			bitCounter++;
	}
	return bitCounter;
}

ap_uint<8> length2keep_mapping(ap_uint<4> lengthValue) {
	ap_uint<8> keep = 0;
	for (uint8_t i=0;i<8;++i) {
		if (i < lengthValue)
			keep.bit(i) = 1;
	}
	return keep;
}

//OBSOLETE_20200312 ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
//OBSOLETE_20200312 	return (inputVector.range(7,0), inputVector(15, 8), inputVector(23, 16), inputVector(31, 24));
//OBSOLETE_20200312 }

//OBSOLETE_20200312 ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
//OBSOLETE_20200312 	return (inputVector.range(7,0), inputVector(15, 8));
//OBSOLETE_20200312 }



/******************************************************************************
 * UDP Port Table (Upt)
 *
 * param[in]  siRph_PortStateReq, Port state request from RxPacketHandler (Rph).
 * param[out] soRph_PortStateRep, Port state reply to [Rph].
 * param[in]  siURIF_LsnReq,      Request to open a port from [URIF].
 * param[out] soURIF_LsnRep,      Open port status reply to [URIF].
 * param[in]  siURIF_ClsReq,      Request to close a port from [URIF].
 *
 * @details
 *  The UDP Port Table (Upt) keeps track of the opened ports. A port is opened
 *  if its state is 'true' and closed othewise.
 *
 *****************************************************************************/
void pUdpPortTable(
        stream<UdpPort>     &siRph_PortStateReq,
        stream<StsBool>     &soRph_PortStateRep,
        stream<UdpPort>     &siURIF_LsnReq,
        stream<StsBool>     &soURIF_LsnRep,
        stream<UdpPort>     &siURIF_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/RXe/", "Upt");

    //-- STATIC ARRAYS --------------------------------------------------------
    static ValBool                  PORT_TABLE[65536];
    #pragma HLS RESOURCE   variable=PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=PORT_TABLE inter false

    if (!siRph_PortStateReq.empty()) {
        // Request to lookup the table from [UPh]
        UdpPort portNum;
        siRph_PortStateReq.read(portNum);
        soRph_PortStateRep.write(PORT_TABLE[portNum]);
    }
    else if (!siURIF_LsnReq.empty()) {
        // Request to open a port from [URIF]
        UdpPort portToOpen;
        siURIF_LsnReq.read(portToOpen);
        if (!PORT_TABLE[portToOpen]) {
            soURIF_LsnRep.write(STS_OPENED);
        }
        else {
            // Port is already opened
            soURIF_LsnRep.write(STS_OPENED);
        }
        PORT_TABLE[portToOpen] = STS_OPENED;
    }
    else if (!siURIF_ClsReq.empty()) {
        UdpPort portToRelease;
        siURIF_ClsReq.read(portToRelease);
        PORT_TABLE[portToRelease] = STS_CLOSED;
    }
}

/*****************************************************************************
 * Rx Packet Handler (Rph)
 *
 * @param[in]  siIhs_UdpDgrm, UDP datagram stream from IpHeaderStripper (Ihs).
 * @param[in]  siIhs_Ip4Hdr,  The header part of the IPv4 packet from [Ihs].
 * @param[in]  siUcc_CsumVal, Checksum valid information from UdpChecksumChecker (Ucc).
 * @param[out] soUpt_PortStateReq, Request for the state of port to UdpPortTable (Upt).
 * @param[in]  siUpt_PortStateRep, Port state reply from [Upt].
 * @param[out] soURIF_Data, UDP data streamto [URIF]
 * @param[out] soURIF_Meta, UDP metadata stream to [URIF].
 * @param[out] soICMP_Data, Control message to InternetControlMessageProtocol[ICMP] engine.

 * @details
 *  This process handles the payload of the incoming IP4 packet and forwards it
 *  the UdpRoleInterface (URIF).
 *
 *****************************************************************************/
void pRxPacketHandler(
        stream<AxisUdp>     &siUcc_UdpDgrm,
        stream<ValBool>     &siUcc_CsumVal,
        stream<AxisIp4>     &siIhs_Ip4Hdr,
        stream<UdpPort>     &soUpt_PortStateReq,
        stream<StsBool>     &siUpt_PortStateRep,
        stream<AxisApp>     &soURIF_Data,
        stream<SocketPair>  &soURIF_Meta,
        stream<AxiWord>     &soICMP_Data)
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
            !soURIF_Data.full()    && !soURIF_Meta.full()) {
            // Read the 3rd IPv4 header word, update and forward the metadata
            AxisIp4 thirdIp4HdrWord;
            siIhs_Ip4Hdr.read(thirdIp4HdrWord);
            rph_udpMeta.dst.addr = thirdIp4HdrWord.getIp4DstAddr();
            if (thirdIp4HdrWord.tlast) {
                rph_doneWithIpHdrStream = true;
            }
            AxisUdp dgrmWord(0, 0, 0);
            if (not rph_emptyPayloadFlag) {
                soURIF_Meta.write(rph_udpMeta);
                // Read the 1st datagram word and forward to [URIF]
                siUcc_UdpDgrm.read(dgrmWord);
                soURIF_Data.write(dgrmWord);
            }
            if (dgrmWord.tlast or rph_emptyPayloadFlag) {
                if (thirdIp4HdrWord.tlast) {
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
        if (!siUcc_UdpDgrm.empty() && !soURIF_Data.full() && !soURIF_Meta.full()) {
            // Forward datagram word
            AxisUdp dgrmWord;
            siUcc_UdpDgrm.read(dgrmWord);
            soURIF_Data.write(dgrmWord);
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_STREAM\n");
            }
            if (dgrmWord.tlast) {
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
                if (currIp4HdrWord.tlast) {
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
            if (currWord.tlast == 1) {
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
            if (currWord.tlast) {
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
            soICMP_Data.write(AxiWord(ip4Word.tdata, ip4Word.tkeep, 0));
            if (DEBUG_LEVEL & TRACE_RPH) {
                printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_STREAM -\n");
            }
            if (ip4Word.tlast) {
                rph_doneWithIpHdrStream = true;
                rph_fsmState = FSM_RPH_PORT_UNREACHABLE_LAST;
            }
        }
        break;
    case FSM_RPH_PORT_UNREACHABLE_LAST:
        if (DEBUG_LEVEL & TRACE_RPH) { printInfo(myName, "FSM_RPH_PORT_UNREACHABLE_LAST -\n"); }
        if (!soICMP_Data.full()) {
            // Forward the first 8 bytes of the datagram (.i.i the UDP header)
            soICMP_Data.write(AxiWord(rph_udpHeaderWord.tdata, rph_udpHeaderWord.tkeep, TLAST));
            rph_fsmState = FSM_RPH_DRAIN_DATAGRAM_STREAM;
        }
        break;
    } // End-of: switch()
}

/******************************************************************************
 * IPv4 Hedaer Stripper (Ihs)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[out] soUcc_UdpDgrm, UDP datagram stream to UdpChecksumChecker (Ucc).
 * @param[out] soUcc_PsdHdrSum, Sum of the pseudo header information to [Ucc].
 * @param[out] soRph_Ip4Hdr,  The header part of the IPv4 packet as a stream to [Rph].
 *
 * @details
 *  This process extracts the UDP pseudo header and the IP header from the
 *  incoming IPv4 packet. The IP header is forwarded to the RxPacketHandler (Rph) for
 *  further processing while the UDP datagram and the UDP pseudo-header are
 *  forwarded to the UDP checksum checker.
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
    AxisUdp     currUdpWord;

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
                soRph_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
            }
            else {  // ihs_ip4HdrLen > 2
                printWarn(myName, "This IPv4 packet contains 2+ option words! FYI, IPV4 options are not supported.\n");
                soRph_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, 0));
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
                soRph_Ip4Hdr.write(AxisIp4(currIp4Word.tdata(31, 0), 0x0F, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER;
            }
            else if (ihs_ip4HdrLen == 2 ) {
                printWarn(myName, "This IPv4 packet contains 4 option words!\n");
                soRph_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
            }
            else {  // ihs_ip4HdrLen > 2
                printWarn(myName, "This IPv4 packet contains 4+ option words!\n");
                soRph_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, 0));
                ihs_ip4HdrLen -= 2;
                ihs_fsmState = FSM_IHS_OPT;
            }
        }
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_OPT - \n"); }
        break;
    case FSM_IHS_UDP_HEADER:
        if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
            //-- READ End of un-aligned UDP header (Data|Csum|Len) --------
            siIPRX_Data.read(currIp4Word);
            //-- Csum accumulate (UdpLen)
            ihs_psdHdrSum += currIp4Word.getUdpLen();
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            //-- Forward UDP PseudoHeaderCsum to [Ucc]
            soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
            //-- Forward the UDP Header (Csum|Len|DP|SP)
            AxisUdp sendWord((currIp4Word.tdata(31, 0),ihs_prevWord.tdata(63, 32)),
                              0xFF, (currIp4Word.tlast and (currIp4Word.tkeep == 0x0F)));
            soUcc_UdpDgrm.write(sendWord);
            if (currIp4Word.tlast) {
                if (currIp4Word.tkeep == 0x0F) {
                    printWarn(myName, "Received a UDP datagram of length = 0!\n");
                    ihs_fsmState = FSM_IHS_IPW0;
                }
                else {
                    ihs_fsmState = FSM_IHS_RESIDUE;
                }
            }
            else {
                ihs_fsmState = FSM_IHS_FORWARD;
            }
        }
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER - \n"); }
        break;
    case FSM_IHS_UDP_HEADER_ALIGNED:
        if (!siIPRX_Data.empty() && !soUcc_UdpDgrm.full()) {
            //-- READ Aligned UDP header (Csum|Len|DP|SP) --------
            siIPRX_Data.read(currIp4Word);
            //-- Cast incoming word into an AxisUdp word
            currUdpWord.tdata = currIp4Word.tdata;
            currUdpWord.tkeep = currIp4Word.tkeep;
            currUdpWord.tlast = currIp4Word.tlast;
            //-- Csum accumulate (UdpLen)
            ihs_psdHdrSum += currUdpWord.getUdpLen();
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            //-- Forward UDP PseudoHeaderCsum to [Ucc]
            soUcc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
            //-- Forward the UDP Header (Csum|Len|DP|SP)
            soUcc_UdpDgrm.write(currUdpWord);
            if (currUdpWord.tlast) {
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
            sendWord.tdata(63, 32) = currIp4Word.tdata(31, 0);
            sendWord.tkeep( 7,  4) = currIp4Word.tkeep( 3, 0);
            sendWord.tdata(31,  0) = ihs_prevWord.tdata(63, 32);
            sendWord.tkeep( 3,  0) = ihs_prevWord.tkeep( 7,  4);
            if (currIp4Word.tlast) {
                if (currIp4Word.tkeep <= 0x0F) {
                    sendWord.tlast = TLAST;
                    ihs_fsmState = FSM_IHS_IPW0;
                }
                else {
                    sendWord.tlast = 0;
                    ihs_fsmState = FSM_IHS_RESIDUE;
                }
            }
            else {
                sendWord.tlast = 0;
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
            currUdpWord.tdata = currIp4Word.tdata;
            currUdpWord.tkeep = currIp4Word.tkeep;
            currUdpWord.tlast = currIp4Word.tlast;
            soUcc_UdpDgrm.write(currUdpWord);
            if (currUdpWord.tlast) {
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
            AxisUdp sendWord(ihs_prevWord.tdata(63, 32), (ihs_prevWord.tkeep >> 4), TLAST);
           soUcc_UdpDgrm.write(sendWord);
           ihs_fsmState = FSM_IHS_IPW0;
        }
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_RESIDUE - \n"); }
        break;
    case FSM_IHS_DROP:
        if (!siIPRX_Data.empty()) {
            //-- READ and DRAIN all AXI-WORDS -------------
            siIPRX_Data.read(currIp4Word);
            if (currIp4Word.tlast) {
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
                // Accumulate the UDP header
                ucc_csum[0] = 0x00000 | currWord.getUdpSrcPort();
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
            if (DEBUG_LEVEL & TRACE_UCC) {
                printInfo(myName,"FSM_UCC_IDLE - UdpLen=%d\n", currWord.getUdpLen().to_ushort());
            }
        }
        break;
    case FSM_UCC_STREAM:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_STREAM - \n"); }
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currWord);
            soRph_UdpDgrm.write(currWord);
            if (currWord.tlast) {
                ucc_fsmState = FSM_UCC_IDLE;
            }
        }
        break;
    case FSM_UCC_ACCUMULATE:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_ACCUMULATE - \n"); }
        if (!siIhs_UdpDgrm.empty()) {
            siIhs_UdpDgrm.read(currWord);
            soRph_UdpDgrm.write(currWord);
            ucc_csum[0] += byteSwap16(currWord.tdata.range(63, 48));
            ucc_csum[0]  = (ucc_csum[0] & 0xFFFF) + (ucc_csum[0] >> 16);
            ucc_csum[1] += byteSwap16(currWord.tdata.range(47, 32));
            ucc_csum[1]  = (ucc_csum[1] & 0xFFFF) + (ucc_csum[1] >> 16);
            ucc_csum[2] += byteSwap16(currWord.tdata.range(31, 16));
            ucc_csum[2]  = (ucc_csum[2] & 0xFFFF) + (ucc_csum[2] >> 16);
            ucc_csum[3] += byteSwap16(currWord.tdata.range(15,  0));
            ucc_csum[3]  = (ucc_csum[3] & 0xFFFF) + (ucc_csum[3] >> 16);
            if (currWord.tlast) {
                ucc_fsmState = FSM_UCC_CHK0;
            }
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
        ucc_csum[0] += ucc_psdHdrCsum;
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


/******************************************************************************
 * Rx Engine (RXe)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[in]  siURIF_LsnReq, UDP open port request from UdpRoleInterface (URIF).
 * @param[out] soURIF_LsnRep, UDP open port reply to [URIF].
 * @param[in]  siURIF_ClsReq, UDP close port request from [URIF].
 * @param[out] soURIF_Data,   UDP data stream to [URIF].
 * @param[out] soURIF_Meta,   UDP metadata stream to [URIF].
 * @param[out] soICMP_Data,   Control message to InternetControlMessageProtocol[ICMP] engine.
 *
 * @details
 *  The Rx path of the UdpOffloadEngine (UOE). This is the path from [IPRX]
 *  to the UdpRoleInterface (URIF) a.k.a the Application [APP].
 *****************************************************************************/
void pRxEngine(
        stream<AxisIp4>         &siIPRX_Data,
        stream<UdpPort>         &siURIF_LsnReq,
        stream<StsBool>         &siURIF_OpnRep,
        stream<UdpPort>         &siURIF_ClsReq,
        stream<AxisApp>         &soURIF_Data,
        stream<SocketPair>      &soURIF_Meta,
        stream<AxiWord>         &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- IP Header Stripper (Ihs)
    static stream<AxisIp4>      ssIhsToRph_Ip4Hdr       ("ssIhsToRph_Ip4Hdr");
    #pragma HLS STREAM variable=ssIhsToRph_Ip4Hdr       depth=32
    //OBSOLETE_20200311 static stream<IpAddrPair>   ssIhsToRph_AddrPair     ("ssIhsToRph_AddrPair");
    //OBSOLETE_20200311 #pragma HLS STREAM variable=ssIhsToRph_AddrPair     depth=8
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
    static stream<UdpPort>      ssUPhToUpt_PortStateReq ("ssUPhToUpt_PortStateReq");
    #pragma HLS STREAM variable=ssUPhToUpt_PortStateReq depth=2

    //-- UDP Port Table (Upt)
    static stream<StsBool>      ssUptToUPh_PortStateRep ("ssUptToUPh_PortStateRep");
    #pragma HLS STREAM variable=ssUptToUPh_PortStateRep depth=2

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
            ssUPhToUpt_PortStateReq,
            ssUptToUPh_PortStateRep,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    pUdpPortTable(
            ssUPhToUpt_PortStateReq,
            ssUptToUPh_PortStateRep,
            siURIF_LsnReq,
            siURIF_OpnRep,
            siURIF_ClsReq);
}

/******************************************************************************
 * Tx Datagram Handler (Tdh)
 *
 * @param[in]  siURIF_Data Data stream from UserRoleInterface (URIF).
 * @param[in]  siURIF_Meta  Metadata stream from [URIF].
 * @param[in]  siURIF_DLen  Data payload length from [URIF].
 * @param[out] soUha_Data   Data stream to UdpHeaderAdder (Uha).
 * @param[out] soUha_Meta   Metadata stream to [Uha].
 * @param[out] soUha_DLen   Data payload length to [Uha].
 * @param[out] soUca_Data   UDP data stream to UdpChecksumAccumulator (Uca).
 *
 * @details
 *  This process handles the raw data coming from the UdpRoleInterface (URIF).
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
        stream<AxisApp>         &siURIF_Data,
        stream<UdpAppMeta>      &siURIF_Meta,
        stream<UdpAppDLen>      &siURIF_DLen,
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
            if (!siURIF_Meta.empty() && !soUca_Data.full()) {
                siURIF_Meta.read(tdh_udpMeta);
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
            if (!siURIF_DLen.empty() && !soUca_Data.full()) {
                siURIF_DLen.read(tdh_appLen);
                // Generate UDP length from incoming payload length
                tdh_udpLen = tdh_appLen + UDP_HEADER_SIZE;
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
            if (!siURIF_Data.empty() && !soUca_Data.full()) {
                // Read 1st data payload word
                siURIF_Data.read(tdh_currWord);
                // Generate 3rd pseudo-packet word [Data|Csum|Len]
                AxisPsd4 thirdPseudoPktWord(0, 0xFF, 0);
                thirdPseudoPktWord.setUdpLen(tdh_udpLen);
                thirdPseudoPktWord.setUdpCsum(0x0000);
                thirdPseudoPktWord.setUdpDataHi(tdh_currWord.getTData());
                if (tdh_currWord.tlast) {
                    soUha_Data.write(tdh_currWord);
                    if (tdh_currWord.tkeep.range(7, 4) == 0) {
                        // Done. Payload <= 4 bytes fits into current pseudo-pkt word
                        tdh_fsmState = FSM_TDH_PSD_PKT1;
                        thirdPseudoPktWord.setTLast(TLAST);
                    }
                    else {
                        // Needs an additional pseudo-pkt-word for remaing 1-4 bytes
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
                fourthPseudoPktWord.setUdpDataLo(tdh_currWord.getTData());
                fourthPseudoPktWord.setTKeep(tdh_currWord.getTKeep() >> 4);
                soUca_Data.write(fourthPseudoPktWord);
                tdh_fsmState = FSM_TDH_PSD_PKT1;
            }
            if (DEBUG_LEVEL & TRACE_TDH) { printInfo(myName, "FSM_TDH_PSD_PKT4\n"); }
            break;
        case FSM_TDH_STREAM:
            // This state streams all the payload data into both the checksum calculation stage and the next stage, reformatting them as required
            if (!siURIF_Data.empty() && !soUha_Data.full() && !soUca_Data.full()) {
                // Always forward the current AppData word to next-stage [Uha]
                soUha_Data.write(tdh_currWord);
                // Save previous AppData and read a new one from [URIF]
                AxisPsd4 currPseudoWord(0, 0xFF, 0);
                AxisApp  prevAppDataWord(tdh_currWord.tdata, tdh_currWord.tkeep, tdh_currWord.tlast);
                siURIF_Data.read(tdh_currWord);
                // Generate new pseudo-packet word
                currPseudoWord.setUdpDataLo(prevAppDataWord.getTData());
                currPseudoWord.setUdpDataHi(tdh_currWord.getTData());
                if (tdh_currWord.tlast) {
                    if (tdh_currWord.tkeep.bit(4) == 1) {
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
                // Generate the last pseudo-packet word and forard it to [Uca]
                AxisPsd4 lastPseudoPktWord(0, 0x00, TLAST);
                lastPseudoPktWord.setUdpDataLo(tdh_currWord.getTData());
                lastPseudoPktWord.setTKeep(tdh_currWord.getTKeep() >> 4);
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
    #pragma HLS RESET   variable=ica_csum

    if (!siTdh_Data.empty()) {
        //OBSOLETE_20200408 ioWord inputWord = dataIn.read();
        //OBSOLETE_20200408 udpChecksum = ((((udpChecksum + inputWord.data.range(63, 48)) +
        //OBSOLETE_20200408                 inputWord.data.range(47, 32)) +
        //OBSOLETE_20200408                 inputWord.data.range(31, 16)) +
        //OBSOLETE_20200408                 inputWord.data.range(15, 0));
        //OBSOLETE_20200408if (inputWord.eop) {
        //OBSOLETE_20200408		udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
        //OBSOLETE_20200408		udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
        //OBSOLETE_20200408		udpChecksum = ~udpChecksum;			// Reverse the bits of the result
        //OBSOLETE_20200408		udpChecksumOut.write(udpChecksum.range(15, 0));	// and write it into the output
        //OBSOLETE_20200408		udpChecksum = 0;
        //OBSOLETE_20200408}
        AxisPsd4 currWord = siTdh_Data.read();
        uca_csum[0] += byteSwap16(currWord.tdata.range(63, 48));
        uca_csum[0]  = (uca_csum[0] & 0xFFFF) + (uca_csum[0] >> 16);
        uca_csum[1] += byteSwap16(currWord.tdata.range(47, 32));
        uca_csum[1]  = (uca_csum[1] & 0xFFFF) + (uca_csum[1] >> 16);
        uca_csum[2] += byteSwap16(currWord.tdata.range(31, 16));
        uca_csum[2]  = (uca_csum[2] & 0xFFFF) + (uca_csum[2] >> 16);
        uca_csum[3] += byteSwap16(currWord.tdata.range(15,  0));
        uca_csum[3]  = (uca_csum[3] & 0xFFFF) + (uca_csum[3] >> 16);
        if (currWord.tlast) {
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
        if (!siTdh_DLen.empty() && !siTdh_Meta.empty() && !siUca_Csum.empty() ) {
                // Read data payload length
                siTdh_DLen.read(uha_appDLen);
                UdpLen  udpLen = uha_appDLen + UDP_HEADER_SIZE;
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
        }
        break;
    case UHA_STREAM:
        if (!siTdh_Data.empty()) {
            AxisApp currAppData = siTdh_Data.read();
            if (uha_appDLen > 8) {
                uha_appDLen -= 8;
                if (currAppData.getTLast() == TLAST) {
                    printWarn(myName, "Malformed - TLAST bit is set but end of Axis stream is not reached !!!\n");
                    currAppData.tkeep = length2keep_mapping(static_cast <uint16_t>(uha_appDLen));
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
                currAppData.tkeep = length2keep_mapping(static_cast <uint16_t>(uha_appDLen));
                uha_appDLen = 0;
                uha_fsmState = UHA_IDLE;
            }
            soIha_Data.write(AxisUdp(currAppData.tdata, currAppData.tkeep, currAppData.tlast));
        }
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
        if(!siUha_Data.empty() && !siUha_IpPair.empty() &&!siUha_UdpLen.empty() && !soIPTX_Data.full()) {
            UdpDgmLen udpLen = siUha_UdpLen.read();
            Ip4PktLen ip4Len = udpLen + IP4_HEADER_SIZE;
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
        if(!siUha_Data.empty() && !siUha_IpPair.empty() && !soIPTX_Data.full()) {
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
        if(!siUha_Data.empty() && !soIPTX_Data.full()) {
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
            forwardIp4Word.setTDataLo(iha_prevUdpWord.getTData());
            forwardIp4Word.setTDataHi(currUdpWord.getTData());
            if(currUdpWord.tlast) {
                if (currUdpWord.tkeep.range(7, 4) != 0) {
                    iha_fsmState = IPH_RESIDUE;
                }
                else {
                    forwardIp4Word.setTKeep((currUdpWord.getTKeep() << 4) | 0x0F);
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
            //OBSOLETE AxisIp4 tempWord = AxisIp4(currUdpWord.tdata.range(63, 32), currUdpWord.tkeep.range(7, 4), 1);
            AxisIp4 forwardIp4Word = AxisIp4(0x0, 0x00, TLAST);
            forwardIp4Word.setTDataLo(iha_prevUdpWord.getTData());
            forwardIp4Word.setTKeep(iha_prevUdpWord.getTKeep() >> 4);
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
 * @param[in]  siURIF_Data, Data stream from UserRoleInterface (URIF).
 * @param[in]  siURIF_Meta, Metadata stream from [URIF].
 * @param[in]  siURIF_DLen, Data length from [URIF].
 * @param[out] soIPTX_Data, Data stream to IpTxHandler (IPTX).
 *
 * @details
 *  The Tx path of the UdpOffloadEngine (UOE). This is the path from the
 *  UdpRoleInterface (URIF) a.k.a the Application [APP] to the [IPRX].
 *****************************************************************************/
void pTxEngine(
        stream<AxisApp>         &siURIF_Data,
        stream<UdpAppMeta>      &siURIF_Meta,
        stream<UdpAppDLen>      &siURIF_DLen,
        stream<AxisIp4>         &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    const char *myName  = concat3(THIS_NAME, "/", "TXe");

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tx Datagram Handler (Tdh)
    static stream<AxisApp>         ssTdhToUha_Data    ("ssTdhToUha_Data");
    #pragma HLS STREAM    variable=ssTdhToUha_Data    depth=4096
    static stream<UdpAppDLen>      ssTdhToUha_DLen    ("ssTdhToUha_DLen");
    #pragma HLS STREAM    variable=ssTdhToUha_DLen    depth=8
    static stream<UdpAppMeta>      ssTdhToUha_Meta    ("ssTdhToUha_Meta");
    #pragma HLS STREAM    variable=ssTdhToUha_Meta    depth=8
    #pragma HLS DATA_PACK variable=ssTdhToUha_Meta
    static stream<AxisPsd4>        ssTdhToUca_Data    ("ssTdhToUca_Data");
    #pragma HLS STREAM    variable=ssTdhToUca_Data    depth=32

    // UdpChecksumAccumulator (Uca)
    static stream<UdpCsum>         ssUcaToUha_Csum    ("ssUcaToUha_Csum");
    #pragma HLS STREAM    variable=ssUcaToUha_Csum    depth=8

    // UdpHeaderAdder (Uha)
    static stream<AxisUdp>         ssUhaToIha_Data    ("ssUhaToIha_Data");
    #pragma HLS STREAM    variable=ssUhaToIha_Data    depth=2048
    static stream<UdpLen>          ssUhaToIha_UdpLen  ("ssUhaToIha_UdpLen");
    #pragma HLS STREAM    variable=ssUhaToIha_Udplen  depth=4
    static stream<IpAddrPair>      ssUhaToIha_IpPair  ("ssUhaToIha_IpPair");
    #pragma HLS STREAM    variable=ssUhaToIha_IpPair  depth=4
    #pragma HLS DATA_PACK variable=ssUhaToIha_IpPair

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTxDatagramHandler(
            siURIF_Data,
            siURIF_Meta,
            siURIF_DLen,
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

/*****************************************************************************/
/* @brief 	Main process of the UDP Offload Engine (UOE).
 *
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * -- IPTX / IP Tx / Data Interface
 * @param[out] soIPTX_Data,   IP4 data stream to IpTxHandler (IPTX).
 * -- URIF / Control Port Interfaces
 * @param[in]  siURIF_LsnReq, UDP open port request from UdpRoleInterface (URIF).
 * @param[out] soURIF_LsnRep, UDP open port reply to [URIF].
 * @param[in]  siURIF_ClsReq, UDP close port request from [URIF].
 * -- URIF / Rx Data Interfaces
 * @param[out] soURIF_Data,   UDP data stream to [URIF].
 * @param[out] soURIF_Meta,   UDP metadata stream to [URIF].
 * -- URIF / Tx Data Interfaces
 * @param[in]  siURIF_Data,   UDP data stream from [URIF].
 * @param[in]  siURIF_Meta,   UDP metadata stream from [URIF].
 * @param[in]  siURIF_DLen,   UDP data length form [URIF].
 * -- ICMP / Message Data Interface
 * @param[out] soICMP_Data,   Data stream to [ICMP].
 *
 *****************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                 &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                 &soIPTX_Data,

        //------------------------------------------------------
        //-- URIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>                 &siURIF_LsnReq,
        stream<StsBool>                 &soURIF_LsnRep,
        stream<UdpPort>                 &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &soURIF_Data,
        stream<UdpAppMeta>              &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &siURIF_Data,
        stream<UdpAppMeta>              &siURIF_Meta,
        stream<UdpAppDLen>              &siURIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxiWord>                 &soICMP_Data)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data       metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_Data       metadata="-bus_bundle soIPTX_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_LsnReq     metadata="-bus_bundle siURIF_LsnReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soURIF_LsnRep     metadata="-bus_bundle soURIF_LsnRep"
    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_ClsReq     metadata="-bus_bundle siURIF_ClsReq"

    #pragma HLS RESOURCE core=AXI4Stream variable=soURIF_Data       metadata="-bus_bundle soURIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soURIF_Meta       metadata="-bus_bundle soURIF_Meta"
    #pragma HLS DATA_PACK                variable=soURIF_Meta

    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_Data       metadata="-bus_bundle siURIF_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_Meta       metadata="-bus_bundle siURIF_Meta"
    #pragma HLS DATA_PACK                variable=siURIF_Meta
    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_PLen       metadata="-bus_bundle siURIF_PLen"

    #pragma HLS RESOURCE core=AXI4Stream variable=soICMP_Data       metadata="-bus_bundle soICMP_Data"

#else

    #pragma HLS INTERFACE axis               port=siIPRX_Data       name=siIPRX_Data
    #pragma HLS INTERFACE axis               port=soIPTX_Data       name=soIPTX_Data

    #pragma HLS INTERFACE axis               port=siURIF_LsnReq     name=siURIF_LsnReq
    #pragma HLS INTERFACE axis               port=soURIF_LsnRep     name=soURIF_LsnRep
    #pragma HLS INTERFACE axis               port=siURIF_ClsReq     name=siURIF_ClsReq

    #pragma HLS INTERFACE axis               port=soURIF_Data       name=soURIF_Data
    #pragma HLS INTERFACE axis               port=soURIF_Meta       name=soURIF_Meta
    #pragma HLS DATA_PACK                    port=soURIF_Meta

    #pragma HLS INTERFACE axis               port=siURIF_Data       name=siURIF_Data
    #pragma HLS INTERFACE axis               port=siURIF_Meta       name=siURIF_Meta
    #pragma HLS DATA_PACK                    port=siURIF_Meta
    #pragma HLS INTERFACE axis               port=siURIF_PLen       name=siURIF_PLen

    #pragma HLS INTERFACE axis               port=soICMP_Data       name=soICMP_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    pRxEngine(
            siIPRX_Data,
            siURIF_LsnReq,
            soURIF_LsnRep,
            siURIF_ClsReq,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    pTxEngine(
            siURIF_Data,
            siURIF_Meta,
            siURIF_DLen,
            soIPTX_Data);
}
