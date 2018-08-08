
#include "mpe.hpp"

using namespace hls;


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
		)
{
#pragma HLS INTERFACE m_axi depth=2048 port=MEM bundle=poMPE_MEM_buffer_AXIM
#pragma HLS INTERFACE axis register forward port=siTcp
#pragma HLS INTERFACE axis register forward port=soTcp
#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_MPE_ctrlLink
#pragma HLS INTERFACE ap_stable register port=MPIif name=pioMPI_interface
#pragma HLS INTERFACE axis register forward port=MPI_data_in
#pragma HLS INTERFACE axis register forward port=MPI_data_out
//TODO: ap_ctrl?? (in order not to need reset in the first place)

//===========================================================
// Core-wide variables


//===========================================================
// Reset global variables 


//===========================================================
// 
 

//===========================================================
//  

  return;
}



