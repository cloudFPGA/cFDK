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

// #include "../../toe/src/toe.hpp"
// #include "tcp_role_if.hpp"

#include <hls_stream.h>
#include <iostream>

#include "../src/tcp_role_if.hpp"

using namespace hls;


int main()
{
	stream<axiWord>			vFPGA_tx_data;
	stream<axiWord>			vFPGA_rx_data;

	stream<axiWord> 		rxData;
	stream<ap_uint<16> > 	rxMetaData;
	stream<axiWord> 		txData;
	stream<ap_uint<16> > 	txMetaData;

	stream<openStatus> 		openConStatus;
	stream<ipTuple> 		openConnection;

	stream<bool> 			listenPortStatus("listenPortStatus");
	stream<ap_uint<16> > 	listenPort("listenPort");

	stream<appNotification> notifications;
	stream<appReadRequest> 	readRequest;

	stream<ap_int<17> >		txStatus;
	stream<ap_uint<16> > 	closeConnection;

	int count = 0;
	while (count < 50) {
		tcp_role_if(
				//-- ROLE / This / Tcp Interfaces
				vFPGA_tx_data, vFPGA_rx_data,
				//-- TOE / Data & MetaData Interfaces
				rxData, rxMetaData,
				txData, txMetaData,
				//-- TOE / This / Open-Connection Interfaces
				openConStatus, openConnection,
				//-- TOE / This / Listen-Port Interfaces
				listenPortStatus, listenPort,
				//-- TOE / This / Data-Read-Request Interfaces
				notifications, readRequest,
				//-- TOE / This / Close-Connection Interfaces
				txStatus, closeConnection);

		if (!listenPort.empty())
		{
			listenPort.read();
			listenPortStatus.write(true);
		}
		count++;
	}
	return 0;
}
