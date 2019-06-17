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

/*** OBSOLETE-20190614 ********
struct txAppTableEntry
{
    ap_uint<16>     ackd;
    ap_uint<16>     mempt;
    txAppTableEntry() {}
    txAppTableEntry(ap_uint<16> ackd, ap_uint<16> mempt)
            :ackd(ackd), mempt(mempt) {}
};
*******************************/

class TxAppTableEntry
{
  public:
    TcpAckNum       ackd;
    ap_uint<16>     mempt;
    TxAppTableEntry() {}
    TxAppTableEntry(TcpAckNum ackd, ap_uint<16> mempt) :
        ackd(ackd), mempt(mempt) {}
};


void tx_app_interface(
        stream<AppData>                &siTRIF_Data,
        stream<AppMeta>                &siTRIF_Meta,
        stream<TcpSessId>              &soSTt_SessStateReq,
        stream<sessionState>           &siSTt_SessStateRep,
        stream<txSarAckPush>           &siTSt_AckPush,
        stream<DmSts>                  &txBufferWriteStatus,

        stream<AxiSockAddr>            &siTRIF_OpnReq,
        stream<ap_uint<16> >           &appCloseConnReq,
        stream<sessionLookupReply>     &siSLc_SessLookupRep,
        stream<ap_uint<16> >           &siPRt_ActPortStateRep,
        stream<sessionState>           &stateTable2txApp_upd_rsp,
        stream<OpenStatus>             &siRXe_SessOpnSts,

        stream<ap_int<17> >            &appTxDataRsp,
        stream<DmCmd>                  &soMEM_TxP_WrCmd,
        stream<AxiWord>                &soMEM_TxP_Data,
        stream<TxSarTableAppPush>      &soTSt_AppPush,

        stream<OpenStatus>             &soTRIF_SessOpnSts,
        stream<AxiSocketPair>          &soSLc_SessLookupReq,
        stream<ReqBit>                 &soPRt_GetFreePortReq,
        stream<stateQuery>             &txApp2stateTable_upd_req,
        stream<event>                  &txApp2eventEng_setEvent,
        stream<OpenStatus>             &rtTimer2txApp_notification,
        ap_uint<32>                     regIpAddress
);
