/*****************************************************************************
 * @file       : udp_app_flash.hpp
 * @brief      : UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the UDP
 *                application embedded in the flash of the cloudFPGA role.
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
 * SHELL/MMIO/EchoCtrl - Config Register
 ********************************************/
enum EchoCtrl {
	ECHO_PATH_THRU	= 0,
	ECHO_STORE_FWD	= 1,
	ECHO_OFF		= 2
};

/********************************************
 * UDP Specific Streaming Interfaces
 ********************************************/

struct UdpWord {            // UDP Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    UdpWord()      {}
    UdpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};


void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / This / Mmio / Config Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_This_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_This_MmioPostPktEn,
        //[TODO] ap_uint<1> piSHL_This_MmioCaptPktEn,

        //------------------------------------------------------
        //-- SHELL / This / Udp Interfaces
        //------------------------------------------------------
        stream<UdpWord>     &siSHL_This_Data,
        stream<UdpWord>     &soTHIS_Shl_Data
);

