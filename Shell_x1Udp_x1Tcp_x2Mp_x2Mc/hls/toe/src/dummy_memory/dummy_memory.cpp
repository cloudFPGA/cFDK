/*****************************************************************************
 * @file       : dummy_memory.cpp
 * @brief      : Buffer memory emulator for the test bench of the TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "dummy_memory.hpp"
#include <iostream>

// Set the private data elements for a Read Command
void DummyMemory::setReadCmd(DmCmd cmd)
{
    this->readAddr = cmd.saddr(15,  0); // Start address
    this->readId   = cmd.saddr(31, 16); // Buffer address

    uint16_t tempLen = (uint16_t) cmd.bbt(15, 0); // Byte to Transfer
    this->readLen    = (int) tempLen;
}

// Set the private data elements for a Write Command
void DummyMemory::setWriteCmd(DmCmd cmd)
{
    this->writeAddr = cmd.saddr(15,  0); // Start address
    this->writeId   = cmd.saddr(31, 16); // Buffer address
}

// Read an AXI word from the memory
void DummyMemory::readWord(AxiWord &word)
{
    readStorageIt = storage.find(readId);
    if (readStorageIt == storage.end()) {
        readStorageIt = createBuffer(readId);
        // check it?
    }
    int i = 0;
    word.tkeep = 0;
    while (readLen > 0 && i < 8) {
        word.tdata((i*8)+7, i*8) = (readStorageIt->second)[readAddr];
        word.tkeep = (word.tkeep << 1);
        word.tkeep++;
        readLen--;
        readAddr++;
        i++;
    }
    if (readLen == 0) {
        word.tlast = 1;
    }
    else {
        word.tlast = 0;
    }
}


// Write an AXI word into the memory
void DummyMemory::writeWord(AxiWord &word)
{
    writeStorageIt = storage.find(writeId);

    if (writeStorageIt == storage.end()) {
        writeStorageIt = createBuffer(writeId);
        // check it?
    }
    // shuffleWord(word.data);
    for (int i = 0; i < 8; i++) {
        if (word.tkeep[i]) {
            (writeStorageIt->second)[writeAddr] = word.tdata((i*8)+7, i*8);
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



/*bool* dummyMemory::getBitMask(ap_uint<4> keep)
{
    switch (keep)
    {
    case 0x01:
        return {true, false, false, false, false, false, false, false};
        break;
    case 0x03:
        return {true, true, false, false, false, false, false, false};
        break;
    case 0x07:
        return {true, true, true, false, false, false, false, false};
        break;
    case 0x0f:
        return {true, true, true, true, false, false, false, false};
        break;
    case 0x1f:
        return {true, true, true, true, true, false, false, false};
        break;
    case 0x3f:
        return {true, true, true, true, true, true, false, false};
        break;
    case 0x7f:
        return {true, true, true, true, true, true, true, false};
        break;
    case 0xff:
        return {true, true, true, true, true, true, true, true};
        break;
    }
    return 0;
}*/
