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

    //OBSOLETE-20190617 static ap_uint<2> tash_state = 0;
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
 * @param[in]  siTRIF_Data,           TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,           TCP metadata stream from TRIF.
 * @param[out] soSTt_Tas_SessStateReq,Session sate request to StateTable (STt).
 * @param[in]  siSTt_Tas_SessStateRep,Session state reply from [STt].
 * @param[in]  siTSt_AckPush,         The push of an AckNum onto the ACK table of [TAi].
 * @param[]
 * @param[in]  siMEM_TxP_WrSts,       Tx memory write status from MEM.
 * @param[in]  siTRIF_OpnReq,         Open connection request from TCP Role I/F (TRIF).
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
 * @param[out] soTRIF_DSts,           TCP data status to TRIF.
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
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<TcpSessId>              &soSTt_Tas_SessStateReq,
        stream<SessionState>           &siSTt_Tas_SessStateRep,
        stream<TStTxSarPush>           &siTSt_PushCmd,
        stream<DmSts>                  &siMEM_TxP_WrSts,
        stream<AxiSockAddr>            &siTRIF_OpnReq,
        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,

        stream<OpenStatus>             &siRXe_SessOpnSts,

        stream<ap_int<17> >            &soTRIF_DSts,
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<TAiTxSarPush>           &soTSt_PushCmd,

        stream<OpenStatus>             &soTRIF_SessOpnSts,
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
            soSTt_Tas_SessStateReq,
            siSTt_Tas_SessStateRep,
            sTasToApt_AcessReq,
            sTatToTas_AcessRep,
            soTRIF_DSts,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            sTasToEmx_Event);

    // Tx Application Accept (Taa)
    tx_app_accept(
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
