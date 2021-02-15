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
 * @file    : nts_types.hpp
 * @brief   : Definition of the types used by the Network Transport Stack
 *             (NTS) component of the cloudFPGA shell.
 *
 * System:   : cloudFPGA
 * Component : Shell
 * Language  : Vivado HLS
 *
 * @remarks  :
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network. A PDU is
 *   therefore composed of a protocol specific control information (e.g a header)
 *   and a user data section.
 *  This source code uses the following terminology:
 *   - a SEGMENT (or TCP Packet) refers to the TCP protocol data unit.
 *   - a PACKET  (or IP  Packet) refers to the IP protocol data unit.
 *   - a FRAME   (or MAC Frame)  refers to the Ethernet data link layer.
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _NTS_TYPES_H_
#define _NTS_TYPES_H_

#include <hls_stream.h>

//OBSOLETE_20210215 #include "AxisApp.hpp"   // Application (TCP segment or UDP datagram)
#include "AxisEth.hpp"   // ETHernet
#include "AxisIp4.hpp"   // IPv4
using namespace hls;


/*******************************************************************************
 * GLOBAL DEFINITIONS USED BY NTS
 *******************************************************************************/

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

#define FLAG_OOO    true
#define FLAG_INO    false

#define LKP_HIT     true
#define LKP_NO_HIT  false

#define STS_OK      1
#define STS_KO      0
#define STS_OPENED  1
#define STS_CLOSED  0

#define ACK_ON      1
#define NO_ACK      0

/*******************************************************************************
 * GENERIC TYPES and CLASSES USED BY NTS
 *******************************************************************************
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
 *******************************************************************************/

//========================================================
//== SINGLE BIT DEFINITIONS
//========================================================
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

//========================================================
//== BOOLEAN DEFINITIONS
//========================================================
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

//========================================================
//== MULTI-BITS DEFINITIONS
//========================================================
typedef ap_uint<16> SessionId;  // TCP Session ID (FIXME - Consider renaming)
typedef ap_uint<16> TcpSessId;  // TCP Session ID (alias for SessionId)


/*******************************************************************************
 * DATA-LINK LAYER-2 - ETHERNET & ARP
 * *****************************************************************************
 * Terminology & Conventions
 *  - a FRAME    (or MAC Frame)    refers to the Ethernet data link layer.
 *  - a MESSAGE  (or ARP Packet)   refers to the ARP protocol data unit.
 *******************************************************************************/

//=========================================================
//== ETHERNET FRAME FIELDS - Constant Definitions
//=========================================================
// Ethernet Broadcast MAC Address
#define ETH_BROADCAST_ADDR 0xFFFFFFFFFFFF

// EtherType protocol numbers
#define ETH_ETHERTYPE_IP4 0x0800
#define ETH_ETHERTYPE_ARP 0x0806

//=========================================================
//== ARP MESSAGE FIELDS - Constant Definitions
//=========================================================
#define ARP_HTYPE_ETHERNET 0x0001  // Hardware type for Ethernet
#define ARP_PTYPE_IPV4     0x0800  // Protocol type for IPv4
#define ARP_HLEN_ETHERNET       6  // Hardware addr length for Ethernet
#define ARP_PLEN_IPV4           4  // Protocol addr length for IPv4
#define ARP_OPER_REQUEST   0x0001  // Operation is request
#define ARP_OPER_REPLY     0x0002  // Operation is reply

/*******************************************************************************
 * NETWORK LAYER-3 - IPv4 & ICMP
 *******************************************************************************
 * Terminology & Conventions
 * - a PACKET  (or IP Packet)   refers to an IP-PDU (i.e., Header+Data).
 * - a MESSAGE (or ICMP Packet) refers to an ICMP protocol data unit.
 *******************************************************************************/

//=====================================================
//== IP4 PACKET FIELDS - Constant Definitions
//=====================================================

// IP4 Broadcast Address
#define IP4_BROADCAST_ADDR  0xFFFFFFFF
// IP4 Protocol numbers
#define IP4_PROT_ICMP       0x01
#define IP4_PROT_TCP        0x06
#define IP4_PROT_UDP        0x11


/*******************************************************************************
 * TRANSPORT LAYER-4 - UDP & TCP
 * *****************************************************************************
 * Terminology & Conventions
 *  - a SEGMENT  (or TCP Segment)  refers to the TCP protocol data unit.
 *  - a DATAGRAM (or UDP Datagram) refers to the UDP protocol data unit.
 *******************************************************************************/

//========================================================
//== LAYER-4 - COMMON TCP and UDP HEADER FIELDS
//========================================================
typedef ap_uint<16> LE_Ly4Port; // Layer-4 Port in LE order
typedef ap_uint<16> LE_Ly4Len;  // Layer-4 Length in LE_order
typedef ap_uint<16> Ly4Port;    // Layer-4 Port
typedef ap_uint<16> Ly4Len;     // Layer-4 header plus data Length

