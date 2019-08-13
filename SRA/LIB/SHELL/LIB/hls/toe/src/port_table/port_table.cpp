/*****************************************************************************
 * @file       : port_table.cpp
 * @brief      : TCP Port Table (PRt) management for TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details
 *  The TCP port table keeps track of the TCP port numbers which are in use.
 *  The PRt maintains two port ranges based on two tables of 32768 x 1-bit:
 *   - One for static ports (0 to 32,767) which are used for listening ports,
 *   - One for dynamically assigned or ephemeral ports (32,768 to 65,535) which
 *     are used for active connections.
 *
 *****************************************************************************/

#include "port_table.hpp"
#include "../../test/test_toe_utils.hpp"

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

// Warning: Avoid using 'enum' here because scoped enums are only available with -std=c++
//   enum PortState : bool {CLOSED_PORT = false, OPENED_PORT    = true};
//   enum PortRange : bool {ACTIVE_PORT = false, LISTENING_PORT = true};
#define LSN_CLOS_PORT false
#define LSN_OPEN_PORT true
#define ACT_FREE_PORT false
#define ACT_USED_PORT true
#define PortState     bool

#define ACTIVE_PORT   false
#define LISTEN_PORT   true
#define PortRange     bool

/******************************************************************************
 * @brief A two input AND gate.
 * ****************************************************************************/
template<class T> void pAnd2(
        T  &pi1,
        T  &pi2,
        T  &po) {
     //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off

    po = pi1 & pi2;
}

/******************************************************************************
 * @brief The Ready (Rdy) process generates the ready signal of the PRt.
 *
 *  @param[in]  piPRt_Ready, The ready signal from PortTable (PRt).
 *  @param[in]  piTBD_Ready, The ready signal from TBD.
 *  @param[out] poNTS_Ready, The ready signal of the TOE.
 *
 ******************************************************************************/
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

/******************************************************************************
 * @brief The Listening Port Table (Lpt) keeps track of the listening ports.
 *
 *  @param[out] poPRt_LptReady,        Ready signal for this process.
 *  @param[in]  siRAi_OpenPortReq,     Request to open a port from RxAppInterface (RAi).
 *  @param[out] soRAi_OpenPortRep,     Reply   to [RAi].
 *  @param[in]  siIrr_GetPortStateCmd, Request port state from InputRequestRouter (Irr).
 *  @param[out] soOrm_GetPortStateRsp  Port state response to OutputReplyMultiplexer (Orm).
 *
 * @details
 *  This table is accessed by two remote processes;
 *   1) the [RxAppInterface] for opening a port in listening mode.
 *   2) the [RxEngine] via the local [InputRequestRouter] process for requesting
 *      the state of a listening port.
 *
 *  If a read and a write operation occur at the same time, the write operation
 *  (.i.e the opening of the port) takes precedence over the read operation (.i.e,
 *  the request of the listening port state).
 *
 * @ingroup port_table
 ******************************************************************************/
