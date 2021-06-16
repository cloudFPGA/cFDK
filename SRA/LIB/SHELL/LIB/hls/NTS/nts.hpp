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
 * @file     : nts.hpp
 * @brief    : Definition of the Network Transport Stack (NTS) component
 *             as if it was an HLS IP core.
 *
 * System:   : cloudFPGA
 * Component : Shell
 * Language    : Vivado HLS
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

#ifndef _NTS_H_
#define _NTS_H_

#include "nts_types.hpp"
#include "nts_config.hpp"

/*******************************************************************************
 * INTERFACE - GENERIC TCP & UDP APPLICATION INTERFACE (APP)
 *******************************************************************************/
typedef AxisRaw     AxisApp;

/*******************************************************************************
 * INTERFACE - TCP APPLICATION INTERFACE (TAIF)
 *******************************************************************************
 * This section defines the interfaces between the Network and Transport Stack
 * (NTS) and the TCP Application Interface (TAIF) layer.
 *******************************************************************************/

//=========================================================
//== TAIF / RECEIVED & TRANSMITTED SEGMENT INTERFACES
//=========================================================

#ifndef _AXIS_CLASS_DEFINED_
//---------------------------------------------------------
//-- TCP APP - DATA
//--  The data section of a TCP segment over an AXI4S I/F.
//---------------------------------------------------------
typedef AxisApp     TcpAppData;
#endif

//---------------------------------------------------------
//-- TCP APP - METADATA
//--  The session identifier to send.
//---------------------------------------------------------
typedef TcpSessId   TcpAppMeta;

//---------------------------------------------------------
//-- TCP APP - DATA LENGTH
//--  The length of the data to send.
//---------------------------------------------------------
typedef TcpDatLen   TcpAppDLen;

//---------------------------------------------------------
//-- TCP APP - NOTIFICATION
//--  Notifies the availability of data for the application
//--  in the TCP Rx buffer.
//---------------------------------------------------------
class TcpAppNotif {
  public:
    SessionId          sessionID;
    TcpDatLen          tcpDatLen;
    Ip4Addr            ip4SrcAddr;
    TcpPort            tcpSrcPort;
    TcpPort            tcpDstPort;
    TcpState           tcpState;
    TcpAppNotif() {}
    TcpAppNotif(SessionId  sessId,              TcpState  tcpState) :
        sessionID( sessId), tcpDatLen( 0),      ip4SrcAddr(0),
        tcpSrcPort(0),      tcpDstPort(0),      tcpState(tcpState) {}
    TcpAppNotif(SessionId  sessId,  TcpDatLen   dataLen,  Ip4Addr    sa,
        TcpPort    sp,      TcpPort    dp) :
        sessionID( sessId), tcpDatLen(dataLen), ip4SrcAddr(sa),
        tcpSrcPort(sp),     tcpDstPort(dp),     tcpState(CLOSED) {}
    TcpAppNotif(SessionId  sessId,  TcpDatLen   dataLen,  Ip4Addr    sa,
        TcpPort    sp,      TcpPort    dp,      TcpState tcpState) :
        sessionID( sessId), tcpDatLen(dataLen), ip4SrcAddr(sa),
        tcpSrcPort(sp),     tcpDstPort(dp),     tcpState(tcpState) {}
};

//---------------------------------------------------------
//-- TCP APP - DATA READ REQUEST
//--  Used by the application to request data from the
//--  TCP Rx buffer.
//---------------------------------------------------------
class TcpAppRdReq {
  public:
    SessionId   sessionID;
    TcpDatLen   length;
    TcpAppRdReq() {}
    TcpAppRdReq(SessionId id,  TcpDatLen len) :
        sessionID(id), length(len) {}
};

//---------------------------------------------------------
//-- TCP APP - DATA SEND REQUEST
//--  Used by the application to request data transmission.
//---------------------------------------------------------
class TcpAppSndReq {
  public:
    SessionId   sessId;
    TcpDatLen   length;
    TcpAppSndReq() {}
    TcpAppSndReq(SessionId id, TcpDatLen len) :
        sessId(id), length(len) {}
};

