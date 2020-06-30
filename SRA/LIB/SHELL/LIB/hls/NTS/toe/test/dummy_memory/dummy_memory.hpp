/*****************************************************************************
 * @file       : dummy_memory.hpp
 * @brief      : A class to emulate the TCP buffer memory.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef DUMMY_MEMORY_H_
#define DUMMY_MEMORY_H_

#include "../../src/toe.hpp"

#include <map>

class DummyMemory {

  private:
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
    void readWord   (AxiWord &word);
    void writeWord  (AxiWord &word);

};

#endif
