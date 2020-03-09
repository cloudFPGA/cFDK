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
#define TRACE_IBUF 1 << 1
#define TRACE_MPD  1 << 2
#define TRACE_ILC  1 << 3
#define TRACE_ICA  1 << 4
#define TRACE_ICC  1 << 5
#define TRACE_IID  1 << 6
#define TRACE_ICL  1 << 7
#define TRACE_IPR  1 << 8
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

ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
	return (inputVector.range(7,0), inputVector(15, 8), inputVector(23, 16), inputVector(31, 24));
}

ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
	return (inputVector.range(7,0), inputVector(15, 8));
}

using namespace hls;


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
   // Output stream, returning true or false to the Rx path depending on if the queried port is open or not
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
 * @param[in]  siIHs_AddrPair,The SA and DA pair of the IPv4 packet from [IHs].
 * @param[in]  siUCc_CsumVal, Checksum valid information from UdpChecksumChecker (UCc).
 *
 * @param[out]
 * @param[out] soURIF_Data, UDP data streamto [URIF]
 * @param[out] soURIF_Meta, UDP metadata stream to [URIF].
 * @param[out] soICMP_Data, Control message to InternetControlMessageProtocol[ICMP] engine.

 * @details
 *  This process handles the payload of the incoming IP4 packet and forwards it
 *  the UdpRoleInterface (URIF).
 *
 *****************************************************************************/
