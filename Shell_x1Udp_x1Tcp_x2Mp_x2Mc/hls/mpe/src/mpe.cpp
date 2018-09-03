
#include "mpe.hpp"
#include "../../smc/src/smc.hpp"

using namespace hls;

ap_uint<32> localMRT[MAX_CLUSTER_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

void mpe_main(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    // ----- link to SMC -----
    ap_uint<32> ctrlLink[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

    // ----- Nts0 / Tcp Interface -----
    stream<Axis<64> >   &siTcp,
    stream<IPMeta>      &siIP,
    stream<Axis<64> >   &soTcp,
    stream<IPMeta>      &soIP,

    // ----- Memory -----
    //ap_uint<8> *MEM, TODO: maybe later

    // ----- MPI_Interface -----
    stream<MPI_Interface> &siMPIif,
    stream<Axis<8> > &siMPI_data,
    stream<Axis<8> > &soMPI_data
    )
{
#pragma HLS INTERFACE axis register forward port=siTcp
#pragma HLS INTERFACE axis register forward port=siIP
#pragma HLS INTERFACE axis register forward port=soTcp
#pragma HLS INTERFACE axis register forward port=soIP
#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_MPE_ctrlLink_AXI
#pragma HLS INTERFACE axis register forward port=siMPIif
#pragma HLS INTERFACE axis register forward port=siMPI_data
#pragma HLS INTERFACE axis register forward port=soMPI_data
#pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset
#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_MPE_ctrlLink_AXI

//#pragma HLS RESOURCE variable=localMRT core=RAM_1P_BRAM //maybe better to decide automatic?


//===========================================================
// Core-wide variables


//===========================================================
// Reset global variables 

  if(sys_reset == 1)
  {
    for(int i = 0; i < MAX_CLUSTER_SIZE; i++)
    {
      localMRT[i] = 0;
    }
    for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    {
      config[i] = 0;
    }
    for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
    {
      status[i] = 0;
    }
  }

//===========================================================
//

  //copy MRT axi Interface
  //MRT data are after possible config DATA
  for(int i = 0; i < MAX_CLUSTER_SIZE; i++)
  {
        //localMRT[i] = MRT[i];
    localMRT[i] = ctrlLink[i + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
  }
  for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
  {
    config[i] = ctrlLink[i];
  }

  //DEBUG
  ctrlLink[3 + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS] = 42;

  //copy routing nodes 0 - 2 for debug
  status[0] = localMRT[0];
  status[1] = localMRT[1];
  status[2] = localMRT[2];


  //TODO: some consistency check for tables? (e.g. every IP address only once...)
 

//===========================================================
//  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
  }

  return;
}



