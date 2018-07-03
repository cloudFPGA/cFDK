#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


typedef enum { HTTP_IDLE = 0, 
						HTTP_PARSE_HEADER = 1, HTTP_PARSE_PAYLOAD = 2 } HttpState;

//extern HttpState httpState; 

int8_t writeHttpStatus(int status, ap_uint<16> startAddress);


#endif
