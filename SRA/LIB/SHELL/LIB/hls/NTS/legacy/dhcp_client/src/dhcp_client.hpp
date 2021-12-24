
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
 * @file       : dhcp_client.hpp
 * @brief      : Dynamic Host Configuration Protocol (DHCP) client.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *****************************************************************************/

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


/********************************************
 * DHCP Meta-Request and Meta-Reply Formats.
 ********************************************/
struct DhcpMetaRep {	// DHCP meta-reply information
	ap_uint<32>		identifier;
	ap_uint<32>		assignedIpAddress;
	ap_uint<32>		serverAddress;
	ap_uint<8>		type;
};

struct DhcpMetaReq {	// DHCP meta-request information
	ap_uint<32>		identifier;
	ap_uint<8>		type;
	ap_uint<32>		requestedIpAddress;

	DhcpMetaReq() {}
	DhcpMetaReq(ap_uint<32> i, ap_uint<8> type) :
					identifier(i), type(type), requestedIpAddress(0) {}
	DhcpMetaReq(ap_uint<32> i, ap_uint<8> type, ap_uint<32> ip) :
					identifier(i), type(type), requestedIpAddress(ip) {}
};


/********************************************
 * DHCP Specific Message Formats.
 ********************************************/

enum DhcpOperationCode {	// Specifies the general type of message.
	DHCPDISCOVER	= 0x01, //  An odd  value indicates a request message.
	DHCPOFFER	 	= 0x02, //  An even value indicates a reply message.
	DHCPREQUEST		= 0x03,
	DHCPACK			= 0x05
};


/********************************************
 * DHCP Specific Constants and Definitions.
 ********************************************/
static const ap_uint<32> MAGIC_COOKIE = 0x63538263;

#ifndef __SYNTHESIS__
	static const ap_uint<32> TIME_US  = 200;
	static const ap_uint<32> TIME_5S  = 100;
	static const ap_uint<32> TIME_30S = 300;
#else
	static const ap_uint<32> TIME_US  = 20000;
	static const ap_uint<32> TIME_5S  = 750750750;
	static const ap_uint<32> TIME_30S = 0xFFFFFFFF;
#endif


/********************************************
 * Socket Transport Address Format.
 ********************************************/
struct SocketAddr {
     ap_uint<16>	port;   // Port in network byte order
     ap_uint<32>	addr;   // IPv4 address
     SocketAddr()   {}
     SocketAddr(ap_uint<32> addr, ap_uint<16> port) :
    	 	 	 	addr(addr), port(port) {}
};


/********************************************
 * UDP Specific Streaming Interfaces.
 ********************************************/
struct UdpWord {			// UDP Streaming Chunk (i.e. 8 bytes)
	ap_uint<64>    tdata;
	ap_uint<8>	   tkeep;
	ap_uint<1>     tlast;
	UdpWord()      {}
	UdpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

struct UdpMeta {			// UDP Socket Pair Association
 	SocketAddr		src;	// Source socket address
 	SocketAddr		dst;	// Destination socket address
 	UdpMeta()		{}
 	UdpMeta(SocketAddr src, SocketAddr dst) :
 					src(src), dst(dst) {}
};

typedef ap_uint<16>	UdpPLen; // UDP Payload Length

typedef ap_uint<16>	UdpPort; // UDP Port Number



/********************************************
 * Generic Streaming Interfaces.
 ********************************************/
typedef bool AxisAck;		// Acknowledgment over Axi4-Stream I/F


/********************************************
 * Other Constants and Definitions.
 ********************************************/
typedef ap_uint<1>	SigOpn;	// Signal that the socket is open.

static const SigOpn SIG_OPEN = 1;




void dhcp_client(

		//------------------------------------------------------
		//-- MMIO / This / Config Interfaces
		//------------------------------------------------------
		ap_uint<1>          &piMMIO_This_Enable,
		ap_uint<48>         &piMMIO_This_MacAddress,

		//------------------------------------------------------
		//-- THIS / Nts  / IPv4 Interfaces
		//------------------------------------------------------
		ap_uint<32>         &poTHIS_Nts_IpAddress,

		//------------------------------------------------------
		//-- UDMX / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<AxisAck>		&siUDMX_This_OpnAck,
		stream<UdpPort>		&soTHIS_Udmx_OpnReq,

		//------------------------------------------------------
		//-- UDMX / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<UdpWord>     &siUDMX_This_Data,
		stream<UdpMeta>     &siUDMX_This_Meta,
		stream<UdpWord>     &soTHIS_Udmx_Data,
		stream<UdpMeta>    	&soTHIS_Udmx_Meta,
		stream<UdpPLen>		&soTHIS_Udmx_PLen
);
