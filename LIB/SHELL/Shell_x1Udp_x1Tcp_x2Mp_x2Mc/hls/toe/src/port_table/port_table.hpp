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

// OBSOLETE-2018-11-23 /********************************************
// OBSOLETE-2018-11-23  * PRt - Structure of the Port Table
// OBSOLETE-2018-11-23  ********************************************/
// OBSOLETE-2018-11-23 struct portTableEntry
// OBSOLETE-2018-11-23 {
// OBSOLETE-2018-11-23    bool listening;
// OBSOLETE-2018-11-23    bool used;
// OBSOLETE-2018-11-23 };


/*****************************************************************************
 * @brief   Main process of the TCP Port Table (PRt).
 *
 * @ingroup port_table
 *****************************************************************************/
//void port_table(
//        stream<AxiTcpPort>         &siRXe_PortStateReq,
//        stream<bool>               &soPortStateRep,
//        stream<TcpPort>            &siRAi_LsnPortStateReq,
//        stream<bool>               &soLsnPortStateRep,
//        stream<ap_uint<1> >        &siTAi_ActPortStateReq,
//        stream<ap_uint<16> >       &soActPortStateRep,
//        stream<ap_uint<16> >       &siSLc_ReleasePort
//);

void port_table(
        stream<AxiTcpPort>      &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        stream<TcpPort>         &siRAi_OpenPortReq,
        stream<RepBit>          &soRAi_OpenPortRep,
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &siTAi_GetFreePortRep,
        stream<TcpPort>         &siSLc_ClosePortCmd
);
