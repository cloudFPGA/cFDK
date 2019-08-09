//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
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

struct NetworkWord { 
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    NetworkWord()      {}
    NetworkWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};


typedef NetworkWord UdpWord;
typedef NetworkWord TcpWord;


typedef ap_uint<16>     NrcPort; // UDP/TCP Port Number
typedef ap_uint<8>      NodeId;  // Cluster Node Id

#define MAX_CF_NODE_ID 128

typedef ap_uint<32>    NetworkDataLength;

struct NetworkMeta {
  NodeId  dst_rank; //ATTENTION: don't use 'id' in a struct...will be ignored by axis directive and lead to segfaults...
  NrcPort dst_port;
  NodeId  src_rank;
  NrcPort src_port;
  NetworkDataLength len;

  //ap_uint<16> padding;
  NetworkMeta() {}
  //"alphabetical order"
  NetworkMeta(NodeId d_id, NrcPort d_port, NodeId s_id, NrcPort s_port, NetworkDataLength length) :
    dst_rank(d_id), dst_port(d_port), src_rank(s_id), src_port(s_port), len(length) {}
 };

//ATTENTION: split between NetworkMeta and NetworkMetaStream is necessary, due to flaws in Vivados hls::stream library
struct NetworkMetaStream {
  NetworkMeta tdata; 
  //ap_uint<(sizeof(NetworkMeta)+7)/8> tkeep; TODO: sizeof seems not to work with ap_ctrl_none!
  ap_uint<10> tkeep; //TODO: set value in constructor correct based on the length
  ap_uint<1> tlast;
  NetworkMetaStream() {}
  NetworkMetaStream(NetworkMeta single_data) : tdata(single_data), tkeep(0xFFF), tlast(1) {}
};


#endif


