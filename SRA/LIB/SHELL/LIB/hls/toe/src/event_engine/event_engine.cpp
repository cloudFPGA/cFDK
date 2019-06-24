/*****************************************************************************
 * @file       : event_engine.cpp
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

#include "event_engine.hpp"
#include "../../test/test_toe_utils.hpp"

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

#define DEBUG_LEVEL (TRACE_OFF | TRACE_EVE)


/*****************************************************************************
 * @brief The Event Engine (EVe) arbitrates the incoming events and forwards
 *         them event to the Tx Engine (TXe).
 *
 * @param[in]  siTAi_Event,      Event from TxApplicationInterface (TAi).
 * @param[in]  siRXe_Event,      Event from RxEngine (RXe).
 * @param[in]  siTIm_Event,      Event from Timers (TIm).
 * @param[out] soAKd_Event,      Event to   AckDelayer (AKd).
 * @param[in]  siAKd_RxEventSig, The ACK Delayer just received an event.
 * @param[in]  siAKd_TxEventSig, The ACK Delayer just forwarded an event.
*  @param[in]  siTXe_RxEventSig, The Tx Engine (TXe)  just received an event.
 *
 * @details
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
        stream<SigBit>          &siTXe_RxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = THIS_NAME;

    static ap_uint<1> eventEnginePriority = 0;

    // Warning: the following counters depend on the FiFo depth between EVe and AKd
    static ap_uint<8> eve_eveTxEventCnt = 0; // Keeps track of the #events forwarded to [AckDelayer]
    static ap_uint<8> eve_akdRxEventCnt = 0; // Keeps track of the #events received  by [AckDelayer]
    static ap_uint<8> eve_akdTxEventCnt = 0; // Keeps track of the #events forwarded to [TxEngine] by [AckDelayer]
    static ap_uint<8> eve_txeRxEventCnt = 0; // Keeps track of the #events received  by [TxEngine]
    extendedEvent ev;

    /*switch (eventEnginePriority)
    {
    case 0:
        if (!txApp2eventEng_setEvent.empty())
        {
            txApp2eventEng_setEvent.read(ev);
            soAKd_Event.write(ev);
        }
        else if (!siRXe_Event.empty())
        {
            siRXe_Event.read(ev);
            soAKd_Event.write(ev);
        }
        else if (!timer2eventEng_setEvent.empty())
        {
            siTIm_Event.read(ev);
            soAKd_Event.write(ev);
        }
        break;
    case 1:*/

    //------------------------------------------
    // Handle input streams from [RxEngine]
    //------------------------------------------
    if (!siRXe_Event.empty() && !soAKd_Event.full()) {
        siRXe_Event.read(ev);
        soAKd_Event.write(ev);
        eve_eveTxEventCnt++;
    }
    else if (eve_eveTxEventCnt == eve_akdRxEventCnt &&
             eve_akdTxEventCnt == eve_txeRxEventCnt) {
        // RetransmitTimer and ProbeTimer events have priority
        if (!siTIm_Event.empty()) {
            siTIm_Event.read(ev);
            soAKd_Event.write(ev);
            eve_eveTxEventCnt++;
            if (DEBUG_LEVEL & TRACE_EVE) {
                if (ev.type == RT)
                    printInfo(myName, "Received RT event from [TIm].\n");
            }
        }
        else if (!siTAi_Event.empty()) {
            siTAi_Event.read(ev);
            soAKd_Event.write(ev);
            eve_eveTxEventCnt++;
            if (DEBUG_LEVEL & TRACE_EVE)
                printInfo(myName, "Received TX event from [TAi].\n");
        }
    }

        //break;
    //} //switch
    //eventEnginePriority++;

    //------------------------------------------
    // Handle input streams from [AckDelayer]
    //------------------------------------------
    if (!siAKd_RxEventSig.empty()) {
        // Remote [AckDelayer] just received an event from [EventEngine]
        siAKd_RxEventSig.read();
        eve_akdRxEventCnt++;
    }

    if (!siAKd_TxEventSig.empty()) {
        // Remote [AckDelayer] just forwarded an event to [TxEngine]
        siAKd_TxEventSig.read();
        eve_akdTxEventCnt++;
    }

    //------------------------------------------
    // Handle input streams from [TxEngine]
    //------------------------------------------
    if (!siTXe_RxEventSig.empty()) {
        siTXe_RxEventSig.read();
        eve_txeRxEventCnt++;
    }
}
