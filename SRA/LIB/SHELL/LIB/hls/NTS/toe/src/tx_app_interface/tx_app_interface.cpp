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

/*******************************************************************************
 * @file       : tx_app_interface.cpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "tx_app_interface.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TAC_TRACE | TAS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TAi"

#define TRACE_OFF   0x0000
#define TRACE_SML  1 <<  1
#define TRACE_SLG  1 <<  2
#define TRACE_MWR  1 <<  3
#define TRACE_TAC  1 <<  4
#define TRACE_TAS  1 <<  5
#define TRACE_TAT  1 <<  6
#define TRACE_TASH 1 <<  7
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief A 2-to-1 generic Stream Multiplexer
 *
 *  @param[in]  si1     The input stream #1.
 *  @param[in]  si2     The input stream #2.
 *  @param[out] so      The output stream.
 *
 * @details
 *  This multiplexer behaves like an arbiter. It takes two streams as inputs and
 *   forwards one of them to the output channel. The stream connected to the
 *   first input always takes precedence over the second one.
 *******************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so)
{
   //-- DIRECTIVES FOR THIS PROCESS --------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*******************************************************************************
 * @brief Tx Application Connect (Tac)
 *
 * @param[in]  siTAIF_OpnReq         Open connection request from TcpApplicationInterface (TAIF).
 * @param[out] soTAIF_OpnRep         Open connection reply to [TAIF].
 * @param[]    siTAIF_ClsReq         Close connection request from [TAIF].
 * @param[out] soSLc_SessLookupReq   Lookup request SessionLookupController (SLc).
 * @param[in]  siSLc_SessLookupRep   Reply from [SLc]
 * @param[out] soPRt_GetFreePortReq  Free port request to PortTable (PRt).
 * @param[out] siPRt_GetFreePortRep  Free port reply from [PRt].
 * @param[in]  siRXe_ActSessState    TCP state of an active session from [RXe].
 * @param[out  soSTt_AcceptStateQry  Session state query to StateTable (STt).  [FIXME - What about ConnectStateReply?]
 * @param[in]  siSTt_AcceptStateRep  Session state reply from [STt].
 * @param[out] soEVe_Event           Event to EventEngine (EVe).
 * @param[in]  siTIm_Notif           Notification from Timers (TIm).
 * @param[in]  piMMIO_IpAddr         IPv4 Address from MMIO.
 *
 * @details
 *   This process performs the creation and tear down of the active connections.
 *   Active connections are the ones opened by the FPGA when operating in client
 *   mode. They make use of dynamically assigned or ephemeral ports which always
 *   fall in the range 32,768 to 65,535 (by TOE's convention).
 *  The operations performed here are somehow similar to the 'connect' and
 *   'close' system calls. The IP tuple of the new connection to open is read
 *   from 'siTAIF_OpnReq'. Next, a free port number is requested from the
 *   PortTable (PRt) and a SYN event is forwarded to the TxeEngine (TXe) in
 *   order to initiate a 3-way handshake with the remote server.
 *  The [APP] is notified upon a connection establishment success or fail via a
 *   status message delivered over 'soTAIF_OpnRep'. Upon success, the message
 *   will provide the [APP] with the 'SessionId' of the new connection.
 *  Sending the 'SessionId' over the 'siTAIF_ClsReq' interface, will tear-down
 *   the connection.
 *******************************************************************************/
