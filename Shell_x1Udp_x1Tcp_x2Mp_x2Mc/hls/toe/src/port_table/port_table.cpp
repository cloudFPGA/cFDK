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
 *   One for static ports (0 to 32,767) which are used for listening ports,
 *   One for dynamically assigned or ephemeral ports (32,768 to 65,535) which
 *    are used for active connections.
 *
 *****************************************************************************/

#include "port_table.hpp"

#define THIS_NAME "TOE/PRt"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
extern bool gTraceEvent;
#define THIS_NAME "TOE/PRt"

#define TRACE_OFF  0x0000
#define TRACE_IRR 1 <<  1
#define TRACE_ORM 1 <<  2
#define TRACE_LPT 1 <<  3
#define TRACE_FPT 1 <<  4
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


enum PortState {CLOSED_PORT = false, OPENED_PORT    = true };
enum PortRange {ACTIVE_PORT = false, LISTENING_PORT = true};
enum OpenReply {KO          = false, OK             = true};


/*****************************************************************************
 * @brief The Listening Port Table (Lpt) keeps track of the listening ports.
 *
 *  @param[in]  siRAi_OpenPortReq,     Request to open a port from [RxAppInterface].
 *  @param[out] soRAi_OpenPortRep,     Reply   to siRAi_OpenPortReq.
 *  @param[in]  siIrr_GetPortStateCmd, Command to get state of port from [InputRequestRouter].
 *  @param[out] soOrm_GetPortStateRsp  Response to get state of port to [OutputReplyMultiplexer].
 *
 * @details
 *  This table is accessed by two remote processes; the Rx Engine (RXe) and the
 *   Tx Applicatiojn Interface (TAi). The RXe does read accesses while the TAi
 *   does read/write accesses.
 *  If a read and a write operation occur on the same address and at the same
 *   time, the read should get the old value, either way it doesn't matter.
 *
 * @ingroup port_table
 ******************************************************************************/
void pListeningPortTable(
        stream<TcpPort>      &siRAi_OpenPortReq,
        stream<RepBit>       &soRAi_OpenPortRep,
        stream<TcpStaPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Lpt");

    static PortState                LISTEN_PORT_TABLE[32768];
    #pragma HLS RESOURCE   variable=LISTEN_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=LISTEN_PORT_TABLE inter false

    ap_uint<16> currPort;

    if (!siRAi_OpenPortReq.empty()) {
        siRAi_OpenPortReq.read(currPort);
        // [TODO] make sure currPort is not equal in 2 consecutive cycles ???
        if (!LISTEN_PORT_TABLE[currPort(14, 0)] && currPort < 32768) {
            // Set the listening port table entry to true
            LISTEN_PORT_TABLE[currPort] = OPENED_PORT;
            // Sent reply to RAi
            soRAi_OpenPortRep.write(OPENED_PORT);
            if (DEBUG_LEVEL & TRACE_LPT)
                printInfo(myName, "[RAi] is requesting to open port #%d in listen mode.\n", currPort.to_uint());
        }
        else
            soRAi_OpenPortRep.write(CLOSED_PORT);
    }
    else if (!siIrr_GetPortStateCmd.empty()) {
        TcpStaPort staticPortNum = siIrr_GetPortStateCmd.read();
        // Sent status of that portNum to Orm
        soOrm_GetPortStateRsp.write(LISTEN_PORT_TABLE[staticPortNum]);
        if (DEBUG_LEVEL & TRACE_LPT)
            printInfo(myName, "[RXe] is querying the state of listen port #%d \n", staticPortNum.to_uint());
    }
}


/*****************************************************************************
 * @brief The Free Port Table (Fpt) keeps track of the active ports.
 *
 * @param[in]  siSLc_ClosePortCmd,    Command to release a port from [SessionLookupController].
 * @param[in]  siIrr_GetPortStateCmd, Command to get state of port from [InputRequestRouter].
 * @param[out] soOrm_GetPortStateRsp, Response to get state of port to [OutputReplyMux].
 * @param[in]  siTAi_GetFreePortReq,  Request to get a free port from [TxAppInterface].
 * @param[out] soTAi_GetFreePortRep,  Reply   to siTAi_GetFreePortReq.
 *
 * @details
 *  This table is accessed by 1 local and 2 remote processes.
 *  If a free port is found, it is written into 'soTAi_GetFreePortRep' and
 *   cached until the TxAppInterface reads it out.
 *  Assumption: We are never going to run out of free ports, since 10K sessions
 *   is much less than 32K ports.
 *
 * @ingroup port_table
 ******************************************************************************/
void pFreePortTable(
        stream<TcpPort>      &siSLc_ClosePortCmd,
        stream<TcpDynPort>   &siIrr_GetPortStateCmd,
        stream<RspBit>       &soOrm_GetPortStateRsp,
        stream<ReqBit>       &siTAi_GetFreePortReq,
        stream<TcpPort>      &soTAi_GetFreePortRep)

