/*****************************************************************************
 * @file       : session_lookup.hpp
 * @brief      : Session Lookup Controller (SLc) of TCP Offload Engine (TOE).
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
 *               TCP Session Lookup Controller.
 *
 *****************************************************************************/

#include "../toe.hpp"

using namespace hls;


enum lookupSource {RX, TX_APP};

enum lookupOp {INSERT, DELETE};

struct slupRouting
{
    bool            isUpdate;
    lookupSource    source;
    slupRouting() {}
    slupRouting(bool isUpdate, lookupSource src)
            :isUpdate(isUpdate), source(src) {}
};

/*
 *  This struct defines the internal storage format of the IP tuple.
 *  This struct uses the terms 'my' and 'their' instead of 'dest' and 'source'.
 *  When a tuple is sent or received from the tx/rx path it is mapped to the
 *  fourTuple struct.
 *  The operator '<' is necessary here for the c++ dummy memory implementation
 *  which uses an std::map.
 */
struct fourTupleInternal
{
    ap_uint<32> myIp;
    ap_uint<32> theirIp;
    ap_uint<16> myPort;
    ap_uint<16> theirPort;
    fourTupleInternal() {}
    fourTupleInternal(ap_uint<32> myIp, ap_uint<32> theirIp, ap_uint<16> myPort, ap_uint<16> theirPort)
    : myIp(myIp), theirIp(theirIp), myPort(myPort), theirPort(theirPort) {}

    bool operator<(const fourTupleInternal& other) const
    {
        if (myIp < other.myIp) {
            return true;
        }
        else if (myIp == other.myIp) {
            if (theirIp < other.theirIp) {
                return true;
            }
            else if(theirIp == other.theirIp) {
                if (myPort < other.myPort) {
                    return true;
                }
                else if (myPort == other.myPort) {
                    if (theirPort < other.theirPort) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

/* @ingroup session_lookup_controller
 *
 */
struct sessionLookupQueryInternal
{
    fourTupleInternal   tuple;
    bool                allowCreation;
    lookupSource        source;
    sessionLookupQueryInternal() {}
    sessionLookupQueryInternal(fourTupleInternal tuple, bool allowCreation, lookupSource src)
            :tuple(tuple), allowCreation(allowCreation), source(src) {}
};

/** @ingroup session_lookup_controller
 *
 */
struct rtlSessionLookupRequest
{
    lookupSource        source;
    fourTupleInternal   key;
    rtlSessionLookupRequest() {}
    rtlSessionLookupRequest(fourTupleInternal tuple, lookupSource src)
                :key(tuple), source(src) {}
};

/*************************************************************************
 * RTL Session Type Definitions
 *************************************************************************/
typedef ap_uint<14> RtlSessId;  // [FIXME - Align size with log2(MAX_SESSIONS)]

/********************************************
 * Rtl Session Update Request
 ********************************************/
struct rtlSessionUpdateRequest
{
    lookupSource        source;
    lookupOp            op;
    RtlSessId           value;
    fourTupleInternal   key;

    rtlSessionUpdateRequest() {}
    rtlSessionUpdateRequest(fourTupleInternal key, RtlSessId value, lookupOp op, lookupSource src) :
        key(key), value(value), op(op), source(src) {}
};


/********************************************
 * Rtl Session Lookup Reply
 *********************************************/
struct rtlSessionLookupReply
{
    lookupSource        source;
    RtlSessId           sessionID;
    bool                hit;

    rtlSessionLookupReply() {}
    rtlSessionLookupReply(bool hit, lookupSource src) :
        hit(hit), sessionID(0), source(src) {}
    rtlSessionLookupReply(bool hit, RtlSessId id, lookupSource src) :
        hit(hit), sessionID(id), source(src) {}
};

/********************************************
 * Rtl Session Update Reply
 *********************************************/
struct rtlSessionUpdateReply
{
    lookupSource        source;
    lookupOp            op;
    RtlSessId           sessionID;

    rtlSessionUpdateReply() {}
    rtlSessionUpdateReply(lookupOp op, lookupSource src) :
        op(op), source(src) {}
    rtlSessionUpdateReply(RtlSessId id, lookupOp op, lookupSource src) :
        sessionID(id), op(op), source(src) {}
};


struct revLupInsert
{
    ap_uint<16>         key;
    fourTupleInternal   value;
    revLupInsert() {};
    revLupInsert(ap_uint<16> key, fourTupleInternal value)
            :key(key), value(value) {}
};

/*****************************************************************************
 * @brief   Main process of the TCP Session Lookup Controller (SLc).
 *
 * @ingroup session_lookup_controller
 *****************************************************************************/
void session_lookup_controller(
        stream<sessionLookupQuery>&         rxEng2sLookup_req,
        stream<sessionLookupReply>&         sLookup2rxEng_rsp,
        stream<ap_uint<16> >&               stateTable2sLookup_releaseSession,
        stream<ap_uint<16> >&               sLookup2portTable_releasePort,
        stream<fourTuple>&                  txApp2sLookup_req,
        stream<sessionLookupReply>&         sLookup2txApp_rsp,
        stream<ap_uint<16> >&               txEng2sLookup_rev_req,
        stream<fourTuple>&                  sLookup2txEng_rev_rsp,
        stream<rtlSessionLookupRequest>&    sessionLookup_req,
        stream<rtlSessionLookupReply>&      sessionLookup_rsp,
        stream<rtlSessionUpdateRequest>&    sessionUpdate_req,
        //stream<rtlSessionUpdateRequest>&  sessionInsert_req,
        //stream<rtlSessionUpdateRequest>&  sessionDelete_req,
        stream<rtlSessionUpdateReply>&      sessionUpdate_rsp,
        ap_uint<16>                         &poSssRelCnt,
        ap_uint<16>                         &poSssRegCnt
);