//--------------------------------------------------------
//-- LAYER-4 - SOCKET ADDRESS
//--------------------------------------------------------
class SockAddr {  // Socket Address stored in NETWORK BYTE ORDER
   public:
    Ip4Addr        addr;   // IPv4 address in NETWORK BYTE ORDER
    Ly4Port        port;   // Layer-4 port in NETWORK BYTE ORDER
    SockAddr() {}
    SockAddr(Ip4Addr ip4Addr, Ly4Port layer4Port) :
        addr(ip4Addr), port(layer4Port) {}
};

inline bool operator == (SockAddr const &s1, SockAddr const &s2) {
        return ((s1.addr == s2.addr) and (s1.port == s2.port));
}

class LE_SockAddr {  // Socket Address stored in LITTLE-ENDIAN order !!!
  public:
    LE_Ip4Address  addr;  // IPv4 address in LITTLE-ENDIAN order !!!
    LE_Ly4Port     port;  // Layer-4 port in LITTLE-ENDIAN order !!!
    LE_SockAddr() {}
    LE_SockAddr(LE_Ip4Address addr, LE_Ly4Port port) :
        addr(addr), port(port) {}
};

struct fourTuple {  // [FIXME-TODO] - Replace w/ LE_SocketPair
    ap_uint<32> srcIp;      // IPv4 address in LITTLE-ENDIAN order !!!
    ap_uint<32> dstIp;      // IPv4 address in LITTLE-ENDIAN order !!!
    ap_uint<16> srcPort;    // TCP  port in in LITTLE-ENDIAN order !!!
    ap_uint<16> dstPort;    // TCP  port in in LITTLE-ENDIAN order !!!
    fourTuple() {}
    fourTuple(ap_uint<32> srcIp, ap_uint<32> dstIp, ap_uint<16> srcPort, ap_uint<16> dstPort)
              : srcIp(srcIp), dstIp(dstIp), srcPort(srcPort), dstPort(dstPort) {}
};

inline bool operator < (fourTuple const& lhs, fourTuple const& rhs) {
        return lhs.dstIp < rhs.dstIp || (lhs.dstIp == rhs.dstIp && lhs.srcIp < rhs.srcIp);
}

//--------------------------------------------------------
//-- LAYER-4 - SOCKET PAIR ASSOCIATION
//--------------------------------------------------------
#ifdef _USE_STRUCT_SOCKET_PAIR_
struct SocketPair {
#else
class SocketPair { // Socket Pair Association in NETWORK-BYTE order !!!
  public:
#endif
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


/*******************************************************************************
 * APPLICATION LAYER-5
 *******************************************************************************
 * This section defines the types and classes related to the Application (APP)
 * layer and interfaces.
 *******************************************************************************/

//=========================================================
//== TCP Connection States
//==  The RFC-793 defines a set of states that a connection
//==   may progresses through during its lifetime. These
//==   states are:  LISTEN, SYN-SENT, SYN-RECEIVED, ESTABLISHED,
//==   FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING, LAST-ACK,
//==   TIME-WAIT, and the fictional state CLOSED.
//==  The implementation of [TOE] does use all of these states:
//==    * There is no explicit 'LISTEN' which is merged into 'CLOSED'.
//==    * The 'CLOSE-WAIT' is not used, since 'sndFIN' is sent out
//==      immediately after the reception of a 'rcvFIN' and the
//==      application is simply notified.
//==    * 'FIN_WAIT_2' is also not used.
//=========================================================
enum TcpState { CLOSED=0,    SYN_SENT,    SYN_RECEIVED,   ESTABLISHED, \
                FIN_WAIT_1,  FIN_WAIT_2,  CLOSING,        TIME_WAIT,   \
                LAST_ACK };

//=========================================================
//== TCP Application Send Error Codes
//==  Error codes returned by NTS after a send request.
//=========================================================
enum TcpAppSndErr { NO_ERROR=0, NO_SPACE, NO_CONNECTION };

//=========================================================
//== TCP Application Notification Codes
//==  Error codes returned by NTS after a data send transfer
//=========================================================
// [TODO] enum TcpAppWrStsCode { FAILED_TO_OPEN_CON=false, CON_IS_OPENED=true };


/*******************************************************************************
 * NTS INTERNAL - TAIF / TOE
 *******************************************************************************/


/*******************************************************************************
 * NTS INTERNAL - ARP / CAM
 *******************************************************************************
 * This section defines the interfaces between the Address Resolution Protocol
 * (ARP) server and the IP Tx HAndler (IPTX).
 *******************************************************************************/

//========================================================
//== ARPCAM - TYPES and CLASSES USED BY THIS INTERFACE
//========================================================

//---------------------------------------------------------
//-- ARPCAM - BIND PAIR - {MAC,IPv4} ASSOCIATION
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
//-- ARPCAM - LOOKUP REQUEST
//---------------------------------------------------------
typedef Ip4Addr ArpLkpRequest;

//---------------------------------------------------------
//-- ARPCAM - LOOKUP REPLY
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
 * NTS INTERNAL - TOE / CAM
 *******************************************************************************
 * This section defines the interfaces between the TCP Offload Engine (TOE) and
 * the Content Addressable Memory (CAM) used to manage the TCP sessions.
 * Warning:
 *   Do not change the order of the fields in the session-lookup-request, the
 *   session-lookup-reply, the session-update-request and the session-update-
 *   reply classes as these structures may end up being mapped to a RTL physical
 *   Axi4-Stream interface between the TOE and the CAM (if CAM is RTL-based).
 * Info: The member elements of the classes are placed into the physical vector
 *   interface in the order they appear in the C code: the first element of the
 *   structure is aligned on the LSB of the vector and the final element of the
 *   structure is aligned with the MSB of the vector.
 *******************************************************************************/

//========================================================
//== TOECAM - TYPES and CLASSES USED BY THIS INTERFACE
//========================================================
typedef ap_uint<14> RtlSessId;  // Used by RTL-based CAM
typedef ap_uint< 1> LkpSrcBit;  // Encodes the initiator of a CAM lookup or update.
#define FROM_RXe   0
#define FROM_TAi   1
enum LkpOpBit { INSERT=0, DELETE };  // Encodes the CAM operation

//--------------------------------------------------------
//-- TOECAM - Four Tuple
//--  This class defines the internal storage used by the
//--  TOECAM implementation for the SocketPair. The class
//--  uses the terms 'my' and 'their' instead of 'dest' and
//--  'src'.
//--  The operator '<' is necessary here for the c++ dummy
//--  memory implementation which uses an std::map.
 //=========================================================
class FourTuple {  // [FIXME - Replace with SocketPair]
  public:
    LE_Ip4Addr  myIp;
    LE_Ip4Addr  theirIp;
    LE_TcpPort  myPort;
    LE_TcpPort  theirPort;
    FourTuple() {}
    FourTuple(LE_Ip4Addr myIp, LE_Ip4Addr theirIp, LE_TcpPort myPort, LE_TcpPort theirPort) :
        myIp(myIp), theirIp(theirIp), myPort(myPort), theirPort(theirPort) {}

