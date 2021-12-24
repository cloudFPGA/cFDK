/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
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
 *******************************************************************************/

/************************************************
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/


/*******************************************************************************
 * @file       : event_engine.cpp
 * @brief      : Event Engine (EVe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "event_engine.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/EVe"

#define TRACE_OFF  0x0000
#define TRACE_EVE 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief The Event Engine (EVe) arbitrates the incoming events and forwards
 *         them to the Tx Engine (TXe) via the ACK Delayer (AKd).
 *
 * @param[in]  siTAi_Event      Event from TxApplicationInterface (TAi).
 * @param[in]  siRXe_Event      Event from RxEngine (RXe).
 * @param[in]  siTIm_Event      Event from Timers (TIm).
 * @param[out] soAKd_Event      Event to   AckDelayer (AKd).
 * @param[in]  siAKd_RxEventSig The AckDelayer just received an event.
 * @param[in]  siAKd_TxEventSig The AckDelayer just forwarded an event.
 * @param[in]  siTXe_RxEventSig The TxEngine (TXe) just received an event.
 *******************************************************************************/
void event_engine(
        stream<Event>           &siTAi_Event,
        stream<ExtendedEvent>   &siRXe_Event,
        stream<Event>           &siTIm_Event,
        stream<ExtendedEvent>   &soAKd_Event,
        stream<SigBit>          &siAKd_RxEventSig,
        stream<SigBit>          &siAKd_TxEventSig,
        stream<SigBit>          &siTXe_RxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = THIS_NAME;

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    //---- Warning: the following counters depend on the FiFo depth between EVe and AKd
    static ap_uint<8>            eve_eve2akd_WrCnt; // #events forwarded by [EVe] to   [AKd]
    #pragma HLS RESET variable = eve_eve2akd_WrCnt
    static ap_uint<8>            eve_eve2akd_RdCnt; // #events received  by [AKd] from [EVe].
    #pragma HLS RESET variable = eve_eve2akd_RdCnt
    static ap_uint<8>            eve_akd2txe_WrCnt; // #events forwarded by [AKd] to  [TXe]
    #pragma HLS RESET variable = eve_akd2txe_WrCnt
    static ap_uint<8>            eve_akd2txe_RdCnt; // #events received  by [TXe] from [Akd]
    #pragma HLS RESET variable = eve_akd2txe_RdCnt

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    ExtendedEvent ev;

    //------------------------------------------
    // Handle input from [RxEngine]
    //------------------------------------------
    if (!siRXe_Event.empty() and !soAKd_Event.full()) {
        siRXe_Event.read(ev);
        soAKd_Event.write(ev);
        if (DEBUG_LEVEL & TRACE_EVE) {
            printInfo(myName, "S%d - Received '%s' from [RXe] (WrCnt=%3d|RdCnt=%3d).\n",
                      ev.sessionID.to_int(), getEventName(ev.type), eve_eve2akd_WrCnt.to_uint(), eve_eve2akd_RdCnt.to_uint());
        }
        eve_eve2akd_WrCnt++;
    }
    else if (eve_eve2akd_WrCnt == eve_eve2akd_RdCnt and
             eve_akd2txe_WrCnt == eve_akd2txe_RdCnt) {
        //------------------------------------------
        // Handle input from [Timers]
        //   RetransmitTimer and ProbeTimer events have priority
        //------------------------------------------
        if (!siTIm_Event.empty()) {
            siTIm_Event.read(ev);
            soAKd_Event.write(ev);
            if (DEBUG_LEVEL & TRACE_EVE) {
                printInfo(myName, "S%d - Received '%s' from [TIm] (WrCnt=%3d|RdCnt=%3d).\n",
                        ev.sessionID.to_int(), getEventName(ev.type), eve_eve2akd_WrCnt.to_uint(), eve_eve2akd_RdCnt.to_uint());
            }
            eve_eve2akd_WrCnt++;
        }
        //--------------------------------------------
        // Handle input from [TcpApplicationInterface]
        //--------------------------------------------
        else if (!siTAi_Event.empty()) {
            siTAi_Event.read(ev);
            assessSize(myName, soAKd_Event, "soAKd_Event", cDepth_EVeToAKd_Event);
            soAKd_Event.write(ev);
            if (DEBUG_LEVEL & TRACE_EVE) {
                printInfo(myName, "S%d - Received '%s' from [TAi] (WrCnt=%3d|RdCnt=%3d).\n",
                        ev.sessionID.to_int(), getEventName(ev.type), eve_eve2akd_WrCnt.to_uint(), eve_eve2akd_RdCnt.to_uint());
            }
            eve_eve2akd_WrCnt++;
        }
    }

    //------------------------------------------
    // Handle inputs from [AckDelayer]
    //------------------------------------------
    if (!siAKd_RxEventSig.empty()) {
        // Remote [AKd] just received an event from [EVe]
        siAKd_RxEventSig.read();
        eve_eve2akd_RdCnt++;
    }

    if (!siAKd_TxEventSig.empty()) {
        // Remote [AKd] just forwarded an event to [TXe]
        siAKd_TxEventSig.read();
        eve_akd2txe_WrCnt++;
    }

    //------------------------------------------
    // Handle input from [TxEngine]
    //------------------------------------------
    if (!siTXe_RxEventSig.empty()) {
        siTXe_RxEventSig.read();
        eve_akd2txe_RdCnt++;
    }
}

/*! \} */
