#include "probe_timer.hpp"

using namespace hls;

/** @ingroup probe_timer
 *  Reads in the Session-ID and activates. a timer with an interval of 5 seconds. When the timer times out
 *  a RT Event is fired to the @ref tx_engine. In case of a zero-window (or too small window) an RT Event
 *  will generate a packet without payload which is the same as a probing packet.
 *  @param[in]      txEng2timer_setProbeTimer
 *  @param[out]     probeTimer2eventEng_setEvent
 */
void probe_timer(   stream<ap_uint<16> >&       rxEng2timer_clearProbeTimer,
                    stream<ap_uint<16> >&       txEng2timer_setProbeTimer,
                    stream<event>&              probeTimer2eventEng_setEvent) {
#pragma HLS DATA_PACK variable=txEng2timer_setProbeTimer
#pragma HLS DATA_PACK variable=probeTimer2eventEng_setEvent

#pragma HLS PIPELINE II=1

    static probe_timer_entry probeTimerTable[MAX_SESSIONS];
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
                probeTimer2eventEng_setEvent.write(event(TX, checkID)); // It's not an RT, we want to resume TX
            }
            else
                probeTimerTable[checkID].time -= 1;
        }
        pt_prevSessionID = checkID;
    }
}
