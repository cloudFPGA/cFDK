
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"

ap_uint<4> cnt = 0xF;
ap_uint<1> transferErr = 0;
char *msg = new char[4];


ap_uint<4> copyAndCheckBurst(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt)
{

	ap_uint<32> buffer[MAX_LINES];
	//ap_uint<32> curLine = xmem[0];

	for(int i = 0; i<MAX_LINES; i++)
	{
		buffer[i] = xmem[i];
	}

	//ap_uint<8> curHeader = (buffer[0] >> 24) & 0xff; No! 
	ap_uint<8> curHeader = buffer[0] & 0xff;
	ap_uint<8> curFooter = (buffer[MAX_LINES-1] >> 24) & 0xff;
	ap_uint<4> curCnt = curHeader & 0xf; 


	if ( curHeader != curFooter)
	{//page is invalid 
		// NO! We are in the middle of a transfer!
		return 1;
	}
	
	if(curCnt == cnt)
	{//page was already transfered
		return 0;
	}

	if (curCnt != ExpCnt)
	{//we must missed something 
		return 2;
	}

	bool lastPage = (curHeader & 0xf0) == 0xf0;

	ap_uint<32> ctrlWord = 0;
	for(int i = 0; i<8; i++)
	{
		ctrlWord |= ((ap_uint<32>) ExpCnt) << (i*4);
	}

	//for simplicity check only lines in between
	for(int i = 1; i<MAX_LINES-1; i++)
	{
		if(buffer[i] != ctrlWord)
		{//data is corrupt 
			return 3;
		}
	}

	if (lastPage)
	{
		return 4;
	}

	return 5;
}

/*
ap_uint<4> copyAndCheckXmem(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt)
{
	ap_uint<32> buffer[MAX_LINES];
//#pragma HLS RESOURCE variable=buffer core=ROM_1P_BRAM

//	if (ExpCnt % 2 == 0)
//	{//even page
		for(int i = 0; i<MAX_LINES; i++)
		{
			buffer[i] = xmem[i];
		}
//	} else { //odd page
//		for(int i = 0; i<MAX_LINES; i++)
//		{
//			buffer[i] = xmem[i+MAX_LINES];
//		}
//	}

	ap_uint<32> ctrlWord = 0;
	for(int i = 0; i<8; i++)
	{
		ctrlWord |= ((ap_uint<32>) ExpCnt) << (i*4);
	}

	for(int i = 0; i<MAX_LINES; i++)
	{
		if(buffer[i] != ctrlWord)
		{
			return 1;
		}
	}

	return 0;
}
*/

