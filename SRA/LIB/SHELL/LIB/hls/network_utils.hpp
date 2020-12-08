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

using namespace hls;


#ifndef _AXIS_CLASS_DEFINED_
#define _AXIS_CLASS_DEFINED_
#include "../../../hls/network.hpp"
//FIXME: merge with definition in AxisRaw.hpp
/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 * It has NO defined byte order. The user can use this for BE and LE (and must ensure the encoding)!.
 */
 template<int D>
   struct Axis {
   protected:
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
   public:
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(~(((ap_uint<D>) single_data) & 0)), tlast(1) {}
     Axis(ap_uint<D> new_data, ap_uint<(D+7/8)> new_keep, ap_uint<1> new_last) : tdata(new_data), tkeep(new_keep), tlast(new_last) {}
     ap_uint<D> getTData() {
    	 return tdata;
     }
     ap_uint<(D+7)/8> getTKeep() {
    	 return tkeep;
     }
     ap_uint<1> getTLast() {
    	 return tlast;
     }
     void setTData(ap_uint<D> new_data) {
    	 tdata = new_data;
     }
     void setTKeep(ap_uint<(D+7)/8> new_keep) {
    	 tkeep = new_keep;
     }
     void setTLast(ap_uint<1> new_last) {
    	 tlast = new_last;
     }
     Axis<64>& operator= (const NetworkWord& nw) {
    	 this->tdata = nw.tdata;
    	 this->tkeep = nw.tkeep;
    	 this->tlast = nw.tlast;
    	 return *this;
     }
     operator NetworkWord() {
    	 return NetworkWord(this->tdata, this->tkeep, this->tlast);
     }
     Axis(NetworkWord nw) : tdata((ap_uint<D>) nw.tdata), tkeep((ap_uint<(D+7/8)>) nw.tkeep), tlast(nw.tlast) {}
   };

typedef Axis<64> 	UdpAppData;

typedef Axis<64>	TcpAppData;

#endif

#define _USE_STRUCT_SOCKET_PAIR_

// import NTS types (if not yet defined)
#include "NTS/nts.hpp"
#include "NTS/nts_types.hpp"
#include "NTS/nts_config.hpp"
#include "NTS/nts_utils.hpp"
#include "NTS/AxisApp.hpp"
#include "NTS/AxisRaw.hpp"


/* ===== NAL specific ====== */

/***********************************************
* Application Metadata
*  Meta-data transfered between TOE and APP.
***********************************************/
typedef TcpSessId   AppMeta;

// --- utility functions -----

void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out);
void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out);

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


