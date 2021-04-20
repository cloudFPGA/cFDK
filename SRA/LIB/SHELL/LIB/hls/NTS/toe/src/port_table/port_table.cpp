/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : port_table.cpp
 * @brief      : Port Table (PRt) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "port_table.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_ORM | TRACE_FPT)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/PRt"

#define TRACE_OFF  0x0000
#define TRACE_IRR 1 <<  1
#define TRACE_ORM 1 <<  2
#define TRACE_LPT 1 <<  3
#define TRACE_FPT 1 <<  4
#define TRACE_RDY 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief A two inputs 'AND' gate.
 * *****************************************************************************/
template<class T> void pAnd2(
        T  &pi1,
        T  &pi2,
        T  &po)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off

    po = pi1 & pi2;
}

/*******************************************************************************
 * @brief Ready Logic (Rdy)
 *
 * @param[in]  piLpt_Ready  The ready signal from ListenPortTable (Lpt).
 * @param[in]  piFpt_Ready  The ready signal from FreePortTable (Fpt).
 * @param[out] poTOE_Ready  The ready signal of PortTable (PRt) to [TOE].
 *
 * @details
 *  This process generates a ready signal indicating that the logic of the
 *   PortTable (PRt) is ready.
 *******************************************************************************/
void pReady(
    StsBool    &piLpt_Ready,
    StsBool    &piFpt_Ready,
    StsBool    &poTOE_Ready)
{
    const char *myName = concat3(THIS_NAME, "/", "Rdy");

    poTOE_Ready = piLpt_Ready and piFpt_Ready;

    if (DEBUG_LEVEL & TRACE_RDY) {
        if (poTOE_Ready)
            printInfo(myName, "Process [PRt] is ready.\n");
    }
}

/*******************************************************************************
 * @brief Listening Port Table (Lpt)
 *
 *  @param[out] poRdy_Ready            Ready signal to ReadyLogic (Rdy).
 *  @param[in]  siRAi_OpenLsnPortReq   Open port request from RxAppInterface (RAi).
 *  @param[out] soRAi_OpenLsnPortRep   Open port reply to [RAi].
 *  @param[in]  siIrr_GetPortStateCmd  Request port state from InputRequestRouter (Irr).
 *  @param[out] soOrm_GetPortStateRsp  Port state response to OutputReplyMultiplexer (Orm).
 *
 * @details
 *  This process keeps track of the port opened in listening mode. It consists
 *   of a port table which is accessed by two remote processes:
 *   1) the RxAppInterface (TAi) when the application (APP) is requesting to
 *      open a port in listening mode.
 *   2) the RxEngine (RXe) via the local InputRequestRouter (Irr) process, when
 *      the [RXe] is requesting the status of a destination port.
 *
 *  If a read and a write operation occur at the same time, the write operation
 *   (.i.e the opening of the port) takes precedence over the read operation.
 *******************************************************************************/
