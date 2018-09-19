#ifndef _MPI_H_
#define _MPI_H_


#include <stdint.h>
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

using namespace hls;


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

#define MPI_SEND_INT 0
#define MPI_RECV_INT 1
#define MPI_SEND_FLOAT 2
#define MPI_RECV_FLOAT 3
#define MPI_BARRIER 4
#define mpiCall uint8_t

/*
 * MPI-F Interface
 */
struct MPI_Interface {
  ap_uint<8>     mpi_call;
  ap_uint<32>    count;
  ap_uint<32>    rank;
  MPI_Interface() {}
};

//HLS PORTS as global connections 
// ----- system reset ---
ap_uint<1> sys_reset;
//EMIF Registers
ap_uint<32> MMIO_in;
ap_uint<32> MMIO_out;
// ----- MPI_Interface -----
stream<MPI_Interface> siMPIif;
stream<MPI_Interface> soMPIif;
stream<Axis<8> > siMPI_data;
stream<Axis<8> > soMPI_data;
// ----- FROM SMC -----
ap_uint<32> role_rank;
ap_uint<32> cluster_size;



#define MPI_Status uint8_t
#define MPI_Comm   uint8_t
#define MPI_Datatype uint8_t

#define MPI_COMM_WORLD 0
#define MPI_INTEGER 0
#define MPI_FLOAT   1


//void MPI_Init(int* argc, char*** argv)
void MPI_Init()
{
#pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE axis register both port=siMPIif
#pragma HLS INTERFACE axis register both port=soMPIif
#pragma HLS INTERFACE axis register both port=siMPI_data
#pragma HLS INTERFACE axis register both port=soMPI_data
#pragma HLS INTERFACE ap_vld register port=role_rank name=poSMC_to_ROLE_rank
#pragma HLS INTERFACE ap_vld register port=cluster_size name=poSMC_to_ROLE_size
}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{

}

void MPI_Comm_size( MPI_Comm communicator, int* size)
{

}

void MPI_Send(
    void* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{


}

void MPI_Recv(
    void* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{

}

void MPI_Finalize()
{

}

void MPI_Barrier(MPI_Comm communicator)
{

}

#endif
