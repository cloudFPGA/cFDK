/*****************************************************************************
 * @file       : tcp_role_if.cpp
 * @brief      : TCP Role Interface (TRIF)
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
 * @details    : This process exposes the TCP data path to the role. The main
 *  purpose of the TRIF is to hide the socket mechanism from the user's role
 *  application. As a result, the user application sends and receives its data
 *  octets over plain Tx and Rx AXI4-Stream interfaces.
 *
 * @note       : The Tx data path (i.e. THIS->TCP) will block until a data
 *                chunk and its corresponding metadata is received by the Rx
 *                path.
 * @remark     : { remark text }
 * @warning    : For the time being, all communications occur over port #80.
 * @todo       : [TODO - Port# must configurable]
 *
 * @see        :
 *
 *****************************************************************************/

#include "tcp_role_if.hpp"
#include <stdint.h>

using namespace hls;


/*****************************************************************************
 * @brief SessionIdCam - Default constructor.
 * @ingroup udp_role_if
 ******************************************************************************/
SessionIdCam::SessionIdCam()
{
    for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i)
    	this->cam[i].valid = 0;
}

/*****************************************************************************
 * @brief SessionIdCam - Write a new entry into the CAM.
 * @ingroup udp_role_if
 *
 * @param[in] wrEntry, the entry to write into the CAM.
 *
 * @return true if the entry was written, otherwise false.
 ******************************************************************************/
bool SessionIdCam::write(SessionIdCamEntry wrEntry)
{
    for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i) {
    	// Go through all the entries in the CAM
		#pragma HLS UNROLL
            if (this->cam[i].valid == 0 && wrEntry.valid == 0) {
            	// Write new entry
                this->cam[i].sessionId = wrEntry.sessionId;
                this->cam[i].bufferId  = wrEntry.bufferId;
                this->cam[i].valid      = 1;
                return true;
            } else if (this->cam[i].valid == 1 && wrEntry.valid == 1 &&
            		   this->cam[i].bufferId == wrEntry.bufferId ) {
            	// Overwrite current entry
                this->cam[i].sessionId = wrEntry.sessionId;
                this->cam[i].bufferId  = wrEntry.bufferId;
                this->cam[i].valid     = 1;
                // If all these conditions are met then return true;
                return true;
            }
    }
    return false;
}

/*****************************************************************************
 * @brief SessionIdCam - search for the presence of a buffer ID into the CAM.
 * @ingroup udp_role_if
 *
 * @param[in] bufferID, the buffer ID to lookup.
 *
 * @return the session ID of the matching buffer ID, otherwise -1.
 ******************************************************************************/
ap_uint<16> SessionIdCam::search(ap_uint<4> bufferId)
{
	for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i) {
    	// Go through all the entries in the CAM
		#pragma HLS UNROLL
        	if ((this->cam[i].valid == 1) && (bufferId == this->cam[i].bufferId)) {
        		// The entry is valid and the buffer ID matches
                return (this->cam[i].sessionId);
            }
    }
    return -1;
}


/*****************************************************************************
 * @brief Session Id Server (sis).
 *
 * @ingroup trif
 *
 * @param[in]  siAcpt_WrEntry, a new session entry from the Accept process.
 * @param[in]  siTxp_BufId,    a new buffer ID from the TxPath process.
 * @param[out] soTxp_SessId,   the session ID corresponding to the incoming
 *                              buffer ID.
 *
 * @return Nothing.
 ******************************************************************************/
void pSessionIdServer (
        stream<SessionIdCamEntry>	&siAcpt_Entry,
        stream<ap_uint<4> > 		&siTxp_BuffId,
        stream<ap_uint<16> > 		&soTxp_SessId)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS INLINE region
	#pragma HLS PIPELINE II=1  // enable_flush

    //OBSOLETE-20181001 static enum uit_state {UIT_IDLE, UIT_RX_READ, UIT_TX_READ, UIT_WRITE, UIT_CLEAR} tcp_ip_table_state;

	//-- LOCAL VARIABLES ------------------------------------------------------
	static SessionIdCam  sessionIdCam;
	#pragma HLS array_partition variable=sessionIdCam.cam complete

    SessionIdCamEntry 	inEntry;
    ap_uint<4> 			inBufferId;
    ap_uint<16> 		inSessionId;

    //OBSOLETE-20181001 static bool rdWrswitch = true;

    if (!siTxp_BuffId.empty() && !soTxp_SessId.full() ) { /*OBSOLETE-20181001 && !rdWrswitch*/
        siTxp_BuffId.read(inBufferId);
        inSessionId = sessionIdCam.search(inBufferId);
        soTxp_SessId.write(inSessionId);
        //rdWrswitch = !rdWrswitch;
    } else if (!siAcpt_Entry.empty()) {	/*OBSOLETE-20181001 && rdWrswitch*/
        siAcpt_Entry.read(inEntry);
        sessionIdCam.write(inEntry);
        //OBSOLETE-20181001 rdWrswitch = !rdWrswitch;
    }

    //rdWrswitch = !rdWrswitch;
}


