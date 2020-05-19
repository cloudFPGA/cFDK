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
 * @file       : test_tcp_shell_if.cpp
 * @brief      : Testbench for the TCP Shell Interface.
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpShellInterface (TSIF)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include <hls_stream.h>
#include <iostream>

#include "../src/tcp_shell_if.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TOE    1 <<  1
#define TRACE_TAF    1 <<  2
#define TRACE_MMIO   1 <<  3
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_TOE | TRACE_TAF)

//------------------------------------------------------
//-- TESTBENCH DEFINITIONS
//------------------------------------------------------
#define MAX_SIM_CYCLES   500
#define LSN_ACK            1

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = 0x8000 + 200;

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
 * @brief Emulate the behavior of the ROLE/TcpAppFlash (TAF).
 *
 * @param[in]  siTSIF_Data,   Data to TcpShellInterface (TSIF).
 * @param[in]  siTSIF_SessId, Session ID to [TSIF].
 * @param[out] soTSIF_Data,   Data stream to [TSIF].
 * @param[out] soTSIF_SessId  SessionID to [TSIF].

 * @details
 *
 ******************************************************************************/
void pTAF(
        //-- TSIF / Rx Data Interface
        stream<AppData>     &siTSIF_Data,
        stream<AppMetaAxis> &siTSIF_SessId,
        //-- TSIF / Tx Data Interface
        stream<AppData>     &soTSIF_Data,
        stream<AppMetaAxis> &soTSIF_SessId)
{
    AppData         currWord;
    AppMetaAxis     tcpSessId;

    const char *myRxName  = concat3(THIS_NAME, "/", "TAF-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "TAF-Tx");

    //------------------------------------------------------
    //-- ALWAYS READ INCOMING DATA STREAM AND ECHO IT  BACK
    //------------------------------------------------------
    static enum RxFsmStates { RX_WAIT_META=0, RX_STREAM } rxFsmState = RX_WAIT_META;
    #pragma HLS reset variable=rdpFsmState

    switch (rxFsmState ) {
    case RX_WAIT_META:
        if (!siTSIF_SessId.empty() and !soTSIF_SessId.full()) {
            siTSIF_SessId.read(tcpSessId);
            soTSIF_SessId.write(tcpSessId);
            rxFsmState  = RX_STREAM;
        }
        break;
    case RX_STREAM:
        if (!siTSIF_Data.empty() && !soTSIF_Data.full()) {
            siTSIF_Data.read(currWord);
            if (DEBUG_LEVEL & TRACE_TAF) {
                 printAxiWord(myRxName, currWord);
            }
            soTSIF_Data.write(currWord);
            if (currWord.tlast)
                rxFsmState  = RX_WAIT_META;
        }
        break;
    }

}

/*****************************************************************************
 * @brief Emulate the behavior of the SHELL/MMIO.
 *
 * @param[in]  pSHL_Ready     Ready Signal from [SHELL].
 * @param[out] poTSIF_Enable  Enable signal to [TSIF].
 *
 ******************************************************************************/
void pMMIO(
        //-- SHELL / Ready Signal
        StsBit      *piSHL_Ready,
        //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
        CmdBit      *poTSIF_Enable)
{
    const char *myName  = concat3(THIS_NAME, "/", "MMIO");

    static bool printOnce = true;

    if (*piSHL_Ready) {
        *poTSIF_Enable = 1;
        if (printOnce) {
            printInfo(myName, "[SHELL/NTS/TOE] is ready -> Enabling operation of the TCP Shell Interface [TSIF].\n");
            printOnce = false;
        }
    }
    else {
        *poTSIF_Enable = 0;
    }
}

/*****************************************************************************
 * @brief Emulate behavior of the SHELL/NTS/TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr,         A ref to the error counter of main.
 * @param[out] poMMIO_Ready,  Ready signal to [MMIO].
 * @param[out] soTSIF_Notif,  Notification to TcpShellInterface (TSIF).
 * @param[in]  siTSIF_DReq,   Data read request from [TSIF].
 * @param[out] soTSIF_Data,   Data to [TSIF].
 * @param[out] soTSIF_SessId, Session Id to [TSIF].
 * @param[in]  siTSIF_LsnReq, Listen port request from [TSIF].
 * @param[out] soTSIF_LsnAck, Listen port acknowledge to [TSIF].
 *
 ******************************************************************************/
void pTOE(
        int                   &nrErr,
        //-- MMIO / Ready Signal
        StsBit                *poMMIO_Ready,
        //-- TSIF / Tx Data Interfaces
        stream<AppNotif>      &soTSIF_Notif,
        stream<AppRdReq>      &siTSIF_DReq,
        stream<AppData>       &soTSIF_Data,
        stream<AppMetaAxis>   &soTSIF_SessId,
        //-- TSIF / Listen Interfaces
        stream<AppLsnReqAxis> &siTSIF_LsnReq,
        stream<AppLsnAckAxis> &soTSIF_LsnAck,
        //-- TSIF / Rx Data Interfaces
        stream<AppData>       &siTSIF_Data,
        stream<AppMetaAxis>   &siTSIF_SessId,
        stream<AppWrSts>      &soTSIF_DSts,
        //-- TSIF / Open Interfaces
        stream<AppOpnReq>     &siTSIF_OpnReq,
        stream<AppOpnRep>     &soTSIF_OpnRep)
{
    TcpSegLen  tcpSegLen  = 32;

    static Ip4Addr hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
    static TcpPort fpgaLsnPort = -1;
    static TcpPort hostTcpPort = DEFAULT_HOST_LSN_PORT;
    static int     loop        = 1;

    static enum LsnStates { LSN_WAIT_REQ,   LSN_SEND_ACK}  lsnState = LSN_WAIT_REQ;
    static enum OpnStates { OPN_WAIT_REQ,   OPN_SEND_REP}  opnState = OPN_WAIT_REQ;
    static enum RxpStates { RXP_SEND_NOTIF, RXP_WAIT_DREQ, RXP_SEND_DATA, RXP_DONE} rxpState = RXP_SEND_NOTIF;
    static enum TxpStates { TXP_WAIT_META,  TXP_RECV_DATA} txpState = TXP_WAIT_META;

    static int  toeStartupDelay = 0x8000;
    static int  rxpStartupDelay = 100;
    static int  txpStartupDelay = 100;
    static bool toeIsReady = false;
    static bool rxpIsReady = false;
    static bool txpIsReady = false;

    const char  *myLsnName = concat3(THIS_NAME, "/", "TOE/Listen");
    const char  *myOpnName = concat3(THIS_NAME, "/", "TOE/OpnCon");
    const char  *myRxpName = concat3(THIS_NAME, "/", "TOE/RxPath");
    const char  *myTxpName = concat3(THIS_NAME, "/", "TOE/TxPath");

    //------------------------------------------------------
    //-- FSM #0 - STARTUP DELAYS
    //------------------------------------------------------
    if (toeStartupDelay) {
        *poMMIO_Ready = 0;
        toeStartupDelay--;
    }
    else {
        *poMMIO_Ready = 1;
        if (rxpStartupDelay)
            rxpStartupDelay--;
        else
            rxpIsReady = true;
        if (txpStartupDelay)
            txpStartupDelay--;
        else
            txpIsReady = true;
    }

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static AppLsnReqAxis appLsnPortReq;

    switch (lsnState) {
    case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
        if (!siTSIF_LsnReq.empty()) {
            siTSIF_LsnReq.read(appLsnPortReq);
            printInfo(myLsnName, "Received a listen port request #%d from [TSIF].\n",
                      appLsnPortReq.tdata.to_int());
            lsnState = LSN_SEND_ACK;
        }
        break;
    case LSN_SEND_ACK: // SEND ACK BACK TO [TSIF]
        if (!soTSIF_LsnAck.full()) {
            soTSIF_LsnAck.write(AppLsnAckAxis(LSN_ACK));
            fpgaLsnPort = appLsnPortReq.tdata.to_int();
            lsnState = LSN_WAIT_REQ;
        }
        else {
            printWarn(myLsnName, "Cannot send listen reply back to [TSIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (lsnState) {

    //------------------------------------------------------
    //-- FSM #2 - OPEN CONNECTION
    //------------------------------------------------------
    AppOpnReq  opnReq;

    OpenStatus  opnReply(DEFAULT_SESSION_ID, SESS_IS_OPENED);
    switch(opnState) {
    case OPN_WAIT_REQ:
        if (!siTSIF_OpnReq.empty()) {
            siTSIF_OpnReq.read(opnReq);
            printInfo(myOpnName, "Received a request to open the following remote socket address:\n");
            printSockAddr(myOpnName, LE_SockAddr(opnReq.addr, opnReq.port));
            opnState = OPN_SEND_REP;
        }
        break;
    case OPN_SEND_REP:
        if (!soTSIF_OpnRep.full()) {
            soTSIF_OpnRep.write(opnReply);
            opnState = OPN_WAIT_REQ;
        }
        else {
            printWarn(myOpnName, "Cannot send open connection reply back to [TSIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (opnState) {

    //------------------------------------------------------
    //-- FSM #3 - RX DATA PATH
    //------------------------------------------------------
    static AppRdReq    appRdReq;
    static SessionId   sessionId;
    static int         byteCnt = 0;
    static int         segCnt  = 0;
    const  int         nrSegToSend = 3;
    static ap_uint<64> data=0;
    ap_uint< 8>        keep;
    ap_uint< 1>        last;

    if (rxpIsReady) {

        switch (rxpState) {
        case RXP_SEND_NOTIF: // SEND A DATA NOTIFICATION TO [TSIF]
            sessionId   = DEFAULT_SESSION_ID;
            tcpSegLen   = DEFAULT_SESSION_LEN;
            hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
            hostTcpPort = DEFAULT_HOST_LSN_PORT;
            if (!soTSIF_Notif.full()) {
                soTSIF_Notif.write(AppNotif(sessionId,  tcpSegLen, hostIp4Addr,
                                            hostTcpPort, fpgaLsnPort));
                printInfo(myRxpName, "Sending notification #%d to [TSIF] (sessId=%d, segLen=%d).\n",
                          segCnt, sessionId.to_int(), tcpSegLen.to_int());
                rxpState = RXP_WAIT_DREQ;
            }
            break;
        case RXP_WAIT_DREQ: // WAIT FOR A DATA REQUEST FROM [TSIF]
            if (!siTSIF_DReq.empty()) {
                siTSIF_DReq.read(appRdReq);
                printInfo(myRxpName, "Received a data read request from [TSIF] (sessId=%d, segLen=%d).\n",
                          appRdReq.sessionID.to_int(), appRdReq.length.to_int());
                byteCnt = 0;
                rxpState = RXP_SEND_DATA;
           }
           break;
        case RXP_SEND_DATA: // FORWARD DATA AND METADATA TO [TSIF]
            // Note: We always assume 'tcpSegLen' is multiple of 8B.
            keep = 0xFF;
            last = (byteCnt==tcpSegLen) ? 1 : 0;
            if (byteCnt == 0) {
                if (!soTSIF_SessId.full() && !soTSIF_Data.full()) {
                    soTSIF_SessId.write(sessionId);
                    soTSIF_Data.write(AppData(data, keep, last));
                    if (DEBUG_LEVEL & TRACE_TOE)
                        printAxiWord(myRxpName, AppData(data, keep, last));
                    byteCnt += 8;
                    data += 8;
                }
                else
                    break;
            }
            else if (byteCnt <= (tcpSegLen)) {
                if (!soTSIF_Data.full()) {
                    soTSIF_Data.write(AppData(data, keep, last));
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

    //------------------------------------------------------
    //-- FSM #4 - TX DATA PATH
    //--    (Always drain the data coming from [TSIF])
    //------------------------------------------------------
    if (txpIsReady) {
        switch (txpState) {
        case TXP_WAIT_META:
            if (!siTSIF_SessId.empty() && !siTSIF_Data.empty()) {
                AppData     appData;
                AppMetaAxis sessId;
                siTSIF_SessId.read(sessId);
                siTSIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myTxpName, "Receiving data for session #%d\n", sessId.tdata.to_uint());
                    printAxiWord(myTxpName, appData);
                }
                if (!appData.tlast)
                    txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siTSIF_Data.empty()) {
                AppData     appData;
                siTSIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE)
                    printAxiWord(myTxpName, appData);
                if (appData.tlast)
                    txpState = TXP_WAIT_META;
            }
            break;
        }
    }
}

/*****************************************************************************
 * @brief Main function.
 *
 ******************************************************************************/
int main()
{
    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- SHL / Mmio Interface
    CmdBit              sMMIO_TSIF_Enable;
    //-- TOE / Ready Signal
    StsBit              sTOE_MMIO_Ready;
    //-- TSIF / Session Connect Id Interface
    SessionId           sTSIF_TAF_SConId;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TAF / Rx Data Interface
    stream<AppData>       ssTAF_TSIF_Data     ("ssTAF_TSIF_Data");
    stream<AppMetaAxis>   ssTAF_TSIF_SessId   ("ssTAF_TSIF_SessId");
    //-- TSIF / Tx Data Interface
    stream<AppData>       ssTSIF_TAF_Data     ("ssTSIF_TAF_Data");
    stream<AppMetaAxis>   ssTSIF_TAF_SessId   ("ssTSIF_TAF_SessId");
    //-- TOE  / Rx Data Interfaces
    stream<AppNotif>      ssTOE_TSIF_Notif    ("ssTOE_TSIF_Notif");
    stream<AppData>       ssTOE_TSIF_Data     ("ssTOE_TSIF_Data");
    stream<AppMetaAxis>   ssTOE_TSIF_SessId   ("ssTOE_TSIF_SessId");
    //-- TSIF / Rx Data Interface
    stream<AppRdReq>      ssTSIF_TOE_DReq     ("ssTSIF_TOE_DReq");
    //-- TOE  / Listen Interface
    stream<AppLsnAckAxis> ssTOE_TSIF_LsnAck   ("ssTOE_TSIF_LsnAck");
    //-- TSIF / Listen Interface
    stream<AppLsnReqAxis> ssTSIF_TOE_LsnReq   ("ssTSIF_TOE_LsnReq");
    //-- TOE  / Tx Data Interfaces
    stream<AppWrSts>      ssTOE_TSIF_DSts     ("ssTOE_TSIF_DSts");
    //-- TSIF  / Tx Data Interfaces
    stream<AppData>       ssTSIF_TOE_Data     ("ssTSIF_TOE_Data");
    stream<AppMetaAxis>   ssTSIF_TOE_SessId   ("ssTSIF_TOE_SessId");
    //-- TOE  / Connect Interfaces
    stream<AppOpnRep>     ssTOE_TSIF_OpnRep   ("ssTOE_TSIF_OpnRep");
    //-- TSIF / Connect Interfaces
    stream<AppOpnReq>     ssTSIF_TOE_OpnReq   ("ssTSIF_TOE_OpnReq");
    stream<AppClsReqAxis> ssTSIF_TOE_ClsReq   ("ssTSIF_TOE_ClsReq");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int             nrErr;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt = 0; // Simulation cycle counter as a global variable
    nrErr      = 0; // Total number of testbench errors

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {

        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(
            nrErr,
            //-- TOE / Ready Signal
            &sTOE_MMIO_Ready,
            //-- TOE / Tx Data Interfaces
            ssTOE_TSIF_Notif,
            ssTSIF_TOE_DReq,
            ssTOE_TSIF_Data,
            ssTOE_TSIF_SessId,
            //-- TOE / Listen Interfaces
            ssTSIF_TOE_LsnReq,
            ssTOE_TSIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            ssTSIF_TOE_Data,
            ssTSIF_TOE_SessId,
            ssTOE_TSIF_DSts,
            //-- TOE / Open Interfaces
            ssTSIF_TOE_OpnReq,
            ssTOE_TSIF_OpnRep);

        //-------------------------------------------------
        //-- EMULATE SHELL/MMIO
        //-------------------------------------------------
        pMMIO(
            //-- TOE / Ready Signal
            &sTOE_MMIO_Ready,
            //-- MMIO / Enable Layer-7 (.i.e APP alias ROLE)
            &sMMIO_TSIF_Enable);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_shell_if(
            //-- SHELL / Mmio Interface
            &sMMIO_TSIF_Enable,
            //-- TAF / Rx & Tx Data Interfaces
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            //-- TOE / Rx Data Interfaces
            ssTOE_TSIF_Notif,
            ssTSIF_TOE_DReq,
            ssTOE_TSIF_Data,
            ssTOE_TSIF_SessId,
            //-- TOE / Listen Interfaces
            ssTSIF_TOE_LsnReq,
            ssTOE_TSIF_LsnAck,
            //-- TOE / Tx Data Interfaces
            ssTSIF_TOE_Data,
            ssTSIF_TOE_SessId,
            ssTOE_TSIF_DSts,
            //-- TOE / Tx Open Interfaces
            ssTSIF_TOE_OpnReq,
            ssTOE_TSIF_OpnRep,
            //-- TOE / Close Interfaces
            ssTSIF_TOE_ClsReq,
            //-- ROLE / Session Connect Id Interface
            &sTSIF_TAF_SConId);

        //-------------------------------------------------
        //-- EMULATE ROLE/TcpApplicationFlash
        //-------------------------------------------------
        pTAF(
            //-- TSIF / Data Interface
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            //-- TAF / Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

    } while ( (gSimCycCnt < gMaxSimCycles) or
              gFatalError or
              (nrErr > 10) );

    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
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
