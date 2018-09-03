
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
ap_uint<1> toDecoup = 0;
ap_uint<8> writeErrCnt = 0;
ap_uint<8> fifoEmptyCnt = 0;

ap_uint<16> bufferInPtrWrite = 0x0;
ap_uint<16> bufferInPtrMaxWrite = 0x0;
ap_uint<16> bufferInPtrNextRead = 0x0;

ap_uint<8> bufferOut[BUFFER_SIZE];
ap_uint<16> bufferOutPtrWrite = 0x0;


ap_uint<4> httpAnswerPageLength = 0;
HttpState httpState = HTTP_IDLE; 
ap_uint<1> ongoingTransfer = 0;

ap_uint<32> Display1 = 0, Display2 = 0, Display3 = 0, Display4 = 0, Display5 = 0, Display6 = 0; 
ap_uint<24> wordsWrittenToIcapCnt = 0;

ap_uint<32> nodeRank = 0;
ap_uint<32> clusterSize = 0; 


ap_uint<32> mpe_status[MPE_NUMBER_STATUS_WORDS];



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
  bufferInPtrNextRead = 0x0;
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
  uint8_t len  = 0;
  //Display1
  len = writeString("Status Display 1: ");
  bufferOut[bufferOutPtrWrite + 3] = Display1 & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (Display1 >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (Display1 >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (Display1 >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  len += 6; 

  //Display2
  len += writeString("Status Display 2: ");
  bufferOut[bufferOutPtrWrite + 3] = Display2 & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (Display2 >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (Display2 >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (Display2 >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  len += 6; 
  /* Display 3 & 4 is less informative outside EMIF Context
   */ 
  
  //insert rank and size 
  len += writeString("Rank: ");
  bufferOut[bufferOutPtrWrite + 3] = nodeRank & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (nodeRank >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (nodeRank >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (nodeRank >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  
  len += writeString("Size: ");
  bufferOut[bufferOutPtrWrite + 3] = clusterSize & 0xFF; 
  bufferOut[bufferOutPtrWrite + 2] = (clusterSize >> 8) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 1] = (clusterSize >> 16) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 0] = (clusterSize >> 24) & 0xFF; 
  bufferOut[bufferOutPtrWrite + 4] = '\r'; 
  bufferOut[bufferOutPtrWrite + 5] = '\n'; 
  bufferOutPtrWrite  += 6;
  len += 6; 

  //MPE status 
  len += writeString("MPE Status (10 lines): \r\n"); //MPE_NUMBER_STATUS_WORDS

  for(int i = 0; i<MPE_NUMBER_STATUS_WORDS; i++)
  {
    bufferOut[bufferOutPtrWrite + 3] = mpe_status[i] & 0xFF; 
    bufferOut[bufferOutPtrWrite + 2] = (mpe_status[i] >> 8) & 0xFF; 
    bufferOut[bufferOutPtrWrite + 1] = (mpe_status[i] >> 16) & 0xFF; 
    bufferOut[bufferOutPtrWrite + 0] = (mpe_status[i] >> 24) & 0xFF; 
    bufferOut[bufferOutPtrWrite + 4] = '\r'; 
    bufferOut[bufferOutPtrWrite + 5] = '\n'; 
    bufferOutPtrWrite  += 6;
    len += 6; 
  }

  return len;
}

void setRank(ap_uint<32> newRank)
{
  nodeRank = newRank; 
  //nothing else to do so far 
}

void setSize(ap_uint<32> newSize)
{
  clusterSize = newSize;
  //nothing else to do so far 
}

ap_uint<4> copyAndCheckBurst(ap_uint<32> xmem[XMEM_SIZE], ap_uint<4> ExpCnt, ap_uint<1> checkPattern)
{

  ap_uint<8> curHeader = 0;
  ap_uint<8> curFooter = 0;

  ap_int<16> buff_pointer = 0-1;
  

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
  
  //printf("bufferInPtrWrite: %d\n", (int) bufferInPtrWrite);
  //printf("lastLine: %d\n", (int) lastLine);
  //now we have a clean transfer
  bufferInPtrWrite += buff_pointer;// +1;
  bufferInPtrMaxWrite = bufferInPtrWrite;
  bufferInPtrWrite++;

  if (bufferInPtrWrite >= (BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE)) 
  { 
    bufferInPtrWrite = 0;
  }
  
  //printf("After UPDATE bufferInPtrWrite: %d\n", (int) bufferInPtrWrite);
  //printf("After UPDATE lastLine: %d\n", (int) lastLine);

  bool lastPage = (curHeader & 0xf0) == 0xf0;

  if(checkPattern == 1)
  {
    ap_uint<8> ctrlByte = (((ap_uint<8>) ExpCnt) << 4) | ExpCnt;
    
    //printf("ctrlByte: %#010x\n",(int) ctrlByte);

    //for simplicity check only lines in between and skip potentiall hangover 
    for(int i = 3; i< PAYLOAD_BYTES_PER_PAGE -3; i++)
    {
      if(bufferIn[bufferInPtrNextRead  + i] != ctrlByte)
      {//data is corrupt 
        //printf("corrupt at %d with %#010x\n",i, (int) bufferIn[bufferInPtrWrite + i]);
        return 3;
      }
    }
    bufferInPtrNextRead = bufferInPtrWrite; //only for check pattern case
  }

  if (lastPage)
  {
    return 4;
  }

  return 5;
}


void smc_main(
    // ----- system reset ---
    ap_uint<1> sys_reset,
    //EMIF Registers
    ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
    //HWICAP and DECOUPLING
    ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup,
    //XMEM
    ap_uint<32> xmem[XMEM_SIZE], 
    //MPE 
    //ap_uint<32> mpeCtrl[MPE_NUMBER_CONFIG_WORDS + MPE_NUMBER_STATUS_WORDS + MAX_CLUSTER_SIZE],
    ap_uint<32> mpeCtrl[XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH],
    //TO ROLE 
    ap_uint<32> *role_rank, ap_uint<32> *cluster_size)
{
#pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset
#pragma HLS RESOURCE variable=bufferIn core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=bufferOut core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=xmem core=RAM_1P_BRAM
#pragma HLS INTERFACE m_axi depth=512 port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_stable register port=decoupStatus name=piDECOUP_SMC_status
#pragma HLS INTERFACE ap_ovld register port=setDecoup name=poSMC_DECOUP_activate
#pragma HLS INTERFACE ap_ovld register port=role_rank name=poSMC_to_ROLE_rank
#pragma HLS INTERFACE ap_ovld register port=cluster_size name=poSMC_to_ROLE_size
#pragma HLS INTERFACE m_axi depth=16383 port=mpeCtrl bundle=poSMC_MPE_ctrlLink_AXI  //0x3fff - 0x2000
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
  ISR = HWICAP[ISR_OFFSET];
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

  if (RST == 1 || sys_reset == 1)
  {
    cnt = 0xf;
    transferErr = 0;
    transferSuccess = 0;
    msg = "IDL";
    httpState = HTTP_IDLE;
    bufferInPtrWrite = 0;
    bufferInPtrMaxWrite = 0;
    bufferInPtrNextRead = 0;
    bufferOutPtrWrite = 0;
    httpAnswerPageLength = 0;
    ongoingTransfer = 0;
    Display1 = 0;
    Display2 = 0;
    Display3 = 0;
    Display4 = 0;
    toDecoup = 0;
    writeErrCnt = 0;
    fifoEmptyCnt = 0;
    wordsWrittenToIcapCnt = 0;
  } 
  
  if (sys_reset == 1)
  {
    nodeRank = 0;
    clusterSize = 0;
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
      //bufferInPtrNextRead = bufferInPtrWrite;

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
              handlePayload = false;
              break; 
            case HTTP_HEADER_PARSED:

                     if( bufferInPtrMaxWrite == bufferInPtrNextRead) 
                     {//corner case 
                       handlePayload = false;
                       httpState = HTTP_READ_PAYLOAD;
                     }
                     //TODO: start of bitfile must be aligned to 4?? do in sender??
                     //NO break 
            case HTTP_READ_PAYLOAD:
                     //handlePayload=true;
                     if (transferSuccess == 1)
                     {
                       httpState = HTTP_REQUEST_COMPLETE;
                     }
                     ongoingTransfer = 1;
                     toDecoup = 1;
                     break;
            case HTTP_REQUEST_COMPLETE: 
                     ongoingTransfer = 1;
                     httpState = HTTP_SEND_RESPONSE;
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
                       //TODO emptyInBuffer??
                       break;
          }

      }


      if (checkPattern == 0 && handlePayload)
      {//means: write to HWICAP 

        toDecoup = 1;

        int i = 0;
        for(i = bufferInPtrNextRead; i<=bufferInPtrMaxWrite -3 ; i += 4)
        {
          ap_uint<32> tmp = 0; 
          if ( notToSwap == 1) 
          {
            tmp |= (ap_uint<32>) bufferIn[0 + i];
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 1]) <<  8);
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 2]) << 16);
            tmp |= (((ap_uint<32>) bufferIn[0 + i + 3]) << 24);
          } else { 
            //default 
            tmp |= (ap_uint<32>) bufferIn[0 + i + 3];
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 2]) <<  8);
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 1]) << 16);
            tmp |= (((ap_uint<32>) bufferIn[0  + i + 0]) << 24);
          }

          if ( tmp == 0x0d0a0d0a) 
          {
            printf("Error: Tried to write 0d0a0d0a.\n");
            writeErrCnt++;

            continue;
          }

          HWICAP[WF_OFFSET] = tmp;
          wordsWrittenToIcapCnt++;
          //printf("writing to HWICAP: %#010x\n",(int) tmp);
        }
            
       // printf("bufferInPtrMaxWrite: %d\n", (int) bufferInPtrMaxWrite);
       // printf("bufferInPtrNextRead: %d\n", (int) bufferInPtrNextRead);
       // printf("wordsWrittenToIcapCnt: %d\n", (int) wordsWrittenToIcapCnt);

        CR_isWritting = CR_value & CR_WRITE;
        if (CR_isWritting != 1)
        {
          HWICAP[CR_OFFSET] = CR_WRITE;
        }

          bufferInPtrNextRead = i; //NOT i + 4, because that is already done by the for loop!

        if(bufferInPtrNextRead >= (BUFFER_SIZE - PAYLOAD_BYTES_PER_PAGE))
        {//should always hit even pages....
          if (parseHTTP == 1)
          {
            ap_uint<4> telomere = bufferInPtrMaxWrite - bufferInPtrNextRead + 1; //+1 because MaxWrite is already filled (not next write)
            printf("telomere: %d\n", (int) telomere);
            
            if (telomere != 0)
            {
              for(int j = 0; j<telomere; j++)
              {
                bufferIn[j] = bufferIn[bufferInPtrNextRead + j];
              }
              bufferInPtrWrite = telomere;
            }
            if (telomere < 0)
            {
              printf("ERROR negativ telomere!\n");
              writeErrCnt++;
            }
          }
          
          bufferInPtrNextRead = 0;
        }
        
        //printf("after UPDATE bufferInPtrNextRead: %d\n", (int) bufferInPtrNextRead);
        
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
    {//last write was done: disable decoupling, if necessary
     
      if (parseHTTP == 0 && ongoingTransfer == 0)
      {
        toDecoup = 0;
      }
    }

  }



