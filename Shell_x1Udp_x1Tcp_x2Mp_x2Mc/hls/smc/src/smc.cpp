
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"

//TODO: static variables?
ap_uint<4> cnt = 0;

void smc_main(ap_uint<32> *MMIO, ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
//#pragma HLS INTERFACE m_axi depth=1 port=SR offset=0x110 bundle=poSMC_to_HWICAP_AXIM
//#pragma HLS INTERFACE m_axi depth=1 port=ISR offset=0x20 bundle=poSMC_to_HWICAP_AXIM
//#pragma HLS INTERFACE m_axi depth=1 port=WFV offset=0x114 bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE m_axi depth=256 port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_none register port=MMIO name=pioMMIO
#pragma HLS INTERFACE ap_none register port=decoupStatus name=piDECOUP_SMC_status
#pragma HLS INTERFACE ap_none register port=setDecoup name=poSMC_DECOUP_activate
// #pragma HLS INTERFACE s_axilite port=return bundle=BUS_A

	ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
	ap_uint<32> WFV_value = 0;

	ap_uint<32> SR = 0, ISR = 0, WFV = 0;

	*setDecoup = 0b0;

	//TODO: also read Abort Status Register -> if CRC fails

	//while(true){

		SR = HWICAP[SR_OFFSET];
		ISR = HWICAP[ISR_OFFSET];
		WFV = HWICAP[WFV_OFFSET];

		Done = SR & 0x1;
		EOS  = (SR & 0x4) >> 2;
		WEMPTY = (ISR & 0x4) >> 2;
		WFV_value = WFV & 0x3FF;

		*MMIO = (WFV_value << WFV_V_SHIFT) | (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | EOS;
		*MMIO |= (decoupStatus | 0x0) << DECOUP_SHIFT;
		*MMIO |= SMC_VERSION << SMC_VERSION_SHIFT;
		*MMIO |= (cnt | 0x0000) << CNT_SHIFT;

		cnt++;

		ap_wait_n(WAIT_CYCLES);

		//cnt = 0xf;

		//*MMIO |=  (cnt | 0x0000) << CNT_SHIFT;

		//ap_wait_n(WAIT_CYCLES);

//#ifdef DEBUG
//		break;
//#endif
	//}

}



