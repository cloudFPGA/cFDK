/*
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
 */

/*******************************************************************************
 * @file       : AxisRaw.hpp
 * @brief      : A generic class used by the Network-Transport-Stack (NTS) to
 *               to transfer a chunk of data  over an AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details : This class is similar to the original 'AxiWord' class and is
 *  expected to slowly replace it over time. The major difference lies in the
 *  members 'tdata', 'tkeep' and 'tlast' of this class being protected.
 *
 *  @warning: The order of the bytes of the 'tdata' member is aligned with the
 *   format generated by the 10GbE MAC of Xilinx which organizes its two 64-bit
 *   Rx and Tx interfaces into 8 lanes (see PG157). The result of this division
 *   into lanes happens to store the 'tdata' member into LITTLE-ENDIAN (LE)
 *   order instead of the initial BID-ENDIAN (BE) order used to transmit bytes
 *   over the physical media.
 *   As an example, consider the 16-bit field "EtherType" of the ETH frame which
 *   value is '0x0800' when the Ethernet frame contains an IPv4 packet. This
 *   field will be transmitted on the media in big-endian order .i.e, a '0x08'
 *   byte followed by a '0x00' byte. However, this field will end up being
 *   ordered in little-endian mode (.i.e, 0x0080) by the AXI4-Stream interface
 *   of the 10GbE MAC.
 *
 *  Therefore, the mapping of a network stream of bytes onto a 64-bits AXIS_RAW
 *  interface in LITTLE-ENDIAN is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     B[07]     |     B[06]     |     B[05]     |     B[04]     |     B[03]     |     B[02]     |     B[01]     |     B[00]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |     B[15]     |     B[14]     |     B[13]     |     B[12]     |     B[11]     |     B[10]     |     B[09]     |     B[08]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @info: What's this Little-Endian(LE) vs Big-Endian(BE) anyhow.
 *  FYI - The original source from Xilinx (available at:
 *   https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip) is
 *   entirely coded with respect to the above mapping of the media network
 *   stream  over a 64-bits interface in little-endian oder. This makes the
 *   initial code particularly difficult to read, maintain and test. Therefore,
 *   this class implements a few methods to access the AXIS data streams as if
 *   they were encoded in the expected big-endian order.
 *     All of this headache could have been avoided if the original code had
 *   swapped the 64-bits bus to/from the Ethernet MAC. This enhancement will be
 *   implemented if/when we will move to a higher data rate (.e.g 25GE or 100GE).
 *   In the mean time, here is a helper to help visualize the LE<->BE relation:
 *
 *                    Lower-Half                     Higher-Half
 *         +---------------+---------------+---------------+---------------+
 *  LITTLE |63   (L-L)   48|47   (L-H)   32|31   (H-L)   16|15   (H-H)    0|
 *         +---------------+---------------+---------------+---------------+
 *               /|\             /|\             /|\             /|\
 *                +---------------|---------------|---------------|-+
 *                                +---------------|-+             | |
 *                                  +-------------+ |             | |
 *                  +---------------|---------------|-------------+ |
 *                 \|/             \|/             \|/             \|/
 *         +---------------+---------------+---------------+---------------+
 *     BIG |63   (H-H)   48|47   (H-L)   32|31   (L-H)   16|15   (L-L)    0|
 *         +---------------+---------------+---------------+---------------+
 *                   Higher-Half                      Lower-Half
 *
 *
 *          63                           32 31                            0
 *
 * [TODO]
 *
 *         +---------------+---------------+---------------+---------------+
 *     BIG |63        Higher-Half        32|31       Lower-Half           0|
 *         +---------------+---------------+---------------+---------------+
 *               *          63                           32 31                            0
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _AXIS_RAW_H_
#define _AXIS_RAW_H_

#include "ap_int.h"

/*******************************************************************************
 * GENERIC AXI4 STREAMING INTERFACES
 *******************************************************************************/
