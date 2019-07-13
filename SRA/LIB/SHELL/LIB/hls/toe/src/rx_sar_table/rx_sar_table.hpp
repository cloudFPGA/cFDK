/*****************************************************************************
 * @file       : rx_sar_table.hpp
 * @brief      : Rx Segmentation And Reassembly Table (RSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/
#include "../toe.hpp"

using namespace hls;

/*****************************************************************************
 * @brief   Main process of the  Rx SAR Table (RSt)..
 *
 *****************************************************************************/
void rx_sar_table(
        stream<RXeRxSarQuery>        &siRXe_RxSarQry,
        stream<RxSarEntry>         &soRXe_RxSarRep,
        stream<RAiRxSarQuery>        &siRAi_RxSarQry,
        stream<RAiRxSarReply>        &soRAi_RxSarRep,
        stream<SessionId>          &siTXe_RxSarReq, // Read only
        stream<RxSarEntry>         &soTxe_RxSarRep
);
