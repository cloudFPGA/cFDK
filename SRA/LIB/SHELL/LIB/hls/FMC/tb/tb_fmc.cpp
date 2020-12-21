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
 * @file       : tb_fmc.cpp
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

#define DEBUG

#ifdef COSIM
//to disable asserts
#define NDEBUG 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/fmc.hpp"
#include "../../../../../hls/cfdk.hpp"
#include "../../../../../hls/network.hpp"
#include "../../NAL/src/nal.hpp" //AFTER fmc.hpp

#include <stdint.h>

#include <hls_stream.h>

using namespace std;

#ifndef COSIM
#include <sys/time.h>
#define HWICAP_SEQ_SIZE ((16*IN_BUFFER_SIZE + 512)/4)
#else
#define HWICAP_SEQ_SIZE ((4*IN_BUFFER_SIZE + 512)/4)
#endif
#define HWICAP_SEQ_START_ADDRESS 1024

//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
ap_uint<32> MMIO_in = 0x0;
ap_uint<32> MMIO;
ap_uint<32> MMIO_in_BE, MMIO_out_BE;
ap_uint<1>  layer_4_enabled = 0b1;
ap_uint<1>  layer_6_enabled = 0b0;
ap_uint<1>  layer_7_enabled = 0b0;
ap_uint<16> role_mmio = 0x1DEA;
ap_uint<32> sim_fpga_time_seconds = 0;
ap_uint<32> sim_fpga_time_minutes = 0;
ap_uint<32> sim_fpga_time_hours   = 0;
ap_uint<1>  decoupActive = 0b0;
ap_uint<1>  decoupStatus = 0b0;
ap_uint<1>  softReset = 0b0;
ap_uint<32> nodeRank_out;
ap_uint<32> clusterSize_out;
ap_uint<32> HWICAP[512];
uint8_t HWICAP_seq_IN[HWICAP_SEQ_SIZE*4];
ap_uint<32> HWICAP_seq_OUT[HWICAP_SEQ_SIZE];
ap_uint<32> xmem[XMEM_SIZE];
ap_uint<32> nalCtrl[NAL_CTRL_LINK_SIZE];
ap_uint<1>  disable_ctrl_link = 0b0;
ap_uint<1>  disable_pyro_link = 0b0;
stream<Axis<8> >  FMC_Debug_Pyrolink    ("FMC_Debug_Pyrolink"); //soPYROLINK
stream<Axis<8> >  Debug_FMC_Pyrolink    ("Debug_FMC_Pyrolink"); //siPYROLINK
stream<TcpWord>   sFMC_NAL_Tcp_data   ("sFMC_Nal_Tcp_data");
stream<AppMeta>   sFMC_NAL_Tcp_sessId ("sFMC_Nal_Tcp_sessId");
stream<TcpWord>   sNAL_FMC_Tcp_data   ("sNAL_FMC_Tcp_data");
stream<AppMeta>   sNAL_FMC_Tcp_sessId ("sNAL_FMC_Tcp_sessId");

ap_uint<32> reverse_byte_order(ap_uint<32> input)
{
  ap_uint<32> output = 0x0;
  output  = (ap_uint<32>) ((input >> 24) & 0xFF);
  output |= (ap_uint<32>) ((input >> 8) & 0xFF00);
  output |= (ap_uint<32>) ((input << 8) & 0xFF0000);
  output |= (ap_uint<32>) ((input << 24) & 0xFF000000);
  return output;
}

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int         simCnt;

void stepDut() {
    ap_uint<32> *used_hwicap_buffer = HWICAP;
    if(use_sequential_hwicap)
    {
      used_hwicap_buffer = HWICAP_seq_OUT;
    }
    MMIO_in_BE = reverse_byte_order(MMIO_in);
    fmc(&MMIO_in_BE, &MMIO_out_BE,
        &layer_4_enabled, &layer_6_enabled, &layer_7_enabled,
        &sim_fpga_time_seconds, &sim_fpga_time_minutes, &sim_fpga_time_hours,
        &role_mmio,
        used_hwicap_buffer, decoupStatus, &decoupActive,
        &softReset, xmem,
        nalCtrl, &disable_ctrl_link, 
        sNAL_FMC_Tcp_data, sNAL_FMC_Tcp_sessId,
        sFMC_NAL_Tcp_data, sFMC_NAL_Tcp_sessId,
#ifdef INCLUDE_PYROLINK
        FMC_Debug_Pyrolink, Debug_FMC_Pyrolink, &disable_pyro_link,
#endif
        &nodeRank_out, &clusterSize_out);
    MMIO = reverse_byte_order(MMIO_out_BE);
    simCnt++;
    printf("[%4.4d] STEP DUT \n", simCnt);
    fflush(stdin);
    fflush(stdout);
}


bool checkResult(ap_uint<32> MMIO, ap_uint<32> expected)
{
  if (MMIO == expected)
  {
    printf("expected result: %#010x\n", (int) MMIO);
    return true;
  }

  printf("[%4.4d] ERROR: Got %#010x instead of %#010x\n", simCnt, (int) MMIO, (int) expected);
  //return false;
  exit(-1);
}

