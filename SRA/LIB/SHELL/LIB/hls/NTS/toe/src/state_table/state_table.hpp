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
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/******************************************************************************
 * @file       : state_table.hpp
 * @brief      : State Table (STt) for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 ******************************************************************************/

#ifndef _TOE_STT_H_
#define _TOE_STT_H_

#include "../../../../NTS/toe/src/toe.hpp"
#include "../../../../NTS/nts_utils.hpp"

using namespace hls;

/********************************************
 * Session States
 ********************************************/
enum SessionState { CLOSED=0,    SYN_SENT,    SYN_RECEIVED,   ESTABLISHED, \
                    FIN_WAIT_1,  FIN_WAIT_2,  CLOSING,        TIME_WAIT,   \
                    LAST_ACK };

#ifndef __SYNTHESIS__
    const std::string  SessionStateString[] = {
                   "CLOSED",    "SYN_SENT",  "SYN_RECEIVED", "ESTABLISHED", \
                   "FIN_WAIT_1","FIN_WAIT_2","CLOSING",      "TIME_WAIT",   \
                   "LAST_ACK" };
#endif

/********************************************
 * STt - Session State Query
 ********************************************/
class StateQuery {
  public:
    SessionId       sessionID;
    SessionState    state;
    RdWrBit         write;
    StateQuery() {}
    StateQuery(SessionId id) :
        sessionID(id), state(CLOSED), write(QUERY_RD) {}
    StateQuery(SessionId id, SessionState state, RdWrBit write) :
        sessionID(id), state(state), write(write) {}
};

/*******************************************************************************
 *
 * @brief ENTITY - State Table (STt)
 *
 *******************************************************************************/
void state_table(
        stream<StateQuery>         &siRXe_SessStateQry,
        stream<SessionState>       &soRXe_SessStateRep,
        stream<StateQuery>         &siTAi_ConnectStateQry,
        stream<SessionState>       &soTAi_ConnectStateRep,
        stream<SessionId>          &siTAi_StreamStateReq,
        stream<SessionState>       &soTAi_StreamStateRep,
        stream<SessionId>          &siTIm_SessCloseCmd,
        stream<SessionId>          &soSLc_SessReleaseCmd
);

#endif

/*! \} */