template<int D>
   class Axis {
     private:
       ap_uint<D>       tdata;
       ap_uint<(D+7)/8> tkeep;
       ap_uint<1>       tlast;
       Axis() {}
       Axis(ap_uint<D> single_data) :
           tdata((ap_uint<D>)single_data), tkeep(~(((ap_uint<D>) single_data) & 0)), tlast(1) {}
   };

/***********************************************
 * AXIS RAW - DEFINITIONS
 ***********************************************/
#define AXIS_ROW_WIDTH_AT_10GE  64
#define ARW                     AXIS_ROW_WIDTH_AT_10GE

#define TLAST       1

/***********************************************
 * AXIS_RAW - TYPE FIELDS DEFINITION
 *   FYI - 'LE' stands for Little-Endian order.
 ***********************************************/
typedef ap_uint<ARW>    LE_tData;
typedef ap_uint<ARW/2>  LE_tDataHalf;
typedef ap_uint<ARW/8>  LE_tKeep;
typedef ap_uint<1>      LE_tLast; // Added for consistency
typedef ap_uint<ARW>    tData;
typedef ap_uint<ARW/2>  tDataHalf;
typedef ap_uint<ARW/8>  tKeep;
typedef ap_uint<ARW/16> tKeepHalf;
typedef ap_uint<1>      tLast;

/*******************************************************************************
 * AXIS_RAW - RAW AXIS-4 STREAMING INTERFACE
 *  An AxisRaw is logically divided into 'ARW/8' bytes. The validity of a given
 *  byte is qualified by the 'tkeep' field, while the assertion of the 'tlast'
 *  bit indicates the end of a stream.
 *
 * @Warning: Members of this class are kept public for compatibility issues with
 *  the legacy code.
 *
 * @TODO-This class is currently hard-coded with AWR=64 --> Make it generic
 *******************************************************************************/
class AxisRaw {

  protected:
    LE_tData    tdata;
    LE_tKeep    tkeep;
    LE_tLast    tlast;

  public:
    AxisRaw()       {}
    AxisRaw(LE_tData tdata, LE_tKeep tkeep, LE_tLast tlast) :
            tdata(tdata), tkeep(tkeep), tlast(tlast) {}

    /*** OBSOLETE ***
    struct axiWord
    {
    	ap_uint<64>		data;
    	ap_uint<8>		keep;
    	ap_uint<1>		last;
    	axiWord() {}
    	axiWord(ap_uint<64>	 data, ap_uint<8> keep, ap_uint<1> last)
    				: data(data), keep(keep), last(last) {}
    };
    *****************/

    // Get the length of this chunk (in bytes)
    int getLen() {
        return keepToLen();
    }
    // Zero the bytes which have their tkeep-bit cleared
    void clearUnusedBytes() {
        for (int i=0, hi=ARW/8-1, lo=0; i<ARW/8; i++) {  // ARW/8 = noBytes
            #pragma HLS UNROLL
            if (tkeep[i] == 0) {
                tdata.range(hi+8*i, lo+8*i) = 0x00;
            }
        }
    }

    /******************************************************
     * BIG-ENDIAN SETTERS AND GETTERS
     *****************************************************/
    // Set the 'tdata' field with a 'data' encoded in Big-Endian order
    void setTData(tData data) {
        tdata.range(63,  0) = byteSwap64(data);
    }
    // Return the 'tdata' field in Big-Endian order
    tData getTData() {
         return byteSwap64(tdata.range(63, 0));
    }
    // Set the 'tkeep' field with respect to a 'data' field encoded in Big-Endian order
    void setTKeep(tKeep keep) {
        tkeep = bitSwap8(keep);
    }
    // Get the 'tkeep' field with respect to a 'data' field encoded in Big-Endian order
    tKeep getTKeep() {
        return bitSwap8(tkeep);
    }
    // Set the tlast field
    void setTLast(tLast last) {
        tlast = last;
        if (last) {
            // Always zero the bytes which have their tkeep-bit cleared.
            // This simplifies the computation of the various checksums and
            // unifies the overall AxisRaw processing and verification.
            this->clearUnusedBytes();
        }
    }
    // Get the tlast bit
    tLast getTLast() {
        return tlast;
    }