void printBuffer(volatile uint8_t *buffer_int, char* msg, int max_pages)
{
  printf("%s: \n",msg);
  for( int i = 0; i < BYTES_PER_PAGE*max_pages; i++)
  {
    uint8_t cur_elem = (ap_uint<8>) buffer_int[i];
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

bool checkSeqHwicap(uint32_t *true_buffer, ap_uint<32> *out_buffer, uint32_t start_address, uint32_t end_address, bool not_to_swap)
{
  uint32_t max_address = end_address;
  if(max_address > HWICAP_SEQ_SIZE)
  {
    max_address = HWICAP_SEQ_SIZE;
    printf("set max verify address to %d.\n",max_address);
  }
  for(int i = start_address; i<max_address; i++)
  {
    ap_uint<32> true_buffer_BE = 0x0;
    if(not_to_swap)
    {
      true_buffer_BE = true_buffer[i];
    } else {
      true_buffer_BE  = (ap_uint<32>) ((true_buffer[i] >> 24) & 0xFF);
      true_buffer_BE |= (ap_uint<32>) ((true_buffer[i] >> 8) & 0xFF00);
      true_buffer_BE |= (ap_uint<32>) ((true_buffer[i] << 8) & 0xFF0000);
      true_buffer_BE |= (ap_uint<32>) ((true_buffer[i] << 24) & 0xFF000000);
    }
    if(true_buffer_BE != out_buffer[i])
    {
      printf("HWICAP OUT is different at position %d: should be %#010x, but is %#010x\n", i, (int) true_buffer_BE, (int) out_buffer[i]);
      printf("relative position: %d; end address: %d\n",(int) (i - start_address), max_address);
      printf("Context:\n\tShould:\n");
      for(int j = i-3; j<i+3; j++)
      {
        ap_uint<32> true_buffer_BE_2 = 0x0;
        if(not_to_swap)
        {
          true_buffer_BE_2 = true_buffer[j];
        } else {
          true_buffer_BE_2  = (ap_uint<32>) ((true_buffer[j] >> 24) & 0xFF);
          true_buffer_BE_2 |= (ap_uint<32>) ((true_buffer[j] >> 8) & 0xFF00);
          true_buffer_BE_2 |= (ap_uint<32>) ((true_buffer[j] << 8) & 0xFF0000);
          true_buffer_BE_2 |= (ap_uint<32>) ((true_buffer[j] << 24) & 0xFF000000);
        }
        printf("\t%4.4d:  %#010x\n",j,(int) true_buffer_BE_2);
      }
      printf("\tIs:\n");
      for(int j = i-3; j<i+3; j++)
      {
        printf("\t%4.4d:  %#010x\n",j,(int) out_buffer[j]);
      }
      printf("\n");
      return false;
    }
  }
  printf("[HWICAP OUT is valid from address %d to %d]\n",start_address, max_address);
  return true;
}

void initBuffer(ap_uint<4> cnt,ap_uint<32> xmem[XMEM_SIZE], bool lastPage, bool withPattern)
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

ap_uint<64>  initStream(ap_uint<4> cnt, stream<NetworkWord> &tcp_data, int nr_words, bool with_pattern, bool inverse_upper, bool insert_tlast)
{
  ap_uint<32> ctrlWord = 0;
  for(int i = 0; i<8; i++)
  {
    if(with_pattern)
    {
      ctrlWord |= ((ap_uint<32>) ++cnt) << (i*4);
    } else {
      ctrlWord |= ((ap_uint<32>) cnt) << (i*4);
    }
  }

  ap_uint<64> data_word = 0;
  if(inverse_upper)
  {
    data_word = (((ap_uint<64>) ~ctrlWord) << 32) | ctrlWord;
  } else {
    data_word = ((ap_uint<64>) ctrlWord << 32) | ctrlWord;
  }

  printf("Writing to Stream: 0x%llx\n",(unsigned long long) data_word);

  for(int i = 0; i<nr_words; i++)
  {
    NetworkWord tmp = NetworkWord();
    tmp.tdata = data_word;
    tmp.tkeep = 0xFF;
    if(i == nr_words -1 && insert_tlast)
    {
      tmp.tlast = 1;
    } else { 
      tmp.tlast = 0;
    }
    tcp_data.write(tmp);
  }

  return data_word;
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

void copyBufferToStream(char *buffer_int, stream<NetworkWord> &tcp_data, int content_len)
{
  printf("Writing %d bytes to Stream: (in hex)\n", content_len);
  for(int i = 0; i<content_len; i+=8)
  {
    ap_uint<64> new_word = 0;
    ap_uint<8> tkeep = 0;
    for(int j = 0; j<8; j++)
    {
        if(i+j < content_len)
        {
          new_word |= ((ap_uint<64>) ((uint8_t) buffer_int[i+j])) << (j*8);
          tkeep |= ((ap_uint<8>) 1) << j;
        } else {
          new_word |= ((ap_uint<64>) ((uint8_t) 0 ))<< (j*8);
          tkeep |= ((ap_uint<8>) 0) << j;
        }
    }
    //printf("Writing to Stream: 0x%llx\n",(unsigned long long) new_word);
    //printf("%016llx\n",(unsigned long long) new_word);

    NetworkWord tmp = NetworkWord();
    tmp.tdata = new_word;
    tmp.tkeep = tkeep;
    if(i >= (content_len - 8))
    {
      tmp.tlast = 1;
    } else {
      tmp.tlast = 0;
    }

    printf("%016llx   | %02x  | %d \n",(unsigned long long) new_word, (int) tkeep, (int) tmp.tlast);
    tcp_data.write(tmp);

  }

}

void drainStream(stream<NetworkWord> &tcp_data)
{
  printf("Draining NetworkStream: (in hex)\n");

  while(!tcp_data.empty())
  {
    NetworkWord tmp = tcp_data.read();
    printf("%016llx\n",(unsigned long long) tmp.tdata);
  }

}

int main(){

  ap_uint<32> SR;
  ap_uint<32> ISR;
  ap_uint<32> WFV;

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

  decoupStatus = 0b1;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0xBEBAFECA) && succeded;

//===========================================================
//Test Displays
  MMIO_in = 0x1 << DSEL_SHIFT;
  decoupStatus = 0b0;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x1007FF07) && succeded && (decoupActive == 0);

  MMIO_in = 0x2 << DSEL_SHIFT;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x20000000) && succeded && (decoupActive == 0);

  MMIO_in = 0x1 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
  decoupStatus = 0b1;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x100FFF07) && succeded && (decoupActive == 1);

  MMIO_in = 0x3 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x3f49444C) && succeded && (decoupActive == 1);

  MMIO_in = 0x1 << DSEL_SHIFT;
  stepDut();
  printf("%#010x\n", (int) MMIO);
  succeded = (MMIO == 0x100FFF07) && succeded && (decoupActive == 0);
  assert(succeded);
  printf("== Display and Decoup Test passed == \n");

