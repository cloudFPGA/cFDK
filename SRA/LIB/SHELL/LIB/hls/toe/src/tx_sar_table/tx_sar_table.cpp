/*****************************************************************************
 * @file       : tx_sar_table.cpp
 * @brief      : Tx Segmentation And Re-assembly Table (TSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "tx_sar_table.hpp"

using namespace hls;

#define THIS_NAME "TOE/TSt"


/*******************************************************************************
 * @brief Tx Sar Table (TSt) - Stores the data structures for managing the
 *         TCP Tx buffer and Tx sliding window.
 *
 * @param[in]  rxEng2txSar_upd_req
 * @param[in]  siTXe_TxSarQry,       Tx SAR query from TxEngine (TXe).
 * @param[out] soTXe_TxSarRep,       Tx SAR reply to [TXe].
 * @param[in]  siTAi_AppPush,        Push 'app' from TxAppInterface (TAi).
 * @param[out] txSar2rxEng_upd_rsp
 * @param[out] soTAi_AckPush,       Pushes an AckNum onto the ACK table of [TAi].
 *
 * @details
 *  This process is accessed by the RxEngine (RXe), the TxAppInterface (TAi) and
 *   the TxEngine (TXe).
 *
 * @TODO check if locking is actually required, especially for rxOut
 *
 *******************************************************************************/
void tx_sar_table(
        stream<RXeTxSarQuery>      &rxEng2txSar_upd_req,
        stream<TXeTxSarQuery>      &siTXe_TxSarQry,
        stream<TXeTxSarReply>      &soTXe_TxSarRep,
        stream<TxSarTableAppPush>  &siTAi_AppPush,
        stream<RXeTxSarReply>      &txSar2rxEng_upd_rsp,
        stream<txSarAckPush>       &soTAi_AckPush)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    static TxSarEntry               TX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_SAR_TABLE inter false
    #pragma HLS RESOURCE   variable=TX_SAR_TABLE core=RAM_T2P_BRAM

    TXeTxSarQuery     sTXeQry;
    TXeTxSarRtQuery   txEngRtUpdate;
    RXeTxSarQuery     tst_rxEngUpdate;
    TxSarTableAppPush appPush;

    if (!siTXe_TxSarQry.empty()) {
        //----------------------------------------
        //-- Query from TX Engine
        //----------------------------------------
        siTXe_TxSarQry.read(sTXeQry);
        if (sTXeQry.write) {
            if (!sTXeQry.isRtQuery) {
                TX_SAR_TABLE[sTXeQry.sessionID].not_ackd = sTXeQry.not_ackd;
                if (sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].app         = sTXeQry.not_ackd;
                    TX_SAR_TABLE[sTXeQry.sessionID].ackd        = sTXeQry.not_ackd-1;
                    TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = 0xFFFF;
                    // Init ACK to txAppInterface
                    soTAi_AckPush.write(txSarAckPush(sTXeQry.sessionID,
                                                     sTXeQry.not_ackd, 1));
                }
                if (sTXeQry.finReady)
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady = sTXeQry.finReady;
                if (sTXeQry.finSent)
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent  = sTXeQry.finSent;
            }
            else {
                txEngRtUpdate = sTXeQry;
                TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = txEngRtUpdate.getThreshold();
                TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS) TODO is this correct or less, eg. 1/2 * MSS
            }
        }
        else {
            //-- Read -----------------
            TcpWindow   minWindow;
            if (TX_SAR_TABLE[sTXeQry.sessionID].cong_window < TX_SAR_TABLE[sTXeQry.sessionID].recv_window)
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].cong_window;
            else
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].recv_window;
            soTXe_TxSarRep.write(TXeTxSarReply( TX_SAR_TABLE[sTXeQry.sessionID].ackd,
                                                TX_SAR_TABLE[sTXeQry.sessionID].not_ackd,
                                                minWindow,
                                                TX_SAR_TABLE[sTXeQry.sessionID].app,
                                                TX_SAR_TABLE[sTXeQry.sessionID].finReady,
                                                TX_SAR_TABLE[sTXeQry.sessionID].finSent));
        }
    }

    else if (!siTAi_AppPush.empty()) {
        //---------------------------------------
        //-- Push app pointer
        //---------------------------------------
        siTAi_AppPush.read(appPush);
        TX_SAR_TABLE[appPush.sessionID].app = appPush.app;
    }
    else if (!rxEng2txSar_upd_req.empty()) {
        //---------------------------------------
        //-- RX Engine
        //---------------------------------------
        rxEng2txSar_upd_req.read(tst_rxEngUpdate);
        if (tst_rxEngUpdate.write) {
            TX_SAR_TABLE[tst_rxEngUpdate.sessionID].ackd        = tst_rxEngUpdate.ackd;
            TX_SAR_TABLE[tst_rxEngUpdate.sessionID].recv_window = tst_rxEngUpdate.recv_window;
            TX_SAR_TABLE[tst_rxEngUpdate.sessionID].cong_window = tst_rxEngUpdate.cong_window;
            TX_SAR_TABLE[tst_rxEngUpdate.sessionID].count       = tst_rxEngUpdate.count;
            if (tst_rxEngUpdate.init == 1) {
                TX_SAR_TABLE[tst_rxEngUpdate.sessionID].finReady = false;
                TX_SAR_TABLE[tst_rxEngUpdate.sessionID].finSent = false;
            }
            // Push ACK to txAppInterface
            soTAi_AckPush.write(txSarAckPush(tst_rxEngUpdate.sessionID, tst_rxEngUpdate.ackd));
        }
        else {
            txSar2rxEng_upd_rsp.write(RXeTxSarReply(TX_SAR_TABLE[tst_rxEngUpdate.sessionID].ackd,
                                                    TX_SAR_TABLE[tst_rxEngUpdate.sessionID].not_ackd,
                                                    TX_SAR_TABLE[tst_rxEngUpdate.sessionID].cong_window,
                                                    TX_SAR_TABLE[tst_rxEngUpdate.sessionID].slowstart_threshold,
                                                    TX_SAR_TABLE[tst_rxEngUpdate.sessionID].count));
        }
    }
}