{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Fpt");

    static PortState                FREE_PORT_TABLE[32768]; // = {false};
    #pragma HLS RESOURCE   variable=FREE_PORT_TABLE core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=FREE_PORT_TABLE inter false

    static TcpDynPort freePort  = 0;
    static bool       searching = false;
    static bool       eval      = false;
    static bool       temp      = false;
    #pragma HLS DEPENDENCE variable=temp inter false

    if (searching) {
        temp = FREE_PORT_TABLE[freePort];
        eval = true;
        searching = false;
    }
    else if (eval) {
        //OBSOLETE-20190111 if (!temp) {
        if (temp == CLOSED_PORT) {
            FREE_PORT_TABLE[freePort] = OPENED_PORT;
            soTAi_GetFreePortRep.write(freePort);
        }
        else
            searching = true;
        eval = false;
        freePort++;
    }
    else if (!siIrr_GetPortStateCmd.empty())
        soOrm_GetPortStateRsp.write(FREE_PORT_TABLE[siIrr_GetPortStateCmd.read()]);
    else if (!siTAi_GetFreePortReq.empty()) {
        siTAi_GetFreePortReq.read();
        searching = true;
    }
    else if (!siSLc_ClosePortCmd.empty()) {
        // [TODO] Make sure no access to same location in 2 consecutive cycles???
        TcpPort currPort = siSLc_ClosePortCmd.read();
        if (currPort.bit(15) == 1)
            FREE_PORT_TABLE[currPort.range(14, 0)] = CLOSED_PORT;
      #ifndef __SYNTHESIS__
        else {
            printError(myName, "SLc is not allowed to release a static port.\n");
            exit(1);
        }
      #endif
    }
}


/*****************************************************************************
 * @brief The Input Request Router (Irr) process checks the range of the port
 *  to be looked-up and forwards that request to the corresponding lookup table.
 *
 *  @param[in]  siRXe_GetPortStateCmd,    Get state of a port from [RxEngine].
 *  @param[out] soLpt_GetLsnPortStateCmd, Get state of a listen port to [ListenPortTable].
 *  @param[out] soFpt_GetActPortStateCmd, Get state of an active port to [FreePortTable].
 *  @param[out] soOrm_QueryRange,         Range of the forwarded query to [OutputReplyMux].
 *
 * @details
 *  The PRt supports two port ranges; one for static ports (0 to 32,767) which
 *   are used for listening ports, and one for dynamically assigned or
 *   ephemeral ports (32,768 to 65,535) which are used for active connections.
 *
 * @ingroup port_table
 ******************************************************************************/
void pInputRequestRouter(
        stream<AxiTcpPort>        &siRXe_GetPortStateCmd,
        stream<TcpStaPort>        &soLpt_GetLsnPortStateCmd,
        stream<TcpDynPort>        &soFpt_GetActPortStateCmd,
        stream<bool>              &soOrm_QueryRange)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Irr");

    // Forward request according to port number,
    if (!siRXe_GetPortStateCmd.empty()) {
        AxiTcpPort rcvPortNum = siRXe_GetPortStateCmd.read();
        // Swap from LITTLE- to BIG-ENDIAN
        TcpPort portToCheck = byteSwap16(rcvPortNum);
        if (portToCheck < 32768) {
            // Listening ports are in the range [0x0000..0x7FFF]
            soLpt_GetLsnPortStateCmd.write(portToCheck.range(14, 0));
            soOrm_QueryRange.write(LISTENING_PORT);
        }
        else {
            // Active ports are in the range [0x8000..0xFFFF]
            soFpt_GetActPortStateCmd.write(portToCheck.range(14, 0));
            soOrm_QueryRange.write(ACTIVE_PORT);
        }
    }
}


/*****************************************************************************
 * @brief The Output rReply Multiplexer (Orm) orders the lookup replies before
 *  sending them back to the Rx Engine.
 *
 *  @param[in]  siIrr_QueryRange,         Range of the query from [InputRequestRouter].
 *  @param[in]  siLpt_GetLsnPortStateRsp, Listen port state response from [ListenPortTable].
 *  @param[in]  siFpt_GetActPortStateRsp, Active port state response from [FreePortTable].
 *  @param[out] soRXe_GetPortStateRsp,    Port state response to [RxEngine].
 *
 * @details
 *
 * @ingroup port_table
 ******************************************************************************/
