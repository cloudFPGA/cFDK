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

/*****************************************************************************
 * @file       : session_lookup.hpp
 * @brief      : Session Lookup Controller (SLc) of TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
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

typedef ap_uint<1> lookupSource;  // Encodes the initiator of a CAM lookup or update.
#define FROM_RXe   0
#define FROM_TAi   1

enum lookupOp {INSERT=0, DELETE};

/********************************************************************
 * SLc / Internal Four Tuple Structure
 *  This class defines the internal storage used by [SLc] for the
 *   SocketPair (alias 4-tuple). The class uses the terms 'my' and
 *   'their' instead of 'dest' and 'src'.
 *  When a socket pair is sent or received from the Tx/Rx path, it is
 *   mapped by [SLc] to this FourTuple structure.
 *  The operator '<' is necessary here for the c++ dummy memory
 *   implementation which uses an std::map.
 ********************************************************************/
class SLcFourTuple {
  public:
	LE_Ip4Addr	myIp;
	LE_Ip4Addr	theirIp;
	LE_TcpPort	myPort;
	LE_TcpPort	theirPort;
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

/**********************************************************
 * SLc / Internal Session Lookup Query
 **********************************************************/
class SLcQuery {
  public:
    SLcFourTuple	tuple;
    bool            allowCreation;
    lookupSource    source;
    SLcQuery() {}
    SLcQuery(SLcFourTuple tuple, bool allowCreation, lookupSource src) :
        tuple(tuple), allowCreation(allowCreation), source(src) {}
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
class RtlSessionLookupRequest
{
  public:
    SLcFourTuple   key;       // 96 bits
    lookupSource   source;    //  1 bit : '0' is [RXe], '1' is [TAi]

    RtlSessionLookupRequest() {}
    RtlSessionLookupRequest(SLcFourTuple tuple, lookupSource src)
                : key(tuple), source(src) {}
};

/********************************************
 * CAM / Session Lookup Reply
 *********************************************/
class RtlSessionLookupReply
{
  public:
    RtlSessId           sessionID; // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe], '1' is [TAi]
    bool                hit;       //  1 bit

    RtlSessionLookupReply() {}
    RtlSessionLookupReply(bool hit, lookupSource src) :
        hit(hit), sessionID(0), source(src) {}
    RtlSessionLookupReply(bool hit, RtlSessId id, lookupSource src) :
        hit(hit), sessionID(id), source(src) {}
};

/********************************************
 * CAM / Session Update Request
 ********************************************/
class RtlSessionUpdateRequest
{
  public:
    SLcFourTuple        key;       // 96 bits
    RtlSessId           value;     // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    lookupOp            op;        //  1 bit : '0' is INSERT, '1' is DELETE

    RtlSessionUpdateRequest() {}
    RtlSessionUpdateRequest(SLcFourTuple key, RtlSessId value, lookupOp op, lookupSource src) :
        key(key), value(value), op(op), source(src) {}
};

/********************************************
 * CAM / Session Update Reply
 *********************************************/
class RtlSessionUpdateReply
{
  public:
    RtlSessId           sessionID; // 14 bits
    lookupSource        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    lookupOp            op;        //  1 bit : '0' is INSERT, '1' is DELETE

    RtlSessionUpdateReply() {}
    RtlSessionUpdateReply(lookupOp op, lookupSource src) :
        op(op), source(src) {}
    RtlSessionUpdateReply(RtlSessId id, lookupOp op, lookupSource src) :
        sessionID(id), op(op), source(src) {}
};

/************************************************
 * SLc / Internal Reverse Lookup structure
 ************************************************/
class SLcReverseLkp
{
  public:
    SessionId           key;
    SLcFourTuple        value;
    SLcReverseLkp() {}
    SLcReverseLkp(SessionId key, SLcFourTuple value) :
        key(key), value(value) {}
};

/*****************************************************************************
 * @brief   Main process of the TCP Session Lookup Controller (SLc).
 *
 *****************************************************************************/
void session_lookup_controller(
        stream<SessionLookupQuery>         &siRXe_SessLookupReq,
        stream<SessionLookupReply>         &soRXe_SessLookupRep,
        stream<SessionId>                  &siSTt_SessReleaseCmd,
        stream<TcpPort>                    &soPRt_ClosePortCmd,
        //OBSOLETE_20200629 stream<LE_SocketPair>  &siTAi_SessLookupReq,
        stream<SocketPair>                 &siTAi_SessLookupReq,
        stream<SessionLookupReply>         &soTAi_SessLookupRep,
        stream<SessionId>                  &siTXe_ReverseLkpReq,
        stream<fourTuple>                  &soTXe_ReverseLkpRep,
        stream<RtlSessionLookupRequest>    &soCAM_SessLookupReq,
        stream<RtlSessionLookupReply>      &siCAM_SessLookupRep,
        stream<RtlSessionUpdateRequest>    &soCAM_SessUpdateReq,
        stream<RtlSessionUpdateReply>      &siCAM_SessUpdateRep,
        ap_uint<16>                        &poSssRelCnt,
        ap_uint<16>                        &poSssRegCnt
);

#endif

