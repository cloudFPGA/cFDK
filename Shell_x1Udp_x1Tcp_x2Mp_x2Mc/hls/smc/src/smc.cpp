
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"

ap_uint<4> cnt = 0;
bool inputWasSet = false;

void smc_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out, ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup)
{
#pragma HLS INTERFACE m_axi depth=512 port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_stable register port=decoupStatus name=piDECOUP_SMC_status
#pragma HLS INTERFACE ap_ovld register port=setDecoup name=poSMC_DECOUP_activate
// #pragma HLS INTERFACE s_axilite port=return bundle=BUS_A
// #pragma HLS INTERFACE ap_ctrl_none port=return

	ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
	ap_uint<32> WFV_value = 0;

	ap_uint<32> SR = 0, ISR = 0, WFV = 0;



	*setDecoup = 0b0;

	//TODO: also read Abort Status Register -> if CRC fails

		SR = HWICAP[SR_OFFSET];
		//ap_wait_n(AXI_PAUSE_CYCLES);
		ISR = HWICAP[ISR_OFFSET];
	//	ap_wait_n(AXI_PAUSE_CYCLES);
		WFV = HWICAP[WFV_OFFSET];

		Done = SR & 0x1;
		EOS  = (SR & 0x4) >> 2;
		WEMPTY = (ISR & 0x4) >> 2;
		WFV_value = WFV & 0x3FF;

		*MMIO_out = (WFV_value << WFV_V_SHIFT) | (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | EOS;
		*MMIO_out |= (decoupStatus | 0x0) << DECOUP_SHIFT;
		*MMIO_out |= SMC_VERSION << SMC_VERSION_SHIFT;


		ap_uint<1> toIncr = (*MMIO_in >> INCR_SHIFT) & 0b1;

		if ( toIncr == 1 && !inputWasSet)
		{
			cnt++;
			inputWasSet = true;
		}

		if ( toIncr == 0 && inputWasSet)
		{
			inputWasSet = false;
		}

		*MMIO_out |= (cnt | 0x0000) << CNT_SHIFT;


		ap_wait_n(WAIT_CYCLES);
}



