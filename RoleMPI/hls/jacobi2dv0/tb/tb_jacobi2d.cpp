
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "../src/MPI.hpp"
#include "../src/test.hpp"

int main(){


  bool succeded = true;
    
  // ----- system reset ---
    ap_uint<1> sys_reset = 0;
    //EMIF Registers
    ap_uint<16> MMIO_in = 0;
    ap_uint<16> MMIO_out = 0;
    // ----- MPI_Interface -----
    stream<MPI_Interface> siMPIif;
    stream<MPI_Interface> soMPIif;
    stream<Axis<8> > siMPI_data;
    stream<Axis<8> > soMPI_data;
    // ----- FROM SMC -----
    ap_uint<32> role_rank = 0;
    ap_uint<32> cluster_size = 0;


    cluster_size = 2;
    role_rank = 1;

    c_testbench_access(MMIO_in, MMIO_out, &siMPIif, &soMPIif, &siMPI_data, &soMPI_data, role_rank, cluster_size);
    
    //test reset 
    //mpi_wrapper(1);



    int grid[DIM][DIM];
    //fill with zeros and init borders with 4
    for(int i = 0; i<DIM; i++)
    {
      for(int j = 0; j<DIM; j++)
      {

        if( i == 0 || i == DIM-1 || j == 0 || j == DIM-1)
        {
          grid[i][j] = 4;
        }
        else {
          grid[i][j] = 0;
        }
      }
    }




  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  info.count = LDIMY*LDIMX*4;
  info.rank = 0;

  siMPIif.write(info);

  char* data = (char* ) grid;

  for(int i = 0; i< info.count; i++)
  {
    Axis<8>  tmp8 = Axis<8>();
    tmp8.tdata = data[i];
    if(i == info.count - 1)
    {
      tmp8.tlast = 1; 
    } else {
      tmp8.tlast = 0;
    }
   printf("write MPI .tdata: %#02x, .tkeep %d, .tlast %d\n", (int) tmp8.tdata, (int) tmp8.tkeep, (int) tmp8.tlast);
    siMPI_data.write(tmp8);
  }



    mpi_wrapper(sys_reset);

    //empty streams
    info = soMPIif.read();
    assert(info.mpi_call == MPI_RECV_INT);
    info = soMPIif.read();
    assert(info.mpi_call == MPI_SEND_INT);

  for(int i = 0; i< info.count; i++)
  {//not while...because we want test if there are exactly i bytes left 
    soMPI_data.read();
  }

  c_testbench_read(&MMIO_out);

  assert(MMIO_out == 0x1111);

    printf("DONE\n");

    return succeded? 0 : -1;
    //return 0;
}
