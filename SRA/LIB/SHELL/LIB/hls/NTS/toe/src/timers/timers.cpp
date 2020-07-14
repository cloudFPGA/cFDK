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
 * @file       : timers.cpp
 * @brief      : Timers (TIm) for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "timers.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (PBT_TRACE | RTT_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TIm"

#define TRACE_OFF  0x0000
#define TRACE_CLT  1 <<  1
#define TRACE_PBT  1 <<  2
#define TRACE_RTT  1 <<  3
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

/*******************************************************************************
 * @brief A 2-to-1 generic Stream Multiplexer
 *
 *  @param[in]  si1     The input stream #1.
 *  @param[in]  si2     The input stream #2.
 *  @param[out] so      The output stream.
 *
 * @details
 *  This multiplexer behaves like an arbiter. It takes two streams as inputs and
 *   forwards one of them to the output channel. The stream connected to the
 *   first input always takes precedence over the second one.
 *******************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*******************************************************************************
 * @brief ReTransmit Timer (Rtt) process
 *
 * @param[in]  siRXe_ReTxTimerCmd   Retransmit timer command from RxEngine (RXe).
 * @param[in]  siTXe_ReTxTimerEvent Retransmit timer event from TxEngine (TXe).
 * @param[out] soEmx_Event          Event to EventMultiplexer (Emx).
 * @param[out] soSmx_SessCloseCmd   Close command to StateTableMux (Smx).
 * @param[out] soTAi_Notif          Notification to TxApplicationInterface (TAi).
 * @param[out] soRAi_Notif          Notification to RxApplicationInterface (RAi).
 *
 * @details
 *  This process receives a session-id and an event-type from [TXe]. If the
 *   retransmit timer corresponding to this session-id is not active, then it
 *   is activated and its time-out interval is set depending on how often the
 *   session already time-outed. Next, an event is fired back to [TXe], whenever
 *   a timer times-out.
 *  The process also receives a retransmission command from [RXe] specifying if
 *   the timer of a session must be stopped or loaded with a default time-out
 *   value.
 *  If a session times-out more than 4 times in a row, it is aborted. A release
 *   command is then sent to the StateTable (STt) and the application is notified.
 *******************************************************************************/
