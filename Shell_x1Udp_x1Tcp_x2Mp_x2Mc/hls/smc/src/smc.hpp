#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"
#include "http.hpp"

//Display1
#define WFV_V_SHIFT 8 
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2
#define DECOUP_SHIFT 18
#define CMD_SHIFT 3
#define ASW1_SHIFT 20

//Display2 
#define ASW2_SHIFT 0
#define ASW3_SHIFT 8
#define ASW4_SHIFT 16

//Display3
#define RCNT_SHIFT 24
#define MSG_SHIFT 0

//Display4
#define ANSWER_LENGTH_SHIFT 0
#define HTTP_STATE_SHIFT 4


//#define WAIT_CYCLES 10
//#define AXI_PAUSE_CYCLES 10
#define LOOP_WAIT_CYCLES 30

#define WS 4
#define SR_OFFSET (0x110/WS)
#define ISR_OFFSET (0x20/WS)
#define WFV_OFFSET (0x114/WS)
#define ASR_OFFSET (0x11C/WS)
#define CR_OFFSET (0x10C/WS)
#define RFO_OFFSET (0x118/WS)
#define WF_OFFSET (0x100/WS)

#define SMC_VERSION 0xA
#define SMC_VERSION_SHIFT 8

// MMIO TO SMC Register
#define DECOUP_CMD_SHIFT 0
#define RST_SHIFT 1
#define DSEL_SHIFT 28
#define WCNT_SHIFT 8
#define START_SHIFT 12
#define SWAP_N_SHIFT 16
#define CHECK_PATTERN_SHIFT 17
#define PARSE_HTTP_SHIFT 18

//XMEM
#define LINES_PER_PAGE 32
#define MAX_PAGES 16
#define XMEM_SIZE (LINES_PER_PAGE * MAX_PAGES)
#define BYTES_PER_PAGE (LINES_PER_PAGE*4)
#define PAYLOAD_BYTES_PER_PAGE (BYTES_PER_PAGE - 2)
#define BUFFER_SIZE 1024 //should be smaller then 2^16, but much bigger than a usual HTTP Header (~ 200 Bytes)
#define MAX_BUF_ITERS 8 //must be < BUFFER_SIZE/Bytes per Round 
#define XMEM_ANSWER_START (1*LINES_PER_PAGE) //Lines! not Bytes!

//HWICAP CR Commands 
#define CR_ABORT 0x10
#define CR_SWRST 0x8 
#define CR_FICLR 0x4 
#define CR_READ  0x2 
#define CR_WRITE 0x1


extern ap_uint<8> bufferIn[BUFFER_SIZE];
extern ap_uint<8> bufferOut[BUFFER_SIZE];
extern ap_uint<16> currentBufferInPtr;
extern ap_uint<16> currentBufferOutPtr;
extern ap_uint<8> iter_count;
extern ap_uint<4> httpAnswerPageLength;

extern HttpState httpState; 
extern ap_uint<16> currentPayloadStart;

void emptyInBuffer();
void emptyOutBuffer();
void writeDisplaysToOutBuffer();

void smc_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
      ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
      ap_uint<32> xmem[XMEM_SIZE]);


#endif
