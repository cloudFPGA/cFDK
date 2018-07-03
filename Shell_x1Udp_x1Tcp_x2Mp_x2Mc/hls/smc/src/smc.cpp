
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"
#include "http.hpp"

ap_uint<4> cnt = 0xF;
ap_uint<1> transferErr = 0;
ap_uint<1> transferSuccess = 0;
char *msg = new char[4];
ap_uint<8> bufferIn[BUFFER_SIZE];
ap_uint<8> hangover_0 = 0;
ap_uint<8> hangover_1 = 0;
bool contains_hangover = false;

ap_uint<16> currentBufferInPtr = 0x0;
ap_uint<8> iter_count = 0;

ap_uint<8> bufferOut[BUFFER_SIZE];
ap_uint<16> currentBufferOutPtr = 0x0;



static HttpState httpState = HTTP_IDLE; 


void copyOutBuffer(ap_uint<4> numberOfPages, ap_uint<32> xmem[XMEM_SIZE])
{
	for(int i = 0; i < numberOfPages*128; i++)
	{
		xmem[XMEM_ANSWER_START + i] = bufferOut[i];
	}
	
}


ap_uint<4> copyAndCheckBurst(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt, ap_uint<1> checkPattern)
{

	ap_uint<8> curHeader = 0;
	ap_uint<8> curFooter = 0;

	ap_int<16> buff_pointer = 0-1;
	
	if ( ExpCnt % 2 == 1)
	{
		buff_pointer = 2-1;
		bufferIn[currentBufferInPtr  + 0] = hangover_0;
		bufferIn[currentBufferInPtr  + 1] = hangover_1;
	}


	for(int i = 0; i<LINES_PER_PAGE; i++)
	{
		ap_uint<32> tmp = 0;
		tmp = xmem[i];

		if ( i == 0 )
		{
			curHeader = tmp & 0xff;
		} else {
			buff_pointer++;
			bufferIn[currentBufferInPtr  + buff_pointer] = (tmp & 0xff);
		}

		buff_pointer++;
		bufferIn[currentBufferInPtr  + buff_pointer] = (tmp >> 8) & 0xff;
		buff_pointer++;
		bufferIn[currentBufferInPtr  + buff_pointer] = (tmp >> 16 ) & 0xff;

		if ( i == LINES_PER_PAGE-1) 
		{
			curFooter = (tmp >> 24) & 0xff;
		} else {
			buff_pointer++;
			bufferIn[currentBufferInPtr  + buff_pointer] = (tmp >> 24) & 0xff;
		}
	}

	if (ExpCnt % 2 == 0)
	{
		hangover_0 = bufferIn[currentBufferInPtr  + (LINES_PER_PAGE-1)*4 + 0];
		hangover_1 = bufferIn[currentBufferInPtr  + (LINES_PER_PAGE-1)*4 + 1];
		contains_hangover = false; //means, only 31 lines!
	} else {
		contains_hangover = true; //means, full 32 lines!
	}

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

	if(checkPattern == 1)
	{
		//ap_uint<32> ctrlWord = 0;
		ap_uint<8> ctrlByte = (((ap_uint<8>) ExpCnt) << 4) | ExpCnt;
		/*for(int i = 0; i<8; i++)
		{
			ctrlWord |= ((ap_uint<32>) ExpCnt) << (i*4);
		}*/ 
		
		//printf("ctrlByte: %#010x\n",(int) ctrlByte);

		//for simplicity check only lines in between and skip potentiall hangover 
		for(int i = 3; i< BYTES_PER_PAGE -3; i++)
		{
			//if(bufferIn[currentBufferInPtr  + i] != ctrlWord)
			if(bufferIn[currentBufferInPtr  + i] != ctrlByte)
			{//data is corrupt 
				//printf("corrupt at %d with %#010x\n",i, (int) bufferIn[currentBufferInPtr + i]);
				return 3;
			}
		}
	}

	if (lastPage)
	{
		return 4;
	}

	return 5;
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
// Core-wide variables
	ap_uint<32> SR = 0, ISR = 0, WFV = 0, ASR = 0, CR = 0, RFO = 0;
	ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
	ap_uint<32> WFV_value = 0, CR_value = 0;


	ap_uint<8> ASW1 = 0, ASW2 = 0, ASW3 = 0, ASW4= 0;

	ap_uint<4> httpAnswerPageLength = 0;

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
// Reset global variables 

	ap_uint<1> RST = (*MMIO_in >> RST_SHIFT) & 0b1; 

	if (RST == 1)
	{
		cnt = 0xf;
		transferErr = 0;
		transferSuccess = 0;
		msg = "IDL";
		currentBufferInPtr = 0;
		iter_count = 0;
		httpState = HTTP_IDLE;
	} 

//===========================================================
// Start & Run Burst transfer; Manage Decoupling
	
	ap_uint<1> toDecoup = (*MMIO_in >> DECOUP_CMD_SHIFT) & 0b1;
	
	ap_uint<1> checkPattern = (*MMIO_in >> CHECK_PATTERN_SHIFT ) & 0b1;
	
	ap_uint<1> parseHTTP = (*MMIO_in >> PARSE_HTTP_SHIFT) & 0b1;

	ap_uint<1> start = (*MMIO_in >> START_SHIFT) & 0b1;

	if (start == 1 && transferErr == 0 && transferSuccess == 0) 
	{
		ap_uint<1> wasAbort = (CR_value & CR_ABORT) >> 4;

		//Maybe CR_ABORT is not set 
		if( ASW1 != 0x00)
		{
			wasAbort = 1;
		}

		if( wasAbort == 1) 
		{ //Abort occured in previous cycle
			transferErr = 1;
			msg = "ABR";
		} else {
			//explicit overflow 
			ap_uint<4> expCnt = cnt + 1;
			if (cnt == 0xf)
			{
				expCnt = 0;
			} 

			if (EOS == 1)
			{
				//Activate Decoupling anyway 
				toDecoup = 1;

				ap_uint<4> ret = copyAndCheckBurst(xmem,expCnt, checkPattern);
		
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
									transferSuccess = 1;
									cnt = expCnt;
									break;
					case 5: 
									msg= " OK";
									cnt = expCnt;
									break;
				}

				ap_uint<32> lastLine = LINES_PER_PAGE-1;
				ap_uint<16> currentAddedPayload = BYTES_PER_PAGE-4;
				if (contains_hangover) 
				{
					lastLine = LINES_PER_PAGE;
					currentAddedPayload = BYTES_PER_PAGE -2;
				} 

				//with HTTP the current buffer pointer is not always the start of the payload
				ap_uint<16> currentPayloadStart = currentBufferInPtr;

				if (parseHTTP == 1)
				{
						httpAnswerPageLength = writeHttpStatus(200,currentBufferOutPtr);
						copyOutBuffer(httpAnswerPageLength,xmem);
				}
		
				if (checkPattern == 0)
				{//means: write to HWICAP

					ap_uint<1> CR_isWritting = CR_value & CR_WRITE;

					if (ret == 4 || ret == 5)
					{// write to HWICAP
						
		
						ap_uint<1> notToSwap = (*MMIO_in >> SWAP_N_SHIFT) & 0b1;
						//Turns out: we need to swap => active low

						for( int i = 0; i<lastLine; i++)
						{
							ap_uint<32> tmp = 0; 
							if ( notToSwap == 1) 
							{
								tmp |= (ap_uint<32>) bufferIn[currentPayloadStart  + i*4];
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 1]) <<  8);
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 2]) << 16);
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 3]) << 24);
							} else { 
								//default 
								tmp |= (ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 3];
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 2]) <<  8);
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 1]) << 16);
								tmp |= (((ap_uint<32>) bufferIn[currentPayloadStart  + i*4 + 0]) << 24);
							}
							
							HWICAP[WF_OFFSET] = tmp;
						}

						if (CR_isWritting != 1)
						{
							HWICAP[CR_OFFSET] = CR_WRITE;
						}
					}

					if (ret == 2)
					{//Abort current opperation and exit 
						HWICAP[CR_OFFSET] = CR_ABORT;
						transferErr = 1;
					}

					if (ret == 4)
					{//wait until all is written 
						while(WFV_value < 0x3FF) 
						{
							ap_wait_n(LOOP_WAIT_CYCLES); 
							
							CR = HWICAP[WFV_OFFSET];
							CR_value = CR & 0x1F; 
							wasAbort = (CR_value & CR_ABORT) >> 4;
							CR_isWritting = CR_value & CR_WRITE;

							if(wasAbort == 1)
							{
								transferErr = 1;
								msg = "ABR"; 
								break;
							}

							if(CR_isWritting != 1)
							{
								HWICAP[CR_OFFSET] = CR_WRITE;
							}
							
							WFV = HWICAP[WFV_OFFSET];
							WFV_value = WFV & 0x3FF;
						}
					}
				}
				

				//This also means, every address < currentBufferInPtr is valid
				currentBufferInPtr += currentAddedPayload;
				iter_count++;
				if (iter_count >= MAX_BUF_ITERS) // >= because the next transfer is written to the space of iter_count+1
				{ 
					iter_count = 0;
					currentBufferInPtr = 0;
				}

			} else {
				msg = "INR";
				transferErr = 1;
			}
		}

	} else if (transferErr == 0 && start == 0 && transferSuccess == 0)
	{
		msg = "IDL";
	}
	//otherwise don't touch msg 


//===========================================================
// Decoupling 

	if ( toDecoup == 1 || transferErr == 1)
	{
		*setDecoup = 0b1;
	} else {
		*setDecoup = 0b0;
	}

//===========================================================
//  putting displays together 


	ap_uint<32> Display1 = 0, Display2 = 0, Display3 = 0, Display4 = 0; 
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

	Display4 = ((ap_uint<32>) httpAnswerPageLength) << ANSWER_LENGTH_SHIFT; 
	Display4 |= ((ap_uint<32>) httpState) << HTTP_STATE_SHIFT;

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
		case 4:
				*MMIO_out = (0x4 << DSEL_SHIFT) | Display4;
					break;
		default: 
				*MMIO_out = 0xBEBAFECA;
					break;
	}

	//ap_wait_n(WAIT_CYCLES);
	return;
}