void pRetransmitTimer(
        stream<RXeReTransTimerCmd>       &siRXe_ReTxTimerCmd,
        stream<TXeReTransTimerCmd>       &siTXe_ReTxTimerEvent,
        stream<Event>                    &soEmx_Event,
        stream<SessionId>                &soSmx_SessCloseCmd,
        stream<SessState>                &soTAi_Notif,
        stream<TcpAppNotif>              &soRAi_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Rtt");

    //-- STATIC ARRAYs ---------------------------------------------------------
    static ReTxTimerEntry           RETRANSMIT_TIMER_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RETRANSMIT_TIMER_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=RETRANSMIT_TIMER_TABLE inter false
    #pragma HLS RESET      variable=RETRANSMIT_TIMER_TABLE

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                rtt_waitForWrite=false;
    #pragma HLS RESET variable=rtt_waitForWrite
    static SessionId           rtt_prevPosition=0;
    #pragma HLS RESET variable=rtt_prevPosition
    static SessionId           rtt_position=0;
    #pragma HLS RESET variable=rtt_position

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static RXeReTransTimerCmd  rtt_rxeCmd;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    ReTxTimerEntry     currEntry;
    TXeReTransTimerCmd txEvent;
    ap_uint<1>         operationSwitch;
    SessionId          currID;

    if (rtt_waitForWrite and (rtt_rxeCmd.sessionID != rtt_prevPosition)) {
        // [TODO - maybe prevprev too]
        if (rtt_rxeCmd.command == LOAD_TIMER) {
            RETRANSMIT_TIMER_TABLE[rtt_rxeCmd.sessionID].time = TIME_1s;
        }
        else {  //-- STOP the timer
            RETRANSMIT_TIMER_TABLE[rtt_rxeCmd.sessionID].time   = 0;
            RETRANSMIT_TIMER_TABLE[rtt_rxeCmd.sessionID].active = false;
        }
        RETRANSMIT_TIMER_TABLE[rtt_rxeCmd.sessionID].retries = 0;
        rtt_waitForWrite = false;
    }
    //------------------------------------------------
    // Handle the input streams from RxEngine
    //------------------------------------------------
    else if (!siRXe_ReTxTimerCmd.empty() && !rtt_waitForWrite) {
        // INFO: Rx path has priority over Tx path
        siRXe_ReTxTimerCmd.read(rtt_rxeCmd);
        rtt_waitForWrite = true;
    }
    else {
        currID = rtt_position;
        //------------------------------------------------
        // Handle the input streams from TxEngine
        //------------------------------------------------
        if (!siTXe_ReTxTimerEvent.empty()) {
            siTXe_ReTxTimerEvent.read(txEvent);
            currID = txEvent.sessionID;
            operationSwitch = 1;
            if ( (txEvent.sessionID-3 <  rtt_position) &&
                 (rtt_position    <= txEvent.sessionID) ) {
                rtt_position += 5;
            }
        }
        else {
            // Increment position
            rtt_position++;
            if (rtt_position >= MAX_SESSIONS) {
                rtt_position = 0;
            }
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
            break;
        case 0:
            if (currEntry.active) {
                if (currEntry.time > 0) {
                    currEntry.time--;
                }
                // We need to check if we can generate another event, otherwise we might
                // end up in a Deadlock since the [TXe] will not be able to set new
                // retransmit timers.
                else if (!soEmx_Event.full()) {
                    currEntry.time   = 0;
                    currEntry.active = false;
                    if (currEntry.retries < 4) {
                        currEntry.retries++;
                        //-- Send timeout event to [TXe]
                        soEmx_Event.write(Event((EventType)currEntry.type,
                                          currID,
                                          currEntry.retries));
                        if (DEBUG_LEVEL & TRACE_RTT) {
                            printInfo(myName, "Forwarding event \'%s\' to [EVe].\n",
                                      getEventType(currEntry.type));
                        }
                    }
                    else {
                        currEntry.retries = 0;
                        soSmx_SessCloseCmd.write(currID);
                        if (currEntry.type == SYN_EVENT) {
                            soTAi_Notif.write(SessState(currID, CLOSED));
                            if (DEBUG_LEVEL & TRACE_RTT) {
                                printWarn(myName, "Notifying [TAi] - Failed to open session %d (event=\'%s\').\n",
                                          currID.to_int(), getEventType(currEntry.type));
                            }
                        }
                        else {
                            soRAi_Notif.write(TcpAppNotif(currID, ESTABLISHED));
                            if (DEBUG_LEVEL & TRACE_RTT) {
                                printWarn(myName, "Notifying [RAi] - Session %d timeout (event=\'%s\').\n",
                                          currID.to_int(), getEventType(currEntry.type));
                            }
                        }
                    }
                }
            }
            break;

        } // End of: switch

        // Write the entry back into the table
        RETRANSMIT_TIMER_TABLE[currID] = currEntry;
        rtt_prevPosition = currID;
    }
}

/*******************************************************************************
 * @brief Probe Timer (Prb) process.
 *
 * @param[in]  siRXe_ClrProbeTimer Clear probe timer command from RxEngine (RXe).
 * @param[in]  siTXe_SetProbeTimer Set probe timer from TxEngine (TXe).
 * @param[out] soEmx_Event         Event to EventMultiplexer (Emx).
 *
 * @details
 *   This process reads in 'set-' and 'clear-session-id' commands from [RXe]
 *    and [TXe]. Upon set request, a timer is initialized with an interval of
 *    of 50ms. When the timer expires, an 'RT_EVENT' is fired to the EventEngine
 *    via [Emx].
 *   In case of a zero-window (or too small window) an 'RT_EVENT' will generate
 *    a packet without payload which is the same as a probing packet.
 *
 *******************************************************************************/
