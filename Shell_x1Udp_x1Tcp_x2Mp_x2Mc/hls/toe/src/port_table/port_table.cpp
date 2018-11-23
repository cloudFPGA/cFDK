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
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

#include "port_table.hpp"

#define THIS_NAME "TOE/PRt"

using namespace hls;

#define DEBUG_LEVEL 2

enum QueryType {ACTIVE_PORT=false, LISTENING_PORT};


/*****************************************************************************
 * @brief The Listening Port Table (Lpt) keeps track of the listening ports.
 *
 *  @param[in]  siRAi_LsnPortStateReq, A request from the Rx App Interface (RAi).
 *  @param[in]  siIrr_GetLsnPortState, Request listening port state from the Input Request Router (Irr).
 *  @param[out] portTable2rxApp_listen_rsp
 *  @param[out] soLsnPortStateSts    Listen port state status.
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
        stream<TcpPort>        &siRAi_LsnPortStateReq,
        stream<ap_uint<15> >   &siIrr_GetLsnPortState,
        stream<bool>           &portTable2rxApp_listen_rsp,
        stream<bool>           &soLsnPortStateSts)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static bool                     listeningPortTable[32768];
    #pragma HLS RESOURCE   variable=listeningPortTable core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=listeningPortTable inter false

    ap_uint<16> currPort;

    if (!siRAi_LsnPortStateReq.empty()) {
        siRAi_LsnPortStateReq.read(currPort);
        // check range, TODO make sure currPort is not equal in 2 consecutive cycles
        if (!listeningPortTable[currPort(14, 0)] && currPort < 32768) {
            listeningPortTable[currPort] = true;
            portTable2rxApp_listen_rsp.write(true);
        }
        else
            portTable2rxApp_listen_rsp.write(false);
    }
    else if (!siIrr_GetLsnPortState.empty())
        soLsnPortStateSts.write(listeningPortTable[siIrr_GetLsnPortState.read()]);
}


/*****************************************************************************
 * @brief The Free Port Table (Fpt) keeps track of the active ports.
 *
 * @param[in]  siSLc_ReleasePort,     Release port command from SLc.
 * @param[in]  sIrr_GetActPortState,  Request active port state from the Input Request Router (Irr).
 * @param[in]  siTAi_ActPortStateReq, Active port state request from Tx App I/F (TAi).
 * @param[out] soActPortStateRep,     Active port state reply to TAi.
 * @param[out] soActPortStateSts,     Active port state status to Output Reply Mux (Orm).

 *
 * @details
 *  This table is accessed by 1 local and 2 remote processes.
 *  If a free port is found, it is written into 'soActPortStateRep' and
 *   cached until @ref tx_app_stream_if reads it out.
 *  Assumption: We are never going to run out of free ports, since 10K sessions
 *   is much less than 32K ports.
 *
 * @ingroup port_table
 ******************************************************************************/
void pFreePortTable(
        stream<ap_uint<16> >   &siSLc_ReleasePort,
        stream<ap_uint<15> >   &sIrr_GetActPortState,
        stream<ap_uint<1> >    &siTAi_ActPortStateReq,
        stream<ap_uint<16> >   &soActPortStateRep,
        stream<bool>           &soActPortStateSts)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static bool                     freePortTable[32768]; // = {false};
    #pragma HLS RESOURCE   variable=freePortTable core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=freePortTable inter false

    static ap_uint<15>  freePort = 0;
    static bool searching   = false;
    static bool eval        = false;
    static bool temp        = false;
    #pragma HLS DEPENDENCE variable=temp inter false

    if (searching) {
        temp = freePortTable[freePort];
        eval = true;
        searching = false;
    }
    else if (eval) {
        if (!temp) {
            freePortTable[freePort] = true;
            soActPortStateRep.write(freePort);
        }
        else
            searching = true;
        eval = false;
        freePort++;
    }
    else if (!sIrr_GetActPortState.empty())
        soActPortStateSts.write(freePortTable[sIrr_GetActPortState.read()]);
    else if (!siTAi_ActPortStateReq.empty()) {
        siTAi_ActPortStateReq.read();
        searching = true;
    }
    else if (!siSLc_ReleasePort.empty()) {
        // check range, TODO make sure no access to same location in 2 consecutive cycles
        ap_uint<16> currPort = siSLc_ReleasePort.read();
        if (currPort.bit(15) == 1)
            freePortTable[currPort.range(14, 0)] = false; //shift
    }
}


