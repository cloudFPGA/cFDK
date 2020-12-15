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
 *  This process performs the creation and tear down of the active connections.
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
 *
 * @warning
 *  The outgoing stream 'soTAIF_SndRep is operated in non-blocking mode to avoid
 *   any stalling of this process. This means that the current reply to be sent
 *   on this stream will be dropped if the stream is full. As a result, the
 *   application process must provision enough buffering to store the reply
 *   returned by this process upon a request to open an active connection.
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
        if (!siTAIF_OpnReq.empty() and !soPRt_GetFreePortReq.full()) {
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
                if (!soTAIF_OpnRep.full()) {
                    soTAIF_OpnRep.write(TcpAppOpnRep(sessLkpRep.sessionID, CLOSED));
                }
                else {
                    // Drop this reply
                    printFatal(myName, "Cannot write 'soTAIF_OpnRep()'. Stream is full!");
                }
            }
        }
        else if (!siRXe_ActSessState.empty()) {
            // If pending, read the state of the on-going active session (.i.e, initiated by [TOE])
            siRXe_ActSessState.read(activeSessState);
            // And forward it to [TAIF]
            //  [TODO-FIXME We should check if [TAIF] is actually waiting for such a status]
            if (!soTAIF_OpnRep.full()) {
                soTAIF_OpnRep.write(activeSessState);
            }
            else {
                // Drop this reply
                printFatal(myName, "Cannot write 'soTAIF_OpnRep()'. Stream is full!");
            }
        }
        else if (!siTIm_Notif.empty()) {
            if (!soTAIF_OpnRep.full()) {
                soTAIF_OpnRep.write(siTIm_Notif.read());
            }
            else {
                // Drop this reply
                printFatal(myName, "Cannot write 'soTAIF_OpnRep()'. Stream is full!");
            }
        }
        else if(!siTAIF_ClsReq.empty()) {
            siTAIF_ClsReq.read(tac_closeSessionID);
            soSTt_ConnectStateQry.write(StateQuery(tac_closeSessionID));
            tac_fsmState = TAC_CLOSE_CONN;
        }
        break;
    case TAC_GET_FREE_PORT:
        if (!siPRt_GetFreePortRep.empty() and !soSLc_SessLookupReq.full()) {
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
    static enum FsmStates { TASH_IDLE=0, TASH_RD_MEM_STATUS_1, TASH_RD_MEM_STATUS_2 } \
                                           tash_fsmState=TASH_IDLE;
    #pragma HLS reset             variable=tash_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static Event      ev;

    switch (tash_fsmState) {
    case TASH_IDLE: //-- Read the Event
        if (!siEmx_Event.empty()) {
            siEmx_Event.read(ev);
            if (ev.type == TX_EVENT) {
                tash_fsmState = TASH_RD_MEM_STATUS_1;
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
    case TASH_RD_MEM_STATUS_1: //-- Read the Memory Write Status #1 (this might also be the last)
        if (!siMEM_TxP_WrSts.empty()) {
            DmSts status = siMEM_TxP_WrSts.read();
            if (status.okay) {
                //OBSOLETE_20201124 ap_uint<17> txAppPtr = ev.address + ev.length;
                ap_uint<TOE_WINDOW_BITS+1> txAppPtr = ev.address + ev.length;
                //OBSOLETE_20201124 if (txAppPtr <= 0x10000) {  // [FIXME - Why '<=' and not '<' ?
                if (txAppPtr[TOE_WINDOW_BITS] == 1) {
                    // The TCP buffer wrapped around
                    tash_fsmState = TASH_RD_MEM_STATUS_2;
                }
                else {
                    // Update the 'txAppPtr' of the TX_SAR_TABLE
                    soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, txAppPtr.range(TOE_WINDOW_BITS-1, 0)));
                    // Forward event to [EVe] which will signal [TXe]
                    soEVe_Event.write(ev);
                    if (DEBUG_LEVEL & TRACE_TASH) {
                        printInfo(myName, "Received TXMEM write status = %d.\n", status.okay.to_int());
                    }
                    tash_fsmState = TASH_IDLE;
                }
            }
        }
        break;
    case TASH_RD_MEM_STATUS_2: //-- Read the Memory Write Status #2
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
            tash_fsmState = TASH_IDLE;
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
    static TxAppTableEntry          TX_APP_TABLE[TOE_MAX_SESSIONS];
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
 * @brief Stream Metadata Loader (Sml)
 *
 * @param[in]  siTAIF_SndReq      APP request to send from [TAIF].
 * @param[out] soTAIF_SndRep      APP send reply to [TAIF].
 * @param[out] soSTt_SessStateReq Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep Session state reply from StateTable (STt).
 * @param[out] soTat_AccessReq    Access request to TxAppTable (Tat).
 * @param[in]  siTat_AccessRep    Access reply from [Tat]
 * @param[out] soMwr_AppMeta      APP memory metadata to MemoryWriter (Mwr).
 * @param[out] soEmx_Event        Event to EventMultiplexer (Emx).
 *
 * @details
 *  The FSM of this process decides if the incoming application data is written
 *   to the TX buffer in DDR4 memory. The process reads the request to send from
 *   the application and loads the necessary metadata from the [Tat] for
 *   transmitting this data.
 *  A request to send consists of a session-id and a data-length information.
 *  Once this request is received from [TAIF], the state of the connection is
 *  checked as well as the available memory space, and a reply is sent back to
 *  [TAIF].
 *   1) If the return code is 'NO_ERROR', the application is allowed to send.
 *      In  that case, the segment memory pointer is loaded into the TxAppTable
 *      and the segment's metadata is forwarded to MemoryWriter (Mwr) process
 *      which will use it to write the incoming data stream into DDR4 memory.
 *   2) If there is not enough space available in the Tx buffer of the session,
 *      the application can either retry its request later or decide to abandon
 *      the transmission.
 *   3) It the connection is not established, the application will be noticed it
 *      should act accordingly (e.g. by first opening the connection).
 *
 * @warning
 *  The outgoing stream 'soTAIF_SndRep is operated in non-blocking mode to avoid
 *   any stalling of this process. This means that the current reply to be sent
 *   on this stream will be dropped if the stream is full. As a result, the
 *   application process must provision enough buffering to store the reply
 *   returned by this process upon a request to send.
 *
 *  [TODO: Implement TCP_NODELAY]
 *******************************************************************************/
