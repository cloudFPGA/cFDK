#ifndef _JACOBI2D_H_
#define _JACOBI2D_H_

#include <stdlib.h>
#include <stdio.h>

#define USE_INTEGER

#define DIM 16
#define LDIMX 16
#define LDIMY 9 // = DIM/2 + 1

#define DATA_CHANNEL_TAG 1
#define CMD_CHANNEL_TAG 2

/*
#ifdef USE_INTEGER
void print_array(const int *A, size_t width, size_t height)
#else
void print_array(const float *A, size_t width, size_t height)
#endif
{
  printf("\n");
  for(size_t i = 0; i < height; ++i)
  {
    for(size_t j = 0; j < width; ++j)
    {
#ifdef USE_INTEGER
      printf("%d ", A[i * width + j]);
#else
      printf("%.2f ", A[i * width + j]);
#endif
    }
    printf("\n");
  }
  printf("\n");
}
*/

//void jacobi2d_main();


int main( int argc, char **argv );


#endif
