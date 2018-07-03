
#include <stdint.h>
//#include "hls_math.h"
#include "ap_int.h"
#include "ap_utils.h"

#include "http.hpp"
#include "smc.hpp" 

extern HttpState httpState; 

static char* http200 = "HTTP/1.1 200 OK\r\nCache-Control: private\r\nContent-Length: 6\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n200 OK";

static char* http400 = "HTTP/1.1 400 Bad Request\r\nCache-Control: private\r\nContent-Length: 15\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n400 Bad Request";

static char* http403 = "HTTP/1.1 403 Forbidden\r\nCache-Control: private\r\nContent-Length: 13\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n403 Forbidden";

static char* http404 = "HTTP/1.1 404 Not Found\r\nCache-Control: private\r\nContent-Length: 13\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n404 Not Found";

static char* http500 = "HTTP/1.1 500 Internal Server Error\r\nCache-Control: private\r\nContent-Length: 25\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n500 Internal Server Error";

int my_strlen(char * s) {
	int i = 0, sum = 0;
	char c = s[0];

	while(c != '\0') {
		sum++;
		c = s[++i];
	}
	return sum;
}


int8_t writeHttpStatus(int status, ap_uint<16> startAddress){

	char* toWrite;

	switch (status) {
		case 200: 
							toWrite = http200; 
							break;
		case 400: 
							toWrite = http400; 
							break;
		case 403: 
							toWrite = http403; 
							break;
		case 404: 
							toWrite = http404; 
							break;
		default: 
							toWrite = http500; 
							break;
	}

	int len = my_strlen(toWrite);

	for(int i = 0; i<len; i++)
	{
		bufferOut[startAddress + i] = toWrite[i];
	}

	int8_t pageCnt = len/128; 

	if(len % 128 != 0)
	{
		pageCnt++;
	}

	return pageCnt;
}



