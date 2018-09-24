#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

//#include "jacobi2d.hpp"
#include "test.hpp"
#include "MPI.hpp"

using namespace hls;

// ----- system reset ---
//ap_uint<1> *sys_reset;
//EMIF Registers
//ap_uint<16> *MMIO_in;
ap_uint<16> MMIO_out;
// ----- MPI_Interface -----
stream<MPI_Interface> *siMPIif;
stream<MPI_Interface> *soMPIif;
stream<Axis<8> > *siMPI_data;
stream<Axis<8> > *soMPI_data;
// ----- FROM SMC -----
ap_uint<32> role_rank;
ap_uint<32> cluster_size;


ap_uint<1> my_app_done = 0;
ap_uint<1> app_init = 0;

ap_uint<4> sendCnt = 0;
ap_uint<4> recvCnt = 0;



void setMMIO_out()
{
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
  ap_uint<16> Display0 = 0;

  Display0  = (ap_uint<16>) ((ap_uint<4>) role_rank);
  Display0 |= ((ap_uint<16>) sendCnt) << SEND_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) recvCnt) << RECV_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) my_app_done) << AP_DONE_SHIFT;
  Display0 |= ((ap_uint<16>) app_init) << AP_INIT_SHIFT;

  MMIO_out = Display0;

}

//void MPI_Init(int* argc, char*** argv)
void MPI_Init()
{

  //TODO: send/wait for INIT packets? 

  while(cluster_size == 0)
  {
    //not yet initialized
    printf("cluster size not yet set!\n");

    //for good intention: wait until stabilized
    ap_wait_n(WAIT_CYCLES);
  } 
  printf("clusterSize: %d, rank: %d\n", (int) cluster_size, (int) role_rank);

  app_init = 1;
  setMMIO_out();

}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{
#pragma HLS INTERFACE ap_vld register port=role_rank name=poSMC_to_ROLE_rank
  *rank = role_rank;
  setMMIO_out();
}

void MPI_Comm_size( MPI_Comm communicator, int* size)
{
#pragma HLS INTERFACE ap_vld register port=cluster_size name=poSMC_to_ROLE_size
  *size = cluster_size;
  setMMIO_out();
}

void send_internal(
    char* data,
    int count,
    MPI_Datatype datatype,
    int destination)
{
#pragma HLS INTERFACE axis register both port=soMPIif
#pragma HLS INTERFACE axis register both port=soMPI_data

  MPI_Interface info = MPI_Interface();
  info.rank = destination;

  //tag is not yet implemented

  int typeWidth = 1;

  switch(datatype)
  {
    case MPI_INTEGER:
      info.mpi_call = MPI_SEND_INT;
      typeWidth = 4;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_SEND_FLOAT;
      typeWidth = 4;
      break;
    default:
      //not yet implemented 
      return;
  }

  info.count = typeWidth * count;

  while(soMPIif->full())
    {
      ap_wait_n(WAIT_CYCLES);
    }
  soMPIif->write(info);


  for(int i = 0; i< info.count; i++)
  {
    //wait for stream 
    while(soMPI_data->full())
    {
      ap_wait_n(WAIT_CYCLES);
    }

    Axis<8>  tmp8 = Axis<8>(data[i]);
    //tmp8.tdata = dataC[i];
    if(i == info.count - 1)
    {
      tmp8.tlast = 1;
    } else {
      tmp8.tlast = 0;
    }
    printf("write MPI data: %#02x\n", (int) tmp8.tdata);
    soMPI_data->write(tmp8);
  }

  sendCnt++;
  printf("MPI_send completed.\n");
  setMMIO_out();

}

void MPI_Send(
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
	//INT Version, so datatype should always be MPI_INTEGER

	char bytes[4*count];

	for(int i=0; i<count; i++)
	{
		  bytes[i*4 + 0] = (data[i] >> 24) & 0xFF;
		  bytes[i*4 + 1] = (data[i] >> 16) & 0xFF;
		  bytes[i*4 + 2] = (data[i] >> 8) & 0xFF;
		  bytes[i*4 + 3] = data[i] & 0xFF;
	}

	send_internal(bytes,count,datatype,destination);

}