void pStreamMetaLoader(
        stream<TcpAppSndReq>        &siTAIF_SndReq,
        stream<TcpAppSndRep>        &soTAIF_SndRep,
        stream<SessionId>           &soSTt_SessStateReq,
        stream<TcpState>            &siSTt_SessStateRep,
        stream<TxAppTableQuery>     &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<AppMemMeta>          &soMwr_AppMeta,
        stream<Event>               &soEmx_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { READ_REQUEST=0,  READ_META } \
                                 mdl_fsmState = READ_REQUEST;
    #pragma HLS reset variable = mdl_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpAppSndReq mdl_appSndReq;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TxAppTableReply  txAppTableReply;
    TcpState         sessState;

    switch(mdl_fsmState) {
    case READ_REQUEST:
        if (!siTAIF_SndReq.empty()) {
            // Read the request to send
            siTAIF_SndReq.read(mdl_appSndReq);
            // Request state of the session
            assessSize(myName, soSTt_SessStateReq, "soSTt_SessStateReq", 2);  // [FIXME-Use constant for the length]
            soSTt_SessStateReq.write(mdl_appSndReq.sessId);
            // Request the value of ACK and MemPtr from TxAppTable
            assessSize(myName, soTat_AccessReq, "soTat_AccessReq", 2);  // [FIXME-Use constant for the length]
            soTat_AccessReq.write(TxAppTableQuery(mdl_appSndReq.sessId));
            if (DEBUG_LEVEL & TRACE_SML) {
                printInfo(myName, "Received new Tx request for session %d.\n", mdl_appSndReq.sessId.to_int());
            }
            mdl_fsmState = READ_META;
        }
        break;
    case READ_META:
        if (!siTat_AccessRep.empty() and !siSTt_SessStateRep.empty()) {
            siSTt_SessStateRep.read(sessState);
            siTat_AccessRep.read(txAppTableReply);
            TcpDatLen maxWriteLength = (txAppTableReply.ackd - txAppTableReply.mempt) - 1;
            /*** [TODO - TCP_NODELAY] ******************
            #if (TCP_NODELAY)
                ap_uint<16> usedLength = ((ap_uint<16>) writeSar.mempt - writeSar.ackd);
                ap_uint<16> usableWindow = 0;
                if (writeSar.min_window > usedLength) {
                    usableWindow = writeSar.min_window - usedLength;
                }
            #endif
            *******************************************/
            if (!soTAIF_SndRep.full()) {
                if (sessState != ESTABLISHED) {
                    // Notify APP about the none-established connection
                    soTAIF_SndRep.write(TcpAppSndRep(mdl_appSndReq.sessId, mdl_appSndReq.length, maxWriteLength, NO_CONNECTION));
                    printError(myName, "Session %d is not established. Current session state is \'%s\'.\n",
                               mdl_appSndReq.sessId.to_uint(), getTcpStateName(sessState));
                }
                else if (mdl_appSndReq.length > maxWriteLength) {
                    // Notify APP about the lack of space
                    soTAIF_SndRep.write(TcpAppSndRep(mdl_appSndReq.sessId, mdl_appSndReq.length, maxWriteLength, NO_SPACE));
                    printError(myName, "There is not enough TxBuf memory space available for session %d.\n",
                               mdl_appSndReq.sessId.to_uint());
                }
                else { //-- Session is ESTABLISHED and data-length <= maxWriteLength
                    // Forward the metadata to the SegmentMemoryWriter (Mwr)
                    soMwr_AppMeta.write(AppMemMeta(mdl_appSndReq.sessId, txAppTableReply.mempt, mdl_appSndReq.length));
                    // Notify APP about acceptance of the transmission
                    soTAIF_SndRep.write(TcpAppSndRep(mdl_appSndReq.sessId, mdl_appSndReq.length, maxWriteLength, NO_ERROR));
                    // Notify [TXe] about new data to be sent via an event to [EVe]
                    assessSize(myName, soEmx_Event, "soEmx_Event", 2);  // [FIXME-Use constant for the length]
                    soEmx_Event.write(Event(TX_EVENT, mdl_appSndReq.sessId, txAppTableReply.mempt, mdl_appSndReq.length));
                    // Update the 'txMemPtr' in TxAppTable
                    soTat_AccessReq.write(TxAppTableQuery(mdl_appSndReq.sessId, txAppTableReply.mempt + mdl_appSndReq.length));
                }
            }
            else {
                // Drop this reply
                printFatal(myName, "Cannot write 'soTAIF_SndRep()'. Stream is full!");
            }
            mdl_fsmState = READ_REQUEST;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Tx Memory Writer (Mwr)
 *
 * @param[in]  siTAIF_Data   APP data stream from [TAIF].
 * @param[in]  siSml_AppMeta APP memory metadata from StreamMetaLoader (Sml).
 * @param[out] soMEM_WrCmd   Tx memory write command to [MEM].
 * @param[out] soMEM_WrData  Tx memory write data to [MEM].
 *
 * @details
 *  This process writes the incoming APP data into the external DRAM upon a
 *   a request issued by the state machine of the MetaDataLoader (Sml).
 *  The Tx buffer memory is organized and managed as a circular buffer in the
 *   DRAM and it may happen that the APP data to be transmitted does not fit
 *   into the remaining memory buffer space because the memory pointer needs to
 *   wrap around. In such a case, the incoming APP data is broken down and is
 *   written into the physical DRAM as two memory buffers.
 *******************************************************************************/
void pTxMemoryWriter(
        stream<TcpAppData>  &siTAIF_Data,
        stream<AppMemMeta>  &siSml_AppMeta,
        stream<DmCmd>       &soMEM_WrCmd,
        stream<AxisApp>     &soMEM_WrData)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "Mwr");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { MWR_IDLE=0,
                            MWR_FWD_ALIGNED, MWR_SPLIT_1ST_CMD,
                            MWR_FWD_1ST_BUF, MWR_FWD_2ND_BUF, MWR_RESIDUE } \
                                 mwr_fsmState=MWR_IDLE;
    #pragma HLS RESET variable = mwr_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AppMemMeta    mwr_appMemMeta;
    static DmCmd         mwr_memWrCmd;
    static AxisApp       mwr_currChunk;
    static TxBufPtr      mwr_firstAccLen;
    static TcpDatLen     mwr_nrBytesToWr;
    static ap_uint<4>    mwr_splitOffset;
    static uint16_t      mwr_debugCounter=1;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    static uint8_t       lengthBuffer;
    static ap_uint<3>    accessResidue;

    switch (mwr_fsmState) {
    case MWR_IDLE:
        if (!siSml_AppMeta.empty() and !soMEM_WrCmd.full()) {
            siSml_AppMeta.read(mwr_appMemMeta);
            //-- Build a memory address for this segment
            TxMemPtr memSegAddr = TOE_TX_MEMORY_BASE; // 0x40000000
            memSegAddr(29, 16) = mwr_appMemMeta.sessId(13, 0);
            memSegAddr(15,  0) = mwr_appMemMeta.addr;
            // Build a data mover command for this segment
            mwr_memWrCmd = DmCmd(memSegAddr, mwr_appMemMeta.len);
            if ((mwr_memWrCmd.saddr(TOE_WINDOW_BITS-1, 0) + mwr_memWrCmd.bbt) > TOE_TX_BUFFER_SIZE) {
                // This segment must be broken in two memory accesses because TCP Tx memory buffer wraps around
                if (DEBUG_LEVEL & TRACE_MWR) {
                    printInfo(myName, "TCP Tx memory buffer wraps around: This segment must be broken in two memory accesses.\n");
                }
                mwr_fsmState = MWR_SPLIT_1ST_CMD;
            }
            else {
                soMEM_WrCmd.write(mwr_memWrCmd);
                mwr_firstAccLen = mwr_memWrCmd.bbt;
                mwr_nrBytesToWr = mwr_firstAccLen;
                mwr_fsmState = MWR_FWD_ALIGNED;
                if (DEBUG_LEVEL & TRACE_MWR) {
                    printInfo(myName, "Issuing memory write command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.bbt.to_uint());
                    mwr_debugCounter++;
                }
            }
        }
        break;
    case MWR_SPLIT_1ST_CMD:
        if (!soMEM_WrCmd.full()) {
            mwr_firstAccLen   = TOE_TX_BUFFER_SIZE - mwr_memWrCmd.saddr;
            mwr_nrBytesToWr   = mwr_firstAccLen;
            soMEM_WrCmd.write(DmCmd(mwr_memWrCmd.saddr, mwr_firstAccLen));
            if (DEBUG_LEVEL & TRACE_MWR) {
                printInfo(myName, "Issuing 1st memory write command #%d - SADDR=0x%9.9x - BBT=%d\n",
                    mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_firstAccLen.to_uint());
            }
            mwr_fsmState = MWR_FWD_1ST_BUF;
        }
        break;
    case MWR_FWD_ALIGNED:
        if (!siTAIF_Data.empty() and !soMEM_WrData.full()) {
            //-- Default streaming state used to forward APP data or splitted
            //-- buffers that are aligned with to the Axis raw width.
            AxisApp memChunk = siTAIF_Data.read();
            soMEM_WrData.write(memChunk);
            if (memChunk.getTLast()) {
                 mwr_fsmState = MWR_IDLE;
            }
         }
        break;
    case MWR_FWD_1ST_BUF:
        if (!siTAIF_Data.empty() and !soMEM_WrData.full()) {
            //-- Create 1st splitted data buffer and stream it to memory
            siTAIF_Data.read(mwr_currChunk);
            AxisApp memChunk = mwr_currChunk;
            if (mwr_nrBytesToWr > (ARW/8)) {
                mwr_nrBytesToWr -= (ARW/8);
            }
            else if (!soMEM_WrCmd.full()) {
                if (mwr_nrBytesToWr == (ARW/8)) {
                    memChunk.setLE_TLast(TLAST);

                    mwr_memWrCmd.saddr(TOE_WINDOW_BITS-1, 0) = 0;
                    mwr_memWrCmd.bbt -= mwr_firstAccLen;
                    soMEM_WrCmd.write(mwr_memWrCmd);
                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printInfo(myName, "Issuing 2nd memory write command #%d - SADDR=0x%9.9x - BBT=%d\n",
                                  mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.bbt.to_uint());
                        mwr_debugCounter++;
                    }
                    mwr_fsmState = MWR_FWD_ALIGNED;
                }
                else {
                    memChunk.setLE_TLast(TLAST);
                    memChunk.setLE_TKeep(lenToLE_tKeep(mwr_nrBytesToWr));
                    #ifndef __SYNTHESIS__
                    memChunk.setLE_TData(0, (ARW-1), ((int)mwr_nrBytesToWr*8));
                    #endif

                    mwr_memWrCmd.saddr(TOE_WINDOW_BITS-1, 0) = 0;
                    mwr_memWrCmd.bbt -= mwr_firstAccLen;
                    soMEM_WrCmd.write(mwr_memWrCmd);
                    if (DEBUG_LEVEL & TRACE_MWR) {
                        printInfo(myName, "Issuing 2nd memory write command #%d - SADDR=0x%9.9x - BBT=%d\n",
                                  mwr_debugCounter, mwr_memWrCmd.saddr.to_uint(), mwr_memWrCmd.bbt.to_uint());
                        mwr_debugCounter++;
                    }
                    mwr_splitOffset = (ARW/8) - mwr_nrBytesToWr;
                    mwr_fsmState = MWR_FWD_2ND_BUF;
                }
            }
            soMEM_WrData.write(memChunk);
            if (DEBUG_LEVEL & TRACE_MWR) { printAxisRaw(myName, "soMEM_WrData =", memChunk); }
        }
        break;
    case MWR_FWD_2ND_BUF:
        if (!siTAIF_Data.empty() and !soMEM_WrData.full()) {
            //-- Alternate streaming state used to re-align a splitted second buffer
            AxisApp prevChunk = mwr_currChunk;
            mwr_currChunk = siTAIF_Data.read();

            AxisApp joinedChunk(0,0,0);  // [FIXME-Create a join method in AxisRaw]
            // Set lower-part of the joined chunk with the last bytes of the previous chunk
            joinedChunk.setLE_TData(prevChunk.getLE_TData((ARW  )-1, ((ARW  )-((int)mwr_splitOffset*8))),
                                                                              ((int)mwr_splitOffset*8)-1, 0);
            joinedChunk.setLE_TKeep(prevChunk.getLE_TKeep((ARW/8)-1, ((ARW/8)-((int)mwr_splitOffset  ))),
                                                                              ((int)mwr_splitOffset  )-1, 0);
            // Set higher part of the joined chunk with the first bytes of the current chunk
            joinedChunk.setLE_TData(mwr_currChunk.getLE_TData((ARW  )-((int)mwr_splitOffset*8)-1, 0),
                                                              (ARW  )                         -1, ((int)mwr_splitOffset*8));
            joinedChunk.setLE_TKeep(mwr_currChunk.getLE_TKeep((ARW/8)-((int)mwr_splitOffset  )-1, 0),
                                                              (ARW/8)                         -1, ((int)mwr_splitOffset  ));
            if (mwr_currChunk.getLE_TLast()) {
                if (mwr_currChunk.getLen() > mwr_nrBytesToWr) {
                    // This cannot be the last chunk because the current one plus
                    // the remainder of the previous one do not fit into a single chunk.
                    // Goto the 'MWR_RESIDUE' and handle the remainder of that chunk.
                    mwr_fsmState = MWR_RESIDUE;
                }
                else {
                    // The entire current chunk and the remainder of the previous chunk
                    // fit into a single chunk. We are done with this 2nd memory buffer.
                    joinedChunk.setLE_TLast(TLAST);
                    mwr_fsmState = MWR_IDLE;
                }
            }
            soMEM_WrData.write(joinedChunk);
            if (DEBUG_LEVEL & TRACE_MWR) {
                printAxisRaw(myName, "soMEM_WrData =", joinedChunk);
            }
        }
        break;
    case MWR_RESIDUE:
        if (!soMEM_WrData.full()) {
            //-- Output the very last unaligned chunk
            AxisApp prevChunk = mwr_currChunk;
            AxisApp lastChunk(0,0,0);

            // Set lower-part of the last chunk with the last bytes of the previous chunk
            lastChunk.setLE_TData(prevChunk.getLE_TData((ARW  )-1, ((ARW  )-((int)mwr_splitOffset*8))),
                                                                            ((int)mwr_splitOffset*8)-1, 0);
            lastChunk.setLE_TKeep(prevChunk.getLE_TKeep((ARW/8)-1, ((ARW/8)-((int)mwr_splitOffset  ))),
                                                                            ((int)mwr_splitOffset  )-1, 0);
            lastChunk.setLE_TLast(TLAST);
            soMEM_WrData.write(lastChunk);
            if (DEBUG_LEVEL & TRACE_MWR) {
                printAxisRaw(myName, "soMEM_TxP_Data =", lastChunk);
            }
            mwr_fsmState = MWR_IDLE;
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
 * @param[in]  siTAIF_Data           APP data stream from [TAIF].
 * @param[in]  siTAIF_SndReq         APP request to send from [TAIF].
 * @param[out] soTAIF_SndRep         APP send reply to [TAIF].
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
 *
 * @warning
 *  To avoid any stalling of this process, the outgoing streams 'soTAIF_OpnRep
 *  and 'soTAIF_Sndrep' are operated in non-blocking mode. This implies that the
 *  user application process connected to these streams must provision enough
 *  buffering to store the corresponding bytes exchanged on these interfaces.
 *******************************************************************************/
void tx_app_interface(
        //-- TAIF / Open-Close Interfaces
        stream<TcpAppOpnReq>           &siTAIF_OpnReq,
        stream<TcpAppOpnRep>           &soTAIF_OpnRep,
        stream<TcpAppClsReq>           &siTAIF_ClsReq,
        //-- TAIF / Send Data Interfaces
        stream<TcpAppData>             &siTAIF_Data,
        stream<TcpAppSndReq>           &siTAIF_SndReq,
        stream<TcpAppSndRep>           &soTAIF_SndRep,
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

    static stream<AppMemMeta>           ssSmlToMwr_AppMeta   ("ssSmlToMwr_AppMeta");
    #pragma HLS stream         variable=ssSmlToMwr_AppMeta   depth=32
    #pragma HLS DATA_PACK      variable=ssSmlToMwr_AppMeta

    static stream<Event>                ssSmlToEmx_Event     ("ssSmlToEmx_Event");
    #pragma HLS stream         variable=ssSmlToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=ssSmlToEmx_Event

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

    pStreamMetaLoader(
            siTAIF_SndReq,
            soTAIF_SndRep,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            ssSmlToTat_AccessQry,
            ssTatToSml_AccessRep,
            ssSmlToMwr_AppMeta,
            ssSmlToEmx_Event);

    pTxMemoryWriter(
            siTAIF_Data,
            ssSmlToMwr_AppMeta,
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
