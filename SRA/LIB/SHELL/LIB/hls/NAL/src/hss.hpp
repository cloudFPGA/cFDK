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


struct NalConfigUpdate {
	ap_uint<16>   config_addr;
	ap_uint<16>   update_value;
	NalConfigUpdate() {}
	NalConfigUpdate(ap_uint<16> ca, ap_uint<16> uv): config_addr(ca), update_value(uv) {}
};

//struct NalMrtUpdate {
//	NodeId 		nid;
//	Ip4Addr 	ip4a;
//	NalMrtUpdate(NodeId node, Ip4Addr addr): nid(node), ip4a(addr) {}
//};

struct NalStatusUpdate {
	ap_uint<16>   status_addr;
	ap_uint<16>   new_value;
	NalStatusUpdate(ap_uint<16> sa, ap_uint<16> nv): status_addr(sa), new_value(nv) {}
};


void axi4liteProcessing(
    ap_uint<32>   ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    ap_uint<32>   *mrt_version_processed,
	stream<NalConfigUpdate> 	&sToTcpAgency,
	stream<NalConfigUpdate> 	&sToPortLogic,
	stream<NalConfigUpdate>		&sToUdpRx,
	stream<NalConfigUpdate>		&sToTcpRx,
	//stream<NalMrtUpdate>		&sMrtUpdate,
	stream<NalStatusUpdate> 	&sStatusUpdate,
	stream<NodeId> 				&sGetIpReq_UdpTx,
	stream<Ip4Addr> 			&sGetIpRep_UdpTx,
	stream<NodeId> 				&sGetIpReq_TcpTx,
	stream<Ip4Addr> 			&sGetIpRep_TcpTx,
	stream<Ip4Addr>				&sGetNidReq_UdpRx,
	stream<NodeId>				&sGetNidRep_UdpRx,
	stream<Ip4Addr>				&sGetNidReq_TcpRx,
	stream<NodeId>				&sGetNidRep_TcpRx,
	stream<Ip4Addr>				&sGetNidReq_TcpTx,
	stream<NodeId>				&sGetNidRep_TcpTx,
    );


#endif

/*! \} */

