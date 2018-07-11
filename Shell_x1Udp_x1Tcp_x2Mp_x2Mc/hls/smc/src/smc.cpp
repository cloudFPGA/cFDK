
#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "smc.hpp"
#include "http.hpp"

ap_uint<4> cnt = 0xF;
ap_uint<1> transferErr = 0;
ap_uint<1> transferSuccess = 0;
char *msg = new char[4];
ap_uint<8> bufferIn[BUFFER_SIZE];
//ap_uint<8> hangover_0 = 0;
//ap_uint<8> hangover_1 = 0;
//bool contains_hangover = false;
ap_uint<1> toDecoup = 0;
ap_uint<8> writeErrCnt = 0;
ap_uint<8> fifoEmptyCnt = 0;

ap_uint<16> bufferInPtrWrite = 0x0;
ap_uint<16> bufferInPtrRead = 0x0;
//ap_uint<16> lastLine = LINES_PER_PAGE-1;
ap_uint<16> lastLine = 0;
//ap_uint<16> currentAddedPayload = BYTES_PER_PAGE-2;

ap_uint<8> bufferOut[BUFFER_SIZE];
ap_uint<16> bufferOutPtrWrite = 0x0;


ap_uint<4> httpAnswerPageLength = 0;
HttpState httpState = HTTP_IDLE; 
ap_uint<1> ongoingTransfer = 0;

ap_uint<32> Display1 = 0, Display2 = 0, Display3 = 0, Display4 = 0, Display5 = 0; 
ap_uint<24> wordsWrittenToIcapCnt = 0;

void copyOutBuffer(ap_uint<4> numberOfPages, ap_uint<32> xmem[XMEM_SIZE], ap_uint<1> notToSwap)
{
  for(int i = 0; i < numberOfPages*LINES_PER_PAGE; i++)
  {
    ap_uint<32> tmp = 0; 

 //   if (notToSwap == 1)
 //   {
 //   tmp = ((ap_uint<32>) bufferOut[i*4 + 3]); 
 //   tmp |= ((ap_uint<32>) bufferOut[i*4 + 2]) << 8; 
 //   tmp |= ((ap_uint<32>) bufferOut[i*4 + 1]) << 16; 
 //   tmp |= ((ap_uint<32>) bufferOut[i*4 + 0]) << 24; 
 // } else {
      tmp = ((ap_uint<32>) bufferOut[i*4 + 0]); 
      tmp |= ((ap_uint<32>) bufferOut[i*4 + 1]) << 8; 
      tmp |= ((ap_uint<32>) bufferOut[i*4 + 2]) << 16; 
      tmp |= ((ap_uint<32>) bufferOut[i*4 + 3]) << 24; 
 //   }

    xmem[XMEM_ANSWER_START + i] = tmp;
  }
  
}


void emptyInBuffer()
{
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    bufferIn[i] = 0x0;
  }
  bufferInPtrWrite = 0x0;
  bufferInPtrRead = 0x0;
}

void emptyOutBuffer()
{
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    bufferOut[i] = 0x0;
  }
  bufferOutPtrWrite = 0x0;
}


