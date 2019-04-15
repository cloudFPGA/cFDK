
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
  stream<Axis<64> > soTcp;
  stream<IPMeta> soIP;

  stream<MPI_Interface> MPIif_in;
  stream<MPI_Interface> MPIif_out;
  stream<Axis<8> > MPI_data_in;
  stream<Axis<8> > MPI_data_out;

  for(int i = 0; i < MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS; i++)
  {
        MRT[i] = 0;
  }

  MRT[0] = 1; //own rank 
  //routing table
  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] = 168496129; //10.11.12.1
  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] = 168496141; //10.11.12.13
  MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] = 168496142; //10.11.12.14
  mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);


  //printf("MRT 3: %d\n",(int) MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 3]);

  //check DEBUG copies
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] == MRT[NUMBER_CONFIG_WORDS + 0]);
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] == MRT[NUMBER_CONFIG_WORDS + 1]);
  assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] == MRT[NUMBER_CONFIG_WORDS + 2]);

  //MPI TX
  Axis<8> tmp8 = Axis<8>();
  tmp8.tdata = 'H';
  tmp8.tkeep = 1;
  tmp8.tlast = 0;
  MPI_data_in.write(tmp8);

  //nothing should happen
  mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);

  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  info.count = 12;
  info.rank = 2;

  MPIif_in.write(info);

  char* msg = "HELLO WORLD!";

  for(int i = 1; i< 12; i++)
  {
    tmp8.tdata = msg[i];
    if(i == 11)
    {
      tmp8.tlast = 1; 
    }
    printf("write MPI data: %#02x\n", (int) tmp8.tdata);
    MPI_data_in.write(tmp8);

    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);

    if(i == 3)
    {
      IPMeta ipDst = soIP.read();
      printf("ipDst: %#010x\n", (unsigned int) ipDst.ipAddress);
      assert(ipDst.ipAddress == MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2]);
    }

    //printf("MPE out: %#010x\n", (long long) soTcp.read().tdata);

  }

  for(int i = 0; i < 20; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }

  Axis<64> tmp64 = Axis<64>();
  while(!soTcp.empty())
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
    tmp64 = soTcp.read();
    printf("MPE out: %#020llx\n", (unsigned long long) tmp64.tdata);
    siTcp.write(tmp64);
  }


  
  //MPI RX 

  //change rank to receiver 
  MRT[0] = 2;
  for(int i = 0; i < 20; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }


  IPMeta ipMS = IPMeta();
  ipMS.ipAddress = 0x0a0b0c0d;

  //nothing should happen until now
  siIP.write(ipMS);
  
  for(int i = 0; i < 20; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }

  MPI_Interface info_out = MPIif_out.read();
  assert(info_out.mpi_call == info.mpi_call);
  assert(info_out.count == info.count);
  assert(info_out.rank == 1);

  for(int i = 0; i< 12; i++)
  {
    tmp8 = MPI_data_out.read();
    
    printf("MPI read data: %#02x\n", (int) tmp8.tdata);

    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);

    // tlast => i=12 
    assert( !(tmp8.tlast == 1) || i==11);
    assert(((int) tmp8.tdata) == msg[i]);

  }


  //printf("DONE\n");
  return succeded? 0 : -1;
  //return 0;
}
