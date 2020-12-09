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

using namespace hls;

void pTcpLsn(
	    ap_uint<16> 				*piMMIO_FmcLsnPort,
		stream<TcpAppLsnReq>   		&soTOE_LsnReq,
		stream<TcpAppLsnRep>   		&siTOE_LsnRep,
		ap_uint<32> 				*tcp_rx_ports_processed,
		bool 						*need_tcp_port_req,
		ap_uint<16> 				*new_relative_port_to_req_tcp,
		ap_uint<16> 				*processed_FMC_listen_port,
		bool						*nts_ready_and_enabled
		);

void pTcpRrh(
		stream<TcpAppNotif>    		&siTOE_Notif,
		stream<TcpAppRdReq>    		&soTOE_DReq,
		ap_uint<1>                  *piFMC_Tcp_data_FIFO_prog_full,
		ap_uint<1>                  *piFMC_Tcp_sessid_FIFO_prog_full,
		SessionId					*cached_tcp_rx_session_id,
		bool						*nts_ready_and_enabled
		);

#endif

/*! \} */

