/*
 * Copyright 2016 -- 2020 IBM Corporation
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
 */

/*******************************************************************************
 * @file       : nts_types.hpp
 * @brief      : Definition of the types used by the Network Transport Stack
 *               (NTS) component of the cloudFPGA shell.
 *
 * System:     : cloudFPGA
 * Component   : Shell
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _NTS_TYPES_H_
#define _NTS_TYPES_H_

#include <hls_stream.h>

#include "AxisApp.hpp"   // Application (TCP segment or UDP datagram)
#include "AxisEth.hpp"   // ETHernet
#include "AxisIp4.hpp"   // IPv4

using namespace hls;


/******************************************************************************
 * GLOBAL DEFINITIONS USED BY NTS
 ******************************************************************************/

#define NTS_OK      1
#define NTS_KO      0
#define OK          NTS_OK
#define KO          NTS_KO

#define CMD_INIT    1
#define CMD_DROP    1
#define CMD_KEEP    0
#define CMD_ENABLE  1
#define CMD_DISABLE 0

#define QUERY_RD    0
#define QUERY_WR    1
#define QUERY_INIT  1
#define QUERY_FAST_RETRANSMIT true

#define FLAG_OFF    0
#define FLAG_ON     1

#define STS_OK      1
#define STS_KO      0
#define STS_OPENED  1
#define STS_CLOSED  0

#define ACK_ON      1
#define NO_ACK      0

static const ap_uint<16> MTU = 1500;

/******************************************************************************
 * GENERIC TYPES and CLASSES USED BY NTS
 ******************************************************************************
 * Some Terminology & Conventions:
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network.
 *  A PDU is therefore composed of a protocol specific control information
 *   (e.g, a header) and a user data section. This source code uses the
 *   following terminology:
 *   - a FRAME    (or MAC Frame)    refers to the Ethernet data link layer.
 *   - a PACKET   (or IP  Packet)   refers to the IP protocol data unit.
 *   - a SEGMENT  (or TCP Segment)  refers to the TCP protocol data unit.
 *   - a DATAGRAM (or UDP Datagram) refers to the UDP protocol data unit.
 ******************************************************************************/

/*********************************************************
 * SINGLE BIT DEFINITIONS
 *********************************************************/
typedef ap_uint<1> AckBit;  // Acknowledge: Always has to go back to the source of the stimulus (e.g. OpenReq/OpenAck).
typedef ap_uint<1> CmdBit;  // Command    : A verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
typedef ap_uint<1> FlagBit; // Flag       : Noon or a verb indicating a toggling state (e.g. on/off). Does not expect a return from recipient.
typedef ap_uint<1> RdWrBit; // Access mode: Read(0) or Write(1)
typedef ap_uint<1> ReqBit;  // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
typedef ap_uint<1> RepBit;  // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
typedef ap_uint<1> RspBit;  // Response   : Used when a reply does not go back to the source of the stimulus.
typedef ap_uint<1> SigBit;  // Signal     : Noun indicating a signal (e.g. RxEventSig). Does not expect a return from recipient.
typedef ap_uint<1> StsBit;  // Status     : Noun or verb indicating a status (e.g. isOpen). Does not  have to go back to source of stimulus.
typedef ap_uint<1> ValBit;  // Valid bit  : Must go along with something to validate/invalidate.

typedef bool AckBool;  // Acknowledge: Always has to go back to the source of the stimulus (e.g. OpenReq/OpenAck).
typedef bool CmdBool;  // Command    : Verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
typedef bool FlagBool; // Flag       : Noon or verb indicating a toggling state (e.g. on/off). Does not expect a return from recipient.
typedef bool HitBool;  // Hit        : Noon or verb indicating a success (e.g. match). Does not expect a return from recipient.
typedef bool ReqBool;  // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
typedef bool RepBool;  // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
typedef bool RspBool;  // Response   : Used when a reply does not go back to the source of the stimulus.
typedef bool SigBool;  // Signal     : Noun indicating a signal (e.g. TxEventSig). Does not expect a return from recipient.
typedef bool StsBool;  // Status     : Noun or verb indicating a status (e.g. isOpen). Does not  have to go back to source of stimulus.
typedef bool ValBool;  // Valid      : Must go along with something to validate/invalidate.

/******************************************************************************
 * DATA-LINK LAYER-2 - ETHERNET & ARP
 * ****************************************************************************
 * Terminology & Conventions
 *  - a FRAME    (or MAC Frame)    refers to the Ethernet data link layer.
 *  - a MESSAGE  (or ARP Packet)   refers to the ARP protocol data unit.
 ******************************************************************************/

