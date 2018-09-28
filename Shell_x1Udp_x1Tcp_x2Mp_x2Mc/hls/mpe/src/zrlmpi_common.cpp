
#include "zrlmpi_common.hpp"
#include <stdint.h>


UINT32 littleEndianToInteger(UINT8 *buffer, int lsb)
{
  UINT32 tmp = 0;
  tmp  = ((UINT32) buffer[lsb + 3]); 
  tmp |= ((UINT32) buffer[lsb + 2]) << 8; 
  tmp |= ((UINT32) buffer[lsb + 1]) << 16; 
  tmp |= ((UINT32) buffer[lsb + 0]) << 24; 

  //printf("LSB: %#1x, return: %#04x\n",(UINT8) buffer[lsb + 3], (UINT32) tmp);

  return tmp;
}

void integerToLittleEndian(UINT32 n, UINT8 *bytes)
{
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;
}



int bytesToHeader(UINT8 bytes[MPIF_HEADER_LENGTH], MPI_Header &header)
{
  //check validity
  for(int i = 0; i< 4; i++)
  {
    if(bytes[i] != 0x96)
    {
      return -1;
    }
  }
  
  for(int i = 18; i<28; i++)
  {
    if(bytes[i] != 0x00)
    {
      return -2;
    }
  }
  
  for(int i = 28; i<32; i++)
  {
    if(bytes[i] != 0x96)
    {
      return -3;
    }
  }

  //convert
  header.dst_rank = littleEndianToInteger(bytes, 4);
  header.src_rank = littleEndianToInteger(bytes,8);
  header.size = littleEndianToInteger(bytes,12);

  header.call = static_cast<mpiCall>((int) bytes[16]);

  header.type = static_cast<mpiCall>((int) bytes[17]);

  return 0;

}

void headerToBytes(MPI_Header header, UINT8 bytes[MPIF_HEADER_LENGTH])
{
  for(int i = 0; i< 4; i++)
  {
    bytes[i] = 0x96;
  }
  UINT8 tmp[4];
  integerToLittleEndian(header.dst_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[4 + i] = tmp[i];
  }
  integerToLittleEndian(header.src_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[8 + i] = tmp[i];
  }
  integerToLittleEndian(header.size, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[12 + i] = tmp[i];
  }

  bytes[16] = (UINT8) header.call; 

  bytes[17] = (UINT8) header.type;

  for(int i = 18; i<28; i++)
  {
    bytes[i] = 0x00; 
  }
  
  for(int i = 28; i<32; i++)
  {
    bytes[i] = 0x96; 
  }

}




