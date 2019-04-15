/*****************************************************************************
 * @file       : test_tcp_role_if.cpp
 * @brief      : Testbench for the TCP Role Interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include <iostream>

#include "../src/tcp_role_if.hpp"
#include "../../toe/src/toe_utils.hpp"

using namespace hls;

using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_TOE    1 <<  1
#define TRACE_ROLE   1 <<  2
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_TOE | TRACE_ROLE)

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
#define MAX_SIM_CYCLES 500

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gMaxSimCycles   = 100;
bool            gTraceEvent     = false;


/*****************************************************************************
 * @brief Emulate the behavior of the ROLE.
 *
 * @param[out] siTRIF_Data,   Data from to [TcpRoleInterface].
 *
 * @details
 *
 * @ingroup test_tcp_role_if
 ******************************************************************************/
void pROLE(
        //-- TRIF / Rx Data Interface
        stream<AppData>     &siTRIF_Data,
        //-- TRIF / Tx Data Interface
        stream<AppData>     &soTOE_Data)
{
    AppData     currWord;

    const char *myRxName  = concat3(THIS_NAME, "/", "ROLE-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "ROLE-Tx");

    //------------------------------------------------------
    //-- ALWAYS READ INCOMING DATA STREAM AND ECHO IT  BACK
    //------------------------------------------------------
    if (!siTRIF_Data.empty() && !soTOE_Data.full()) {
        siTRIF_Data.read(currWord);
        if (DEBUG_LEVEL & TRACE_ROLE)
            printAxiWord(myRxName, currWord);
        soTOE_Data.write(currWord);
        if (DEBUG_LEVEL & TRACE_ROLE)
            printAxiWord(myTxName, currWord);
    }

}


/*****************************************************************************
 * @brief Emulate the behavior of the TCP Offload Engine (TOE).
 *
 * @param[out] soTRIF_Notif,  Notification from [TcpRoleInterface].
 * @param[in]  siTRIF_DReq,   Data read request to [TcpRoleInterface].
 * @param[out] soTRIF_Data,   Data to [TcpRoleInterface].
 * @param[out] soTRIF_Meta,   Metadata to [TcpRoleInterface].
 * @param[in]  siTRIF_LsnReq, A ref to listen port request from TRIF.
 * @param[out] soTRIF_LsnAck, A ref to listen port acknowledge to TOE.
 *
 * @details
 *
 * @ingroup test_tcp_role_if
 ******************************************************************************/
void pTOE(
        //-- TOE / Tx Data Interfaces
        stream<AppNotif>    &soTRIF_Notif,
        stream<AppRdReq>    &siTRIF_DReq,
        stream<AppData>     &soTRIF_Data,
        stream<AppMeta>     &soTRIF_Meta,
        //-- TOE / Listen Interfaces
        stream<AppLsnReq>   &siTRIF_LsnReq,
        stream<AppLsnAck>   &soTRIF_LsnAck,
        //-- TOE / Rx Data Interfaces
        stream<AppData>     &siTRIF_Data,
        stream<AppMeta>     &siTRIF_Meta,
        stream<AppWrSts>    &soTRIF_DSts,
        //-- TOE / Open Interfaces
        stream<AppOpnReq>   &siTRIF_OpnReq,
        stream<AppOpnSts>   &soTRIF_OpnSts)
{
    SessionId  sessionId  = 42;
    TcpSegLen  tcpSegLen  = 32;
    Ip4SrcAddr ip4SrcAddr = 0x0A0A0A0A;
    TcpDstPort tcpDstPort;

    static int  appDstPort = -1;
    static int  loop       = 1;

    static enum FSM_TXSTATE { FSM_WAIT4LSNREQ, FSM_NOTIF, FSM_WAIT4DREQ,
                              FSM_DATA, FSM_DONE} fsmTxState = FSM_WAIT4LSNREQ;

    static enum FSM_RXSTATE { FSM_IDLE, FSM_READ} fsmRxState = FSM_IDLE;

    const char *myName   = concat3(THIS_NAME, "/", "TOE");
    const char *myRxName = concat3(THIS_NAME, "/", "TOE-Rx");
    const char *myTxName = concat3(THIS_NAME, "/", "TOE-Tx");

    switch (fsmTxState) {

    case FSM_WAIT4LSNREQ:
        //------------------------------------------------------
        //-- STEP-1 : CHECK IF A LISTENING REQUEST IS PENDING
        //------------------------------------------------------
        if (!siTRIF_LsnReq.empty()) {
            AppLsnReq appLsnPortReq;
            siTRIF_LsnReq.read(appLsnPortReq);
            printInfo(myName, "Received a listen port request #%d from [TRIF].\n",
                              appLsnPortReq.to_int());
            soTRIF_LsnAck.write(true);
            appDstPort = appLsnPortReq.to_int();
            fsmTxState = FSM_NOTIF;
        }
        break;

    case FSM_NOTIF:
        //------------------------------------------------------
        //-- STEP-2 : SEND A NOTIFICATION TO TRIF
        //------------------------------------------------------
        sessionId  = 42;
        tcpSegLen  = 32;
        ip4SrcAddr = 0x0A0A0A0A;
        tcpDstPort = appDstPort;
        soTRIF_Notif.write(AppNotif(sessionId,  tcpSegLen,
                                    ip4SrcAddr, tcpDstPort));
        printInfo(myTxName, "Sending notification to [TRIF] (sessId=%d, segLen=%d).\n",
                             sessionId.to_int(), tcpSegLen.to_int());
        fsmTxState = FSM_WAIT4DREQ;
        break;

    case FSM_WAIT4DREQ:
         //------------------------------------------------------
         //-- STEP-3 : WAIT FOR A DATA REQUEST FROM TRIF
         //------------------------------------------------------
        if (!siTRIF_DReq.empty()) {
            AppRdReq appRdReq;
            siTRIF_DReq.read(appRdReq);
            printInfo(myName, "Received a data read request from [TRIF] (sessId=%d, segLen=%d).\n",
                               appRdReq.sessionID.to_int(), appRdReq.length.to_int());
            fsmTxState = FSM_DATA;
        }
        break;

    case FSM_DATA:
        //------------------------------------------------------
        //-- STEP-4 : FORWARD DATA and METADATA to TRIF
        //--  We always assume 'tcpSegLen' is multiple of 8B.
        //------------------------------------------------------
        for (int i=0; i<(tcpSegLen/8); i++) {
            if (i == 0) {
                soTRIF_Meta.write(sessionId);
            }
            ap_uint<64> data = i;
            ap_uint< 8> keep = 0xFF;
            ap_uint< 1> last = (i==(tcpSegLen/8)-1) ? 1 : 0;
            soTRIF_Data.write(AppData(data, keep, last));
        }
        fsmTxState = FSM_DONE;
        break;

    case FSM_DONE:
        //------------------------------------------------------
        //-- END OF THE FSM TX SEQUENCE
        //------------------------------------------------------
        break;

    }  // End-of: switch())

    //------------------------------------------------------
    //-- ALWAYS - Reply to Open Request from TRIF
    //------------------------------------------------------
    if (!siTRIF_OpnReq.empty()) {
        AppOpnReq   macSockAddr;
        OpenStatus  opnStatus(42, true);
        SockAddr    sockAddr;   // Socket Address stored in NETWORK BYTE ORDER

        siTRIF_OpnReq.read(macSockAddr);
        sockAddr.addr = byteSwap32(macSockAddr.addr);
        sockAddr.port = byteSwap16(macSockAddr.port);
        printInfo(myName, "Received a request to open a socket address.\n");
        printSockAddr(myName, sockAddr);
        soTRIF_OpnSts.write(opnStatus);
    }

    //------------------------------------------------------
    //-- ALWAYS - Drain the data coming from TRIF
    //------------------------------------------------------
    switch (fsmRxState) {
    case FSM_IDLE:
        if (!siTRIF_Meta.empty() && !siTRIF_Data.empty()) {
            AppData     appData;
            AppMeta     appMeta;
            siTRIF_Meta.read(appMeta);
            siTRIF_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_TOE) {
                printInfo(myRxName, "Receiving data for session #%d\n", appMeta.to_int());
                printAxiWord(myRxName, appData);
            }
            if (!appData.tlast)
                fsmRxState = FSM_READ;
        }
        break;
    case FSM_READ:
        if (!siTRIF_Data.empty()) {
            AppData     appData;
            siTRIF_Data.read(appData);
            if (DEBUG_LEVEL & TRACE_TOE)
                printAxiWord(myRxName, appData);
            if (appData.tlast)
                fsmRxState = FSM_IDLE;
        }
        break;
    }

}


