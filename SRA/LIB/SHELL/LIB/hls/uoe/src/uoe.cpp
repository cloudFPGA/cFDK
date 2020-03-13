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
        UdpPort portNum = siUPh_PortStateReq.read();
        soUPh_PortStateRep.write(PORT_TABLE[portNum]);
    }
    else if (!siURIF_OpnReq.empty()) {
        // REquest to open a port from [URIF]
        UdpPort portToOpen = siURIF_OpnReq.read();
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
        UdpPort portToRelease = siURIF_ClsReq.read();
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
        stream<AppData>     &soURIF_Data,
		stream<metadata>    &soURIF_Meta,
        stream<AxiWord>     &soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_IDLE=0, FSM_PORT_LOOKUP, FSM_STREAM,
                            FSM_STREAM_FIRST,         FSM_DRAIN_DATAGRAM_STREAM,
                            FSM_DRAIN_IP4HDR_STREAM,  FSM_PORT_UNREACHABLE_1ST,
                            FSM_PORT_UNREACHABLE_2ND, FSM_PORT_UNREACHABLE_STREAM,
                            FSM_PORT_UNREACHABLE_LAST } rph_fsmState=FSM_IDLE;
    #pragma HLS RESET                           variable=rph_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIp4      rph_1stIp4HdrWord;
    static AxisIp4      rph_2ndIp4HdrWord;
    static AxisUdp      rph_1stUdpHdrWord;
    static ap_uint<4>   rph_ipHdrCnt;
    static FlagBit      rph_emptyPayloadFlag;
    static FlagBool     rph_ipHdrStreamIsEmpty;

    static metadata 	rxPathMetadata = metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));

    switch(rph_fsmState) {
    case FSM_IDLE:
        if (!siUCc_UdpDgrm.empty() && !siIHs_Ip4Hdr.empty() && !soUPt_PortStateReq.full()) {
            rph_ipHdrStreamIsEmpty = false;
            // Read the 1st IPv4 header word and retrieve the IHL
            rph_1stIp4HdrWord = siIHs_Ip4Hdr.read();
            //OBSOLETE_20200311 hdrWordCount = currHdrWord.tdata.range(3,0) - 2;
            rph_ipHdrCnt = rph_1stIp4HdrWord.getIp4HdrLen();
            // Read the the 1st datagram word (this is also the last one)
            rph_1stUdpHdrWord = siUCc_UdpDgrm.read();
            // Check if payload of datagram empty
            //OBSOLETE_2200311 (byteSwap16(bufferWord.tdata.range(47, 32)) > 8) ? rxEngNoPayloadFlag = 0 : rxEngNoPayloadFlag = 1;
            if (rph_1stUdpHdrWord.getUdpLen() > 8) {
                rph_emptyPayloadFlag = 0;
            }
            else {
                rph_emptyPayloadFlag = 1;
            }
            // Request the state of this port to UdpPortTable (UPt)
            //OBSOLETE_20200311 soUPt_PortStateReq.write(byteSwap16(bufferWord.tdata.range(31, 16)));
            soUPt_PortStateReq.write(rph_1stUdpHdrWord.getUdpDstPort());
            //OBSOLETE_20200311 rxPathMetadata = metadata(sockaddr_in(byteSwap16(bufferWord.tdata.range(15, 0)), ipAddrPair.ipSa), sockaddr_in(byteSwap16(bufferWord.tdata.range(31, 16)), ipAddrPair.ipDa));
            rxPathMetadata.sourceSocket.port = rph_1stUdpHdrWord.getUdpSrcPort();
            rxPathMetadata.destinationSocket.port = rph_1stUdpHdrWord.getUdpDstPort();
            rph_fsmState = FSM_PORT_LOOKUP;
        }
        break;
    case FSM_PORT_LOOKUP:
        if (!siUPt_PortStateRep.empty() && !siUCc_CsumVal.empty() && !siIHs_Ip4Hdr.empty()) {
            bool csumResult = siUCc_CsumVal.read();
            bool portLkpRes = siUPt_PortStateRep.read();
            // Read the 2nd IPv4 header word and update the metadata structure
            rph_2ndIp4HdrWord = siIHs_Ip4Hdr.read();
            rxPathMetadata.sourceSocket.addr = rph_2ndIp4HdrWord.getIp4SrcAddr();
            if(portLkpRes && csumResult && (rph_emptyPayloadFlag == 0)) {
                rph_fsmState = FSM_STREAM_FIRST;
            }
            else if (rph_emptyPayloadFlag) {
                rph_fsmState = FSM_DRAIN_IP4HDR_STREAM ;
            }
            else if (not csumResult) {
                rph_fsmState = FSM_DRAIN_DATAGRAM_STREAM;
            }
            else {
                rph_fsmState = FSM_PORT_UNREACHABLE_1ST;
            }
        }
        break;
    case FSM_STREAM_FIRST:
        if (!siUCc_UdpDgrm.empty() && !siIHs_Ip4Hdr.empty() &&
            !soURIF_Data.full()    && !soURIF_Meta.full()) {
            // Read the 3rd IPv4 header word and update and forward the metadata
            AxisIp4 thirdIp4HdrWord = siIHs_Ip4Hdr.read();
            rxPathMetadata.destinationSocket.addr = thirdIp4HdrWord.getIp4DstAddr();
            soURIF_Meta.write(rxPathMetadata);
            if (thirdIp4HdrWord.tlast) {
                rph_ipHdrStreamIsEmpty = true;
            }
            // Read the 1st datagram word and forward to [URIF]
            AxisUdp dgrmWord = siUCc_UdpDgrm.read();
            soURIF_Data.write(dgrmWord);
            if (dgrmWord.tlast) {
                if (thirdIp4HdrWord.tlast) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_IDLE;
                }
                else {
                    // They were options words that remain to be drained
                    rph_fsmState = FSM_DRAIN_IP4HDR_STREAM;
                }
            }
            else {
                rph_fsmState = FSM_STREAM;
            }
        }
        break;
    case FSM_STREAM:
        if (!siUCc_UdpDgrm.empty() && !soURIF_Data.full() && !soURIF_Meta.full()) {
            // Forward datagram word
            AxisUdp dgrmWord = siUCc_UdpDgrm.read();
            soURIF_Data.write(dgrmWord);
            if (dgrmWord.tlast) {
                if (rph_ipHdrStreamIsEmpty) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_IDLE;
                }
                else {
                    // They are IPv4 header words that remain to be drained
                    rph_fsmState = FSM_DRAIN_IP4HDR_STREAM;
                }
            }
            else if (!siIHs_Ip4Hdr.empty()) {
                // Drain any pending IPv4 header word
                AxisIp4 currIp4HdrWord = siIHs_Ip4Hdr.read();
                if (currIp4HdrWord.tlast) {
                    rph_ipHdrStreamIsEmpty = true;
                }
            }
        }
        break;
    case FSM_DRAIN_DATAGRAM_STREAM:
        //-- Drop and drain the entire datagram
        if (!siUCc_UdpDgrm.empty()) {
            AxisUdp currWord = siUCc_UdpDgrm.read();
            if (currWord.tlast == 1) {
                if (rph_ipHdrStreamIsEmpty) {
                    // Both incoming stream are empty. We are done.
                    rph_fsmState = FSM_IDLE;
                }
                else {
                    // Now go and drain the corresponding IPV4 header stream
                    rph_fsmState = FSM_DRAIN_IP4HDR_STREAM;
                }
            }
        }
        break;
    case FSM_DRAIN_IP4HDR_STREAM :
        //-- Drain the IPv4 Header Stream
        if (!siIHs_Ip4Hdr.empty()) {
            AxisIp4 currWord = siIHs_Ip4Hdr.read();
            if (currWord.tlast) {
            rph_fsmState = FSM_IDLE;
            }
        }
        break;
    case FSM_PORT_UNREACHABLE_1ST:
        if (!soICMP_Data.full()) {
            // Forward the 1st word of the IPv4 header
            soICMP_Data.write(rph_1stIp4HdrWord);
            rph_fsmState = FSM_PORT_UNREACHABLE_2ND;
        }
        break;
    case FSM_PORT_UNREACHABLE_2ND:
        if (!soICMP_Data.full()) {
            // Forward the 2nd word of the IPv4 header
            soICMP_Data.write(rph_2ndIp4HdrWord);
            rph_fsmState = FSM_PORT_UNREACHABLE_STREAM;
        }
        break;
    case FSM_PORT_UNREACHABLE_STREAM:
        if (!siIHs_Ip4Hdr.empty() && !soICMP_Data.full()) {
            // Forward remaining of the IPv4 header words
            AxisIp4 ip4Word = siIHs_Ip4Hdr.read();
            // Awlways clear the LAST bit because the UDP header will follow
            soICMP_Data.write(AxiWord(ip4Word.tdata, ip4Word.tkeep, 0));
            if (ip4Word.tlast) {
                rph_ipHdrStreamIsEmpty = true;
                rph_fsmState = FSM_PORT_UNREACHABLE_LAST;
            }
        }
        break;
    case FSM_PORT_UNREACHABLE_LAST:
        if (!soICMP_Data.full()) {
            // Forward the first 8 bytes of the datagram (.i.i the UDP header)
            soICMP_Data.write(AxiWord(rph_1stUdpHdrWord.tdata, rph_1stUdpHdrWord.tkeep, TLAST));
            rph_fsmState = FSM_DRAIN_DATAGRAM_STREAM;
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
 *  This process extracts the UDP datagram and the IP header from the incoming
 *  IPv4 packet. The IP header is forwarded to the RxPacketHandler (RPh) for
 *  further processing while the UDP datagram first goes through the the UDP
 *  checksum checker.
 *
 *****************************************************************************/
void pIpHeaderStripper(
        stream<AxisIp4>      &siIPRX_Data,
        stream<AxisUdp>      &soUCc_UdpDgrm,
        stream<ap_uint<17> > &soUCc_PsdHdrSum,
        stream<AxisIp4>      &soRPh_Ip4Hdr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IHs");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {FSM_IPW0=0,  FSM_IPW1, FSM_IPW2, FSM_IPW3,
                           FSM_FORWARD, FSM_FORWARDALIGNED, FSM_RESIDUE,
                           FSM_DROP } ihs_fsmState=FSM_IPW0;
    #pragma HLS RESET        variable=ihs_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<4>    ihs_bitCount;
    static ap_uint<3>    ihs_ipHdrCnt;
    static AxisIp4       ihs_prevWord;
    static Ip4HdrLen     ihs_ip4HdrLen;
    static ap_uint<17>   ihs_psdHdrSum;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4     currWord;

    switch(ihs_fsmState) {
    case FSM_IPW0:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 1st AXI-WORD (Frag|Flags|Id|TotLen|ToS|Ver|IHL) ---------
            currWord      = siIPRX_Data.read();
            ihs_ip4HdrLen = currWord.getIp4HdrLen();
            if (ihs_ip4HdrLen < 5) {
                printWarn(myName, "Received an IPv4 packet with unvalid IHL. This packet will be dropped.\n");
                ihs_fsmState  = FSM_DROP;
            }
            else {
                soRPh_Ip4Hdr.write(currWord);
                ihs_psdHdrSum = currWord.getIp4TotalLen();
                if (DEBUG_LEVEL & TRACE_IHS) {
                    printInfo(myName, "Received a new IPv4 packet (IHL=%d)\n", ihs_ip4HdrLen.to_uint());
                }
                ihs_ip4HdrLen -= 2;
                ihs_fsmState  = FSM_IPW1;
            }
        }
        break;
    case FSM_IPW1:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 2nd AXI_WORD (SA|HdrCsum|Prot|TTL) --
            currWord = siIPRX_Data.read();
            soRPh_Ip4Hdr.write(currWord);
            ihs_psdHdrSum += (0x00, currWord.getIp4Prot());
            ihs_psdHdrSum += currWord.getIp4SrcAddr().range(31,16);
            ihs_psdHdrSum += currWord.getIp4SrcAddr().range(15, 0);
            ihs_ip4HdrLen -= 2;
            ihs_fsmState = FSM_IPW2;
        }
        break;
    case FSM_IPW2:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            //-- READ 3rd AXI-WORD (DP|SP|DA) or (Opt|DA)
            currWord = siIPRX_Data.read();
            ihs_psdHdrSum += currWord.getIp4DstAddr().range(31,16);
            ihs_psdHdrSum += currWord.getIp4DstAddr().range(15, 0);
            soUCc_PsdHdrSum.write(ihs_psdHdrSum);
            if (ihs_ip4HdrLen == 1) {
                // This a typical IPv4 header with a length of 20 bytes (5*4).
                soRPh_Ip4Hdr.write(AxisIp4(currWord.getIp4DstAddr(), 0x0F, TLAST));
                ihs_fsmState = FSM_IPW3;
            }
            else {
                printWarn(myName, "This IPv4 packet contains options! FYI, IPV4 options are not supported and will be dropped.\n");
                soRPh_Ip4Hdr.write(currWord);
                if (ihs_ip4HdrLen == 2 ) {
                    ihs_fsmState = FSM_FORWARDALIGNED;
                }
                else {  // ihs_ip4HdrLen > 2
                    ihs_ip4HdrLen -= 2;
                    ihs_fsmState = FSM_IPW2;
                }
            }
        }
        break;
    case FSM_IPW3:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ 4th AXI-WORD (Data|Csum|Len) --------
            currWord = siIPRX_Data.read();
            //-- Forward the UDP Header (Csum|Len|DP|SP)
            AxisUdp sendWord((currWord.tdata(31, 0),ihs_prevWord.tdata(63, 32)),
                              0xFF, (currWord.tlast and (currWord.tkeep == 0x0F)));
            soUCc_UdpDgrm.write(sendWord);
            if (currWord.tlast) {
                if (currWord.tkeep == 0x0F) {
                    printWarn(myName, "Received a UDP datagram of length = 0!\n");
                    ihs_fsmState = FSM_IPW0;
                }
                else {
                    ihs_fsmState = FSM_RESIDUE;
                }
            }
            else {
                ihs_fsmState = FSM_FORWARD;
            }
        }
        break;
    case FSM_FORWARD:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ n-th AXI-WORD (Data) ----------------
            currWord = siIPRX_Data.read();
            //-- Forward 1/2 of previous word and 1/2 of current word)
            AxisUdp sendWord;
            sendWord.tdata(63, 32) = currWord.tdata(31, 0);
            sendWord.tkeep( 7,  4) = currWord.tkeep( 3, 0);
            sendWord.tdata(31,  0) = ihs_prevWord.tdata(63, 32);
            sendWord.tkeep( 3,  0) = ihs_prevWord.tkeep( 7,  4);
            if (currWord.tlast) {
                if (currWord.tkeep <= 0x0F) {
                    sendWord.tlast = TLAST;
                    ihs_fsmState = FSM_IPW0;
                }
                else {
                    sendWord.tlast = 0;
                    ihs_fsmState = FSM_RESIDUE;
                }
            }
            else {
                sendWord.tlast = 0;
                ihs_fsmState = FSM_FORWARD;
            }
            soUCc_UdpDgrm.write(sendWord);
        }
        break;
    case FSM_RESIDUE:
        if (soUCc_UdpDgrm.full()) {
            //-- Forward the very last bytes of the current word
            AxisUdp sendWord(ihs_prevWord.tdata(63, 32), (ihs_prevWord.tkeep >> 4), TLAST);
           soUCc_UdpDgrm.write(sendWord);
           ihs_fsmState = FSM_IPW0;
        }
        break;
    case FSM_FORWARDALIGNED:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full()) {
            //-- READ n-4th ALIGNED AXI-WORD --------------
            currWord = siIPRX_Data.read();
            soUCc_UdpDgrm.write(currWord);
            if (currWord.tlast) {
                ihs_fsmState = FSM_IPW0;
            }
            else {
                ihs_fsmState = FSM_FORWARDALIGNED;
            }
        }
        break;
    case FSM_DROP:
        if (!siIPRX_Data.empty()) {
            //-- READ and DRAIN all AXI-WORDS -------------
            currWord = siIPRX_Data.read();
            if (currWord.tlast) {
                ihs_fsmState = FSM_IPW0;
            }
        }
        break;

    } // End-of: switch

    ihs_prevWord = currWord;

} // End-of: pIpHeaderStripper

