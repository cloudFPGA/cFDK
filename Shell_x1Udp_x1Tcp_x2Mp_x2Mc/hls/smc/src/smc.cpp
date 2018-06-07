
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"

void smc_main(ap_uint<32> *MMIO, ap_uint<32> *HWICAP)
{
//#pragma HLS INTERFACE m_axi depth=1 port=SR offset=0x110 bundle=poSMC_to_HWICAP_AXIM
//#pragma HLS INTERFACE m_axi depth=1 port=ISR offset=0x20 bundle=poSMC_to_HWICAP_AXIM
//#pragma HLS INTERFACE m_axi depth=1 port=WFV offset=0x114 bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE m_axi port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_none register port=MMIO name=pioMMIO

// #pragma HLS INTERFACE s_axilite port=return bundle=BUS_A

	ap_uint<16> Done = 0, EOS = 0, WEMPTY = 0, WFV_value = 0;

	ap_uint<32> SR = 0, ISR = 0, WFV = 0;

	//TODO: also read Abort Status Register -> if CRC fails

	while(true){

		SR = HWICAP[SR_OFFSET];
		ISR = HWICAP[ISR_OFFSET];
		WFV = HWICAP[WFV_OFFSET];

		Done = SR & 0x1;
		EOS  = SR & 0x4;
		WEMPTY = ISR & 0x4;
		WFV_value = WFV & 0x7FF;

		*MMIO = (WFV_value << WFV_V_SHIFT) | (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | (EOS >> 2);

		ap_wait_n(WAIT_CYCLES);


#ifdef DEBUG
		break;
#endif
	}

}



