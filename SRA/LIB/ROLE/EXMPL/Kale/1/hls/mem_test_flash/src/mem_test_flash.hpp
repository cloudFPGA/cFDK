// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Memory test in HSL using AXI DataMover 
// *
// * Created : Dec. 2018
// * Authors : Burkhard Ringlein (NGL@zurich.ibm.com)
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2017.4 (64-bit)
// * Depends : None
// *
// *
// *****************************************************************************

#ifndef _MEM_TEST_H_
#define _MEM_TEST_H_


#include <stdint.h>
#include <stdio.h>
#include <hls_stream.h>
#include "ap_int.h"


using namespace std;
using namespace hls;

#define FSM_IDLE 0
#define FSM_WR_PAT_CMD 1
#define FSM_WR_PAT_DATA 2
#define FSM_WR_PAT_STS 3
#define FSM_RD_PAT_CMD 4
#define FSM_RD_PAT_DATA 5
#define FSM_RD_PAT_STS 6
#define FSM_WR_ANTI_CMD 7
#define FSM_WR_ANTI_DATA 8
#define FSM_WR_ANTI_STS 9
#define FSM_RD_ANTI_CMD 10
#define FSM_RD_ANTI_DATA 11
#define FSM_RD_ANTI_STS 12
//#define FsmState uint8_t


#define PHASE_IDLE 0
#define PHASE_RAMP_WRITE 1
#define PHASE_RAMP_READ 2
#define PHASE_STRESS 3


#define MEM_START_ADDR 0x000000000 // Start address of user space in DDR4

//#ifdef DEBUG
//#define MEM_END_ADDR 0x2000 //DEBUG
//#else

//8GB 
//#define MEM_END_ADDR   0x1FFFFF000 // End address of user space in DDR4
//4GB 
//#define MEM_END_ADDR   0xFFFFF000 // End address of user space in DDR4

//#endif
#include "dynamic.hpp"

#define CHECK_CHUNK_SIZE 0x1000 //4 KiB
#define BYTE_PER_MEM_WORD 64
#define TRANSFERS_PER_CHUNK (CHECK_CHUNK_SIZE/BYTE_PER_MEM_WORD) //64


//#define CYCLES_UNTIL_TIMEOUT 0xF4240
#define CYCLES_UNTIL_TIMEOUT 0x1000

/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
template<int D>
struct Axis {
  ap_uint<D>       tdata;
  ap_uint<(D+7)/8> tkeep;
  ap_uint<1>       tlast;
  Axis() {}
  Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
};


// AXI DataMover - Format of the command word (c.f PG022)
struct DmCmd
{
  ap_uint<23>   btt;
  ap_uint<1>    type;
  ap_uint<6>    dsa;
  ap_uint<1>    eof;
  ap_uint<1>    drr;
  ap_uint<40>   saddr;
  ap_uint<4>    tag;
  ap_uint<4>    rsvd;
  DmCmd() {}
  DmCmd(ap_uint<40> addr, ap_uint<16> len) :
    btt(len), type(1), dsa(0), eof(1), drr(0), saddr(addr), tag(0x7), rsvd(0) {}
};


// AXI DataMover - Format of the status word (c.f PG022)
struct DmSts
{
  ap_uint<4>    tag;
  ap_uint<1>    interr;
  ap_uint<1>    decerr;
  ap_uint<1>    slverr;
  ap_uint<1>    okay;
  DmSts() {}
};


void mem_test_flash_main(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    // ----- MMIO ------
    ap_uint<2> DIAG_CTRL_IN,
    ap_uint<2> *DIAG_STAT_OUT,
    // ---- add. Debug output ----
    ap_uint<16> *debug_out,

    //------------------------------------------------------
    //-- SHELL / Role / Mem / Mp0 Interface
    //------------------------------------------------------
    //---- Read Path (MM2S) ------------
    stream<DmCmd>     &soMemRdCmdP0,
    stream<DmSts>     &siMemRdStsP0,
    stream<Axis<512> >    &siMemReadP0,
    //---- Write Path (S2MM) -----------
    stream<DmCmd>     &soMemWrCmdP0,
    stream<DmSts>     &siMemWrStsP0,
    stream<Axis<512> >    &soMemWriteP0  //,

//------------------------------------------------------
//-- SHELL / Role / Mem / Mp1 Interface
//------------------------------------------------------
//---- Read Path (MM2S) ------------
//stream<DmCmd>     &soMemRdCmdP1,
//stream<DmSts>     &siMemRdStsP1,
//stream<Axis<512> >    &siMemReadP1,
//---- Write Path (S2MM) -----------
//stream<DmCmd>     &soMemWrCmdP1,
//stream<DmSts>     &siMemWrStsP1,
// stream<Axis<512> >   &soMemWriteP1
);

#endif
