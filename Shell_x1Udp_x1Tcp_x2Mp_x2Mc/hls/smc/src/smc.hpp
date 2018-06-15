#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"


#define WFV_V_SHIFT 20
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2
#define DECOUP_SHIFT 3

#define WAIT_CYCLES 10
#define AXI_PAUSE_CYCLES 10

#define SR_OFFSET 0x110
#define ISR_OFFSET 0x20
#define WFV_OFFSET 0x114

#define SMC_VERSION 0xA
#define SMC_VERSION_SHIFT 8

#define CNT_SHIFT 12

#define INCR_SHIFT 0

void smc_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out, ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup);

#endif