//===========================================================
//Test Counter & Xmem
  printf("===== XMEM =====\n");

  int cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  decoupStatus = 0b0;
  stepDut(); //simCnt 7
  succeded &= checkResult(MMIO, 0x30204F4B);

  cnt = 1;
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  stepDut();
  succeded &= checkResult(MMIO, 0x31204F4B);

  cnt = 2;
  stepDut();
  succeded &= checkResult(MMIO, 0x31555444);
  
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  stepDut();
  succeded &= checkResult(MMIO, 0x32204F4B);

  cnt = 3;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  xmem[2] = 42;
  stepDut();
  succeded &= checkResult(MMIO, 0x33204F4B);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << RST_SHIFT);
  stepDut(); //12
  succeded &= checkResult(MMIO, 0x3349444C);

  cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  stepDut();
  succeded &= checkResult(MMIO, 0x30204F4B);
  
  cnt = 1;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  xmem[0] =  42;
  stepDut();
  succeded &= checkResult(MMIO, 0x30494E56);
  
  //Test RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x3049444C);
  
  //Test ABR
  cnt = 0;
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  //HWICAP[CR_OFFSET] = CR_ABORT;
  HWICAP[ASR_OFFSET] = 0x42;
  stepDut();
  succeded &= checkResult(MMIO, 0x3f414252);

  HWICAP[ASR_OFFSET] = 0x0;
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x3f49444C);
  assert(succeded);

  //RST again (BOR)
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //18
  //succeded &= checkResult(MMIO, 0x3f424f52);
  succeded &= checkResult(MMIO & 0xFF000000, 0x3f000000);

  HWICAP[CR_OFFSET] = 0x0;
  //one complete transfer with overflow
  //LOOP
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);
  for(int i = 0; i<0xf; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, false); 
    stepDut();
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
  stepDut(); //34
  succeded &= checkResult(MMIO, 0x3f204f4b);
  //printBuffer32(xmem, "Xmem:");

//  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);
  cnt = 0x0;
  initBuffer((ap_uint<4>) cnt, xmem, false, false);
  HWICAP[WF_OFFSET] = 42;
  HWICAP[CR_OFFSET] = 0;
  stepDut();
  //succeded &= checkResult(MMIO, 0x30204F4B) && (HWICAP[WF_OFFSET] == 42);
  succeded &= checkResult(MMIO, 0x30204F4B);
  assert(HWICAP[WF_OFFSET] != 42);
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  
  MMIO_in = 0x3 << DSEL_SHIFT |(126 << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT);
  cnt = 0x1;
  initBuffer((ap_uint<4>) cnt, xmem, true, false);
  stepDut();
  succeded &= checkResult(MMIO, 0x31535543);
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  
  //Check CR_WRITE 
  HWICAP[CR_OFFSET] = 0;
  stepDut(); //37
  succeded &= checkResult(MMIO, 0x31535543);
  assert(HWICAP[CR_OFFSET] == 0);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x3149444C);
  assert(decoupActive == 0);

  //one complete transfer with overflow and Memcheck
  //LOOP
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | (1 << SWAP_SHIFT);
  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT);
  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << CHECK_PATTERN_SHIFT);

  xmem[0] = 0xAABBCC00;
  xmem[4] = 0x12121212;
  xmem[127] = 0xAABBCCFF;
  stepDut();
  assert(checkResult(MMIO, 0x3f494e56));
  assert(decoupActive == 0);

  for(int i = 0; i<0xf; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, true); 
    stepDut();

    //printBuffer(bufferIn, "bufferIn", 7);
    //printBuffer32(xmem,"Xmem",1);
    printf("MMIO out: %#010x\n", (int) MMIO);
    //assert((MMIO & 0xf0ffffff) == 0x30204F4B);
    //msg could also be UTD instead of OK
    //assert((MMIO & 0xf0ffffff) == (0x30204F4B | 0x30555444));
    assert(decoupActive == 0);
  }
  
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, false, true);
  stepDut(); //55
  assert(checkResult(MMIO & 0xf0ffffff, 0x30555444));
  
  printBuffer(bufferIn, "buffer after 16 check pattern transfers:",8);

  printf("== XMEM Test passed == \n");
