
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

	ap_uint<32> HWICAP[512];

	SR=0x5;
	ISR=0x4;
	WFV=0x7FF;

	HWICAP[SR_OFFSET] = SR;
	HWICAP[ISR_OFFSET] = ISR;
	HWICAP[WFV_OFFSET] = WFV;
	HWICAP[ASR_OFFSET] = 0;
	HWICAP[CR_OFFSET] = 0;
	HWICAP[RFO_OFFSET] = 0x3FF;

	bool succeded = true;

	ap_uint<32> MMIO_in = 0x0;


	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive);
	printf("%#010x\n", (int) MMIO);
	//int expected_value = 0x3ff00a0f + (i << CNT_SHIFT);
	//succeded = (MMIO == expected_value) && succeded;
	 
	succeded = (MMIO == 0xffc00a0f) && succeded;

	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive);
    printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0xffc00a0f) && succeded;

	MMIO_in = 0b1;
		smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc01a07) && succeded;

	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc01a07) && succeded;

	MMIO_in = 0b0;
		smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc01a07) && succeded;

	MMIO_in = 0b1;
		smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc02a07) && succeded;

	MMIO_in = 0x10;
		smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc02a07) && succeded && (decoupActive == 1);

	MMIO_in = 0x0;
		smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive);
		printf("%#010x\n", (int) MMIO);
		succeded = (MMIO == 0xffc02a07) && succeded && (decoupActive == 0);


	return succeded? 0 : -1;
	//return 0;
}
