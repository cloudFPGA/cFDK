
#include <stdint.h>
//#include "hls_math.h"
#include "ap_int.h"
#include "ap_utils.h"

#include "http.hpp"
#include "smc.hpp" 

extern HttpState httpState; 

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


int my_strlen(char * s) {
	int i = 0, sum = 0;
	char c = s[0];

	while(c != '\0') {
		sum++;
		c = s[++i];
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
		arr[i] = '\0';		// Append string terminator
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

	if ( content_length > 0)
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
	
	len += writeString(httpNL); // to finish header 

	int8_t pageCnt = len/BYTES_PER_PAGE; 

	if(len % BYTES_PER_PAGE > 0)
	{
		pageCnt++;
	}

	return pageCnt;
	//return 1;
}


ap_uint<16> parseHttpInput()
{
	

	//return offset to payload 
	return 0; 
}




