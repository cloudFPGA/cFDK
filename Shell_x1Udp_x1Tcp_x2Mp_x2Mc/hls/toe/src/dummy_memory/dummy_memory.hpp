#ifndef MEM_H_
#define MEM_H_

#include "../toe.hpp"
#include <map>

class dummyMemory {
public:
    void setReadCmd(mmCmd cmd);
    void setWriteCmd(mmCmd cmd);
    void readWord(axiWord& word);
    void writeWord(axiWord& word);
private:
    std::map<ap_uint<16>, ap_uint<8>*>::iterator createBuffer(ap_uint<16> id);
    void shuffleWord(ap_uint<64>& );
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
