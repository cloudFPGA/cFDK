#include "../toe.hpp"

using namespace hls;

/** @defgroup rx_sar_table RX SAR Table
 *  @ingroup tcp_module
 *  @TODO rename why SAR
 */
void rx_sar_table(	stream<rxSarRecvd>&			rxEng2rxSar_upd_req,
					stream<rxSarAppd>&			rxApp2rxSar_upd_req,
					stream<ap_uint<16> >&		txEng2rxSar_req, //read only
					stream<rxSarEntry>&			rxSar2rxEng_upd_rsp,
					stream<rxSarAppd>&			rxSar2rxApp_upd_rsp,
					stream<rxSarEntry>&			rxSar2txEng_rsp);
