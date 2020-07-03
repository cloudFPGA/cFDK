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

using namespace hls;

/*******************************************************************************
 * INTERNAL TYPES and CLASSES USED BY THIS CAM
 *******************************************************************************/
typedef ap_uint<1> LkpSrcBit;  // Encodes the initiator of a CAM lookup or update.
#define FROM_RXe   0
#define FROM_TAi   1

enum LkpOpBit {INSERT=0, DELETE};  // Encodes the CAM operation
//enum LkpOpBit {INSERT=0, DELETE};  // Encodes the CAM operation

//=========================================================
//== Four Tuple Structure
//=========================================================
/*
 *  This class defines the internal storage used by this CAM implementation for
 *   the SocketPair (alias 4-tuple). The class uses the terms 'my' and 'their'
 *   instead of 'dest' and 'src'.
 *  When a socket pair is received from TOE, it is mapped by the CAM to this
 *   FourTuple structure.
 *  The operator '<' is necessary here for the c++ dummy memory implementation
 *   which uses an std::map.
 */
class FourTuple {  // [FIXME - Replace with SocketPair]

  public:
    LE_Ip4Addr  myIp;
    LE_Ip4Addr  theirIp;
    LE_TcpPort  myPort;
    LE_TcpPort  theirPort;
    FourTuple() {}
    FourTuple(LE_Ip4Addr myIp, LE_Ip4Addr theirIp, LE_TcpPort myPort, LE_TcpPort theirPort) :
        myIp(myIp), theirIp(theirIp), myPort(myPort), theirPort(theirPort) {}

    bool operator < (const FourTuple& other) const {
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

inline bool operator == (FourTuple const &s1, FourTuple const &s2) {
    return ((s1.myIp    == s2.myIp)    && (s1.myPort    == s2.myPort)    &&
            (s1.theirIp == s2.theirIp) && (s1.theirPort == s2.theirPort));
}

//=========================================================
//== KEY-VALUE PAIR
//=========================================================
class KeyValuePair {
  public:
    FourTuple   key;       // 96 bits
    //OBSOLETE_20200701 RtlSessId   value;     // 14 bits
    SessionId   value;     // 16 bits
    bool        valid;
    KeyValuePair() {}
    KeyValuePair(FourTuple key, SessionId value, bool valid) :
        key(key), value(value), valid(valid) {}
};


/*******************************************************************************
 * RTL TYPES and CLASSES USED BY TOECAM
 *******************************************************************************
 * Warning:
 *   Don't change the order of the fields in the session-
 *   lookup-request, session-lookup-reply, session-update-
 *   request and session-update-reply as these structures
 *   end up being mapped to a physical Axi4-Stream interface
 *   between the TOE and the CAM.
 * Info: The member elements of the struct are placed into
 *   the physical vector interface in the order they appear
 *   in the C code: the first element of the struct is alig-
 *   ned on the LSB of the vector and the final element of
 *   the struct is aligned with the MSB of the vector.
 *******************************************************************************/

//=========================================================
//== RTL - Session Identifier
//=========================================================
typedef ap_uint<14> RtlSessId;

//=========================================================
//== RTL - Session Lookup Request
//=========================================================
class RtlSessionLookupRequest {
  public:
    FourTuple        key;       // 96 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe], '1' is [TAi]

    RtlSessionLookupRequest() {}
    RtlSessionLookupRequest(FourTuple tuple, LkpSrcBit src)
                : key(tuple), source(src) {}
};

//=========================================================
//== RTL - Session Lookup Reply
//=========================================================
class RtlSessionLookupReply {
  public:
    RtlSessId        sessionID; // 14 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe], '1' is [TAi]
    bool             hit;       //  1 bit

    RtlSessionLookupReply() {}
    RtlSessionLookupReply(bool hit, LkpSrcBit src) :
        hit(hit), sessionID(0), source(src) {}
    RtlSessionLookupReply(bool hit, RtlSessId id, LkpSrcBit src) :
        hit(hit), sessionID(id), source(src) {}
};

//=========================================================
//== RTL - Session Update Request
//=========================================================
class RtlSessionUpdateRequest {
  public:
    FourTuple        key;       // 96 bits
    RtlSessId        value;     // 14 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    LkpOpBit         op;        //  1 bit : '0' is INSERT, '1' is DELETE

    RtlSessionUpdateRequest() {}
    RtlSessionUpdateRequest(FourTuple key, RtlSessId value, LkpOpBit op, LkpSrcBit src) :
        key(key), value(value), op(op), source(src) {}
};

//=========================================================
//== RTL - Session Update Reply
//=========================================================
class RtlSessionUpdateReply {
  public:
    RtlSessId        sessionID; // 14 bits
    LkpSrcBit        source;    //  1 bit : '0' is [RXe],  '1' is [TAi]
    LkpOpBit         op;        //  1 bit : '0' is INSERT, '1' is DELETE

    RtlSessionUpdateReply() {}
    RtlSessionUpdateReply(LkpOpBit op, LkpSrcBit src) :
        op(op), source(src) {}
    RtlSessionUpdateReply(RtlSessId id, LkpOpBit op, LkpSrcBit src) :
        sessionID(id), op(op), source(src) {}
};


/*******************************************************************************
 *
 * ENTITY - CONTENT ADDRESSABLE MEMORY (TOECAM)
 *
 *******************************************************************************/
void toecam(
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
