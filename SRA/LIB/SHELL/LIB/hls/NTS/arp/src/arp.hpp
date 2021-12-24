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
 * @file       : arp.hpp
 * @brief      : Address Resolution Protocol (ARP) Server
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_ARP
 * \{
 *******************************************************************************/

#ifndef _ARP_H_
#define _ARP_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../AxisEth.hpp"

/************************************************
 * ARP Metadata
 *  Structure extracted from incoming ARP packet.
 *  Solely used inside [ARP].
 ************************************************/
class ArpMeta  // ARP Metadata
{
  public:
    EthAddr         remoteMacAddr;
    EtherType       etherType;
    ArpHwType       arpHwType;
    ArpProtType     arpProtType;
    ArpHwLen        arpHwLen;
    ArpProtLen      arpProtLen;
    ArpSendHwAddr   arpSendHwAddr;
    ArpSendProtAddr arpSendProtAddr;
    ArpMeta() {}
};

/********************************************
 * CAM / Lookup OpCodes
 ********************************************/
typedef ap_uint<1> ArpLkpOp;
enum               ArpLkpOpCodes   { ARP_INSERT=0, ARP_DELETE=1 };

/********************************************
 * CAM / MAC Update Request
 ********************************************/
class RtlMacUpdateRequest {
  public:
    EthAddr         value;  // 48-bits
    Ip4Addr         key;    // 32-bits
    ArpLkpOp        opcode; //  1-bit : '0' is INSERT, '1' is DELETE

    RtlMacUpdateRequest() {}
    RtlMacUpdateRequest(Ip4Addr key, EthAddr value, ArpLkpOp opcode) :
            key(key), value(value), opcode(opcode) {}
};

/********************************************
 * CAM / MAC Update Reply
 ********************************************/
class RtlMacUpdateReply {
  public:
    EthAddr         value;  // 48-bits
    ArpLkpOp        opcode; //  1-bit : '0' is INSERT, '1' is DELETE

    RtlMacUpdateReply() {}
    RtlMacUpdateReply(ArpLkpOp opcode) :
        opcode(opcode) {}
    RtlMacUpdateReply(EthAddr value, ArpLkpOp opcode) :
        value(value), opcode(opcode) {}
};

/********************************************
 * CAM / MAC LookupR Request
 ********************************************/
class RtlMacLookupRequest {
  public:
    Ip4Addr         key;
    RtlMacLookupRequest() {}
    RtlMacLookupRequest(Ip4Addr searchKey) :
        key(searchKey) {}
};

/********************************************
 * CAM / MAC Lookup Reply
 ********************************************/
class RtlMacLookupReply {
  public:
    EthAddr         value;  // 48 bits
    HitBool         hit;    //  8 bits
    RtlMacLookupReply() {}
    RtlMacLookupReply(HitBool hit, EthAddr value) :
        hit(hit), value(value) {}
};

/*******************************************************************************
 *
 * ENTITY - ADDRESS RESOLUTION PROTOCOL (ARP) SERVER
 *
 *******************************************************************************/
#if HLS_VERSION == 2017

    void arp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interface
        //------------------------------------------------------
        stream<AxisEth>             &siIPRX_Data,
        //------------------------------------------------------
        //-- ETH Interface
        //------------------------------------------------------
        stream<AxisEth>             &soETH_Data,
        //------------------------------------------------------
        //-- IPTX Interfaces
        //------------------------------------------------------
        stream<Ip4Addr>             &siIPTX_MacLkpReq,
        stream<ArpLkpReply>         &soIPTX_MacLkpRep,
        //------------------------------------------------------
        //-- CAM Interfaces
        //------------------------------------------------------
        stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
        stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
        stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
        stream<RtlMacUpdateReply>   &siCAM_MacUpdRep
    );

#else
    void arp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interface
        //------------------------------------------------------
        stream<AxisRaw>             &siIPRX_Data,
        //------------------------------------------------------
        //-- ETH Interface
        //------------------------------------------------------
        stream<AxisRaw>             &soETH_Data,
        //------------------------------------------------------
        //-- IPTX Interfaces
        //------------------------------------------------------
        stream<Ip4Addr>             &siIPTX_MacLkpReq,
        stream<ArpLkpReply>         &soIPTX_MacLkpRep,
        //------------------------------------------------------
        //-- CAM Interfaces
        //------------------------------------------------------
        stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
        stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
        stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
        stream<RtlMacUpdateReply>   &siCAM_MacUpdRep
    );
#endif    // HLS_VERSION

#endif

/*! \} */
