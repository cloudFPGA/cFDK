#include "../toe.hpp"

using namespace hls;

/** @defgroup tx_sar_table TX SAR Table
 *  @ingroup tcp_module
 *  @TODO rename, why SAR
 */
void tx_sar_table(  stream<rxTxSarQuery>&           rxEng2txSar_upd_req,
                    stream<txTxSarQuery>&           txEng2txSar_upd_req,
                    stream<txAppTxSarPush>&         txApp2txSar_app_push,
                    stream<rxTxSarReply>&           txSar2rxEng_upd_rsp,
                    stream<txTxSarReply>&           txSar2txEng_upd_rsp,
                    stream<txSarAckPush>&           txSar2txApp_ack_push);
