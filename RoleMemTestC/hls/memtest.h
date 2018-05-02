
//#include <stdint.h>
#include "ap_int.h"
typedef ap_uint<64> TYPE;
typedef ap_uint<32> ADDRTYPE;

typedef ap_uint<16> u_int16_t;

#include "hls_stream.h" 

void memtest_app(u_int16_t cmdRx, u_int16_t cmdTx,
		        hls::stream<TYPE> &memRxData, hls::stream<TYPE> &memTxData,
		        hls::stream<ADDRTYPE> &memRxAddr, hls::stream<ADDRTYPE> &memTxAddr);


