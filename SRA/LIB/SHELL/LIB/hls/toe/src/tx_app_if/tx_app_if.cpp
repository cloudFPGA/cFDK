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
 * @brief Tx Application Accept (Taa).
 *   [FIXME - Rename file or move this process in tc_app_interface.cpp]
 *
 * @param[in]  siTRIF_OpnReq,        Open connection request from TCP Role I/F (TRIF).
 * @param[]
 * @param[in]  siSLc_SessLookupRep,  Reply from [SLc]
 * @param[out] siPRt_ActPortStateRep, Active port state reply from [PRt].
 * @param[out] soSLc_SessLookupReq,  Request a session lookup to Session Lookup Controller (SLc).
 * @param[out] soPRt_GetFreePortReq, Request to get a free port to Port Table {PRt).
 * @param[in]  siRXe_SessOpnSts,     Session open status from [RXe].
 * @param[out] soTRIF_SessOpnSts,    Session open status to [TRIF].
 * @param
 * @param
 * @param[out] soEVe_Event,          Event to EventEngine (EVe).
 * @param
 * @param
 *
 * @details
 *  This process performs the creation and tear down of the active connections.
 *   Active connections are the ones opened by the FPGA as client and they make
 *   use of dynamically assigned or ephemeral ports in the range 32,768 to
 *   65,535. The operations performed here are somehow similar to the 'accept'
 *   and 'close' system calls.
 *  The IP tuple of the new connection to open is read from 'siTRIF_OpnReq'.
 *
 *  [TODO]
 *   and then requests a free port number from 'port_table' and fires a SYN
 *   event. Once the connection is established it notifies the application
 *   through 'appOpenConOut' and delivers the Session-ID belonging to the new
 *   connection. (Client connection to remote HOST or FPGA socket (COn).)
 *  If opening of the connection is not successful this is also indicated
 *   through the 'appOpenConOut'. By sending the Session-ID through 'closeConIn'
 *   the application can initiate the teardown of the connection.
 *
 * @ingroup tx_app_if
 ******************************************************************************/
void tx_app_accept(
        stream<AxiSockAddr>         &siTRIF_OpnReq,
        stream<ap_uint<16> >        &closeConnReq,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<TcpPort>             &siPRt_ActPortStateRep,
        stream<sessionState>        &stateTable2txApp_upd_rsp,
        stream<OpenStatus>          &siRXe_SessOpnSts,
        stream<OpenStatus>          &soTRIF_SessOpnSts,
        stream<AxiSocketPair>       &soSLc_SessLookupReq,
        stream<ReqBit>              &soPRt_GetFreePortReq,
        stream<stateQuery>          &txApp2stateTable_upd_req,
        stream<event>               &soEVe_Event,
        stream<OpenStatus>          &rtTimer2txApp_notification,
        AxiIp4Address                regIpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static ap_uint<16> tai_closeSessionID;

    static stream<AxiSockAddr>  localFifo ("localFifo");
    #pragma HLS stream variable=localFifo depth=4

    //OBSSOLETE static enum FsmState {IDLE=0, CLOSE_CONN} tai_fsmState = IDLE;
    static enum TasFsmStates {TAS_IDLE=0, TAS_GET_FREE_PORT, TAS_CLOSE_CONN } tasFsmState = TAS_IDLE;

    switch (tasFsmState) {

    case TAS_IDLE:
        if (!siTRIF_OpnReq.empty() && !soPRt_GetFreePortReq.full()) {
            localFifo.write(siTRIF_OpnReq.read());
            soPRt_GetFreePortReq.write(true);
            tasFsmState = TAS_GET_FREE_PORT;
        }
        else if (!siSLc_SessLookupRep.empty()) {
            // Read the session and check its state
            sessionLookupReply session = siSLc_SessLookupRep.read();
            if (session.hit) {
                soEVe_Event.write(event(SYN, session.sessionID));
                txApp2stateTable_upd_req.write(stateQuery(session.sessionID, SYN_SENT, 1));
            }
            else {
                // Tell the APP that the open connection failed
                soTRIF_SessOpnSts.write(OpenStatus(0, false));
            }
        }
        else if (!siRXe_SessOpnSts.empty())
            soTRIF_SessOpnSts.write(siRXe_SessOpnSts.read());  //Maybe check if we are actually waiting for this one
        else if (!rtTimer2txApp_notification.empty())
            soTRIF_SessOpnSts.write(rtTimer2txApp_notification.read());
        else if(!closeConnReq.empty()) {    // Close Request
            closeConnReq.read(tai_closeSessionID);
            txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID));
            tasFsmState = TAS_CLOSE_CONN;
        }
        break;

    case TAS_GET_FREE_PORT:
        if (!siPRt_ActPortStateRep.empty() && !soSLc_SessLookupReq.full()) {
            //OBSOLETE-20190521 ap_uint<16> freePort = siPRt_ActPortStateRep.read() + 32768; // 0x8000 is already added by PRt
            TcpPort     freePort      = siPRt_ActPortStateRep.read();
            AxiSockAddr axiServerAddr = localFifo.read();
            //OBSOLETE-20181218 soSLc_SessLookupReq.write(
            //        fourTuple(regIpAddress, byteSwap32(server_addr.ip_address),
            //        byteSwap16(freePort), byteSwap16(server_addr.ip_port)));
            soSLc_SessLookupReq.write(AxiSocketPair(AxiSockAddr(regIpAddress,       byteSwap16(freePort)),
                                                    AxiSockAddr(axiServerAddr.addr, axiServerAddr.port)));
            tasFsmState = TAS_IDLE;
        }
        break;

    case TAS_CLOSE_CONN:
        if (!stateTable2txApp_upd_rsp.empty()) {
            sessionState state = stateTable2txApp_upd_rsp.read();
            //TODO might add CLOSE_WAIT here???
            if ((state == ESTABLISHED) || (state == FIN_WAIT_2) || (state == FIN_WAIT_1)) { //TODO Why if FIN already SENT
                txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID, FIN_WAIT_1, 1));
                soEVe_Event.write(event(FIN, tai_closeSessionID));
            }
            else
                txApp2stateTable_upd_req.write(stateQuery(tai_closeSessionID, state, 1)); // Have to release lock
            tasFsmState = TAS_IDLE;
        }
        break;
    }
}
