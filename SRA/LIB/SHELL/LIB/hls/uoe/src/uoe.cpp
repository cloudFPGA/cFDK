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
 * @brief      : UDP offload engine.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This process implements the UDP core engine.
 *
 * @note       : { text }
 * @remark     : { remark text }
 * @warning    : { warning message }
 * @todo       : { paragraph describing what is to be done }
 *
 * @see        : https://www.stack.nl/~dimitri/doxygen/manual/commands.html
 *
 *****************************************************************************/

#include "uoe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

using namespace hls;

#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPd | TRACE_IBUF)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "UDP"

#define TRACE_OFF  0x0000
#define TRACE_IHS  1 << 1
#define TRACE_RPH  1 << 2
#define TRACE_UCC  1 << 3
#define TRACE_UPT  1 << 4
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
 * UDP Port Table (UPt)
 *
 * param[in]  siUPh_PortStateReq, Port state request from UdpPacketHandler (UPh).
 * param[out] soUPh_PortStateRep, Port state reply to [UPh].
 * param[in]  siURIF_OpnReq,      Request to open a port from [URIF].
 * param[out] soURIF_OPnRep,      Open port status reply to [URIF].
 * param[in]  siURIF_ClsReq,      Request to close a port from [URIF].
 *
 * @details
 *  The UDP Port Table (UPt) keeps track of the opened ports. A port is opened
 *  if its state is 'true' and closed othewise.
 *
 *****************************************************************************/
