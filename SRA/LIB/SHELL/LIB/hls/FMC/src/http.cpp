/*******************************************************************************
 * Copyright 2016 -- 2020 IBM Corporation
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
 * @file       : http.cpp
 * @brief      : 
 *
 * System:     : cloudFPGA
 * Component   : Shell, FPGA Management Core (FMC)
 * Language    : Vivado HLS
 * 
 * \ingroup FMC
 * \addtogroup FMC
 * \{
 *****************************************************************************/

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <assert.h>

#include "fmc.hpp" 
#include "http.hpp"


static char* httpHeader = "HTTP/1.1 ";
static char* generalHeaderBegin = "Cache-Control: private\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/";
static char* httpNL = "\r\n";
static char* contentLengthHeader = "Content-Length: ";

static char* status200 = "200 OK";
static char* status400 = "400 Bad Request";
static char* status403 = "403 Forbidden";
static char* status404 = "404 Not Found";
static char* status422 = "422 Unprocessable Entity";
static char* status500 = "500 Internal Server Error";


int my_strlen(char *s) {
  int sum = 0;
  char c = s[0];

  while(c != '\0') {
    sum++;
    c = s[sum];
  }
  return sum;
}

int my_wordlen(char *s) {
  //word in the sense of: string to next space
  int sum = 0;
  char c = s[0];

  while(c != ' ') {
    sum++;
    c = s[sum];
  }
  return sum;
}


int writeString(char* s)
{
  //int len = my_strlen_out(s);
  //for(int i = 0; i<len; i++)
  int len = 0;
  for(int i = 0; i<OUT_BUFFER_SIZE; i++)
  {
    if(s[i] == '\0')
    {
      break;
    }
    bufferOut[bufferOutPtrWrite + i] = s[i];
    len++;
  }
  bufferOutPtrWrite += len;
  assert(bufferOutPtrWrite < OUT_BUFFER_SIZE);
  return len;
}

void strrev (char *str)
{
  unsigned char temp, len=0, i=0;

  if( str == '\0' || !(*str) )    // If str is NULL or empty, do nothing
    return;

  while(str[len] != '\0')
  {
    len++;
  }
  len=len-1;

  // Swap the characters
  while(i < len)
  {
    temp = str[i];
    str[i] = str[len];
    str[len] = temp;
    i++;
    len--;
  }
}


