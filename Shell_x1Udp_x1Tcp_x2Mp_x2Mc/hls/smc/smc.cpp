
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#define WFV_V_SHIFT 19
#define DONE_SHIFT 1
#define WEMPTY_SHIFT 2
#define WAIT_CYCLES 1000

void smc_main(ap_uint<32> *MMIO, ap_uint<32> *SR, ap_uint<32> *ISR, ap_uint<32> *WFV)
{
#pragma HLS INTERFACE m_axi depth=1 port=SR offset=0x110 bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE m_axi depth=1 port=ISR offset=0x20 bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE m_axi depth=1 port=WFV offset=0x114 bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_none register port=MMIO name=pioMMIO

// #pragma HLS INTERFACE s_axilite port=return bundle=BUS_A

	unsigned int Done = 0, EOS = 0, WEMPTY = 0, WFV_value = 0;

	//while(true){

		Done = *SR & 0x1;
		EOS  = *SR & 0x4;
		WEMPTY = *ISR & 0x4;
		WFV_value = *WFV & 0x7FF;

		*MMIO = (WFV_value << WFV_V_SHIFT) | (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | EOS;

		ap_wait_n(WAIT_CYCLES);
	//}

}



