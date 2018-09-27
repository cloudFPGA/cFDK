/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP-Role interface.
 *
 *****************************************************************************/

#include "../../toe/src/toe.hpp"



#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


#define no_of_session_id_table_entries 4


//struct axiWord {            // TCP Streaming Chunk (i.e. 8 bytes)
//    ap_uint<64>    tdata;
//    ap_uint<8>     tkeep;
//    ap_uint<1>     tlast;
//    axiWord()      {}
//    axiWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
//                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
//};


struct session_id_table_entry {
	ap_uint<16> session_id;
	ap_uint<4> buffer_id;
	ap_uint<1> valid;

	session_id_table_entry() {}
	session_id_table_entry(ap_uint<16> session_id, ap_uint<4> buffer_id, ap_uint<1> valid)
					  :session_id(session_id), buffer_id(buffer_id), valid(valid){}
};


class session_id_cam {
public:
	session_id_table_entry filter_entries[no_of_session_id_table_entries];
	session_id_cam();
	bool write(session_id_table_entry write_entry); // Returns true if write completed successfully, else false
	ap_uint<16>  compare_buffer_id(ap_uint<4> q_buffer_id);
	/*return buffer id*/
	ap_uint<4>  compare_session_id(ap_uint<16> q_session_id);
};

void tai_session_id_table_server(stream<session_id_table_entry> &w_entry,
						stream <bool > & w_entry_done,
						stream<ap_uint<4> > &q_buffer_id,
						stream<ap_uint<16> > &r_session_id);

void tcp_role_if(

		//------------------------------------------------------
		//-- ROLE / This / Tcp Interfaces
		//------------------------------------------------------
		stream<axiWord>& 			vFPGA_tx_data,
		stream<axiWord>& 			vFPGA_rx_data,

		//------------------------------------------------------
		//-- TOE / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<axiWord>				&iRxData,
		stream<ap_uint<16> >		&iRxMetaData,
		stream<axiWord>				&oTxData,
		stream<ap_uint<16> >		&oTxMetaData,

		//------------------------------------------------------
		//-- TOE / This / Open-Connection Interfaces
		//------------------------------------------------------
		stream<openStatus>			&iOpenConStatus,
		stream<ipTuple>				&oOpenConnection,

		//------------------------------------------------------
		//-- TOE / This / Listen-Port Interfaces
		//------------------------------------------------------
		stream<bool>				&iListenPortStatus,
		stream<ap_uint<16> >		&oListenPort,

		//------------------------------------------------------
		//-- TOE / This / Data-Read-Request Interfaces
		//------------------------------------------------------
		stream<appNotification>		&iNotifications,
		stream<appReadRequest>		&oReadRequest,

		//------------------------------------------------------
		//-- TOE / This / Close-Connection Interfaces
		//------------------------------------------------------
		stream<ap_int<17> >& 		iTxStatus,
		stream<ap_uint<16> >& 		oCloseConnection

);




















/*** OBSOLET **********************
void tcp_role_if(

		stream<axiWord>  		  & vFPGA_tx_data,
		stream<axiWord>  		  & vFPGA_rx_data,

		stream<ap_uint<16> >      & listenPort,
		stream<bool>              & listenPortStatus,

		// This is disabled for the time being, because it adds complexity/potential issues
		//stream<ap_uint<16> >& closePort,
		stream<appNotification>   & notifications,
		stream<appReadRequest>    & readRequest,

		stream<ap_uint<16> >      & rxMetaData,
		stream<axiWord>           & rxData,

		stream<ipTuple>           & openConnection,
		stream<openStatus>        & openConStatus,

		stream<ap_uint<16> >      & closeConnection,

		stream<ap_uint<16> >      & txMetaData,
		stream<axiWord>           & txData,

		stream<ap_int<17> >       & txStatus);
***********************************/
