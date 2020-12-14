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
#include "hss.hpp"

using namespace hls;

void pUdpTX(
		stream<NetworkWord>         &siUdp_data,
		stream<NetworkMetaStream>   &siUdp_meta,
	    stream<UdpAppData>          &soUOE_Data,
	    stream<UdpAppMeta>          &soUOE_Meta,
	    stream<UdpAppDLen>          &soUOE_DLen,
		stream<NodeId> 				&sGetIpReq_UdpTx,
		stream<Ip4Addr> 			&sGetIpRep_UdpTx,
		ap_uint<32> 				*ipAddrBE,
		bool						*nts_ready_and_enabled,
		stream<NalEventNotif> 		&internal_event_fifo
		);

void pUdpRx(
		stream<UdpPort>             &soUOE_LsnReq,
		stream<StsBool>             &siUOE_LsnRep,
		stream<NetworkWord>         &soUdp_data,
		stream<NetworkMetaStream>   &soUdp_meta,
	    stream<UdpAppData>          &siUOE_Data,
	    stream<UdpAppMeta>          &siUOE_Meta,
		stream<NalConfigUpdate>		&sConfigUpdate,
		stream<Ip4Addr>				&sGetNidReq_UdpRx,
		stream<NodeId>	 			&sGetNidRep_UdpRx,
		bool 						*need_udp_port_req,
		ap_uint<16>					*new_relative_port_to_req_udp,
		ap_uint<32>					*udp_rx_ports_processed,
		bool						*nts_ready_and_enabled,
		bool						*detected_cache_invalidation,
		stream<NalEventNotif> 		&internal_event_fifo
		);

void pUdpCls(
	    stream<UdpPort>             &soUOE_ClsReq,
	    stream<StsBool>             &siUOE_ClsRep,
		ap_uint<32>					*udp_rx_ports_to_close,
		bool						*start_udp_cls_fsm,
		bool						*nts_ready_and_enabled
		);


#endif

/*! \} */

