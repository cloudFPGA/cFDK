/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

/*****************************************************************************
 * @file       : http.hpp
 * @brief      : The HTTP parsing functions for the FMC.
 *
 * System:     : cloudFPGA
 * Component   : Shell, FPGA Management Core (FMC)
 * Language    : Vivado HLS
 * 
 * \ingroup FMC
 * \addtogroup FMC
 * \{
 *****************************************************************************/

#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>
#include "ap_int.h" 


//#define COSIM
//#ifdef COSIM 
#define REQ_INVALID   0x01
#define POST_CONFIG   0x02
#define GET_STATUS    0x40  //MUST NOT BE EUQIVALENT to OPRV_SKIPPED!!
#define PUT_RANK      0x08
#define PUT_SIZE      0x10
#define POST_ROUTING  0x20
#define CUSTOM_API    0x80
#define RequestType uint8_t
//#else
//typedef enum {REQ_INVALID = 0, POST_CONFIG, GET_STATUS, PUT_RANK, PUT_SIZE, POST_ROUTING} RequestType;
//#endif

extern RequestType reqType; 


int writeString(char* s);
int8_t writeHttpStatus(int status, uint16_t content_length);
int request_len(ap_uint<16> offset, int maxLength);

int my_wordlen(char *s);
int my_atoi(char *str, int strlen);
//void my_itoa(unsigned long num, char *arr, unsigned char base);
int writeUnsignedLong(unsigned long num, uint8_t base);


void parseHttpInput(bool transferErr, ap_uint<1> wasAbort, bool invalidPayload, bool rx_done);


#endif



/*! \} */
