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

/*******************************************************************************
 * @file       : test_icmp.cpp
 * @brief      : Testbench for the Internet Control Message Protocol (ICMP) server.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_ICMP
 * \addtogroup NTS_ICMP_TEST
 * \{
 *****************************************************************************/

#ifndef _TEST_ICMP_H_
#define _TEST_ICMP_H_

#include "../src/icmp.hpp"
#include "../../../NTS/nts_types.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../../NTS/SimEthFrame.hpp"
#include "../../../NTS/SimIcmpPacket.hpp"
#include "../../../NTS/SimIp4Packet.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'TB_STARTUP_DELAY' is used to delay the start of the [TB] functions.
//    'TB_GRACE_TIME'    adds some cycles to drain the DUT at the end before.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES   25000
#define TB_STARTUP_DELAY        0
#define TB_GRACE_TIME        5000

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent;
bool            gFatalError;
unsigned int    gSimCycCnt;
unsigned int    gMaxSimCycles;

#endif

/*! \} */
