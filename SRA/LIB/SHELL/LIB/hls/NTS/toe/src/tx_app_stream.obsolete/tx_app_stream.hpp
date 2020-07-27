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
 * @file       : tx_app_stream.hpp
 * @brief      : Tx Application Stream (Tas) management
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;

#define ERROR_NOSPACE        1
#define ERROR_NOCONNCECTION  2

/** @ingroup tx_app_stream_if
 *
 */
struct eventMeta
{
    ap_uint<16> sessionID;
    ap_uint<16> address;
    ap_uint<16> length;
    eventMeta() {}
    eventMeta(ap_uint<16> id, ap_uint<16> addr, ap_uint<16> len)
                :sessionID(id), address(addr), length(len) {}
};


/***********************************************
 * Metadata for storing a segment in memory
 ***********************************************/
class SegMemMeta {
  public:
    TcpSessId    sessId;
    TcpBufAdr    addr;
    TcpSegLen    len;
    bool         drop;
    SegMemMeta() {}
    SegMemMeta(bool drop) :
        sessId(0), addr(0), len(0), drop(drop) {}
    SegMemMeta(TcpSessId sessId, TcpBufAdr addr, TcpSegLen len) :
        sessId(sessId), addr(addr), len(len), drop(false) {}
};


/*****************************************************************************
 * @brief   Main process of the Tx Application Stream (Tas).
 *
 *****************************************************************************/
void tx_app_stream(
        stream<AppMeta>             &siTRIF_Meta,
        stream<AppWrSts>            &soTRIF_DSts,
        stream<TcpSessId>           &soSTt_SessStateReq,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<TxAppTableQuery>     &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<SegMemMeta>          &sSmlToSmw_SegMeta,
        stream<Event>               &soEVe_Event
);