//===========================================================
//Test HTTP
 
  printf("===== HTTP =====\n");

  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x3f49444C);

  // GET TEST
  char *httpBuffer = new char[1024];
  char* getStatus = "GET /status HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n";
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  int lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  //printBuffer(bufferIn, "buffer after GET transfers:",2);
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut(); //58
  succeded &= checkResult(MMIO & 0xFF00FFF0, 0x43000070);
  assert(decoupActive == 0);
  printBuffer(bufferOut, "Valid HTTP GET: BufferOut:",4);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x3049444C);
  assert(decoupActive == 0);

  // INVALID TEST 
  getStatus = "GET /theWorldAndEveryghing HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n";
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut(); //60
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  //printBuffer(bufferIn, "buffer after Invalid GET transfers:",2);
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x43000061);
  assert(decoupActive == 0);
  
  printBuffer(bufferOut, "INVALID REQUEST; BufferOut:",2);
 // printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);

  HWICAP[WFV_OFFSET] = 0x710;
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x3049444C);
  assert(decoupActive == 0);

  // INVALID TEST 2
  getStatus = "GET /theWorldAndEveryghing HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n"; //i.e. missing second \r\n
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  //printBuffer(bufferIn, "buffer after 2. Invalid GET transfers:",2);
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x43000061);
  assert(decoupActive == 0);
  
  //printBuffer(bufferOut, "BufferOut:",2);
  //printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);

  HWICAP[WFV_OFFSET] = 0x710;


  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //65
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x3049444C);

  // POST TEST 
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
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  HWICAP[WF_OFFSET] = 0x42;
  HWICAP[WFV_OFFSET] = 0x1FE; //to not trigger the "empty FIFO" counter
  //printBuffer32(xmem, "Xmem:",2);
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30204F4B);
  printBuffer(bufferIn, "buffer IN after POST 1/2:",3);
  copyBufferToXmem(&httpBuffer[128],xmem );
 // printBuffer32(xmem, "Xmem:",2);
  stepDut();
  succeded &= checkResult(MMIO, 0x31535543);
  printBuffer(bufferIn, "buffer IN after POST 2/2:",3);
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  printBuffer(bufferOut, "POST TEST 1; BufferOut:",2);
  succeded &= checkResult(MMIO, 0x43000072);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);
  //printBuffer32(xmem, "Xmem:");
  assert(decoupActive == 0);
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  //assert(HWICAP[WF_OFFSET] == 0x32303030);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //69
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x3049444C);

  // POST TEST 2
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
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  HWICAP[WF_OFFSET] = 0x42;
  //printBuffer32(xmem, "Xmem:",2);
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut(); //70
  succeded &= checkResult(MMIO, 0x30204F4B);
  printBuffer(bufferIn, "buffer IN after POST 1/3:",3);
  copyBufferToXmem(&httpBuffer[128],xmem );
  //printBuffer32(xmem, "Xmem:",2);
  stepDut();
  succeded &= checkResult(MMIO, 0x31204F4B);
  assert(decoupActive == 1);
  
  printBuffer(bufferIn, "buffer IN after POST 2/3:",3);
    printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);

  for(int i = 2; i<0x1f; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, false); 
    HWICAP[CR_OFFSET] = 0;
    //printBuffer32(xmem,"Xmem",1);
    stepDut();
    //test double call --> nothing should change
    for(int j = 0; j< 4; j++)
    {
      stepDut();
    }
    
    assert(decoupActive == 1);
    assert(HWICAP[CR_OFFSET] == CR_WRITE);


  }
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, true, false);
  stepDut(); //217
  succeded &= checkResult(MMIO, 0x3f535543);

  stepDut();
  int WF_should = 0;
    WF_should = (xmem[LINES_PER_PAGE-3] & 0xff000000);
    WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff) << 16;
    WF_should |= (xmem[LINES_PER_PAGE-2] & 0xff00);
    WF_should |= ((xmem[LINES_PER_PAGE-2] & 0xff0000) >> 16);
  //printf("WF_should: %#010x\n", WF_should);
    
 // printBuffer32(xmem,"Xmem",1);
  stepDut();
  printBuffer(bufferIn, "bufferIn after 15 HTTP transfer", 8);
  
  //Check CR_WRITE 
  HWICAP[CR_OFFSET] = 0;
  stepDut(); //220
  succeded &= checkResult(MMIO, 0x3f535543);
  assert(HWICAP[CR_OFFSET] == 0);


  MMIO_in = 0x4 << DSEL_SHIFT;
  stepDut();
  succeded &= checkResult(MMIO, 0x40000072);
  assert(decoupActive == 0);
  
  printBuffer(bufferOut, "BufferOut:",2);
  printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);

  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  //succeded &= checkResult(MMIO & 0xf0ffffff, 0x30424f52); //FMC is already boring
  succeded &= checkResult(MMIO & 0xFF000000, 0x3f000000);
  assert(decoupActive == 0);

  // PUT RANK TEST
  getStatus = "PUT /rank/42 HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n"; 
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  //printBuffer(bufferIn, "buffer after PUT RANK transfers:",2);
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x40000071);
  assert(decoupActive == 0);
  printBuffer(bufferOut, "PUT RANK: BufferOut:",2);
  //printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  assert(nodeRank_out == 42);

  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //225
  //succeded &= checkResult(MMIO & 0xf0ffffff, 0x30424f52);
  succeded &= checkResult(MMIO & 0xFF000000, 0x30000000);
  assert(decoupActive == 0);

  // PUT RANK INVALID TEST
  getStatus = "PUT /rank/abc HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n"; 
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
 // printBuffer(bufferIn, "buffer after PUT RANK transfers:",2);
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x40000061);
  assert(decoupActive == 0);
  printBuffer(bufferOut, "PUT RANK INVALID: BufferOut:",2);
  //printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  //assert(nodeRank_out == 0);

  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO & 0xFF000000, 0x30000000);
  assert(decoupActive == 0);

  // PUT SIZE TEST
  getStatus = "PUT /size/5 HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n"; 
  httpBuffer[0] = 0xF0;
  strcpy(&httpBuffer[1],getStatus);
  httpBuffer[strlen(getStatus)+1] = 0x0;
  httpBuffer[127] = 0xF0;
  //printBuffer((ap_uint<8>*)(uint8_t*) httpBuffer, "httpBuffer");
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = strlen(getStatus) % 126;
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  //printBuffer(bufferIn, "buffer after PUT SIZE transfers:",2);
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x40000071);
  assert(decoupActive == 0);
  printBuffer(bufferOut, "PUT SIZE: BufferOut:",2);
  //printf("XMEM_ANSWER_START: %#010x\n",(int) xmem[XMEM_ANSWER_START]);
  //printBuffer32(xmem, "Xmem:");
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);
  assert(clusterSize_out == 5);
  
  printf("== HTTP Test passed == \n");

