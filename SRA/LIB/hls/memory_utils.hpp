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