void pRxPacketHandler(
		stream<axiWord>     &siIHs_UdpDgrm,             // [TODO - AxisUdp]
		stream<axiWord>     &siIHs_Ip4Hdr,              // [TODO - AxisIp4]
		stream<IpAddrPair>  &siIHs_AddrPair,
		stream<ValBool>     &siUCc_CsumVal,
		stream<ap_uint<16> > &rxEng2portTable_check_req,
		stream<bool>		&portTable2rxEng_assign,

		stream<axiWord> 	&soURIF_Data,
		stream<metadata> 	&soURIF_Meta,
		stream<axiWord>		&soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

	static enum iState{IP_IDLE = 0, IP_WAIT, IP_STREAMFIRST, IP_STREAMIP, IP_STREAMREST, IP_DROP, IP_PORT_UNREACHABLE_IP_FIRST, IP_PORT_UNREACHABLE_IP, IP_PORT_UNREACHABLE_UDP, IP_PORT_UNREACHABLE_RESIDUE, IP_DRAIN} inputPathState;

	static metadata 	rxPathMetadata 		= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
	static axiWord  	bufferWord			= axiWord(0, 0, 0); 
	static axiWord		ipHdrWord			= axiWord(0, 0, 0);
	static ap_uint<4>	rxEngIpHdrWordCount	= 0;
	static ap_uint<1>	rxEngNoPayloadFlag	= 0;

	switch(inputPathState) {
	case IP_IDLE:
		if (!siIHs_UdpDgrm.empty() && !siIHs_AddrPair.empty()  && !siIHs_Ip4Hdr.empty() && !rxEng2portTable_check_req.full()) {
			//OBSOLETE_20200308 ipTuple ipAddressBuffer = incomingIPaddresses.read();
			IpAddrPair ipAddressBuffer = siIHs_AddrPair.read();
			ipHdrWord 			= siIHs_Ip4Hdr.read();
			rxEngIpHdrWordCount = ipHdrWord.data.range(3,0) - 2;
			bufferWord = siIHs_UdpDgrm.read();						// Assumes that the input stream is byte-aligned and has been stripped from the IP header. Hence the first word will be the IDP header
			(byteSwap16(bufferWord.data.range(47, 32)) > 8) ? rxEngNoPayloadFlag = 0 : rxEngNoPayloadFlag = 1;
			rxEng2portTable_check_req.write(byteSwap16(bufferWord.data.range(31, 16)));		// Send the port state query to the port table
			rxPathMetadata	= metadata(sockaddr_in(byteSwap16(bufferWord.data.range(15, 0)), ipAddressBuffer.ipSa), sockaddr_in(byteSwap16(bufferWord.data.range(31, 16)), ipAddressBuffer.ipDa));
			inputPathState 	= IP_WAIT;
		}
		break;
	case IP_WAIT:
		if (!portTable2rxEng_assign.empty() && !siUCc_CsumVal.empty()) {
			bool checksumResult = siUCc_CsumVal.read();
			bool portResult		= portTable2rxEng_assign.read();
			if(portResult && checksumResult && rxEngNoPayloadFlag == 0)
				inputPathState = IP_STREAMFIRST;
			else if (!checksumResult || rxEngNoPayloadFlag == 1)
				inputPathState = IP_DROP;
			else
				inputPathState = IP_PORT_UNREACHABLE_IP_FIRST;
		}
		break;
	case IP_STREAMFIRST:
		if (!siIHs_UdpDgrm.empty() && !soURIF_Data.full() && !soURIF_Meta.full()) {
			axiWord inputWord = siIHs_UdpDgrm.read();
			soURIF_Data.write(inputWord);
			soURIF_Meta.write(rxPathMetadata);
			if (inputWord.last == 1)
				inputPathState = IP_DRAIN;
			else
				inputPathState = IP_STREAMIP;
		}
		break;
	case IP_STREAMIP:
		if (!siIHs_UdpDgrm.empty() && !siIHs_Ip4Hdr.empty() && !soURIF_Data.full() && !soURIF_Meta.full()) {
			axiWord inputWord = siIHs_UdpDgrm.read();
			siIHs_Ip4Hdr.read();
			soURIF_Data.write(inputWord);
			if (inputWord.last == 1) {
				if (rxEngIpHdrWordCount <= 2)
					inputPathState = IP_IDLE;
				else
					inputPathState = IP_DRAIN;
			}
			else {
				if (rxEngIpHdrWordCount <= 2)
					inputPathState = IP_STREAMREST;
			}
			rxEngIpHdrWordCount -= 2;
		}
		break;
	case IP_STREAMREST:
		if (!siIHs_UdpDgrm.empty() && !soURIF_Data.full()) {
			axiWord inputWord = siIHs_UdpDgrm.read();
			soURIF_Data.write(inputWord);
			if (inputWord.last == 1) {
				inputPathState = IP_IDLE;
			}
		}
		break;
	case IP_DROP:
		if (!siIHs_UdpDgrm.empty()) {
			axiWord inputWord = siIHs_UdpDgrm.read();
			if (rxEngIpHdrWordCount > 0) {
				siIHs_Ip4Hdr.read();
				rxEngIpHdrWordCount > 2 ? rxEngIpHdrWordCount -= 2 : rxEngIpHdrWordCount = 0;
			}
			if (inputWord.last == 1) {
				if (rxEngIpHdrWordCount == 0 )
					inputPathState = IP_IDLE;
				else 
					inputPathState = IP_DRAIN;
			}
		}
		break;
	case IP_PORT_UNREACHABLE_IP_FIRST:
		if (!soICMP_Data.full()) {
			soICMP_Data.write(ipHdrWord);
			inputPathState = IP_PORT_UNREACHABLE_IP;
		}
		break;
	case IP_PORT_UNREACHABLE_IP:
		if (!siIHs_Ip4Hdr.empty() && !soICMP_Data.full()) {
			axiWord inputWord = siIHs_Ip4Hdr.read();
			if (inputWord.last == 1) {
				inputWord = axiWord(inputWord.data, 0xFF, 0);
				inputWord.data.range(63, 32) = bufferWord.data.range(31, 0);
				inputPathState = IP_PORT_UNREACHABLE_UDP;
			}
			soICMP_Data.write(inputWord);
		}	
		break;
	case IP_PORT_UNREACHABLE_UDP:
		if (!siIHs_UdpDgrm.empty() && !soICMP_Data.full()) { ///To here/////
			axiWord inputWord = siIHs_UdpDgrm.read();
			bufferWord.data = (inputWord.data.range(31, 0), bufferWord.data.range(63, 32));
			if (inputWord.last == 1) {
				if (inputWord.keep.range(7, 4) == 0x0) {	// If there's no residue
					bufferWord.keep.range(7, 4) = inputWord.keep.range(3, 0);
					bufferWord.last = 1;
					inputPathState = IP_IDLE;
				}
				else {
					bufferWord.last	= 0;
					inputPathState 	= IP_PORT_UNREACHABLE_RESIDUE;
				}
				soICMP_Data.write(bufferWord);
			}
			else
				soICMP_Data.write(bufferWord);///// from here
			bufferWord.data  = inputWord.data;
		}
		break;
	case IP_PORT_UNREACHABLE_RESIDUE:
		if (!soICMP_Data.full()) {
			bufferWord = axiWord(0, 0, 1);
			bufferWord.data.range(31, 0) 	= bufferWord.data.range(63, 32);
			bufferWord.keep.range(3, 0)		= bufferWord.keep.range(7, 4);
			soICMP_Data.write(bufferWord);
			inputPathState = IP_IDLE;
		}
		break;
	case IP_DRAIN:
		if (!siIHs_Ip4Hdr.empty()) {
			siIHs_Ip4Hdr.read();
			if (rxEngIpHdrWordCount > 2)	
				rxEngIpHdrWordCount -= 2;
			else
				inputPathState = IP_IDLE;
		}
		break;
	}
}

