#ifndef _JACOBI2D_H_
#define _JACOBI2D_H_

#include <stdlib.h>
#include <stdio.h>

#include "MPI.hpp"

#define DIM 128
#define LDIMX 128
#define LDIMY 65 // = DIM/2 + 1



//int main( int argc, char **argv );
//DUE TO SHITTY HLS...
//void app_main();
void app_main(
    // ----- MPI_Interface -----
    //ap_uint<16> *MMIO_out,
    stream<MPI_Interface> *soMPIif,
    stream<Axis<8> > *soMPI_data,
    stream<Axis<8> > *siMPI_data
    );


#endif
