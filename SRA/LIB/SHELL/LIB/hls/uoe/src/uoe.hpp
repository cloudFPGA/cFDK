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
 * @file       : udp.hpp
 * @brief      : Defines and prototypes related to the UDP offload engine.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef _UDP_H
#define _UDP_H

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
#include "../../AxisApp.hpp"
#include "../../AxisIp4.hpp"

using namespace hls;

const Ip4Prot   UDP_PROTOCOL = 17; // IP protocol number for UDP

/***********************************************
 * IPv4 ADDRESS PAIR
 ***********************************************/
class IpAddrPair {
  public:
    Ip4Addr     ipSa;
    Ip4Addr     ipDa;
    IpAddrPair() {}
    IpAddrPair(Ip4Addr ipSa, Ip4Addr ipDa) :
        ipSa(ipSa), ipDa(ipDa) {}
};

/***********************************************
 * Definition of the UDP Role Interfaces (URIF)
 ***********************************************/
typedef AxiWord      UdpAppData;
typedef SocketPair   UdpAppMeta;
typedef UdpLen       UdpAppDLen;

/*************************************************************************
 *
 * ENTITY - UDP OFFLOAD ENGINE (UOE)
 *
 *************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>         &soIPTX_Data,

        //------------------------------------------------------
        //-- URIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>         &siURIF_OpnReq,
        stream<StsBool>         &soURIF_OpnRep,
        stream<UdpPort>         &siURIF_ClsReq,

        //------------------------------------------------------
        //-- URIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>         &soURIF_Data,
        stream<UdpAppMeta>      &soURIF_Meta,

        //------------------------------------------------------
        //-- URIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AxisApp>         &siURIF_Data,
        stream<UdpAppMeta>      &siURIF_Meta,
        stream<UdpAppDLen>      &siURIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxiWord>         &soICMP_Data
);

#endif
