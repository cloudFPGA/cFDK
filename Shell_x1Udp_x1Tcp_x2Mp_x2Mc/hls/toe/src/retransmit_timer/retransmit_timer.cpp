/*****************************************************************************
 * @file       : retransmit_timer.cpp
 * @brief      : Re-Transmission Timer (Rtt) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "retransmit_timer.hpp"

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_OFF | TRACE_TIM)
 ************************************************/
extern bool gTraceEvent;
#define THIS_NAME "TOE/TIm/Rtt"

#define TRACE_OFF  0x0000
#define TRACE_RTT  1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF | TRACE_RTT)


/*****************************************************************************
 * @brief ReTransmit Timer (Rtt) process.
 *
 * @param[in]      siRXe_ReTxTimerCmd,   Retransmit timer command from [RxEngine].
 * @param[in]      siTXe_ReTxTimerEvent, Retransmit timer event from [TxEngine].
 * @param[out]     soEmx_Event,          Event to Event Multiplexer (Emx).
 * @param[out]     soSmx_ReleaseState,   Set event to StateTable Mux (Smx).
 * @param[out]     soTAi_Notif,          Notification to Tx Application I/F (TAi).
 * @param[out]     soRAi_Notif,          Notification to Rx Application I/F (RAi).
 *
 * @details
 *  This process receives a session-id and event-type from [TxEngine]. If the
 *   retransmit timer corresponding to this session-id is not active, then it
 *   is activated and its time-out interval is set depending on how often the
 *   session already time-outed. Next, an event is fired back to [TxEnghine],
 *   whenever a timer times-out.
 *  The process also receives a command from [RxEngine] specifying if the timer
 *   of a session must be stopped or loaded with a default time-out value.
 *  If a session times-out more than 4 times in a row, it is aborted. A release
 *   command is then sent to [StateTable] and the application is notified
 *   [FIXME - the application is notified through @param timerNotificationFifoOut].
 *
 * @ingroup retransmit_timer
 *******************************************************************************/
void pRetransmitTimer(
        stream<ReTxTimerCmd>             &siRXe_ReTxTimerCmd,
        stream<ReTxTimerEvent>           &siTXe_ReTxTimerEvent,
        stream<event>                    &soEmx_Event,
        stream<ap_uint<16> >             &soSmx_ReleaseState,
        stream<OpenStatus>               &soTAi_Notif,
        stream<appNotification>          &soRAi_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    //#pragma HLS INLINE

    #pragma HLS DATA_PACK variable=soEmx_Event
    #pragma HLS DATA_PACK variable=soSmx_ReleaseState

    static ReTxTimerEntry           RETRANSMIT_TIMER_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RETRANSMIT_TIMER_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=RETRANSMIT_TIMER_TABLE inter false

    const char *myName  = THIS_NAME;

    static bool                     rtt_waitForWrite = false;
    static SessionId                rtt_position     = 0;
    static SessionId                rtt_prevPosition = 0;
    static ReTxTimerCmd             rtt_cmd;

    ReTxTimerEntry  currEntry;
    ReTxTimerEvent  txEvent;
    ap_uint<1>      operationSwitch = 0;
    SessionId       currID;

    if (rtt_waitForWrite && rtt_cmd.sessionID != rtt_prevPosition) {
        // [TODO - maybe prevprev too]
        //OBSOLETE-20190181 if (!rtt_cmd.stop)
        if (rtt_cmd.command == LOAD_TIMER)
            RETRANSMIT_TIMER_TABLE[rtt_cmd.sessionID].time = TIME_1s;
        else {
            RETRANSMIT_TIMER_TABLE[rtt_cmd.sessionID].time   = 0;
            RETRANSMIT_TIMER_TABLE[rtt_cmd.sessionID].active = false;
        }
        RETRANSMIT_TIMER_TABLE[rtt_cmd.sessionID].retries = 0;
        rtt_waitForWrite = false;
    }
    //------------------------------------------------
    // Handle the input streams from [RxEngine]
    //------------------------------------------------
    else if (!siRXe_ReTxTimerCmd.empty() && !rtt_waitForWrite) {
        // INFO: Rx path has priority over Tx path
        siRXe_ReTxTimerCmd.read(rtt_cmd);
        rtt_waitForWrite = true;
    }
    else {
        currID = rtt_position;
        //------------------------------------------------
        // Handle the input streams from [TxEngine]
        //------------------------------------------------
        if (!siTXe_ReTxTimerEvent.empty()) {
            siTXe_ReTxTimerEvent.read(txEvent);
            currID = txEvent.sessionID;
            operationSwitch = 1;
            if ( (txEvent.sessionID-3 <  rtt_position) &&
                 (rtt_position    <= txEvent.sessionID) )
                rtt_position += 5;
        }
        else {
            // Increment position
            rtt_position++;
            if (rtt_position >= MAX_SESSIONS)
                rtt_position = 0;
            operationSwitch = 0;
        }

        // Get current entry from table
        currEntry = RETRANSMIT_TIMER_TABLE[currID];

        switch (operationSwitch) {

        case 1: // Got an event from [TxEngine]
            currEntry.type = txEvent.type;
            if (!currEntry.active) {
                switch(currEntry.retries) {
                case 0:
                    currEntry.time = TIME_1s;
                    break;
                case 1:
                    currEntry.time = TIME_5s;
                    break;
                case 2:
                    currEntry.time = TIME_10s;
                    break;
                case 3:
                    currEntry.time = TIME_15s;
                    break;
                default:
                    currEntry.time = TIME_30s;
                    break;
                }
            }
            currEntry.active = true;
            //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].active = currEntry.active;
            //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].time   = currEntry.time;
            //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].type   = currEntry.type;
            break;

        case 0:
            if (currEntry.active) {
                if (currEntry.time > 0)
                    currEntry.time--;
                // We need to check if we can generate another event, otherwise we might
                // end up in a Deadlock since the [TxEngine] will not be able to set new
                // retransmit timers.
                else if (!soEmx_Event.full()) {
                    currEntry.time   = 0;
                    currEntry.active = false;
                    //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].active = currEntry.active;
                    if (currEntry.retries < 4) {
                        currEntry.retries++;
                        soEmx_Event.write(event((eventType)currEntry.type,
                                          currID,
                                          currEntry.retries));
                        if (DEBUG_LEVEL & TRACE_RTT) {
                            if (currEntry.type == RT_EVENT)
                                printInfo(myName, "Forwarding RT event to [EventEngine].\n");
                            }
                    }
                    else {
                        currEntry.retries = 0;
                        soSmx_ReleaseState.write(currID);
                        if (currEntry.type == SYN_EVENT)
                            soTAi_Notif.write(OpenStatus(currID, false));
                        else
                            soRAi_Notif.write(appNotification(currID, true)); //TIME_OUT
                    }
                }
            }
            //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].time    = currEntry.time;
            //OBSOLETE-20190181 RETRANSMIT_TIMER_TABLE[currID].retries = currEntry.retries;
            break;

        } // End of: switch

        // Write the entry back into the table
        RETRANSMIT_TIMER_TABLE[currID] = currEntry;
        rtt_prevPosition = currID;
    }
}
