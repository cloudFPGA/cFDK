
#include <stdio.h>
#include <stdlib.h>
#include "../src/mpe.hpp"

#include <stdint.h>

using namespace hls;

int main(){

  bool succeded = true;
  ap_uint<32> MRT[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
  ap_uint<1> sys_reset = 0b0;
  stream<Axis<64> > siTcp, storeSEND_REQ, storeData;
  stream<IPMeta>  siIP;
  stream<Axis<64> > soTcp;
  stream<IPMeta> soIP;

  stream<MPI_Interface> MPIif_in;
  stream<MPI_Interface> MPIif_out;
  stream<Axis<8> > MPI_data_in;
  stream<Axis<8> > MPI_data_out, tmp8Stream;

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

  Axis<64> tmp64 = Axis<64>();
  Axis<8>  tmp8 = Axis<8>();

  //MPI_send() 

  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  info.count = 12;
  info.rank = 2;

  MPIif_in.write(info);

  char* msg = "HELLO WORLD!";

  for(int i = 0; i< 12; i++)
  {
    tmp8.tdata = msg[i];
    if(i == 11)
    {
      tmp8.tlast = 1; 
    }
   // printf("write MPI data: %#02x\n", (int) tmp8.tdata);
    MPI_data_in.write(tmp8);

    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }

  ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 

  //SEND_REQUEST expected 
  IPMeta ipDst = soIP.read();
  printf("ipDst: %#010x\n", (unsigned int) ipDst.ipAddress);
  assert(ipDst.ipAddress == MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2]);


  //while(!soTcp.empty())
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
    tmp64 = soTcp.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    storeSEND_REQ.write(tmp64);
    for(int j = 0; j<8; j++)
    {
      bytes[i*8 + j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
    }
  }

  int ret = bytesToHeader(bytes, header);
  assert(ret == 0);
  assert(header.dst_rank == 2 && header.src_rank == 1); 
  assert(header.type == SEND_REQUEST);

  //assemble clear to send 
  header = MPI_Header();
  printf("send CLEAR_TO_SEND\n");

  header.type = CLEAR_TO_SEND;
  header.src_rank = 2;
  header.dst_rank = 1;
  header.size = 0;
  header.call = MPI_RECV_INT;
  headerToBytes(header, bytes);
  //write header
  for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
  {
    Axis<8> tmp = Axis<8>(bytes[i]);
    tmp.tlast = 0;
    if ( i == MPIF_HEADER_LENGTH - 1)
    {
      tmp.tlast = 1;
    }
    tmp8Stream.write(tmp);
  }
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
      convertAxisToNtsWidth(tmp8Stream, tmp64);
      siTcp.write(tmp64);
      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp64.tdata);
  }

  siIP.write(ipDst);
  
  for(int i = 0; i < 20; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }

  //Data 
  ipDst = soIP.read();
  printf("ipDst: %#010x\n", (unsigned int) ipDst.ipAddress);
  assert(ipDst.ipAddress == MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2]);

  while(!soTcp.empty())
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
    tmp64 = soTcp.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    storeData.write(tmp64);
  }

  //assemble ACK
  header = MPI_Header();

  header.type = ACK;
  header.src_rank = 2;
  header.dst_rank = 1;
  header.size = 0;
  header.call = MPI_RECV_INT;
  headerToBytes(header, bytes);
  //write header
  for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
  {
    Axis<8> tmp = Axis<8>(bytes[i]);
    tmp.tlast = 0;
    if ( i == MPIF_HEADER_LENGTH - 1)
    {
      tmp.tlast = 1;
    }
    tmp8Stream.write(tmp);
  }
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
      convertAxisToNtsWidth(tmp8Stream, tmp64);
      siTcp.write(tmp64);
      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp64.tdata);
  }


  siIP.write(ipDst);
  
  for(int i = 0; i < 20; i++)
  {
    mpe_main(sys_reset, MRT, siTcp, siIP, soTcp, soIP, MPIif_in, MPIif_out, MPI_data_in, MPI_data_out);
  }


  printf("DONE\n");
  return succeded? 0 : -1;
  //return 0;
}
