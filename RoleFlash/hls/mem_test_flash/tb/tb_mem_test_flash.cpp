// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Test bench for te memory test in HSL using AXI DataMover 
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
#include "../src/mem_test_flash.hpp"

//#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace hls;
//using namespace boost::multiprecision;

#define TEST_MEM_SIZE 512

int main() {


  //-- SHELL / Role / Mem / Mp0 Interface
  //---- Read Path (MM2S) ------------
  stream<DmCmd>     sROL_Shl_Mem_RdCmdP0("sROL_Shl_Mem_RdCmdP0");
  stream<DmSts>     sSHL_Rol_Mem_RdStsP0("sSHL_Rol_Mem_RdStsP0");
  stream<Axis<512> >    sSHL_Rol_Mem_ReadP0("sSHL_Rol_Mem_ReadP0");
  //---- Write Path (S2MM) -----------
  stream<DmCmd>     sROL_Shl_Mem_WrCmdP0("sROL_Shl_Mem_WrCmdP0");
  stream<DmSts>     sSHL_Rol_Mem_WrStsP0("sSHL_Rol_Mem_WrStsP0");
  stream<Axis<512> >    sROL_Shl_Mem_WriteP0("sROL_Shl_Mem_WriteP0");
  // ----- system reset ---
  ap_uint<1> sys_reset;
  // ----- MMIO ------
  ap_uint<2> DIAG_CTRL_IN;
  ap_uint<2> DIAG_STAT_OUT;
  ap_uint<16> debug_out;


  DmCmd         dmCmd_MemCmdP0;
  DmSts         dmSts_MemWrStsP0;
  DmSts         dmSts_MemRdStsP0;
  Axis<512>     memP0;
  ap_uint<64>   currentMemPattern = 0;

  sys_reset = 1;

#define DUT mem_test_flash_main(sys_reset, DIAG_CTRL_IN, &DIAG_STAT_OUT, &debug_out,sROL_Shl_Mem_RdCmdP0, sSHL_Rol_Mem_RdStsP0, sSHL_Rol_Mem_ReadP0,sROL_Shl_Mem_WrCmdP0, sSHL_Rol_Mem_WrStsP0, sROL_Shl_Mem_WriteP0);
  DUT
    sys_reset = 0;
  DUT

  DIAG_CTRL_IN = 0b01;
  DUT
    assert(DIAG_STAT_OUT = 0b10);

#define TEST_ITERATIONS (MEM_END_ADDR/CHECK_CHUNK_SIZE + 1)

  //phase ramp write

  currentMemPattern = 0;
  for(int j = 0; j< TEST_ITERATIONS; j++)
  {
    DUT
      sROL_Shl_Mem_WrCmdP0.read(dmCmd_MemCmdP0);
    assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
    assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE*j); 
    assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);
    //Pattern
    for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
    {
      DUT 
        sROL_Shl_Mem_WriteP0.read(memP0);
      //printf("tdata: 0x64%llX)\n", (uint512_t) ((ap_uint<512>) memP0.tdata));
      //printf("tkeep: 0x%llX\n", (uint64_t) memP0.tkeep);
      if(i < 63)
      {
        assert(memP0.tlast == 0);
      } else {
        assert(memP0.tlast == 1);
      }
      //assert(memP0.tkeep == (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF) );
      assert(memP0.tkeep == 0xffffffffffffffff);
      currentMemPattern++;
      assert(memP0.tdata == (ap_uint<512>) (currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern));

      sSHL_Rol_Mem_ReadP0.write(memP0);

    }
    dmSts_MemWrStsP0.tag = 7;
    dmSts_MemWrStsP0.okay = 1;
    dmSts_MemWrStsP0.interr = 0;
    dmSts_MemWrStsP0.slverr = 0;
    dmSts_MemWrStsP0.decerr = 0;
    sSHL_Rol_Mem_WrStsP0.write(dmSts_MemWrStsP0);
    DUT
      printf("debug_out: 0x%x\n", (uint16_t) debug_out);
    assert((debug_out & 0xFF) == 0x0087);

    //idle State
    DUT

  }

  printf("write done.\n");

  //phase ramp read

  for(int j = 0; j<TEST_ITERATIONS; j++)
  {

    DUT
      sROL_Shl_Mem_RdCmdP0.read(dmCmd_MemCmdP0);
    assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
    assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE * j); 
    assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);

    for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
    {
      DUT
    }

    dmSts_MemRdStsP0.tag = 7;
    dmSts_MemRdStsP0.okay = 1;
    dmSts_MemRdStsP0.interr = 0;
    dmSts_MemRdStsP0.slverr = 0;
    dmSts_MemRdStsP0.decerr = 0;
    sSHL_Rol_Mem_RdStsP0.write(dmSts_MemWrStsP0);
    DUT
      printf("debug_out: 0x%x\n", (uint16_t) debug_out);
    assert(debug_out == 0x8787);

    //idle State
    DUT

  }

  printf("RAMD completed\n");
  //idle State
  DUT

    //phase stress
    for(int j = 0; j<TEST_ITERATIONS; j++)
    {

      DUT
        sROL_Shl_Mem_WrCmdP0.read(dmCmd_MemCmdP0);
      assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
      assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE*j); 
      assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);

      //Pattern
      for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
      {
        DUT 
          sROL_Shl_Mem_WriteP0.read(memP0);
        //printf("tdata: 0x64%llX)\n", (uint512_t) ((ap_uint<512>) memP0.tdata));
        //printf("tkeep: 0x%llX\n", (uint64_t) memP0.tkeep);
        if(i < 63)
        {
          assert(memP0.tlast == 0);
        } else {
          assert(memP0.tlast == 1);
        }
        //assert(memP0.tkeep == (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF) );
        assert(memP0.tkeep == 0xffffffffffffffff);
        currentMemPattern = i+1;
        assert(memP0.tdata == (ap_uint<512>) (currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern));

        sSHL_Rol_Mem_ReadP0.write(memP0);

      }

      dmSts_MemWrStsP0.tag = 7;
      dmSts_MemWrStsP0.okay = 1;
      dmSts_MemWrStsP0.interr = 0;
      dmSts_MemWrStsP0.slverr = 0;
      dmSts_MemWrStsP0.decerr = 0;
      sSHL_Rol_Mem_WrStsP0.write(dmSts_MemWrStsP0);
      DUT
        printf("debug_out: 0x%x\n", (uint16_t) debug_out);
      assert((debug_out & 0xFF) == 0x0087);

      DUT
        sROL_Shl_Mem_RdCmdP0.read(dmCmd_MemCmdP0);
      assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
      assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE * j); 
      assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);

      for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
      {
        DUT
      }

      dmSts_MemRdStsP0.tag = 7;
      dmSts_MemRdStsP0.okay = 1;
      dmSts_MemRdStsP0.interr = 0;
      dmSts_MemRdStsP0.slverr = 0;
      dmSts_MemRdStsP0.decerr = 0;
      sSHL_Rol_Mem_RdStsP0.write(dmSts_MemWrStsP0);
      DUT
        printf("debug_out: 0x%x\n", (uint16_t) debug_out);
      assert(debug_out == 0x8787);

      printf("%d. Write & Read Pattern completed.\n", j);


      DUT
        sROL_Shl_Mem_WrCmdP0.read(dmCmd_MemCmdP0);
      assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
      assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE*j); 
      assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);


      //Antipattern
      for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
      {
        DUT 
          sROL_Shl_Mem_WriteP0.read(memP0);
        //printf("tdata: 0x64%llX)\n", (uint512_t) ((ap_uint<512>) memP0.tdata));
        //printf("tkeep: 0x%llX\n", (uint64_t) memP0.tkeep);
        if(i < 63)
        {
          assert(memP0.tlast == 0);
        } else {
          assert(memP0.tlast == 1);
        }
        //assert(memP0.tkeep == (0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF) );
        assert(memP0.tkeep == 0xffffffffffffffff);
        currentMemPattern = ~(i+1);
        assert(memP0.tdata == (ap_uint<512>) (currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern,currentMemPattern));

        sSHL_Rol_Mem_ReadP0.write(memP0);

      }

      dmSts_MemWrStsP0.tag = 7;
      dmSts_MemWrStsP0.okay = 1;
      dmSts_MemWrStsP0.interr = 0;
      dmSts_MemWrStsP0.slverr = 0;
      dmSts_MemWrStsP0.decerr = 0;
      sSHL_Rol_Mem_WrStsP0.write(dmSts_MemWrStsP0);
      DUT
        printf("debug_out: 0x%x\n", (uint16_t) debug_out);
      assert(debug_out == 0x8787);

      DUT
        sROL_Shl_Mem_RdCmdP0.read(dmCmd_MemCmdP0);
      assert(dmCmd_MemCmdP0.btt == CHECK_CHUNK_SIZE); 
      assert(dmCmd_MemCmdP0.saddr == CHECK_CHUNK_SIZE * j); 
      assert(dmCmd_MemCmdP0.type == 1 && dmCmd_MemCmdP0.dsa == 0 && dmCmd_MemCmdP0.eof == 1 && dmCmd_MemCmdP0.drr == 0 && dmCmd_MemCmdP0.tag == 0x7);

      for(int i = 0; i < TRANSFERS_PER_CHUNK; i++)
      {
        DUT
      }

      dmSts_MemRdStsP0.tag = 7;
      dmSts_MemRdStsP0.okay = 1;
      dmSts_MemRdStsP0.interr = 0;
      dmSts_MemRdStsP0.slverr = 0;
      dmSts_MemRdStsP0.decerr = 0;
      sSHL_Rol_Mem_RdStsP0.write(dmSts_MemWrStsP0);
      DUT
        printf("debug_out: 0x%x\n", (uint16_t) debug_out);
      assert(debug_out == 0x8787);

      printf("%d. Write & Read Antipattern completed.\n", j);

      //Idle State
      DUT

    }


  printf("------ DONE ------\n");
  return 0;
}
