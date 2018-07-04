#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


typedef enum { HTTP_IDLE = 0, 
						HTTP_PARSE_HEADER = 1, HTTP_HEADER_PARSED = 2, 
						HTTP_READ_PAYLPAD = 3, HTTP_REQUEST_COMPLETE = 4,
						HTTP_INVALID_REQUEST = 5 } HttpState;


int8_t writeHttpStatus(int status, uint16_t content_length);


#endif