//---------------------------------------------------------
//-- TCP APP - DATA SEND REPLY
//--  Status returned by NTS to APP after request to send.
//---------------------------------------------------------
class TcpAppSndRep {
public:
    SessionId    sessId;    // The session ID
    TcpDatLen    length;    // The #bytes that were requested to be written
    TcpDatLen    spaceLeft; // The remaining space in the TCP buffer
    TcpAppSndErr error;     // The error code (OK=0)
    TcpAppSndRep() {}
    TcpAppSndRep(SessionId sessId, TcpDatLen datLen, TcpDatLen space, TcpAppSndErr rc) :
        sessId(sessId), length(datLen), spaceLeft(space), error(rc) {}
};

//=========================================================
//== TAIF / OPEN & CLOSE CONNECTION INTERFACES
//=========================================================

//--------------------------------------------------------
//-- TCP APP - OPEN CONNECTION REQUEST
//--  The socket address to be opened.
//--------------------------------------------------------
typedef SockAddr        TcpAppOpnReq;

//--------------------------------------------------------
//-- TCP APP - OPEN CONNECTION REPLY
//--  Reports the state of a TCP connection according to RFC-793.
//--------------------------------------------------------
class TcpAppOpnRep {
  public:
    SessionId   sessId;
    TcpState    tcpState;
    TcpAppOpnRep() {}
    TcpAppOpnRep(SessionId sessId, TcpState tcpState) :
        sessId(sessId), tcpState(tcpState) {}
};

//--------------------------------------------------------
//-- TCP APP - CLOSE CONNECTION REQUEST
//--  The socket address to be closed.
//--  [FIXME-What about creating a class 'AppConReq' with a member 'opn/cls']
//--------------------------------------------------------
typedef SessionId       TcpAppClsReq;

//=========================================================
//== TAIF / LISTEN PORT INTERFACES
//=========================================================

//---------------------------------------------------------
//-- TCP APP - LISTEN REQUEST
//--  The TCP port to open for listening.
//--  [FIXME-What about creating a class 'AppLsnReq' with a member 'start/stop']
//---------------------------------------------------------
typedef TcpPort     TcpAppLsnReq;

//---------------------------------------------------------
//-- TCP APP - LISTEN REPLY
//--  The port status returned by NTS upon listen request.
//---------------------------------------------------------
typedef RepBool     TcpAppLsnRep;


/*******************************************************************************
 * INTERFACE - UDP APPLICATION INTERFACE (UAIF)
 *******************************************************************************
 * This section defines the interfaces between the Network and Transport Stack
 * (NTS) and the UDP Application Interface (UAIF) layer.
 *******************************************************************************/

//=========================================================
//== UAIF / RECEIVED & TRANSMITTED DATAGRAM INTERFACES
//=========================================================

#ifndef _AXIS_CLASS_DEFINED_
#define _AXIS_CLASS_DEFINED_
//---------------------------------------------------------
//-- UDP APP - DATA
//--  The data section of an UDP datagram over an AXI4S I/F.
//---------------------------------------------------------
typedef AxisRaw     UdpAppData;
#endif

//OBSOLETE_20210604 //---------------------------------------------------------
//OBSOLETE_20210604 //-- UDP APP - METADATA
//OBSOLETE_20210604 //--  The socket pair association of a connection.
//OBSOLETE_20210604 //---------------------------------------------------------
//OBSOLETE_20210604 typedef SocketPair  UdpAppMeta;

//---------------------------------------------------------
//-- UDP APP - DATA LENGTH
//--  The length of the datagram.
//---------------------------------------------------------
typedef UdpDatLen   UdpAppDLen;

