/*******************************************************************************
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
 *******************************************************************************/

//  *
//  *     Description:
//  *        This file contains network types and functions that are shared
//  *        across HLS cores.
//  *
//  *
//  *----------------------------------------------------------------------------
//  *
//  * @details    : Data structures, types and prototypes definitions
//  *               that are shared across HLS cores in the Shell.
//  *
//  * @terminology:
//  *      In telecommunications, a protocol data unit (PDU) is a single unit of
//  *       information transmitted among peer entities of a computer network.
//  *      A PDU is therefore composed of a protocol specific control information
//  *       (e.g, a header) and a user data section.
//  *  This source code uses the following terminology:
//  *   - a SEGMENT  (or TCP Packet) refers to the TCP protocol data unit.
//  *   - a DATAGRAM )or UDP Packet) refers to the UDP protocol data unit.
//  *   - a PACKET   (or IP  Packet) refers to the IP protocol data unit.
//  *   - a FRAME    (or MAC Frame)  refers to the Ethernet data link layer.
//  *

#ifndef _CF_NETWORK_UTILS_
#define _CF_NETWORK_UTILS_


#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <vector>

#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

//user network interface
#include "../../../hls/network.hpp"

using namespace hls;


// import NTS types (if not yet defined)
#include "NTS/nts.hpp"
#include "NTS/nts_types.hpp"
#include "NTS/nts_config.hpp"
#include "NTS/nts_utils.hpp"
#include "NTS/AxisRaw.hpp"


/* ===== NAL specific ====== */

/***********************************************
 * Application Metadata
 *  Meta-data transfered between TOE and APP.
 ***********************************************/
typedef TcpSessId   AppMeta;

// --- utility functions -----

//void convertAxisToNtsWidth(stream<AxisRaw<8> > &small, AxisRa &out);

ap_uint<32> bigEndianToInteger(ap_uint<8> *buffer, int lsb);
void integerToBigEndian(ap_uint<32> n, ap_uint<8> *bytes);

/* MOVED to nts_utils.hpp
   ap_uint<16> swapWord   (ap_uint<16> inpWord);       // [FIXME - To be replaced w/ byteSwap16]
   ap_uint<16> byteSwap16 (ap_uint<16> inputVector);
   ap_uint<32> swapDWord  (ap_uint<32> inpDWord);      // [FIXME - To be replaced w/ byteSwap32]
   ap_uint<32> byteSwap32 (ap_uint<32> inputVector);
   */

ap_uint<8>  lenToKeep  (ap_uint<4> noValidBytes);
//ap_uint<8>  returnKeep (ap_uint<4> length);
ap_uint<4>  keepToLen  (ap_uint<8> keepValue);
//ap_uint<4>  keepMapping(ap_uint<8> keepValue);

#endif


