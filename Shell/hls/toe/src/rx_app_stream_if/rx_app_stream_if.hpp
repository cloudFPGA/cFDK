#include "../toe.hpp"

using namespace hls;

/** @defgroup rx_app_stream_if RX Application Stream Interface
 *  @ingroup app_if
 */
void rx_app_stream_if(stream<appReadRequest>&	appRxDataReq,
					  stream<rxSarAppd>&		rxSar2rxApp_upd_rsp,
					  stream<ap_uint<16> >&		appRxDataRspMetadata,
					  stream<rxSarAppd>&		rxApp2rxSar_upd_req,
					  stream<mmCmd>&			rxBufferReadCmd);
