/*****************************************************************************
 * @file       : dhcp_client.hpp
 * @brief      : Dynamic Host Configuration Protocol (DHCP) client.
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
 * 				 DHCP-client.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include "ap_int.h"		//OBSOLETE-20180828 #include "tcp_ip.hpp"

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




//OBSOLETE-20180828 struct sockaddr_in {
//OBSOLETE-20180828     ap_uint<16>     port;   /* port in network byte order */
//OBSOLETE-20180828     ap_uint<32>		addr;   /* internet address */
//OBSOLETE-20180828     sockaddr_in () {}
//OBSOLETE-20180828     sockaddr_in (ap_uint<32> addr, ap_uint<16> port)
//OBSOLETE-20180828     	:addr(addr), port(port) {}
//OBSOLETE-20180828 };

//OBSOLETE-20180828 struct udpMetadata {
//OBSOLETE-20180828 	sockaddr_in sourceSocket;
//OBSOLETE-20180828 	sockaddr_in destinationSocket;
//OBSOLETE-20180828 	udpMetadata() {}
//OBSOLETE-20180828 	udpMetadata(sockaddr_in src, sockaddr_in dst)
//OBSOLETE-20180828 		:sourceSocket(src), destinationSocket(dst) {}
//OBSOLETE-20180828 };
