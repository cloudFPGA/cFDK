#include "tx_sar_table.hpp"

using namespace hls;

/*******************************************************************************
 * @brief Tx Sar Table (TSt)
 *
 * @details
 *  This data structure stores the information to manage the Tx sliding window.
 *  It is accessed by the RxEngine (RXe), the TxAppInterface (TAi) and the
 *  TxEngine (TXe).
 *
 *  @TODO check if locking is actually required, especially for rxOut
 *  @param[in] rxEng2txSar_upd_req
 *  @param[in] txEng2txSar_upd_req
 *  @param[in] siTAi_AppPush,        Push 'app' from TxAppInterface (TAi).
 *  @param[out] txSar2rxEng_upd_rsp
 *  @param[out] txSar2txEng_upd_rsp
 *  @param[out] soTAi_AckPush,       Pushes an AckNum onto the ACK table of [TAi].
 *******************************************************************************/
void tx_sar_table(
        stream<rxTxSarQuery>       &rxEng2txSar_upd_req,
        stream<txTxSarQuery>       &txEng2txSar_upd_req,
        stream<TxSarTableAppPush>  &siTAi_AppPush,
        stream<rxTxSarReply>       &txSar2rxEng_upd_rsp,
        stream<txTxSarReply>       &txSar2txEng_upd_rsp,
        stream<txSarAckPush>       &soTAi_AckPush)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1

    static txSarEntry               TX_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_TABLE inter false
    #pragma HLS RESOURCE   variable=TX_TABLE core=RAM_T2P_BRAM

    txTxSarQuery      tst_txEngUpdate;
    txTxSarRtQuery    txEngRtUpdate;
    rxTxSarQuery      tst_rxEngUpdate;
    TxSarTableAppPush appPush;

    if (!txEng2txSar_upd_req.empty()) { // TX Engine
        txEng2txSar_upd_req.read(tst_txEngUpdate);
        if (tst_txEngUpdate.write) {
            if (!tst_txEngUpdate.isRtQuery) {
                TX_TABLE[tst_txEngUpdate.sessionID].not_ackd = tst_txEngUpdate.not_ackd;
                if (tst_txEngUpdate.init) {
                    TX_TABLE[tst_txEngUpdate.sessionID].app = tst_txEngUpdate.not_ackd;
                    TX_TABLE[tst_txEngUpdate.sessionID].ackd = tst_txEngUpdate.not_ackd-1;
                    TX_TABLE[tst_txEngUpdate.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    TX_TABLE[tst_txEngUpdate.sessionID].slowstart_threshold = 0xFFFF;
                    soTAi_AckPush.write(txSarAckPush(tst_txEngUpdate.sessionID,
                                                     tst_txEngUpdate.not_ackd, 1)); // Init ACK to txAppInterface
                }
                if (tst_txEngUpdate.finReady)
                    TX_TABLE[tst_txEngUpdate.sessionID].finReady = tst_txEngUpdate.finReady;
                if (tst_txEngUpdate.finSent)
                    TX_TABLE[tst_txEngUpdate.sessionID].finSent = tst_txEngUpdate.finSent;
            }
            else {
                txEngRtUpdate = tst_txEngUpdate;
                TX_TABLE[tst_txEngUpdate.sessionID].slowstart_threshold = txEngRtUpdate.getThreshold();
                TX_TABLE[tst_txEngUpdate.sessionID].cong_window = 0x3908; // 10 x 1460(MSS) TODO is this correct or less, eg. 1/2 * MSS
            }
        }
        else { // Read
            ap_uint<16> minWindow;
            if (TX_TABLE[tst_txEngUpdate.sessionID].cong_window < TX_TABLE[tst_txEngUpdate.sessionID].recv_window)
                minWindow = TX_TABLE[tst_txEngUpdate.sessionID].cong_window;
            else
                minWindow = TX_TABLE[tst_txEngUpdate.sessionID].recv_window;
            txSar2txEng_upd_rsp.write(txTxSarReply( TX_TABLE[tst_txEngUpdate.sessionID].ackd,
                                                    TX_TABLE[tst_txEngUpdate.sessionID].not_ackd,
                                                    minWindow,
                                                    TX_TABLE[tst_txEngUpdate.sessionID].app,
                                                    TX_TABLE[tst_txEngUpdate.sessionID].finReady,
                                                    TX_TABLE[tst_txEngUpdate.sessionID].finSent));
        }
    }

    else if (!siTAi_AppPush.empty()) {
        // Push app pointer
        siTAi_AppPush.read(appPush);
        TX_TABLE[appPush.sessionID].app = appPush.app;
    }
    else if (!rxEng2txSar_upd_req.empty()) { // RX Engine
        rxEng2txSar_upd_req.read(tst_rxEngUpdate);
        if (tst_rxEngUpdate.write) {
            TX_TABLE[tst_rxEngUpdate.sessionID].ackd = tst_rxEngUpdate.ackd;
            TX_TABLE[tst_rxEngUpdate.sessionID].recv_window = tst_rxEngUpdate.recv_window;
            TX_TABLE[tst_rxEngUpdate.sessionID].cong_window = tst_rxEngUpdate.cong_window;
            TX_TABLE[tst_rxEngUpdate.sessionID].count = tst_rxEngUpdate.count;
            if (tst_rxEngUpdate.init == 1) {
                TX_TABLE[tst_rxEngUpdate.sessionID].finReady = false;
                TX_TABLE[tst_rxEngUpdate.sessionID].finSent = false;
            }
            // Push ACK to txAppInterface
            soTAi_AckPush.write(txSarAckPush(tst_rxEngUpdate.sessionID, tst_rxEngUpdate.ackd));
        }
        else {
            txSar2rxEng_upd_rsp.write(rxTxSarReply( TX_TABLE[tst_rxEngUpdate.sessionID].ackd,
                                                    TX_TABLE[tst_rxEngUpdate.sessionID].not_ackd,
                                                    TX_TABLE[tst_rxEngUpdate.sessionID].cong_window,
                                                    TX_TABLE[tst_rxEngUpdate.sessionID].slowstart_threshold,
                                                    TX_TABLE[tst_rxEngUpdate.sessionID].count));
        }
    }
}
