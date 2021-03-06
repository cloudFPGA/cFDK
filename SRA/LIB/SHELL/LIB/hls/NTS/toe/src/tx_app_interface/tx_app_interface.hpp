/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
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
 *******************************************************************************/

/************************************************
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
#include "../../../../NTS/toe/src/toe_utils.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR USING DEFINES IN PRAGMAS
 *  @see https://www.xilinx.com/support/answers/46111.html
 ************************************************/
#define PRAGMA_SUB(x)      _Pragma (#x)
#define DO_PRAGMA(x)      PRAGMA_SUB(x)

/************************************************
 * DEFINITIONS OF THE LOCAL STREAM DEPTHS
 *  Postponed for the time being:
 *  @see https://www.xilinx.com/support/answers/46111.html
 ************************************************/
#define DEPTH_ssEmxToTash_Event       2
#define DEPTH_ssTacToEmx_Event        2
#define DEPTH_ssTatToSml_AccessRep    2
#define DEPTH_ssSmlToTat_AccessQry    2
#define DEPTH_ssSmlToMwr_SegMeta    128
#define DEPTH_ssSmlToEmx_Event        2
#define DEPTH_ssSlgToMwr_Data       256
#define DEPTH_ssSlgToSml_SegLen      32


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
 * Metadata for storing APP data in memory
 ************************************************/
class AppMemMeta {
  public:
    TcpSessId    sessId;
    TcpBufAdr    addr;
    TcpDatLen    len;
    AppMemMeta() {}
    AppMemMeta(TcpSessId sessId, TcpBufAdr addr, TcpDatLen len) :
        sessId(sessId), addr(addr), len(len) {}
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
        stream<TcpAppSndReq>           &siTAIF_SndReq,
        stream<TcpAppSndRep>           &soTAIF_SndRep,
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
