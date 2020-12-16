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
#include "hss.hpp"

using namespace hls;

void pTcpLsn(
		stream<TcpAppLsnReq>   		&soTOE_LsnReq,
		stream<TcpAppLsnRep>   		&siTOE_LsnRep,
		stream<TcpPort>				&sTcpPortsToOpen,
		stream<bool>				&sTcpPortsOpenFeedback,
		const bool					*nts_ready_and_enabled
		);

void pTcpRRh(
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppRdReq>       &soTOE_DReq,
	//add add and delete streams
    ap_uint<1>                *piFMC_Tcp_data_FIFO_prog_full,
    ap_uint<1>                *piFMC_Tcp_sessid_FIFO_prog_full,
    const bool				  *role_fifo_empty,
    const bool                *nts_ready_and_enabled
    );

void pTcpRDp(
    stream<TcpAppData>        &siTOE_Data,
    stream<TcpAppMeta>        &siTOE_SessId,
    stream<TcpAppData>          &soFMC_Tcp_data,
    stream<TcpAppMeta>          &soFMC_Tcp_SessId,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<Ip4Addr>       &sGetNidReq_TcpRx,
    stream<NodeId>        &sGetNidRep_TcpRx,
    ap_uint<32>         *piMMIO_CfrmIp4Addr,
    const ap_uint<16>       *processed_FMC_listen_port,
    ap_uint<1>          *layer_7_enabled,
    ap_uint<1>          *role_decoupled,
    //SessionId         *cached_tcp_rx_session_id,
    bool            *expect_FMC_response,
    const bool          *nts_ready_and_enabled,
    const bool          *detected_cache_invalidation,
    stream<NalEventNotif>     &internal_event_fifo
    );

void pTcpWRp(
		stream<TcpAppData>          &siFMC_Tcp_data,
		stream<TcpAppMeta>          &siFMC_Tcp_SessId,
	    stream<NetworkWord>         &siTcp_data,
	    stream<NetworkMetaStream>   &siTcp_meta,
		stream<TcpAppData>     		&soTOE_Data,
		stream<TcpAppMeta>     		&soTOE_SessId,
		stream<NodeId> 				&sGetIpReq_TcpTx,
		stream<Ip4Addr> 			&sGetIpRep_TcpTx,
		stream<Ip4Addr> 			&sGetNidReq_TcpTx,
		stream<NodeId> 				&sGetNidRep_TcpTx,
		const bool					*expect_FMC_response,
		ap_uint<64>  				*tripple_for_new_connection,
		bool 						*tcp_need_new_connection_request,
		bool 						*tcp_new_connection_failure,
		const bool					*nts_ready_and_enabled,
		const bool					*detected_cache_invalidation,
		stream<NalEventNotif> 		&internal_event_fifo
		);

void pTcpCOn(
		stream<TcpAppOpnReq>      	&soTOE_OpnReq,
		stream<TcpAppOpnRep>   		&siTOE_OpnRep,
		//stream<TcpAppClsReq>   		&soTOE_ClsReq,
		ap_uint<64>  				*tripple_for_new_connection,
		bool 						*tcp_need_new_connection_request,
		bool 						*tcp_new_connection_failure,
		const bool					*nts_ready_and_enabled
		);

void pTcpCls(
		stream<TcpAppClsReq>   		&soTOE_ClsReq,
		bool						*start_tcp_cls_fsm,
		const bool					*nts_ready_and_enabled
		);

#endif

/*! \} */

