#include "rx_app_if.hpp"

using namespace hls;


/*****************************************************************************
 * @brief This application interface is used to open passive connections.
 *          [TODO -  Consider renaming]
 *
 *  @param[in]  siTRIF_ListenPortReq,  Request from TCP Role Interface (TRIF).
 *  @param[in]  appStopListeningIn
 *  @param[in]  rxAppPorTableListenIn
 *  @param[in]  rxAppPortTableCloseIn
 *  @param[out] appListeningOut
 *  @param[out] soPRt_ListenReq,       Listen request to Port Table (PRt).
 *
 * @ingroup rx_app_if
 ******************************************************************************/
// TODO this does not seem to be very necessary
void rx_app_if(
        stream<TcpPort>                    &siTRIF_ListenPortReq,
        // This is disabled for the time being, because it adds complexity/potential issues
        //stream<ap_uint<16> >             &appStopListeningIn,
        stream<bool>                       &portTable2rxApp_listen_rsp,
        stream<bool>                       &appListenPortRsp,
        stream<TcpPort>                    &soPRt_ListenReq)
        //stream<ap_uint<16> >             &rxAppPortTableCloseIn,)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    //OBSOLETE-20181120 #pragma HLS resource core=AXI4Stream variable=appListenPortRsp metadata="-bus_bundle m_axis_listen_port_rsp"
	//OBSOLETE-20181120 #pragma HLS resource core=AXI4Stream variable=appListenPortReq metadata="-bus_bundle s_axis_listen_port_req"

    static bool rai_wait = false;

    static ap_uint<16> rai_counter = 0;

    ap_uint<16> tempPort;
    ap_uint<16> listenPort;
    bool listening;

    // TODO maybe do a state machine

    // Listening Port Open, why not asynchron??
    if (!siTRIF_ListenPortReq.empty() && !rai_wait) {
        soPRt_ListenReq.write(siTRIF_ListenPortReq.read());
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
