#include "tx_app_if.hpp"

using namespace hls;

/** @ingroup tx_app_if
 *  This interface exposes the creation and tear down of connetions to the application.
 *  The IP tuple for a new connection is read from @param appOpenConIn, the interface then
 *  requests a free port number from the @ref port_table and fires a SYN event. Once the connetion
 *  is established it notifies the application through @p appOpenConOut and delivers the Session-ID
 *  belonging to the new connection.
 *  If opening of the connection is not successful this is also indicated through the @p
 *  appOpenConOut.
 *  By sending the Session-ID through @p closeConIn the application can initiate the teardown of
 *  the connection.
 *  @param[in]      appOpenConIn
 *  @param[in]      closeConIn
 *  @param[in]      txAppSessionLookupIn
 *  @param[in]      portTableIn
 *  @param[in]      stateTableIn
 *  @param[in]      conEstablishedIn
 *  @param[out]     appOpenConOut
 *  @param[out]     txAppSessionLookupOut
 *  @param[out]     portTableOut
 *  @param[out]     stateTableOut
 *  @param[out]     eventFifoOut
 *  @TODO reorganize code
 */
void tx_app_if( stream<ipTuple>&                appOpenConnReq,
                stream<ap_uint<16> >&           closeConnReq,
                stream<sessionLookupReply>&     sLookup2txApp_rsp,
                stream<ap_uint<16> >&           portTable2txApp_port_rsp,
                stream<sessionState>&           stateTable2txApp_upd_rsp,
                stream<openStatus>&             conEstablishedIn, //alter
                stream<openStatus>&             appOpenConnRsp,
                stream<fourTuple>&              txApp2sLookup_req,
                stream<ap_uint<1> >&            txApp2portTable_port_req,
                stream<stateQuery>&             txApp2stateTable_upd_req,
                stream<event>&                  txApp2eventEng_setEvent,
                stream<openStatus>&             rtTimer2txApp_notification,
                ap_uint<32>                     regIpAddress) {
#pragma HLS INLINE off
#pragma HLS pipeline II=1

    enum taiFsmStateType {IDLE, CLOSE_CONN};
    static taiFsmStateType tai_fsmState = IDLE;
    static ap_uint<16> tai_closeSessionID;
    static stream<ipTuple> txAppIpTupleBuffer("txAppIpTupleBuffer");
    #pragma HLS stream variable=txAppIpTupleBuffer          depth=4

    switch (tai_fsmState) {
    case IDLE:
        if (!appOpenConnReq.empty() && !txApp2portTable_port_req.full()) {
            txAppIpTupleBuffer.write(appOpenConnReq.read());
            txApp2portTable_port_req.write(1);
        }
        else if (!sLookup2txApp_rsp.empty())    {
            sessionLookupReply session = sLookup2txApp_rsp.read();                          // Read session
            if (session.hit) {                                                              // Get session state
                txApp2eventEng_setEvent.write(event(SYN, session.sessionID));
                txApp2stateTable_upd_req.write(stateQuery(session.sessionID, SYN_SENT, 1));
            }
            else
                appOpenConnRsp.write(openStatus(0, false));
        }
        else if (!conEstablishedIn.empty())
            appOpenConnRsp.write(conEstablishedIn.read());  //Maybe check if we are actually waiting for this one
        else if (!rtTimer2txApp_notification.empty())
            appOpenConnRsp.write(rtTimer2txApp_notification.read());
        else if(!closeConnReq.empty()) {    // Close Request
            closeConnReq.read(tai_closeSessionID);
            txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID));
            tai_fsmState = CLOSE_CONN;
        }
        else if (!portTable2txApp_port_rsp.empty() && !txApp2sLookup_req.full()) {
            ap_uint<16> freePort    = portTable2txApp_port_rsp.read() + 32768;
            ipTuple server_addr     = txAppIpTupleBuffer.read();
            txApp2sLookup_req.write(fourTuple(regIpAddress, byteSwap32(server_addr.ip_address), byteSwap16(freePort), byteSwap16(server_addr.ip_port))); // Implicit creationAllowed <= true
        }
        break;
    case CLOSE_CONN:
        if (!stateTable2txApp_upd_rsp.empty()) {
            sessionState state = stateTable2txApp_upd_rsp.read();
            //TODO might add CLOSE_WAIT here???
            if ((state == ESTABLISHED) || (state == FIN_WAIT_2) || (state == FIN_WAIT_1)) { //TODO Why if FIN already SENT
                txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID, FIN_WAIT_1, 1));
                txApp2eventEng_setEvent.write(event(FIN, tai_closeSessionID));
            }
            else
                txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID, state, 1)); // Have to release lock
            tai_fsmState = IDLE;
        }
        break;
    } //switch
}
