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
 * @file       : tx_app_interface.hpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_TAI_H_
#define _TOE_TAI_H_

#include "../../../../NTS/nts.hpp"
#include "../../../../NTS/nts_utils.hpp"
#include "../../../../NTS/SimNtsUtils.hpp"
#include "../../../../NTS/toe/src/toe.hpp"
//#include "../../../../NTS/toe/src/event_engine/event_engine.hpp"
//#include "../../../../NTS/toe/src/state_table/state_table.hpp"

using namespace hls;

//OBSOLET_20200713 #define ERROR_NOSPACE        1
//OBSOLET_20200713 #define ERROR_NOCONNCECTION  2


/************************************************
 * Tx Application Table (Tat)
 *  Structure to manage the FPGA Send Window
 ************************************************/
class TxAppTableEntry {
  public:
    TcpAckNum       ackd;
    TxBufPtr        mempt;
    TxAppTableEntry() {}
    TxAppTableEntry(TcpAckNum ackd, ap_uint<16> mempt) :
        ackd(ackd), mempt(mempt) {}
};

/************************************************
 * Metadata for storing a segment in memory
 ************************************************/
class SegMemMeta {
  public:
    TcpSessId    sessId;
    TcpBufAdr    addr;
    TcpSegLen    len;
    bool         drop;
    SegMemMeta() {}
    SegMemMeta(bool drop) :
        sessId(0), addr(0), len(0), drop(drop) {}
    SegMemMeta(TcpSessId sessId, TcpBufAdr addr, TcpSegLen len) :
        sessId(sessId), addr(addr), len(len), drop(false) {}
};

/*******************************************************************************
 *
 * @brief ENTITY - Tx Application Interface (TAi)
 *
 *******************************************************************************/
void tx_app_interface(
        //-- TAIF / Open-Close Interfaces
        stream<TcpAppOpnReq>           &siTAIF_OpnReq,
        stream<TcpAppOpnRep>           &soTAIF_OpnRep,
        stream<TcpAppClsReq>           &siTAIF_ClsReq,
        //-- TAIF / Data Stream Interfaces
        stream<TcpAppData>             &siTAIF_Data,
        stream<TcpAppMeta>             &siTAIF_Meta,
        stream<TcpAppWrSts>            &soTAIF_DSts,
        //-- MEM / Tx PATH Interface
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxisApp>                &soMEM_TxP_Data,
        stream<DmSts>                  &siMEM_TxP_WrSts,
        //-- State Table Interfaces
        stream<SessionId>              &soSTt_SessStateReq,
        stream<TcpState>               &siSTt_SessStateRep,
        stream<StateQuery>             &soSTt_ConnectStateQry,
        stream<TcpState>               &siSTt_ConnectStateRep,
        //-- Session Lookup Controller Interface
        stream<SocketPair>             &soSLc_SessLookupReq,
        stream<SessionLookupReply>     &siSLc_SessLookupRep,
        //-- Port Table Interfaces
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<TcpPort>                &siPRt_GetFreePortRep,
        //-- Tx SAR TAble Interfaces
        stream<TStTxSarPush>           &siTSt_PushCmd,
        stream<TAiTxSarPush>           &soTSt_PushCmd,
        //-- Rx Engine Interface
        stream<SessState>              &siRXe_ActSessState,
        //-- Event Engine Interface
        stream<Event>                  &soEVe_Event,
        //-- Timers Interface
        stream<SessState>              &siTIm_Notif,
        //-- MMIO / IPv4 Address
        LE_Ip4Addr                      piMMIO_IpAddr  // [FIXME]
);

#endif

/*! \} */
