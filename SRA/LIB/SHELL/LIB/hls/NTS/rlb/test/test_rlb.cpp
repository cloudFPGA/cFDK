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
 * @file       : test_uoe.cpp
 * @brief      : Testbench for the UDP Offload Engine (UOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_RLB
 * \{
 *****************************************************************************/

#include "test_rlb.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGRF   1 << 1
#define TRACE_CGTF   1 << 2
#define TRACE_DUMTF  1 << 3
#define TRACE_ALL    0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
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


/*******************************************************************************
 * @brief Main function.
 *******************************************************************************/
int main(int argc, char *argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;        // Tb error counter.

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    StsBit                  sRLB_MMIO_Ready;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<StsBool>         ssTOE_RLB_Data    ("ssTOE_RLB_Data");
    stream<StsBool>         ssUOE_RLB_Data    ("ssUOE_RLB_Data");


    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_rlb' STARTS HERE                                       ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printf("\n");

    for (int i=0; i<gMaxSimCycles; i++) {
        switch(i) {
        case 1:
            ssTOE_RLB_Data.write(false);
            ssUOE_RLB_Data.write(false);
            break;
        case 10:
            if (sRLB_MMIO_Ready != false) {
                 printError(THIS_NAME, "The ready signal is expected inactive (false) but was found to be active (true).\n");
                 nrErr++;
            }
            break;
        case 25:
            ssTOE_RLB_Data.write(true);
            break;
        case 50:
            ssUOE_RLB_Data.write(true);
            break;
        case 75:
             if (sRLB_MMIO_Ready != true) {
                  printError(THIS_NAME, "The ready signal is expected active (true) but was found to be inactive (false).\n");
                  nrErr++;
             }
             break;
        default:
            break;
        }
        //-- RUN one cycle of the DUT
        rlb(
            //-- MMIO Interface
            &sRLB_MMIO_Ready,
            //-- UOE / Data Stream Interface
            ssUOE_RLB_Data,
            //-- TOE / Data Stream Interface
            ssTOE_RLB_Data
        );
        stepSim();
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_rlb' ENDS HERE                                         ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

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

    return 0;
}

/*! \} */
