
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "../src/smc.hpp"



int main(){

	ap_uint<32> MMIO;
	ap_uint<32> SR;
	ap_uint<32> ISR;
	ap_uint<32> WFV;
	ap_uint<1>  decoupActive = 0b0;

	ap_uint<32> HWICAP[0x120];

	SR=0x5;
	ISR=0x4;
	WFV=0x7FF;

	HWICAP[SR_OFFSET] = SR;
	HWICAP[ISR_OFFSET] = ISR;
	HWICAP[WFV_OFFSET] = WFV;

	bool succeded = true;

	//for(int i=0; i<5; i++)
	//{
	smc_main(&MMIO, HWICAP, 0b1, &decoupActive);


	printf("%#010x\n", (int) MMIO);
	//int expected_value = 0x3ff00a0f + (i << CNT_SHIFT);

	 //succeded = (MMIO == expected_value) && succeded;
	 
	succeded = (MMIO == 0x3ff00a0f) && succeded;

	//}

	smc_main(&MMIO, HWICAP, 0b0, &decoupActive);


	printf("%#010x\n", (int) MMIO);

	succeded = (MMIO == 0x3ff01a07) && succeded;

	return succeded? 0 : -1;
	//return 0;
}
