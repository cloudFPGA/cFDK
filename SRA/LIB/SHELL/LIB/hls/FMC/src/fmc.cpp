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
 * @file       : fmc.cpp
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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "fmc.hpp"
#include "http.hpp"

ap_uint<8> writeErrCnt = 0;
ap_uint<8> fifoEmptyCnt = 0;
ap_uint<8> fifoFullCnt = 0;
ap_uint<4> xmem_page_trans_cnt = 0xF;

stream<uint8_t> internal_icap_fifo ("sInternalIcapFifo");
stream<uint8_t> icap_hangover_fifo ("sIcapHangoverFifo");
IcapFsmState fsmHwicap = ICAP_FSM_RESET;
bool fifo_operation_in_progress = false;
uint8_t fifo_overflow_buffer[8];
bool process_fifo_overflow_buffer = false;
uint8_t fifo_overflow_buffer_length = 8;

uint8_t bufferIn[IN_BUFFER_SIZE];
uint32_t bufferInPtrWrite = 0x0;
uint32_t bufferInPtrMaxWrite = 0x0;
uint32_t lastSeenBufferInPtrMaxWrite = 0x0;
uint32_t bufferInPtrNextRead = 0x0;
bool tcp_write_only_fifo = false;

uint8_t bufferOut[OUT_BUFFER_SIZE];
uint16_t bufferOutPtrWrite = 0x0;
uint16_t bufferOutContentLength = 0x0;
uint16_t bufferOutPtrNextRead = 0x0;
uint16_t lastSeenBufferOutPtrNextRead = 0x0;


#ifndef __SYNTHESIS__
bool use_sequential_hwicap = false;
uint32_t sequential_hwicap_address = 0;
#endif

static char* httpNL = "\r\n";
HttpState httpState = HTTP_IDLE; 

ap_uint<32> Display1 = 0, Display2 = 0, Display3 = 0, Display4 = 0, Display5 = 0, Display6 = 0, Display7 = 0, Display8 = 0, Display9 = 0;
ap_uint<28> wordsWrittenToIcapCnt = 0;
ap_uint<28> tcp_words_received = 0;

ap_uint<32> nodeRank = 0;
ap_uint<32> clusterSize = 0; 

ap_uint<1> toDecoup_persistent = 0;

ap_uint<8> mpe_status_request_cnt = 0;
ap_uint<32> mpe_status[NRC_NUMBER_STATUS_WORDS];
bool mpe_status_disabled = false;

ap_uint<8> fpga_status[NUMBER_FPGA_STATE_REGISTERS];
ap_uint<32> ctrl_link_next_check_seconds = 0;
//bool ctrl_link_transfer_ongoing = false;
ap_uint<32> mrt_copy_index = 0;


ap_uint<16> current_role_mmio = 0;

ap_uint<32> fpga_time_seconds = 0;
ap_uint<32> fpga_time_minutes = 0;
ap_uint<32> fpga_time_hours   = 0;

ap_uint<7> lastResponsePageCnt = 0;
ap_uint<4> responePageCnt = 0; //for Display 4

//persistent states...
GlobalState currentGlobalOperation = GLOBAL_IDLE;
//bool ongoingTransfer_persistent = false;
bool streaming_mode_persistent = false;
bool transferError_persistent = false;
bool invalidPayload_persistent = false;
//bool failedPR_persistent = false;
//bool swapBytes_persistent = false;
bool globalOperationDone_persistent = false;
bool axi_wasnot_ready_persistent = false;
ap_uint<32> global_state_wait_counter_persistent = 0;

AppMeta currentTcpSessId = 0;
bool TcpSessId_updated_persistent = false;
ap_uint<1> tcpModeEnabled = 0;
uint8_t tcp_iteration_count = 0;
//bool tcp_rx_blocked_by_processing = false;

//flags 
ap_uint<1> flag_check_xmem_pattern = 0;
ap_uint<1> flag_silent_skip = 0;
ap_uint<1> last_xmem_page_received_persistent = 0;
ap_uint<1> flag_continuous_tcp_rx = 0;

//TCP FSMs
TcpFsmState fsmTcpSessId_RX = TCP_FSM_RESET;
TcpFsmState fsmTcpSessId_TX = TCP_FSM_RESET;
TcpFsmState fsmTcpData_RX = TCP_FSM_RESET;
TcpFsmState fsmTcpData_TX = TCP_FSM_RESET;
bool run_nested_loop_helper = false;
bool goto_done_if_idle_tcp_rx = false;
ap_uint<4> received_TCP_SessIds_cnt = 0;

//EOF detection
uint8_t last_3_chars[3];
ap_uint<2> detected_http_nl_cnt = 0;
ap_uint<2> target_http_nl_cnt = 0;
uint32_t positions_of_detected_http_nl[4];

//TCP hangover
uint8_t buffer_hangover_bytes[3];
bool hwicap_hangover_present = true;
uint8_t hwicap_hangover_size = 0;

//NRC "Backup"
bool tables_initialized = false;
ap_uint<32> current_MRT[MAX_MRT_SIZE];
ap_uint<32> current_nrc_config[NUMBER_CONFIG_WORDS];
ap_uint<32> current_nrc_mrt_version = 0;
bool need_to_update_nrc_mrt = false;
bool need_to_update_nrc_config = false;
LinkFsmStateType linkCtrlFSM = LINKFSM_WAIT;
ap_uint<32> max_discovered_node_id = 0;





uint8_t bytesToPages(int len, bool update_global_variables)
{
  printf("bytesToPages: len %d\n",len);

  uint8_t pageCnt = len/BYTES_PER_PAGE; 

  if(update_global_variables)
  {
    lastResponsePageCnt = len % BYTES_PER_PAGE;
    printf("lastResponsePageCnt: %d\n", (int) lastResponsePageCnt);
  }

  //we need to ceil the page count (if lastResponsePageCnt != 0....)
  if(len % BYTES_PER_PAGE > 0)
  {
    pageCnt++;
  } 

  if(update_global_variables)
  { // for Display4
    responePageCnt = pageCnt;
    printf("responePageCnt: %d\n", (int) responePageCnt);
  }

  assert(pageCnt <= MAX_PAGES);

  return pageCnt;

}


//void copyOutBuffer(ap_uint<4> numberOfPages, ap_uint<32> xmem[XMEM_SIZE], ap_uint<1> notToSwap)
void copyOutBuffer(ap_uint<4> numberOfPages, ap_uint<32> xmem[XMEM_SIZE])
{
  for(int i = 0; i < numberOfPages*LINES_PER_PAGE; i++)
  {
    ap_uint<32> tmp = 0; 

    tmp = ((ap_uint<32>) bufferOut[i*4 + 0]); 
    tmp |= ((ap_uint<32>) bufferOut[i*4 + 1]) << 8; 
    tmp |= ((ap_uint<32>) bufferOut[i*4 + 2]) << 16; 
    tmp |= ((ap_uint<32>) bufferOut[i*4 + 3]) << 24; 
    xmem[XMEM_ANSWER_START + i] = tmp;
  }

}


void emptyInBuffer()
{
  for(int i = 0; i < IN_BUFFER_SIZE; i++)
  {
//#pragma HLS unroll
    bufferIn[i] = 0x0;
  }
  bufferInPtrWrite = 0x0;
  bufferInPtrNextRead = 0x0;
  bufferInPtrMaxWrite = 0x0;
  lastSeenBufferInPtrMaxWrite = 0x0;
  hwicap_hangover_present = false;
  hwicap_hangover_size = 0;
  for(int i = 0; i < 3; i++)
  {
#pragma HLS unroll
    buffer_hangover_bytes[i] = 0x0;
  }
  while(!internal_icap_fifo.empty())
  {
    internal_icap_fifo.read();
  }
  while(!icap_hangover_fifo.empty())
  {
    icap_hangover_fifo.read();
  }
  tcp_write_only_fifo = false;
  fsmHwicap = ICAP_FSM_IDLE;
  fifo_operation_in_progress = false;
  for(int i = 0; i<8; i++)
  {
    fifo_overflow_buffer[i] = 0x0;
  }
  process_fifo_overflow_buffer = false;
  fifo_overflow_buffer_length = 0;
  printf("\t inBuffer cleaned\n");
}

void emptyOutBuffer()
{
  for(int i = 0; i < OUT_BUFFER_SIZE; i++)
  {
//#pragma HLS unroll
    bufferOut[i] = 0x0;
  }
  bufferOutPtrWrite = 0x0;
  bufferOutContentLength = 0x0;
  bufferOutPtrNextRead = 0x0;
  lastSeenBufferOutPtrNextRead = 0x0;
  printf("\t outBuffer cleaned\n");
}


uint32_t writeDisplaysToOutBuffer()
{
  uint32_t len  = 0;
  //Display1
  len = writeString("Status Display 1: ");
  len += writeUnsignedLong((uint32_t) Display1, 16);
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2; 

  //Display2
  len += writeString("Status Display 2: ");
  len += writeUnsignedLong((uint32_t) Display2, 16);
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2; 
  
  //Display 3 & 4 is less informative outside EMIF Context
  
  //insert rank and size
  len += writeString("Rank: ");
  len += writeUnsignedLong(nodeRank, 10); 
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2; 

  len += writeString("Size: ");
  len += writeUnsignedLong(clusterSize, 10);
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2; 

  //NRC status
  len += writeString("NRC Status (16 lines): \r\n"); //NRC_NUMBER_STATUS_WORDS

  for(int i = 0; i<NRC_NUMBER_STATUS_WORDS; i++)
  {
    if(i<=9)
    {
      bufferOut[bufferOutPtrWrite] = '0' + i; //poor mans ascii cast
    } else { 
      bufferOut[bufferOutPtrWrite] = '1';
      bufferOutPtrWrite++;
      len++;
      bufferOut[bufferOutPtrWrite] = '0' + (i-10);
    }
    bufferOutPtrWrite++;
    len++;
    len+=writeString(": ");
    if( mpe_status_disabled )
    {
      bufferOut[bufferOutPtrWrite + 3] = mpe_status[i] & 0xFF; 
      bufferOut[bufferOutPtrWrite + 2] = (mpe_status[i] >> 8) & 0xFF; 
      bufferOut[bufferOutPtrWrite + 1] = (mpe_status[i] >> 16) & 0xFF; 
      bufferOut[bufferOutPtrWrite + 0] = (mpe_status[i] >> 24) & 0xFF; 
      bufferOut[bufferOutPtrWrite + 4] = '\r'; 
      bufferOut[bufferOutPtrWrite + 5] = '\n'; 
      bufferOutPtrWrite  += 6;
      len += 6; 
    } else {
      len += writeUnsignedLong((uint32_t) mpe_status[i], 16);
      bufferOut[bufferOutPtrWrite + 0] = '\r'; 
      bufferOut[bufferOutPtrWrite + 1] = '\n'; 
      bufferOutPtrWrite  += 2;
      len += 2; 
    }
  }
  
  //FPGA status 
  len += writeString("FPGA Status (8 lines): \r\n"); //NUMBER_FPGA_STATE_REGISTERS
  for(int i = 0; i<NUMBER_FPGA_STATE_REGISTERS; i++)
  {
    if(i<=9)
    {
      bufferOut[bufferOutPtrWrite] = '0' + i; //poor mans ascii cast
    } else { 
      bufferOut[bufferOutPtrWrite] = '1';
      bufferOutPtrWrite++;
      len++;
      bufferOut[bufferOutPtrWrite] = '0' + (i-10);
    }
    bufferOutPtrWrite++;
    len++;
    len+=writeString(": ");
    //we have a bit size of 8
    uint8_t val = fpga_status[i];
    bufferOut[bufferOutPtrWrite] = (val > 9)? (val-10) + 'A' : val + '0'; //is only 8bit
    bufferOut[bufferOutPtrWrite + 1] = '\r'; 
    bufferOut[bufferOutPtrWrite + 2] = '\n'; 
    bufferOutPtrWrite  += 3;
    len += 3; 
  }
  
  //print uptime
  len += writeString("FPGA uptime: ");
  bufferOut[bufferOutPtrWrite + 0] = '0' + (fpga_time_hours / 10); 
  bufferOut[bufferOutPtrWrite + 1] = '0' + (fpga_time_hours % 10); 
  bufferOut[bufferOutPtrWrite + 2] = ':'; 
  bufferOutPtrWrite  += 3;
  len += 3;
  bufferOut[bufferOutPtrWrite + 0] = '0' + (fpga_time_minutes / 10); 
  bufferOut[bufferOutPtrWrite + 1] = '0' + (fpga_time_minutes % 10); 
  bufferOut[bufferOutPtrWrite + 2] = ':'; 
  bufferOutPtrWrite  += 3;
  len += 3;
  bufferOut[bufferOutPtrWrite + 0] = '0' + (fpga_time_seconds / 10); 
  bufferOut[bufferOutPtrWrite + 1] = '0' + (fpga_time_seconds % 10); 
  bufferOut[bufferOutPtrWrite + 2] = '\r'; 
  bufferOut[bufferOutPtrWrite + 3] = '\n'; 
  bufferOutPtrWrite  += 4;
  len += 3;
  
  //print cFDK version 
  len += writeString("cFDK/FMC version: ");
  len += writeString(CFDK_VERSION_STRING);
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2;
  
  //print Role version vector
  len += writeString("current ROLE version: ");
  len += writeUnsignedLong((uint16_t) current_role_mmio, 16);
  bufferOut[bufferOutPtrWrite + 0] = '\r'; 
  bufferOut[bufferOutPtrWrite + 1] = '\n'; 
  bufferOutPtrWrite  += 2;
  len += 2;

  
  len += writeString(httpNL);
  return len;
}

void setRank(ap_uint<32> newRank)
{
  nodeRank = newRank; 
  need_to_update_nrc_config = true;
  //nothing else to do so far 
}

void setSize(ap_uint<32> newSize)
{
  clusterSize = newSize;
  //nothing else to do so far 
}