//write integer until 64bits
int writeUnsignedLong(unsigned long num, uint8_t base)
{
  unsigned char i=0,rem=0;
  char arr[66];

    // Handle 0 explicitly, otherwise empty string is printed for 0
    if (num == 0)
    {
      arr[i++] = '0';
    }
    // Process individual digits
    while (num != 0 && i<=64)
    {
      rem = num % base;
      arr[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0';
      num = num/base;
    }
    arr[i] = '\0';    // Append string terminator
    strrev(arr);        // Reverses the string

    //now, copy to buffer out
    //<i, because we don't want to copy \0
    for(int p = 0; p<i; p++)
    {
        bufferOut[bufferOutPtrWrite + p] = arr[p];
    }
    bufferOutPtrWrite += i;
    printf("written unsigned long %s of length %d with basis %d\n",arr,i, (int) base);

    assert(bufferOutPtrWrite < OUT_BUFFER_SIZE);
    return i;
}


void my_itoa(unsigned long num, char *arr, unsigned char base)
{
  unsigned char i=0,rem=0;

    // Handle 0 explicitly, otherwise empty string is printed for 0
    if (num == 0)
    {
      arr[i++] = '0';
      arr[i] = '\0';
    }

    // Process individual digits
    while (num != 0)
    {
      rem = num % base;
      arr[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0';
      num = num/base;
    }
    arr[i] = '\0';    // Append string terminator
    strrev(arr);        // Reverses the string
}



int my_atoi(char *str, int strlen)
{
  int res = 0; 

  for (int i = 0; i < strlen; ++i)
  {
    res = res*10 + (str[i] - '0');
  }

  return res;
}


int8_t writeHttpStatus(int status, uint16_t content_length){

  char* toWrite;

  switch (status) {
    case 200: 
              toWrite = status200; 
              break;
    case 400: 
              toWrite = status400; 
              break;
    case 403: 
              toWrite = status403; 
              break;
    case 404: 
              toWrite = status404; 
              break;
    case 422: 
              toWrite = status422; 
              break;
    default: 
              toWrite = status500; 
              break;
  }

  //Assemble Header 
  int len = writeString(httpHeader);
  len += writeString(toWrite);
  len += writeString(httpNL);
  len += writeString(generalHeaderBegin);
  len += writeString(CFDK_VERSION_STRING);
  len += writeString(httpNL);
  //use CFDK version

  //TODO: 
 /* if ( content_length > 0)
  {
    char *lengthAscii = new char[6];

    for(int i = 0; i < 6; i++)
    {
      lengthAscii[i] = 0x20;
    }

    my_itoa(content_length, lengthAscii, 10);

    len += writeString(contentLengthHeader); 
    len += writeString(lengthAscii);
    len += writeString(httpNL);
  }
  */
  len += writeString(httpNL); // to finish header 

  return len;
}


int my_strcmp(char *tmp1, volatile uint8_t tmp2[IN_BUFFER_SIZE], int max_length)
{
  int cnt = 0;
  while(*tmp1 && tmp2[cnt])
  {
    if(*tmp1== tmp2[cnt])
    {
      tmp1++;
      cnt++;
    }
    else
    {
      if(*tmp1< tmp2[cnt])
      {
        return -1;
      }
      else
      {
        return 1;
      }
    }
    if (cnt == max_length -1 )
    {
      return 0;  //equal until max_length
    }
  }
  return 0; //strings are same
}



static char *statusPath = "GET /status";
static char *configurePath = "POST /configure";
static char *putRank = "PUT /rank/";
static char *putSize = "PUT /size/";
static char *postRouting = "POST /routing";

RequestType reqType = REQ_INVALID;
uint8_t additional_http_lines_number = 0;
uint8_t indexToStringBegin[MAX_ADDITIONAL_HTTP_LINES];
char additional_http_lines_content[MAX_ADDITIONAL_HTTP_CONTENT];
uint8_t custom_api_call_number = INVALID_CUSTOM_API_CALL;

int request_len(ap_uint<16> offset, int maxLength)
{ 

  if (bufferIn[offset + 0] == 0x00)
  {//empty 
    return 0; 
  }

  int sum = 0;
  char c1 = 0, c2 = 0, c3 = 0, c4 = 0;
  c1 = (char) bufferIn[offset + 0 + 0];
  c2 = (char) bufferIn[offset + 0 + 1];
  c3 = (char) bufferIn[offset + 0 + 2];
  c4 = (char) bufferIn[offset + 0 + 3];

  while( (c1 != '\r' || c2 != '\n' || c3 != '\r' || c4 != '\n' ) && sum < maxLength)
  {
    //NOT! sum += 4;
    sum++;
    c1 = (char) bufferIn[offset + sum + 0];
    c2 = (char) bufferIn[offset + sum + 1];
    c3 = (char) bufferIn[offset + sum + 2];
    c4 = (char) bufferIn[offset + sum + 3];
  }

  
  if (sum >= maxLength)
  { //not a valid end of header found --> invalid content, since we should have already the complete request in the buffer here 
    return -1;
  } 

  return sum;

}

int8_t extract_path(bool rx_done)
{
  int stringlen = my_strlen((char*) (uint8_t*) bufferIn); 
  // strlen works, because after the request is the buffer 00ed 
  // and we see single pages --> not the complete transfer at once
  
  printf("stringlen: %d\n",(int) stringlen);

  if (stringlen == 0)
  {
    return 0;
  }
  

  int requestLen = request_len(0, stringlen);
  printf("requestLen: %d\n",(int) requestLen);


  //from here it looks like a valid header 

  reqType = REQ_INVALID; //reset
  custom_api_call_number = INVALID_CUSTOM_API_CALL;

  if(requestLen <= 0)
  {//not a valid header 
    if (stringlen == PAYLOAD_BYTES_PER_PAGE || !rx_done)
    {//not yet complete
      return -1;
    }
    return -2;
  }

 if (my_strcmp(statusPath, bufferIn, my_strlen(statusPath)) == 0 )
  {
    reqType = GET_STATUS; 
    return 1;
  } else if (my_strcmp(configurePath, bufferIn, my_strlen(configurePath)) == 0 )
  { 
    bufferInPtrNextRead = requestLen + 4;
    reqType = POST_CONFIG; 
    return 2;
  } else if(my_strcmp(putRank, bufferIn, my_strlen(putRank)) == 0 )
  {
    reqType = PUT_RANK;
    char* intStart = (char*) &bufferIn[my_strlen(putRank)];
    int intLen = my_wordlen(intStart);

    ap_uint<32> newRank = (unsigned int) my_atoi(intStart, intLen);
    if(newRank >= MAX_CLUSTER_SIZE)
    {//invalid 
      return -2;
    }
    setRank(newRank);
    return 3;
  } else if(my_strcmp(putSize, bufferIn, my_strlen(putSize)) == 0 )
  {
    reqType = PUT_SIZE;
    char* intStart = (char*) &bufferIn[my_strlen(putSize)];
    int intLen = my_wordlen(intStart);

    ap_uint<32> newSize = (unsigned int) my_atoi(intStart, intLen);
    if(newSize >= MAX_CLUSTER_SIZE)
    {//invalid 
      return -2;
    }
    setSize(newSize);
    return 4;
  } else if(my_strcmp(postRouting, bufferIn, my_strlen(postRouting)) == 0 )
  {
    reqType = POST_ROUTING;
    bufferInPtrNextRead = requestLen + 4;

    //TODO 
    return 5;
  } else { 
    //check custom API
    for(uint8_t i = 0; i< additional_http_lines_number; i++)
    {
      char* string_start = &additional_http_lines_content[indexToStringBegin[i]];
      if(my_strcmp(string_start, bufferIn, my_strlen(string_start)) == 0)
      {
        reqType = CUSTOM_API;
        bufferInPtrNextRead = requestLen + 4; //for the \r\n
        custom_api_call_number = i;
        return 6;
      }
    }
    
    //Invalid / Not Found
    return -3;
  } 
  //return -2;

}


void parseHttpInput(bool transferErr, ap_uint<1> wasAbort, bool invalidPayload, bool rx_done)
{

 switch (httpState) {
    case HTTP_IDLE: //both the same
    case HTTP_PARSE_HEADER: 
             //search for HTTP 
             switch (extract_path(rx_done)) {
              case -3: //404
                 httpState = HTTP_INVALID_REQUEST;
                //emptyOutBuffer(); ensured by global state machine
                bufferOutContentLength = writeHttpStatus(404,0);
                   break; 
               case -2: //invalid content 
                 httpState = HTTP_INVALID_REQUEST;
                //emptyOutBuffer(); ensured by global state machine
                bufferOutContentLength = writeHttpStatus(400,0);
                   break; 
               case -1: //not yet complete 
                 httpState = HTTP_PARSE_HEADER; 
                    break;
                case 0: //not vaild until now
                    break;
                case 1: //get status 
                    httpState = HTTP_SEND_RESPONSE;
                    break;
                case 2: //post config 
                    httpState = HTTP_HEADER_PARSED;
                    break;
                case 3: //put rank 
                    httpState = HTTP_SEND_RESPONSE; 
                    break; 
                case 4: //put size 
                    httpState = HTTP_SEND_RESPONSE;
                    break;
                case 5: //post routing
                    httpState = HTTP_HEADER_PARSED;
                    break;
                case 6: //custom MCC API
                    httpState = HTTP_HEADER_PARSED;
                    break;
             }
               break;
    case HTTP_HEADER_PARSED: //this state is valid for one core-cycle: after that the current payload should start at 0
               httpState = HTTP_READ_PAYLOAD;
               //no break 
    case HTTP_READ_PAYLOAD:
               break; 
    case HTTP_REQUEST_COMPLETE: 
             //  break; 
    case HTTP_SEND_RESPONSE:
                //emptyOutBuffer(); ensured by global state machine
               if(wasAbort == 1) //abort always also triggers transferErr --> so check this first
               {
                 bufferOutContentLength = writeHttpStatus(500,0);
               } else if (transferErr == true || invalidPayload == true)
               {
                 bufferOutContentLength = writeHttpStatus(422,0);
               } else if(reqType == GET_STATUS)
               { 
                 //combine status 
                 //length??
                 bufferOutContentLength = writeHttpStatus(200,0);
                 //write status 
                 uint32_t contentLen = writeDisplaysToOutBuffer();
                 writeString(httpNL); //to finish body 
                 if (contentLen > 0)
                 {
                   bufferOutContentLength += contentLen;
                 }
               } else if(reqType == POST_CONFIG)
               { 
                bufferOutContentLength = writeHttpStatus(200,0);
                bufferOutContentLength += writeString("Partial reconfiguration finished successfully!\r\n\r\n");
                 //write success message 
               } else { 
                 //PUT_RANK, PUT_SIZE, POST_ROUTING
                bufferOutContentLength = writeHttpStatus(200,0);
               }
               httpState = HTTP_DONE;
               break;
    default:
               break;
  }

  printf("parseHttpInput returns with state %d\n",httpState);
  //printf("RequestType after parseHttpInput %d\n",reqType);

}



/*! \} */


