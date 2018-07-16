
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "../src/smc.hpp"

#include <stdint.h>

bool checkResult(ap_uint<32> MMIO, ap_uint<32> expected)
{
  if (MMIO == expected)
  {
    printf("expected result: %#010x\n", (int) MMIO);
    return true;
  }

  printf("ERROR: Got %#010x instead of %#010x\n", (int) MMIO, (int) expected);
  return false;
  //exit -1;
}

void printBuffer(ap_uint<8> buffer_int[BUFFER_SIZE], char* msg, int max_pages)
{
  printf("%s: \n",msg);
  //for( int i = 0; i < BUFFER_SIZE; i++)
  for( int i = 0; i < BYTES_PER_PAGE*max_pages; i++)
  {
    uint8_t cur_elem = (char) buffer_int[i];
    printf("%02x ", cur_elem);

    if(i % 8 == 7)
    {
      printf("\n");
    }

  }
}

void printBuffer32(ap_uint<32> buffer_int[XMEM_SIZE], char* msg, int max_pages)
{
  printf("%s: \n",msg);
  for( int i = 0; i < LINES_PER_PAGE*max_pages; i++)
  {
    int cur_elem = (int) buffer_int[i];
    printf("%08x ", cur_elem);

    if(i % 8 == 7)
    {
      printf("\n");
    }

  }
}


void initBuffer(ap_uint<4> cnt,ap_uint<32> xmem[XMEM_SIZE], bool lastPage, bool withPattern )
{
  ap_uint<32> ctrlWord = 0;
  for(int i = 0; i<8; i++)
  {
    ctrlWord |= ((ap_uint<32>) cnt) << (i*4);
  }
  
  for(int i = 0; i<LINES_PER_PAGE; i++)
  {
    if (withPattern) 
    {
    xmem[i] = ctrlWord;
    } else {
    //xmem[i] = ctrlWord+i; 
    // xmem[i] = (ctrlWord << 16) + (rand() % 65536);
      xmem[i] = (((ap_uint<32>) cnt) << 24) | 0x0A0B00 | ((ap_uint<32>) i);
    }
  }
  //printf("CtrlWord: %#010x\n",(int) ctrlWord);
  
  //Set Header and Footer
  ap_uint<8> header = (ap_uint<8>) cnt;
  if (lastPage) 
  {
    header |= 0xf0; 
  }
  //ap_uint<32> headerLine = header | (ctrlWord & 0xFFFFFF00);
  ap_uint<32> headerLine = header | (xmem[0] & 0xFFFFFF00);
  //ap_uint<32> footerLine = (((ap_uint<32>) header) << 24) | (ctrlWord & 0x00FFFFFF);
  ap_uint<32> footerLine = (((ap_uint<32>) header) << 24) | (xmem[LINES_PER_PAGE-1] & 0x00FFFFFF);
  xmem[0] = headerLine;
  xmem[LINES_PER_PAGE-1] = footerLine;

}

void copyBufferToXmem(char* buffer_int, ap_uint<32> xmem[XMEM_SIZE])
{
  for(int i = 0; i<LINES_PER_PAGE; i++)
  {
    ap_uint<32> tmp = 0; 
    tmp = ((ap_uint<32>) (uint8_t) buffer_int[i*4 + 0]); 
    tmp |= ((ap_uint<32>)(uint8_t) buffer_int[i*4 + 1]) << 8; 
    tmp |= ((ap_uint<32>)(uint8_t) buffer_int[i*4 + 2]) << 16; 
    tmp |= ((ap_uint<32>)(uint8_t) buffer_int[i*4 + 3]) << 24; 
    xmem[i] = tmp;;
  }
}

int main(){

  ap_uint<32> MMIO;
  ap_uint<32> SR;
  ap_uint<32> ISR;
  ap_uint<32> WFV;
  ap_uint<1>  decoupActive = 0b0;

  ap_uint<32> HWICAP[512];
  ap_uint<32> xmem[XMEM_SIZE];

  SR=0x5;
  ISR=0x4;
  WFV=0x7FF;

  HWICAP[SR_OFFSET] = SR;
  HWICAP[ISR_OFFSET] = ISR;
  HWICAP[WFV_OFFSET] = WFV;
  HWICAP[ASR_OFFSET] = 0;
  HWICAP[CR_OFFSET] = 0;
  //HWICAP[RFO_OFFSET] = 0x3FF;

  bool succeded = true;

  ap_uint<32> MMIO_in = 0x0;


  smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0xBEBAFECA) && succeded;