void pUdpPortTable(
        stream<UdpPort>     &siUPh_PortStateReq,
        stream<StsBool>     &soUPh_PortStateRep,
        stream<UdpPort>     &siURIF_OpnReq,
        stream<StsBool>     &siURIF_OpnRep,
        stream<UdpPort>     &siURIF_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush

    const char *myName = concat3(THIS_NAME, "/", "UPt");

    //-- STATIC ARRAYS --------------------------------------------------------
    static ValBool                  PORT_TABLE[65536];
    #pragma HLS RESOURCE   variable=PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=PORT_TABLE inter false

    if (!siUPh_PortStateReq.empty()) {
        // Request to lookup the table from [UPh]
        UdpPort portNum;
        siUPh_PortStateReq.read(portNum);
        soUPh_PortStateRep.write(PORT_TABLE[portNum]);
    }
    else if (!siURIF_OpnReq.empty()) {
        // REquest to open a port from [URIF]
        UdpPort portToOpen;
        siURIF_OpnReq.read(portToOpen);
        if (!PORT_TABLE[portToOpen]) {
            siURIF_OpnRep.write(STS_OPENED);
        }
        else {
            // Port is already opened [FIXME-Should we toggle the state?]
            siURIF_OpnRep.write(false);
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
 * Rx Packet Handler (RPh)
 *
 * @param[in]  siIHs_UdpDgrm, UDP datagram stream from IpHeaderStripper (IHs).
 * @param[in]  siIHs_Ip4Hdr,  The header part of the IPv4 packet from [IHs].
 * @param[in]  siUCc_CsumVal, Checksum valid information from UdpChecksumChecker (UCc).
 * @param[out] soUPt_PortStateReq, Request for the state of port to UdpPortTable (UPt).
 * @param[in]  siUPt_PortStateRep, Port state reply from [UPt].
 * @param[out] soURIF_Data, UDP data streamto [URIF]
 * @param[out] soURIF_Meta, UDP metadata stream to [URIF].
 * @param[out] soICMP_Data, Control message to InternetControlMessageProtocol[ICMP] engine.

 * @details
 *  This process handles the payload of the incoming IP4 packet and forwards it
 *  the UdpRoleInterface (URIF).
 *
 *****************************************************************************/
void pRxPacketHandler(
        stream<AxisUdp>     &siUCc_UdpDgrm,
        stream<ValBool>     &siUCc_CsumVal,
        stream<AxisIp4>     &siIHs_Ip4Hdr,
        stream<UdpPort>     &soUPt_PortStateReq,
        stream<StsBool>     &siUPt_PortStateRep,
        stream<AxisApp>     &soURIF_Data,
        stream<SocketPair>  &soURIF_Meta,
        stream<AxiWord>     &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "RPh");

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
        if (!siUCc_UdpDgrm.empty() && !siIHs_Ip4Hdr.empty() &&
            !soUPt_PortStateReq.full()) {
            rph_doneWithIpHdrStream = false;
            // Read the 1st IPv4 header word and retrieve the IHL
            siIHs_Ip4Hdr.read(rph_1stIp4HdrWord);
            // Read the the 1st datagram word
            siUCc_UdpDgrm.read(rph_udpHeaderWord);
            // Request the state of this port to UdpPortTable (UPt)
            soUPt_PortStateReq.write(rph_udpHeaderWord.getUdpDstPort());
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
        if (!siUPt_PortStateRep.empty() && !siUCc_CsumVal.empty() && !siIHs_Ip4Hdr.empty()) {
            bool csumResult = siUCc_CsumVal.read();
            bool portLkpRes = siUPt_PortStateRep.read();
            // Read the 2nd IPv4 header word and update the metadata structure
            siIHs_Ip4Hdr.read(rph_2ndIp4HdrWord);
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
        if (!siUCc_UdpDgrm.empty() && !siIHs_Ip4Hdr.empty() &&
            !soURIF_Data.full()    && !soURIF_Meta.full()) {
            // Read the 3rd IPv4 header word, update and forward the metadata
            AxisIp4 thirdIp4HdrWord;
            siIHs_Ip4Hdr.read(thirdIp4HdrWord);
            rph_udpMeta.dst.addr = thirdIp4HdrWord.getIp4DstAddr();
            if (thirdIp4HdrWord.tlast) {
                rph_doneWithIpHdrStream = true;
            }
            AxisUdp dgrmWord(0, 0, 0);
            if (not rph_emptyPayloadFlag) {
                soURIF_Meta.write(rph_udpMeta);
                // Read the 1st datagram word and forward to [URIF]
                siUCc_UdpDgrm.read(dgrmWord);
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
        if (!siUCc_UdpDgrm.empty() && !soURIF_Data.full() && !soURIF_Meta.full()) {
            // Forward datagram word
            AxisUdp dgrmWord;
            siUCc_UdpDgrm.read(dgrmWord);
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
            else if (!siIHs_Ip4Hdr.empty() && not rph_doneWithIpHdrStream) {
                // Drain any pending IPv4 header word
                AxisIp4 currIp4HdrWord;
                siIHs_Ip4Hdr.read(currIp4HdrWord);
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
        if (!siUCc_UdpDgrm.empty()) {
            AxisUdp currWord;
            siUCc_UdpDgrm.read(currWord);
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
        if (!siIHs_Ip4Hdr.empty()) {
            AxisIp4 currWord;
            siIHs_Ip4Hdr.read(currWord);
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
        if (!siIHs_Ip4Hdr.empty() && !soICMP_Data.full()) {
            // Forward remaining of the IPv4 header words
            AxisIp4 ip4Word;
            siIHs_Ip4Hdr.read(ip4Word);
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
 * IPv4 Hedaer Stripper (IHs)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[out] soUCc_UdpDgrm, UDP datagram stream to UdpChecksumChecker (UCc).
 * @param[out] soUCc_PsdHdrSum, Sum of the pseudo header information to [UCc].
 * @param[out] soRPh_Ip4Hdr,  The header part of the IPv4 packet as a stream to [RPh].
 *
 * @details
 *  This process extracts the UDP pseudo header and the IP header from the
 *  incoming IPv4 packet. The IP header is forwarded to the RxPacketHandler (RPh) for
 *  further processing while the UDP datagram and the UDP pseudo-header are
 *  forwarded to the UDP checksum checker.
 *
 *****************************************************************************/
void pIpHeaderStripper(
        stream<AxisIp4>      &siIPRX_Data,
        stream<AxisUdp>      &soUCc_UdpDgrm,
        stream<UdpCsum>      &soUCc_PsdHdrSum,
        stream<AxisIp4>      &soRPh_Ip4Hdr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IHs");

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
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 1st AXI-WORD (Frag|Flags|Id|TotLen|ToS|Ver|IHL) ---------
            siIPRX_Data.read(currIp4Word);
            ihs_ip4HdrLen = currIp4Word.getIp4HdrLen();
            if (ihs_ip4HdrLen < 5) {
                printWarn(myName, "FSM_IHS_IPW0 - Received an IPv4 packet with invalid IHL. This packet will be dropped.\n");
                ihs_fsmState  = FSM_IHS_DROP;
            }
            else {
                soRPh_Ip4Hdr.write(currIp4Word);
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
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_IPW1 - \n"); }
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 2nd AXI_WORD (SA|HdrCsum|Prot|TTL)
            siIPRX_Data.read(currIp4Word);
            soRPh_Ip4Hdr.write(currIp4Word);
            //-- Csum accumulate (SA+Prot)
            ihs_psdHdrSum  = (0x00, currIp4Word.getIp4Prot());
            ihs_psdHdrSum += currIp4Word.getIp4SrcAddr().range(31,16);
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            ihs_psdHdrSum += currIp4Word.getIp4SrcAddr().range(15, 0);
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            ihs_ip4HdrLen -= 2;
            ihs_fsmState = FSM_IHS_IPW2;
        }
        break;
    case FSM_IHS_IPW2:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_IPW2 - \n"); }
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 3rd AXI-WORD (DP|SP|DA) or (Opt|DA)
            siIPRX_Data.read(currIp4Word);
            //-- Csum accumulate (DA)
            ihs_psdHdrSum += currIp4Word.getIp4DstAddr().range(31,16);
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            ihs_psdHdrSum += currIp4Word.getIp4DstAddr().range(15, 0);
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            if (ihs_ip4HdrLen == 1) {
                // This a typical IPv4 header with a length of 20 bytes (5*4).
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.getLE_Ip4DstAddr(), 0x0F, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER;
            }
            else if (ihs_ip4HdrLen == 2 ) {
                printWarn(myName, "This IPv4 packet contains 1 option word! FYI, IPV4 options are not supported.\n");
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
            }
            else {  // ihs_ip4HdrLen > 2
                printWarn(myName, "This IPv4 packet contains 2+ option words! FYI, IPV4 options are not supported.\n");
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, 0));
                ihs_ip4HdrLen -= 2;
                ihs_fsmState = FSM_IHS_OPT;
            }
        }
        break;
    case FSM_IHS_OPT:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_OPT - \n"); }
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            printWarn(myName, "This IPv4 packet contains options! FYI, IPV4 options are not supported and will be dropped.\n");
            //-- READ more Options (OPT|Opt) and/or Data (Data|Opt)
            siIPRX_Data.read(currIp4Word);
            if (ihs_ip4HdrLen == 1) {
                printWarn(myName, "This IPv4 packet contains 3 option words!\n");
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.tdata(31, 0), 0x0F, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER;
            }
            else if (ihs_ip4HdrLen == 2 ) {
                printWarn(myName, "This IPv4 packet contains 4 option words!\n");
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, TLAST));
                ihs_fsmState = FSM_IHS_UDP_HEADER_ALIGNED;
            }
            else {  // ihs_ip4HdrLen > 2
                printWarn(myName, "This IPv4 packet contains 4+ option words!\n");
                soRPh_Ip4Hdr.write(AxisIp4(currIp4Word.tdata, currIp4Word.tkeep, 0));
                ihs_ip4HdrLen -= 2;
                ihs_fsmState = FSM_IHS_OPT;
            }
        }
        break;
    case FSM_IHS_UDP_HEADER:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER - \n"); }
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ End of un-aligned UDP header (Data|Csum|Len) --------
            siIPRX_Data.read(currIp4Word);
            //-- Csum accumulate (UdpLen)
            ihs_psdHdrSum += currIp4Word.getUdpLen();
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            //-- Forward UDP PseudoHeaderCsum to [UCc]
            soUCc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
            //-- Forward the UDP Header (Csum|Len|DP|SP)
            AxisUdp sendWord((currIp4Word.tdata(31, 0),ihs_prevWord.tdata(63, 32)),
                              0xFF, (currIp4Word.tlast and (currIp4Word.tkeep == 0x0F)));
            soUCc_UdpDgrm.write(sendWord);
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
        break;
    case FSM_IHS_UDP_HEADER_ALIGNED:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_UDP_HEADER_ALIGNED - \n"); }
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ Aligned UDP header (Csum|Len|DP|SP) --------
            siIPRX_Data.read(currIp4Word);
            //-- Cast incoming word into an AxisUdp word
            currUdpWord.tdata = currIp4Word.tdata;
            currUdpWord.tkeep = currIp4Word.tkeep;
            currUdpWord.tlast = currIp4Word.tlast;
            //-- Csum accumulate (UdpLen)
            ihs_psdHdrSum += currUdpWord.getUdpLen();
            ihs_psdHdrSum  = (ihs_psdHdrSum & 0xFFFF) + (ihs_psdHdrSum >> 16);
            //-- Forward UDP PseudoHeaderCsum to [UCc]
            soUCc_PsdHdrSum.write(ihs_psdHdrSum(15, 0));
            //-- Forward the UDP Header (Csum|Len|DP|SP)
            soUCc_UdpDgrm.write(currUdpWord);
            if (currUdpWord.tlast) {
                printWarn(myName, "Received a UDP datagram of length = 0!\n");
                ihs_fsmState = FSM_IHS_IPW0;
            }
            else {
                ihs_fsmState = FSM_IHS_FORWARD_ALIGNED;
            }
        }
        break;
    case FSM_IHS_FORWARD:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_FORWARD - \n"); }
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
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
            soUCc_UdpDgrm.write(sendWord);
        }
        break;
    case FSM_IHS_FORWARD_ALIGNED:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_FORWARD_ALIGNED - \n"); }
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ UDP ALIGNED AXI-WORD --------------
            siIPRX_Data.read(currIp4Word);
            //-- Cast incoming word into an AxisUdp word
            currUdpWord.tdata = currIp4Word.tdata;
            currUdpWord.tkeep = currIp4Word.tkeep;
            currUdpWord.tlast = currIp4Word.tlast;
            soUCc_UdpDgrm.write(currUdpWord);
            if (currUdpWord.tlast) {
                ihs_fsmState = FSM_IHS_IPW0;
            }
            else {
                ihs_fsmState = FSM_IHS_FORWARD_ALIGNED;
            }
        }
        break;
    case FSM_IHS_RESIDUE:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_RESIDUE - \n"); }
        if (!soUCc_UdpDgrm.full()) {
            //-- Forward the very last bytes of the current word
            AxisUdp sendWord(ihs_prevWord.tdata(63, 32), (ihs_prevWord.tkeep >> 4), TLAST);
           soUCc_UdpDgrm.write(sendWord);
           ihs_fsmState = FSM_IHS_IPW0;
        }
        break;
    case FSM_IHS_DROP:
        if (DEBUG_LEVEL & TRACE_IHS) { printInfo(myName, "FSM_IHS_DROP - \n"); }
        if (!siIPRX_Data.empty()) {
            //-- READ and DRAIN all AXI-WORDS -------------
            siIPRX_Data.read(currIp4Word);
            if (currIp4Word.tlast) {
                ihs_fsmState = FSM_IHS_IPW0;
            }
        }
        break;
    } // End-of: switch

    ihs_prevWord = currIp4Word;

} // End-of: pIpHeaderStripper

/******************************************************************************
 * UDP Checksum Checker (UCc)
 *
 * @param[in]  siUCc_UdpDgrm, UDP datagram stream from IpHeaderStripper (IHs).
 * @param[in]  siUCc_PsdHdrSum, Pseudo header sum (SA+DA+Prot+Len) from [IHs].
 * @param[out] soRPh_UdpDgrm, UDP datagram stream to RxPacketHandler (RPh).
 * @param[out] soRPh_CsumVal, Checksum valid information to [RPh].
 *
 * @details
 *  This process accumulates the checksum over the UDP header and the UDP data.
 *  The computed checksum is compared with the embedded checksum when 'TLAST'
 *  is reached, and the result is forwarded to the RxPacketHandler (RPh).
 *
 *****************************************************************************/
void pUdpChecksumChecker(
        stream<AxisUdp>      &siIHs_UdpDgrm,
        stream<UdpCsum>      &siIHs_PsdHdrSum,
        stream<AxisUdp>      &soRPh_UdpDgrm,
        stream<ValBool>      &soRPh_CsumVal)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "UCc");

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
        if (!siIHs_UdpDgrm.empty() && !siIHs_PsdHdrSum.empty()) {
            //-- READ 1st DTGM_WORD (CSUM|LEN|DP|SP)
            siIHs_UdpDgrm.read(currWord);
            //-- READ the checksum of the pseudo header
            siIHs_PsdHdrSum.read(ucc_psdHdrCsum);
            // Always forward datagram to [RPh]
            soRPh_UdpDgrm.write(currWord);
            if (currWord.getUdpCsum() == 0x0000) {
                // An all zero transmitted checksum  value means that the
                // transmitter generated no checksum.
                soRPh_CsumVal.write(true);
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
        if (!siIHs_UdpDgrm.empty()) {
            siIHs_UdpDgrm.read(currWord);
            soRPh_UdpDgrm.write(currWord);
            if (currWord.tlast) {
                ucc_fsmState = FSM_UCC_IDLE;
            }
        }
        break;
    case FSM_UCC_ACCUMULATE:
        if (DEBUG_LEVEL & TRACE_UCC) { printInfo(myName,"FSM_UCC_ACCUMULATE - \n"); }
        if (!siIHs_UdpDgrm.empty()) {
            siIHs_UdpDgrm.read(currWord);
            soRPh_UdpDgrm.write(currWord);
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
            soRPh_CsumVal.write(true);
        }
        else {
            soRPh_CsumVal.write(false);
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
 * @param[in]  siURIF_OpnReq, UDP open port request from UdpRoleInterface (URIF).
 * @param[out] soURIF_OpnRep, UDP open port reply to [URIF].
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
        stream<UdpPort>         &siURIF_OpnReq,
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

    //-- IP Header Stripper (IHs)
    static stream<AxisIp4>      ssIHsToRPh_Ip4Hdr       ("ssIHsToRPh_Ip4Hdr");
    #pragma HLS STREAM variable=ssIHsToRPh_Ip4Hdr       depth=32
    //OBSOLETE_20200311 static stream<IpAddrPair>   ssIHsToRPh_AddrPair     ("ssIHsToRPh_AddrPair");
    //OBSOLETE_20200311 #pragma HLS STREAM variable=ssIHsToRPh_AddrPair     depth=8
    static stream<AxisUdp>      ssIHsToUCc_UdpDgrm      ("ssIHsToUCc_UdpDgrm");
    #pragma HLS STREAM variable=ssIHsToUCc_UdpDgrm      depth=8
    static stream<UdpCsum>      ssIHsToUCc_PsdHdrSum    ("ssIHsToUCc_PsdHdrSum");
    #pragma HLS STREAM variable=ssIHsToUCc_PsdHdrSum    depth=8
    //-- UDP Checksum Checker (UCc)
    static stream<AxisUdp>      ssUCcToRPh_UdpDgrm      ("ssUCcToRPh_UdpDgrm");
    #pragma HLS STREAM variable=ssUCcToRPh_UdpDgrm      depth=4096
    static stream<ValBool>      ssUCcToRPh_CsumVal      ("ssUCcToRPh_CsumVal");
    #pragma HLS STREAM variable=ssUCcToRPh_CsumVal      depth=8

    //-- UDP Packet Handler (UPh)
    static stream<UdpPort>      ssUPhToUPt_PortStateReq ("ssUPhToUPt_PortStateReq");
    #pragma HLS STREAM variable=ssUPhToUPt_PortStateReq depth=2

    //-- UDP Port Table (UPt)
    static stream<StsBool>      ssUPtToUPh_PortStateRep ("ssUPtToUPh_PortStateRep");
    #pragma HLS STREAM variable=ssUPtToUPh_PortStateRep depth=2

    pIpHeaderStripper(
            siIPRX_Data,
            ssIHsToUCc_UdpDgrm,
            ssIHsToUCc_PsdHdrSum,
            ssIHsToRPh_Ip4Hdr);

    pUdpChecksumChecker(
            ssIHsToUCc_UdpDgrm,
            ssIHsToUCc_PsdHdrSum,
            ssUCcToRPh_UdpDgrm,
            ssUCcToRPh_CsumVal);

    pRxPacketHandler(
            ssUCcToRPh_UdpDgrm,
            ssUCcToRPh_CsumVal,
            ssIHsToRPh_Ip4Hdr,
            ssUPhToUPt_PortStateReq,
            ssUPtToUPh_PortStateRep,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    pUdpPortTable(
            ssUPhToUPt_PortStateReq,
            ssUPtToUPh_PortStateRep,
            siURIF_OpnReq,
            siURIF_OpnRep,
            siURIF_ClsReq);
}


void outputPathWriteFunction(
		stream<AxisApp> 		&siURIF_Data,
		stream<UdpAppMeta> 		&siURIF_Meta,
		stream<UdpAppDLen> 	    &siURIF_DLen,
		stream<ap_uint<64> > 	&packetData,
		stream<ap_uint<16> > 	&packetLength,
		stream<UdpAppMeta> 		&udpMetadata,
		stream<ioWord> 			&outputPathWriteFunction2checksumCalculation)
{
	#pragma HLS INLINE off
	#pragma HLS pipeline II=1 enable_flush
	
	static enum opwfState{OW_IDLE = 0, OW_PSEUDOHEADER, OW_MIX, OW_STREAM, OW_RESIDUE, OW_SWCS, OW_ONLYPACKETRESIDUE} outputPathWriteFunctionState;
	static AxisApp 		outputPathInputWord 	= AxiWord(0, 0, 0);									// Temporary buffer for the input data word
	static ap_uint<16> 	outputPathPacketLength 	= 0;												// Temporary buffer for the packet data length
	//OBSOELTE_20200403 static metadata	tempMetadata = metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));	// Temporary buffer for the destination & source IP addresses & ports
	static UdpAppMeta tempMetadata = SocketPair(SockAddr(0,0), SockAddr(0,0)); // Temporary buffer for the destination & source IP addresses & ports

	switch(outputPathWriteFunctionState) {
		case OW_IDLE:
			if (!siURIF_Meta.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				siURIF_Meta.read(tempMetadata);		// Read in the metadata
				udpMetadata.write(tempMetadata);				// Write the metadata in the buffer for the next stage
				ioWord checksumOutput = {0, 0};					// Temporary variable for the checksum calculation data word
				//OBSOLETE_20200403 checksumOutput.data = (byteSwap32(tempMetadata.destinationSocket.addr), byteSwap32(tempMetadata.sourceSocket.addr));	// Create the first checksum calc. data word. Byte swap the addresses
				checksumOutput.data = (byteSwap32(tempMetadata.dst.addr), byteSwap32(tempMetadata.src.addr));	// Create the first checksum calc. data word. Byte swap the addresses
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);	// Write the data word into the output
				outputPathWriteFunctionState = OW_PSEUDOHEADER;						// Move into the next state
			}
			break;
		case OW_PSEUDOHEADER:
			if (!siURIF_DLen.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				siURIF_DLen.read(outputPathPacketLength);		// Read in the payload length
				outputPathPacketLength += 8;							// Increase the length to take the UDP header into account.
				packetLength.write(outputPathPacketLength);				// Write length into the buffer for the next stage
				ioWord	checksumOutput		= {0x1100, 0};				// Create the 2nd checksum word. 0x1100 is the protocol used
				//OBSOLETE_20200403 checksumOutput.data.range(63, 16) = (byteSwap16(tempMetadata.destinationSocket.port), byteSwap16(tempMetadata.sourceSocket.port), byteSwap16(outputPathPacketLength));	// Add destination & source port & packet length info for the checksum calculation
				 checksumOutput.data.range(63, 16) = (byteSwap16(tempMetadata.dst.port), byteSwap16(tempMetadata.src.port), byteSwap16(outputPathPacketLength));	// Add destination & source port & packet length info for the checksum calculation
				//std::cerr << std::hex << checksumOutput << std::endl;
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);	// Write the checksum word into the output
				outputPathWriteFunctionState = OW_MIX;								// Move to the next state
			}
			break;
		case OW_MIX:
			if (!siURIF_Data.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				siURIF_Data.read(outputPathInputWord);	// Read in the first payload length
				ioWord checksumOutput				= {0, 0};					// First payload length
				checksumOutput.data.range(15, 0) 	= (outputPathPacketLength.range(7, 0), outputPathPacketLength.range(15, 8)); 	// Packet length for the checksum calculation data
				checksumOutput.data.range(63, 32)	= outputPathInputWord.tdata.range(31, 0);										// Payload data copy to the checksum calculation
				if (outputPathInputWord.tlast == 1)	{		// When the last data word is read
					packetData.write(outputPathInputWord.tdata);
					if (outputPathInputWord.tkeep.range(7, 4) == 0) {
						outputPathWriteFunctionState = OW_IDLE;	// Move to the residue state and output any remaining data.
						checksumOutput.eop = 1;
					}
					else 
						outputPathWriteFunctionState = OW_SWCS;
				}
				else
					outputPathWriteFunctionState = OW_STREAM;	// Go into next state
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);	// Write checksum calculation data word
			}
			break;	
		case OW_STREAM:	// This state streams all the payload data into both the checksum calculation stage and the next stage, reformatting them as required
			if (!siURIF_Data.empty() && !packetData.full() && !outputPathWriteFunction2checksumCalculation.full()) {
				packetData.write(outputPathInputWord.tdata);										// Write the realignned data word to the next stage
				ioWord 	checksumOutput				= {0, 0};
				checksumOutput.data.range(31,0)		= outputPathInputWord.tdata.range(63, 32);	// Realign the output data word for the next stage
				siURIF_Data.read(outputPathInputWord);					// Read the next data word
				checksumOutput.data.range(63, 32)	= outputPathInputWord.tdata.range(31, 0);
				if (outputPathInputWord.tlast == 1) {
					if (outputPathInputWord.tkeep.bit(4) == 1)		// When the last data word is read
						outputPathWriteFunctionState = OW_RESIDUE;	// Move to the residue state and output any remaining data.
					else {
						outputPathWriteFunctionState = OW_ONLYPACKETRESIDUE;
						checksumOutput.eop = 1;
					}
				}
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);				// Write the checksum calculation data word
			}
			break;
		case OW_ONLYPACKETRESIDUE:
			if (!packetData.full()) {
				packetData.write(outputPathInputWord.tdata);
				outputPathWriteFunctionState	= OW_IDLE;
			}
			break;
		case OW_SWCS:
			if (!outputPathWriteFunction2checksumCalculation.full()) {
				ioWord checksumOutput		= {0, 1};
				checksumOutput.data.range(31, 0) = outputPathInputWord.tdata.range(63, 32);
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);
				outputPathWriteFunctionState	= OW_IDLE;
			}
			break;
		case OW_RESIDUE:
			if (!packetData.full() && !outputPathWriteFunction2checksumCalculation.full()) {
				ioWord checksumOutput		= {0, 1};
				checksumOutput.data.range(31, 0)		= outputPathInputWord.tdata.range(63, 32);
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);
				packetData.write(outputPathInputWord.tdata);
				outputPathWriteFunctionState	= OW_IDLE;
			}
			break;
	}
}