/*****************************************************************************
 * @brief Open/Close a Connection (occ).
 *
 * @ingroup trif
 *
 * @param[in]  siTOE_OpnSts,  open connection status from TOE.
 * @param[out] soTOE_OpnReq,  open connection request to TOE.
 * @param[in]  siTOE_WrSts,   write status from TOE.
 * @param[out] soTOE_ClsReq,  close connection request to TOE.
 *
 * @return Nothing.
 *
 * @note  This code is not executed. It is added here to terminate every HLS
 * 			stream of the module.
 ******************************************************************************/
void pOpenCloseConn(
		stream<TcpOpnSts>		&siTOE_OpnSts,
        stream<TcpOpnReq>		&soTOE_OpnReq,
		stream<TcpWrSts>		&siTOE_WrSts,
        stream<TcpClsReq>		&soTOE_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1	//OBSOLETE-20180925 #pragma HLS INLINE OFF

    TcpOpnSts	newConn;
    TcpOpnReq	sockAddr;

    if (!siTOE_OpnSts.empty() && !soTOE_OpnReq.full() && !soTOE_ClsReq.full()) {
        siTOE_OpnSts.read(newConn);
        sockAddr.addr = 0x0b010101;
        sockAddr.port = 0x3412;
        soTOE_OpnReq.write(sockAddr);
        if (newConn.success) {
            soTOE_ClsReq.write(newConn.sessionID);
            //closePort.write(occ_sockAddr.ip_port);
        }
    }

    if (!siTOE_WrSts.empty()) {	//Make Checks
    	siTOE_WrSts.read();
    }
}


/*****************************************************************************
 * @brief Start listening (lsn) for incoming connections on a specific port.
 *
 * @ingroup trif
 *
 * @param[in]  siTOE_LsnAck,  listen port acknowledge from TOE.
 * @param[out] soTOE_LsnReq,  listen port request to TOE.
 *
 * @return Nothing.
 ******************************************************************************/
void pListen(
		 stream<TcpLsnAck>		&siTOE_LsnAck,
		 stream<TcpLsnReq>		&soTOE_LsnReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1		//OBSOLETE-20180925 #pragma HLS INLINE OFF

    static bool lsn_ack = false;
    static bool lsn_req = false;

	#pragma HLS RESET variable=lsn_ack
	#pragma HLS RESET variable=lsn_ack

    // Bind and listen on port #80 at startup
    if (!lsn_ack && !lsn_req && !soTOE_LsnReq.full()) {
        soTOE_LsnReq.write(80);
        lsn_req = true;
    } else if (lsn_req && !siTOE_LsnAck.empty()) {
    	// Check if the request for listening on port was successful, otherwise try again
        siTOE_LsnAck.read(lsn_ack);
        lsn_req = false;
    }
}


/*****************************************************************************
 * @brief Accept a new connection (acpt).
 *
 * @ingroup trif
 *
 * @param[in]  siTOE_Notif,	a new connection notification from TOE.
 * @param[out] soTOE_RdReq, a read request to TOE.
 * @param[out] soSis_Entry, a new entry for the SessionIdServer process.
 *
 * @return Nothing.
 ******************************************************************************/