    /******************************************************
     * LITTLE-ENDIAN SETTERS AND GETTERS
     *****************************************************/
    // Set the 'tdata' field with a 'data' encoded in Little-Endian order
    void setLE_TData(LE_tData data, int hi=ARW-1, int lo=0) {
        tdata.range(hi, lo) = data;
    }
    // Return the 'tdata' field in Little-Endian order
    LE_tData getLE_TData(int hi=ARW-1, int lo=0) {
        return tdata.range(hi, lo);
    }
    // Set the 'tdata' field with the upper-half part of a 'data' encoded in Little-Endian order (.i.e, data(31,0))
    void setLE_TDataHi(LE_tData data) {
        tdata.range(31, 0) = data.range(31, 0);  // [TODO]
    }
    // Get the 'tdata' field in Little-Endian order and return its upper-half part (.i.e data(31,0))
    LE_tDataHalf getLE_TDataHi() {
        return getLE_TData().range(31, 0);  // [TODO]
    }
    // Set the 'tdata' field with the lower-half part of a 'data' encoded in Little-Endian order (.i.e, data(63,32))
    void setLE_TDataLo(LE_tData data) {
        tdata.range(63, 32) = data.range(63, 32);  // [TODO]
    }
    // Get the 'tdata' field in Little-Endian order and return its lower-half part (.i.e, data(63,32)
    LE_tDataHalf getLE_TDataLo() {
        return getLE_TData().range(63, 32);  // [TODO]
    }
    // Set the 'tkeep' field with respect to the 'tdata' field encoded in Little-Endian order
    void setLE_TKeep(LE_tKeep keep, int hi=ARW/8-1, int lo=0) {
        tkeep.range(hi, lo) = keep;
    }
    // Get the 'tkeep' field with respect to the 'tdata' field encoded in Little-Endian order
    LE_tKeep getLE_TKeep(int hi=ARW/8-1, int lo=0) {
        return tkeep.range(hi, lo);
    }
    // Set the tlast field
    void setLE_TLast(LE_tLast last) {
        tlast = last;
    }
    // Get the tlast bit
    LE_tLast getLE_TLast() {
        return tlast;
    }

    // Assess the consistency of 'tkeep' and 'tlast'
    // [FIXME - Shall we move this into SimNtsUtil.cpp]
    bool isValid() {
        if (((this->tlast == 0) and (this->tkeep != 0xFF)) or
            ((this->tlast == 1) and (this->keepToLen() == 0))) {
            return false;
        }
        return true;
    }
  protected:
    // Return the number of valid bytes
    int keepToLen() {
        switch(this->tkeep){
            case 0x01: return 1; break;
            case 0x03: return 2; break;
            case 0x07: return 3; break;
            case 0x0F: return 4; break;
            case 0x1F: return 5; break;
            case 0x3F: return 6; break;
            case 0x7F: return 7; break;
            case 0xFF: return 8; break;
        }
        return 0;
    }

  private:
    // Reverse the bits within a nibble.
    ap_uint<4> bitSwap4(ap_uint<4> inputVector) {
        return (inputVector.range(0,3));
    }
    // Reverse the bits within a byte.
    ap_uint<8> bitSwap8(ap_uint<8> inputVector) {
        return (inputVector.range(0,7));
    }
    // Swap the two bytes of a word (.i.e, 16 bits).
    ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
        return (inputVector.range(7,0), inputVector(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits).
    ap_uint<32> byteSwap32(ap_uint<32> inpDWord) {
        return (inpDWord.range( 7, 0), inpDWord.range(15,  8),
                inpDWord.range(23,16), inpDWord.range(31, 24));
    }
    // Swap the eight bytes of a quad-word (.i.e, 64 bits).
    ap_uint<64> byteSwap64(ap_uint<64> inpQWord) {
        return (inpQWord.range( 7, 0), inpQWord(15,  8),
                inpQWord.range(23,16), inpQWord(31, 24),
                inpQWord.range(39,32), inpQWord(47, 40),
                inpQWord.range(55,48), inpQWord(63, 56));
    }

};

#endif

/*! \} */