/******************************************************************************
 * IPv4 Hedaer Stripper (IHs)
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[out] soRPh_UdpDgrm, UDP datagram stream to RxPacketHandler (RPh).
 * @param[out] soRPh_Ip4Hdr,  The header part of the IPv4 packet as a stream to [RPh].
 * @param[out] soRPh_AddrPair,The SA and DA pair of the IPv4 packet to [RPh].
 * @param[out] soUCc_UdpDgrm, UDP datagram stream to UdpChecksumChecker (UCc).
 *
 * @details
 *  This process extracts the UDP datagram from the incoming IPv4 packet. The
 *  resulting two streams are forwarded to the RxPacketHandler (RPh) for
 *  further processing.
 *  The IPv4-source and IPv4-destination addresses are also extracted during
 *  this parsing process and are also forwarded to [RPh].
 *
 *****************************************************************************/
void pIpHeaderStripper(
		stream<axiWord>     &siIPRX_Data,          // [TODO-AxisIp4]
		stream<axiWord>     &soRPh_UdpDgrm,        // [TODO-AxisUdp]
		stream<axiWord>     &soRPh_Ip4Hdr,         // [TODO-AxisIp4]
		stream<IpAddrPair>  &soRPh_AddrPair,
		stream<AxisUdp>     &soUCc_UdpDgrm)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IHs");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {STRIP_IDLE=0,     STRIP_IP,               STRIP_SKIPOPTIONS,
                           STRIP_IP2,        STRIP_FORWARD,          STRIP_FORWARDALIGNED,
                           STRIP_FORWARD_CS, STRIP_FORWARDCSALIGNED, STRIP_RESIDUE,
                           STRIP_CS_RESIDUE, STRIP_CS_RESIDUEALIGNNED } ihs_fsmState=STRIP_IDLE;
    #pragma HLS RESET                                          variable=ihs_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<4>    ihs_bitCount;  // Counts #bytes which are valid in the last data word
    static ap_uint<3>    ihs_ipHdrWordCount;
    static axiWord       ihs_currWord;    // [TODO-AxisUdp]
    static UdpLen        ihs_udpLen;
    static Ip4HdrLen     ihs_ip4HdrLen;
    static IpAddrPair    ihs_ipAddrPair;

    switch(ihs_fsmState) {
    case STRIP_IDLE:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            ihs_currWord       = siIPRX_Data.read();
            ihs_ipHdrWordCount = 0;
            ihs_bitCount       = 0;
            ihs_udpLen = (ihs_currWord.data.range(23, 16), ihs_currWord.data.range(31, 24));
            ihs_ip4HdrLen  = ihs_currWord.data.range(3, 0);										// Store the IP header length in 32-bit words
            ihs_udpLen     = ihs_udpLen - (ihs_ip4HdrLen * 4);
            soRPh_Ip4Hdr.write(ihs_currWord);
            ihs_fsmState = STRIP_IP;
        }
        break;
    case STRIP_IP:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full()) {
            ihs_currWord = siIPRX_Data.read();
            ihs_ipHdrWordCount = 1;
            ihs_ipAddrPair.ipSa = byteSwap32(ihs_currWord.data.range(63, 32));
            soRPh_Ip4Hdr.write(ihs_currWord);
            ihs_ip4HdrLen -= 2;
            if (ihs_ip4HdrLen == 3) {
                // This a typical IPv4 header with a length of 20 bytes (5*4).
                ihs_fsmState = STRIP_IP2;
            }
            else if (ihs_ip4HdrLen > 3) {
                // This IPv4 header contains IP options that will be ignored
                ihs_fsmState = STRIP_SKIPOPTIONS;
            }
        }
        break;
    case STRIP_SKIPOPTIONS:
        if (!siIPRX_Data.empty() && !soRPh_Ip4Hdr.full() && !soRPh_AddrPair.full()) {
            ihs_currWord = siIPRX_Data.read();
            if (ihs_ipHdrWordCount == 1) {
            	ihs_ipAddrPair.ipDa = byteSwap32(ihs_currWord.data.range(31, 0));
				soRPh_AddrPair.write(ihs_ipAddrPair);
				AxisUdp checksumWord = AxisUdp(0, 0xFF, 0);
				checksumWord.tdata = (byteSwap32(ihs_ipAddrPair.ipDa), byteSwap32(ihs_ipAddrPair.ipSa));
				soUCc_UdpDgrm.write(checksumWord);
			}
			soRPh_Ip4Hdr.write(ihs_currWord);
			ihs_ip4HdrLen -= 2;
			ihs_ipHdrWordCount++;
			if (ihs_ip4HdrLen == 2) {
				ihs_fsmState = STRIP_FORWARDALIGNED;
			}
			else if (ihs_ip4HdrLen == 3) {
				ihs_fsmState = STRIP_IP2;
			}
			else if (ihs_ip4HdrLen > 3) {
				ihs_fsmState = STRIP_SKIPOPTIONS;
			}
		}
		break;
    case STRIP_IP2:
        if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_AddrPair.full()) {
            ihs_currWord = siIPRX_Data.read();
            if (ihs_ipHdrWordCount == 1) {
                // The current word is the 2nd one
            	ihs_ipAddrPair.ipDa = byteSwap32(ihs_currWord.data.range(31, 0));
				soRPh_AddrPair.write(ihs_ipAddrPair);
				AxisUdp checksumWord = AxisUdp((byteSwap32(ihs_ipAddrPair.ipDa), byteSwap32(ihs_ipAddrPair.ipSa)), 0xFF, 0);
				soUCc_UdpDgrm.write(checksumWord);
			}
			soRPh_Ip4Hdr.write(axiWord(ihs_currWord.data.range(31, 0), 0x0F, 1));
			ihs_fsmState = STRIP_FORWARD;
		}
		break;
	case STRIP_FORWARDALIGNED:
		if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			ihs_currWord 						= siIPRX_Data.read();
			AxisUdp checksumWord 			= AxisUdp(0x1100, 0xFF, 0);
			checksumWord.tdata.range(31, 16) = byteSwap16(ihs_udpLen.range(15, 0));	// Same for the UDP length.
			checksumWord.tdata.range(63, 32) = ihs_currWord.data.range(31, 0);
			soUCc_UdpDgrm.write(checksumWord);
			soRPh_UdpDgrm.write(ihs_currWord);
			ihs_fsmState = STRIP_FORWARDCSALIGNED;
		}
		break;
	case STRIP_FORWARD:
		if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			AxisUdp checksumWord 			= AxisUdp(0x1100, 0xFF, 0);   // Inject Protocol length in the pseudo header
			checksumWord.tdata.range(31, 16) = byteSwap16(ihs_udpLen.range(15, 0));	// Same for the UDP length.
			checksumWord.tdata.range(63, 32) = ihs_currWord.data.range(63, 32);
			soUCc_UdpDgrm.write(checksumWord);
			axiWord tempWord 				= axiWord(0, 0x0F, 0);
			tempWord.data.range(31, 0) 		= ihs_currWord.data.range(63, 32);
			ihs_currWord 						= siIPRX_Data.read();
			tempWord.data.range(63, 32) 	= ihs_currWord.data.range(31, 0);
			tempWord.keep.range(7, 4) 		= ihs_currWord.keep.range(3, 0);
			soRPh_UdpDgrm.write(tempWord);
			if (ihs_currWord.last == 1) {
				ihs_bitCount = countBits(ihs_currWord.keep);
				ihs_fsmState = STRIP_RESIDUE;
			}
			else
				ihs_fsmState = STRIP_FORWARD_CS;
		}
		break;
	case STRIP_FORWARDCSALIGNED:
		if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			AxisUdp checksumWord 			= AxisUdp(0, 0xFF, 0);
			checksumWord.tdata.range(31, 0)	= ihs_currWord.data.range(63, 32);
			ihs_currWord 						= siIPRX_Data.read();
			checksumWord.tdata.range(63, 32)	= ihs_currWord.data.range(31, 0);
			soUCc_UdpDgrm.write(checksumWord);
			soRPh_UdpDgrm.write(ihs_currWord);
			if (ihs_currWord.last == 1) {
				if (ihs_currWord.keep.range(7, 4) > 0)
					ihs_fsmState = STRIP_CS_RESIDUEALIGNNED;
				else
					ihs_fsmState = STRIP_IDLE;
			}
		}
		break;
	case STRIP_FORWARD_CS:
			if (!siIPRX_Data.empty() && !soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
				soUCc_UdpDgrm.write(AxisUdp(ihs_currWord.data, ihs_currWord.keep, ihs_currWord.last));
				axiWord tempWord 				= axiWord(0, 0x0F, 0);
				tempWord.data.range(31, 0) 		= ihs_currWord.data.range(63, 32);
				ihs_currWord 						= siIPRX_Data.read();
				tempWord.data.range(63, 32) 	= ihs_currWord.data.range(31, 0);
				tempWord.keep.range(7, 4) 		= ihs_currWord.keep.range(3, 0);
				if (ihs_currWord.last == 1) {
					ihs_bitCount = countBits(ihs_currWord.keep);
					if (ihs_currWord.keep.range(7, 4) != 0)
						ihs_fsmState = STRIP_RESIDUE;
					else {
						tempWord.keep.range(7,4) = ihs_currWord.keep.range(3, 0);
						tempWord.last = 1;
						ihs_fsmState = STRIP_CS_RESIDUE;
					}
				}
				soRPh_UdpDgrm.write(tempWord);
			}
			break;
	case STRIP_RESIDUE:
		if (!soUCc_UdpDgrm.full() && !soRPh_UdpDgrm.full()) {
			axiWord tempWord = axiWord(0, 0, 1);
			AxisUdp csWord = AxisUdp(0, ihs_currWord.keep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = ihs_outWord.data.range((bitCounter * 8) - 1, 0);
			csWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0) = ihs_currWord.data.range((ihs_bitCount.to_int() * 8) - 1, 0);
			soUCc_UdpDgrm.write(csWord);
			tempWord.data.range(31, 0) = ihs_currWord.data.range(63, 32);
			tempWord.keep.range(3, 0)  = ihs_currWord.keep.range(7, 4);
			soRPh_UdpDgrm.write(tempWord);
			ihs_fsmState = STRIP_IDLE;
		}
		break;
	case STRIP_CS_RESIDUE:
		if (!soUCc_UdpDgrm.full()) {
			AxisUdp csWord = AxisUdp(0, ihs_currWord.keep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = ihs_outWord.data.range((bitCounter * 8) - 1, 0);
			csWord.tdata.range((ihs_bitCount.to_int() * 8) - 1, 0) = ihs_currWord.data.range((ihs_bitCount.to_int() * 8) - 1, 0);
			soUCc_UdpDgrm.write(csWord);
			ihs_fsmState = STRIP_IDLE;
		}
		break;
	case STRIP_CS_RESIDUEALIGNNED:
		if (!soUCc_UdpDgrm.full()) {
			//axiWord csWord = {outputWord.data.range(31, 0), outputWord.keep.range(7, 4), 1};
			//soUCc_UdpDgrm.write(csWord);
			soUCc_UdpDgrm.write(AxisUdp(ihs_currWord.data.range(63, 32), ihs_currWord.keep.range(7, 4), 1));
			ihs_fsmState = STRIP_IDLE;
		}
		break;	
	}
} // End-of: pIpHeaderStripper


