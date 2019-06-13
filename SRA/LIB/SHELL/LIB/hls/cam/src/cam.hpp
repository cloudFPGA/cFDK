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

#ifndef CAM_H_
#define CAM_H_

//using namespace hls;

#include "../../toe/src/session_lookup_controller/session_lookup_controller.hpp"


//typedef ap_uint<1> lookupSource;  // Encodes the initiator of a CAM lookup or update.
//#define FROM_RXe   0
//#define FROM_TAi   1

//enum lookupOp {INSERT, DELETE};

/***
struct slupRouting
{
    bool            isUpdate;
    lookupSource    source;
    slupRouting() {}
    slupRouting(bool isUpdate, lookupSource src)
            :isUpdate(isUpdate), source(src) {}
};
****/

/********************************************************************
 * Session Lookup Controller / Internal Four Tuple Structure
 *  This struct defines the internal storage format of the IP tuple.
 *  This struct uses the terms 'my' and 'their' instead of 'dest' and
 *   'source'.
 *  When a tuple is sent or received from the tx/rx path it is mapped
 *   to the fourTuple struct.
 *  The operator '<' is necessary here for the c++ dummy memory
 *   implementation which uses an std::map.
 ********************************************************************/
//struct fourTupleInternal
//{
//    ap_uint<32> myIp;
//    ap_uint<32> theirIp;
//    ap_uint<16> myPort;
//    ap_uint<16> theirPort;
//    fourTupleInternal() {}
//    fourTupleInternal(ap_uint<32> myIp, ap_uint<32> theirIp, ap_uint<16> myPort, ap_uint<16> theirPort)
//    : myIp(myIp), theirIp(theirIp), myPort(myPort), theirPort(theirPort) {}
//
//    bool operator<(const fourTupleInternal& other) const
//    {
//        if (myIp < other.myIp) {
//            return true;
//        }
//        else if (myIp == other.myIp) {
//            if (theirIp < other.theirIp) {
//                return true;
//            }
//            else if(theirIp == other.theirIp) {
//                if (myPort < other.myPort) {
//                    return true;
//                }
//                else if (myPort == other.myPort) {
//                    if (theirPort < other.theirPort) {
//                        return true;
//                    }
//                }
//            }
//        }
//        return false;
//    }
//};

/**********************************************************
 * Session Lookup Controller / Internal Query Structure
 **********************************************************/
//struct sessionLookupQueryInternal
//{
//    fourTupleInternal   tuple;
//    bool                allowCreation;
//    lookupSource        source;
//    sessionLookupQueryInternal() {}
//    sessionLookupQueryInternal(fourTupleInternal tuple, bool allowCreation, lookupSource src)
//            :tuple(tuple), allowCreation(allowCreation), source(src) {}
//};

/**********************************************************
 * Smart CAM interface (CAM)
 *
 * Warning:
 *   Don't change the order of the fields in the session-
 *   lookup-request, session-lookup-reply, session-update-
 *   request and session-update-reply as these structures
 *   end up being mapped to a physical Axi4-Stream interface
 *   between the TOE and the CAM.
 **********************************************************/
//typedef ap_uint<14> RtlSessId;  // [FIXME - Align size with log2(MAX_SESSIONS)]

/********************************************
 * CAM / Session Lookup Request
 ********************************************/
//struct rtlSessionLookupRequest {
//    lookupSource        source;    //  1 bit : '0' is [RXe], '1' is [TAi]
//    fourTupleInternal   key;       // 96 bits
//    rtlSessionLookupRequest() {}
//    rtlSessionLookupRequest(fourTupleInternal tuple, lookupSource src)
//                :key(tuple), source(src) {}
//};

/********************************************
 * CAM / Session Lookup Reply
 *********************************************/
//struct rtlSessionLookupReply
//{
//    lookupSource        source;    //  1 bit
//    RtlSessId           sessionID; // 14 bits
//    bool                hit;       //  1 bit
//
//    rtlSessionLookupReply() {}
//    rtlSessionLookupReply(bool hit, lookupSource src) :
//        hit(hit), sessionID(0), source(src) {}
//    rtlSessionLookupReply(bool hit, RtlSessId id, lookupSource src) :
//        hit(hit), sessionID(id), source(src) {}
//};

/********************************************
 * CAM / Session Update Request
 ********************************************/
//struct rtlSessionUpdateRequest
//{
//    lookupSource        source;    //  1 bit
//    lookupOp            op;        //  1 bit
//    RtlSessId           value;     // 14 bits
//    fourTupleInternal   key;       // 96 bits
//
//    rtlSessionUpdateRequest() {}
//    rtlSessionUpdateRequest(fourTupleInternal key, RtlSessId value, lookupOp op, lookupSource src) :
//        key(key), value(value), op(op), source(src) {}
//};

/********************************************
 * CAM / Session Update Reply
 *********************************************/
//struct rtlSessionUpdateReply
//{
//    lookupSource        source;    //  1 bit
//    lookupOp            op;        //  1 bit
//    RtlSessId           sessionID; // 14 bits
//
//    rtlSessionUpdateReply() {}
//    rtlSessionUpdateReply(lookupOp op, lookupSource src) :
//        op(op), source(src) {}
//    rtlSessionUpdateReply(RtlSessId id, lookupOp op, lookupSource src) :
//        sessionID(id), op(op), source(src) {}
//};


//struct revLupInsert
//{
//    ap_uint<16>         key;
//    fourTupleInternal   value;
//    revLupInsert() {};
//    revLupInsert(ap_uint<16> key, fourTupleInternal value)
//            :key(key), value(value) {}
//};


/***********************************************
 * KEY-VALUE PAIR
 ***********************************************/
class KeyValuePair
{
  public:
    fourTupleInternal   key;       // 96 bits
    RtlSessId           value;     // 14 bits
    bool                valid;
    KeyValuePair() {}
    KeyValuePair(fourTupleInternal key, RtlSessId value, bool valid) :
        key(key), value(value), valid(valid) {}

};

inline bool operator == (fourTupleInternal const &s1, fourTupleInternal const &s2) {
            return ((s1.myIp    == s2.myIp)    && (s1.myPort    == s2.myPort)    &&
                    (s1.theirIp == s2.theirIp) && (s1.theirPort == s2.theirPort));
    }


/******************************************************************************
 * @brief   Main process of the Content-Addressable Memory (CAM).
 ******************************************************************************/
void cam(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        ap_uint<1>                          *poMMIO_CamReady,

        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<rtlSessionLookupRequest>     &siTOE_SssLkpReq,
        stream<rtlSessionLookupReply>       &soTOE_SssLkpRep,
        stream<rtlSessionUpdateRequest>     &siTOE_SssUpdReq,
        stream<rtlSessionUpdateReply>       &soTOE_SssUpdRep

);

#endif
