/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 * @file       : rlb.cpp
 * @brief      : Ready Logic Barrier (RLB)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_RLB
 * \{
 *****************************************************************************/

#include "rlb.hpp"

using namespace hls;

#undef USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPd | TRACE_IBUF)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "RLB"

#define TRACE_OFF  0x0000
#define TRACE_TODO 1 << 1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/******************************************************************************
 * @brief  Main process of the Ready Logic Barrier (RLB).
 *
 * -- MMIO Interface
 * @param[out] poMMIO_Ready   Ready signal to [SHELL/MMIO].
 * -- UOE / Ready Data Stream
 * @param[in]  siUOE_Ready    Ready data stream from UdpOffloadEngine (UOE).
 * -- TOE / Ready Data Stream
 * @param[in]  siTOE_Ready    Ready data stream from TcpOffloadEngine (TOE).
 *
 ******************************************************************************/
void rlb(

        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        StsBit                          *poMMIO_Ready,

        //------------------------------------------------------
        //-- UOE / Data Stream Interface
        //------------------------------------------------------
        stream<StsBool>                 &siUOE_Ready,

        //------------------------------------------------------
        //-- TOE / Data Stream Interface
        //------------------------------------------------------
        stream<StsBool>                 &siTOE_Ready)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_none register   port=poMMIO_Ready   name=poMMIO_Ready

    #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_Ready    metadata="-bus_bundle siUOE_Ready"

    #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_Ready    metadata="-bus_bundle siTOE_Ready"

#else
    #pragma HLS INTERFACE ap_none register   port=poMMIO_Ready   name=poMMIO_Ready

    #pragma HLS INTERFACE axis register both port=siUOE_Ready    name=siUOE_Ready
    #pragma HLS INTERFACE axis register both port=siTOE_Ready    name=siTOE_Ready

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat2(THIS_NAME, "RLB");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { BARRIER=0, SYNC } rlb_fsmState;
    #pragma HLS RESET                variable=rlb_fsmState
    static StsBool                            rlb_uoeReady;
    #pragma HSL RESET                variable=rlb_uoeReady
    static StsBool                            rlb_toeReady;
    #pragma HSL RESET                variable=rlb_toeReady


    switch(rlb_fsmState) {
    case BARRIER:
        *poMMIO_Ready = 0;
        if(!siUOE_Ready.empty()) {
            siUOE_Ready.read(rlb_uoeReady);
        }
        if(!siTOE_Ready.empty()) {
            siTOE_Ready.read(rlb_toeReady);
        }
        if (rlb_uoeReady and rlb_toeReady) {
            rlb_fsmState = SYNC;
        }
        break;
    case SYNC:
        *poMMIO_Ready = 1;
        break;
    }
}

/*! \} */
