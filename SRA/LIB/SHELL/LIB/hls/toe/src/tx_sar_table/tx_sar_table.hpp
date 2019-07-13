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
 * @brief   Main process of the Tx SAR Table (TSt).
 *
 ******************************************************************************/
void tx_sar_table(
        stream<RXeTxSarQuery>      &siRXe_TxSarQry,
        stream<RXeTxSarReply>      &soRXe_TxSarRep,
        stream<TXeTxSarQuery>      &siTXe_TxSarQry,
        stream<TXeTxSarReply>      &soTXe_TxSarRep,
        stream<TAiTxSarPush>       &siTAi_AppPush,
        stream<TStTxSarPush>       &soTAi_AckPush
);
