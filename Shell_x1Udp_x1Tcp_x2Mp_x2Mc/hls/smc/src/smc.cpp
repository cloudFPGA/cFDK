
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"

ap_uint<4> cnt = 0;

ap_uint<4> copyAndCheckXmem(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> cnt)
{
	ap_uint<32> buffer[MAX_LINES];
//#pragma HLS RESOURCE variable=buffer core=ROM_1P_BRAM

	if (cnt % 2 == 0)
	{//even page
		for(int i = 0; i<MAX_LINES; i++)
		{
			buffer[i] = xmem[i];
		}
	} else { //odd page
		for(int i = 0; i<MAX_LINES; i++)
		{
			buffer[i] = xmem[i+MAX_LINES];
		}
	}

	ap_uint<32> ctrlWord = 0;
	for(int i = 0; i<8; i++)
	{
		ctrlWord |= ((ap_uint<32>) cnt) << (i*4);
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
// Connection to HWICAP
	ap_uint<32> SR = 0, ISR = 0, WFV = 0, ASR = 0, CR = 0, RFO = 0;
	ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
	ap_uint<32> WFV_value = 0, CR_value = 0;


	ap_uint<8> ASW1 = 0, ASW2 = 0, ASW3 = 0, ASW4= 0;


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
// Counter Handshake and Memory Copy
	
	ap_uint<4> Wcnt = (*MMIO_in >> WCNT_SHIFT) & 0xF; 
	char *msg = new char[4];
	//msg = "ABC";

	if (Wcnt == (cnt + 1) )
	{ 
		ap_uint<4> ret = copyAndCheckXmem(xmem,(cnt+1));
		if ( ret == 0)
		{
			cnt++;
			msg = " OK";
		} else {
			msg = "INV"; //Invalid data
		}
	} else if (Wcnt == cnt ) 
	{ 
		if (cnt == 0)
		{
			msg = "UNU"; // unused
		} else {
			msg = "UTD"; //Up-to-date
		}
	} else {
		msg = "CMM"; //Counter Miss match
	}
	
//===========================================================
//

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
	Display1 |= ASW1 << ASW1_SHIFT;
	Display1 |= CR_value << CMD_SHIFT; 

	Display2 |= ASW2 << ASW2_SHIFT;
	Display2 |= ASW3 << ASW3_SHIFT;
	Display2 |= ASW4 << ASW4_SHIFT; 

	Display3 |= ((ap_uint<32>) cnt) << RCNT_SHIFT;
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

	ap_wait_n(WAIT_CYCLES);
}



