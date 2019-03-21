/*****************************************************************************
 * @file       : tcp_role_if.cpp
 * @brief      : TCP Role Interface (TRIF)
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
 * @details    : This process exposes the TCP data path to the role. The main
 *  purpose of the TRIF is to hide the socket mechanism from the user's role
 *  application. As a result, the user application sends and receives its data
 *  octets over plain Tx and Rx AXI4-Stream interfaces.
 *
 * @note       : The Tx data path (i.e. THIS->TOE) will block until a data
 *                chunk and its corresponding metadata is received by the Rx
 *                path.
 * @remark     : { remark text }
 * @warning    : 
 * @todo       : [TODO - Port# must configurable]
 *
 * @see        :
 *
 *****************************************************************************/

#include "tcp_role_if.hpp"
#include "../../toe/src/toe_utils.hpp"

#include <stdint.h>

using namespace hls;
using namespace std;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TRIF"

#define TRACE_OFF  0x0000
#define TRACE_RXP 1 <<  1
#define TRACE_TXP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_OPN 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_LSN | TRACE_OPN)


enum DropCmd {KEEP_CMD=false, DROP_CMD};


/*****************************************************************************
 * @brief SessionIdCam - Default constructor.
 * @ingroup udp_role_if
 ******************************************************************************/
SessionIdCam::SessionIdCam()
{
    for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i)
        this->CAM[i].valid = 0;
}

/*****************************************************************************
 * @brief SessionIdCam - Write a new entry into the CAM.
 * @ingroup udp_role_if
 *
 * @param[in] wrEntry, the entry to write into the CAM.
 *
 * @return true if the entry was written, otherwise false.
 ******************************************************************************/
bool SessionIdCam::write(SessionIdCamEntry wrEntry)
{
    for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i) {
        // Go through all the entries in the CAM
        #pragma HLS UNROLL
            if ((this->CAM[i].valid == 0) && (wrEntry.valid == true)) {
                // Write new entry
                this->CAM[i].sessId = wrEntry.sessId;
                this->CAM[i].valid  = true;
                return true;
            } else if ((this->CAM[i].sessId = wrEntry.sessId) && (wrEntry.valid == false)) {
                // Clear the valid bit of the current entry
                this->CAM[i].valid  = false;
                return true;
            }
    }
    return false;
}

/*****************************************************************************
 * @brief SessionIdCam::search
 *  Search the CAM for a valid session.
 *
 * @ingroup udp_role_if
 *
 * @param[in] sessId, the session ID to lookup.
 *
 * @return true if session ID is valid otherwise false;
 ******************************************************************************/
bool SessionIdCam::search(SessionId sessId)
{
    for (uint8_t i=0; i<NR_SESSION_ENTRIES; ++i) {
        // Go through all the entries in the CAM
        #pragma HLS UNROLL
            if ((this->CAM[i].valid == 1) && (sessId == this->CAM[i].sessId)) {
                // The entry is valid and the session ID matches
                return (true);
            }
    }
    return false;
}


/*****************************************************************************
 * @brief Open/Close a Connection for transmission (OPn).
 *
 * @ingroup trif
 *
 * @param[in]  piSHL_SysRst, System reset from [SHELL].
 * @param[in]  piSHL_SysRst,  A system reset that we control (not HLS).
 * @param[in]  siSAm_SockAddr,the socket address to open from [SessionAcceptManager].
 * @param[in]  siTOE_OpnSts,  open connection status from TOE.
 * @param[out] soTOE_OpnReq,  open connection request to TOE.
 * @param[out] soTOE_ClsReq,  close connection request to TOE.
 *
 * @return Nothing.
 *
 * @note  This code is not executed. It is added here to terminate every HLS
 *          stream of the module.
 ******************************************************************************/