/*** OBSOLETE_20200310 ***
void pIpHeaderStripper(
        stream<AxisIp4>     &siIPRX_Data,
        stream<AxisUdp>     &soRPh_UdpDgrm,
        stream<AxisIp4>     &soRPh_Ip4Hdr,
		stream<IpAddrPair>  &soRPh_AddrPair,
		stream<AxisUdp>     &soUCc_UdpDgrm)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IHs");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {FSM_IDLE=0,     FSM_IP,               FSM_SKIPOPTIONS,
                           FSM_IP2,        FSM_FORWARD,          FSM_FORWARDALIGNED,
                           FSM_FORWARD_CS, FSM_FORWARDCSALIGNED, FSM_RESIDUE,
                           FSM_CS_RESIDUE, FSM_CS_RESIDUEALIGNNED } ihs_fsmState=FSM_IDLE;
    #pragma HLS RESET                                          variable=ihs_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<4>    ihs_bitCount;  // Counts #bytes which are valid in the last data word
    static ap_uint<3>    ihs_ipHdrWordCount;
    static AxisIp4       ihs_currWord;
    static UdpLen        ihs_udpLen;
    static Ip4HdrLen     ihs_ip4HdrLen;
    static IpAddrPair    ihs_ipAddrPair;

    switch(ihs_fsmState) {
    case FSM_IDLE:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            ihs_currWord  = siIPRX_Data.read();
            //-- READ 1st AXI-WORD (Frag|Flags|Id|TotLen|ToS|Ver|IHL) ---------
            ihs_ipHdrWordCount = 0;
            ihs_bitCount  = 0;
            //OBSOLETE_20200310 ihs_udpLen    = (ihs_currWord.tdata.range(23, 16), ihs_currWord.tdata.range(31, 24));
            //OBSOLETE_20200310 ihs_ip4HdrLen = ihs_currWord.tdata.range(3, 0);
            ihs_ip4HdrLen = ihs_currWord.getIp4HdrLen();
            ihs_udpLen    = ihs_currWord.getIp4TotalLen() - (ihs_ip4HdrLen * 4);
            soRPh_Ip4Hdr.write(ihs_currWord);
            ihs_fsmState  = FSM_IP;
        }
        break;
    case FSM_IP:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            ihs_currWord = siIPRX_Data.read();
            //-- READ 2nd AXI_WORD (SA|HdrCsum|Prot|TTL) ----------------------
            ihs_ipHdrWordCount = 1;
            //OBSOLETE_20200310 ihs_ipAddrPair.ipSa = byteSwap32(ihs_currWord.tdata.range(63, 32));
            ihs_ipAddrPair.ipSa = ihs_currWord.getIp4SrcAddr();
            ihs_ip4HdrLen -= 2;
            soRPh_Ip4Hdr.write(ihs_currWord);
            if (ihs_ip4HdrLen == 3) {
                // This a typical IPv4 header with a length of 20 bytes (5*4).
                ihs_fsmState = FSM_IP2;
            }
            else if (ihs_ip4HdrLen > 3) {
                // This IPv4 header contains IP options that will be ignored
                ihs_fsmState = FSM_SKIPOPTIONS;
            }
        }
        break;
    case FSM_SKIPOPTIONS:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full() && !soRPh_AddrPair.full()) {
            ihs_currWord = siIPRX_Data.read();
            if (ihs_ipHdrWordCount == 1) {
            	ihs_ipAddrPair.ipDa = byteSwap32(ihs_currWord.tdata.range(31, 0));
				soRPh_AddrPair.write(ihs_ipAddrPair);
				AxisUdp checksumWord = AxisUdp(0, 0xFF, 0);
				checksumWord.tdata = (byteSwap32(ihs_ipAddrPair.ipDa), byteSwap32(ihs_ipAddrPair.ipSa));
				soUCc_UdpDgrm.write(checksumWord);
			}
			soRPh_Ip4Hdr.write(ihs_currWord);
			ihs_ip4HdrLen -= 2;
			ihs_ipHdrWordCount++;
			if (ihs_ip4HdrLen == 2) {
				ihs_fsmState = FSM_FORWARDALIGNED;
			}
			else if (ihs_ip4HdrLen == 3) {
				ihs_fsmState = FSM_IP2;
			}
			else if (ihs_ip4HdrLen > 3) {
				ihs_fsmState = FSM_SKIPOPTIONS;
			}
		}
		break;
    case FSM_IP2:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_AddrPair.full()) {
            //-- READ 3rd AXI-WORD (DP|SP|DA) ---------------------------------
            ihs_currWord = siIPRX_Data.read();
            //OBSOLETE_20200310 if (ihs_ipHdrWordCount == 1) {  // [FIXME - This test is not needed]
                //OBSOLETE_20200310 ihs_ipAddrPair.ipDa = byteSwap32(ihs_currWord.tdata.range(31, 0));
                ihs_ipAddrPair.ipDa = ihs_currWord.getIp4DstAddr();
                soRPh_AddrPair.write(ihs_ipAddrPair);
                //OBSOLETE_20200310 AxisUdp csumWord = AxisUdp((byteSwap32(ihs_ipAddrPair.ipDa),
                //OBSOLETE_20200310                             byteSwap32(ihs_ipAddrPair.ipSa)), 0xFF, 0);
                AxiWord csumWord = AxiWord((ihs_currWord.getUdpDstPort(),
                                            ihs_currWord.getUdpSrcPort()), 0x0F, 0);
                soUCc_UdpDgrm.write(csumWord);
                //OBSOLETE_20200310 }
            //OBSOLETE_202020310 soRPh_Ip4Hdr.write(AxisIp4(ihs_currWord.tdata.range(31, 0), 0x0F, 1));
            soRPh_Ip4Hdr.write(AxisIp4(ihs_currWord.getIp4DstAddr(), 0x0F, 1));
            ihs_fsmState = FSM_FORWARD;
        }
        break;
    case FSM_FORWARDALIGNED:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
            //-- READ 4th AXI-WORD (Data|Csum|Len) ----------------------------
            ihs_currWord = siIPRX_Data.read();
            AxisUdp csumWord             = AxisUdp(0x1100, 0xFF, 0);
            csumWord.tdata.range(31, 16) = byteSwap16(ihs_udpLen.range(15, 0));	// Same for the UDP length.
            csumWord.tdata.range(63, 32) = ihs_currWord.tdata.range(31, 0);
            //csumWord                     = AxisUdp(0x1100, 0xFF, 0);
            //csumWord.tdata.range(31, 16) = ihs_currWord.getUdpCsum();
            //csumWord.tdata.range(63, 32) = ihs_currWord.tdata.range(31, 0);
			soUCc_UdpDgrm.write(csumWord);
			soRPh_UdpDgrm.write(AxisUdp(ihs_currWord));
			ihs_fsmState = FSM_FORWARDCSALIGNED;
		}
		break;
    case FSM_FORWARD:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			AxisUdp checksumWord 			= AxisUdp(0x1100, 0xFF, 0);   // Inject Protocol length in the pseudo header
			checksumWord.tdata.range(31, 16) = byteSwap16(ihs_udpLen.range(15, 0));	// Same for the UDP length.
			checksumWord.tdata.range(63, 32) = ihs_currWord.tdata.range(63, 32);
			soUCc_UdpDgrm.write(checksumWord);
			AxisUdp tempWord 				= AxisUdp(0, 0x0F, 0);
			tempWord.tdata.range(31, 0) 	= ihs_currWord.tdata.range(63, 32);
			ihs_currWord 					= siIPRX_Data.read();
			tempWord.tdata.range(63, 32) 	= ihs_currWord.tdata.range(31, 0);
			tempWord.tkeep.range(7, 4) 		= ihs_currWord.tkeep.range(3, 0);
			soRPh_UdpDgrm.write(tempWord);
			if (ihs_currWord.tlast == 1) {
				ihs_bitCount = countBits(ihs_currWord.tkeep);
				ihs_fsmState = FSM_RESIDUE;
			}
			else
				ihs_fsmState = FSM_FORWARD_CS;
		}
		break;
	case FSM_FORWARDCSALIGNED:
		if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			AxisUdp checksumWord 			= AxisUdp(0, 0xFF, 0);
			checksumWord.tdata.range(31, 0)	= ihs_currWord.tdata.range(63, 32);
			ihs_currWord 						= siIPRX_Data.read();
			checksumWord.tdata.range(63, 32)	= ihs_currWord.tdata.range(31, 0);
			soUCc_UdpDgrm.write(checksumWord);
			soRPh_UdpDgrm.write(AxisUdp(ihs_currWord));
			if (ihs_currWord.tlast == 1) {
				if (ihs_currWord.tkeep.range(7, 4) > 0)
					ihs_fsmState = FSM_CS_RESIDUEALIGNNED;
				else
					ihs_fsmState = FSM_IDLE;
			}
		}
		break;
	case FSM_FORWARD_CS:
			if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
				soUCc_UdpDgrm.write(AxisUdp(ihs_currWord.tdata, ihs_currWord.tkeep, ihs_currWord.tlast));
				AxisUdp tempWord 				= AxisUdp(0, 0x0F, 0);
				tempWord.tdata.range(31, 0) 		= ihs_currWord.tdata.range(63, 32);
				ihs_currWord 						= siIPRX_Data.read();
				tempWord.tdata.range(63, 32) 	= ihs_currWord.tdata.range(31, 0);
				tempWord.tkeep.range(7, 4) 		= ihs_currWord.tkeep.range(3, 0);
				if (ihs_currWord.tlast == 1) {
					ihs_bitCount = countBits(ihs_currWord.tkeep);
					if (ihs_currWord.tkeep.range(7, 4) != 0)
						ihs_fsmState = FSM_RESIDUE;
					else {
						tempWord.tkeep.range(7,4) = ihs_currWord.tkeep.range(3, 0);
						tempWord.tlast = 1;
						ihs_fsmState = FSM_CS_RESIDUE;
					}
				}
				soRPh_UdpDgrm.write(tempWord);
			}
			break;
	case FSM_RESIDUE:
		if (!soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			AxisUdp tempWord = AxisUdp(0, 0, 1);
			AxisUdp csWord = AxisUdp(0, ihs_currWord.tkeep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = ihs_outWord.data.range((bitCounter * 8) - 1, 0);
			csWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0) = ihs_currWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0);
			soUCc_UdpDgrm.write(csWord);
			tempWord.tdata.range(31, 0) = ihs_currWord.tdata.range(63, 32);
			tempWord.tkeep.range(3, 0)  = ihs_currWord.tkeep.range(7, 4);
			soRPh_UdpDgrm.write(tempWord);
			ihs_fsmState = FSM_IDLE;
		}
		break;
	case FSM_CS_RESIDUE:
		if (!soUCc_UdpDgrm.full()) {
			AxisUdp csWord = AxisUdp(0, ihs_currWord.tkeep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = ihs_outWord.data.range((bitCounter * 8) - 1, 0);
			csWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0) = ihs_currWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0);
			soUCc_UdpDgrm.write(csWord);
			ihs_fsmState = FSM_IDLE;
		}
		break;
	case FSM_CS_RESIDUEALIGNNED:
		if (!soUCc_UdpDgrm.full()) {
			//axiWord csWord = {outputWord.data.range(31, 0), outputWord.keep.range(7, 4), 1};
			//soUCc_UdpDgrm.write(csWord);
			soUCc_UdpDgrm.write(AxisUdp(ihs_currWord.tdata.range(63, 32), ihs_currWord.tkeep.range(7, 4), 1));
			ihs_fsmState = FSM_IDLE;
		}
		break;
	}
} // End-of: pIpHeaderStripper
*** OBSOLETE_20200310 ***/



