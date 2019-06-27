/*****************************************************************************
 * @file       : rx_app_stream_if.hpp
 * @brief      : Rx Application Stream Interface (RAsi) of the TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *               TCP Rx Engine.
 *
 *****************************************************************************/

#include "../toe.hpp"

using namespace hls;

/*****************************************************************************
 * @brief   Main process of the Rx Application Stream Interface (RAsi).
 *
 * @ingroup rx_app_stream_if
 *****************************************************************************/

void rx_app_stream_if(
        stream<AppRdReq>            &siTRIF_DataReq,
        stream<SessionId>           &appRxDataRspMetadata,
        stream<RAiRxSarQuery>         &soRSt_RxSarQry,
        stream<RAiRxSarReply>         &siRSt_RxSarRep,
        stream<DmCmd>               &rxBufferReadCmd
);
