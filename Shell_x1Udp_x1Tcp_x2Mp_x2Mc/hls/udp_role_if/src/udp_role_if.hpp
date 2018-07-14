#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;

const uint16_t 		REQUEST 		= 0x0100;
const uint16_t 		REPLY 			= 0x0200;
const ap_uint<32>	replyTimeOut 	= 65536;

const ap_uint<48> MY_MAC_ADDR 	= 0xE59D02350A00; 	// LSB first, 00:0A:35:02:9D:E5
const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

const uint8_t 	noOfArpTableEntries	= 8;

/*
 * A generic unsigned AXI4-Stream interface.
 */
 template<int D>
   struct Axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
   };

//OBSOLETE-20180706 struct AxiWord {
//OBSOLETE-20180706 	ap_uint<64>		data;
//OBSOLETE-20180706 	ap_uint<8>		keep;
//OBSOLETE-20180706 	ap_uint<1>		last;
//OBSOLETE-20180706 };

struct SocketAddr {
    ap_uint<16>     port;   // Port in network byte order
    ap_uint<32>		addr;   // IPv4 address
};

struct Metadata {
	SocketAddr		src;	// Source socket address
	SocketAddr		dst;	// Destination socket address
};


void udp_role_if (

		//------------------------------------------------------
		//-- ROLE / This / Udp Interfaces
		//------------------------------------------------------
		stream<Axis<64> >	&siROL_This_Data,
		stream<Axis<64> >   &soTHIS_Rol_Data,

		//------------------------------------------------------
		//-- UDMX / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<Axis<1> >   	&siUDMX_This_OpnAck,
		stream<Axis<16> >	&soTHIS_Udmx_OpnReq,

		//------------------------------------------------------
	    //-- UDMX / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<Axis<64> >	&siUDMX_This_Data,
		stream<Metadata>	&siUDMX_This_Meta,
		stream<Axis<64> >	&soTHIS_Udmx_Data,
		stream<Metadata>	&soTHIS_Udmx_Meta,
		stream<Axis<16> >	&soTHIS_Udmx_Len
);
