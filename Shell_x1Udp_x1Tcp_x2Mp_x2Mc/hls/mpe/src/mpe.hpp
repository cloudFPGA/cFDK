#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
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

/*
 * MPI Interface
 */
 struct MPI_Interface {
	// stream<Axis<8> > 	 data_in;
	// stream<Axis<8> >  	data_out;
	 ap_uint<8>		   	mpi_call;
	 ap_uint<32>	   	count_in;
	 ap_uint<32>		count_out;
	 ap_uint<32>		src_rank;
	 ap_uint<32>		dst_rank;
	 MPI_Interface() {}
 };

void mpe_main(
		// ----- link to SMC -----
		ap_uint<32> *ctrlLink,

		// ----- Nts0 / Tcp Interface -----
		stream<Axis<64> >		&siTcp,
		stream<Axis<64> >		&soTcp,

		// ----- Memory -----
		ap_uint<8> *MEM,

		// ----- MPI_Interface -----
		MPI_Interface *MPIif,
		stream<Axis<8> > &MPI_data_in,
		stream<Axis<8> > &MPI_data_out
		);


#endif
