/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 * @file       : test_toecam.cpp
 * @brief      : Testbench for the Content-Addressable Memory (CAM).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOECAM
 * \addtogroup NTS_TOECAM_TEST
 * \{
 *****************************************************************************/

#include "test_toecam.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TOE     1 <<  1
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

/*****************************************************************************
 * @brief Emulate the behavior of the TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr            A ref to the error counter of main.
 * //-- TOE / Lookup Request Interfaces
 * @param[out] soCAM_SssLkpReq  Session lookup request to CAM,
 * @param[in]  siCAM_SssLkpRep  Session lookup reply from CAM.
 * //-- TOE / Update Request Interfaces
 * @param[out] soCAM_SssUpdReq  Session update request to CAM.
 * @param[in]  siCAM_SssUpdRep  Session update reply from CAM.
 *
 ******************************************************************************/
void pTOE(
        int                                 &nrErr,
        //-- Session Lookup & Update Interfaces
        stream<CamSessionLookupRequest>     &soCAM_SssLkpReq,
        stream<CamSessionLookupReply>       &siCAM_SssLkpRep,
        stream<CamSessionUpdateRequest>     &soCAM_SssUpdReq,
        stream<CamSessionUpdateReply>       &siCAM_SssUpdRep)
{
    const char *myName = concat3(THIS_NAME, "/", "TOE");

    static enum slcStates { LOOKUP_REQ, LOOKUP_REP,
                            INSERT_REQ, INSERT_REP, \
                            DELETE_REQ, DELETE_REP, \
                            TB_ERROR,   TB_DONE } slcState = LOOKUP_REQ;

    //------------------------------------------------------
    //-- EMULATE THE SESSION LOOKUP CONTROLLER
    //------------------------------------------------------
    static int rdCnt;

    switch (slcState) {
    case LOOKUP_REQ: // SEND A LOOKUP REQUEST TO [CAM]
        for (int i=0; i<CAM_SIZE; i++) {
            if (!soCAM_SssLkpReq.full()) {
                // Build a new request
                FourTuple key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
                              DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
                LkpSrcBit src = FROM_RXe;
                CamSessionLookupRequest lkpRequest(key, src);
                // Send the new request
                soCAM_SssLkpReq.write(lkpRequest);
                printInfo(myName, "Sending LOOKUP request[%d] to [CAM].\n", i);
            }
            else {
                printWarn(myName, "Cannot send LOOKUP request to [CAM] because stream is full.\n");
                nrErr++;
                slcState = TB_ERROR;
            }
        }
        // Goto next step
        slcState = LOOKUP_REP;
        rdCnt = 0;
        break;
    case LOOKUP_REP: // WAIT FOR LOOKUP REPLY FROM [CAM]
        while (rdCnt < CAM_SIZE) {
            if (!siCAM_SssLkpRep.empty()) {
                CamSessionLookupReply lkpReply;
                siCAM_SssLkpRep.read(lkpReply);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myName, "Received a lookup reply from [CAM]. \n");
                    printInfo(myName, "Src=%d, SessId=%d, Hit=%d\n", lkpReply.source.to_int(),
                               lkpReply.sessionID.to_int(), lkpReply.hit);
                }
                rdCnt++;
            }
            else
                return;
        }
        // Goto next step
        slcState = INSERT_REQ;
        rdCnt = 0;
        break;
    case INSERT_REQ: // SEND AN INSERT REQUEST TO [CAM]
        for (int i=0; i<CAM_SIZE; i++) {
            if (!soCAM_SssUpdReq.full()) {
                // Build a new request
                FourTuple key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
                              DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
                LkpSrcBit src = FROM_RXe;
                RtlSessId value = DEFAULT_SESSION_ID+i;
                CamSessionUpdateRequest updRequest(key, value, INSERT, src);
                // Send the new request
                soCAM_SssUpdReq.write(updRequest);
                printInfo(myName, "Sending UPDATE request[%d] to [CAM].\n", i);
            }
            else {
                printWarn(myName, "Cannot send INSERT request to [CAM] because stream is full.\n");
                nrErr++;
                slcState = TB_ERROR;
            }
        }
        // Goto next step
        slcState = INSERT_REP;
        rdCnt = 0;
        break;
    case INSERT_REP: // WAIT FOR INSERT REPLY FROM [CAM]
        while (rdCnt<CAM_SIZE) {
            if (!siCAM_SssUpdRep.empty()) {
                CamSessionUpdateReply updReply;
                siCAM_SssUpdRep.read(updReply);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myName, "Received an insert reply from [CAM]. \n");
                    printInfo(myName, "Src=%d, Op=%d, SessId=%d.\n",
                              updReply.source.to_int(), updReply.op,
                              updReply.sessionID.to_int());
                }
                if (updReply.sessionID != DEFAULT_SESSION_ID+rdCnt) {
                    printError(myName, "Got a wrong session ID (%d) as reply from [CAM].\n",
                               updReply.source.to_int());
                    nrErr++;
                    slcState = TB_ERROR;
                }
                rdCnt++;
            }
            else
                return;
        }
        // Goto next step
        slcState = DELETE_REQ;
        rdCnt = 0;
        break;
    case DELETE_REQ: // SEND A DELETE REQUEST TO [CAM]
        for (int i=0; i<CAM_SIZE; i++) {
            if (!soCAM_SssUpdReq.full()) {
                // Build a new request
                FourTuple key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
                              DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
                LkpSrcBit src = FROM_RXe;
                RtlSessId value = DEFAULT_SESSION_ID+i;
                CamSessionUpdateRequest updRequest(key, value, DELETE, src);
                // Send the new request
                soCAM_SssUpdReq.write(updRequest);
                printInfo(myName, "Sending DELETE request[%d] to [CAM].\n", i);
            }
            else {
                printWarn(myName, "Cannot send DELETE request to [CAM] because stream is full.\n");
                nrErr++;
                slcState = TB_ERROR;
            }
        }
        // Goto next step
        slcState = DELETE_REP;
        rdCnt = 0;
        break;
    case DELETE_REP: // WAIT FOR DELETE REPLY FROM [CAM]
        while (rdCnt<CAM_SIZE) {
            if (!siCAM_SssUpdRep.empty()) {
                CamSessionUpdateReply updReply;
                siCAM_SssUpdRep.read(updReply);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myName, "Received a delete reply from [CAM]. \n");
                    printInfo(myName, "Src=%d, Op=%d, SessId=%d.\n",
                              updReply.source.to_int(), updReply.op,
                              updReply.sessionID.to_int());
                }
                if (updReply.sessionID != DEFAULT_SESSION_ID+rdCnt) {
                    printError(myName, "Got a wrong session ID (%d) as reply from [CAM].\n",
                               updReply.source.to_int());
                    nrErr++;
                    slcState = TB_ERROR;
                }
                rdCnt++;
            }
            else
                return;
        }
        // Goto next step
        slcState = TB_DONE;
        break;
    case TB_ERROR:
        slcState = TB_ERROR;
        break;
    case TB_DONE:
        slcState = TB_DONE;
        break;
    }  // End-of: switch (lsnState) {

}


