/************************************************
 * Copyright (c) 2016-2019, IBM Research.
 * Copyright (c) 2015, Xilinx, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************/

//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared accross HLS cores. 
//  *
//  *
//  *----------------------------------------------------------------------------
//  *
//  * @details    : Data structures, types and prototypes definitions for the
//  *                   cFDK Role.
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

//exclude file nts.hpp
//#define _NTS_H_
#include "NTS/nts.hpp"

#include "NTS/nts_types.hpp"
#include "NTS/nts_config.hpp"
#include "NTS/nts_utils.hpp"
#include "NTS/AxisApp.hpp"
#include "NTS/AxisRaw.hpp"

using namespace hls;

/* ===== NAL specific ====== */

/***********************************************
* Application Metadata
*  Meta-data transfered between TOE and APP.
***********************************************/
typedef TcpSessId   AppMeta;

// --- utility functions -----

//TODO
//void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out);
//void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out);

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


