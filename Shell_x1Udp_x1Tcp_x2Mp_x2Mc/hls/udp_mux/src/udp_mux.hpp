/*****************************************************************************
 * @file       : udp_mux.hpp
 * @brief      : UDP-Mux Interface.
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
 * 				 UDP-Mux interface.
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

/*
 * A generic unsigned AXI4-Stream interface.
 */
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

//OBSOLETE-20180706 struct axiWord {
//OBSOLETE-20180706 	ap_uint<64>		data;
//OBSOLETE-20180706 	ap_uint<8>		keep;
//OBSOLETE-20180706 	ap_uint<1>		last;
//OBSOLETE-20180706 };

//OBSOLETE-20180706 struct sockaddr_in {
//OBSOLETE-20180706     ap_uint<16>     port;   /* port in network byte order */
//OBSOLETE-20180706     ap_uint<32>		addr;   /* internet address */
//OBSOLETE-20180706 };

struct SocketAddr {
     ap_uint<16>    port;   // Port in network byte order
     ap_uint<32>	addr;   // IPv4 address
};

//OBSOLETE-20180706 struct metadata {
//OBSOLETE-20180706 	sockaddr_in sourceSocket;
//OBSOLETE-20180706 	sockaddr_in destinationSocket;
//OBSOLETE-20180706 };

struct Metadata {
 	SocketAddr		src;	// Source socket address
 	SocketAddr		dst;	// Destination socket address
 };

//OBSOLETE-20180706 void udp_mux( stream<axiWord>			&rxDataIn, 		stream<metadata>&     	rxMetadataIn,
//OBSOLETE-20180706               stream<axiWord> 			&rxDataOutDhcp, stream<metadata>&     	rxMetadataOutDhcp,
//OBSOLETE-20180706               stream<axiWord> 			&rxDataOutApp, 	stream<metadata>&     	rxMetadataOutApp,
			   
//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenOut, 		stream<bool >& portOpenReplyIn,
//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenInDhcp, 	stream<bool >& portOpenReplyOutDhcp,
//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenInApp, 	stream<bool >& portOpenReplyOutApp,
			   
//OBSOLETE-20180706               stream<axiWord> 		&txDataInDhcp, 	stream<metadata> 	&txMetadataInDhcp, 	stream<ap_uint<16> > 	&txLengthInDhcp,
//OBSOLETE-20180706               stream<axiWord> 		&txDataInApp, 	stream<metadata> 	&txMetadataInApp, 	stream<ap_uint<16> > 	&txLengthInApp,
//OBSOLETE-20180706               stream<axiWord> 		&txDataOut, 	stream<metadata> 	&txMetadataOut, 	stream<ap_uint<16> > 	&txLengthOut
//OBSOLETE-20180706 );



void udp_mux (

		//------------------------------------------------------
		//-- DHCP / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<Axis<16> >		&siDHCP_This_OpnReq,
		stream<Axis<1> >		&soTHIS_Dhcp_OpnAck,

	    //------------------------------------------------------
	    //-- DHCP / This / Data & MetaData Interfaces
	    //------------------------------------------------------
		stream<Axis<64> >		&siDHCP_This_Data,
		stream<Metadata>        &siDHCP_This_Meta,
		stream<Axis<16> >    	&siDHCP_This_Len,
		stream<Axis<64> >		&soTHIS_Dhcp_Data,
		stream<Metadata>		&soTHIS_Dhcp_Meta,

		//------------------------------------------------------
	    //-- UDP  / This / Open-Port Interface
		//------------------------------------------------------
		stream<Axis<1> > 		&siUDP_This_OpnAck,
		stream<Axis<16> >		&soTHIS_Udp_OpnReq,

		//------------------------------------------------------
		//-- UDP / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<Axis<64> >       &siUDP_This_Data,
		stream<Metadata>		&siUDP_This_Meta,
		stream<Axis<64> >       &soTHIS_Udp_Data,
		stream<Metadata>        &soTHIS_Udp_Meta,
		stream<Axis<16> >    	&soTHIS_Udp_Len,

		//------------------------------------------------------
		//-- URIF / This / Open-Port Interface
		//------------------------------------------------------
		stream<Axis<16> >		&siURIF_This_OpnReq,
		stream<Axis<1> >		&soTHIS_Urif_OpnAck,

		//------------------------------------------------------
		//-- URIF / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<Axis<64> >       &siURIF_This_Data,
		stream<Metadata>        &siURIF_This_Meta,
		stream<Axis<16> >    	&siURIF_This_Len,
		stream<Axis<64> >       &soTHIS_Urif_Data,
		stream<Metadata>		&soTHIS_Urif_Meta
);