ap_uint<4> copyAndCheckBurst(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt, ap_uint<7> lastPageCnt_in)
{

  ap_uint<8> curHeader = 0;
  ap_uint<8> curFooter = 0;

  ap_int<16> buff_pointer = 0-1;


  for(int i = 0; i<LINES_PER_PAGE; i++)
  {
    ap_uint<32> tmp = 0;
    tmp = xmem[i];

    if ( i == 0 )
    {
      curHeader = tmp & 0xff;
    } else {
      buff_pointer++;
      bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp & 0xff);
    }

    buff_pointer++;
    bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 8) & 0xff;
    buff_pointer++;
    bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 16 ) & 0xff;

    if ( i == LINES_PER_PAGE-1) 
    {
      curFooter = (tmp >> 24) & 0xff;
    } else {
      buff_pointer++;
      bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 24) & 0xff;
    }
  }


  ap_uint<4> curCnt = curHeader & 0xf; 


  if ( curHeader != curFooter)
  {//we are in the middle of a transfer!
    return 1;
  }

  if(curCnt == xmem_page_trans_cnt)
  {//page was already transfered
    return 0;
  }

  if (curCnt != ExpCnt)
  {//we must missed something 
    return 2;
  }

  bool lastPage = (curHeader & 0xf0) == 0xf0;

  //now we have a clean transfer
  if(lastPage && flag_check_xmem_pattern == 0)
  { //we didn't received a full page;
    bufferInPtrWrite += (lastPageCnt_in -1); //-1, because the address is 0 based!
    printf("lastPageCnt_in %d\n", (int) lastPageCnt_in);
  } else {
    bufferInPtrWrite += buff_pointer;// +1;
  }
  bufferInPtrMaxWrite = bufferInPtrWrite;
  bufferInPtrWrite++;

  if (bufferInPtrWrite >= (IN_BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE)) 
  { 
    bufferInPtrWrite = 0;
  }

  if(flag_check_xmem_pattern == 1)
  {
    ap_uint<8> ctrlByte = (((ap_uint<8>) ExpCnt) << 4) | ExpCnt;

    //printf("ctrlByte: %#010x\n",(int) ctrlByte);

    //for simplicity check only lines in between and skip potentiall hangover 
    for(int i = 3; i< PAYLOAD_BYTES_PER_PAGE -3; i++)
    {
      if(bufferIn[bufferInPtrNextRead  + i] != ctrlByte)
      {//data is corrupt 
        //printf("corrupt at %d with %#010x\n",i, (int) bufferIn[bufferInPtrWrite + i]);
        return 3;
      }
    }
    bufferInPtrNextRead = bufferInPtrWrite; //only for check pattern case
  }

  if (lastPage)
  {
    return 4;
  }

  return 5;
}


