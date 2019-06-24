/******************************************************************************
 * @file       : state_table.hpp
 * @brief      : State Table (STt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 ******************************************************************************/

#include "../toe.hpp"

using namespace hls;

/*****************************************************************************
 * @brief   Main process of the State Table (STt).
 *
 *****************************************************************************/
void state_table(
        stream<StateQuery>         &siRXe_SessStateQry,
        stream<SessionState>       &soRXe_SessStateRep,
        stream<StateQuery>         &siTAi_Taa_StateQry,
        stream<SessionState>       &soTAi_Taa_StateRep,
        stream<SessionId>          &siTAi_Tas_StateReq,
        stream<SessionState>       &soTAi_Tas_StateRep,
        stream<SessionId>          &soSLc_SessCloseCmd,
        stream<SessionId>          &soSLc_SessReleaseCmd
);
