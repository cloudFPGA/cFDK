/*****************************************************************************
 * @file       : rx_app_if.hpp
 * @brief      : Rx Application Interface (RAi) of the TCP Offload Engine (TOE).
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
 *               TCP Rx Application Interface.
 *
 * [TODO - Merge the upper rx_app_iterface located in toe.cpp with this one]
 *
 *****************************************************************************/
#include "../toe.hpp"

using namespace hls;


/*************************************************************************
 *
 * ENTITY - TCP APPLICATION INTERFACE (Rai)
 *
 *************************************************************************/
void rx_app_if(
        stream<TcpPort>                    &siTRIF_LsnReq,
        stream<AckBit>                     &soTRIF_LsnAck,
        stream<TcpPort>                    &soPRt_LsnReq,
        stream<AckBit>                     &siPRt_LsnAck
);
