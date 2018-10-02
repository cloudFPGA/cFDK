#include "tx_app_interface.hpp"

using namespace hls;

void txAppStatusHandler(stream<mmStatus>&               txBufferWriteStatus,
                        stream<event>&                  tasi_eventCacheFifo,
                        stream<txAppTxSarPush>&         txApp2txSar_app_push,
                        stream<event>&                  txAppStream2eventEng_setEvent){
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
            mmStatus status = txBufferWriteStatus.read();
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
            mmStatus status = txBufferWriteStatus.read();
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


void tx_app_table(  stream<txSarAckPush>&       txSar2txApp_ack_push,
                    stream<txAppTxSarQuery>&    txApp_upd_req,
                    stream<txAppTxSarReply>&    txApp_upd_rsp) {
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

void tx_app_interface(stream<ap_uint<16> >&         appTxDataReqMetadata,
                    stream<axiWord>&                appTxDataReq,
                    stream<sessionState>&           stateTable2txApp_rsp,
                    stream<txSarAckPush>&           txSar2txApp_ack_push,
                    stream<mmStatus>&               txBufferWriteStatus,

                    stream<ipTuple>&                appOpenConnReq,
                    stream<ap_uint<16> >&           appCloseConnReq,
                    stream<sessionLookupReply>&     sLookup2txApp_rsp,
                    stream<ap_uint<16> >&           portTable2txApp_port_rsp,
                    stream<sessionState>&           stateTable2txApp_upd_rsp,
                    stream<openStatus>&             conEstablishedFifo,

                    stream<ap_int<17> >&            appTxDataRsp,
                    stream<ap_uint<16> >&           txApp2stateTable_req,
                    stream<mmCmd>&                  txBufferWriteCmd,
                    stream<axiWord>&                txBufferWriteData,
                    stream<txAppTxSarPush>&         txApp2txSar_push,

                    stream<openStatus>&             appOpenConnRsp,
                    stream<fourTuple>&              txApp2sLookup_req,
                    stream<ap_uint<1> >&            txApp2portTable_port_req,
                    stream<stateQuery>&             txApp2stateTable_upd_req,
                    stream<event>&                  txApp2eventEng_setEvent,
                    stream<openStatus>&             rtTimer2txApp_notification,
                    ap_uint<32>                     regIpAddress)
{
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

    // Merge Events
    //txEventMerger(txApp2eventEng_mergeEvent, txAppStream2event_mergeEvent, txApp_eventCache);
    mergeFunction(txApp2eventEng_mergeEvent, txAppStream2event_mergeEvent, txApp_eventCache);
    //streamMerger<event>(txApp2eventEng_mergeEvent, txAppStream2event_mergeEvent, txApp_eventCache);
    txAppStatusHandler(txBufferWriteStatus, txApp_eventCache, txApp2txSar_push, txApp2eventEng_setEvent);

    // TX application Stream Interface
    tx_app_stream_if(   appTxDataReqMetadata,
                        appTxDataReq,
                        stateTable2txApp_rsp,
                        txSar2txApp_upd_rsp,
                        appTxDataRsp,
                        txApp2stateTable_req,
                        txApp2txSar_upd_req,
                        txBufferWriteCmd,
                        txBufferWriteData,
                        txAppStream2event_mergeEvent);

    // TX Application Interface
    tx_app_if(  appOpenConnReq,
                appCloseConnReq,
                sLookup2txApp_rsp,
                portTable2txApp_port_rsp,
                stateTable2txApp_upd_rsp,
                conEstablishedFifo,
                appOpenConnRsp,
                txApp2sLookup_req,
                txApp2portTable_port_req,
                txApp2stateTable_upd_req,
                txApp2eventEng_mergeEvent,
                rtTimer2txApp_notification,
                regIpAddress);

    // TX App Meta Table
    tx_app_table(   txSar2txApp_ack_push,
                    txApp2txSar_upd_req,
                    txSar2txApp_upd_rsp);
}