void fmc(
    //EMIF Registers
    ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
    //state of the FPGA
    ap_uint<1> *layer_4_enabled,
    ap_uint<1> *layer_6_enabled,
    ap_uint<1> *layer_7_enabled,
    //get FPGA time
    ap_uint<32> *in_time_seconds,
    ap_uint<32> *in_time_minutes,
    ap_uint<32> *in_time_hours,
    //get ROLE version
    ap_uint<16> *role_mmio_in,
    //HWICAP and DECOUPLING
    ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
    // Soft Reset
    ap_uint<1> *setSoftReset,
    //XMEM
    ap_uint<32> xmem[XMEM_SIZE], 
    //NRC
    ap_uint<32> nrcCtrl[NRC_CTRL_LINK_SIZE],
    ap_uint<1> *disable_ctrl_link,
    stream<TcpWord>             &siNRC_Tcp_data,
    stream<AppMeta>           &siNRC_Tcp_SessId,
    stream<TcpWord>             &soNRC_Tcp_data,
    stream<AppMeta>           &soNRC_Tcp_SessId,
#ifdef INCLUDE_PYROLINK
    //Pyrolink
    stream<Axis<8> >  &soPYROLINK,
    stream<Axis<8> >  &siPYROLINK,
    ap_uint<1> *disable_pyro_link,
#endif
    //TO ROLE
    ap_uint<32> *role_rank, ap_uint<32> *cluster_size)
{
//#pragma HLS RESOURCE variable=bufferIn core=RAM_2P_BRAM
//#pragma HLS RESOURCE variable=bufferIn core=RAM_S2P_LUTRAM
//#pragma HLS RESOURCE variable=bufferOut core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=xmem core=RAM_1P_BRAM
#pragma HLS INTERFACE m_axi depth=512 port=HWICAP bundle=boHWICAP
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_vld register port=layer_4_enabled name=piLayer4enabled
#pragma HLS INTERFACE ap_vld register port=layer_6_enabled name=piLayer6enabled
#pragma HLS INTERFACE ap_vld register port=layer_7_enabled name=piLayer7enabled
#pragma HLS INTERFACE ap_vld register port=in_time_seconds name=piTime_seconds
#pragma HLS INTERFACE ap_vld register port=in_time_minutes name=piTime_minutes
#pragma HLS INTERFACE ap_vld register port=in_time_hours   name=piTime_hours
#pragma HLS INTERFACE ap_vld register port=role_mmio_in    name=piRole_mmio
#pragma HLS INTERFACE ap_stable register port=decoupStatus name=piDECOUP_status
#pragma HLS INTERFACE ap_ovld register port=setDecoup name=poDECOUP_activate
#pragma HLS INTERFACE ap_ovld register port=role_rank name=poROLE_rank
#pragma HLS INTERFACE ap_ovld register port=cluster_size name=poROLE_size
#pragma HLS INTERFACE m_axi depth=16383 port=nrcCtrl bundle=boNRC_ctrlLink 
  //max_read_burst_length=1 max_write_burst_length=1 //0x3fff - 0x2000
#pragma HLS INTERFACE ap_stable register port=disable_ctrl_link name=piDisableCtrlLink
#pragma HLS INTERFACE ap_ovld register port=setSoftReset name=poSoftReset 

#ifdef INCLUDE_PYROLINK
#pragma HLS INTERFACE ap_fifo register both port=soPYROLINK
#pragma HLS INTERFACE ap_fifo register both port=siPYROLINK
#pragma HLS INTERFACE ap_stable register port=disable_pyro_link name=piDisablePyroLink
#endif

#pragma HLS INTERFACE ap_fifo port=siNRC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=soNRC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=siNRC_Tcp_SessId
#pragma HLS INTERFACE ap_fifo port=soNRC_Tcp_SessId
//ap_ctrl is default (i.e. ap_hs)
//#pragma HLS DATAFLOW TODO: crashes Vivado..

#pragma HLS STREAM variable=internal_icap_fifo depth=4096
#pragma HLS STREAM variable=icap_hangover_fifo depth=3

#pragma HLS reset variable=mpe_status_request_cnt
#pragma HLS reset variable=httpState
#pragma HLS reset variable=bufferInPtrWrite
#pragma HLS reset variable=bufferInPtrMaxWrite
#pragma HLS reset variable=lastSeenBufferInPtrMaxWrite
#pragma HLS reset variable=bufferInPtrNextRead
#pragma HLS reset variable=tcp_write_only_fifo
#pragma HLS reset variable=bufferOutPtrWrite
#pragma HLS reset variable=bufferOutContentLength
#pragma HLS reset variable=bufferOutPtrNextRead
#pragma HLS reset variable=lastSeenBufferOutPtrNextRead
#pragma HLS reset variable=writeErrCnt
#pragma HLS reset variable=fifoEmptyCnt
#pragma HLS reset variable=fifoFullCnt
#pragma HLS reset variable=wordsWrittenToIcapCnt
#pragma HLS reset variable=fsmHwicap
#pragma HLS reset variable=fifo_operation_in_progress
#pragma HLS reset variable=tcp_words_received
#pragma HLS reset variable=fifo_overflow_buffer_length
#pragma HLS reset variable=process_fifo_overflow_buffer
#pragma HLS reset variable=globalOperationDone_persistent
#pragma HLS reset variable=transferError_persistent
#pragma HLS reset variable=invalidPayload_persistent
#pragma HLS reset variable=flag_check_xmem_pattern
#pragma HLS reset variable=flag_silent_skip
#pragma HLS reset variable=toDecoup_persistent
#pragma HLS reset variable=lastResponsePageCnt
#pragma HLS reset variable=responePageCnt
#pragma HLS reset variable=xmem_page_trans_cnt
#pragma HLS reset variable=last_xmem_page_received_persistent
#pragma HLS reset variable=flag_continuous_tcp_rx
#pragma HLS reset variable=axi_wasnot_ready_persistent
#pragma HLS reset variable=global_state_wait_counter_persistent
#pragma HLS reset variable=currentTcpSessId
#pragma HLS reset variable=TcpSessId_updated_persistent
#pragma HLS reset variable=tcpModeEnabled
#pragma HLS reset variable=tcp_iteration_count
#pragma HLS reset variable=fsmTcpSessId_TX
#pragma HLS reset variable=fsmTcpSessId_RX
#pragma HLS reset variable=fsmTcpData_TX
#pragma HLS reset variable=fsmTcpData_RX
#pragma HLS reset variable=run_nested_loop_helper
#pragma HLS reset variable=goto_done_if_idle_tcp_rx
#pragma HLS reset variable=received_TCP_SessIds_cnt

#pragma HLS reset variable=current_nrc_mrt_version
#pragma HLS reset variable=current_MRT
#pragma HLS reset variable=current_nrc_config
#pragma HLS reset variable=nodeRank
#pragma HLS reset variable=clusterSize
#pragma HLS reset variable=tables_initialized
#pragma HLS reset variable=mpe_status_disabled
#pragma HLS reset variable=need_to_update_nrc_mrt
#pragma HLS reset variable=need_to_update_nrc_config
#pragma HLS reset variable=ctrl_link_next_check_seconds
#pragma HLS reset variable=mrt_copy_index
//#pragma HLS reset variable=ctrl_link_transfer_ongoing
#pragma HLS reset variable=linkCtrlFSM
#pragma HLS reset variable=max_discovered_node_id
#pragma HLS reset variable=detected_http_nl_cnt
#pragma HLS reset variable=target_http_nl_cnt
#pragma HLS reset variable=hwicap_hangover_present
#pragma HLS reset variable=hwicap_hangover_size




  //===========================================================
  // Core-wide variables
  ap_uint<32> SR = 0, HWICAP_Done = 0, EOS = 0, CR = 0, CR_value = 0, ASR = 0;
  ap_uint<8> ASW1 = 0, ASW2 = 0, ASW3 = 0, ASW4= 0;
  
  ap_uint<32> ISR = 0, WFV = 0; // RFO = 0;
  ap_uint<32> WFV_value = 0, WEMPTY = 0;

  ap_uint<4> expCnt = 0;
  ap_uint<4> copyRet = 0;


  ap_uint<1> pyroSendRequestBit = 0;
  char msg_buf[4] = {0x49,0x44,0x4C,'\n'}; //IDL
  char *msg = &msg_buf[0];

  OprvType lastReturnValue = 0x0;
  uint8_t currentProgramLength = 0;
  OpcodeType opcodeProgram[MAX_PROGRAM_LENGTH];
  OprvType   programMask[MAX_PROGRAM_LENGTH];

  if(!tables_initialized)
  {
    for(int i = 0; i < MAX_MRT_SIZE; i++)
    {
//#pragma HLS unroll
      current_MRT[i] = 0x0;
    }
    for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    {
//#pragma HLS unroll
      current_nrc_config[i] = 0x0;
    }
    for(int i = 0; i < NUMBER_FPGA_STATE_REGISTERS; i++)
    {
//#pragma HLS unroll
      fpga_status[i] = 0x0;
    }
    for(int i = 0; i < 2; i++)
    {
//#pragma HLS unroll
      last_3_chars[i] = 0x0;
    }
    for(int i = 0; i < 3; i++)
    {
//#pragma HLS unroll
      buffer_hangover_bytes[i] = 0x0;
      positions_of_detected_http_nl[i] = 0x0;
    }
    tables_initialized = true;
  }

  // ++++++++++++++++++ evaluate outside world ++++++++++++++++
  //===========================================================
  // Connection to HWICAP
  // essential data (ask every time)
  SR = HWICAP[SR_OFFSET];
  CR = HWICAP[CR_OFFSET];
  ASR = HWICAP[ASR_OFFSET];
  ASW1 = ASR & 0xFF;
  ASW2 = (ASR & 0xFF00) >> 8;
  ASW3 = (ASR & 0xFF0000) >> 16;
  ASW4 = (ASR & 0xFF000000) >> 24;
  
  HWICAP_Done = SR & 0x1;
  EOS  = (SR & 0x4) >> 2;
  CR_value = CR & 0x1F; 
    
  ISR = HWICAP[ISR_OFFSET];
  WFV = HWICAP[WFV_OFFSET];

  //RFO = HWICAP[RFO_OFFSET];
  WEMPTY = (ISR & 0x4) >> 2;
  WFV_value = WFV & 0x7FF;


  ap_uint<1> wasAbort = (CR_value & CR_ABORT) >> 4;
  //Maybe CR_ABORT is not set 
  if( ASW1 != 0x00)
  {
    wasAbort = 1;
  }


  //MMIO commands
  ap_uint<32> MMIO_in_LE = 0x0;
  MMIO_in_LE  = (ap_uint<32>) ((*MMIO_in >> 24) & 0xFF);
  MMIO_in_LE |= (ap_uint<32>) ((*MMIO_in >> 8) & 0xFF00);
  MMIO_in_LE |= (ap_uint<32>) ((*MMIO_in << 8) & 0xFF0000);
  MMIO_in_LE |= (ap_uint<32>) ((*MMIO_in << 24) & 0xFF000000);

  ap_uint<1> checkPattern = (MMIO_in_LE >> CHECK_PATTERN_SHIFT ) & 0b1;

  ap_uint<1> parseHTTP = (MMIO_in_LE >> PARSE_HTTP_SHIFT) & 0b1;

  ap_uint<1> notToSwap = (MMIO_in_LE >> SWAP_N_SHIFT) & 0b1;
  //Turns out: we need to swap => active low

  ap_uint<1> startXmemTrans = (MMIO_in_LE >> START_SHIFT) & 0b1;


  ap_uint<1> pyroReadReq = (MMIO_in_LE >> PYRO_READ_REQUEST_SHIFT) & 0b1;

  ap_uint<1> tcpModeStart = (MMIO_in_LE >> ENABLE_TCP_MODE_SHIFT) & 0b1;

  ap_uint<1> pyroRecvMode = (MMIO_in_LE >> PYRO_MODE_SHIFT) & 0b1;

  ap_uint<7> lastPageCnt_in = (MMIO_in_LE >> LAST_PAGE_CNT_SHIFT) & 0x7F;

  ap_uint<1> manuallyToDecoup = (MMIO_in_LE >> DECOUP_CMD_SHIFT) & 0b1; 

  ap_uint<1> CR_isWriting = CR_value & CR_WRITE;

  ap_uint<1> reset_from_psoc = (MMIO_in_LE >> RST_SHIFT) & 0b1;

#ifdef INCLUDE_PYROLINK
  if(*disable_pyro_link == 0)
  {
    pyroSendRequestBit = siPYROLINK.empty()? 0 : 1;
  }
#else
  pyroSendRequestBit = 0;
#endif

  fpga_status[FPGA_STATE_LAYER_4] = *layer_4_enabled;
  fpga_status[FPGA_STATE_LAYER_6] = *layer_6_enabled;
  fpga_status[FPGA_STATE_LAYER_7] = *layer_7_enabled;
  fpga_status[FPGA_STATE_CONFIG_UPDATE] = need_to_update_nrc_config;
  fpga_status[FPGA_STATE_MRT_UPDATE] = need_to_update_nrc_mrt;

  fpga_time_seconds = *in_time_seconds;
  fpga_time_minutes = *in_time_minutes;
  fpga_time_hours   = *in_time_hours;

  current_role_mmio = *role_mmio_in;
  
  
  // ++++++++++++++++++ IMMEDIATE ACTIONS +++++++++++++++++++
  //===========================================================

  // ++++++++++++++++++ MAKE THE PLAN  +++++++++++++++++++
  //===========================================================


  bool iterate_again = true;
  while(iterate_again) 
  { 
    iterate_again = false;
    switch(currentGlobalOperation)
    {
      default: //no break, just for default
      case GLOBAL_IDLE: 
        //general stuff before a new global operation
        globalOperationDone_persistent = false;
        transferError_persistent = false;
        invalidPayload_persistent = false;
        last_xmem_page_received_persistent = 0;
        axi_wasnot_ready_persistent = false;
        global_state_wait_counter_persistent = 0;
        tcpModeEnabled = 0;
        flag_continuous_tcp_rx = 0;
        if(reset_from_psoc == 1)
        { 
          tcp_iteration_count = 0;
          //tcp_rx_blocked_by_processing = false;
          writeErrCnt = 0;
          fifoEmptyCnt = 0;
          fifoFullCnt = 0;
          //wordsWrittenToIcapCnt = 0;
          iterate_again = false;
          //hwicap_waiting_for_tcp = false;
          break;
        }
        //ongoingTransfer_persistent = false;
        currentProgramLength = 0;
        //looking for a new job...but with priorities
        if(tcpModeStart == 1 && parseHTTP == 1)
        {
          currentGlobalOperation = GLOBAL_TCP_HTTP;
          tcp_iteration_count++;
          
          TcpSessId_updated_persistent = false;
          //reset TX TCP FSMs
          fsmTcpSessId_TX = TCP_FSM_RESET;
          fsmTcpData_TX = TCP_FSM_RESET;

          currentTcpSessId = 0;
          lastResponsePageCnt = 0;
          iterate_again = true;
          httpState = HTTP_IDLE;
          reqType = REQ_INVALID;
          lastResponsePageCnt = 0;
          
          opcodeProgram[0] = OP_CLEAR_IN_BUFFER;
          programMask[0] = MASK_ALWAYS;
          opcodeProgram[1] = OP_CLEAR_OUT_BUFFER;
          programMask[1] = MASK_ALWAYS;
          opcodeProgram[2] = OP_TCP_CNT_RESET;
          programMask[2] = MASK_ALWAYS;
          currentProgramLength = 3;
        }
        else if(tcpModeStart == 1)
        {
          //This was GLOBAL_TCP_TO_HWICAP, but is now NOP
          currentGlobalOperation = GLOBAL_IDLE;
        } else if(parseHTTP == 1 && startXmemTrans == 1)
        {
          httpState = HTTP_IDLE;
          reqType = REQ_INVALID;
          xmem_page_trans_cnt = 0xf;
          lastResponsePageCnt = 0;

          currentGlobalOperation = GLOBAL_XMEM_HTTP;
          opcodeProgram[0] = OP_CLEAR_IN_BUFFER;
          programMask[0] = MASK_ALWAYS;
          opcodeProgram[1] = OP_CLEAR_OUT_BUFFER;
          programMask[1] = MASK_ALWAYS;
          currentProgramLength = 2;
          iterate_again = true;
        } else if(checkPattern == 1 && startXmemTrans == 1)
        {
          xmem_page_trans_cnt = 0xf;

          currentGlobalOperation = GLOBAL_XMEM_CHECK_PATTERN;
          //only in the beginning! 
          opcodeProgram[0] = OP_CLEAR_IN_BUFFER;
          programMask[0] = MASK_ALWAYS;
          currentProgramLength = 1;
          iterate_again = true;
        } else if(startXmemTrans == 1)
        {
          xmem_page_trans_cnt = 0xf;
          lastResponsePageCnt = 0;

          currentGlobalOperation = GLOBAL_XMEM_TO_HWICAP;
          opcodeProgram[0] = OP_CLEAR_IN_BUFFER;
          programMask[0] = MASK_ALWAYS;
          currentProgramLength = 1;
          iterate_again = true;
#ifdef INCLUDE_PYROLINK
        } else if(pyroRecvMode == 1 && *disable_pyro_link == 0)
        {
          currentGlobalOperation = GLOBAL_PYROLINK_RECV; //i.e. Coaxium to FPGA
          xmem_page_trans_cnt = 0xf;

          opcodeProgram[0] = OP_CLEAR_IN_BUFFER;
          programMask[0] = MASK_ALWAYS;
          currentProgramLength = 1;
          iterate_again = true;

        } else if(pyroReadReq == 1 && *disable_pyro_link == 0)
        {
          currentGlobalOperation = GLOBAL_PYROLINK_TRANS; //i.e. FPGA to Coaxium
          
          opcodeProgram[0] = OP_CLEAR_OUT_BUFFER;
          programMask[0] = MASK_ALWAYS;
          currentProgramLength = 1;
          iterate_again = true;
#endif
        } else if(manuallyToDecoup == 1)
        {
          currentGlobalOperation = GLOBAL_MANUAL_DECOUPLING;
          iterate_again = true;
        } else { 
          //else...we just do the daily jobs...
          //currentProgramLength = 0;
          msg = "BOR"; //BOR..ING 
        }
        //need to break to reevaluate switch-case
        break;

      case GLOBAL_XMEM_CHECK_PATTERN:
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        if(globalOperationDone_persistent)
        { // means, we have this operation done and wait for status change 
          if(startXmemTrans == 0)
          {
            currentGlobalOperation = GLOBAL_IDLE;
          }
          //do nothing
          msg="SUC"; //maintain SUC message (for XMEM transfer)
          currentProgramLength = 0;
          break;
        }
        if(transferError_persistent)
        {
          //do not change state?
          msg="ERR";
          currentProgramLength=0;
          break;
        }

        opcodeProgram[currentProgramLength] = OP_ENABLE_XMEM_CHECK_PATTERN;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
        programMask[currentProgramLength] = MASK_ALWAYS; 
        currentProgramLength++;

        //execute multiple times if not OPRV_DONE
        for(uint8_t i = 0; i<15; i++)
        {
          opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
          programMask[currentProgramLength] = OPRV_OK | OPRV_PARTIAL_COMPLETE | OPRV_NOT_COMPLETE; 
          currentProgramLength++;
        }

        //to switch from skipped to done 
        //we skip for FAIL or DONE --> in both cases we won't continue and msg is set accordingly
        opcodeProgram[currentProgramLength] = OP_DONE;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;

        break;

      case GLOBAL_XMEM_TO_HWICAP:
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        if(globalOperationDone_persistent)
        { // means, we have this operation done and wait for status change 
          if(startXmemTrans == 0)
          {
            currentGlobalOperation = GLOBAL_IDLE;
          }
          //do nothing else
          currentProgramLength=0;
          msg="SUC"; //maintain SUC message (for XMEM transfer)
          break;
        }
        if(wasAbort == 1)
        {
          //do not change state?
          msg="ABR";
          currentProgramLength=0;
          break;
        }
        if(transferError_persistent)
        {
          //do not change state?
          msg="ERR";
          currentProgramLength=0;
          break;
        }

        opcodeProgram[currentProgramLength] = OP_ACTIVATE_DECOUP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
        programMask[currentProgramLength] = MASK_ALWAYS; 
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_BUFFER_TO_HWICAP;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
        currentProgramLength++;
        //done for now
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_OK | OPRV_NOT_COMPLETE;
        currentProgramLength++;
        //done at all or fail (?)
        opcodeProgram[currentProgramLength] = OP_DEACTIVATE_DECOUP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_CLEAR_ROUTING_TABLE;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ABORT_HWICAP;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_FAIL;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;

        break;
      
      case GLOBAL_XMEM_HTTP:
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        if(globalOperationDone_persistent)
        { // means, we have this operation done and wait for status change 
          if(startXmemTrans == 0)
          {
            currentGlobalOperation = GLOBAL_IDLE;
          }
          //do nothing
          currentProgramLength = 0;
          msg="SUC"; //maintain SUC message (for XMEM transfer)
          break;
        }
        if(wasAbort == 1)
        {
          //do not change state?
          msg="ABR";
          currentProgramLength=0;
          break;
        }
        if(transferError_persistent)
        {
          //do not change state?
          msg="ERR";
          currentProgramLength=0;
          break;
        }
        
        if(reqType == POST_ROUTING)
        {
          opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
          programMask[currentProgramLength] = MASK_ALWAYS; 
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT; //wait for data or fatal failure
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE | OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_BUFFER_TO_ROUTING;
          programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_DONE | OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          break;
        }
        if(reqType == POST_CONFIG)
        {//decoup already activated
          opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
          programMask[currentProgramLength] = MASK_ALWAYS; 
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT; //wait for data of fatal failure
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE | OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_BUFFER_TO_HWICAP;
          programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
          currentProgramLength++;
          //wait for next junk
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_OK | OPRV_NOT_COMPLETE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          //failed?
          opcodeProgram[currentProgramLength] = OP_ABORT_HWICAP;
          programMask[currentProgramLength] = OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          //restore RV
          opcodeProgram[currentProgramLength] = OP_FAIL;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_FAIL;
          currentProgramLength++;
          //Reconfiguration done 
          opcodeProgram[currentProgramLength] = OP_DEACTIVATE_DECOUP;
          programMask[currentProgramLength] = OPRV_SKIPPED;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_CLEAR_ROUTING_TABLE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DONE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          break;
        }

        //else...first run?
        opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
        programMask[currentProgramLength] = MASK_ALWAYS; 
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT; //wait for data or fatal failure
        programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE | OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP; //If we just need to create a response
        programMask[currentProgramLength] = OPRV_USER;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM; //send if necessary
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT; //exit after send
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_COPY_REQTYPE_TO_RETURN;
        programMask[currentProgramLength] = OPRV_PARTIAL_COMPLETE;
        currentProgramLength++;
        //if skipped --> OK or not complete --> more data, also exit
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        //not exit --> RV is request type, only POST_CONFIG or POST_ROUTING remains 
        opcodeProgram[currentProgramLength] = OP_BUFFER_TO_ROUTING;
        programMask[currentProgramLength] = POST_ROUTING;
        currentProgramLength++;
        //if done --> send reply
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        //here neither routing not complete or post config
        opcodeProgram[currentProgramLength] = OP_COPY_REQTYPE_TO_RETURN;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        //if routing --> we need more data (and erase the RV)
        opcodeProgram[currentProgramLength] = OP_OK;
        programMask[currentProgramLength] = POST_ROUTING;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        //now, it is POST configure
        opcodeProgram[currentProgramLength] = OP_ACTIVATE_DECOUP;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        // we lost the information of DONE in the RV, but it is also stored in last_xmem_page_received_persistent
        opcodeProgram[currentProgramLength] = OP_BUFFER_TO_HWICAP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        //wait for next junk
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_OK | OPRV_NOT_COMPLETE;
        currentProgramLength++;
        //Reconfiguration done 
        opcodeProgram[currentProgramLength] = OP_DEACTIVATE_DECOUP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_CLEAR_ROUTING_TABLE;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        //if fail --> abort
        opcodeProgram[currentProgramLength] = OP_ABORT_HWICAP;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        //restore RV
        opcodeProgram[currentProgramLength] = OP_FAIL;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_OK;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;

        break;

      case GLOBAL_TCP_HTTP:
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          tcp_iteration_count = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        //TODO: implement timeout for NOT complete requests
        tcpModeEnabled = 1;
        msg="TCP";
        
        if(globalOperationDone_persistent)
        { 
          //"self reset" --> start over 
          //but wait until TX TCP FSMs have finished
          if( fsmTcpSessId_TX != TCP_FSM_PROCESS_DATA
              && fsmTcpData_TX != TCP_FSM_PROCESS_DATA)
          {
            currentGlobalOperation = GLOBAL_IDLE;
            iterate_again = true;
            msg="SUC"; //maintain SUC message
            //do nothing else
            currentProgramLength=0;
          } else {
            printf("\t\t\tWaiting for TCP FSMs to finish...\n");
            msg="WFF"; //Wait for FSM 

            //give the FSM the option to update Flags
            currentProgramLength=0;
            opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
            programMask[currentProgramLength] = MASK_ALWAYS;
            currentProgramLength++;
            opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
            programMask[currentProgramLength] = MASK_ALWAYS;
            currentProgramLength++;
          }
          break;
        }
        if(wasAbort == 1)
        {
          //do not change state?
          msg="ABR";
          currentProgramLength=0;
          break;
        }
        if(transferError_persistent)
        {
          //TODO: self reset? 
          //do not change state?
          msg="ERR";
          currentProgramLength=0;
          break;
        }
        printf("GLOBAL_TCP_HTTP: make a plan with REQUEST_TYPE: %d\n", (int) reqType);

        if(!TcpSessId_updated_persistent && !globalOperationDone_persistent)
        {
          opcodeProgram[currentProgramLength] = OP_WAIT_FOR_TCP_SESS;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE;
          currentProgramLength++;
          //clearing the buffers is done by the "self reset" above
        }
        
        if(reqType == POST_ROUTING)
        {//cont tcp already activated
          opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_FILL_BUFFER_TCP;
          programMask[currentProgramLength] = MASK_ALWAYS; 
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT; //wait for data or fatal failure
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE | OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_BUFFER_TO_ROUTING;
          programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_DONE | OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DEACTIVATE_CONT_TCP;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          break;
        }
        if(reqType == POST_CONFIG)
        {//decoup already activated
          //cont tcp already activated
          opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_FIFO_TO_HWICAP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT; //we have smth left to do
          programMask[currentProgramLength] = OPRV_OK | OPRV_NOT_COMPLETE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          //failed?
          opcodeProgram[currentProgramLength] = OP_ABORT_HWICAP;
          programMask[currentProgramLength] = OPRV_FAIL;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          //restore RV
          opcodeProgram[currentProgramLength] = OP_FAIL;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_FAIL;
          currentProgramLength++;
          //Reconfiguration done 
          opcodeProgram[currentProgramLength] = OP_DEACTIVATE_DECOUP;
          programMask[currentProgramLength] = OPRV_SKIPPED;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_CLEAR_ROUTING_TABLE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DONE;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
          programMask[currentProgramLength] = OPRV_OK;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_DEACTIVATE_CONT_TCP;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_DONE;
          currentProgramLength++;
          break;
        }

        //else...first run?
        opcodeProgram[currentProgramLength] = OP_FILL_BUFFER_TCP;
        programMask[currentProgramLength] = MASK_ALWAYS; 
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT; //wait for data or fatal failure
        programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE | OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP; //If we just need to create a response
        programMask[currentProgramLength] = OPRV_USER;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP; //send if necessary
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT; //exit after send
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_COPY_REQTYPE_TO_RETURN;
        programMask[currentProgramLength] = OPRV_PARTIAL_COMPLETE;
        currentProgramLength++;
        //if skipped --> OK or not complete --> more data, also exit
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        //not exit --> RV is request type, only POST_CONFIG or POST_ROUTING remains 
        opcodeProgram[currentProgramLength] = OP_BUFFER_TO_ROUTING;
        programMask[currentProgramLength] = POST_ROUTING;
        currentProgramLength++;
        //if done --> send reply
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        //here neither routing not complete or post config
        //in all cases: we need continuous TCP and expect a payload
        opcodeProgram[currentProgramLength] = OP_ACTIVATE_CONT_TCP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_TCP_RX_STOP_ON_EOP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_COPY_REQTYPE_TO_RETURN;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        //if routing --> we need more data (and erase the RV)
        opcodeProgram[currentProgramLength] = OP_OK;
        programMask[currentProgramLength] = POST_ROUTING;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        //now, it is POST configure
        opcodeProgram[currentProgramLength] = OP_ACTIVATE_DECOUP;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        // we lost the information of DONE in the RV, but it is also stored in last_xmem_page_received_persistent or in detected_http_nl_cnt
        opcodeProgram[currentProgramLength] = OP_FIFO_TO_HWICAP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT; //we have smth left to do
        programMask[currentProgramLength] = OPRV_OK | OPRV_NOT_COMPLETE;
        currentProgramLength++;
        //Reconfiguration done
        opcodeProgram[currentProgramLength] = OP_DEACTIVATE_DECOUP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_CLEAR_ROUTING_TABLE;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DEACTIVATE_CONT_TCP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DISABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        //if fail --> abort
        opcodeProgram[currentProgramLength] = OP_ABORT_HWICAP;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_UPDATE_HTTP_STATE;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_HANDLE_HTTP;
        programMask[currentProgramLength] = OPRV_OK;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_TCP_SESS;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_TCP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_DEACTIVATE_CONT_TCP;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        //restore RV
        opcodeProgram[currentProgramLength] = OP_FAIL;
        programMask[currentProgramLength] = OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_FAIL;
        currentProgramLength++;
        //in all other cases: we need to wait for data
        opcodeProgram[currentProgramLength] = OP_OK;
        programMask[currentProgramLength] = OPRV_SKIPPED;
        currentProgramLength++;
        break;

      case GLOBAL_PYROLINK_RECV: //i.e. Coaxium to FPGA
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        if(globalOperationDone_persistent)
        { // means, we have this operation done and wait for status change 
          if(pyroRecvMode == 0)
          {
            currentGlobalOperation = GLOBAL_IDLE;
          }
          //do nothing else
          currentProgramLength=0;
          msg="SUC"; //maintain SUC message (for XMEM transfer)
          break;
        }
        if(transferError_persistent)
        {
          //do not change state?
          msg="ERR";
          currentProgramLength=0;
          break;
        }

        if(!axi_wasnot_ready_persistent)
        {
          opcodeProgram[currentProgramLength] = OP_XMEM_COPY_DATA;
          programMask[currentProgramLength] = MASK_ALWAYS; 
          currentProgramLength++;
          opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
          programMask[currentProgramLength] = MASK_ALWAYS;
          currentProgramLength++;
          //done for now
          opcodeProgram[currentProgramLength] = OP_EXIT;
          programMask[currentProgramLength] = OPRV_NOT_COMPLETE | OPRV_PARTIAL_COMPLETE;
          currentProgramLength++;
        }
        //we have smth to write
        opcodeProgram[currentProgramLength] = OP_BUFFER_TO_PYROLINK;
        programMask[currentProgramLength] = OPRV_DONE | OPRV_OK;
        currentProgramLength++;
        //--> if receiver not ready must be handled afterwards
        break;

      case GLOBAL_PYROLINK_TRANS: //i.e. FPGA to Coaxium
        if(reset_from_psoc == 1)
        { //reset form counter etc. are done in GLOBAL_IDLE;
          currentGlobalOperation = GLOBAL_IDLE;
          //do nothing
          currentProgramLength = 0;
          //to trigger all resets
          iterate_again = true;
          break;
        }
        if(globalOperationDone_persistent)
        { // means, we have this operation done and wait for status change 
          if(pyroReadReq == 0)
          {
            currentGlobalOperation = GLOBAL_IDLE;
          }
          //do nothing else
          currentProgramLength=0;
          pyroSendRequestBit = 0; //signal OPRV_DONE
          break;
        }
        if(transferError_persistent)
        {
          if(global_state_wait_counter_persistent >= GLOBAL_MAX_WAIT_COUNT)
          {
            msg = "NOC";
          } else {
            msg="ERR";
          }
          currentProgramLength=0;
          break;
        }
        
        if(axi_wasnot_ready_persistent)
        {//sender didn't send data, we wait for maximum cycles 
          global_state_wait_counter_persistent++;
          if(global_state_wait_counter_persistent >= GLOBAL_MAX_WAIT_COUNT)
          {
            transferError_persistent = true;
            msg="NOC";
            currentProgramLength = 0;
            break;
          }
        }

        opcodeProgram[currentProgramLength] = OP_PYROLINK_TO_OUTBUFFER;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_ENABLE_SILENT_SKIP;
        programMask[currentProgramLength] = MASK_ALWAYS;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_SEND_BUFFER_XMEM;
        programMask[currentProgramLength] = OPRV_OK | OPRV_DONE;
        currentProgramLength++;
        opcodeProgram[currentProgramLength] = OP_EXIT;
        programMask[currentProgramLength] = OPRV_OK | OPRV_DONE;
        currentProgramLength++;
        //something isn't done -> leaves OPRV_NOT_COMPLETE and OPRV_FAIL for handling

        break;

      case GLOBAL_MANUAL_DECOUPLING:
        //currentProgramLength = 0;
        //toDecoup_persistent = 1;, no, with programms...

        opcodeProgram[0] = OP_ACTIVATE_DECOUP;
        programMask[0] = MASK_ALWAYS;
        currentProgramLength = 1;

        if(manuallyToDecoup == 0)
        {
          currentGlobalOperation = GLOBAL_IDLE;
          opcodeProgram[0] = OP_DEACTIVATE_DECOUP;
          programMask[0] = MASK_ALWAYS;
          currentProgramLength = 1;
        }
        break;

    }
  }

  // ++++++++++++++++++ HANDLE TCP (before executing the plan)  ++++++++++++++++

  if(tcpModeEnabled == 1)
  {
//#pragma HLS dataflow interval=1 FIXME: causes Vivado HLS 2017.4 to crash...

    switch(fsmTcpSessId_RX) {

      default:
      case TCP_FSM_RESET: 
        fsmTcpSessId_RX = TCP_FSM_IDLE;
        break;
      case TCP_FSM_IDLE: 
        //just stay here
        break;

      case TCP_FSM_W84_START:
        fsmTcpSessId_RX = TCP_FSM_PROCESS_DATA;
        //no break;
      case TCP_FSM_PROCESS_DATA:
        if(!siNRC_Tcp_SessId.empty())
        {
          //we assume that the NRC always sends a valid pair of SessId and data (because we control it)
          AppMeta tmp = 0x0;
          if(siNRC_Tcp_SessId.read_nb(tmp))
          {
          received_TCP_SessIds_cnt++;
          currentTcpSessId = tmp;
          }
        } 
        break;
      case TCP_FSM_DONE:
        // just stay here?
        break;
    }

    switch(fsmTcpSessId_TX) {

      default:
      case TCP_FSM_RESET: 
        fsmTcpSessId_TX = TCP_FSM_IDLE;
        currentTcpSessId = 0;
        break;
      case TCP_FSM_IDLE: 
        //just stay here
        break;

      case TCP_FSM_W84_START:
        fsmTcpSessId_TX = TCP_FSM_PROCESS_DATA;
        //no break;
      case TCP_FSM_PROCESS_DATA:
        if(!soNRC_Tcp_SessId.full())
        {
          if(soNRC_Tcp_SessId.write_nb(currentTcpSessId))
          {
            fsmTcpSessId_TX = TCP_FSM_DONE;
          }
        }
        break;
      case TCP_FSM_DONE:
        // just stay here?
        break;
    }

    switch(fsmTcpData_RX) {

      default:
      case TCP_FSM_RESET:
        fsmTcpData_RX = TCP_FSM_IDLE;
        break;
      case TCP_FSM_IDLE: 
        //just stay here
        break;

      case TCP_FSM_W84_START:
        fsmTcpData_RX = TCP_FSM_PROCESS_DATA;
        //no break;
      case TCP_FSM_PROCESS_DATA:
          for(int f = 0; f<IN_BUFFER_SIZE; f++)
          {
            if(siNRC_Tcp_data.empty() || internal_icap_fifo.full() )
            {
              break;
            }
            if(process_fifo_overflow_buffer)
            {
              printf("try to empty overflow buffer\n");
              int new_write_index = 0;
              bool once_blocked = false;
              for(int i =0; i < fifo_overflow_buffer_length; i++)
              {
                if(once_blocked || !internal_icap_fifo.write_nb(fifo_overflow_buffer[i]) )
                {
                  once_blocked = true;
                  if(i != new_write_index)
                  {
                    fifo_overflow_buffer[new_write_index] = fifo_overflow_buffer[i];
                    new_write_index++;
                  }
                }
              }
              fifo_overflow_buffer_length = new_write_index;
              if(!once_blocked)
              {
                process_fifo_overflow_buffer = false;
              } else {
                printf("TCP RX still blocked by hwicap fifo\n");
                break;
              }
            }

            NetworkWord big = NetworkWord();
            if(!siNRC_Tcp_data.read_nb(big))
            {
              break;
            }

            fifo_overflow_buffer_length = 0;
            for(int i = 0; i < 8; i++)
            {
#pragma HLS unroll factor=8
              if((big.tkeep >> i) == 0)
              {
                continue;
              }
              ap_uint<8> current_byte = (ap_uint<8>) (big.tdata >> i*8);
              if(!tcp_write_only_fifo)
              {
                bufferIn[bufferInPtrWrite] = current_byte;
              }
              if(detected_http_nl_cnt >= 1)
              {
                if(!internal_icap_fifo.write_nb(current_byte))
                {
                  fifo_overflow_buffer[fifo_overflow_buffer_length] = current_byte;
                  fifo_overflow_buffer_length++;
                  process_fifo_overflow_buffer = true;
                }
              }
              if(!tcp_write_only_fifo)
              {
                if(bufferInPtrWrite >= 3)
                {
                  if(bufferIn[bufferInPtrWrite] == 0xa &&
                      bufferIn[bufferInPtrWrite -1] == 0xd &&
                      bufferIn[bufferInPtrWrite -2] == 0xa &&
                      bufferIn[bufferInPtrWrite -3] == 0xd )
                  {//HTTP EOF/EOP detected
                    positions_of_detected_http_nl[detected_http_nl_cnt] = bufferInPtrWrite - 3;
                    detected_http_nl_cnt++;
                    printf("TCP RX: detected %d. HTTP NL at position %d\n",(int) detected_http_nl_cnt, (int) (bufferInPtrWrite - 3));
                  }
                } else if (bufferInPtrMaxWrite >= 3 ) {
                  //hangover case if _not_ just cleaned
                  printf("bufferInPtrMaxWrite: %d\n", (int) bufferInPtrMaxWrite);
                  uint8_t chars_to_compare[4];
                  uint32_t position = 0;
                  for(int j = 3; j>=0; j--)
                  {
#pragma HLS unroll factor=4
                    int relative_buffer_in_write = ((int) bufferInPtrWrite) - j;
                    //if( (((int) bufferInPtrWrite) -j) < 0)
                    if(relative_buffer_in_write < 0)
                    {
                      assert(((3-j) >= 0) && ((j-3) < 3));
                      chars_to_compare[3-j] = last_3_chars[3-j];
                    } else {
                      position = IN_BUFFER_SIZE - j + 1;
                      //assert((bufferInPtrWrite-j) >= 0 && (bufferInPtrWrite-j) < IN_BUFFER_SIZE);
                      assert(relative_buffer_in_write >= 0 && relative_buffer_in_write < IN_BUFFER_SIZE);
                      assert(((3-j) >= 0) && ((j-3) < 4));
                      //chars_to_compare[3-j] = bufferIn[bufferInPtrWrite-j];
                      chars_to_compare[3-j] = bufferIn[relative_buffer_in_write];
                    }
                  }
                  printf("chars_to_compare: 0x%02x%02x%02x%02x\n", chars_to_compare[0], chars_to_compare[1], chars_to_compare[2], chars_to_compare[3]);
                  if(chars_to_compare[0] == 0xd && chars_to_compare[1] == 0xa &&
                      chars_to_compare[2] == 0xd && chars_to_compare[3] == 0xa)
                  {//HTTP EOF/EOP detected
                    positions_of_detected_http_nl[detected_http_nl_cnt] = position;
                    detected_http_nl_cnt++;
                    printf("TCP RX: detected %d. HTTP NL at position %d\n",(int) detected_http_nl_cnt, (int) position);
                  }
                }
                //TODO: write explicit poison pill? or use 2nd http nl count?
                bufferInPtrWrite++;
              }
            } //inner for

            tcp_words_received++;
            if(!tcp_write_only_fifo)
            {
              bufferInPtrMaxWrite = bufferInPtrWrite - 1;
              if(bufferInPtrWrite >= (IN_BUFFER_SIZE - NETWORK_WORD_BYTE_WIDTH))
              {
                //for hangover detection
                last_3_chars[0] = bufferIn[bufferInPtrMaxWrite -2];
                last_3_chars[1] = bufferIn[bufferInPtrMaxWrite -1];
                last_3_chars[2] = bufferIn[bufferInPtrMaxWrite];
                //we need to process first
                bufferInPtrWrite = 0; //TODO?
                tcp_write_only_fifo = true;
                printf("\tlast_3_chars: 0x%02x%02x%02x\n", last_3_chars[0], last_3_chars[1], last_3_chars[2]);
                break;
              }
            }

            if( (big.tlast == 1 && (flag_continuous_tcp_rx == 0 || detected_http_nl_cnt >= target_http_nl_cnt ))
                || goto_done_if_idle_tcp_rx )
            {
              fsmTcpData_RX = TCP_FSM_DONE;
              break;
            }
          } //outer loop
        printf("after TCP RX: bufferInPtrWrite %d; bufferInPtrMaxWrite %d;\n", (int) bufferInPtrWrite, (int) bufferInPtrMaxWrite);
        break;
      case TCP_FSM_DONE:
        // just stay here?
        break;
      case TCP_FSM_ERROR:
        //TODO 
        break;
    } 

    switch(fsmTcpData_TX) {

      default:
      case TCP_FSM_RESET:
        fsmTcpData_TX = TCP_FSM_IDLE;
        break;
      case TCP_FSM_IDLE: 
        //just stay here
        break;

      case TCP_FSM_W84_START:
        fsmTcpData_TX = TCP_FSM_PROCESS_DATA;
        //no break;
      case TCP_FSM_PROCESS_DATA:
        //if(!soNRC_Tcp_data.full())
        run_nested_loop_helper = true;
        while(!soNRC_Tcp_data.full() && run_nested_loop_helper)
        {
          //out = NetworkWord();
          NetworkWord out = NetworkWord();
          out.tdata = 0;
          out.tlast = 0;
          out.tkeep = 0;

          for(int i = 0; i < 8; i++)
          {
#pragma HLS unroll
            out.tdata |= ((ap_uint<64>) (bufferOut[bufferOutPtrNextRead + i]) )<< (i*8);
            out.tkeep |= (ap_uint<8>) 0x1 << i;

            if(bufferOutPtrNextRead + i >= (bufferOutContentLength-1))
            {
              out.tlast = 1;
              fsmTcpData_TX = TCP_FSM_DONE;
              break;
            }

          }
          if(soNRC_Tcp_data.write_nb(out))
          {
            bufferOutPtrNextRead += 8;
            //break while
            if(out.tlast == 1)
            {
              run_nested_loop_helper = false;
              break;
            }
          } else {
            run_nested_loop_helper = false;
            break;
          }
        } //while
        //else {
        //  printf("\t ----------- soNRC_Tcp_data is full -----------");
        //}
        break;
      case TCP_FSM_DONE:
        // just stay here?
        break;
    }

    printf("fsmTcpSessId_RX: %d; \tfsmTcpSessId_TX %d\n", fsmTcpSessId_RX, fsmTcpSessId_TX);
    printf("fsmTcpData_RX: %d; \tfsmTcpData_TX %d\n", fsmTcpData_RX, fsmTcpData_TX);
  }


  // ++++++++++++++++++ HWICAP TCP FSM ++++++++++++++++++++

  switch(fsmHwicap)
  {
    default:
    case ICAP_FSM_RESET:
      fsmHwicap = ICAP_FSM_IDLE;
    case ICAP_FSM_IDLE:
      //just stay here?
      break;

    case ICAP_FSM_WRITE:
      {
        if( EOS != 1)
        {//HWICAP is not accessible
          fsmHwicap = ICAP_FSM_ERROR;
          break;
        }
        CR_isWriting = CR_value & CR_WRITE;
        if (CR_isWriting != 1)
        {
          HWICAP[CR_OFFSET] = CR_WRITE;
        }
        //get current FIFO vaccancies
        WFV = HWICAP[WFV_OFFSET];
        WFV_value = WFV & 0x7FF;
        uint32_t max_words_to_write = WFV_value;
        printf("HWICAP FSM: max_words_to_write %d\n", (int) max_words_to_write);
        for(int f = 0; f<IN_BUFFER_SIZE; f++)
        {
          if(internal_icap_fifo.empty())
          {
            break;
          }
          if(max_words_to_write == 0)
          {
#ifndef __SYNTHESIS__
            if(use_sequential_hwicap)
            { //for the csim, we need to brake in all cases
              break;
            }
#endif
            //update FIFO vaccancies
            WFV = HWICAP[WFV_OFFSET];
            WFV_value = WFV & 0x7FF;
            max_words_to_write = WFV_value;
            printf("UPDATE: max_words_to_write %d\n", (int) max_words_to_write);
            if(max_words_to_write == 0)
            {
              break;
            }
          }
          uint8_t bytes_read[4];
          uint8_t bytes_read_count = 0;
          while(!icap_hangover_fifo.empty())
          {
            uint8_t read_var = 0x0;
            if(icap_hangover_fifo.read_nb(read_var))
            {
              bytes_read[bytes_read_count] = read_var;
              bytes_read_count++;
            } else {
              break;
            }
          }
          while( (bytes_read_count < 4) && !internal_icap_fifo.empty() )
          {
            uint8_t read_var = 0x0;
            if(internal_icap_fifo.read_nb(read_var))
            {
              bytes_read[bytes_read_count] = read_var;
              bytes_read_count++;
            } else {
              break;
            }
          }
          if(bytes_read_count != 4)
          { //didn't read a full word
            printf("FIFO hangover bytes: 0x");
            for(int i = 0; i < bytes_read_count; i++)
            {
              icap_hangover_fifo.write(bytes_read[i]);
              printf("%02x ", (int) bytes_read[i]);
            }
            printf("\nFIFO hangover with size %d\n", (int) bytes_read_count);
            break;
          } else { //we have a full word
            ap_uint<32> tmp = 0;
            if (notToSwap == 1)
            {
              tmp |= (ap_uint<32>)   bytes_read[0];
              tmp |= (((ap_uint<32>) bytes_read[1]) <<  8);
              tmp |= (((ap_uint<32>) bytes_read[2]) << 16);
              tmp |= (((ap_uint<32>) bytes_read[3]) << 24);
            } else { 
              //default 
              tmp |= (ap_uint<32>)   bytes_read[3];
              tmp |= (((ap_uint<32>) bytes_read[2]) <<  8);
              tmp |= (((ap_uint<32>) bytes_read[1]) << 16);
              tmp |= (((ap_uint<32>) bytes_read[0]) << 24);
            }

            if ( tmp == 0x0d0a0d0a || tmp == 0x0a0d0a0d )
            { //is like a poison pill, we are done for today
              printf("HTTP NL received, treat it as Poison Pill...\n");
              if(tcp_write_only_fifo)
              {
                positions_of_detected_http_nl[detected_http_nl_cnt] = ICAP_FIFO_POISON_PILL;
                detected_http_nl_cnt++;
              }
              fsmHwicap = ICAP_FSM_DONE;
              break;
            } else if ( tmp == ICAP_FIFO_POISON_PILL || tmp == ICAP_FIFO_POISON_PILL_REVERSE)
            { //we are done for today
              printf("Poison Pill received...\n");
              fsmHwicap = ICAP_FSM_DONE;
              break;
            } else { 
#ifndef __SYNTHESIS__
              if(use_sequential_hwicap)
              {
                HWICAP[sequential_hwicap_address] = tmp;
                sequential_hwicap_address++;
              }
              //TODO: decrement WFV value of testbench
              //for debugging reasons, we write it twice
              HWICAP[WF_OFFSET] = tmp;
#else
              HWICAP[WF_OFFSET] = tmp;
#endif
              wordsWrittenToIcapCnt++;
              max_words_to_write--;
              printf("writing to HWICAP: %#010x\n",(int) tmp);
            }
          }
        } //while
        
        WFV = HWICAP[WFV_OFFSET];
        WFV_value = WFV & 0x7FF;
        if (WFV_value >= HWICAP_FIFO_DEPTH && (WFV_value != max_words_to_write) )
        {
          fifoEmptyCnt++;
          //TODO: re-iterate again?
        }

        if(WFV_value <= HWICAP_FIFO_NEARLY_FULL_TRIGGER)
        {
          fifoFullCnt++;
        }
      }
      break;

    case ICAP_FSM_DONE:
      //stay here
      break;
    case ICAP_FSM_ERROR:
      //stay here
      break;
    case ICAP_FSM_DRAIN:
      while(!internal_icap_fifo.empty())
      {
        internal_icap_fifo.read();
      }
      while(!icap_hangover_fifo.empty())
      {
        icap_hangover_fifo.read();
      }
      fsmHwicap = ICAP_FSM_IDLE;
      break;
  }
  printf("fsmHwicap: %d\n", fsmHwicap);
  // ++++++++++++++++++ EXECUTE THE PLAN  ++++++++++++++++

  lastReturnValue = OPRV_OK; //allow the first operation to be executed 

  //reset flags 
  flag_check_xmem_pattern = 0;
  flag_silent_skip = 0;

  printf("currentProgramLength: %d\n", (int) currentProgramLength);
  printf("currentGlobalOperation: %d\n", (int) currentGlobalOperation);
  assert(currentProgramLength < MAX_PROGRAM_LENGTH);


  //progLoop: 
  for(uint8_t progCnt = 0; progCnt < currentProgramLength; progCnt++)
  {
    OprvType   currentMask = programMask[progCnt];
    OpcodeType currentOpcode = opcodeProgram[progCnt];
    printf("PC %d, lst RV %d, opcode %d\n", (int) progCnt, (int) lastReturnValue, (int) currentOpcode);

    OprvType mask_result = currentMask & lastReturnValue;
    if(((uint8_t) mask_result) == 0)
    {//i.e. we skip this command 
      if(flag_silent_skip == 0)
      {
        lastReturnValue = OPRV_SKIPPED;
      }
      printf("[%d] operation skipped (silent: %d)\n", (int) progCnt, (int) flag_silent_skip);
      continue;
    }


    switch(currentOpcode)
    {

      default: //NO Break
      case OP_NOP: //we do nothing, also leave return value untouched
        break;

      case OP_ENABLE_XMEM_CHECK_PATTERN:
        flag_check_xmem_pattern = 1;
        lastReturnValue = OPRV_OK;
        break;

      case OP_DISABLE_XMEM_CHECK_PATTERN:
        flag_check_xmem_pattern = 0;
        lastReturnValue = OPRV_OK;
        break;

      case OP_ENABLE_SILENT_SKIP:
        flag_silent_skip = 1;
        break;

      case OP_DISABLE_SILENT_SKIP:
        flag_silent_skip = 0;
        break;

      case OP_SET_NOT_TO_SWAP:
        notToSwap = 1;
        break;

      case OP_UNSET_NOT_TO_SWAP:
        notToSwap = 0; //default 
        break;

      case OP_XMEM_COPY_DATA: //sensitive to FLAG check pattern 
          last_xmem_page_received_persistent = 0;
          //explicit overflow 
          expCnt = xmem_page_trans_cnt + 1;
          if (xmem_page_trans_cnt == 0xf)
          {
            expCnt = 0;
          } 
          copyRet = copyAndCheckBurst(xmem,expCnt, lastPageCnt_in);

          switch (copyRet) {
            default:
            case 0:
              msg = "UTD"; //Up To Date
              lastReturnValue = OPRV_NOT_COMPLETE;
              break;
            case 1:
              msg = "INV"; //Invalid
              //We are in the middle of a page transfer
              lastReturnValue = OPRV_PARTIAL_COMPLETE;
              break;
            case 2:
              msg = "CMM"; //Counter MisMatch
              lastReturnValue = OPRV_FAIL;
              break;
            case 3:
              msg = "COR"; //Corrupt pattern 
              lastReturnValue = OPRV_FAIL;
              break;
            case 4: 
              //we received a lastpage
              msg = "SUC"; //success
              xmem_page_trans_cnt = expCnt;
              last_xmem_page_received_persistent = 1;
              lastReturnValue = OPRV_DONE;
              break;
            case 5: //we received a page, but not the last one
              msg= " OK";
              xmem_page_trans_cnt = expCnt;
              lastReturnValue = OPRV_OK;
              break;
          }
        break;

      case OP_TCP_CNT_RESET:
        goto_done_if_idle_tcp_rx = false;
        received_TCP_SessIds_cnt = 0;
        //we reset HTTP EOF detection
        detected_http_nl_cnt = 0;
        target_http_nl_cnt = 0;
        for(int i = 0; i < 3; i++)
        {
#pragma HLS unroll
          last_3_chars[i] = 0x0;
          positions_of_detected_http_nl[i] = 0x0;
        }
        tcp_write_only_fifo = false;
        break;

      case OP_WAIT_FOR_TCP_SESS:
        if(!TcpSessId_updated_persistent)
        {
          if(fsmTcpSessId_RX == TCP_FSM_IDLE || fsmTcpSessId_RX == TCP_FSM_RESET )
          {
            fsmTcpSessId_RX = TCP_FSM_W84_START;
            lastReturnValue = OPRV_NOT_COMPLETE;
          } else if((fsmTcpSessId_RX == TCP_FSM_PROCESS_DATA && (received_TCP_SessIds_cnt >= 1) ) 
                      || fsmTcpSessId_RX == TCP_FSM_DONE)
          {
            lastReturnValue = OPRV_OK;
            TcpSessId_updated_persistent = true;
            tcp_words_received = 0;
          } else {//we still wait
            lastReturnValue = OPRV_NOT_COMPLETE;
          }
        } else {
          lastReturnValue = OPRV_OK;
        }
        break;

      case OP_ACTIVATE_CONT_TCP:
        flag_continuous_tcp_rx = 1;
        goto_done_if_idle_tcp_rx = false;
        if(fsmTcpData_RX == TCP_FSM_IDLE || fsmTcpData_RX == TCP_FSM_DONE)
        {
          fsmTcpData_RX = TCP_FSM_PROCESS_DATA;
        }
        break;

      case OP_DEACTIVATE_CONT_TCP:
        flag_continuous_tcp_rx = 0;
        target_http_nl_cnt = 0;
        goto_done_if_idle_tcp_rx = true;
        break;

      case OP_TCP_RX_STOP_ON_EOR:
        target_http_nl_cnt = 1;
        break;
      
      case OP_TCP_RX_STOP_ON_EOP:
        target_http_nl_cnt = 2;
        break;

      case OP_FILL_BUFFER_TCP:
        if(fsmTcpData_RX == TCP_FSM_IDLE && !fifo_operation_in_progress)
        {
          goto_done_if_idle_tcp_rx = false;
          fsmTcpData_RX = TCP_FSM_PROCESS_DATA;
          lastReturnValue = OPRV_NOT_COMPLETE;
        } else if(fsmTcpData_RX == TCP_FSM_DONE)
        {
          lastReturnValue = OPRV_DONE;
          fsmTcpData_RX = TCP_FSM_IDLE;
        } else if(bufferInPtrMaxWrite == lastSeenBufferInPtrMaxWrite 
                  && !tcp_write_only_fifo //this isn't increasing the counter
                  && !fifo_operation_in_progress
                  )
        { //nothing new
          lastReturnValue = OPRV_NOT_COMPLETE;
          //printf("tcp_write_only_fifo: %d\n", (int) tcp_write_only_fifo);
        } else {
          //looks like we get something
          //or stuff is still left for processing
          lastReturnValue = OPRV_OK;
        }
        //in any case 
        lastSeenBufferInPtrMaxWrite = bufferInPtrMaxWrite;
        break;

      case OP_HANDLE_HTTP: 
        {
          //TODO: break in multiple opcodes?
          printf("httpState bevore parseHTTP: %d\n",httpState);
          bool rx_done = false;
          if(lastReturnValue == OPRV_DONE)
          {
            rx_done = true;
          }

          parseHttpInput(transferError_persistent, wasAbort, invalidPayload_persistent, rx_done); 

          printf("reqType after parseHTTP: %d\n", reqType);

          switch (httpState) {
            case HTTP_IDLE:
              lastReturnValue = OPRV_OK;
              break; 
            case HTTP_PARSE_HEADER: 
              lastReturnValue = OPRV_OK;
              break; 
            case HTTP_HEADER_PARSED:

              if( bufferInPtrMaxWrite == bufferInPtrNextRead) 
              {//corner case 
                httpState = HTTP_READ_PAYLOAD;
                lastReturnValue = OPRV_NOT_COMPLETE;
              } else {
                lastReturnValue = OPRV_PARTIAL_COMPLETE;
              }
              //TODO: start of bitfile must be aligned to 4?? do in sender??
              break;
            case HTTP_READ_PAYLOAD:
              // Do nothing?
              lastReturnValue = OPRV_PARTIAL_COMPLETE;
              break;
            case HTTP_REQUEST_COMPLETE: 
              httpState = HTTP_SEND_RESPONSE;
              // no break??
            case HTTP_SEND_RESPONSE:
              lastReturnValue = OPRV_USER;
              break;
            case HTTP_INVALID_REQUEST:
              //no break 
            case HTTP_DONE:
              lastReturnValue = OPRV_DONE;
              break;
          }
        }
        break;

      case OP_UPDATE_HTTP_STATE:
        if(lastReturnValue == OPRV_FAIL)
        {
          invalidPayload_persistent = true;
        }
        if(wasAbort == 1 || transferError_persistent == true || invalidPayload_persistent == true)
        {
          httpState = HTTP_SEND_RESPONSE;
        }
        if(lastReturnValue == OPRV_DONE)
        {
          httpState = HTTP_REQUEST_COMPLETE;
        }
        lastReturnValue = OPRV_OK;
        break;

      case OP_CHECK_HTTP_EOR:
        if(detected_http_nl_cnt >= 1)
        {
          lastReturnValue = OPRV_DONE;
        } else { 
          lastReturnValue = OPRV_NOT_COMPLETE;
        }
        break;
      
      case OP_CHECK_HTTP_EOP:
        if(detected_http_nl_cnt >= 2)
        {
          lastReturnValue = OPRV_DONE;
        } else { 
          lastReturnValue = OPRV_NOT_COMPLETE;
        }
        break;

      case OP_COPY_REQTYPE_TO_RETURN:
        lastReturnValue = reqType;
        break;

      case OP_BUFFER_TO_HWICAP:
        {
          if (EOS != 1)
          {//HWICAP is not accessible
            msg = "INR";
            lastReturnValue = OPRV_FAIL;
            break;
          }
          uint32_t maxPayloadWrite = 0;
          if(bufferInPtrMaxWrite >= 4)
          {
            maxPayloadWrite = bufferInPtrMaxWrite - 3;
          }
          if(currentGlobalOperation == GLOBAL_TCP_HTTP)
          {
            if(detected_http_nl_cnt >= 2 
                && (positions_of_detected_http_nl[1] <= bufferInPtrMaxWrite) )
            {
              maxPayloadWrite = positions_of_detected_http_nl[1] - 4; //-4, becauese we have <= in the for
              lastReturnValue = OPRV_DONE;
              printf("STOP at 2. HTTP NL\n");
            }

          }
          //printf("Writing Buffer to HWICAP from %d to %d (including); notToSwap = %d, max_bytes_to_write: %d, write_iteration: %d, read_iteration: %d\n", (int) bufferInPtrNextRead, (int) maxPayloadWrite,(int) notToSwap, (int) max_bytes_to_write,(int) bufferIn_write_iteration_cnt, (int) bufferIn_read_iteration_cnt);
          printf("Writing Buffer to HWICAP from %d to %d (including); notToSwap = %d\n", (int) bufferInPtrNextRead, (int) maxPayloadWrite,(int) notToSwap);
          ap_uint<32> old_bufferInPtrNextRead = bufferInPtrNextRead;
          ap_uint<32> old_wordsWrittenToIcapCnt = wordsWrittenToIcapCnt;
          //for(i = bufferInPtrNextRead; i <= maxPayloadWrite; i += 4)
          uint32_t i = bufferInPtrNextRead;
          //while(i <= maxPayloadWrite) //we have substracted -3, so <=
          while(i <= maxPayloadWrite && maxPayloadWrite > 0) //we have substracted -3, so <=
          {
            ap_uint<32> tmp = 0;

            if(currentGlobalOperation == GLOBAL_TCP_HTTP && hwicap_hangover_present
                && i == 0) //only for first iteration, if bufferInPtrNextRead = 0
            {
              for(int j = 0; j < 4; j++)
              {
#pragma HLS unroll factor=4
                if( notToSwap == 1)
                {
                  if(j - (((int) hwicap_hangover_size) -1) <= 0)
                  {//from hangover bytes
                    tmp |= (((ap_uint<32>) buffer_hangover_bytes[j]) <<  8*j);
                  } else {//from buffer
                    tmp |= (((ap_uint<32>) bufferIn[j - hwicap_hangover_size]) <<  8*j); //actually -1 + 1...
                  }
                } else { //default
                  if(j - (((int) hwicap_hangover_size) -1) <= 0)
                  {//from hanover bytes
                    tmp |= (((ap_uint<32>) buffer_hangover_bytes[j]) <<  8*(3-j));
                  } else {//from buffer
                    assert((j - hwicap_hangover_size) >= 0);
                    tmp |= (((ap_uint<32>) bufferIn[j - hwicap_hangover_size]) <<  8*(3-j)); //actually -1 + 1...
                  }
                }
              }
              hwicap_hangover_present = false;
              i += 4 - hwicap_hangover_size;
              printf("hangover of size %d inserted.\n", (int) hwicap_hangover_size);
            } else {
              if (notToSwap == 1)
              {
                tmp |= (ap_uint<32>)   bufferIn[i + 0];
                tmp |= (((ap_uint<32>) bufferIn[i + 1]) <<  8);
                tmp |= (((ap_uint<32>) bufferIn[i + 2]) << 16);
                tmp |= (((ap_uint<32>) bufferIn[i + 3]) << 24);
              } else { 
                //default 
                tmp |= (ap_uint<32>)   bufferIn[i + 3];
                tmp |= (((ap_uint<32>) bufferIn[i + 2]) <<  8);
                tmp |= (((ap_uint<32>) bufferIn[i + 1]) << 16);
                tmp |= (((ap_uint<32>) bufferIn[i + 0]) << 24);
              }
              i += 4;
            }

            if ( tmp == 0x0d0a0d0a || tmp == 0x0a0d0a0d )
            { //in theory, should never happen...
              printf("Error: Tried to write 0d0a0d0a.\n");
              writeErrCnt++;
              //continue;
            } else {
#ifndef __SYNTHESIS__
              if(use_sequential_hwicap)
              {
                HWICAP[sequential_hwicap_address] = tmp;
                sequential_hwicap_address++;
              }
              //for debugging reasons, we write it twice
                HWICAP[WF_OFFSET] = tmp;
#else
              HWICAP[WF_OFFSET] = tmp;
#endif
              wordsWrittenToIcapCnt++;
              printf("writing to HWICAP: %#010x\n",(int) tmp);
            }
          } //while

          // printf("bufferInPtrMaxWrite: %d\n", (int) bufferInPtrMaxWrite);
          // printf("bufferInPtrNextRead: %d\n", (int) bufferInPtrNextRead);
          // printf("wordsWrittenToIcapCnt: %d\n", (int) wordsWrittenToIcapCnt);

          CR_isWriting = CR_value & CR_WRITE;
          if (CR_isWriting != 1)
          {
            HWICAP[CR_OFFSET] = CR_WRITE;
          }


          bufferInPtrNextRead = i; //no +4, since it is done as last step in the loop
          printf("bufferInPtrNextRead: %d\n", (int) bufferInPtrNextRead);
          printf("bufferInPtrMaxWrite: %d\n", (int) bufferInPtrMaxWrite);
          //printf("bufferInPtrMaxWrite_old_iteration: %d\n", (int) bufferInPtrMaxWrite_old_iteration);
          printf("bufferInPtrWrite: %d\n", (int) bufferInPtrWrite);
          
          WFV = HWICAP[WFV_OFFSET];
          WFV_value = WFV & 0x7FF;
          //if (WFV_value == 0x7FF)
          if (WFV_value >= HWICAP_FIFO_DEPTH && (bufferInPtrNextRead > old_bufferInPtrNextRead))
          {
            //printf("FIFO is unexpected empty\n");
            fifoEmptyCnt++;
          }

          if(WFV_value <= HWICAP_FIFO_NEARLY_FULL_TRIGGER)
          {
            fifoFullCnt++;
          }


          if(currentGlobalOperation == GLOBAL_XMEM_HTTP || currentGlobalOperation == GLOBAL_XMEM_TO_HWICAP)
          {
            if(bufferInPtrNextRead >= (IN_BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE))
            {//should always hit even pages....
              if (parseHTTP == 1)
              {
                ap_int<5> telomere = bufferInPtrMaxWrite - bufferInPtrNextRead + 1; //+1 because MaxWrite is already filled (not next write)
                printf("telomere: %d\n", (int) telomere);

                if (telomere != 0)
                {
                  for(int j = 0; j<telomere; j++)
                  {
                    bufferIn[j] = bufferIn[bufferInPtrNextRead + j];
                  }
                  bufferInPtrWrite = telomere;
                }
                if (telomere < 0)
                {
                  printf("ERROR negativ telomere!\n");
                  writeErrCnt++;
                }
              }

              bufferInPtrNextRead = 0;
            }
          }

          if(currentGlobalOperation == GLOBAL_TCP_HTTP)
          {
            //check for hangover
            if(bufferInPtrWrite < bufferInPtrNextRead)
            {
                ap_int<5> telomere = bufferInPtrMaxWrite - bufferInPtrNextRead + 1; //+1 because MaxWrite is already filled (not next write)
                printf("telomere: %d\n", (int) telomere);
                assert(telomere >= 0 && telomere < 4);
                hwicap_hangover_size = 0;
                if(telomere > 0 && telomere < 4)
                {
                  printf("hangover bytes: 0x");
                  for(int j = 0; j<telomere; j++)
                  {
                    buffer_hangover_bytes[j] = bufferIn[bufferInPtrNextRead + j];
                    printf("%02x ",(int) buffer_hangover_bytes[j]);
                  }
                  hwicap_hangover_present = true;
                  hwicap_hangover_size = telomere;
                  printf("\nBuffer hangover with size %d\n", (int) hwicap_hangover_size);
                } else if(telomere != 0)
                { //in theory, should never happen
                  printf("invalid telomere count!\n");
                  writeErrCnt++;
                }
                //in all cases
                bufferInPtrNextRead = 0;
            }
          }
          printf("after UPDATE bufferInPtrNextRead: %d\n", (int) bufferInPtrNextRead);
          
          if(lastReturnValue == OPRV_DONE || last_xmem_page_received_persistent == 1) 
          { //pass done
            lastReturnValue = OPRV_DONE;
          } else if(bufferInPtrNextRead == old_bufferInPtrNextRead)
          {
            lastReturnValue = OPRV_NOT_COMPLETE;
          } else {
            lastReturnValue = OPRV_OK;
          }
        }
        break;

      case OP_FIFO_TO_HWICAP:
        if(fsmHwicap == ICAP_FSM_IDLE)
        {
          fsmHwicap = ICAP_FSM_WRITE;
          fifo_operation_in_progress = true;
          lastReturnValue = OPRV_NOT_COMPLETE;
          printf("started HWICAP FIFO FSM");
        } else if(fsmHwicap == ICAP_FSM_DONE)
        {
          lastReturnValue = OPRV_DONE;
          fifo_operation_in_progress = false;
          fsmHwicap = ICAP_FSM_IDLE;
        } else if(fsmHwicap == ICAP_FSM_ERROR || fsmHwicap == ICAP_FSM_DRAIN)
        {
          lastReturnValue = OPRV_FAIL;
          fifo_operation_in_progress = false;
          fsmHwicap = ICAP_FSM_DRAIN;
        } else {
          //we are doing smth
          lastReturnValue = OPRV_OK;
        }
        break;

      case OP_BUFFER_TO_PYROLINK:
#ifdef INCLUDE_PYROLINK
        //TODO handle swapping / Big Endiannes?

        if(*disable_pyro_link == 0) //to avoid blocking...
        {
          if(lastReturnValue == OPRV_DONE || last_xmem_page_received_persistent == 1)
          { //pass done
            lastReturnValue = OPRV_DONE;
          } else {
            lastReturnValue = OPRV_OK; //overwrite later if necessary
          }

          //printf("OP_BUFFER_TO_PYROLINK: bufferInPtrNextRead %d, bufferInPtrMaxWrite %d\n", (int) bufferInPtrNextRead, (int) bufferInPtrMaxWrite);
          uint32_t i = 0;
          for(i = bufferInPtrNextRead; i <= bufferInPtrMaxWrite; i++)
          {
            if(!soPYROLINK.full())
            {
              Axis<8> tmp = Axis<8>(bufferIn[i]);
              tmp.tkeep = 1;
              if( last_xmem_page_received_persistent == 1 && i == bufferInPtrMaxWrite ) //not -1, because both is 0 based
              {
                tmp.tlast = 1;
                //no break necessary, this should anyway be the last rount
              } else {
                tmp.tlast = 0;
              }

              soPYROLINK.write(tmp);

            } else {
              lastReturnValue = OPRV_NOT_COMPLETE;
              //TODO: must we i--? 
              break;
            }
          }
          bufferInPtrNextRead = i;
        } else { 
          lastReturnValue = OPRV_FAIL;
        }
#endif
        break;

      case OP_BUFFER_TO_ROUTING: 
        if (*disable_ctrl_link == 0) 
        {
          int bodyLen = request_len(bufferInPtrNextRead, bufferInPtrMaxWrite - bufferInPtrNextRead); 

          int i = bufferInPtrNextRead;
          int maxWhile = bufferInPtrMaxWrite - MIN_ROUTING_TABLE_LINE;

          need_to_update_nrc_mrt = true;

          if(bodyLen > 0)
          { //body is complete here 
            maxWhile = bufferInPtrNextRead + bodyLen; //bodyLen is WITHOUT the actual footer 
            lastReturnValue = OPRV_DONE;
            current_nrc_mrt_version++;
          } else { 
            lastReturnValue = OPRV_NOT_COMPLETE;
          }

          printf("nextRead: %d, maxWhile %d, notToSwap %d\n", i, maxWhile, (int) notToSwap);

          while(i<maxWhile)
          {
            char* intStart = (char*) &bufferIn[i];
            int intLen = my_wordlen(intStart);
            if(i + intLen + 1 + 4 + 1> bufferInPtrMaxWrite) //1: space, 4: IPv4 address 1: \n (now we know the actual length)
            {
              //TODO 
              break; 
            }
            //looks like complete line is here 
            ap_uint<32> rankID = (unsigned int) my_atoi(intStart, intLen);
            printf("intLen: %d, rankdID: %d\n", intLen, (unsigned int) rankID);
            i += intLen; 
            i++; //space
            ap_uint<32> tmp = 0; 

            if ( notToSwap == 1) 
            {
              tmp |= (ap_uint<32>) bufferIn[i + 0];
              tmp |= (((ap_uint<32>) bufferIn[i + 1]) <<  8);
              tmp |= (((ap_uint<32>) bufferIn[i + 2]) << 16);
              tmp |= (((ap_uint<32>) bufferIn[i + 3]) << 24);
            } else { 
              //default
              tmp |= (ap_uint<32>) bufferIn[i + 3];
              tmp |= (((ap_uint<32>) bufferIn[i + 2]) <<  8);
              tmp |= (((ap_uint<32>) bufferIn[i + 1]) << 16);
              tmp |= (((ap_uint<32>) bufferIn[i + 0]) << 24);
            }
            i+= 4;

            i++; //newline

            if(rankID >= MAX_CLUSTER_SIZE)
            { //ignore and return 422
              invalidPayload_persistent = true;
              lastReturnValue = OPRV_DONE;
              need_to_update_nrc_mrt = false;
              current_nrc_mrt_version--;
              printf("invalid routing table detected.\n");
              break;
            }
            //dont' check for current cluster size!
            //it could be that the CFRM want's to override the entries of removed nodes

            //transfer to NRC: 
            current_MRT[rankID] = tmp; 
            if(rankID > max_discovered_node_id)
            {
              max_discovered_node_id = rankID;
              printf("max_discovered_node_id: %d\n", (int) max_discovered_node_id);
            }
            //lastaddressWrittenToNRC = tmp;
            //lastNIDWrittentoNRC = rankID;
            printf("writing on address %d to NRC: %#010x\n",(unsigned int) rankID, (int) tmp);
          }

          bufferInPtrNextRead = i; //NOT i + x, because that is already done by the for loop!
          //return value already set!
        } else {
          lastReturnValue = OPRV_FAIL;
          transferError_persistent = true;
        }
        break;

      case OP_CLEAR_ROUTING_TABLE:
        need_to_update_nrc_mrt = true;
        current_nrc_mrt_version++;
        for(int i = 0; i < max_discovered_node_id; i++)
        {
          current_MRT[i] = 0x0;
        }
        //lastReturnValue unchanged
        break;

      case OP_PYROLINK_TO_OUTBUFFER:
#ifdef INCLUDE_PYROLINK
        if(*disable_pyro_link == 0)
        {
          int i = 0;
          if(!siPYROLINK.empty())
          {
            lastReturnValue = OPRV_OK; //overwrite if necessary

            //printf("bufferOutPtrWrite: %d\n", (int) bufferOutPtrWrite);

            for(i = bufferOutPtrWrite; i<IN_BUFFER_SIZE; i++)
            {
              if(!siPYROLINK.empty())
              {
                Axis<8> tmp = siPYROLINK.read();
                if (tmp.tkeep != 1)
                {//skip it
                  i--; 
                  continue;
                }
                bufferOut[bufferOutPtrWrite + i] = tmp.tdata;
                if(tmp.tlast == 1)
                {
                  lastReturnValue = OPRV_DONE;
                  break;
                }
              }
            }
            bufferOutPtrWrite += i + 1; //where to write NEXT
            bufferOutContentLength = bufferOutPtrWrite; //still plus 1, because it's a length
          } else {
            //we didn't receive anything
            lastReturnValue = OPRV_NOT_COMPLETE;
          }

        } else { 
          lastReturnValue = OPRV_FAIL;
        }
#endif
        break;

      case OP_SEND_TCP_SESS:
        //if we are in an OPRV_DONE "state": we just start the TX FSM and then go to "success"
        //the global state will wait until all TCP FSMs are done before it starts over
        //The TX FSMs block on sending, after the FMC ISA finished...so this OP-code is for triggering the FSM to start to work
        if(TcpSessId_updated_persistent)
        {
          if(fsmTcpSessId_TX == TCP_FSM_IDLE)
          {
            fsmTcpSessId_TX = TCP_FSM_W84_START;
            if(lastReturnValue != OPRV_DONE)
            {
             lastReturnValue = OPRV_NOT_COMPLETE;
            }
          } else if(fsmTcpSessId_TX == TCP_FSM_DONE)
          {
            if(lastReturnValue != OPRV_DONE)
            {
              lastReturnValue = OPRV_OK;
            }
            TcpSessId_updated_persistent = false;
            received_TCP_SessIds_cnt = 0;
            fsmTcpSessId_TX = TCP_FSM_IDLE;
          } else {//we still wait
            if(lastReturnValue != OPRV_DONE)
            {
              lastReturnValue = OPRV_NOT_COMPLETE;
            }
          }
        } else {
          if(lastReturnValue != OPRV_DONE)
          { 
            lastReturnValue = OPRV_OK;
          }
        }
        break;

      case OP_SEND_BUFFER_TCP:
        //if we are in an OPRV_DONE "state": we just start the TX FSM and then go to "success"
        //the global state will wait until all TCP FSMs are done before it starts over
        //The TX FSMs block on sending, after the FMC ISA finished...so this OP-code is for triggering the FSM to start to work
        if(fsmTcpData_TX == TCP_FSM_IDLE)
        {
          fsmTcpData_TX = TCP_FSM_PROCESS_DATA;
          if(lastReturnValue != OPRV_DONE)
          {
            lastReturnValue = OPRV_NOT_COMPLETE;
          }
        } else if(fsmTcpData_TX == TCP_FSM_DONE)
        {
          fsmTcpData_TX = TCP_FSM_IDLE;
          lastReturnValue = OPRV_DONE;
        } else if(bufferOutPtrNextRead == lastSeenBufferOutPtrNextRead)
        {//looks like there are no news
          if(lastReturnValue != OPRV_DONE)
          {
            lastReturnValue = OPRV_NOT_COMPLETE;
          }
        } else {
          if(lastReturnValue != OPRV_DONE)
          {
            lastReturnValue = OPRV_OK;
          }
        } 
        lastSeenBufferOutPtrNextRead = bufferOutPtrNextRead;
        break;

      case OP_SEND_BUFFER_XMEM: 
        {
          uint8_t pageCnt = bytesToPages(bufferOutContentLength, true);
          copyOutBuffer(pageCnt,xmem);

          if(lastReturnValue == OPRV_DONE) 
          { //pass done
            lastReturnValue = OPRV_DONE;
          } else {
            lastReturnValue = OPRV_OK;
          }
        }
        break; 

      case OP_CLEAR_IN_BUFFER:
        emptyInBuffer();
        lastReturnValue = OPRV_OK;
        break;

      case OP_CLEAR_OUT_BUFFER:
        emptyOutBuffer();
        lastReturnValue = OPRV_OK;
        break;

      case OP_ACTIVATE_DECOUP:
        toDecoup_persistent = 1;
        lastReturnValue = OPRV_OK;
        break;

      case OP_DEACTIVATE_DECOUP:
        toDecoup_persistent = 0;
        if(lastReturnValue == OPRV_DONE) 
        { //pass done
          lastReturnValue = OPRV_DONE;
        } else {
          lastReturnValue = OPRV_OK;
        }
        break;

      case OP_ABORT_HWICAP:
        HWICAP[CR_OFFSET] = CR_ABORT;
        transferError_persistent = true;
        wasAbort = 1;
        lastReturnValue = OPRV_OK;
        break;

      case OP_FAIL:
        lastReturnValue = OPRV_FAIL;
        break;

      case OP_DONE:
        lastReturnValue = OPRV_DONE;
        //break progLoop; //does this work?
        break;

      case OP_OK:
        lastReturnValue = OPRV_OK;
        break;

      case OP_EXIT:
        //lastReturnValue unchanged
        progCnt = MAX_PROGRAM_LENGTH;
        break;

    } //switch
  } //prog loop


    // ++++++++++++++++++ UPDATE OPERATIONS  ++++++++++++++++

  switch(currentGlobalOperation)
  {
    default: //no break, just for default
    case GLOBAL_IDLE: 
      //nothing to do 
      break;

    case GLOBAL_XMEM_CHECK_PATTERN:
      //OPRV_NOT_COMPLETE, OPRV_PARTIAL_COMPLETE --> continue
      if(lastReturnValue == OPRV_DONE)
      {//received last page 
        //Displays are set in daily task 
        //ATTENTION: we only change state if input changes
        globalOperationDone_persistent = true;
      } else if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
      }
      break;

    case GLOBAL_XMEM_TO_HWICAP:
      if(lastReturnValue == OPRV_DONE)
      {//looks like we are done
        //Displays are set in daily task 
        globalOperationDone_persistent = true;
      }
      if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
      }
      break;
    
    case GLOBAL_XMEM_HTTP:
      if(lastReturnValue == OPRV_DONE)
      {//looks like we are done
        //Displays are set in daily taksk 
        globalOperationDone_persistent = true;
      }
      if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
      }
      break;

    case GLOBAL_TCP_HTTP:
      // continue in this mode when HTTP_IDLE and environment not changed
      if(lastReturnValue == OPRV_DONE)
      {//looks like we are done
        //Displays are set in daily taksk 
        globalOperationDone_persistent = true;
        msg="SUC";
      }
      if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
        msg="ERR";
      }
      if(lastReturnValue == OPRV_NOT_COMPLETE)
      {
        msg="WAT";
      }
      if(globalOperationDone_persistent && (lastReturnValue != OPRV_DONE))
      {//we wait for the TCP FSM's to finish..
        msg="WFF";
      }
      break;

    case GLOBAL_PYROLINK_RECV: //i.e. Coaxium to FPGA
      if(lastReturnValue == OPRV_DONE)
      {//looks like we are done
        //Displays are set in daily tasks
        globalOperationDone_persistent = true;
      }
      if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
      }
      if(lastReturnValue == OPRV_NOT_COMPLETE)
      {
        axi_wasnot_ready_persistent = true;
      } else {
        axi_wasnot_ready_persistent = false;
      }
      break;

    case GLOBAL_PYROLINK_TRANS: //i.e. FPGA to Coaxium
      if(lastReturnValue == OPRV_DONE)
      {//looks like we are done
        //Displays are set in daily tasks
        globalOperationDone_persistent = true;
        pyroSendRequestBit = 0;
      }
      if(lastReturnValue == OPRV_FAIL)
      {
        transferError_persistent = true;
      }
      if(lastReturnValue == OPRV_NOT_COMPLETE)
      {
        axi_wasnot_ready_persistent = true;
      } else {
        axi_wasnot_ready_persistent = false;
      }
      //OPRV_OK --> we need another round
      break;

    case GLOBAL_MANUAL_DECOUPLING:
      //nothing to do
      break;
  }


  // ++++++++++++++++++ EVERY-DAY JOBS ++++++++++++++++++++

  if( wasAbort == 1) 
  { //Abort occured in previous cycle
    msg = "ABR";
  }

  // SOFT RESET 
  *setSoftReset = (MMIO_in_LE >> SOFT_RST_SHIFT) & 0b1;

  //===========================================================
  // Decoupling 

  if ( toDecoup_persistent == 1 || wasAbort == 1)
  {
  *setDecoup = 0b1;
} else {
  *setDecoup = 0b0;
}

