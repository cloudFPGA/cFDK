/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : session_lookup.hpp
 * @brief      : Session Lookup Controller (SLc) of TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_SLC_H_
#define _TOE_SLC_H_

#include "../../../../NTS/nts_utils.hpp"
#include "../../../../NTS/toe/src/toe.hpp"

using namespace hls;

/*******************************************************************************
 * GLOBAL DEFINITIONS USED BY SLc
 ******************************************************************************/

/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY SLc
 *******************************************************************************/
typedef FourTuple  SLcFourTuple;

//=========================================================
//== SLc - Internal Session Lookup Query
//=========================================================
class SLcQuery {
  public:
    SLcFourTuple    tuple;
    bool            allowCreation;
    LkpSrcBit       source;
    SLcQuery() {}
    SLcQuery(SLcFourTuple tuple, bool allowCreation, LkpSrcBit src) :
        tuple(tuple), allowCreation(allowCreation), source(src) {}
};

//=========================================================
//== SLc - Internal Reverse Lookup
//=========================================================
class SLcReverseLkp
{
  public:
    SessionId           key;
    SLcFourTuple        value;
    SLcReverseLkp() {}
    SLcReverseLkp(SessionId key, SLcFourTuple value) :
        key(key), value(value) {}
};


/*******************************************************************************
 *
 * @brief ENTITY - Session Lookup Controller (SLc)
 *
 *******************************************************************************/
void session_lookup_controller(
        stream<SessionLookupQuery>         &siRXe_SessLookupReq,
        stream<SessionLookupReply>         &soRXe_SessLookupRep,
        stream<SessionId>                  &siSTt_SessReleaseCmd,
        stream<TcpPort>                    &soPRt_ClosePortCmd,
        stream<SocketPair>                 &siTAi_SessLookupReq,
        stream<SessionLookupReply>         &soTAi_SessLookupRep,
        stream<SessionId>                  &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &soTXe_ReverseLkpRep,
        stream<CamSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<CamSessionLookupReply>      &siCAM_SessLookupRep,
        stream<CamSessionUpdateRequest>    &soCAM_SessUpdateReq,
        stream<CamSessionUpdateReply>      &siCAM_SessUpdateRep,
        ap_uint<16>                        &poSssRelCnt,
        ap_uint<16>                        &poSssRegCnt
);

#endif

/*! \} */

