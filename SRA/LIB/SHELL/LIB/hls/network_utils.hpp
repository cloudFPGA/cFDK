//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared accross HLS cores. 
//  *
//  * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
//  * Copyright 2015-2019 - IBM Research - All Rights Reserved.
//  *
//  *----------------------------------------------------------------------------
//  *
//  * @details    : Data structures, types and prototypes definitions for the
//  *                   cFDK Role.
//  *
//  * @terminology:
//  *      In telecommunications, a protocol data unit (PDU) is a single unit of
//  *       information transmitted among peer entities of a computer network.
//  *      A PDU is therefore composed of a protocol specific control information
//  *       (e.g, a header) and a user data section.
//  *  This source code uses the following terminology:
//  *   - a SEGMENT  (or TCP Packet) refers to the TCP protocol data unit.
//  *   - a DATAGRAM )or UDP Packet) refers to the UDP protocol data unit.
//  *   - a PACKET   (or IP  Packet) refers to the IP protocol data unit.
//  *   - a FRAME    (or MAC Frame)  refers to the Ethernet data link layer.
//  *

#ifndef _CF_NETWORK_UTILS_
#define _CF_NETWORK_UTILS_


#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <vector>

#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

static const uint16_t MAX_SESSIONS = 32;

#define NO_TX_SESSIONS 10 // Number of Tx Sessions to open for testing

// Forward declarations.
struct rtlSessionUpdateRequest;
struct rtlSessionUpdateReply;
struct rtlSessionLookupReply;
struct rtlSessionLookupRequest;




#define OOO_N 4     // number of OOO blocks accepted
#define OOO_W 4288  // window {max(offset + length)} of sequence numbers beyond recvd accepted

// Usually TCP Maximum Segment Size is 1460. Here, we use 1456 to support TCP options.
static const ap_uint<16> MTU = 1500;
static const ap_uint<16> MSS = 1456;  // MTU-IP-Hdr-TCP-Hdr=1500-20-20
//OBSOLETE static const ap_uint<16> MY_MSS = 576;

// OOO Parameters
//static const int OOO_N = 4;       // number of OOO blocks accepted
//static const int OOO_W = 4288;    // window {max(offset + length)} of sequence numbers beyond recvd accepted
static const int OOO_N_BITS = 3;        // bits required to represent OOO_N+1, need 0 to show no OOO blocks are valid
static const int OOO_W_BITS = 13;       // bits required to represent OOO_W
static const int OOO_L_BITS = 13;       // max length in bits of OOO blocks allowed

//static const int OOO_N_BITS = ceil(log10(OOO_N+1)/log10(2));      // (3) bits required to represent OOO_N
//static const int OOO_W_BITS = ceil(log10(OOO_W)  /log10(2));      // (13) bits required to represent OOO_W

static const ap_uint<32> SEQ_mid = 2147483648; // used in Modulo Arithmetic Comparison 2^(32-1) of sequence numbers etc.


using namespace hls;

#ifndef _AXI_CLASS_DEFINED_
#define _AXI_CLASS_DEFINED_
//FIXME: merge with definition in AxisRaw.hpp
/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
 template<int D>
   struct Axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(~(((ap_uint<D>) single_data) & 0)), tlast(1) {}
   };


#endif


 struct IPMeta {
   ap_uint<32> ipAddress;
   IPMeta() {}
   IPMeta(ap_uint<32> ip) : ipAddress(ip) {}
 };

/********************************************
 * Socket transport address.
 ********************************************/

struct SocketAddr {
     ap_uint<16>    port;   // Port in network byte order
     ap_uint<32>    addr;   // IPv4 address (or node_id)
};


/********************************************
 * Generic Streaming Interfaces.
 ********************************************/

typedef bool AxisAck;       // Acknowledgment over Axi4-Stream I/F


/********************************************
 * UDP Specific Streaming Interfaces.
 ********************************************/


struct UdpMeta {            // UDP Socket Pair Association
    SocketAddr      src;    // Source socket address
    SocketAddr      dst;    // Destination socket address 
    //UdpMeta()       {}
};

typedef ap_uint<16>     UdpPLen; // UDP Payload Length

typedef ap_uint<16>     UdpPort; // UDP Port Number


#define BROADCASTCHANNELS 2

// SOME QUERY AND COMMAND DEFINITIONS
#define QUERY_RD              0
#define QUERY_WR              1
#define QUERY_INIT            1

#define QUERY_FAST_RETRANSMIT true

#define CMD_INIT  1

/*************************************************************************
 * GLOBAL DEFINES and GENERIC TYPES
 *************************************************************************/
#define cIP4_ADDR_WIDTH            32

#define cTCP_PORT_WIDTH            16

#define cSHL_TOE_SESS_ID_WIDTH     16   // [TODO - Move into a CFG file.]
#define cSHL_TOE_LSN_ACK_WIDTH      1   // [TODO - Move into a CFG file.]
#define cSHL_TOE_LSN_REQ_WIDTH     cSHL_TOE_SESS_ID_WIDTH
#define cSHL_TOE_OPN_REQ_WIDTH    (cIP4_ADDR_WIDTH + cTCP_PORT_WIDTH)
#define cSHL_TOE_CLS_REQ_WIDTH     cSHL_TOE_SESS_ID_WIDTH

typedef ap_uint<cSHL_TOE_SESS_ID_WIDTH> SessionId;
typedef ap_uint<cSHL_TOE_LSN_ACK_WIDTH> LsnAck;
typedef ap_uint<cSHL_TOE_LSN_REQ_WIDTH> LsnReq;
typedef ap_uint<cSHL_TOE_OPN_REQ_WIDTH> OpnReq;
typedef ap_uint<cSHL_TOE_CLS_REQ_WIDTH> ClsReq;


/********************************************
 * SINGLE BIT DEFINITIONS
 ********************************************/

typedef ap_uint<1> RdWrBit; // Access mode: Read(0) or Write(1)
typedef ap_uint<1> CmdBit;  // Command    : A verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.

typedef bool AckBit;  // Acknowledge: Always has to go back to the source of the stimulus (.e.g OpenReq/OpenAck).
typedef bool CmdBool; // Command    : Verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
typedef bool ReqBit;  // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
typedef bool RepBit;  // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
typedef bool RspBit;  // Response   : Used when a reply does not go back to the source of the stimulus.
typedef bool SigBool; // Signal     : Noun indicating a signal (e.g. TxEventSig). Does not expect a return from recipient.
typedef bool StsBool; // Status     : Noun or verb indicating a status (.e.g isOpen). Does not  have to go back to source of stimulus.
typedef bool ValBit;  // Valid bit  : Must go along with something to validate/invalidate.

