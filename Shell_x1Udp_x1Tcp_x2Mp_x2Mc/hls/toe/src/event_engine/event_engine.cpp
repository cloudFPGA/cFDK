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
 *----------------------------------------------------------------------------
 *
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

#include "event_engine.hpp"

#define THIS_NAME "TOE/EVe"

using namespace hls;

#define DEBUG_LEVEL 1


/*****************************************************************************
 * @brief The Event Engine (EVe) arbitrates the incoming events and forwards
 *         them event to the Tx Engine (TXe).
 *
 * @param[in]  siTAi_Event,     Event from Tx Application Interface (TAi).
 * @param[in]  siRXe_Event,     Event from the Rx Engine (RXe).
 * @param[in]  siTIm_Event,     Event from Timers (TIm).
 * @param[out] soAKd_Event,      Event to the ACK Delayer (AKd).
 * @param[in]  siAKd_RxEventSig, The ACK Delayer (AKd) just received an event.
 * @param[in]  siAKd_TxEventSig, The ACK Delayer (AKd) just forwarded an event.
*  @param[in]  siTXe_RxEventSig, The Tx Engine (TXe) just received an event.
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
        stream<ap_uint<1> >     &siAKd_RxEventSig,
        stream<ap_uint<1> >     &siAKd_TxEventSig,
        stream<ap_uint<1> >     &siTXe_RxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static ap_uint<1> eventEnginePriority = 0;
    static ap_uint<8> ee_writeCounter     = 0;
    static ap_uint<8> ee_adReadCounter    = 0; //depends on FIFO depth
    static ap_uint<8> ee_adWriteCounter   = 0; //depends on FIFO depth
    static ap_uint<8> ee_txEngReadCounter = 0; //depends on FIFO depth
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

        if (!siRXe_Event.empty() && !soAKd_Event.full()) {
            siRXe_Event.read(ev);
            soAKd_Event.write(ev);
            ee_writeCounter++;
        }
        else if (ee_writeCounter   == ee_adReadCounter &&
                 ee_adWriteCounter == ee_txEngReadCounter) {
            // rtTimer and probeTimer events have priority
            if (!siTIm_Event.empty()) {
                siTIm_Event.read(ev);
                soAKd_Event.write(ev);
                ee_writeCounter++;
            }
            else if (!siTAi_Event.empty()) {
                siTAi_Event.read(ev);
                soAKd_Event.write(ev);
                ee_writeCounter++;
            }
        }

        //break;
    //} //switch
    //eventEnginePriority++;

    if (!siAKd_RxEventSig.empty()) {
        siAKd_RxEventSig.read();
        ee_adReadCounter++;
    }
    if (!siAKd_TxEventSig.empty()) {
        ee_adWriteCounter++;
        siAKd_TxEventSig.read();
    }
    if (!siTXe_RxEventSig.empty()) {
        ee_txEngReadCounter++;
        siTXe_RxEventSig.read();
    }
}
