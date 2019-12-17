/*****************************************************************************
 * @file       : test_tcp_role_if.cpp
 * @brief      : Testbench for the TCP Role Interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
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
#include "../../toe/test/test_toe_utils.hpp"

using namespace hls;

using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TOE    1 <<  1
#define TRACE_ROLE   1 <<  2
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_TOE | TRACE_ROLE)

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
#define MAX_SIM_CYCLES   500


//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent     = false;
bool            gFatalError     = false;
unsigned int    gMaxSimCycles   = 0x8000 + 200;

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x0057      // TOE listens on port     = 87 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   0x80        // TB listens on port      = 128

#define DEFAULT_SESSION_ID      42
#define DEFAULT_SESSION_LEN     32

/*****************************************************************************
 * @brief Emulate the behavior of the ROLE.
 *
 * @param[in]  siTRIF_Data,   Data to TcpRoleInterface (TRIF).
 * @param[in]  siTRIF_SessId, Session ID to [TRIF].
 * @param[out] soROLE_Data,   Data stream to [TRIF].
 * @param[out] soROLE_SessId  SessionID to [TRFI].
 * @details
 *
 * @ingroup test_tcp_role_if
 ******************************************************************************/
void pROLE(
        //-- TRIF / Rx Data Interface
        stream<AppData>     &siTRIF_Data,
        stream<AppMeta>     &siTRIF_SessId,
        //-- TRIF / Tx Data Interface
        stream<AppData>     &soTRIF_Data,
        stream<AppMeta>     &soTRIF_SessId)
{
    AppData     currWord;
    AppMeta     tcpSessId;

    const char *myRxName  = concat3(THIS_NAME, "/", "ROLE-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "ROLE-Tx");

    //------------------------------------------------------
    //-- ALWAYS READ INCOMING DATA STREAM AND ECHO IT  BACK
    //------------------------------------------------------
    static enum RxFsmStates { RX_WAIT_META=0, RX_STREAM } rxFsmState = RX_WAIT_META;
    #pragma HLS reset variable=rdpFsmState

    switch (rxFsmState ) {
    case RX_WAIT_META:
        if (!siTRIF_SessId.empty() and !soTRIF_SessId.full()) {
            siTRIF_SessId.read(tcpSessId);
            soTRIF_SessId.write(tcpSessId);
            rxFsmState  = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siTRIF_Data.empty() && !soTRIF_Data.full()) {
            siTRIF_Data.read(currWord);
            if (DEBUG_LEVEL & TRACE_ROLE) {
                 printAxiWord(myRxName, currWord);
            }
            soTRIF_Data.write(currWord);
            if (currWord.tlast)
                rxFsmState  = RX_WAIT_META;
        }
        break;
    }

}

/*****************************************************************************
 * @brief Emulate the behavior of the TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr,         A ref to the error counter of main.
 * @param[out] soTRIF_Notif,  Notification to TcpRoleInterface (TRIF).
 * @param[in]  siTRIF_DReq,   Data read request from [TRIF}.
 * @param[out] soTRIF_Data,   Data to [TRIF].
 * @param[out] soTRIF_SessId, Session Id to [TRIF].
 * @param[in]  siTRIF_LsnReq, Listen port request from [TRIF].
 * @param[out] soTRIF_LsnAck, Listen port acknowledge to [TRIF].
 *
 ******************************************************************************/
void pTOE(
        int                 &nrErr,
        //-- TOE / Tx Data Interfaces
        stream<AppNotif>    &soTRIF_Notif,
        stream<AppRdReq>    &siTRIF_DReq,
        stream<AppData>     &soTRIF_Data,
        stream<AppMeta>     &soTRIF_SessId,
        //-- TOE / Listen Interfaces
        stream<AppLsnReq>   &siTRIF_LsnReq,
        stream<AppLsnAck>   &soTRIF_LsnAck,
        //-- TOE / Rx Data Interfaces
        stream<AppData>     &siTRIF_Data,
        stream<AppMeta>     &siTRIF_SessId,
        stream<AppWrSts>    &soTRIF_DSts,
        //-- TOE / Open Interfaces
        stream<AppOpnReq>   &siTRIF_OpnReq,
        stream<AppOpnSts>   &soTRIF_OpnRep)
{
    TcpSegLen  tcpSegLen  = 32;

    static Ip4Addr hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
    static TcpPort fpgaLsnPort = -1;
    static int     loop        = 1;

    static enum LsnStates { LSN_WAIT_REQ,   LSN_SEND_ACK}  lsnState = LSN_WAIT_REQ;
    static enum OpnStates { OPN_WAIT_REQ,   OPN_SEND_REP}  opnState = OPN_WAIT_REQ;
    static enum RxpStates { RXP_SEND_NOTIF, RXP_WAIT_DREQ, RXP_SEND_DATA, RXP_DONE} rxpState = RXP_SEND_NOTIF;
    static enum TxpStates { TXP_WAIT_META,  TXP_RECV_DATA} txpState = TXP_WAIT_META;

    static int opnStartupDelay =  0;
    static int rxpStartupDelay =  0x8000 + 100;
    static int txpStartupDelay =  0;

    const char *myLsnName = concat3(THIS_NAME, "/", "TOE/Listen");
    const char *myOpnName = concat3(THIS_NAME, "/", "TOE/OpnCon");
    const char *myRxpName = concat3(THIS_NAME, "/", "TOE/RxPath");
    const char *myTxpName = concat3(THIS_NAME, "/", "TOE/TxPath");

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static AppLsnReq appLsnPortReq;

    switch (lsnState) {
    case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
        if (!siTRIF_LsnReq.empty()) {
            siTRIF_LsnReq.read(appLsnPortReq);
            printInfo(myLsnName, "Received a listen port request #%d from [TRIF].\n",
                      appLsnPortReq.to_int());
            lsnState = LSN_SEND_ACK;
        }
        break;
    case LSN_SEND_ACK: // SEND ACK BACK TO [TRIF]
        if (!soTRIF_LsnAck.full()) {
            soTRIF_LsnAck.write(true);
            fpgaLsnPort = appLsnPortReq.to_int();
            lsnState = LSN_WAIT_REQ;
        }
        else {
            printWarn(myLsnName, "Cannot send listen reply back to [TRIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (lsnState) {

    //------------------------------------------------------
    //-- FSM #2 - OPEN CONNECTION
    //------------------------------------------------------
    AppOpnReq   leHostSockAddr(byteSwap32(DEFAULT_HOST_IP4_ADDR),
                               byteSwap16(DEFAULT_HOST_LSN_PORT));
    OpenStatus  opnReply(DEFAULT_SESSION_ID, true);
    if (!opnStartupDelay) {
        switch(opnState) {
        case OPN_WAIT_REQ:
            if (!siTRIF_OpnReq.empty()) {
                siTRIF_OpnReq.read(leHostSockAddr);
                printInfo(myOpnName, "Received a request to open the following remote socket address:\n");
                printSockAddr(myOpnName, leHostSockAddr);
                opnState = OPN_SEND_REP;
            }
            break;
        case OPN_SEND_REP:
            if (!soTRIF_OpnRep.full()) {
                soTRIF_OpnRep.write(opnReply);
                opnState = OPN_WAIT_REQ;
            }
            else {
                printWarn(myOpnName, "Cannot send open connection reply back to [TRIF] because stream is full.\n");
            }
            break;
        }  // End-of: switch (opnState) {
    }
    else
        opnStartupDelay--;

    //------------------------------------------------------
    //-- FSM #3 - RX DATA PATH
    //------------------------------------------------------
    static AppRdReq    appRdReq;
    static AppMeta     sessionId;
    static int         byteCnt = 0;
    static int         segCnt  = 0;
    const  int         nrSegToSend = 3;
    static ap_uint<64> data=0;
    ap_uint< 8>        keep;
    ap_uint< 1>        last;
    if (!rxpStartupDelay) {
        switch (rxpState) {
        case RXP_SEND_NOTIF: // SEND A DATA NOTIFICATION TO [TRIF]
            sessionId   = DEFAULT_SESSION_ID;
            tcpSegLen   = DEFAULT_SESSION_LEN;
            hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
            if (!soTRIF_Notif.full()) {
                soTRIF_Notif.write(AppNotif(sessionId,  tcpSegLen,
                                            hostIp4Addr, fpgaLsnPort));
                printInfo(myRxpName, "Sending notification #%d to [TRIF] (sessId=%d, segLen=%d).\n",
                          segCnt, sessionId.to_int(), tcpSegLen.to_int());
                rxpState = RXP_WAIT_DREQ;
            }
            break;
        case RXP_WAIT_DREQ: // WAIT FOR A DATA REQUEST FROM [TRIF]
            if (!siTRIF_DReq.empty()) {
                siTRIF_DReq.read(appRdReq);
                printInfo(myRxpName, "Received a data read request from [TRIF] (sessId=%d, segLen=%d).\n",
                          appRdReq.sessionID.to_int(), appRdReq.length.to_int());
                byteCnt = 0;
                rxpState = RXP_SEND_DATA;
           }
           break;
        case RXP_SEND_DATA: // FORWARD DATA AND METADATA TO [TRIF]
            // Note: We always assume 'tcpSegLen' is multiple of 8B.
            keep = 0xFF;
            last = (byteCnt==tcpSegLen) ? 1 : 0;
            if (byteCnt == 0) {
                if (!soTRIF_SessId.full() && !soTRIF_Data.full()) {
                    soTRIF_SessId.write(sessionId);
                    soTRIF_Data.write(AppData(data, keep, last));
                    if (DEBUG_LEVEL & TRACE_TOE)
                        printAxiWord(myRxpName, AppData(data, keep, last));
                    byteCnt += 8;
                    data += 8;
                }
                else
                    break;
            }
            else if (byteCnt <= (tcpSegLen)) {
                if (!soTRIF_Data.full()) {
                    soTRIF_Data.write(AppData(data, keep, last));
                    if (DEBUG_LEVEL & TRACE_TOE)
                        printAxiWord(myRxpName, AppData(data, keep, last));
                    byteCnt += 8;
                    data += 8;
                }
            }
            else {
                segCnt++;
                if (segCnt == nrSegToSend)
                    rxpState = RXP_DONE;
                else
                    rxpState = RXP_SEND_NOTIF;
            }
            break;
        case RXP_DONE: // END OF THE RX PATH SEQUENCE
            // ALL SEGMENTS HAVE BEEN SENT
            break;
        }  // End-of: switch())
    }
    else
    	rxpStartupDelay--;

    //------------------------------------------------------
    //-- FSM #4 - TX DATA PATH
    //--    (Always drain the data coming from [TRIF])
    //------------------------------------------------------
    if (!txpStartupDelay) {
        switch (txpState) {
        case TXP_WAIT_META:
            if (!siTRIF_SessId.empty() && !siTRIF_Data.empty()) {
                AppData     appData;
                AppMeta     sessId;
                siTRIF_SessId.read(sessId);
                siTRIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myTxpName, "Receiving data for session #%d\n", sessId.to_int());
                    printAxiWord(myTxpName, appData);
                }
                if (!appData.tlast)
                    txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siTRIF_Data.empty()) {
                AppData     appData;
                siTRIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE)
                    printAxiWord(myTxpName, appData);
                if (appData.tlast)
                    txpState = TXP_WAIT_META;
            }
            break;
        }
    }
    else
        txpStartupDelay--;
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
    //-- ROLE / Rx Data Interface
    stream<AppData>     sROLE_To_TRIF_Data     ("sROLE_To_TRIF_Data");
    stream<TcpSessId>   sROLE_To_TRIF_SessId   ("sROLE_To_TRIF_SessId");
    //-- ROLE / This / Tx Data Interface
    stream<AppData>     sTRIF_To_ROLE_Data     ("sTRIF_To_ROLE_Data");
    stream<TcpSessId>   sTRIF_To_ROLE_SessId   ("sTRIF_To_ROLE_SessId");
    //-- TOE  / Rx Data Interfaces
    stream<AppNotif>    sTOE_To_TRIF_Notif     ("sTOE_To_TRIF_Notif");
    stream<AppData>     sTOE_To_TRIF_Data      ("sTOE_To_TRIF_Data");
    stream<AppMeta>     sTOE_To_TRIF_SessId    ("sTOE_to_TRIF_SessId");
    stream<AppRdReq>    sTRIF_To_TOE_DReq      ("sTRIF_To_TOE_DReq");
    //-- TOE  / Listen Interfaces
    stream<AppLsnAck>   sTOE_To_TRIF_LsnAck    ("sTOE_To_TRIF_LsnAck");
    stream<AppLsnReq>   sTRIF_To_TOE_LsnReq    ("sTRIF_To_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<AppWrSts>    sTOE_To_TRIF_DSts      ("sTOE_To_TRIF_DSts");
    stream<AppData>     sTRIF_To_TOE_Data      ("sTRIF_To_TOE_Data");
    stream<AppMeta>     sTRIF_To_TOE_SessId    ("sTRIF_To_TOE_SessId");
    //-- TOE  / Connect Interfaces
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

        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(
            nrErr,
            //-- TOE / Tx Data Interfaces
            sTOE_To_TRIF_Notif,
            sTRIF_To_TOE_DReq,
            sTOE_To_TRIF_Data,
            sTOE_To_TRIF_SessId,
            //-- TOE / Listen Interfaces
            sTRIF_To_TOE_LsnReq,
            sTOE_To_TRIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            sTRIF_To_TOE_Data,
            sTRIF_To_TOE_SessId,
            sTOE_To_TRIF_DSts,
            //-- TOE / Open Interfaces
            sTRIF_To_TOE_OpnReq,
            sTOE_To_TRIF_OpnSts);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_role_if(
            //-- ROLE / Rx & Tx Data Interfaces
            sROLE_To_TRIF_Data,
            sROLE_To_TRIF_SessId,
            sTRIF_To_ROLE_Data,
            sTRIF_To_ROLE_SessId,
            //-- TOE / Rx Data Interfaces
            sTOE_To_TRIF_Notif,
            sTRIF_To_TOE_DReq,
            sTOE_To_TRIF_Data,
            sTOE_To_TRIF_SessId,
            //-- TOE / Listen Interfaces
            sTRIF_To_TOE_LsnReq,
            sTOE_To_TRIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            sTRIF_To_TOE_Data,
            sTRIF_To_TOE_SessId,
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
            sTRIF_To_ROLE_SessId,
            //-- TRIF / Tx Data Interface
            sROLE_To_TRIF_Data,
            sROLE_To_TRIF_SessId);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        simCycCnt++;
        if (gTraceEvent || ((simCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", simCycCnt);
            gTraceEvent = false;
        }

    } while ( (simCycCnt < gMaxSimCycles) or
              gFatalError or
              (nrErr > 10) );

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
