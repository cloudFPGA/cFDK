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
 * @param[in]
 * @param[in]  siRXe_SessLookupReq, Request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep, Reply from CAM to RXe.
 * @param[in]  siTAi_SessLookupReq, Request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupRep, Reply from CAM to TAi.
 * @param[in]
 * @param[out] soUrs_InsertSessReq, Request to insert session to [UpdateRequestSender].
 * @param[out]
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
        stream<rtlSessionUpdateReply>       &sessionInsert_rsp,
        stream<sessionLookupQuery>          &siRXe_SessLookupReq,
        stream<sessionLookupReply>          &soRXe_SessLookupRep,
        stream<AxiSocketPair>               &siTAi_SessLookupReq,
        stream<sessionLookupReply>          &soTAi_SessLookupRep,
        stream<RtlSessId>                   &sessionIdFreeList,
        stream<rtlSessionUpdateRequest>     &soUrs_InsertSessReq,
        stream<revLupInsert>                &reverseTableInsertFifo)
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
    //ap_uint<16> sessionID;

    //OBSOLETE-20181124 enum slcFsmStateType {LUP_REQ, LUP_RSP, UPD_RSP};
    //OBSOLETE-20181124 static slcFsmStateType slc_fsmState = LUP_REQ;
    static enum FsmState {LUP_REQ=0, LUP_RSP, UPD_RSP} slc_fsmState = LUP_REQ;

    switch (slc_fsmState) {

    case LUP_REQ:
        if (!siTAi_SessLookupReq.empty()) {
        	AxiSocketPair sockPairTODO = siTAi_SessLookupReq.read();
            fourTuple toeTuple = fourTuple(sockPairTODO.src.addr, sockPairTODO.dst.addr,
                                           sockPairTODO.src.port, sockPairTODO.dst.port);
            sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(
                    toeTuple.srcIp,   toeTuple.dstIp,
                    toeTuple.srcPort, toeTuple.dstPort), true, TX_APP);
            soCAM_SessLookupReq.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
            sQueryCache.write(intQuery);
            slc_fsmState = LUP_RSP;
        }
        else if (!siRXe_SessLookupReq.empty()) {
            sessionLookupQuery query = siRXe_SessLookupReq.read();
            //OBSOLETE-20181120 sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(query.tuple.dstIp, query.tuple.srcIp, query.tuple.dstPort, query.tuple.srcPort), query.allowCreation, RX);
            sessionLookupQueryInternal intQuery = sessionLookupQueryInternal(fourTupleInternal(
                    query.tuple.dst.addr, query.tuple.src.addr,
                    query.tuple.dst.port, query.tuple.src.port),
                    query.allowCreation, RX);
            soCAM_SessLookupReq.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
            sQueryCache.write(intQuery);
            slc_fsmState = LUP_RSP;
        }
        break;

    case LUP_RSP:
        if(!siCAM_SessLookupRep.empty() && !sQueryCache.empty()) {
            rtlSessionLookupReply lupReply = siCAM_SessLookupRep.read();
            sessionLookupQueryInternal intQuery = sQueryCache.read();
            if (!lupReply.hit && intQuery.allowCreation && !sessionIdFreeList.empty()) {
                RtlSessId freeID = sessionIdFreeList.read();
                soUrs_InsertSessReq.write(rtlSessionUpdateRequest(intQuery.tuple, freeID, INSERT, lupReply.source));
                sInsertTuples.write(intQuery.tuple);
                slc_fsmState = UPD_RSP;
            }
            else {
                if (lupReply.source == RX)
                    soRXe_SessLookupRep.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
                else
                    soTAi_SessLookupRep.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
                slc_fsmState = LUP_REQ;
            }
        }
        break;

    case UPD_RSP:
        if (!sessionInsert_rsp.empty() && !sInsertTuples.empty()) {
            rtlSessionUpdateReply insertReply = sessionInsert_rsp.read();
            fourTupleInternal tuple = sInsertTuples.read();
            if (insertReply.source == RX)
                soRXe_SessLookupRep.write(sessionLookupReply(insertReply.sessionID, true));
            else
                soTAi_SessLookupRep.write(sessionLookupReply(insertReply.sessionID, true));
            reverseTableInsertFifo.write(revLupInsert(insertReply.sessionID, tuple));
            slc_fsmState = LUP_REQ;
        }
        break;
    }
}