//===========================================================
//Test Displays
  MMIO_in = 0x1 << DSEL_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x1007FF07) && succeded && (decoupActive == 0);

  MMIO_in = 0x2 << DSEL_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x20000000) && succeded && (decoupActive == 0);

  MMIO_in = 0x1 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x100FFF07) && succeded && (decoupActive == 1);

  MMIO_in = 0x3 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x3f49444C) && succeded && (decoupActive == 1);

  MMIO_in = 0x1 << DSEL_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x100FFF07) && succeded && (decoupActive == 0);

//===========================================================
//Test Counter & Xmem

  int cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30204F4B);

  cnt = 1;
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31204F4B);

  cnt = 2;
  //initBuffer((ap_uint<4>) cnt, xmem);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31555444);
  
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x32204F4B);

  //smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  //succeded &= checkResult(MMIO, 0x32555444);

  cnt = 3;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  xmem[2] = 42;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  //succeded &= checkResult(MMIO, 0x32434F52);
  succeded &= checkResult(MMIO, 0x33204F4B);
  
  /*initBuffer((ap_uint<4>) cnt, xmem, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x32434F52);*/

  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30204F4B);
  
  cnt = 1;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  xmem[0] =  42;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30494E56);
  
  //Test RST
  MMIO_in = 0x3 << DSEL_SHIFT | ( 0 << WCNT_SHIFT) | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);
  
  //Test ABR
  cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  //HWICAP[CR_OFFSET] = CR_ABORT;
  HWICAP[ASR_OFFSET] = 0x42;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3F414252);

  HWICAP[ASR_OFFSET] = 0x0;
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  HWICAP[CR_OFFSET] = 0x0;
  //one complete transfer with overflow
  //LOOP
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | (1 << SWAP_SHIFT);
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);
  for(int i = 0; i<0xf; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, false); 
    smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
    assert(decoupActive == 1);

    //printBuffer(bufferIn, "bufferIn", 7);
    //printBuffer32(xmem,"Xmem",1);
    //printf("currentBufferInPtr: %#010x\n",(int) currentBufferInPtr);
    printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);
    //printf("xmem: %#010x\n",(int) xmem[LINES_PER_PAGE-1]);
    int WF_should = 0;
    if ( i % 2 == 0)
    { 
      WF_should = (((xmem[LINES_PER_PAGE-2] & 0xff00) >>8) << 24);
      WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff0000);
      WF_should |= ((xmem[LINES_PER_PAGE-2] & 0xff000000) >> 16); //24-8
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff);
    } else {
      WF_should = (xmem[LINES_PER_PAGE-2] & 0xff000000);
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff) << 16;
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff00);
      WF_should |= ((xmem[LINES_PER_PAGE-1] & 0xff0000) >> 16);
    }
    printf("WF_should: %#010x\n", WF_should);
    assert((int) HWICAP[WF_OFFSET] == WF_should);

  }
  
  printBuffer(bufferIn, "buffer after 0xf transfers:",8);

  printf("CR: %#010x\n", CR_OFFSET);
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f204f4b);
  //printBuffer32(xmem, "Xmem:");

//  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);
  cnt = 0x0;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  HWICAP[WF_OFFSET] = 42;
  HWICAP[CR_OFFSET] = 0;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  //succeded &= checkResult(MMIO, 0x30204F4B) && (HWICAP[WF_OFFSET] == 42);
  succeded &= checkResult(MMIO, 0x30204F4B);
  assert(HWICAP[WF_OFFSET] != 42);
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  cnt = 0x1;
  initBuffer((ap_uint<4>) cnt, xmem, true, false);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31535543);
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  
  //Check CR_WRITE 
  HWICAP[CR_OFFSET] = 0;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31535543);
  assert(HWICAP[CR_OFFSET] == 0);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);
  assert(decoupActive == 0);

  //one complete transfer with overflow and Memcheck
  //LOOP
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | (1 << SWAP_SHIFT);
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);

  xmem[0] = 0xAABBCC00;
  xmem[4] = 0x12121212;
  xmem[127] = 0xAABBCCFF;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  assert(checkResult(MMIO, 0x3f494e56));
  assert(decoupActive == 0);

  for(int i = 0; i<0xf; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, true); 
    smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);

    //printBuffer(bufferIn, "bufferIn", 7);
    //printBuffer32(xmem,"Xmem",1);
    printf("MMIO out: %#010x\n", (int) MMIO);
    assert((MMIO & 0xf0ffffff) == 0x30204F4B);
    assert(decoupActive == 0);
  }
  
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, false, true);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  assert(checkResult(MMIO, 0x3f204f4b));
  
  printBuffer(bufferIn, "buffer after 16 check pattern transfers:",8);