//---------------------------------------------------------
//-- UDP APP - METADATA
//--  The meta-data of a UDP connection.
//--
//--  [INFO] Do not use struct 'SocketPair' here because
//--   'DATA_PACK' optimization does not support packing
//--   structs which contain other structs.
//---------------------------------------------------------
class UdpAppMeta {
  public:
    Ip4Addr     ip4SrcAddr; // IPv4 source address in NETWORK BYTE ORDER
    Ly4Port     udpSrcPort; // UDP  source port    in NETWORK BYTE ORDER
    Ip4Addr     ip4DstAddr; // IPv4 destination address in NETWORK BYTE ORDER
    Ly4Port     udpDstPort; // UDP  destination port    in NETWORK BYTE ORDER
    UdpAppMeta() {}
    UdpAppMeta(Ip4Addr srcAddr, Ly4Port srcPort, Ip4Addr dstAddr, Ly4Port dstPort) :
        ip4SrcAddr(srcAddr), udpSrcPort(srcPort), ip4DstAddr(dstAddr), udpDstPort(dstPort) {}
    UdpAppMeta(SockAddr srcSock, SockAddr dstSock) :
        ip4SrcAddr(srcSock.addr), udpSrcPort(srcSock.port), ip4DstAddr(dstSock.addr), udpDstPort(dstSock.port) {}
    UdpAppMeta(SocketPair sockPair) :
            ip4SrcAddr(sockPair.src.addr), udpSrcPort(sockPair.src.port), ip4DstAddr(sockPair.dst.addr), udpDstPort(sockPair.dst.port) {}
};

//=========================================================
//== UAIF / OPEN & CLOSE PORT INTERFACES
//=========================================================

//---------------------------------------------------------
//-- UDP APP - LISTEN REQUEST
//--  The UDP port to open for listening.
//---------------------------------------------------------
typedef Ly4Port     UdpAppLsnReq;

//---------------------------------------------------------
//-- UDP APP - LISTEN REPLY
//--  The port status returned by NTS upon listen request.
//---------------------------------------------------------
typedef StsBool     UdpAppLsnRep;

//--------------------------------------------------------
//-- UDP APP - CLOSE PORT REQUEST
//--  The listen port to close.
//--------------------------------------------------------
typedef Ly4Port     UdpAppClsReq; // [FIXME-What about creating a class 'AppLsnReq' with a member 'start/stop']

//--------------------------------------------------------
//-- UDP APP - CLOSE PORT REPLY
//--  Reports the status of the port closing.
//--------------------------------------------------------
typedef StsBool     UdpAppClsRep;


/*******************************************************************************
 *
 * ENTITY - NETWORK TRANSPORT STACK (NTS)
 *
 *******************************************************************************/
void nts(

        //------------------------------------------------------
        //-- TAIF / Received Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>     &soTAIF_Notif,
        stream<TcpAppRdReq>     &siTAIF_DReq,
        stream<TcpAppData>      &soTAIF_Data,
        stream<TcpAppMeta>      &soTAIF_Meta,

        //------------------------------------------------------
        //-- TAIF / Listen Port Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>    &siTAIF_LsnReq,
        stream<TcpAppLsnRep>    &soTAIF_LsnRep,

        //------------------------------------------------------
        //-- TAIF / Transmit Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppData>      &siTAIF_Data,
        stream<TcpAppSndReq>    &siTAIF_SndReq,
        stream<TcpAppSndRep>    &soTAIF_SndRep,

        //------------------------------------------------------
        //-- TAIF / Open Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>    &siTAIF_OpnReq,
        stream<TcpAppOpnRep>    &soTAIF_OpnRep,

        //------------------------------------------------------
        //-- TAIF / Close Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>    &siTAIF_ClsReq,
        //-- Not USed           &soTAIF_ClsSts,

        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>    &siUAIF_LsnReq,
        stream<UdpAppLsnRep>    &soUAIF_LsnRep,
        stream<UdpAppClsReq>    &siUAIF_ClsReq,
        stream<UdpAppClsRep>    &soUAIF_ClsRep,

        //------------------------------------------------------
        //-- UAIF / Received Datagram Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soUAIF_Data,
        stream<UdpAppMeta>      &soUAIF_Meta,

        //------------------------------------------------------
        //-- UAIF / Transmit Datatagram Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen

);

#endif

/*! \} */


