/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/************************************************
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
#include "../../AxisIp4.hpp"
#include "../../AxisPsd4.hpp"


/*******************************************************************************
 * CONSTANTS DERIVED FROM THE NTS CONFIGURATION FILE
 *******************************************************************************/

//-- The Maximum Datagram Size (MDS) that can be received or sent by UOE
//--  FYI: MDS is rounded modulo 8 bytes to match the chunk size.
static const Ly4Len UDP_MDS = (MTU_ZYC2-IP4_HEADER_LEN-UDP_HEADER_LEN) & ~0x7;  // 1416

//-------------------------------------------------------------------
//-- DEFINES FOR THE UOE INTERNAL STREAMS (can be changed)
//-------------------------------------------------------------------
#define UOE_ELASTIC_DATA_BUFFER  16*1024  // In Bytes
#define UOE_ELASTIC_HEADER_BUFF       64  // In Headers

//-------------------------------------------------------------------
//-- DERIVED CONSTANTS FOR THE UOE INTERNAl STREAMS (don't touch)
//-------------------------------------------------------------------
const int cUdpRxDataFifoSize = (UOE_ELASTIC_DATA_BUFFER)/(ARW/8); // Size of UDP Rx data buffer (in chunks)
const int cUdpRxHdrsFifoSize = (UOE_ELASTIC_HEADER_BUFF); // Size of the UDP Rx header buffer (in UDP headers)
const int cIp4RxHdrsFifoSize = (cUdpRxHdrsFifoSize * 4);  // Size of the IP4 Rx header buffer (1-header=4-entries in the FiFo)
const int cMtuSize           = (2*MTU)/(ARW/8);           // Minimum size to store one MTU

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
#if HLS_VERSION == 2017

    void uoe_top(

        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                   piMMIO_En,
        stream<ap_uint<16> >    &soMMIO_DropCnt,
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
        stream<UdpAppDLen>      &soUAIF_DLen,

        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisIcmp>        &soICMP_Data
    );
#else

    void uoe_top(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        CmdBit                   piMMIO_En,
        stream<ap_uint<16> >    &soMMIO_DropCnt,
        stream<StsBool>         &soMMIO_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisRaw>         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisRaw>         &soIPTX_Data,

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
        stream<UdpAppDLen>      &soUAIF_DLen,

        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen,

        //------------------------------------------------------
        //-- ICMP / Message Data Interface (Port Unreachable)
        //------------------------------------------------------
        stream<AxisRaw>         &soICMP_Data
    );

#endif  // HLS_VERSION

#endif

/*! \} */
