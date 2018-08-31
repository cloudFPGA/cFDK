
#include <stdio.h>
#include <stdlib.h>
#include "../src/mpe.hpp"

#include <stdint.h>

using namespace hls;

int main(){

  bool succeded = true;
  ap_uint<32> MRT[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
  ap_uint<1> sys_reset = 0b0;
  stream<Axis<64> > siTcp;
  stream<IPMeta>  siIP;
  stream<Axis<64> >	soTcp;
  stream<IPMeta> soIP;

  stream<MPI_Interface> MPIif;
  stream<Axis<8> > MPI_data_in;
  stream<Axis<8> > MPI_data_out;

  for(int i = 0; i < MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS; i++)
  {
  			MRT[i] = 0;
  }

  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] = 168496129; //10.11.12.1
  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] = 168496141; //10.11.12.13
  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] = 168496142; //10.11.12.14
  mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif, MPI_data_in, MPI_data_out);


  printf("MRT 3: %d\n",(int) MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 3]);

  //check DEBUG copies
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] == MRT[NUMBER_CONFIG_WORDS + 0]);
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] == MRT[NUMBER_CONFIG_WORDS + 1]);
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] == MRT[NUMBER_CONFIG_WORDS + 2]);


  //printf("DONE\n");
  return succeded? 0 : -1;
  //return 0;
}
