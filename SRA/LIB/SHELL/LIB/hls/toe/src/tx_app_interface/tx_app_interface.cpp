/*****************************************************************************
 * @file       : tx_app_interface.cpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *****************************************************************************/

#include "tx_app_interface.hpp"

using namespace hls;


/*****************************************************************************
 * @brief Tx Application Status Handler (Tas).
 *
 * @param[]
 * @param[]
 * @param[]
 *
 * @details
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void txAppStatusHandler(
        stream<DmSts>           &txBufferWriteStatus,
        stream<event>           &tasi_eventCacheFifo,
        stream<txAppTxSarPush>  &txApp2txSar_app_push,
        stream<event>           &txAppStream2eventEng_setEvent)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    static ap_uint<2> tash_state = 0;
    static event ev;
    static ap_uint<1> wrStatusCounter = 0;

    switch (tash_state) {
    case 0:
        if (!tasi_eventCacheFifo.empty()) {
            wrStatusCounter = 0;
            tasi_eventCacheFifo.read(ev);
            if (ev.type == TX)
                tash_state = 1;
            else
                txAppStream2eventEng_setEvent.write(ev);
        }
        break;
    case 1:
        if (!txBufferWriteStatus.empty()) {
            DmSts status = txBufferWriteStatus.read();
            ap_uint<17> tempLength = ev.address + ev.length;
            if (tempLength <= 0x10000) {
                if (status.okay) {
                    txApp2txSar_app_push.write(txAppTxSarPush(ev.sessionID, tempLength.range(15, 0))); // App pointer update, pointer is released
                    txAppStream2eventEng_setEvent.write(ev);
                }
                tash_state = 0;
            }
            else
                tash_state = 2;
        }
        break;
    case 2:
        if (!txBufferWriteStatus.empty()) {
            DmSts status = txBufferWriteStatus.read();
            ap_uint<17> tempLength = (ev.address + ev.length);
            if (status.okay) {
                txApp2txSar_app_push.write(txAppTxSarPush(ev.sessionID, tempLength.range(15, 0))); // App pointer update, pointer is released
                txAppStream2eventEng_setEvent.write(ev);
            }
            tash_state = 0;
        }
        break;
    } //switch
}


/*****************************************************************************
 * @brief Tx Application Table (Tab).
 *
 * @param[]
 * @param[]
 * @param[]
 *
 * @details
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void tx_app_table(
        stream<txSarAckPush>&       txSar2txApp_ack_push,
        stream<txAppTxSarQuery>&    txApp_upd_req,
        stream<txAppTxSarReply>&    txApp_upd_rsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static txAppTableEntry app_table[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=app_table inter false
    txSarAckPush    ackPush;
    txAppTxSarQuery txAppUpdate;

    if (!txSar2txApp_ack_push.empty()) {
        txSar2txApp_ack_push.read(ackPush);
        if (ackPush.init) { // At init this is actually not_ackd
            app_table[ackPush.sessionID].ackd = ackPush.ackd-1;
            app_table[ackPush.sessionID].mempt = ackPush.ackd;
        }
        else
            app_table[ackPush.sessionID].ackd = ackPush.ackd;
    }
    else if (!txApp_upd_req.empty()) {
        txApp_upd_req.read(txAppUpdate);

        if(txAppUpdate.write) // Write
            app_table[txAppUpdate.sessionID].mempt = txAppUpdate.mempt;
        else // Read
            txApp_upd_rsp.write(txAppTxSarReply(txAppUpdate.sessionID, app_table[txAppUpdate.sessionID].ackd, app_table[txAppUpdate.sessionID].mempt));
    }

}


/*****************************************************************************
 * @brief The tx_app_interface (TAi) is front-end of the TCP Role I/F (TRIF).
 *
 * @param[in]  siTRIF_Data,           TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,           TCP metadata stream from TRIF.
 * @param[in]  siSTt_SessionStateRep, Session state rply from [STt].
 * @param[]
 * @param[]
 * @param[in]  siTRIF_OpnReq, Open connection request from TCP Role I/F (TRIF).
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
 * @param[out] soTRIF_SessOpnSts,     Open status to [TRIF].
 * @param[out] soSLc_SessLookupReq,   Session lookup request to Session Lookup Controller(SLc).
 * @param[out] soPRt_ActPortStateReq  Request to get a free port to Port Table (PRt).
 * @param[]
 * @param[]
 * @param[]
 * @param[]
 *
 * @details
 *
 * @ingroup tx_app_interface
 ******************************************************************************/