void pOpenCloseConn(
        ap_uint<1>           piSHL_SysRst,
        stream<SockAddr>    &siSAm_SockAddr,
        stream<AppOpnSts>   &siTOE_OpnSts,
        stream<AppOpnReq>   &soTOE_OpnReq,
        stream<AppClsReq>   &soTOE_ClsReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "OPn");

    AppOpnSts   newConn;
    SockAddr    sockAddr;     // Socket Address stored in NETWORK BYTE ORDER
    AppOpnReq   macSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER

    static ap_uint<8>   idleTime;
    static enum FSM_STATE { FSM_IDLE, FSM_OPN_REQ, FSM_OPN_STS, FSM_OPN_DONE } fsmOpen=FSM_IDLE;

    if (piSHL_SysRst == 1) {
        #ifndef __SYNTHESIS__
            idleTime =   5;
        #else
            idleTime = 255;
        #endif
        fsmOpen = FSM_IDLE;
        return;
    }

    /*** OBSOLETE ********
    if (!openConStatus.empty() && !openConnection.full() && !closeConnection.full()) {
    		openConStatus.read(newConn);
    		tuple.ip_address = 0x0b010101; //0x0a010101;
    		tuple.ip_port =  0x3412;//0x4412;
    		openConnection.write(tuple);
    		if (newConn.success) {
    			closeConnection.write(newConn.sessionID);
    			//closePort.write(tuple.ip_port);
    		}
    	}
    **********************/

    switch (fsmOpen) {
        case FSM_IDLE:
            if (idleTime > 0)
                idleTime--;
            else
                fsmOpen = FSM_OPN_REQ;
            break;
        case FSM_OPN_REQ:
            if (!soTOE_OpnReq.full()) {
                SockAddr    sockAddr(0x0A0CC832, 8803);   // {10.12.200.50, 8803}
                macSockAddr.addr = byteSwap32(sockAddr.addr);
                macSockAddr.port = byteSwap16(sockAddr.port);
                soTOE_OpnReq.write(macSockAddr);
                if (DEBUG_LEVEL & TRACE_OPN) {
                    printInfo(myName, "Request to open SockAddr={0x%8.8X, 0x%4.4X}.\n",
                              sockAddr.addr.to_uint(), sockAddr.port.to_uint());
                }
                fsmOpen = FSM_OPN_STS;
            }
            else if (!siTOE_OpnSts.empty()) {
                // Drain the status stream
                siTOE_OpnSts.read();
            }
            break;
        case FSM_OPN_STS:
            if (!siTOE_OpnSts.empty()) {
                // Read the status stream
                siTOE_OpnSts.read(newConn);
                if (!newConn.success) {
                    soTOE_ClsReq.write(newConn.sessionID);
                    if (DEBUG_LEVEL & TRACE_OPN) {
                        printInfo(myName, "Request to close sessionId=%d.\n", newConn.sessionID.to_uint());
                    }
                    fsmOpen = FSM_IDLE;
                }
                else {
                    if (DEBUG_LEVEL & TRACE_OPN) {
                        printInfo(myName, "Socket was successfully opened..\n");
                    }
                    fsmOpen = FSM_OPN_DONE;
                }
            }
            break;
        case FSM_OPN_DONE:
            break;
    }

    /*** OBSOLETE *********
    if (!siTOE_OpnSts.empty() && !soTOE_OpnReq.full() && !soTOE_ClsReq.full()) {
        siTOE_OpnSts.read(newConn);
        macSockAddr.addr = byteSwap32(0x0A0CC832);    // 10.12.200.50
        macSockAddr.port = byteSwap16(8803);
        soTOE_OpnReq.write(macSockAddr);
        if (DEBUG_LEVEL & TRACE_OPN) {
            printInfo(myName, "Request to open SockAddr={0x%8.8X, 0x%4.4X}.\n",
                               sockAddr.addr.to_uint(), sockAddr.port.to_uint());
        }
        if (!newConn.success) {
            soTOE_ClsReq.write(newConn.sessionID);
            if (DEBUG_LEVEL & TRACE_OPN) {
                printInfo(myName, "Request to close sessionId=%d.\n", newConn.sessionID.to_uint());
            }
        }
    }

    if (!siSAm_SockAddr.empty()) {
        siSAm_SockAddr.read(sockAddr);
    }
    *************************/

    /*** [FIXME] *******
    if (!siSAm_SockAddr.empty() && !soTOE_OpnReq.full()) {
        //OBSOLETE sockAddr.addr = 0x32C80C0A;    // reversed byte order of 10.12.200.50
        //OBSOLETE sockAddr.port = 0x5050;        // reversed byte order of 20560
        siSAm_SockAddr.read(sockAddr);
        macSockAddr.addr = byteSwap32(sockAddr.addr);
        macSockAddr.port = byteSwap16(sockAddr.port);
        soTOE_OpnReq.write(macSockAddr);
        if (DEBUG_LEVEL & TRACE_OPN) {
            printInfo(myName, "Request to open SockAddr={0x%8.8X, 0x%4.4X}.\n",
                              sockAddr.addr.to_uint(), sockAddr.port.to_uint());
        }
    }
    else if (!siTOE_OpnSts.empty()) {
        siTOE_OpnSts.read(newConn);
        // Here we add some dummy code which should never be executed but is
        // required to avoid the 'ClsReq' ports to be synthesized away during optimization.
        if (!newConn.success) {
            soTOE_ClsReq.write(newConn.sessionID);
        }
    }
    ********************/

}


