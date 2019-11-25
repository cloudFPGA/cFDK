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
#define TRACE_TAA  1 <<  1
#define TRACE_TAS  1 <<  2
#define TRACE_TAT  1 <<  3
#define TRACE_TASH 1 <<  4
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * @brief Tx Application Accept (Taa).
 *
 * @param[in]  siTRIF_OpnReq,        Open connection request from TCP Role I/F (TRIF).
 * @param[out] soTRIF_OpnRep,        Open connection reply to [TRIF].
 * @param[]
 * @param[in]  siSLc_SessLookupRep,  Reply from [SLc]
 * @param[out] siPRt_ActPortStateRep,Active port state reply from [PRt].
 * @param[in]  siRXe_SessOpnSts,     Session open status from [RXe].

 * @param[out] soSLc_SessLookupReq,  Request a session lookup to Session Lookup Controller (SLc).
 * @param[out] soPRt_GetFreePortReq, Request to get a free port to Port Table {PRt).
 * @param[out  soSTt_SessStateQry,   Session state query to StateTable (STt).
 * @param[in]  siSTt_SessStateRep,   Session state reply from [STt].
 * @param[out] soEVe_Event,          Event to EventEngine (EVe).
 * @param[todo]rtTimer2txApp_notification
 * @param[in]  piMMIO_IpAddr         IP4 Address from MMIO.
 *
 * @details
 *  This process performs the creation and tear down of the active connections.
 *   Active connections are the ones opened by the FPGA as client and they make
 *   use of dynamically assigned or ephemeral ports in the range 32,768 to
 *   65,535. The operations performed here are somehow similar to the 'accept'
 *   and 'close' system calls.
 *  The IP tuple of the new connection to open is read from 'siTRIF_OpnReq'.
 *
 *  [TODO]
 *   and then requests a free port number from 'port_table' and fires a SYN
 *   event. Once the connection is established it notifies the application
 *   through 'appOpenConOut' and delivers the Session-ID belonging to the new
 *   connection. (Client connection to remote HOST or FPGA socket (COn).)
 *  If opening of the connection is not successful this is also indicated
 *   through the 'appOpenConOut'. By sending the Session-ID through 'closeConIn'
 *   the application can initiate the teardown of the connection.
 *
 * @ingroup tx_app_if
 ******************************************************************************/
void pTxAppAccept(
        stream<LE_SockAddr>         &siTRIF_OpnReq,
        stream<OpenStatus>          &soTRIF_OpnRep,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<TcpPort>             &siPRt_ActPortStateRep,
        stream<OpenStatus>          &siRXe_SessOpnSts,
        stream<LE_SocketPair>       &soSLc_SessLookupReq,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<StateQuery>          &soSTt_SessStateQry,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<Event>               &soEVe_Event,
        stream<OpenStatus>          &rtTimer2txApp_notification,
        LE_Ip4Address                piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Taa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {TAA_IDLE=0, TAA_GET_FREE_PORT, TAA_CLOSE_CONN } taaFsmState = TAA_IDLE;
    #pragma HLS reset                                             variable=taaFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SessionId            tai_closeSessionID;

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<LE_SockAddr>  localFifo ("localFifo");
    #pragma HLS stream variable=localFifo depth=4

    OpenStatus  openSessStatus;

    switch (taaFsmState) {

    case TAA_IDLE:
        if (!siTRIF_OpnReq.empty() && !soPRt_GetFreePortReq.full()) {
            localFifo.write(siTRIF_OpnReq.read());
            soPRt_GetFreePortReq.write(1);
            taaFsmState = TAA_GET_FREE_PORT;
        }
        else if (!siSLc_SessLookupRep.empty()) {
            // Read the session and check its state
            sessionLookupReply session = siSLc_SessLookupRep.read();
            if (session.hit) {
                soEVe_Event.write(Event(SYN_EVENT, session.sessionID));
                soSTt_SessStateQry.write(StateQuery(session.sessionID, SYN_SENT, 1));
            }
            else {
                // Tell the APP that the open connection failed
                soTRIF_OpnRep.write(OpenStatus(0, FAILED_TO_OPEN_SESS));
            }
        }
        else if (!siRXe_SessOpnSts.empty()) {
            // Read the status but do not forward to TRIF because it is actually
            // not waiting for this one
            siRXe_SessOpnSts.read(openSessStatus);
            soTRIF_OpnRep.write(openSessStatus);
        }
        else if (!rtTimer2txApp_notification.empty())
            soTRIF_OpnRep.write(rtTimer2txApp_notification.read());
        else if(!closeConnReq.empty()) {    // Close Request
            closeConnReq.read(tai_closeSessionID);
            soSTt_SessStateQry.write(StateQuery(tai_closeSessionID));
            taaFsmState = TAA_CLOSE_CONN;
        }
        break;

    case TAA_GET_FREE_PORT:
        if (!siPRt_ActPortStateRep.empty() && !soSLc_SessLookupReq.full()) {
            TcpPort     freePort      = siPRt_ActPortStateRep.read();
            LE_SockAddr leServerAddr = localFifo.read();
            soSLc_SessLookupReq.write(LE_SocketPair(LE_SockAddr(piMMIO_IpAddr,     byteSwap16(freePort)),
                                                    LE_SockAddr(leServerAddr.addr, leServerAddr.port)));
            taaFsmState = TAA_IDLE;
        }
        break;

    case TAA_CLOSE_CONN:
        if (!siSTt_SessStateRep.empty()) {
            SessionState state = siSTt_SessStateRep.read();
            //TODO might add CLOSE_WAIT here???
            if ((state == ESTABLISHED) || (state == FIN_WAIT_2) || (state == FIN_WAIT_1)) { //TODO Why if FIN already SENT
                soSTt_SessStateQry.write(StateQuery(tai_closeSessionID, FIN_WAIT_1, 1));
                soEVe_Event.write(Event(FIN_EVENT, tai_closeSessionID));
            }
            else
                soSTt_SessStateQry.write(StateQuery(tai_closeSessionID, state, 1)); // Have to release lock
            taaFsmState = TAA_IDLE;
        }
        break;
    }
}


/*****************************************************************************
 * @brief Tx Application Status Handler (Tash).
 *
 * @param[in]  siMEM_TxP_WrSts, Tx memory write status from [MEM].
 * @param[in]  siEmx_Event,     Event from the event multiplexer (Emx).
 * @param[out] soTSt_PushCmd,   Push command to TxSarTable (TSt).
 * @param[out] soEVe_Event,     Event to EventEngine (EVe).
 *
 * @details
 *
 * @ingroup tx_app_interface
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
    static enum TashFsmStates { S0=0, S1, S2 } tashFsmState = S0;
    #pragma HLS reset                 variable=tashFsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static Event      ev;

    switch (tashFsmState) {
    case S0:
        if (!siEmx_Event.empty()) {
            siEmx_Event.read(ev);
            if (ev.type == TX_EVENT) {
                if (DEBUG_LEVEL & TRACE_TASH) {
                    printInfo(myName, "Received \'TX\' event from [TAI].\n");
                }
                tashFsmState = S1;
            }
            else
                soEVe_Event.write(ev);
        }
        break;

    case S1:
        if (!siMEM_TxP_WrSts.empty()) {
            DmSts status = siMEM_TxP_WrSts.read();
            ap_uint<17> tempLength = ev.address + ev.length;
            if (tempLength <= 0x10000) {
                if (status.okay) {
                    soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, tempLength.range(15, 0))); // App pointer update, pointer is released
                    soEVe_Event.write(ev);
                }
                if (DEBUG_LEVEL & TRACE_TASH) {
                    printInfo(myName, "Received 1st TXMEM write status = %d.\n", status.okay.to_int());
                }
                tashFsmState = S0;
            }
            else {
                tashFsmState = S2;
            }
        }
        break;

    case S2:
        if (!siMEM_TxP_WrSts.empty()) {
            DmSts status = siMEM_TxP_WrSts.read();
            ap_uint<17> tempLength = (ev.address + ev.length);
            if (status.okay) {
                // App pointer update, pointer is released
                soTSt_PushCmd.write(TAiTxSarPush(ev.sessionID, tempLength.range(15, 0)));
                soEVe_Event.write(ev);
            }
            if (DEBUG_LEVEL & TRACE_TASH) {
                printInfo(myName, "Received 2nd TXMEM write status = %d.\n", status.okay.to_int());
            }
            tashFsmState = S0;
        }
        break;

    } // End of: switch
}


/*****************************************************************************
 * @brief Tx Application Table (Tat).
 *
 * @param[in]  siTSt_PushCmd,  Push command from TxSarTable (TSt).
 * @param[in]  siTas_AcessReq, Access request from TxAppStream (Tas).
 * @param[out] soTAs_AcessRep, Access reply to [Tas].
 *
 * @details
 *  This table keeps tack of the Tx ACK numbers and Tx memory pointers.
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void pTxAppTable(
        stream<TStTxSarPush>      &siTSt_PushCmd,
        stream<TxAppTableRequest> &siTas_AcessReq,
        stream<TxAppTableReply>   &siTas_AcessRep)
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
    TxAppTableRequest txAppUpdate;

    if (!siTSt_PushCmd.empty()) {
        siTSt_PushCmd.read(ackPush);
        if (ackPush.init) { // At init this is actually not_ackd
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd-1;
            TX_APP_TABLE[ackPush.sessionID].mempt = ackPush.ackd;
        }
        else {
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd;
        }
    }
    else if (!siTas_AcessReq.empty()) {
        siTas_AcessReq.read(txAppUpdate);
        if(txAppUpdate.write) { // Write
            TX_APP_TABLE[txAppUpdate.sessId].mempt = txAppUpdate.mempt;
        }
        else { // Read
            siTas_AcessRep.write(TxAppTableReply(txAppUpdate.sessId, TX_APP_TABLE[txAppUpdate.sessId].ackd, TX_APP_TABLE[txAppUpdate.sessId].mempt));
        }
    }
}


/*****************************************************************************
 * @brief The tx_app_interface (TAi) is front-end of the TCP Role I/F (TRIF).
 *
 * @param[in]  siTRIF_OpnReq,         Open connection request from TCP Role I/F (TRIF).
 * @param[out] soTRIF_OpnRep,         Open connection reply to [TRIF].
 * @param[in]  siTRIF_Data,           TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,           TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,           TCP data status to TRIF.
 * @param[out] soSTt_Tas_SessStateReq,Session sate request to StateTable (STt).
 * @param[in]  siSTt_Tas_SessStateRep,Session state reply from [STt].
 * @param[in]  siTSt_AckPush,         The push of an AckNum onto the ACK table of [TAi].
 * @param[in]  appCloseConnReq
 * @param[in]  siSLc_SessLookupRep
 * @param[in]  siPRt_ActPortStateRep, Active port state reply from [PRt].
 * @param[in]  siRXe_SessOpnSts, Session open status from [RXe].
 * @param[out] soMEM_TxP_WrCmd,       Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,        Tx memory data to MEM.
 * @param[in]  siMEM_TxP_WrSts,       Tx memory write status from MEM.
 * @param[out] soTSt_PushCmd,         Push command to TxSarTable (TSt).
 * @param[out] soSLc_SessLookupReq,   Session lookup request to Session Lookup Controller(SLc).
 * @param[out] soPRt_GetFreePortReq,  Request to get a free port to Port Table (PRt).
 * @param[out] soSTt_Taa_SessStateQry,Session state query to [STt].
 * @param[in]  siSTt_Taa_SessStateRep,Session state reply from [STt].
 * @param[out] soEVe_Event,           Event to EventEngine (EVe).
 * @param[]    rtTimer2txApp_notification
 * @param[in]  regIpAddress
 *
 * @details
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void tx_app_interface(
        //-- TRIF / Open Interfaces
        stream<LE_SockAddr>            &siTRIF_OpnReq,
        stream<OpenStatus>             &soTRIF_OpnRep,
        //-- TRIF / Data Stream Interfaces
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<AppWrSts>               &soTRIF_DSts,
        //--
        stream<TcpSessId>              &soSTt_Tas_SessStateReq,
        stream<SessionState>           &siSTt_Tas_SessStateRep,
        stream<TStTxSarPush>           &siTSt_PushCmd,

        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,
        stream<OpenStatus>             &siRXe_SessOpnSts,
        //-- MEM / Tx PATH Interface
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<DmSts>                  &siMEM_TxP_WrSts,

        stream<TAiTxSarPush>           &soTSt_PushCmd,
        stream<LE_SocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<StateQuery>             &soSTt_Taa_SessStateQry,
        stream<SessionState>           &siSTt_Taa_SessStateRep,
        stream<Event>                  &soEVe_Event,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        LE_Ip4Addr                      piMMIO_IpAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //OBSOLETE_20191009 #pragma HLS INLINE
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- Event Multiplexer (Emx) -----------------------------------------
    static stream<Event>                sEmxToTash_Event    ("sEmxToTash_Event");
    #pragma HLS stream         variable=sEmxToTash_Event    depth=2
    #pragma HLS DATA_PACK      variable=sEmxToTash_Event

    //-- Tx App Accept (Taa)----------------------------------------------
    static stream<Event>                sTaaToEmx_Event     ("sTaaToEmx_Event");
    #pragma HLS stream         variable=sTaaToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=sTaaToEmx_Event

    //-- Tx App Stream (Tas)----------------------------------------------
    static stream<TxAppTableRequest>    sTasToApt_AcessReq  ("sTasToApt_AcessReq");
    #pragma HLS stream         variable=sTasToApt_AcessReq  depth=2
    #pragma HLS DATA_PACK      variable=sTasToApt_AcessReq

    static stream<Event>                sTasToEmx_Event     ("sTasToEmx_Event");
    #pragma HLS stream         variable=sTasToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=sTasToEmx_Event

    //-- Tx App Table (Tat)-----------------------------------------------
    static stream<TxAppTableReply>      sTatToTas_AcessRep  ("sTatToTas_AcessRep");
    #pragma HLS stream         variable=sTatToTas_AcessRep  depth=2
    #pragma HLS DATA_PACK      variable=sTatToTas_AcessRep

    // Event Multiplexer (Emx)
    pStreamMux(
        sTaaToEmx_Event,
        sTasToEmx_Event,
        sEmxToTash_Event);

    // Tx Application Status Handler (Tash)
    pTxAppStatusHandler(
        siMEM_TxP_WrSts,
        sEmxToTash_Event,
        soTSt_PushCmd,
        soEVe_Event);

    // Tx Application Stream (Tas)
    tx_app_stream(
            siTRIF_Data,
            siTRIF_Meta,
            soTRIF_DSts,
            soSTt_Tas_SessStateReq,
            siSTt_Tas_SessStateRep,
            sTasToApt_AcessReq,
            sTatToTas_AcessRep,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            sTasToEmx_Event);

    // Tx Application Accept (Taa)
    pTxAppAccept(
            siTRIF_OpnReq,
            soTRIF_OpnRep,
            appCloseConnReq,
            siSLc_SessLookupRep,
            siPRt_ActPortStateRep,
            siRXe_SessOpnSts,
            soSLc_SessLookupReq,
            soPRt_GetFreePortReq,
            soSTt_Taa_SessStateQry,
            siSTt_Taa_SessStateRep,
            sTaaToEmx_Event,
            rtTimer2txApp_notification,
            piMMIO_IpAddr);

    // Tx Application Table (Tat)
    pTxAppTable(
            siTSt_PushCmd,
            sTasToApt_AcessReq,
            sTatToTas_AcessRep);
}
