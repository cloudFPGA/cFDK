#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "jacobi2d.hpp"
#include "MPI.hpp"

using namespace hls;

//void MPI_Init()
void MPI_Init(int* argc, char*** argv)
{
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


void mpi_wrapper(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    //EMIF Registers
    ap_uint<32> MMIO_in,
    ap_uint<32> MMIO_out,
    // ----- MPI_Interface -----
    stream<MPI_Interface> siMPIif,
    stream<MPI_Interface> soMPIif,
    stream<Axis<8> > siMPI_data,
    stream<Axis<8> > soMPI_data,
    // ----- FROM SMC -----
    ap_uint<32> role_rank,
    ap_uint<32> cluster_size
    )
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

  int argc = 0;
  char *argv[] = {NULL};

  main(argc, &argv[0]);


}