/*****************************************************************************
 * @brief Request the TOE to start listening (LSn) for incoming connections
 *  on a specific port (.i.e open connection for reception).
 *
 * @ingroup trif
 *
 * @param[in]  piSHL_SysRst,  System reset from [SHELL].
 * @param[in]  siTOE_LsnAck,  listen port acknowledge from TOE.
 * @param[out] soTOE_LsnReq,  listen port request to TOE.
 *
 * @warning
 *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
 *   32,767) which are used for listening ports, and one for dynamically
 *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
 *   connections. Therefore, listening port numbers should be in the range 0
 *   to 32,767.
 * 
 * @return Nothing.
 ******************************************************************************/
void pListen(
        ap_uint<1>           piSHL_SysRst,
        stream<AppLsnAck>   &siTOE_LsnAck,
        stream<AppLsnReq>   &soTOE_LsnReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "LSn");
    const int  cDefLsnPort = 8803;  // FYI, this is the ZIP code of Ruschlikon ;-)

    static bool  lsnAck      = false;
    static bool  isListening = false;

    if (piSHL_SysRst == 1) {
        lsnAck      = false;
        isListening = false;
        return;
    }

    if (!isListening) {
        // Request to listen on a default port
        if (!lsnAck && !soTOE_LsnReq.full()) {
            soTOE_LsnReq.write(cDefLsnPort);
            isListening = true;
            // [TODO-Add an MMIO status indicating that we are listening.]
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Request to listen on port #%d.\n", cDefLsnPort);
            }
        }
    } else if (isListening && !siTOE_LsnAck.empty()) {
        // Check if the listening request was successful, otherwise try again
        siTOE_LsnAck.read(lsnAck);
        if (lsnAck == false)
            isListening = false;
        else {
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Received listen acknowledgement from TOE.\n");
            }
        }
    }

}


/*****************************************************************************
 * @brief Session Accept Manager (SAm).
 *  Waits for a notification indicating the availability of new data for
 *  the ROLE. If the session is valid, data will be forwarded to the ROLE
 *  by the [RxPath] process, otherwise the incoming data will be dumped.
 *  Also, if a session is valid but destination socket is not yet open, a
 *  request to open that socket is sent to the [Open] process. Otherwise,
 *  a drop command is sent to the [RxPath] process.
 *
 * @ingroup trif
 *
 * @param[in]  piSHL_SysRst,   System reset from [SHELL].
 * @param[in]  siTOE_Notif,    a new Rx data notification from TOE.
 * @param[out] soTOE_DReq,     a Rx data request to TOE.
 * @param[out] soOPn_SockAddr, the socket address to open.
 * @param[out] soRXp_DropCmd,  a drop command to the [RxPath] process.
 *
 * @return Nothing.
 ******************************************************************************/