/*******************************************************************************
 * @brief Main function.
 *
 *******************************************************************************/
int main(int argc, char* argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gTraceEvent   = false;
    gFatalError   = false;
    gSimCycCnt    = 0;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- MMIO Interfaces
    ap_uint<1>                       sMMIO_CamReady("sMMIO_CamReady");
    //-- CAM / This / Session Lookup & Update Interfaces
    stream<CamSessionLookupRequest>  ssTOE_CAM_SssLkpReq("ssTOE_CAM_SssLkpReq");
    stream<CamSessionLookupReply>    ssCAM_TOE_SssLkpRep("ssCAM_TOE_SssLkpRep");
    stream<CamSessionUpdateRequest>  ssTOE_CAM_SssUpdReq("ssTOE_CAM_SssUpdReq");
    stream<CamSessionUpdateReply>    ssCAM_TOE_SssUpdRep("ssCAM_TOE_SssUpdRep");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int     nrErr = 0;  // Total number of testbench errors
    int     tbRun = 0;  // Total duration of the test (in clock cycles)

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_toecam' STARTS HERE                                    ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    tbRun = (nrErr == 0) ? (TB_MAX_SIM_CYCLES + TB_GRACE_TIME) : 0;
    while (tbRun) {
        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        if (sMMIO_CamReady == 1) {
            pTOE(
                nrErr,
                //-- TOE / Lookup Request Interfaces
                ssTOE_CAM_SssLkpReq,
                ssCAM_TOE_SssLkpRep,
                //-- TOE / Update Request Interfaces
                ssTOE_CAM_SssUpdReq,
                ssCAM_TOE_SssUpdRep
            );
        }
        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        toecam(
            //-- MMIO Interfaces
            &sMMIO_CamReady,
            //-- Session Lookup & Update Interfaces
            ssTOE_CAM_SssLkpReq,
            ssCAM_TOE_SssLkpRep,
            ssTOE_CAM_SssUpdReq,
            ssCAM_TOE_SssUpdRep
        );

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        stepSim();
        tbRun--;
    } // End of: while()

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_toecam' ENDS HERE                                      ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

    if (nrErr) {
         printError(THIS_NAME, "###########################################################\n");
         printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###########################################################\n");
         printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}

/*! \} */
