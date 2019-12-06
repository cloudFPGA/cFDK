/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.


All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : rx_app_interface.cpp
 * @brief      : Rx Application Interface (RAi)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "rx_app_interface.hpp"
#include "../../test/test_toe_utils.hpp"

using namespace hls;


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (RAA_TRACE | RAD_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/RAi"

#define TRACE_OFF   0x0000
#define TRACE_RAA  1 <<  1
#define TRACE_RAD  1 <<  2
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

/*****************************************************************************
 * @brief A 2-to-1 Stream multiplexer.
 * ***************************************************************************/
  template<typename T> void pStreamMux(
          stream<T>  &si1,
          stream<T>  &si2,
          stream<T>  &so)
{

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*****************************************************************************
 * @brief The Rx Application Stream (RAs) retrieves the data of an
 *         established connection from the buffer memory and forwards it to
 *         the application.
 *
 * @param[in]  siTRIF_DataReq,   Data request from TcpRoleInterface (TRIF).
 * @param[out] soTRIF_Meta,      Metadata to TRIF.
 * @param[out] soRSt_RxSarQry,   Query to RxSarTable (RSt)
 * @param[in]  siRSt_RxSarRep,   Reply from [RSt].
 * @param[out] soMEM_RxP_RdCmd,  Rx memory read command to MEM.
 *
 * @detail
 *  This application interface is used to receive data streams of established
 *  connections. The Application polls data from the buffer by sending a
 *  readRequest. The module checks if the readRequest is valid then it sends a
 *  read request to the memory. After processing the request the MetaData
 *  containing the Session-ID is also written back.
 *
 *****************************************************************************/
void pRxAppStream(
	  stream<AppRdReq>            &siTRIF_DataReq,
	  stream<SessionId>           &soTRIF_Meta,
	  stream<RAiRxSarQuery>       &soRSt_RxSarQry,
	  stream<RAiRxSarReply>       &siRSt_RxSarRep,
	  stream<DmCmd>               &soMEM_RxP_RdCmd)
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
  #pragma HLS PIPELINE II=1
  #pragma HLS INLINE off

  static ap_uint<16>    rasi_readLength;
  static uint16_t       rxAppBreakTemp  = 0;
  static uint16_t       rxRdCounter = 0; // For Debug

  static enum FsmStates { S0=0, S1 } fsmState=S0;

  switch (fsmState) {
	  case S0:
		  if (!siTRIF_DataReq.empty() && !soRSt_RxSarQry.full()) {
			  AppRdReq  appReadRequest = siTRIF_DataReq.read();
			  if (appReadRequest.length != 0) {
				  // Make sure length is not 0, otherwise Data Mover will hang up
				  soRSt_RxSarQry.write(RAiRxSarQuery(appReadRequest.sessionID));
				  // Get app pointer
				  rasi_readLength = appReadRequest.length;
				  fsmState = S1;
			  }
		  }
		  break;

	  case S1:
		  if (!siRSt_RxSarRep.empty() && !soTRIF_Meta.full() &&
			  !soMEM_RxP_RdCmd.full() && !soRSt_RxSarQry.full()) {
			  RAiRxSarReply rxSar = siRSt_RxSarRep.read();
			  RxMemPtr      rxMemAddr = 0;
			  rxMemAddr(29, 16) = rxSar.sessionID(13, 0);
			  rxMemAddr(15,  0) = rxSar.appd;
			  soTRIF_Meta.write(rxSar.sessionID);
			  rxRdCounter++;
			  DmCmd rxAppTempCmd = DmCmd(rxMemAddr, rasi_readLength);
			  soMEM_RxP_RdCmd.write(rxAppTempCmd);
			  // Update the APP read pointer
			  soRSt_RxSarQry.write(RAiRxSarQuery(rxSar.sessionID, rxSar.appd+rasi_readLength));
			  fsmState = S0;
		  }
		  break;
  }
}

/******************************************************************************
 * @brief [TODO]
 *
 * @param[in]
 * @param[out] soMEM_RxP_RdCmd,       Rx memory read command to MEM.
 * @param[in]
 *
 ******************************************************************************/
void pRxMemAccess(
        stream<DmCmd>        &inputMemAccess,
        stream<DmCmd>        &soMEM_RxP_RdCmd,
        stream<ap_uint<1> >  &rxAppDoubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                rxAppBreakdown = false;
    #pragma HLS reset variable=rxAppBreakdown

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static DmCmd rxAppTempCmd;
    static ap_uint<16> rxAppAccLength = 0;

    if (rxAppBreakdown == false) {
        if (!inputMemAccess.empty() && !soMEM_RxP_RdCmd.full()) {
            rxAppTempCmd = inputMemAccess.read();
            if ((rxAppTempCmd.saddr.range(15, 0) + rxAppTempCmd.bbt) > 65536) {
                rxAppAccLength = 65536 - rxAppTempCmd.saddr;
                soMEM_RxP_RdCmd.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
                rxAppBreakdown = true;
            }
            else {
                soMEM_RxP_RdCmd.write(rxAppTempCmd);
            }
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt << std::endl;
            rxAppDoubleAccess.write(rxAppBreakdown);
        }
    }
    else if (rxAppBreakdown == true) {
        if (!soMEM_RxP_RdCmd.full()) {
            rxAppTempCmd.saddr.range(15, 0) = 0;
            rxAppAccLength = rxAppTempCmd.bbt - rxAppAccLength;
            soMEM_RxP_RdCmd.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
            rxAppBreakdown = false;
        }
    }
}

/******************************************************************************
 * @brief [TODO]
 *
 * @param[in]  siMEM_RxP_Data,  Rx memory data from MEM.
 * @param[out] soTRIF_Data,     TCP data stream to TRIF.
 *
 ******************************************************************************/
void pRxMemRead(
        stream<AxiWord>     &siMEM_RxP_Data,
        stream<AxiWord>     &soTRIF_Data,
        stream<ap_uint<1> > &rxAppDoubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    static AxiWord rxAppMemRdRxWord = AxiWord(0, 0, 0);
    static ap_uint<1> rxAppDoubleAccessFlag = 0;
    static enum rAstate {RXAPP_IDLE = 0, RXAPP_STREAM, RXAPP_JOIN, RXAPP_STREAMMERGED, RXAPP_STREAMUNMERGED, RXAPP_RESIDUE} rxAppState;
    static ap_uint<4> rxAppMemRdOffset = 0;
    static ap_uint<8> rxAppOffsetBuffer = 0;

    switch(rxAppState) {
    case RXAPP_IDLE:
        if (!rxAppDoubleAccess.empty() && !siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            //rxAppMemRdOffset = 0;
            rxAppDoubleAccessFlag = rxAppDoubleAccess.read();
            siMEM_RxP_Data.read(rxAppMemRdRxWord);
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    soTRIF_Data.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0)  { // If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                soTRIF_Data.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
            else {                                                              // Finally if there are more words in this memory access,
                rxAppState = RXAPP_STREAM;                                      // then go to RXAPP_STREAM to read them
                soTRIF_Data.write(rxAppMemRdRxWord);                              // and output the current word
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }

        }
        break;
    case RXAPP_STREAM:                                                          // This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // Verify that there's data in the input and space in the output
            siMEM_RxP_Data.read(rxAppMemRdRxWord);                            // Read the data word in
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    soTRIF_Data.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0) {// If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                soTRIF_Data.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                rxAppState = RXAPP_IDLE;                                        // and go back to the idle state
            }
            else {                                                              // If the segment data hasn't finished yet
                soTRIF_Data.write(rxAppMemRdRxWord);                              // output them and stay in this state
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
        }
        break;
    case RXAPP_STREAMUNMERGED:                                                  // This state handles 2nd mem.access data when no realignment is required
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = siMEM_RxP_Data.read();                                 // If so read the data in a tempVariable
            if (temp.tlast == 1)                                                     // If this is the last data word...
                rxAppState = RXAPP_IDLE;                                        // Go back to the output state. Everything else is perfectly fine as is
            soTRIF_Data.write(temp);                                              // Finally, output the data word before changing states
            std::cerr << "Mem.Data: " << std::hex << temp.tdata << " - " << temp.tkeep << " - " << temp.tlast << std::endl;
        }
        break;
    case RXAPP_JOIN:                                                            // This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(((rxAppMemRdOffset.to_uint64() * 8) - 1), 0);    // In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
            rxAppMemRdRxWord = siMEM_RxP_Data.read();
            temp.tdata.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.tdata.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);                 // Buffer & realign temp into rxAppmemRdRxWord (which is a static variable)
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.tkeep);                    // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.tlast == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                        // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.tkeep = returnKeep(tempCounter + rxAppMemRdOffset);     // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.tlast = 1;                                  // also set the LAST to 1, since this is going to be the final word of this segment
                    rxAppState = RXAPP_IDLE;                                    // And go back to idle when finished with this state
                }
                else
                    rxAppState = RXAPP_RESIDUE;                                     // then go to the RXAPP_RESIDUE to output the remaining data words
            }
            else
                rxAppState = RXAPP_STREAMMERGED;                                    // then go to the RXAPP_STREAMMERGED to output the remaining data words
            soTRIF_Data.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_STREAMMERGED:                                                    // This state outputs all of the remaining, realigned data words of the 2nd mem.access, which resulted from a data word
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // Verify that there's data at the input and that the output is ready to receive data
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxAppMemRdRxWord = siMEM_RxP_Data.read();                             // Read the new data word in
            temp.tdata.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.tdata.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.tkeep);            // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.tlast == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                            // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.tkeep = returnKeep(tempCounter + rxAppMemRdOffset);             // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.tlast = 1;                                                  // also set the LAST to 1, since this is going to be the final word of this segment
                    rxAppState = RXAPP_IDLE;                                        // And go back to idle when finished with this state
                }
                else                                                                // If this not the last word, because it doesn't fit in the available space in this data word
                    rxAppState = RXAPP_RESIDUE;                                         // then go to the RXAPP_RESIDUE to output the remainder of this data word
            }
            soTRIF_Data.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_RESIDUE:
        if (!soTRIF_Data.full()) {
            AxiWord temp = AxiWord(0, returnKeep(rxAppOffsetBuffer), 1);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            soTRIF_Data.write(temp);                                              // And finally write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
            rxAppState = RXAPP_IDLE;                                            // And go back to the idle stage
        }
        break;
    }
}

