
#include <stdio.h>
#include <stdlib.h>
#include "smc.hpp"

int main(){

	ap_uint<32> MMIO;
	ap_uint<32> SR;
	ap_uint<32> ISR;
	ap_uint<32> WFV;

	SR=0x5;
	ISR=0x4;
	WFV=0x7FF;

	smc_main(&MMIO,&SR, &ISR, &WFV);


	printf("%#010x\n", (int) MMIO);

	bool succeded = MMIO == 0x3ff80016;

	return succeded? 0 : -1 ;
}
