/*******************************************************************************
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
 *******************************************************************************/

/*****************************************************************************
 * @file       : tss.hpp
 * @brief      : The TCP Sub System (TSS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/


#ifndef _NAL_TSS_H_
#define _NAL_TSS_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

#include "nal.hpp"
#include "cam8.hpp"

using namespace hls;

void pTcpLsn(
    stream<TcpAppLsnReq>      &soTOE_LsnReq,
    stream<TcpAppLsnRep>      &siTOE_LsnRep,
    stream<TcpPort>           &sTcpPortsToOpen,
    stream<bool>              &sTcpPortsOpenFeedback
    );


void pTcpRxNotifEnq(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *piNTS_ready,
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppNotif>       &sTcpNotif_buffer
    );

void pTcpRRh(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *piNTS_ready,
    ap_uint<32>               *piMMIO_CfrmIp4Addr,
    ap_uint<16>               *piMMIO_FmcLsnPort,
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppRdReq>       &soTOE_DReq,
    stream<NalNewTableEntry>  &sAddNewTripple_TcpRrh,
    stream<SessionId>         &sMarkAsPriv,
    stream<SessionId>         &sDeleteEntryBySid,
    stream<TcpAppRdReq>       &sRDp_ReqNotif,
    stream<PacketLen>         &fmc_write_cnt_sig,
    stream<PacketLen>         &role_write_cnt_sig
    );

void pTcpRDp(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *piNTS_ready,
    stream<TcpAppRdReq>       &sRDp_ReqNotif,
    stream<TcpAppData>        &siTOE_Data,
    stream<TcpAppMeta>        &siTOE_SessId,
    stream<TcpAppData>        &soFMC_data,
    stream<TcpAppMeta>        &soFMC_SessId,
    stream<NetworkWord>       &soTcp_data,
    stream<NetworkMetaStream> &soTcp_meta,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<Ip4Addr>           &sGetNidReq_TcpRx,
    stream<NodeId>            &sGetNidRep_TcpRx,
    stream<SessionId>         &sGetTripleFromSid_Req,
    stream<NalTriple>         &sGetTripleFromSid_Rep,
    //stream<SessionId>       &sMarkAsPriv,
    ap_uint<32>               *piMMIO_CfrmIp4Addr,
    ap_uint<16>               *piMMIO_FmcLsnPort,
    ap_uint<1>                *layer_7_enabled,
    ap_uint<1>                *role_decoupled,
    stream<bool>              &cache_inval_sig,
    stream<NalEventNotif>     &internal_event_fifo
    );

    void pRoleTcpRxDeq(
        ap_uint<1>                *layer_7_enabled,
        ap_uint<1>                *role_decoupled,
        stream<NetworkWord>       &sRoleTcpDataRx_buffer,
        stream<NetworkMetaStream> &sRoleTcpMetaRx_buffer,
        stream<NetworkWord>       &soTcp_data,
        stream<NetworkMetaStream> &soTcp_meta,
        stream<PacketLen>         &role_write_cnt_sig
        );

    void pFmcTcpRxDeq(
        stream<TcpAppData>        &sFmcTcpDataRx_buffer,
        stream<TcpAppMeta>        &sFmcTcpMetaRx_buffer,
        stream<TcpAppData>        &soFmc_data,
        stream<TcpAppMeta>        &soFmc_meta,
        stream<PacketLen>         &fmc_write_cnt_sig
        );

    void pTcpWRp(
        ap_uint<1>                *layer_4_enabled,
        ap_uint<1>                *piNTS_ready,
        stream<TcpAppData>        &siFMC_data,
        stream<TcpAppMeta>        &siFMC_SessId,
        stream<NetworkWord>       &siTcp_data,
        stream<NetworkMetaStream> &siTcp_meta,
        stream<TcpAppData>        &soTOE_Data,
        stream<TcpAppMeta>        &soTOE_SessId,
        stream<TcpDatLen>         &soTOE_len,
        stream<NodeId>            &sGetIpReq_TcpTx,
        stream<Ip4Addr>           &sGetIpRep_TcpTx,
        stream<NalTriple>         &sGetSidFromTriple_Req,
        stream<SessionId>         &sGetSidFromTriple_Rep,
        stream<NalTriple>         &sNewTcpCon_Req,
        stream<NalNewTcpConRep>   &sNewTcpCon_Rep,
        stream<bool>              &cache_inval_sig,
        stream<NalEventNotif>     &internal_event_fifo
        );


    void pTcpWBu(
        ap_uint<1>                *layer_4_enabled,
        ap_uint<1>                *piNTS_ready,
        stream<TcpAppData>        &siWrp_Data,
        stream<TcpAppMeta>        &siWrp_SessId,
        stream<TcpDatLen>         &siWrp_len,
        stream<TcpAppData>        &soTOE_Data,
        stream<TcpAppSndReq>      &soTOE_SndReq,
        stream<TcpAppSndRep>      &siTOE_SndRep
        );

    void pTcpCOn(
        stream<TcpAppOpnReq>      &soTOE_OpnReq,
        stream<TcpAppOpnRep>      &siTOE_OpnRep,
        stream<NalNewTableEntry>  &sAddNewTriple_TcpCon,
        stream<NalTriple>         &sNewTcpCon_Req,
        stream<NalNewTcpConRep>   &sNewTcpCon_Rep
        );

    void pTcpCls(
        stream<TcpAppClsReq>      &soTOE_ClsReq,
        stream<bool>              &sGetNextDelRow_Req,
        stream<SessionId>         &sGetNextDelRow_Rep,
        stream<bool>              &sStartTclCls
        );

#endif

    /*! \} */