void pSessionAcceptManager(
        ap_uint<1>           piSHL_SysRst,
        stream<AppNotif>    &siTOE_Notif,
        stream<AppRdReq>    &soTOE_DReq,
        stream<SockAddr>    &soOPn_SockAddr,
        stream<CmdBit>      &soRXp_DropCmd)
{
    const char *myName  = concat3(THIS_NAME, "/", "SAm");

    //-- LOCAL VARIABLES ------------------------------------------------------
    static  SessionIdCam                 sessionIdCam;
    #pragma HLS array_partition variable=sessionIdCam.CAM complete

    //OBSOLETE static SessionId  drr_sessId  = 99;
    //OBSOLETE static bool       drr_firstWr = true;
    AppNotif notif;
    bool     sessStatus = false;

    static ap_uint<8>   nrSess   = 0;
    static enum FSM_STATE { FSM_WAIT_NOTIFICATION, FSM_WRITE_SESSION } fsmAccept=FSM_WAIT_NOTIFICATION;

    if (piSHL_SysRst == 1) {
        nrSess    = 0;
        fsmAccept = FSM_WAIT_NOTIFICATION;
    }

    switch(fsmAccept) {

        /****
         case FSM_WAIT_NOTIFICATION:
            if (!siTOE_Notif.empty() && !soTOE_DReq.full() &&
                !soOPn_SockAddr.full() && !soRXp_DropCmd.full() ) {
                siTOE_Notif.read(notif);
                if (notif.tcpSegLen == 0)
                    return;
                // Always request the data segment to be received
                soTOE_DReq.write(AppRdReq(notif.sessionID, notif.tcpSegLen));
                // Search if session was previously accepted
                sessStatus = sessionIdCam.search(notif.sessionID);
                if (sessStatus == true) {
                    soRXp_DropCmd.write(KEEP_CMD);
                }
                else {
                    if (nrSess < TRIF_MAX_SESSIONS) {
                        // Open a socket for this connection
                        //  WARNING: The Port Table (PRt) of TOE supports two port ranges;
                        //   one for static ports (0 to 32,767) which are used for listening
                        //   ports, and one for dynamically assigned or ephemeral ports
                        //   (32,768 to 65,535) which are used for active connections.
                        //   Therefore, ephemeral port numbers should be in the range 0 to
                        //   32,767.
                        //  [FIXME: The 'Notif' class should include the source socket address]
                        TcpPort tcpSrcPort;
                        if (notif.tcpDstPort < 0x8000)
                            tcpSrcPort = notif.tcpDstPort + 0x8000;
                        else
                            tcpSrcPort = notif.tcpDstPort;
                        soOPn_SockAddr.write(SockAddr(notif.ip4SrcAddr, tcpSrcPort));
                        // Accept the incoming data segment
                        soRXp_DropCmd.write(KEEP_CMD);
                        // Add this session ID to the CAM
                        fsmAccept = FSM_WRITE_SESSION;
                    }
                    else {
                        // Drop the incoming data segment
                        soRXp_DropCmd.write(DROP_CMD);
                    }
                }
            }
            break;

        case FSM_WRITE_SESSION:
            sessionIdCam.write(SessionIdCamEntry(notif.sessionID, true));
            nrSess++;
            fsmAccept = FSM_WAIT_NOTIFICATION;
            break;
         ****/

        case FSM_WAIT_NOTIFICATION:
            if (!siTOE_Notif.empty() && !soTOE_DReq.full()) {
                siTOE_Notif.read(notif);
                if (notif.tcpSegLen == 0)
                    return;
                // Always request the data segment to be received
                soTOE_DReq.write(AppRdReq(notif.sessionID, notif.tcpSegLen));
            }
            break;
    }

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    //OBSOLETE #pragma HLS reset variable=fsmAccept

}


