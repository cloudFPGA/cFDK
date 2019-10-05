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
 *               TCP Session Lookup Controller (SLc).
 *
 *****************************************************************************/

#ifndef SLC_H_
#define SLC_H_

#include "../toe.hpp"

using namespace hls;


//enum lookupSource {FROM_RXe, FROM_TAi};
typedef ap_uint<1> lookupSource;  // Encodes the initiator of a CAM lookup or update.
#define FROM_RXe   0
#define FROM_TAi   1

enum lookupOp {INSERT, DELETE};

struct slupRouting
{
    bool            isUpdate;
    lookupSource    source;
    slupRouting() {}
    slupRouting(bool isUpdate, lookupSource src)
            :isUpdate(isUpdate), source(src) {}
};

/********************************************************************
 * Session Lookup Controller / Internal Four Tuple Structure
 *  This struct defines the internal storage used by [SLc] to store
 *   a 4-tuple. The struct uses the terms 'my' and 'their' instead of
 *   'dest' and 'source'.
 *  When a socket pair is sent or received from the Tx/Rx path, it is
 *   mapped by [SLc] to this fourTuple struct.
 *  The operator '<' is necessary here for the c++ dummy memory
 *   implementation which uses an std::map.
 ********************************************************************/
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

/**********************************************************
 * Session Lookup Controller / Internal Query Structure
 **********************************************************/
struct sessionLookupQueryInternal
{
    fourTupleInternal   tuple;
    bool                allowCreation;
    lookupSource        source;
    sessionLookupQueryInternal() {}
    sessionLookupQueryInternal(fourTupleInternal tuple, bool allowCreation, lookupSource src)
            :tuple(tuple), allowCreation(allowCreation), source(src) {}
};

/**********************************************************
 * Smart CAM interface (CAM)
 *
 *  [FIXME - MOVE THIS SECTION INTO TOE.HPP]
 *
 * Warning:
 *   Don't change the order of the fields in the session-
 *   lookup-request, session-lookup-reply, session-update-
 *   request and session-update-reply as these structures
 *   end up being mapped to a physical Axi4-Stream interface
 *   between the TOE and the CAM.
 * Info: The member elements of the struct are placed into
 *   the physical vector interface in the order the appear
 *   in the C code: the first element of the struct is alig-
 *   ned on the LSB of the vector and the final element of
 *   the struct is aligned with the MSB of the vector. 
 **********************************************************/
typedef ap_uint<14> RtlSessId;

/********************************************
 * CAM / Session Lookup Request
 ********************************************/
struct rtlSessionLookupRequest     // [FIXME - Rename ]
{
    fourTupleInternal   key;       // 96 bits 
    lookupSource        source;    //  1 bit : '0' is [RXe], '1' is [TAi]

    rtlSessionLookupRequest() {}
    rtlSessionLookupRequest(fourTupleInternal tuple, lookupSource src)
                : key(tuple), source(src) {}
};

/********************************************
 * CAM / Session Lookup Reply
 *********************************************/
struct rtlSessionLookupReply       // [FIXME - Rename ]
{
    RtlSessId           sessionID; // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe], '1' is [TAi]
    bool                hit;       //  1 bit

    rtlSessionLookupReply() {}
    rtlSessionLookupReply(bool hit, lookupSource src) :
        hit(hit), sessionID(0), source(src) {}
    rtlSessionLookupReply(bool hit, RtlSessId id, lookupSource src) :
        hit(hit), sessionID(id), source(src) {}
};

/********************************************
 * CAM / Session Update Request
 ********************************************/
struct rtlSessionUpdateRequest     // [FIXME - Rename ]
{
    fourTupleInternal   key;       // 96 bits
    RtlSessId           value;     // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    lookupOp            op;        //  1 bit : '0' is INSERT, '1' is DELETE

    rtlSessionUpdateRequest() {}
    rtlSessionUpdateRequest(fourTupleInternal key, RtlSessId value, lookupOp op, lookupSource src) :
        key(key), value(value), op(op), source(src) {}
};

/********************************************
 * CAM / Session Update Reply
 *********************************************/
struct rtlSessionUpdateReply       // [FIXME - Rename ]
{
    RtlSessId           sessionID; // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    lookupOp            op;        //  1 bit : '0' is INSERT, '1' is DELETE

    rtlSessionUpdateReply() {}
    rtlSessionUpdateReply(lookupOp op, lookupSource src) :
        op(op), source(src) {}
    rtlSessionUpdateReply(RtlSessId id, lookupOp op, lookupSource src) :
        sessionID(id), op(op), source(src) {}
};


struct revLupInsert
{
    SessionId           key;
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
        stream<sessionLookupQuery>         &siRXe_SessLookupReq,
        stream<sessionLookupReply>         &soRXe_SessLookupRep,
        stream<ap_uint<16> >               &stateTable2sLookup_releaseSession,
        stream<ap_uint<16> >               &sLookup2portTable_releasePort,
        stream<LE_SocketPair>              &siTAi_SessLookupReq,
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
        ap_uint<16>                        &poSssRegCnt
);

#endif

