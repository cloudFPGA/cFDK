#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"

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


#define WAIT_CYCLES 10
#define AXI_PAUSE_CYCLES 10

#define WS 4
#define SR_OFFSET (0x110/WS)
#define ISR_OFFSET (0x20/WS)
#define WFV_OFFSET (0x114/WS)
#define ASR_OFFSET (0x11C/WS)
#define CR_OFFSET (0x10C/WS)
#define RFO_OFFSET (0x118/WS)

#define SMC_VERSION 0xA
#define SMC_VERSION_SHIFT 8

// MMIO TO SMC Register
#define DECOUP_CMD_SHIFT 0
#define DSEL_SHIFT 28
#define WCNT_SHIFT 8

//XMEM
#define MAX_LINES 32
#define MAX_PAGES 2
#define XMEM_SIZE (MAX_LINES * MAX_PAGES)

void smc_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
			ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
			ap_uint<32> xmem[XMEM_SIZE]);


#endif
