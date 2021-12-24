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
 * @file       : iptx.hpp
 * @brief      : IP Transmitter packet handler (IPTX)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_IPTX
 * \{
 *******************************************************************************/

#ifndef _IPTX_H_
#define _IPTX_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../AxisEth.hpp"
#include "../../AxisIp4.hpp"


/*******************************************************************************
 *
 * ENTITY - IP TX HANDLER (IPTX)
 *
 *******************************************************************************/
#if HLS_VERSION == 2017

    void iptx_top(
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

#else

    void iptx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,

        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxisRaw>         &siL3MUX_Data,

        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxisRaw>         &soL2MUX_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep
    );

#endif  // HLS_VERSION

#endif

/*! \} */
