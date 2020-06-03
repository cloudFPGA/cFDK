/************************************************
Copyright (c) 2016-2019, IBM Research.

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

/*****************************************************************************
 * @file       : tcp_shell_if.cpp
 * @brief      : TCP Shell Interface (TSIF)
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This process handles the control flow interface between the
 *  the SHELL and the ROLE. The main purpose of the TSIF is to open port(s) for
 *   listening and for connecting to remote host(s).
 *
 * @todo       : [TODO - The logic for listening and connecting is most likely going to end up in the FMC].
 *
 *****************************************************************************/

#include "tcp_shell_if.hpp"

using namespace hls;
using namespace std;

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif


#define THIS_NAME "TSIF"  // TcpShellInterface

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CON 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  TCP Role Interface, unless the user specifies new ones
//--  via TBD.
//--  FYI --> 8803 is the ZIP code of Ruschlikon ;-)
//---------------------------------------------------------
#define DEFAULT_FPGA_LSN_PORT   0x2263      // TOE    listens on port = 8803 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571



/******************************************************************************
 * @brief Client connection to remote HOST or FPGA socket (COn).
 *
 * @param[in]  piSHL_Enable,  enable signal from [SHELL].
 * @param[out] soSHL_OpnReq,  open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep,  open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq,  close connection request to [SHELL].
 * @param[out] poTAF_SConId,  session connect Id to [ROLE/TAF].
 *
 * @details
 *  For testing purposes, this connect process opens a single connection to a
 *   'hard-coded' remote HOST IP address (10.12.200.50) and port 8803.
 *   [FIXME-Make remote-IP and -PORT should be configurable].
 *  The session ID of this single connection is provided to the TCP Application
 *  Flash process (TAF) of the ROLE over the 'poTAF_SConId' port.
 *
 * [FIXME - THIS PROCESS IS DISABLED UNTIL WE ADD A SW CONFIG of THE REMOTE SOCKET]
 ******************************************************************************/
void pConnect(
        CmdBit                *piSHL_Enable,
        stream<AppOpnReq>     &soSHL_OpnReq,
        stream<AppOpnRep>     &siSHL_OpnRep,
        stream<AppClsReqAxis> &soSHL_ClsReq,
        SessionId             *poTAF_SConId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "COn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates{ OPN_IDLE, OPN_REQ, OPN_REP, OPN_DONE }
                               con_fsmState=OPN_IDLE;
    #pragma HLS reset variable=con_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static LE_SockAddr   con_leHostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER
                                              // [FIXME - Change to default Big-Endian]
    static AppOpnRep     con_newCon;
    static ap_uint< 12>  con_watchDogTimer;

    switch (con_fsmState) {

    case OPN_IDLE:
        if (*piSHL_Enable != 1) {
            if (!siSHL_OpnRep.empty()) {
                // Drain any potential status data
                siSHL_OpnRep.read(con_newCon);
                printWarn(myName, "Draining unexpected residue from the \'OpnRep\' stream. As a result, request to close sessionId=%d.\n", con_newCon.sessionID.to_uint());
                soSHL_ClsReq.write(con_newCon.sessionID);
            }
        }
        else {
            // [FIXME - THIS PROCESS IS DISABLED UNTIL WE ADD A SW CONFIG of THE REMOTE SOCKET]
            con_fsmState = OPN_IDLE;  // FIXME --> Should be OPN_REQ;
        }
        break;

    case OPN_REQ:
        if (!soSHL_OpnReq.full()) {
            // [FIXME - Remove the hard coding of this socket]
            SockAddr    hostSockAddr(DEFAULT_HOST_IP4_ADDR, DEFAULT_HOST_LSN_PORT);
            con_leHostSockAddr.addr = byteSwap32(hostSockAddr.addr);
            con_leHostSockAddr.port = byteSwap16(hostSockAddr.port);
            soSHL_OpnReq.write(con_leHostSockAddr);
            if (DEBUG_LEVEL & TRACE_CON) {
                printInfo(myName, "Client is requesting to connect to remote socket:\n");
                printSockAddr(myName, con_leHostSockAddr);
            }
            #ifndef __SYNTHESIS__
                con_watchDogTimer = 250;
            #else
                con_watchDogTimer = 10000;
            #endif
            con_fsmState = OPN_REP;
        }
        break;

    case OPN_REP:
        con_watchDogTimer--;
        if (!siSHL_OpnRep.empty()) {
            // Read the reply stream
            siSHL_OpnRep.read(con_newCon);
            if (con_newCon.success) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printInfo(myName, "Client successfully connected to remote socket:\n");
                    printSockAddr(myName, con_leHostSockAddr);
                    printInfo(myName, "The Session ID of this connection is: %d\n", con_newCon.sessionID.to_int());
                }
                *poTAF_SConId = con_newCon.sessionID;
                con_fsmState = OPN_DONE;
            }
            else {
                 printError(myName, "Client failed to connect to remote socket:\n");
                 printSockAddr(myName, con_leHostSockAddr);
                 con_fsmState = OPN_DONE;
            }
        }
        else {
            if (con_watchDogTimer == 0) {
                if (DEBUG_LEVEL & TRACE_CON) {
                    printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
                    printSockAddr(myName, con_leHostSockAddr);
                }
                #ifndef __SYNTHESIS__
                  con_watchDogTimer = 250;
                #else
                  con_watchDogTimer = 10000;
                #endif
            }

        }
        break;
    case OPN_DONE:
        break;
    }
}

