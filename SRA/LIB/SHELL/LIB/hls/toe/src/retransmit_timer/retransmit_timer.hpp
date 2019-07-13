/*****************************************************************************
 * @file       : retransmit_timer.hpp
 * @brief      : Retransmission timers (Rtt) of the TCP Offload Engine (TOE).
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
 * @details    : Data structures, types and prototypes definitions for the
 *               TCP Retransmission Timers.
 *
 *****************************************************************************/

#include "../toe.hpp"

using namespace hls;

/********************************************
 * Rtt - Retransmission Timer Entry
 ********************************************/

/*** OBSOLETE-20180181 ***
struct retransmitTimerEntry
{
    ap_uint<32>     time;
    ap_uint<3>      retries;
    bool            active;
    eventType       type;
};
**************************/

enum StateEntry {DISABLED_ENTRY = false,
                 ACTIVE_ENTRY   = true};

struct ReTxTimerEntry
{
    ap_uint<32>     time;
    ap_uint<3>      retries;
    bool            active;
    EventType       type;
};


struct mergedInput
{
    ap_uint<16>     sessionID;
    eventType       type;
    bool            stop;
    bool            isSet;
    mergedInput() {}
    mergedInput(ap_uint<16> id, eventType t)
            :sessionID(id), type(t), stop(false), isSet(true) {}
    mergedInput(ap_uint<16> id, bool stop)
            :sessionID(id), type(RT), stop(stop), isSet(false) {}
};


/*****************************************************************************
 * @brief   Main process of the TCP Retransmission Timers (Rt).
 *
 * @ingroup retransmit_timer
 *****************************************************************************/
void pRetransmitTimer(
        stream<RXeReTransTimerCmd> &siRXe_ReTxTimerCmd,
        stream<TXeReTransTimerCmd> &siTXe_SetReTxTimer,
        stream<event>              &soEmx_SetEvent,
        stream<ap_uint<16> >       &soSmx_ReleaseState,
        stream<OpenStatus>         &soTAi_Notif,
        stream<AppNotif>           &soRAi_Notif
);
