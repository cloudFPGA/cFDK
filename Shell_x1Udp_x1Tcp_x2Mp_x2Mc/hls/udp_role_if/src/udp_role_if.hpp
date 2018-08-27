/*****************************************************************************
 * @file       : udp_role_if.hpp
 * @brief      : UDP Role Interface.
 *  *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 * 				 UDP-Role interface.
 *
 *****************************************************************************/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;


/********************************************
 * Socket transport address.
 ********************************************/

struct SocketAddr {
     ap_uint<16>    port;   // Port in network byte order
     ap_uint<32>	addr;   // IPv4 address
};


/********************************************
 * Generic Streaming Interfaces.
 ********************************************/

typedef bool AxisAck;		// Acknowledgment over Axi4-Stream I/F


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
};

typedef ap_uint<16>	UdpPLen; // UDP Payload Length

typedef ap_uint<16>	UdpPort; // UDP Port Number


void udp_role_if (

		//------------------------------------------------------
		//-- ROLE / This / Udp Interfaces
		//------------------------------------------------------
		stream<UdpWord>		&siROL_This_Data,
		stream<UdpWord>   	&soTHIS_Rol_Data,

		//------------------------------------------------------
		//-- UDMX / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<AxisAck>   	&siUDMX_This_OpnAck,
		stream<UdpPort>		&soTHIS_Udmx_OpnReq,

		//------------------------------------------------------
	    //-- UDMX / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<UdpWord>		&siUDMX_This_Data,
		stream<UdpMeta>		&siUDMX_This_Meta,
		stream<UdpWord>		&soTHIS_Udmx_Data,
		stream<UdpMeta>		&soTHIS_Udmx_Meta,
		stream<UdpPLen>		&soTHIS_Udmx_Len
);



/********************************************
 * A generic unsigned AXI4-Stream interface.

template<int D>
struct Axis {
	ap_uint<D>       tdata;
	ap_uint<(D+7)/8> tkeep;
	ap_uint<1>       tlast;
	Axis() {}
	Axis(ap_uint<D> t_data) : tdata((ap_uint<D>)t_data) {
		int val = 0;
		for (int bit=0; bit<(D+7)/8; bit++)
			val |= (1 << bit);
		tkeep = val;
		tlast = 1;
	}
};
********************************************/

//OBSOLETE-20180706 const uint16_t 		REQUEST 		= 0x0100;
//OBSOLETE-20180706 const uint16_t 		REPLY 			= 0x0200;
//OBSOLETE-20180706 const ap_uint<32>	replyTimeOut 	= 65536;

//OBSOLETE-20180706 const ap_uint<48> MY_MAC_ADDR 	= 0xE59D02350A00; 	// LSB first, 00:0A:35:02:9D:E5
//OBSOLETE-20180706 const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

//OBSOLETE-20180706 const uint8_t 	noOfArpTableEntries	= 8;

//OBSOLETE-20180706 struct AxiWord {
//OBSOLETE-20180706 	ap_uint<64>		data;
//OBSOLETE-20180706 	ap_uint<8>		keep;
//OBSOLETE-20180706 	ap_uint<1>		last;
//OBSOLETE-20180706 };

//OBSOLETE-20180816 struct SocketAddr {
//OBSOLETE-20180816     ap_uint<16>     port;   // Port in network byte order
//OBSOLETE-20180816     ap_uint<32>		addr;   // IPv4 address
//OBSOLETE-20180816 };

//OBSOLETE-20180816 struct Metadata {
//OBSOLETE-20180816 	SocketAddr		src;	// Source socket address
//OBSOLETE-20180816 	SocketAddr		dst;	// Destination socket address
//OBSOLETE-20180816 };