/*****************************************************************************
 * @brief Request the SHELL/NTS/TOE to start listening (LSn) for incoming
 *  connections on a specific port (.i.e open connection for reception mode).
 *
 * @param[in]  piSHL_Enable,   enable signal from [SHELL].
 * @param[out] soSHL_LsnReq,   listen port request to [SHELL].
 * @param[in]  siSHL_LsnAck,   listen port acknowledge from [SHELL].
 *
 * @warning
 *  The Port Table (PRt) of SHELL/NTS/TOE supports two port ranges; one for
 *   static ports (0 to 32,767) which are used for listening ports, and one for
 *   dynamically assigned or ephemeral ports (32,768 to 65,535) which are used
 *   for active connections. Therefore, listening port numbers must be in the
 *   range 0 to 32,767.
 ******************************************************************************/
void pListen(
        ap_uint<1>            *piSHL_Enable,
        stream<AppLsnReqAxis> &soSHL_LsnReq,
        stream<AppLsnAckAxis> &siSHL_LsnAck)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char    *myName      = concat3(THIS_NAME, "/", "LSn");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { LSN_IDLE, LSN_SEND_REQ, LSN_WAIT_ACK, LSN_DONE } \
	                           lsn_fsmState=LSN_IDLE;
    #pragma HLS reset variable=lsn_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<8>  lsn_watchDogTimer;

    switch (lsn_fsmState) {

    case LSN_IDLE:
        if (*piSHL_Enable != 1)
            return;
        else
            lsn_fsmState = LSN_SEND_REQ;
        break;

    case LSN_SEND_REQ:
        if (!soSHL_LsnReq.full()) {
            TcpPort  tcpListenPort = DEFAULT_FPGA_LSN_PORT;
            soSHL_LsnReq.write(tcpListenPort);
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
            #ifndef __SYNTHESIS__
                lsn_watchDogTimer = 10;
            #else
                lsn_watchDogTimer = 100;
            #endif
            lsn_fsmState = LSN_WAIT_ACK;
            }
        }
        else {
            printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
        }
        break;

    case LSN_WAIT_ACK:
        lsn_watchDogTimer--;
        if (!siSHL_LsnAck.empty()) {
            AppLsnAckAxis  listenDone;
            siSHL_LsnAck.read(listenDone);
            if (listenDone.tdata) {
                printInfo(myName, "Received listen acknowledgment from [TOE].\n");
                lsn_fsmState = LSN_DONE;
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        else {
            if (lsn_watchDogTimer == 0) {
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                           DEFAULT_FPGA_LSN_PORT, DEFAULT_FPGA_LSN_PORT);
                lsn_fsmState = LSN_SEND_REQ;
            }
        }
        break;

    case LSN_DONE:
        break;
    }  // End-of: switch()
}

/*****************************************************************************
 * @brief ReadRequestHandler (RRh).
 *  Waits for a notification indicating the availability of new data for
 *  the ROLE/TAF. If the TCP segment length is greater than 0, the notification
 *  is accepted.
 *
 * @param[in]  siSHL_Notif, a new Rx data notification from [SHELL].
 * @param[out] soSHL_DReq,  a Rx data request to [SHELL].
 *
 * @return Nothing.
 ******************************************************************************/
