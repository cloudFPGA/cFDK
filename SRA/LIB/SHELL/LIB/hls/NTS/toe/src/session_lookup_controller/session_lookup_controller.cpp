/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/************************************************
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
 * @file       : session_lookup.cpp
 * @brief      : Session Lookup Controller (SLc) of TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "session_lookup_controller.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_LRH | TRACE_URH)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/SLc"

#define TRACE_OFF 0x0000
#define TRACE_LRH 1 << 1
#define TRACE_RLT 1 << 2
#define TRACE_SIM 1 << 3
#define TRACE_URH 1 << 4
#define TRACE_URS 1 << 5

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Session Id Manager (Sim)
 *
 * @param[in]  siUrs_FreeId   The session ID to recycle from the UpdateRequestSender (Urs).
 * @param[out] soLrh_FreeList The free list of session IDs to LookupReplyHandler (Lrh).
 *
 * @details
 *  Implements the free list of session IDs as a FiFo stream.
 *******************************************************************************/
void pSessionIdManager(
        stream<RtlSessId>    &siUrs_FreeId,
        stream<RtlSessId>    &soLrh_FreeList)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Sim");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static RtlSessId           sim_counter=0;
    #pragma HLS reset variable=sim_counter

    if (sim_counter < TOE_MAX_SESSIONS) {
        // Initialize the free list after a reset
        soLrh_FreeList.write(sim_counter);
        sim_counter++;
    }
    else if (!siUrs_FreeId.empty()) {
        // Recycle the incoming session ID
        RtlSessId rtlSessId;
        siUrs_FreeId.read(rtlSessId);
        soLrh_FreeList.write(rtlSessId);
    }
}

/*******************************************************************************
 * @brief Lookup Reply Handler (Lrh)
 *
 * @param[out] soCAM_SessLookupReq Request to ternary CAM (CAM).
 * @param[in]  siCAM_SessLookupRep Reply from ternary CAM.
 * @param[in]  siUrh_SessUpdateRsp Update response from Update Reply Handler (Urh).
 * @param[in]  siRXe_SessLookupReq Request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep Reply from CAM to RXe.
 * @param[in]  siTAi_SessLookupReq Request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupRep Reply from CAM to TAi.
 * @param[in]  siSim_FreeList      Free liste from Session Id Manager (Sid).
 * @param[out] soUrs_InsertSessReq Request to insert session to Update Request Sender (Urs).
 * @param[out] soRlt_ReverseLkpRsp Reverse lookup response to Reverse Lookup Table (Rlt).
 *
 * @details
 *  This is the front-end process of the ternary content addressable memory
 *   (TCAM or CAM for short). It handles the session lookup requests from [RXe]
 *    and [TAi]. The process prioritizes [TAi] over [RXe] when forwarding the
 *    lookup request to the ternary content addressable memory.
 *  If there was no hit and the request is allowed to create a new session
 *   entry in the CAM, such a new entry is created. Otherwise, the session ID
 *   corresponding to the matching lookup is sent back to lookup requester.
 *  [TODO-FIXME - This process does not yet handle the deletion of a session].
 *******************************************************************************/