//===========================================================
//Test HTTP
  
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  // GET TEST
//MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT) | ( 1 << SWAP_N_SHIFT);
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  char *httpBuffer = new char[512];
  char* getStatus = "GET /status HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n";
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem);

  xmem[XMEM_ANSWER_START] = 42;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  
  //printBuffer(bufferIn, "buffer after GET transfers:",2);

  //one pause cycle, nothing should happen (but required by state machine)
/*  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);*/
  
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x40000072);
  assert(decoupActive == 0);
  
  //printBuffer(bufferOut, "BufferOut:",2);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);
  assert(decoupActive == 0);

  // INVALID TEST 
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  getStatus = "GET /theWorldAndEveryghing HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n";
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem);

  xmem[XMEM_ANSWER_START] = 42;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  
  //printBuffer(bufferIn, "buffer after Invalid GET transfers:",2);

  //one pause cycle, nothing should happen (but required by state machine)
/*  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);*/
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x40000061);
  assert(decoupActive == 0);
  
  //printBuffer(bufferOut, "BufferOut:",2);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);

  HWICAP[WFV_OFFSET] = 0x710;

  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  // POST TEST 
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  getStatus = "POST /configure HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nContent-Length: 1607\r\n \
Content-Type: application/x-www-form-urlencodedAB\r\n\r\nffffffffffbb11220044ffffffffffffffffaa99556620000000200000002000";
  printf("%s\n",getStatus);

  httpBuffer[0] = 0x00;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0x00; //this will damage some payload, but should be ok here 
  httpBuffer[128] = 0xF1;
  httpBuffer[255] = 0XF1;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "POST httpBuffer", 3);
  copyBufferToXmem(httpBuffer,xmem);

  xmem[XMEM_ANSWER_START] = 42;
  HWICAP[WF_OFFSET] = 0x42;

  //printBuffer32(xmem, "Xmem:",2);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30204F4B);
  
  printBuffer(bufferIn, "buffer IN after POST 1/2:",3);

  copyBufferToXmem(&httpBuffer[128],xmem);
 // printBuffer32(xmem, "Xmem:",2);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31535543);
  assert(decoupActive == 1);
  printBuffer(bufferIn, "buffer IN after POST 2/2:",3);
  

  //one pause cycle, nothing should happen (but required by state machine)
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31535543);
  //assert(decoupActive == 1); is in the middle...
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x40000072);
  
  printBuffer(bufferOut, "BufferOut:",2);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);

  //printBuffer32(xmem, "Xmem:");
  assert(decoupActive == 0);
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  //assert(HWICAP[WF_OFFSET] == 0x32303030);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f49444C);

  // POST TEST 2
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  //getStatus = "POST /configure HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nContent-Length: 1607\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nffff000000bb11220044ffffffffffffffffaa995566200000002000000020000000200000002000000020000000200000002000000020000000200000002000000020000000200000002000000000200000002000000020000000200000002000";
  getStatus = "POST /configure HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nffff000000bb11220044ffffffffffffffffaa995566200000002000000020000000200000002000000020000000200000002000000020000000200000002000000020000000200000002000000000200000002000000020000000200000002000";

  printf("%s\n",getStatus);

  httpBuffer[0] = 0x00;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0x00; //this will damage some payload, but should be ok here 
  httpBuffer[128] = 0x01;
  httpBuffer[255] = 0X01;
  httpBuffer[256] = 0xF2;
  httpBuffer[383] = 0xF2;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "POST httpBuffer", 3);
  copyBufferToXmem(httpBuffer,xmem);

  xmem[XMEM_ANSWER_START] = 42;
  HWICAP[WF_OFFSET] = 0x42;

  //printBuffer32(xmem, "Xmem:",2);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x30204F4B);
  
  printBuffer(bufferIn, "buffer IN after POST 1/3:",3);
  copyBufferToXmem(&httpBuffer[128],xmem);
  //printBuffer32(xmem, "Xmem:",2);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x31204F4B);
  assert(decoupActive == 1);
  
  printBuffer(bufferIn, "buffer IN after POST 2/3:",3);
    printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);

