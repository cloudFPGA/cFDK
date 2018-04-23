#include "rx_app_if.hpp"

using namespace hls;

/** @ingroup rx_app_if
 *  This application interface is used to open passive connections
 *  @param[in]		appListeningIn
 *  @param[in]		appStopListeningIn
 *  @param[in]		rxAppPorTableListenIn
 *  @param[in]		rxAppPortTableCloseIn
 *  @param[out]		appListeningOut
 *  @param[out]		rxAppPorTableListenOut
 */
// TODO this does not seem to be very necessary
void rx_app_if(	stream<ap_uint<16> >&				appListenPortReq,
				// This is disabled for the time being, because it adds complexity/potential issues
				//stream<ap_uint<16> >&				appStopListeningIn,
				stream<bool>&						portTable2rxApp_listen_rsp,
				stream<bool>&						appListenPortRsp,
				stream<ap_uint<16> >&				rxApp2portTable_listen_req)
				//stream<ap_uint<16> >&				rxAppPortTableCloseIn,)
{
#pragma HLS PIPELINE II=1

#pragma HLS resource core=AXI4Stream variable=appListenPortRsp metadata="-bus_bundle m_axis_listen_port_rsp"
#pragma HLS resource core=AXI4Stream variable=appListenPortReq metadata="-bus_bundle s_axis_listen_port_req"

	static bool rai_wait = false;

	static ap_uint<16> rai_counter = 0;

	ap_uint<16> tempPort;
	ap_uint<16> listenPort;
	bool listening;

	// TODO maybe do a state machine
	// Listening Port Open, why not asynchron??
	if (!appListenPortReq.empty() && !rai_wait)
	{
		//appListenPortReq.read(tempPort);
		//listenPort(7, 0) = tempPort(15, 8);
		//listenPort(15, 8) = tempPort(7, 0);
		rxApp2portTable_listen_req.write(appListenPortReq.read());
		rai_wait = true;
	}
	else if (!portTable2rxApp_listen_rsp.empty() && rai_wait)
	{
		portTable2rxApp_listen_rsp.read(listening);
		appListenPortRsp.write(listening);
		rai_wait = false;
	}

	// Listening Port Close
	/*if (!appStopListeningIn.empty())
	{
		rxAppPortTableCloseIn.write(appStopListeningIn.read());
	}*/
}
