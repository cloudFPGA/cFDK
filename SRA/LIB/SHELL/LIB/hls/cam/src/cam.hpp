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
