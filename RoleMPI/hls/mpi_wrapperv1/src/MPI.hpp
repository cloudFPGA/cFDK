#ifndef _MPI_H_
#define _MPI_H_


#include <stdint.h>
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

using namespace hls;

#define WAIT_CYCLES 10
//Display0
#define RECV_CNT_SHIFT 8
#define SEND_CNT_SHIFT 4
#define AP_DONE_SHIFT 12
#define AP_INIT_SHIFT 13

#define RECV_WRITE_INFO 0
#define RECV_READ_DATA 1
#define RECV_FINISH 2
#define WrapperRecvState uint8_t

#define SEND_WRITE_INFO 0
#define SEND_WRITE_DATA 1
#define SEND_FINISH 2
#define WrapperSendState uint8_t



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


#define MPI_Status uint8_t
#define MPI_Comm   uint8_t
#define MPI_Datatype uint8_t

#define MPI_COMM_WORLD 0
#define MPI_INTEGER 0
#define MPI_FLOAT   1


//void MPI_Init(int* argc, char*** argv);
void MPI_Init();
void MPI_Comm_rank(MPI_Comm communicator, int* rank);
void MPI_Comm_size( MPI_Comm communicator, int* size);


void MPI_Send(
	// ----- MPI_Interface -----
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *soMPI_data,
	// ----- MPI Signature -----
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator);


void MPI_Recv(
	// ----- MPI_Interface -----
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *siMPI_data,
	// ----- MPI Signature -----
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status);


void MPI_Finalize();

//void MPI_Barrier(MPI_Comm communicator);


void mpi_wrapper(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    // ----- FROM SMC -----
    ap_uint<32> role_rank_arg,
    ap_uint<32> cluster_size_arg,
	// ----- TO SMC ------
	ap_uint<16> *MMIO_out,
	// ----- MPI_Interface -----
	//stream<MPI_Interface> *soMPIif,
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *soMPI_data,
	stream<Axis<8> > *siMPI_data
    );


#endif
