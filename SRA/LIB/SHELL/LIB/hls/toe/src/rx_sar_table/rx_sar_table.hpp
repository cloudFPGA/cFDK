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
        stream<rxSarRecvd>     &rxEng2rxSar_upd_req,
        stream<rxSarAppd>      &rxApp2rxSar_upd_req,
        stream<ap_uint<16> >   &txEng2rxSar_req, //read only
        stream<RxSarEntry>     &rxSar2rxEng_upd_rsp,
        stream<rxSarAppd>      &rxSar2rxApp_upd_rsp,
        stream<RxSarEntry>     &rxSar2txEng_rsp);
