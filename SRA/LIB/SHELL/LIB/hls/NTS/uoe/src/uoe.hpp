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
 * @file       : uoe.hpp
 * @brief      : UDP Offload Engine (UOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_UOE
 * \{
 *****************************************************************************/

#ifndef _UOE_H_
#define _UOE_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../AxisApp.hpp"
#include "../../AxisIp4.hpp"
#include "../../AxisPsd4.hpp"

using namespace hls;

/*******************************************************************************
 * CONSTANTS DERIVED FROM THE NTS CONFIGURATION FILE
 *******************************************************************************/

//-- The Maximum Datagram Size (MDS) that can be received or sent by UOE
//--  FYI: MDS is rounded modulo 8 bytes to match the chunk size.
static const Ly4Len UDP_MDS = (MTU_ZYC2-IP4_HEADER_LEN-UDP_HEADER_LEN) & ~0x7;  // 1416


/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY TOE
 *******************************************************************************/

//---------------------------------------------------------
//-- UOE - IPv4 ADDRESS PAIR
//---------------------------------------------------------
class IpAddrPair {
  public:
    Ip4Addr     ipSa;
    Ip4Addr     ipDa;
    IpAddrPair() {}
    IpAddrPair(Ip4Addr ipSa, Ip4Addr ipDa) :
        ipSa(ipSa), ipDa(ipDa) {}
};


/******************************************************************************
 *
 * ENTITY - UDP OFFLOAD ENGINE (UOE)
 *
 ******************************************************************************/
void uoe(

        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                   piMMIO_En,
        stream<StsBool>         &soMMIO_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>         &soIPTX_Data,

        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>    &siUAIF_LsnReq,
        stream<UdpAppLsnRep>    &soUAIF_LsnRep,
        stream<UdpAppClsReq>    &siUAIF_ClsReq,
        stream<UdpAppClsRep>    &soUAIF_ClsRep,

        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soUAIF_Data,
        stream<UdpAppMeta>      &soUAIF_Meta,

        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisIcmp>         &soICMP_Data
);

#endif

/*! \} */