void pListeningPortTable(
        StsBool              &poPRt_LptReady,
        stream<TcpPort>      &siRAi_OpenPortReq,
        stream<RepBit>       &soRAi_OpenPortRep,
        stream<TcpStaPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Lpt");

    static PortState                LISTEN_PORT_TABLE[0x8000];
    #pragma HLS RESOURCE   variable=LISTEN_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=LISTEN_PORT_TABLE inter false

    static bool                isLPtInit = false;
    #pragma HLS reset variable=isLPtInit
    static TcpPort             lsnPortNum = 0;
    #pragma HLS reset variable=lsnPortNum

    // The table must be cleared upon reset
    if (!isLPtInit) {
        LISTEN_PORT_TABLE[lsnPortNum(14, 0)] = LSN_CLOS_PORT;
        lsnPortNum += 1;
        if (lsnPortNum == 0x8000) {
          isLPtInit = true;
          if (DEBUG_LEVEL & TRACE_LPT) {
              printInfo(myName, "Done with initialization of LISTEN_PORT_TABLE.\n");
          }
        }
    }
    else {
        if (!siRAi_OpenPortReq.empty() and !soRAi_OpenPortRep.full()) {
            siRAi_OpenPortReq.read(lsnPortNum);
            // [TODO] Let's add a specific bit to specifically open/close a port.
            // [TODO] The meaning of the reply bit should be SUCCESS/FAILL OK/KO
            if (lsnPortNum < 0x8000) {
                // Listening port number falls in the range [0..32,767]
                // We can set the listening port table entry to true
                LISTEN_PORT_TABLE[lsnPortNum] = LSN_OPEN_PORT;
                // Sent reply to RAi
                soRAi_OpenPortRep.write(LSN_OPEN_PORT);
                if (DEBUG_LEVEL & TRACE_LPT)
                printInfo(myName, "[RAi] is requesting to open port #%d in listen mode.\n", lsnPortNum.to_uint());
            }
            else {
                // [TODO] The meaning of the reply bit should be SUCCESS/FAILL OK/KO
                soRAi_OpenPortRep.write(LSN_CLOS_PORT);
            } 
        }
        else if (!siIrr_GetPortStateCmd.empty()) {
            // Warning: Cannot add "and !soOrm_GetPortStateRsp.full()" here because
            //  it increases the task interval from 1 to 2!
            TcpStaPort staticPortNum = siIrr_GetPortStateCmd.read();
            // Sent status of that portNum to Orm
            soOrm_GetPortStateRsp.write(LISTEN_PORT_TABLE[staticPortNum]);
            if (DEBUG_LEVEL & TRACE_LPT)
                printInfo(myName, "[RXe] is querying the state of listen port #%d \n", staticPortNum.to_uint());
        }
    }

    // ALWAYS
    poPRt_LptReady = isLPtInit;
}

/*****************************************************************************
 * @brief The Free Port Table (Fpt) keeps track of the opened source ports.
 *
 * @param[out] poPRt_FptReady,        Ready signal for this process.
 * @param[in]  siSLc_CloseActPortCmd, Command to release a port from SessionLookupController (SLc).
 * @param[in]  siIrr_GetPortStateCmd, Command to get state of port from InputRequestRouter (Irr).
 * @param[out] soOrm_GetPortStateRsp, Response to get state of port to OutputReplyMux (Orm).
 * @param[in]  siTAi_GetFreePortReq,  Request to get a free port from TxAppInterface (TAi).
 * @param[out] soTAi_GetFreePortRep,  Reply   to siTAi_GetFreePortReq.
 *
 * @details
 *  This table contains a free list of ephemeral ports (also referred as dynamic
 *  ports) that the TOE uses as local source ports when it opens new connections.
 *  A dynamic source port is implicitly assigned in the range (32,768 to 65,535)
 *  and there is now way (or reason) to explicitly specify such a port number.
 *  This table is accessed by 1 local [Irr] and 2 remote [SLc][TAi] processes.
 *
 * @Warning
 *  The initial design point assumed a maximum of 10K sessions, which is much
 *  less than the possible 32K active ports.
 *
 * @ingroup port_table
 ******************************************************************************/
void pFreePortTable(
        StsBool              &poPRt_FptReady,
        stream<TcpPort>      &siSLc_CloseActPortCmd,
        stream<TcpDynPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp,
        stream<ReqBit>       &siTAi_GetFreePortReq,
        stream<TcpPort>      &soTAi_GetFreePortRep)

