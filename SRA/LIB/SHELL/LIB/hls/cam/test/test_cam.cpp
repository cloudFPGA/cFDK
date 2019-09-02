/*****************************************************************************
 * @file       : test_cam.cpp
 * @brief      : Testbench for the Content-Addressable Memory (CAM).
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

#include "../src/cam.hpp"
#include "../../toe/test/test_toe_utils.hpp"

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

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent     = false;
bool            gFatalError     = false;
unsigned int    gSimCycCnt      = 0;
unsigned int    gMaxSimCycles   = 200;


//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC813  // TOE's local IP Address  = 10.12.200.19
#define DEFAULT_FPGA_TCP_PORT   0x2263      // TOE listens on port     = 8803 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_TCP_PORT   0xa263      // TB listens on port      = 41,571 (dynamic ports must be 32768..65535)

#define DEFAULT_SESSION_ID      42          // This TB open only one session.

#define CAM_SIZE                 2          // Number of CAM entries.


/*****************************************************************************
 * @brief Emulate the behavior of the TCP Offload Engine (TOE).
 *
 * @param[in]  nrErr,           A ref to the error counter of main.
 * //-- TOE / Lookup Request Interfaces
 * @param[out] soCAM_SssLkpReq, Session lookup request to CAM,
 * @param[in]  siCAM_SssLkpRep, Session lookup reply from CAM.
 * //-- TOE / Update Request Interfaces
 * @param[out] soCAM_SssUpdReq, Session update request to CAM.
 * @param[in]  siCAM_SssUpdRep, Session update reply from CAM.
 *
 ******************************************************************************/
void pTOE(
        int                                 &nrErr,
        //-- Session Lookup & Update Interfaces
        stream<rtlSessionLookupRequest>     &soCAM_SssLkpReq,
        stream<rtlSessionLookupReply>       &siCAM_SssLkpRep,
        stream<rtlSessionUpdateRequest>     &soCAM_SssUpdReq,
        stream<rtlSessionUpdateReply>       &siCAM_SssUpdRep)
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
				fourTupleInternal key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
									  DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
				lookupSource      src = FROM_RXe;
				rtlSessionLookupRequest lkpRequest(key, src);
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
				rtlSessionLookupReply lkpReply;
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
				fourTupleInternal key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
									  DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
				lookupSource      src = FROM_RXe;
				RtlSessId       value = DEFAULT_SESSION_ID+i;
				rtlSessionUpdateRequest updRequest(key, value, INSERT, src);
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
				rtlSessionUpdateReply updReply;
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
				fourTupleInternal key(DEFAULT_FPGA_IP4_ADDR,   DEFAULT_HOST_IP4_ADDR, \
									  DEFAULT_FPGA_TCP_PORT+i, DEFAULT_HOST_TCP_PORT+i);
				lookupSource      src = FROM_RXe;
				RtlSessId       value = DEFAULT_SESSION_ID+i;
				rtlSessionUpdateRequest updRequest(key, value, DELETE, src);
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
				rtlSessionUpdateReply updReply;
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

    //-- MMIO Interfaces
    ap_uint<1>                       wMMIO_CamReady("wMMIO_CamReady");

    //-- CAM / This / Session Lookup & Update Interfaces
    stream<rtlSessionLookupRequest>  sTOE_TO_CAM_SssLkpReq("sTOE_TO_CAM_SssLkpReq");
    stream<rtlSessionLookupReply>    sCAM_TO_TOE_SssLkpRep("sCAM_TO_TOE_SssLkpRep");
    stream<rtlSessionUpdateRequest>  sTOE_TO_CAM_SssUpdReq("sTOE_TO_CAM_SssUpdReq");
    stream<rtlSessionUpdateReply>    sCAM_TO_TOE_SssUpdRep("sCAM_TO_TOE_SssUpdRep");

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
        if (wMMIO_CamReady == 1) {
            pTOE(
                nrErr,
                //-- TOE / Lookup Request Interfaces
                sTOE_TO_CAM_SssLkpReq,
                sCAM_TO_TOE_SssLkpRep,
                //-- TOE / Update Reque/st Interfaces
                sTOE_TO_CAM_SssUpdReq,
                sCAM_TO_TOE_SssUpdRep
            );
        }
        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        cam(
            //-- MMIO Interfaces
            &wMMIO_CamReady,
            //-- Session Lookup & Update Interfaces
            sTOE_TO_CAM_SssLkpReq,
            sCAM_TO_TOE_SssLkpRep,
            sTOE_TO_CAM_SssUpdReq,
            sCAM_TO_TOE_SssUpdRep
        );

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        simCycCnt++;
        if (gTraceEvent || ((simCycCnt % 1000) == 0)) {
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
