#include "session_lookup_controller.hpp"

using namespace hls;

/** @ingroup session_lookup_controller
 *  SessionID manager
 *  @param[in]      fin_id, IDs that are released and appended to the SessionID free list
 *  @param[out]     new_id, get a new SessionID from the SessionID free list
 */
void sessionIdManager(  stream<ap_uint<14> >&       new_id,
                        stream<ap_uint<14> >&       fin_id)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    static ap_uint<14> counter = 0;
    ap_uint<14> sessionID;

    if (!fin_id.empty())
    {
        fin_id.read(sessionID);
        new_id.write(sessionID);
    }
    else if (counter < 10000)
    {
        new_id.write(counter);
        counter++;
    }

}

/** @ingroup session_lookup_controller
 *  Merges the lookups from the @ref rx_engine and the @tx_app_if and forwards them to the RTL
 *  Session Lookup Table
 *  @param[in]      rxLookupIn
 *  @param[in]      txAppLookupIn
 *  @param[out]     rtlLookupIn
 *  @param[out]     lookups
 */
/*void lookupRequestMerger( stream<sessionLookupQuery>&             rxLookupIn,
                    stream<fourTuple>&                      txAppLookupIn,
                    stream<rtlSessionLookupRequest>&        rtlLookupIn,
                    stream<sessionLookupQueryInternal>&     lookups)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE off

    fourTuple hlsTuple;
    fourTupleInternal rtlTuple;
    sessionLookupQuery query;

    if (!txAppLookupIn.empty())
    {
        txAppLookupIn.read(hlsTuple);
        rtlTuple.theirIp = hlsTuple.dstIp;
        rtlTuple.theirPort = hlsTuple.dstPort;
        rtlTuple.myIp = hlsTuple.srcIp;
        rtlTuple.myPort = hlsTuple.srcPort;
        rtlLookupIn.write(rtlSessionLookupRequest(rtlTuple, TX_APP));
        lookups.write(sessionLookupQueryInternal(rtlTuple, true)); //FIXME change this
    }
    else if (!rxLookupIn.empty())
    {
        rxLookupIn.read(query);
        rtlTuple.theirIp = query.tuple.srcIp;
        rtlTuple.theirPort = query.tuple.srcPort;
        rtlTuple.myIp = query.tuple.dstIp;
        rtlTuple.myPort = query.tuple.dstPort;
        rtlLookupIn.write(rtlSessionLookupRequest(rtlTuple, RX));
        lookups.write(sessionLookupQueryInternal(rtlTuple, query.allowCreation));
    }
}*/

/** @ingroup session_lookup_controller
 *  Handles the Lookup relies from the RTL Lookup Table, if there was no hit,
 *  it checks if the request is allowed to create a new sessionID and does so.
 *  If it is a hit, the reply is forwarded to the corresponding source.
 *  It also handles the replies of the Session Updates [Inserts/Deletes], in case
 *  of insert the response with the new sessionID is replied to the request source.
 *  @param[in]      txEng2sLookup_rev_req
 *  @param[in]      sessionLookup_rsp
 *  @param[in]      sessionUpdatea_rsp
 *  @param[in]      stateTable2sLookup_releaseSession
 *  @param[in]      lookups
 *  @param[out]     sLookup2rxEng_rsp
 *  @param[out]     sLookup2txApp_rsp
 *  @param[out]     sLookup2txEng_rev_rsp
 *  @param[out]     sessionLookup_req
 *  @param[out]     sLookup2portTable_releasePort
 *  @TODO document more
 */
void lookupReplyHandler(//stream<ap_uint<16> >&                 txEng2sLookup_rev_req,
                        stream<rtlSessionLookupReply>&          sessionLookup_rsp,
                        //stream<rtlSessionUpdateReply>&            sessionUpdate_rsp,
                        //stream<ap_uint<16> >&                 stateTable2sLookup_releaseSession,
                        //stream<rtlSessionUpdateRequest>&      deleteCache,
                        stream<sessionLookupQueryInternal>&     lookups,
                        stream<ap_uint<14> >&                   sessionIdFreeList,
                        //stream<sessionLookupReply>&               sLookup2rxEng_rsp,
                        stream<sessionLookupReply>&             lookupReplies,
                        stream<slupRouting>&                    routingInfo,
                        stream<fourTupleInternal>&              insertTuples,
                        stream<rtlSessionUpdateRequest>&        insertCache)
                        //stream<ap_uint<16> >&                 sLookup2portTable_releasePort,
                        //stream<ap_uint<14> >&                 sessionIdFinFifo,
                        //ap_uint<16>& regSessionCount)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    /*static fourTupleInternal reverseLookupTable[10000];
    #pragma HLS RESOURCE variable=reverseLookupTable core=RAM_T2P_BRAM
    #pragma HLS DATA_PACK variable=reverseLookupTable
