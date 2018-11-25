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

#include "ack_delay.hpp"

#define THIS_NAME "TOE/AKd"

using namespace hls;

#define DEBUG_LEVEL 1


/*****************************************************************************
 * @brief The ACK Delayer (AKd) manages the transmission of the ACks.
 *
 * @param[in]  siEVe_Event,  Event from Event Engine (EVe).
 * @param[out] soTXe_Event,  Event to Tx Engine (TXe).
 * @param[out] soEVe_RxEventSig, Signals the reception of an event.
 * @param[out] soEVe_TxEventSig, Signals the transmission of an event.
 *
 * @details
 *
 * @ingroup event_engine
 *****************************************************************************/
void ack_delay(
        stream<extendedEvent>   &siEVe_Event,
        stream<extendedEvent>   &soTXe_Event,
        stream<ap_uint<1> >     &soEVe_RxEventSig,
        stream<ap_uint<1> >     &soEVe_TxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static ap_uint<12>              ack_table[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=ack_table core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ack_table inter false

    static ap_uint<16>  ad_pointer = 0;

    if (!siEVe_Event.empty()) {
        extendedEvent ev = siEVe_Event.read();
        soEVe_RxEventSig.write(1);
        if (ev.type == ACK && ack_table[ev.sessionID] == 0) {
            // CHECK IF THERE IS A DELAYED ACK
            ack_table[ev.sessionID] = TIME_100ms;
        }
        else if (ev.type != ACK) {
            // Assumption no SYN/RST
            ack_table[ev.sessionID] = 0;
            soTXe_Event.write(ev);
            soEVe_TxEventSig.write(1);
        }
    }
    else {
        if (ack_table[ad_pointer] > 0 && !soTXe_Event.full()) {
            if (ack_table[ad_pointer] == 1) {
                soTXe_Event.write(event(ACK, ad_pointer));
                soEVe_TxEventSig.write(1);
            }
            ack_table[ad_pointer] -= 1;     // Decrease value
        }
        ad_pointer++;
        if (ad_pointer == MAX_SESSIONS)
            ad_pointer = 0;
    }
}