/*****************************************************************************
 * @brief The Request Router (Rqr) process checks the range of the port to be
 *  looked-up and forwards that request to the corresponding lookup table.
 *
 *  @param[in]  siRXe_PortStateReq, Port state request from Rx Engine (RXe).
 *  @param[out] soGetLsnPortState,  Request state of a listing type of port.
 *  @param[out] soGetActPortState,  Request state of an active type of port.
 *  @param[out] soQueryType,        The type of forwarded query.
 *
 * @details
 *  The PRt supports two port ranges; one for static ports (0 to 32,767) which
 *   are used for listening ports, and one for dynamically assigned or
 *   ephemeral ports (32,768 to 65,535) which are used for active connections.
 *
 * @ingroup port_table
 ******************************************************************************/
void pRequestRouter(
        stream<AxiTcpPort>          &siRXe_PortStateReq,
        stream<ap_uint<15> >        &soGetLsnPortState,
        stream<ap_uint<15> >        &soGetActPortState,
        stream<bool>                &soQueryType)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    // Forward request according to port number,
    if (!siRXe_PortStateReq.empty()) {
        AxiTcpPort rcvPortNum = siRXe_PortStateReq.read();
        // Swap from LITTLE- to BIG-ENDIAN
        TcpPort portToCheck = byteSwap16(rcvPortNum);
        if (portToCheck < 32768) {
            // Listening ports are in the range [0x0000..0x7FFF]
            soGetLsnPortState.write(portToCheck.range(14, 0));
            soQueryType.write(LISTENING_PORT);
        }
        else {
            // Active ports are in the range [0x8000..0xFFFF]
            soGetActPortState.write(portToCheck.range(14, 0));
            soQueryType.write(ACTIVE_PORT);
        }
    }
}


/*****************************************************************************
 * @brief The Output rReply Multiplexer (Orm) orders the lookup replies before
 *  sending them back to the Rx Engine.
 *
 *  @param[in]  siRqr_QueryType,       The type of ongoing query.
 *  @param[in]  siLpt_LsnPortStateSts, Listen port state status from Listen Port Table (Lpt).
 *  @param[in]  siFpt_ActPortStateSts, Listen port state status from Free Port Table (Fpt).
 *  @param[out] soPortStateRep,        Port state reply (to RXe).
 *
 * @details
 *
 * @ingroup port_table
 ******************************************************************************/
void pOutputReplyMultiplexer(
        stream<bool>   &siRqr_QueryType,
        stream<bool>   &siLpt_LsnPortStateSts,
        stream<bool>   &siFpt_ActPortStateSts,
        stream<bool>   &soPortStateRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    enum cmFsmStateType {READ_DST = 0, READ_LISTENING, READ_USED}; // Read out responses from tables in order and merge them
    static cmFsmStateType cm_fsmState = READ_DST;

    switch (cm_fsmState) {
    case READ_DST:
        if (!siRqr_QueryType.empty()) {
            bool qryType = siRqr_QueryType.read();
            if (qryType == LISTENING_PORT)
                cm_fsmState = READ_LISTENING;
            else
                cm_fsmState = READ_USED;
        }
        break;
    case READ_LISTENING:
        if (!siLpt_LsnPortStateSts.empty()) {
            soPortStateRep.write(siLpt_LsnPortStateSts.read());
            cm_fsmState = READ_DST;
        }
        break;
    case READ_USED:
        if (!siFpt_ActPortStateSts.empty()) {
            soPortStateRep.write(siFpt_ActPortStateSts.read());
            cm_fsmState = READ_DST;
        }
        break;
    }
}


/*****************************************************************************
 * @brief A wrapper for the input and output multiplexer processes.
 *  [TODO - This is useless, consider removing].
 *
 * @ingroup port_table
 ******************************************************************************/