//TODO: fix paralell declaration
//typedef ap_uint<1> AckBit;  // Acknowledge: Always has to go back to the source of the stimulus (.e.g OpenReq/OpenAck).
//typedef ap_uint<1> CmdBit;  // Command    : A verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
//typedef ap_uint<1> FlagBit; // Flag       : Noon or a verb indicating a toggling state (e.g. on/off). Does not expect a return from recipient.
//typedef ap_uint<1> RdWrBit; // Access mode: Read(0) or Write(1)
//typedef ap_uint<1> ReqBit;  // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
//typedef ap_uint<1> RepBit;  // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
//typedef ap_uint<1> RspBit;  // Response   : Used when a reply does not go back to the source of the stimulus.
//typedef ap_uint<1> SigBit;  // Signal     : Noun indicating a signal (e.g. RxEventSig). Does not expect a return from recipient.
typedef ap_uint<1> StsBit;  // Status     : Noun or verb indicating a status (.e.g isOpen). Does not  have to go back to source of stimulus.
//typedef ap_uint<1> ValBit;  // Valid bit  : Must go along with something to validate/invalidate.
//
//typedef bool AckBool; // Acknowledge: Always has to go back to the source of the stimulus (.e.g OpenReq/OpenAck).
//typedef bool CmdBool; // Command    : Verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
//typedef bool ReqBool; // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
//typedef bool RepBool; // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
//typedef bool RspBool; // Response   : Used when a reply does not go back to the source of the stimulus.
//typedef bool SigBool; // Signal     : Noun indicating a signal (e.g. TxEventSig). Does not expect a return from recipient.
//typedef bool StsBool; // Status     : Noun or verb indicating a status (.e.g isOpen). Does not  have to go back to source of stimulus.
//typedef bool ValBool;  // Valid bit  : Must go along with something to validate/invalidate.



/********************************************
 * GENERAL ENUMERATIONS
 ********************************************/

// WARNING ABOUT ENUMERATIONS:
//   Avoid using 'enum' for boolean variables because scoped enums are only available with -std=c++
//   E.g.: enum PortState : bool {CLOSED_PORT = false, OPENED_PORT = true};

enum eventType {TX, RT, ACK, SYN, SYN_ACK, FIN, RST, ACK_NODELAY};

/*
 * There is no explicit LISTEN state
 * CLOSE-WAIT state is not used, since the FIN is sent out immediately after we receive a FIN, the application is simply notified
 * FIN_WAIT_2 is also unused
 * LISTEN is merged into CLOSED
 */

enum notificationType {PKG, CLOSE, TIME_OUT, RESET};
enum { WORD_0, WORD_1, WORD_2, WORD_3, WORD_4 };


/* (adapted from Linux /net/tcp.h line 292)
* The next routines deal with comparing 32 bit unsigned ints
* and worry about wraparound (automatic with unsigned arithmetic).
*
* These functions are equivalent to the following operators (modulo 2^32)
* before()  <
* !after()  <=
* after()   >
* !before() >=
*
*/
static inline bool before(ap_uint<32> seq1, ap_uint<32> seq2) {
    return (ap_int<32>)(seq1-seq2) < 0;
}
#define after(seq2, seq1)       before(seq1, seq2)


/*************************************************************************
 * NETWORK LAYER-2 DEFINITIONS
 *************************************************************************
 * Terminology & Conventions
 * - a FRAME refers to the Ethernet (.i.e, a Data-Link-Layer PDU).
 *************************************************************************/

/*************************************************************************
 * IPv4 and TCP Field Type Definitions as Encoded by the MAC.
 *   WARNING:
 *     The IPv4 and TCP fields defined in this section refer to the format
 *     generated by the 10GbE MAC of Xilinx which organizes its two 64-bit
 *     Rx and Tx interfaces into 8 lanes (see PG157). The result of this
 *     division into lanes, is that the IPv4 and TCP fields end up being
 *     stored in LITTLE-ENDIAN  order instead of the initial big-endian
 *     order used to transmit bytes over the media. As an example, consider
 *     the 16-bit field "Total Length" of an IPv4 packet and let us assume
 *     that this length is 40. This field will be transmitted on the media
 *     in big-endian order as '0x00' followed by '0x28'. However, the same
 *     field will end up being ordered in little-endian mode (.i.e, 0x2800)
 *     by the AXI4-Stream interface of the 10GbE MAC.
 *
 *     The mapping of an IPv4/TCP packet onto the AXI4-Stream interface of
 *     the 10GbE MAC is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag. Offset  |Flags|         |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Destination Port        |          Source Port          |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Acknowledgment Number                      |                        Sequence Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               |                               |                               |   |U|A|P|R|S|F|  Data |       |
 *  |         Urgent Pointer        |           Checksum            |            Window             |   |R|C|S|S|Y|I| Offset|  Res  |
 *  |                               |                               |                               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *************************************************************************/

/**********************************************************
 * IPv4 HEADER FIELDS TRANSMITTED BY THE MAC
 *  Type Definitions in AXI4-Stream Order (Little-Endian)
 **********************************************************/
typedef ap_uint< 4> LeIp4Version;     // IPv4 Version over Axi
typedef ap_uint< 4> LeIp4HdrLen;      // IPv4 Internet Header Length over Axi
typedef ap_uint< 8> LeIp4ToS;         // IPv4 Type of Service over Axi
typedef ap_uint<16> LeIp4TotalLen;    // IPv4 Total Length over Axi
typedef ap_uint<32> LeIp4SrcAddr;     // IPv4 Source Address over Axi
typedef ap_uint<32> LeIp4DstAddr;     // IPv4 Destination Address over Axi
typedef ap_uint<32> LeIp4Address;     // IPv4 Source or Destination Address over Axi
typedef ap_uint<32> LeIp4Addr;        // IPv4 Source or Destination Address over Axi
typedef ap_uint<64> LeIpData;         // IPv4 Data stream

/*********************************************************
 * TCP HEADER FIELDS TRANSMITTED BY THE MAC
 *  Type Definitions in AXI4-Stream Order (Little-Endian)
 *********************************************************/
