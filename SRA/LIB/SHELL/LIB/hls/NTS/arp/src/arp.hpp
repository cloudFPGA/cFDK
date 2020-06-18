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

/*******************************************************************************
 * @file       : arp.hpp
 * @brief      : Address Resolution Protocol (ARP) Server.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_ARP
 * \addtogroup NTS_ARP
 * \{
 *******************************************************************************/

#ifndef _ARP_H_
#define _ARP_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../AxisEth.hpp"

using namespace hls;

//OBSOLETE_20200617 #define NO_OF_BROADCASTS 2

//OBSOLETEE_20200617 const EthAddr     ETH_BROADCAST_MAC  = 0xFFFFFFFFFFFF; // Broadcast MAC Address
//OBSOLETEE_20200617 const EtherType   ETH_ETHERTYPE_ARP  = 0x0806; // EtherType value ARP

//OBSOLETEE_20200617 const ArpHwType   ARP_HTYPE_ETHERNET = 0x0001; //  Hardware type for Ethernet
//OBSOLETEE_20200617 const ArpProtType ARP_PTYPE_IPV4     = 0x0800; // Protocol type for IPv4
//OBSOLETEE_20200617 const ArpHwLen    ARP_HLEN_ETHERNET  =      6; // Hardware addr length for Ethernet
//OBSOLETEE_20200617 const ArpProtLen  ARP_PLEN_IPV4      =      4; // Protocol addr length for IPv4
//OBSOLETEE_20200617 const ArpOper     ARP_OPER_REQUEST   = 0x0001; // Operation is request
//OBSOLETEE_20200617 const ArpOper     ARP_OPER_REPLY     = 0x0002; // Operation is reply

//OBSOLETEE_20200617 const Ip4Addr     IP4_BROADCAST_ADD  = 0xFFFFFFFF; // Broadcast IP4 Address

//OBSOLETE_20200617 typedef ap_uint<1> ArpLkpHit;
//OBSOLETE_20200617 enum               ArpLkpHitStates { NO_HIT=0, HIT=1 };

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
void arp(
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

#endif
