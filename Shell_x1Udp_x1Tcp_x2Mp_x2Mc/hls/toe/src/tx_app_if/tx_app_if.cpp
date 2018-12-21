/*****************************************************************************
 * @file       : tx_app_if.cpp
 * @brief      : Tx Application Interface (Tai) // [FIXME - Need a better name]
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *****************************************************************************/

#include "tx_app_if.hpp"

using namespace hls;


/*****************************************************************************
 * @brief Tx Application Status Handler (Tas).
 *
 * @param[in]  siTRIF_OpnReq,       Open connection request from TCP Role I/F (TRIF).
 * @param[]
 * @param[]
 *
 *
 *
 * @param[out] soSLc_SessLookupReq, Session lookup request to Session Lookup Controller(SLc).
 *
 * @details
 *  This interface exposes the creation and tear down of connections to the
 *   application. The IP tuple for a new connection is read from 'appOpenConIn'
 *   and then requests a free port number from 'port_table' and fires a SYN
 *   event. Once the connection is established it notifies the application
 *   through 'appOpenConOut' and delivers the Session-ID belonging to the new
 *   connection.
 *  If opening of the connection is not successful this is also indicated
 *   through the 'appOpenConOut'. By sending the Session-ID through 'closeConIn'
 *   the application can initiate the teardown of the connection.
 *
 * @ingroup tx_app_if
 ******************************************************************************/
void tx_app_if(
        stream<AxiSockAddr>         &siTRIF_OpnReq,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &sLookup2txApp_rsp,
        stream<ap_uint<16> >        &portTable2txApp_port_rsp,
        stream<sessionState>        &stateTable2txApp_upd_rsp,
        stream<OpenStatus>          &conEstablishedIn, //alter
        stream<OpenStatus>          &appOpenConnRsp,
        stream<AxiSocketPair>       &soSLc_SessLookupReq,
        stream<ap_uint<1> >         &txApp2portTable_port_req,
        stream<stateQuery>          &txApp2stateTable_upd_req,
        stream<event>               &txApp2eventEng_setEvent,
        stream<OpenStatus>          &rtTimer2txApp_notification,
		AxiIp4Address                regIpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static ap_uint<16> tai_closeSessionID;

    static stream<AxiSockAddr>  localFifo ("localFifo");
    #pragma HLS stream variable=localFifo depth=4

    static enum FsmState {IDLE=0, CLOSE_CONN} tai_fsmState = IDLE;

    switch (tai_fsmState) {

    case IDLE:
        if (!siTRIF_OpnReq.empty() && !txApp2portTable_port_req.full()) {
            localFifo.write(siTRIF_OpnReq.read());
            txApp2portTable_port_req.write(1);
        }
        else if (!sLookup2txApp_rsp.empty())    {
            sessionLookupReply session = sLookup2txApp_rsp.read();                          // Read session
            if (session.hit) {                                                              // Get session state
                txApp2eventEng_setEvent.write(event(SYN, session.sessionID));
                txApp2stateTable_upd_req.write(stateQuery(session.sessionID, SYN_SENT, 1));
            }
            else
                appOpenConnRsp.write(OpenStatus(0, false));
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
        else if (!portTable2txApp_port_rsp.empty() && !soSLc_SessLookupReq.full()) {
            ap_uint<16> freePort    = portTable2txApp_port_rsp.read() + 32768;
            AxiSockAddr axiServerAddr = localFifo.read();
            //OBSOLETE-20181218 soSLc_SessLookupReq.write(fourTuple(regIpAddress, byteSwap32(server_addr.ip_address), byteSwap16(freePort), byteSwap16(server_addr.ip_port))); // Implicit creationAllowed <= true
            soSLc_SessLookupReq.write(AxiSocketPair(AxiSockAddr(regIpAddress,       byteSwap16(freePort)),
                                                    AxiSockAddr(axiServerAddr.addr, axiServerAddr.port)));
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
    }
}
