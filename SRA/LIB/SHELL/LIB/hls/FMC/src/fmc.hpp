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
 * @file       : fmc.hpp
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

#ifndef _FMC_H_
#define _FMC_H_

#include "ap_int.h"
#include "http.hpp"
#include "../../../../../hls/cfdk.hpp"
#include "../../../../../hls/network.hpp"
#include "../../network_utils.hpp"


using namespace hls;


//Display1
#define WFV_V_SHIFT 8
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2
#define DECOUP_SHIFT 19
#define CMD_SHIFT 3
#define ASW1_SHIFT 20
#define TCP_OPERATION_SHIFT 18

//Display2 
#define ASW2_SHIFT 0
#define ASW3_SHIFT 8
#define ASW4_SHIFT 16
#define NOT_TO_SWAP_SHIFT 24

//Display3
#define RCNT_SHIFT 24
#define MSG_SHIFT 0

//Display4
#define ANSWER_LENGTH_SHIFT 0
#define HTTP_STATE_SHIFT 4
#define WRITE_ERROR_CNT_SHIFT 8
#define EMPTY_FIFO_CNT_SHIFT 16
#define GLOBAL_STATE_SHIFT 24

//Display6
#define RANK_SHIFT 0
#define SIZE_SHIFT 8
#define LAST_PAGE_WRITE_CNT_SHIFT 16
#define PYRO_SEND_REQUEST_SHIFT 23

//Display7
#define RX_SESS_STATE_SHIFT 0
#define RX_DATA_STATE_SHIFT 4
#define TX_SESS_STATE_SHIFT 8
#define TX_DATA_STATE_SHIFT 12
#define TCP_ITER_COUNT_SHIFT 16
#define DETECTED_HTTPNL_SHIFT 24

//Display8
#define TCP_RX_CNT_SHIFT 0

//Display9
#define BUFFER_IN_MAX_SHIFT 0
#define FULL_FIFO_CNT_SHIFT 16
#define ICAP_FSM_SHIFT 24


#define WS 4
#define SR_OFFSET (0x110/WS)
#define ISR_OFFSET (0x20/WS)
#define WFV_OFFSET (0x114/WS)
#define ASR_OFFSET (0x11C/WS)
#define CR_OFFSET (0x10C/WS)
#define RFO_OFFSET (0x118/WS)
#define WF_OFFSET (0x100/WS)

//#define FMC_VERSION 0xA
//#define FMC_VERSION_SHIFT 8

// MMIO TO FMC Register
#define DECOUP_CMD_SHIFT 0
#define RST_SHIFT 1
#define DSEL_SHIFT 28
//#define WCNT_SHIFT 8
#define START_SHIFT 12
#define SWAP_N_SHIFT 16
#define CHECK_PATTERN_SHIFT 13
#define PARSE_HTTP_SHIFT 14
#define SOFT_RST_SHIFT 2
#define PYRO_MODE_SHIFT 15
#define PYRO_READ_REQUEST_SHIFT 3
#define ENABLE_TCP_MODE_SHIFT 4
#define ENABLE_FAKE_HWICAP_SHIFT 5
#define LAST_PAGE_CNT_SHIFT 17 //until 23, including

//XMEM
#define LINES_PER_PAGE 32
#define MAX_PAGES 16
#define XMEM_SIZE (LINES_PER_PAGE * MAX_PAGES)
#define BYTES_PER_PAGE (LINES_PER_PAGE*4)
#define PAYLOAD_BYTES_PER_PAGE (BYTES_PER_PAGE - 2)

//should be smaller then 2^16, but much bigger than a usual HTTP Header (~ 200 Bytes)
//#define IN_BUFFER_SIZE 2048
//#define IN_BUFFER_SIZE 8192 //8K
#define IN_BUFFER_SIZE 4096 //4K, = HWICAP FIFO DEPTH (in Bytes)
//#define IN_BUFFER_SIZE 6144 //6K
#define OUT_BUFFER_SIZE 1024
#define XMEM_ANSWER_START (1*LINES_PER_PAGE) //Lines! not Bytes