    bool operator < (const FourTuple& other) const {
        if (myIp < other.myIp) {
            return true;
        }
        else if (myIp == other.myIp) {
            if (theirIp < other.theirIp) {
                return true;
            }
            else if(theirIp == other.theirIp) {
                if (myPort < other.myPort) {
                    return true;
                }
                else if (myPort == other.myPort) {
                    if (theirPort < other.theirPort) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

inline bool operator == (FourTuple const &s1, FourTuple const &s2) {
    return ((s1.myIp    == s2.myIp)    && (s1.myPort    == s2.myPort)    &&
            (s1.theirIp == s2.theirIp) && (s1.theirPort == s2.theirPort));
}

//--------------------------------------------------------
//-- TOECAM - Session Lookup Request
//--------------------------------------------------------
class CamSessionLookupRequest {
  public:
    FourTuple     key;       // 96 bits
    LkpSrcBit     source;    //  1 bit : '0' is [RXe], '1' is [TAi]

    CamSessionLookupRequest() {}
    CamSessionLookupRequest(FourTuple tuple, LkpSrcBit src)
                : key(tuple), source(src) {}
};

//--------------------------------------------------------
//-- CAM - Session Lookup Reply
//--------------------------------------------------------
class CamSessionLookupReply {
  public:
    RtlSessId        sessionID; // 14 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe], '1' is [TAi]
    bool             hit;       //  1 bit

    CamSessionLookupReply() {}
    CamSessionLookupReply(bool hit, LkpSrcBit src) :
        hit(hit), sessionID(0), source(src) {}
    CamSessionLookupReply(bool hit, RtlSessId id, LkpSrcBit src) :
        hit(hit), sessionID(id), source(src) {}
};

//--------------------------------------------------------
//-- CAM - Session Update Request
//--------------------------------------------------------
class CamSessionUpdateRequest {
  public:
    FourTuple     key;       // 96 bits
    RtlSessId     value;     // 14 bits
    LkpSrcBit     source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    LkpOpBit      op;        //  1 bit : '0' is INSERT, '1' is DELETE

    CamSessionUpdateRequest() {}
    CamSessionUpdateRequest(FourTuple key, RtlSessId value, LkpOpBit op, LkpSrcBit src) :
        key(key), value(value), op(op), source(src) {}
};

//--------------------------------------------------------
//-- CAM - Session Update Reply
//--------------------------------------------------------
class CamSessionUpdateReply {
  public:
    RtlSessId        sessionID; // 14 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    LkpOpBit         op;        //  1 bit : '0' is INSERT, '1' is DELETE

    CamSessionUpdateReply() {}
    CamSessionUpdateReply(LkpOpBit op, LkpSrcBit src) :
        op(op), source(src) {}
    CamSessionUpdateReply(RtlSessId id, LkpOpBit op, LkpSrcBit src) :
        sessionID(id), op(op), source(src) {}
};

#endif

/*! \} */
















