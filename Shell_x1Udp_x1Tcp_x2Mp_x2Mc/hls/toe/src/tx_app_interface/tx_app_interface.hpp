#include "../toe.hpp"
#include "../tx_app_if/tx_app_if.hpp"
#include "../tx_app_stream_if/tx_app_stream_if.hpp"

using namespace hls;

struct txAppTableEntry
{
    ap_uint<16>     ackd;
    ap_uint<16>     mempt;
    txAppTableEntry() {}
    txAppTableEntry(ap_uint<16> ackd, ap_uint<16> mempt)
            :ackd(ackd), mempt(mempt) {}
};

void tx_app_interface(
        stream<ap_uint<16> >           &appTxDataReqMetadata,
        stream<axiWord>                &appTxDataReq,
        stream<sessionState>           &stateTable2txApp_rsp,
        stream<txSarAckPush>           &txSar2txApp_ack_push,
        stream<mmStatus>               &txBufferWriteStatus,

        stream<ipTuple>                &appOpenConnReq,
        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &sLookup2txApp_rsp,
        stream<ap_uint<16> >           &portTable2txApp_port_rsp,
        stream<sessionState>           &stateTable2txApp_upd_rsp,
        stream<openStatus>             &conEstablishedFifo,

        stream<ap_int<17> >            &appTxDataRsp,
        stream<ap_uint<16> >           &txApp2stateTable_req,
        stream<mmCmd>                  &txBufferWriteCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<txAppTxSarPush>         &txApp2txSar_push,

        stream<openStatus>             &appOpenConnRsp,
        stream<fourTuple>              &txApp2sLookup_req,
        stream<ap_uint<1> >            &txApp2portTable_port_req,
        stream<stateQuery>             &txApp2stateTable_upd_req,
        stream<event>                  &txApp2eventEng_setEvent,
        stream<openStatus>             &rtTimer2txApp_notification,
        ap_uint<32>                     regIpAddress
);
