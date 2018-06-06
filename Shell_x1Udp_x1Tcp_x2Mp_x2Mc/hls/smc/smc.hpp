#ifndef _SMC_H_
#define _SMC_H_

#include "ap_int.h"

void smc_main(ap_uint<32> *MMIO, ap_uint<32> *SR, ap_uint<32> *ISR, ap_uint<32> *WFV);


#endif
