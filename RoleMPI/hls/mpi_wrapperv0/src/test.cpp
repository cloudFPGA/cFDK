
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "test.hpp"
#include "MPI.hpp"


//for debugging
/*
   void print_int_array(const int *A, size_t width, size_t height)
   {
   printf("\n");
   for(size_t i = 0; i < height; ++i)
   {
   for(size_t j = 0; j < width; ++j)
   {
   printf("%d ", A[i * width + j]);
   }
   printf("\n");
   }
   printf("\n");
   }
   */


//int main( int argc, char **argv )
//DUE TO SHITTY HLS...
void app_main()
{
  MPI_Init();
  //MPI_Init(&argc, &argv);


  int        rank, size;
  MPI_Status status;


  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  //Slaves ...

  printf("Here is rank %d, size is %d. \n",rank, size);

  int local_grid[LDIMY][LDIMX];
  int local_new[LDIMY][LDIMX];
  //#pragma HLS RESOURCE variable=local_grid core=ROM_2P_BRAM
  MPI_Recv(&local_grid[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);

  // print_int_array((const int*) local_grid, LDIMX, LDIMY);

  //only one iteration for now
  //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
  for(int i = 1; i<LDIMY-1; i++)
  {
    for(int j = 1; j<LDIMX-1; j++)
    {
      local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
    }
  }
  MPI_Send(&local_new[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);

  //print_int_array((const int*) local_new, LDIMX, LDIMY);

  printf("Calculation finished.\n");


  MPI_Finalize();

  return;
}



