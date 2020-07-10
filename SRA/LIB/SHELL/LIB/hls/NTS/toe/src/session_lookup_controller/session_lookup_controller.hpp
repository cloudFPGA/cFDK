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

#include "../../../../NTS/toe/src/toe.hpp"
#include "../../../../NTS/nts_utils.hpp"

using namespace hls;

// Forward class declarations
//class RtlSessionUpdateRequest;
//class RtlSessionUpdateReply;
//class RtlSessionLookupReply;
//class RtlSessionLookupRequest;

/******************************************************************************
 * GLOBAL DEFINITIONS USED BY SLc
 ******************************************************************************/
//OBSOLETE_20200710 #define FROM_RXe   0
//OBSOLETE_20200710 #define FROM_TAi   1

//OBSOLETE_20200701 enum lookupOp {INSERT=0, DELETE};

/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY SLc
 *******************************************************************************/
typedef ap_uint<1> lookupSource;  // [FIXME - Replace by LkpSrcBit] Encodes the initiator of a CAM lookup or update.
typedef FourTuple  SLcFourTuple;

//=========================================================
//== SLc - Internal Four Tuple Structure
//==  This class defines the internal storage used by [SLc]
//==  for the SocketPair (alias 4-tuple). The class uses the
//==  terms 'my' and 'their' instead of 'dest' and 'src'.
//==  When a socket pair is sent or received from the Tx/Rx
//==  path, it is mapped by [SLc] to this FourTuple structure.
//==  The operator '<' is necessary here for the c++ dummy
//==  memory implementation which uses an std::map.
//=========================================================

/*** OBSOLETE_20200701 ****************
class SLcFourTuple {
  public:
    LE_Ip4Addr  myIp;
    LE_Ip4Addr  theirIp;
    LE_TcpPort  myPort;
    LE_TcpPort  theirPort;
    SLcFourTuple() {}
    SLcFourTuple(LE_Ip4Addr myIp, LE_Ip4Addr theirIp, LE_TcpPort myPort, LE_TcpPort theirPort) :
        myIp(myIp), theirIp(theirIp), myPort(myPort), theirPort(theirPort) {}

    bool operator<(const SLcFourTuple& other) const {
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
*******************************************/

//=========================================================
//== SLc - Internal Session Lookup Query
//=========================================================
class SLcQuery {
  public:
    SLcFourTuple	tuple;
    bool            allowCreation;
    lookupSource    source;
    SLcQuery() {}
    SLcQuery(SLcFourTuple tuple, bool allowCreation, lookupSource src) :
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

