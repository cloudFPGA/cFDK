/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

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
 * @file       : icmp_server.hpp
 * @brief      :  Internet Control Message Protocol server (ICMP).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   IP-Tx handler.
 *
 *****************************************************************************/

#ifndef ICMP_H_
#define ICMP_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <cstdlib>

#include "../../toe/src/toe.hpp"
#include "../../toe/src/toe_utils.hpp"
#include "../../AxisEth.hpp"

using namespace hls;

const IcmpType  ICMP_ECHO_REPLY              = 0x00; // Echo reply (used to ping)
const IcmpType  ICMP_DESTINATION_UNREACHABLE = 0x03; //
const IcmpType  ICMP_ECHO_REQUEST            = 0x08; // Echo request (used to ping)
const IcmpType  ICMP_TIME_EXCEEDED           = 0x0B;

const IcmpCode  ICMP_TTL_EXPIRED_IN_TRANSIT       = 0x00;
const IcmpCode  ICMP_DESTINATION_PORT_UNREACHABLE = 0x03;

const Ip4Prot   ICMP_PROTOCOL = 0x01; // IP protocol number for ICMP

typedef ap_uint<17> Sum17;    // 16-bit 1's complement sum with carry
typedef ap_uint<17> LE_Sum17; // 16-bit 1's complement sum with carry


void icmp_server(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        Ip4Addr             piMMIO_IpAddress,

        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisIp4>    &siIPRX_Data,
        stream<AxisIp4>    &siIPRX_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxiWord>    &siUDP_Data,  // [TODO-AxisUdp]

        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxisIp4>    &soIPTX_Data

);

#endif
