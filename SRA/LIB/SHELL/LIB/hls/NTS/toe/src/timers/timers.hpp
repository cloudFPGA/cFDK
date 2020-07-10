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
 * @file       : timers.hpp
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

#ifndef _TOE_TIM_H_
#define _TOE_TIM_H_

#include "../toe.hpp"
#include "../event_engine/event_engine.hpp"
#include "../../../../NTS/nts_utils.hpp"

using namespace hls;

enum StateEntry {DISABLED_ENTRY = false,
                 ACTIVE_ENTRY   = true};

/********************************************
 * Cls - Close Timer Entry
 ********************************************/
class CloseTimerEntry
{
  public:
    ap_uint<32> time;
    bool        active;
    CloseTimerEntry() {}
};

/********************************************
 * Prb - Probe Timer Entry
 ********************************************/
class ProbeTimerEntry
{
  public:
    ap_uint<32>     time;
    bool            active;
    ProbeTimerEntry() {}
};

/********************************************
 * Rtt - Retransmission Timer Entry
 ********************************************/
class ReTxTimerEntry
{
  public:
    ap_uint<32>     time;
    ap_uint<3>      retries;
    bool            active;
    EventType       type;
    ReTxTimerEntry() {}
};

/*******************************************************************************
 *
 * @brief ENTITY - Timers (TIm)
 *
 *******************************************************************************/
void timers(
        stream<RXeReTransTimerCmd> &siRXe_ReTxTimerCmd,
        stream<TXeReTransTimerCmd> &siTXe_ReTxTimerevent,
        stream<ap_uint<16> >       &siRXe_ClrProbeTimer,
        stream<ap_uint<16> >       &siTXe_SetProbeTimer,
        stream<ap_uint<16> >       &siRXe_CloseTimer,
        stream<SessionId>          &soSTt_SessCloseCmd,
        stream<Event>              &soEVe_Event,
        stream<OpenStatus>         &soTAi_Notif,
        stream<AppNotif>           &soRAi_Notif
);

#endif

/*! \} */