void pMultiplexer(
        stream<AxiTcpPort>         &siRXe_PortStateReq,
        stream<ap_uint<15> >       &soGetLsnPortState,
        stream<ap_uint<15> >       &soGetActPortState,
        stream<bool>               &siLpt_LsnPortStateSts,
        stream<bool>               &soActPortStateSts,
        stream<bool>               &soPortStateRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //#pragma HLS PIPELINE II=1
    #pragma HLS INLINE

    static stream<bool>         sRtrToMxr_QueryType ("sRtrToMxr_QueryType");
    #pragma HLS STREAM variable=sRtrToMxr_QueryType depth=4

    pRequestRouter(
            siRXe_PortStateReq,
            soGetLsnPortState,
            soGetActPortState,
            sRtrToMxr_QueryType);

    pOutputReplyMultiplexer(
            sRtrToMxr_QueryType,
            siLpt_LsnPortStateSts,
            soActPortStateSts,
            soPortStateRep);
}


/*****************************************************************************
 * @brief The port_table (PRt) keeps track of the state of the TCP ports.
 *
 *  @param[in]  siRXe_PortStateReq,    Port state request from Rx Engine (RXe).
 *  @param[out] soPortStateRep,        Port state reply to RXe.
 *  @param[in]  siRAi_LsnPortStateReq, Listen port request from Rx App I/F (RAi).
 *  @param[out] soLsnPortStateRep,     Listen port reply to RAi.
 *  @param[in]  siTAi_ActPortStateReq, Active port state request from Tx App I/F (TAi).
 *  @param[out] soActPortStateRep,     Active port state response to TAi.
 *  @param[in]  siSLc_ReleasePort,     Release port command from Session Lookup Controller (SLc).
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
        stream<AxiTcpPort>         &siRXe_PortStateReq,
        stream<bool>               &soPortStateRep,
        stream<TcpPort>            &siRAi_LsnPortStateReq,
        stream<bool>               &soLsnPortStateRep,
        stream<ap_uint<1> >        &siTAi_ActPortStateReq,
        stream<ap_uint<16> >       &soActPortStateRep,
        stream<ap_uint<16> >       &siSLc_ReleasePort)
{

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //#pragma HLS dataflow interval=1
    //#pragma HLS PIPELINE II=1
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Input Request Router (Irr)
    static stream<ap_uint<15> > sIrrToLpt_GetLsnPortState ("sIrrToLpt_GetLsnPortState");
    #pragma HLS STREAM variable=sIrrToLpt_GetLsnPortState depth=2

    static stream<ap_uint<15> > sIrrToFptGetActPortState  ("sIrrToFptGetActPortState");
    #pragma HLS STREAM variable=sIrrToFptGetActPortState  depth=2

    //-- Listening Port Table (Lpt)
    static stream<bool>         sLptToOrm_LsnPortStateSts ("sLptToOrm_LsnPortStateSts");
    #pragma HLS STREAM variable=sLptToOrm_LsnPortStateSts depth=2

    //-- Free Port Table (Fpt)
    static stream<bool>         sFptToOrm_ActPortStateSts ("sFptToOrm_ActPortStateSts");
    #pragma HLS STREAM variable=sFptToOrm_ActPortStateSts depth=2

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    // Listening PortTable
    pListeningPortTable(
            siRAi_LsnPortStateReq,
            sIrrToLpt_GetLsnPortState,
            soLsnPortStateRep,
            sLptToOrm_LsnPortStateSts);

    // Free PortTable
    pFreePortTable(
            siSLc_ReleasePort,
            sIrrToFptGetActPortState,
            siTAi_ActPortStateReq,
            soActPortStateRep,
            sFptToOrm_ActPortStateSts);

    // Multiplex the requests and queries
    pMultiplexer(
            siRXe_PortStateReq,
            sIrrToLpt_GetLsnPortState,
            sIrrToFptGetActPortState,
            sLptToOrm_LsnPortStateSts,
            sFptToOrm_ActPortStateSts,
            soPortStateRep);

}
