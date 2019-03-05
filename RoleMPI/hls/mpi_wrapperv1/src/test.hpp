#ifndef JACOBI_TEST_H
#define JACOBI_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include "MPI.hpp"

#define USE_INTEGER

#define DIM 1024
#define LDIMX 1024
#define LDIMY 64

#define PACKETLENGTH 256
#define CORNER_VALUE_INT 128
#define JACOBI_ITERATIONS 10

#define DATA_CHANNEL_TAG 1 
#define CMD_CHANNEL_TAG 2


int app_main(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<Axis<8> > *soMPI_data,
    stream<Axis<8> > *siMPI_data
    );

#endif