/*****************************************************************************
 * @brief Rx Path (RXp) - From TOE to ROLE.
 *  Process waits for a new data segment and forwards it to the ROLE.
 *
 * @ingroup trif
 *
 * @param[in]  piSHL_SysRst, System reset from [SHELL].
 * @param[in]  siTOE_Data,   Data from TOE.
 * @param[in]  siTOE_Meta,   Metadata from TOE.
 * @param[out] soROL_Data,   Data to ROLE.
 * @param[in]  siSAm_DropCmd,Drop command from [SessionAcceptManager].
 * @param[out] soTXp_SessId, The incoming session ID forwarded to [TxPath].
 *
 * @return Nothing.
 *****************************************************************************/
void pRxPath(
        ap_uint<1>           piSHL_SysRst,
        stream<AppData>     &siTOE_Data,
        stream<AppMeta>     &siTOE_Meta,
        stream<AppData>     &soROL_Data,
        stream<ValBit>      &siSAm_DropCmd,
        stream<SessionId>   &soTXp_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
     #pragma HLS PIPELINE II=1
     //OBSOLETE #pragma HLS reset variable=fsmRxP

    const char *myName  = concat3(THIS_NAME, "/", "RXp");

    SessionId       sessionID;
    AppData         currWord;
    ValBit          dropCmd;

    static enum FSM_STATE { FSM_IDLE=0, FSM_STREAM, FSM_DROP } fsmRxP;

    switch (fsmRxP) {

        case FSM_IDLE:
            /****
            if (!siTOE_Meta.empty() && !siSAm_DropCmd.empty()) {
                siTOE_Meta.read(sessionID);
                siSAm_DropCmd.read(dropCmd);
                if (dropCmd == KEEP_CMD) {
                    soTXp_SessId.write(sessionID);
                    fsmRxP = FSM_STREAM;
                }
                else
                    fsmRxP = FSM_DROP;
            }
            ****/
            if (!siTOE_Meta.empty())  {
                siTOE_Meta.read(sessionID);
                soTXp_SessId.write(sessionID);
                fsmRxP = FSM_STREAM;
            }
            break;

        case FSM_STREAM:
            if (!siTOE_Data.empty() && !soROL_Data.full()) {
                siTOE_Data.read(currWord);
                soROL_Data.write(currWord);
                if (currWord.tlast)
                    fsmRxP = FSM_IDLE;
            }
            break;

        /***
        case FSM_DROP:
            if (!siTOE_Data.empty()) {
                siTOE_Data.read(currWord);
                if (currWord.tlast)
                    fsmRxP = FSM_IDLE;
            }
            break;
        ***/
    }

}


/*****************************************************************************
 * @brief Tx Path (Txp) - From ROLE to TOE.
 *  Process waits for a new data segment and forwards it to TOE.
 *
 * @ingroup trif
 *
 * @param[in]  piSHL_SysRst, System reset from [SHELL].
 * @param[in]  siROL_Data,   Tx data from ROLE.
 * @param[out] soTOE_Data,   Tx data to TOE.
 * @param[out] soTOE_Meta,   Tx metadata to to TOE.
 * @param[in]  siTOE_DSts,   Tx data write status from TOE.
 * @param[in]  siRXp_SessId, The session ID from [RxPath].
 *
 * @return Nothing.
 ******************************************************************************/