/*****************************************************************************
 * @brief This application interface is used to open passive connections.
 *          [TODO -  Consider renaming]
 *
 * @param[in]  siTRIF_LsnReq,         TCP listen port request from TRIF.
 * @param[out] soTRIF_LsnAck,         TCP listen port acknowledge to TRIF.
 * @param[out] soPRt_LsnReq,          Listen port request to PortTable (PRt).
 * @param[in]  siPRt_LsnAck,          Listen port acknowledge from [PRt].
 *
 ******************************************************************************/
// TODO this does not seem to be very necessary
void pRxLsnInterface(
        stream<TcpPort>                    &siTRIF_LsnReq,
        stream<AckBit>                     &soTRIF_LsnAck,
        stream<TcpPort>                    &soPRt_LsnReq,
        stream<AckBit>                     &siPRt_LsnAck)
        // This is disabled for the time being, because it adds complexity/potential issues
        //stream<ap_uint<16> >             &appStopListeningIn,
        //stream<ap_uint<16> >             &rxAppPortTableCloseIn,)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                rai_wait = false;
    #pragma HLS reset variable=rai_wait

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TcpPort            listenPort;
    AckBit             listening;

    // TODO maybe do a state machine

    // Listening Port Open, why not asynchron??
    if (!siTRIF_LsnReq.empty() && !rai_wait) {
        soPRt_LsnReq.write(siTRIF_LsnReq.read());
        rai_wait = true;
    }
    else if (!siPRt_LsnAck.empty() && rai_wait) {
        siPRt_LsnAck.read(listening);
        soTRIF_LsnAck.write(listening);
        rai_wait = false;
    }

    // Listening Port Close
    /*if (!appStopListeningIn.empty())
    {
        rxAppPortTableCloseIn.write(appStopListeningIn.read());
    }*/
}