/******************************************************************************
 * UDP Checksum Checker (UCc)
 *
 * @param[in]  siUCc_UdpDgrm, UDP datagram stream from IpHeaderStripper (IHs).
 * @param[in]  siUCc_PsdHdrSum, Sum of the pseudo header information from [IHs].
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
        stream<ap_uint<17> > &siIHs_PsdHdrSum,
        stream<AxisUdp>      &soRPh_UdpDgrm,
        stream<ValBool>      &soRPh_CsumVal)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_IDLE=0, FSM_ACCUMULATE,
                                        FSM_STREAM } ucc_fsmState=FSM_IDLE;
    #pragma HLS RESET                       variable=ucc_fsmState
    static ap_uint<10>         ucc_wordCount=0;
    #pragma HLS RESET variable=ucc_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<32>  ucc_csum;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisUdp     currWord;
    ap_uint<17> psdHdrCsum;

    switch (ucc_fsmState) {
    case FSM_IDLE:
        if (!siIHs_UdpDgrm.empty() && !siIHs_PsdHdrSum.empty()) {
            // This is the UDP header
            siIHs_UdpDgrm.read(currWord);
            siIHs_PsdHdrSum.read(psdHdrCsum);
            soRPh_UdpDgrm.write(currWord);
            if (currWord.getUdpCsum() == 0x0000) {
                // An all zero  transmitted checksum  value means that the
                // transmitter  generated  no checksum.
                soRPh_CsumVal.write(true);
                ucc_fsmState = FSM_STREAM;
            }
            else {
                ucc_csum  = ((((psdHdrCsum +
                                currWord.getUdpLen()) +
                                currWord.getUdpDstPort()) +
                                currWord.getUdpSrcPort()));
                ucc_fsmState = FSM_ACCUMULATE;
            }
        }
        break;
    case FSM_STREAM:
        if (!siIHs_UdpDgrm.empty()) {
            siIHs_UdpDgrm.read(currWord);
            soRPh_UdpDgrm.write(currWord);
            if (currWord.tlast) {
                ucc_fsmState = FSM_IDLE;
            }
        }
        break;
    case FSM_ACCUMULATE:
        if (!siIHs_UdpDgrm.empty()) {
            siIHs_UdpDgrm.read(currWord);
            soRPh_UdpDgrm.write(currWord);
            ucc_csum = ((((ucc_csum +
                           currWord.tdata.range(63, 48)) +
                           currWord.tdata.range(47, 32)) +
                           currWord.tdata.range(31, 16)) +
                           currWord.tdata.range(15,  0));
            if (currWord.tlast) {
                ucc_csum = (ucc_csum & 0xFFFF) + (ucc_csum >> 16);
                ucc_csum = (ucc_csum & 0xFFFF) + (ucc_csum >> 16);
                ucc_csum = ~ucc_csum;
                if (ucc_csum == 0) {
                    soRPh_CsumVal.write(true);
                }
                else {
                    soRPh_CsumVal.write(false);
                }
                ucc_fsmState = FSM_IDLE;
            }
        }
        break;

    } // End-of: switch

/***

			ucc_wordCount++;
			AxiWord currWord = siIHs_UdpDgrm.read();
			if(ucc_wordCount == 3)
				receivedChecksum = (currWord.tdata.range(23, 16), currWord.tdata.range(31, 24));
			udpChecksum = ((((udpChecksum + currWord.tdata.range(63, 48)) + currWord.tdata.range(47, 32)) + currWord.tdata.range(31, 16)) + currWord.tdata.range(15, 0));
			if (currWord.tlast) {
				ucc_wordCount	= 0;
				udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
				udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
				udpChecksum = ~udpChecksum;			// Reverse the bits of the result
				ap_uint<16> tempChecksum = udpChecksum.range(15, 0);
				bool csumResult = tempChecksum == 0 || receivedChecksum == 0;
				soRPh_CsumVal.write(csumResult);
				udpChecksum = 0;
			}
		}
***/

}

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
        stream<AppData>         &soURIF_Data,
        stream<metadata> 		&soURIF_Meta,       // TODO
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
    static stream<ap_uint<17> > ssIHsToUCc_PsdHdrSum    ("ssIHsToUCc_PsdHdrSum");
    #pragma HLS STREAM variable=ssIHsToUCc_PsdHdrSum    depth=4
    //-- UDP Checksum Checker (UCc)
    static stream<AxisUdp>      ssUCcToRPh_UdpDgrm      ("ssUCcToRPh_UdpDgrm");
    #pragma HLS STREAM variable=ssUCcToRPh_UdpDgrm      depth=4096
    static stream<ValBool>      ssUCcToRPh_CsumVal      ("ssUCcToRPh_CsumVal");
    #pragma HLS STREAM variable=ssUCcToRPh_CsumVal      depth=4

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
		stream<axiWord> 		&siURIF_Data,
		stream<metadata> 		&siURIF_Meta,
		stream<ap_uint<16> > 	&siURIF_PLen,
		stream<ap_uint<64> > 	&packetData,
		stream<ap_uint<16> > 	&packetLength,
		stream<metadata> 		&udpMetadata,
		stream<ioWord> 			&outputPathWriteFunction2checksumCalculation)
{
	#pragma HLS INLINE off
	#pragma HLS pipeline II=1 enable_flush
	
	static enum opwfState{OW_IDLE = 0, OW_PSEUDOHEADER, OW_MIX, OW_STREAM, OW_RESIDUE, OW_SWCS, OW_ONLYPACKETRESIDUE} outputPathWriteFunctionState;
	static axiWord 		outputPathInputWord 	= axiWord(0, 0, 0);									// Temporary buffer for the input data word
	static ap_uint<16> 	outputPathPacketLength 	= 0;												// Temporary buffer for the packet data length
	static metadata 	tempMetadata			= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));	// Temporary buffer for the destination & source IP addresses & ports

	switch(outputPathWriteFunctionState) {
		case OW_IDLE:
			if (!siURIF_Meta.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				tempMetadata = siURIF_Meta.read();		// Read in the metadata
				udpMetadata.write(tempMetadata);				// Write the metadata in the buffer for the next stage
				ioWord checksumOutput = {0, 0};					// Temporary variable for the checksum calculation data word
				checksumOutput.data = (byteSwap32(tempMetadata.destinationSocket.addr), byteSwap32(tempMetadata.sourceSocket.addr));	// Create the first checksum calc. data word. Byte swap the addresses
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);	// Write the data word into the output
				outputPathWriteFunctionState = OW_PSEUDOHEADER;						// Move into the next state
			}
			break;
		case OW_PSEUDOHEADER:
			if (!siURIF_PLen.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				outputPathPacketLength = siURIF_PLen.read();		// Read in the payload length
				outputPathPacketLength += 8;							// Increase the length to take the UDP header into account.
				packetLength.write(outputPathPacketLength);				// Write length into the buffer for the next stage
				ioWord	checksumOutput		= {0x1100, 0};				// Create the 2nd checksum word. 0x1100 is the protocol used
				checksumOutput.data.range(63, 16) = (byteSwap16(tempMetadata.destinationSocket.port), byteSwap16(tempMetadata.sourceSocket.port), byteSwap16(outputPathPacketLength));	// Add destination & source port & packet length info for the checksum calculation
				//std::cerr << std::hex << checksumOutput << std::endl;
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);	// Write the checksum word into the output
				outputPathWriteFunctionState = OW_MIX;								// Move to the next state
			}
			break;
		case OW_MIX:
			if (!siURIF_Data.empty() && !outputPathWriteFunction2checksumCalculation.full()) {
				outputPathInputWord					= siURIF_Data.read();	// Read in the first payload length
				ioWord checksumOutput				= {0, 0};					// First payload length
				checksumOutput.data.range(15, 0) 	= (outputPathPacketLength.range(7, 0), outputPathPacketLength.range(15, 8)); 	// Packet length for the checksum calculation data
				checksumOutput.data.range(63, 32)	= outputPathInputWord.data.range(31, 0);										// Payload data copy to the checksum calculation
				if (outputPathInputWord.last == 1)	{		// When the last data word is read
					packetData.write(outputPathInputWord.data);
					if (outputPathInputWord.keep.range(7, 4) == 0) {
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
				packetData.write(outputPathInputWord.data);										// Write the realignned data word to the next stage
				ioWord 	checksumOutput				= {0, 0};
				checksumOutput.data.range(31,0)		= outputPathInputWord.data.range(63, 32);	// Realign the output data word for the next stage
				outputPathInputWord 				= siURIF_Data.read();					// Read the next data word
				checksumOutput.data.range(63, 32)	= outputPathInputWord.data.range(31, 0);
				if (outputPathInputWord.last == 1) {
					if (outputPathInputWord.keep.bit(4) == 1)		// When the last data word is read
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
				packetData.write(outputPathInputWord.data);
				outputPathWriteFunctionState	= OW_IDLE;
			}
			break;
		case OW_SWCS:
			if (!outputPathWriteFunction2checksumCalculation.full()) {
				ioWord checksumOutput		= {0, 1};
				checksumOutput.data.range(31, 0) = outputPathInputWord.data.range(63, 32);
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);
				outputPathWriteFunctionState	= OW_IDLE;
			}
			break;
		case OW_RESIDUE:
			if (!packetData.full() && !outputPathWriteFunction2checksumCalculation.full()) {
				ioWord checksumOutput		= {0, 1};
				checksumOutput.data.range(31, 0)		= outputPathInputWord.data.range(63, 32);
				outputPathWriteFunction2checksumCalculation.write(checksumOutput);
				packetData.write(outputPathInputWord.data);
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
		stream<metadata>        &udpMetadata,
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
				metadata tempMetadata 			= udpMetadata.read();
				axiWord outputWord 				= axiWord(0, 0xFF, 0);
				outputWord.data.range(15, 0) 	= byteSwap16(tempMetadata.sourceSocket.port);
				outputWord.data.range(31, 16) 	= byteSwap16(tempMetadata.destinationSocket.port);
				outputWord.data.range(47, 32)	= byteSwap16(interimLength);
				outputWord.data.range(63, 48)	= udpChecksum.read();
				//std::cerr << std::hex << outputWord.data << std::endl;
				outputPathOutData.write(outputWord);
				IpAddrPair tempIPaddresses = IpAddrPair(tempMetadata.sourceSocket.addr, tempMetadata.destinationSocket.addr);
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
		stream<axiWord>     &outputPathOutData,
		stream<ap_uint<16> > &outputPathReadFunction2addIpHeader_length)
{

#pragma HLS pipeline II=1 enable_flush

	static enum iState {IPH_IDLE, IPH_IP1, IPH_IP2, IPH_FORWARD, IPH_RESIDUE} iphState;
	static IpAddrPair ipHeaderTuple = IpAddrPair(0, 0);
	static axiWord    outputWord = axiWord(0, 0, 0);

	switch(iphState) {
	case IPH_IDLE:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathRead2addIpHeader_ipAddress.empty() &&!outputPathReadFunction2addIpHeader_length.empty() && !outputPathOutData.full()) {
			ap_uint<16> tempLength = outputPathReadFunction2addIpHeader_length.read();
			tempLength += 20;
			axiWord tempWord = axiWord(0x0000000034000045, 0xFF, 0);
			tempWord.data.range(31, 16) = byteSwap16(tempLength);
			//tempWord.data.range(31, 16) = (tempLength.range(7, 0), tempLength.range(15, 0));
			outputPathOutData.write(tempWord);
			iphState = IPH_IP1;
		}
		break;
	case IPH_IP1:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathRead2addIpHeader_ipAddress.empty() && !outputPathOutData.full()) {
			axiWord tempWord = axiWord(0x0, 0xFF, 0);
			ipHeaderTuple = outputPathRead2addIpHeader_ipAddress.read();
			tempWord.data.range(31, 0) = 0x000011FF;
			tempWord.data.range(63, 32) = byteSwap32(ipHeaderTuple.ipSa);
			outputPathOutData.write(tempWord);
			iphState = IPH_IP2;
		}
		break;
	case IPH_IP2:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathOutData.full()) {
			axiWord tempWord = axiWord(0x0, 0xFF, 0);
			outputWord = outputPathRead2addIpHeader_data.read();
			tempWord.data.range(31, 0) = byteSwap32(ipHeaderTuple.ipDa);
			tempWord.data.range(63, 32) = outputWord.data.range(31, 0);
			outputPathOutData.write(tempWord);
			iphState = IPH_FORWARD;
		}
		break;
	case IPH_FORWARD:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathOutData.full()) {
			axiWord tempWord 			= axiWord(0x0, 0x0F, 0);
			tempWord.data.range(31, 0) 	= outputWord.data.range(63, 32);
			outputWord 					= outputPathRead2addIpHeader_data.read();
			tempWord.data.range(63, 32) = outputWord.data.range(31, 0);
			if(outputWord.last) {
				if (outputWord.keep.range(7, 4) != 0) {
					tempWord.keep.range(7, 4) = 0xF;
					iphState = IPH_RESIDUE;
				}
				else {
					tempWord.keep.range(7, 4) = outputWord.keep.range(3, 0);
					tempWord.last = 1;
					iphState = IPH_IDLE;
				}
			}
			else
				tempWord.keep.range(7, 4) = 0xF;
			outputPathOutData.write(tempWord);
		}
		break;
	case IPH_RESIDUE:
			if (!outputPathOutData.full()) {
				axiWord tempWord 			= axiWord(outputWord.data.range(63, 32), outputWord.keep.range(7, 4), 1);
				outputPathOutData.write(tempWord);
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
		stream<axiWord> 		&siURIF_Data,
		stream<metadata> 		&siURIF_Meta,
		stream<ap_uint<16> > 	&siURIF_PLen,
		stream<axiWord> 		&soIPTX_Data)
{

#pragma HLS INLINE

	// Declare intermediate streams for inter-function communication
	static stream<ap_uint<64> > packetData("packetData");
	static stream<ap_uint<16> > packetLength("packetLength");
	static stream<metadata>		udpMetadata("udpMetadata");
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
			siURIF_PLen,
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
        stream<axiWord>                 &soIPTX_Data,

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
        stream<AppData>                 &soURIF_Data,
        stream<metadata>                &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>                 &siURIF_Data,
        stream<metadata>                &siURIF_Meta,
        stream<ap_uint<16> >            &siURIF_PLen,

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