void pReadRequestHandler(
        stream<AppNotif>    &siSHL_Notif,
        stream<AppRdReq>    &soSHL_DReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RRh");

    //-- LOCAL STATIC VARIABLES W/ RESET --------------------------------------
    static enum FsmStates { RRH_WAIT_NOTIF, RRH_SEND_DREQ } \
	                           rrh_fsmState=RRH_WAIT_NOTIF;
    #pragma HLS reset variable=rrh_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static   AppNotif rrh_notif;

    switch(rrh_fsmState) {
    case RRH_WAIT_NOTIF:
        if (!siSHL_Notif.empty()) {
            siSHL_Notif.read(rrh_notif);
            if (rrh_notif.tcpSegLen != 0) {
                // Always request the data segment to be received
                rrh_fsmState = RRH_SEND_DREQ;
            }
        }
        break;
    case RRH_SEND_DREQ:
        if (!soSHL_DReq.full()) {
            soSHL_DReq.write(AppRdReq(rrh_notif.sessionID, rrh_notif.tcpSegLen));
            rrh_fsmState = RRH_WAIT_NOTIF;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Read Path (RDp) - From SHELL/TOE to ROLE/TAF.
 *  Process waits for a new data segment to read and forwards it to the TAF.
 *
 * @param[in]  siSHL_Data,   Data from [SHELL].
 * @param[in]  siSHL_SessId, Session Id from [SHELL].
 * @param[out] soTAF_Data,   Data to [TAF].
 * @param[out] soTAF_SessId, Session Id to [TAF].
 *
 * @return Nothing.
 *****************************************************************************/
void pReadPath(
        stream<AppData>     &siSHL_Data,
        stream<AppMetaAxis> &siSHL_SessId,
        stream<AppData>     &soTAF_Data,
        stream<AppMetaAxis> &soTAF_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "RDp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { RDP_WAIT_META=0, RDP_STREAM } \
	                           rdp_fsmState = RDP_WAIT_META;
    #pragma HLS reset variable=rdp_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AppData     currWord;
    AppMetaAxis sessId;

    switch (rdp_fsmState ) {
    case RDP_WAIT_META:
        if (!siSHL_SessId.empty() and !soTAF_SessId.full()) {
            siSHL_SessId.read(sessId);
            soTAF_SessId.write(sessId);
            rdp_fsmState  = RDP_STREAM;
        }
        break;
    case RDP_STREAM:
        if (!siSHL_Data.empty() && !soTAF_Data.full()) {
            siSHL_Data.read(currWord);
            if (DEBUG_LEVEL & TRACE_RDP) {
                 printAxiWord(myName, currWord);
            }
            soTAF_Data.write(currWord);
            if (currWord.tlast)
                rdp_fsmState  = RDP_WAIT_META;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Write Path (WRp) - From ROLE/TAF to SHELL/NTS/TOE.
 *  Process waits for a new data segment to write and forwards it to SHELL.
 *
 * @param[in]  siTAF_Data,   Tx data from [ROLE/TAF].
 * @param[in]  siTAF_SessId, the session Id from [ROLE/TAF].
 * @param[out] soSHL_Data,   Tx data to [SHELL].
 * @param[out] soSHL_SessId, Tx session Id to to [SHELL].
 * @param[in]  siSHL_DSts,   Tx data write status from [SHELL].
 *
 * @return Nothing.
 ******************************************************************************/
void pWritePath(
        stream<AppData>      &siTAF_Data,
        stream<AppMetaAxis>  &siTAF_SessId,
        stream<AppData>      &soSHL_Data,
        stream<AppMetaAxis>  &soSHL_SessId,
        stream<AppWrSts>     &siSHL_DSts)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "WRp");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { WRP_WAIT_META=0, WRP_STREAM } \
                               wrp_fsmState = WRP_WAIT_META;
    #pragma HLS reset variable=wrp_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AppMetaAxis   tcpSessId;
    AxiWord       currWordIn;

    switch (wrp_fsmState) {
    case WRP_WAIT_META:
        //-- Always read the session Id provided by [ROLE/TAF]
        if (!siTAF_SessId.empty() and !soSHL_SessId.full()) {
            siTAF_SessId.read(tcpSessId);
            soSHL_SessId.write(tcpSessId);
            if (DEBUG_LEVEL & TRACE_WRP) {
                printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                          tcpSessId.tdata.to_uint());
            }
            wrp_fsmState = WRP_STREAM;
        }
        break;
    case WRP_STREAM:
        if (!siTAF_Data.empty() && !soSHL_Data.full()) {
            siTAF_Data.read(currWordIn);
            if (DEBUG_LEVEL & TRACE_WRP) {
                 printAxiWord(myName, currWordIn);
            }
            soSHL_Data.write(currWordIn);
            if(currWordIn.tlast)
                wrp_fsmState = WRP_WAIT_META;
        }
        break;
    }

    //-- ALWAYS -----------------------
    if (!siSHL_DSts.empty()) {
        siSHL_DSts.read();  // [TODO] Checking.
    }
}


/*****************************************************************************
 * @brief   Main process of the TCP Shell Interface (TSIF)
 *
 * @param[in]  piSHL_Mmio_En Enable signal from [SHELL/MMIO].
 * @param[in]  siTAF_Data    TCP data stream from TcpAppFlash (TAF).
 * @param[in]  siTAF_SessId  TCP session Id  from [TAF].
 * @param[out] soTAF_Data    TCP data stream to   [TAF].
 * @param[out] soTAF_SessId  TCP session Id  to   [TAF].
 * @param[in]  siSHL_Notif   TCP data notification from [SHELL].
 * @param[out] soSHL_DReq    TCP data request to [SHELL].
 * @param[in]  siSHL_Data    TCP data stream from [SHELL].
 * @param[in]  siSHL_SessId  TCP metadata from [SHELL].
 * @param[out] soSHL_LsnReq  TCP listen port request to [SHELL].
 * @param[in]  siSHL_LsnAck  TCP listen port acknowledge from [SHELL].
 * @param[out] soSHL_Data    TCP data stream to [SHELL].
 * @param[out] soSHL_SessId  TCP metadata to [SHELL].
 * @param[in]  siSHL_DSts    TCP write data status from [SHELL].
 * @param[out] soSHL_OpnReq  TCP open connection request to [SHELL].
 * @param[in]  siSHL_OpnRep  TCP open connection reply from [SHELL].
 * @param[out] soSHL_ClsReq  TCP close connection request to [SHELL].
 * @param[out] poTAF_SConId  TCP session connect id to [ROLE/TAF].
 *
 * @return Nothing.
 *
 *****************************************************************************/
void tcp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- TAF / TxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &siTAF_Data,
        stream<AppMetaAxis>   &siTAF_SessId,

        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &soTAF_Data,
        stream<AppMetaAxis>   &soTAF_SessId,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>      &siSHL_Notif,
        stream<AppRdReq>      &soSHL_DReq,
        stream<AppData>       &siSHL_Data,
        stream<AppMetaAxis>   &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReqAxis> &soSHL_LsnReq,
        stream<AppLsnAckAxis> &siSHL_LsnAck,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>       &soSHL_Data,
        stream<AppMetaAxis>   &soSHL_SessId,
        stream<AppWrSts>      &siSHL_DSts,

        //------------------------------------------------------
        //-- SHELL / Tx Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>     &soSHL_OpnReq,
        stream<AppOpnRep>     &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReqAxis> &soSHL_ClsReq,

        //------------------------------------------------------
        //-- TAF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId             *poTAF_SConId)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

  #if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En    name=piSHL_Mmio_En

    #pragma HLS resource core=AXI4Stream variable=siTAF_Data   metadata="-bus_bundle siTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTAF_SessId metadata="-bus_bundle siTAF_SessId"

    #pragma HLS resource core=AXI4Stream variable=soTAF_Data   metadata="-bus_bundle soTAF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTAF_SessId metadata="-bus_bundle soTAF_SessId"

    #pragma HLS resource core=AXI4Stream variable=siSHL_Notif  metadata="-bus_bundle siSHL_Notif"
    #pragma HLS DATA_PACK                variable=siSHL_Notif
    #pragma HLS resource core=AXI4Stream variable=soSHL_DReq   metadata="-bus_bundle soSHL_DReq"
    #pragma HLS DATA_PACK                variable=soSHL_DReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_Data   metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_SessId metadata="-bus_bundle siSHL_SessId"

    #pragma HLS resource core=AXI4Stream variable=soSHL_LsnReq metadata="-bus_bundle soSHL_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=siSHL_LsnAck metadata="-bus_bundle siSHL_LsnAck"

    #pragma HLS resource core=AXI4Stream variable=soSHL_Data   metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_SessId metadata="-bus_bundle soSHL_SessId"
    #pragma HLS resource core=AXI4Stream variable=siSHL_DSts   metadata="-bus_bundle siSHL_DSts"
    #pragma HLS DATA_PACK                variable=siSHL_DSts

    #pragma HLS resource core=AXI4Stream variable=soSHL_OpnReq metadata="-bus_bundle soSHL_OpnReq"
    #pragma HLS DATA_PACK                variable=soSHL_OpnReq
    #pragma HLS resource core=AXI4Stream variable=siSHL_OpnRep metadata="-bus_bundle siSHL_OpnRep"
    #pragma HLS DATA_PACK                variable=siSHL_OpnRep

    #pragma HLS resource core=AXI4Stream variable=soSHL_ClsReq metadata="-bus_bundle soSHL_ClsReq"

    #pragma HLS INTERFACE ap_vld register    port=poTAF_SConId

  #else

    #pragma HLS INTERFACE ap_stable          port=piSHL_Mmio_En  name=piSHL_Mmio_En

    #pragma HLS INTERFACE axis register both port=siTAF_Data     name=siTAF_Data
    #pragma HLS INTERFACE axis register both port=siTAF_SessId   name=siTAF_SessId

    #pragma HLS INTERFACE axis register both port=soTAF_Data     name=soTAF_Data
    #pragma HLS INTERFACE axis register both port=soTAF_SessId   name=soTAF_SessId

    #pragma HLS INTERFACE axis register both port=siSHL_Notif    name=siSHL_Notif
    #pragma HLS DATA_PACK                variable=siSHL_Notif
    #pragma HLS INTERFACE axis register both port=soSHL_DReq     name=soSHL_DReq
    #pragma HLS DATA_PACK                variable=soSHL_DReq
    #pragma HLS INTERFACE axis register both port=siSHL_Data     name=siSHL_Data
    #pragma HLS INTERFACE axis register both port=siSHL_SessId   name=siSHL_SessId

    #pragma HLS INTERFACE axis register both port=soSHL_LsnReq   name=soSHL_LsnReq
    #pragma HLS INTERFACE axis register both port=siSHL_LsnAck   name=siSHL_LsnAck

    #pragma HLS INTERFACE axis register both port=soSHL_Data     name=soSHL_Data
    #pragma HLS INTERFACE axis register both port=soSHL_SessId   name=soSHL_SessId
    #pragma HLS INTERFACE axis register both port=siSHL_DSts     name=siSHL_DSts
    #pragma HLS DATA_PACK                variable=siSHL_DSts

    #pragma HLS INTERFACE axis register both port=soSHL_OpnReq   name=soSHL_OpnReq
    #pragma HLS DATA_PACK                variable=soSHL_OpnReq
    #pragma HLS INTERFACE axis register both port=siSHL_OpnRep   name=siSHL_OpnRep
    #pragma HLS DATA_PACK                variable=siSHL_OpnRep

    #pragma HLS INTERFACE axis register both port=soSHL_ClsReq   name=soSHL_ClsReq

    #pragma HLS INTERFACE ap_vld register    port=poTAF_SConId

  #endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS --------------------------------------------------------

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pConnect(
            piSHL_Mmio_En,
            soSHL_OpnReq,
            siSHL_OpnRep,
            soSHL_ClsReq,
            poTAF_SConId);

    pListen(
            piSHL_Mmio_En,
            soSHL_LsnReq,
            siSHL_LsnAck);

    pReadRequestHandler(
            siSHL_Notif,
            soSHL_DReq);

    pReadPath(
            siSHL_Data,
            siSHL_SessId,
            soTAF_Data,
            soTAF_SessId);

    pWritePath(
            siTAF_Data,
            siTAF_SessId,
            soSHL_Data,
            soSHL_SessId,
            siSHL_DSts);

}

