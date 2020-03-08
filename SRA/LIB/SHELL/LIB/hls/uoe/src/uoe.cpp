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
//#include "../../toe/test/test_toe_utils.hpp"

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

void portTable(
		stream<ap_uint<16> >&	rxEng2portTable_check_req,		// Input stream, which queries the port table to determine if the provided port is open
		stream<ap_uint<16> >&	siURIF_OpnReq,			// Input stream, which allows the application to specify which port's state to toggle
		stream<ap_uint<16> >&	siURIF_ClsReq,			// Input stream, which allows the application to release a port upon completing its use
		stream<bool>&			portTable2app_port_assign,		// Output stream, over which the port table returns an assigned port to the application
		stream<bool>&			portTable2rxEng_check_rsp)
{	// Output stream, returning true or false to the Rx path depending on if the queried port is open or not
#pragma HLS pipeline II=1 enable_flush

	static bool portTable[65536];
	#pragma HLS RESOURCE variable=portTable core=RAM_T2P_BRAM
	#pragma HLS DEPENDENCE variable=portTable inter false

	if (!rxEng2portTable_check_req.empty())	{					// This part handles querying the state of a port when a packet is received on the Rx side.
		ap_uint<16>	currPort = rxEng2portTable_check_req.read();
		portTable2rxEng_check_rsp.write(portTable[currPort]);
	}
	else if (!siURIF_OpnReq.empty()) {
		ap_uint<16> portToOpen	= siURIF_OpnReq.read();
		if (!portTable[portToOpen])
			portTable2app_port_assign.write(true);
		else
			portTable2app_port_assign.write(false);
		portTable[portToOpen] = true;
	}
	else if (!siURIF_ClsReq.empty()) {
		ap_uint<16> releasedPort 	= siURIF_ClsReq.read();
		portTable[releasedPort] 	= false;
	}
}