#pragma HLS DEPENDENCE variable=reverseLookupTable inter false
*/

    //static ap_uint<16> usedSessionIDs = 0;
    //static stream<fourTupleInternal> inserts;

    //fourTupleInternal tuple;
    sessionLookupQueryInternal query;
    //fourTuple toeTuple;
    rtlSessionLookupReply lupReply;
    ap_uint<16> sessionID;
    //ap_uint<16> releaseID;
    ap_uint<14> freeID;

    if (!sessionLookup_rsp.empty() && !lookups.empty())
    {
        sessionLookup_rsp.read(lupReply);
        lookups.read(query);
        //if (!lupReply.hit && query.creationAllowed && (usedSessionIDs < 10000))
        if (!lupReply.hit && query.creationAllowed && !sessionIdFreeList.empty())
        {
            //RX just get one
            //updateIn.write(rtlSessionUpdateRequest(query.tuple, lupReply.source));
            sessionIdFreeList.read(freeID);
            insertCache.write(rtlSessionUpdateRequest(query.tuple, freeID, INSERT, lupReply.source));
            insertTuples.write(query.tuple);
            //usedSessionIDs++;
            //regSessionCount = usedSessionIDs;

            routingInfo.write(slupRouting(true, lupReply.source));
        }
        else
        {
            lookupReplies.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
            routingInfo.write(slupRouting(false, lupReply.source));
            /*if (lupReply.source == RX)
            {
                sLookup2rxEng_rsp.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
            }
            else
            {
                sLookup2txApp_rsp.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
            }*/
        }
    }
    /*else if (!deleteCache.empty())
    {
        sessionUpdate_req.write(deleteCache.read());
        insertTuples.write(fourTupleInternal());
        //lrh_rtlUpdates.write(tuple);
        usedSessionIDs--;
        //regSessionCount = usedSessionIDs;
    }*/
}

void updateRequestSender(stream<rtlSessionUpdateRequest>&       insertCache,
                    stream<rtlSessionUpdateRequest>&        deleteCache,
                    stream<rtlSessionUpdateRequest>&        sessionUpdate_req)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off
    static ap_uint<16> usedSessionIDs = 0;

    if (!insertCache.empty())
    {
        sessionUpdate_req.write(insertCache.read());
        usedSessionIDs++;
    }
    else if (!deleteCache.empty())
    {
        sessionUpdate_req.write(deleteCache.read());
        //insertTuples.write(fourTupleInternal());
        //lrh_rtlUpdates.write(tuple);
        usedSessionIDs--;
        //regSessionCount = usedSessionIDs;
    }
}


void updateReplyHandler(    stream<rtlSessionUpdateReply>&          sessionUpdate_rsp,
                            stream<fourTupleInternal>&              insertTuples,
                            stream<sessionLookupReply>&             updateReplies,
                            stream<revLupInsert>&                   reverseTableInsertFifo,
                            stream<ap_uint<14> >&                   sessionIdFinFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    static bool flip = false;
    rtlSessionUpdateReply upReply;
    fourTupleInternal tuple;

    if (!insertTuples.empty() && !flip)
    {
        insertTuples.read(tuple);
        reverseTableInsertFifo.write(revLupInsert(0, tuple));
        flip = true;
    }

    else if (!sessionUpdate_rsp.empty() && flip)// && !insertTuples.empty())
    {
        sessionUpdate_rsp.read(upReply);
        //insertTuples.read(tuple);
        if (upReply.op == INSERT)
        {
            //insertTuples.read(tuple);
            updateReplies.write(sessionLookupReply(upReply.sessionID, true));
            //reverseTableInsertFifo.write(revLupInsert(upReply.sessionID, tuple));
        }
        else // DELETE
        {
            sessionIdFinFifo.write(upReply.sessionID);
        }
        flip = false;
    }
}

