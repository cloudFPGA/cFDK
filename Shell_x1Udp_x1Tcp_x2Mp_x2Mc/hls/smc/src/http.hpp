#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


//#define COSIM
#ifdef COSIM 

#define REQ_INVALID 0
#define POST_CONFIG 1
#define GET_STATUS 2
#define PUT_RANK 3
#define PUT_SIZE 4 
#define POST_ROUTING 5
#define RequestType uint8_t

#else
typedef enum {REQ_INVALID = 0, POST_CONFIG, GET_STATUS, PUT_RANK, PUT_SIZE, POST_ROUTING} RequestType;
#endif

int writeString(char* s);
int8_t writeHttpStatus(int status, uint16_t content_length);
int request_len(ap_uint<16> offset, int maxLength);

void parseHttpInput(ap_uint<1> transferErr, ap_uint<1> wasAbort);


#endif
