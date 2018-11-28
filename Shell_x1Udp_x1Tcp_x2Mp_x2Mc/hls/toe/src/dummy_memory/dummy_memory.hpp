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

#ifndef MEM_H_
#define MEM_H_

#include "../toe.hpp"

#include <map>

class DummyMemory {

public:
    void setReadCmd(mmCmd cmd);
    void setWriteCmd(mmCmd cmd);
    void readWord(AxiWord &word);
    void writeWord(AxiWord &word);

private:
    std::map<ap_uint<16>, ap_uint<8>*>::iterator createBuffer(ap_uint<16> id);
    void shuffleWord(ap_uint<64> &);
    bool* getBitMask(ap_uint<4> keep);
    ap_uint<16> readAddr; //<8>
    ap_uint<16> readId;
    int readLen;
    ap_uint<16> writeAddr; //<8>
    ap_uint<16> writeId;
    //ap_uint<16> writeLen;
    std::map<ap_uint<16>, ap_uint<8>*> storage;
    std::map<ap_uint<16>, ap_uint<8>*>::iterator readStorageIt;
    std::map<ap_uint<16>, ap_uint<8>*>::iterator writeStorageIt;
};

#endif