/******************************************************************************
 * UDP Checksum Checker (UCc)
 *
 * @param[in]  siUCc_UdpDgrm, UDP datagram stream from IpHeaderStripper (IHs).
 * @param[out] soRPh_CsumVal, Checksum valid information to RxPacketHandler (RPh).
 *
 * @details
 *  This process accumulates the checksum over the UDP header and the UDP data.
 *  The computed checksum is compared with the embedded checksum when 'TLAST'
 *  is reached, and the result is forwarded to the RxPacketHandler (RPh).
 *
 *****************************************************************************/

void pUdpChecksumChecker(
        stream<AxisUdp>     &siIHs_UdpDgrm,
        stream<ValBool>     &soRPh_CsumVal)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<10>         ucc_wordCount=0;
    #pragma HLS RESET variable=ucc_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------


	static ap_uint<32> 	udpChecksum 		= 0;
	static ap_uint<16>	receivedChecksum	= 0;


    if (!siIHs_UdpDgrm.empty()) {
        ucc_wordCount++;
        AxisUdp currWord = siIHs_UdpDgrm.read();
        if(ucc_wordCount == 3)
			receivedChecksum = (currWord.tdata.range(23, 16), currWord.tdata.range(31, 24));
		udpChecksum = ((((udpChecksum + currWord.tdata.range(63, 48)) + currWord.tdata.range(47, 32)) + currWord.tdata.range(31, 16)) + currWord.tdata.range(15, 0));
		if (currWord.tlast) {
			ucc_wordCount	= 0;
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = ~udpChecksum;			// Reverse the bits of the result
			ap_uint<16> tempChecksum = udpChecksum.range(15, 0);
			soRPh_CsumVal.write(tempChecksum == 0 || receivedChecksum == 0);
			udpChecksum = 0;
		}
	}
}