void tx_app_interface(
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<sessionState>           &siSTt_SessStateRep,
        stream<txSarAckPush>           &txSar2txApp_ack_push,
        stream<DmSts>                  &txBufferWriteStatus,

        stream<AxiSockAddr>            &siTRIF_OpnReq,
        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,
        stream<sessionState>           &stateTable2txApp_upd_rsp,
        stream<OpenStatus>             &siRXe_SessOpnSts,

        stream<ap_int<17> >            &appTxDataRsp,
        stream<TcpSessId>              &soSTt_SessStateReq,
        stream<DmCmd>                  &txBufferWriteCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<txAppTxSarPush>         &txApp2txSar_push,

        stream<OpenStatus>             &soTRIF_SessOpnSts,
        stream<AxiSocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<stateQuery>             &txApp2stateTable_upd_req,
        stream<event>                  &txApp2eventEng_setEvent,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        ap_uint<32>                     regIpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    // Fifos
    static stream<event> txApp2eventEng_mergeEvent("txApp2eventEng_mergeEvent");
    static stream<event> txAppStream2event_mergeEvent("txAppStream2event_mergeEvent");
    #pragma HLS stream variable=txApp2eventEng_mergeEvent       depth=2
    #pragma HLS stream variable=txAppStream2event_mergeEvent    depth=2
    #pragma HLS DATA_PACK variable=txApp2eventEng_mergeEvent
    #pragma HLS DATA_PACK variable=txAppStream2event_mergeEvent

    static stream<event> txApp_eventCache("txApp_eventCache");
    #pragma HLS stream variable=txApp_eventCache    depth=2
    #pragma HLS DATA_PACK variable=txApp_eventCache

    static stream<txAppTxSarQuery>      txApp2txSar_upd_req("txApp2txSar_upd_req");
    static stream<txAppTxSarReply>      txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");
    #pragma HLS stream variable=txApp2txSar_upd_req     depth=2
    #pragma HLS stream variable=txSar2txApp_upd_rsp     depth=2
    #pragma HLS DATA_PACK variable=txApp2txSar_upd_req
    #pragma HLS DATA_PACK variable=txSar2txApp_upd_rsp

    // Multiplex Events
    pStreamMux(
        txApp2eventEng_mergeEvent,
        txAppStream2event_mergeEvent,
        txApp_eventCache);

    txAppStatusHandler(
        txBufferWriteStatus,
        txApp_eventCache,
        txApp2txSar_push,
        txApp2eventEng_setEvent);

    // TX application Stream management
    tx_app_stream(
            siTRIF_Data,
            siTRIF_Meta,
            siSTt_SessStateRep,
            txSar2txApp_upd_rsp,
            appTxDataRsp,
            soSTt_SessStateReq,
            txApp2txSar_upd_req,
            txBufferWriteCmd,
            soMEM_TxP_Data,
            txAppStream2event_mergeEvent);

    // TX Application Status
    tx_app_if(
            siTRIF_OpnReq,
            appCloseConnReq,
            siSLc_SessLookupRep,
            siPRt_ActPortStateRep,
            stateTable2txApp_upd_rsp,
            siRXe_SessOpnSts,
            soTRIF_SessOpnSts,
            soSLc_SessLookupReq,
            soPRt_GetFreePortReq,
            txApp2stateTable_upd_req,
            txApp2eventEng_mergeEvent,
            rtTimer2txApp_notification,
            regIpAddress);

    // TX App Meta Table
    tx_app_table(
            txSar2txApp_ack_push,
            txApp2txSar_upd_req,
            txSar2txApp_upd_rsp);
}