void udpChecksumCalculation(
		stream<ioWord>			&dataIn,
		stream<ap_uint<16> >	&udpChecksumOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1 enable_flush

	static ap_uint<32> 	udpChecksum 	= 0;

	if (!dataIn.empty()) {
		ioWord inputWord = dataIn.read();
		udpChecksum = ((((udpChecksum + inputWord.data.range(63, 48)) + inputWord.data.range(47, 32)) + inputWord.data.range(31, 16)) + inputWord.data.range(15, 0));
		if (inputWord.eop) {
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = ~udpChecksum;			// Reverse the bits of the result
			udpChecksumOut.write(udpChecksum.range(15, 0));	// and write it into the output
			udpChecksum = 0;
		}
	}
}


void outputPathReadFunction(
		stream<ap_uint<64> >    &packetData,
		stream<ap_uint<16> >    &packetLength,
		stream<UdpAppMeta>         &udpMetadata,
		stream<ap_uint<16> >    &udpChecksum,
		stream<axiWord>         &outputPathOutData,
		stream<IpAddrPair>      &outIPaddresses,
		stream<ap_uint<16> >    &outputPathReadFunction2addIpHeader_length)
{

	#pragma HLS INLINE off
	#pragma HLS pipeline II=1 enable_flush

	static enum oprfState{OR_IDLE = 0, OR_STREAM} outputPathReadFunctionState;
	static ap_uint<16> readFunctionOutputPathPacketLength;
	static uint32_t myPacketCounter = 0;

	switch(outputPathReadFunctionState) {
		case OR_IDLE:
			if (!packetLength.empty() && !udpMetadata.empty() && !udpChecksum.empty() ) {
				//std::cerr << myPacketCounter << std::endl;
				myPacketCounter++;

				packetLength.read(readFunctionOutputPathPacketLength);				// Read the packet length
				ap_uint<16> interimLength = readFunctionOutputPathPacketLength;	// Add the length of the UDP header
				readFunctionOutputPathPacketLength -= 8;
				outputPathReadFunction2addIpHeader_length.write(interimLength);
				UdpAppMeta tempMetadata 			= udpMetadata.read();
				axiWord outputWord 				= axiWord(0, 0xFF, 0);
				outputWord.data.range(15, 0) 	= byteSwap16(tempMetadata.src.port);
				outputWord.data.range(31, 16) 	= byteSwap16(tempMetadata.dst.port);
				outputWord.data.range(47, 32)	= byteSwap16(interimLength);
				outputWord.data.range(63, 48)	= udpChecksum.read();
				//std::cerr << std::hex << outputWord.data << std::endl;
				outputPathOutData.write(outputWord);
				IpAddrPair tempIPaddresses = IpAddrPair(tempMetadata.src.addr, tempMetadata.dst.addr);
				outIPaddresses.write(tempIPaddresses);
				outputPathReadFunctionState = OR_STREAM;
			}
			break;
		case OR_STREAM:
			if (!packetData.empty()) {
				ap_uint<64> inputWord = packetData.read();
				axiWord outputWord = axiWord(inputWord, 0xFF, 0);
				//std::cerr << std::hex << outputWord.data << std::endl;
				if (readFunctionOutputPathPacketLength > 8)
					readFunctionOutputPathPacketLength -= 8;
				else {
					outputWord.last = 1;
					outputWord.keep = length2keep_mapping(static_cast <uint16_t>(readFunctionOutputPathPacketLength));
					readFunctionOutputPathPacketLength = 0;
					outputPathReadFunctionState = OR_IDLE;
				}
				outputPathOutData.write(outputWord);
			}
		break;
	}
}


