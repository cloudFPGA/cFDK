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
 * @file       : state_table.cpp
 * @brief      : State Table (STt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 ******************************************************************************/

#include "state_table.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (STT_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/STt"

#define TRACE_OFF  0x0000
#define TRACE_STT 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief State Table (STt)
 *
 * @param[in]  siRXe_SessStateQry    Session state query from RxEngine (RXe).
 * @param[out] soRXe_SessStateRep    Session state reply to [RXe].
 * @param[in]  siTAi_ConnectStateQry Session state query from [TxAppInterface/TxAppConnect(TAi/Tac).
 * @param[out] soTAi_ConnectStateRep Session state reply to [TAi/Tac].
 * @param[in]  siTAi_StreamStateReq  Session state request from TxAppInterface/TxAppStream (TAi/Tas).
 * @param[out] soTAi_StreamStateRep  Session state reply to [TAi/Tas].
 * @param[in]  siTIm_SessCloseCmd    Session close command from Timers (TIm).
 * @param[out] soSLc_SessReleaseCmd  Release session command to SessionLookupController (SLc).
 *
 * @details
 *  The StateTable (STt) stores the connection state of each session during its
 *   lifetime.  The states are defined in RFC793 as: LISTEN, SYN-SENT,
 *   SYN-RECEIVED, ESTABLISHED, FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING,
 *   LAST-ACK, TIME-WAIT and CLOSED.
 *  The StateTable is accessed by the RxEngine (RXe), the TxAppInterface (TAi)
 *   and by the TxEngine (TXe). The process also receives session close commands
 *   from the Timers (TIm) and it sends session release commands to the
 *   SessionLookupController (SLc).
 *******************************************************************************/
void state_table(
        stream<StateQuery>         &siRXe_SessStateQry,
        stream<TcpState>           &soRXe_SessStateRep,
        stream<StateQuery>         &siTAi_ConnectStateQry,
        stream<TcpState>           &soTAi_ConnectStateRep,
        stream<SessionId>          &siTAi_StreamStateReq,
        stream<TcpState>           &soTAi_StreamStateRep,
        stream<SessionId>          &siTIm_SessCloseCmd,
        stream<SessionId>          &soSLc_SessReleaseCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- STATIC ARRAYS ---------------------------------------------------------
    static TcpState                 SESS_STATE_TABLE[TOE_MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=SESS_STATE_TABLE core=RAM_2P_BRAM
    #pragma HLS DEPENDENCE variable=SESS_STATE_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                stt_rxWait=false;
    #pragma HLS RESET variable=stt_rxWait
    static bool                stt_txWait=false;
    #pragma HLS RESET variable=stt_txWait
    static bool                stt_rxSessionLocked=false;
    #pragma HLS RESET variable=stt_rxSessionLocked
    static bool                stt_txSessionLocked=false;
    #pragma HLS RESET variable=stt_txSessionLocked
    static bool                stt_closeWait=false;
    #pragma HLS RESET variable=stt_closeWait

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static StateQuery   stt_txAccess;
    static StateQuery   stt_rxAccess;
    static SessionId    stt_txSessionID;
    static SessionId    stt_rxSessionID;
    static SessionId    stt_closeSessionID;

    if(!siTAi_ConnectStateQry.empty() && !stt_txWait) {
        //-------------------------------------------------
        //-- Request from TAi/Connect
        //-------------------------------------------------
        siTAi_ConnectStateQry.read(stt_txAccess);
        if ((stt_txAccess.sessionID == stt_rxSessionID) && stt_rxSessionLocked) {
            stt_txWait = true;
        }
        else {
            if (stt_txAccess.write) {
                SESS_STATE_TABLE[stt_txAccess.sessionID] = stt_txAccess.state;
                stt_txSessionLocked = false;
                if (DEBUG_LEVEL & TRACE_STT) {
                    printInfo(THIS_NAME, "TAi is requesting to set SESS_STATE_TABLE[%d] = %s.\n", \
                              stt_txAccess.sessionID.to_uint(), \
                              getTcpStateName(stt_txAccess.state));
                }
            }
            else {
                soTAi_ConnectStateRep.write(SESS_STATE_TABLE[stt_txAccess.sessionID]);
                // Lock on every read
                stt_txSessionID = stt_txAccess.sessionID;
                stt_txSessionLocked = true;
            }
        }
    }
    else if (!siTAi_StreamStateReq.empty()) {
        //-------------------------------------------------
        //-- Request from TAi/Stream
        //-------------------------------------------------
        SessionId sessionID;
        siTAi_StreamStateReq.read(sessionID);
        soTAi_StreamStateRep.write(SESS_STATE_TABLE[sessionID]);
    }
    else if(!siRXe_SessStateQry.empty() && !stt_rxWait) {
        //-------------------------------------------------
        //-- Request from RxEngine
        //-------------------------------------------------
        siRXe_SessStateQry.read(stt_rxAccess);
        if ((stt_rxAccess.sessionID == stt_txSessionID) && stt_txSessionLocked) {
            stt_rxWait = true;
        }
        else {
            if (stt_rxAccess.write) {
                if (stt_rxAccess.state == CLOSED) {
                    // [TODO] Do we need to check if already closed?
                    // [TODO] && state_table[stt_rxAccess.sessionID] != CLOSED)
                    soSLc_SessReleaseCmd.write(stt_rxAccess.sessionID);
                }
                SESS_STATE_TABLE[stt_rxAccess.sessionID] = stt_rxAccess.state;
                stt_rxSessionLocked = false;
                if (DEBUG_LEVEL & TRACE_STT) {
                    printInfo(THIS_NAME, "RXe is requesting to set SESS_STATE_TABLE[%d] = %s.\n", \
                              stt_rxAccess.sessionID.to_uint(), \
                              getTcpStateName(stt_rxAccess.state));
                }
            }
            else {
                soRXe_SessStateRep.write(SESS_STATE_TABLE[stt_rxAccess.sessionID]);
                stt_rxSessionID = stt_rxAccess.sessionID;
                stt_rxSessionLocked = true;
            }
        }
    }
    else if (!siTIm_SessCloseCmd.empty() && !stt_closeWait) {
        //-------------------------------------------------
        //-- Request to close connection from Timers
        //-------------------------------------------------
        siTIm_SessCloseCmd.read(stt_closeSessionID);
        if (((stt_closeSessionID == stt_rxSessionID) && stt_rxSessionLocked) || \
            ((stt_closeSessionID == stt_txSessionID) && stt_txSessionLocked)) {
            // The session is currently locked
            stt_closeWait = true;
        }
        else {
            SESS_STATE_TABLE[stt_closeSessionID] = CLOSED;
            soSLc_SessReleaseCmd.write(stt_closeSessionID);
            if (DEBUG_LEVEL & TRACE_STT) {
                printWarn(THIS_NAME, "TIm is requesting to close connection with SessId=%d.\n",
                		stt_closeSessionID.to_uint());
            }
        }
    }
    else if (stt_txWait) {
        //-------------------------------------------------
        //-- Tx Wait
        //-------------------------------------------------
        if ((stt_txAccess.sessionID != stt_rxSessionID) || !stt_rxSessionLocked) {
            if (stt_txAccess.write) {
                SESS_STATE_TABLE[stt_txAccess.sessionID] = stt_txAccess.state;
                stt_txSessionLocked = false;
            }
            else {
                soTAi_ConnectStateRep.write(SESS_STATE_TABLE[stt_txAccess.sessionID]);
                stt_txSessionID = stt_txAccess.sessionID;
                stt_txSessionLocked = true;
            }
            stt_txWait = false;
        }
    }
    else if (stt_rxWait) {
        //-------------------------------------------------
        //-- Rx Wait
        //-------------------------------------------------
        if ((stt_rxAccess.sessionID != stt_txSessionID) || !stt_txSessionLocked) {
            if (stt_rxAccess.write) {
                if (stt_rxAccess.state == CLOSED) {
                    soSLc_SessReleaseCmd.write(stt_rxAccess.sessionID);
                }
                SESS_STATE_TABLE[stt_rxAccess.sessionID] = stt_rxAccess.state;
                stt_rxSessionLocked = false;
            }
            else {
                soRXe_SessStateRep.write(SESS_STATE_TABLE[stt_rxAccess.sessionID]);
                stt_rxSessionID = stt_rxAccess.sessionID;
                stt_rxSessionLocked = true;
            }
            stt_rxWait = false;
        }
    }
    else if (stt_closeWait) {
        //-------------------------------------------------
        //-- Close Wait
        //-------------------------------------------------
        if (((stt_closeSessionID != stt_rxSessionID) || !stt_rxSessionLocked) && \
            ((stt_closeSessionID != stt_txSessionID) || !stt_txSessionLocked)) {
            SESS_STATE_TABLE[stt_closeSessionID] = CLOSED;
            soSLc_SessReleaseCmd.write(stt_closeSessionID);
            stt_closeWait = false;
        }
    }
}

/*! \} */
