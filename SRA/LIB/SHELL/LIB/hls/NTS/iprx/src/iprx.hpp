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


/*******************************************************************************
 * @file       : iprx.hpp
 * @brief      : IP Receiver packet handler (IPRX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_IPRX
 * \{
 *******************************************************************************/

#ifndef _IPRX_H_
#define _IPRX_H_

#include <ap_shift_reg.h>

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../../NTS/AxisArp.hpp"
#include "../../../NTS/AxisEth.hpp"
#include "../../../NTS/AxisIp4.hpp"

const UdpLen  MaxDatagramSize = 32768; // Maximum size of an IP datagram in bytes [FIXME - Why not 65535?]

/***********************************************
 * SUB-CHECKSUMS COMPUTATION
 ***********************************************/
class SubSums {
  public:
    ap_uint<17>         sum0;
    ap_uint<17>         sum1;
    ap_uint<17>         sum2;
    ap_uint<17>         sum3;
    bool                ipMatch;
    SubSums() {}
    SubSums(ap_uint<17> sums[4], bool match) :
        sum0(sums[0]), sum1(sums[1]), sum2(sums[2]), sum3(sums[3]), ipMatch(match) {}
    SubSums(ap_uint<17> s0, ap_uint<17> s1, ap_uint<17> s2, ap_uint<17> s3, bool match) :
        sum0(s0), sum1(s1), sum2(s2), sum3(s3), ipMatch(match) {}
};

/*******************************************************************************
 *
 * ENTITY - IP RX HANDLER (IPRX)
 *
 *******************************************************************************/
#if HLS_VERSION == 2017

    void iprx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_Ip4Address,

        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxisEth>     &siETH_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxisArp>     &soARP_Data,

        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &soICMP_Data,
        stream<AxisIp4>     &soICMP_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soUOE_Data,

        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soTOE_Data
    );

#else

    void iprx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_Ip4Address,

        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxisRaw>     &siETH_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soARP_Data,

        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &soICMP_Data,
        stream<AxisRaw>     &soICMP_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soUOE_Data,

        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soTOE_Data
    );

#endif  // HLS_VERSION

#endif

/*! \} */
