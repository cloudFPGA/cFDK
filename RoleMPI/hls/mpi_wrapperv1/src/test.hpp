#ifndef _JACOBI2D_H_
#define _JACOBI2D_H_

#include <stdlib.h>
#include <stdio.h>

#include "MPI.hpp"

#define DIM 4
#define LDIMX 4
#define LDIMY 3 // = DIM/2 + 1



//int main( int argc, char **argv );
//DUE TO SHITTY HLS...
//void app_main();
void app_main(
		// ----- MPI_Interface -----
		stream<MPI_Interface> *soMPIif,
		stream<Axis<8> > *soMPI_data,
		stream<Axis<8> > *siMPI_data
		);


#endif
