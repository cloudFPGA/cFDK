//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Aug 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains memory types and functions that are shared accross HLS cores. 
//  *
//  *

#ifndef _CF_MEMORY_UTILS_
#define _CF_MEMORY_UTILS_

#include <stdio.h>
#include <stdint.h>

#include "ap_int.h"

using namespace hls;


/***********************************************
 * Data Mover Command Interface (c.f PG022)
 ***********************************************/
class DmCmd
{
  public:
    ap_uint<23>     bbt;    // Bytes To Transfer
    ap_uint<1>      type;   // Type of AXI4 access (0=FIXED, 1=INCR)
    ap_uint<6>      dsa;    // DRE Stream Alignment
    ap_uint<1>      eof;    // End of Frame
    ap_uint<1>      drr;    // DRE ReAlignment Request
    ap_uint<40>     saddr;  // Start Address
    ap_uint<4>      tag;    // Command Tag
    ap_uint<4>      rsvd;   // Reserved
    DmCmd() {}
    DmCmd(ap_uint<40> addr, ap_uint<16> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
};

struct mmCmd
{
    ap_uint<23> bbt;
    ap_uint<1>  type;
    ap_uint<6>  dsa;
    ap_uint<1>  eof;
    ap_uint<1>  drr;
    ap_uint<40> saddr;
    ap_uint<4>  tag;
    ap_uint<4>  rsvd;
    mmCmd() {}
    mmCmd(ap_uint<40> addr, ap_uint<16> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}

};

/***********************************************
 * Data Mover Status Interface (c.f PG022)
 ***********************************************/
class DmSts
{
  public:
    ap_uint<4>      tag;
    ap_uint<1>      interr;
    ap_uint<1>      decerr;
    ap_uint<1>      slverr;
    ap_uint<1>      okay;
    DmSts() {}
};

struct mmStatus
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
};

//TODO is this required??
struct mm_ibtt_status
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
    ap_uint<22> brc_vd;
    ap_uint<1>  eop;
};


#endif