void recv_internal(
	    char* data,
	    int count,
	    MPI_Datatype datatype,
	    int source,
	    MPI_Status* status)
{
#pragma HLS INTERFACE axis register both port=siMPIif
#pragma HLS INTERFACE axis register both port=soMPIif
#pragma HLS INTERFACE axis register both port=siMPI_data
  MPI_Interface info = MPI_Interface();
  info.rank = source;

  //tag is not yet implemented

  int typeWidth = 1;

  switch(datatype)
  {
    case MPI_INTEGER:
      info.mpi_call = MPI_RECV_INT;
      typeWidth = 4;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_RECV_FLOAT;
      typeWidth = 4;
      break;
    default:
      //not yet implemented 
      return;
  }

  info.count = typeWidth * count;

  while(soMPIif->full())
    {
      ap_wait_n(WAIT_CYCLES);
    }
  soMPIif->write(info);

  bool tlastOccured = false;
  
  for(int i = 0; i< info.count; i++)
  {
    //wait for stream 
    while(siMPI_data->empty())
    {
      ap_wait_n(WAIT_CYCLES);
    }

    Axis<8>  tmp8 = siMPI_data->read();
    printf("read MPI data: %#02x\n", (int) tmp8.tdata);

    data[i] = (char) tmp8.tdata;
    //type conversion is done implicitely by app itself

    if( tmp8.tlast == 1)
    {
      printf("received TLAST at count %d!\n", i);
      tlastOccured = true;
      break;
    }
  }

  *status = 1;

  //to empty stream 
  siMPIif->read();
  while(!siMPI_data->empty() && !tlastOccured)
  {
    printf("received stream longer than count!\n");
    Axis<8>  tmp8 = siMPI_data->read();
    *status = 2;
  }

  recvCnt++;
  printf("MPI_recv completed.\n");
  setMMIO_out();

}


void MPI_Recv(
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{
/*	int typeWidth = 1;

	  switch(datatype)
	  {
	    case MPI_INTEGER:
	      typeWidth = 4;
	      break;
	    case MPI_FLOAT:
	      typeWidth = 4;
	      break;
	    default:
	      //not yet implemented
	      return;
	  }*/

	//INT Version, so datatype should always be MPI_INTEGER

	char dataChar[4*count];

	recv_internal(dataChar, count, datatype, source, status);

	for(int i=0; i<count; i++)
	{
		data[i]  = ((int) dataChar[i*4 + 3]);
		data[i] |= ((int) dataChar[i*4 + 2]) << 8;
		data[i] |= ((int) dataChar[i*4 + 1]) << 16;
		data[i] |= ((int) dataChar[i*4 + 0]) << 24;
	}


}

void MPI_Finalize()
{
  //TODO: send something like DONE packets?
  my_app_done = 1;
  app_init = 0;
  setMMIO_out();
}

/*void MPI_Barrier(MPI_Comm communicator)
{
  //not yet implemented
}*/

void c_testbench_access(
    // ----- system reset ---
    //ap_uint<1> *sys_reset_arg,
    //EMIF Registers
    ap_uint<16> MMIO_in_arg,
    ap_uint<16> MMIO_out_arg,
    // ----- MPI_Interface -----
    stream<MPI_Interface> *siMPIif_arg,
    stream<MPI_Interface> *soMPIif_arg,
    stream<Axis<8> > *siMPI_data_arg,
    stream<Axis<8> > *soMPI_data_arg,
    // ----- FROM SMC -----
    ap_uint<32> role_rank_arg,
    ap_uint<32> cluster_size_arg
    )
{
  //sys_reset = sys_reset_arg;

  //MMIO_in = MMIO_in_arg;
  MMIO_out = MMIO_out_arg;

  siMPIif = siMPIif_arg;
  soMPIif = soMPIif_arg;

  siMPI_data = siMPI_data_arg;
  soMPI_data = soMPI_data_arg;

  role_rank = role_rank_arg;
  cluster_size = cluster_size_arg;

}

void c_testbench_read(
    ap_uint<16> *MMIO_out_arg
    )
{
	*MMIO_out_arg = MMIO_out;
}

//void mpi_wrapper(
//    // ----- system reset ---
//    ap_uint<1> sys_reset,
//    //EMIF Registers
//    ap_uint<16> *MMIO_in,
//    ap_uint<16> *MMIO_out,
//    // ----- MPI_Interface -----
//    stream<MPI_Interface> &siMPIif,
//    stream<MPI_Interface> &soMPIif,
//    stream<Axis<8> > &siMPI_data,
//    stream<Axis<8> > &soMPI_data,
//    // ----- FROM SMC -----
//    ap_uint<32> *role_rank,
//    ap_uint<32> *cluster_size
//    )
void mpi_wrapper(
    // ----- system reset ---
    ap_uint<1> sys_reset
    )
{
#pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset
//#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO


  //===========================================================
  // Reset global variables 

  if(sys_reset == 1)
  {
    my_app_done = 0;
    sendCnt = 0;
    recvCnt = 0;
    app_init = 0;
    //don't start app in reset state 

    setMMIO_out();
    return;
  }

  //===========================================================
  // Start main program 

  if(my_app_done == 0)
  {
    app_main();
  }

}