//===========================================================
//Test PYROLINK
#ifdef INCLUDE_PYROLINK
 
  assert(succeded);
  printf("===== PYROLINK TEST =====\n");

  //RST ...BOR
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //231
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x30424f52); //TODO: msg filed may have changed

  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << PYRO_MODE_SHIFT) | 10 << LAST_PAGE_CNT_SHIFT;

  for(int i = 0; i<0xf; i++)
  {
    cnt = i;
    initBuffer((ap_uint<4>) cnt, xmem, false, true); 
    stepDut();

    ap_uint<8> ctrlByte = ((cnt & 0xf) << 4) | (cnt & 0xf);
    for(int j = 0; j <126; j++)
    {
      assert(!FMC_Debug_Pyrolink.empty());
      Axis<8> tmp = FMC_Debug_Pyrolink.read();
      assert(tmp.tdata == ctrlByte);
    }

  }
  
  assert(decoupActive == 0);
  cnt = 0xf;
  initBuffer((ap_uint<4>) cnt, xmem, true, true);
  stepDut(); 
  assert(checkResult(MMIO & 0xf0ffffff, 0x30535543));

  //10 Byte only on the last page
  ap_uint<8> ctrlByte = ((cnt & 0xf) << 4) | (cnt & 0xf);
  for(int j = 0; j <10; j++)
  {
    assert(!FMC_Debug_Pyrolink.empty());
    Axis<8> tmp = FMC_Debug_Pyrolink.read();
    //printf("last page byte %d: .tdata: %d .tkeep: %d  .tlast: %d\n",j, (int) tmp.tdata, (int) tmp.tkeep, (int) tmp.tlast);

    assert(tmp.tdata == ctrlByte);
    if(j == 9)
    {
      assert(tmp.tlast == 1);
    }
  }
  //assert nothing left 
  assert(FMC_Debug_Pyrolink.empty());
  
  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //248
  succeded &= checkResult(MMIO & 0xf0ffffff, 0x3049444C);

  for(int j = 0; j<(3*128 + 14); j++)
  {
    Axis<8> tmp = Axis<8>();
    tmp.tkeep = 1;
    tmp.tlast = 0; 
    tmp.tdata = j;
    Debug_FMC_Pyrolink.write(tmp);
  }
    
  Axis<8> tmp = Axis<8>();
  tmp.tkeep = 1;
  tmp.tlast = 1; 
  tmp.tdata = 3*128 + 15;
  Debug_FMC_Pyrolink.write(tmp);

  //test read request
  MMIO_in = 0x6 << DSEL_SHIFT;
  stepDut(); 
  succeded &= checkResult(MMIO & 0xf0800000, 0x60800000); //we are only interested in the pyro send request bit
  
  MMIO_in = 0x6 << DSEL_SHIFT | (1 << PYRO_READ_REQUEST_SHIFT);
  stepDut(); //250
  succeded &= checkResult(MMIO & 0xf0ff0000, 0x600F0000);
  
  printBuffer(bufferOut, "buffer OUT after Pyrolink READ:",4);
  assert(bufferOut[3*128+15 - 1] == (3*128 + 15) % 256);
  
  printf("== PYROLINK Test passed == \n");
#else
  printf("-- Skipping PYROLINK Test (+20 stepDut) -- \n");
  simCnt += 20;
#endif

//===========================================================
//Test NAL
 
  printf("===== NAL =====\n");
  layer_6_enabled = 0b1;
  sim_fpga_time_seconds = 2;

  //RST
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //251
  succeded &= checkResult(MMIO & 0xf0f0f000, (0x3049444C | 0x30424f52) & 0xf0f0f000); //FMC is somehow bored
  assert(decoupActive == 0);
  
  //POST ROUTING TABLE
  getStatus = "POST /routing HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n";

  printf("%s\n",getStatus);

  httpBuffer[0] = 0x00;
  strcpy(&httpBuffer[1],getStatus);
  int startTable = strlen(getStatus); 
  int routingTable[25] =  {0x30,0x20, 0x0a, 0x0b, 0x0c, 0x01, 0x0a, 0x31,0x20, 0x0a, 0x0b, 0x0c, 0x02, 0x0a, 0x32,0x20, 0x0a, 0x0b, 0x0c, 0x05, 0x0a, 0x0d, 0x0a, 0x0d, 0x0a};
  {
    int j = startTable + 1; 
    int i = 0;
    while(i<25)
    {
      if(j == 127) 
      { 
        httpBuffer[j] = 0x00;
        httpBuffer[j + 1] = 0xF1;
        j += 2;
        continue; 
      } 

      httpBuffer[j] = routingTable[i]; 
      i++;
      j++;

    }
    //to be sure
    httpBuffer[127] = 0x00;
    httpBuffer[128] = 0xF1;
    httpBuffer[255] = 0xF1;
  }
  printBuffer((uint8_t*) httpBuffer, "POST ROUTING httpBuffer", 3);
  copyBufferToXmem(httpBuffer,xmem );
  xmem[XMEM_ANSWER_START] = 42;
  lastPageBytes = (strlen(getStatus) + 25) % 126;
  
  sim_fpga_time_seconds = 5;

  //MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  MMIO_in = 0x3 << DSEL_SHIFT |(lastPageBytes << LAST_PAGE_CNT_SHIFT) | ( 1 << START_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x30204F4B);
  copyBufferToXmem(&httpBuffer[128],xmem );
  stepDut();
  succeded &= checkResult(MMIO, 0x30535543);
  assert(decoupActive == 0);
  
  MMIO_in = 0x4 << DSEL_SHIFT | ( 1 << PARSE_HTTP_SHIFT);
  stepDut();
  succeded &= checkResult(MMIO, 0x40000071);
  assert(decoupActive == 0);
  printBuffer(bufferIn, "POST_ROUTING: BufferIn:",3);
  printBuffer(bufferOut, "POST_ROUTING: BufferOut:",2);
  assert(xmem[XMEM_ANSWER_START] == 0x50545448);

  stepDut(); //255 //for coping MRT
  stepDut();
  stepDut(); //257
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 0] == 0x0a0b0c01);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 1] == 0x0a0b0c02);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 2] == 0x0a0b0c05);

  printf("== NAL Test passed == \n");
//===========================================================
//Test TCP
 
  printf("===== TCP =====\n");
  