void pTxPath(
        ap_uint<1>            piSHL_SysRst,
        stream<AppData>      &siROL_Data,
        stream<AppData>      &soTOE_Data,
        stream<AppMeta>      &soTOE_Meta,
        stream<AppWrSts>     &siTOE_DSts,
        stream<SessionId>    &siRXp_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "TXp");

    AxiWord   currWordIn;
    AxiWord   currWordOut;
    SessionId tcpSessId;

    //-- FSM ----------------
    static enum FSM_STATE {FSM_IDLE=0, FSM_STREAM} fsmTxP;

    switch (fsmTxP) {

        case FSM_IDLE:
            if(!siROL_Data.empty() && !siRXp_SessId.empty() &&
               !soTOE_Data.full()  && !soTOE_Meta.full() ) {
                siRXp_SessId.read(tcpSessId);
                siROL_Data.read(currWordIn);
                if (DEBUG_LEVEL & TRACE_TXP) {
                    printInfo(myName, "Receiving data for session #%d\n", tcpSessId.to_int());
                    printAxiWord(myName, currWordIn);
                }
                if(!currWordIn.tlast) {
                    soTOE_Data.write(currWordIn);
                    soTOE_Meta.write(tcpSessId);
                    fsmTxP = FSM_STREAM;
                }
            }
        break;

        case FSM_STREAM:
            if (!siROL_Data.empty() && !soTOE_Data.full()) {
                siROL_Data.read(currWordIn);
                if (DEBUG_LEVEL & TRACE_TXP) {
                     printAxiWord(myName, currWordIn);
                }
                soTOE_Data.write(currWordIn);
                if(currWordIn.tlast)
                    fsmTxP = FSM_IDLE;
            }
        break;
    }

    //-- ALWAYS -----------------------
    if (!siTOE_DSts.empty()) {
        siTOE_DSts.read();  // [TODO] Checking.
    }

}


/*****************************************************************************
 * @brief   Main process of the TCP Role Interface (TRIF)
 * @ingroup tcp_role_if
 *
 * @param[in]  piSHL_SysRst      A system reset that we control (not HLS).
 * @param[in]  siROL_This_Data   TCP Rx data stream from ROLE.
 * @param[out] soTHIS_Rol_Data   TCP Tx data stream to ROLE.
 * @param[in]  siTOE_This_Notif  TCP Rx data notification from TOE.
 * @param[out] soTHIS_Toe_RdReq  TCP Rx data request to TOE.
 * @param[in]  siTOE_This_Data   TCP Rx data from TOE.
 * @param[in]  siTOE_This_Meta   TCP Rx metadata from TOE.
 * @param[out] soTHIS_Toe_LsnReq TCP Rx listen port request to TOE.
 * @param[in]  siTOE_This_LsnAck TCP Rx listen port acknowledge from TOE.
 * @param[out] soTHIS_Toe_Data   TCP Tx data to TOE.
 * @param[out] soTHIS_Toe_Meta   TCP Tx metadata to TOE.
 * @param[in]  siTOE_This_DSts   TCP Tx data status from TOE.
 * @param[out] soTHIS_Toe_OpnReq TCP Tx open connection request to TOE.
 * @param[in]  siTOE_This_OpnSts TCP Tx open connection status from TOE.
 * @param[out] soTHIS_Toe_ClsReq TCP Tx close connection request to TOE.
 *
 * @return Nothing.
 *
 * @remark     : Session id is only updated if a new connection is established.
 *                Therefore, the role does not have to always return the same
 *                amount of segments received.
 *****************************************************************************/
