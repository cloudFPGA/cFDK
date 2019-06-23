/*****************************************************************************
 * @file       : session_lookup_controller.cpp
 * @brief      : Session Lookup Controller (SLc) of TCP Offload Engine (TOE)
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
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

#include "session_lookup_controller.hpp"

#define THIS_NAME "TOE/SLc"

using namespace hls;

#define DEBUG_LEVEL 2


/*****************************************************************************
 * @brief Session Id Manager (Sim)
 *
 * @param[in]  siUrs_FreeId,   The session ID to recycle from [UpdateRequestSender].
 * @param[out] soLrh_FreeList, The free list of session IDs to [LookupReplyHandler].
 *
 * @details
 *  Implements the free list of session IDs as a FiFo stream.
 *
 * @ingroup session_lookup_controller
 *****************************************************************************/
void pSessionIdManager(
        stream<RtlSessId>    &siUrs_FreeId,
        stream<RtlSessId>    &soLrh_FreeList)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static RtlSessId           counter = 0;
    #pragma HLS reset variable=counter

    RtlSessId rtlSessId;   // [FIXME - Why is sessionId 14 bits and not 16?]

    if (counter < MAX_SESSIONS) {
        // Initialize the free list after a reset
        soLrh_FreeList.write(counter);
        counter++;
    }
    else if (!siUrs_FreeId.empty()) {
        // Recycle the incoming session ID
        siUrs_FreeId.read(rtlSessId);
        soLrh_FreeList.write(rtlSessId);
    }
}


/*****************************************************************************
 * @brief The Lookup Reply Handler (Lrh) is the front-end process of the
 *  ternary content addressable memory (TCAM or CAM for short).
 *
 * @param[out] soCAM_SessLookupReq, Request to ternary CAM (CAM).
 * @param[in]  siCAM_SessLookupRep, Reply from ternary CAM.
 * @param[in]  siUrh_SessUpdateRsp, Update response from Update Reply Handler (Urh).
 * @param[in]  siRXe_SessLookupReq, Request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep, Reply from CAM to RXe.
 * @param[in]  siTAi_SessLookupReq, Request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupRep, Reply from CAM to TAi.
 * @param[in]  siSim_FreeList,      Free liste from Session Id Manager (Sid).
 * @param[out] soUrs_InsertSessReq, Request to insert session to Update Request Sender (Urs).
 * @param[out] soRlt_ReverseLkpRsp, Reverse lookup response to Reverse Lookup Table (Rlt).
 *
 * @details
 *  Handles the session lookup replies from the ternary content addressable
 *   memory (TCAM or CAM for short).
 *  If there was no hit, it checks if the request is allowed to create a new
 *   sessionID and does so. If there is a hit, the reply is forwarded to the
 *   corresponding source.
 *  It also handles the replies of the Session Updates [Inserts/Deletes], in
 *   case of insert the response with the new sessionID is replied to the
 *   request source.
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void pLookupReplyHandler(
        stream<rtlSessionLookupRequest>     &soCAM_SessLookupReq,
        stream<rtlSessionLookupReply>       &siCAM_SessLookupRep,
        stream<rtlSessionUpdateReply>       &siUrh_SessUpdateRsp,
        stream<sessionLookupQuery>          &siRXe_SessLookupReq,
        stream<sessionLookupReply>          &soRXe_SessLookupRep,
        stream<AxiSocketPair>               &siTAi_SessLookupReq,
        stream<sessionLookupReply>          &soTAi_SessLookupRep,
        stream<RtlSessId>                   &siSim_FreeList,
        stream<rtlSessionUpdateRequest>     &soUrs_InsertSessReq,
        stream<revLupInsert>                &soRlt_ReverseLkpRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    //-- LOCAL STREAMS  -------------------------------------------------------
    static stream<fourTupleInternal>          sInsertTuples("sInsertTuples");
    #pragma HLS STREAM               variable=sInsertTuples depth=4

    static stream<sessionLookupQueryInternal> sQueryCache ("sQueryCache");
    #pragma HLS STREAM               variable=sQueryCache depth=8

    sessionLookupQueryInternal intQuery;

    static enum LrhFsmStates {LRH_LKP_REQ=0, LRH_LKP_RSP, LRH_UPD_RSP} lrhFsmState = LRH_LKP_REQ;

    switch (lrhFsmState) {

    case LRH_LKP_REQ:
        if (!siTAi_SessLookupReq.empty()) {
            AxiSocketPair axiSockPair = siTAi_SessLookupReq.read();
            fourTuple toeTuple = fourTuple(axiSockPair.src.addr, axiSockPair.dst.addr,
                                           axiSockPair.src.port, axiSockPair.dst.port);
            sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(
                    toeTuple.srcIp,   toeTuple.dstIp,
                    toeTuple.srcPort, toeTuple.dstPort), true, FROM_TAi);
            soCAM_SessLookupReq.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
            sQueryCache.write(intQuery);
            lrhFsmState = LRH_LKP_RSP;
        }
        else if (!siRXe_SessLookupReq.empty()) {
            sessionLookupQuery query = siRXe_SessLookupReq.read();
            /*** OBSOLETE-20181120
            sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(
                    query.tuple.dstIp,    query.tuple.srcIp,
                    query.tuple.dstPort,  query.tuple.srcPort),
                    query.allowCreation, RX); ***/
            sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(
                    query.tuple.dst.addr, query.tuple.src.addr,
                    query.tuple.dst.port, query.tuple.src.port),
                    query.allowCreation, FROM_RXe);
            soCAM_SessLookupReq.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
            sQueryCache.write(intQuery);
            lrhFsmState = LRH_LKP_RSP;
        }
        break;

    case LRH_LKP_RSP:
        if(!siCAM_SessLookupRep.empty() && !sQueryCache.empty()) {
            rtlSessionLookupReply lupReply = siCAM_SessLookupRep.read();
            sessionLookupQueryInternal intQuery = sQueryCache.read();
            if (!lupReply.hit && intQuery.allowCreation && !siSim_FreeList.empty()) {
                RtlSessId freeID = siSim_FreeList.read();
                soUrs_InsertSessReq.write(rtlSessionUpdateRequest(intQuery.tuple, freeID, INSERT, lupReply.source));
                sInsertTuples.write(intQuery.tuple);
                lrhFsmState = LRH_UPD_RSP;
            }
            else {
                if (lupReply.source == FROM_RXe)
                    soRXe_SessLookupRep.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
                else
                    soTAi_SessLookupRep.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
                lrhFsmState = LRH_LKP_REQ;
            }
        }
        break;

    case LRH_UPD_RSP:
        if (!siUrh_SessUpdateRsp.empty() && !sInsertTuples.empty()) {
            rtlSessionUpdateReply insertReply = siUrh_SessUpdateRsp.read();
            fourTupleInternal tuple = sInsertTuples.read();
            if (insertReply.source == FROM_RXe)
                soRXe_SessLookupRep.write(sessionLookupReply(insertReply.sessionID, true));
            else
                soTAi_SessLookupRep.write(sessionLookupReply(insertReply.sessionID, true));
            soRlt_ReverseLkpRsp.write(revLupInsert(insertReply.sessionID, tuple));
            lrhFsmState = LRH_LKP_REQ;
        }
        break;
    }
}