//RST again (BOR)
  MMIO_in = 0x3 << DSEL_SHIFT | (1 << RST_SHIFT);
  stepDut(); //258
  succeded &= checkResult(MMIO & 0xFF000000, 0x30000000);

  HWICAP[CR_OFFSET] = 0x0;
  ap_uint<32> WF_ctrl = 0;

  getStatus = "GET /status HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.47.0\r\nAccept: */*\r\n\r\n";
  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));

  Axis<16> sessId = Axis<16>(43);
  sessId.setTLast(1);
  //sNAL_FMC_Tcp_sessId.write(sessId);
  sNAL_FMC_Tcp_sessId.write(sessId.getTData());

  MMIO_in = 0x3 << DSEL_SHIFT | ( 1 << ENABLE_TCP_MODE_SHIFT) | ( 1 << PARSE_HTTP_SHIFT);

  //new TCP FSM: should take only 4 cycles
  stepDut();
  stepDut(); //260
  stepDut();
  stepDut();
  printBuffer(bufferIn, "Valid HTTP GET: BufferIn:",2);
  printBuffer(bufferOut, "Valid HTTP GET: BufferOut:",4);

  Axis<16> sessId_back = sFMC_NAL_Tcp_sessId.read();
  assert(sessId.getTData() == sessId_back.getTData());
  drainStream(sFMC_NAL_Tcp_data);
  assert(sFMC_NAL_Tcp_data.empty());
  
  //"self reset"
  stepDut();
  stepDut();
  
  // POST TEST via TCP
  // first chunk
  getStatus = "POST /configure HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nffff000000bb11220044ffffffffffffffffaa9955662000000021000000220000002300000024000000250000002600000027000000280abcd";

  printf("%s\n",getStatus);

  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));

  sessId = Axis<16>(45);
  sessId.setTLast(1);
  sNAL_FMC_Tcp_sessId.write(sessId.getTData());

  HWICAP[WFV_OFFSET] = 0x3FF;
  HWICAP[WF_OFFSET] = 0x42;
  HWICAP[CR_OFFSET] = 0;
  stepDut(); //265
  stepDut();
  stepDut();
  assert(decoupActive == 1); //should be there immediately
  assert(HWICAP[CR_OFFSET] == CR_WRITE);
  //printBuffer(bufferIn, "bufferIn after TCP HTTP transfer", 2);
  
  //second chunk
  getStatus = "e290000002000000021000000220000002300000000240000002500000026000002700123456789AB\r\n\r\n";
  // -> 196 payload bytes

  printf("%s\n",getStatus);

  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));

  //check telomere (should NOT be used)
  assert(bufferInPtrNextRead > 0);
  stepDut();
  printBuffer(bufferIn, "bufferIn after TCP HTTP transfer", 4);

  //process remaining
  //HWICAP[WFV_OFFSET] = 0x1DE;
  for(int j = 0; j< 3; j++)
  {
    stepDut();
  }

  //printBuffer(bufferOut, "Valid HTTP POST: BufferOut:",4);

  assert(decoupActive == 0);
  printf("WF: %#010x\n",(int) HWICAP[WF_OFFSET]);
  //WF_should = 0;
  //WF_should = 0x00002000;
  WF_should = 0x38394142; //we test in Ascii...
  printf("WF_should: %#010x\n", WF_should);
  assert((int) HWICAP[WF_OFFSET] == WF_should);

  //Check CR_WRITE 
  HWICAP[CR_OFFSET] = 0;
  stepDut(); //271
  succeded &= checkResult(MMIO, 0x30574154); //WAT
  assert(decoupActive == 0);
  assert(HWICAP[CR_OFFSET] == 0);
  
  sessId_back = sFMC_NAL_Tcp_sessId.read();
  assert(sessId.getTData() == sessId_back.getTData());
  drainStream(sFMC_NAL_Tcp_data);
  assert(sFMC_NAL_Tcp_data.empty());
  assert(sFMC_NAL_Tcp_sessId.empty());
  
  //"self reset"
  stepDut(); //272
  stepDut();

  //POST ROUTING via TCP
  getStatus = "POST /routing HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n";

  printf("%s\n",getStatus);

  strcpy(&httpBuffer[0],getStatus);
  startTable = strlen(getStatus); 
  //int routingTable[25] =  {0x30,0x20, 0x0a, 0x0b, 0x0c, 0x01, 0x0a, 0x31,0x20, 0x0a, 0x0b, 0x0c, 0x02, 0x0a, 0x32,0x20, 0x0a, 0x0b, 0x0c, 0x05, 0x0a, 0x0d, 0x0a, 0x0d, 0x0a};
  //reuse routing table
  {
    int j = startTable;
    int i = 0;
    while(i<25)
    {
      httpBuffer[j] = routingTable[i]; 
      i++;
      j++;
    }
  }
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus) + 25);
  
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 0] = 0x0;
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 1] = 0x0;
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 2] = 0x0;

  sessId = Axis<16>(46);
  sessId.setTLast(1);
  sNAL_FMC_Tcp_sessId.write(sessId.getTData());

  stepDut(); //three step only for TCP
  stepDut(); //275
  stepDut();
  stepDut();
  //sim_fpga_time_seconds += 4;
  
  printBuffer(bufferIn, "bufferIn after TCP HTTP POST /routing", 2);
  
  sessId_back = sFMC_NAL_Tcp_sessId.read();
  assert(sessId.getTData() == sessId_back.getTData());
  drainStream(sFMC_NAL_Tcp_data);
  assert(sFMC_NAL_Tcp_data.empty());
  assert(decoupActive == 0);
  
  //some for updating MRT
  for(int j = 0; j< 2; j++)
  {
    stepDut();
  }
 
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 0] == 0x0a0b0c01);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 1] == 0x0a0b0c02);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 2] == 0x0a0b0c05);
  
  //"self reset"
  stepDut(); //279
  stepDut();

  //INVALID POST ROUTING via TCP
  getStatus = "POST /routing HTTP/1.1\r\nUser-Agent: curl/7.47.0\r\nContent-Type: application/x-www-form-urlencoded\r\nbla\r\n\r\n";

  printf("%s\n",getStatus);

  strcpy(&httpBuffer[0],getStatus);
  startTable = strlen(getStatus); 
  //int routingTable[25] =  {0x30,0x20, 0x0a, 0x0b, 0x0c, 0x01, 0x0a, 0x31,0x20, 0x0a, 0x0b, 0x0c, 0x02, 0x0a, 0x32,0x20, 0x0a, 0x0b, 0x0c, 0x05, 0x0a, 0x0d, 0x0a, 0x0d, 0x0a};
  //reuse routing table
  {
    int j = startTable + 3;//this should be enought to become invalid
    int i = 0;
    while(i<25)
    {
      httpBuffer[j] = routingTable[i]; 
      i++;
      j++;
    }
  }
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus) + 25 + 3);
  
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 0] = 0x0;
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 1] = 0x0;
  nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 2] = 0x0;

  sessId = Axis<16>(47);
  sessId.setTLast(1);
  sNAL_FMC_Tcp_sessId.write(sessId.getTData());

  stepDut(); //three step only for TCP
  stepDut();
  stepDut(); //283
  
  //printBuffer(bufferIn, "bufferIn after TCP HTTP POST /routing", 2);
  
  sessId_back = sFMC_NAL_Tcp_sessId.read();
  assert(sessId.getTData() == sessId_back.getTData());
  
  //check 422
  printf("Check stream:\n0x312e312f50545448\n0x706e552032323420\n0x6261737365636f72\n0x7469746e4520656c\n0x65686361430a0d79\n");
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x312e312f50545448);
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x706e552032323420);
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x6261737365636f72);
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x7469746e4520656c);
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x65686361430a0d79);
  //drain remaining
  printf("Partial ");
  drainStream(sFMC_NAL_Tcp_data);
  assert(sFMC_NAL_Tcp_data.empty());
  
  //nothing should have happened
  assert(decoupActive == 0);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 0] == 0x0);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 1] == 0x0);
  assert(nalCtrl[NAL_CTRL_LINK_MRT_START_ADDR + 2] == 0x0);
  
  //"self reset"
  stepDut();
  stepDut(); //285
  
  // POST config via TCP with hangover
  use_sequential_hwicap = true;
  sequential_hwicap_address = HWICAP_SEQ_START_ADDRESS;
  uint32_t hwicap_in_address = HWICAP_SEQ_START_ADDRESS*4;

  // first chunk
  getStatus = "POST /configure HTTP/1.1\r\nUser-Agent: curl/7.47.01\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nffff000000bb11220044ffffffffffffffffaa9955662000000021000000220000002300000024000000250000002600000027000000280abcd";

  strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],"ffff000000bb11220044ffffffffffffffffaa9955662000000021000000220000002300000024000000250000002600000027000000280abcd");
  hwicap_in_address += strlen("ffff000000bb11220044ffffffffffffffffaa9955662000000021000000220000002300000024000000250000002600000027000000280abcd");

  printf("%s\n",getStatus);
  printf("\t....with large payload\n\n");

  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));
  int message_size = strlen(getStatus);

  sessId = Axis<16>(73);
  sessId.setTLast(1);
  sNAL_FMC_Tcp_sessId.write(sessId.getTData());

  HWICAP_seq_OUT[SR_OFFSET] = SR;
  HWICAP_seq_OUT[WF_OFFSET] = 0x42;
  HWICAP_seq_OUT[CR_OFFSET] = 0;
  //HWICAP_seq_OUT[WFV_OFFSET] = 0x1DE;
  HWICAP_seq_OUT[WFV_OFFSET] = 0x3FF;
  stepDut();
  stepDut();
  stepDut();
  assert(decoupActive == 1); //should be there immediately
  assert(HWICAP_seq_OUT[CR_OFFSET] == CR_WRITE);
  
  //second chunk
  getStatus = "e2900000020000000210000002200000023000000002400000025000000260002700123456789AB";
  // -> 194 payload bytes (first and second)

  //printf("%s\n",getStatus);

  strcpy(&httpBuffer[0],getStatus);
  strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],getStatus);
  hwicap_in_address += strlen(getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));
  message_size += strlen(getStatus);

  stepDut();
  printBuffer(bufferIn, "bufferIn after TCP HTTP transfer", 4);

  for(int j = 0; j< 3; j++)
  {
    stepDut();
  }

  //printBuffer(bufferOut, "Valid HTTP POST: BufferOut:",4);

  assert(decoupActive == 1);
  printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
  WF_should = 0x36373839;
  printf("WF_should: %#010x\n", WF_should);
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);

  assert(checkSeqHwicap((uint32_t*) HWICAP_seq_IN,HWICAP_seq_OUT,HWICAP_SEQ_START_ADDRESS,sequential_hwicap_address,false));

  //HWICAP_seq_OUT[WFV_OFFSET] = 0x2DE;
  //now, we post more than BUFFER_SIZE
  bool overflow_happened = false;
  while(message_size < IN_BUFFER_SIZE + 20)
  {
    //next chunk
    //getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // -> 80 payload bytes
    getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // --> 460 payload bytes
  
    strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],getStatus);
    hwicap_in_address += strlen(getStatus);

    strcpy(&httpBuffer[0],getStatus);
    copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));
    message_size += strlen(getStatus);
    //printf("------- message size %d ------------\n", (int) message_size);

    stepDut();
    assert(decoupActive == 1);
    //printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
    //WF_should = 0x38394142;
    WF_should = 0x36373839;
    //printf("WF_should: %#010x\n", WF_should);
    if(!(message_size > IN_BUFFER_SIZE) && !overflow_happened)
    {
      assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
    } else {
      overflow_happened = true;
    }
  }

  //write what's left
  stepDut();
  //to activate and see context, add +1 for the end address
  assert(checkSeqHwicap((uint32_t*) HWICAP_seq_IN,HWICAP_seq_OUT,HWICAP_SEQ_START_ADDRESS,sequential_hwicap_address,false));

  //test WFV feedback
  HWICAP_seq_OUT[WFV_OFFSET] = 0x3;
  //next chunk
  getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
  // -> 80 payload bytes
  
  strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],getStatus);
  hwicap_in_address += strlen(getStatus);

  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));
  message_size += strlen(getStatus);

  stepDut();
  assert(decoupActive == 1);
  printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
  WF_should = 0x67683230; //only 3 words
  printf("WF_should: %#010x\n", WF_should);
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  
  //now, we set HWICAP to blocking and wait until an overflow
  HWICAP_seq_OUT[WFV_OFFSET] = 0x0;
  overflow_happened = false;
  message_size = 0;
  while(message_size < IN_BUFFER_SIZE + 300)
  {
    //next chunk
    //getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // -> 240 payload bytes
    
    getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // --> 460 payload bytes

    strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],getStatus);
    hwicap_in_address += strlen(getStatus);

    strcpy(&httpBuffer[0],getStatus);
    copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));
    message_size += strlen(getStatus);
    //printf("------- message size %d ------------\n", (int) message_size);

    stepDut();
    assert(decoupActive == 1);
    printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
    //WF_should = 0x36373839;
    WF_should = 0x67683230; //nothing should change
    printf("WF_should: %#010x\n", WF_should);
    assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  }
  
  //one word only
  HWICAP_seq_OUT[WFV_OFFSET] = 0x1;
  stepDut();
  assert(decoupActive == 1);
  WF_should = 0x30303030;
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  stepDut();
  assert(decoupActive == 1);
  WF_should = 0x30303231;
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  //hangover test
  HWICAP_seq_OUT[WFV_OFFSET] = 0x1E1;
  stepDut();
  assert(decoupActive == 1);
  WF_should = 0x30303030;
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  //one word only
  HWICAP_seq_OUT[WFV_OFFSET] = 0x1;
  stepDut();
  assert(decoupActive == 1);
  WF_should = 0x30303232;
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  //now, we let it go
  HWICAP_seq_OUT[WFV_OFFSET] = 0x3FF;
  stepDut();
  stepDut();
  stepDut();
  stepDut();
  assert(decoupActive == 1);
  //printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
  WF_should = 0x36373839; //everything should be processed
  //printf("WF_should: %#010x\n", WF_should);
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  //to activate and see context, add +1 for the end address
  assert(checkSeqHwicap((uint32_t*) HWICAP_seq_IN,HWICAP_seq_OUT,HWICAP_SEQ_START_ADDRESS,sequential_hwicap_address,false));
  
