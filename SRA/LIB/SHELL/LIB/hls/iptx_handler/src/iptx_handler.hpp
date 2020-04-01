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
 * @file       : iptx_handler.hpp
 * @brief      : IP transmit frame handler (IPTX).
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

#ifndef IPTX_H_
#define IPTX_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

#include "../../toe/src/toe.hpp"
#include "../../toe/src/toe_utils.hpp"
#include "../../AxisEth.hpp"
#include "../../AxisIp4.hpp"

using namespace hls;

class ArpLkpReply
{
  public:
    EthAddr     macAddress;
    HitState    hit;
    ArpLkpReply() {}
    ArpLkpReply(EthAddr macAdd, HitState hit) :
        macAddress(macAdd), hit(hit) {}
};


void iptx_handler(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,

        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siL3MUX_Data,

        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxisEth>         &soL2MUX_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep
);

#endif
