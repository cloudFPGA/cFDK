/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * @file       : toecam.hpp
 * @brief      : Content-Addressable Memory (CAM) for TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOECAM
 * \{
 *******************************************************************************/

#ifndef _TOECAM_H_
#define _TOECAM_H_

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"


/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY THIS CAM
 *******************************************************************************/

//=========================================================
//== KEY-VALUE PAIR
//=========================================================
class KeyValuePair {
  public:
    FourTuple    key;       // 96 bits
    RtlSessId    value;     // 14 bits
    ValBool      valid;
    KeyValuePair() {}
    KeyValuePair(FourTuple key, RtlSessId value, ValBool valid) :
        key(key), value(value), valid(valid) {}
};


/*******************************************************************************
 *
 * ENTITY - CONTENT ADDRESSABLE MEMORY (TOECAM)
 *
 *******************************************************************************/
void toecam_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        StsBit                              *poMMIO_CamReady,
        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<CamSessionLookupRequest>     &siTOE_SssLkpReq,
        stream<CamSessionLookupReply>       &soTOE_SssLkpRep,
        stream<CamSessionUpdateRequest>     &siTOE_SssUpdReq,
        stream<CamSessionUpdateReply>       &soTOE_SssUpdRep
);

#endif

/*! \} */
