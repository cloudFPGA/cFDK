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

#include "ack_delay.hpp"

#define THIS_NAME "TOE/AKd"

using namespace hls;

#define DEBUG_LEVEL 1


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
 * @ingroup event_engine
 *****************************************************************************/
void ack_delay(
        stream<extendedEvent>   &siEVe_Event,
        stream<extendedEvent>   &soTXe_Event,
        stream<SigBit>          &soEVe_RxEventSig,
        stream<SigBit>          &soEVe_TxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static ap_uint<12>              ACK_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=ACK_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ACK_TABLE inter false

    static ap_uint<16>  akd_pointer = 0;

    extendedEvent ev;

    if (!siEVe_Event.empty()) {
        siEVe_Event.read(ev);
        // Tell the [EventEngine] that we just received an event
        soEVe_RxEventSig.write(true);
        // Check if there is a delayed ACK
        if (ev.type == ACK && ACK_TABLE[ev.sessionID] == 0) {
            ACK_TABLE[ev.sessionID] = TIME_100ms;  // [FIXME TIME_64us ??]
        }
        else {
            // Assumption no SYN/RST
            ACK_TABLE[ev.sessionID] = 0;
            // Forward event to [TxEngine]
            soTXe_Event.write(ev);
            // Tell the [EventEngine] that we just forwarded an event to TXe
            soEVe_TxEventSig.write(true);
        }
    }
    else {
        if (ACK_TABLE[akd_pointer] > 0 && !soTXe_Event.full()) {
            if (ACK_TABLE[akd_pointer] == 1) {
                // Request the [TxEngine] to generate an ACK for this session #
                soTXe_Event.write(event(ACK, akd_pointer)); // [FIXME - Stream name should be soTXe_GenAckReq]
                // Tell the [EventEngine] that we just forwarded an event to TXe
                soEVe_TxEventSig.write(true);
            }
            ACK_TABLE[akd_pointer] -= 1;     // Decrease value
        }
        akd_pointer++;
        if (akd_pointer == MAX_SESSIONS)
            akd_pointer = 0;
    }
}
