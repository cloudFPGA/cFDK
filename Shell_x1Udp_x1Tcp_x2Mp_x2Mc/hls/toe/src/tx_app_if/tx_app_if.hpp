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

void tx_app_if(
        stream<AxiSockAddr>         &siTRIF_OpnReq,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &sLookup2txApp_rsp,
        stream<ap_uint<16> >        &portTable2txApp_port_rsp,
        stream<sessionState>        &stateTable2txApp_upd_rsp,
        stream<OpenStatus>          &conEstablishedIn, //alter
        stream<OpenStatus>          &appOpenConnRsp,
        stream<AxiSocketPair>       &soSLc_SessLookupReq,
        stream<ap_uint<1> >         &txApp2portTable_port_req,
        stream<stateQuery>          &txApp2stateTable_upd_req,
        stream<event>               &txApp2eventEng_setEvent,
        stream<OpenStatus>          &rtTimer2txApp_notification,
        AxiIp4Address                regIpAddress
);
