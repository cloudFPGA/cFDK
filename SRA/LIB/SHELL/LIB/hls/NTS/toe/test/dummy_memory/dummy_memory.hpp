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
 * @file       : dummy_memory.hpp
 * @brief      : A class to emulate the TCP buffer memory.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _DUMMY_MEMORY_H_
#define _DUMMY_MEMORY_H_

#include "../../../../NTS/nts.hpp"
#include "../../../../MEM/mem.hpp"
//#include "../../../../NTS/toe/src/toe.hpp"
// #include "../../../../NTS/toe/src/state_table/state_table.hpp"
// #include "../../../NTS/nts_utils.hpp"
// #include "../../../NTS/SimNtsUtils.hpp"

#include <map>

/*******************************************************************************
 *
 * ENTITY - DUMMY MEMORY
 *
 *******************************************************************************/
class DummyMemory {

  private: // [FIXME - Use appropriate types]
    ap_uint<16> readAddr;   // Read Address within a read buffer (16 LSbits -->  64k bytes)
    ap_uint<16> readId;     // Address of the read buffer in DDR (16 MSbits -->  64k buffer).
    int         readLen;
    ap_uint<16> writeAddr;  // Write Address within a write buffer (16 LSbits -->  64k bytes)
    ap_uint<16> writeId;    // Address of the write buffer in DDR  (16 MSbits -->  64k buffer).

    std::map<ap_uint<16>, ap_uint<8>*>           storage;
    std::map<ap_uint<16>, ap_uint<8>*>::iterator readStorageIt;
    std::map<ap_uint<16>, ap_uint<8>*>::iterator writeStorageIt;

    std::map<ap_uint<16>, ap_uint<8>*>::iterator createBuffer(ap_uint<16> id);
    void  shuffleWord(ap_uint<64> &);
    bool *getBitMask (ap_uint< 4> keep);

  public:
    void setReadCmd (DmCmd    cmd);
    void setWriteCmd(DmCmd    cmd);
    void readChunk  (AxisApp &chunk);
    void writeChunk (AxisApp &chunk);

};

#endif

/*! \} */