{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Fpt");

    static PortRange                ACTIVE_PORT_TABLE[0x8000];
    #pragma HLS RESOURCE   variable=ACTIVE_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ACTIVE_PORT_TABLE inter false

    static bool          searching = false;
    static bool               eval = false;
    static bool          portState = ACT_USED_PORT;
    #pragma HLS DEPENDENCE variable=portState inter false
    static bool          isFPtInit = false;
    #pragma HLS reset      variable=isFPtInit
    static TcpDynPort               dynPortNum = 0x7FFF;
    #pragma HLS reset      variable=dynPortNum

    // The table is a free list that must be initialized upon reset
    if (!isFPtInit) {
        ACTIVE_PORT_TABLE[dynPortNum] = ACT_FREE_PORT;
        dynPortNum -= 1;
        if (dynPortNum == 0) {
            isFPtInit = true;
            if (DEBUG_LEVEL & TRACE_FPT) {
                printInfo(myName, "Done with initialization of ACTIVE_PORT_TABLE.\n");
            }
        }
    }
    else {
        if (searching) {
            portState = ACTIVE_PORT_TABLE[dynPortNum];
            searching = false;
            eval      = true;
        }
        else if (eval) {
            if (portState == ACT_FREE_PORT) {
                // Found a free entry port in the table
                if (!soTAi_GetFreePortRep.full()) {
                    // Stop evaluating. Set port number to USED and forward to [TAi]
                    eval = false;
                    ACTIVE_PORT_TABLE[dynPortNum] = ACT_USED_PORT;
                    // Add 0x8000 before sending back
                    soTAi_GetFreePortRep.write(0x8000 + dynPortNum);
                }
            }
            else {
                // Continue searching
                searching = true;
            }
            dynPortNum++;
        }
        else if (!siIrr_GetPortStateCmd.empty()) {
            // Warning: Cannot add "and !soOrm_GetPortStateRsp.full()" here because
            //  it increases the task interval from 1 to 2!
            TcpDynPort portNum = siIrr_GetPortStateCmd.read();
            soOrm_GetPortStateRsp.write(ACTIVE_PORT_TABLE[portNum]);
        }
        else if (!siTAi_GetFreePortReq.empty()) {
            siTAi_GetFreePortReq.read();
            searching = true;
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
    poPRt_FptReady = isFPtInit;
}


/*****************************************************************************
 * @brief The Input Request Router (Irr) process checks the range of the port
 *  to be looked-up and forwards that request to the corresponding lookup table.
 *
 *  @param[in]  siRXe_GetPortStateCmd,    Get state of a port from RxEngine (RXe).
 *  @param[out] soLpt_GetLsnPortStateCmd, Get state of a listen port to ListenPortTable (LPt).
 *  @param[out] soFpt_GetActPortStateCmd, Get state of an active port to FreePortTable (Fpt).
 *  @param[out] soOrm_QueryRange,         Range of the forwarded query to OutputReplyMux (Orm).
 *
 * @details
 *  The PRt supports two port ranges; one for static ports (0 to 32,767) which
 *   are used for listening ports, and one for dynamically assigned or
 *   ephemeral ports (32,768 to 65,535) which are used for active connections.
 *
 * @ingroup port_table
 ******************************************************************************/
void pInputRequestRouter(
        stream<TcpPort>           &siRXe_GetPortStateCmd,
        stream<TcpStaPort>        &soLpt_GetLsnPortStateCmd,
        stream<TcpDynPort>        &soFpt_GetActPortStateCmd,
        stream<PortRange>         &soOrm_QueryRange)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
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


/*****************************************************************************
 * @brief The Output Reply Multiplexer (Orm) orders the lookup replies before
 *  sending them back to the Rx Engine.
 *
 *  @param[in]  siIrr_QueryRange,         Range of the query from InputRequestRouter (Irr).
 *  @param[in]  siLpt_GetLsnPortStateRsp, Listen port state response from ListenPortTable (LPt).
 *  @param[in]  siFpt_GetActPortStateRsp, Active port state response from FreePortTable (Fpt).
 *  @param[out] soRXe_GetPortStateRsp,    Port state response to RxEngine (RXe).
 *
 * @ingroup port_table
 ******************************************************************************/
void pOutputReplyMultiplexer(
        stream<PortRange> &siIrr_QueryRange,
        stream<RspBit>    &siLpt_GetLsnPortStateRsp,
        stream<RspBit>    &siFpt_GetActPortStateRsp,
        stream<RspBit>    &soRXe_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static PortRange qryType;

    static enum OrmFsmStates { WAIT_FOR_QUERY_FROM_Irr=0, FORWARD_LSN_PORT_STATE_RSP,
                               FORWARD_ACT_PORT_STATE_RSP } ormFsmState=WAIT_FOR_QUERY_FROM_Irr;

    // Read out responses from tables in order and merge them
    switch (ormFsmState) {

    case WAIT_FOR_QUERY_FROM_Irr:
        if (!siIrr_QueryRange.empty()) {
            qryType = siIrr_QueryRange.read();
            if (qryType == LISTEN_PORT)
                ormFsmState = FORWARD_LSN_PORT_STATE_RSP;
            else
                ormFsmState = FORWARD_ACT_PORT_STATE_RSP;
        }
        break;

    case FORWARD_LSN_PORT_STATE_RSP:
        if (!siLpt_GetLsnPortStateRsp.empty() and !soRXe_GetPortStateRsp.full()) {
            soRXe_GetPortStateRsp.write(siLpt_GetLsnPortStateRsp.read());
            ormFsmState = WAIT_FOR_QUERY_FROM_Irr;
        }
        break;

    case FORWARD_ACT_PORT_STATE_RSP:
        if (!siFpt_GetActPortStateRsp.empty() and !soRXe_GetPortStateRsp.full()) {
            soRXe_GetPortStateRsp.write(siFpt_GetActPortStateRsp.read());
            ormFsmState = WAIT_FOR_QUERY_FROM_Irr;
        }
        break;
    }
}



/*****************************************************************************
 * @brief The port_table (PRt) keeps track of the TCP port numbers which are
 *         in use and therefore opened.
 *
 *  @param[out] poTOE_Ready,           PRt's ready signal.
 *  @param[in]  siRXe_GetPortStateReq, Request to get state of a port from RxEngine (RXe).
 *  @param[out] soRXe_GetPortStateRep, Reply   to siRXe_GetPortStateReq.
 *  @param[in]  siRAi_OpenPortReq,     Request to open a port from RxAppInterface (RAi).
 *  @param[out] soRAi_OpenPortRep,     Reply   to siRAi_OpenPortReq.
 *  @param[in]  siTAi_GetFreePortReq,  Request to get a free port from TxAppInterface (TAi).
 *  @param[out] soTAi_GetFreePortRep,  Reply   to siTAi_GetFreePortReq.
 *  @param[in]  siSLc_CloseActPortCmd, Command to close an active port from SessionLookupController (SLc).
 *
 * @details
 *  The port_table process contains an array of 65536 entries;  one for each
 *   port number.
 *  A port can be either closed, in listen, or in active state. According to
 *   the standard, the PRt supports two port ranges; one for static ports (0
 *   to 32,767) which are used for listening ports, and one for dynamically
 *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
 *   connections.
 *  The PRt receives requests from 3 sources:
 *   1) opening (listening) requests from the Rx Application Interface (RAi),
 *   2) requests to check if a given port is open from the Rx Engine (RXe),
 *   3) requests for a free port from the Tx Application Interface (TAi).
 *
 * @ingroup port_table
 ******************************************************************************/
void port_table(
        StsBool                 &poTOE_Ready,
        stream<TcpPort>         &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        stream<TcpPort>         &siRAi_OpenPortReq,
        stream<RepBit>          &soRAi_OpenPortRep,
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &soTAi_GetFreePortRep,
        stream<TcpPort>         &siSLc_CloseActPortCmd)
{

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL SIGNALS AND STREAMS
    //--    (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    // Listening PortTable
    pListeningPortTable(
            sLptToAnd2_Ready,
            siRAi_OpenPortReq,
            soRAi_OpenPortRep,
            ssIrrToLpt_GetLsnPortStateCmd,
            ssLptToOrm_GetLsnPortStateRsp);

    // Free PortTable
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

    // Multiplexes the output replies
    pOutputReplyMultiplexer(
            ssIrrToOrm_QueryRange,
            ssLptToOrm_GetLsnPortStateRsp,
            ssFptToOrm_GetActPortStateRsp,
            soRXe_GetPortStateRep);

    // Bitwise AND of the two local ready signals
    pReady(
            sLptToAnd2_Ready,
            sFptToAnd2_Ready,
            poTOE_Ready);

}
