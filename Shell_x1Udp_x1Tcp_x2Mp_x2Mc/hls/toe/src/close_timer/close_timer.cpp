#include "close_timer.hpp"

using namespace hls;

/** @ingroup close_timer
 *  Reads in Session-IDs, the corresponding is kept in the TIME-WAIT state for 60s before
 *  it gets closed by writing its ID into the closeTimerReleaseFifo.
 *  @param[in]      timer2closeTimer_setTimer, FIFO containing Session-ID of the sessions which are in the TIME-WAIT state
 *  @param[out]     timer2stateTable_releaseState, write Sessions which are closed into this FIFO
 */
void close_timer(   stream<ap_uint<16> >&       rxEng2timer_setCloseTimer,
                    stream<ap_uint<16> >&       closeTimer2stateTable_releaseState)
{
#pragma HLS PIPELINE II=1

#pragma HLS DATA_PACK variable=rxEng2timer_setCloseTimer
#pragma HLS DATA_PACK variable=closeTimer2stateTable_releaseState

    static close_timer_entry closeTimerTable[MAX_SESSIONS];
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