typedef ap_uint<16> LeTcpSrcPort;     // TCP Source Port over Axi
typedef ap_uint<16> LeTcpDstPort;     // TCP Destination Port over Axi
typedef ap_uint<16> LeTcpPort;        // TCP Source or Destination Port over Axi
typedef ap_uint<32> LeTcpSeqNum;      // TCP Sequence Number over Axi
typedef ap_uint<32> LeTcpAckNum;      // TCP Acknowledgment Number over Axi
typedef ap_uint<4>  LeTcpDataOff;     // TCP Data Offset over Axi
typedef ap_uint<6>  LeTcpCtrlBits;    // TCP Control Bits over Axi
typedef ap_uint<16> LeTcpWindow;      // TCP Window over Axi
typedef ap_uint<16> LeTcpChecksum;    // TCP Checksum
typedef ap_uint<16> LeTcpUrgPtr;      // TCP Urgent Pointer over Axi
typedef ap_uint<64> LeTcpData;        // TCP Data stream


/*************************************************************************
 * NETWORK LAYER-3 DEFINITIONS
 *************************************************************************
 * Terminology & Conventions
 * - a PACKET (or IpPacket) refers to an IP-PDU (i.e., Header+Data).
 *************************************************************************/

/*********************************************************
 * IPv4 - HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint< 4> Ip4Version;     // IP4 Version
typedef ap_uint< 4> Ip4HdrLen;      // IP4 Header Length in octets (same as 4*Ip4HeaderLen)
typedef ap_uint< 8> Ip4ToS;         // IP4 Type of Service
typedef ap_uint<16> Ip4TotalLen;    // IP4 Total  Length
typedef ap_uint<16> Ip4Ident;       // IP4 Identification
typedef ap_uint<13> Ip4FragOff;     // IP4 Fragment Offset
typedef ap_uint< 3> Ip4Flags;       // IP4 Flags
typedef ap_uint< 8> Ip4TtL;         // IP4 Time to Live
typedef ap_uint< 8> Ip4Prot;        // IP4 Protocol
typedef ap_uint< 8> Ip4HdrCsum;     // IP4 Header Checksum
typedef ap_uint<32> Ip4SrcAddr;     // IP4 Source Address
typedef ap_uint<32> Ip4DstAddr;     // IP4 Destination Address
typedef ap_uint<32> Ip4Address;     // IP4 Source or Destination Address
typedef ap_uint<32> Ip4Addr;        // IP4 Source or Destination Address

typedef ap_uint<16> Ip4PktLen;      // IP4 Packet Length in octets (same as Ip4TotalLen)
typedef ap_uint<16> Ip4DatLen;      // IP4 Data   Length in octets (same as Ip4PktLen minus Ip4HdrLen)


/*************************************************************************
 * NETWORK LAYER-4 DEFINITIONS
 *************************************************************************
 * Terminology & Conventions
 * - a SEGMENT (or TcpPacket)  refers to a TCP-PDU (i.e., Header+Data).
 * - a DATAGRAM (or UdpPacket) refers to a UDP-PDU (i.e., Header+Data).
 *************************************************************************/

/*********************************************************
 * TCP - HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<16> TcpSrcPort;     // TCP Source Port
typedef ap_uint<16> TcpDstPort;     // TCP Destination Port
typedef ap_uint<16> TcpPort;        // TCP Source or Destination Port Number
typedef ap_uint<32> TcpSeqNum;      // TCP Sequence Number
typedef ap_uint<32> TcpAckNum;      // TCP Acknowledge Number
typedef ap_uint<4>  TcpDataOff;     // TCP Data Offset
typedef ap_uint<6>  TcpCtrlBits;    // TCP Control Bits
typedef ap_uint<1>  TcpCtrlBit;     // TCP Control Bit
typedef ap_uint<16> TcpWindow;      // TCP Window
typedef ap_uint<16> TcpChecksum;    // TCP Checksum
typedef ap_uint<16> TcpCSum;        // TCP Checksum
typedef ap_uint<16> TcpUrgPtr;      // TCP Urgent Pointer

typedef ap_uint< 8> TcpOptKind;     // TCP Option Kind
typedef ap_uint<16> TcpOptMss;      // TCP Option Maximum Segment Size

typedef ap_uint<16> TcpSegLen;      // TCP Segment Length in octets (same as Ip4DatLen)
typedef ap_uint< 8> TcpHdrLen;      // TCP Header  Length in octets
typedef ap_uint<16> TcpDatLen;      // TCP Data    Length in octets (same as TcpSegLen minus TcpHdrLen)

/***********************************************
 * TCP - PORT RANGES (Static & Ephemeral)
 ***********************************************/
typedef ap_uint<15> TcpStaPort;     // TCP Static  Port [0x0000..0x7FFF]
typedef ap_uint<15> TcpDynPort;     // TCP Dynamic Port [0x8000..0xFFFF]



/*************************************************************************
 * GENERIC TYPES and CLASSES USED BY TOE
 *************************************************************************
 * Terminology & Conventions
 * - .
 * - .
 *************************************************************************/

typedef ap_uint<16> SessionId;


/***********************************************
 * SOCKET ADDRESS (alias ipTuple)
 ***********************************************/
class LeSockAddr {   // Socket Address stored in LITTLE-ENDIAN order !!!
  public:
    LeIp4Address   addr;   // IPv4 address in LITTLE-ENDIAN order !!!
    LeTcpPort      port;   // TCP  port in in LITTLE-ENDIAN order !!!
    LeSockAddr() {}
    LeSockAddr(LeIp4Address addr, LeTcpPort port) :
        addr(addr), port(port) {}
};

struct ipTuple
{
    ap_uint<32>     ip_address;
    ap_uint<16>     ip_port;
};

class SockAddr {   // Socket Address stored in NETWORK BYTE ORDER
   public:
    Ip4Addr         addr;   // IPv4 address in NETWORK BYTE ORDER
    TcpPort         port;   // TCP  port    in NETWORK BYTE ORDER
    SockAddr() {}
    SockAddr(Ip4Addr ip4Addr, TcpPort tcpPort) :
        addr(ip4Addr), port(tcpPort) {}
};


/***********************************************
 * SOCKET PAIR ASSOCIATION (alias FourTuple)
 ***********************************************/
