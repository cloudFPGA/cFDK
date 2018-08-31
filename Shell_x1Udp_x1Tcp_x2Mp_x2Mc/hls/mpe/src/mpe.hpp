#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>
#include "../../smc/src/smc.hpp"

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

 struct IPMeta {
	 ap_uint<32> ipAddress;
	 IPMeta() {}
 };


#define NUMBER_CONFIG_WORDS 10
#define NUMBER_STATUS_WORDS 10

 /*
  * ctrlLINK Structure:
  * 1.         0 --            NUMBER_CONFIG_WORDS -1 :  possible configuration from SMC to MPE
  * 2. NUMBER_CONFIG_WORDS --  NUMBER_STATUS_WORDS -1 :  possible status from MPE to SMC
  * 3. NUMBER_STATUS_WORDS --  MAX_CLUSTER_SIZE +
  *                              NUMBER_CONFIG_WORDS +
  *                              NUMBER_STATUS_WORDS    : Message Routing Table (MRT)
  */

void mpe_main(
		// ----- system reset ---
				ap_uint<1> sys_reset,
				// ----- link to SMC -----
				ap_uint<32> ctrlLink[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

				// ----- Nts0 / Tcp Interface -----
				stream<Axis<64> >		&siTcp,
				stream<IPMeta> 			&siIP,
				stream<Axis<64> >		&soTcp,
				stream<IPMeta>			&soIP,

				// ----- Memory -----
				//ap_uint<8> *MEM, TODO: maybe later

				// ----- MPI_Interface -----
				stream<MPI_Interface> &siMPIif,
				stream<Axis<8> > &siMPI_data,
				stream<Axis<8> > &soMPI_data
		);


#endif
