#include "../toe.hpp"

using namespace hls;

/** @defgroup rx_app_if RX Application Interface
 *  @ingroup app_if
 *
 */
void rx_app_if( stream<ap_uint<16> >&               appListenPortReq,
                stream<bool>&                       portTable2rxApp_listen_rsp,
                stream<bool>&                       appListenPortRsp,
                stream<ap_uint<16> >&               rxApp2portTable_listen_req);