/*****************************************************************************
 * @brief Update Request Sender (Urs).
 *
 * @param[in]  siLrh_InsertSessReq, Request to insert session from LookupReplyHandler (Lrh).
 * @param[in]  siRlt_SessDeleteReq, Request to delete session from Reverse Lookup Table (Rlt).
 * @param[out] soCAM_SessUpdateReq, Update request to [CAM].
 * @param[out] soSim_FreeId,        The SessId to recycle to the [SessionIdManager].
 * @param[out] poSssRelCnt,         Session release count to DEBUG.
 * @param[out] poSssRegCnt,         Session register count to DEBUG.
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void pUpdateRequestSender(
        stream<rtlSessionUpdateRequest>     &siLrh_InsertSessReq,
        stream<rtlSessionUpdateRequest>     &siRlt_SessDeleteReq,
        stream<rtlSessionUpdateRequest>     &soCAM_SessUpdateReq,
        stream<RtlSessId>                   &soSim_FreeId,
        ap_uint<16>                         &poSssRelCnt,
        ap_uint<16>                         &poSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static ap_uint<16> usedSessionIDs     = 0;
    static ap_uint<16> releasedSessionIDs = 0;

    rtlSessionUpdateRequest request;

    if (!siLrh_InsertSessReq.empty()) {
        soCAM_SessUpdateReq.write(siLrh_InsertSessReq.read());
        usedSessionIDs++;
    }
    else if (!siRlt_SessDeleteReq.empty()) {
        siRlt_SessDeleteReq.read(request);
        soCAM_SessUpdateReq.write(request);
        soSim_FreeId.write(request.value);
        usedSessionIDs--;
        releasedSessionIDs++;
    }
    // Always
    poSssRegCnt = usedSessionIDs;      // [FIXME-Replace Reg by Used]
    poSssRelCnt = releasedSessionIDs;  // [FIXME-Remove this counter]
}


/*****************************************************************************
 * @brief Update Reply Handler (Urh)...
 *
 *  @param[in]  siCAM_SessUpdateRep, Session update reply from [CAM].
 *  @param[out] soLrh_SessUpdateRsp, Session update response to Lookup Reply Handler (Lrh).
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void pUpdateReplyHandler(
        stream<rtlSessionUpdateReply>   &siCAM_SessUpdateRep,
        stream<rtlSessionUpdateReply>   &soLrh_SessUpdateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    rtlSessionUpdateReply upReply;
    fourTupleInternal     tuple;

    if (!siCAM_SessUpdateRep.empty()) {
        siCAM_SessUpdateRep.read(upReply);
        if (upReply.op == INSERT)
           soLrh_SessUpdateRsp.write(upReply);
    }
}


/*****************************************************************************
 * @brief Reverse Lookup Table (Rlt)
 *
 *  @param[in]  sLrh_ReverseLkpRsp,   Reverse lookup response from Lookup Reply HAndler (Lrh).
 *  @param[in]  siSTt_SessReleaseCmd, Session release command from State Table (STt).
 *  @param[in[  siTXe_ReverseLkpReq,  Reverse lookup request from Tx Engine (TXe).
 *  @param[out] soTXe_ReverseLkpRep,  Reverse lookup reply to Tx Engine (TXe).
 *  @param[out] soPRt_ClosePortCmd,   Command to close a port for the Port Table (PRt).
 *  @param[out] soUrs_SessDeleteReq,  Request to delete session to Update Request Sender (Urs).
 *
 *****************************************************************************/