void pLookupReplyHandler(
        stream<CamSessionLookupRequest>     &soCAM_SessLookupReq,
        stream<CamSessionLookupReply>       &siCAM_SessLookupRep,
        stream<CamSessionUpdateReply>       &siUrh_SessUpdateRsp,
        stream<SessionLookupQuery>          &siRXe_SessLookupReq,
        stream<SessionLookupReply>          &soRXe_SessLookupRep,
        stream<SocketPair>                  &siTAi_SessLookupReq,
        stream<SessionLookupReply>          &soTAi_SessLookupRep,
        stream<RtlSessId>                   &siSim_FreeList,
        stream<CamSessionUpdateRequest>     &soUrs_InsertSessReq,
        stream<SLcReverseLkp>               &soRlt_ReverseLkpRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Lrh");

    //-- LOCAL STREAMS  --------------------------------------------------------
    static stream<SLcFourTuple>               ssInsertPipe ("ssInsertPipe");
    #pragma HLS STREAM               variable=ssInsertPipe depth=4

    static stream<SLcQuery>                   ssLookupPipe ("ssLookupPipe");
    #pragma HLS STREAM               variable=ssLookupPipe depth=8

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { WAIT_FOR_SESS_LKP_REQ=0, WAIT_FOR_CAM_LKP_REP, WAIT_FOR_CAM_UPD_REP } \
                                 lrh_fsmState=WAIT_FOR_SESS_LKP_REQ;
    #pragma HLS RESET	variable=lrh_fsmState

    switch (lrh_fsmState) {
    case WAIT_FOR_SESS_LKP_REQ:
        if (!siTAi_SessLookupReq.empty()) {
            SocketPair sockPair = siTAi_SessLookupReq.read();
            fourTuple toeTuple = fourTuple(byteSwap32(sockPair.src.addr), byteSwap32(sockPair.dst.addr),
                                           byteSwap16(sockPair.src.port), byteSwap16(sockPair.dst.port));
            // Create internal query { myIp, TheirIp, myPort, theirPort}
            SLcQuery slcQuery = SLcQuery(SLcFourTuple(
                    toeTuple.srcIp,   toeTuple.dstIp,
                    toeTuple.srcPort, toeTuple.dstPort), true, FROM_TAi);
            soCAM_SessLookupReq.write(CamSessionLookupRequest(slcQuery.tuple, slcQuery.source));
            ssLookupPipe.write(slcQuery);
            lrh_fsmState = WAIT_FOR_CAM_LKP_REP;
        }
        else if (!siRXe_SessLookupReq.empty()) {
            SessionLookupQuery query = siRXe_SessLookupReq.read();
            // Create internal query { myIp, TheirIp, myPort, theirPort}
            SLcQuery slcQuery = SLcQuery(SLcFourTuple(
                    query.tuple.dst.addr, query.tuple.src.addr,
                    query.tuple.dst.port, query.tuple.src.port),
                    query.allowCreation, FROM_RXe);
            soCAM_SessLookupReq.write(CamSessionLookupRequest(slcQuery.tuple, slcQuery.source));
            ssLookupPipe.write(slcQuery);
            lrh_fsmState = WAIT_FOR_CAM_LKP_REP;
        }
        break;
    case WAIT_FOR_CAM_LKP_REP:
        if(!siCAM_SessLookupRep.empty() && !ssLookupPipe.empty()) {
            CamSessionLookupReply lupReply = siCAM_SessLookupRep.read();
            SLcQuery              slcQuery = ssLookupPipe.read();
            if (!lupReply.hit && slcQuery.allowCreation && !siSim_FreeList.empty()) {
                RtlSessId freeID = siSim_FreeList.read();
                // Request to insert a new session into the CAM
                soUrs_InsertSessReq.write(CamSessionUpdateRequest(slcQuery.tuple, freeID, INSERT, lupReply.source));
                ssInsertPipe.write(slcQuery.tuple);
                lrh_fsmState = WAIT_FOR_CAM_UPD_REP;
            }
            else {
                // We have a HIT
                if (lupReply.source == FROM_RXe) {
                    soRXe_SessLookupRep.write(SessionLookupReply(lupReply.sessionID, lupReply.hit));
                }
                else {
                    soTAi_SessLookupRep.write(SessionLookupReply(lupReply.sessionID, lupReply.hit));
                }
                lrh_fsmState = WAIT_FOR_SESS_LKP_REQ;
            }
        }
        break;
    case WAIT_FOR_CAM_UPD_REP:
        if (!siUrh_SessUpdateRsp.empty() && !ssInsertPipe.empty()) {
            CamSessionUpdateReply insertReply = siUrh_SessUpdateRsp.read();
            SLcFourTuple tuple = ssInsertPipe.read();
            if (insertReply.source == FROM_RXe) {
                soRXe_SessLookupRep.write(SessionLookupReply(insertReply.sessionID, true));
            }
            else {
                soTAi_SessLookupRep.write(SessionLookupReply(insertReply.sessionID, true));
            }
            soRlt_ReverseLkpRsp.write(SLcReverseLkp(insertReply.sessionID, tuple));
            lrh_fsmState = WAIT_FOR_SESS_LKP_REQ;
        }
        break;
    }
} // End of: pLookupReplyHandler

