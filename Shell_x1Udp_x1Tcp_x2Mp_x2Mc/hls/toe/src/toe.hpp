/*****************************************************************************
 * @file       : toe.hpp
 * @brief      : TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP offload engine.
 *
 * @terminology:
 *      In telecommunications, a protocol data unit (PDU) is a single unit of
 *       information transmitted among peer entities of a computer network.
 *      A PDU is therefore composed of a protocol specific control information
 *       (e.g, a header) and a user data section.
 *  This source code uses the following terminology:
 *   - a SEGMENT (or TCP Packet) refers to the TCP protocol data unit.
 *   - a DATAGRAM (or UDP Packet) refers to the UDP protocol data unit.
 *   - a PACKET (or IP Packet) refers to the IP protocol data unit.
 *   - a FRAME (or MAC Frame) refers to the Ethernet data link layer.
 *
 *****************************************************************************/

#ifndef TOE_H_
#define TOE_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <vector>

static const uint16_t MAX_SESSIONS = 32;

//OBSOLETE #include "session_lookup_controller/session_lookup_controller.hpp"

#define noTxSessions 1 // Number of Tx Sessions to open for testing

extern uint32_t packetCounter;
extern uint32_t idleCycCnt;
extern unsigned int     gSimCycCnt;
// Forward declarations.
struct rtlSessionUpdateRequest;
struct rtlSessionUpdateReply;
struct rtlSessionLookupReply;
struct rtlSessionLookupRequest;


#define OOO_N 4     // number of OOO blocks accepted
#define OOO_W 4288  // window {max(offset + length)} of sequence numbers beyond recvd accepted

//usually tcp segment size is 1460. Here, we use 1456 to support tcp options
static const ap_uint<16> MMS = 1456; //536;
static const ap_uint<16> MY_MSS = 576;

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

#ifndef __SYNTHESIS__
static const ap_uint<32> TIME_1ms       = 1;
static const ap_uint<32> TIME_5ms       = 1;
static const ap_uint<32> TIME_25ms      = 1;
static const ap_uint<32> TIME_50ms      = 1;
static const ap_uint<32> TIME_100ms     = 1;
static const ap_uint<32> TIME_250ms     = 1;
static const ap_uint<32> TIME_1s        = 1;
static const ap_uint<32> TIME_5s        = 1;
static const ap_uint<32> TIME_7s        = 2;
static const ap_uint<32> TIME_10s       = 3;
static const ap_uint<32> TIME_15s       = 4;
static const ap_uint<32> TIME_20s       = 5;
static const ap_uint<32> TIME_30s       = 6;
static const ap_uint<32> TIME_60s       = 1;
static const ap_uint<32> TIME_120s      = 120;
#else
static const ap_uint<32> TIME_1ms       = 15;
static const ap_uint<32> TIME_5ms       = 75;
static const ap_uint<32> TIME_25ms      = 375;
static const ap_uint<32> TIME_50ms      = 750;
static const ap_uint<32> TIME_100ms     = 1515;
static const ap_uint<32> TIME_250ms     = 3780;
static const ap_uint<32> TIME_1s        = 15150;
static const ap_uint<32> TIME_5s        = 75075;
static const ap_uint<32> TIME_7s        = 105105;
static const ap_uint<32> TIME_10s       = 150150;
static const ap_uint<32> TIME_15s       = 225225;
static const ap_uint<32> TIME_20s       = 300300;
static const ap_uint<32> TIME_30s       = 450450;
static const ap_uint<32> TIME_60s       = 900900;
static const ap_uint<32> TIME_120s      = 1801801;
#endif

#define BROADCASTCHANNELS 2
enum eventType {TX, RT, ACK, SYN, SYN_ACK, FIN, RST, ACK_NODELAY};

/*
 * There is no explicit LISTEN state
 * CLOSE-WAIT state is not used, since the FIN is sent out immediately after we receive a FIN, the application is simply notified
 * FIN_WAIT_2 is also unused
 * LISTEN is merged into CLOSED
 */
enum sessionState {CLOSED, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, CLOSING, TIME_WAIT, LAST_ACK};
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
 * STREAM - Generic Stream Type Definitions
 *************************************************************************/

/********************************************
 * SINGLE BIT - Types and Definitions
 ********************************************/