/*****************************************************************************
 * @brief Update Request Sender (Urs).
 *
 * @param[in]  siLrh_InsertSessReq, Request to insert session from [LookupReplyHandler].
 * @
 * @
 * @param[out] soSim_FreeId, The SessId to recycle to the [SessionIdManager].
 * @param[out] poSssRelCnt,  Session release count to DEBUG.
 * @param[out] poSssRegCnt,  Session register count to DEBUG.
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void pUpdateRequestSender(
        stream<rtlSessionUpdateRequest>     &siLrh_InsertSessReq,
        stream<rtlSessionUpdateRequest>     &sessionDelete_req,
        stream<rtlSessionUpdateRequest>     &sessionUpdate_req,
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
        sessionUpdate_req.write(siLrh_InsertSessReq.read());
        usedSessionIDs++;
    }
    else if (!sessionDelete_req.empty()) {
        sessionDelete_req.read(request);
        sessionUpdate_req.write(request);
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
 *  @param[in]
 *  @param[out] soLrh_SessInsertRsp, Session insertion response to Lookup Reply Handler (Lrh).
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void pUpdateReplyHandler(
        stream<rtlSessionUpdateReply>   &sessionUpdate_rsp,
        stream<rtlSessionUpdateReply>   &soLrh_SessInsertRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    rtlSessionUpdateReply upReply;
    fourTupleInternal     tuple;

    if (!sessionUpdate_rsp.empty()) {
        sessionUpdate_rsp.read(upReply);
        if (upReply.op == INSERT)
        	soLrh_SessInsertRsp.write(upReply);
    }
}


/*****************************************************************************
 * @brief Reverse Lookup Table Interface (Rlti)...
 *
 *  @param[in]
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void reverseLookupTableInterface(
        stream<revLupInsert>    &revTableInserts,
        stream<ap_uint<16> >    &stateTable2sLookup_releaseSession,
        stream<ap_uint<16> >    &txEng2sLookup_rev_req,
        stream<ap_uint<16> >    &sLookup2portTable_releasePort,
        stream<rtlSessionUpdateRequest> &deleteCache,
        stream<fourTuple>       &sLookup2txEng_rev_rsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static fourTupleInternal reverseLookupTable[MAX_SESSIONS];
    #pragma HLS RESOURCE variable=reverseLookupTable core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=reverseLookupTable inter false
    static bool tupleValid[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=tupleValid inter false

    fourTuple           toeTuple;

    if (!revTableInserts.empty()) {
        revLupInsert        insert = revTableInserts.read();
        reverseLookupTable[insert.key] = insert.value;
        tupleValid[insert.key] = true;
    }
    else if (!stateTable2sLookup_releaseSession.empty()) { // TODO check if else if necessary
        ap_uint<16> sessionID = stateTable2sLookup_releaseSession.read();
        fourTupleInternal releaseTuple = reverseLookupTable[sessionID];
        if (tupleValid[sessionID]) { // if valid
            sLookup2portTable_releasePort.write(releaseTuple.myPort);
            deleteCache.write(rtlSessionUpdateRequest(releaseTuple, sessionID, DELETE, RX));
        }
        tupleValid[sessionID] = false;
    }
    else if (!txEng2sLookup_rev_req.empty()) {
        ap_uint<16> sessionID = txEng2sLookup_rev_req.read();
        sLookup2txEng_rev_rsp.write(fourTuple(reverseLookupTable[sessionID].myIp, reverseLookupTable[sessionID].theirIp, reverseLookupTable[sessionID].myPort, reverseLookupTable[sessionID].theirPort));
    }
}

/*****************************************************************************
 * @brief Main process of the Session Lookup Controller (SLc).
 *
 * @param[in]  siRXe_SessLookupReq, Session lookup request from Rx Engine (RXe).
 * @param[out] soRXe_SessLookupRep, Session lookup reply to RXe.
 * @param[in]
 * @param[in]
 * @param[in]  siTAi_SessLookupReq, Session lookup request from Tx App. I/F (TAi).
 * @param[out] soTAi_SessLookupReq, Session lookup reply to TAi.
 * @param[in]  siTXe_ReverseLkpReq, Reverse lookup request for Tx Engine (TXe).
 * @param[out]
 * @param[out]
 * @param[out] soCAM_SessLookupReq, Request to ternary CAM (CAM).
 * @param[in]  siCAM_SessLookupRep, Reply from ternary CAM.
 * @param[out]
 * @param[out]
 * @param[out]
 * @param[out] poSssRelCnt,         Session release count.
 * @param[out] poSssRegCnt,         Session register count.
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
        stream<ap_uint<16> >               &stateTable2sLookup_releaseSession,
        stream<ap_uint<16> >               &sLookup2portTable_releasePort,
        stream<AxiSocketPair>              &siTAi_SessLookupReq,
        stream<sessionLookupReply>         &soTAi_SessLookupRep,
        stream<ap_uint<16> >               &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &sLookup2txEng_rev_rsp,
        stream<rtlSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<rtlSessionLookupReply>      &siCAM_SessLookupRep,
        stream<rtlSessionUpdateRequest>    &sessionUpdate_req,
        //OBSOLETE-20190128 stream<rtlSessionUpdateRequest>  &sessionInsert_req,
        //OBSOLETE-20190128 stream<rtlSessionUpdateRequest>  &sessionDelete_req,
        stream<rtlSessionUpdateReply>      &sessionUpdate_rsp,
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


    // Update Reply Handler
    static stream<rtlSessionUpdateReply> slc_sessionInsert_rsp("slc_sessionInsert_rsp");
    #pragma HLS STREAM          variable=slc_sessionInsert_rsp depth=4

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    static stream<sessionLookupQueryInternal> slc_lookups("slc_lookups");
    #pragma HLS stream               variable=slc_lookups depth=4
    #pragma HLS DATA_PACK            variable=slc_lookups











    static stream<rtlSessionUpdateRequest>    sessionDelete_req("sessionDelete_req");
    #pragma HLS STREAM               variable=sessionDelete_req depth=4

    static stream<revLupInsert>               reverseLupInsertFifo("reverseLupInsertFifo");
    #pragma HLS STREAM               variable=reverseLupInsertFifo depth=4

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pSessionIdManager(
            sUrsToSim_FreeId,
            sSimToLrh_FreeList);

    pLookupReplyHandler(
            soCAM_SessLookupReq,
            siCAM_SessLookupRep,
            slc_sessionInsert_rsp,
            siRXe_SessLookupReq,
            soRXe_SessLookupRep,
            siTAi_SessLookupReq,
            soTAi_SessLookupRep,
            sSimToLrh_FreeList,
            sLrhToUrs_InsertSessReq,
            reverseLupInsertFifo);
            //poSssRegCnt);

    pUpdateRequestSender(
            sLrhToUrs_InsertSessReq,
            sessionDelete_req,
            sessionUpdate_req,
            sUrsToSim_FreeId,
            poSssRelCnt,
            poSssRegCnt);

    pUpdateReplyHandler(
            sessionUpdate_rsp,
            slc_sessionInsert_rsp);

    reverseLookupTableInterface(
            reverseLupInsertFifo,
            stateTable2sLookup_releaseSession,
            siTXe_ReverseLkpReq,
            sLookup2portTable_releasePort,
            sessionDelete_req,
            sLookup2txEng_rev_rsp);
}
