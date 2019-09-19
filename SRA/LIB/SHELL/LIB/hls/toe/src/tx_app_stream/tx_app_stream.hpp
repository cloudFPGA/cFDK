/*****************************************************************************
 * @file       : tx_app_stream.hpp
 * @brief      : Tx Application Stream (Tas) management
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *****************************************************************************/



#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;

//OBSOLETE_20190919 static const ap_uint<17> ERROR_NOSPACE              = -1;
//OBSOLETE_20190919 static const ap_uint<17> ERROR_NOCONNCECTION        = -2;

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

/*** OBSOLETE-20190611 ******
struct pkgPushMeta
{
    ap_uint<16> sessionID;
    ap_uint<16> address;
    ap_uint<16> length;
    bool        drop;
    pkgPushMeta() {}
    pkgPushMeta(bool drop)
                        :sessionID(0), address(0), length(0), drop(drop) {}
    pkgPushMeta(ap_uint<16> id, ap_uint<16> addr, ap_uint<16> len)
                    :sessionID(id), address(addr), length(len), drop(false) {}
};
***********************************/

/***********************************************
 * Metadata for storing a segment in memory
 ***********************************************/
class SegMemMeta {
  public:
    TcpSessId    sessId;
    ap_uint<16>  addr;
    ap_uint<16>  len;
    bool         drop;
    SegMemMeta() {}
    SegMemMeta(bool drop) :
        sessId(0), addr(0), len(0), drop(drop) {}
    SegMemMeta(TcpSessId sessId, ap_uint<16> addr, ap_uint<16> len) :
        sessId(sessId), addr(addr), len(len), drop(false) {}
};


/*****************************************************************************
 * @brief   Main process of the Tx Application Stream (Tas).
 *
 *****************************************************************************/
void tx_app_stream(
        stream<AppData>            &siTRIF_Data,
        stream<AppMeta>            &siTRIF_Meta,
        stream<AppWrSts>           &soTRIF_DSts,
        stream<TcpSessId>          &soSTt_SessStateReq,
        stream<SessionState>       &siSTt_SessStateRep,
        stream<TxAppTableRequest>  &soTat_AcessReq,
        stream<TxAppTableReply>    &siTat_AcessRep,
        stream<DmCmd>              &soMEM_TxP_WrCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<event>              &txAppStream2eventEng_setEvent
);