void addIpHeader(
		stream<axiWord>     &outputPathRead2addIpHeader_data,
		stream<IpAddrPair>  &outputPathRead2addIpHeader_ipAddress,
		stream<AxisIp4>     &soIPTX_Data,
		stream<ap_uint<16> > &outputPathReadFunction2addIpHeader_length)
{

#pragma HLS pipeline II=1 enable_flush

	static enum iState {IPH_IDLE, IPH_IP1, IPH_IP2, IPH_FORWARD, IPH_RESIDUE} iphState;
	static IpAddrPair ipHeaderTuple = IpAddrPair(0, 0);
	static axiWord    outputWord = axiWord(0, 0, 0);

	switch(iphState) {
	case IPH_IDLE:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathRead2addIpHeader_ipAddress.empty() &&!outputPathReadFunction2addIpHeader_length.empty() && !soIPTX_Data.full()) {
			ap_uint<16> tempLength = outputPathReadFunction2addIpHeader_length.read();
			tempLength += 20;
			AxisIp4 tempWord = AxisIp4(0x0000000034000045, 0xFF, 0);
			tempWord.tdata.range(31, 16) = byteSwap16(tempLength);
			//tempWord.data.range(31, 16) = (tempLength.range(7, 0), tempLength.range(15, 0));
			soIPTX_Data.write(tempWord);
			iphState = IPH_IP1;
		}
		break;
	case IPH_IP1:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathRead2addIpHeader_ipAddress.empty() && !soIPTX_Data.full()) {
			AxisIp4 tempWord = AxisIp4(0x0, 0xFF, 0);
			ipHeaderTuple = outputPathRead2addIpHeader_ipAddress.read();
			tempWord.tdata.range(31, 0) = 0x000011FF;
			tempWord.tdata.range(63, 32) = byteSwap32(ipHeaderTuple.ipSa);
			soIPTX_Data.write(tempWord);
			iphState = IPH_IP2;
		}
		break;
	case IPH_IP2:
		if(!outputPathRead2addIpHeader_data.empty() && !soIPTX_Data.full()) {
			AxisIp4 tempWord = AxisIp4(0x0, 0xFF, 0);
			outputWord = outputPathRead2addIpHeader_data.read();
			tempWord.tdata.range(31, 0) = byteSwap32(ipHeaderTuple.ipDa);
			tempWord.tdata.range(63, 32) = outputWord.data.range(31, 0);
			soIPTX_Data.write(tempWord);
			iphState = IPH_FORWARD;
		}
		break;
	case IPH_FORWARD:
		if(!outputPathRead2addIpHeader_data.empty() && !soIPTX_Data.full()) {
			AxisIp4 tempWord 			= AxisIp4(0x0, 0x0F, 0);
			tempWord.tdata.range(31, 0) 	= outputWord.data.range(63, 32);
			outputWord 					= outputPathRead2addIpHeader_data.read();
			tempWord.tdata.range(63, 32) = outputWord.data.range(31, 0);
			if(outputWord.last) {
				if (outputWord.keep.range(7, 4) != 0) {
					tempWord.tkeep.range(7, 4) = 0xF;
					iphState = IPH_RESIDUE;
				}
				else {
					tempWord.tkeep.range(7, 4) = outputWord.keep.range(3, 0);
					tempWord.tlast = 1;
					iphState = IPH_IDLE;
				}
			}
			else
				tempWord.tkeep.range(7, 4) = 0xF;
			soIPTX_Data.write(tempWord);
		}
		break;
	case IPH_RESIDUE:
			if (!soIPTX_Data.full()) {
				AxisIp4 tempWord = AxisIp4(outputWord.data.range(63, 32), outputWord.keep.range(7, 4), 1);
				soIPTX_Data.write(tempWord);
				iphState = IPH_IDLE;
			}
		break;
	}
}