void pProbeTimer(
        stream<SessionId>    &siRXe_ClrProbeTimer,
        stream<SessionId>    &siTXe_SetProbeTimer,
        stream<Event>        &soEmx_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Pbt");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static ProbeTimerEntry          PROBE_TIMER_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=PROBE_TIMER_TABLE core=RAM_T2P_BRAM
    #pragma HLS DATA_PACK  variable=PROBE_TIMER_TABLE
    #pragma HLS DEPENDENCE variable=PROBE_TIMER_TABLE inter false
    #pragma HLS RESET      variable=PROBE_TIMER_TABLE

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                pbt_WaitForWrite=false;
    #pragma HLS RESET variable=pbt_WaitForWrite
    static SessionId           pbt_currSessId=0;
    #pragma HLS RESET variable=pbt_currSessId
    static SessionId           pbt_updtSessId=0;
    #pragma HLS RESET variable=pbt_updtSessId
    static SessionId           pbt_prevSessId=0;
    #pragma HLS RESET variable=pbt_prevSessId

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    bool      fastResume = false;

    if (pbt_WaitForWrite) {
        //-- Update the table
        if (pbt_updtSessId != pbt_prevSessId) {
            PROBE_TIMER_TABLE[pbt_updtSessId].time = TIME_50ms;
            PROBE_TIMER_TABLE[pbt_updtSessId].active = true;
            pbt_WaitForWrite = false;
        }
        pbt_prevSessId--;
    }
    else if (!siTXe_SetProbeTimer.empty()) {
        //-- Read the Session-Id to set
        siTXe_SetProbeTimer.read(pbt_updtSessId);
        pbt_WaitForWrite = true;
    }
    else { // if (!soEmx_Event.full()) this leads to II=2
        SessionId sessIdToProcess = pbt_currSessId;

        if (!siRXe_ClrProbeTimer.empty()) {
            //-- Read the Session-Id to clear
            siRXe_ClrProbeTimer.read(sessIdToProcess);
            fastResume = true;
        }
        else {
            pbt_currSessId++;
            if (pbt_currSessId == MAX_SESSIONS) {
                pbt_currSessId = 0;
            }
        }

        if (PROBE_TIMER_TABLE[sessIdToProcess].active && !soEmx_Event.full()) {
            if (PROBE_TIMER_TABLE[sessIdToProcess].time == 0 || fastResume) {
                //-- Clear (de-activate) the current session-ID
                PROBE_TIMER_TABLE[sessIdToProcess].active = false;
                #if !(TCP_NODELAY)
                    soEmx_Event.write(Event(TX_EVENT, sessIdToProcess));
                #else
                    soEmx_Event.write(Event(RT_EVENT, sessIdToProcess));
                #endif
                fastResume = false;
            }
            else {
                PROBE_TIMER_TABLE[sessIdToProcess].time -= 1;
            }
        }
        pbt_prevSessId = sessIdToProcess;
    }
}

/*******************************************************************************
 * @brief Close Timer (Clt) process
 *
 * @param[in]  siRXe_CloseTimer    The session-id that is closing from [RXe].
 * @param[out] soSmx_SessCloseCmd  Close command to StateTableMux (Smx).
 *
 * @details
 *  This process reads in the session-id that is currently closing. This sessId
 *   is kept in the 'TIME-WAIT' state for an additional 60s before it gets closed.
 ******************************************************************************/
void pCloseTimer(
        stream<SessionId>    &siRXe_CloseTimer,
        stream<SessionId>    &soSmx_SessCloseCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    //-- STATIC ARRAYS ---------------------------------------------------------
    static CloseTimerEntry          CLOSE_TIMER_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=CLOSE_TIMER_TABLE core=RAM_T2P_BRAM
    #pragma HLS DATA_PACK  variable=CLOSE_TIMER_TABLE
    #pragma HLS DEPENDENCE variable=CLOSE_TIMER_TABLE inter false
    #pragma HLS RESET      variable=CLOSE_TIMER_TABLE

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                clt_waitForWrite=false;
    #pragma HLS RESET variable=clt_waitForWrite
    static SessionId           clt_currSessId=0;
    #pragma HLS RESET variable=clt_currSessId
    static SessionId           clt_prevSessId=0;
    #pragma HLS RESET variable=clt_prevSessId

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static SessionId           clt_sessIdToSet;

    if (clt_waitForWrite) {
        //-- Update the table
        if (clt_sessIdToSet != clt_prevSessId) {
            CLOSE_TIMER_TABLE[clt_sessIdToSet].time = TIME_60s;
            CLOSE_TIMER_TABLE[clt_sessIdToSet].active = true;
            clt_waitForWrite = false;
        }
        clt_prevSessId--;
    }
    else if (!siRXe_CloseTimer.empty()) {
        //-- Read the Session-Id to set
        siRXe_CloseTimer.read(clt_sessIdToSet);
        clt_waitForWrite = true;
    }
    else {
        clt_prevSessId = clt_currSessId;
        // Check if timer is 0, otherwise decrement
        if (CLOSE_TIMER_TABLE[clt_currSessId].active) {
            if (CLOSE_TIMER_TABLE[clt_currSessId].time > 0) {
                CLOSE_TIMER_TABLE[clt_currSessId].time -= 1;
            }
            else {
                CLOSE_TIMER_TABLE[clt_currSessId].time = 0;
                CLOSE_TIMER_TABLE[clt_currSessId].active = false;
                soSmx_SessCloseCmd.write(clt_currSessId);
            }
        }

        if (clt_currSessId == MAX_SESSIONS) {
            clt_currSessId = 0;
        }
        else {
            clt_currSessId++;
        }
    }
}