//=========================================================
//== ETHERNET FRAME FIELDS - Constant Definitions
//=========================================================
// EtherType protocol numbers
#define IP4_PROTOCOL    0x0800
#define ARP_PROTOCOL    0x0806

//---------------------------------------------------------
//-- ARP BIND PAIR - {MAC,IPv4} ASSOCIATION
//---------------------------------------------------------
class ArpBindPair {
  public:
    EthAddr  macAddr;
    Ip4Addr  ip4Addr;
    ArpBindPair() {}
    ArpBindPair(EthAddr newMac, Ip4Addr newIp4) :
        macAddr(newMac), ip4Addr(newIp4) {}
};

//---------------------------------------------------------
//-- ARP LOOKUP REPLY
 //---------------------------------------------------------
class ArpLkpReply {
  public:
    EthAddr     macAddress;
    HitBool     hit;
    ArpLkpReply() {}
    ArpLkpReply(EthAddr macAdd, HitBool hit) :
        macAddress(macAdd), hit(hit) {}
};


/*******************************************************************************
 * NETWORK LAYER-3 - IPv4 & ICMP
 *******************************************************************************
 * Terminology & Conventions                                                   *
 * - a PACKET (or IpPacket) refers to an IP-PDU (i.e., Header+Data).           *
 *******************************************************************************/

// [TODO-ADD ALL TEH TYPES HERE]

//-----------------------------------------------------
//-- IP4 PACKET FIELDS - Constant Definitions
//-----------------------------------------------------
// IP protocol numbers
#define ICMP_PROTOCOL   0x01
#define TCP_PROTOCOL    0x06
#define UDP_PROTOCOL    0x11


/******************************************************************************
 * TRANSPORT LAYER-4 - UDP & TCP
 * ****************************************************************************
 * Terminology & Conventions
 *  - a SEGMENT  (or TCP Segment)  refers to the TCP protocol data unit.
 *  - a DATAGRAM (or UDP Datagram) refers to the UDP protocol data unit.
 ******************************************************************************/

/*********************************************************
 * LAYER-4 - COMMON TCP and UDP HEADER FIELDS
 *********************************************************/
typedef ap_uint<16> LE_Ly4Port; // Layer-4 Port in LE order
typedef ap_uint<16> LE_Ly4Len;  // Layer-4 Length in LE_order
typedef ap_uint<16> Ly4Port;    // Layer-4 Port
typedef ap_uint<16> Ly4Len;     // Layer-4 header plus data Length

/*********************************************************
 * TCP SESSION IDENTIFIER
 *********************************************************/
typedef ap_uint<16> SessionId;

/*********************************************************
 * SOCKET ADDRESS
 *********************************************************/
class SockAddr {  // Socket Address stored in NETWORK BYTE ORDER
   public:
    Ip4Addr        addr;   // IPv4 address in NETWORK BYTE ORDER
    Ly4Port        port;   // Layer-4 port in NETWORK BYTE ORDER
    SockAddr() {}
    SockAddr(Ip4Addr ip4Addr, Ly4Port layer4Port) :
        addr(ip4Addr), port(layer4Port) {}
};

class LE_SockAddr {  // Socket Address stored in LITTLE-ENDIAN order !!!
  public:
    LE_Ip4Address  addr;  // IPv4 address in LITTLE-ENDIAN order !!!
    LE_Ly4Port     port;  // Layer-4 port in LITTLE-ENDIAN order !!!
    LE_SockAddr() {}
    LE_SockAddr(LE_Ip4Address addr, LE_Ly4Port port) :
        addr(addr), port(port) {}
};

/*********************************************************
 * SOCKET PAIR ASSOCIATION
 *********************************************************/
class LE_SocketPair { // Socket Pair Association in LITTLE-ENDIAN order !!!
  public:
    LE_SockAddr  src;  // Source socket address in LITTLE-ENDIAN order !!!
    LE_SockAddr  dst;  // Destination socket address in LITTLE-ENDIAN order !!!
    LE_SocketPair() {}
    LE_SocketPair(LE_SockAddr src, LE_SockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (LE_SocketPair const &s1, LE_SocketPair const &s2) {
        return ((s1.dst.addr < s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}

class SocketPair { // Socket Pair Association in NETWORK-BYTE order !!!
  public:
    SockAddr  src;  // Source socket address in NETWORK-BYTE order !!!
    SockAddr  dst;  // Destination socket address in NETWORK-BYTE order !!!
    SocketPair() {}
    SocketPair(SockAddr src, SockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (SocketPair const &s1, SocketPair const &s2) {
        return ((s1.dst.addr <  s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}

#endif

/*! \} */
