/*  copyBufferToXmem(&httpBuffer[256],xmem);
  //printBuffer32(xmem, "Xmem:",2);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x32535543);
  assert(decoupActive == 1);
  //printBuffer(bufferIn, "buffer IN after POST 3/3:",3); */
  for(int i = 2; i<0x1f; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, false); 
    HWICAP[CR_OFFSET] = 0;
    printBuffer32(xmem,"Xmem",1);
    smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
    //test double call --> nothing should change
    for(int j = 0; j< 4; j++)
    {
    //printf("DOUBLE CALL\n");
      printf("%d CALL\n",j);
      smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
    }
    //printf("TRIBLE CALL\n");
    //smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
    
    assert(decoupActive == 1);
    assert(HWICAP[CR_OFFSET] == CR_WRITE);

    printBuffer(bufferIn, "bufferIn", 8);
    //printf("bufferInPtrRead: %#010x\n",(int) bufferInPtrRead);
 /*   printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);
    //printf("xmem: %#010x\n",(int) xmem[LINES_PER_PAGE-1]);
    int WF_should = 0;
    if ( i % 2 == 0)
    { 
      WF_should = (((xmem[LINES_PER_PAGE-2] & 0xff00) >>8) << 24);
      WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff0000);
      WF_should |= ((xmem[LINES_PER_PAGE-2] & 0xff000000) >> 16); //24-8
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff);
    } else {
      WF_should = (xmem[LINES_PER_PAGE-2] & 0xff000000);
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff) << 16;
      WF_should |= (xmem[LINES_PER_PAGE-1] & 0xff00);
      WF_should |= ((xmem[LINES_PER_PAGE-1] & 0xff0000) >> 16);
    }
    printf("WF_should: %#010x\n", WF_should);
  //  assert((int) HWICAP[WF_OFFSET] == WF_should);*/
  //  can't check that --> may be offset of 1/2/3 

  }
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, true, false);
  //xmem[126] = 0x0d0a0d0a;
  //xmem[LINES_PER_PAGE -2 ] = 0x0d000000;
  //xmem[LINES_PER_PAGE -1] = 0xff0a0d0a;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f535543);
  assert(decoupActive == 1);

  printf("DOUBLE CALL (Final)\n");
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  printf("TRIBLE CALL (Final)\n");
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);
  int WF_should = 0;
    WF_should = (xmem[LINES_PER_PAGE-3] & 0xff000000);
    WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff) << 16;
    WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff00);
    WF_should |= ((xmem[LINES_PER_PAGE-2] & 0xff0000) >> 16);
  printf("WF_should: %#010x\n", WF_should);
  //succeded &= checkResult(MMIO, 0x3f204f4b);
    
  printBuffer32(xmem,"Xmem",1);
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  printBuffer(bufferIn, "bufferIn after 15 HTTP transfer", 8);
  //assert((int) HWICAP[WF_OFFSET] == WF_should);
/*
  //one pause cycle, nothing should happen (but required by state machine)
  MMIO_in = 0x4 << DSEL_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  //succeded &= checkResult(MMIO, 0x3f535543); 
  succeded &= checkResult(MMIO, 0x40000040); 
  assert(decoupActive == 1);
  
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  //succeded &= checkResult(MMIO, 0x3f535543);
  succeded &= checkResult(MMIO, 0x40000071); 
//  assert(decoupActive == 1);*/
  
  //Check CR_WRITE 
  HWICAP[CR_OFFSET] = 0;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x3f535543);
  assert(HWICAP[CR_OFFSET] == 0);


  //MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  MMIO_in = 0x4 << DSEL_SHIFT;
  smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
  succeded &= checkResult(MMIO, 0x40000072);
  assert(decoupActive == 0);
  
  printBuffer(bufferOut, "BufferOut:",2);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);

  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  
  //printf("DONE\n");

  return succeded? 0 : -1;
  //return 0;
}
