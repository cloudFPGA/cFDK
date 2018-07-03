#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


typedef enum HttpState { HTTP_IDLE = 0, 
						HTTP_PARSE_HEADER = 1, HTTP_PARSE_PAYLOAD = 2 } HttpState;

extern HttpState httpStateBla; 

int8_t writeHttpOK(ap_uint<16> startAddress);


#endif
