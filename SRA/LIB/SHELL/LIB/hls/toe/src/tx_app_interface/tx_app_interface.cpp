/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.


All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : tx_app_interface.cpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "tx_app_interface.hpp"
#include "../../test/test_toe_utils.hpp"

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TAA_TRACE | TAS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TAi"

#define TRACE_OFF   0x0000
#define TRACE_SML  1 <<  1
#define TRACE_SLG  1 <<  2
#define TRACE_MWR  1 <<  3
#define TRACE_TAA  1 <<  4
#define TRACE_TAS  1 <<  5
#define TRACE_TAT  1 <<  6
#define TRACE_TASH 1 <<  7
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * @brief A 2-to-1 Stream multiplexer.
 * ***************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so)
{
   //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*****************************************************************************
 * @brief Tx Application Accept (Taa).
 *
 * @param[in]  siTRIF_OpnReq,        Open connection request from TCP Role I/F (TRIF).
 * @param[out] soTRIF_OpnRep,        Open connection reply to [TRIF].
 * @param[]    siTRIF_ClsReq,        Close connection request from [TRIF].
 * @param[out] soSLc_SessLookupReq,  Lookup request SessionLookupController (SLc).
 * @param[in]  siSLc_SessLookupRep,  Reply from [SLc]
 * @param[out] soPRt_GetFreePortReq, Free port request to PortTable {PRt).
 * @param[out] siPRt_GetFreePortRep, Free port reply from [PRt].
 * @param[in]  siRXe_SessOpnSts,     Session open status from [RXe].
 * @param[out  soSTt_AcceptStateQry, Session state query to StateTable (STt).
 * @param[in]  siSTt_AcceptStateRep, Session state reply from [STt].
 * @param[out] soEVe_Event,          Event to EventEngine (EVe).
 * @param[in]  siTIm_Notif,          Notification from Timers (TIm).
 * @param[in]  piMMIO_IpAddr         IP4 Address from MMIO.
 *
 * @details
 *  This process performs the creation and tear down of the active connections.
 *   Active connections are the ones opened by the FPGA as client and they make
 *   use of dynamically assigned or ephemeral ports in the range 32,768 to
 *   65,535. The operations performed here are somehow similar to the 'accept'
 *   and 'close' system calls.
 *  The IP tuple of the new connection to open is read from 'siTRIF_OpnReq'.
 *   Next, a free port number is requested from the PortTable and a SYN event
 *   is forwarded to the TxeEngine in order to initiate a 3-way handshake with
 *   the remote host.
 *  The [APP] is notified upon a connection establishment success or fail via a
 *   status message delivered over 'soTRIF_OpnRep'. Upon success, the message
 *   will provide the [APP] with the 'SessionId' of the new connection.
 *  Sending the 'SessionId' over the 'siTRIF_ClsReq' interface, will tear-down
 *   the connection.
 *
 ******************************************************************************/
void pTxAppAccept(
        stream<LE_SockAddr>         &siTRIF_OpnReq,
        stream<OpenStatus>          &soTRIF_OpnRep,
        stream<AppClsReq>           &siTRIF_ClsReq,
        stream<LE_SocketPair>       &soSLc_SessLookupReq,
		stream<SessionLookupReply>  &siSLc_SessLookupRep,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<TcpPort>             &siPRt_GetFreePortRep,
        stream<StateQuery>          &soSTt_AcceptStateQry,
        stream<SessionState>        &siSTt_AcceptStateRep,
        stream<OpenStatus>          &siRXe_SessOpnSts,
        stream<Event>               &soEVe_Event,
        stream<OpenStatus>          &siTIm_Notif,
        LE_Ip4Address                piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Taa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {TAA_IDLE=0, TAA_GET_FREE_PORT, TAA_CLOSE_CONN } \
                                taa_fsmState=TAA_IDLE;
    #pragma HLS RESET  variable=taa_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SessionId    taa_closeSessionID;

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<LE_SockAddr>  taa_localFifo    ("taa_localFifo");
    #pragma HLS stream variable=taa_localFifo    depth=4

    OpenStatus  openSessStatus;

    switch (taa_fsmState) {

    case TAA_IDLE:
        if (!siTRIF_OpnReq.empty() && !soPRt_GetFreePortReq.full()) {
        	assessSize(myName, taa_localFifo, "taa_localFifo", 4);  // [FIXME-Use constant for the length]
            taa_localFifo.write(siTRIF_OpnReq.read());
            soPRt_GetFreePortReq.write(1);
            taa_fsmState = TAA_GET_FREE_PORT;
        }
        else if (!siSLc_SessLookupRep.empty()) {
            // Read the session lookup reply and check its state
            SessionLookupReply session = siSLc_SessLookupRep.read();
            if (session.hit) {
                soEVe_Event.write(Event(SYN_EVENT, session.sessionID));
                soSTt_AcceptStateQry.write(StateQuery(session.sessionID, SYN_SENT, QUERY_WR));
            }
            else {
                // Tell the [APP ]that the open connection failed
                soTRIF_OpnRep.write(OpenStatus(session.sessionID, FAILED_TO_OPEN_SESS));
            }
        }
        else if (!siRXe_SessOpnSts.empty()) {
            // Read the open session result from [RXe]
            siRXe_SessOpnSts.read(openSessStatus);
            // And forward it to [TRIF]
            //  [TODO-We should check if [TRIF] is actually waiting for such a status]
            soTRIF_OpnRep.write(openSessStatus);
        }
        else if (!siTIm_Notif.empty()) {
            soTRIF_OpnRep.write(siTIm_Notif.read());
        }
        else if(!siTRIF_ClsReq.empty()) {
            siTRIF_ClsReq.read(taa_closeSessionID);
            soSTt_AcceptStateQry.write(StateQuery(taa_closeSessionID));
            taa_fsmState = TAA_CLOSE_CONN;
        }
        break;

    case TAA_GET_FREE_PORT:
        if (!siPRt_GetFreePortRep.empty() && !soSLc_SessLookupReq.full()) {
            TcpPort     freePort     = siPRt_GetFreePortRep.read();
            LE_SockAddr leServerAddr = taa_localFifo.read();
            soSLc_SessLookupReq.write(LE_SocketPair(
                          LE_SockAddr(piMMIO_IpAddr,     byteSwap16(freePort)),
                          LE_SockAddr(leServerAddr.addr, leServerAddr.port)));
            taa_fsmState = TAA_IDLE;
        }
        break;

    case TAA_CLOSE_CONN:
        if (!siSTt_AcceptStateRep.empty()) {
            SessionState state = siSTt_AcceptStateRep.read();
            //TODO might add CLOSE_WAIT here???
            if ((state == ESTABLISHED) || (state == FIN_WAIT_2) || (state == FIN_WAIT_1)) {
                // [TODO - Why if FIN already SENT]
                soSTt_AcceptStateQry.write(StateQuery(taa_closeSessionID, FIN_WAIT_1, QUERY_WR));
                soEVe_Event.write(Event(FIN_EVENT, taa_closeSessionID));
            }
            else {
                soSTt_AcceptStateQry.write(StateQuery(taa_closeSessionID, state, QUERY_WR));
            }
            taa_fsmState = TAA_IDLE;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Tx Application Status Handler (Tash).
 *
 * @param[in]  siMEM_TxP_WrSts, Tx memory write status from [MEM].
 * @param[in]  siEmx_Event,     Event from the EventMultiplexer (Emx).
 * @param[out] soTSt_PushCmd,   Push command to TxSarTable (TSt).
 * @param[out] soEVe_Event,     Event to EventEngine (EVe).
 *
 * @details
 *  This process waits and reads events from [Emx]. If the incoming event is a
 *   'TX_EVENT', the process will wait for a (or possibly two) memory status
 *   from [MEM] and will forward command to update the 'TxApplicationPointer'
 *   of the [TSt].
 *  Whatever the received event, it is always forwarded to [EVe].
 ******************************************************************************/
void pTxAppStatusHandler(
        stream<DmSts>             &siMEM_TxP_WrSts,
        stream<Event>             &siEmx_Event,
        stream<TAiTxSarPush>      &soTSt_PushCmd,
        stream<Event>             &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Tash");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0=0, S1, S2 } tash_fsmState=S0;
    #pragma HLS reset             variable=tash_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static Event      ev;

    switch (tash_fsmState) {
    case S0: //-- Read the Event
        if (!siEmx_Event.empty()) {
            siEmx_Event.read(ev);
            if (ev.type == TX_EVENT) {
                tash_fsmState = S1;
            }
            else {
                soEVe_Event.write(ev);
            }
            if (DEBUG_LEVEL & TRACE_TASH) {
                printInfo(myName, "Received event \'%s\' from [Emx].\n",
                                  myEventTypeToString(ev.type));
            }
        }
        break;
    case S1: //-- Read the Memory Write Status #1 (this might also be the last)
        if (!siMEM_TxP_WrSts.empty()) {
            DmSts status = siMEM_TxP_WrSts.read();
            if (status.okay) {
            	ap_uint<17> txAppPtr = ev.address + ev.length;
				/*** OBSOLETE_20191212 **********
				if (tempLength <= 0x10000) {
					if (status.okay) {
						soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, tempLength.range(15, 0))); // App pointer update, pointer is released
						soEVe_Event.write(ev);
					}
					if (DEBUG_LEVEL & TRACE_TASH) {
						printInfo(myName, "Received 1st TXMEM write status = %d.\n", status.okay.to_int());
					}
					tash_fsmState = S0;
				}
				else {
					tash_fsmState = S2;
				}
				*********************************/
            	if (txAppPtr[16] == 1) {
            		// The TCP buffer wrapped around
            		tash_fsmState = S2;
            	}
            	else {
            		// Update the 'txAppPtr' of the TX_SAR_TABLE
					soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, txAppPtr.range(15, 0)));
					// Forward event to [EVe] which will signal [TXe]
					soEVe_Event.write(ev);
					if (DEBUG_LEVEL & TRACE_TASH) {
						printInfo(myName, "Received TXMEM write status = %d.\n", status.okay.to_int());
					}
					tash_fsmState = S0;
            	}
            }
        }
        break;
    case S2: //-- Read the Memory Write Status #2
        if (!siMEM_TxP_WrSts.empty()) {
            DmSts status = siMEM_TxP_WrSts.read();
            TxBufPtr txAppPtr = (ev.address + ev.length);
            if (status.okay) {
                // Update the 'txAppPtr' of the TX_SAR_TABLE
                soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, txAppPtr));
                // Forward event to [EVe] which will signal [TXe]
                soEVe_Event.write(ev);
                if (DEBUG_LEVEL & TRACE_TASH) {
                    printInfo(myName, "Received TXMEM write status = %d (this was a split access).\n", status.okay.to_int());
                }
            }
            tash_fsmState = S0;
        }
        break;

    } // End of: switch
}

/*****************************************************************************
 * @brief Tx Application Table (Tat).
 *
 * @param[in]  siTSt_PushCmd,   Push command from TxSarTable (TSt).
 * @param[in]  siTas_AccessQry, Access query from TxAppStream (Tas).
 * @param[out] soTAs_AccessRep, Access reply to [Tas].
 *
 * @details
 *  This table keeps tack of the Tx ACK numbers and Tx memory pointers.
 *
 ******************************************************************************/
void pTxAppTable(
        stream<TStTxSarPush>      &siTSt_PushCmd,
        stream<TxAppTableQuery>   &siTas_AccessQry,
        stream<TxAppTableReply>   &siTas_AccessRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Tat");

    //-- STATIC ARRAYS --------------------------------------------------------
    static TxAppTableEntry          TX_APP_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_APP_TABLE inter false
    #pragma HLS reset      variable=TX_APP_TABLE

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TStTxSarPush      ackPush;
    TxAppTableQuery   txAppUpdate;

    if (!siTSt_PushCmd.empty()) {
        siTSt_PushCmd.read(ackPush);
        if (ackPush.init) {
            // At init this is actually not_ackd
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd-1;
            TX_APP_TABLE[ackPush.sessionID].mempt = ackPush.ackd;
        }
        else {
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd;
        }
    }
    else if (!siTas_AccessQry.empty()) {
        siTas_AccessQry.read(txAppUpdate);
        if(txAppUpdate.write) {
            TX_APP_TABLE[txAppUpdate.sessId].mempt = txAppUpdate.mempt;
        }
        else {
            siTas_AccessRep.write(TxAppTableReply(txAppUpdate.sessId,
                                  TX_APP_TABLE[txAppUpdate.sessId].ackd,
								  TX_APP_TABLE[txAppUpdate.sessId].mempt));
        }
    }
}

/*******************************************************************************
 * @brief Stream Length Generator (Slg)
 *
 * @param[in]  siTRIF_Data,      TCP data stream from TRIF.
 * @param[out] soMwr_Data,       TCP data stream to MemoryWriter (Mwr).
 * @param[out] soSml_SegLen,     The length of the TCP segment to StreamMetaLoader (Sml);
 *
 * @details
 *   This process generates the length of the incoming TCP segment from [APP]
 *    while forwarding that same data stream to the MemoryWriter (Mwr).
 *******************************************************************************/
void pStreamLengthGenerator(
        stream<AppData>     &siTRIF_Data,
        stream<AxiWord>     &soMwr_Data,
        stream<TcpSegLen>   &soSml_SegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Slg");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static TcpSegLen           slg_segLen=0;
    #pragma HLS RESET variable=slg_segLen

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord currWord = AxiWord(0, 0xFF, 0);

    if (!siTRIF_Data.empty()) {
        siTRIF_Data.read(currWord);
        soMwr_Data.write(currWord);
        slg_segLen += keepMapping(currWord.tkeep);
        if (currWord.tlast) {
            assessSize(myName, soSml_SegLen, "soSml_SegLen", 32);  // [FIXME-Use constant for the length]
            soSml_SegLen.write(slg_segLen);
            if (DEBUG_LEVEL & TRACE_SLG) {
                printInfo(myName, "Received end-of-segment. SegLen=%d\n", slg_segLen.to_int());
            }
            slg_segLen = 0;
        }
    }
}

/******************************************************************************
 * @brief Stream Metadata Loader (Sml)
 *
 * @param[in]  siTRIF_Meta,        The TCP session Id this segment belongs to.
 * @param[out] soTRIF_DSts,        The TCP write transfer status to TRIF.
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from StateTable (STt).
 * @param[out] soTat_AccessReq,    Access request to TxAppTable (Tat).
 * @param[in]  siTat_AccessRep,    Access reply from [Tat]
 * @param[in]  siSlg_SegLen,       The TCP segment length from SegmentLengthGenerator (slg).
 * @param[out] soMwr_SegMeta,      Segment metadata to MemoryWriter (Mwr).
 * @param[out] soEmx_Event,        Event to EventMultiplexer (Emx).
 *
 * @details
 *  Reads the request from the application and loads the necessary metadata for
 *   transmitting this segment. After metadata is received from [TRIF], the
 *   state of the connection is checked and the segment memory pointer is loaded
 *   into the APP pointer table. Next, the segment metadata are forwarded to the
 *   MemoryWriter (Mwr) process which will write the segment into the DDR4
 *   buffer memory.
 * The FSM of this process decides if the segment is written to the TX buffer o
 *  is discarded.
 *
 *  [FIXME-TODO: Implement TCP_NODELAY]
 *
 ******************************************************************************/
void pStreamMetaLoader(
        stream<AppMeta>             &siTRIF_Meta,
        stream<AppWrSts>            &soTRIF_DSts,
        stream<TcpSessId>           &soSTt_SessStateReq,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<TxAppTableQuery>     &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<SegMemMeta>          &sSmlToMwr_SegMeta,
        stream<Event>               &soEmx_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { READ_REQUEST=0,  READ_META } \
											mdl_fsmState = READ_REQUEST;
    #pragma HLS reset            variable = mdl_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpSessId tcpSessId;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TxAppTableReply  txAppTableReply;
    SessionState     sessState;
    TcpSegLen        segLen;

    switch(mdl_fsmState) {

    case READ_REQUEST:
        if (!siTRIF_Meta.empty()) {
            // Read the session ID
            siTRIF_Meta.read(tcpSessId);
            // Request state of the session
            assessSize(myName, soSTt_SessStateReq, "soSTt_SessStateReq", 2);  // [FIXME-Use constant for the length]
            soSTt_SessStateReq.write(tcpSessId);
            // Request the value of ACK and MemPtr from TxAppTable
            assessSize(myName, soTat_AccessReq, "soTat_AccessReq", 2);  // [FIXME-Use constant for the length]
            soTat_AccessReq.write(TxAppTableQuery(tcpSessId));
            if (DEBUG_LEVEL & TRACE_SML) {
                printInfo(myName, "Received new Tx request for session %d.\n", tcpSessId.to_int());
            }
            mdl_fsmState = READ_META;
        }
        break;

    case READ_META:
        if (!siTat_AccessRep.empty() && !siSTt_SessStateRep.empty() && !siSlg_SegLen.empty()) {
            siSTt_SessStateRep.read(sessState);
            siTat_AccessRep.read(txAppTableReply);
            siSlg_SegLen.read(segLen);  // [FIXME - Why not use the length field of
            TcpSegLen maxWriteLength = (txAppTableReply.ackd - txAppTableReply.mempt) - 1;

            /*** [TODO - TCP_NODELAY] ******************
            #if (TCP_NODELAY)
                ap_uint<16> usedLength = ((ap_uint<16>) writeSar.mempt - writeSar.ackd);
                ap_uint<16> usableWindow = 0;
                if (writeSar.min_window > usedLength) {
                    usableWindow = writeSar.min_window - usedLength;
                }
            #endif
            *******************************************/

            if (sessState != ESTABLISHED) {
                // Drop this segment
                assessSize(myName, sSmlToMwr_SegMeta, "sSmlToMwr_SegMeta", 128);  // [FIXME-Use constant for the length]
                sSmlToMwr_SegMeta.write(SegMemMeta(CMD_DROP));
                // Notify [APP] about the fail
                soTRIF_DSts.write(AppWrSts(STS_KO, ERROR_NOCONNCECTION));
                printError(myName, "Session %d is not established. Current session state is \'%s\'.\n",
                           tcpSessId.to_uint(), SessionStateString[sessState].c_str());
            }
            else if (segLen > maxWriteLength) {
                sSmlToMwr_SegMeta.write(SegMemMeta(CMD_DROP));
                // Notify [APP] about fail
                soTRIF_DSts.write(AppWrSts(STS_KO, ERROR_NOSPACE));
                printError(myName, "There is no TxBuf memory space available for session %d.\n",
                           tcpSessId.to_uint());
            }
            else { //-- Session is ESTABLISHED and segLen <= maxWriteLength
            	// Forward the metadata to the SegmentMemoryWriter (Mwr)
                sSmlToMwr_SegMeta.write(SegMemMeta(tcpSessId, txAppTableReply.mempt, segLen));
                // Forward data status back to [APP]
                soTRIF_DSts.write(AppWrSts(STS_OK, segLen));
                // Notify [TXe] about data to be sent via an event to [EVe]
                assessSize(myName, soEmx_Event, "soEmx_Event", 2);  // [FIXME-Use constant for the length]
                soEmx_Event.write(Event(TX_EVENT, tcpSessId, txAppTableReply.mempt, segLen));
                // Update the 'txMemPtr' in TxAppTable
                soTat_AccessReq.write(TxAppTableQuery(tcpSessId, txAppTableReply.mempt + segLen));
            }
            mdl_fsmState = READ_REQUEST;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Memory Writer (Mwr)
 *
 * @param[in]  siSlg_Data,      TCP data stream from SegmentLengthGenerator (Slg).
 * @param[in]  siSml_SegMeta,   Segment memory metadata from StreamMetaLoader (Sml).
 * @param[out] soMEM_TxP_WrCmd, Tx memory write command to [MEM].
 * @param[out] soMEM_TxP_Data,  Tx memory data to [MEM].
 *
 * @details
 *   Writes the incoming TCP segment into the external DRAM, unless the state
 *   machine of the MetaDataLoader (Sml) decides to drop the segment.
 *
 *******************************************************************************/
void pMemWriter(
        stream<AxiWord>         &siSlg_Data,
        stream<SegMemMeta>      &siSml_SegMeta,
        stream<DmCmd>           &soMEM_TxP_WrCmd,
        stream<AxiWord>         &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "Mwr");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { MWR_IDLE=0, MWR_IDLEbis, S1, S2, S3, S4, S5 } \
								 smw_fsmState=MWR_IDLE;
    #pragma HLS RESET variable = smw_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SegMemMeta    segMemMeta;
    static bool          segMustBeSplitted;
    static DmCmd         dmCmd;
    static AxiWord       currWord;
    static TcpSegLen     breakLen;
    // static TcpSegLen     splitLen1;
    // static TcpSegLen     splitLen2;

    static uint8_t       lengthBuffer;     // OBSOLETE-20190907  = 0;
    static ap_uint<3>    accessResidue;    // OBSOLETE-20190907 = 0;
    static uint32_t      txAppWordCounter; // OBSOLETE-20190907 = 0;

    switch (smw_fsmState) {

    case MWR_IDLE:
        if (!siSml_SegMeta.empty() && !soMEM_TxP_WrCmd.full()) {
            siSml_SegMeta.read(segMemMeta);

            if (!segMemMeta.drop) {

                // Build a memory address for this segment in the upper 2GB [FIXME-Must be configurable]
                ap_uint<32> memSegAddr; // [TODO-typedef+33-bits]
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = segMemMeta.sessId(13, 0);
                memSegAddr(15,  0) = segMemMeta.addr;

                // Build a data mover command for this segment
                dmCmd = DmCmd(memSegAddr, segMemMeta.len);

                if ((dmCmd.saddr.range(15, 0) + dmCmd.bbt) > 65536) {
                    segMustBeSplitted = true;
                    printInfo(myName, "Segment must be splitted.\n");
                    /*** OBSOLETE-20190907 ****************
                    // was moved to MWR_IDLEbis because of timing issues
                    breakLen = 65536 - dmCmd.saddr;
                    dmCmd.bbt -= breakLen;
                    tempDmCmd = DmCmd(dmCmd.saddr, breakLen);
                    segIsSplitted = true;
                    ****************************************/
                    smw_fsmState = MWR_IDLEbis;
                    break;
                }
                else {
                    segMustBeSplitted = false;
                    soMEM_TxP_WrCmd.write(dmCmd);
                    breakLen = dmCmd.bbt;
                    smw_fsmState = S1;
                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printDmCmd(myName, dmCmd);
                    }
                    break;
                }
            }
            smw_fsmState = S1;
        }
        break;

    case MWR_IDLEbis:
        breakLen   = 65536 - dmCmd.saddr;
        dmCmd.bbt -= breakLen;
        soMEM_TxP_WrCmd.write(DmCmd(dmCmd.saddr, breakLen));
        smw_fsmState = S1;
        if (DEBUG_LEVEL & TRACE_MWR) {
            printDmCmd(myName, dmCmd);
        }
        break;

    case S1:
        if (!siSlg_Data.empty()) {
            siSlg_Data.read(currWord);
            AxiWord outputWord = currWord;
            ap_uint<4> byteCount = keepMapping(currWord.tkeep);
            // OBSOLETE_20191213 if (!segMemMeta.drop) {  // Was already tested above
			if (breakLen > 8)
				breakLen -= 8;
			else {
				if (segMustBeSplitted == true) {
					if (dmCmd.saddr.range(15, 0) % 8 != 0) {
						// If the word is not perfectly aligned then there is some magic to be worked.
						outputWord.tkeep = lenToKeep(breakLen);
					}
					outputWord.tlast = 1;
					smw_fsmState = S2;
					accessResidue = byteCount - breakLen;
					lengthBuffer = breakLen;  // Buffer the number of bits consumed.
				}
				else {
					smw_fsmState = MWR_IDLE;
				}
			}
			txAppWordCounter++;
			soMEM_TxP_Data.write(outputWord);
            // OBSOLETE_20191213_Not_Needed  }
            // OBSOLETE_20191213_Not_Needed }else {
            // OBSOLETE_20191213_Not_Needed }    if (currWord.tlast == 1)
            // OBSOLETE_20191213_Not_Needed }        smw_fsmState = MWR_IDLE;
            // OBSOLETE_20191213_Not_Needed }}
        }
        break;

    case S2:
        if (!soMEM_TxP_WrCmd.full()) {
            if (dmCmd.saddr.range(15, 0) % 8 == 0)
                smw_fsmState = S3;
            //else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
            else if (dmCmd.bbt - accessResidue > 0)
                smw_fsmState = S4;
            else
                smw_fsmState = S5;
            dmCmd.saddr.range(15, 0) = 0;
            breakLen = dmCmd.bbt;
            soMEM_TxP_WrCmd.write(DmCmd(dmCmd.saddr, breakLen));
            segMustBeSplitted = false;
        }
        break;
    case S3: // This is the non-realignment state
        if (!siSlg_Data.empty() & !soMEM_TxP_Data.full()) {
            siSlg_Data.read(currWord);
            if (!segMemMeta.drop) {
                txAppWordCounter++;
                soMEM_TxP_Data.write(currWord);
            }
            if (currWord.tlast == 1)
                smw_fsmState = MWR_IDLE;
        }
        break;
    case S4: // We go into this state when we need to realign things
        if (!siSlg_Data.empty() && !soMEM_TxP_Data.full()) {
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = currWord.tdata.range(63, lengthBuffer*8);
            currWord = siSlg_Data.read();
            outputWord.tdata.range(63, (8-lengthBuffer)*8) = currWord.tdata.range((lengthBuffer * 8), 0 );

            if (!segMemMeta.drop) {
                if (currWord.tlast == 1) {
                    if (breakLen - accessResidue > lengthBuffer)  {
                        // In this case there's residue to be handled
                        breakLen -=8;
                        smw_fsmState = S5;
                    }
                    else {
                        smw_fsmState = MWR_IDLE;
                        outputWord.tkeep = lenToKeep(breakLen);
                        outputWord.tlast = 1;
                    }
                }
                else
                    breakLen -= 8;
                soMEM_TxP_Data.write(outputWord);
            }
            else {
                if (currWord.tlast == 1)
                    smw_fsmState = MWR_IDLE;
            }
        }
        break;
    case S5:
        if (!soMEM_TxP_Data.full()) {
            if (!segMemMeta.drop) {
                AxiWord outputWord = AxiWord(0, lenToKeep(breakLen), 1);
                outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = currWord.tdata.range(63, lengthBuffer*8);
                soMEM_TxP_Data.write(outputWord);
                smw_fsmState = MWR_IDLE;
            }
        }
        break;
    } // End-of switch
}

/*******************************************************************************
 * @brief Tx App Stream (Tas)
 *
 * @param[in]  siTRIF_Data,        TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,        TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,        TCP data status to TRIF.
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from [STt].
 * @param[out] soTat_AccessQry,    Access query to TxAppTable (Tat).
 * @param[in]  siTat_AccessRep,    Access reply from [Tat].
 * @param[out] soMEM_TxP_WrCmd,    Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,     Tx memory data to MEM.
 * @param[out] soEVe_Event,        Event to EventEngine (EVe).
 *
 * @details
 *  This process transmits the data streams of established connections.
 *   After data and metadata are received from [TRIF], the state of the
 *   connection is checked and the segment memory pointer is loaded into
 *   the APP pointer table.
 *  Next, data are written into the DDR4 buffer memory. The application is
 *   notified through [TODO] if the write to the buffer succeeded. In case
 *   of success the length of the write is returned, otherwise -1;
 *
 ******************************************************************************/
/*** OBSOLETE_20191216 ****************
void pTxAppStream(
        stream<AppData>            &siTRIF_Data,
        stream<AppMeta>            &siTRIF_Meta,
        stream<AppWrSts>           &soTRIF_DSts,
        stream<TcpSessId>          &soSTt_SessStateReq,
        stream<SessionState>       &siSTt_SessStateRep,
        stream<TxAppTableQuery>    &soTat_AccessQry,
        stream<TxAppTableReply>    &siTat_AccessRep,
        stream<DmCmd>              &soMEM_TxP_WrCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<Event>              &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Stream Length Generator (Slg) ---------------------------------------
    static stream<AxiWord>         ssSlgToMwr_Data    ("ssSlgToMwr_Data");
    #pragma HLS stream    variable=ssSlgToMwr_Data    depth=256
    #pragma HLS DATA_PACK variable=ssSlgToMwr_Data

    static stream<TcpSegLen>       ssSlgToSml_SegLen  ("ssSlgToSml_SegLen");
    #pragma HLS stream    variable=ssSlgToSml_SegLen  depth=32

    //-- Stream Metadata Loader (Sml) -----------------------------------------------
    static stream<SegMemMeta>      ssSmlToMwr_SegMeta ("ssSmlToMwr_SegMeta");
    #pragma HLS stream    variable=ssSmlToMwr_SegMeta depth=128
    #pragma HLS DATA_PACK variable=ssSmlToMwr_SegMeta

    pStreamLengthGenerator(
            siTRIF_Data,
            ssSlgToMwr_Data,
            ssSlgToSml_SegLen);

    pStreamMetaLoader(
            siTRIF_Meta,
            soTRIF_DSts,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            soTat_AccessQry,
            siTat_AccessRep,
            ssSlgToSml_SegLen,
            ssSmlToMwr_SegMeta,
            soEVe_Event);

    pMemWriter(
            ssSlgToMwr_Data,
            ssSmlToMwr_SegMeta,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data);

}
*** OBSOLETE_20191216 ****************/

/*****************************************************************************
 * @brief The tx_app_interface (TAi) is front-end of the TCP Role I/F (TRIF).
 *
 * @param[in]  siTRIF_OpnReq,         Open connection request from TCP Role I/F (TRIF).
 * @param[out] soTRIF_OpnRep,         Open connection reply to [TRIF].
 * @param[in]  siTRIF_ClsReq,         Close connection request from [TRIF]
 * @param[in]  siTRIF_Data,           TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,           TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,           TCP data status to TRIF.
 * @param[out] soMEM_TxP_WrCmd,       Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,        Tx memory data to MEM.
 * @param[in]  siMEM_TxP_WrSts,       Tx memory write status from MEM.
 * @param[out] soSTt_SessStateReq,    Session sate request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep,    Session state reply from [STt].
 * @param[out] soSTt_AcceptStateQry,  Session state query to [STt].
 * @param[in]  siSTt_AcceptStateRep,  Session state reply from [STt].
 * @param[out] soSLc_SessLookupReq,   Session lookup request to SessionLookupController(SLc).
 * @param[in]  siSLc_SessLookupRep,   Session lookup reply from [SLc].
 * @param[out] soPRt_GetFreePortReq,  Free port request to PortTable (PRt).
 * @param[in]  siPRt_GetFreePortRep,  Free port reply from [PRt].
 * @param[in]  siTSt_PushCmd,         Push command for an AckNum from TxSarTable (TSt).
 * @param[out] soTSt_PushCmd,         Push command for an AppPtr to [TSt].
 * @param[in]  siRXe_SessOpnSts,      Session open status from [RXe].
 * @param[out] soEVe_Event,           Event to EventEngine (EVe).
 * @param[in]  siTIm_Notif,           Notification from Timers (TIm).
 * @param[in]  piMMIO_IpAddr,         IPv4 address from [MMIO].
 *
 * @details
 *
 ******************************************************************************/
void tx_app_interface(
        //-- TRIF / Open-Close Interfaces
        stream<LE_SockAddr>            &siTRIF_OpnReq,
        stream<OpenStatus>             &soTRIF_OpnRep,
        stream<AppClsReq>              &siTRIF_ClsReq,
        //-- TRIF / Data Stream Interfaces
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<AppWrSts>               &soTRIF_DSts,
        //-- MEM / Tx PATH Interface
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<DmSts>                  &siMEM_TxP_WrSts,
        //-- State Table Interfaces
        stream<TcpSessId>              &soSTt_SessStateReq,
        stream<SessionState>           &siSTt_SessStateRep,
        stream<StateQuery>             &soSTt_AcceptStateQry,
        stream<SessionState>           &siSTt_AcceptStateRep,
		//-- Session Lookup Controller Interface
        stream<LE_SocketPair>          &soSLc_SessLookupReq,
        stream<SessionLookupReply>     &siSLc_SessLookupRep,
		//-- Port Table Interfaces
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<TcpPort>                &siPRt_GetFreePortRep,
		//-- Tx SAR TAble Interfaces
        stream<TStTxSarPush>           &siTSt_PushCmd,
        stream<TAiTxSarPush>           &soTSt_PushCmd,
		//-- Rx Engine Interface
        stream<OpenStatus>             &siRXe_SessOpnSts,
		//-- Event Engine Interface
        stream<Event>                  &soEVe_Event,
		//-- Timers Interface
        stream<OpenStatus>             &siTIm_Notif,
		//-- MMIO / IPv4 Address
        LE_Ip4Addr                      piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Event Multiplexer (Emx) ----------------------------------------------
    static stream<Event>                ssEmxToTash_Event    ("ssEmxToTash_Event");
    #pragma HLS stream         variable=ssEmxToTash_Event    depth=2
    #pragma HLS DATA_PACK      variable=ssEmxToTash_Event

    //-- Tx App Accept (Taa)---------------------------------------------------
    static stream<Event>                ssTaaToEmx_Event     ("ssTaaToEmx_Event");
    #pragma HLS stream         variable=ssTaaToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=ssTaaToEmx_Event

    /*** OBSOLETE_20191216 ************
    //-- Tx App Stream (Tas)---------------------------------------------------
    static stream<TxAppTableQuery>      ssTasToTat_AccessQry ("ssTasToTat_AccessQry");
    #pragma HLS stream         variable=ssTasToTat_AccessQry depth=2
    #pragma HLS DATA_PACK      variable=ssTasToTat_AccessQry

    static stream<Event>                ssTasToEmx_Event     ("ssTasToEmx_Event");
    #pragma HLS stream         variable=ssTasToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=ssTasToEmx_Event

    //-- Tx App Table (Tat)----------------------------------------------------
    static stream<TxAppTableReply>      ssTatToTas_AccessRep ("ssTatToTas_AccessRep");
    #pragma HLS stream         variable=ssTatToTas_AccessRep depth=2
    #pragma HLS DATA_PACK      variable=ssTatToTas_AccessRep
    *** OBSOLETE_20191216 ************/

    //-- Tx App Table (Tat)----------------------------------------------------
    static stream<TxAppTableReply>      ssTatToSml_AccessRep ("ssTatToSml_AccessRep");
    #pragma HLS stream         variable=ssTatToSml_AccessRep depth=2
    #pragma HLS DATA_PACK      variable=ssTatToSml_AccessRep

    //-- StreamMetaLoader (Sml)------------------------------------------------
    static stream<TxAppTableQuery>      ssSmlToTat_AccessQry ("ssSmlToTat_AccessQry");
    #pragma HLS stream         variable=ssSmlToTat_AccessQry depth=2
    #pragma HLS DATA_PACK      variable=ssSmlToTat_AccessQry

    static stream<SegMemMeta>           ssSmlToMwr_SegMeta   ("ssSmlToMwr_SegMeta");
    #pragma HLS stream         variable=ssSmlToMwr_SegMeta   depth=128
    #pragma HLS DATA_PACK      variable=ssSmlToMwr_SegMeta

    static stream<Event>                ssSmlToEmx_Event     ("ssSmlToEmx_Event");
    #pragma HLS stream         variable=ssSmlToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=ssSmlToEmx_Event

    //-- Stream Length Generator (Slg) ----------------------------------------
    static stream<AxiWord>              ssSlgToMwr_Data      ("ssSlgToMwr_Data");
    #pragma HLS stream         variable=ssSlgToMwr_Data      depth=256
    #pragma HLS DATA_PACK      variable=ssSlgToMwr_Data

    static stream<TcpSegLen>       ssSlgToSml_SegLen         ("ssSlgToSml_SegLen");
    #pragma HLS stream    variable=ssSlgToSml_SegLen         depth=32

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    // Event Multiplexer (Emx)
    pStreamMux(
        ssTaaToEmx_Event,
        ssSmlToEmx_Event,
        ssEmxToTash_Event);

    // Tx Application Status Handler (Tash)
    pTxAppStatusHandler(
        siMEM_TxP_WrSts,
        ssEmxToTash_Event,
        soTSt_PushCmd,
        soEVe_Event);

    /*** OBSOLETE_20191216 ************
    // Tx Application Stream (Tas)
    pTxAppStream(
            siTRIF_Data,
            siTRIF_Meta,
            soTRIF_DSts,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            ssTasToTat_AccessQry,
            ssTatToTas_AccessRep,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            ssTasToEmx_Event);
    *** OBSOLETE_20191216 ************/

    pStreamLengthGenerator(
            siTRIF_Data,
            ssSlgToMwr_Data,
            ssSlgToSml_SegLen);

    pStreamMetaLoader(
            siTRIF_Meta,
            soTRIF_DSts,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            ssSmlToTat_AccessQry,
            ssTatToSml_AccessRep,
            ssSlgToSml_SegLen,
            ssSmlToMwr_SegMeta,
            ssSmlToEmx_Event);

    pMemWriter(
            ssSlgToMwr_Data,
            ssSmlToMwr_SegMeta,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data);

    // Tx Application Accept (Taa)
    pTxAppAccept(
            siTRIF_OpnReq,
            soTRIF_OpnRep,
            siTRIF_ClsReq,
            soSLc_SessLookupReq,
            siSLc_SessLookupRep,
            soPRt_GetFreePortReq,
            siPRt_GetFreePortRep,
            soSTt_AcceptStateQry,
            siSTt_AcceptStateRep,
            siRXe_SessOpnSts,
            ssTaaToEmx_Event,
            siTIm_Notif,
            piMMIO_IpAddr);

    // Tx Application Table (Tat)
    pTxAppTable(
            siTSt_PushCmd,
            ssSmlToTat_AccessQry,
            ssTatToSml_AccessRep);
}
