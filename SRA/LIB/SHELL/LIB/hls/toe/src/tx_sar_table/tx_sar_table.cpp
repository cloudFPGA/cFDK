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
 * @param[in]  siRXe_TxSarQry, Query from RxEngine (RXe).
 * @param[out] soRXe_TxSarRep, Reply to [RXe].
 * @param[in]  siTXe_TxSarQry, Tx SAR query from TxEngine (TXe).
 * @param[out] soTXe_TxSarRep, Tx SAR reply to [TXe].
 * @param[in]  siTAi_PushCmd,  Push command from TxAppInterface (TAi).
 * @param[out] soTAi_PushCmd,  Push command to [TAi].
 *
 * @details
 *  This process is accessed by the RxEngine (RXe), the TxEngine (TXe) and the
 *   TxAppInterface (TAi).
 *
 *******************************************************************************/
void tx_sar_table(
        stream<RXeTxSarQuery>      &siRXe_TxSarQry,
        stream<RXeTxSarReply>      &soRXe_TxSarRep,
        stream<TXeTxSarQuery>      &siTXe_TxSarQry,
        stream<TXeTxSarReply>      &soTXe_TxSarRep,
        stream<TAiTxSarPush>       &siTAi_PushCmd,
        stream<TStTxSarPush>       &soTAi_PushCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    static TxSarEntry               TX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_SAR_TABLE inter false
    #pragma HLS RESOURCE   variable=TX_SAR_TABLE core=RAM_T2P_BRAM

    TXeTxSarQuery     sTXeQry;
    TXeTxSarRtQuery   sTXeRtQry;
    RXeTxSarQuery     sRXeQry;
    TAiTxSarPush      sTAiCmd;

    if (!siTXe_TxSarQry.empty()) {
        //----------------------------------------
        //-- Query from TX Engine
        //----------------------------------------
        siTXe_TxSarQry.read(sTXeQry);
        if (sTXeQry.write) {
            //-- Write Query (or RtQuery)
            if (!sTXeQry.isRtQuery) {
                TX_SAR_TABLE[sTXeQry.sessionID].not_ackd = sTXeQry.not_ackd;
                if (sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].app         = sTXeQry.not_ackd;
                    TX_SAR_TABLE[sTXeQry.sessionID].ackd        = sTXeQry.not_ackd-1;
                    TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = 0xFFFF;
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady    = sTXeQry.finReady;
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent     = sTXeQry.finSent;
                    // Init ACK on the TxAppInterface side
                    soTAi_PushCmd.write(TStTxSarPush(sTXeQry.sessionID,
                                                     sTXeQry.not_ackd, CMD_INIT));
                }
                if (sTXeQry.finReady)
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady = sTXeQry.finReady;
                if (sTXeQry.finSent)
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent  = sTXeQry.finSent;
            }
            else {
                sTXeRtQry = sTXeQry;
                TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = sTXeRtQry.getThreshold();
                TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS) TODO is this correct or less, eg. 1/2 * MSS
            }
        }
        else {
            //-- Read Query
            TcpWindow   minWindow;
            if (TX_SAR_TABLE[sTXeQry.sessionID].cong_window < TX_SAR_TABLE[sTXeQry.sessionID].recv_window) {
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].cong_window;
            }
            else {
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].recv_window;
            }
            soTXe_TxSarRep.write(TXeTxSarReply(TX_SAR_TABLE[sTXeQry.sessionID].ackd,
                                               TX_SAR_TABLE[sTXeQry.sessionID].not_ackd,
                                               minWindow,
                                               TX_SAR_TABLE[sTXeQry.sessionID].app,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finReady,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finSent));
        }
    }

    else if (!siTAi_PushCmd.empty()) {
        //---------------------------------------
        //-- Push an new 'txAppPtr' in the table
        //---------------------------------------
        siTAi_PushCmd.read(sTAiCmd);
        TX_SAR_TABLE[sTAiCmd.sessionID].app = sTAiCmd.app;
    }
    else if (!siRXe_TxSarQry.empty()) {
        //---------------------------------------
        //-- RX Engine
        //---------------------------------------
        siRXe_TxSarQry.read(sRXeQry);
        if (sRXeQry.write) {
            TX_SAR_TABLE[sRXeQry.sessionID].ackd        = sRXeQry.ackd;
            TX_SAR_TABLE[sRXeQry.sessionID].recv_window = sRXeQry.recv_window;
            TX_SAR_TABLE[sRXeQry.sessionID].cong_window = sRXeQry.cong_window;
            TX_SAR_TABLE[sRXeQry.sessionID].count       = sRXeQry.count;
            //OBSOLETE-20190628 if (sRXeQry.init == 1) {
            //OBSOLETE-20190628     TX_SAR_TABLE[sRXeQry.sessionID].finReady = false;
            //OBSOLETE-20190628     TX_SAR_TABLE[sRXeQry.sessionID].finSent = false;
            //OBSOLETE-20190628 }
            TX_SAR_TABLE[sRXeQry.sessionID].fastRetransmitted = sRXeQry.fastRetransmitted;
            // Push ACK to txAppInterface
            soTAi_PushCmd.write(TStTxSarPush(sRXeQry.sessionID, sRXeQry.ackd));
        }
        else {
            //-- Read Query
            soRXe_TxSarRep.write(RXeTxSarReply(TX_SAR_TABLE[sRXeQry.sessionID].ackd,
                                               TX_SAR_TABLE[sRXeQry.sessionID].not_ackd,
                                               TX_SAR_TABLE[sRXeQry.sessionID].cong_window,
                                               TX_SAR_TABLE[sRXeQry.sessionID].slowstart_threshold,
                                               TX_SAR_TABLE[sRXeQry.sessionID].count,
                                               TX_SAR_TABLE[sRXeQry.sessionID].fastRetransmitted));
        }
    }
}