/******************************************************************************
 * Tx Engine (TXe)
 *
 * @param [(dir)] <parameter-name> { parameter description }
 * @param[in]     _inArg1 Description of first function argument.
 * @param[out]    _outArg2 Description of second function argument.
 * @param[in,out] _inoutArg3 Description of third function argument.
 *
 * @details
 *  The Tx path of the UdpOffloadEngine (UOE). This is the path from
 *  UdpRoleInterface (URIF) a.k.a the Application [APP] to the [IPRX].
 *****************************************************************************/
void pTxEngine(
		stream<AxisApp> 		&siURIF_Data,
		stream<UdpAppMeta> 		&siURIF_Meta,
		stream<UdpAppDLen> 	    &siURIF_DLen,
		stream<AxisIp4> 		&soIPTX_Data)
{

#pragma HLS INLINE

	// Declare intermediate streams for inter-function communication
	static stream<ap_uint<64> > packetData("packetData");
	static stream<ap_uint<16> > packetLength("packetLength");
	static stream<UdpAppMeta>		udpMetadata("udpMetadata");
	static stream<ap_uint<16> > checksumCalculation2outputPathReadFunction("checksumCalculation2outputPathReadFunction");
	static stream<ioWord> 		outputPathWriteFunction2checksumCalculation("outputPathWriteFunction2checksumCalculation");
	static stream<axiWord>		outputPathRead2addIpHeader_data("outputPathRead2addIpHeader_data");
	static stream<IpAddrPair>	outputPathRead2addIpHeader_ipAddress("outputPathRead2addIpHeader_ipAddress");
	static stream<ap_uint<16> > outputPathReadFunction2addIpHeader_length("outputPathReadFunction2addIpHeader_length");
	
	#pragma HLS DATA_PACK 	variable=udpMetadata
	#pragma HLS DATA_PACK 	variable=outputPathRead2addIpHeader_ipAddress
	
	#pragma HLS STREAM variable=packetData 									depth=4096
	#pragma HLS STREAM variable=packetLength								depth=8
	#pragma HLS STREAM variable=udpMetadata 								depth=8
	#pragma HLS STREAM variable=outputPathWriteFunction2checksumCalculation	depth=32
	
	outputPathWriteFunction(
			siURIF_Data,
			siURIF_Meta,
			siURIF_DLen,
			packetData,
			packetLength,
			udpMetadata,
			outputPathWriteFunction2checksumCalculation);	// This function receives the data from the user logic, creates the pseudo-header for UDP checksum calculation and send it on to the read stage.

	udpChecksumCalculation(
			outputPathWriteFunction2checksumCalculation,
			checksumCalculation2outputPathReadFunction); // Calculates the UDP checksum value

	outputPathReadFunction(
			packetData,
			packetLength,
			udpMetadata,
			checksumCalculation2outputPathReadFunction,
			outputPathRead2addIpHeader_data,
			outputPathRead2addIpHeader_ipAddress,
			outputPathReadFunction2addIpHeader_length); // Reads the checksum value and composes the UDP packet

	addIpHeader(
			outputPathRead2addIpHeader_data,
			outputPathRead2addIpHeader_ipAddress,
			soIPTX_Data,
			outputPathReadFunction2addIpHeader_length); // Adds the IP header on top of the UDP one.

}


