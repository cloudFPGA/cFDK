/*****************************************************************************
 * @file       : udp_mux.hpp
 * @brief      : UDP-Mux Interface.
 **
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
 *               UDP-Mux interface.
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
     ap_uint<32>    addr;   // IPv4 address
};


/********************************************
 * Generic Streaming Interfaces.
 ********************************************/

typedef bool AxisAck;       // Acknowledgment over Axi4-Stream I/F


/********************************************
 * UDP Specific Streaming Interfaces.
 ********************************************/

struct UdpWord {            // UDP Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    UdpWord()      {}
    UdpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

struct UdpMeta {            // UDP Socket Pair Association
    SocketAddr      src;    // Source socket address
    SocketAddr      dst;    // Destination socket address
};

typedef ap_uint<16>     UdpPLen; // UDP Payload Length

typedef ap_uint<16>     UdpPort; // UDP Port Number


void udp_mux (

        //------------------------------------------------------
        //-- DHCP / This / Open-Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>         &siDHCP_This_OpnReq,
        stream<AxisAck>         &soTHIS_Dhcp_OpnAck,

        //------------------------------------------------------
        //-- DHCP / This / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<UdpWord>         &siDHCP_This_Data,
        stream<UdpMeta>         &siDHCP_This_Meta,
        stream<UdpPLen>         &siDHCP_This_Len,
        stream<UdpWord>         &soTHIS_Dhcp_Data,
        stream<UdpMeta>         &soTHIS_Dhcp_Meta,

        //------------------------------------------------------
        //-- UDP  / This / Open-Port Interface
        //------------------------------------------------------
        stream<AxisAck>         &siUDP_This_OpnAck,
        stream<UdpPort>         &soTHIS_Udp_OpnReq,

        //------------------------------------------------------
        //-- UDP / This / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<UdpWord>         &siUDP_This_Data,
        stream<UdpMeta>         &siUDP_This_Meta,
        stream<UdpWord>         &soTHIS_Udp_Data,
        stream<UdpMeta>         &soTHIS_Udp_Meta,
        stream<UdpPLen>         &soTHIS_Udp_Len,

        //------------------------------------------------------
        //-- URIF / This / Open-Port Interface
        //------------------------------------------------------
        stream<UdpPort>         &siURIF_This_OpnReq,
        stream<AxisAck>         &soTHIS_Urif_OpnAck,

        //------------------------------------------------------
        //-- URIF / This / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<UdpWord>         &siURIF_This_Data,
        stream<UdpMeta>         &siURIF_This_Meta,
        stream<UdpPLen>         &siURIF_This_Len,
        stream<UdpWord>         &soTHIS_Urif_Data,
        stream<UdpMeta>         &soTHIS_Urif_Meta
);


/********************************************
 * A generic unsigned AXI4-Stream interface.
 *
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

//OBSOLETE-20180706 struct sockaddr_in {
//OBSOLETE-20180706     ap_uint<16>     port;   /* port in network byte order */
//OBSOLETE-20180706     ap_uint<32>		addr;   /* internet address */
//OBSOLETE-20180706 };

//OBSOLETE-20180706 struct axiWord {
//OBSOLETE-20180706     ap_uint<64>		data;
//OBSOLETE-20180706     ap_uint<8>		keep;
//OBSOLETE-20180706     ap_uint<1>		last;
//OBSOLETE-20180706 };

//OBSOLETE-20180706 struct metadata {
//OBSOLETE-20180706     sockaddr_in sourceSocket;
//OBSOLETE-20180706     sockaddr_in destinationSocket;
//OBSOLETE-20180706 };

//OBSOLETE-20180706 void udp_mux( stream<axiWord>			&rxDataIn,      stream<metadata>&       rxMetadataIn,
//OBSOLETE-20180706               stream<axiWord>           &rxDataOutDhcp, stream<metadata>&       rxMetadataOutDhcp,
//OBSOLETE-20180706               stream<axiWord>           &rxDataOutApp,  stream<metadata>&       rxMetadataOutApp,

//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenOut,        stream<bool >& portOpenReplyIn,
//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenInDhcp,     stream<bool >& portOpenReplyOutDhcp,
//OBSOLETE-20180706               stream<ap_uint<16> >&  requestPortOpenInApp,  stream<bool >& portOpenReplyOutApp,

//OBSOLETE-20180706               stream<axiWord>       &txDataInDhcp,  stream<metadata>    &txMetadataInDhcp,  stream<ap_uint<16> >    &txLengthInDhcp,
//OBSOLETE-20180706               stream<axiWord>       &txDataInApp,   stream<metadata>    &txMetadataInApp,   stream<ap_uint<16> >    &txLengthInApp,
//OBSOLETE-20180706               stream<axiWord>       &txDataOut,     stream<metadata>    &txMetadataOut,     stream<ap_uint<16> >    &txLengthOut
//OBSOLETE-20180706 );
