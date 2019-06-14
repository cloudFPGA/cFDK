/*****************************************************************************
 * @file       : tx_app_interface.hpp
 * @brief      : Tx Application Interface (TAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *----------------------------------------------------------------------------
 * @details    : Data structures, types and prototypes definitions for the
 *               Tx Application Interface.
 *****************************************************************************/

#include "../toe.hpp"
#include "../tx_app_if/tx_app_if.hpp"
#include "../tx_app_stream/tx_app_stream.hpp"

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
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,

        stream<sessionState>           &stateTable2txApp_rsp,
        stream<txSarAckPush>           &txSar2txApp_ack_push,
        stream<DmSts>                  &txBufferWriteStatus,

        stream<AxiSockAddr>            &siTRIF_OpnReq,
        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &sLookup2txApp_rsp,
        stream<ap_uint<16> >           &portTable2txApp_port_rsp,
        stream<sessionState>           &stateTable2txApp_upd_rsp,
        stream<OpenStatus>             &conEstablishedFifo,

        stream<ap_int<17> >            &appTxDataRsp,
        stream<ap_uint<16> >           &txApp2stateTable_req,
        stream<DmCmd>                  &txBufferWriteCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<txAppTxSarPush>         &txApp2txSar_push,

        stream<OpenStatus>             &appOpenConnRsp,
        stream<AxiSocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soTAi_GetFreePortReq,
        stream<stateQuery>             &txApp2stateTable_upd_req,
        stream<event>                  &txApp2eventEng_setEvent,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        ap_uint<32>                     regIpAddress
);
