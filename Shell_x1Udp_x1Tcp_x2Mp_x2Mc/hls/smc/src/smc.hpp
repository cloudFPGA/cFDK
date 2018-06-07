#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"


#define WFV_V_SHIFT 19
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2

#define WAIT_CYCLES 1000

#define SR_OFFSET 0x110
#define ISR_OFFSET 0x20
#define WFV_OFFSET 0x114

//void smc_main(ap_uint<32> *MMIO, ap_uint<32> *SR, ap_uint<32> *ISR, ap_uint<32> *WFV);
void smc_main(ap_uint<32> *MMIO, ap_uint<32> *HWICAP);

#endif
