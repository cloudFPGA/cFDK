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

/*****************************************************************************
 * @file       : uss.hpp
 * @brief      : The UDP Sub System (USS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/


#ifndef _NAL_USS_H_
#define _NAL_USS_H_

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

void pUdpTX(
    stream<NetworkWord>         &siUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen,
    stream<NodeId>              &sGetIpReq_UdpTx,
    stream<Ip4Addr>             &sGetIpRep_UdpTx,
    const ap_uint<32>           *ipAddrBE,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    );

void pUoeUdpTxDeq(
    ap_uint<1>                  *layer_4_enabled,
    ap_uint<1>                  *piNTS_ready,
    stream<UdpAppData>          &sUoeTxBuffer_Data,
    stream<UdpAppMeta>          &sUoeTxBuffer_Meta,
    stream<UdpAppDLen>          &sUoeTxBuffer_DLen,
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen
    );

void pUdpRx(
    ap_uint<1>                  *layer_7_enabled,
    ap_uint<1>                  *role_decoupled,
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &soUdp_meta,
    stream<UdpAppData>          &siUOE_Data,
    stream<UdpAppMeta>          &siUOE_Meta,
    stream<UdpAppDLen>          &siUOE_DLen,
    stream<NalConfigUpdate>     &sConfigUpdate,
    stream<Ip4Addr>             &sGetNidReq_UdpRx,
    stream<NodeId>              &sGetNidRep_UdpRx,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    );


void pRoleUdpRxDeq(
    ap_uint<1>                  *layer_7_enabled,
    ap_uint<1>                  *role_decoupled,
    stream<NetworkWord>         &sRoleUdpDataRx_buffer,
    stream<NetworkMetaStream>   &sRoleUdpMetaRx_buffer,
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &soUdp_meta
    );


void pUdpLsn(
      stream<UdpPort>           &soUOE_LsnReq,
      stream<StsBool>           &siUOE_LsnRep,
      stream<UdpPort>           &sUdpPortsToOpen,
      stream<bool>              &sUdpPortsOpenFeedback
    );

void pUdpCls(
    stream<UdpPort>             &soUOE_ClsReq,
    stream<StsBool>             &siUOE_ClsRep,
    stream<UdpPort>             &sUdpPortsToClose
    );


#endif

/*! \} */