void pAccept(
        stream<TcpNotif>			&siTOE_Notif,
        stream<TcpRdReq>			&soTOE_RdReq,
        stream<SessionIdCamEntry>	&soSis_Entry)
{
	 //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1		//#pragma HLS INLINE OFF

    static ap_uint<16> 	acpt_sessId  = 99;
    static bool 		acpt_firstWr = true;

    static enum FSM_STATE { FSM_LISTEN_NOTIFICATION = 0, FSM_WRITE_SESSION } acpt_fsmState;

    TcpNotif notif;

    switch(acpt_fsmState) {

    	case FSM_LISTEN_NOTIFICATION:
            if (!siTOE_Notif.empty() && !soTOE_RdReq.full()) {
                siTOE_Notif.read(notif);

                if (notif.length != 0) {
                    soTOE_RdReq.write(TcpRdReq(notif.sessionID, notif.length));
                }

                if(acpt_sessId != notif.sessionID ||  acpt_firstWr) {
                    acpt_sessId = notif.sessionID;
                    acpt_fsmState = FSM_WRITE_SESSION;
                }
            }
        break;

        case FSM_WRITE_SESSION:
            if (!soSis_Entry.full()) {
                if(acpt_firstWr) {
                    soSis_Entry.write(SessionIdCamEntry(acpt_sessId, 1, 0));
                    acpt_firstWr = !acpt_firstWr;
                } else
                    soSis_Entry.write(SessionIdCamEntry(acpt_sessId, 1, 1));

                acpt_fsmState = FSM_LISTEN_NOTIFICATION;
            }
        break;
    }
}


/*** OBSOLETE-20180925 *** Was moved into 'pOpenCloseConn' **********
void tai_check_tx_status(
        stream<ap_int<17> >& txStatus)
{
	#pragma HLS PIPELINE II=1 	//#pragma HLS INLINE OFF

    if (!txStatus.empty()) //Make Checks
    {
        txStatus.read();
    }
}
*********************************************************************/


/*****************************************************************************
 * @brief Rx Path (rxp) - From TOE to ROLE.
 *
 * @ingroup trif
 *
 * @param[in]  siTOE_Data,	data from TOE.
 * @param[in]  siTOE_Meta,	metadata from TOE.
 * @param[out] soROL_Data,  data to ROLE.
 *
 * @return Nothing.
 ******************************************************************************/
void pRxPath(
        stream<TcpWord>		&siTOE_Data,
		stream<TcpMeta>		&siTOE_Meta,
        stream<TcpWord>		&soROL_Data)
{
   //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

    ap_uint<16> 		sessionID;
    TcpWord 			currWord;
    //OBSOLETE-20180925 static ap_uint<16> 	old_session_id = 0;

    static enum FsmState { FSM_IDLE = 0, FSM_STREAM } rxp_fsmState;

    switch (rxp_fsmState)
    {
        case FSM_IDLE:
            if (!siTOE_Meta.empty()) {
            	siTOE_Meta.read(sessionID);
                rxp_fsmState = FSM_STREAM;
            }
        break;
        case FSM_STREAM:
            if (!siTOE_Data.empty() && !soROL_Data.full()) {
                siTOE_Data.read(currWord);
                soROL_Data.write(currWord);

                if (currWord.tlast)
                    rxp_fsmState = FSM_IDLE;
            }
        break;
    }
}


/*****************************************************************************
 * @brief Tx Path (txp) - From ROLE to TOE.
 *
 * @ingroup trif
 *
 * @param[in]  siROL_Data,	data from ROLE.
 * @param[out] soTOE_Data,	data to TOE.
 * @param[out] soTOE_Meta,	metadata to to TOE.
 * @param[in]  siSis_SessId,session ID from  the SessionIdServer process.
 * @param[out] soSis_BufId, buffer ID to the SessionIdTable process.
 *
 * @return Nothing.
 ******************************************************************************/
