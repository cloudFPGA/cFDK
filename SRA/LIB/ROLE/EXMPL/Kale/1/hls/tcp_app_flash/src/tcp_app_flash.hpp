/************************************************
Copyright (c) 2016-2019, IBM Research.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : tcp_app_flash.hpp
 * @brief      : TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the TCP
 *                application embedded in the ROLE of the cloudFPGA flash.
 *
 *****************************************************************************/

#include <stdio.h>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/toe/src/toe.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/toe/test/test_toe_utils.hpp"

using namespace hls;


/********************************************
 * SHELL/MMIO/EchoCtrl - Config Register
 ********************************************/
enum EchoCtrl {
    ECHO_PATH_THRU = 0,
    ECHO_STORE_FWD = 1,
    ECHO_OFF       = 2
};


/*************************************************************************
 *
 * ENTITY - TCP APPLICATION FLASH (TAF)
 *
 *************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          *piSHL_MmioEchoCtrl,
        CmdBit              *piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> *piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- TRIF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId           *piTRIF_SConnectId,

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

