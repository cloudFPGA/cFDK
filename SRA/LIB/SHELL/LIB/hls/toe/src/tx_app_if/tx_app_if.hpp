/*****************************************************************************
 * @file       : tx_app_if.hpp
 * @brief      : Tx Application Interface (Tai)
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
#include "../toe_utils.hpp"

using namespace hls;

void tx_app_accept(
        stream<AxiSockAddr>         &siTRIF_OpnReq,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<TcpPort>             &siPRt_ActPortStateRep,
        stream<OpenStatus>          &siRXe_SessOpnSts,
        stream<OpenStatus>          &soTRIF_SessOpnSts,
        stream<AxiSocketPair>       &soSLc_SessLookupReq,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<StateQuery>          &soSTt_SessStateQry,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<event>               &soEVe_Event,
        stream<OpenStatus>          &rtTimer2txApp_notification,
        AxiIp4Address                regIpAddress
);
