/*****************************************************************************
 * File:            tcp_role_if.cpp
 * Creation Date:   May 14, 2018
 * Description:     <Short description (1-2 lines)>
 *
 * Copyright 2014-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "tcp_role_if.hpp"

using namespace hls;

///////////////////////////////////////////////////////////////////////
/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
session_id_cam::session_id_cam() {
	for (uint8_t i=0;i<no_of_session_id_table_entries;++i) // Go through all the entries in the filter
      	this->filter_entries[i].valid = 0;  // And mark them as invali
}


/*****************************************************************************/
/* session_id_cam::write
 *
 * \desc            Description of function.
 *
 * \param[in]		write_entry is ...
 *
 * \return          True if ...
 *
 *****************************************************************************/
bool session_id_cam::write(session_id_table_entry write_entry)
{
//#pragma HLS PI Role_Udp_Tcp_McDp_4BEmifPELINE II=1

	for (uint8_t i=0;i<no_of_session_id_table_entries;++i) { // Go through all the entries in the filter
#pragma HLS UNROLL
      		if (this->filter_entries[i].valid == 0 && write_entry.valid == 0) { // write
         		this->filter_entries[i].session_id = write_entry.session_id;
         		this->filter_entries[i].buffer_id = write_entry.buffer_id;
         		this->filter_entries[i].valid = 1; // If all these conditions are met then return true;
         		return true;
      		} else if (this->filter_entries[i].valid == 1 && write_entry.valid == 1 && this->filter_entries[i].buffer_id == write_entry.buffer_id ) { // overwrite
         		this->filter_entries[i].session_id = write_entry.session_id;
         		this->filter_entries[i].buffer_id = write_entry.buffer_id;
         		this->filter_entries[i].valid = 1; // If all these conditions are met then return true;
         		return true;
      		}
   	}
   	return false;
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
ap_uint<16> session_id_cam::compare_buffer_id(
		ap_uint<4> q_buffer_id)
{
//#pragma HLS PIPELINE II=1

	for (uint8_t i=0;i<no_of_session_id_table_entries;++i){ // Go through all the entries in the filter
#pragma HLS UNROLL
      		if ((this->filter_entries[i].valid == 1) && (q_buffer_id == this->filter_entries[i].buffer_id)){ // Check if the entry is valid and if the addresses match
         		return this->filter_entries[i].session_id; // If so delete the entry (mark as invalid)
      		}
   	}
   	return -1;
}


/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_session_id_table_server(
		stream<session_id_table_entry>& w_entry,
		stream<ap_uint<4> > &			q_buffer_id,
		stream<ap_uint<16> > &			r_session_id)
{
#pragma HLS INLINE region
#pragma HLS PIPELINE II=1 //enable_flush

	//static enum uit_state {UIT_IDLE, UIT_RX_READ, UIT_TX_READ, UIT_WRITE, UIT_CLEAR} tcp_ip_table_state;

   	static session_id_cam  session_id_cam_table;
#pragma HLS array_partition variable=session_id_cam_table.filter_entries complete

	session_id_table_entry in_entry;
	ap_uint<4> in_buffer_id;
	ap_uint<16> in_session_id;
	//static bool rdWrswitch = true;

	if(!q_buffer_id.empty() && !r_session_id.full() /*&& !rdWrswitch*/){
		q_buffer_id.read(in_buffer_id);
		in_session_id = session_id_cam_table.compare_buffer_id(in_buffer_id);
		r_session_id.write(in_session_id);
		//rdWrswitch = !rdWrswitch;
	} else if(!w_entry.empty() /*&& rdWrswitch*/){
		w_entry.read(in_entry);
		session_id_cam_table.write(in_entry);
		//rdWrswitch = !rdWrswitch;
	}

	//rdWrswitch = !rdWrswitch;
}

///////////////////////////////////////////////////////////////////////////////////////


/** @ingroup echo_server_application
 *
 */

//#if 0

/*
ap_uint<4> keep_to_len(ap_uint<8> keepValue) { 		// This function counts the number of 1s in an 8-bit value
	ap_uint<4> counter = 0;
	for (ap_uint<4> i=0;i<8;++i) {
		if (keepValue.bit(i) == 1)
			counter++;
	}
	return counter;
}
*/

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
ap_uint<4> keep_to_len(ap_uint<8> keepValue) { 		// This function counts the number of 1s in an 8-bit value
	ap_uint<4> counter = 0;

	switch(keepValue){
		case 0x01: counter = 1; break;
		case 0x03: counter = 2; break;
		case 0x07: counter = 3; break;
		case 0x0F: counter = 4; break;
		case 0x1F: counter = 5; break;
		case 0x3F: counter = 6; break;
		case 0x7F: counter = 7; break;
		case 0xFF: counter = 8; break;
	}
	return counter;
}


/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_open_connection(
		stream<ipTuple>& 		openConnection,
		stream<openStatus>& 	openConStatus,
		stream<ap_uint<16> >& 	closeConnection)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE OFF

	openStatus newConn;
	ipTuple tuple;

	if (!openConStatus.empty() && !openConnection.full() && !closeConnection.full()) {
		openConStatus.read(newConn);
		tuple.ip_address = 0x0b010101; 	// [TODO: Need to be received from the MGMT module]
		tuple.ip_port =  0x3412;		// [TODO: Need to be received from the MGMT module]
		openConnection.write(tuple);
		if (newConn.success) {
			closeConnection.write(newConn.sessionID);
			//closePort.write(tuple.ip_port);
		}
	}
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_listen_port_status(
		stream<ap_uint<16> >& 	listenPort,
		stream<bool>& 			listenPortStatus)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE OFF

	static bool listenDone = false;
	static bool waitPortStatus = false;

	// Open/Listen on Port at startup
	if (!listenDone && !waitPortStatus && !listenPort.full()) {
		listenPort.write(80);
		waitPortStatus = true;
	} else if (waitPortStatus && !listenPortStatus.empty()) { // Check if listening on Port was successful, otherwise try again
		listenPortStatus.read(listenDone);
		waitPortStatus = false;
	}
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_listen_new_data(
		stream<appNotification>& 		notifications,
		stream<appReadRequest>& 		readRequest,
		stream<session_id_table_entry>& sess_entry)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE OFF

	static ap_uint<16> sess_id = 99;
	static bool first_write = true;
	static enum listen_new_data { LISTEN_NOTIFICATION = 0, WRITE_SESSION } listen_new_data_state;
	appNotification notification;

	switch(listen_new_data_state)
	{
		case LISTEN_NOTIFICATION:
			if (!notifications.empty() && !readRequest.full()){
				notifications.read(notification);

				if (notification.length != 0){
					readRequest.write(appReadRequest(notification.sessionID, notification.length));
				}

				if(sess_id != notification.sessionID ||  first_write) {
					sess_id = notification.sessionID;
					listen_new_data_state = WRITE_SESSION;
				}
			}
		break;
		case WRITE_SESSION:
			if (!sess_entry.full()){
				if(first_write){
					sess_entry.write(session_id_table_entry(sess_id, 1, 0));
					first_write = !first_write;
				} else
					sess_entry.write(session_id_table_entry(sess_id, 1, 1));

				listen_new_data_state = LISTEN_NOTIFICATION;
			}
		break;
	}
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_check_tx_status(
		stream<ap_int<17> >& txStatus)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE OFF

	if (!txStatus.empty()) //Make Checks
	{
		txStatus.read();
	}
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_net_to_app(
		stream<ap_uint<16> >& 	rxMetaData,
		stream<axiWord>& 		rxData       /*, stream<ap_uint<16> >& txMetaData,*/,
		stream<axiWord>& 		txData)
{
#pragma HLS PIPELINE II=1

	static enum read_write_engine { RWE_SESSION_ID = 0, RWE_DATA } read_write_engine_state;
	ap_uint<16> sessionID;
	axiWord currWord;
	static ap_uint<16> old_session_id = 0;

	switch (read_write_engine_state)
	{
		case RWE_SESSION_ID:
			if (!rxMetaData.empty() /*&& !txMetaData.full()*/) {
				rxMetaData.read(sessionID);
				//if(old_session_id != sessionID){
					//txMetaData.write(sessionID);
					//old_session_id = sessionID;
				//}
				read_write_engine_state = RWE_DATA;
			}
		break;
		case RWE_DATA:
			if (!rxData.empty() && !txData.full()) {
				rxData.read(currWord);
				txData.write(currWord);

				if (currWord.last)
					read_write_engine_state = RWE_SESSION_ID;
			}
		break;
	}
}


/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_app_to_buf(
		stream<axiWord>& 		vFPGA_tx_data,
		stream<ap_uint<4> >& 	q_buffer_id,
		stream<axiWord>& 		buff_data)
{
#pragma HLS PIPELINE II=1

	static enum read_write_engine { RWE_SESS_RD = 0, RWE_BUFF_DATA} read_write_engine_state;
	axiWord currWord;

	switch (read_write_engine_state)
	{
		case RWE_SESS_RD:
			if(!vFPGA_tx_data.empty() && !q_buffer_id.full() && !buff_data.full()){
				vFPGA_tx_data.read(currWord);
				buff_data.write(currWord);
				q_buffer_id.write(1);

				if(!currWord.last)
					read_write_engine_state = RWE_BUFF_DATA;
			}
		break;
		case RWE_BUFF_DATA:
			if (!vFPGA_tx_data.empty() && !buff_data.full()) {
				vFPGA_tx_data.read(currWord);
				buff_data.write(currWord);

				if(currWord.last)
					read_write_engine_state = RWE_SESS_RD;
			}
		break;
	}
}

/*****************************************************************************/
/* NameOfTheFunction
 *
 * \desc            Description of function.
 *
 * \param[in|out|stream] <NameOfTheParameter> with short description.
 *
 * \return          The returned value.
 *
 *****************************************************************************/
void tai_app_to_net(
		stream<axiWord>& 		buff_data,
		stream<ap_uint<16> >& 	txMetaData,
		stream<axiWord>& 		oTxData,
		stream<ap_uint<16> >& 	r_session_id)
{
#pragma HLS PIPELINE II=1

	static enum read_write_engine { RWE_SESS_RD = 0, RWE_TX_DATA} read_write_engine_state;
	ap_uint<16> sessionID;
	axiWord currWord;

	switch (read_write_engine_state)
	{
		case RWE_SESS_RD:
			if(!buff_data.empty() && !txMetaData.full() && !oTxData.full() && !r_session_id.empty()){
				txMetaData.write(r_session_id.read());
				buff_data.read(currWord);
				oTxData.write(currWord);

				if(!currWord.last)
					read_write_engine_state = RWE_TX_DATA;
			}
		break;
		case RWE_TX_DATA:
			if (!buff_data.empty() && !oTxData.full()) {
				buff_data.read(currWord);
				oTxData.write(currWord);

				if(currWord.last)
					read_write_engine_state = RWE_SESS_RD;
			}
		break;
	}
}


/*****************************************************************************/
/* tcp_role_if
 *
 * \desc            Description of function.
 *
 * \param[stream]	oListenPort is...
 *
 * \param[stream]	iListenPortStatus is ...
 *
 * \param[stream]	iNotifications ...
 *
 * \param[stream]	oReadRequest ..
 *
 * \param[stream]	iRxMetaData ...
 *
 * \param[stream]	iRxData
 *
 * \param[stream]	oOpenConnection
 *
 * \param[stream]	iOpenConStatus
 *
 * \param[stream]   oCloseConnection ...
 *
 * \param[stream]	oTxMetaData ...
 *
 * \param[stream]   oTxData ...
 *
 * \param[stream]	iTxStatus ...
 *
 * \param[stream] 	vFPGA_rx_data ...
 *
 * \param[stream]	vFPGA_tx_data is
 *
 * \return          The returned value.
 *
 *****************************************************************************/

/*session id is updated for only if a new connection is established. therefore, app does not have to
 * always return the same amount of segments received */

void tcp_role_if(
		stream<ap_uint<16> >& 		oListenPort,
		stream<bool>& 				iListenPortStatus,
		stream<appNotification>& 	iNotifications,
		stream<appReadRequest>& 	oReadRequest,
		stream<ap_uint<16> >& 		iRxMetaData,
		stream<axiWord>& 			iRxData,
		stream<ipTuple>& 			oOpenConnection,
		stream<openStatus>& 		iOpenConStatus,
		stream<ap_uint<16> >& 		oCloseConnection,
		stream<ap_uint<16> >& 		oTxMetaData,
		stream<axiWord>& 			oTxData,
		stream<ap_int<17> >& 		iTxStatus,
		stream<axiWord>& 			vFPGA_rx_data,
		stream<axiWord>& 			vFPGA_tx_data)
{

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW

#pragma HLS resource core=AXI4Stream variable=oListenPort metadata="-bus_bundle m_axis_listen_port"
#pragma HLS resource core=AXI4Stream variable=iListenPortStatus metadata="-bus_bundle s_axis_listen_port_status"
#pragma HLS resource core=AXI4Stream variable=iNotifications metadata="-bus_bundle s_axis_notifications"
#pragma HLS resource core=AXI4Stream variable=oReadRequest metadata="-bus_bundle m_axis_read_request"
#pragma HLS resource core=AXI4Stream variable=iRxMetaData metadata="-bus_bundle s_axis_rx_meta_data"
#pragma HLS resource core=AXI4Stream variable=iRxData metadata="-bus_bundle s_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=oOpenConnection metadata="-bus_bundle m_axis_open_connection"
#pragma HLS resource core=AXI4Stream variable=iOpenConStatus metadata="-bus_bundle s_axis_open_connection_status"
#pragma HLS resource core=AXI4Stream variable=oCloseConnection metadata="-bus_bundle m_axis_close_connection"
#pragma HLS resource core=AXI4Stream variable=oTxMetaData metadata="-bus_bundle m_axis_tx_meta_data"
#pragma HLS resource core=AXI4Stream variable=oTxData metadata="-bus_bundle m_axis_tx_data"
#pragma HLS resource core=AXI4Stream variable=iTxStatus metadata="-bus_bundle s_axis_tx_status"
#pragma HLS resource core=AXI4Stream variable=vFPGA_rx_data metadata="-bus_bundle m_axis_vfpga_rx_data"
#pragma HLS resource core=AXI4Stream variable=vFPGA_tx_data metadata="-bus_bundle s_axis_vfpga_tx_data"

#pragma HLS DATA_PACK variable=iNotifications
#pragma HLS DATA_PACK variable=oReadRequest
#pragma HLS DATA_PACK variable=oOpenConnection
#pragma HLS DATA_PACK variable=iOpenConStatus


static stream<session_id_table_entry>  w_entry("w_entry");
#pragma HLS STREAM variable=w_entry depth=1
static stream<ap_uint<4> > q_buffer_id("q_buffer_id");
#pragma HLS STREAM variable=q_buffer_id depth=1
static stream<ap_uint<16> > r_session_id("r_session_id");
#pragma HLS STREAM variable=r_session_id depth=1

static stream<axiWord>  buff_data("buff_data");
#pragma HLS STREAM variable=buff_data depth=1024

	tai_open_connection(oOpenConnection, iOpenConStatus, oCloseConnection);
	tai_listen_port_status(oListenPort, iListenPortStatus);

	tai_listen_new_data(iNotifications, oReadRequest, w_entry);

	tai_check_tx_status(iTxStatus);

	tai_session_id_table_server(w_entry, q_buffer_id, r_session_id);

	tai_net_to_app(iRxMetaData, iRxData, vFPGA_rx_data);

	tai_app_to_buf(vFPGA_tx_data, q_buffer_id, buff_data);
	tai_app_to_net(buff_data, oTxMetaData, oTxData, r_session_id);

}

