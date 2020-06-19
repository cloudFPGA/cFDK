/*
 * Copyright 2016 -- 2020 IBM Corporation
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
 */

/*******************************************************************************
 * @file       : cam.hpp
 * @brief      : Content-Addressable Memory (CAM) of TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_CAM
 * \addtogroup NTS_CAM
 * \{
 *******************************************************************************/

#ifndef _CAM_H_
#define _CAM_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"

using namespace hls;

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

/*******************************************************************************
 *
 * ENTITY - CONTENT ADDRESSABLE MEMOERY (CAM)
 *
 *******************************************************************************/
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

/*! \} */
