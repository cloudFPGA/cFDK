
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "../src/smc.hpp"



int main(){

	ap_uint<32> MMIO;
	ap_uint<32> SR;
	ap_uint<32> ISR;
	ap_uint<32> WFV;

	ap_uint<32> HWICAP[0x120];

	SR=0x5;
	ISR=0x4;
	WFV=0x7FF;

	HWICAP[SR_OFFSET] = SR;
	HWICAP[ISR_OFFSET] = ISR;
	HWICAP[WFV_OFFSET] = WFV;

	//smc_main(&MMIO,&SR, &ISR, &WFV);
	smc_main(&MMIO, HWICAP);


	printf("%#010x\n", (int) MMIO);

	bool succeded = MMIO == 0x3ff80013;

	return succeded? 0 : -1;
}
