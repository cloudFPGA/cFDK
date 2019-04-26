//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared accross HLS cores. 
//  *


#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>
#include "network_utils.hpp"


void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out)
{

  out.tdata = 0;
  out.tlast = 0;
  out.tkeep = 0;

  for(int i = 0; i < 8; i++)
  //for(int i = 7; i >=0 ; i--)
  {
    if(!small.empty())
    {
      Axis<8> tmp = small.read();
      //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
      out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< (i*8);
      out.tkeep |= (ap_uint<8>) 0x01 << i;
      //NO latch, because last read from small is still last read
      out.tlast = tmp.tlast;

    } else {
      printf("tried to read empty small stream!\n");
      ////adapt tdata and tkeep to meet default shape
      //out.tdata = out.tdata >> (i+1)*8;
      //out.tkeep = out.tkeep >> (i+1);
      break;
    }
  }

}

void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out)
{

  int positionOfTlast = 8; 
  ap_uint<8> tkeep = big.tkeep;
  for(int i = 0; i<8; i++) //no reverse order!
  {
    tkeep = (tkeep >> 1);
    if((tkeep & 0x01) == 0)
    {
      positionOfTlast = i;
      break;
    }
  }

  //for(int i = 7; i >=0 ; i--)
  for(int i = 0; i < 8; i++)
  {
    //out.full? 
    Axis<8> tmp = Axis<8>(); 
    if(i == positionOfTlast)
    //if(i == 0)
    {
      //only possible position...
      tmp.tlast = big.tlast;
      printf("tlast set.\n");
    } else {
      tmp.tlast = 0;
    }
    tmp.tdata = (ap_uint<8>) (big.tdata >> i*8);
    //tmp.tdata = (ap_uint<8>) (big.tdata >> (7-i)*8);
    tmp.tkeep = (ap_uint<1>) (big.tkeep >> i);
    //tmp.tkeep = (ap_uint<1>) (big.tkeep >> (7-i));

    if(tmp.tkeep == 0)
    {
      continue;
    }

    out.write(tmp); 
  }

}

#define UINT8  ap_uint<8>
#define UINT32 ap_uint<32>

UINT32 bigEndianToInteger(UINT8 *buffer, int lsb)
{
  UINT32 tmp = 0;
  tmp  = ((UINT32) buffer[lsb + 0]); 
  tmp |= ((UINT32) buffer[lsb + 1]) << 8; 
  tmp |= ((UINT32) buffer[lsb + 2]) << 16; 
  tmp |= ((UINT32) buffer[lsb + 3]) << 24; 

  //printf("LSB: %#1x, return: %#04x\n",(UINT8) buffer[lsb + 3], (UINT32) tmp);

  return tmp;
}

void integerToBigEndian(UINT32 n, UINT8 *bytes)
{
  bytes[3] = (n >> 24) & 0xFF;
  bytes[2] = (n >> 16) & 0xFF;
  bytes[1] = (n >> 8) & 0xFF;
  bytes[0] = n & 0xFF;
}


