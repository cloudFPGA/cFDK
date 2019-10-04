/************************************************
Copyright (c) 2016, Xilinx, Inc.
Copyright (c) 2016-2019, IBM Research.

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

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TAA_TRACE | TAS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TAi"

#define TRACE_OFF  0x0000
#define TRACE_TAA 1 <<  1
#define TRACE_TAS 1 <<  2
#define TRACE_TAT 1 <<  3
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * @brief Tx Application Accept (Taa).
 *
 * @param[in]  siTRIF_OpnReq,        Open connection request from TCP Role I/F (TRIF).
 * @param[]
 * @param[in]  siSLc_SessLookupRep,  Reply from [SLc]
 * @param[out] siPRt_ActPortStateRep,Active port state reply from [PRt].
 * @param[in]  siRXe_SessOpnSts,     Session open status from [RXe].
 * @param[out] soTRIF_SessOpnSts,    Session open status to [TRIF].
 * @param[out] soSLc_SessLookupReq,  Request a session lookup to Session Lookup Controller (SLc).
 * @param[out] soPRt_GetFreePortReq, Request to get a free port to Port Table {PRt).
 * @param[out  soSTt_SessStateQry,   Session state query to StateTable (STt).
 * @param[in]  siSTt_SessStateRep,   Session state reply from [STt].
 * @param[out] soEVe_Event,          Event to EventEngine (EVe).
 * @param
 * @param
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
        stream<AxiSockAddr>         &siTRIF_OpnReq,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<TcpPort>             &siPRt_ActPortStateRep,
        stream<OpenStatus>          &siRXe_SessOpnSts,
        stream<OpenStatus>          &soTRIF_SessOpnSts,
        stream<AxiSocketPair>       &soSLc_SessLookupReq,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<StateQuery>          &soSTt_SessStateQry,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<event>               &soEVe_Event,
        stream<OpenStatus>          &rtTimer2txApp_notification,
        AxiIp4Address                regIpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //OBSOLETE-20190907 #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static ap_uint<16> tai_closeSessionID;

    static stream<AxiSockAddr>  localFifo ("localFifo");
    #pragma HLS stream variable=localFifo depth=4

    OpenStatus  openSessStatus;

    static enum TasFsmStates {TAS_IDLE=0, TAS_GET_FREE_PORT, TAS_CLOSE_CONN } tasFsmState = TAS_IDLE;

    switch (tasFsmState) {

    case TAS_IDLE:
        if (!siTRIF_OpnReq.empty() && !soPRt_GetFreePortReq.full()) {
            localFifo.write(siTRIF_OpnReq.read());
            soPRt_GetFreePortReq.write(1);
            tasFsmState = TAS_GET_FREE_PORT;
        }
        else if (!siSLc_SessLookupRep.empty()) {
            // Read the session and check its state
            sessionLookupReply session = siSLc_SessLookupRep.read();
            if (session.hit) {
                soEVe_Event.write(event(SYN, session.sessionID));
                soSTt_SessStateQry.write(StateQuery(session.sessionID, SYN_SENT, 1));
            }
            else {
                // Tell the APP that the open connection failed
                soTRIF_SessOpnSts.write(OpenStatus(0, FAILED_TO_OPEN_SESS));
            }
        }
        else if (!siRXe_SessOpnSts.empty()) {
            // Read the status but do not forward to TRIF because it is actually
            // not waiting for this one
            siRXe_SessOpnSts.read(openSessStatus);
            soTRIF_SessOpnSts.write(openSessStatus);
        }
        else if (!rtTimer2txApp_notification.empty())
            soTRIF_SessOpnSts.write(rtTimer2txApp_notification.read());
        else if(!closeConnReq.empty()) {    // Close Request
            closeConnReq.read(tai_closeSessionID);
            soSTt_SessStateQry.write(StateQuery(tai_closeSessionID));
            tasFsmState = TAS_CLOSE_CONN;
        }
        break;

    case TAS_GET_FREE_PORT:
        if (!siPRt_ActPortStateRep.empty() && !soSLc_SessLookupReq.full()) {
            //OBSOLETE-20190521 ap_uint<16> freePort = siPRt_ActPortStateRep.read() + 32768; // 0x8000 is already added by PRt
            TcpPort     freePort      = siPRt_ActPortStateRep.read();
            AxiSockAddr axiServerAddr = localFifo.read();
            //OBSOLETE-20181218 soSLc_SessLookupReq.write(
            //        fourTuple(regIpAddress, byteSwap32(server_addr.ip_address),
            //        byteSwap16(freePort), byteSwap16(server_addr.ip_port)));
            soSLc_SessLookupReq.write(AxiSocketPair(AxiSockAddr(regIpAddress,       byteSwap16(freePort)),
                                                    AxiSockAddr(axiServerAddr.addr, axiServerAddr.port)));
            tasFsmState = TAS_IDLE;
        }
        break;

    case TAS_CLOSE_CONN:
        if (!siSTt_SessStateRep.empty()) {
            SessionState state = siSTt_SessStateRep.read();
            //TODO might add CLOSE_WAIT here???
            if ((state == ESTABLISHED) || (state == FIN_WAIT_2) || (state == FIN_WAIT_1)) { //TODO Why if FIN already SENT
                soSTt_SessStateQry.write(StateQuery(tai_closeSessionID, FIN_WAIT_1, 1));
                soEVe_Event.write(event(FIN, tai_closeSessionID));
            }
            else
                soSTt_SessStateQry.write(StateQuery(tai_closeSessionID, state, 1)); // Have to release lock
            tasFsmState = TAS_IDLE;
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
        stream<event>             &siEmx_Event,
        stream<TAiTxSarPush>      &soTSt_PushCmd,
        stream<event>             &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    static event      ev;

    static enum TashFsmStates { S0, S1, S2 } tashFsmState = S0;

    switch (tashFsmState) {
    case S0:
        if (!siEmx_Event.empty()) {
            siEmx_Event.read(ev);
            if (ev.type == TX)
                tashFsmState = S1;
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
                tashFsmState = S0;
            }
            else
                tashFsmState = S2;
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
            tashFsmState = S0;
        }
        break;
    } //switch
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

    static TxAppTableEntry          TX_APP_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_APP_TABLE inter false

    TStTxSarPush      ackPush;
    TxAppTableRequest txAppUpdate;

    if (!siTSt_PushCmd.empty()) {
        siTSt_PushCmd.read(ackPush);
        if (ackPush.init) { // At init this is actually not_ackd
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd-1;
            TX_APP_TABLE[ackPush.sessionID].mempt = ackPush.ackd;
        }
        else
            TX_APP_TABLE[ackPush.sessionID].ackd = ackPush.ackd;
    }
    else if (!siTas_AcessReq.empty()) {
        siTas_AcessReq.read(txAppUpdate);

        if(txAppUpdate.write) // Write
            TX_APP_TABLE[txAppUpdate.sessId].mempt = txAppUpdate.mempt;
        else // Read
            siTas_AcessRep.write(TxAppTableReply(txAppUpdate.sessId, TX_APP_TABLE[txAppUpdate.sessId].ackd, TX_APP_TABLE[txAppUpdate.sessId].mempt));
    }

}


/*****************************************************************************
 * @brief The tx_app_interface (TAi) is front-end of the TCP Role I/F (TRIF).
 *
 * @param[in]  siTRIF_OpnReq,         Open connection request from TCP Role I/F (TRIF).
 * @param[out] soTRIF_SessOpnSts,     Open status to [TRIF].
 * @param[in]  siTRIF_Data,           TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,           TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,           TCP data status to TRIF.
 *
 * @param[out] soSTt_Tas_SessStateReq,Session sate request to StateTable (STt).
 * @param[in]  siSTt_Tas_SessStateRep,Session state reply from [STt].
 * @param[in]  siTSt_AckPush,         The push of an AckNum onto the ACK table of [TAi].
 * @param[]
 * @param[in]  siMEM_TxP_WrSts,       Tx memory write status from MEM.
 * @param[]
 * @param[]
 * @param[out] siPRt_ActPortStateRep, Active port state reply from [PRt].
 * @param[]
 * @param[in]  siRXe_SessOpnSts, Session open status from [RXe].
 * @param[]
 * @param[out] soSTt_SessStateReq,    State request to StateTable (STt).
 * @param[]
 * @param[]
 * @param[]


 * @param[out] soSLc_SessLookupReq,   Session lookup request to Session Lookup Controller(SLc).
 * @param[out] soPRt_ActPortStateReq  Request to get a free port to Port Table (PRt).
 * @param[]
 * @param[]
 * @param[]
 * @param[]
 * @param[out] soMEM_TxP_WrCmd,       Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,        Tx memory data to MEM.
 * @param[out] soTSt_PushCmd,         Push command to TxSarTable (TSt).
 * @param
 * @param[out] soSTt_Taa_SessStateQry,Session state query to [STt].
 * @param[in]  siSTt_Taa_SessStateRep,Session state reply from [STt].
 * @param[out] soEVe_Event,           Event to EventEngine (EVe).
 * @parm
 * @parm
 *
 * @details
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void tx_app_interface(
        //-- TRIF / Open Interfaces
        stream<AxiSockAddr>            &siTRIF_OpnReq,
        stream<OpenStatus>             &soTRIF_SessOpnSts,
        //-- TRIF / Data Stream Interfaces
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<AppWrSts>               &soTRIF_DSts,
        //--
        stream<TcpSessId>              &soSTt_Tas_SessStateReq,
        stream<SessionState>           &siSTt_Tas_SessStateRep,
        stream<TStTxSarPush>           &siTSt_PushCmd,
        stream<DmSts>                  &siMEM_TxP_WrSts,

        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,

        stream<OpenStatus>             &siRXe_SessOpnSts,


        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<TAiTxSarPush>           &soTSt_PushCmd,


        stream<AxiSocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<StateQuery>             &soSTt_Taa_SessStateQry,
        stream<SessionState>           &siSTt_Taa_SessStateRep,
        stream<event>                  &soEVe_Event,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        ap_uint<32>                     regIpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-- Event Multiplexer (Emx) -----------------------------------------
    static stream<event>                sEmxToTash_Event    ("sEmxToTash_Event");
    #pragma HLS stream         variable=sEmxToTash_Event    depth=2
    #pragma HLS DATA_PACK      variable=sEmxToTash_Event

    //-- Tx App Accept (Taa)----------------------------------------------
    static stream<event>                sTaaToEmx_Event     ("sTaaToEmx_Event");
    #pragma HLS stream         variable=sTaaToEmx_Event     depth=2
    #pragma HLS DATA_PACK      variable=sTaaToEmx_Event

    //-- Tx App Stream (Tas)----------------------------------------------
    static stream<TxAppTableRequest>    sTasToApt_AcessReq  ("sTasToApt_AcessReq");
    #pragma HLS stream         variable=sTasToApt_AcessReq  depth=2
    #pragma HLS DATA_PACK      variable=sTasToApt_AcessReq

    static stream<event>                sTasToEmx_Event     ("sTasToEmx_Event");
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
            appCloseConnReq,
            siSLc_SessLookupRep,
            siPRt_ActPortStateRep,
            siRXe_SessOpnSts,
            soTRIF_SessOpnSts,
            soSLc_SessLookupReq,
            soPRt_GetFreePortReq,
            soSTt_Taa_SessStateQry,
            siSTt_Taa_SessStateRep,
            sTaaToEmx_Event,
            rtTimer2txApp_notification,
            regIpAddress);

    // Tx Application Table (Tat)
    pTxAppTable(
            siTSt_PushCmd,
            sTasToApt_AcessReq,
            sTatToTas_AcessRep);
}