void pTxAppConnect(
        stream<TcpAppOpnReq>        &siTAIF_OpnReq,
        stream<TcpAppOpnRep>        &soTAIF_OpnRep,
        stream<TcpAppClsReq>        &siTAIF_ClsReq,
        stream<SocketPair>          &soSLc_SessLookupReq,
        stream<SessionLookupReply>  &siSLc_SessLookupRep,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<TcpPort>             &siPRt_GetFreePortRep,
        stream<StateQuery>          &soSTt_ConnectStateQry,
        stream<TcpState>            &siSTt_ConnectStateRep,
        stream<SessState>           &siRXe_ActSessState,
        stream<Event>               &soEVe_Event,
        stream<SessState>           &siTIm_Notif,
        Ip4Address                   piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Tac");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates {TAC_IDLE=0, TAC_GET_FREE_PORT, TAC_CLOSE_CONN } \
                                tac_fsmState=TAC_IDLE;
    #pragma HLS RESET  variable=tac_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static SessionId    tac_closeSessionID;

    //-- LOCAL STREAMS ---------------------------------------------------------
    static stream<SockAddr>     tac_localFifo ("tac_localFifo");
    const  int                  tac_localFifo_depth = 4;
    #pragma HLS stream variable=tac_localFifo depth=tac_localFifo_depth

    SessState  activeSessState;

    switch (tac_fsmState) {
    case TAC_IDLE:
        if (!siTAIF_OpnReq.empty() && !soPRt_GetFreePortReq.full()) {
            assessSize(myName, tac_localFifo, "tac_localFifo", tac_localFifo_depth);
            tac_localFifo.write(siTAIF_OpnReq.read());
            soPRt_GetFreePortReq.write(1);
            tac_fsmState = TAC_GET_FREE_PORT;
        }
        else if (!siSLc_SessLookupRep.empty()) {
            // Read the session lookup reply and check its state
            SessionLookupReply sessLkpRep = siSLc_SessLookupRep.read();
            if (sessLkpRep.hit) {
                soEVe_Event.write(Event(SYN_EVENT, sessLkpRep.sessionID));
                soSTt_ConnectStateQry.write(StateQuery(sessLkpRep.sessionID, SYN_SENT, QUERY_WR));
            }
            else {
                // Tell the [APP ]that opening of the active connection failed
                //OBSOLETE_20200714 soTAIF_OpnRep.write(OpenStatus(sessLkpRep.sessionID, FAILED_TO_OPEN_SESS));
                soTAIF_OpnRep.write(TcpAppOpnRep(sessLkpRep.sessionID, CLOSED));
            }
        }
        else if (!siRXe_ActSessState.empty()) {
            // If pending, read the state of the on-going active session (.i.e, initiated by [TOE])
            siRXe_ActSessState.read(activeSessState);
            // And forward it to [TAIF]
            //  [TODO-FIXME We should check if [TAIF] is actually waiting for such a status]
            soTAIF_OpnRep.write(activeSessState);
        }
        else if (!siTIm_Notif.empty()) {
            soTAIF_OpnRep.write(siTIm_Notif.read());
        }
        else if(!siTAIF_ClsReq.empty()) {
            siTAIF_ClsReq.read(tac_closeSessionID);
            soSTt_ConnectStateQry.write(StateQuery(tac_closeSessionID));
            tac_fsmState = TAC_CLOSE_CONN;
        }
        break;
    case TAC_GET_FREE_PORT:
        if (!siPRt_GetFreePortRep.empty() && !soSLc_SessLookupReq.full()) {
            TcpPort  freePort   = siPRt_GetFreePortRep.read();
            SockAddr serverAddr = tac_localFifo.read();
            // Request the [SLc] to create a new entry in its session table
            soSLc_SessLookupReq.write(SocketPair(
                                      SockAddr(piMMIO_IpAddr,   freePort),
                                      SockAddr(serverAddr.addr, serverAddr.port)));
            tac_fsmState = TAC_IDLE;
        }
        break;
    case TAC_CLOSE_CONN:
        if (!siSTt_ConnectStateRep.empty()) {
            TcpState tcpState = siSTt_ConnectStateRep.read();
            // [FIXME-TODO Not yet tested]
            if ((tcpState == ESTABLISHED) or (tcpState == FIN_WAIT_2) or (tcpState == FIN_WAIT_1)) {
                // [TODO - Why if FIN already SENT]
                soSTt_ConnectStateQry.write(StateQuery(tac_closeSessionID, FIN_WAIT_1, QUERY_WR));
                soEVe_Event.write(Event(FIN_EVENT, tac_closeSessionID));
            }
            else {
                soSTt_ConnectStateQry.write(StateQuery(tac_closeSessionID, tcpState, QUERY_WR));
            }
            tac_fsmState = TAC_IDLE;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Tx Application Status Handler (Tash)
 *
 * @param[in]  siMEM_TxP_WrSts Tx memory write status from [MEM].
 * @param[in]  siEmx_Event     Event from the EventMultiplexer (Emx).
 * @param[out] soTSt_PushCmd   Push command to TxSarTable (TSt).
 * @param[out] soEVe_Event     Event to EventEngine (EVe).
 *
 * @details
 *  This process waits and reads events from [Emx]. If the incoming event is a
 *   'TX_EVENT', the process will wait for one (or possibly two) memory status
 *   from [MEM] and will forward a command to update the 'TxApplicationPointer'
 *   of the [TSt].
 *  Whatever the received event, it is always forwarded to [EVe].
 *******************************************************************************/
void pTxAppStatusHandler(
        stream<DmSts>             &siMEM_TxP_WrSts,
        stream<Event>             &siEmx_Event,
        stream<TAiTxSarPush>      &soTSt_PushCmd,
        stream<Event>             &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Tash");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { S0=0, S1, S2 } tash_fsmState=S0;
    #pragma HLS reset             variable=tash_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
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
                                  getEventName(ev.type));
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
                if (txAppPtr <= 0x10000) {  // [FIXME - Why '<=' and not '<' ?]
                    // Update the 'txAppPtr' of the TX_SAR_TABLE
                    soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, txAppPtr.range(15, 0)));
                    // Forward event to [EVe] which will signal [TXe]
                    soEVe_Event.write(ev);
                    if (DEBUG_LEVEL & TRACE_TASH) {
                        printInfo(myName, "Received TXMEM write status = %d.\n", status.okay.to_int());
                    }
                    tash_fsmState = S0;
                }
                else {
                    // The TCP buffer wrapped around
                    tash_fsmState = S2;
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

/*******************************************************************************
 * @brief Tx Application Table (Tat)
 *
 * @param[in]  siTSt_PushCmd    Push command from TxSarTable (TSt).
 * @param[in]  siTas_AccessQry  Access query from TxAppStream (Tas).
 * @param[out] soTAs_AccessRep  Access reply to [Tas].
 *
 * @details
 *  This table keeps tack of the Tx ACK numbers and Tx memory pointers.
 *
 *******************************************************************************/
void pTxAppTable(
        stream<TStTxSarPush>      &siTSt_PushCmd,
        stream<TxAppTableQuery>   &siTas_AccessQry,
        stream<TxAppTableReply>   &siTas_AccessRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Tat");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static TxAppTableEntry          TX_APP_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_APP_TABLE inter false
    #pragma HLS reset      variable=TX_APP_TABLE

    //-- DYNAMIC VARIABLES -----------------------------------------------------
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
 * @param[in]  siTAIF_Data   TCP data stream from [TAIF].
 * @param[out] soMwr_Data    TCP data stream to MemoryWriter (Mwr).
 * @param[out] soSml_SegLen  The length of the TCP segment to StreamMetaLoader (Sml).
 *
 * @details
 *   This process generates the length of the incoming TCP segment from [APP]
 *    while forwarding that same data stream to the MemoryWriter (Mwr).
 *    [FIXME - This part is completely bugus!!! Must fix like with UOE]
 *******************************************************************************/
void pStreamLengthGenerator(
        stream<TcpAppData>  &siTAIF_Data,
        stream<AxisApp>     &soMwr_Data,
        stream<TcpSegLen>   &soSml_SegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Slg");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static TcpSegLen           slg_segLen=0;
    #pragma HLS RESET variable=slg_segLen

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisApp currChunk = AxisApp(0, 0xFF, 0);

    if (!siTAIF_Data.empty()) {
        siTAIF_Data.read(currChunk);
        soMwr_Data.write(currChunk);
        //OBSOLETE_20200708 slg_segLen += keepMapping(currWord.tkeep);
        slg_segLen += currChunk.getLen();
        if (currChunk.getTLast()) {
            assessSize(myName, soSml_SegLen, "soSml_SegLen", 32);
            soSml_SegLen.write(slg_segLen);
            if (DEBUG_LEVEL & TRACE_SLG) {
                printInfo(myName, "Received end-of-segment. SegLen=%d\n", slg_segLen.to_int());
            }
            slg_segLen = 0;
        }
    }
}

/*******************************************************************************
 * @brief Stream Metadata Loader (Sml)
 *
 * @param[in]  siTAIF_Meta        The TCP session Id this segment belongs to.
 * @param[out] soTAIF_DSts        The TCP write transfer status to TAIF.
 * @param[out] soSTt_SessStateReq Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep Session state reply from StateTable (STt).
 * @param[out] soTat_AccessReq    Access request to TxAppTable (Tat).
 * @param[in]  siTat_AccessRep    Access reply from [Tat]
 * @param[in]  siSlg_SegLen       The TCP segment length from SegmentLengthGenerator (Slg).
 * @param[out] soMwr_SegMeta      Segment metadata to MemoryWriter (Mwr).
 * @param[out] soEmx_Event        Event to EventMultiplexer (Emx).
 *
 * @details
 *  This process reads the request from the application and loads the necessary
 *   metadata from the [Tat] for transmitting this segment. After a session-id
 *   request is received from [TAIF], the state of the connection is checked and
 *   the segment memory pointer is loaded into the TxAppTable. Next, segment's
 *   metadata is forwarded to the MemoryWriter (Mwr) process which will write
 *   the segment into the DDR4 buffer memory.
 * The FSM of this process decides if the segment is written to the TX buffer or
 *  is discarded.
 *
 *  [TODO: Implement TCP_NODELAY]
 *******************************************************************************/
void pStreamMetaLoader(
        stream<TcpAppMeta>          &siTAIF_Meta,
        stream<TcpAppWrSts>         &soTAIF_DSts,
        stream<SessionId>           &soSTt_SessStateReq,
        stream<TcpState>            &siSTt_SessStateRep,
        stream<TxAppTableQuery>     &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<SegMemMeta>          &sSmlToMwr_SegMeta,
        stream<Event>               &soEmx_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { READ_REQUEST=0,  READ_META } \
                                            mdl_fsmState = READ_REQUEST;
    #pragma HLS reset            variable = mdl_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SessionId sessId;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TxAppTableReply  txAppTableReply;
    TcpState         sessState;
    TcpSegLen        segLen;

    switch(mdl_fsmState) {
    case READ_REQUEST:
        if (!siTAIF_Meta.empty()) {
            // Read the session ID
            siTAIF_Meta.read(sessId);
            // Request state of the session
            assessSize(myName, soSTt_SessStateReq, "soSTt_SessStateReq", 2);  // [FIXME-Use constant for the length]
            soSTt_SessStateReq.write(sessId);
            // Request the value of ACK and MemPtr from TxAppTable
            assessSize(myName, soTat_AccessReq, "soTat_AccessReq", 2);  // [FIXME-Use constant for the length]
            soTat_AccessReq.write(TxAppTableQuery(sessId));
            if (DEBUG_LEVEL & TRACE_SML) {
                printInfo(myName, "Received new Tx request for session %d.\n", sessId.to_int());
            }
            mdl_fsmState = READ_META;
        }
        break;
    case READ_META:
        if (!siTat_AccessRep.empty() && !siSTt_SessStateRep.empty() && !siSlg_SegLen.empty()) {
            siSTt_SessStateRep.read(sessState);
            siTat_AccessRep.read(txAppTableReply);
            // [FIXME][FIXME][FIXME] Incoming data stream must me managed like new UOE !!!
            siSlg_SegLen.read(segLen);
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
                soTAIF_DSts.write(TcpAppWrSts(TCP_APP_WR_STS_KO, TCP_APP_WR_STS_NOCONNECTION));
                printError(myName, "Session %d is not established. Current session state is \'%s\'.\n",
                           sessId.to_uint(), TcpStateString[sessState].c_str());
            }
            else if (segLen > maxWriteLength) {
                sSmlToMwr_SegMeta.write(SegMemMeta(CMD_DROP));
                // Notify [APP] about fail
                soTAIF_DSts.write(TcpAppWrSts(TCP_APP_WR_STS_KO, TCP_APP_WR_STS_NOSPACE));
                printError(myName, "There is no TxBuf memory space available for session %d.\n",
                           sessId.to_uint());
            }
            else { //-- Session is ESTABLISHED and segLen <= maxWriteLength
                // Forward the metadata to the SegmentMemoryWriter (Mwr)
                sSmlToMwr_SegMeta.write(SegMemMeta(sessId, txAppTableReply.mempt, segLen));
                // Forward data status back to [APP]
                soTAIF_DSts.write(TcpAppWrSts(STS_OK, segLen));
                // Notify [TXe] about data to be sent via an event to [EVe]
                assessSize(myName, soEmx_Event, "soEmx_Event", 2);  // [FIXME-Use constant for the length]
                soEmx_Event.write(Event(TX_EVENT, sessId, txAppTableReply.mempt, segLen));
                // Update the 'txMemPtr' in TxAppTable
                soTat_AccessReq.write(TxAppTableQuery(sessId, txAppTableReply.mempt + segLen));
            }
            mdl_fsmState = READ_REQUEST;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Memory Writer (Mwr)
 *
 * @param[in]  siSlg_Data      TCP data stream from SegmentLengthGenerator (Slg).
 * @param[in]  siSml_SegMeta   Segment memory metadata from StreamMetaLoader (Sml).
 * @param[out] soMEM_TxP_WrCmd Tx memory write command to [MEM].
 * @param[out] soMEM_TxP_Data  Tx memory data to [MEM].
 *
 * @details
 *   This process writes the incoming TCP segment into the external DRAM, unless
 *    the state machine of the MetaDataLoader (Sml) decides to drop the segment.
 *******************************************************************************/
void pMemWriter(
        stream<AxisApp>     &siSlg_Data,
        stream<SegMemMeta>  &siSml_SegMeta,
        stream<DmCmd>       &soMEM_TxP_WrCmd,
        stream<AxisApp>     &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "Mwr");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { MWR_IDLE=0, MWR_IDLEbis, S1, S2, S3, S4, S5 } \
                                 mwr_fsmState=MWR_IDLE;
    #pragma HLS RESET variable = mwr_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static SegMemMeta    mwr_segMemMeta;
    static bool          mwr_segMustBeSplitted;
    static DmCmd         mwr_dmCmd;
    static AxisApp       mwr_currChunk;
    static TcpSegLen     mwr_breakLen;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    static uint8_t       lengthBuffer;
    static ap_uint<3>    accessResidue;
    static uint32_t      txAppWordCounter;

    switch (mwr_fsmState) {
    case MWR_IDLE:
        if (!siSml_SegMeta.empty() && !soMEM_TxP_WrCmd.full()) {
            siSml_SegMeta.read(mwr_segMemMeta);
            if (!mwr_segMemMeta.drop) {
                // Build a memory address for this segment in the upper 2GB [FIXME-Must be configurable]
                ap_uint<32> memSegAddr; // [FIXME-TODO-typedef+33-bits]
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = mwr_segMemMeta.sessId(13, 0);
                memSegAddr(15,  0) = mwr_segMemMeta.addr;
                // Build a data mover command for this segment
                mwr_dmCmd = DmCmd(memSegAddr, mwr_segMemMeta.len);
                if ((mwr_dmCmd.saddr.range(15, 0) + mwr_dmCmd.bbt) > 65536) {
                    mwr_segMustBeSplitted = true;
                    printInfo(myName, "Segment must be splitted.\n");
                    /*** OBSOLETE-20190907 ****************
                    // was moved to MWR_IDLEbis because of timing issues
                    breakLen = 65536 - dmCmd.saddr;
                    dmCmd.bbt -= breakLen;
                    tempDmCmd = DmCmd(dmCmd.saddr, breakLen);
                    segIsSplitted = true;
                    ****************************************/
                    mwr_fsmState = MWR_IDLEbis;
                    break;
                }
                else {
                    mwr_segMustBeSplitted = false;
                    soMEM_TxP_WrCmd.write(mwr_dmCmd);
                    mwr_breakLen = mwr_dmCmd.bbt;
                    mwr_fsmState = S1;
                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printDmCmd(myName, mwr_dmCmd);
                    }
                    break;
                }
            }
            mwr_fsmState = S1;
        }
        break;
    case MWR_IDLEbis:
        mwr_breakLen   = 65536 - mwr_dmCmd.saddr;
        mwr_dmCmd.bbt -= mwr_breakLen;
        soMEM_TxP_WrCmd.write(DmCmd(mwr_dmCmd.saddr, mwr_breakLen));
        mwr_fsmState = S1;
        if (DEBUG_LEVEL & TRACE_MWR) { printDmCmd(myName, mwr_dmCmd); }
        break;
    case S1:
        if (!siSlg_Data.empty()) {
            siSlg_Data.read(mwr_currChunk);
            AxisApp memChunk = mwr_currChunk;
            ap_uint<4> byteCount = mwr_currChunk.getLen();
            if (mwr_breakLen > 8) {
                mwr_breakLen -= 8;
            }
            else {
                if (mwr_segMustBeSplitted == true) {
                    if (mwr_dmCmd.saddr.range(15, 0) % 8 != 0) {
                        // If the word is not perfectly aligned then there is some magic to be worked.
                        //OBSOLETE_20200708 sendChunk.tkeep = lenToKeep(mwr_breakLen);
                        memChunk.setLE_TKeep(lenToLE_tKeep(mwr_breakLen));
                    }
                    memChunk.setTLast(TLAST);
                    mwr_fsmState = S2;
                    accessResidue = byteCount - mwr_breakLen;
                    lengthBuffer = mwr_breakLen;  // Buffer the number of bits consumed.
                }
                else {
                	mwr_fsmState = MWR_IDLE;
                }
            }
            txAppWordCounter++;
            soMEM_TxP_Data.write(memChunk);
        }
        break;
    case S2:
        if (!soMEM_TxP_WrCmd.full()) {
            if (mwr_dmCmd.saddr.range(15, 0) % 8 == 0)
                mwr_fsmState = S3;
            //else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
            else if (mwr_dmCmd.bbt - accessResidue > 0)
                mwr_fsmState = S4;
            else
                mwr_fsmState = S5;
            mwr_dmCmd.saddr.range(15, 0) = 0;
            mwr_breakLen = mwr_dmCmd.bbt;
            soMEM_TxP_WrCmd.write(DmCmd(mwr_dmCmd.saddr, mwr_breakLen));
            mwr_segMustBeSplitted = false;
        }
        break;
    case S3: // This is the non-realignment state
        if (!siSlg_Data.empty() & !soMEM_TxP_Data.full()) {
            siSlg_Data.read(mwr_currChunk);
            if (!mwr_segMemMeta.drop) {
                txAppWordCounter++;
                soMEM_TxP_Data.write(mwr_currChunk);
            }
            if (mwr_currChunk.getTLast())
                mwr_fsmState = MWR_IDLE;
        }
        break;
    case S4: // We go into this state when we need to realign things
        if (!siSlg_Data.empty() && !soMEM_TxP_Data.full()) {
            AxisApp memChunk = AxisApp(0, 0xFF, 0);
            //OBSOLETE_20200708 memChunk.tdata.range(((8-lengthBuffer)*8) - 1, 0) = mwr_currChunk.tdata.range(63, lengthBuffer*8);
            memChunk.setLE_TData(mwr_currChunk.getLE_TData(63, lengthBuffer*8),
                                 ((8-lengthBuffer)*8) - 1, 0);
            mwr_currChunk = siSlg_Data.read();
            //OBSOLETE_20200708 memChunk.tdata.range(63, (8-lengthBuffer)*8) = mwr_currChunk.tdata.range((lengthBuffer * 8), 0 );
            memChunk.setLE_TData(mwr_currChunk.getLE_TData((lengthBuffer * 8), 0), \
                                 63, (8-lengthBuffer)*8);
            if (!mwr_segMemMeta.drop) {
                if (mwr_currChunk.getTLast()) {
                    if (mwr_breakLen - accessResidue > lengthBuffer)  {
                        // In this case there's residue to be handled
                        mwr_breakLen -=8;
                        mwr_fsmState = S5;
                    }
                    else {
                        mwr_fsmState = MWR_IDLE;
                        //OBSOLETE_20200708 memChunk.tkeep = lenToKeep(mwr_breakLen);
                        memChunk.setLE_TKeep(lenToLE_tKeep(mwr_breakLen));
                        memChunk.setTLast(TLAST);
                    }
                }
                else
                    mwr_breakLen -= 8;
                soMEM_TxP_Data.write(memChunk);
            }
            else {
                if (mwr_currChunk.getTLast()) {
                    mwr_fsmState = MWR_IDLE;
                }
            }
        }
        break;
    case S5:
        if (!soMEM_TxP_Data.full()) {
            if (!mwr_segMemMeta.drop) {
                AxisApp memChunk = AxisApp(0, lenToLE_tKeep(mwr_breakLen), 1);
                //OBSOLETE_20200708 memChunk.tdata.range(((8-lengthBuffer)*8) - 1, 0) = mwr_currChunk.tdata.range(63, lengthBuffer*8);
                memChunk.setLE_TData(mwr_currChunk.getLE_TData(63, lengthBuffer*8), \
                                      ((8-lengthBuffer)*8) - 1, 0);
                soMEM_TxP_Data.write(memChunk);
                mwr_fsmState = MWR_IDLE;
            }
        }
        break;
    } // End-of switch
}

/*******************************************************************************
 * @brief Tx Application Interface (TAi)
 *
 * @param[in]  siTAIF_OpnReq         Open connection request from TCP Role I/F (TAIF).
 * @param[out] soTAIF_OpnRep         Open connection reply to [TAIF].
 * @param[in]  siTAIF_ClsReq         Close connection request from [TAIF]
 * @param[in]  siTAIF_Data           TCP data stream from TAIF.
 * @param[in]  siTAIF_Meta           TCP metadata stream from TAIF.
 * @param[out] soTAIF_DSts           TCP data status to TAIF.
 * @param[out] soMEM_TxP_WrCmd       Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data        Tx memory data to MEM.
 * @param[in]  siMEM_TxP_WrSts       Tx memory write status from MEM.
 * @param[out] soSTt_SessStateReq    Session sate request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep    Session state reply from [STt].
 * @param[out] soSTt_AcceptStateQry  Session state query to [STt].
 * @param[in]  siSTt_AcceptStateRep  Session state reply from [STt].
 * @param[out] soSLc_SessLookupReq   Session lookup request to SessionLookupController(SLc).
 * @param[in]  siSLc_SessLookupRep   Session lookup reply from [SLc].
 * @param[out] soPRt_GetFreePortReq  Free port request to PortTable (PRt).
 * @param[in]  siPRt_GetFreePortRep  Free port reply from [PRt].
 * @param[in]  siTSt_PushCmd         Push command for an AckNum from TxSarTable (TSt).
 * @param[out] soTSt_PushCmd         Push command for an AppPtr to [TSt].
 * @param[in]  siRXe_ActSessState    TCP state of active session from [RXe].
 * @param[out] soEVe_Event           Event to EventEngine (EVe).
 * @param[in]  siTIm_Notif           Notification from Timers (TIm).
 * @param[in]  piMMIO_IpAddr         IPv4 address from [MMIO].
 *
 * @details
 *  This process is the front-end interface to the TCP application layer.
 *******************************************************************************/
void tx_app_interface(
        //-- TAIF / Open-Close Interfaces
        stream<TcpAppOpnReq>           &siTAIF_OpnReq,
        stream<TcpAppOpnRep>           &soTAIF_OpnRep,
        stream<TcpAppClsReq>           &siTAIF_ClsReq,
        //-- TAIF / Data Stream Interfaces
        stream<TcpAppData>             &siTAIF_Data,
        stream<TcpAppMeta>             &siTAIF_Meta,
        stream<TcpAppWrSts>            &soTAIF_DSts,
        //-- MEM / Tx PATH Interface
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxisApp>                &soMEM_TxP_Data,
        stream<DmSts>                  &siMEM_TxP_WrSts,
        //-- State Table Interfaces
        stream<SessionId>              &soSTt_SessStateReq,
        stream<TcpState>               &siSTt_SessStateRep,
        stream<StateQuery>             &soSTt_ConnectStateQry,
        stream<TcpState>               &siSTt_ConnectStateRep,
        //-- Session Lookup Controller Interface
        stream<SocketPair>             &soSLc_SessLookupReq,
        stream<SessionLookupReply>     &siSLc_SessLookupRep,
        //-- Port Table Interfaces
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<TcpPort>                &siPRt_GetFreePortRep,
        //-- Tx SAR TAble Interfaces
        stream<TStTxSarPush>           &siTSt_PushCmd,
        stream<TAiTxSarPush>           &soTSt_PushCmd,
        //-- Rx Engine Interface
        stream<SessState>              &siRXe_ActSessState,
        //-- Event Engine Interface
        stream<Event>                  &soEVe_Event,
        //-- Timers Interface
        stream<SessState>              &siTIm_Notif,
        //-- MMIO / IPv4 Address
        Ip4Addr                         piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Event Multiplexer (Emx) ----------------------------------------------
    static stream<Event>                ssEmxToTash_Event    ("ssEmxToTash_Event");
    #pragma HLS stream         variable=ssEmxToTash_Event    depth=2
    #pragma HLS DATA_PACK      variable=ssEmxToTash_Event

    //-- Tx App Connect (Tac) --------------------------------------------------
    static stream<Event>                ssTacToEmx_Event     ("ssTacToEmx_Event");
    #pragma HLS stream         variable=ssTacToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=ssTacToEmx_Event

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
    static stream<AxisApp>              ssSlgToMwr_Data      ("ssSlgToMwr_Data");
    #pragma HLS stream         variable=ssSlgToMwr_Data      depth=256
    #pragma HLS DATA_PACK      variable=ssSlgToMwr_Data

    static stream<TcpSegLen>       ssSlgToSml_SegLen         ("ssSlgToSml_SegLen");
    #pragma HLS stream    variable=ssSlgToSml_SegLen         depth=32

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    // Event Multiplexer (Emx)
    pStreamMux(
        ssTacToEmx_Event,
        ssSmlToEmx_Event,
        ssEmxToTash_Event);

    // Tx Application Status Handler (Tash)
    pTxAppStatusHandler(
        siMEM_TxP_WrSts,
        ssEmxToTash_Event,
        soTSt_PushCmd,
        soEVe_Event);

    pStreamLengthGenerator(
            siTAIF_Data,
            ssSlgToMwr_Data,
            ssSlgToSml_SegLen);

    pStreamMetaLoader(
            siTAIF_Meta,
            soTAIF_DSts,
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

    // Tx Application Connect (Tac)
    pTxAppConnect(
            siTAIF_OpnReq,
            soTAIF_OpnRep,
            siTAIF_ClsReq,
            soSLc_SessLookupReq,
            siSLc_SessLookupRep,
            soPRt_GetFreePortReq,
            siPRt_GetFreePortRep,
            soSTt_ConnectStateQry,
            siSTt_ConnectStateRep,
            siRXe_ActSessState,
            ssTacToEmx_Event,
            siTIm_Notif,
            piMMIO_IpAddr);

    // Tx Application Table (Tat)
    pTxAppTable(
            siTSt_PushCmd,
            ssSmlToTat_AccessQry,
            ssTatToSml_AccessRep);
}

/*! \} */
