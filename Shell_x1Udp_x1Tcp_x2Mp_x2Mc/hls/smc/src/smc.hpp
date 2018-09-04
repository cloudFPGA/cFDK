#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"
#include "http.hpp"

//Display1
#define WFV_V_SHIFT 8 
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2
#define DECOUP_SHIFT 19
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

//Display6 
#define RANK_SHIFT 0 
#define SIZE_SHIFT 8


//#define AXI_PAUSE_CYCLES 10

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
#define CHECK_PATTERN_SHIFT 13
#define PARSE_HTTP_SHIFT 14

//XMEM
#define LINES_PER_PAGE 32
#define MAX_PAGES 16
#define XMEM_SIZE (LINES_PER_PAGE * MAX_PAGES)
#define BYTES_PER_PAGE (LINES_PER_PAGE*4)
#define PAYLOAD_BYTES_PER_PAGE (BYTES_PER_PAGE - 2)
#define BUFFER_SIZE 2048 //should be smaller then 2^16, but much bigger than a usual HTTP Header (~ 200 Bytes)
#define XMEM_ANSWER_START (1*LINES_PER_PAGE) //Lines! not Bytes!

//HWICAP CR Commands 
#define CR_ABORT 0x10
#define CR_SWRST 0x8 
#define CR_FICLR 0x4 
#define CR_READ  0x2 
#define CR_WRITE 0x1

//MAX CLUSTER/MAX RANK 
#define MAX_CLUSTER_SIZE 1024  //only one limit is enough, there is no rank > clusterSize...
#include "../../mpe/src/mpe.hpp" //seems to have a dependency to MAX_CLUSTER_SIZE, so must be after it...

//Cosim enum Bug fix
//#define COSIM 
#ifdef COSIM

#define HTTP_IDLE 0
#define HTTP_PARSE_HEADER 1
#define HTTP_HEADER_PARSED 2
#define HTTP_READ_PAYLOAD 3
#define HTTP_REQUEST_COMPLETE 4
#define HTTP_SEND_RESPONSE 5
#define HTTP_INVALID_REQUEST 6
#define HTTP_DONE 7 
#define HttpState uint8_t 

#else 
typedef enum { HTTP_IDLE = 0, 
            HTTP_PARSE_HEADER = 1, HTTP_HEADER_PARSED = 2, 
            HTTP_READ_PAYLOAD = 3, HTTP_REQUEST_COMPLETE = 4,
            HTTP_SEND_RESPONSE = 5, HTTP_INVALID_REQUEST =  6,
            HTTP_DONE = 7 } HttpState; 
#endif

extern ap_uint<8> bufferIn[BUFFER_SIZE];
extern ap_uint<8> bufferOut[BUFFER_SIZE];
extern ap_uint<16> bufferInPtrWrite;
extern ap_uint<16> bufferInPtrNextRead;
extern ap_uint<16> bufferOutPtrWrite;
extern ap_uint<4> httpAnswerPageLength;

extern HttpState httpState; 
//extern ap_uint<16> currentPayloadStart;

void emptyInBuffer();
void emptyOutBuffer();
uint8_t writeDisplaysToOutBuffer();

void setRank(ap_uint<32> newRank);
void setSize(ap_uint<32> newSize);

void smc_main(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    //EMIF Registers
    ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
    //HWICAP and DECOUPLING
    ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
    //XMEM
    ap_uint<32> xmem[XMEM_SIZE], 
    //MPE 
    //ap_uint<32> mpeCtrl[MPE_NUMBER_CONFIG_WORDS + MPE_NUMBER_STATUS_WORDS + MAX_CLUSTER_SIZE],
    ap_uint<32> mpeCtrl[MPE_CTRL_LINK_SIZE],
    //TO ROLE 
    ap_uint<32> *role_rank, ap_uint<32> *cluster_size);


#endif
