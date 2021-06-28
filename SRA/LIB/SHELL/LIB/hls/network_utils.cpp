/*******************************************************************************
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains network types and functions that are shared
//  *        across HLS cores.
//  *


#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>
#include <queue>
#include <string>
#include "network_utils.hpp"
#include "NTS/AxisRaw.hpp"


//OBSOLETE_20210628 void convertAxisToNtsWidth(stream<Axis<8> > &small, AxisRaw &out)
//OBSOLETE_20210628 {
//OBSOLETE_20210628 
//OBSOLETE_20210628   out.setTData(0x0);
//OBSOLETE_20210628   out.setTLast(0);
//OBSOLETE_20210628   out.setTKeep(0);
//OBSOLETE_20210628 
//OBSOLETE_20210628   tData newd = 0x0;
//OBSOLETE_20210628   tKeep newk = 0x0;
//OBSOLETE_20210628 
//OBSOLETE_20210628   for(int i = 0; i < 8; i++)
//OBSOLETE_20210628     //for(int i = 7; i >=0 ; i--)
//OBSOLETE_20210628   {
//OBSOLETE_20210628     if(!small.empty())
//OBSOLETE_20210628     {
//OBSOLETE_20210628       Axis<8> tmp = small.read();
//OBSOLETE_20210628       //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
//OBSOLETE_20210628       //out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< (i*8);
//OBSOLETE_20210628       newd |= ((ap_uint<64>) (tmp.getTData()) )<< (i*8);
//OBSOLETE_20210628       //out.tkeep |= (ap_uint<8>) 0x01 << i;
//OBSOLETE_20210628       newk = (ap_uint<8>) 0x01 << i;
//OBSOLETE_20210628       //NO latch, because last read from small is still last read
//OBSOLETE_20210628       //out.tlast = tmp.tlast;
//OBSOLETE_20210628       out.setTLast(tmp.getTLast());
//OBSOLETE_20210628 
//OBSOLETE_20210628     } else {
//OBSOLETE_20210628       printf("tried to read empty small stream!\n");
//OBSOLETE_20210628       ////adapt tdata and tkeep to meet default shape
//OBSOLETE_20210628       //out.tdata = out.tdata >> (i+1)*8;
//OBSOLETE_20210628       //out.tkeep = out.tkeep >> (i+1);
//OBSOLETE_20210628       break;
//OBSOLETE_20210628     }
//OBSOLETE_20210628   }
//OBSOLETE_20210628 
//OBSOLETE_20210628   out.setTData(newd);
//OBSOLETE_20210628   out.setTKeep(newk);
//OBSOLETE_20210628 
//OBSOLETE_20210628 }
//OBSOLETE_20210628 
//OBSOLETE_20210628 
//OBSOLETE_20210628 void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out)
//OBSOLETE_20210628 {
//OBSOLETE_20210628 
//OBSOLETE_20210628   int positionOfTlast = 8; 
//OBSOLETE_20210628   ap_uint<8> tkeep = big.getTKeep();
//OBSOLETE_20210628   for(int i = 0; i<8; i++) //no reverse order!
//OBSOLETE_20210628   {
//OBSOLETE_20210628     tkeep = (tkeep >> 1);
//OBSOLETE_20210628     if((tkeep & 0x01) == 0)
//OBSOLETE_20210628     {
//OBSOLETE_20210628       positionOfTlast = i;
//OBSOLETE_20210628       break;
//OBSOLETE_20210628     }
//OBSOLETE_20210628   }
//OBSOLETE_20210628 
//OBSOLETE_20210628   //for(int i = 7; i >=0 ; i--)
//OBSOLETE_20210628   for(int i = 0; i < 8; i++)
//OBSOLETE_20210628   {
//OBSOLETE_20210628     //out.full? 
//OBSOLETE_20210628     Axis<8> tmp = Axis<8>(); 
//OBSOLETE_20210628     if(i == positionOfTlast)
//OBSOLETE_20210628       //if(i == 0)
//OBSOLETE_20210628     {
//OBSOLETE_20210628       //only possible position...
//OBSOLETE_20210628       tmp.setTLast(big.getTLast());
//OBSOLETE_20210628       printf("tlast set.\n");
//OBSOLETE_20210628     } else {
//OBSOLETE_20210628       tmp.setTLast(0);
//OBSOLETE_20210628     }
//OBSOLETE_20210628     tmp.setTData((ap_uint<8>) (big.getTData() >> i*8));
//OBSOLETE_20210628     tmp.setTKeep((ap_uint<1>) (big.getTKeep() >> i));
//OBSOLETE_20210628 
//OBSOLETE_20210628     if(tmp.getTKeep() == 0)
//OBSOLETE_20210628     {
//OBSOLETE_20210628       continue;
//OBSOLETE_20210628     }
//OBSOLETE_20210628 
//OBSOLETE_20210628     out.write(tmp); 
//OBSOLETE_20210628   }
//OBSOLETE_20210628 
//OBSOLETE_20210628 }


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