void smc_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
			ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
			ap_uint<32> xmem[XMEM_SIZE])
{
#pragma HLS RESOURCE variable=xmem core=RAM_1P_BRAM
#pragma HLS INTERFACE m_axi depth=512 port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_stable register port=decoupStatus name=piDECOUP_SMC_status
#pragma HLS INTERFACE ap_ovld register port=setDecoup name=poSMC_DECOUP_activate


//===========================================================
// Core-wide variables
	ap_uint<32> SR = 0, ISR = 0, WFV = 0, ASR = 0, CR = 0, RFO = 0;
	ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
	ap_uint<32> WFV_value = 0, CR_value = 0;


	ap_uint<8> ASW1 = 0, ASW2 = 0, ASW3 = 0, ASW4= 0;

//===========================================================
// Connection to HWICAP

	SR = HWICAP[SR_OFFSET];
	//ap_wait_n(AXI_PAUSE_CYCLES);
	ISR = HWICAP[ISR_OFFSET];
	//	ap_wait_n(AXI_PAUSE_CYCLES);
	WFV = HWICAP[WFV_OFFSET];

	ASR = HWICAP[ASR_OFFSET];
	//RFO = HWICAP[RFO_OFFSET];
	CR = HWICAP[CR_OFFSET];

	Done = SR & 0x1;
	EOS  = (SR & 0x4) >> 2;
	WEMPTY = (ISR & 0x4) >> 2;
	WFV_value = WFV & 0x3FF;
	CR_value = CR & 0x1F; 

	ASW1 = ASR & 0xFF;
	ASW2 = (ASR & 0xFF00) >> 8;
	ASW3 = (ASR & 0xFF0000) >> 16;
	ASW4 = (ASR & 0xFF000000) >> 24;

//===========================================================
// Decoupling 
	ap_uint<1> toDecoup = (*MMIO_in >> DECOUP_CMD_SHIFT) & 0b1;

	if ( toDecoup == 1 )
	{
		*setDecoup = 0b1;
	} else {
		*setDecoup = 0b0;
	}

//===========================================================
// Reset global variables 

	ap_uint<1> RST = (*MMIO_in >> RST_SHIFT) & 0b1; 

	if (RST == 1)
	{
		cnt = 0xf;
		transferErr = 0;
		msg = "IDL";
		
	} 

//===========================================================
// Counter Handshake and Memory Copy v1
	/*
	ap_uint<4> Wcnt = (*MMIO_in >> WCNT_SHIFT) & 0xF; 
	//msg = "ABC";

	//explicit overflow 
	ap_uint<4> expCnt = cnt + 1;
	if (cnt == 0xf)
	{
		expCnt = 0;
	}

	if (Wcnt == expCnt )
	{ 
		ap_uint<4> ret = copyAndCheckXmem(xmem,expCnt);
		if ( ret == 0)
		{
			cnt = expCnt;
			msg = " OK";
		} else {
			msg = "INV"; //Invalid data
		}
	} else if (Wcnt == cnt ) 
	{ 
		msg = "UTD"; //Up-to-date
	} else {
		msg = "CMM"; //Counter Miss match
	}
	*/
//===========================================================
// Start & Run Burst transfer 
	
	ap_uint<1> start = (*MMIO_in >> START_SHIFT) & 0b1;

	if (start == 1 && transferErr == 0) 
	{
		//explicit overflow 
		ap_uint<4> expCnt = cnt + 1;
		if (cnt == 0xf)
		{
			expCnt = 0;
		} 

		ap_uint<4> ret = copyAndCheckBurst(xmem,expCnt);

		switch (ret) {
			case 0:
							 msg = "UTD";
								 break;
			case 1:
							 msg = "INV";
							 //transferErr = 1; NO!
								 break;
			case 2:
							 msg = "CMM";
							 transferErr = 1;
								 break;
			case 3:
							 msg = "COR";
							 transferErr = 1;
								 break;
			case 4:
							 msg = "SUC";
							 //set error to not start again 
							 transferErr=1;
							 cnt = expCnt;
								 break;
			default: 
							 msg= " OK";
							 cnt = expCnt;
							 break;
		}

	} else if (transferErr == 0 && start == 0)
	{
		msg = "IDL";
	}
	//otherwise don't touch msg 


//===========================================================
//  putting displays together 


	ap_uint<32> Display1 = 0, Display2 = 0, Display3 = 0; 
	ap_uint<4> Dsel = 0;

	Dsel = (*MMIO_in >> DSEL_SHIFT) & 0xF;

	Display1 = (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | EOS;
	Display1 |= WFV_value << WFV_V_SHIFT;
	//Display1 |= RFO << WFV_V_SHIFT;
	//Display1 |= (decoupStatus | 0x00000000)  << DECOUP_SHIFT;
	Display1 |= ((ap_uint<32>) decoupStatus)  << DECOUP_SHIFT;
	Display1 |= ((ap_uint<32>) ASW1) << ASW1_SHIFT;
	Display1 |= ((ap_uint<32>) CR_value) << CMD_SHIFT;

	Display2 = ((ap_uint<32>) ASW2) << ASW2_SHIFT;
	Display2 |= ((ap_uint<32>) ASW3) << ASW3_SHIFT;
	Display2 |= ((ap_uint<32>) ASW4) << ASW4_SHIFT;

	Display3 = ((ap_uint<32>) cnt) << RCNT_SHIFT;
	Display3 |= ((ap_uint<32>) msg[0]) << MSG_SHIFT + 16;
	Display3 |= ((ap_uint<32>) msg[1]) << MSG_SHIFT + 8;
	Display3 |= ((ap_uint<32>) msg[2]) << MSG_SHIFT + 0;

	switch (Dsel) {
		case 1:
				*MMIO_out = (0x1 << DSEL_SHIFT) | Display1;
					 break;
		case 2:
				*MMIO_out = (0x2 << DSEL_SHIFT) | Display2;
					 break;
		case 3:
				*MMIO_out = (0x3 << DSEL_SHIFT) | Display3;
					 break;
		default: 
				*MMIO_out = 0xBEBAFECA;
					break;
	}

	//ap_wait_n(WAIT_CYCLES);
	return;
}



