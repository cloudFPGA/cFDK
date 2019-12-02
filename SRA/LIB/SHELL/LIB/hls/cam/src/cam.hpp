/*****************************************************************************
 * @file       : cam.hpp
 * @brief      : Content-Addressable Memory (CAM) of TCP Offload Engine (TOE).
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

#include "../../toe/src/session_lookup_controller/session_lookup_controller.hpp"


/***********************************************
 * KEY-VALUE PAIR
 ***********************************************/
class KeyValuePair
{
  public:
    SLcFourTuple        key;       // 96 bits
    RtlSessId           value;     // 14 bits
    bool                valid;
    KeyValuePair() {}
    KeyValuePair(SLcFourTuple key, RtlSessId value, bool valid) :
        key(key), value(value), valid(valid) {}

};

inline bool operator == (SLcFourTuple const &s1, SLcFourTuple const &s2) {
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
        StsBit                              *poMMIO_CamReady,

        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<RtlSessionLookupRequest>     &siTOE_SssLkpReq,
        stream<RtlSessionLookupReply>       &soTOE_SssLkpRep,
        stream<RtlSessionUpdateRequest>     &siTOE_SssUpdReq,
        stream<RtlSessionUpdateReply>       &soTOE_SssUpdRep

);

#endif
