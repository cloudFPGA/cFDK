/*****************************************************************************
 * @file       : tx_sar_table.cpp
 * @brief      : Tx SAR Table (TSt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *****************************************************************************/

#include "../toe.hpp"

using namespace hls;


/******************************************************************************
 * @brief   Main process of the Tx SAR TAble (TSt).
 *
 * @ingroup rx_sar_table
 ******************************************************************************/
void tx_sar_table(
        stream<rxTxSarQuery>       &rxEng2txSar_upd_req,
        stream<txTxSarQuery>       &txEng2txSar_upd_req,
        stream<TxSarTableAppPush>  &siTAi_AppPush,
        stream<rxTxSarReply>       &txSar2rxEng_upd_rsp,
        stream<txTxSarReply>       &txSar2txEng_upd_rsp,
        stream<txSarAckPush>       &soTAi_AckPush
);