/*****************************************************************************/
/* @brief { Brief description (1-2 lines) }
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[in]  siURIF_OpnReq, UDP open port request from UdpRoleInterface (URIF).
 * @param[out] soURIF_OpnRep, UDP open port reply to [URIF].
 * @param[in]  siURIF_ClsReq, UDP close port request from [URIF].
 * @param[out] soURIF_Data,   UDP data stream to [URIF].
 * @param[out] soURIF_Meta,   UDP metadata stream to [URIF].
 * @param[out] soICMP_Data,   Control message to InternetControlMessageProtocol[ICMP] engine.
 *
 *****************************************************************************/
void rxEngine(
        stream<axiWord>         &siIPRX_Data,
        stream<ap_uint<16> > 	&siURIF_OpnReq,
        stream<bool> 			&siURIF_OpnRep,
        stream<ap_uint<16> >	&siURIF_ClsReq,
        stream<axiWord> 		&soURIF_Data,
        stream<metadata> 		&soURIF_Meta,
        stream<axiWord> 		&soICMP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- IP Header Stripper (IHs)
    static stream<axiWord>      ssIHsToRXh_UdpDgrm      ("ssIHsToRXh_UdpDgrm");
    #pragma HLS STREAM variable=ssIHsToRXh_UdpDgrm      depth=4096
    static stream<axiWord>      ssIHsToRPh_Ip4Hdr       ("ssIHsToRPh_Ip4Hdr");
    #pragma HLS STREAM variable=ssIHsToRPh_Ip4Hdr       depth=32
    static stream<IpAddrPair>   ssIHsToRPh_AddrPair     ("ssIHsToRPh_AddrPair");
    #pragma HLS STREAM variable=ssIHsToRPh_AddrPair     depth=8
    static stream<AxisUdp>      ssIHsToUCc_UdpDgrm      ("ssIHsToUCc_UdpDgrm");
    #pragma HLS STREAM variable=ssIHsToUCc_UdpDgrm      depth=8

    //-- UDP Checksum Checker (UCc)
    static stream<bool>         ssUCcToRPh_CsumVal      ("ssUCcToRPh_CsumVal");
    #pragma HLS STREAM variable=ssUCcToRPh_CsumVal      depth=4

    //-- UDP Packet Handler (UPh)
    static stream<UdpPort>      ssUPhToUPt_PortStateReq ("ssUPhToUPt_PortStateReq");
    #pragma HLS STREAM variable=ssUPhToUPt_PortStateReq depth=2

    //-- UDP Port Table (UPt)
    static stream<StsBool>      ssUPtToUPh_PortStateRep ("ssUPtToUPh_PortStateRep");
    #pragma HLS STREAM variable=ssUPtToUPh_PortStateRep depth=2


    pIpHeaderStripper(
            siIPRX_Data,
            ssIHsToRXh_UdpDgrm,
            ssIHsToRPh_Ip4Hdr,
            ssIHsToRPh_AddrPair,
            ssIHsToUCc_UdpDgrm);

    pUdpChecksumChecker(
            ssIHsToUCc_UdpDgrm,
            ssUCcToRPh_CsumVal);

    pRxPacketHandler(
            ssIHsToRXh_UdpDgrm,
            ssIHsToRPh_Ip4Hdr,
            ssIHsToRPh_AddrPair,
            ssUCcToRPh_CsumVal,
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

/*****************************************************************************/
/* @brief { Brief description (1-2 lines) }
 * @ingroup (<groupname> [<groupname> <groupname>])
 *
 * @param [(dir)] <parameter-name> { parameter description }
 * @param[in]     _inArg1 Description of first function argument.
 * @param[out]    _outArg2 Description of second function argument.
 * @param[in,out] _inoutArg3 Description of third function argument.
 *
 * @return { description of the return value }.
 *****************************************************************************/
void txEngine(
		stream<axiWord> 		&siURIF_Data,
		stream<metadata> 		&siURIF_Meta,
		stream<ap_uint<16> > 	&siURIF_PLen,
		stream<axiWord> 		&soIPTX_Data) {

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
        stream<axiWord>                 &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<axiWord>                 &soIPTX_Data,

        //------------------------------------------------------
        //-- URIF / Control Port Interfaces
        //------------------------------------------------------
        stream<ap_uint<16> >            &siURIF_OpnReq,
        stream<bool>                    &soURIF_OpnRep,
        stream<ap_uint<16> >            &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>                 &soURIF_Data,
        stream<metadata>                &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>                 &siURIF_Data,
        stream<metadata>                &siURIF_Meta,
        stream<ap_uint<16> >            &siURIF_PLen,



        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<axiWord>                 &soICMP_Data)
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

    rxEngine(
            siIPRX_Data,
            siURIF_OpnReq,
            soURIF_OpnRep,
            siURIF_ClsReq,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    txEngine(
            siURIF_Data,
            siURIF_Meta,
            siURIF_PLen,
            soIPTX_Data);
}