void inputPathRxEngine(
		stream<axiWord>		&inputPathInputData,
		stream<ipTuple>		&incomingIPaddresses,
		stream<axiWord>		&inputPathIPheader,
		stream<ap_uint<16> > &rxEng2portTable_check_req,
		stream<bool>		&portTable2rxEng_assign,
		stream<bool> 		&rxChecksum2rxEngine,
		stream<axiWord> 	&inputPathOutputData,
		stream<metadata> 	&soURIF_Meta,
		stream<axiWord>		&soICMP_Data)
{
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
		if (!inputPathInputData.empty() && !incomingIPaddresses.empty()  && !inputPathIPheader.empty() && !rxEng2portTable_check_req.full()) {
			ipTuple ipAddressBuffer = incomingIPaddresses.read();
			//axiWord inputWord = {0, 0xFF, 0};
			ipHdrWord 			= inputPathIPheader.read();
			rxEngIpHdrWordCount = ipHdrWord.data.range(3,0) - 2;
			bufferWord = inputPathInputData.read();						// Assumes that the input stream is byte-aligned and has been stripped from the IP header. Hence the first word will be the IDP header
			(byteSwap16(bufferWord.data.range(47, 32)) > 8) ? rxEngNoPayloadFlag = 0 : rxEngNoPayloadFlag = 1;
			rxEng2portTable_check_req.write(byteSwap16(bufferWord.data.range(31, 16)));		// Send the port state query to the port table
			rxPathMetadata	= metadata(sockaddr_in(byteSwap16(bufferWord.data.range(15, 0)), ipAddressBuffer.sourceIP), sockaddr_in(byteSwap16(bufferWord.data.range(31, 16)), ipAddressBuffer.destinationIP));
			inputPathState 	= IP_WAIT;
		}
		break;
	case IP_WAIT:
		if (!portTable2rxEng_assign.empty() && !rxChecksum2rxEngine.empty()) {
			bool checksumResult = rxChecksum2rxEngine.read();
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
		if (!inputPathInputData.empty() && !inputPathOutputData.full() && !soURIF_Meta.full()) {
			axiWord inputWord = inputPathInputData.read();	
			inputPathOutputData.write(inputWord);
			soURIF_Meta.write(rxPathMetadata);
			if (inputWord.last == 1)
				inputPathState = IP_DRAIN;
			else
				inputPathState = IP_STREAMIP;
		}
		break;
	case IP_STREAMIP:
		if (!inputPathInputData.empty() && !inputPathIPheader.empty() && !inputPathOutputData.full() && !soURIF_Meta.full()) {
			axiWord inputWord = inputPathInputData.read();	
			inputPathIPheader.read();
			inputPathOutputData.write(inputWord);
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
		if (!inputPathInputData.empty() && !inputPathOutputData.full()) {
			axiWord inputWord = inputPathInputData.read();
			inputPathOutputData.write(inputWord);
			if (inputWord.last == 1) {
				inputPathState = IP_IDLE;
			}
		}
		break;
	case IP_DROP:
		if (!inputPathInputData.empty()) {
			axiWord inputWord = inputPathInputData.read();
			if (rxEngIpHdrWordCount > 0) {
				inputPathIPheader.read();
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
		if (!inputPathIPheader.empty() && !soICMP_Data.full()) {
			axiWord inputWord = inputPathIPheader.read();
			if (inputWord.last == 1) {
				inputWord = axiWord(inputWord.data, 0xFF, 0);
				inputWord.data.range(63, 32) = bufferWord.data.range(31, 0);
				inputPathState = IP_PORT_UNREACHABLE_UDP;
			}
			soICMP_Data.write(inputWord);
		}	
		break;
	case IP_PORT_UNREACHABLE_UDP:
		if (!inputPathInputData.empty() && !soICMP_Data.full()) { ///To here/////
			axiWord inputWord = inputPathInputData.read();
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
		if (!inputPathIPheader.empty()) {
			inputPathIPheader.read();
			if (rxEngIpHdrWordCount > 2)	
				rxEngIpHdrWordCount -= 2;
			else
				inputPathState = IP_IDLE;
		}
		break;
	}
}

void stripIpHeader(
		stream<axiWord> &siIPRX_Data,
		stream<axiWord> &strip2inputPath_data,
		stream<axiWord> &strip2inputPath_IpHeader,
		stream<ipTuple> &strip2inputPath_IP,
		stream<axiWord> &strip2rxChecksum)
{
	#pragma HLS INLINE off
	#pragma HLS pipeline II=1 enable_flush

	static enum sState{STRIP_IDLE = 0, STRIP_IP, STRIP_SKIPOPTIONS, STRIP_IP2, STRIP_FORWARD, STRIP_FORWARDALIGNED, STRIP_FORWARD_CS, STRIP_FORWARDCSALIGNED, STRIP_RESIDUE, STRIP_CS_RESIDUE, STRIP_CS_RESIDUEALIGNNED} stripState;
	static axiWord 		outputWord		= axiWord(0, 0, 0);
	static ipTuple		stripIpTuple 	= ipTuple(0, 0);
	static ap_uint<16>	udpLength 		= 0;
	static ap_uint<4>	ipHeaderLength	= 0;
	static ap_uint<4>	bitCounter		= 0; 		// Used to count the number of bytes which are valid in the last data word
	static ap_uint<3>	ipHdrWordCount	= 0;

	switch(stripState) {
	case STRIP_IDLE:
		if (!siIPRX_Data.empty() && !strip2inputPath_IpHeader.full()) {
			bitCounter		= 0;
			ipHdrWordCount	= 0;
			outputWord 		= siIPRX_Data.read();
			udpLength 		= (outputWord.data.range(23, 16), outputWord.data.range(31, 24));	// Store the IP packet length
			ipHeaderLength 	= outputWord.data.range(3, 0);										// Store the IP header length in 32-bit words
			udpLength		= udpLength - (ipHeaderLength * 4);
			strip2inputPath_IpHeader.write(outputWord);
			stripState 		= STRIP_IP;
		}
		break;
	case STRIP_IP:
		if (!siIPRX_Data.empty() && !strip2inputPath_IpHeader.full()) {
			outputWord = siIPRX_Data.read();
			stripIpTuple.sourceIP = byteSwap32(outputWord.data.range(63, 32));
			strip2inputPath_IpHeader.write(outputWord);
			ipHdrWordCount	= 1;
			ipHeaderLength -= 2;
			if (ipHeaderLength == 3)				// Check the remaining IP Hdr legth ad determine if there are options that need to be ignored or if the next word contains UDP Header
				stripState = STRIP_IP2;
			else if (ipHeaderLength > 3)
				stripState = STRIP_SKIPOPTIONS;
		}
		break;
	case STRIP_SKIPOPTIONS:
		if (!siIPRX_Data.empty() && !strip2inputPath_IpHeader.full()  && !strip2inputPath_IP.full()) {
			outputWord = siIPRX_Data.read();
			if (ipHdrWordCount == 1) {
				stripIpTuple.destinationIP = byteSwap32(outputWord.data.range(31, 0));
				strip2inputPath_IP.write(stripIpTuple);
				axiWord checksumWord = axiWord(0, 0xFF, 0);
				checksumWord.data = (byteSwap32(stripIpTuple.destinationIP), byteSwap32(stripIpTuple.sourceIP));
				strip2rxChecksum.write(checksumWord);
			}
			strip2inputPath_IpHeader.write(outputWord);
			ipHeaderLength -= 2;
			ipHdrWordCount++;
			if (ipHeaderLength == 2)
				stripState = STRIP_FORWARDALIGNED;
			else if (ipHeaderLength == 3)
				stripState = STRIP_IP2;
			else if (ipHeaderLength > 3)
				stripState = STRIP_SKIPOPTIONS;
		}
		break;
	case STRIP_IP2:
		if (!siIPRX_Data.empty() && !strip2rxChecksum.full() && !strip2inputPath_IP.full()) {
			outputWord = siIPRX_Data.read();
			if (ipHdrWordCount == 1) {
				stripIpTuple.destinationIP = byteSwap32(outputWord.data.range(31, 0));
				strip2inputPath_IP.write(stripIpTuple);
				axiWord checksumWord = axiWord((byteSwap32(stripIpTuple.destinationIP), byteSwap32(stripIpTuple.sourceIP)), 0xFF, 0);
				strip2rxChecksum.write(checksumWord);
			}
			strip2inputPath_IpHeader.write(axiWord(outputWord.data.range(31, 0), 0x0F, 1));
			stripState = STRIP_FORWARD;
		}
		break;
	case STRIP_FORWARDALIGNED:
		if (!siIPRX_Data.empty() && !strip2rxChecksum.full() && !strip2inputPath_data.full()) {
			outputWord 						= siIPRX_Data.read();
			axiWord checksumWord 			= axiWord(0x1100, 0xFF, 0);
			checksumWord.data.range(31, 16) = byteSwap16(udpLength.range(15, 0));	// Same for the UDP length.
			checksumWord.data.range(63, 32) = outputWord.data.range(31, 0);
			strip2rxChecksum.write(checksumWord);
			strip2inputPath_data.write(outputWord);
			stripState = STRIP_FORWARDCSALIGNED;
		}
		break;
	case STRIP_FORWARD:
		if (!siIPRX_Data.empty() && !strip2rxChecksum.full() && !strip2inputPath_data.full()) {
			axiWord checksumWord 			= axiWord(0x1100, 0xFF, 0);   // Inject Protocol length in the pseudo header
			checksumWord.data.range(31, 16) = byteSwap16(udpLength.range(15, 0));	// Same for the UDP length.
			checksumWord.data.range(63, 32) = outputWord.data.range(63, 32);
			strip2rxChecksum.write(checksumWord);
			axiWord tempWord 				= axiWord(0, 0x0F, 0);
			tempWord.data.range(31, 0) 		= outputWord.data.range(63, 32);
			outputWord 						= siIPRX_Data.read();
			tempWord.data.range(63, 32) 	= outputWord.data.range(31, 0);
			tempWord.keep.range(7, 4) 		= outputWord.keep.range(3, 0);
			strip2inputPath_data.write(tempWord);
			if (outputWord.last == 1) {
				bitCounter = countBits(outputWord.keep);
				stripState = STRIP_RESIDUE;
			}
			else
				stripState = STRIP_FORWARD_CS;
		}
		break;
	case STRIP_FORWARDCSALIGNED:
		if (!siIPRX_Data.empty() && !strip2rxChecksum.full() && !strip2inputPath_data.full()) {
			axiWord checksumWord 			= axiWord(0, 0xFF, 0);
			checksumWord.data.range(31, 0)	= outputWord.data.range(63, 32);
			outputWord 						= siIPRX_Data.read();
			checksumWord.data.range(63, 32)	= outputWord.data.range(31, 0);
			strip2rxChecksum.write(checksumWord);
			strip2inputPath_data.write(outputWord);
			if (outputWord.last == 1) {
				if (outputWord.keep.range(7, 4) > 0)
					stripState = STRIP_CS_RESIDUEALIGNNED;
				else
					stripState = STRIP_IDLE;
			}
		}
		break;
	case STRIP_FORWARD_CS:
			if (!siIPRX_Data.empty() && !strip2rxChecksum.full() && !strip2inputPath_data.full()) {
				strip2rxChecksum.write(outputWord);
				//std::cerr << std::hex << outputWord.data << std::endl;
				axiWord tempWord 				= axiWord(0, 0x0F, 0);
				tempWord.data.range(31, 0) 		= outputWord.data.range(63, 32);
				outputWord 						= siIPRX_Data.read();
				tempWord.data.range(63, 32) 	= outputWord.data.range(31, 0);
				tempWord.keep.range(7, 4) 		= outputWord.keep.range(3, 0);
				if (outputWord.last == 1) {
					bitCounter = countBits(outputWord.keep);
					if (outputWord.keep.range(7, 4) != 0)
						stripState = STRIP_RESIDUE;
					else {
						tempWord.keep.range(7,4) = outputWord.keep.range(3, 0);
						tempWord.last = 1;
						stripState = STRIP_CS_RESIDUE;
					}
				}
				strip2inputPath_data.write(tempWord);
			}
			break;
	case STRIP_RESIDUE:
		if (!strip2rxChecksum.full() && !strip2inputPath_data.full()) {
			axiWord tempWord = axiWord(0, 0, 1);
			axiWord csWord = axiWord(0, outputWord.keep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = outputWord.data.range((bitCounter * 8) - 1, 0);
			csWord.data.range((bitCounter.to_int() * 8) - 1, 0) = outputWord.data.range((bitCounter.to_int() * 8) - 1, 0);
			strip2rxChecksum.write(csWord);
			////std::cerr << std::hex << outputWord.data << std::endl;
			tempWord.data.range(31, 0) = outputWord.data.range(63, 32);
			tempWord.keep.range(3, 0)  = outputWord.keep.range(7, 4);
			//std::cerr << std::hex << tempWord.data << std::endl;
			strip2inputPath_data.write(tempWord);
			stripState = STRIP_IDLE;
		}
		break;
	case STRIP_CS_RESIDUE:
		if (!strip2rxChecksum.full()) {
			axiWord csWord = axiWord(0, outputWord.keep, 1);
			//OBSOLETE-20180813 csWord.data.range((bitCounter * 8) - 1, 0) = outputWord.data.range((bitCounter * 8) - 1, 0);
			csWord.data.range((bitCounter.to_int() * 8) - 1, 0) = outputWord.data.range((bitCounter.to_int() * 8) - 1, 0);
			strip2rxChecksum.write(csWord);
			stripState = STRIP_IDLE;
		}
		break;
	case STRIP_CS_RESIDUEALIGNNED:
		if (!strip2rxChecksum.full()) {
			//axiWord csWord = {outputWord.data.range(31, 0), outputWord.keep.range(7, 4), 1};
			//strip2rxChecksum.write(csWord);
			strip2rxChecksum.write(axiWord(outputWord.data.range(63, 32), outputWord.keep.range(7, 4), 1));
			stripState = STRIP_IDLE;
		}
		break;	
	}
}

void rxEngineUdpChecksumVerification(stream<axiWord>	&dataIn,
						    		 stream<bool> 		&udpChecksumOut) {
#pragma HLS INLINE off
#pragma HLS pipeline II=1 enable_flush

	static ap_uint<32> 	udpChecksum 		= 0;
	static ap_uint<16>	receivedChecksum	= 0;
	static ap_uint<10>	wordCounter			= 0;

	if (!dataIn.empty()) {
		wordCounter++;
		axiWord inputWord = dataIn.read();
		if(wordCounter == 3)
			receivedChecksum = (inputWord.data.range(23, 16), inputWord.data.range(31, 24));
		udpChecksum = ((((udpChecksum + inputWord.data.range(63, 48)) + inputWord.data.range(47, 32)) + inputWord.data.range(31, 16)) + inputWord.data.range(15, 0));
		if (inputWord.last) {
			wordCounter	= 0;
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
			udpChecksum = ~udpChecksum;			// Reverse the bits of the result
			ap_uint<16> tempChecksum = udpChecksum.range(15, 0);
			udpChecksumOut.write(tempChecksum == 0 || receivedChecksum == 0);
			udpChecksum = 0;
		}
	}
}

/*****************************************************************************/
/* @brief { Brief description (1-2 lines) }
 *
 * @param[in]  siIPRX_Data,   IP4 data stream from IpRxHAndler (IPRX).
 * @param[in]  siURIF_OpnReq,
 *
 *
 *****************************************************************************/
void rxEngine(
		stream<axiWord>         &siIPRX_Data,
		stream<ap_uint<16> > 	&siURIF_OpnReq,
		stream<ap_uint<16> >	&siURIF_ClsReq,
		stream<bool> 			&portTable2app_port_assign,
		stream<axiWord> 		&inputPathOutData,
		stream<metadata> 		&soURIF_Meta,
		stream<axiWord> 		&soICMP_Data) {

#pragma HLS INLINE
	static stream<ap_uint<16> >		rxEng2portTable_check_req("rxEng2portTable_check_req");
	static stream<bool>				portTable2rxEng_assign("portTable2rxEng_assign");
	static stream<axiWord>			strip2inputPath_data("strip2inputPath_data");
	static stream<ipTuple>			strip2inputPath_IP("strip2inputPath_IP");
	static stream<axiWord>			strip2rxChecksum("strip2rxChecksum");
	static stream<bool>				rxChecksum2rxEngine("rxChecksum2rxEngine");
	static stream<axiWord>			strip2inputPath_IPheader("strip2inputPath_IPheader");

	#pragma HLS STREAM variable=strip2inputPath_data 						depth=4096
	#pragma HLS STREAM variable=strip2inputPath_IPheader					depth=32
	#pragma HLS STREAM variable=strip2inputPath_IP							depth=8
	#pragma HLS STREAM variable=strip2rxChecksum 							depth=8
	#pragma HLS STREAM variable=rxChecksum2rxEngine							depth=4

	stripIpHeader(
			siIPRX_Data,
			strip2inputPath_data,
			strip2inputPath_IPheader,
			strip2inputPath_IP,
			strip2rxChecksum);

	rxEngineUdpChecksumVerification(
			strip2rxChecksum,
			rxChecksum2rxEngine);

    inputPathRxEngine(
    		strip2inputPath_data,
			strip2inputPath_IP,
			strip2inputPath_IPheader,
			rxEng2portTable_check_req,
			portTable2rxEng_assign,
			rxChecksum2rxEngine,
			inputPathOutData,
			soURIF_Meta,
			soICMP_Data);

	portTable(
			rxEng2portTable_check_req,
			siURIF_OpnReq,
			siURIF_ClsReq,
			portTable2app_port_assign,
			portTable2rxEng_assign);
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

void udpChecksumCalculation(stream<ioWord>&	dataIn,
						    stream<ap_uint<16> >&	udpChecksumOut) {
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
		stream<ap_uint<64> > &packetData,
		stream<ap_uint<16> > &packetLength,
		stream<metadata> &udpMetadata,
		stream<ap_uint<16> > &udpChecksum,
		stream<axiWord> &outputPathOutData,
		stream<ipTuple> &outIPaddresses,
		stream<ap_uint<16> > &outputPathReadFunction2addIpHeader_length)
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
				ipTuple tempIPaddresses = ipTuple(tempMetadata.sourceSocket.addr, tempMetadata.destinationSocket.addr);
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
		stream<axiWord> &outputPathRead2addIpHeader_data,
		stream<ipTuple> &outputPathRead2addIpHeader_ipAddress,
		stream<axiWord> &outputPathOutData,
		stream<ap_uint<16> > &outputPathReadFunction2addIpHeader_length)
{

#pragma HLS pipeline II=1 enable_flush

	static enum iState {IPH_IDLE, IPH_IP1, IPH_IP2, IPH_FORWARD, IPH_RESIDUE} iphState;
	static ipTuple ipHeaderTuple = ipTuple(0, 0);
	static axiWord outputWord = axiWord(0, 0, 0);

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
			tempWord.data.range(63, 32) = byteSwap32(ipHeaderTuple.sourceIP);
			outputPathOutData.write(tempWord);
			iphState = IPH_IP2;
		}
		break;
	case IPH_IP2:
		if(!outputPathRead2addIpHeader_data.empty() && !outputPathOutData.full()) {
			axiWord tempWord = axiWord(0x0, 0xFF, 0);
			outputWord = outputPathRead2addIpHeader_data.read();
			tempWord.data.range(31, 0) = byteSwap32(ipHeaderTuple.destinationIP);
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
	static stream<ipTuple>		outputPathRead2addIpHeader_ipAddress("outputPathRead2addIpHeader_ipAddress");
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
 *
 * -- URIF / Open-Port Interfaces
 * @param[in]  siURIF_OpnReq, UDP open connection request from UdpRoleInterface (URIF).
 * @param[out] soURIF_OpnRep, UDP open connection reply to [URIF].
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
        //-----------------------------------------------------
        stream<axiWord>                 &soURIF_Data,
        stream<metadata>                &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //-----------------------------------------------------
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

#pragma HLS INTERFACE axis      port=siURIF_OpnReq     name=siURIF_OpnReq

#pragma HLS INTERFACE axis      port=siIPRX_Data       name=siIPRX_Data
#pragma HLS INTERFACE axis      port=soIPTX_Data       name=soIPTX_Data

#pragma HLS INTERFACE axis      port=siURIF_OpnReq     name=siURIF_OpnReq
#pragma HLS INTERFACE axis      port=soURIF_OpnRep     name=soURIF_OpnRep
#pragma HLS INTERFACE axis      port=siURIF_ClsReq     name=siURIF_ClsReq

#pragma HLS INTERFACE axis      port=soURIF_Data       name=soURIF_Data
#pragma HLS INTERFACE axis      port=soURIF_Meta       name=soURIF_Meta
#pragma HLS DATA_PACK           port=soURIF_Meta

#pragma HLS INTERFACE axis      port=siURIF_Data       name=siURIF_Data
#pragma HLS INTERFACE axis      port=siURIF_Meta       name=siURIF_Meta
#pragma HLS DATA_PACK           port=siURIF_Meta
#pragma HLS INTERFACE axis      port=siURIF_PLen       name=siURIF_PLen

#pragma HLS INTERFACE axis      port=soICMP_Data       name=soICMP_Data

#endif

	// Data pack all of the interfaces that consist of structs (except the axiWord ones, which are to be mapped to the AXI4S I/F fields)

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    rxEngine(
            siIPRX_Data,
            siURIF_OpnReq,
            siURIF_ClsReq,
            soURIF_OpnRep,
            soURIF_Data,
            soURIF_Meta,
            soICMP_Data);

    txEngine(
            siURIF_Data,
            siURIF_Meta,
            siURIF_PLen,
            soIPTX_Data);
}