void pReverseLookupTable(
        stream<revLupInsert>    &siLrh_ReverseLkpRsp,
        stream<SessionId>       &siSTt_SessReleaseCmd,
        stream<SessionId>       &siTXe_ReverseLkpReq,
        stream<fourTuple>       &soTXe_ReverseLkpRep,
        stream<TcpPort>         &soPRt_ClosePortCmd,
        stream<rtlSessionUpdateRequest> &soUrs_SessDeleteReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static fourTupleInternal        REVERSE_LOOKUP_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=REVERSE_LOOKUP_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=REVERSE_LOOKUP_TABLE inter false

    static bool                     TUPLE_VALID_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TUPLE_VALID_TABLE inter false

    fourTuple           toeTuple;

    if (!siLrh_ReverseLkpRsp.empty()) {
        revLupInsert        insert = siLrh_ReverseLkpRsp.read();
        REVERSE_LOOKUP_TABLE[insert.key] = insert.value;
        TUPLE_VALID_TABLE[insert.key] = true;
    }
    else if (!siSTt_SessReleaseCmd.empty()) { // TODO check if else if necessary
        SessionId sessionId = siSTt_SessReleaseCmd.read();
        fourTupleInternal releaseTuple = REVERSE_LOOKUP_TABLE[sessionId];
        if (TUPLE_VALID_TABLE[sessionId]) { // if valid
            soPRt_ClosePortCmd.write(releaseTuple.myPort);
            soUrs_SessDeleteReq.write(rtlSessionUpdateRequest(releaseTuple, sessionId, DELETE, FROM_RXe));
        }
        TUPLE_VALID_TABLE[sessionId] = false;
    }
    else if (!siTXe_ReverseLkpReq.empty()) {
        SessionId sessionId = siTXe_ReverseLkpReq.read();
        soTXe_ReverseLkpRep.write(fourTuple(REVERSE_LOOKUP_TABLE[sessionId].myIp, REVERSE_LOOKUP_TABLE[sessionId].theirIp, REVERSE_LOOKUP_TABLE[sessionId].myPort, REVERSE_LOOKUP_TABLE[sessionId].theirPort));
    }
}

