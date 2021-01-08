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
 * @file       : toe.hpp
 * @brief      : TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP offload engine.
 *
 * @remarks   :
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network.
 *  A PDU is therefore composed of a protocol specific control information
 *   (e.g a header) and a user data section.
 *  This source code uses the following terminology:
 *   - a SEGMENT (or TCP Packet) refers to the TCP protocol data unit.
 *   - a PACKET  (or IP  Packet) refers to the IP protocol data unit.
 *   - a FRAME   (or MAC Frame)  refers to the Ethernet data link layer.
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_H_
#define _TOE_H_

#include <stdint.h>
#include "ap_int.h"

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../../MEM/mem.hpp"

using namespace hls;

/*******************************************************************************
 * CONSTANTS DERIVED FROM THE NTS CONFIGURATION FILE
 *******************************************************************************/

//-- The Maximum Segment Size (MSS) that can be received and processed by TOE
//--  FYI: MSS is rounded modulo 8 bytes for better efficiency.
static const TcpSegLen MY_MSS = (MTU - IP4_HEADER_LEN - TCP_HEADER_LEN) & ~0x7; // 1456

//-- The Maximum Segment Size (MSS) that can be transmitted by TOE
//--  FYI: This MSS is advertised by the remote host during the 3-may handshake.
static const TcpSegLen THEIR_MSS = ZYC2_MSS; // 1352


/*******************************************************************************
 * DEFINITIONS
 ******************************************************************************/
#define TOE_SIZEOF_LISTEN_PORT_TABLE    0x8000
#define TOE_SIZEOF_ACTIVE_PORT_TABLE    0x8000
#define TOE_FIRST_EPHEMERAL_PORT_NUM    0x8000 // Dynamic ports are in the range 32768..65535

#define TOE_FEATURE_USED_FOR_DEBUGGING  0


extern uint32_t      packetCounter;  // [FIXME] Remove
extern uint32_t      idleCycCnt;     // [FIXME] Remove
extern unsigned int  gSimCycCnt;     // [FIXME] Remove

//OBSOLETE_20210104 #define OOO_N 4     // number of OOO blocks accepted
//OBSOLETE_20210104 #define OOO_W 4288  // window {max(offset + length)} of sequence numbers beyond recvd accepted
//OBSOLETE_20210104 static const int OOO_N_BITS = 3;        // bits required to represent OOO_N+1, need 0 to show no OOO blocks are valid
//OBSOLETE_20210104 static const int OOO_W_BITS = 13;       // bits required to represent OOO_W
//OBSOLETE_20210104 static const int OOO_L_BITS = 13;       // max length in bits of OOO blocks allowed

//OBSOLETE_20210104 static const ap_uint<32> SEQ_mid = 2147483648; // used in Modulo Arithmetic Comparison 2^(32-1) of sequence numbers etc.