void pOutputReplyMultiplexer(
        stream<bool>      &siIrr_QueryRange,
        stream<RspBit>    &siLpt_GetLsnPortStateRsp,
        stream<RspBit>    &siFpt_GetActPortStateRsp,
        stream<RspBit>    &soRXe_GetPortStateRsp)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    //OBSOLETE enum cmFsmStateType {READ_DST = 0, READ_LISTENING, READ_USED}; // Read out responses from tables in order and merge them
    //OBSOLETE static cmFsmStateType cm_fsmState = READ_DST;

    static enum FsmState {READ_DST=0, READ_LISTENING, READ_USED} cm_fsmState=READ_DST;

    // Read out responses from tables in order and merge them
    switch (cm_fsmState) {

    case READ_DST:
        if (!siIrr_QueryRange.empty()) {
            bool qryType = siIrr_QueryRange.read();
            if (qryType == LISTENING_PORT)
                cm_fsmState = READ_LISTENING;
            else
                cm_fsmState = READ_USED;
        }
        break;

    case READ_LISTENING:
        if (!siLpt_GetLsnPortStateRsp.empty()) {
            soRXe_GetPortStateRsp.write(siLpt_GetLsnPortStateRsp.read());
            cm_fsmState = READ_DST;
        }
        break;
    case READ_USED:
        if (!siFpt_GetActPortStateRsp.empty()) {
            soRXe_GetPortStateRsp.write(siFpt_GetActPortStateRsp.read());
            cm_fsmState = READ_DST;
        }
        break;
    }
}


/*****************************************************************************
 * @brief The port_table (PRt) keeps track of the TCP port numbers which are
 *         in use and therefore opened.
 *
 *  @param[in]  siRXe_GetPortStateReq, Request to get state of a port from [RxEngine].
 *  @param[out] soRXe_GetPortStateRep, Reply   to siRXe_GetPortStateReq.
 *  @param[in]  siRAi_OpenPortReq,     Request to open a port from [RxAppInterface].
 *  @param[out] soRAi_OpenPortRep,     Reply   to siRAi_OpenPortReq.
 *  @param[in]  siTAi_GetFreePortReq,  Request to get a free port from [TxAppInterface].
 *  @param[out] siTAi_GetFreePortRep,  Reply   to siTAi_GetFreePortReq.
 *  @param[in]  siSLc_ClosePortCmd,    Command to close a port from [SessionLookupController].
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
        stream<AxiTcpPort>      &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        stream<TcpPort>         &siRAi_OpenPortReq,
        stream<RepBit>          &soRAi_OpenPortRep,
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &siTAi_GetFreePortRep,
        stream<TcpPort>         &siSLc_ClosePortCmd)
{

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //OBSOLETE #pragma HLS dataflow interval=1
    //OBSOLETE #pragma HLS PIPELINE II=1
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Input Request Router (Irr)
    static stream<TcpStaPort>   sIrrToLpt_GetLsnPortStateCmd ("sIrrToLpt_GetLsnPortStateCmd");
    #pragma HLS STREAM variable=sIrrToLpt_GetLsnPortStateCmd depth=2

    static stream<TcpDynPort>   sIrrToFpt_GetActPortStateCmd ("sIrrToFpt_GetActPortStateCmd");
    #pragma HLS STREAM variable=sIrrToFpt_GetActPortStateCmd depth=2

    static stream<bool>         sIrrToOrm_QueryRange         ("sIrrToOrm_QueryRange");
    #pragma HLS STREAM variable=sIrrToOrm_QueryRange         depth=4

    //-- Listening Port Table (Lpt)
    static stream<RspBit>       sLptToOrm_GetLsnPortStateRsp ("sLptToOrm_GetLsnPortStateRsp");
    #pragma HLS STREAM variable=sLptToOrm_GetLsnPortStateRsp depth=2

    //-- Free Port Table (Fpt)
    static stream<RspBit>       sFptToOrm_GetActPortStateRsp ("sFptToOrm_GetActPortStateRsp");
    #pragma HLS STREAM variable=sFptToOrm_GetActPortStateRsp depth=2

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    // Listening PortTable
    pListeningPortTable(
            siRAi_OpenPortReq,
            soRAi_OpenPortRep,
            sIrrToLpt_GetLsnPortStateCmd,
            sLptToOrm_GetLsnPortStateRsp);

    // Free PortTable
    pFreePortTable(
            siSLc_ClosePortCmd,
            sIrrToFpt_GetActPortStateCmd,
            sFptToOrm_GetActPortStateRsp,
            siTAi_GetFreePortReq,
            siTAi_GetFreePortRep);

    // Routes the input requests
    pInputRequestRouter(
            siRXe_GetPortStateReq,
            sIrrToLpt_GetLsnPortStateCmd,
            sIrrToFpt_GetActPortStateCmd,
            sIrrToOrm_QueryRange);

    // Multiplexes the output replies
    pOutputReplyMultiplexer(
            sIrrToOrm_QueryRange,
            sLptToOrm_GetLsnPortStateRsp,
            sFptToOrm_GetActPortStateRsp,
            soRXe_GetPortStateRep);

}