typedef bool AckBit;    // Acknowledge: Always has to go back to the source of the request (e.g. OpenReq/OpenAck).
typedef bool CmdBit;    // Command    : Indicates an action (e.g. Drop). Does not expect a return from recipient.
typedef bool QryBit;    // Query      : Indicates a demand for an answer.
typedef bool ReqBit;    // Request    : Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
typedef bool RepBit;    // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
typedef bool RspBit;    // Response   : Can go back to a different source.
typedef bool StsBit;    // Status bit : Does not  have to go back (e.g. isOpen).
typedef bool ValBit;    // Valid bit  : Must go along with something to validate/invalidate.


/********************************************
 * AXIS - Generic AXI4-Streaming Interface
 ********************************************/
struct AxiWord {    // AXI4-Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>     tdata;
    ap_uint<8>      tkeep;
    ap_uint<1>      tlast;
    AxiWord()       {}
    AxiWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
            tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

struct axiWord {
    ap_uint<64>     data;
    ap_uint<8>      keep;
    ap_uint<1>      last;
    axiWord() {}
    axiWord(ap_uint<64>      data, ap_uint<8> keep, ap_uint<1> last) :
            data(data), keep(keep), last(last) {}
};


/*************************************************************************
 * NETWORK LAYER-2 SECTION
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
 *     order used to transmit bytes over the media. As an exemple, consider
 *     the 16-bit field "Total Length" of an IPv4 packet and let us assume
 *     that this length is 40. This field will be transmitted on the media
 *     in big-endian order as '0x00' followed by '0x28'. Finally, the same
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

/*************************************************************************
 * IPv4 - Header Type Definitions in AXI4-Stream Order (Little-Endian)
 *************************************************************************/
typedef ap_uint< 4> AxiIp4Version;     // IPv4 Version over Axi
typedef ap_uint< 4> AxiIp4HdrLen;      // IPv4 Internet Header Length over Axi
typedef ap_uint< 8> AxiIp4ToS;         // IPv4 Type of Service over Axi
typedef ap_uint<16> AxiIp4TotalLen;    // IPv4 Total Length over Axi
typedef ap_uint<32> AxiIp4SrcAddr;     // IPv4 Source Address over Axi
typedef ap_uint<32> AxiIp4DstAddr;     // IPv4 Destination Address over Axi
typedef ap_uint<32> AxiIp4Address;     // IPv4 Source or Destination Address over Axi
typedef ap_uint<32> AxiIp4Addr;        // IPv4 Source or Destination Address over Axi

/*************************************************************************
 * TCP - Header Type Definitions in Axi4-Stream Order (Little-Endian)
 *************************************************************************/
typedef ap_uint<16> AxiTcpSrcPort;     // TCP Source Port over Axi
typedef ap_uint<16> AxiTcpDstPort;     // TCP Destination Port over Axi
typedef ap_uint<16> AxiTcpPort;        // TCP Source or Destination Port over Axi
typedef ap_uint<32> AxiTcpSeqNum;      // TCP Sequence Number over Axi
typedef ap_uint<32> AxiTcpAckNum;      // TCP Acknowledgment Number over Axi
typedef ap_uint<4>  AxiTcpDataOff;     // TCP Data Offset over Axi
typedef ap_uint<6>  AxiTcpCtrlBits;    // TCP Control Bits over Axi
typedef ap_uint<16> AxiTcpWindow;      // TCP Window over Axi
typedef ap_uint<16> AxiTcpChecksum;    // TCP Checksum
typedef ap_uint<16> AxiTcpUrgPtr;      // TCP Urgent Pointer over Axi


/*************************************************************************
 * NETWORK LAYER-3 SECTION
 *************************************************************************
 * Terminology & Conventions
 * - a PACKET (or IpPacket) refers to an IP-PDU (i.e., Header+Data).
 *************************************************************************/

/******************************************************
 * IP4 - Internal Type Definitions (as used by the CPU)
 ******************************************************/
typedef ap_uint< 4> Ip4Version;     // IP4 Version
typedef ap_uint< 8> Ip4HdrLen;      // IP4 Header Length in octets (same as 4*Ip4HeaderLen)
typedef ap_uint< 8> Ip4ToS;         // IP4 Type of Service
typedef ap_uint<16> Ip4TotalLen;    // IP4 Total  Length
typedef ap_uint<32> Ip4SrcAddr;     // IP4 Source Address
typedef ap_uint<32> Ip4DstAddr;     // IP4 Destination Address
typedef ap_uint<32> Ip4Address;     // IP4 Source or Destination Address
typedef ap_uint<32> Ip4Addr;        // IP4 Source or Destination Address

typedef ap_uint<16> Ip4PktLen;      // IP4 Packet Length in octets (same as Ip4TotalLen)
typedef ap_uint<16> Ip4DatLen;      // IP4 Data   Length in octets (same as Ip4PktLen minus Ip4HdrLen)


/********************************************
 * IP4 - Streaming Type Definition
 ********************************************/
typedef AxiWord     Ip4Word;




/*************************************************************************
 * NETWORK LAYER-4 SECTION
 *************************************************************************
 * Terminology & Conventions
 * - a SEGMENT (or TcpPacket) refers to a TCP-PDU (i.e., Header+Data).
 * - a DATAGRAM (or UdpPacket) refrers to a UDP-PDU (i.e., Header+Data).
 *************************************************************************/

/******************************************************
 * TCP - Computer Type Definitions (in Little-Endian)
 ******************************************************/
typedef ap_uint<16> TcpSrcPort;     // TCP Source Port
typedef ap_uint<16> TcpDstPort;     // TCP Destination Port
typedef ap_uint<16> TcpPort;        // TCP Source or Destination Port Number
typedef ap_uint<32> TcpSeqNum;      // TCP Sequence Number
typedef ap_uint<32> TcpAckNum;      // TCP Acknowledge Number
typedef ap_uint<4>  TcpDataOff;     // TCP Data Offset
typedef ap_uint<6>  TcpCtrlBits;    // TCP Control Bits
typedef ap_uint<16> TcpWindow;      // TCP Window
typedef ap_uint<16> TcpChecksum;    // TCP Checksum
typedef ap_uint<16> TcpCSum;        // TCP Checksum
typedef ap_uint<16> TcpUrgPtr;      // TCP Urgent Pointer

typedef ap_uint<16> TcpSegLen;      // TCP Segment Length in octets (same as Ip4DatLen)
typedef ap_uint< 8> TcpHdrLen;      // TCP Header  Length in octets
typedef ap_uint<16> TcpDatLen;      // TCP Data    Length in octets (same as TcpSegLen minus TcpHdrLen)


/********************************************
 * Socket Transport Pair & Address
 ********************************************/

struct SockAddr {   // Socket Address stored in LITTLE-ENDIAN order !!!
    AxiIp4Address   addr;   // IPv4 address in LITTLE-ENDIAN order !!!
    AxiTcpPort      port;   // TCP  port in in LITTLE-ENDIAN order !!!
    SockAddr() {}
    SockAddr(AxiIp4Address addr, AxiTcpPort port) :
        addr(addr), port(port) {}
};

struct SocketPair { // Socket Pair Association in LITTLE-ENDIAN order !!!
    SockAddr    src;    // Source socket address
    SockAddr    dst;    // Destination socket address
    SocketPair() {}
    SocketPair(SockAddr src, SockAddr dst) :
        src(src), dst(dst) {}
};


inline bool operator < (SocketPair const &s1, SocketPair const &s2) {
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



typedef ap_uint<16> TcpSessId;  // TCP Session ID
typedef ap_uint<4>  TcpBuffId;  // TCP buffer  ID


/********************************************
 * TCP - Streaming Type Definition
 ********************************************/
typedef AxiWord         TcpWord;



struct ipTuple
{
    ap_uint<32>     ip_address;
    ap_uint<16>     ip_port;
};



/********************************************
 * Session Lookup Controller (SLc)
 ********************************************/

typedef ap_uint<16>     SessionId;

struct sessionLookupQuery
{
    SocketPair  tuple;
    bool        allowCreation;
    sessionLookupQuery() {}
    sessionLookupQuery(SocketPair tuple, bool allowCreation) :
        tuple(tuple), allowCreation(allowCreation) {}
};

struct sessionLookupReply
{
	SessionId   sessionID;
    bool        hit;
    sessionLookupReply() {}
    sessionLookupReply(SessionId id, bool hit) :
        sessionID(id), hit(hit) {}
};

/********************************************
 * Port Table (PRt)
 ********************************************/
#define QUERY_RD  0
#define QUERY_WR  1

struct stateQuery
{
	SessionId       sessionID;
    sessionState    state;
    ap_uint<1>      write;
    stateQuery() {}
    stateQuery(SessionId id) :
        sessionID(id), state(CLOSED), write(0) {}
    stateQuery(SessionId id, sessionState state, ap_uint<1> write) :
        sessionID(id), state(state), write(write) {}
};


/********************************************
 * Rx SAR Table (SRt)
 ********************************************/
struct rxSarEntry
{
    ap_uint<32> recvd;
    ap_uint<16> appd;
};

struct rxSarRecvd
{
	SessionId       sessionID;
    ap_uint<32>     recvd;
    ap_uint<1>      write;
    ap_uint<1>      init;
    rxSarRecvd() {}
    rxSarRecvd(SessionId id) :
        sessionID(id), recvd(0), write(0), init(0) {}
    rxSarRecvd(SessionId id, ap_uint<32> recvd, ap_uint<1> write) :
        sessionID(id), recvd(recvd), write(write), init(0) {}
    rxSarRecvd(SessionId id, ap_uint<32> recvd, ap_uint<1> write, ap_uint<1> init) :
        sessionID(id), recvd(recvd), write(write), init(init) {}
};

struct rxSarAppd
{
	SessionId       sessionID;
    ap_uint<16>     appd;
    // ap_uint<32> recvd; // for comparison with application data request - ensure appd + length < recvd
    ap_uint<1>  write;
    rxSarAppd() {}
    rxSarAppd(SessionId id) :
        sessionID(id), appd(0), write(0) {}
    rxSarAppd(SessionId id, ap_uint<16> appd) :
        sessionID(id), appd(appd), write(1) {}
};


/*struct rxSarOffset
{
    ap_uint<16> sessionID;
    ap_uint<16> offset[OOO_N];
    rxSarOffset() {}
    rxSarOffset(ap_uint<16> id, ap_uint<4> index, ap_uint<16> offset)
            :sessionID(id), {}


};

// Original rxSarRecvd, before OOO change
struct rxSarRecvd
{
    ap_uint<16> sessionID;
    ap_uint<32> recvd;
    ap_uint<1> write;
    ap_uint<1> init;
    rxSarRecvd() {}
    rxSarRecvd(ap_uint<16> id)
            :sessionID(id), recvd(0), write(0), init(0) {}
    rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write)
            :sessionID(id), recvd(recvd), write(write), init(0) {}
    rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write, ap_uint<1> init)
            :sessionID(id), recvd(recvd), write(write), init(init) {}
};*/

/********************************************
 * Tx SAR Table (TSt)
 ********************************************/
struct txSarEntry
{
    ap_uint<32> ackd;
    ap_uint<32> not_ackd;
    ap_uint<16> recv_window;
    ap_uint<16> cong_window;
    ap_uint<16> slowstart_threshold;
    ap_uint<16> app;
    ap_uint<2>  count;
    bool        finReady;
    bool        finSent;
};

struct rxTxSarQuery
{
	SessionId   sessionID;
    ap_uint<32> ackd;
    ap_uint<16> recv_window;
    ap_uint<16> cong_window;
    ap_uint<2>  count;
    ap_uint<1>  write;
    ap_uint<1>  init;
    rxTxSarQuery () {}
    rxTxSarQuery(SessionId id) :
        sessionID(id), ackd(0), recv_window(0), count(0), write(0) {}
    rxTxSarQuery(SessionId id, ap_uint<32> ackd, ap_uint<16> recv_win, ap_uint<16> cong_win, ap_uint<2> count, ap_uint<1> init) :
        sessionID(id), ackd(ackd), recv_window(recv_win), cong_window(cong_win), count(count), write(1), init(init) {}
};

struct txTxSarQuery
{
	SessionId   sessionID;
    ap_uint<32> not_ackd;
    ap_uint<1>  write;
    ap_uint<1>  init;
    bool        finReady;
    bool        finSent;
    bool        isRtQuery;
    txTxSarQuery() {}
    txTxSarQuery(SessionId id) :
        sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    //txTxSarQuery(ap_uint<16> id, ap_uint<1> lock)
    //          :sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    txTxSarQuery(SessionId id, ap_uint<32> not_ackd, ap_uint<1> write) :
        sessionID(id), not_ackd(not_ackd), write(write), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    txTxSarQuery(SessionId id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(false), finSent(false), isRtQuery(false) {}
    txTxSarQuery(SessionId id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init, bool finReady, bool finSent) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(false) {}
    txTxSarQuery(SessionId id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init, bool finReady, bool finSent, bool isRt) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(isRt) {}
};

struct txTxSarRtQuery : public txTxSarQuery
{
    txTxSarRtQuery() {}
    txTxSarRtQuery(const txTxSarQuery& q) :
        txTxSarQuery(q.sessionID, q.not_ackd, q.write, q.init, q.finReady, q.finSent, q.isRtQuery) {}
    txTxSarRtQuery(SessionId id, ap_uint<16> ssthresh) :
         txTxSarQuery(id, ssthresh, 1, 0, false, false, true) {}
    ap_uint<16> getThreshold() {
    	return not_ackd(15, 0);
    }
};


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

/********************************************
 * Rx SAR Table (RSt) Reply
 ********************************************/
struct rxTxSarReply
{
    TcpAckNum       prevAck;     //OBSOLETE-20181126 ap_uint<32>     prevAck;
    TcpAckNum       nextByte;    //OBSOLETE-20181126 ap_uint<32>     nextByte;
    TcpWindow       cong_window; //OBSOLETE-20181126  ap_uint<16>     cong_window;
    ap_uint<16>     slowstart_threshold;
    ap_uint<2>      count;
    rxTxSarReply() {}
    rxTxSarReply(ap_uint<32> ack, ap_uint<32> next, ap_uint<16> cong_win, ap_uint<16> sstresh, ap_uint<2> count) :
        prevAck(ack), nextByte(next), cong_window(cong_win), slowstart_threshold(sstresh), count(count) {}
};



struct txAppTxSarReply
{
	SessionId   sessionID;
    ap_uint<16> ackd;
    ap_uint<16> mempt;
    txAppTxSarReply() {}
    txAppTxSarReply(SessionId id, ap_uint<16> ackd, ap_uint<16> pt) :
        sessionID(id), ackd(ackd), mempt(pt) {}
};

struct txAppTxSarPush
{
	SessionId   sessionID;
    ap_uint<16> app;
    txAppTxSarPush() {}
    txAppTxSarPush(SessionId id, ap_uint<16> app) :
         sessionID(id), app(app) {}
};

struct txSarAckPush
{
	SessionId   sessionID;
    ap_uint<16> ackd;
    ap_uint<1>  init;
    txSarAckPush() {}
    txSarAckPush(SessionId id, ap_uint<16> ackd) :
        sessionID(id), ackd(ackd), init(0) {}
    txSarAckPush(SessionId id, ap_uint<16> ackd, ap_uint<1> init) :
        sessionID(id), ackd(ackd), init(init) {}
};

/********************************************
 * Tx SAR Table (TSt) Reply
 ********************************************/
struct txTxSarReply
{
	TcpAckNum       ackd;       //OBSOLETE ap_uint<32>  ackd;
	TcpAckNum       not_ackd;   //OBSOLETE ap_uint<32>  not_ackd;
    TcpWindow       min_window; //OBSOLETE ap_uint<16>  min_window;
    ap_uint<16>     app;
    bool            finReady;
    bool            finSent;
    txTxSarReply() {}
    txTxSarReply(ap_uint<32> ack, ap_uint<32> nack, ap_uint<16> min_window, ap_uint<16> app, bool finReady, bool finSent) :
        ackd(ack), not_ackd(nack), min_window(min_window), app(app), finReady(finReady), finSent(finSent) {}
};

struct rxRetransmitTimerUpdate {
	SessionId   sessionID;
    bool        stop;
    rxRetransmitTimerUpdate() {}
    rxRetransmitTimerUpdate(SessionId id) :
         sessionID(id), stop(0) {}
    rxRetransmitTimerUpdate(ap_uint<16> id, bool stop) :
        sessionID(id), stop(stop) {}
};

struct txRetransmitTimerSet {
	SessionId   sessionID;
    eventType   type;
    txRetransmitTimerSet() {}
    txRetransmitTimerSet(SessionId id) :
        sessionID(id), type(RT) {} //FIXME??
    txRetransmitTimerSet(SessionId id, eventType type) :
        sessionID(id), type(type) {}
};

/********************************************
 * Event Engine
 ********************************************/
struct event
{
    eventType       type;
    SessionId       sessionID;
    ap_uint<16>     address;
    ap_uint<16>     length;
    ap_uint<3>      rt_count;
    //bool          retransmit;
    event() {}
    //event(const event&) {}
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
    SocketPair  tuple;    // [FIXME - Considre renaming]
    extendedEvent() {}
    extendedEvent(const event& ev) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    extendedEvent(const event& ev, SocketPair tuple) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count), tuple(tuple) {}
};

struct rstEvent : public event
{
    rstEvent() {}
    rstEvent(const event& ev) :
        event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    rstEvent(ap_uint<32> seq) :                         //:event(RST, 0, false), seq(seq) {}
        event(RST, 0, seq(31, 16), seq(15, 0), 0) {}
    rstEvent(SessionId id, ap_uint<32> seq) :
        event(RST, id, seq(31, 16), seq(15, 0), 1) {}   //:event(RST, id, true), seq(seq) {}
    rstEvent(SessionId id, ap_uint<32> seq, bool hasSessionID) :
        event(RST, id, seq(31, 16), seq(15, 0), hasSessionID) {}  //:event(RST, id, hasSessionID), seq(seq) {}
    ap_uint<32> getAckNumb() {
        ap_uint<32> seq;
        seq(31, 16) = address;
        seq(15, 0) = length;
        return seq;
    }
    bool hasSessionID() {
        return (rt_count != 0);
    }
};








/*************************************************************************
 * EXTERNAL DRAM MEMORY INTERFACE SECTION
 *************************************************************************
 * Terminology & Conventions (see Xilinx LogiCORE PG022).
 *  [DM]  stands for AXI Data Mover
 *  [DRE] stands for Data Realignment Engine.
 *************************************************************************/

/********************************************
 * Data Mover Command Interface (c.f PG022)
 ********************************************/
struct DmCmd
{
    ap_uint<23>     bbt;    // Bytes To Transfer
    ap_uint<1>      type;   // Type of AXI4 access (0=FIXED, 1=INCR)
    ap_uint<6>      dsa;    // DRE Stream Alignment
    ap_uint<1>      eof;    // End of Frame
    ap_uint<1>      drr;    // DRE ReAlignment Request
    ap_uint<32>     saddr;  // Start Address
    ap_uint<4>      tag;    // Command Tag
    ap_uint<4>      rsvd;   // Reserved
    DmCmd() {}
    DmCmd(ap_uint<32> addr, ap_uint<16> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
};


struct mmCmd
{
    ap_uint<23> bbt;
    ap_uint<1>  type;
    ap_uint<6>  dsa;
    ap_uint<1>  eof;
    ap_uint<1>  drr;
    ap_uint<32> saddr;
    ap_uint<4>  tag;
    ap_uint<4>  rsvd;
    mmCmd() {}
    mmCmd(ap_uint<32> addr, ap_uint<16> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}

    /*mm_cmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<1> last)
            :bbt(len), type(1), dsa(0), eof(last), drr(1), saddr(addr), tag(0), rsvd(0) {}*/
    /*mm_cmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<4> dsa)
            :bbt(len), type(1), dsa(dsa), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}*/
};

/********************************************
 * Data Mover Status Interface (c.f PG022)
 ********************************************/
struct DmSts
{
    ap_uint<4>      tag;
    ap_uint<1>      interr;
    ap_uint<1>      decerr;
    ap_uint<1>      slverr;
    ap_uint<1>      okay;
    DmSts() {}
};


struct mmStatus
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
};




//TODO is this required??
struct mm_ibtt_status
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
    ap_uint<22> brc_vd;
    ap_uint<1>  eop;
};

struct openStatus
{
	SessionId   sessionID;
    bool        success;
    openStatus() {}
    openStatus(SessionId id, bool success) :
        sessionID(id), success(success) {}
};

struct appNotification
{
	SessionId          sessionID;
    ap_uint<16>        length;
    ap_uint<32>        ipAddress;
    ap_uint<16>        dstPort;
    bool               closed;
    appNotification() {}
    appNotification(SessionId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port) :
        sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(false) {}
    appNotification(SessionId id, bool closed) :
        sessionID(id), length(0), ipAddress(0),  dstPort(0), closed(closed) {}
    appNotification(SessionId id, ap_uint<32> addr, ap_uint<16> port, bool closed) :
        sessionID(id), length(0), ipAddress(addr),  dstPort(port), closed(closed) {}
    appNotification(SessionId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port, bool closed) :
        sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(closed) {}
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
//ap_uint<8> length2keep_mapping(uint16_t lengthValue);
ap_uint<8> returnKeep(ap_uint<4> length);
ap_uint<16> byteSwap16(ap_uint<16> inputVector);
ap_uint<32> byteSwap32(ap_uint<32> inputVector);
ap_uint<4> keepMapping(ap_uint<8> keepValue);       // This function counts the number of 1s in an 8-bit value

//template<class type> void streamBroadcaster (stream<type> &input, stream<type> &output1, stream<type> &output2);
//template<class type> void streamMerger(stream<type> &input1, stream<type>& input2, stream<type>& output);
template<typename T> void mergeFunction(stream<T>& in1, stream<T>& in2, stream<T>& out);

void toe(

        //------------------------------------------------------
        //-- From MMIO Interfaces
        //------------------------------------------------------
        ap_uint<32>                             piMMIO_This_IpAddr,


        //------------------------------------------------------
        //-- IPRX / This / IP Rx / Data Interface
        //------------------------------------------------------
        stream<Ip4Word>                         &siIPRX_This_Data,

        //------------------------------------------------------
        //-- L3MUX / This / IP Tx / Data Interface
        //------------------------------------------------------
        stream<Ip4Word>                         &soTHIS_L3mux_Data,

        //------------------------------------------------------
        //-- TRIF / This / ROLE Rx / Data Interfaces
        //------------------------------------------------------
        stream<appReadRequest>                  &siTRIF_This_DReq,
        stream<appNotification>                 &soTHIS_Trif_Notif,
        stream<axiWord>                         &soTHIS_Trif_Data,
        stream<ap_uint<16> >                    &soTHIS_Trif_Meta,

        //------------------------------------------------------
        //-- TRIF / This / ROLE Rx / Ctrl Interfaces
        //------------------------------------------------------
        stream<ap_uint<16> >                    &siTRIF_This_LsnReq,
        stream<bool>                            &soTHIS_Trif_LsnAck,

        //------------------------------------------------------
        //-- TRIF / This / ROLE Tx / Data Interfaces
        //------------------------------------------------------
        stream<axiWord>                         &siTRIF_This_Data,
        stream<ap_uint<16> >                    &siTRIF_This_Meta,
        stream<ap_int<17> >                     &soTHIS_Trif_DSts,

        //------------------------------------------------------
        //-- TRIF / This / Tx PATH / Ctrl Interfaces
        //------------------------------------------------------
        stream<ipTuple>                         &siTRIF_This_OpnReq,
        stream<openStatus>                      &soTHIS_Trif_OpnSts,
        stream<ap_uint<16> >                    &siTRIF_This_ClsReq,
        //-- Not USed                           &soTHIS_Trif_ClsSts,

        //------------------------------------------------------
        //-- MEM / This / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_This_RxP_RdSts,
        stream<mmCmd>                           &soTHIS_Mem_RxP_RdCmd,
        stream<axiWord>                         &siMEM_This_RxP_Data,
        stream<mmStatus>                        &siMEM_This_RxP_WrSts,
        stream<mmCmd>                           &soTHIS_Mem_RxP_WrCmd,
        stream<axiWord>                         &soTHIS_Mem_RxP_Data,

        //------------------------------------------------------
        //-- MEM / This / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_This_TxP_RdSts,
        stream<mmCmd>                           &soTHIS_Mem_TxP_RdCmd,
        stream<AxiWord>                         &siMEM_This_TxP_Data,
        stream<mmStatus>                        &siMEM_This_TxP_WrSts,
        stream<mmCmd>                           &soTHIS_Mem_TxP_WrCmd,
        stream<AxiWord>                         &soTHIS_Mem_TxP_Data,

        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<rtlSessionLookupReply>           &siCAM_This_SssLkpRpl,
        stream<rtlSessionUpdateReply>           &siCAM_This_SssUpdRpl,
        stream<rtlSessionLookupRequest>         &soTHIS_Cam_SssLkpReq,
        stream<rtlSessionUpdateRequest>         &soTHIS_Cam_SssUpdReq,

        //------------------------------------------------------
        //-- DEBUG / Session Statistics Interfaces
        //------------------------------------------------------
        ap_uint<16>                             &poTHIS_Dbg_SssRelCnt,
        ap_uint<16>                             &poTHIS_Dbg_SssRegCnt);

#endif
