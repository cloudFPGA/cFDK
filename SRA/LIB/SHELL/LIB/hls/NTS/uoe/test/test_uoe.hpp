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
 * @file       : test_uoe.hpp
 * @brief      : Testbench for the UDP Offload Engine (UOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_UOE
 * \{
 *****************************************************************************/

#ifndef _TEST_UOE_H_
#define _TEST_UOE_H_

#include <set>

#include "../src/uoe.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../../NTS/SimUdpDatagram.hpp"
#include "../../../NTS/SimIp4Packet.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'TB_TARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES   250000
#define TB_STARTUP_DELAY         0
#define TB_GRACE_TIME         1000 // Adds some cycles to drain the DUT before exiting

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gSimCycCnt    = 0;
unsigned int    gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

//---------------------------------------------------------
//-- TESTBENCH MODES OF OPERATION
//---------------------------------------------------------
enum TestMode { RX_MODE='0',   TX_DGRM_MODE='1', TX_STRM_MODE='2',
                OPEN_MODE='3', BIDIR_MODE='4',   ECHO_MODE='5',     };

#endif

/*! \} */