void lookupResponseMerger(  stream<slupRouting>&            routingInfo,
                            stream<sessionLookupReply>&     lookupReplies,
                            stream<sessionLookupReply>&     updateReplies,
                            stream<sessionLookupReply>&     sLookup2rxEng_rsp,
                            stream<sessionLookupReply>&     sLookup2txApp_rsp)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE off

    static bool getRouting = true;
    static slupRouting routing;

    if (getRouting)
    {
        if (!routingInfo.empty())
        {
            routingInfo.read(routing);
            getRouting = false;
        }
    }
    else if (routing.isUpdate && !updateReplies.empty())
    {
        if (routing.source == RX)
        {
            sLookup2rxEng_rsp.write(updateReplies.read());
        }
        else
        {
            sLookup2txApp_rsp.write(updateReplies.read());
        }
        getRouting = true;
    }
    else if (!routing.isUpdate && !lookupReplies.empty())
    {
        if (routing.source == RX)
        {
            sLookup2rxEng_rsp.write(lookupReplies.read());
        }
        else
        {
            sLookup2txApp_rsp.write(lookupReplies.read());
        }
        getRouting = true;
    }

}


void reverseLookupTableInterface(   stream<revLupInsert>& revTableInserts,
                                    stream<ap_uint<16> >& stateTable2sLookup_releaseSession,
                                    stream<ap_uint<16> >& txEng2sLookup_rev_req,
                                    stream<ap_uint<16> >& sLookup2portTable_releasePort,
                                    stream<rtlSessionUpdateRequest> & deleteCache,
                                    stream<fourTuple>& sLookup2txEng_rev_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    static fourTupleInternal reverseLookupTable[10000];
    #pragma HLS RESOURCE variable=reverseLookupTable core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=reverseLookupTable inter false

    revLupInsert        insert;
    fourTuple           toeTuple;
    fourTupleInternal   releaseTuple;
    ap_uint<16>         sessionID;

    if (!revTableInserts.empty())
    {
        revTableInserts.read(insert);
        reverseLookupTable[insert.key] = insert.value;
    }
    // TODO check if else if necessary
    else if (!stateTable2sLookup_releaseSession.empty())
    {
        stateTable2sLookup_releaseSession.read(sessionID);
        releaseTuple = reverseLookupTable[sessionID];
        sLookup2portTable_releasePort.write(releaseTuple.myPort);
        deleteCache.write(rtlSessionUpdateRequest(releaseTuple, sessionID, DELETE, RX));
    }
    else if (!txEng2sLookup_rev_req.empty())
    {
        txEng2sLookup_rev_req.read(sessionID);
        toeTuple.srcIp = reverseLookupTable[sessionID].myIp;
        toeTuple.dstIp = reverseLookupTable[sessionID].theirIp;
        toeTuple.srcPort = reverseLookupTable[sessionID].myPort;
        toeTuple.dstPort = reverseLookupTable[sessionID].theirPort;
        sLookup2txEng_rev_rsp.write(toeTuple);
    }
}

/** @ingroup session_lookup_controller
 *  This module acts as a wrapper for the RTL implementation of the SessionID Table.
 *  It also includes the wrapper for the sessionID free list which keeps track of the free SessionIDs
 *  @param[in]      rxLookupIn
 *  @param[in]      stateTable2sLookup_releaseSession
 *  @param[in]      txAppLookupIn
 *  @param[in]      txLookup
 *  @param[in]      lookupIn
 *  @param[in]      updateIn
 *  @param[out]     rxLookupOut
 *  @param[out]     portReleaseOut
 *  @param[out]     txAppLookupOut
 *  @param[out]     txResponse
 *  @param[out]     lookupOut
 *  @param[out]     updateOut
 *  @TODO rename
 */