#ifndef COSIM
  //test random pattern
  printf("----------- Random Test -----------\n");
  HWICAP_seq_OUT[WFV_OFFSET] = 0x3FF;
  message_size = 0;
  char stringBuffer[512];
  uint32_t max_message_size = 5*IN_BUFFER_SIZE + 300;
  bool correction_done = false;
  struct timeval now;
  gettimeofday(&now, NULL);
  uint32_t random_seed = now.tv_sec + now.tv_usec;
  printf("\tSeed %d\n", random_seed);
  srand(random_seed);
  while(message_size < max_message_size)
  {
    //next chunk
    //getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // -> 240 payload bytes
    
    getStatus = "abcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789ABabcdefgh20000000210000002200000023000000002400000025abcdef26ghijk2700123456789AB";
    // --> 460 payload bytes

    strcpy(stringBuffer,getStatus);

    //to check every packet size
    if(message_size > 1024 && message_size < (max_message_size - 1024) )
    {
      uint16_t stop_byte = rand() % (strlen(getStatus) - 1);
      //if(stop_byte < 2*NETWORK_WORD_BYTE_WIDTH)
      //{
      //  stop_byte += 2*NETWORK_WORD_BYTE_WIDTH;
      //}
      if(stop_byte < 3)
      {
        stop_byte += 3;
      }
      printf("cutting payload at %d\n", stop_byte);
      stringBuffer[stop_byte] = '\0';
    } else if( (message_size > (max_message_size - 1024)) && !correction_done)
    {
      correction_done = true;
      uint16_t stop_byte = 32 + ( 4 - (message_size % 4));
      printf("correcting payload with length %d\n", stop_byte);
      stringBuffer[stop_byte] = '\0';
    }
  
    strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],stringBuffer);
    hwicap_in_address += strlen(stringBuffer);

    strcpy(&httpBuffer[0],stringBuffer);
    copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(stringBuffer));
    message_size += strlen(stringBuffer);
    //printf("------- message size %d ------------\n", (int) message_size);

    stepDut();
    
    uint8_t multiple_iterations = rand() % 8;
    for(int k = 0; k < multiple_iterations; k++)
    {
      printf("multiple FMC calls with no input %d.\n",multiple_iterations);
      stepDut();
    }
    assert(decoupActive == 1);
    //we cant test the last word, but we can test the total
    assert(checkSeqHwicap((uint32_t*) HWICAP_seq_IN,HWICAP_seq_OUT,HWICAP_SEQ_START_ADDRESS,sequential_hwicap_address,false));
  }

  //to write all remaining stuff (including possible wrap around)
  stepDut();
  stepDut();
  assert(decoupActive == 1);