void pTxPath(
        stream<TcpWord>		 &siROL_Data,
		stream<TcpWord>      &soTOE_Data,
		stream<TcpMeta> 	 &soTOE_Meta,
		stream<ap_uint<16> > &siSis_SessId,
        stream<ap_uint<4> >	 &soSis_BufId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

	//-- LOCAL STREAMS --------------------------------------------------------
	static stream<TcpWord>		sFifo_Data("sFifo_Data");
    #pragma HLS STREAM variable=sFifo_Data depth=1024


    TcpWord currWordIn;
    TcpWord currWordOut;

    //-- FSM #1: INPUT STREAMING ------
    static enum FsmInState {FSM_IDLE_IN = 0, FSM_STREAM_IN} txp_fsmInState;

    switch (txp_fsmInState) {

    	case FSM_IDLE_IN:
            if(!siROL_Data.empty() && !soSis_BufId.full() && !sFifo_Data.full()) {
            	siROL_Data.read(currWordIn);
                sFifo_Data.write(currWordIn);
                soSis_BufId.write(1);

                if(!currWordIn.tlast)
                    txp_fsmInState = FSM_STREAM_IN;
            }
        break;

    	case FSM_STREAM_IN:
            if (!siROL_Data.empty() && !sFifo_Data.full()) {
                siROL_Data.read(currWordIn);
                sFifo_Data.write(currWordIn);

                if(currWordIn.tlast)
                    txp_fsmInState = FSM_IDLE_IN;
            }
        break;
    }


    //-- FSM #2: OUTPUT STREAMING -----
    static enum FsmOutState {FSM_IDLE_OUT = 0, FSM_STREAM_OUT} txp_fsmOutState;

    switch (txp_fsmOutState) {

        case FSM_IDLE_OUT:
            if(!sFifo_Data.empty() && !soTOE_Data.full() && !soTOE_Meta.full() && !siSis_SessId.empty()) {
                sFifo_Data.read(currWordOut);
                soTOE_Data.write(currWordOut);
                soTOE_Meta.write(siSis_SessId.read());

                if(!currWordOut.tlast)
                    txp_fsmOutState = FSM_STREAM_OUT;
            }
        break;

        case FSM_STREAM_OUT:
            if (!sFifo_Data.empty() && !soTOE_Data.full()) {
                sFifo_Data.read(currWordOut);
                soTOE_Data.write(currWordOut);

                if(currWordOut.tlast)
                    txp_fsmOutState = FSM_IDLE_OUT;
            }
        break;
    }

}


/*** OBSOLETE-20180925 *******************************************
void tai_app_to_buf(
        stream<axiWord>		&vFPGA_tx_data,
        stream<ap_uint<4> >	&q_buffer_id,
        stream<axiWord>		&buff_data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
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
********************************************************************/

/*** OBSOLETE-20180925 *******************************************
void tai_app_to_net(
        stream<axiWord>&        buff_data,
        stream<ap_uint<16> >&   txMetaData,
        stream<axiWord>&        oTxData,
        stream<ap_uint<16> >&   r_session_id)
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
********************************************************************/


/*****************************************************************************
 * @brief   Main process of the TCP Role Interface
 * @ingroup tcp_role_if
 *
 * @param[in]  siROL_This_Data   TCP data stream from the ROLE.
 * @param[out] soTHIS_Rol_Data   TCP data stream to the ROLE.
 * @param[in]  siTOE_This_Data   Data path from the TOE.
 * @param[in]  siTOE_This_Meta   Metadata from the TOE.
 * @param[out] soTHIS_Toe_Data   Data path to the TOE.
 * @param[out] soTHIS_Toe_Meta   Metadata to the TOE.
 * @param[in]  siTOE_This_OpnSts Open connection status from TOE.
 * @param[out] soTHIS_Toe_OpnReq Open connection request to TOE.
 * @param[in]  siTOE_This_LsnAck Listen port acknowledge from TOE.
 * @param[out] soTHIS_Toe_LsnReq Listen port request to TOE.
 * @param[in]  siTOE_This_Notif  Read notification from TOE.
 * @param[out] soTHIS_Toe_RdReq  Read request to TOE.
 * @param[in]  siTOE_This_WrSts  Write status from TOE.
 * @param[out] soTHIS_Toe_ClsReq Close connection request to TOE.
 *
 * @return Nothing.
 *
 * @remark     : Session id is only updated if a new connection is established.
 *                Therefore, the role does not have to always return the same
 *                amount of segments received.
 *****************************************************************************/
