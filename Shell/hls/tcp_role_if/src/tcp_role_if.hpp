/*****************************************************************************
 * File:            tcp_role_if.hpp
 * Creation Date:   May 14, 2018
 * Description: 	Prototype and defines for the TCP role interface.
 *
 * Copyright 2014-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../../toe/src/toe.hpp"

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


#define no_of_session_id_table_entries 4

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


/** @defgroup echo_server_application Echo Server Application
 *
 */
void tcp_role_if(stream<ap_uint<16> >      & listenPort, 
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