/*****************************************************************************
 * @brief Main function.
 *
 * @ingroup test_tcp_role_if
 ******************************************************************************/

int main()
{

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    //-- SHELL / System Reset
    ap_uint<1>          piSHL_SysRst;
    //-- ROLE / Rx Data Interface
    stream<AppData>     sROLE_To_TRIF_Data     ("sROLE_To_TRIF_Data");
    //-- ROLE / This / Tx Data Interface
    stream<AppData>     sTRIF_To_ROLE_Data     ("sTRIF_To_ROLE_Data");
    //-- TOE  / Rx Data Interfaces
    stream<AppNotif>    sTOE_To_TRIF_Notif     ("sTOE_To_TRIF_Notif");
    stream<AppData>     sTOE_To_TRIF_Data      ("sTOE_To_TRIF_Data");
    stream<AppMeta>     sTOE_to_TRIF_Meta      ("sTOE_to_TRIF_Meta");
    stream<AppRdReq>    sTRIF_To_TOE_DReq      ("sTRIF_To_TOE_DReq");
    //-- TOE  / Listen Interfaces
    stream<AppLsnAck>   sTOE_To_TRIF_LsnAck    ("sTOE_To_TRIF_LsnAck");
    stream<AppLsnReq>   sTRIF_To_TOE_LsnReq    ("sTRIF_To_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<AppWrSts>    sTOE_To_TRIF_DSts      ("sTOE_To_TRIF_DSts");
    stream<AppData>     sTRIF_To_TOE_Data      ("sTRIF_To_TOE_Data");
    stream<AppMeta>     sTRIF_To_TOE_Meta      ("sTRIF_To_TOE_Meta");
    //-- TOE  / Open Interfaces
    stream<AppOpnSts>   sTOE_To_TRIF_OpnSts    ("sTOE_To_TRIF_OpnSts");
    stream<AppOpnReq>   sTRIF_To_TOE_OpnReq    ("sTRIF_To_TOE_OpnReq");
    stream<AppClsReq>   sTRIF_To_TOE_ClsReq    ("sTRIF_To_TOE_ClsReq");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    unsigned int    simCycCnt = 0;
    int             nrErr;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    simCycCnt = 0;  // Simulation cycle counter as a global variable
    nrErr     = 0;  // Total number of testbench errors

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {

        //-----------------------------------------------------
        //-- SYSTEM RESET
        //-----------------------------------------------------
        if (simCycCnt < 10)
            piSHL_SysRst.write(1);
        else
            piSHL_SysRst.write(0);

        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(
            //-- TOE / Tx Data Interfaces
            sTOE_To_TRIF_Notif,
            sTRIF_To_TOE_DReq,
            sTOE_To_TRIF_Data,
            sTOE_to_TRIF_Meta,
            //-- TOE / Listen Interfaces
            sTRIF_To_TOE_LsnReq,
            sTOE_To_TRIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            sTRIF_To_TOE_Data,
            sTRIF_To_TOE_Meta,
            sTOE_To_TRIF_DSts,
            //-- TOE / Open Interfaces
            sTRIF_To_TOE_OpnReq,
            sTOE_To_TRIF_OpnSts);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_role_if(
            //-- SHELL / System Reset
            piSHL_SysRst,
            //-- ROLE / Rx & Tx Data Interfaces
            sROLE_To_TRIF_Data,
            sTRIF_To_ROLE_Data,
            //-- TOE / Rx Data Interfaces
            sTOE_To_TRIF_Notif,
            sTRIF_To_TOE_DReq,
            sTOE_To_TRIF_Data,
            sTOE_to_TRIF_Meta,
            //-- TOE / Listen Interfaces
            sTRIF_To_TOE_LsnReq,
            sTOE_To_TRIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            sTRIF_To_TOE_Data,
            sTRIF_To_TOE_Meta,
            sTOE_To_TRIF_DSts,
            //-- TOE / Tx Open Interfaces
            sTRIF_To_TOE_OpnReq,
            sTOE_To_TRIF_OpnSts,
            //-- TOE / Close Interfaces
            sTRIF_To_TOE_ClsReq);

        //-------------------------------------------------
        //-- EMULATE APP
        //-------------------------------------------------
        pROLE(
            //-- TRIF / Rx Data Interface
            sTRIF_To_ROLE_Data,
            //-- TRIF / Tx Data Interface
            sROLE_To_TRIF_Data);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        simCycCnt++;
        if (gTraceEvent || ((simCycCnt % 100) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", simCycCnt);
            gTraceEvent = false;
        }

    } while (simCycCnt < gMaxSimCycles);


    printf("-- [@%4.4d] -----------------------------\n", simCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n\n");

    if (nrErr) {
         printError(THIS_NAME, "###########################################################\n");
         printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###########################################################\n");
     }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}
