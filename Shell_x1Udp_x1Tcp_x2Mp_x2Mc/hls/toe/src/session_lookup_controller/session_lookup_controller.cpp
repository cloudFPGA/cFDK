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
 * @param[in]  fin_id, IDs that are released and appended to the SessionID free list
 * @param[out] new_id, get a new SessionID from the SessionID free list
 *
 *
 * @details
 *
 *
 * @ingroup session_lookup_controller
 *****************************************************************************/
void sessionIdManager(
        stream<ap_uint<14> >    &new_id,
        stream<ap_uint<14> >    &fin_id)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static ap_uint<14> counter = 0;

    if (!fin_id.empty()) {
        new_id.write(fin_id.read());
    }
    else if (counter < MAX_SESSIONS) {
        new_id.write(counter);
        counter++;
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
 * @param[out]
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
        stream<fourTuple>                   &siTAi_SessLookupReq,
        stream<sessionLookupReply>          &soTAi_SessLookupRep,
        stream<ap_uint<14> >                &sessionIdFreeList,
        stream<rtlSessionUpdateRequest>     &sessionInsert_req,
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
            fourTuple toeTuple = siTAi_SessLookupReq.read();
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
                ap_uint<14> freeID = sessionIdFreeList.read();
                sessionInsert_req.write(rtlSessionUpdateRequest(intQuery.tuple, freeID, INSERT, lupReply.source));
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
 * @brief Update Request Sender (Urs)...
 *
 *  @param[in]
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void updateRequestSender(
        stream<rtlSessionUpdateRequest>     &sessionInsert_req,
        stream<rtlSessionUpdateRequest>     &sessionDelete_req,
        stream<rtlSessionUpdateRequest>     &sessionUpdate_req,
        stream<ap_uint<14> >                &sessionIdFinFifo,
        ap_uint<16>                         &poSssRelCnt,
        ap_uint<16>                         &poSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static ap_uint<16> usedSessionIDs = 0;
    static ap_uint<16> releasedSessionIDs = 0;

    if (!sessionInsert_req.empty()) {
        sessionUpdate_req.write(sessionInsert_req.read());
        usedSessionIDs++;
        poSssRegCnt = usedSessionIDs;
    }
    else if (!sessionDelete_req.empty()) {
        rtlSessionUpdateRequest request = sessionDelete_req.read();
        sessionUpdate_req.write(request);
        sessionIdFinFifo.write(request.value);
        //usedSessionIDs--;
        releasedSessionIDs++;
        poSssRelCnt = releasedSessionIDs;
        //poSssRegCnt = usedSessionIDs;
    }
}


/*****************************************************************************
 * @brief Update ReplyHandler (Urh)...
 *
 *  @param[in]
 *
 * @details
 *  TODO...
 *
 * @ingroup session_lookup_controller
 *
 *****************************************************************************/
void updateReplyHandler(    stream<rtlSessionUpdateReply>&          sessionUpdate_rsp,
                            stream<rtlSessionUpdateReply>&          sessionInsert_rsp) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    rtlSessionUpdateReply upReply;
    fourTupleInternal tuple;

    if (!sessionUpdate_rsp.empty()) {
        sessionUpdate_rsp.read(upReply);
        if (upReply.op == INSERT)
            sessionInsert_rsp.write(upReply);
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
 *
 * -- DEBUG / Session Statistics Interfaces
 * @param[out] poSssRelCnt,	Session release count.
 * @param[out] poSssRegCnt,	Session register count.]
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
        stream<fourTuple>                  &siTAi_SessLookupReq,
        stream<sessionLookupReply>         &soTAi_SessLookupRep,
        stream<ap_uint<16> >               &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &sLookup2txEng_rev_rsp,
        stream<rtlSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<rtlSessionLookupReply>      &siCAM_SessLookupRep,
        stream<rtlSessionUpdateRequest>    &sessionUpdate_req,
        //stream<rtlSessionUpdateRequest>  &sessionInsert_req,
        //stream<rtlSessionUpdateRequest>  &sessionDelete_req,
        stream<rtlSessionUpdateReply>      &sessionUpdate_rsp,
        ap_uint<16>                        &poSssRelCnt,
        ap_uint<16>                        &poSssRegCnt)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE    //OBSOLETE  #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------


    static stream<sessionLookupQueryInternal> slc_lookups("slc_lookups");
    #pragma HLS stream               variable=slc_lookups depth=4
    #pragma HLS DATA_PACK            variable=slc_lookups

    static stream<ap_uint<14> >               slc_sessionIdFreeList("slc_sessionIdFreeList");
    static stream<ap_uint<14> >               slc_sessionIdFinFifo("slc_sessionIdFinFifo");
    #pragma HLS stream variable=              slc_sessionIdFreeList depth=16384
    #pragma HLS stream variable=              slc_sessionIdFinFifo depth=2

    static stream<rtlSessionUpdateReply>      slc_sessionInsert_rsp("slc_sessionInsert_rsp");
    #pragma HLS STREAM               variable=slc_sessionInsert_rsp depth=4

    static stream<rtlSessionUpdateRequest>    sessionInsert_req("sessionInsert_req");
    #pragma HLS STREAM               variable=sessionInsert_req depth=4

    static stream<rtlSessionUpdateRequest>    sessionDelete_req("sessionDelete_req");
    #pragma HLS STREAM               variable=sessionDelete_req depth=4

    static stream<revLupInsert>               reverseLupInsertFifo("reverseLupInsertFifo");
    #pragma HLS STREAM               variable=reverseLupInsertFifo depth=4

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    sessionIdManager(
            slc_sessionIdFreeList,
            slc_sessionIdFinFifo);

    pLookupReplyHandler(
            soCAM_SessLookupReq,
            siCAM_SessLookupRep,
            slc_sessionInsert_rsp,
            siRXe_SessLookupReq,
            soRXe_SessLookupRep,
            siTAi_SessLookupReq,
            soTAi_SessLookupRep,
            slc_sessionIdFreeList,
            sessionInsert_req,
            reverseLupInsertFifo);
            //poSssRegCnt);

    updateRequestSender(sessionInsert_req,
            sessionDelete_req,
            sessionUpdate_req,
            slc_sessionIdFinFifo,
            poSssRelCnt,
            poSssRegCnt);

    updateReplyHandler(
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
