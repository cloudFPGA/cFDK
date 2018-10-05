#include "../toe.hpp"

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


/** @defgroup tx_app_stream_if TX Application Stream Interface
 *  @ingroup app_if
 */
void tx_app_stream_if(  stream<ap_uint<16> >&           appTxDataReqMetaData,
                        stream<axiWord>&                appTxDataReq,
                        stream<sessionState>&           stateTable2txApp_rsp,
                        stream<txAppTxSarReply>&        txSar2txApp_upd_rsp, //TODO rename
                        stream<ap_int<17> >&            appTxDataRsp,
                        stream<ap_uint<16> >&           txApp2stateTable_req,
                        stream<txAppTxSarQuery>&        txApp2txSar_upd_req, //TODO rename
                        stream<mmCmd>&                  txBufferWriteCmd,
                        stream<axiWord>&                txBufferWriteData,
                        stream<event>&                  txAppStream2eventEng_setEvent);