//===========================================================
// Decoupling 

  if ( toDecoup == 1 || transferErr == 1 )
  {
    *setDecoup = 0b1;
  } else {
    *setDecoup = 0b0;
  }

//===========================================================
// Set RANK and SIZE 

  *role_rank = nodeRank; 
  *cluster_size = clusterSize; 

//===========================================================
// connection to MPE 

  //start and set auto restart (TODO: can't harm? ) 
  
  if(mpeCtrl[MPE_AXI_CTRL_REGISTER] != 0x81)
  { 
    mpeCtrl[MPE_AXI_CTRL_REGISTER] = 0x81; //ap_start and auto_restart
  } 

  //copy status 
  for(int i = 0; i<MPE_NUMBER_STATUS_WORDS; i++)
  {
    mpe_status[i] = mpeCtrl[XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE + MPE_NUMBER_CONFIG_WORDS + i];
  }
  
  //for debuging the connection 
  mpeCtrl[XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] = 168496129; //10.11.12.1
  mpeCtrl[XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] = 168496141; //10.11.12.13
  mpeCtrl[XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] = 168496142; //10.11.12.14


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

  Display6  = nodeRank << RANK_SHIFT; 
  Display6 |= clusterSize << SIZE_SHIFT; 

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
    case 6:
        *MMIO_out = (0x6 << DSEL_SHIFT) | Display6;
          break;
    default: 
        *MMIO_out = 0xBEBAFECA;
          break;
  }

  return;
}