#endif

  //last chunk
  //HWICAP_seq_OUT[WFV_OFFSET] = 0x710;
  getStatus = "0012345678abcdefgh\r\n\r\n";
  // -> 18 payload bytes
  
  strcpy((char*) &HWICAP_seq_IN[hwicap_in_address],"0012345678abcdefgh");
  hwicap_in_address += strlen(getStatus);

  strcpy(&httpBuffer[0],getStatus);
  copyBufferToStream(httpBuffer,sNAL_FMC_Tcp_data,strlen(getStatus));

  stepDut();
  stepDut();
  //now, we are done
  assert(decoupActive == 0);
  printf("WF: %#010x\n",(int) HWICAP_seq_OUT[WF_OFFSET]);
  WF_should = 0x65666768;
  printf("WF_should: %#010x\n", WF_should);
  assert((int) HWICAP_seq_OUT[WF_OFFSET] == WF_should);
  
  //now, we can check the complete buffer, it should be also empty afterwards...
  assert(checkSeqHwicap((uint32_t*) HWICAP_seq_IN,HWICAP_seq_OUT,HWICAP_SEQ_START_ADDRESS,sequential_hwicap_address+8,false));

  //Check CR_WRITE 
  HWICAP_seq_OUT[CR_OFFSET] = 0;
  stepDut(); //to finish
  stepDut(); //334 (if buffer = 2048)
  succeded &= checkResult(MMIO, 0x30574154); //WAT
  assert(decoupActive == 0);
  assert(HWICAP_seq_OUT[CR_OFFSET] == 0);

  sessId_back = sFMC_NAL_Tcp_sessId.read();
  assert(sessId.getTData() == sessId_back.getTData());
  printf("Check stream:\n0x312e312f50545448\n0x0d4b4f2030303220\n");
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x312e312f50545448);
  assert(sFMC_NAL_Tcp_data.read().tdata == 0x0d4b4f2030303220);
  //drain remaining
  printf("Partial ");
  drainStream(sFMC_NAL_Tcp_data);
  assert(sFMC_NAL_Tcp_data.empty());

  //TODO: test if FMC stays operational after a failed request

  printf("== TCP Test passed == \n");


#ifndef COSIM
  return succeded? 0 : -1;
#else
  //since some vectors and their timestamps shift during cosim...
  return 0;
#endif
}


/*! \} */

