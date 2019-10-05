/************************************************
Copyright (c) 2015, Xilinx, Inc.
Copyright (c) 2016-2019, IBM Research.

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
 * @file       : rx_engine.hpp
 * @brief      : Rx Engine (RXe) of the TCP Offload Engine (TOE).
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
 *               TCP Rx Engine.
 *
 *****************************************************************************/

#include "../../toe.hpp"
#include "../../toe_utils.hpp"

using namespace hls;


/********************************************
 * RXe - MetaData Interface
 ********************************************/
class RXeMeta {
  public:
    TcpSeqNum   seqNumb;    // TCP Sequence Number
    TcpAckNum   ackNumb;    // TCP Acknowledgment Number
    TcpWindow   winSize;    // TCP Window Size
    TcpSegLen   length;     // TCP Segment Length
    ap_uint<1>  ack;
    ap_uint<1>  rst;
    ap_uint<1>  syn;
    ap_uint<1>  fin;
    RXeMeta() {}
};

/*** OBSOLETE-20190822
struct rxEngineMetaData
{
    TcpSeqNum   seqNumb;    // TCP Sequence Number
    TcpAckNum   ackNumb;    // TCP Acknowledgment Number
    TcpWindow   winSize;    // TCP Window Size
    TcpSegLen   length;     // TCP Segment Length
    ap_uint<1>  ack;
    ap_uint<1>  rst;
    ap_uint<1>  syn;
    ap_uint<1>  fin;
    //ap_uint<16> dstPort;
};
***/

/********************************************
 * RXe - FsmMetaData Interface
 ********************************************/
class RXeFsmMeta {
  public:
    SessionId           sessionId;
    Ip4SrcAddr          ip4SrcAddr;
    TcpSrcPort          tcpSrcPort;
    TcpDstPort          tcpDstPort;
    RXeMeta             meta;
    RXeFsmMeta() {}
    RXeFsmMeta(SessionId sessId,  Ip4SrcAddr ipSA,  TcpSrcPort tcpSP,  TcpDstPort tcpDP,  RXeMeta rxeMeta) :
               sessionId(sessId), ip4SrcAddr(ipSA), tcpSrcPort(tcpSP), tcpDstPort(tcpDP),    meta(rxeMeta) {}
};

/*** OBSOLETE-20190822
struct rxFsmMetaData  // [TODO - Rename]
{
    SessionId           sessionId;
    Ip4SrcAddr          ip4SrcAddr;
    TcpDstPort          tcpDstPort;
    //OBSOLETE-20190822 rxEngineMetaData    meta; //check if all needed
    RXeMeta             meta;
    rxFsmMetaData() {}
    rxFsmMetaData(SessionId id, Ip4SrcAddr ipAddr, TcpDstPort tcpPort, RXeMeta meta) :
        sessionId(id), ip4SrcAddr(ipAddr), tcpDstPort(tcpPort), meta(meta) {}
};
***/

/*****************************************************************************
 * @brief   Main process of the TCP Rx Engine (RXe).
 *
 *****************************************************************************/
void rx_engine(
        stream<Ip4overMac>              &siIPRX_Pkt,
        stream<sessionLookupQuery>      &soSLc_SessLookupReq,
        stream<sessionLookupReply>      &siSLc_SessLookupRep,
        stream<StateQuery>              &soSTt_SessStateReq,
        stream<SessionState>            &siSTt_SessStateRep,
        stream<TcpPort>                 &soPRt_PortStateReq,
        stream<RepBit>                  &siPRt_PortStateRep,
        stream<RXeRxSarQuery>           &soRSt_RxSarQry,
        stream<RxSarEntry>              &siRSt_RxSarRep,
        stream<RXeTxSarQuery>           &soTSt_TxSarQry,
        stream<RXeTxSarReply>           &siTSt_TxSarRep,
        stream<RXeReTransTimerCmd>      &soTIm_ReTxTimerCmd,
        stream<ap_uint<16> >            &soTIm_ClearProbeTimer,
        stream<ap_uint<16> >            &soTIm_CloseTimer,
        stream<extendedEvent>           &soEVe_SetEvent,
        stream<OpenStatus>              &soTAi_SessOpnSts,
        stream<AppNotif>                &soRAi_RxNotif,
        stream<DmCmd>                   &soMEM_WrCmd,
        stream<AxiWord>                 &soMEM_WrData,
        stream<DmSts>                   &siMEM_WrSts
);
