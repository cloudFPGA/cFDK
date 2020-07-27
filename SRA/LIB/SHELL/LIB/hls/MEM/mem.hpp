/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : mem.hpp
 * @brief      : Memory Sub-System (MEM) for the cloudFPGA shell.
 *
 * System:     : cloudFPGA
 * Component   : Shell
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *               DDR4 Memory Sub-System.
 *
 * \ingroup MEM
 * \addtogroup MEM
 * \{
 *******************************************************************************/

#ifndef _MEM_H_
#define _MEM_H_

#include "ap_int.h"

/******************************************************************************
 * GLOBAL DEFINITIONS USED BY MEM
 ******************************************************************************/
#define KU60_MEM_BANK0_SIZE   8*1024*1024  // 8GB = 2^33
#define KU60_MEM_BANK1_SIZE   8*1024*1024  // 8GB = 2^33

/******************************************************************************
 * DDR4 - MEMORY SUB-SYSTEM INTERFACES
 ******************************************************************************
 * Terminology & Conventions (see Xilinx LogiCORE PG022).
 *  [DM]  stands for AXI Data Mover
 *  [DRE] stands for Data Realignment Engine.
 ******************************************************************************/

//=========================================================
//== Data Mover Command Interface (c.f PG022)
//=========================================================
class DmCmd {
  public:
    ap_uint<23>     bbt;    // Bytes To Transfer (1 up to 8,388,607 bytes. 0 is not allowed)
    ap_uint<1>      type;   // Type of AXI4 access (0=FIXED, 1=INCR)
    ap_uint<6>      dsa;    // DRE Stream Alignment
    ap_uint<1>      eof;    // End of Frame
    ap_uint<1>      drr;    // DRE ReAlignment Request
    ap_uint<40>     saddr;  // Start Address
    ap_uint<4>      tag;    // Command Tag
    ap_uint<4>      rsvd;   // Reserved
 
   DmCmd() {}
   DmCmd(ap_uint<40> addr, ap_uint<23> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
};

//=========================================================
//== Data Mover Status Interface (c.f PG022)
//=========================================================
class DmSts {
  public:
    ap_uint<4>      tag;
    ap_uint<1>      interr;
    ap_uint<1>      decerr;
    ap_uint<1>      slverr;
    ap_uint<1>      okay;

    DmSts() {}
};

#endif

/*! \} */
