/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
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

/************************************************
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : dummy_memory.cpp
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

#include "dummy_memory.hpp"

// Set the private data elements for a Read Command
void DummyMemory::setReadCmd(DmCmd cmd) {
    this->readAddr = cmd.saddr(15,  0); // Start address
    this->readId   = cmd.saddr(31, 16); // Buffer address
    uint16_t tempLen = (uint16_t) cmd.btt(15, 0); // Byte to Transfer
    this->readLen    = (int) tempLen;
}

// Set the private data elements for a Write Command
void DummyMemory::setWriteCmd(DmCmd cmd) {
    this->writeAddr = cmd.saddr(15,  0); // Start address
    this->writeId   = cmd.saddr(31, 16); // Buffer address
}

// Read a data chunk from the memory
void DummyMemory::readChunk(AxisApp &chunk) {
    readStorageIt = storage.find(readId);
    if (readStorageIt == storage.end()) {
        readStorageIt = createBuffer(readId);
        // check it?
    }
    int i = 0;
    chunk.setLE_TKeep(0);
    while (this->readLen > 0 and i < 8) {
        chunk.setLE_TData((readStorageIt->second)[readAddr], (i*8)+7, i*8);
        chunk.setLE_TKeep(chunk.getLE_TKeep() | (0x01 << i));
        readLen--;
        readAddr++;
        i++;
    }
    if (this->readLen == 0) {
        chunk.setTLast(TLAST);
    }
    else {
        chunk.setTLast(0);
    }
}

// Write a data chunk into the memory
void DummyMemory::writeChunk(AxisApp &chunk) {
    writeStorageIt = storage.find(writeId);
    if (writeStorageIt == storage.end()) {
        writeStorageIt = createBuffer(writeId);
        // check it?
    }
    // shuffleWord(word.data);
    for (int i = 0; i < 8; i++) {
        if (chunk.getLE_TKeep()[i]) {
            (writeStorageIt->second)[writeAddr] = chunk.getLE_TData((i*8)+7, i*8);
            writeAddr++;
        }
        else {
            break;
        }
    }
}


std::map<ap_uint<16>, ap_uint<8>*>::iterator DummyMemory::createBuffer(ap_uint<16> id)
{
    ap_uint<8>* array = new ap_uint<8>[65536]; // [255] default
    std::pair<std::map<ap_uint<16>, ap_uint<8>*>::iterator, bool> ret;

    ret = storage.insert(std::make_pair(id, array));
    if (ret.second) {
        return ret.first;
    }
    return storage.end();
}

void DummyMemory::shuffleWord(ap_uint<64>& word)
{
    ap_uint<64> temp;

    temp( 7,  0) = word(63, 56);
    temp(15,  8) = word(55, 48);
    temp(23, 16) = word(47, 40);
    temp(31, 24) = word(39, 32);

    temp(39, 32) = word(31, 24);
    temp(47, 40) = word(23, 16);
    temp(55, 48) = word(15, 8);
    temp(63, 56) = word(7, 0);
    word = temp;
}

/*! \} */
