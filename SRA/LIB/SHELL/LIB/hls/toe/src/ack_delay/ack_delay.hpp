/*****************************************************************************
 * @file       : ack_delay.cpp
 * @brief      : ACK Delayer (AKd) of the TCP Offload Engine (TOE)
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
 * @brief   Main process of the ACK Delayer (AKd).
 *
 * @ingroup event_engine
 *****************************************************************************/
void ack_delay(
        stream<extendedEvent>   &siEVe_Event,
        stream<extendedEvent>   &soTXe_Event,
        stream<SigBool>         &soEVe_RxEventSig,
        stream<SigBool>         &soEVe_TxEventSig
);