void pListeningPortTable(
        StsBool              &poRdy_Ready,
        stream<TcpPort>      &siRAi_OpenLsnPortReq,
        stream<RepBit>       &soRAi_OpenLsnPortRep,
        stream<TcpStaPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Lpt");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static PortState                LISTEN_PORT_TABLE[0x8000];
    #pragma HLS RESOURCE   variable=LISTEN_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=LISTEN_PORT_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                lpt_isLPtInit=false;
    #pragma HLS reset variable=lpt_isLPtInit
    static TcpPort             lpt_lsnPortNum=0;
    #pragma HLS reset variable=lpt_lsnPortNum

    // This table must be cleared upon reset
    if (!lpt_isLPtInit) {
        LISTEN_PORT_TABLE[lpt_lsnPortNum(14, 0)] = STS_CLOSED;
        lpt_lsnPortNum += 1;
        if (lpt_lsnPortNum == 0x8000) {
          lpt_isLPtInit = true;
          if (DEBUG_LEVEL & TRACE_LPT) {
              printInfo(myName, "Done with initialization of LISTEN_PORT_TABLE.\n");
          }
        }
    }
    else {
        if (!siRAi_OpenLsnPortReq.empty() and !soRAi_OpenLsnPortRep.full()) {
            siRAi_OpenLsnPortReq.read(lpt_lsnPortNum);
            // [TODO] Let's add a specific bit to specifically open/close a port.
            if (lpt_lsnPortNum < 0x8000) {
                // Listening port number falls in the range [0..32,767]
                // We can set the listening port table entry to true
                LISTEN_PORT_TABLE[lpt_lsnPortNum] = STS_OPENED;
                // Sent reply to RAi
                soRAi_OpenLsnPortRep.write(STS_OPENED);
                if (DEBUG_LEVEL & TRACE_LPT)
                printInfo(myName, "[RAi] is requesting to open port #%d in listen mode.\n",
                          lpt_lsnPortNum.to_uint());
            }
            else {
                soRAi_OpenLsnPortRep.write(STS_CLOSED);
            } 
        }
        else if (!siIrr_GetPortStateCmd.empty()) {
            // Warning: Cannot add "and !soOrm_GetPortStateRsp.full()" here because
            //  it increases the task interval from 1 to 2!
            TcpStaPort staticPortNum = siIrr_GetPortStateCmd.read();
            // Sent status of that portNum to Orm
            soOrm_GetPortStateRsp.write(LISTEN_PORT_TABLE[staticPortNum]);
            if (DEBUG_LEVEL & TRACE_LPT)
                printInfo(myName, "[RXe] is querying the state of listen port #%d \n",
                          staticPortNum.to_uint());
        }
    }
    //-- ALWAYS
    poRdy_Ready = lpt_isLPtInit;
}

/*******************************************************************************
 * @brief Free Port Table (Fpt)
 *
 * @param[out] poRdy_Ready            Ready signal to ReadyLogic (Rdy).
 * @param[in]  siSLc_CloseActPortCmd  Command to release a port from SessionLookupController (SLc).
 * @param[in]  siIrr_GetPortStateCmd  Command to get port state from InputRequestRouter (Irr).
 * @param[out] soOrm_GetPortStateRsp  Response to get port state to OutputReplyMux (Orm).
 * @param[in]  siTAi_GetFreePortReq   Get free port request from TxAppInterface (TAi).
 * @param[out] soTAi_GetFreePortRep   Get free port reply to [TAi].
 *
 * @details
 *  This process keeps track of the opened source ports used in by active
 *   connections. It consists of a  table containing a free list of ephemeral
 *   ports (also referred as dynamic ports) that the TOE uses as local source
 *   ports when it opens new active connections.
 *  A dynamic source port is implicitly assigned in the range (32,768 to 65,535)
 *   and there is no way (or reason) to explicitly specify such a port number.
 *  This table is accessed by one local [Irr] and 2 remote [SLc][TAi] processes.
 *
 * @Warning
 *  The initial design point assumed a maximum of 10K sessions, which is much
 *  less than the possible 32K active ports.
 *******************************************************************************/
void pFreePortTable(
        StsBool              &poRdy_Ready,
        stream<TcpPort>      &siSLc_CloseActPortCmd,
        stream<TcpDynPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp,
        stream<ReqBit>       &siTAi_GetFreePortReq,
        stream<TcpPort>      &soTAi_GetFreePortRep)

