
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "../src/MPI.hpp"


int main(){


  bool succeded = true;
    
  // ----- system reset ---
    ap_uint<1> sys_reset = 0;
    //EMIF Registers
    ap_uint<16> MMIO_in = 0;
    ap_uint<16> MMIO_out = 0;
    // ----- MPI_Interface -----
    stream<MPI_Interface> siMPIif_arg;
    stream<MPI_Interface> soMPIif_arg;
    stream<Axis<8> > siMPI_data_arg;
    stream<Axis<8> > soMPI_data_arg;
    // ----- FROM SMC -----
    ap_uint<32> role_rank = 0;
    ap_uint<32> cluster_size = 0;


    //mpi_wrapper(sys_reset, &MMIO_in, &MMIO_out, siMPIif_arg, soMPIif_arg, siMPI_data_arg, soMPI_data_arg, &role_rank, &cluster_size); 
    c_testbench_access(&sys_reset, &MMIO_in, &MMIO_out, &siMPIif_arg, &soMPIif_arg, &siMPI_data_arg, &soMPI_data_arg, &role_rank, &cluster_size); 
    
    mpi_wrapper();

    cluster_size = 2;
    role_rank = 1;
    
    mpi_wrapper();


  printf("DONE\n");

  return succeded? 0 : -1;
  //return 0;
}
