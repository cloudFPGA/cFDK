
#include <stdint.h>
//#include "hls_math.h"
#include "ap_int.h"
#include "ap_utils.h"

#include "http.hpp"
#include "smc.hpp" 

extern HttpState httpState; 
//extern ap_uint<16> currentPayloadStart;

//static char* status500 = "HTTP/1.1 500 Internal Server Error\r\nCache-Control: private\r\nContent-Length: 25\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n\r\n500 Internal Server Error";
static char* httpHeader = "HTTP/1.1 ";
static char* generalHeader = "Cache-Control: private\r\nContent-Type: text/plain; charset=utf-8\r\nServer: cloudFPGA/0.2\r\n";
static char* httpNL = "\r\n";
static char* contentLengthHeader = "Content-Length: ";

static char* status200 = "200 OK";
static char* status400 = "400 Bad Request";
static char* status403 = "403 Forbidden";
static char* status404 = "404 Not Found";
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


int writeString(char* s)
{
  int len = my_strlen(s);
  for(int i = 0; i<len; i++)
  {
    bufferOut[currentBufferOutPtr + i] = s[i];
  }
  currentBufferOutPtr += len;

  return len;
}


// from http://www.techackers.com/integer-ascii-itoa-function-c/

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
    default: 
              toWrite = status500; 
              break;
  }

  //Assemble Header 
  int len = writeString(httpHeader);
  len += writeString(toWrite);
  len += writeString(httpNL);
  len += writeString(generalHeader);

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

  int8_t pageCnt = len/BYTES_PER_PAGE; 

  if(len % BYTES_PER_PAGE > 0)
  {
    pageCnt++;
  }

  return pageCnt;
  //return 1;
}


// from http://simplestcodings.blogspot.com/2010/08/custom-string-compare-function-in-c.html, adapted
int my_strcmp(char *temp1, ap_uint<8> temp2[BUFFER_SIZE], int max_length)
{
  int cnt = 0;
  while(*temp1 && temp2[cnt])
  {
    if(*temp1== temp2[cnt])
    {
      temp1++;
      //temp2++; 
      cnt++;
    }
    else
    {
      if(*temp1< temp2[cnt])
      {
        return -1;  //returning a negative value
      }
      else
      {
        return 1;   //returning a positive value
      }
    }
    if (cnt == max_length -1 )
    {
      return 0; 
    } 
    //cnt++;
  }
  return 0; //return 0 when strings are same
}



static char *statusPath = "GET /status";
static char *configurePath = "POST /configure";

enum RequestType {INVALID = 0, POST_CONFIG, GET_STATUS};
enum RequestType reqType = INVALID; 

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
    //sum += 4;
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

int8_t extract_path()
{
  int stringlen = my_strlen((char*) (uint8_t*) bufferIn); 
  // strlen works, because after the request is the buffer 00ed 
  // and we see single pages --> not the complete transfer at once
  

  if (stringlen == 0)
  {
    return 0;
  }
  if (stringlen == PAYLOAD_BYTES_PER_PAGE && bufferIn[0 + stringlen -1] != '\n') //last byte of a header
  {
    // i.e. request is longer than one page 
    return -1;
  }
  // a post request is not always ended with a \n or a payload (bitfile)  can also have a \0 in it => wie don't know the exact header length now 
  

  printf("stringlen: %d\n",(int) stringlen);

  int requestLen = request_len(0, stringlen);
  //int requestLen = 135;
  printf("requestLen: %d\n",(int) requestLen);

  //from here it looks like a valid header 


  reqType = INVALID; //reset 

  if(requestLen == -1)
  {//not a valid header
    return -2;
  }

 if (my_strcmp(statusPath, bufferIn ,my_strlen(statusPath)) == 0 )
  {
    reqType = GET_STATUS; 
    return 1;
  } else if (my_strcmp(configurePath, bufferIn ,my_strlen(configurePath)) == 0 )
  { 
    currentPayloadStart = requestLen + 4;
    reqType = POST_CONFIG; 
    return 2;
  } else { //Invalid
    return -2;
  } 
  return -2;

}


void parseHttpInput(ap_uint<1> transferErr, ap_uint<1> wasAbort)
{

 switch (httpState) {
    case HTTP_IDLE: //both the same
    case HTTP_PARSE_HEADER: 
             //search for HTTP 
             switch (extract_path()) {
             /*  case -3: //404
                 httpState = HTTP_INVALID_REQUEST;
                emptyOutBuffer();
                httpAnswerPageLength = writeHttpStatus(404,0);
                   break; */
               case -2: //invalid content 
                 httpState = HTTP_INVALID_REQUEST;
                emptyOutBuffer();
                httpAnswerPageLength = writeHttpStatus(400,0);
                   break; 
               case -1: //not yet complete 
                 httpState = HTTP_PARSE_HEADER; 
                    break;
                case 0: //not vaild until now
                    break;
                case 1: //get status 
                    httpState = HTTP_REQUEST_COMPLETE;
                    break;
                case 2: //post config 
                    httpState = HTTP_HEADER_PARSED;
                    break;
             }
               break;
    case HTTP_HEADER_PARSED: //this state is valid for one core-cycle: after that the current payload should start at 0 
               httpState = HTTP_READ_PAYLOAD;
               currentPayloadStart = currentBufferInPtr;
               break; 
    case HTTP_REQUEST_COMPLETE: //for requests without payload -> need step in between in other process -> no break here!
    case HTTP_SEND_RESPONSE:
               emptyOutBuffer();
               if (transferErr == 1)
               {
                 httpAnswerPageLength = writeHttpStatus(400,0);
               } else if(wasAbort == 1)
               {
                 httpAnswerPageLength = writeHttpStatus(500,0);
               } else if(reqType == GET_STATUS)
               { 
                 //combine status 
                 //length??
                 //int answerLength = 0;
                 //httpAnswerPageLength = writeHttpStatus(200,answerLength);
                 httpAnswerPageLength = writeHttpStatus(200,0);
                 //write status
                 writeDisplaysToOutBuffer();
                 writeString(httpNL); //to finish body
               } else {
                httpAnswerPageLength = writeHttpStatus(200,0);
               }
               httpState = HTTP_DONE;
               break;
    default:
               break;
  }

  printf("parseHttpInput returns with state %d\n",httpState);

}




