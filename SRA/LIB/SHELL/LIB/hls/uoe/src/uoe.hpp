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

using namespace hls;

//OBSOLETE-20180619 #define myIP 0x0101010A

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

/*************************************************************************
 *
 * ENTITY - UDP OFFLOAD ENGINE (UOE)
 *
 *************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<axiWord>         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<axiWord>         &soIPTX_Data,

        //------------------------------------------------------
        //-- URIF / Control Port Interfaces
        //------------------------------------------------------
        stream<ap_uint<16> >    &siURIF_OpnReq,
        stream<bool>            &soURIF_OpnRep,
        stream<ap_uint<16> >    &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>         &soURIF_Data,
        stream<metadata>        &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<axiWord>         &siURIF_Data,
        stream<metadata>        &siURIF_Meta,
        stream<ap_uint<16> >    &siURIF_PLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<axiWord>         &soICMP_Data
);

#endif
