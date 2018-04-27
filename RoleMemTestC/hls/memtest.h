
#include "ap_int.h" 
typedef ap_uint<64> TYPE;
typedef ap_uint<32> ADDRTYPE;

#include "hls_stream.h" 

void memtest_app(hls:stream<TYPE> &cmdRx, hls:stream<TYPE> &cmdTx, 
									hls:stream<TYPE> &memRxData, hls:stream<TYPE> &memTxData
									hls:stream<ADDRTYPE> &memRxAddr, hls:stream<ADDRTYPE> &memTxAddr);


