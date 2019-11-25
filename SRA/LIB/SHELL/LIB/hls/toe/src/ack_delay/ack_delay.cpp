/************************************************
Copyright (c) 2016-2019, IBM Research.
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


/*****************************************************************************
 * @file       : ack_delay.cpp
 * @brief      : ACK Delayer (AKd) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "ack_delay.hpp"
#include "../../test/test_toe_utils.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_AKD)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/AKd"

#define TRACE_OFF  0x0000
#define TRACE_AKD 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_AKD)

/*****************************************************************************
 * @brief The ACK Delayer (AKd) manages the transmission delay of the ACks.
 *
 * @param[in]  siEVe_Event,      Event from Event Engine (EVe).
 * @param[out] soTXe_Event,      Event to Tx Engine (TXe).
 * @param[out] soEVe_RxEventSig, Signals the reception of an event.
 * @param[out] soEVe_TxEventSig, Signals the transmission of an event.
 *
 * @details
 *  Upon reception of an ACK, the counter associated to the corresponding
 *   session is initialized to (100ms/MAX_SESSIONS). Next, this counter is
 *   decremented every (MAX_SESSIONS) until it reaches zero. At that time,
 *   a request to generate an ACK for that session is forwarded to the
 *   [TxEngine].
 *
 *****************************************************************************/
void ack_delay(
        stream<ExtendedEvent>   &siEVe_Event,
        stream<ExtendedEvent>   &soTXe_Event,
        stream<SigBit>          &soEVe_RxEventSig,
        stream<SigBool>         &soEVe_TxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS --------------------------------------------------------
    // [FIXME - The type of 'ACK_TABLE' must be configured as a functions of 'TIME_XXX']
    // [FIXME - TIME_64us  = ( 64.000/0.0064/MAX_SESSIONS) + 1 = 10.000/32 = 0x139 ( 9-bits)
    // [FIXME - TIME_128us = (128.000/0.0064/MAX_SESSIONS) + 1 = 20.000/32 = 0x271 (10-bits)
    static ap_uint<12>              ACK_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=ACK_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ACK_TABLE inter false
    #pragma HLS reset      variable=ACK_TABLE

    // [FIXME - The type of 'akdPtr' could be configured as a functions of 'MAX_SESSIONS']
    // [FIXME - static const int NR_BITS = ceil(log10(MAX_SESSIONS)/log10(2));
    static SessionId           akdPtr;
    #pragma HLS reset variable=akdPtr

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    ExtendedEvent ev;

    if (!siEVe_Event.empty()) {
        siEVe_Event.read(ev);
        // Tell the [EventEngine] that we just received an event
        assessSize(myName, soEVe_RxEventSig, "soEVe_RxEventSig", 2); // [FIXME-Use constant for the length]
        soEVe_RxEventSig.write(1);
        if (DEBUG_LEVEL & TRACE_AKD) {
            printInfo(myName, "Received event of type \'%s\' for session #%d.\n", myEventTypeToString(ev.type), ev.sessionID.to_int());
        }

        // Check if there is a delayed ACK
        if (ev.type == ACK_EVENT && ACK_TABLE[ev.sessionID] == 0) {
            ACK_TABLE[ev.sessionID] = ACKD_64us;
            if (DEBUG_LEVEL & TRACE_AKD) {
                printInfo(myName, "Creating a delayed ACK for session #%d.\n", ev.sessionID.to_int());
            }
        }
        else {
            // Assumption no SYN/RST
            ACK_TABLE[ev.sessionID] = 0;
            if (DEBUG_LEVEL & TRACE_AKD) {
                printInfo(myName, "Removing any pending delayed ACK for session #%d.\n", ev.sessionID.to_int());
                printInfo(myName, "Forwarding event \'%s\' to [TXe].\n", myEventTypeToString(ev.type));
            }
            // Forward event to [TxEngine]
            assessSize(myName, soTXe_Event, "soTXe_Event", 16);  // [FIXME-Use constant for the length]
            soTXe_Event.write(ev);
            // Tell the [EventEngine] that we just forwarded an event to TXe
            assessSize(myName, soEVe_TxEventSig, "soEVe_TxEventSig", 2);
            soEVe_TxEventSig.write(true);
        }
    }
    else {
        if (ACK_TABLE[akdPtr] > 0 && !soTXe_Event.full()) {
            if (ACK_TABLE[akdPtr] == 1) {
                soTXe_Event.write(Event(ACK_EVENT, akdPtr)); // [FIXME - Stream name could be soTXe_GenAckReq]
                if (DEBUG_LEVEL & TRACE_AKD) {
                    printInfo(myName, "Requesting [TXe] to generate an ACK for session #%d.\n", akdPtr.to_int());
                }
                // Tell the [EventEngine] that we just forwarded an event to TXe
                assessSize(myName, soEVe_TxEventSig, "soEVe_TxEventSig", 2);  // [FIXME-Use constant for the length]
                soEVe_TxEventSig.write(true);
            }
            // [FIXME - Shall we not move the decrement outside of the 'full()' condition ?]
            ACK_TABLE[akdPtr] -= 1;     // Decrease value
        }
        akdPtr++;
        if (akdPtr == MAX_SESSIONS)
            akdPtr = 0;
    }
}
