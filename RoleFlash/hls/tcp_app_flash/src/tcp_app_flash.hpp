/*****************************************************************************
 * @file       : tcp_app_flash.hpp
 * @brief      : TCP Application Flash (UAF).
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
 * @details    : Data structures, types and prototypes definitions for the TCP
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
 * TCP Specific Streaming Interfaces
 ********************************************/

struct TcpWord {            // TCP Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    TcpWord()      {}
    TcpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};


void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / This / Mmio / Config Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_This_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_This_MmioPostPktEn,
        //[TODO] ap_uint<1> piSHL_This_MmioCaptPktEn,

        //------------------------------------------------------
        //-- SHELL / This / Tcp Interfaces
        //------------------------------------------------------
        stream<TcpWord>     &siSHL_This_Data,
        stream<TcpWord>     &soTHIS_Shl_Data
);