{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Fpt");

    //-- STATIC ARRAYS ---------------------------------------------------------
    static PortRange                ACTIVE_PORT_TABLE[0x8000];
    #pragma HLS RESOURCE   variable=ACTIVE_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ACTIVE_PORT_TABLE inter false

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                     fpt_isFPtInit=false;
    #pragma HLS reset      variable=fpt_isFPtInit
    static bool                     fpt_searching=false;
    #pragma HLS reset      variable=fpt_searching
    static bool                     fpt_eval=false;
    #pragma HLS reset      variable=fpt_eval
    static TcpDynPort               fpt_dynPortNum=0x7FFF;
    #pragma HLS reset      variable=fpt_dynPortNum

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static bool          portState = ACT_USED_PORT;
    #pragma HLS DEPENDENCE variable=portState inter false

    // The table is a free list that must be initialized upon reset
    if (!fpt_isFPtInit) {
        ACTIVE_PORT_TABLE[fpt_dynPortNum] = ACT_FREE_PORT;
        fpt_dynPortNum -= 1;
        if (fpt_dynPortNum == 0) {
            fpt_isFPtInit = true;
            if (DEBUG_LEVEL & TRACE_FPT) {
                printInfo(myName, "Done with initialization of ACTIVE_PORT_TABLE.\n");
            }
        }
    }
    else {
        if (fpt_searching) {
            portState = ACTIVE_PORT_TABLE[fpt_dynPortNum];
            fpt_searching = false;
            fpt_eval      = true;
        }
        else if (fpt_eval) {
            if (portState == ACT_FREE_PORT) {
                // Found a free entry port in the table
                if (!soTAi_GetFreePortRep.full()) {
                    // Stop evaluating. Set port number to USED and forward to [TAi]
                    fpt_eval = false;
                    ACTIVE_PORT_TABLE[fpt_dynPortNum] = ACT_USED_PORT;
                    // Add 0x8000 before sending back
                    soTAi_GetFreePortRep.write(0x8000 + fpt_dynPortNum);
                }
            }
            else {
                // Continue searching
            	fpt_searching = true;
            }
            fpt_dynPortNum++;
        }
        else if (!siIrr_GetPortStateCmd.empty()) {
            // Warning: Cannot add "and !soOrm_GetPortStateRsp.full()" here because
            //  it increases the task interval from 1 to 2!
            TcpDynPort portNum = siIrr_GetPortStateCmd.read();
            soOrm_GetPortStateRsp.write(ACTIVE_PORT_TABLE[portNum]);
        }
        else if (!siTAi_GetFreePortReq.empty()) {
            siTAi_GetFreePortReq.read();
            fpt_searching = true;
        }
        else if (!siSLc_CloseActPortCmd.empty()) {
            TcpPort tcpPort = siSLc_CloseActPortCmd.read();
            if (tcpPort.bit(15) == 1) {
                // Assess that port number >= 0x8000
                ACTIVE_PORT_TABLE[tcpPort.range(14, 0)] = ACT_FREE_PORT;
            }
          #ifndef __SYNTHESIS__
            else {
                printError(myName, "SLc is not allowed to release a static port.\n");
                exit(1);
            }
          #endif
        }
    }
    // ALWAYS
    poRdy_Ready = fpt_isFPtInit;
}

/*******************************************************************************
 * @brief Input Request Router (Irr)
 *
 * @param[in]  siRXe_GetPortStateCmd     Get state of a port from RxEngine (RXe).
 * @param[out] soLpt_GetLsnPortStateCmd  Get state of a listen port to ListenPortTable (LPt).
 * @param[out] soFpt_GetActPortStateCmd  Get state of an active port to FreePortTable (Fpt).
 * @param[out] soOrm_QueryRange          Range of the forwarded query to OutputReplyMux (Orm).
 *
 * @details
 *  This process checks the range of the port to be looked-up and forwards that
 *   request to the corresponding lookup table ([Lpt] or [Fpt]).
 *  The [PRt] supports two port ranges; one for static ports (0 to 32,767) which
 *   are used for listening ports, and one for dynamically assigned or
 *   ephemeral ports (32,768 to 65,535) which are used for active connections.
 *******************************************************************************/