/*******************************************************************************
 * @brief Update Request Sender (Urs)
 *
 * @param[in]  siLrh_InsertSessReq Request to insert session from LookupReplyHandler (Lrh).
 * @param[in]  siRlt_SessDeleteReq Request to delete session from Reverse Lookup Table (Rlt).
 * @param[out] soCAM_SessUpdateReq Update request to [CAM].
 * @param[out] soSim_FreeId        The SessId to recycle to the [SessionIdManager].
 * @param[out] soSssRelCnt         Session release count to DEBUG.
 * @param[out] soSssRegCnt         Session register count to DEBUG.
 *
 * @details
 *  This process sends the insertion or deletion requests to the ternary content
 *   addressable memory (TCAM or CAM for short).
 *  If a session deletion is requested, the corresponding sessionId is collected
 *   from the request and is forwarded to the SessionIdManager for re-cycling.
 *
 *******************************************************************************/
void pUpdateRequestSender(
        stream<CamSessionUpdateRequest>     &siLrh_InsertSessReq,
        stream<CamSessionUpdateRequest>     &siRlt_SessDeleteReq,
        stream<CamSessionUpdateRequest>     &soCAM_SessUpdateReq,
        stream<RtlSessId>                   &soSim_FreeId,
        stream<ap_uint<16> >                &soSssRelCnt,
        stream<ap_uint<16> >                &soSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Urs");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<16>         urs_insertedSessions=0;
    #pragma HLS RESET variable=urs_insertedSessions
    static ap_uint<16>         urs_releasedSessions=0;
    #pragma HLS RESET variable=urs_releasedSessions

    if (!siLrh_InsertSessReq.empty()) {
        soCAM_SessUpdateReq.write(siLrh_InsertSessReq.read());
        urs_insertedSessions++;
    }
    else if (!siRlt_SessDeleteReq.empty()) {
        CamSessionUpdateRequest request;
        siRlt_SessDeleteReq.read(request);
        soCAM_SessUpdateReq.write(request);
        soSim_FreeId.write(request.value);
        urs_releasedSessions++;
    }
    // Always
    if (!soSssRegCnt.full()) {
        soSssRegCnt.write(urs_insertedSessions);
    }
    if (!soSssRelCnt.full()) {
        soSssRelCnt.write(urs_releasedSessions);
    }
}


/*******************************************************************************
 * @brief Update Reply Handler (Urh)
 *
 *  @param[in]  siCAM_SessUpdateRep  Session update reply from [CAM].
 *  @param[out] soLrh_SessUpdateRsp  Session update response to LookupReplyHandler (Lrh).*
 *******************************************************************************/
void pUpdateReplyHandler(
        stream<CamSessionUpdateReply>   &siCAM_SessUpdateRep,
        stream<CamSessionUpdateReply>   &soLrh_SessUpdateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Urh");

    if (!siCAM_SessUpdateRep.empty()) {
        CamSessionUpdateReply upReply;
        siCAM_SessUpdateRep.read(upReply);
        if (upReply.op == INSERT)
           soLrh_SessUpdateRsp.write(upReply);
    }
}

/*******************************************************************************
 * @brief Reverse Lookup Table (Rlt)
 *
 *  @param[in]  sLrh_ReverseLkpRsp   Reverse lookup response from LookupReplyHandler (Lrh).
 *  @param[in]  siSTt_SessReleaseCmd Session release command from StateTable (STt).
 *  @param[in[  siTXe_ReverseLkpReq  Reverse lookup request from TxEngine (TXe).
 *  @param[out] soTXe_ReverseLkpRep  Reverse lookup reply to TxEngine (TXe).
 *  @param[out] soPRt_ClosePortCmd   Command to close a port for the PortTable (PRt).
 *  @param[out] soUrs_SessDeleteReq  Request to delete a session to UpdateRequestSender (Urs).
 *
 * @details
 *  The REVERSE_LOOKUP_TABLE stores a four-tuple piece of information at memory
 *   address indexed by the 'SessionId' of that 4-tuple. This table is used
 *   to retrieve the 4-tuple information corresponding to a SessionId upon
 *   request from [TXe].
 *******************************************************************************/
