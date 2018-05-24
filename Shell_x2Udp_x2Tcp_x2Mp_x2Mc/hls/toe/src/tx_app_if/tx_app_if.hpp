#include "../toe.hpp"

using namespace hls;

/** @defgroup tx_app_if TX Application Interface
 *  @ingroup app_if
 */
void tx_app_if(	stream<ipTuple>&				appOpenConnReq,
				stream<ap_uint<16> >&			closeConnReq,
				stream<sessionLookupReply>&		sLookup2txApp_rsp,
				stream<ap_uint<16> >&			portTable2txApp_port_rsp,
				stream<sessionState>&			stateTable2txApp_upd_rsp,
				stream<openStatus>&				conEstablishedIn, //alter
				stream<openStatus>&				appOpenConnRsp,
				stream<fourTuple>&				txApp2sLookup_req,
				stream<ap_uint<1> >&			txApp2portTable_port_req,
				stream<stateQuery>&				txApp2stateTable_upd_req,
				stream<event>&					txApp2eventEng_setEvent,
				stream<openStatus>&				rtTimer2txApp_notification,
				ap_uint<32>						regIpAddress);