//HWICAP CR Commands
#define CR_ABORT 0x10
#define CR_SWRST 0x8
#define CR_FICLR 0x4
#define CR_READ  0x2
#define CR_WRITE 0x1

#define HWICAP_FIFO_DEPTH 1023
#define MAX_HWICAP_DATA_CHUNK_BYTES ((4*HWICAP_FIFO_DEPTH) - 24)
#define HWICAP_FIFO_NEARLY_FULL_TRIGGER 4

#include "../../NAL/src/nal.hpp" 
//MAX CLUSTER/MAX RANK
//only one limit is enough, there is no rank > clusterSize...
//#define MAX_CLUSTER_SIZE 128
#define MAX_CLUSTER_SIZE (MAX_MRT_SIZE)


#define MIN_ROUTING_TABLE_LINE (1+1+4+1) //1: rank, 1: space, 4: IPv4-Address, 1: \n 


//HTTP State
#define HTTP_IDLE 0
#define HTTP_PARSE_HEADER 1
#define HTTP_HEADER_PARSED 2
#define HTTP_READ_PAYLOAD 3
#define HTTP_REQUEST_COMPLETE 4
#define HTTP_SEND_RESPONSE 5
#define HTTP_INVALID_REQUEST 6
#define HTTP_DONE 7
#define HttpState uint8_t

//Global Opperations
#define GLOBAL_IDLE 0
#define GLOBAL_XMEM_CHECK_PATTERN 1
#define GLOBAL_XMEM_TO_HWICAP 2
#define GLOBAL_XMEM_HTTP 3
#define GLOBAL_TCP_HTTP 4
#define GLOBAL_PYROLINK_RECV 5
#define GLOBAL_PYROLINK_TRANS 6
#define GLOBAL_MANUAL_DECOUPLING 7
//#define GLOBAL_TCP_TO_HWICAP 8
#define GlobalState uint8_t

//Operations return values (one hot encoded)
#define OprvType uint8_t
#define OPRV_OK 0x1
#define OPRV_FAIL 0x2
#define OPRV_SKIPPED 0x4
#define OPRV_NOT_COMPLETE 0x8
#define OPRV_PARTIAL_COMPLETE 0x10
#define OPRV_DONE 0x20
#define OPRV_USER 0x40
#define MASK_ALWAYS 0xFF

//TCP FSMs state
#define TCP_FSM_RESET 0
#define TCP_FSM_IDLE  1
#define TCP_FSM_W84_START 2
#define TCP_FSM_PROCESS_DATA 3
#define TCP_FSM_DONE 4
#define TCP_FSM_ERROR 5
//#define TCP_FSM_PROCESS_SECOND 7
#define TcpFsmState uint8_t

//HWICAP FSM state
#define ICAP_FSM_RESET 0
#define ICAP_FSM_IDLE 1
#define ICAP_FSM_WRITE 2
#define ICAP_FSM_DONE 3
#define ICAP_FSM_ERROR 4
#define ICAP_FSM_DRAIN 5
#define IcapFsmState uint8_t
#define ICAP_FIFO_POISON_PILL 0x0a0b1c2d //isn't a valid JTAG?
#define ICAP_FIFO_POISON_PILL_REVERSE 0x2d1c0b0a


//FPGA state registers
#define NUMBER_FPGA_STATE_REGISTERS 8
#define FPGA_STATE_LAYER_4 0
#define FPGA_STATE_LAYER_6 1
#define FPGA_STATE_LAYER_7 2
#define FPGA_STATE_CONFIG_UPDATE 3
#define FPGA_STATE_MRT_UPDATE 4
#define FPGA_STATE_NTS_READY 5

//ctrl link interval
#define CHECK_CTRL_LINK_INTERVAL_SECONDS 2
#define CHECK_HWICAP_INTERVAL_SECONDS 2

#define LINKFSM_IDLE 0
#define LINKFSM_WAIT 1
#define LINKFSM_UPDATE_MRT 2
#define LINKFSM_UPDATE_CONFIG 3
#define LINKFSM_UPDATE_STATE 4
#define LINKFSM_UPDATE_SAVED_STATE 5
#define LinkFsmStateType uint8_t



