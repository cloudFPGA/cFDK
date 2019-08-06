/*****************************************************************************
 * @file       : port_table.hpp
 * @brief      : Port Table (PRt) of the TCP Offload Engine (TOE).
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
 * @details    : Data structures, types & prototypes definitions for TOE/PRt.
 *
 *****************************************************************************/

#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;


/*****************************************************************************
 * @brief   Main process of the TCP Port Table (PRt).
 *****************************************************************************/
void port_table(
        StsBool                 &poTOE_Ready,
        stream<AxiTcpPort>      &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        stream<TcpPort>         &siRAi_OpenPortReq,
        stream<RepBit>          &soRAi_OpenPortRep,
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &siTAi_GetFreePortRep,
        stream<TcpPort>         &siSLc_ClosePortCmd
);