void session_lookup_controller( stream<sessionLookupQueryInternal>&     lookups,
                                //stream<sessionLookupQuery>&           rxEng2sLookup_req,
                                stream<sessionLookupReply>&         sLookup2rxEng_rsp,
                                stream<ap_uint<16> >&               stateTable2sLookup_releaseSession,
                                stream<ap_uint<16> >&               sLookup2portTable_releasePort,
                                //stream<fourTuple>&                    txApp2sLookup_req,
                                stream<sessionLookupReply>&         sLookup2txApp_rsp,
                                stream<ap_uint<16> >&               txEng2sLookup_rev_req,
                                stream<fourTuple>&                  sLookup2txEng_rev_rsp,
                                //stream<rtlSessionLookupRequest>&  sessionLookup_req,
                                stream<rtlSessionLookupReply>&      sessionLookup_rsp,
                                stream<rtlSessionUpdateRequest>&    sessionUpdate_req,
                                stream<rtlSessionUpdateReply>&      sessionUpdate_rsp
                                )//ap_uint<16>& regSessionCount)
{
#pragma HLS dataflow interval=1
//#pragma HLS PIPELINE II=1
//#pragma HLS INLINE off

//#pragma HLS resource core=AXI4Stream variable=new_id metadata="-bus_bundle m_axi_new_id_stream"
//#pragma HLS resource core=AXI4Stream variable=fin_id metadata="-bus_bundle s_axi_fin_id_stream"

    /*static stream<sessionLookupQueryInternal> slc_lookups("slc_lookups");
    #pragma HLS stream variable=slc_lookups depth=4
    #pragma HLS DATA_PACK variable=slc_lookups*/

    static stream<ap_uint<14> > slc_sessionIdFreeList("slc_sessionIdFreeList");
    static stream<ap_uint<14> > slc_sessionIdFinFifo("slc_sessionIdFinFifo");
    #pragma HLS stream variable=slc_sessionIdFreeList depth=16384
    #pragma HLS stream variable=slc_sessionIdFinFifo depth=2

    static stream<sessionLookupReply>   slc_lookupReplies("slc_lookupReplies");
    static stream<sessionLookupReply>   slc_updateReplies("slc_updateReplies");
#pragma HLS STREAM variable=slc_lookupReplies depth=4
#pragma HLS STREAM variable=slc_updateReplies depth=4

    static stream<slupRouting>          slc_routingInfo("slc_routingInfo");
#pragma HLS STREAM variable=slc_routingInfo depth=4

    static stream<rtlSessionUpdateRequest>  insertCache("insertCache");
        #pragma HLS STREAM variable=insertCache depth=4

    static stream<rtlSessionUpdateRequest>  deleteCache("deleteCache");
    #pragma HLS STREAM variable=deleteCache depth=4

    static stream<fourTupleInternal>        slc_insertTuples("slc_insertTuples");
    #pragma HLS STREAM variable=slc_insertTuples depth=4

    static stream<revLupInsert>             reverseLupInsertFifo("reverseLupInsertFifo");
        #pragma HLS STREAM variable=reverseLupInsertFifo depth=4


    sessionIdManager(slc_sessionIdFreeList, slc_sessionIdFinFifo);

    //lookupReplyMerger(rxEng2sLookup_req, txApp2sLookup_req, sessionLookup_req, slc_lookups);

    lookupReplyHandler( //txEng2sLookup_rev_req,
                        sessionLookup_rsp,
                        //deleteCache,
                        //sessionUpdate_rsp,
                        //stateTable2sLookup_releaseSession,
                        lookups,
                        slc_sessionIdFreeList,
                        //sLookup2rxEng_rsp,
                        slc_lookupReplies,
                        slc_routingInfo,
                        slc_insertTuples,
                        //sLookup2txEng_rev_rsp,
                        insertCache
                        //sLookup2portTable_releasePort,
                        //slc_sessionIdFinFifo,
                        );//regSessionCount);


    updateRequestSender(insertCache, deleteCache, sessionUpdate_req);

    updateReplyHandler( sessionUpdate_rsp,
                        slc_insertTuples,
                        slc_updateReplies,
                        reverseLupInsertFifo,
                        slc_sessionIdFinFifo);

    lookupResponseMerger(   slc_routingInfo,
                            slc_lookupReplies, slc_updateReplies,
                            sLookup2rxEng_rsp, sLookup2txApp_rsp);

    reverseLookupTableInterface(    reverseLupInsertFifo,
                                    stateTable2sLookup_releaseSession,
                                    txEng2sLookup_rev_req,
                                    sLookup2portTable_releasePort,
                                    deleteCache,
                                    sLookup2txEng_rev_rsp);
}
