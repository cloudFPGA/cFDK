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
 * @file       : udp.hpp
 * @brief      : Defines and prototypes related to the UDP offload engine.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
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

#include "../../toe/src/toe.hpp"
#include "../../toe/src/toe_utils.hpp"
#include "../../AxisIp4.hpp"

using namespace hls;

const Ip4Prot   UDP_PROTOCOL = 17; // IP protocol number for UDP

//OBSOLETE-20180619 #define myIP 0x0101010A

//OBSOLETE_20200306 #define MTU	1500  // Maximum Transmission Unit in bytes

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

/*** OBSOLETE_202020306 ***************
struct ipTuple {
	ap_uint<32>		sourceIP;
	ap_uint<32>		destinationIP;
	ipTuple() 		{}
	ipTuple(ap_uint<32> sourceIP, ap_uint<32> destinationIP)
					: sourceIP(sourceIP), destinationIP(destinationIP) {}
};
***************************************/

/***********************************************
 * IPv4 ADDRESS PAIR
 ***********************************************/
class IpAddrPair {
  public:
    Ip4Addr     ipSa;
    Ip4Addr     ipDa;
    IpAddrPair() {}
    IpAddrPair(Ip4Addr ipSa, Ip4Addr ipDa) :
        ipSa(ipSa), ipDa(ipDa) {}
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


/***********************************************
 * UDP Application Data
 *  Data transfered between UOE and APP.
 ***********************************************/
typedef AxiWord      AppData;


/*************************************************************************
 *
 * ENTITY - UDP OFFLOAD ENGINE (UOE)
 *
 *************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<axiWord>         &soIPTX_Data,

        //------------------------------------------------------
        //-- URIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>         &siURIF_OpnReq,
        stream<StsBool>         &soURIF_OpnRep,
        stream<UdpPort>         &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppData>         &soURIF_Data,
        stream<metadata>        &soURIF_Meta, // [TODO - Rename UdpMeta]

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>         &siURIF_Data,
        stream<metadata>        &siURIF_Meta,
        stream<ap_uint<16> >    &siURIF_PLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxiWord>         &soICMP_Data
);

#endif
