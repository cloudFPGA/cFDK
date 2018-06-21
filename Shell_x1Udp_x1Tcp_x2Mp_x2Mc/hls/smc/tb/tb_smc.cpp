
#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "../src/smc.hpp"


bool checkResult(ap_uint<32> MMIO, ap_uint<32> expected)
{
	if (MMIO == expected)
	{
		printf("expected result: %#010x\n", (int) MMIO);
		return true;
	}

	printf("ERROR: Got %#010x instead of %#010x\n", (int) MMIO, (int) expected);
	return false;
	//exit -1;
}

void initBuffer(ap_uint<4> cnt,ap_uint<32> xmem[XMEM_SIZE] )
{
	ap_uint<32> ctrlWord = 0;
	for(int i = 0; i<8; i++)
	{
		ctrlWord |= ((ap_uint<32>) cnt) << (i*4);
	}
	for(int i = 0; i<MAX_LINES; i++)
	{
	//	if (cnt % 2 == 0)
	//	{
			xmem[i] = ctrlWord;
	//	} else {
	//		xmem[i+MAX_LINES] = ctrlWord;
	//	}
	}
	//printf("CtrlWord: %#010x\n",(int) ctrlWord);
}


int main(){

	ap_uint<32> MMIO;
	ap_uint<32> SR;
	ap_uint<32> ISR;
	ap_uint<32> WFV;
	ap_uint<1>  decoupActive = 0b0;

	ap_uint<32> HWICAP[512];
	ap_uint<32> xmem[XMEM_SIZE];

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


	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0xBEBAFECA) && succeded;

//===========================================================
//Test Displays
	MMIO_in = 0x1 << DSEL_SHIFT;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0x1003FF07) && succeded && (decoupActive == 0);

	MMIO_in = 0x2 << DSEL_SHIFT;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0x20000000) && succeded && (decoupActive == 0);

	MMIO_in = 0x1 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0x1007FF07) && succeded && (decoupActive == 1);

	MMIO_in = 0x3 << DSEL_SHIFT | 0b1 << DECOUP_CMD_SHIFT;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0x30555444) && succeded && (decoupActive == 1);

	MMIO_in = 0x1 << DSEL_SHIFT;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b1, &decoupActive, xmem);
	printf("%#010x\n", (int) MMIO);
	succeded = (MMIO == 0x1007FF07) && succeded && (decoupActive == 0);

//===========================================================
//Test Counter & Xmem

	int cnt = 1;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt,xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x31204F4B);

	cnt = 2;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x32204F4B);

	cnt = 3;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	//initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x32494E56);

	//cnt = 3;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x33204F4B);

	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x33555444);

	cnt = 4;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	xmem[1] = 42;
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x33494E56);

	cnt = 5;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	//initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x33434D4D);

	cnt = 4;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x34204f4b);
	
	//Test RST
	MMIO_in = 0x3 << DSEL_SHIFT | ( 0 << WCNT_SHIFT) | (1 << RST_SHIFT);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x30555444);

	for(int i = 0; i<0xf; i++)
	{
	cnt = i;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);

	}
	
	cnt = 0xf;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x3f204f4b);

	cnt = 0x0;
	MMIO_in = 0x3 << DSEL_SHIFT | ( cnt << WCNT_SHIFT);
	initBuffer((ap_uint<4>) cnt, xmem);
	smc_main(&MMIO_in, &MMIO, HWICAP, 0b0, &decoupActive, xmem);
	succeded &= checkResult(MMIO, 0x30204f4b);


	return succeded? 0 : -1;
	//return 0;
}