/*******************************************************************************
 * @brief The Timers (TIm)
 *
 * @param[in]  siRXe_ReTxTimerCmd   Retransmission timer command from Rx Engine (RXe).
 * @param[in]  siRXe_ClrProbeTimer  Clear probe timer from [RXe].
 * @param[in]  siRXe_CloseTimer     Close timer from [RXe].
 * @param[in]  siTXe_ReTxTimerEvent Retransmission timer event from Tx Engine (TXe).
 * @param[in]  siTXe_SetProbeTimer  Set probe timer from [TXe].
 * @param[out] soEVe_Event          Event to EventEngine (EVe).
 * @param[out] soSTt_SessCloseCmd   Close session command to State Table (STt).
 * @param[out] soTAi_Notif          Notification to Tx Application Interface (TAi).
 * @param[out] soRAi_Notif          Notification to Rx Application Interface (RAi).
 *
 * @detail
 *  This process includes all the timer-based processes of the [TOE].
 *******************************************************************************/
void timers(
        stream<RXeReTransTimerCmd> &siRXe_ReTxTimerCmd,
        stream<SessionId>          &siRXe_ClrProbeTimer,
        stream<SessionId>          &siRXe_CloseTimer,
        stream<TXeReTransTimerCmd> &siTXe_ReTxTimerevent,
        stream<SessionId>          &siTXe_SetProbeTimer,
        stream<SessionId>          &soSTt_SessCloseCmd,
        stream<Event>              &soEVe_Event,
        stream<SessState>          &soTAi_Notif,
        stream<TcpAppNotif>        &soRAi_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE

    //==========================================================================
    //== LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //==========================================================================
    static stream<SessionId>       ssClsToSmx_SessCloseCmd   ("ssClsToSmx_SessCloseCmd");
    #pragma HLS stream    variable=ssClsToSmx_SessCloseCmd   depth=2

    static stream<SessionId>       ssRttToSmx_SessCloseCmd   ("ssRttToSmx_SessCloseCmd");
    #pragma HLS stream    variable=ssRttToSmx_SessCloseCmd   depth=2

    static stream<Event>           ssRttToEmx_Event          ("ssRttToEmx_Event");
    #pragma HLS stream    variable=ssRttToEmx_Event          depth=2
    #pragma HLS DATA_PACK variable=ssRttToEmx_Event

    static stream<Event>           ssPbtToEmx_Event          ("ssPbtToEmx_Event");
    #pragma HLS stream    variable=ssPbtToEmx_Event          depth=2
    #pragma HLS DATA_PACK variable=ssPbtToEmx_Event

    // Event Mux (Emx) based on template stream Mux
    //  Notice order --> RetransmitTimer comes before ProbeTimer
    pStreamMux(
        ssRttToEmx_Event,
        ssPbtToEmx_Event,
        soEVe_Event);

    // ReTransmit  Timer (Rtt)
    pRetransmitTimer(
        siRXe_ReTxTimerCmd,
        siTXe_ReTxTimerevent,
        ssRttToEmx_Event,
        ssRttToSmx_SessCloseCmd,
        soTAi_Notif,
        soRAi_Notif);

    // Probe Timer (Pbt)
    pProbeTimer(
        siRXe_ClrProbeTimer,
        siTXe_SetProbeTimer,
        ssPbtToEmx_Event);

    pCloseTimer(
        siRXe_CloseTimer,
        ssClsToSmx_SessCloseCmd);

    // State table release Mux (Smx) based on template stream Mux
    pStreamMux(
        ssClsToSmx_SessCloseCmd,
        ssRttToSmx_SessCloseCmd,
        soSTt_SessCloseCmd);

}

/*! \} */