//===========================================================
// Set RANK and SIZE 

*role_rank = nodeRank; 
*cluster_size = clusterSize; 


//===========================================================
// connection to NRC 

//to not overwrite NRC updates
if((*disable_ctrl_link == 0) && (*layer_4_enabled == 0) && tables_initialized )
{
  //reset of NTS closes ports
  current_nrc_config[NRC_CONFIG_SAVED_UDP_PORTS] = 0x0;
  current_nrc_config[NRC_CONFIG_SAVED_TCP_PORTS] = 0x0;
  current_nrc_config[NRC_CONFIG_SAVED_FMC_PORTS] = 0x0;
  need_to_update_nrc_config = false; //the NRC know this too
}

if((*disable_ctrl_link == 0) && (*layer_6_enabled == 1) && tables_initialized //to avoid blocking...
    && !fifo_operation_in_progress //to avoid delays of PR
  )
{ //and we need valid tables

  mpe_status_disabled = false;

  switch (linkCtrlFSM) 
  {
    case LINKFSM_IDLE:
      ctrl_link_next_check_seconds = fpga_time_seconds + CHECK_CTRL_LINK_INTERVAL_SECONDS;
      if(ctrl_link_next_check_seconds >= 60)
      {
        ctrl_link_next_check_seconds = 0;
      }
      linkCtrlFSM = LINKFSM_WAIT;
      break;

    case LINKFSM_WAIT:
      if(need_to_update_nrc_config)
      {
        linkCtrlFSM = LINKFSM_UPDATE_CONFIG;
      } 
      else if(need_to_update_nrc_mrt)
      {
        mrt_copy_index = 0;
        linkCtrlFSM = LINKFSM_UPDATE_MRT;
      } 
      else if(ctrl_link_next_check_seconds <= fpga_time_seconds)
      {
        mpe_status_request_cnt = 0;
        linkCtrlFSM = LINKFSM_UPDATE_STATE;
      }
      break;

    case LINKFSM_UPDATE_CONFIG:
      //e.g. to recover from a reset
      nrcCtrl[NRC_CTRL_LINK_CONFIG_START_ADDR + NRC_CONFIG_OWN_RANK] = nodeRank; 
        nrcCtrl[NRC_CTRL_LINK_CONFIG_START_ADDR + NRC_CONFIG_SAVED_UDP_PORTS] = current_nrc_config[NRC_CONFIG_SAVED_UDP_PORTS];
        nrcCtrl[NRC_CTRL_LINK_CONFIG_START_ADDR + NRC_CONFIG_SAVED_TCP_PORTS] = current_nrc_config[NRC_CONFIG_SAVED_TCP_PORTS];
        nrcCtrl[NRC_CTRL_LINK_CONFIG_START_ADDR + NRC_CONFIG_SAVED_FMC_PORTS] = current_nrc_config[NRC_CONFIG_SAVED_FMC_PORTS];
        need_to_update_nrc_config = false;
        linkCtrlFSM = LINKFSM_IDLE;
        break;

      case LINKFSM_UPDATE_MRT:
        printf("linkFSM: updating entry %d with value 0x%08x; max_discovered_node_id: %d\n",(int) mrt_copy_index, (int) current_MRT[mrt_copy_index], (int) max_discovered_node_id);
        nrcCtrl[NRC_CTRL_LINK_MRT_START_ADDR + mrt_copy_index] = current_MRT[mrt_copy_index];
        mrt_copy_index++;

        if(mrt_copy_index > MAX_MRT_SIZE || mrt_copy_index > max_discovered_node_id)
        {
          nrcCtrl[NRC_CTRL_LINK_CONFIG_START_ADDR + NRC_CONFIG_MRT_VERSION] = current_nrc_mrt_version;
          need_to_update_nrc_mrt = false;
          linkCtrlFSM = LINKFSM_IDLE;
        }
        break;

      case LINKFSM_UPDATE_STATE:
        mpe_status[mpe_status_request_cnt] = nrcCtrl[NRC_CTRL_LINK_STATUS_START_ADDR + mpe_status_request_cnt];
        mpe_status_request_cnt++; 
        if(mpe_status_request_cnt >= NRC_NUMBER_STATUS_WORDS)
        {
          if(mpe_status[NRC_STATUS_MRT_VERSION] != current_nrc_mrt_version)
          {
            need_to_update_nrc_mrt = true;
            need_to_update_nrc_config = true; //better to do both
            linkCtrlFSM = LINKFSM_WAIT;
          } 
          else if (mpe_status[NRC_STATUS_OWN_RANK] != nodeRank)
          {
            need_to_update_nrc_config = true;
            linkCtrlFSM = LINKFSM_WAIT;
          } else {
            linkCtrlFSM = LINKFSM_UPDATE_SAVED_STATE;
          }
        } 
        break;
      case LINKFSM_UPDATE_SAVED_STATE:
        //indicies don't match: need to to manually 
        current_nrc_config[NRC_CONFIG_SAVED_UDP_PORTS] = mpe_status[NRC_STATUS_OPEN_UDP_PORTS];
        current_nrc_config[NRC_CONFIG_SAVED_TCP_PORTS] = mpe_status[NRC_STATUS_OPEN_TCP_PORTS];
        current_nrc_config[NRC_CONFIG_SAVED_FMC_PORTS] = mpe_status[NRC_STATUS_FMC_PORT_PROCESSED];
        linkCtrlFSM = LINKFSM_IDLE;
        break;
    }
    printf("ctrlLINK FSM: %d\n", linkCtrlFSM);

  } else {
    //ctrlLink disabled 
    //hex for "DISABLED"
    mpe_status_disabled = true;
    mpe_status[0] = 0x44495341;
    mpe_status[1] = 0x424c4544;
  }


  //===========================================================
  //  putting displays together 


  ap_uint<4> Dsel = 0;

  Dsel = (MMIO_in_LE >> DSEL_SHIFT) & 0xF;

  Display1 = (WEMPTY << WEMPTY_SHIFT) | (HWICAP_Done << DONE_SHIFT) | EOS;
  Display1 |= WFV_value << WFV_V_SHIFT;
  Display1 |= ((ap_uint<32>) decoupStatus)  << DECOUP_SHIFT;
  Display1 |= ((ap_uint<32>) tcpModeEnabled)  << TCP_OPERATION_SHIFT;
  Display1 |= ((ap_uint<32>) ASW1) << ASW1_SHIFT;
  Display1 |= ((ap_uint<32>) CR_value) << CMD_SHIFT;

  Display2 = ((ap_uint<32>) ASW2) << ASW2_SHIFT;
  Display2 |= ((ap_uint<32>) ASW3) << ASW3_SHIFT;
  Display2 |= ((ap_uint<32>) ASW4) << ASW4_SHIFT;
  Display2 |= ((ap_uint<32>) notToSwap) << NOT_TO_SWAP_SHIFT;

  Display3 = ((ap_uint<32>) xmem_page_trans_cnt) << RCNT_SHIFT;
  Display3 |= ((ap_uint<32>) msg[0]) << MSG_SHIFT + 16;
  Display3 |= ((ap_uint<32>) msg[1]) << MSG_SHIFT + 8;
  Display3 |= ((ap_uint<32>) msg[2]) << MSG_SHIFT + 0; 

  Display4 = ((ap_uint<32>) responePageCnt) << ANSWER_LENGTH_SHIFT; 
  Display4 |= ((int) httpState) << HTTP_STATE_SHIFT;
  Display4 |= ((ap_uint<32>) writeErrCnt) << WRITE_ERROR_CNT_SHIFT;
  Display4 |= ((ap_uint<32>) fifoEmptyCnt) << EMPTY_FIFO_CNT_SHIFT;
  Display4 |= ((ap_uint<32>) currentGlobalOperation) << GLOBAL_STATE_SHIFT;

  Display5 = (ap_uint<32>) wordsWrittenToIcapCnt; 

  Display6 = nodeRank << RANK_SHIFT; 
  Display6 |= clusterSize << SIZE_SHIFT; 
  Display6 |= ((ap_uint<32>) lastResponsePageCnt)  << LAST_PAGE_WRITE_CNT_SHIFT;
  Display6 |= ((ap_uint<32>) pyroSendRequestBit)  << PYRO_SEND_REQUEST_SHIFT;

  Display7  = ((ap_uint<32>) fsmTcpSessId_RX) << RX_SESS_STATE_SHIFT;
  Display7 |= ((ap_uint<32>) fsmTcpData_RX) << RX_DATA_STATE_SHIFT;
  Display7 |= ((ap_uint<32>) fsmTcpSessId_TX) << TX_SESS_STATE_SHIFT;
  Display7 |= ((ap_uint<32>) fsmTcpData_TX) << TX_DATA_STATE_SHIFT;
  Display7 |= ((ap_uint<32>) tcp_iteration_count) << TCP_ITER_COUNT_SHIFT;
  Display7 |= ((ap_uint<32>) detected_http_nl_cnt) << DETECTED_HTTPNL_SHIFT;

  Display8 = (ap_uint<32>) tcp_words_received;

  Display9 = (bufferInPtrMaxWrite & 0x000FFFFF) << BUFFER_IN_MAX_SHIFT;
  Display9 |= ((ap_uint<32>) fifoFullCnt) << FULL_FIFO_CNT_SHIFT;
  Display9 |= ((ap_uint<32>) fsmHwicap) << ICAP_FSM_SHIFT;

  ap_uint<32> MMIO_out_LE = 0x0;

  switch (Dsel) {
    case 1:
      MMIO_out_LE = (0x1 << DSEL_SHIFT) | Display1;
      break;
    case 2:
      MMIO_out_LE = (0x2 << DSEL_SHIFT) | Display2;
      break;
    case 3:
      MMIO_out_LE = (0x3 << DSEL_SHIFT) | Display3;
      break;
    case 4:
      MMIO_out_LE = (0x4 << DSEL_SHIFT) | Display4;
      break;
    case 5:
      MMIO_out_LE = (0x5 << DSEL_SHIFT) | Display5;
      break;
    case 6:
      MMIO_out_LE = (0x6 << DSEL_SHIFT) | Display6;
      break;
    case 7:
      MMIO_out_LE = (0x7 << DSEL_SHIFT) | Display7;
      break;
    case 8:
      MMIO_out_LE = (0x8 << DSEL_SHIFT) | Display8;
      break;
    case 9:
      MMIO_out_LE = (0x9 << DSEL_SHIFT) | Display9;
      break;
    default:
      MMIO_out_LE = 0xBEBAFECA;
      break;
  }
  
  ap_uint<32> MMIO_out_BE = 0x0;
  MMIO_out_BE  = (ap_uint<32>) ((MMIO_out_LE >> 24) & 0xFF);
  MMIO_out_BE |= (ap_uint<32>) ((MMIO_out_LE >> 8) & 0xFF00);
  MMIO_out_BE |= (ap_uint<32>) ((MMIO_out_LE << 8) & 0xFF0000);
  MMIO_out_BE |= (ap_uint<32>) ((MMIO_out_LE << 24) & 0xFF000000);

  *MMIO_out = MMIO_out_BE;

  return;
}


/*! \} */

