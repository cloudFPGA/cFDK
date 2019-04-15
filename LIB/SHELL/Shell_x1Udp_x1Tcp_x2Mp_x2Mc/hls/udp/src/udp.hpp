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
