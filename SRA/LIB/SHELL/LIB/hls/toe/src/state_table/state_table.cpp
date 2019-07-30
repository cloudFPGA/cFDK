/******************************************************************************
 * @file       : state_table.cpp
 * @brief      : State Table (STt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 ******************************************************************************/

#include "state_table.hpp"
#include "../../test/test_toe_utils.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/STt"

#define TRACE_OFF  0x0000
#define TRACE_STT 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/******************************************************************************
 * @brief State Table (STt) .
 *
 * @param[in]  siRXe_SessStateQry,  Session state query from RxEngine (RXe).
 * @param[out] soRXe_SessStateRep,  Session state reply to [RXe].
 * @param[in]  siTAi_Taa_StateQry,  Session state query from TxAppInterface_TxAppAccept(TAi_Taa).
 * @param[out] soTAi_Taa_StateRep,  Session state reply to [TAi_Taa].
 * @param[in]  siTAi_Tas_StateReq,  Session state request from TxAppInterface_TxAppStream (TAi_Tas).
 * @param[out] soTAi_Tas_StateRep,  Session state reply to [TAi_Tas].
 * @param[in]  siTIm_SessCloseCmd,  Session close command from Timers (TIm).
 * @param[out] soSLc_SessReleaseCmd,Release session command to SessionLookupController (SLc).
 *
 * @details
 *  The (session) StateTable stores the TCP connection state of each session.
 *  It is accessed by the RxEngine, the TxAppInterface and by the TxEngine.
 *  It also receives session close commands from the Timers and sends session
 *  release commands to the SessionLookupController.
 *
 *****************************************************************************/
void state_table(
        stream<StateQuery>         &siRXe_SessStateQry,
        stream<SessionState>       &soRXe_SessStateRep,
        stream<StateQuery>         &siTAi_Taa_StateQry,
        stream<SessionState>       &soTAi_Taa_StateRep,
        stream<SessionId>          &siTAi_Tas_StateReq,
        stream<SessionState>       &soTAi_Tas_StateRep,
        stream<SessionId>          &siTIm_SessCloseCmd,
        stream<SessionId>          &soSLc_SessReleaseCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static SessionState             SESS_STATE_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=SESS_STATE_TABLE core=RAM_2P_BRAM
    #pragma HLS DEPENDENCE variable=SESS_STATE_TABLE inter false

    static SessionId    stt_txSessionID;
    static SessionId    stt_rxSessionID;
    static bool         stt_rxSessionLocked = false;
    static bool         stt_txSessionLocked = false;

    static StateQuery   stt_txAccess;
    static StateQuery   stt_rxAccess;
    static bool         stt_txWait = false;
    static bool         stt_rxWait = false;

    static SessionId    stt_closeSessionID;
    static bool         stt_closeWait = false;

    SessionId           sessionID;

    if(!siTAi_Taa_StateQry.empty() && !stt_txWait) {
        //-------------------------------------------------
        //-- From TAi/TxAppAccept (TAi_Taa)
        //-------------------------------------------------
        siTAi_Taa_StateQry.read(stt_txAccess);
        if ((stt_txAccess.sessionID == stt_rxSessionID) && stt_rxSessionLocked)
            stt_txWait = true;
        else {
            if (stt_txAccess.write) {
                SESS_STATE_TABLE[stt_txAccess.sessionID] = stt_txAccess.state;
                stt_txSessionLocked = false;
                if (DEBUG_LEVEL & TRACE_STT)
                    printInfo(THIS_NAME, "TAi is requesting to set SESS_STATE_TABLE[%d] = %s.\n", \
                              stt_txAccess.sessionID.to_uint(), \
                              SessionStateString[stt_txAccess.state].c_str());
            }
            else {
                soTAi_Taa_StateRep.write(SESS_STATE_TABLE[stt_txAccess.sessionID]);
                // Lock on every read
                stt_txSessionID = stt_txAccess.sessionID;
                stt_txSessionLocked = true;
            }
        }
    }
    else if (!siTAi_Tas_StateReq.empty()) {
        //-------------------------------------------------
        //-- From TAi / TxAppStream (TAi_Tas)
        //-------------------------------------------------
        siTAi_Tas_StateReq.read(sessionID);
        soTAi_Tas_StateRep.write(SESS_STATE_TABLE[sessionID]);
    }
    else if(!siRXe_SessStateQry.empty() && !stt_rxWait) {
        //-------------------------------------------------
        //-- Request from RxEngine
        //-------------------------------------------------
        siRXe_SessStateQry.read(stt_rxAccess);
        if ((stt_rxAccess.sessionID == stt_txSessionID) && stt_txSessionLocked)
            stt_rxWait = true;
        else {
            if (stt_rxAccess.write) {
                if (stt_rxAccess.state == CLOSED)// && state_table[stt_rxAccess.sessionID] != CLOSED) // We check if it was not closed before, not sure if necessary
                    soSLc_SessReleaseCmd.write(stt_rxAccess.sessionID);
                SESS_STATE_TABLE[stt_rxAccess.sessionID] = stt_rxAccess.state;
                stt_rxSessionLocked = false;
                if (DEBUG_LEVEL & TRACE_STT)
                    printInfo(THIS_NAME, "RXe is requesting to set SESS_STATE_TABLE[%d] = %s.\n", \
                              stt_rxAccess.sessionID.to_uint(), \
                              SessionStateString[stt_rxAccess.state].c_str());
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
        //-- Close command from Timers
        //-------------------------------------------------
        siTIm_SessCloseCmd.read(stt_closeSessionID);
        if (((stt_closeSessionID == stt_rxSessionID) && stt_rxSessionLocked) || \
            ((stt_closeSessionID == stt_txSessionID) && stt_txSessionLocked))
            // The session is currently locked
            stt_closeWait = true;
        else {
            SESS_STATE_TABLE[stt_closeSessionID] = CLOSED;
            soSLc_SessReleaseCmd.write(stt_closeSessionID);
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
                soTAi_Taa_StateRep.write(SESS_STATE_TABLE[stt_txAccess.sessionID]);
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
                if (stt_rxAccess.state == CLOSED)
                    soSLc_SessReleaseCmd.write(stt_rxAccess.sessionID);
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
