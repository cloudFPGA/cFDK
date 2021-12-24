/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * @file     : test_ack_delay.hpp
 * @brief    : Testbench for or ACK delayer (AKd) function of TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack, TCP Offload Engine (TOE)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE_TEST
 * \{
 *****************************************************************************/

#ifndef _TEST_ACK_DELAY_H_
#define _TEST_ACK_DELAY_H_

#include "../src/ack_delay.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_STARTUP_DELAY    25
#define TB_GRACE_TIME     2500  // Adds some cycles to drain the DUT before exiting
#define TB_STARTUP_TIME     25

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gSimCycCnt    = 0;
unsigned int    gMaxSimCycles = TB_STARTUP_DELAY + TB_GRACE_TIME;

#endif

/*! \} */
