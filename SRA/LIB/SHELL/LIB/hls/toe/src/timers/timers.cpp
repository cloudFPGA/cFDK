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
 * @file       : event_engine.cpp
 * @brief      : Event Engine (EVe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "timers.hpp"
#include "../../test/test_toe_utils.hpp"

//OBSOLETE_20191202 #include "../close_timer/close_timer.hpp"
//OBSOLETE_20191202 #include "../probe_timer/probe_timer.hpp"
//OBSOLETE_20191202 #include "../retransmit_timer/retransmit_timer.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TIm"

#define TRACE_OFF  0x0000
#define TRACE_RTT  1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF | TRACE_RTT)


/*****************************************************************************
 * @brief A 2-to-1 Stream multiplexer.
 * ***************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so) {

	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*****************************************************************************
 * @brief ReTransmit Timer (Rtt) process.
 *
 * @param[in]      siRXe_ReTxTimerCmd,   Retransmit timer command from [RxEngine].
 * @param[in]      siTXe_ReTxTimerEvent, Retransmit timer event from [TxEngine].
 * @param[out]     soEmx_Event,          Event to Event Multiplexer (Emx).
 * @param[out]     soSMx_CloseSessCmd,   Close command to StateTable Mux (Smx).
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
 *******************************************************************************/
void pRetransmitTimer(
        stream<RXeReTransTimerCmd>       &siRXe_ReTxTimerCmd,
        stream<TXeReTransTimerCmd>       &siTXe_ReTxTimerEvent,
        stream<Event>                    &soEmx_Event,
        stream<SessionId>                &soSMx_CloseSessCmd,
        stream<OpenStatus>               &soTAi_Notif,
        stream<AppNotif>                 &soRAi_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    #pragma HLS DATA_PACK variable=soEmx_Event
    // NOT NEEDED #pragma HLS DATA_PACK variable=soSMx_CloseSessCmd

    static ReTxTimerEntry           RETRANSMIT_TIMER_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RETRANSMIT_TIMER_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=RETRANSMIT_TIMER_TABLE inter false

    const char *myName  = THIS_NAME;

    static bool                     rtt_waitForWrite = false;
    static SessionId                rtt_position     = 0;
    static SessionId                rtt_prevPosition = 0;
    static RXeReTransTimerCmd       rtt_cmd;

    ReTxTimerEntry     currEntry;
    TXeReTransTimerCmd txEvent;
    ap_uint<1>         operationSwitch = 0;
    SessionId          currID;

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
                        soEmx_Event.write(Event((EventType)currEntry.type,
                                          currID,
                                          currEntry.retries));
                        if (DEBUG_LEVEL & TRACE_RTT) {
                            if (currEntry.type == RT_EVENT)
                                printInfo(myName, "Forwarding RT event to [EventEngine].\n");
                            }
                    }
                    else {
                        currEntry.retries = 0;
                        soSMx_CloseSessCmd.write(currID);
                        if (currEntry.type == SYN_EVENT)
                            soTAi_Notif.write(OpenStatus(currID, FAILED_TO_OPEN_SESS));
                        else
                            soRAi_Notif.write(AppNotif(currID, SESS_IS_OPENED)); // TIME_OUT??
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

/*
 *  Reads in the Session-ID and activates. a timer with an interval of 5 seconds. When the timer times out
 *  a RT Event is fired to the @ref tx_engine. In case of a zero-window (or too small window) an RT Event
 *  will generate a packet without payload which is the same as a probing packet.
 *  @param[in]      txEng2timer_setProbeTimer
 *  @param[out]     probeTimer2eventEng_setEvent
 */
void pProbeTimer(
		stream<ap_uint<16> >    &rxEng2timer_clearProbeTimer,
        stream<ap_uint<16> >    &txEng2timer_setProbeTimer,
        stream<Event>           &probeTimer2eventEng_setEvent)
{

    #pragma HLS DATA_PACK variable=txEng2timer_setProbeTimer
    #pragma HLS DATA_PACK variable=probeTimer2eventEng_setEvent

    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static ProbeTimerEntry	probeTimerTable[MAX_SESSIONS];
    #pragma HLS RESOURCE variable=probeTimerTable core=RAM_T2P_BRAM
    #pragma HLS DATA_PACK variable=probeTimerTable
    #pragma HLS DEPENDENCE variable=probeTimerTable inter false

    static ap_uint<16>      pt_currSessionID = 0;
    static ap_uint<16>      pt_updSessionID = 0;
    static ap_uint<16>      pt_prevSessionID = 0;
    static bool             pt_WaitForWrite = false;

    ap_uint<16> checkID;
    bool        fastResume = false;

    if (pt_WaitForWrite) {
        if (pt_updSessionID != pt_prevSessionID) {
            probeTimerTable[pt_updSessionID].time = TIME_50ms;
            probeTimerTable[pt_updSessionID].active = true;
            pt_WaitForWrite = false;
        }
        pt_prevSessionID--;
    }
    else if (!txEng2timer_setProbeTimer.empty()) {
        txEng2timer_setProbeTimer.read(pt_updSessionID);
        pt_WaitForWrite = true;
    }
    else { // if (!probeTimer2eventEng_setEvent.full()) this leads to II=2
        if (!rxEng2timer_clearProbeTimer.empty()) {
            rxEng2timer_clearProbeTimer.read(checkID);
            fastResume = true;
        }
        else {
            (pt_currSessionID == MAX_SESSIONS) ? pt_currSessionID = 0 : pt_currSessionID++;
            checkID = pt_currSessionID;
        }
        // Check if 0, otherwise decrement
        if (probeTimerTable[checkID].active) {
            if (fastResume) {
                probeTimerTable[checkID].active = false;
                //fastResume = false;
            }
            //if (probeTimerTable[checkID].time == 0 || fastResume) {
            else if (probeTimerTable[checkID].time == 0 && !probeTimer2eventEng_setEvent.full()) {
                //probeTimerTable[checkID].time = 0;
                probeTimerTable[checkID].active = false;
                probeTimer2eventEng_setEvent.write(Event(TX_EVENT, checkID)); // It's not an RT, we want to resume TX
            }
            else
                probeTimerTable[checkID].time -= 1;
        }
        pt_prevSessionID = checkID;
    }
}

/*
 *  Reads in Session-IDs, the corresponding is kept in the TIME-WAIT state for 60s before
 *  it gets closed by writing its ID into the closeTimerReleaseFifo.
 *  @param[in]      timer2closeTimer_setTimer, FIFO containing Session-ID of the sessions which are in the TIME-WAIT state
 *  @param[out]     timer2stateTable_releaseState, write Sessions which are closed into this FIFO
 */
void pCloseTimer(
		stream<ap_uint<16> >	&rxEng2timer_setCloseTimer,
		stream<ap_uint<16> >	&closeTimer2stateTable_releaseState)
{
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    #pragma HLS DATA_PACK variable=rxEng2timer_setCloseTimer
    #pragma HLS DATA_PACK variable=closeTimer2stateTable_releaseState

    static CloseTimerEntry	closeTimerTable[MAX_SESSIONS];
    #pragma HLS RESOURCE variable=closeTimerTable core=RAM_T2P_BRAM
    #pragma HLS DATA_PACK variable=closeTimerTable
    #pragma HLS DEPENDENCE variable=closeTimerTable inter false

    static ap_uint<16>  ct_currSessionID = 0;
    static ap_uint<16>  ct_setSessionID = 0;
    static ap_uint<16>  ct_prevSessionID = 0;
    static bool         ct_waitForWrite = false;
    //ap_uint<16> sessionID;

    if (ct_waitForWrite) {
        if (ct_setSessionID != ct_prevSessionID) {
            closeTimerTable[ct_setSessionID].time = TIME_60s;
            closeTimerTable[ct_setSessionID].active = true;
            ct_waitForWrite = false;
        }
        ct_prevSessionID--;
    }
    else if (!rxEng2timer_setCloseTimer.empty()) {
        rxEng2timer_setCloseTimer.read(ct_setSessionID);
        ct_waitForWrite = true;
    }
    else {
        ct_prevSessionID = ct_currSessionID;

        if (closeTimerTable[ct_currSessionID].active) { // Check if 0, otherwise decrement
            if (closeTimerTable[ct_currSessionID].time > 0)
                closeTimerTable[ct_currSessionID].time -= 1;
            else {
                closeTimerTable[ct_currSessionID].time = 0;
                closeTimerTable[ct_currSessionID].active = false;
                closeTimer2stateTable_releaseState.write(ct_currSessionID);
            }
        }
        (ct_currSessionID == MAX_SESSIONS) ? ct_currSessionID = 0 : ct_currSessionID++;
    }
}


/*****************************************************************************
* @brief The Timers (TIm) includes all the timer-based processes .
*
* @param[in]  siRXe_ReTxTimerCmd,  Retransmission timer command from Rx Engine (RXe).
* @param[in]  siTXe_ReTxTimerEvent,Retransmission timer event from Tx Engine (TXe).
* @param[in]  siRXe_ClrProbeTimer, Clear probe timer from [RXe].
* @param[in]  siTXe_SetProbeTimer, Set probe timer from [TXe].
* @param[in]  siRXe_CloseTimer,    Close timer from [RXe].
* @param[out] soEVe_Event,         Event to EventEngine (EVe).
* @param[out] soSTt_SessCloseCmd,  Close session command to State Table (STt).
* @param[out] soTAi_Notif,         Notification to Tx Application Interface (TAi).
* @param[out] soRAi_Notif,         Notification to Rx Application Interface (TAi).
*
*****************************************************************************/
void timers(
		stream<RXeReTransTimerCmd> &siRXe_ReTxTimerCmd,
		stream<TXeReTransTimerCmd> &siTXe_ReTxTimerevent,
		stream<ap_uint<16> >       &siRXe_ClrProbeTimer,
		stream<ap_uint<16> >       &siTXe_SetProbeTimer,
		stream<ap_uint<16> >       &siRXe_CloseTimer,
		stream<SessionId>          &soSTt_SessCloseCmd,
		stream<Event>              &soEVe_Event,
		stream<OpenStatus>         &soTAi_Notif,
		stream<AppNotif>           &soRAi_Notif)
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
  //OBSOLETE-20191111 #pragma HLS INLINE
  #pragma HLS INLINE // off

  static stream<ap_uint<16> > sCloseTimer2Mux_ReleaseState ("sCloseTimer2Mux_ReleaseState");
  #pragma HLS stream variable=sCloseTimer2Mux_ReleaseState depth=2

  static stream<ap_uint<16> > sRttToSmx_SessCloseCmd       ("sRttToSmx_SessCloseCmd");
  #pragma HLS stream variable=sRttToSmx_SessCloseCmd       depth=2

  static stream<Event>        sRttToEmx_Event              ("sRttToEmx_Event");
  #pragma HLS stream variable=sRttToEmx_Event              depth=2

  static stream<Event>        sPbToEmx_Event               ("sPbToEmx_Event");
  #pragma HLS stream variable=sPbToEmx_Event               depth=2

  // Event Mux (Emx) based on template stream Mux
  //  Notice order --> RetransmitTimer comes before ProbeTimer
  pStreamMux(
		  sRttToEmx_Event,
		  sPbToEmx_Event,
		  soEVe_Event);

  // ReTransmit  Timer (Rtt)
  pRetransmitTimer(
		  siRXe_ReTxTimerCmd,
		  siTXe_ReTxTimerevent,
		  sRttToEmx_Event,
		  sRttToSmx_SessCloseCmd,
		  soTAi_Notif,
		  soRAi_Notif);

  // Probe Timer (Pbt)
  pProbeTimer(
		  siRXe_ClrProbeTimer,
		  siTXe_SetProbeTimer,
		  sPbToEmx_Event);

  pCloseTimer(
		  siRXe_CloseTimer,
		  sCloseTimer2Mux_ReleaseState);

  // State table release Mux (Smx) based on template stream Mux
  pStreamMux(
		  sCloseTimer2Mux_ReleaseState,
		  sRttToSmx_SessCloseCmd,
		  soSTt_SessCloseCmd);

}

