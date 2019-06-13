/*****************************************************************************
 * @file       : tcp_app_flash.hpp
 * @brief      : TCP Application Flash (TAF).
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
 *                application embedded in the ROLE of the cloudFPGA flash.
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
    ECHO_PATH_THRU = 0,
    ECHO_STORE_FWD = 1,
    ECHO_OFF       = 2
};


/************************************************
 * AXI4 STREAMING INTERFACES (alias AXIS)
 ************************************************/
class AxiWord {    // AXI4-Streaming Chunk (i.e. 8 bytes)
public:
    ap_uint<64>     tdata;
    ap_uint<8>      tkeep;
    ap_uint<1>      tlast;
public:
    AxiWord()       {}
    AxiWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
            tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

/********************************************
 * TCP Session Id
 ********************************************/
typedef ap_uint<16> TcpSessId;

/***********************************************
 * Application Data
 *  Data transfered between TOE and APP.
 ***********************************************/
typedef AxiWord     AppData;

/***********************************************
 * Application Metadata
 *  Meta-data transfered between TOE and APP.
 ***********************************************/
typedef TcpSessId   AppMeta;


/*************************************************************************
 *
 * ENTITY - TCP APPLICATION FLASH (TAF)
 *
 *************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siSHL_Data,
        stream<AppMeta>     &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soSHL_Data,
        stream<AppMeta>     &soSHL_SessId
);