void pInputRequestRouter(
        stream<TcpPort>           &siRXe_GetPortStateCmd,
        stream<TcpStaPort>        &soLpt_GetLsnPortStateCmd,
        stream<TcpDynPort>        &soFpt_GetActPortStateCmd,
        stream<PortRange>         &soOrm_QueryRange)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Irr");

    // Forward request according to port number,
    if (!siRXe_GetPortStateCmd.empty()) {
        TcpPort portToCheck = siRXe_GetPortStateCmd.read();
        if (DEBUG_LEVEL & TRACE_IRR) {
            printInfo(myName, "[RXe] is requesting the state of port #%d.\n", portToCheck.to_int());
        }
        if (portToCheck < 0x8000) {
            // Listening ports are in the range [0x0000..0x7FFF]
            soLpt_GetLsnPortStateCmd.write(portToCheck.range(14, 0));
            soOrm_QueryRange.write(LISTEN_PORT);
        }
        else {
            // Active ports are in the range [0x8000..0xFFFF]
            soFpt_GetActPortStateCmd.write(portToCheck.range(14, 0));
            soOrm_QueryRange.write(ACTIVE_PORT);
        }
    }
}


/******************************************************************************
 * @brief Output Reply Multiplexer (Orm)
 *
 * @param[in]  siIrr_QueryRange          Range of the query from InputRequestRouter (Irr).
 * @param[in]  siLpt_GetLsnPortStateRsp  Listen port state response from ListenPortTable (LPt).
 * @param[in]  siFpt_GetActPortStateRsp  Active port state response from FreePortTable (Fpt).
 * @param[out] soRXe_GetPortStateRsp     Port state response to RxEngine (RXe).
 *
 * @details
 *  This process orders the lookup replies before sending them back to the
 *   RxEngine (RXe).
 *******************************************************************************/
void pOutputReplyMultiplexer(
        stream<PortRange> &siIrr_QueryRange,
        stream<RspBit>    &siLpt_GetLsnPortStateRsp,
        stream<RspBit>    &siFpt_GetActPortStateRsp,
        stream<RspBit>    &soRXe_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ORM_WAIT_FOR_QUERY_FROM_Irr=0,
                            ORM_FORWARD_LSN_PORT_STATE_RSP,
                            ORM_FORWARD_ACT_PORT_STATE_RSP } orm_fsmState=ORM_WAIT_FOR_QUERY_FROM_Irr;

    // Read out responses from tables in order and merge them
    switch (orm_fsmState) {
    case ORM_WAIT_FOR_QUERY_FROM_Irr:
        if (!siIrr_QueryRange.empty()) {
            PortRange qryType = siIrr_QueryRange.read();
            if (qryType == LISTEN_PORT)
                orm_fsmState = ORM_FORWARD_LSN_PORT_STATE_RSP;
            else
                orm_fsmState = ORM_FORWARD_ACT_PORT_STATE_RSP;
        }
        break;
    case ORM_FORWARD_LSN_PORT_STATE_RSP:
        if (!siLpt_GetLsnPortStateRsp.empty() and !soRXe_GetPortStateRsp.full()) {
            soRXe_GetPortStateRsp.write(siLpt_GetLsnPortStateRsp.read());
            orm_fsmState = ORM_WAIT_FOR_QUERY_FROM_Irr;
        }
        break;
    case ORM_FORWARD_ACT_PORT_STATE_RSP:
        if (!siFpt_GetActPortStateRsp.empty() and !soRXe_GetPortStateRsp.full()) {
            soRXe_GetPortStateRsp.write(siFpt_GetActPortStateRsp.read());
            orm_fsmState = ORM_WAIT_FOR_QUERY_FROM_Irr;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Port Table (PRt)
 *
 * @param[out] poTOE_Ready            The ready logic signal of this process to [TOE].
 * @param[in]  siRXe_GetPortStateReq  Port state request from RxEngine (RXe).
 * @param[out] soRXe_GetPortStateRep  Port state reply to [RXe].
 * @param[in]  siRAi_OpenLsnPortReq   Open port request from RxAppInterface (RAi).
 * @param[out] soRAi_OpenLsnPortRep   Open port reply to [RAi].
 * @param[in]  siTAi_GetFreePortReq   Free port request from TxAppInterface (TAi).
 * @param[out] soTAi_GetFreePortRep   Free port reply o [TAi].
 * @param[in]  siSLc_CloseActPortCmd  Command to close an active port from SessionLookupController (SLc).
 *
 * @details
 *  This process keeps track of the TCP port numbers which are in use and
 *   therefore opened. It maintains two port ranges based on two tables of
 *   32768 x 1-bit:
 *   - One for static ports (0 to 32,767) which are used for listening ports,
 *   - One for dynamically assigned or ephemeral ports (32,768 to 65,535)
 *     which are used for active connections open by [TOE].
 *  A port can be either closed, in listen, or in active state.
 *  The PortTable receives requests from 3 sources:
 *   1) opening (listening) requests from the RxApplication Interface (RAi),
 *   2) requests to check if a given port is open from the RxEngine (RXe),
 *   3) requests for a free port from the TxApplicationInterface (TAi).
 *******************************************************************************/
