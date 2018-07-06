#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


typedef enum { HTTP_IDLE = 0, 
            HTTP_PARSE_HEADER = 1, HTTP_HEADER_PARSED = 2, 
            HTTP_READ_PAYLOAD = 3, HTTP_REQUEST_COMPLETE = 4,
            HTTP_SEND_RESPONSE = 5, HTTP_INVALID_REQUEST =  6,
            HTTP_DONE = 7 } HttpState;


int writeString(char* s);
int8_t writeHttpStatus(int status, uint16_t content_length);

void parseHttpInput(ap_uint<1> transferErr, ap_uint<1> wasAbort);


#endif
