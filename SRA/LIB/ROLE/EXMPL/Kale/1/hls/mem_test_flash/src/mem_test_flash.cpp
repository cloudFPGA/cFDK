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

#include "mem_test_flash.hpp"

using namespace std;
using namespace hls;

ap_uint<8> fsmState = FSM_IDLE;
bool runContiniously = false;
ap_uint<1> wasError = 0;
ap_uint<33> lastCheckedAddress = 0;
ap_uint<33> currentPatternAdderss = 0;
ap_uint<64> currentMemPattern = 0;
ap_uint<32> patternWriteNum = 0;
ap_uint<16> debugVec = 0;
ap_uint<8> testPhase = PHASE_IDLE;
ap_uint<32> timeoutCnt = 0;

ap_uint<8> STS_to_Vector(DmSts sts)
{
  ap_uint<8> ret = 0;
  ret |= sts.tag;
  ret |= ((ap_uint<8>) sts.interr) << 4;
  ret |= ((ap_uint<8>) sts.decerr) << 5;
  ret |= ((ap_uint<8>) sts.slverr) << 6;
  ret |= ((ap_uint<8>) sts.okay)   << 7;
  return ret;
}


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
    stream<DmCmd>       &soMemRdCmdP0,
    stream<DmSts>       &siMemRdStsP0,
    stream<Axis<512 > > &siMemReadP0,
    //---- Write Path (S2MM) -----------
    stream<DmCmd>       &soMemWrCmdP0,
    stream<DmSts>       &siMemWrStsP0,
    stream<Axis<512> >  &soMemWriteP0

    ) 
{

#pragma HLS INTERFACE ap_vld register port=sys_reset name=piSysReset
#pragma HLS INTERFACE ap_vld register port=DIAG_CTRL_IN name=piMMIO_diag_ctrl
#pragma HLS INTERFACE ap_ovld register port=DIAG_STAT_OUT name=poMMIO_diag_stat
#pragma HLS INTERFACE ap_ovld register port=debug_out name=poDebug

  // Bundling: SHELL / Role / Mem / Mp0 / Read Interface
#pragma HLS INTERFACE axis register both port=soMemRdCmdP0
#pragma HLS INTERFACE axis register both port=siMemRdStsP0
#pragma HLS INTERFACE axis register both port=siMemReadP0

#pragma HLS DATA_PACK variable=soMemRdCmdP0 instance=soMemRdCmdP0
#pragma HLS DATA_PACK variable=siMemRdStsP0 instance=siMemRdStsP0

  // Bundling: SHELL / Role / Mem / Mp0 / Write Interface
#pragma HLS INTERFACE axis register both port=soMemWrCmdP0
#pragma HLS INTERFACE axis register both port=siMemWrStsP0
#pragma HLS INTERFACE axis register both port=soMemWriteP0

#pragma HLS DATA_PACK variable=soMemWrCmdP0 instance=soMemWrCmdP0
#pragma HLS DATA_PACK variable=siMemWrStsP0 instance=siMemWrStsP0



  Axis<512>     memP0;
  DmSts         memRdStsP0;
  DmSts         memWrStsP0;

  //initalize 
  memP0.tdata = 0;
  memP0.tlast = 0;
  memP0.tkeep = 0;


  if(sys_reset == 1)
  {
    fsmState = FSM_IDLE;
    runContiniously = false;
    wasError = 0;
    //lastCheckedAddress = 0;
    lastCheckedAddress = MEM_END_ADDR + 1; //to stop the test
    currentPatternAdderss = 0;
    currentMemPattern = 0;
    patternWriteNum = 0;
    debugVec = 0;
    testPhase = PHASE_IDLE;
    timeoutCnt = 0;
    return;
  }


  switch(fsmState) {

    case FSM_IDLE:
      switch(DIAG_CTRL_IN) {
        case 0x3: //reserved --> idle
        case 0x0: //stay IDLE, stop test
          *DIAG_STAT_OUT = (0 << 1) | wasError;
          runContiniously = false;
          lastCheckedAddress = MEM_START_ADDR;
          break; 
        case 0x2: 
          runContiniously = true;
          //NO break
        case 0x1: //Run once
          if (lastCheckedAddress == MEM_START_ADDR)
          { // start new test
            wasError = 0;
            lastCheckedAddress = MEM_START_ADDR;
            fsmState = FSM_WR_PAT_CMD;
            *DIAG_STAT_OUT = 0b10;
            debugVec = 0;
            testPhase = PHASE_RAMP_WRITE;
            currentMemPattern = 0;
          } else if(lastCheckedAddress >= MEM_END_ADDR)
          {//checked space completely once 

            if(testPhase == PHASE_RAMP_WRITE)
            {
              testPhase = PHASE_RAMP_READ;
              lastCheckedAddress = MEM_START_ADDR;
              fsmState = FSM_RD_PAT_CMD;
              currentMemPattern = 0;
            } else if(testPhase == PHASE_RAMP_READ)
            {
              testPhase = PHASE_STRESS;
              lastCheckedAddress = MEM_START_ADDR;
              fsmState = FSM_WR_PAT_CMD;
              currentMemPattern = 0;
            } else if (runContiniously)
            {
              fsmState = FSM_WR_PAT_CMD;
              lastCheckedAddress = MEM_START_ADDR;
              currentMemPattern = 0;
              testPhase = PHASE_RAMP_WRITE;
              *DIAG_STAT_OUT = (1 << 1) | wasError;
            } else { //stay here
              fsmState = FSM_IDLE;
              *DIAG_STAT_OUT = (0 << 1) | wasError;
            }
          } else { //continue current run

            if(testPhase == PHASE_RAMP_READ)
            {
              fsmState = FSM_RD_PAT_CMD;
            } else {
              fsmState = FSM_WR_PAT_CMD;
            }

            *DIAG_STAT_OUT = (1 << 1) | wasError;
            if(testPhase == PHASE_STRESS)
            {
              currentMemPattern = 0;
            }
          }
          break;
      }

      //stop on error
      if(wasError == 1)
      {
        fsmState = FSM_IDLE;
        *DIAG_STAT_OUT = (0 << 1) | wasError;
      }

      //set current address
      if(lastCheckedAddress == MEM_START_ADDR)
      {
        currentPatternAdderss = MEM_START_ADDR;
      } else {
        currentPatternAdderss = lastCheckedAddress+1;
      }
      break;

    case FSM_WR_PAT_CMD:
      if (!soMemWrCmdP0.full()) {
        //-- Post a memory write command to SHELL/Mem/Mp0
        soMemWrCmdP0.write(DmCmd(currentPatternAdderss, CHECK_CHUNK_SIZE));
        patternWriteNum = 0;
        fsmState = FSM_WR_PAT_DATA;
      }
      break;

    case FSM_WR_PAT_DATA:
      if (!soMemWriteP0.full()) {
        //-- Assemble a memory word and write it to DRAM
        currentMemPattern++;
        memP0.tdata = (ap_uint<512>) (currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern);
        ap_uint<8> keepVal = 0xFF;
        memP0.tkeep = (ap_uint<64>) (keepVal, keepVal, keepVal, keepVal, keepVal, keepVal, keepVal, keepVal);
        //memP0.tkeep = (ap_uint<64>) (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF);

        if(patternWriteNum == TRANSFERS_PER_CHUNK -1) 
        {
          memP0.tlast = 1;
          fsmState = FSM_WR_PAT_STS;
          timeoutCnt = 0;
        } else {
          memP0.tlast = 0;
        }
        soMemWriteP0.write(memP0);
        patternWriteNum++;
      }
      break;

    case FSM_WR_PAT_STS:
      if (!siMemWrStsP0.empty()) {
        //-- Get the memory write status for Mem/Mp0
        siMemWrStsP0.read(memWrStsP0);
        //latch errors
        debugVec |= (ap_uint<16>) STS_to_Vector(memWrStsP0);

        if(testPhase == PHASE_RAMP_WRITE)
        {
          fsmState = FSM_IDLE;
          debugVec |= ((ap_uint<16>) STS_to_Vector(memRdStsP0) )<< 8;
          lastCheckedAddress = currentPatternAdderss+CHECK_CHUNK_SIZE -1;
        } else {
          fsmState = FSM_RD_PAT_CMD;
          currentMemPattern = 0;
        }
      } else { 
        timeoutCnt++;
        if (timeoutCnt >= CYCLES_UNTIL_TIMEOUT)
        {
          wasError = 1;
          fsmState = FSM_IDLE;
        }
      }

      break;

    case FSM_RD_PAT_CMD:
      if (!soMemRdCmdP0.full()) {
        //-- Post a memory read command to SHELL/Mem/Mp0
        soMemRdCmdP0.write(DmCmd(currentPatternAdderss, CHECK_CHUNK_SIZE));
        fsmState = FSM_RD_PAT_DATA;
      }
      break;

    case FSM_RD_PAT_DATA:
      if (!siMemReadP0.empty()) {
        //-- Read a memory word from DRAM
        siMemReadP0.read(memP0);
        currentMemPattern++;
        if (memP0.tdata != ((ap_uint<512>) (currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern)) )
        {
          printf("error in pattern reading!\n");
          wasError = 1;
        }
        /*if (memP0.tkeep != (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF))
          {
          printf("error in tkeep\n");
          }*/
        //I trust that there will be a tlast (so no counting)
        if (memP0.tlast == 1)
        {
          fsmState = FSM_RD_PAT_STS;
          timeoutCnt = 0;
        }
      }
      break;

    case FSM_RD_PAT_STS:
      if (!siMemRdStsP0.empty()) {
        //-- Get the memory read status for Mem/Mp0
        siMemRdStsP0.read(memRdStsP0);
        //latch errors
        debugVec |= ((ap_uint<16>) STS_to_Vector(memRdStsP0) )<< 8;

        if(testPhase == PHASE_RAMP_READ)
        {
          fsmState = FSM_IDLE;
          debugVec |= ((ap_uint<16>) STS_to_Vector(memRdStsP0) )<< 8;
          lastCheckedAddress = currentPatternAdderss+CHECK_CHUNK_SIZE -1;
        } else {
          fsmState = FSM_WR_ANTI_CMD;
          currentMemPattern = 0;
        }
      } else { 
        timeoutCnt++;
        if (timeoutCnt >= CYCLES_UNTIL_TIMEOUT)
        {
          wasError = 1;
          fsmState = FSM_IDLE;
        }
      }
      break;

    case FSM_WR_ANTI_CMD:
      if (!soMemWrCmdP0.full()) {
        //-- Post a memory write command to SHELL/Mem/Mp0
        soMemWrCmdP0.write(DmCmd(currentPatternAdderss, CHECK_CHUNK_SIZE));
        currentMemPattern = 0;
        patternWriteNum = 0;
        fsmState = FSM_WR_ANTI_DATA;
      }
      break;

    case FSM_WR_ANTI_DATA:
      if (!soMemWriteP0.full()) {
        //-- Assemble a memory word and write it to DRAM
        currentMemPattern++;
        ap_uint<64> currentAntiPattern = ~currentMemPattern;
        //debug 
        //printf("AntiPattern: 0x%llX\n", (uint64_t) currentAntiPattern);

        memP0.tdata = (currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern);
        ap_uint<8> keepVal = 0xFF;
        memP0.tkeep = (ap_uint<64>) (keepVal, keepVal, keepVal, keepVal, keepVal, keepVal, keepVal, keepVal);
        //memP0.tkeep = (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF);

        if(patternWriteNum == TRANSFERS_PER_CHUNK -1) 
        {
          memP0.tlast = 1;
          fsmState = FSM_WR_ANTI_STS;
          timeoutCnt = 0;
        } else {
          memP0.tlast = 0;
        }
        soMemWriteP0.write(memP0);
        patternWriteNum++;
      }
      break;

    case FSM_WR_ANTI_STS:
      if (!siMemWrStsP0.empty()) {
        //-- Get the memory write status for Mem/Mp0
        siMemWrStsP0.read(memWrStsP0);
        //latch errors
        debugVec |= (ap_uint<16>) STS_to_Vector(memWrStsP0);
        fsmState = FSM_RD_ANTI_CMD;
      } else { 
        timeoutCnt++;
        if (timeoutCnt >= CYCLES_UNTIL_TIMEOUT)
        {
          wasError = 1;
          fsmState = FSM_IDLE;
        }
      }
      break;

    case FSM_RD_ANTI_CMD:
      if (!soMemRdCmdP0.full()) {
        //-- Post a memory read command to SHELL/Mem/Mp0
        soMemRdCmdP0.write(DmCmd(currentPatternAdderss, CHECK_CHUNK_SIZE));
        currentMemPattern = 0;
        fsmState = FSM_RD_ANTI_DATA;
      }
      break;

    case FSM_RD_ANTI_DATA:
      if (!siMemReadP0.empty()) {
        //-- Read a memory word from DRAM
        siMemReadP0.read(memP0);
        currentMemPattern++;
        ap_uint<64> currentAntiPattern = ~currentMemPattern;

        if (memP0.tdata != ((ap_uint<512>) (currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern,currentAntiPattern)) )
        {
          printf("error in antipattern reading!\n");
          wasError = 1;
        }
        /*if (memP0.tkeep != (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF))
          {
          printf("error in tkeep\n");
          }*/
        //I trust that there will be a tlast (so no counting)
        if (memP0.tlast == 1)
        {
          fsmState = FSM_RD_ANTI_STS;
          timeoutCnt = 0;
        }
      }
      break;

    case FSM_RD_ANTI_STS:
      if (!siMemRdStsP0.empty()) {
        //-- Get the memory read status for Mem/Mp0
        siMemRdStsP0.read(memRdStsP0);
        //latch errors
        debugVec |= ((ap_uint<16>) STS_to_Vector(memRdStsP0) )<< 8;
        lastCheckedAddress = currentPatternAdderss+CHECK_CHUNK_SIZE -1;
        fsmState = FSM_IDLE;
      } else { 
        timeoutCnt++;
        if (timeoutCnt >= CYCLES_UNTIL_TIMEOUT)
        {
          wasError = 1;
          fsmState = FSM_IDLE;
        }
      }
      break;

  }  // End: switch

  *debug_out = debugVec;

  return;
}