void port_table(
        StsBool                 &poTOE_Ready,
        stream<TcpPort>         &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        stream<TcpPort>         &siRAi_OpenLsnPortReq,
        stream<AckBit>          &soRAi_OpenLsnPortAck,
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &soTAi_GetFreePortRep,
        stream<TcpPort>         &siSLc_CloseActPortCmd)
{

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE

    //--------------------------------------------------------------------------
    //-- LOCAL SIGNALS AND STREAMS
    //--    (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Input Request Router (Irr)
    static stream<TcpStaPort>   ssIrrToLpt_GetLsnPortStateCmd ("ssIrrToLpt_GetLsnPortStateCmd");
    #pragma HLS STREAM variable=ssIrrToLpt_GetLsnPortStateCmd depth=2

    static stream<TcpDynPort>   ssIrrToFpt_GetActPortStateCmd ("ssIrrToFpt_GetActPortStateCmd");
    #pragma HLS STREAM variable=ssIrrToFpt_GetActPortStateCmd depth=2

    static stream<bool>         ssIrrToOrm_QueryRange         ("ssIrrToOrm_QueryRange");
    #pragma HLS STREAM variable=ssIrrToOrm_QueryRange         depth=4

    //-- Listening Port Table (Lpt)
    StsBool                      sLptToAnd2_Ready;
    static stream<RspBit>       ssLptToOrm_GetLsnPortStateRsp ("ssLptToOrm_GetLsnPortStateRsp");
    #pragma HLS STREAM variable=ssLptToOrm_GetLsnPortStateRsp depth=2

    //-- Free Port Table (Fpt)
    StsBool                      sFptToAnd2_Ready;
    static stream<RspBit>       ssFptToOrm_GetActPortStateRsp ("ssFptToOrm_GetActPortStateRsp");
    #pragma HLS STREAM variable=ssFptToOrm_GetActPortStateRsp depth=2

    //--------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //--------------------------------------------------------------------------

    pListeningPortTable(
            sLptToAnd2_Ready,
            siRAi_OpenLsnPortReq,
            soRAi_OpenLsnPortAck,
            ssIrrToLpt_GetLsnPortStateCmd,
            ssLptToOrm_GetLsnPortStateRsp);

    pFreePortTable(
            sFptToAnd2_Ready,
            siSLc_CloseActPortCmd,
            ssIrrToFpt_GetActPortStateCmd,
            ssFptToOrm_GetActPortStateRsp,
            siTAi_GetFreePortReq,
            soTAi_GetFreePortRep);

    // Routes the input requests
    pInputRequestRouter(
            siRXe_GetPortStateReq,
            ssIrrToLpt_GetLsnPortStateCmd,
            ssIrrToFpt_GetActPortStateCmd,
            ssIrrToOrm_QueryRange);

    pOutputReplyMultiplexer(
            ssIrrToOrm_QueryRange,
            ssLptToOrm_GetLsnPortStateRsp,
            ssFptToOrm_GetActPortStateRsp,
            soRXe_GetPortStateRep);

    pReady(
            sLptToAnd2_Ready,
            sFptToAnd2_Ready,
            poTOE_Ready);

}

/*! \} */