//Internal Opcodes
#define OpcodeType uint8_t
#define OP_NOP                          0
#define OP_ENABLE_XMEM_CHECK_PATTERN    1
#define OP_XMEM_COPY_DATA               2
#define OP_FILL_BUFFER_TCP              3
#define OP_HANDLE_HTTP                  4
#define OP_CHECK_HTTP_EOR               5
#define OP_CHECK_HTTP_EOP               6
#define OP_BUFFER_TO_HWICAP             7
#define OP_BUFFER_TO_PYROLINK           8
#define OP_BUFFER_TO_ROUTING            9
#define OP_SEND_BUFFER_TCP              10
#define OP_SEND_BUFFER_XMEM             11
#define OP_CLEAR_IN_BUFFER              12
#define OP_CLEAR_OUT_BUFFER             13
#define OP_DONE                         14
#define OP_DISABLE_XMEM_CHECK_PATTERN   15
#define OP_ACTIVATE_DECOUP              16
#define OP_DEACTIVATE_DECOUP            17
#define OP_ABORT_HWICAP                 18
#define OP_FAIL                         19
#define OP_UPDATE_HTTP_STATE            20
#define OP_COPY_REQTYPE_TO_RETURN       21
#define OP_EXIT                         22
#define OP_ENABLE_SILENT_SKIP           23
#define OP_DISABLE_SILENT_SKIP          24
#define OP_OK                           25
#define OP_PYROLINK_TO_OUTBUFFER        26
#define OP_WAIT_FOR_TCP_SESS            27
#define OP_SEND_TCP_SESS                28
#define OP_SET_NOT_TO_SWAP              29
#define OP_UNSET_NOT_TO_SWAP            30
#define OP_ACTIVATE_CONT_TCP            31
#define OP_DEACTIVATE_CONT_TCP          32
#define OP_TCP_RX_STOP_ON_EOR           33
#define OP_TCP_RX_STOP_ON_EOP           34
#define OP_TCP_CNT_RESET                35
#define OP_FIFO_TO_HWICAP               36
#define OP_CLEAR_ROUTING_TABLE          37


#define MAX_PROGRAM_LENGTH 64



#define GLOBAL_MAX_WAIT_COUNT (5 * 1024)

extern uint8_t bufferIn[IN_BUFFER_SIZE];
extern uint8_t bufferOut[OUT_BUFFER_SIZE];
extern uint32_t bufferInPtrWrite;
extern uint32_t bufferInPtrNextRead;
extern uint16_t bufferOutPtrWrite;
extern uint16_t bufferOutContentLength;

#ifndef __SYNTHESIS__
extern bool use_sequential_hwicap;
extern uint32_t sequential_hwicap_address;
#endif

extern HttpState httpState; 
//extern ap_uint<16> currentPayloadStart;

void emptyInBuffer();
void emptyOutBuffer();
uint32_t writeDisplaysToOutBuffer();

void setRank(ap_uint<32> newRank);
void setSize(ap_uint<32> newSize);

void fmc(
    //EMIF Registers
    ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
    //state of the FPGA
    ap_uint<1> *layer_4_enabled,
    ap_uint<1> *layer_6_enabled,
    ap_uint<1> *layer_7_enabled,
    ap_uint<1> *nts_ready,
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
    ap_uint<32> nalCtrl[NAL_CTRL_LINK_SIZE],
    ap_uint<1> *disable_ctrl_link,
    stream<TcpWord>           &siNAL_Tcp_data,
    stream<AppMeta>           &siNAL_Tcp_SessId,
    stream<TcpWord>           &soNAL_Tcp_data,
    stream<AppMeta>           &soNAL_Tcp_SessId,
#ifdef INCLUDE_PYROLINK
    //Pyrolink
    stream<Axis<8> >  &soPYROLINK,
    stream<Axis<8> >  &siPYROLINK,
    ap_uint<1> *disable_pyro_link,
#endif
    //TO ROLE
    ap_uint<32> *role_rank, ap_uint<32> *cluster_size);


#endif


/*! \} */
