#include "rx_app_if.hpp"

using namespace hls;


/*****************************************************************************
 * @brief This application interface is used to open passive connections.
 *          [TODO -  Consider renaming]
 *
 * @param[in]  siTRIF_LsnReq,         TCP listen port request from TRIF.
 * @param[out] soTRIF_LsnAck,         TCP listen port acknowledge to TRIF.
 * @param[out] soPRt_LsnReq,          Listen port request to PortTable (PRt).
 * @param[in]  siPRt_LsnAck,          Listen port acknowledge from [PRt].
 *
 ******************************************************************************/
// TODO this does not seem to be very necessary
void rx_app_if(
        stream<TcpPort>                    &siTRIF_LsnReq,
        stream<AckBit>                     &soTRIF_LsnAck,
        stream<TcpPort>                    &soPRt_LsnReq,
        stream<AckBit>                     &siPRt_LsnAck
        // This is disabled for the time being, because it adds complexity/potential issues
        //stream<ap_uint<16> >             &appStopListeningIn,
)
        //stream<ap_uint<16> >             &rxAppPortTableCloseIn,)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                rai_wait = false;
    #pragma HLS reset variable=rai_wait

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TcpPort            listenPort;
    AckBit             listening;

    // TODO maybe do a state machine

    // Listening Port Open, why not asynchron??
    if (!siTRIF_LsnReq.empty() && !rai_wait) {
        soPRt_LsnReq.write(siTRIF_LsnReq.read());
        rai_wait = true;
    }
    else if (!siPRt_LsnAck.empty() && rai_wait) {
        siPRt_LsnAck.read(listening);
        soTRIF_LsnAck.write(listening);
        rai_wait = false;
    }

    // Listening Port Close
    /*if (!appStopListeningIn.empty())
    {
        rxAppPortTableCloseIn.write(appStopListeningIn.read());
    }*/
}
