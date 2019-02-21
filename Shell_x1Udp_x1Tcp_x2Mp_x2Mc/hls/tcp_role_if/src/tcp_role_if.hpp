/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface.
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
 *                   TCP-Role interface.
 *
 *****************************************************************************/

//OBSOLETE-20181010 #include "../../toe/src/toe.hpp"

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


#define NR_SESSION_ENTRIES 4


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



/*************************************************************************
 * NETWORK LAYER-3 SECTION
 *************************************************************************
 * Terminology & Conventions
 * - a PACKET (or IpPacket) refers to an IP-PDU (i.e., Header+Data).
 *************************************************************************/

typedef ap_uint<32> Ip4Addr;    // IP4 fixed 32-bit length address


/*************************************************************************
 * NETWORK LAYER-4 SECTION
 *************************************************************************
 * Terminology & Conventions
 * - a SEGMENT (or TcpPacket) refers to a  TCP-PDU (i.e., Header+Data).
 * - a DATAGRAM (or UdpPacket) refers to a UDP-PDU (i.e., Header+Data).
 *************************************************************************/

/********************************************
 * TCP - Header Type Definitions
 ********************************************/


/********************************************
 * TCP - Specific Type Definitions
 ********************************************/
typedef ap_uint<16> TcpSegLen;  // TCP Segment Length in octets (same as Ip4DatLen)
typedef ap_uint< 8> TcpHdrLen;  // TCP Header  Length in octets
typedef ap_uint<16> TcpDatLen;  // TCP Data    Length in octets (same as TcpSegLen minus TcpHdrLen)

typedef ap_uint<16>     TcpPort;    // TCP Port Number


/********************************************
 * Generic TCP Type Definitions
 ********************************************/
typedef ap_uint<16> TcpSessId;  // TCP Session ID
typedef ap_uint<4>  TcpBuffId;  // TCP buffer  ID


/********************************************
 * Session Id Table Entry
 ********************************************/
struct SessionIdCamEntry {
    TcpSessId   sessionId;
    TcpBuffId   bufferId;
    ap_uint<1>  valid;

    SessionIdCamEntry() {}
    SessionIdCamEntry(ap_uint<16> session_id, ap_uint<4> buffer_id, ap_uint<1> valid) :
        sessionId(session_id), bufferId(buffer_id), valid(valid){}
};


/********************************************
 * Session Id CAM
 ********************************************/
class SessionIdCam {
    public:
        SessionIdCamEntry cam[NR_SESSION_ENTRIES];
        SessionIdCam();
        bool        write(SessionIdCamEntry wrEntry);
        TcpSessId   search(TcpBuffId        buffId);
        TcpBuffId   search(TcpSessId        sessId);
};


/********************************************
 * Socket Transport Address.
 ********************************************/
struct SockAddr {   // Socket Address
    Ip4Addr     addr;   // IPv4 address in reversed network byte order !!!
    TcpPort     port;   // Port in reversed network byte order !!!
    SockAddr() {}
    SockAddr(Ip4Addr addr, TcpPort port) :
        addr(addr), port(port) {}
};

struct SocketPair {     // Socket Pair Association
    SockAddr    src;    // Source socket address
    SockAddr    dst;    // Destination socket address
    SocketPair() {}
    SocketPair(SockAddr src, SockAddr dst) :
        src(src), dst(dst) {}
};


/********************************************
 * TCP Specific Streaming Interfaces.
 ********************************************/
typedef AxiWord     TcpWord;

typedef TcpSessId   TcpMeta;    // TCP MetaData

struct TcpOpnSts {              // TCP Open Status
    TcpSessId   sessionID;
    bool        success;
    TcpOpnSts() {}
    TcpOpnSts(TcpSessId id, bool success) :
        sessionID(id), success(success) {}
};

typedef SockAddr    TcpOpnReq;  // TCP Open Request

struct TcpNotif {               // TCP Notification
    TcpSessId       sessionID;
    ap_uint<16>     length;
    ap_uint<32>     ipAddress;
    ap_uint<16>     dstPort;
    bool            closed;
    TcpNotif() {}

    TcpNotif(TcpSessId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port) :
        sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(false) {}

    TcpNotif(TcpSessId id, bool closed) :
        sessionID(id), length(0), ipAddress(0),  dstPort(0), closed(closed) {}

    TcpNotif(TcpSessId id, ap_uint<32> addr, ap_uint<16> port, bool closed) :
        sessionID(id), length(0), ipAddress(addr),  dstPort(port), closed(closed) {}

    TcpNotif(TcpSessId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port, bool closed) :
        sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(closed) {}
};

struct TcpRdReq {               // TCP Read Request
    TcpSessId   sessionID;
    ap_uint<16> length;
    TcpRdReq() {}

    TcpRdReq(TcpSessId id, ap_uint<16> len) :
        sessionID(id), length(len) {}
};

typedef bool        TcpLsnAck;  // TCP Listen Acknowledge

typedef ap_uint<16> TcpLsnReq;  // TCP Listen Request

typedef ap_int<17>  TcpDSts;    // TCP Data Status

typedef ap_uint<16> TcpClsReq;  // TCP Close Request



void tcp_role_if(

        //------------------------------------------------------
        //-- ROLE / This / Rx Data Interface
        //------------------------------------------------------
        stream<AxiWord>     &siROL_This_Data,

        //------------------------------------------------------
        //-- ROLE / This / Tx Data Interface
        //------------------------------------------------------
        stream<AxiWord>     &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- TOE / This / Rx Data Interfaces
        //------------------------------------------------------
        stream<TcpNotif>    &siTOE_This_Notif,
        stream<AxiWord>     &siTOE_This_Data,
        stream<TcpMeta>         &siTOE_This_Meta,
        stream<TcpRdReq>    &soTHIS_Toe_DReq,

        //------------------------------------------------------
        //-- TOE / This / Rx Ctrl Interfaces
        //------------------------------------------------------
        stream<TcpLsnAck>   &siTOE_This_LsnAck,
        stream<TcpLsnReq>   &soTHIS_Toe_LsnReq,

        //------------------------------------------------------
        //-- TOE / This / Tx Data Interfaces
        //------------------------------------------------------
        stream<TcpDSts>     &siTOE_This_DSts,
        stream<AxiWord>     &soTHIS_Toe_Data,
        stream<TcpMeta>         &soTHIS_Toe_Meta,

        //------------------------------------------------------
        //-- TOE / This / Tx Ctrl Interfaces
        //------------------------------------------------------
        stream<TcpOpnSts>   &siTOE_This_OpnSts,
        stream<TcpOpnReq>   &soTHIS_Toe_OpnReq,
        stream<TcpClsReq>   &soTHIS_Toe_ClsReq

);


