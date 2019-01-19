/*****************************************************************************
 * @file       : event_engine.hpp
 * @brief      : Event Engine (EVe) of the TCP Offload Engine (TOE)
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
 * @brief   Main process of the Event Engine (EVe).
 *
 * @ingroup event_engine
 *****************************************************************************/
void event_engine(
        stream<event>           &siTAi_Event,
        stream<extendedEvent>   &siRXe_Event,
        stream<event>           &siTIm_Event,
        stream<extendedEvent>   &soAKd_Event,
        stream<SigBit>          &siAKd_RxEventSig,
        stream<SigBit>          &siAKd_TxEventSig,
        stream<SigBit>          &siTXe_RxEventSig
);

