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

/*******************************************************************************
 * @file       : ack_delay.cpp
 * @brief      : ACK Delayer (AKd) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "ack_delay.hpp"
#include "../toe_utils.hpp"

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

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief ACK Delayer (AKd)
 *
 * @param[in]  siEVe_Event      Event from Event Engine (EVe).
 * @param[out] soEVe_RxEventSig Signals the reception of an event.
 * @param[out] soEVe_TxEventSig Signals the transmission of an event.
 * @param[out] soTXe_Event      Event to Tx Engine (TXe).
 *
 * @details
 *  This process manages the transmission delay of the ACKs. Upon reception
 *   of an ACK, the counter associated to the corresponding session is
 *   initialized to (64us/MAX_SESSIONS). Next, this counter is decremented
 *   every (MAX_SESSIONS) until it reaches zero. At that time, a request to
 *   generate an ACK for that session is forwarded to the TxEngine (Txe).
 *******************************************************************************/
void ack_delay(
        stream<ExtendedEvent>   &siEVe_Event,
        stream<SigBit>          &soEVe_RxEventSig,
        stream<SigBit>          &soEVe_TxEventSig,
        stream<ExtendedEvent>   &soTXe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 //OBSOLETE_20210819 enable_flush

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS ---------------------------------------------------------
    // [TODO - The type of 'ACK_TABLE' can be configured as a functions of 'TIME_XXX' and 'MAX_SESSIONS']
    // [ E.g. - TIME_64us  = ( 64.000/0.0064/32) + 1 =  313 = 0x139 ( 9-bits)
    // [ E.g. - TIME_64us  = ( 64.000/0.0064/ 8) + 1 = 1251 = 0x4E3 (11-bits)
    // [ E.g. - TIME_128us = (128.000/0.0064/32) + 1 =  626 = 0x271 (10-bits)
    static ap_uint<12>              ACK_TABLE[TOE_MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=ACK_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ACK_TABLE inter false
    #pragma HLS RESET      variable=ACK_TABLE

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    // [TODO - The type of 'akd_Ptr' could be configured as a functions of 'MAX_SESSIONS']
    // [ E.g. - static const int NR_BITS = ceil(log10(MAX_SESSIONS)/log10(2));
    static SessionId           akd_Ptr;
    #pragma HLS RESET variable=akd_Ptr

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    ExtendedEvent ev;

    if (!siEVe_Event.empty()) {

        // Read incoming event and inform [EVe] that the FiFo stream was read
        siEVe_Event.read(ev);
        assessSize(myName, soEVe_RxEventSig, "soEVe_RxEventSig", cDepth_AKdToEVe_Event);
        soEVe_RxEventSig.write(1);

        if (ev.type == ACK_EVENT) {
            if (ACK_TABLE[ev.sessionID] == 0) {
                // There is no delayed ACK pending --> Schedule a new one
                ACK_TABLE[ev.sessionID] = ACKD_64us;
                if (DEBUG_LEVEL & TRACE_AKD) {
                    printInfo(myName, "S%d - Received \'%s\' - Setting ACK_TABLE[%d]=%d\n",
                              ev.sessionID.to_int(), getEventName(ev.type), ev.sessionID.to_uint(), ACK_TABLE[ev.sessionID].to_uint());
                }
            }
            else {
                if (DEBUG_LEVEL & TRACE_AKD) {
                     printInfo(myName, "S%d - Received \'%s\' - Current ACK_TABLE[%d]=%d\n",
                              ev.sessionID.to_int(), getEventName(ev.type), ev.sessionID.to_uint(), ACK_TABLE[ev.sessionID].to_uint());
                }
            }
        }
        else {
            // Any other event
            ACK_TABLE[ev.sessionID] = 0;
            // Forward event to TxEngine
            assessSize(myName, soTXe_Event, "soTXe_Event", cDepth_AKdToTXe_Event);
            soTXe_Event.write(ev);
            // Tell the EventEngine that we just forwarded an mone-ACK event to TXe
            assessSize(myName, soEVe_TxEventSig, "soEVe_TxEventSig", cDepth_AKdToEVe_Event);
            soEVe_TxEventSig.write(1);
            // Debug trace
            if (DEBUG_LEVEL & TRACE_AKD) {
                printInfo(myName, "S%d - Received '%s' - Clearing ACK_TABLE[%d]=%d\n",
                                  ev.sessionID.to_int(), getEventName(ev.type), ev.sessionID.to_uint(), ACK_TABLE[ev.sessionID].to_uint());
            }
        }
    }
    else {
        if (ACK_TABLE[akd_Ptr] > 0 and !soTXe_Event.full()) {
            if (ACK_TABLE[akd_Ptr] == 1) {
                soTXe_Event.write(Event(ACK_EVENT, akd_Ptr));
                // Tell the EventEngine that we just forwarded an event to TXe
                assessSize(myName, soEVe_TxEventSig, "soEVe_TxEventSig", cDepth_AKdToEVe_Event);
                soEVe_TxEventSig.write(1);
                if (DEBUG_LEVEL & TRACE_AKD) {
                    printInfo(myName, "S%d - It's ACK Time - Requesting [TXe] to generate an new ACK\n",
                              akd_Ptr.to_int());
                }
            }
            ACK_TABLE[akd_Ptr] -= 1;
        }
        akd_Ptr++;
        if (akd_Ptr == TOE_MAX_SESSIONS) {
            akd_Ptr = 0;
        }
    }
}