uint8_t writeDisplaysToOutBuffer()
{
  //Display1
  writeString("Status Display 1: ");
  bufferOut[bufferOutPtrWrite + 3] = Display1 & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (Display1 >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (Display1 >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (Display1 >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  //Display2
  writeString("Status Display 2: ");
  bufferOut[bufferOutPtrWrite + 3] = Display2 & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (Display2 >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (Display2 >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (Display2 >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  /* Display 3 & 4 is less informative outside EMIF Context
   */ 

  return 12;
}


ap_uint<4> copyAndCheckBurst(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt, ap_uint<1> checkPattern)
{

  ap_uint<8> curHeader = 0;
  ap_uint<8> curFooter = 0;

  ap_int<16> buff_pointer = 0-1;
  
 /* if ( ExpCnt % 2 == 1)
  {
    buff_pointer = 2-1;
    bufferIn[bufferInPtrWrite  + 0] = hangover_0;
    bufferIn[bufferInPtrWrite  + 1] = hangover_1;
  }*/


  for(int i = 0; i<LINES_PER_PAGE; i++)
  {
    ap_uint<32> tmp = 0;
    tmp = xmem[i];

    if ( i == 0 )
    {
      curHeader = tmp & 0xff;
    } else {
      buff_pointer++;
      bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp & 0xff);
    }

    buff_pointer++;
    bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 8) & 0xff;
    buff_pointer++;
    bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 16 ) & 0xff;

    if ( i == LINES_PER_PAGE-1) 
    {
      curFooter = (tmp >> 24) & 0xff;
    } else {
      buff_pointer++;
      bufferIn[bufferInPtrWrite  + buff_pointer] = (tmp >> 24) & 0xff;
    }
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
  
  //now we have a clean transfer
  bufferInPtrWrite += buff_pointer +1;
  
  if (bufferInPtrWrite >= (BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE)) 
  { 
    bufferInPtrWrite = 0;
  }
  
  if (lastLine > (BUFFER_SIZE/4 - LINES_PER_PAGE + 1))
  {
    lastLine = 0;
  }

  if (ExpCnt % 2 == 0)
  {
   /* hangover_0 = bufferIn[bufferInPtrWrite  + (LINES_PER_PAGE-1)*4 + 0];
    hangover_1 = bufferIn[bufferInPtrWrite  + (LINES_PER_PAGE-1)*4 + 1];*/
    lastLine += LINES_PER_PAGE - 1; //now absolute numbers!
    //currentAddedPayload = 
    //contains_hangover = false; //means, only 31 lines!
  } else {
    //contains_hangover = true; //means, full 32 lines!
    lastLine += LINES_PER_PAGE;
  }

  bool lastPage = (curHeader & 0xf0) == 0xf0;

  if(checkPattern == 1)
  {
    ap_uint<8> ctrlByte = (((ap_uint<8>) ExpCnt) << 4) | ExpCnt;
    
    //printf("ctrlByte: %#010x\n",(int) ctrlByte);

    //for simplicity check only lines in between and skip potentiall hangover 
    for(int i = 3; i< PAYLOAD_BYTES_PER_PAGE -3; i++)
    {
      if(bufferIn[bufferInPtrRead  + i] != ctrlByte)
      {//data is corrupt 
        //printf("corrupt at %d with %#010x\n",i, (int) bufferIn[bufferInPtrWrite + i]);
        return 3;
      }
    }
    bufferInPtrRead = bufferInPtrWrite; //only for check pattern case
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
//TODO: ap_ctrl?? (in order not to need reset in the first place)

//===========================================================
// Core-wide variables
  ap_uint<32> SR = 0, ISR = 0, WFV = 0, ASR = 0, CR = 0; // RFO = 0;
  ap_uint<32> Done = 0, EOS = 0, WEMPTY = 0;
  ap_uint<32> WFV_value = 0, CR_value = 0;


  ap_uint<8> ASW1 = 0, ASW2 = 0, ASW3 = 0, ASW4= 0;


//===========================================================
// Connection to HWICAP

  SR = HWICAP[SR_OFFSET];
  //ap_wait_n(AXI_PAUSE_CYCLES);
  ISR = HWICAP[ISR_OFFSET];
  //  ap_wait_n(AXI_PAUSE_CYCLES);
  WFV = HWICAP[WFV_OFFSET];

  ASR = HWICAP[ASR_OFFSET];
  //RFO = HWICAP[RFO_OFFSET];
  CR = HWICAP[CR_OFFSET];

  Done = SR & 0x1;
  EOS  = (SR & 0x4) >> 2;
  WEMPTY = (ISR & 0x4) >> 2;
  WFV_value = WFV & 0x7FF;
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
    httpState = HTTP_IDLE;
    bufferInPtrWrite = 0;
    bufferInPtrRead = 0;
    bufferOutPtrWrite = 0;
    httpAnswerPageLength = 0;
    ongoingTransfer = 0;
    Display1 = 0;
    Display2 = 0;
    Display3 = 0;
    Display4 = 0;
    toDecoup = 0;
    lastLine = 0; 
    writeErrCnt = 0;
    fifoEmptyCnt = 0;
    //currentAddedPayload = BYTES_PER_PAGE-2;
    wordsWrittenToIcapCnt = 0;
  } 

//===========================================================
// Start & Run Burst transfer; Manage Decoupling
  
  ap_uint<1> checkPattern = (*MMIO_in >> CHECK_PATTERN_SHIFT ) & 0b1;
  
  ap_uint<1> parseHTTP = (*MMIO_in >> PARSE_HTTP_SHIFT) & 0b1;
            
  ap_uint<1> notToSwap = (*MMIO_in >> SWAP_N_SHIFT) & 0b1;
  //Turns out: we need to swap => active low

  ap_uint<1> start = (*MMIO_in >> START_SHIFT) & 0b1;

  ap_uint<1> wasAbort = (CR_value & CR_ABORT) >> 4;
  
  if (parseHTTP == 0 && ongoingTransfer == 0)
  { // only in manual mode 
    toDecoup = (*MMIO_in >> DECOUP_CMD_SHIFT) & 0b1; 
  }

  bool handlePayload = false; 
  ap_uint<4> copyRet = 0;

  if (start == 1 && transferErr == 0 && transferSuccess == 0) 
  {

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
        //Activate Decoupling anyway --> not for HTTP etc.
        //toDecoup = 1;

        if (parseHTTP == 1 && httpState == HTTP_IDLE)
        {
          emptyInBuffer();
        }

        copyRet = copyAndCheckBurst(xmem,expCnt, checkPattern);
    
        switch (copyRet) {
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
                  handlePayload = true; //default without HTTP 
                  break;
          case 5: 
                  msg= " OK";
                  cnt = expCnt;
                  handlePayload = true; //default without HTTP 
                  break;
        }

        /*if (contains_hangover) 
        {
          lastLine = LINES_PER_PAGE;
          currentAddedPayload = BYTES_PER_PAGE -2;
        } */


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
    
  if ((checkPattern == 0 && handlePayload) || ongoingTransfer == 1)
  {//means: write to HWICAP

    ap_uint<1> CR_isWritting = CR_value & CR_WRITE;

    if (copyRet == 4 || copyRet == 5 || ongoingTransfer == 1)
    {// valid buffer -> write to HWICAP
  
      //with HTTP the current buffer pointer is not always the start of the payload
      //bufferInPtrRead = bufferInPtrWrite;

      if (parseHTTP == 1 || ongoingTransfer == 1)
      {

          if(wasAbort == 1 || transferErr == 1)
          {
            httpState = HTTP_SEND_RESPONSE;
          }

          printf("httpState bevore parseHTTP: %d\n",httpState);

          parseHttpInput(transferErr,wasAbort); 

          switch (httpState) {
            case HTTP_IDLE: 
              handlePayload = false;
              break; 
            case HTTP_PARSE_HEADER: 
              //bufferInPtrWrite += currentAddedPayload;
              handlePayload = false;
              break; 
            case HTTP_HEADER_PARSED:
                     //if( bufferInPtrWrite + currentAddedPayload == bufferInPtrRead) 
                     //if( bufferInPtrWrite + (lastLine*4) == bufferInPtrRead) 
                     if( (lastLine*4) == bufferInPtrRead) 
                     {//corner case 
                       handlePayload = false;
                       httpState = HTTP_READ_PAYLOAD;
                     }
                     //TODO: adapt lastLine to offset between bufferInPtrRead and bufferInPtrWrite!!
                     //--> start of bitfile must be aligned to 4?? do in sender??
                     //NO break 
            case HTTP_READ_PAYLOAD:
                     //handlePayload=true;
                     //duplicates are prevented by the lastLine!!
                     if (transferSuccess == 1)
                     {
                       httpState = HTTP_REQUEST_COMPLETE;
                     /* printf("lastLine bevore update: %d\n",(int) lastLine);
                      printf("bufferInPtrRead: %d\n", (int) bufferInPtrRead);
                       //lastLine = request_len(bufferInPtrRead,lastLine*4) / 4; //update last word 
                       lastLine = request_len(bufferInPtrRead,BYTES_PER_PAGE) / 4 + bufferInPtrRead/4; //update last word 
                      printf("lastLine after update: %d\n", (int) lastLine);*/
                     }
                     //bufferInPtrWrite += currentAddedPayload;
                     ongoingTransfer = 1;
                     toDecoup = 1;
                     break;
            case HTTP_REQUEST_COMPLETE: 
                      //handlePayload = false;
                     //handlePayload=true;
                     ongoingTransfer = 1;
                     httpState = HTTP_SEND_RESPONSE;
                     //bufferInPtrWrite += currentAddedPayload;
                       break;
            case HTTP_SEND_RESPONSE:
                       handlePayload = false;
                       ongoingTransfer = 1;
                       break;
            case HTTP_INVALID_REQUEST:
                       //transferErr = 1;
                       //set transferSuccess in order not to activate Decoup 
                       //no break 
            case HTTP_DONE:
                       copyOutBuffer(httpAnswerPageLength,xmem,notToSwap);
                       transferSuccess = 1;
                       ongoingTransfer = 0;
                       handlePayload = false;
                       toDecoup = 0;
                       break;
          }

      }


      if (checkPattern == 0 && handlePayload)
      {//means: write to HWICAP 

        toDecoup = 1;

        //for( int i = bufferInPtrRead/4; i<lastLine; i++)
        for( int i = bufferInPtrRead; i<lastLine*4; i += 4)
        {
          ap_uint<32> tmp = 0; 
          if ( notToSwap == 1) 
          {
            tmp |= (ap_uint<32>) bufferIn[0 + i];
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 1]) <<  8);
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 2]) << 16);
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 3]) << 24);
            //tmp |= (ap_uint<32>) bufferIn[bufferInPtrRead  + i*4];
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 1]) <<  8);
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 2]) << 16);
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 3]) << 24);
          } else { 
            //default 
            tmp |= (ap_uint<32>) bufferIn[0 + i + 3];
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 2]) <<  8);
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 1]) << 16);
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 0]) << 24);
            //tmp |= (ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 3];
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 2]) <<  8);
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 1]) << 16);
            //tmp |= (((ap_uint<32>) bufferIn[bufferInPtrRead  + i*4 + 0]) << 24);
          }

          if ( tmp == 0x0d0a0d0a) 
          {
            printf("Error: Tried to write 0d0a0d0a.\n");
            writeErrCnt++;

            continue;
          }

          HWICAP[WF_OFFSET] = tmp;
          wordsWrittenToIcapCnt++;
          printf("writing to HWICAP: %#010x\n",(int) tmp);
        }
            
        printf("lastLine: %d\n", (int) lastLine);
        printf("bufferInPtrRead: %d\n", (int) bufferInPtrRead);

        CR_isWritting = CR_value & CR_WRITE;
        if (CR_isWritting != 1)
        {
          HWICAP[CR_OFFSET] = CR_WRITE;
        }

        //bufferInPtrRead += 4*lastLine;
        bufferInPtrRead = 4*lastLine;

        if(bufferInPtrRead >= (BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE))
        {//should always hit even pages....
          bufferInPtrRead = 0;
        }
        
        WFV = HWICAP[WFV_OFFSET];
        WFV_value = WFV & 0x7FF;
        if (WFV_value == 0x7FF)
        {
          //printf("FIFO is unexpected empty\n");
          fifoEmptyCnt++;
        }

      }
    }

    if (copyRet == 2)
    {//Abort current opperation and exit 
      HWICAP[CR_OFFSET] = CR_ABORT;
      transferErr = 1;
    }

    if (copyRet == 4 && handlePayload)
    {//wait until all is written 
     
    /*  printf("Wait for final write\n");

      while(WFV_value < 0x7FF) 
      {
        ap_wait_n(LOOP_WAIT_CYCLES); 
        
        CR = HWICAP[CR_OFFSET];
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
        WFV_value = WFV & 0x7FF;
      }*/

      if (parseHTTP == 0 && ongoingTransfer == 0)
      {
        toDecoup = 0;
      }
    }

  /*  if ( parseHTTP == 0 && handlePayload)
    {//update read ptr 
      bufferInPtrRead = bufferInPtrWrite;
    }*/

  }
 /* 
  if (parseHTTP == 0 && handlePayload) // else to above
  {
   //bufferInPtrWrite += currentAddedPayload;
  }*/

  //if (iter_count >= MAX_BUF_ITERS) // >= because the next transfer is written to the space of iter_count+1




//===========================================================
// Decoupling 

  if ( toDecoup == 1 || transferErr == 1 )
  {
    *setDecoup = 0b1;
  } else {
    *setDecoup = 0b0;
  }

//===========================================================
//  putting displays together 


  ap_uint<4> Dsel = 0;

  Dsel = (*MMIO_in >> DSEL_SHIFT) & 0xF;

  Display1 = (WEMPTY << WEMPTY_SHIFT) | (Done << DONE_SHIFT) | EOS;
  Display1 |= WFV_value << WFV_V_SHIFT;
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
  Display4 |= ((int) httpState) << HTTP_STATE_SHIFT;
  Display4 |= ((ap_uint<32>) writeErrCnt) << 8;
  Display4 |= ((ap_uint<32>) fifoEmptyCnt) << 16;

  Display5 = (ap_uint<32>) wordsWrittenToIcapCnt;

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
    case 5:
        *MMIO_out = (0x5 << DSEL_SHIFT) | Display5;
          break;
    default: 
        *MMIO_out = 0xBEBAFECA;
          break;
  }

  //ap_wait_n(WAIT_CYCLES);
  return;
}



