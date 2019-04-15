#include "tx_sar_table.hpp"

using namespace hls;

/** @ingroup tx_sar_table
 *  This data structure stores the TX(transmitting) sliding window
 *  and handles concurrent access from the @ref rx_engine, @ref tx_app_if
 *  and @ref tx_engine
 *  @TODO check if locking is actually required, especially for rxOut
 *  @param[in] rxEng2txSar_upd_req
 *  @param[in] txEng2txSar_upd_req
 *  @param[in] txApp2txSar_app_push
 *  @param[out] txSar2rxEng_upd_rsp
 *  @param[out] txSar2txEng_upd_rsp
 *  @param[out] txSar2txApp_ack_push
 */
void tx_sar_table(
        stream<rxTxSarQuery>&       rxEng2txSar_upd_req,
        stream<txTxSarQuery>&       txEng2txSar_upd_req,
        stream<txAppTxSarPush>&     txApp2txSar_app_push,
        stream<rxTxSarReply>&       txSar2rxEng_upd_rsp,
        stream<txTxSarReply>&       txSar2txEng_upd_rsp,
        stream<txSarAckPush>&       txSar2txApp_ack_push)
{
#pragma HLS PIPELINE II=1
    static txSarEntry tx_table[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=tx_table inter false
    #pragma HLS RESOURCE variable=tx_table core=RAM_T2P_BRAM

    txTxSarQuery tst_txEngUpdate;
    txTxSarRtQuery txEngRtUpdate;
    rxTxSarQuery tst_rxEngUpdate;
    txAppTxSarPush push;

    if (!txEng2txSar_upd_req.empty()) { // TX Engine
        txEng2txSar_upd_req.read(tst_txEngUpdate);
        if (tst_txEngUpdate.write) {
            if (!tst_txEngUpdate.isRtQuery) {
                tx_table[tst_txEngUpdate.sessionID].not_ackd = tst_txEngUpdate.not_ackd;
                if (tst_txEngUpdate.init) {
                    tx_table[tst_txEngUpdate.sessionID].app = tst_txEngUpdate.not_ackd;
                    tx_table[tst_txEngUpdate.sessionID].ackd = tst_txEngUpdate.not_ackd-1;
                    tx_table[tst_txEngUpdate.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    tx_table[tst_txEngUpdate.sessionID].slowstart_threshold = 0xFFFF;
                    txSar2txApp_ack_push.write(txSarAckPush(tst_txEngUpdate.sessionID, tst_txEngUpdate.not_ackd, 1)); // Init ACK to txAppInterface
                }
                if (tst_txEngUpdate.finReady)
                    tx_table[tst_txEngUpdate.sessionID].finReady = tst_txEngUpdate.finReady;
                if (tst_txEngUpdate.finSent)
                    tx_table[tst_txEngUpdate.sessionID].finSent = tst_txEngUpdate.finSent;
            }
            else {
                txEngRtUpdate = tst_txEngUpdate;
                tx_table[tst_txEngUpdate.sessionID].slowstart_threshold = txEngRtUpdate.getThreshold();
                tx_table[tst_txEngUpdate.sessionID].cong_window = 0x3908; // 10 x 1460(MSS) TODO is this correct or less, eg. 1/2 * MSS
            }
        }
        else { // Read
            ap_uint<16> minWindow;
            if (tx_table[tst_txEngUpdate.sessionID].cong_window < tx_table[tst_txEngUpdate.sessionID].recv_window)
                minWindow = tx_table[tst_txEngUpdate.sessionID].cong_window;
            else
                minWindow = tx_table[tst_txEngUpdate.sessionID].recv_window;
            txSar2txEng_upd_rsp.write(txTxSarReply( tx_table[tst_txEngUpdate.sessionID].ackd,
                                                    tx_table[tst_txEngUpdate.sessionID].not_ackd,
                                                    minWindow,
                                                    tx_table[tst_txEngUpdate.sessionID].app,
                                                    tx_table[tst_txEngUpdate.sessionID].finReady,
                                                    tx_table[tst_txEngUpdate.sessionID].finSent));
        }
    }

    else if (!txApp2txSar_app_push.empty()) { // TX App Stream If, write only
        txApp2txSar_app_push.read(push);
        tx_table[push.sessionID].app = push.app;
    }
    else if (!rxEng2txSar_upd_req.empty()) { // RX Engine
        rxEng2txSar_upd_req.read(tst_rxEngUpdate);
        if (tst_rxEngUpdate.write) {
            tx_table[tst_rxEngUpdate.sessionID].ackd = tst_rxEngUpdate.ackd;
            tx_table[tst_rxEngUpdate.sessionID].recv_window = tst_rxEngUpdate.recv_window;
            tx_table[tst_rxEngUpdate.sessionID].cong_window = tst_rxEngUpdate.cong_window;
            tx_table[tst_rxEngUpdate.sessionID].count = tst_rxEngUpdate.count;
            if (tst_rxEngUpdate.init == 1) {
                tx_table[tst_rxEngUpdate.sessionID].finReady = false;
                tx_table[tst_rxEngUpdate.sessionID].finSent = false;
            }
            // Push ACK to txAppInterface
            txSar2txApp_ack_push.write(txSarAckPush(tst_rxEngUpdate.sessionID, tst_rxEngUpdate.ackd));
        }
        else {
            txSar2rxEng_upd_rsp.write(rxTxSarReply( tx_table[tst_rxEngUpdate.sessionID].ackd,
                                                    tx_table[tst_rxEngUpdate.sessionID].not_ackd,
                                                    tx_table[tst_rxEngUpdate.sessionID].cong_window,
                                                    tx_table[tst_rxEngUpdate.sessionID].slowstart_threshold,
                                                    tx_table[tst_rxEngUpdate.sessionID].count));
        }
    }
}
