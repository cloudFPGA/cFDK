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


/*****************************************************************************
 * @file       : udp.hpp
 * @brief      : UDP offload engine.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * @details    : This process implements the original UDP core engine. It is
 *               not used by the cloudFPGA system and was replaced by the
 *               UDP Offload Engine (UOE).
 *****************************************************************************/

#ifndef _UDP_H
#define _UDP_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <cstdlib>

using namespace hls;

#define MTU	1500			// Maximum Transmission Unit in bytes

extern uint32_t clockCounter;

struct sockaddr_in {
    ap_uint<16>		port;   /* port in network byte order */
    ap_uint<32>		addr;   /* internet address */
    sockaddr_in() 	{}
    sockaddr_in(ap_uint<16> port, ap_uint<32> addr)
    				: port(port), addr(addr) {}
};

struct metadata {
	sockaddr_in		sourceSocket;
	sockaddr_in		destinationSocket;
	metadata() 		{}
	metadata(sockaddr_in sourceSocket, sockaddr_in destinationSocket)
			 	 	: sourceSocket(sourceSocket), destinationSocket(destinationSocket) {}
};

struct ipTuple {
	ap_uint<32>		sourceIP;
	ap_uint<32>		destinationIP;
	ipTuple() 		{}
	ipTuple(ap_uint<32> sourceIP, ap_uint<32> destinationIP)
					: sourceIP(sourceIP), destinationIP(destinationIP) {}
};

struct ioWord  {
	ap_uint<64>		data;
	ap_uint<1>		eop;
};

struct axiWord {
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
	axiWord() 		{}
	axiWord(ap_uint<64> data, ap_uint<8> keep, ap_uint<1> last)
					:data(data), keep(keep), last(last) {}
};

void udp(
		stream<axiWord> 		&inputPathInData,
		stream<axiWord> 		&inputpathOutData,
		stream<ap_uint<16> > 	&openPort,
		stream<bool> 			&confirmPortStatus,
		stream<metadata> 		&inputPathOutputMetadata,
		stream<ap_uint<16> > 	&portRelease, 				// Input Path Streams
	    stream<axiWord> 		&outputPathInData,
		stream<axiWord> 		&outputPathOutData,
		stream<metadata> 		&outputPathInMetadata,
		stream<ap_uint<16> > 	&outputpathInLength,
		stream<axiWord>			&inputPathPortUnreachable);	// Output Path Streams
#endif
