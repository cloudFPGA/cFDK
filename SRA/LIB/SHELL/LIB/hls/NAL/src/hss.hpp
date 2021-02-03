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
 * @file       : hss.hpp
 * @brief      : The Housekeeping Sub System (HSS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/


#ifndef _NAL_HSS_H_
#define _NAL_HSS_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

#include "nal.hpp"

using namespace hls;


void axi4liteProcessing(
    ap_uint<32>   ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    //stream<NalConfigUpdate> &sToTcpAgency, //(currently not used)
    stream<NalConfigUpdate>   &sToPortLogic,
    stream<NalConfigUpdate>   &sToUdpRx,
    stream<NalConfigUpdate>   &sToTcpRx,
    stream<NalConfigUpdate>   &sToStatusProc,
    //stream<NalMrtUpdate>    &sMrtUpdate,
    ap_uint<32>               localMRT[MAX_MRT_SIZE],
    stream<uint32_t>          &mrt_version_update_0,
    stream<uint32_t>          &mrt_version_update_1,
    stream<NalStatusUpdate>   &sStatusUpdate
    );


void pMrtAgency(
    const ap_uint<32>         localMRT[MAX_MRT_SIZE],
    stream<NodeId>            &sGetIpReq_UdpTx,
    stream<Ip4Addr>           &sGetIpRep_UdpTx,
    stream<NodeId>            &sGetIpReq_TcpTx,
    stream<Ip4Addr>           &sGetIpRep_TcpTx,
    stream<Ip4Addr>           &sGetNidReq_UdpRx,
    stream<NodeId>            &sGetNidRep_UdpRx,
    stream<Ip4Addr>           &sGetNidReq_TcpRx,
    stream<NodeId>            &sGetNidRep_TcpRx//,
    //stream<Ip4Addr>         &sGetNidReq_TcpTx,
    //stream<NodeId>          &sGetNidRep_TcpTx
    );


void pPortLogic(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *layer_7_enabled,
    ap_uint<1>                *role_decoupled,
    ap_uint<1>                *piNTS_ready,
    ap_uint<16>               *piMMIO_FmcLsnPort,
    ap_uint<32>               *pi_udp_rx_ports,
    ap_uint<32>               *pi_tcp_rx_ports,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<UdpPort>           &sUdpPortsToOpen,
    stream<UdpPort>           &sUdpPortsToClose,
    stream<TcpPort>           &sTcpPortsToOpen,
    stream<bool>              &sUdpPortsOpenFeedback,
    stream<bool>              &sTcpPortsOpenFeedback,
    stream<bool>              &sMarkToDel_unpriv,
    stream<NalPortUpdate>     &sPortUpdate,
    stream<bool>              &sStartTclCls
    );

void pCacheInvalDetection(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *layer_7_enabled,
    ap_uint<1>                *role_decoupled,
    ap_uint<1>                *piNTS_ready,
    stream<uint32_t>          &mrt_version_update,
    stream<bool>              &inval_del_sig,
    stream<bool>              &cache_inval_0,
    stream<bool>              &cache_inval_1,
    stream<bool>              &cache_inval_2, //MUST be connected to TCP
    stream<bool>              &cache_inval_3  //MUST be connected to TCP
    );

void pTcpAgency(
    stream<SessionId>         &sGetTripleFromSid_Req,
    stream<NalTriple>         &sGetTripleFromSid_Rep,
    stream<NalTriple>         &sGetSidFromTriple_Req,
    stream<SessionId>         &sGetSidFromTriple_Rep,
    stream<NalNewTableEntry>  &sAddNewTriple_TcpRrh,
    stream<NalNewTableEntry>  &sAddNewTriple_TcpCon,
    stream<SessionId>         &sDeleteEntryBySid,
    stream<bool>              &inval_del_sig,
    stream<SessionId>         &sMarkAsPriv,
    stream<bool>              &sMarkToDel_unpriv,
    stream<bool>              &sGetNextDelRow_Req,
    stream<SessionId>         &sGetNextDelRow_Rep
    );


#endif

    /*! \} */