#ifndef __SYNTHESIS__
  // HowTo - You should adjust the value of 'TIME_1s' such that the testbench
  //   works with your longest segment. In other words, if 'TIME_1s' is too short
  //   and/or your segment is too long, you may experience retransmission events
  //   (RT) which will break the test. You may want to use 'siIPRX_OneSeg.dat' or
  //   'siIPRX_TwoSeg.dat' to tune this parameter.
  static const ap_uint<32> TIME_1s        =   250;

  static const ap_uint<32> TIME_1us       = (((ap_uint<32>)(TIME_1s/1000000) > 1) ? (ap_uint<32>)(TIME_1s/1000000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_64us      = (((ap_uint<32>)(TIME_1s/  15625) > 1) ? (ap_uint<32>)(TIME_1s/  15625) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_128us     = (((ap_uint<32>)(TIME_1s/  31250) > 1) ? (ap_uint<32>)(TIME_1s/  31250) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_256us     = (((ap_uint<32>)(TIME_1s/  62500) > 1) ? (ap_uint<32>)(TIME_1s/  62500) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_512us     = (((ap_uint<32>)(TIME_1s/ 125000) > 1) ? (ap_uint<32>)(TIME_1s/ 125000) : (ap_uint<32>)1);

  static const ap_uint<32> ACKD_16us      = ( 16.0/0.0064/TOE_MAX_SESSIONS/100) + 1;
  static const ap_uint<32> ACKD_32us      = ( 32.0/0.0064/TOE_MAX_SESSIONS/100) + 1;
  static const ap_uint<32> ACKD_64us      = ( 64.0/0.0064/TOE_MAX_SESSIONS/100) + 1;

  static const ap_uint<32> TIME_1ms       = (((ap_uint<32>)(TIME_1s/1000) > 1) ? (ap_uint<32>)(TIME_1s/1000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_5ms       = (((ap_uint<32>)(TIME_1s/ 200) > 1) ? (ap_uint<32>)(TIME_1s/ 200) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_25ms      = (((ap_uint<32>)(TIME_1s/  40) > 1) ? (ap_uint<32>)(TIME_1s/  40) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_50ms      = (((ap_uint<32>)(TIME_1s/  20) > 1) ? (ap_uint<32>)(TIME_1s/  20) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_100ms     = (((ap_uint<32>)(TIME_1s/  10) > 1) ? (ap_uint<32>)(TIME_1s/  10) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_250ms     = (((ap_uint<32>)(TIME_1s/   4) > 1) ? (ap_uint<32>)(TIME_1s/   4) : (ap_uint<32>)1);

  static const ap_uint<32> TIME_5s        = (  5*TIME_1s);
  static const ap_uint<32> TIME_7s        = (  7*TIME_1s);
  static const ap_uint<32> TIME_10s       = ( 10*TIME_1s);
  static const ap_uint<32> TIME_15s       = ( 15*TIME_1s);
  static const ap_uint<32> TIME_20s       = ( 20*TIME_1s);
  static const ap_uint<32> TIME_30s       = ( 30*TIME_1s);
  static const ap_uint<32> TIME_60s       = ( 60*TIME_1s);
  static const ap_uint<32> TIME_120s      = (120*TIME_1s);
#else
  static const ap_uint<32> TIME_1us       = (  1.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> ACKD_16us      = ( 16.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> ACKD_32us      = ( 32.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> ACKD_64us      = ( 64.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_128us     = (128.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_256us     = (256.0/0.0064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_512us     = (512.0/0.0064/TOE_MAX_SESSIONS) + 1;

  static const ap_uint<32> TIME_1ms       = (  1.0/0.0000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5ms       = (  5.0/0.0000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_25ms      = ( 25.0/0.0000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_50ms      = ( 50.0/0.0000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_100ms     = (100.0/0.0000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_250ms     = (250.0/0.0000064/TOE_MAX_SESSIONS) + 1;

  static const ap_uint<32> TIME_1s        = (  1.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5s        = (  5.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_7s        = (  7.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_10s       = ( 10.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_15s       = ( 15.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_20s       = ( 20.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_30s       = ( 30.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_60s       = ( 60.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_120s      = (120.0/0.0000000064/TOE_MAX_SESSIONS) + 1;
#endif

  //OBSOLETE_20210104 #define BROADCASTCHANNELS 2


/*******************************************************************************
 * GLOBAL DEFINES and GENERIC TYPES
 *******************************************************************************/
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

/*******************************************************************************
 * GENERAL ENUMERATIONS
 *
 *  WARNING ABOUT ENUMERATIONS:
 *   Avoid using 'enum' for boolean variables because scoped enums are only
 *   available with -std=c++.
 *    E.g.: enum PortState : bool { CLOSED_PORT=false, OPENED_PORT=true };
 *******************************************************************************/

enum notificationType {PKG, CLOSE, TIME_OUT, RESET};
enum { WORD_0,  WORD_1,   WORD_2,  WORD_3,  WORD_4,  WORD_5 };
enum { CHUNK_0, CHUNK_1, CHUNK_2, CHUNK_3, CHUNK_4, CHUNK_5 };


#define TLAST       1

/*********************************************************
 * AXIS TYPE FIELDS DEFINITION
 *   FYI - 'LE' stands for Little-Endian order.
 *********************************************************/
typedef ap_uint<64> LE_tData;  // [FIXME] Can be removed
typedef ap_uint< 8> LE_tKeep;
typedef ap_uint<64> tData;
typedef ap_uint<32> tDataHalf;
typedef ap_uint< 8> tKeep;
typedef ap_uint< 1> tLast;



/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY TOE
 *******************************************************************************
 * Terminology & Conventions
 * - .
 * - .
 *******************************************************************************/

//---------------------------------------------------------
//-- TOE - EVENT TYPES
//---------------------------------------------------------
enum EventType { TX_EVENT=0,    RT_EVENT,  ACK_EVENT, SYN_EVENT, \
                 SYN_ACK_EVENT, FIN_EVENT, RST_EVENT, ACK_NODELAY_EVENT };

//---------------------------------------------------------
//-- TOE - SESSION STATE
//---------------------------------------------------------
typedef TcpAppOpnRep SessState;  // TCP state according to RFC-793

//---------------------------------------------------------
//-- TOE - TCP PORT RANGES (Static & Ephemeral)
//---------------------------------------------------------
typedef ap_uint<15> TcpStaPort;  // TCP Static  Port [0x0000..0x7FFF]
typedef ap_uint<15> TcpDynPort;  // TCP Dynamic Port [0x8000..0xFFFF]

//---------------------------------------------------------
//-- TOE - Some Rx & Tx SAR Types
//---------------------------------------------------------
typedef TcpSeqNum   RxSeqNum;   // A sequence number received from the network layer
typedef TcpAckNum   TxAckNum;   // An acknowledge number transmitted to the network layer
typedef TcpWindow   RcvWinSize; // A received window size
typedef TcpWindow   SndWinSize; // A sending  window size

typedef ap_uint<32>              RxMemPtr;  // A pointer to RxMemBuff ( 4GB)  [FIXME <33>]
typedef ap_uint<32>              TxMemPtr;  // A pointer to TxMemBuff ( 4GB)  [FIXME <33>]
typedef ap_uint<TOE_WINDOW_BITS> TcpBufAdr; // A TCP buffer address   (64KB)
typedef TcpBufAdr                RxBufPtr;  // A pointer to RxSessBuf (64KB)
typedef TcpBufAdr                TxBufPtr;  // A pointer to TxSessBuf (64KB)

//---------------------------------------------------------
//--  SOCKET ADDRESS (alias ipTuple)
//---------------------------------------------------------
struct ipTuple // [TODO] - Replace w/ SockAddr
{
    ap_uint<32>     ip_address;
    ap_uint<16>     ip_port;
};

/*******************************************************************************
 * INTERFACE TYPES and CLASSES USED BY SESSION LOOKUP CONTROLLER (SLc)
 *******************************************************************************/

typedef bool HitState;
enum         HitStates { SESSION_UNKNOWN = false, SESSION_EXISTS = true};
//OBSOLETE_20200703 typedef SessionId   TcpSessId;  // TCP Session ID

//=========================================================
//== SLc - Session Lookup Query
//=========================================================
class SessionLookupQuery {
  public:
    LE_SocketPair  tuple;   // [FIXME - Name and type]
    bool           allowCreation;
    SessionLookupQuery() {}
    SessionLookupQuery(LE_SocketPair tuple, bool allowCreation) :
        tuple(tuple), allowCreation(allowCreation) {}
};

//=========================================================
//== SLc - Session Lookup Reply
//=========================================================
class SessionLookupReply {
  public:
    SessionId   sessionID;
    HitState    hit;
    SessionLookupReply() {}
    SessionLookupReply(SessionId id, HitState hit) :
        sessionID(id), hit(hit) {}
};

/*******************************************************************************
 * INTERFACE TYPES and CLASSES USED BY STATE TABLE (STt)
 *******************************************************************************/

//=========================================================
//== STt - Session State Query
//=========================================================
class StateQuery {
  public:
    SessionId       sessionID;
    TcpState        state;
    RdWrBit         write;
    StateQuery() {}
    StateQuery(SessionId id) :
        sessionID(id), state(CLOSED), write(QUERY_RD) {}
    StateQuery(SessionId id, TcpState state, RdWrBit write) :
        sessionID(id), state(state), write(write) {}
};

//=========================================================
//== RSt / Generic Reply
//=========================================================
class RxSarReply {
  public:
    RxBufPtr    appd;
    RxSeqNum    rcvd;
    RxSeqNum    oooHead;

    StsBool     gap;
    RxSarReply() {}
    RxSarReply(RxBufPtr appd, RxSeqNum rcvd, StsBool gap, RxSeqNum oooHead) :
        appd(appd), rcvd(rcvd), gap(gap), oooHead(oooHead) {}
};

//=========================================================
//== RSt / Query from RXe
//=========================================================
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

//=========================================================
//== RSt / Query from RAi
//=========================================================
class RAiRxSarQuery {
  public:
    SessionId       sessionID;
    RxBufPtr        appd;  // Next byte to be consumed by [APP]
    RdWrBit         write;
    RAiRxSarQuery() {}
    RAiRxSarQuery(SessionId id) :
        sessionID(id), appd(0), write(0) {}
    RAiRxSarQuery(SessionId id, RxBufPtr appd) :
        sessionID(id), appd(appd), write(1) {}
};

//=========================================================
//== RSt / Reply to RAi
//=========================================================
class RAiRxSarReply {
  public:
    SessionId       sessionID;
    RxBufPtr        appd;  // Next byte to be consumed by [APP]
    RAiRxSarReply() {}
    RAiRxSarReply(SessionId id, RxBufPtr appd) :
        sessionID(id), appd(appd) {}
};

/*******************************************************************************
 * Tx SAR Table (TSt)
 *******************************************************************************/

//=========================================================
//== TSt / Query from RXe
//=========================================================
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

//=========================================================
//== TSt / Reply to RXe
//=========================================================
class RXeTxSarReply {
  public:
    TxAckNum        prevAckd;  // Bytes TX'ed and ACK'ed
    TxAckNum        prevUnak;  // Bytes TX'ed but not ACK'ed
    TcpWindow       cong_window;
    TcpWindow       slowstart_threshold;
    ap_uint<2>      count;
    CmdBool         fastRetransmitted;
    RXeTxSarReply() {}
    RXeTxSarReply(TxAckNum ackd, TxAckNum unak, TcpWindow cong_win, TcpWindow sstresh, ap_uint<2> count, CmdBool fastRetransmitted) :
        prevAckd(ackd), prevUnak(unak), cong_window(cong_win), slowstart_threshold(sstresh), count(count), fastRetransmitted(fastRetransmitted) {}
};

//=========================================================
//== TSt / Query from TXe
//=========================================================
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

//=========================================================
//== TSt / Reply to TXe
//=========================================================
class TXeTxSarReply {
  public:
    TxAckNum        ackd;       // TX'ed and ACK'ed
    TxAckNum        not_ackd;   // TX'ed but not ACK'ed
    TcpWindow       min_window; // Min(cong_window, recv_window)
    TxBufPtr        app;        // Written by APP
    bool            finReady;
    bool            finSent;
    TXeTxSarReply() {}
    TXeTxSarReply(ap_uint<32> ack, ap_uint<32> nack, ap_uint<16> min_window, ap_uint<16> app, bool finReady, bool finSent) :
        ackd(ack), not_ackd(nack), min_window(min_window), app(app), finReady(finReady), finSent(finSent) {}
};

//=========================================================
//== TSt / Re-transmission Query from TXe
//=========================================================
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

//=========================================================
//== TSt / Tx Application Interface
//=========================================================
class TAiTxSarPush {
  public:
    SessionId       sessionID;
    TxBufPtr        app;
    TAiTxSarPush() {}
    TAiTxSarPush(SessionId id, TxBufPtr app) :
         sessionID(id), app(app) {}
};

//=========================================================
//== TSt / Command from TSt
//=========================================================
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

/*******************************************************************************
 * Tx Application Interface (TAi)
 *******************************************************************************/

//=========================================================
//== TAI / Tx Application Table (Tat) Request
//=========================================================
class TxAppTableQuery {
  public:
    SessionId   sessId;
    TxBufPtr    mempt;
    bool        write;
    TxAppTableQuery() {}
    TxAppTableQuery(SessionId id) :
        sessId(id), mempt(0), write(false) {}
    TxAppTableQuery(SessionId id, ap_uint<16> pt) :
        sessId(id), mempt(pt), write(true) {}
};

//=========================================================
//== TAI / Tx Application Table (Tat) Reply
//=========================================================
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

/*******************************************************************************
 * Timers (TIm)
 *******************************************************************************/
enum TimerCmd {LOAD_TIMER = false,
               STOP_TIMER = true};

//=========================================================
//== TIm / ReTransmit Timer Command from RXe
//=========================================================
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

//=========================================================
//== TIm / ReTransmit Timer Command form TXe
//=========================================================
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

/*******************************************************************************
 * Event Engine (EVe)
 *******************************************************************************/

//=========================================================
//== EVe / Event
//=========================================================
class Event
{
  public:
    EventType       type;
    SessionId       sessionID;
    TcpBufAdr       address;
    TcpSegLen       length;
    ap_uint<3>      rt_count;  // [FIXME - Make this type configurable]
    Event() {}
    Event(EventType type, SessionId id) :
        type(type), sessionID(id), address(0), length(0), rt_count(0) {}
    Event(EventType type, SessionId id, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(0), length(0), rt_count(rt_count) {}
    Event(EventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len) :
        type(type), sessionID(id), address(addr), length(len), rt_count(0) {}
    Event(EventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(addr), length(len), rt_count(rt_count) {}
};

//=========================================================
//== EVe / ExtendedEvent
//=========================================================
class ExtendedEvent : public Event
{
  public:
    LE_SocketPair  tuple;    // [FIXME - Rename and change type]
    ExtendedEvent() {}
    ExtendedEvent(const Event& ev) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    ExtendedEvent(const Event& ev, LE_SocketPair tuple) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count), tuple(tuple) {}
};

//=========================================================
//== EVe / RstEvent  // [FIXME- TODO]
//=========================================================
struct rstEvent : public Event  // [FIXME - Class naming convention]
{
    rstEvent() {}
    rstEvent(const Event& ev) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    rstEvent(RxSeqNum  seq) :              //:Event(RST, 0, false), seq(seq) {}
        Event(RST_EVENT, 0, seq(31, 16), seq(15, 0), 0) {}
    rstEvent(SessionId id, RxSeqNum seq) :
        Event(RST_EVENT, id, seq(31, 16), seq(15, 0), 1) {}   //:Event(RST, id, true), seq(seq) {}
    rstEvent(SessionId id, RxSeqNum seq, bool hasSessionID) :
        Event(RST_EVENT, id, seq(31, 16), seq(15, 0), hasSessionID) {}  //:Event(RST, id, hasSessionID), seq(seq) {}
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


/*******************************************************************************
 * DDR MEMORY SUB-SYSTEM INTERFACES
 *******************************************************************************
 * Terminology & Conventions (see Xilinx LogiCORE PG022).
 *  [DM]  stands for AXI Data Mover
 *  [DRE] stands for Data Realignment Engine.
 *******************************************************************************/

#define RXMEMBUF    65536   // 64KB = 2^16
#define TXMEMBUF    65536   // 64KB = 2^16

//=========================================================
//== MUX / A 2-to-1 Stream multiplexer.
//=========================================================
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so);

/*******************************************************************************
 *
 * ENTITY - TCP OFFLOAD ENGINE (TOE)
 *
 *******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr                                  piMMIO_IpAddr,

        //------------------------------------------------------
        //-- NTS Interfaces
        //------------------------------------------------------
        StsBit                                  &poNTS_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                         &siIPRX_Data,

        //------------------------------------------------------
        //-- IPTX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                         &soIPTX_Data,

        //------------------------------------------------------
        //-- TAIF / Receive Data Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>                     &soTAIF_Notif,
        stream<TcpAppRdReq>                     &siTAIF_DReq,
        stream<TcpAppData>                      &soTAIF_Data,
        stream<TcpAppMeta>                      &soTAIF_Meta,

        //------------------------------------------------------
        //-- TAIF / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>                    &siTAIF_LsnReq,
        stream<TcpAppLsnRep>                    &soTAIF_LsnRep,

        //------------------------------------------------------
        //-- TAIF / Send Data Interfaces
        //------------------------------------------------------
        stream<TcpAppData>                      &siTAIF_Data,
        stream<TcpAppSndReq>                    &siTAIF_SndReq,
        stream<TcpAppSndRep>                    &soTAIF_SndRep,

        //------------------------------------------------------
        //-- TAIF / Open Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>                    &siTAIF_OpnReq,
        stream<TcpAppOpnRep>                    &soTAIF_OpnRep,

        //------------------------------------------------------
        //-- TAIF / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>                    &siTAIF_ClsReq,
        //-- Not Used                           &soTAIF_ClsSts,

        //------------------------------------------------------
        //-- MEM / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_RxP_RdSts,
        stream<DmCmd>                           &soMEM_RxP_RdCmd,
        stream<AxisApp>                         &siMEM_RxP_Data,
        stream<DmSts>                           &siMEM_RxP_WrSts,
        stream<DmCmd>                           &soMEM_RxP_WrCmd,
        stream<AxisApp>                         &soMEM_RxP_Data,

        //------------------------------------------------------
        //-- MEM / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_TxP_RdSts,
        stream<DmCmd>                           &soMEM_TxP_RdCmd,
        stream<AxisApp>                         &siMEM_TxP_Data,
        stream<DmSts>                           &siMEM_TxP_WrSts,
        stream<DmCmd>                           &soMEM_TxP_WrCmd,
        stream<AxisApp>                         &soMEM_TxP_Data,

        //------------------------------------------------------
        //-- CAM / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<CamSessionLookupRequest>         &soCAM_SssLkpReq,
        stream<CamSessionLookupReply>           &siCAM_SssLkpRpl,
        stream<CamSessionUpdateRequest>         &soCAM_SssUpdReq,
        stream<CamSessionUpdateReply>           &siCAM_SssUpdRpl,

        //------------------------------------------------------
        //-- DEBUG / Interfaces
        //------------------------------------------------------
        //-- DEBUG / Session Statistics Interfaces
        ap_uint<16>                             &poDBG_SssRelCnt,
        ap_uint<16>                             &poDBG_SssRegCnt
        #if TOE_FEATURE_USED_FOR_DEBUGGING
        //-- DEBUG / SimCycCounter
        ap_uint<32>                        &poSimCycCount
        #endif
);

#endif

/*! \} */