/*****************************************************************************/
/* @brief 	Main process of the UDP Offload Engine (UOE).
 *
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * -- IPTX / IP Tx / Data Interface
 * @param[out] soIPTX_Data,   IP4 data stream to IpTxHandler (IPTX).
 * -- URIF / Control Port Interfaces
 * @param[in]  siURIF_OpnReq, UDP open port request from UdpRoleInterface (URIF).
 * @param[out] soURIF_OpnRep, UDP open port reply to [URIF].
 * @param[in]  siURIF_ClsReq, UDP close port request from [URIF].
 *
 * -- URIF / Tx Data Interfaces
 * @param[out] soURIF_Data,   UDP data stream to [URIF].
 * @param[out] soURIF_Meta,   UDP metadata stream to [URIF].
 * -- TODO
 * @param[in]  portRelease
 *
 *
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
		//   [TODO - Rename into LsnReq/LsnAck]
		//   [TODO - URIF / Listen Interfaces]
        //------------------------------------------------------
        stream<UdpPort>                 &siURIF_OpnReq,
        stream<StsBool>                 &soURIF_OpnRep,
        stream<UdpPort>                 &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &soURIF_Data,
        stream<UdpAppMeta>                 &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>                 &siURIF_Data,
        stream<UdpAppMeta>                 &siURIF_Meta,
        stream<UdpAppDLen>                 &siURIF_PLen,

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

    #pragma HLS RESOURCE core=AXI4Stream variable=siURIF_OpnReq     metadata="-bus_bundle siURIF_OpnReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soURIF_OpnRep     metadata="-bus_bundle soURIF_OpnRep"
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

    #pragma HLS INTERFACE axis               port=siURIF_OpnReq     name=siURIF_OpnReq

    #pragma HLS INTERFACE axis               port=siIPRX_Data       name=siIPRX_Data
    #pragma HLS INTERFACE axis               port=soIPTX_Data       name=soIPTX_Data

    #pragma HLS INTERFACE axis               port=siURIF_OpnReq     name=siURIF_OpnReq
    #pragma HLS INTERFACE axis               port=soURIF_OpnRep     name=soURIF_OpnRep
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
            siURIF_OpnReq,
            soURIF_OpnRep,
            siURIF_ClsReq,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    pTxEngine(
            siURIF_Data,
            siURIF_Meta,
            siURIF_PLen,
            soIPTX_Data);
}