void tcp_role_if(
        //------------------------------------------------------
        //-- SHELL / System Reset
        //------------------------------------------------------
        ap_uint<1>           piSHL_SysRst,

        //------------------------------------------------------
        //-- ROLE / Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siROL_This_Data,

        //------------------------------------------------------
        //-- ROLE / Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- TOE / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>    &siTOE_This_Notif,
        stream<AppRdReq>    &soTHIS_Toe_DReq,
        stream<AppData>     &siTOE_This_Data,
        stream<AppMeta>     &siTOE_This_Meta,

        //------------------------------------------------------
        //-- TOE / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReq>   &soTHIS_Toe_LsnReq,
        stream<AppLsnAck>   &siTOE_This_LsnAck,

        //------------------------------------------------------
        //-- TOE / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>     &soTHIS_Toe_Data,
        stream<AppMeta>     &soTHIS_Toe_Meta,
        stream<AppWrSts>    &siTOE_This_DSts,

        //------------------------------------------------------
        //-- TOE / Tx Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>   &soTHIS_Toe_OpnReq,
        stream<AppOpnSts>   &siTOE_This_OpnSts,

        //------------------------------------------------------
        //-- TOE / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReq>   &soTHIS_Toe_ClsReq)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable port=piSHL_SysRst

    #pragma HLS resource core=AXI4Stream variable=siROL_This_Data   metadata="-bus_bundle siROL_This_Data"

    #pragma HLS resource core=AXI4Stream variable=soTHIS_Rol_Data   metadata="-bus_bundle soTHIS_Rol_Data"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Notif  metadata="-bus_bundle siTOE_This_Notif"
    #pragma HLS DATA_PACK                variable=siTOE_This_Notif
    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Data   metadata="-bus_bundle siTOE_This_Data"
    #pragma HLS resource core=AXI4Stream variable=siTOE_This_Meta   metadata="-bus_bundle siTOE_This_Meta"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_DReq   metadata="-bus_bundle soTHIS_Toe_DReq"
    #pragma HLS DATA_PACK                variable=soTHIS_Toe_DReq

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_LsnAck metadata="-bus_bundle siTOE_This_LsnAck"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_LsnReq metadata="-bus_bundle soTHIS_Toe_LsnReq"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_DSts   metadata="-bus_bundle siTOE_This_DSts"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_Data   metadata="-bus_bundle soTHIS_Toe_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_Meta   metadata="-bus_bundle soTHIS_Toe_Meta"

    #pragma HLS resource core=AXI4Stream variable=siTOE_This_OpnSts metadata="-bus_bundle siTOE_This_OpnSts"
    #pragma HLS DATA_PACK                variable=siTOE_This_OpnSts
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_OpnReq metadata="-bus_bundle soTHIS_Toe_OpnReq"
    #pragma HLS DATA_PACK                variable=soTHIS_Toe_OpnReq
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Toe_ClsReq metadata="-bus_bundle soTHIS_Toe_ClsReq"

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<ValBit>              sSAmToRXp_DropCmd    ("sSAmToRXp_DropCmd");
    #pragma HLS_STREAM        variable=sSAmToRXp_DropCmd    depth=1

    static stream<SockAddr>            sSAmToOPn_SockAddr   ("sSAmToOPn_SockAddr");
    #pragma HLS_STREAM        variable=sSAmToOPn_SockAddr   depth=1

    static stream<TcpSessId>           sRXpToTXp_SessId     ("sRXpToTXp_SessId");
    #pragma HLS STREAM        variable=sRXpToTXp_SessId     depth=1

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pOpenCloseConn(
            piSHL_SysRst,
            sSAmToOPn_SockAddr,
            siTOE_This_OpnSts,
            soTHIS_Toe_OpnReq,
            soTHIS_Toe_ClsReq);

    pListen(
            piSHL_SysRst,
            siTOE_This_LsnAck,
            soTHIS_Toe_LsnReq);

    pSessionAcceptManager(
            piSHL_SysRst,
            siTOE_This_Notif,
            soTHIS_Toe_DReq,
            sSAmToOPn_SockAddr,
            sSAmToRXp_DropCmd);

    pRxPath(
            piSHL_SysRst,
            siTOE_This_Data,
            siTOE_This_Meta,
            soTHIS_Rol_Data,
            sSAmToRXp_DropCmd,
            sRXpToTXp_SessId);

    pTxPath(
            piSHL_SysRst,
            siROL_This_Data,
            soTHIS_Toe_Data,
            soTHIS_Toe_Meta,
            siTOE_This_DSts,
            sRXpToTXp_SessId);

}

