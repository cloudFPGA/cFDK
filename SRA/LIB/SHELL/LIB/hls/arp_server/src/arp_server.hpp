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
 * @file       : arp_server.hpp
 * @brief      : Address Resolution Server (ARS).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                ARP server.
 *
 *****************************************************************************/

#ifndef ARS_H_
#define ARS_H_

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

using namespace hls;

const EthAddr     ETH_BROADCAST_MAC  = 0xFFFFFFFFFFFF; // Broadcast MAC Address
const EtherType   ETH_ETHERTYPE_ARP  = 0x0806; // EtherType value ARP

const ArpHwType   ARP_HTYPE_ETHERNET = 0x0001; //  Hardware type for Ethernet
const ArpProtType ARP_PTYPE_IPV4     = 0x0800; // Protocol type for IPv4
const ArpHwLen    ARP_HLEN_ETHERNET  =      6; // Hardware addr length for Ethernet
const ArpProtLen  ARP_PLEN_IPV4      =      4; // Protocol addr length for IPv4
const ArpOper     ARP_OPER_REQUEST   = 0x0001; // Operation is request
const ArpOper     ARP_OPER_REPLY     = 0x0002; // Operation is reply

const Ip4Addr     IP4_BROADCAST_ADD  = 0xFFFFFFFF; // Broadcast IP4 Address


typedef ap_uint<1> ArpLkpHit;
enum               ArpLkpHitStates { NO_HIT=0, HIT=1 };

typedef ap_uint<1> ArpLkpOp;
enum               ArpLkpOpCodes   { ARP_INSERT=0, ARP_DELETE=1 };

//OBSOLETE_20200301 typedef ap_uint<1> ArpLkpSrc;
//OBSOLETE_20200301 enum               ArpLkpSources   { IPRX=0, OTHER=1 };

/*** OBSOLETE-20200218 ****************
struct arpTableReply
{
	ap_uint<48> macAddress;
	bool		hit;
	arpTableReply() {}
	arpTableReply(ap_uint<48> macAdd, bool hit)
			:macAddress(macAdd), hit(hit) {}
};
***************************************/

/************************************************
 * ARP LOOKUP REPLY
 ************************************************/
class ArpLkpReply
{
  public:
    EthAddr     macAddress;
    ArpLkpHit   hit;
    ArpLkpReply() {}
    ArpLkpReply(EthAddr macAdd, ArpLkpHit hit) :
        macAddress(macAdd), hit(hit) {}
};


/************************************************
 * ARP BINDING - {MAC,IPv4} ASSOCIATION
 ************************************************/
class ArpBinding {
  public:
    EthAddr  macAddr;
    Ip4Addr  ip4Addr;
    ArpBinding() {}
    ArpBinding(EthAddr newMac, Ip4Addr newIp4) :
        macAddr(newMac), ip4Addr(newIp4) {}
};

/*** OBSOLETE-20201214 ****************
struct arpTableEntry {
    LE_EthAddr       macAddress;
    LE_Ip4Addr       ipAddress;
    ValBit           valid;
    arpTableEntry() {}
    arpTableEntry(LE_EthAddr newMac, LE_Ip4Addr newIp, ValBit newValid) :
        macAddress(newMac), ipAddress(newIp), valid(newValid) {}
};
***************************************/

/************************************************
 * ARP Metadata
 *  Structure extracted from incoming ARP packet.
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


/*** OBSOLETE-20200218 ****************
struct rtlMacUpdateRequest {
	ap_uint<1>			source;
	ap_uint<1>			op;
	ap_uint<48>			value;
	ap_uint<32>			key;

	rtlMacUpdateRequest() {}
	rtlMacUpdateRequest(ap_uint<32> key, ap_uint<48> value, ap_uint<1> op)
			:key(key), value(value), op(op), source(0) {}
};
***************************************/


/********************************************
 * CAM / MAC Update Request
 ********************************************/
class RtlMacUpdateRequest {
  public:
    EthAddr         value;  // 48-bits
    Ip4Addr         key;    // 32-bits
    //OBSOLETE_20200301 ap_uint<1>      source;
    ArpLkpOp        opcode; //  1-bit : '0' is INSERT, '1' is DELETE

    RtlMacUpdateRequest() {}
    //OBSOLETE_20200301 RtlMacUpdateRequest(Ip4Addr key, EthAddr value, arpLkpOp op) :
    //OBSOLETE_20200301     key(key), value(value), op(op), source(0) {}
    RtlMacUpdateRequest(Ip4Addr key, EthAddr value, ArpLkpOp opcode) :
            key(key), value(value), opcode(opcode) {}
};

/********************************************
 * CAM / MAC Update Reply
 ********************************************/
class RtlMacUpdateReply {
  public:
    EthAddr         value;  // 48-bits
    //OBSOLETE_20200301 ap_uint<1>      source;
    ArpLkpOp        opcode; //  1-bit : '0' is INSERT, '1' is DELETE

    RtlMacUpdateReply() {}
    //OBSOLETE_20200301 RtlMacUpdateReply(ArpLkpOp opcode) :
    //OBSOLETE_20200301     opcode(opcode), source(0) {}
    //OBSOLETE_20200301 RtlMacUpdateReply(EthAddr value, ArpLkpOp opcode) :
    //OBSOLETE_20200301     value(value), opcode(opcode), source(0) {}
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
    //OBSOLETE_20200301 ap_uint<1>      source;
    RtlMacLookupRequest() {}
    //OBSOLETE_20200301 RtlMacLookupRequest(Ip4Addr searchKey) :
    //OBSOLETE_20200301     key(searchKey), source(0) {}
    RtlMacLookupRequest(Ip4Addr searchKey) :
        key(searchKey) {}
};

/********************************************
 * CAM / MAC Lookup Reply
 ********************************************/
class RtlMacLookupReply {
  public:
    EthAddr         value;  // 48 bits
    ArpLkpHit       hit;    //  8 bits
    RtlMacLookupReply() {}
    RtlMacLookupReply(ArpLkpHit hit, EthAddr value) :
        hit(hit), value(value) {}
};


void arp_server(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_IpAddress,
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
