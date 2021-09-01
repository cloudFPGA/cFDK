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

/*******************************************************************************
 * @file       : test_ack_delay.cpp
 * @brief      : Testbench for ACK delayer (AKd) function of TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack, TCP Offload Engine (TOE)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE_TEST
 * \{
 *******************************************************************************/

#include "test_ack_delay.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (SND_TRACE | RCV_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_RCV    1 << 1
#define TRACE_SND    1 << 2
#define TRACE_ALL    0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 100) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

/*****************************************************************************
 * @brief Main (does use any param).
 ******************************************************************************/
int main(int argc, char* argv[])
{

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gTraceEvent   = false;
    gFatalError   = false;
    gSimCycCnt    = 0;
    // gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;     // Tb error counter.

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- Incoming streams
    stream<ExtendedEvent> ssEVeToAKd_Event;
    //-- Outgoing streams
    stream<ExtendedEvent> ssAKdToTXe_Event;
    stream<SigBit>        ssAKdToEVe_RxEventSig;
    stream<SigBit>        ssAKdToEVe_TxEventSig;

    SessionId      sessId = TOE_MAX_SESSIONS-1;
    ExtendedEvent  outEvent;

    unsigned int   rxEventSig=0;
    unsigned int   txEventSig=0;
    int            nrInpAck = 0;
    int            nrOutAck = 0;



    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_iprx' STARTS HERE                                      ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    int tbRun = 1000;
    int loop  = 0;

    while (tbRun) {

        //------------------------------------------------------
        //-- CREATE DUT INPUT TRAFFIC AS STREAMS
        //------------------------------------------------------
		if (loop == 5) {
			// Create a SYN event
			ssEVeToAKd_Event.write(Event(SYN_EVENT, sessId));
		}
		else if (loop >= 10 and loop <  25) {
			// Create a burst of 10 ACKs every second clock cycle
			if (loop % 2 == 0) {
				ssEVeToAKd_Event.write(Event(ACK_EVENT, sessId));
				nrInpAck++;
			}
		}
		else if (loop > 100 and loop < 150) {
			// Create a burst of 10 ACKs every fourth loop
			if (loop % 4 == 0) {
				ssEVeToAKd_Event.write(Event(ACK_EVENT, sessId));
				nrInpAck++;
			}
		}
		else if (loop > 200 and loop < 300) {
			// Create a burst of ACKs every 10th loop
			if (loop % 10 == 0) {
				ssEVeToAKd_Event.write(Event(ACK_EVENT, sessId));
				nrInpAck++;
			}
		}
		loop++;

        //------------------------------------------------------
        //-- RUN DUT
        //------------------------------------------------------
        ack_delay(
            //-- Event Engine Interfaces
            ssEVeToAKd_Event,
            ssAKdToEVe_RxEventSig,
			ssAKdToEVe_TxEventSig,
            //-- Tx Engine Interface
			ssAKdToTXe_Event);
        tbRun--;
        stepSim();

        //---------------------------------------------------------------
        //-- DRAIN DUT OUTPUT STREAMS
        //---------------------------------------------------------------
        if (!ssAKdToTXe_Event.empty()) {
            ssAKdToTXe_Event.read(outEvent);
            if (outEvent.type == ACK_EVENT) {
        	    nrOutAck++;
            }
	    }
        if (!ssAKdToEVe_RxEventSig.empty()) {
        	ssAKdToEVe_RxEventSig.read();
        	rxEventSig++;
        }
        if (!ssAKdToEVe_TxEventSig.empty()) {
        	ssAKdToEVe_TxEventSig.read();
        	txEventSig++;
        }
    }

    //---------------------------------------------------------------
    //-- PRINT OVERALL TESTBENCH STATUS
    //---------------------------------------------------------------
    printInfo(THIS_NAME, "Number of received  ACKs   : %5d \n", nrInpAck);
    printInfo(THIS_NAME, "Number of forwarded ACKs   : %5d \n", nrOutAck);
    printInfo(THIS_NAME, "Number of Rx event signals : %5d \n", rxEventSig);
    printInfo(THIS_NAME, "Number of Tx event signals : %5d \n", txEventSig);

    //---------------------------------------------------------------
    //-- ASSESS TESTBENCH RESULTS
    //---------------------------------------------------------------
    nrErr = 0;

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
    else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return nrErr;
}
