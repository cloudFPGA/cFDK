/*****************************************************************************
 * @file       : test_tcp_role_if.cpp
 * @brief      : Testbench for the TCP role interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include <iostream>

#include "../src/tcp_role_if.hpp"

using namespace hls;


int main()
{
	stream<TcpWord>			sROLE_TRIF_Data		("sROLE_TRIF_Data");
	stream<TcpWord>			sTRIF_ROLE_Data		("sTRIF_ROLE_Data");

	stream<TcpWord> 		sTOE_TRIF_Data		("sTOE_TRIF_Data");
	stream<TcpMeta> 		sTOE_TRIF_Meta		("sTOE_TRIF_Meta");
	stream<TcpWord> 		sTRIF_TOE_Data		("sTRIF_TOE_Data");
	stream<TcpMeta> 		sTRIF_TOE_Meta		("sTRIF_TOE_Meta");

	stream<TcpOpnSts> 		sTOE_TRIF_OpnSts	("sTOE_TRIF_OpnSts");
	stream<TcpOpnReq> 		sTRIF_TOE_OpnReq	("sTRIF_TOE_OpnReq");

	stream<TcpLsnAck>		sTOE_TRIF_LsnAck	("sTOE_TRIF_LsnAck");
	stream<TcpLsnReq> 		sTRIF_TOE_LsnReq	("sTRIF_TOE_LsnReq");

	stream<TcpNotif> 		sTOE_TRIF_Notif		("sTOE_TRIF_Notif");
	stream<TcpRdReq> 		sTRIF_TOE_RdReq		("sTRIF_TOE_RdReq");

	stream<TcpWrSts>		sTOE_TRIF_WrSts		("sTOE_TRIF_WrSts");

	stream<TcpClsReq> 		sTRIF_TOE_ClsReq	("sTRIF_TOE_ClsReq");

	int count = 0;
	while (count < 50) {
		tcp_role_if(
				//-- ROLE / This / Tcp Interfaces
				sROLE_TRIF_Data, sTRIF_ROLE_Data,
				//-- TOE / Data & MetaData Interfaces
				sTOE_TRIF_Data, sTOE_TRIF_Meta,
				sTRIF_TOE_Data, sTRIF_TOE_Meta,
				//-- TOE / This / Open-Connection Interfaces
				sTOE_TRIF_OpnSts, sTRIF_TOE_OpnReq,
				//-- TOE / This / Listen-Port Interfaces
				sTOE_TRIF_LsnAck, sTRIF_TOE_LsnReq,
				//-- TOE / This / Data-Read-Request Interfaces
				sTOE_TRIF_Notif, sTRIF_TOE_RdReq,
				//-- TOE / This / Write-Status Interface
				sTOE_TRIF_WrSts,
				//-- TOE / This / Close-Connection Interface
				sTRIF_TOE_ClsReq);

		//-- Emulate the listening interface of the TOE
		if (!sTRIF_TOE_LsnReq.empty()) {
			sTRIF_TOE_LsnReq.read();
			sTOE_TRIF_LsnAck.write(true);
		}
		count++;
	}
	return 0;
}
