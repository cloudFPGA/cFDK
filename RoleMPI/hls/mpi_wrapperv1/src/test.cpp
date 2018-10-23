
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "MPI.hpp"
#include "test.hpp"


int app_main(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<Axis<8> > *soMPI_data,
    stream<Axis<8> > *siMPI_data
    )
{
  int        rank, size;
  MPI_Status status;

  MPI_Init();

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );


  printf("Here is rank %d, size is %d. \n",rank, size);


    int local_grid[LDIMY + 1][LDIMX];
    int local_new[LDIMY + 1][LDIMX];

    //MPI_Recv(soMPIif, siMPI_data, &local_grid[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    int number_of_recv_packets = LDIMY + 1; 
    if(rank == size -1)
    {
      number_of_recv_packets--;
    }
    for(int j = 0; j< number_of_recv_packets; j++)
    {
      MPI_Recv(soMPIif, siMPI_data, &local_grid[j][0], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    }

    // print_int_array((const int*) local_grid, LDIMX, LDIMY);

    //only one iteration for now
    //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
    for(int i = 1; i < number_of_recv_packets; i++)
    {
      for(int j = 1; j<LDIMX-1; j++)
      {
        local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
      }
    }
    //MPI_Send(soMPIif, soMPI_data, &local_new[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
    for(int j = 0; j< number_of_recv_packets; j++)
    {
      MPI_Send(soMPIif, soMPI_data, &local_new[j][0], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
    }

    //print_int_array((const int*) local_new, LDIMX, LDIMY);

    //printf("Calculation finished.\n");

  MPI_Finalize();
  return 0;
}


