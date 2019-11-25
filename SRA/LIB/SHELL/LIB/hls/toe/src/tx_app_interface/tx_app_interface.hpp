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

/*****************************************************************************
 * @file       : tx_app_interface.hpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 * @details    : Data structures, types and prototypes definitions for the
 *               Tx Application Interface.
 *****************************************************************************/

#include "../toe.hpp"
#include "../tx_app_stream/tx_app_stream.hpp"

using namespace hls;

class TxAppTableEntry
{
  public:
    TcpAckNum       ackd;
    ap_uint<16>     mempt;
    TxAppTableEntry() {}
    TxAppTableEntry(TcpAckNum ackd, ap_uint<16> mempt) :
        ackd(ackd), mempt(mempt) {}
};

/*****************************************************************************
 * @brief   Main process of the Tx Application Interface (TAi).
 *
 *****************************************************************************/
void tx_app_interface(
        //-- TRIF / Open Interfaces
        stream<LE_SockAddr>            &siTRIF_OpnReq,
        stream<OpenStatus>             &soTRIF_OpnRep,
        //-- TRIF / Data Stream Interfaces
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<AppWrSts>               &soTRIF_DSts,

        stream<TcpSessId>              &soSTt_SessStateReq,
        stream<SessionState>           &siSTt_SessStateRep,
        stream<TStTxSarPush>           &siTSt_AckPush,

        stream<ap_uint<16> >           &appCloseConnReq,
        stream<SessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,
        stream<OpenStatus>             &siRXe_SessOpnSts,

        //-- MEM / Tx PATH Interface
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<DmSts>                  &siMEM_TxP_WrSts,

        stream<TAiTxSarPush>           &soTSt_PushCmd,
        stream<LE_SocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<StateQuery>             &soSTt_Taa_SessStateQry,
        stream<SessionState>           &siSTt_Taa_SessStateRep,
        stream<Event>                  &soEVe_Event,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        LE_Ip4Addr                      piMMIO_IpAddr
);
