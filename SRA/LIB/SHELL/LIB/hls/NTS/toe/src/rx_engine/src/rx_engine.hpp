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
 * @file       : rx_engine.hpp
 * @brief      : Rx Engine (RXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_RXE_H_
#define _TOE_RXE_H_

#include "../../../../../NTS/nts.hpp"
#include "../../../../../NTS/nts_utils.hpp"
#include "../../../../../NTS/SimNtsUtils.hpp"
#include "../../../../../NTS/toe/src/toe.hpp"
#include "../../../../../NTS/AxisIp4.hpp"
#include "../../../../../NTS/AxisTcp.hpp"
#include "../../../../../NTS/AxisPsd4.hpp"

using namespace hls;

//OBSOLETE_20200710 enum DropCmd { KEEP_CMD=false, DROP_CMD=true };

/********************************************
 * RXe - MetaData Interface
 ********************************************/
class RXeMeta {
  public:
    TcpSeqNum   seqNumb;    // TCP Sequence Number
    TcpAckNum   ackNumb;    // TCP Acknowledgment Number
    TcpWindow   winSize;    // TCP Window Size
    TcpSegLen   length;     // TCP Segment Length
    TcpCtrlBit  ack;
    TcpCtrlBit  rst;
    TcpCtrlBit  syn;
    TcpCtrlBit  fin;
    RXeMeta() {}
};

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

/*******************************************************************************
 * CONSTANTS FOR THE INTERNAL STREAM DEPTHS
 *******************************************************************************/
const int cDepth_FsmToEve_Event    =  4;
const int cDepth_FsmToMwr_WrCmd    = 16;
const int cDepth_FsmToRan_Notif    = 16;  // This depends on the memory delay
const int cDepth_FsmToTsd_DropCmd  = 16;

const int cDepth_MdhToEvm_Event    =  4;
const int cDepth_MdhToTsd_DropCmd  = 16;

const int cDepth_MwrToRan_SplitSeg = 16;

/*******************************************************************************
 *
 * @brief ENTITY - Rx Engine (RXe)
 *
 *******************************************************************************/
void rx_engine(
        //-- IP Rx Interface
        stream<AxisIp4>                 &siIPRX_Data,
        //-- Session Lookup Controller Interface
        stream<SessionLookupQuery>      &soSLc_SessLkReq,
        stream<SessionLookupReply>      &siSLc_SessLkRep,
        //-- State Table Interface
        stream<StateQuery>              &soSTt_StateQry,
        stream<TcpState>                &siSTt_StateRep,
        //-- Port Table Interface
        stream<TcpPort>                 &soPRt_PortStateReq,
        stream<RepBit>                  &siPRt_PortStateRep,
        //-- Rx SAR Table Interface
        stream<RXeRxSarQuery>           &soRSt_RxSarQry,
        stream<RxSarReply>              &siRSt_RxSarRep,
        //-- Tx SAR Table Interface
        stream<RXeTxSarQuery>           &soTSt_TxSarQry,
        stream<RXeTxSarReply>           &siTSt_TxSarRep,
        	//-- Timers Interface
        stream<RXeReTransTimerCmd>      &soTIm_ReTxTimerCmd,
        stream<SessionId>               &soTIm_ClearProbeTimer,
        stream<SessionId>               &soTIm_CloseTimer,
        //-- Event Engine Interface
        stream<ExtendedEvent>           &soEVe_SetEvent,
        //-- Tx Application Interface
        stream<SessState>               &soTAi_SessOpnSts,
        //-- Rx Application Interface
        stream<TcpAppNotif>             &soRAi_RxNotif,
        //-- MEM / Rx Write Path Interface
        stream<DmCmd>                   &soMEM_WrCmd,
        stream<AxisApp>                 &soMEM_WrData,
        stream<DmSts>                   &siMEM_WrSts,
        //--MMIO Interfaces
        stream<StsBit>                  &soMMIO_RxMemWrErr,
        stream<ap_uint<8> >             &soMMIO_CrcDropCnt,
        stream<ap_uint<8> >             &soMMIO_SessDropCnt,
        stream<ap_uint<8> >             &soMMIO_OooDropCnt,
        //-- DEBUG Interfaces
        stream<RxBufPtr>                &soDBG_RxFreeSpace,
        stream<ap_uint<32> >            &soDBG_TcpIpRxByteCnt,
        stream<ap_uint<8> >             &soDBG_oooDebug
);

#endif

/*! \} */