void pReverseLookupTable(
        stream<SLcReverseLkp>   &siLrh_ReverseLkpRsp,
        stream<SessionId>       &siSTt_SessReleaseCmd,
        stream<SessionId>       &siTXe_ReverseLkpReq,
        stream<fourTuple>       &soTXe_ReverseLkpRep,
        stream<TcpPort>         &soPRt_ClosePortCmd,
        stream<CamSessionUpdateRequest> &soUrs_SessDeleteReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Rlt");

    //-- STATIC ARRAYS --------------------------------------------------------
    static SLcFourTuple             REVERSE_LOOKUP_TABLE[TOE_MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=REVERSE_LOOKUP_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=REVERSE_LOOKUP_TABLE inter false
    static ValBool                  TUPLE_VALID_TABLE[TOE_MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TUPLE_VALID_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                rlt_isInit=false;
    #pragma HLS reset variable=rlt_isInit
    static RtlSessId           rlt_counter=0;
    #pragma HLS reset variable=rlt_counter

    if (!rlt_isInit) {
        // The two tables must be cleared upon reset
        TUPLE_VALID_TABLE[rlt_counter] = false;
        if (rlt_counter < TOE_MAX_SESSIONS) {
            rlt_counter++;
        }
        else {
            rlt_isInit = true;
        }
    }
    else {
        // Normal operation
        if (!siLrh_ReverseLkpRsp.empty()) {
             // Update the two TABLEs
            SLcReverseLkp insert = siLrh_ReverseLkpRsp.read();
            REVERSE_LOOKUP_TABLE[insert.key] = insert.value;
            TUPLE_VALID_TABLE[insert.key]    = true;
        }
        else if (!siSTt_SessReleaseCmd.empty()) {
            // Release a session
            SessionId sessionId = siSTt_SessReleaseCmd.read();
            SLcFourTuple releaseTuple = REVERSE_LOOKUP_TABLE[sessionId];
            if (TUPLE_VALID_TABLE[sessionId]) { // if valid
                soPRt_ClosePortCmd.write(releaseTuple.myPort);
                soUrs_SessDeleteReq.write(CamSessionUpdateRequest(releaseTuple, sessionId, DELETE, FROM_RXe));
            }
            TUPLE_VALID_TABLE[sessionId] = false;
        }
        else if (!siTXe_ReverseLkpReq.empty()) {
            // Return 4-tuple corresponding to a given session Id
            SessionId sessionId = siTXe_ReverseLkpReq.read();
            soTXe_ReverseLkpRep.write(fourTuple(
                    REVERSE_LOOKUP_TABLE[sessionId].myIp,
                    REVERSE_LOOKUP_TABLE[sessionId].theirIp,
                    REVERSE_LOOKUP_TABLE[sessionId].myPort,
                    REVERSE_LOOKUP_TABLE[sessionId].theirPort));
        }
    }
}

/*****************************************************************************
 * @brief Session Lookup Controller (SLc)
 *
 * @param[in]  siRXe_SessLookupReq  Session lookup request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep  Session lookup reply to [RXe].
 * @param[in]  siSTt_SessReleaseCmd Session release command from State Table (STt).
 * @param[out] soPRt_ClosePortCmd   Command to close a port for the Port Table (PRt).
 * @param[in]  siTAi_SessLookupReq  Session lookup request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupReq  Session lookup reply to [TAi].
 * @param[in]  siTXe_ReverseLkpReq  Reverse lookup request from Tx Engine (TXe).
 * @param[out] soTXe_ReverseLkpRep  Reverse lookup reply to [TXe].
 * @param[out] soCAM_SessLookupReq  Lookup request to ternary CAM (CAM).
 * @param[in]  siCAM_SessLookupRep  Lookup reply from [CAM].
 * @param[out] soCAM_SessUpdateReq  Update request to [CAM].
 * @param[in]  siCAM_SessUpdateRep  Update reply from [CAM].
 * @param[out] soSssRelCnt          Session release count.
 * @param[out] soSssRegCnt          Session register count.
 *
 * @details
 *  The SLc maps a four-tuple information {{IP4_SA,TCP_SA},{IP4_DA,TCP_DP}} of
 *   a socket pair to a so-called 'sessionID'. This session ID represents the
 *   TCP connection and is used as an index into the various data structures of
 *   the TOE.
 *  This module acts as a wrapper for the RTL implementation the TCAM which
 *   holds the session ID table.
 *  It also includes the wrapper for the sessionID free list which keeps track
 *   of the free SessionID.
 *
 *****************************************************************************/
void session_lookup_controller(
        stream<SessionLookupQuery>         &siRXe_SessLookupReq,
        stream<SessionLookupReply>         &soRXe_SessLookupRep,
        stream<SessionId>                  &siSTt_SessReleaseCmd,
        stream<TcpPort>                    &soPRt_ClosePortCmd,
        stream<SocketPair>                 &siTAi_SessLookupReq,
        stream<SessionLookupReply>         &soTAi_SessLookupRep,
        stream<SessionId>                  &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &soTXe_ReverseLkpRep,
        stream<CamSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<CamSessionLookupReply>      &siCAM_SessLookupRep,
        stream<CamSessionUpdateRequest>    &soCAM_SessUpdateReq,
        stream<CamSessionUpdateReply>      &siCAM_SessUpdateRep,
        stream<ap_uint<16> >               &soSssRelCnt,
        stream<ap_uint<16> >               &soSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE

    //--------------------------------------------------------------------------
    //-- LOCAL SIGNALS AND STREAMS
    //--    (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    // Session Id Manager (Sim) ------------------------------------------------
    static stream<RtlSessId>               ssSimToLrh_FreeList      ("ssSimToLrh_FreeList");
    #pragma HLS stream            variable=ssSimToLrh_FreeList      depth=16384  // 0x4000 [FIXME - Can we replace 16384 with MAX_SESSIONS]

    // Lookup Reply Handler (Lrh) ----------------------------------------------
    static stream<RtlSessId>               ssUrsToSim_FreeId        ("ssUrsToSim_FreeId");
    #pragma HLS stream            variable=ssUrsToSim_FreeId        depth=2

    static stream<CamSessionUpdateRequest> ssLrhToUrs_InsertSessReq ("ssLrhToUrs_InsertSessReq");
    #pragma HLS STREAM            variable=ssLrhToUrs_InsertSessReq depth=4

    static stream<SLcReverseLkp>           ssLrhToRlt_ReverseLkpRsp ("ssLrhToRlt_ReverseLkpRsp");
    #pragma HLS STREAM            variable=ssLrhToRlt_ReverseLkpRsp depth=4

    // Update Reply Handler
    static stream<CamSessionUpdateReply>   ssUrhToLrh_SessUpdateRsp ("ssUrhToLrh_SessUpdateRsp");
    #pragma HLS STREAM            variable=ssUrhToLrh_SessUpdateRsp depth=4

    // Reverse Lookup Table (Rlt) ----------------------------------------------
    static stream<CamSessionUpdateRequest> ssRltToUrs_SessDeleteReq ("ssRltToUrs_SessDeleteReq");
    #pragma HLS STREAM            variable=ssRltToUrs_SessDeleteReq depth=4

    //--------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //--------------------------------------------------------------------------

    pSessionIdManager(
            ssUrsToSim_FreeId,
            ssSimToLrh_FreeList);

    pLookupReplyHandler(
            soCAM_SessLookupReq,
            siCAM_SessLookupRep,
            ssUrhToLrh_SessUpdateRsp,
            siRXe_SessLookupReq,
            soRXe_SessLookupRep,
            siTAi_SessLookupReq,
            soTAi_SessLookupRep,
            ssSimToLrh_FreeList,
            ssLrhToUrs_InsertSessReq,
            ssLrhToRlt_ReverseLkpRsp);

    pUpdateRequestSender(
            ssLrhToUrs_InsertSessReq,
            ssRltToUrs_SessDeleteReq,
            soCAM_SessUpdateReq,
            ssUrsToSim_FreeId,
            soSssRelCnt,
            soSssRegCnt);

    pUpdateReplyHandler(
            siCAM_SessUpdateRep,
            ssUrhToLrh_SessUpdateRsp);

    pReverseLookupTable(
            ssLrhToRlt_ReverseLkpRsp,
            siSTt_SessReleaseCmd,
            siTXe_ReverseLkpReq,
            soTXe_ReverseLkpRep,
            soPRt_ClosePortCmd,
            ssRltToUrs_SessDeleteReq);
}

/*! \} */
