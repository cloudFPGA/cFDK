/*******************************************************************************
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
 *******************************************************************************/

//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared between SHELL and ROLE
//  *

#ifndef _CF_NETWORK_USER_UTILS_
#define _CF_NETWORK_USER_UTILS_


#include <stdint.h>
#include <stdio.h>

#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>


using namespace hls;

/********************************************
 * Generic Network Streaming Interfaces.
 ********************************************/

#define NETWORK_WORD_BYTE_WIDTH 8
#define NETWORK_WORD_BIT_WIDTH 64

struct NetworkWord {
  ap_uint<64>    tdata;
  ap_uint<8>     tkeep;
  ap_uint<1>     tlast;
  NetworkWord()      {}
  NetworkWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
    tdata(tdata), tkeep(tkeep), tlast(tlast) {}
  NetworkWord(ap_uint<64> single_data) : tdata(single_data), tkeep(0xFFF), tlast(1) {}
  ap_uint<64> getTData() {
    return tdata;
  }
  ap_uint<8> getTKeep() {
    return tkeep;
  }
  ap_uint<1> getTLast() {
    return tlast;
  }
  void setTData(ap_uint<64> new_data) {
    tdata = new_data;
  }
  void setTKeep(ap_uint<8> new_keep) {
    tkeep = new_keep;
  }
  void setTLast(ap_uint<1> new_last) {
    tlast = new_last;
  }
};


typedef NetworkWord UdpWord;
typedef NetworkWord TcpWord;


typedef ap_uint<16>     NrcPort; // UDP/TCP Port Number
typedef ap_uint<8>      NodeId;  // Cluster Node Id

//#define MAX_CF_NODE_ID (128-1)
#define MAX_CF_NODE_ID (64-1)

#define NAL_THIS_FPGA_PSEUDO_NID (MAX_CF_NODE_ID + 1)

#define NAL_RX_MIN_PORT 2718
#define NAL_RX_MAX_PORT 2749

typedef ap_uint<16>    NetworkDataLength;

struct NetworkMeta {
  NodeId  dst_rank;
  NrcPort dst_port;
  NodeId  src_rank;
  NrcPort src_port;
  NetworkDataLength len;

  NetworkMeta() {}
  //"alphabetical order"
  NetworkMeta(NodeId d_id, NrcPort d_port, NodeId s_id, NrcPort s_port, NetworkDataLength length) :
    dst_rank(d_id), dst_port(d_port), src_rank(s_id), src_port(s_port), len(length) {}
};

//split between NetworkMeta and NetworkMetaStream to not make the Shell Role interface depend of "DATA_PACK"
struct NetworkMetaStream {
  NetworkMeta tdata;
  ap_uint<8> tkeep;
  ap_uint<1> tlast;
  NetworkMetaStream() {}
  NetworkMetaStream(NetworkMeta single_data) : tdata(single_data), tkeep(0xFFF), tlast(1) {}
};


#endif