void tcp_role_if(

        //------------------------------------------------------
        //-- ROLE / This / Tcp Interfaces
        //------------------------------------------------------
        stream<TcpWord>     &siROL_This_Data,
        stream<TcpWord>     &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- TOE / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<TcpWord>     &siTOE_This_Data,
        stream<TcpMeta>		&siTOE_This_Meta,
        stream<TcpWord>     &soTHIS_Toe_Data,
        stream<TcpMeta>		&soTHIS_Toe_Meta,

        //------------------------------------------------------
        //-- TOE / This / Open-Connection Interfaces
        //------------------------------------------------------
        stream<TcpOpnSts>	&siTOE_This_OpnSts,
        stream<TcpOpnReq>	&soTHIS_Toe_OpnReq,

        //------------------------------------------------------
        //-- TOE / This / Listen-Port Interfaces
        //------------------------------------------------------
        stream<TcpLsnAck>   &siTOE_This_LsnAck,
        stream<TcpLsnReq>	&soTHIS_Toe_LsnReq,

        //------------------------------------------------------
        //-- TOE / This / Read-Request Interfaces
        //------------------------------------------------------
        stream<TcpNotif>	&siTOE_This_Notif,
        stream<TcpRdReq> 	&soTHIS_Toe_RdReq,

        //------------------------------------------------------
        //-- TOE / This / Write-Status Interface
        //------------------------------------------------------
        stream<TcpWrSts>	&siTOE_This_WrSts,

        //------------------------------------------------------
        //-- TOE / This / Close-Connection Interface
        //------------------------------------------------------
        stream<TcpClsReq>	&soTHIS_Toe_ClsReq)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS resource core=AXI4Stream variable=siROL_This_Data   metadata="-bus_bundle siROL_This_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Rol_Data   metadata="-bus_bundle soTHIS_Rol_Data"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Data   metadata="-bus_bundle siTOE_This_Data"
    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Meta   metadata="-bus_bundle siTOE_This_Meta"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_Data   metadata="-bus_bundle soTHIS_Toe_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_Meta   metadata="-bus_bundle soTHIS_Toe_Meta"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_OpnSts metadata="-bus_bundle siTOE_This_OpnSts"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_OpnReq metadata="-bus_bundle soTHIS_Toe_OpnReq"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_LsnAck metadata="-bus_bundle siTOE_This_LsnAck"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_LsnReq metadata="-bus_bundle soTHIS_Toe_LsnReq"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Notif  metadata="-bus_bundle siTOE_This_Notif"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_RdReq  metadata="-bus_bundle soTHIS_Toe_RdReq"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_WrSts  metadata="-bus_bundle siTOE_This_WrSts"

    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_ClsReq metadata="-bus_bundle soTHIS_Toe_ClsReq"

    #pragma HLS DATA_PACK variable=siTOE_This_OpnSts
    #pragma HLS DATA_PACK variable=soTHIS_Toe_OpnReq

    #pragma HLS DATA_PACK variable=siTOE_This_Notif
    #pragma HLS DATA_PACK variable=soTHIS_Toe_RdReq

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<SessionIdCamEntry>   sAcptToSis_Entry("sAcptToSis_Entry");
    #pragma HLS STREAM             variable=sAcptToSis_Entry depth=1

    static stream<ap_uint<4> >              sTxpToSis_BufId ("sTxpToSis_BufId");
    #pragma HLS STREAM             variable=sTxpToSis_BufId depth=1

    static stream<ap_uint<16> >             sSisToTxp_SessId("sSisToTxp_SessId");
    #pragma HLS STREAM             variable=sSisToTxp_SessId depth=1

    //OBSOLETE-20180925 static stream<axiWord>                  buff_data       ("buff_data");
    //OBSOLETE-20180925 #pragma HLS STREAM             variable=buff_data depth=1024

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pOpenCloseConn(siTOE_This_OpnSts, soTHIS_Toe_OpnReq, siTOE_This_WrSts , soTHIS_Toe_ClsReq);

    pListen(siTOE_This_LsnAck, soTHIS_Toe_LsnReq);

    pAccept(siTOE_This_Notif, soTHIS_Toe_RdReq, sAcptToSis_Entry);

    //OBSOLETE-20180925 tai_check_tx_status(siTOE_This_WrSts);

    pSessionIdServer(sAcptToSis_Entry, sTxpToSis_BufId, sSisToTxp_SessId);

    pRxPath(siTOE_This_Data, siTOE_This_Meta, soTHIS_Rol_Data);


    //OBSOLETE-20180925 tai_app_to_buf(siROL_This_Data, sTxpToSis_BufId, buff_data);

    //OBSOLETE-20180925 tai_app_to_net(buff_data, soTHIS_Toe_Meta, soTHIS_Toe_Data, sSisToTxp_SessId);

    pTxPath(siROL_This_Data, soTHIS_Toe_Data, soTHIS_Toe_Meta, sSisToTxp_SessId,sTxpToSis_BufId);
}

