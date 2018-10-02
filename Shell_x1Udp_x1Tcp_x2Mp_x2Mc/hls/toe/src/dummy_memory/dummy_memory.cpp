#include "dummy_memory.hpp"
#include <iostream>


/*dummyMemory::dummyMemory(const dummyMemory& another)
{

}*/

void dummyMemory::setReadCmd(mmCmd cmd)
{
//  readAddr = cmd.saddr(7, 0);
    readAddr = cmd.saddr(15, 0);
    readId = cmd.saddr(31, 16);
    uint16_t tempLen = (uint16_t) cmd.bbt(15, 0);
    readLen = (int) tempLen;
    //std::cout << readLen << std::endl;
}

void dummyMemory::setWriteCmd(mmCmd cmd)
{
//  writeAddr = cmd.saddr(7, 0);
    writeAddr = cmd.saddr(15, 0);
    writeId = cmd.saddr(31, 16);
}


void dummyMemory::readWord(axiWord& word)
{
    readStorageIt = storage.find(readId);
    if (readStorageIt == storage.end())
    {
        readStorageIt = createBuffer(readId);
        // check it?
    }
    int i = 0;
    word.keep = 0;
    while (readLen > 0 && i < 8)
    {
        word.data((i*8)+7, i*8) = (readStorageIt->second)[readAddr];
        word.keep = (word.keep << 1);
        word.keep++;
        readLen--;
        readAddr++;
        i++;
    }
    if (readLen == 0)
    {
        word.last = 1;
    }
    else
    {
        word.last = 0;
    }
}

void dummyMemory::writeWord(axiWord& word)
{
    writeStorageIt = storage.find(writeId);
    if (writeStorageIt == storage.end())
    {
        writeStorageIt = createBuffer(writeId);
        // check it?
    }
    //shuffleWord(word.data);
    for (int i = 0; i < 8; i++)
    {
        if (word.keep[i])
        {
            (writeStorageIt->second)[writeAddr] = word.data((i*8)+7, i*8);
            writeAddr++;
        }
        else
        {
            break;
        }
    }
}

std::map<ap_uint<16>, ap_uint<8>*>::iterator dummyMemory::createBuffer(ap_uint<16> id)
{
    ap_uint<8>* array = new ap_uint<8>[65536]; // [255] default
    std::pair<std::map<ap_uint<16>, ap_uint<8>*>::iterator, bool> ret;
    ret = storage.insert(std::make_pair(id, array));
    if (ret.second)
    {
        return ret.first;
    }
    return storage.end();
}

void dummyMemory::shuffleWord(ap_uint<64>& word)
{
    ap_uint<64> temp;
    temp(7, 0) = word(63, 56);
    temp(15, 8) = word(55, 48);
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
