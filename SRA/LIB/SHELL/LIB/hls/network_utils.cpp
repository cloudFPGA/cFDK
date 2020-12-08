/************************************************
 * Copyright (c) 2016-2019, IBM Research.
 * Copyright (c) 2015, Xilinx, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************/

//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared accross HLS cores. 
//  *
//  *


#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>
#include <queue>
#include <string>
#include "network_utils.hpp"
#include "NTS/AxisRaw.hpp"


void convertAxisToNtsWidth(stream<Axis<8> > &small, AxisRaw &out)
{

  out.setTData(0x0);
  out.setTLast(0);
  out.setTKeep(0);

  tData newd = 0x0;
  tKeep newk = 0x0;

  for(int i = 0; i < 8; i++)
  //for(int i = 7; i >=0 ; i--)
  {
    if(!small.empty())
    {
      Axis<8> tmp = small.read();
      //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
      //out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< (i*8);
      newd |= ((ap_uint<64>) (tmp.getTData()) )<< (i*8);
      //out.tkeep |= (ap_uint<8>) 0x01 << i;
      newk = (ap_uint<8>) 0x01 << i;
      //NO latch, because last read from small is still last read
      //out.tlast = tmp.tlast;
      out.setTLast(tmp.getTLast());

    } else {
      printf("tried to read empty small stream!\n");
      ////adapt tdata and tkeep to meet default shape
      //out.tdata = out.tdata >> (i+1)*8;
      //out.tkeep = out.tkeep >> (i+1);
      break;
    }
  }

  out.setTData(newd);
  out.setTKeep(newk);

}


void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out)
{

  int positionOfTlast = 8; 
  ap_uint<8> tkeep = big.getTKeep();
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
      tmp.setTLast(big.getTLast());
      printf("tlast set.\n");
    } else {
      tmp.setTLast(0);
    }
    tmp.setTData((ap_uint<8>) (big.getTData() >> i*8));
    tmp.setTKeep((ap_uint<1>) (big.getTKeep() >> i));

    if(tmp.getTKeep() == 0)
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


/*****************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord, the 16-bit unsigned data to swap.
 *
 * @return a 16-bit unsigned data.
 *****************************************************************************/
/* MOVED to nts_utils.hpp
ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8));
}
*/

/*****************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord, a 32-bit unsigned data.
 *
 * @return a 32-bit unsigned data.
 *****************************************************************************/
/* MOVED to nts_utils.hpp
ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
    return (inputVector.range( 7, 0), inputVector(15,  8),
        inputVector.range(23,16), inputVector(31, 24));
}
*/

/*****************************************************************************
 * @brief Returns the number of valid bytes in an AxiWord.
 * @param[in] The 'tkeep' field of the AxiWord.
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepValue) {
    ap_uint<4> count = 0;
    switch(keepValue){
        case 0x01: count = 1; break;
        case 0x03: count = 2; break;
        case 0x07: count = 3; break;
        case 0x0F: count = 4; break;
        case 0x1F: count = 5; break;
        case 0x3F: count = 6; break;
        case 0x7F: count = 7; break;
        case 0xFF: count = 8; break;
    }
    return count;
}

/*****************************************************************************
 * @brief Returns the 'tkeep' field of an AxiWord as a function of the number
 *  of valid bytes in that word.
 * @param[in] The number of valid bytes in an AxiWord.
 *****************************************************************************/
ap_uint<8> lenToKeep(ap_uint<4> noValidBytes) {
    ap_uint<8> keep = 0;

    switch(noValidBytes) {
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}