/*****************************************************************************
 * @brief Main process of the Session Lookup Controller (SLc).
 *
 * @param[in]  siRXe_SessLookupReq,  Session lookup request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep,  Session lookup reply to [RXe].
 * @param[in]  siSTt_SessReleaseCmd, Session release command from State Table (STt).
 * @param[out] soPRt_ClosePortCmd,   Command to close a port for the Port Table (PRt).
 * @param[in]  siTAi_SessLookupReq,  Session lookup request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupReq,  Session lookup reply to [TAi].
 * @param[in]  siTXe_ReverseLkpReq,  Reverse lookup request from Tx Engine (TXe).
 * @param[out] soTXe_ReverseLkpRep,  Reverse lookup reply to [TXe].
 * @param[out] soCAM_SessLookupReq,  Lookup request to ternary CAM (CAM).
 * @param[in]  siCAM_SessLookupRep,  Lookup reply from [CAM].
 * @param[out] soCAM_SessUpdateReq,  Update request to [CAM].
 * @param[in]  siCAM_SessUpdateRep,  Update reply from [CAM].
 * @param[out]
 * @param[out] poSssRelCnt,          Session release count.
 * @param[out] poSssRegCnt,          Session register count.
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
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void session_lookup_controller(
        stream<sessionLookupQuery>         &siRXe_SessLookupReq,
        stream<sessionLookupReply>         &soRXe_SessLookupRep,
        stream<SessionId>                  &siSTt_SessReleaseCmd,
        stream<TcpPort>                    &soPRt_ClosePortCmd,
        stream<AxiSocketPair>              &siTAi_SessLookupReq,
        stream<sessionLookupReply>         &soTAi_SessLookupRep,
        stream<SessionId>                  &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &soTXe_ReverseLkpRep,
        stream<rtlSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<rtlSessionLookupReply>      &siCAM_SessLookupRep,
        stream<rtlSessionUpdateRequest>    &soCAM_SessUpdateReq,
        stream<rtlSessionUpdateReply>      &siCAM_SessUpdateRep,
        ap_uint<16>                        &poSssRelCnt,
        ap_uint<16>                        &poSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE    //OBSOLETE  #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    // Session Id Manager (Sim) -----------------------------------------------
    static stream<RtlSessId>               sSimToLrh_FreeList      ("sSimToLrh_FreeList");
    #pragma HLS stream            variable=sSimToLrh_FreeList      depth=16384  // 0x4000 [FIXME - Can we replace 16384 with MAX_SESSIONS]

    // Lookup Reply Handler (Lrh) ---------------------------------------------
    static stream<RtlSessId>               sUrsToSim_FreeId        ("sUrsToSim_FreeId");
    #pragma HLS stream            variable=sUrsToSim_FreeId        depth=2

    static stream<rtlSessionUpdateRequest> sLrhToUrs_InsertSessReq ("sLrhToUrs_InsertSessReq");
    #pragma HLS STREAM            variable=sLrhToUrs_InsertSessReq depth=4

    static stream<revLupInsert>            sLrhToRlt_ReverseLkpRsp ("sLrhToRlt_ReverseLkpRsp");
    #pragma HLS STREAM            variable=sLrhToRlt_ReverseLkpRsp depth=4

    // Update Reply Handler
    static stream<rtlSessionUpdateReply>   sUrhToLrh_SessUpdateRsp ("sUrhToLrh_SessUpdateRsp");
    #pragma HLS STREAM            variable=sUrhToLrh_SessUpdateRsp depth=4

    // Reverse Lookup Table (Rlt) ---------------------------------------------
    static stream<rtlSessionUpdateRequest> sRltToUrs_SessDeleteReq ("sRltToUrs_SessDeleteReq");
    #pragma HLS STREAM            variable=sRltToUrs_SessDeleteReq depth=4

    //OBSOLETE-20190512 static stream<sessionLookupQueryInternal> slc_lookups("slc_lookups");
    //OBSOLETE-20190512 #pragma HLS stream               variable=slc_lookups depth=4
    //OBSOLETE-20190512 #pragma HLS DATA_PACK            variable=slc_lookups

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pSessionIdManager(
            sUrsToSim_FreeId,
            sSimToLrh_FreeList);

    pLookupReplyHandler(
            soCAM_SessLookupReq,
            siCAM_SessLookupRep,
            sUrhToLrh_SessUpdateRsp,
            siRXe_SessLookupReq,
            soRXe_SessLookupRep,
            siTAi_SessLookupReq,
            soTAi_SessLookupRep,
            sSimToLrh_FreeList,
            sLrhToUrs_InsertSessReq,
            sLrhToRlt_ReverseLkpRsp);
            //poSssRegCnt);

    pUpdateRequestSender(
            sLrhToUrs_InsertSessReq,
            sRltToUrs_SessDeleteReq,
            soCAM_SessUpdateReq,
            sUrsToSim_FreeId,
            poSssRelCnt,
            poSssRegCnt);

    pUpdateReplyHandler(
            siCAM_SessUpdateRep,
            sUrhToLrh_SessUpdateRsp);

    pReverseLookupTable(
            sLrhToRlt_ReverseLkpRsp,
            siSTt_SessReleaseCmd,
            siTXe_ReverseLkpReq,
            soTXe_ReverseLkpRep,
            soPRt_ClosePortCmd,
            sRltToUrs_SessDeleteReq);
}
