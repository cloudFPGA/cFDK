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
 * @file       : test_toecam.cpp
 * @brief      : Testbench for the Content-Addressable Memory (CAM) of TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOECAM
 * \{
 *******************************************************************************/

#ifndef _TEST_TOECAM_H_
#define _TEST_TOECAM_H_

#include <hls_stream.h>
#include <map>
#include <stdio.h>
#include <string>

#include "../src/toecam.hpp"
#include "../../../NTS/nts_types.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"
#include "../../../NTS/SimArpPacket.hpp"
#include "../../../NTS/SimEthFrame.hpp"
#include "../../../NTS/SimIp4Packet.hpp"

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'TB_STARTUP_DELAY' is used to delay the start of the [TB] functions.
//    'TB_GRACE_TIME'    adds some cycles to drain the DUT at the end before.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES     250
#define TB_STARTUP_DELAY        0
#define TB_GRACE_TIME         500

#define CAM_LOOKUP_LATENCY  2
#define CAM_UPDATE_LATENCY 10

const Ip4Addr RESERVED_SENDER_PROTOCOL_ADDRESS = 0xCAFEFADE; // Do not use in DAT files

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent;
bool            gFatalError;
unsigned int    gSimCycCnt;
unsigned int    gMaxSimCycles;

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

#endif

/*! \} */
