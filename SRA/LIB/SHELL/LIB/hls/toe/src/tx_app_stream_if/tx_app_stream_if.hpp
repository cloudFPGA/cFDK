#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;

static const ap_uint<17> ERROR_NOSPACE              = -1;
static const ap_uint<17> ERROR_NOCONNCECTION        = -2;

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


/** @defgroup tx_app_stream_if TX Application Stream Interface
 *  @ingroup app_if
 */
void tx_app_stream_if(
        stream<AxiWord>            &appTxDataReq,
        stream<ap_uint<16> >       &appTxDataReqMetaData,
        stream<sessionState>       &stateTable2txApp_rsp,
        stream<txAppTxSarReply>    &txSar2txApp_upd_rsp, //TODO rename
        stream<ap_int<17> >        &appTxDataRsp,
        stream<ap_uint<16> >       &txApp2stateTable_req,
        stream<txAppTxSarQuery>    &txApp2txSar_upd_req, //TODO rename
        stream<DmCmd>              &txBufferWriteCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<event>              &txAppStream2eventEng_setEvent
);