class LeSocketPair { // Socket Pair Association in LITTLE-ENDIAN order !!!
  public:
    LeSockAddr    src;    // Source socket address in LITTLE-ENDIAN order !!!
    LeSockAddr    dst;    // Destination socket address in LITTLE-ENDIAN order !!!
    LeSocketPair() {}
    LeSocketPair(LeSockAddr src, LeSockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (LeSocketPair const &s1, LeSocketPair const &s2) {
        return ((s1.dst.addr < s2.dst.addr) ||
               (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}

struct fourTuple {
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

class SocketPair { // Socket Pair Association in NETWORK-BYTE order !!!
  public:
    SockAddr    src;    // Source socket address in NETWORK-BYTE order !!!
    SockAddr    dst;    // Destination socket address in NETWORK-BYTE order !!!
    SocketPair() {}
    SocketPair(SockAddr src, SockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (SocketPair const &s1, SocketPair const &s2) {
        return ((s1.dst.addr <  s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}


/***********************************************
 * AXI4 STREAMING INTERFACES (alias AXIS)
 ************************************************/
class AxiWord {    // AXI4-Streaming Chunk (i.e. 8 bytes)
public:
    ap_uint<64>     tdata;
    ap_uint<8>      tkeep;
    ap_uint<1>      tlast;
public:
    AxiWord()       {}
    AxiWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
            tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

#define TLAST       1

// Sub-types of the generic AXI4-Stream Interface
//------------------------------------------------
typedef AxiWord Ip4Word;   // An AXI4-Stream carrying IPv4 type of data
//typedef AxiWord TcpWord;   // An AXI4-Stream carrying TCP  type of data


//OBSOLETE struct axiWord {
//OBSOLETE     ap_uint<64>     data;
//OBSOLETE     ap_uint<8>      keep;
//OBSOLETE     ap_uint<1>      last;
//OBSOLETE     axiWord() {}
//OBSOLETE     axiWord(ap_uint<64>      data, ap_uint<8> keep, ap_uint<1> last) :
//OBSOLETE             data(data), keep(keep), last(last) {}
//OBSOLETE };


/***********************************************
 * Open Session Status
 *  Reports if a session is opened or closed.
 ***********************************************/
enum SessOpnSts { FAILED_TO_OPEN_SESS=false, SESS_IS_OPENED=true };

class OpenStatus
{
  public:
    SessionId    sessionID;
    SessOpnSts   success;          // [FIXME - rename this member]
    OpenStatus() {}
    OpenStatus(SessionId sessId, SessOpnSts success) :
        sessionID(sessId), success(success) {}
};

/********************************************
 * Session Lookup Controller (SLc)
 ********************************************/
typedef SessionId   TcpSessId;  // TCP Session ID

//OBSOLETE typedef ap_uint<4>  TcpBuffId;  // TCP buffer  ID  [FIXME - Needed?]

struct sessionLookupQuery
{
    LeSocketPair  tuple;
    bool           allowCreation;
    sessionLookupQuery() {}
    sessionLookupQuery(LeSocketPair tuple, bool allowCreation) :
        tuple(tuple), allowCreation(allowCreation) {}
};

typedef bool HitState;
enum         HitStates {SESSION_UNKNOWN = false, SESSION_EXISTS = true};

struct sessionLookupReply
{
    SessionId   sessionID;
    HitState    hit;
    sessionLookupReply() {}
    sessionLookupReply(SessionId id, HitState hit) :
        sessionID(id), hit(hit) {}
};
/*** [TODO] ***
class SessionLookupReply
{

    enum         HitStates {SESSION_UNKNOWN = false, SESSION_EXISTS = true};
    typedef bool HitState;
  public:
    SessionId   sessionID;
    HitState    hitState;
    SessionLookupReply() {}
    SessionLookupReply(SessionId id, HitState hit) :
        sessionID(id), hitState(hit) {}
};
***/

/********************************************
 * State Table (STt)
 ********************************************/
enum SessionState { CLOSED=0,    SYN_SENT,    SYN_RECEIVED,   ESTABLISHED, \
                    FIN_WAIT_1,  FIN_WAIT_2,  CLOSING,        TIME_WAIT,   \
                    LAST_ACK };

//see simulation_utils!!

// Session State Query
class StateQuery {
  public:
    SessionId       sessionID;
    SessionState    state;
    RdWrBit         write;
    StateQuery() {}
    StateQuery(SessionId id) :
        sessionID(id), state(CLOSED), write(QUERY_RD) {}
    StateQuery(SessionId id, SessionState state, RdWrBit write) :
        sessionID(id), state(state), write(write) {}
};


/********************************************
 * Port Table (PRt)
 ********************************************/
// NotUsed typedef bool PortState;
// NotUSed enum         PortStates {PORT_IS_CLOSED = false, PORT_IS_OPENED = true};

// NotUsed typedef bool PortRange;
// NotUsed enum         PortRanges {PORT_IS_ACTIVE = false, PORT_IS_LISTENING = true};

/********************************************
 * Some Rx & Tx SAR Types
 ********************************************/
typedef TcpSeqNum   RxSeqNum;   // A received sequence number [TODO - Replace Rx with Rcv]
typedef TcpWindow   RcvWinSize; // A received window size
typedef TcpAckNum   TxAckNum;   // An acknowledgement number [TODO - Replace Tx with Snd]
typedef TcpWindow   SndWinSize; // A sending  window size

typedef ap_uint<32> RxMemPtr;  // A pointer to RxMemBuff ( 4GB)
typedef ap_uint<32> TxMemPtr;  // A pointer to TxMemBuff ( 4GB)
typedef ap_uint<16> RxBufPtr;  // A pointer to RxSessBuf (64KB)
typedef ap_uint<16> TxBufPtr;  // A pointer to TxSessBuf (64KB)

/************************************************
 * Rx SAR Table (RSt)
 *  Structure to manage the FPGA Receive Window
 ************************************************/
class RxSarEntry {
  public:
    RxSeqNum        rcvd;  // Octest RCV'ed and ACK'ed octets (Receive Next)
    RxBufPtr        appd;  // Ptr in circular APP data buffer (64KB)
    RxSarEntry() {}
};

// RSt / Query from RXe
//----------------------
class RXeRxSarQuery {
  public:
    SessionId       sessionID;
    RxSeqNum        rcvd;  // Expected SeqNum of the next byte from remote device.
    RdWrBit         write;
    CmdBit          init;
    RXeRxSarQuery() {}
    RXeRxSarQuery(SessionId id) : // Read query
        sessionID(id), rcvd(0),     write(0),     init(0) {}
    RXeRxSarQuery(SessionId id, RxSeqNum recvd, RdWrBit write) :
        sessionID(id), rcvd(recvd), write(write), init(0) {}
    RXeRxSarQuery(SessionId id, RxSeqNum recvd, RdWrBit write, CmdBit init) :
        sessionID(id), rcvd(recvd), write(write), init(init) {}
};

// RSt / Query from RAi
//----------------------
class RAiRxSarQuery {
  public:
    SessionId       sessionID;
    RxBufPtr        appd; // APP data read ptr
    RdWrBit         write;
    RAiRxSarQuery() {}
    RAiRxSarQuery(SessionId id) :
        sessionID(id), appd(0), write(0) {}
    RAiRxSarQuery(SessionId id, ap_uint<16> appd) :
        sessionID(id), appd(appd), write(1) {}
};

class RAiRxSarReply {
  public:
    SessionId       sessionID;
    RxBufPtr        appd; // Read by APP
    RAiRxSarReply() {}
    RAiRxSarReply(SessionId id, ap_uint<16> appd) :
        sessionID(id), appd(appd) {}
};

/********************************************
 * Tx SAR Table (TSt)
 ********************************************/
class TxSarEntry {
  public:
    TxAckNum        ackd;        // Octets TX'ed and ACK'ed
    TxAckNum        not_ackd;    // Octets TX'ed but not ACK'ed
    RcvWinSize      recv_window; // Remote receiver's buffer size (their)
    SndWinSize      cong_window; // Local receiver's buffer size  (my)
    TcpWindow       slowstart_threshold;
    TxBufPtr        app;
    ap_uint<2>      count;
    bool            fastRetransmitted;
    bool            finReady;
    bool            finSent;
    TxSarEntry() {};
};

// TSt / Query from RXe
//----------------------
class RXeTxSarQuery {
  public:
    SessionId       sessionID;
    TxAckNum        ackd;         // TX'ed and ACK'ed
    RcvWinSize      recv_window;  // Remote receiver's buffer size (their)
    SndWinSize      cong_window;  // Local receiver's buffer size  (my)
    ap_uint<2>      count;
    CmdBool         fastRetransmitted;
    RdWrBit         write;
    RXeTxSarQuery () {}
    RXeTxSarQuery(SessionId id) : // Read Query
        sessionID(id), ackd(0), recv_window(0), count(0), fastRetransmitted(false), write(0) {}
    RXeTxSarQuery(SessionId id, TxAckNum ackd, RcvWinSize recv_win, SndWinSize cong_win, ap_uint<2> count, CmdBool fastRetransmitted) : // Write Query
        sessionID(id), ackd(ackd), recv_window(recv_win), cong_window(cong_win), count(count), fastRetransmitted(fastRetransmitted), write(1) {}
};

// TSt / Reply to RXe
//--------------------
class RXeTxSarReply {
  public:
    TxAckNum        prevAck;
    TxAckNum        nextByte;
    TcpWindow       cong_window;
    ap_uint<16>     slowstart_threshold;
    ap_uint<2>      count;
    CmdBool         fastRetransmitted;
    RXeTxSarReply() {}
    RXeTxSarReply(TxAckNum ack, TxAckNum next, TcpWindow cong_win, ap_uint<16> sstresh, ap_uint<2> count, CmdBool fastRetransmitted) :
        prevAck(ack), nextByte(next), cong_window(cong_win), slowstart_threshold(sstresh), count(count), fastRetransmitted(fastRetransmitted) {}
};

// TSt / Query from TXe
//----------------------
class TXeTxSarQuery {
  public:
    SessionId       sessionID;
    TxAckNum        not_ackd;   // TX'ed but not ACK'ed
    RdWrBit         write;
    CmdBit          init;
    bool            finReady;
    bool            finSent;
    bool            isRtQuery;
    TXeTxSarQuery() {}
    TXeTxSarQuery(SessionId id) :
        sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write) :
        sessionID(id), not_ackd(not_ackd), write(write), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init, bool finReady, bool finSent) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init, bool finReady, bool finSent, bool isRt) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(isRt) {}
};

// TSt / Reply to TXe
//--------------------
class TXeTxSarReply {
  public:
	TxAckNum        ackd;       // ACK'ed
	TxAckNum        not_ackd;   // TX'ed but not ACK'ed
    TcpWindow       min_window; // Min(cong_window, recv_window)
    TxBufPtr        app;        // Written by APP
    bool            finReady;
    bool            finSent;
    TXeTxSarReply() {}
    TXeTxSarReply(ap_uint<32> ack, ap_uint<32> nack, ap_uint<16> min_window, ap_uint<16> app, bool finReady, bool finSent) :
        ackd(ack), not_ackd(nack), min_window(min_window), app(app), finReady(finReady), finSent(finSent) {}
};

// TSt / Re-transmission Query from TXe
//--------------------------------------
class TXeTxSarRtQuery : public TXeTxSarQuery
{
  public:
    TXeTxSarRtQuery() {}
    TXeTxSarRtQuery(const TXeTxSarQuery& q) :
        TXeTxSarQuery(q.sessionID, q.not_ackd, q.write, q.init, q.finReady, q.finSent, q.isRtQuery) {}
    TXeTxSarRtQuery(SessionId id, ap_uint<16> ssthresh) :
        TXeTxSarQuery(id, ssthresh, 1, 0, false, false, true) {}
    ap_uint<16> getThreshold() {
        return not_ackd(15, 0);
    }
};

// TSt / Tx Application Interface
//--------------------------------
class TAiTxSarPush {
  public:
    SessionId       sessionID;
    TxBufPtr        app;
    TAiTxSarPush() {}
    TAiTxSarPush(SessionId id, TxBufPtr app) :
         sessionID(id), app(app) {}
};

// TSt / Command from TSt
//------------------------
class TStTxSarPush {
  public:
    SessionId       sessionID;
    ap_uint<16>     ackd;
#if (TCP_NODELAY)
    ap_uint<16> min_window;
#endif
    CmdBit          init;
    TStTxSarPush() {}
#if !(TCP_NODELAY)
    TStTxSarPush(SessionId id, ap_uint<16> ackd) :
        sessionID(id), ackd(ackd), init(0) {}
    TStTxSarPush(SessionId id, ap_uint<16> ackd, CmdBit init) :
        sessionID(id), ackd(ackd), init(init) {}
#else
    TStTxSarPush(SessionId id, ap_uint<16> ackd, ap_uint<16> min_window) :
        sessionID(id), ackd(ackd), min_window(min_window), init(0) {}
    TStTxSarPush(SessionId id, ap_uint<16> ackd, ap_uint<16> min_window, CmdBit init) :
        sessionID(id), ackd(ackd), min_window(min_window), init(init) {}
#endif
};


/********************************************
 * Tx Application Interface (TAi)
 ********************************************/

//-- TAI / Tx Application Table (Tat) Request
/*** OBSOLETE-20190616 ************
struct txAppTxSarQuery
{
    SessionId   sessionID;
    ap_uint<16> mempt;
    bool        write;
    txAppTxSarQuery() {}
    txAppTxSarQuery(SessionId id)
            :sessionID(id), mempt(0), write(false) {}
    txAppTxSarQuery(SessionId id, ap_uint<16> pt)
            :sessionID(id), mempt(pt), write(true) {}
};
***********************************/
class TxAppTableRequest {
  public:
    SessionId   sessId;
    ap_uint<16> mempt;
    bool        write;
    TxAppTableRequest() {}
    TxAppTableRequest(SessionId id) :
        sessId(id), mempt(0), write(false) {}
    TxAppTableRequest(SessionId id, ap_uint<16> pt) :
        sessId(id), mempt(pt), write(true) {}
};

//-- TAI / Tx Application Table (Tat) Reply
/*** OBSOLETE-20190616 ******
struct txAppTxSarReply
{
	SessionId   sessionID;
    ap_uint<16> ackd;
    ap_uint<16> mempt;
    txAppTxSarReply() {}
    txAppTxSarReply(SessionId id, ap_uint<16> ackd, ap_uint<16> pt) :
        sessionID(id), ackd(ackd), mempt(pt) {}
};
*******************************/
class TxAppTableReply {
  public:
    SessionId   sessId;
    ap_uint<16> ackd;
    TxBufPtr    mempt;
    #if (TCP_NODELAY)
      ap_uint<16> min_window;
    #endif
    TxAppTableReply() {}
    #if !(TCP_NODELAY)
      TxAppTableReply(SessionId id, ap_uint<16> ackd, TxBufPtr pt) :
           sessId(id), ackd(ackd), mempt(pt) {}
    #else
      TxAppTableReply(SessionId id, ap_uint<16> ackd, TxBufPtr pt, ap_uint<16> min_window) :
          sessionID(id), ackd(ackd), mempt(pt), min_window(min_window) {}
    #endif
};


/********************************************
 * Timers (TIm)
 ********************************************/
enum TimerCmd {LOAD_TIMER = false,
               STOP_TIMER = true};

// TIm / ReTransmit Timer Command from RXe
//-----------------------------------------
class RXeReTransTimerCmd {
  public:
    SessionId   sessionID;
    TimerCmd    command;  // { LOAD=false; STOP=true}
    RXeReTransTimerCmd() {}
    RXeReTransTimerCmd(SessionId id) :
        sessionID(id), command(STOP_TIMER) {}
    RXeReTransTimerCmd(SessionId id, TimerCmd cmd) :
        sessionID(id), command(cmd) {}
};

enum EventType {TX_EVENT, RT_EVENT, ACK_EVENT, SYN_EVENT,
                SYN_ACK_EVENT, FIN_EVENT, RST_EVENT, ACK_NODELAY_EVENT};

// TIm / ReTransmit Timer Command form TXe
//-----------------------------------------
class TXeReTransTimerCmd {
  public:
    SessionId   sessionID;
    EventType   type;
    TXeReTransTimerCmd() {}
    TXeReTransTimerCmd(SessionId id) :
        sessionID(id), type(RT_EVENT) {} // [FIXME - Why RT??]
    TXeReTransTimerCmd(SessionId id, EventType type) :
        sessionID(id), type(type) {}
};

/********************************************
 * Event Engine
 ********************************************/
struct event  // [TODO - Rename]
{
    eventType       type;
    SessionId       sessionID;
    ap_uint<16>     address;
    ap_uint<16>     length;
    ap_uint<3>      rt_count;  // [FIXME - Make this type configurable]
    event() {}
    event(eventType type, SessionId id) :
        type(type), sessionID(id), address(0), length(0), rt_count(0) {}
    event(eventType type, SessionId id, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(0), length(0), rt_count(rt_count) {}
    event(eventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len) :
        type(type), sessionID(id), address(addr), length(len), rt_count(0) {}
    event(eventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(addr), length(len), rt_count(rt_count) {}
};

struct extendedEvent : public event
{
    LeSocketPair  tuple;    // [FIXME - Consider renaming]
    extendedEvent() {}
    extendedEvent(const event& ev) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    extendedEvent(const event& ev, LeSocketPair tuple) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count), tuple(tuple) {}
};

struct rstEvent : public event
{
    rstEvent() {}
    rstEvent(const event& ev) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    rstEvent(RxSeqNum  seq) :                         //:event(RST, 0, false), seq(seq) {}
        event(RST, 0, seq(31, 16), seq(15, 0), 0) {}
    rstEvent(SessionId id, RxSeqNum seq) :
        event(RST, id, seq(31, 16), seq(15, 0), 1) {}   //:event(RST, id, true), seq(seq) {}
    rstEvent(SessionId id, RxSeqNum seq, bool hasSessionID) :
        event(RST, id, seq(31, 16), seq(15, 0), hasSessionID) {}  //:event(RST, id, hasSessionID), seq(seq) {}
    TxAckNum getAckNumb() {
        RxSeqNum seq;
        seq(31, 16) = address;
        seq(15, 0) = length;
        return seq;
    }
    bool hasSessionID() {
        return (rt_count != 0);
    }
};


/*******************************************************************
 * IP4 - Streaming Word Class Definition as Encoded by the MAC.
 *******************************************************************/
class Ip4overAxi: public AxiWord {

  public:
    Ip4overAxi() {}
    Ip4overAxi(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
      AxiWord(tdata, tkeep, tlast) {}

    // Set-Get the IP4 Version
    void setIp4Version(Ip4Version ver)          {                  tdata.range( 7,  4) = ver;             }
    Ip4Version getIp4Version()                  {           return tdata.range( 7,  4);                   }

    // Set-Get the IP4 Internet Header Length
    void      setIp4HdrLen(Ip4HdrLen ihl)       {                  tdata.range( 3,  0) = ihl;             }
    Ip4HdrLen getIp4HdrLen()                    {           return tdata.range( 3,  0);                   }
    // Set the IP4 Type of Service
    void setIp4ToS(Ip4ToS tos)                  {                  tdata.range(15,  8) = tos;             }

    // Set the IP4 Total Length
    void        setIp4TotalLen(Ip4TotalLen len) {                  tdata.range(31, 16) = swapWord(len);   }
    Ip4TotalLen getIp4TotalLen()                { return swapWord (tdata.range(31, 16));                  }
    //OBSOLETE LeIp4TotalLen getIp4TotLen() { return swapWord(tdata.range(31, 16)); }

    // Set the IP4 Identification
    void setIp4Ident(Ip4Ident id)               {                  tdata.range(47, 32) = swapWord(id);    }
    // Set the IP4 Fragment Offset
    void setIp4FragOff(Ip4FragOff offset)       {                  tdata.range(63, 56) = offset( 7, 0);
                                                                   tdata.range(52, 48) = offset(12, 8);   }
    // Set the IP4 Flags
    void setIp4Flags(Ip4Flags flags)            {                  tdata.range(55, 53) = flags;           }
    // Set the IP4 Time to Live
    void setIp4TtL(Ip4TtL ttl)                  {                  tdata.range( 7,  0) = ttl;             }
    // Set the IP4 Protocol
    void setIp4Prot(Ip4Prot prot)               {                  tdata.range(15,  8) = prot;            }
    // Set the IP4 Header Checksum
    void setIp4HdrCsum(Ip4HdrCsum csum)         {                  tdata.range(31, 16) = csum;            }

    // Set-Get the IP4 Source Address
    void          setIp4SrcAddr(Ip4Addr addr)   {                  tdata.range(63, 32) = swapDWord(addr); }
    Ip4Addr       getIp4SrcAddr()               { return swapDWord(tdata.range(63, 32));                  }
    LeIp4Addr   getLeIp4SrcAddr()               {           return tdata.range(63, 32);                   }

    // Set-Get the IP4 Destination Address
    void          setIp4DstAddr(Ip4Addr addr)   {                  tdata.range(31,  0) = swapDWord(addr); }
    Ip4Addr       getIp4DstAddr()               { return swapDWord(tdata.range(31,  0));                  }
    LeIp4Addr   getLeIp4DstAddr()               {           return tdata.range(31,  0);                   }

    // Set-Get the TCP Source Port
    void          setTcpSrcPort(TcpPort port)   {                  tdata.range(47, 32) = swapWord(port);  }
    TcpPort       getTcpSrcPort()               { return swapWord (tdata.range(47, 32));                  }
    LeTcpPort   getLeTcpSrcPort()               {           return tdata.range(47, 32) ;                  }

    // Set-Get the TCP Destination Port
    void          setTcpDstPort(TcpPort port)   {                  tdata.range(63, 48) = swapWord(port);  }
    TcpPort       getTcpDstPort()               { return swapWord (tdata.range(63, 48));                  }
    LeTcpPort   getLeTcpDstPort()               {           return tdata.range(63, 48);                   }

    // Set-Get the TCP Sequence Number
    void       setTcpSeqNum(TcpSeqNum num)      {                  tdata.range(31,  0) = swapDWord(num);  }
    TcpSeqNum  getTcpSeqNum()                   { return swapDWord(tdata.range(31,  0));                  }

    // Set-Get the TCP Acknowledgment Number
    void       setTcpAckNum(TcpAckNum num)      {                  tdata.range(63, 32) = swapDWord(num);  }
    TcpAckNum  getTcpAckNum()                   { return swapDWord(tdata.range(63, 32));                  }

    // Set-Get the TCP Data Offset
    void       setTcpDataOff(TcpDataOff offset) {                  tdata.range( 7,  4) = offset;          }
    TcpDataOff getTcpDataOff()                  { return           tdata.range( 7,  4);                   }

    // Set-Get the TCP Control Bits
    void setTcpCtrlFin(TcpCtrlBit bit)          {                  tdata.bit( 8) = bit;                   }
    TcpCtrlBit getTcpCtrlFin()                  {           return tdata.bit( 8);                         }
    void setTcpCtrlSyn(TcpCtrlBit bit)          {                  tdata.bit( 9) = bit;                   }
    TcpCtrlBit getTcpCtrlSyn()                  {           return tdata.bit( 9);                         }
    void setTcpCtrlRst(TcpCtrlBit bit)          {                  tdata.bit(10) = bit;                   }
    TcpCtrlBit getTcpCtrlRst()                  {           return tdata.bit(10);                         }
    void setTcpCtrlPsh(TcpCtrlBit bit)          {                  tdata.bit(11) = bit;                   }
    TcpCtrlBit getTcpCtrlPsh()                  {           return tdata.bit(11);                         }
    void setTcpCtrlAck(TcpCtrlBit bit)          {                  tdata.bit(12) = bit;                   }
    TcpCtrlBit getTcpCtrlAck()                  {           return tdata.bit(12);                         }
    void setTcpCtrlUrg(TcpCtrlBit bit)          {                  tdata.bit(13) = bit;                   }
    TcpCtrlBit getTcpCtrlUrg()                  {           return tdata.bit(13);                         }

    // Set-Get the TCP Window
    void        setTcpWindow(TcpWindow win)     {                  tdata.range(31, 16) = swapWord(win);   }
    TcpWindow   getTcpWindow()                  { return swapWord (tdata.range(31, 16));                  }

    // Set-Get the TCP Checksum
    void        setTcpChecksum(TcpChecksum csum){                  tdata.range(47, 32) = swapWord(csum);                   }
    TcpChecksum getTcpChecksum()                { return swapWord (tdata.range(47, 32));                  }

    // Set-Get the TCP Urgent Pointer
    void        setTcpUrgPtr(TcpUrgPtr ptr)     {                  tdata.range(63, 48) = swapWord(ptr);   }
    TcpUrgPtr   getTcpUrgPtr()                  { return swapWord (tdata.range(63, 48));                  }

    // Set-Get the TCP Options
    void        setTcpOptKind(TcpOptKind val)   {                  tdata.range( 7,  0);                   }
    TcpOptKind  getTcpOptKind()                 { return           tdata.range( 7,  0);                   }
    void        setTcpOptMss(TcpOptMss val)     {                  tdata.range(31, 16);                   }
    TcpOptMss   getTcpOptMss()                  { return swapWord (tdata.range(31, 16));                  }

    //OBSOLETE Copy Assignment from an AxiWord
    //OBSOLETE Ip4overAxi& operator= (const Ip4overAxi&);
    //OBSOLETE Ip4overAxi& operator= (const TcpWord&);

  private:
    // Swap the two bytes of a word (.i.e, 16 bits)
    ap_uint<16> swapWord(ap_uint<16> inpWord) {
      return (inpWord.range(7,0), inpWord(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits)
    ap_uint<32> swapDWord(ap_uint<32> inpDWord) {
      return (inpDWord.range( 7, 0), inpDWord(15,  8),
              inpDWord.range(23,16), inpDWord(31, 24));
    }

}; // End of: Ip4overAxi



/*******************************************************************
 * TCP - Streaming Word Class Definition as Encoded by the MAC.
 *******************************************************************/
//class TcpOverAxi : public Ip4overAxi {
//    public:
//        TcpOverAxi() {}
//        TcpOverAxi(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
//            Ip4overAxi(tdata, tkeep, tlast) {}
//}


/*************************************************************************
 * TCP ROLE INTERFACES
 *************************************************************************
 * Terminology & Conventions.
 *  [APP] stands for Application (this is also a synonym for ROLE).
 *************************************************************************/

/***********************************************
 * Application Notification
 *  Indicates that data are available for the
 *  application in the TCP Rx buffer.
 *
 * [FIXME: consider using member 'opened' instead
 *   of 'closed'.]
 * [FIXME: AppNotif could contain a sub-class
 *  'AppRdReq' and a sub-class "SocketPair'.]
 ***********************************************/
class AppNotif
{
  public:
    SessionId          sessionID;
    TcpSegLen          tcpSegLen;
    Ip4Addr            ip4SrcAddr;
    TcpPort            tcpSrcPort;
    TcpPort            tcpDstPort;
    bool               closed;
    AppNotif() {}
    AppNotif(SessionId  sessId,                      bool       closed) :
             sessionID( sessId), tcpSegLen( 0),      ip4SrcAddr(0),
             tcpSrcPort(0),      tcpDstPort(0),      closed(    closed) {}
    AppNotif(SessionId  sessId,  TcpSegLen  segLen,  Ip4Addr    sa,
             TcpPort    sp,      TcpPort    dp) :
             sessionID( sessId), tcpSegLen( segLen), ip4SrcAddr(sa),
             tcpSrcPort(sp),     tcpDstPort(dp),     closed(    false) {}
    AppNotif(SessionId  sessId,  TcpSegLen  segLen,  Ip4Addr    sa,
             TcpPort    sp,      TcpPort    dp,      bool       closed) :
             sessionID( sessId), tcpSegLen( segLen), ip4SrcAddr(sa),
             tcpSrcPort(sp),     tcpDstPort(dp),     closed(    closed) {}
};

/***********************************************
 * Application Read Request
 *  Used by the application for requesting to
 *  read data from the TCP Rx buffer.
 ***********************************************/
class AppRdReq
{
  public:
    SessionId   sessionID;
    TcpSegLen   length;
    AppRdReq() {}
    AppRdReq(SessionId id, TcpSegLen len) :
        sessionID(id), length(len) {}
};

struct appReadRequest
{
	SessionId   sessionID;
    //ap_uint<16> address;
    ap_uint<16> length;
    appReadRequest() {}
    appReadRequest(SessionId id, ap_uint<16> len) :
        sessionID(id), length(len) {}
};

/***********************************************
 * Application Write Status
 *  The status returned by TOE after a write
 *  data transfer.
 ***********************************************/
typedef ap_int<17>  AppWrSts;
//TODO: this definiton creates two streams...
//class AppWrSts
//{
//  public:
//    TcpSegLen    segLen;  // The #bytes written or an error code if status==0
//    StsBit       status;  // OK=1
//    AppWrSts() {}
//    AppWrSts(StsBit sts, TcpSegLen len) :
//        status(sts), segLen(len) {}
//};

/***********************************************
 * Application Data
 *  Data transfered between TOE and APP.
 ***********************************************/
typedef AxiWord     AppData;

/***********************************************
 * Application Metadata
 *  Meta-data transfered between TOE and APP.
 ***********************************************/
typedef TcpSessId   AppMeta;

/***********************************************
 * Application Open Request
 *  The socket address that the application
 *  wants to open.
 ***********************************************/
typedef LeSockAddr AppOpnReq;  //[FIXME - switch to NETWORK ORDER]

/***********************************************
 * Application Open Status [TODO - Rename to Reply]
 *  Information returned by TOE after an open
 *  connection request.
 ***********************************************/
typedef OpenStatus  AppOpnSts;

/***********************************************
 * Application Listen Request
 *  The TCP port that the application is willing
 *  to open for listening.
 ***********************************************/
typedef TcpPort     AppLsnReq;

/***********************************************
 * Application Listen Acknowledgment
 *  Acknowledge bit returned by TOE after a
 *  TCP listening port request.
 ***********************************************/
typedef AckBit      AppLsnAck;

////TODO fix parallel declaration
//class AppLsnAckAxis : public Axis<cSHL_TOE_LSN_ACK_WIDTH> {
//  public:
//    AppLsnAckAxis() {}
//    AppLsnAckAxis(AppLsnAck ack) :
//        Axis<cSHL_TOE_LSN_ACK_WIDTH>(ack) {}
//};

/***********************************************
 * Application Close Request
 *  The socket address that the application
 *  wants to open.
 ***********************************************/
typedef SessionId   AppClsReq;


template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so);



// ================ some functions ==========


void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out);
void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out);

ap_uint<32> bigEndianToInteger(ap_uint<8> *buffer, int lsb);
void integerToBigEndian(ap_uint<32> n, ap_uint<8> *bytes);

ap_uint<16> swapWord   (ap_uint<16> inpWord);       // [FIXME - To be replaced w/ byteSwap16]
ap_uint<16> byteSwap16 (ap_uint<16> inputVector);
ap_uint<32> swapDWord  (ap_uint<32> inpDWord);      // [FIXME - To be replaced w/ byteSwap32]
ap_uint<32> byteSwap32 (ap_uint<32> inputVector);

ap_uint<8>  lenToKeep  (ap_uint<4> noValidBytes);
ap_uint<8>  returnKeep (ap_uint<4> length);
ap_uint<4>  keepToLen  (ap_uint<8> keepValue);
ap_uint<4>  keepMapping(ap_uint<8> keepValue);

#endif


