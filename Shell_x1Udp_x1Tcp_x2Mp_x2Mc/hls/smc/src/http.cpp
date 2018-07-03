
#include <stdint.h>
//#include <math.h>
//#include "hls_math.h"
#include "ap_int.h"
#include "ap_utils.h"

#include "http.hpp"
#include "smc.hpp" 

HttpState httpState = HTTP_IDLE; 

static char* httpOK = "HTTP/1.1 200 OK\r\nCache-Control: private\r\nContent-Length: 108\r\nContent-Type: text/plain; charset=utf-8";
int httpOKlen = 108;


int8_t writeHttpOK(ap_uint<16> startAddress){

	for(int i = 0; i<httpOKlen; i++)
	{
		bufferOut[startAddress + i] = httpOK[i];
	}

	//return ceil(((float) httpOKlen)/128);
	return 1;
}