/*****************************************************************************
 * @brief Rx Application Interface (RAi).
 *
 * @param[out] soTRIF_Notif,          Tells the APP that data are available in the TCP Rx buffer.
 * @param[in]  siTRIF_DataReq,        Data request from TcpRoleInterface (TRIF).
 * @param[out] soTRIF_Data,           TCP data stream to TRIF.
 * @param[out] soTRIF_Meta,           Metadata to TRIF.
 * @param[in]  siTRIF_LsnReq,         TCP listen port request from [TRIF].
 * @param[out] soTRIF_LsnAck,         TCP listen port acknowledge to [TRIF].
 * @param[out] soPRt_LsnReq,          TCP listen port acknowledge from [PRt].
 * @param[in]  siPRt_LsnAck,          Port state reply from [PRt].
 * @param[in]  siRXe_Notif,           Notification from [RXe].
 * @param
 * @param
 * @param
 * @param[out] soMEM_RxP_RdCmd,       Rx memory read command to MEM.
 * @param[in]  siMEM_RxP_Data,        Rx memory data from MEM.
 *
 ******************************************************************************/
void rx_app_interface(
        //-- TRIF / Notification Interfaces
        stream<AppNotif>            &soTRIF_Notif,
        stream<AppRdReq>            &siTRIF_DataReq,
        //-- TRIF / Data Stream Interfaces
        stream<AxiWord>             &soTRIF_Data,
        stream<SessionId>           &soTRIF_Meta,
        //-- TRIF / Listen Interfaces
        stream<AppLsnReq>           &siTRIF_LsnReq,
        stream<AppLsnAck>           &soTRIF_LsnAck,
        //-- PRt / Port Table Interfaces
        stream<TcpPort>             &soPRt_LsnReq,
        stream<AckBit>              &siPRt_LsnAck,
        //-- RXe / Rx Engine Notification Interface
        stream<AppNotif>            &siRXe_Notif,
        //-- TIm / Timers Notification Interface
        stream<AppNotif>            &siTIm_Notif,
        //-- Rx SAR Table Interface
        stream<RAiRxSarQuery>       &soRSt_RxSarReq,
        stream<RAiRxSarReply>       &siRSt_RxSarRep,
        //-- MEM / DDR4 Memory Interface
        stream<DmCmd>               &soMEM_RxP_RdCmd,
        stream<AxiWord>             &siMEM_RxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	//OBSOLETE-2019111 #pragma HLS INLINE
    #pragma HLS INLINE // off

    static stream<DmCmd>        rxAppStreamIf2memAccessBreakdown ("rxAppStreamIf2memAccessBreakdown");
    #pragma HLS stream variable=rxAppStreamIf2memAccessBreakdown depth=16

    static stream<ap_uint<1> >  rxAppDoubleAccess                ("rxAppDoubleAccess");
    #pragma HLS stream variable=rxAppDoubleAccess                depth=16

    // RX Application Stream
    //OBSOLETE_20191206 rx_app_stream_if(
    pRxAppStream(
            siTRIF_DataReq,
            soTRIF_Meta,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            rxAppStreamIf2memAccessBreakdown);

    pRxMemAccess(
            rxAppStreamIf2memAccessBreakdown,
            soMEM_RxP_RdCmd,
            rxAppDoubleAccess);

    pRxMemRead(
            siMEM_RxP_Data,
            soTRIF_Data,
            rxAppDoubleAccess);

    pRxLsnInterface(
            siTRIF_LsnReq,
            soTRIF_LsnAck,
            soPRt_LsnReq,
            siPRt_LsnAck);

    // Multiplex the notifications
    pStreamMux(
            siRXe_Notif,
            siTIm_Notif,
            soTRIF_Notif);

}
