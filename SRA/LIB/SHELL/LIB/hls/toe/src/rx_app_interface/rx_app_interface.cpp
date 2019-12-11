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
#define TRACE_LAI  1 <<  1
#define TRACE_MRD  1 <<  2
#define TRACE_NMX  1 <<  3
#define TRACE_RAS  1 <<  4
#define TRACE_RMA  1 <<  5
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
 * @param[out] soRma_MemRdCmd,   Rx memory read command to RxMemAccess (Rma).
 *
 * @detail
 *  This process provides the application (a.k.a Role) with the data streams of
 *   the established connections via the TcpRoleInterface (TRIF).
 *  The process waits for a valid data read request from [TRIF] and generates a
 *   corresponding read command for the Rx buffer memory via [Rma]. Next, a
 *   request to update the RxApp pointer of the session is forwarded to [RSt],
 *   and a meta-data (.i.e the session-id) is sent back to [TRIF] to signal that
 *   the request has been processed.
 *
 *****************************************************************************/
void pRxAppStream(
	stream<AppRdReq>            &siTRIF_DataReq,
	stream<SessionId>           &soTRIF_Meta,
	stream<RAiRxSarQuery>       &soRSt_RxSarQry,
	stream<RAiRxSarReply>       &siRSt_RxSarRep,
	stream<DmCmd>               &soRma_MemRdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Ras");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0=0, S1 } \
								 ras_fsmState=S0;
    #pragma HLS RESET   variable=ras_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpSegLen    ras_readLength;

    switch (ras_fsmState) {
    case S0:
    	if (!siTRIF_DataReq.empty() && !soRSt_RxSarQry.full()) {
    		AppRdReq  appReadRequest = siTRIF_DataReq.read();
			if (appReadRequest.length != 0) {
				// Make sure length is not 0, otherwise Data Mover will hang
				soRSt_RxSarQry.write(RAiRxSarQuery(appReadRequest.sessionID));
				ras_readLength = appReadRequest.length;
				ras_fsmState = S1;
			}
		}
		break;

    case S1:
		if (!siRSt_RxSarRep.empty() && !soTRIF_Meta.full() &&
			!soRma_MemRdCmd.full() && !soRSt_RxSarQry.full()) {
			RAiRxSarReply rxSarRep = siRSt_RxSarRep.read();
			// Signal that the data request has been processed by sending
			// the SessId back to [TRIF]
			soTRIF_Meta.write(rxSarRep.sessionID);
			// Generate Rx memory buffer read command
			RxMemPtr rxMemAddr = 0;
			rxMemAddr(29, 16) = rxSarRep.sessionID(13, 0);
			rxMemAddr(15,  0) = rxSarRep.appd;
			soRma_MemRdCmd.write(DmCmd(rxMemAddr, ras_readLength));
			// Update the APP read pointer
			soRSt_RxSarQry.write(RAiRxSarQuery(rxSarRep.sessionID, rxSarRep.appd+ras_readLength));
			ras_fsmState = S0;
		}
		break;
    }
}

/******************************************************************************
 * @brief Rx Memory Access (Rma)
 *
 * @param[in]  siRas_MemRdCmd,  Rx memory read command from RxAppStream (Ras).
 * @param[out] soMEM_RxP_RdCmd, Rx memory read command to MEM.
 * @param[out] soMrd_SplitSeg,  Signal pRxMemRead (Mrd) that segment is broken in two Rx memory buffers.
 *
 ******************************************************************************/
void pRxMemAccess(
        stream<DmCmd>   &siRas_MemRdCmd,
        stream<DmCmd>   &soMEM_RxP_RdCmd,
        stream<StsBit>  &soMrd_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Rma");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static FlagBit             rma_doubleAccessFlag=FLAG_OFF;
    #pragma HLS reset variable=rma_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static DmCmd               rma_memRdCmd;
    static TcpSegLen           rma_memRdLen;

    if (rma_doubleAccessFlag == FLAG_OFF) {
        if (!siRas_MemRdCmd.empty() && !soMEM_RxP_RdCmd.full()) {
            rma_memRdCmd = siRas_MemRdCmd.read();
            if ((rma_memRdCmd.saddr.range(15, 0) + rma_memRdCmd.bbt) > 65536) {
                rma_memRdLen = 65536 - rma_memRdCmd.saddr;
                soMEM_RxP_RdCmd.write(DmCmd(rma_memRdCmd.saddr, rma_memRdLen));
                rma_doubleAccessFlag = FLAG_ON;
            }
            else {
                soMEM_RxP_RdCmd.write(rma_memRdCmd);
            }
            soMrd_SplitSeg.write(rma_doubleAccessFlag);
        }
    }
    else if (rma_doubleAccessFlag == FLAG_ON) {
        if (!soMEM_RxP_RdCmd.full()) {
            rma_memRdCmd.saddr.range(15, 0) = 0;
            rma_memRdLen = rma_memRdCmd.bbt - rma_memRdLen;
            soMEM_RxP_RdCmd.write(DmCmd(rma_memRdCmd.saddr, rma_memRdLen));
            rma_doubleAccessFlag = FLAG_OFF;
        }
    }
}

/******************************************************************************
 * @brief Memory Reader (Mrd)
 *
 * @param[in]  siMEM_RxP_Data, Rx memory data from [MEM].
 * @param[out] soTRIF_Data,    TCP data stream to [TRIF].
 * @param[in]  siRma_SplitSeg, Status indicating that segment is broken in two Rx memory buffers.
 *******************************************************************************/
void pMemReader(
        stream<AxiWord>  &siMEM_RxP_Data,
        stream<AxiWord>  &soTRIF_Data,
        stream<StsBit>   &siRma_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

	const char *myName = concat3(THIS_NAME, "/", "Mrd");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { MRD_IDLE=0, MRD_STREAM, MRD_JOIN, MRD_STREAMMERGED, \
                            MRD_STREAMUNMERGED, MRD_RESIDUE} \
								mrd_fsmState=MRD_IDLE;
    #pragma HLS RESET  variable=mrd_fsmState
    static FlagBit              mrd_doubleAccessFlag=FLAG_OFF;
    #pragma HLS RESET  variable=mrd_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxiWord     mrd_axiWord;
    static ap_uint<4>  mrd_memRdOffset;
    static ap_uint<8>  mrd_offsetBuffer;

    switch(mrd_fsmState) {

    case MRD_IDLE:
        //-- Default starting state
        if (!siRma_SplitSeg.empty() && !siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            siRma_SplitSeg.read(mrd_doubleAccessFlag);
            siMEM_RxP_Data.read(mrd_axiWord);
            // Count the number of valid bytes in this data word
            mrd_memRdOffset = keepMapping(mrd_axiWord.tkeep);

            if (mrd_axiWord.tlast == 1 && mrd_doubleAccessFlag == FLAG_ON) {
                // This is the last word and this access was broken down in 2.
                // Therefore, clear the last flag in the current axiWord
                mrd_axiWord.tlast = ~mrd_doubleAccessFlag;
                if (mrd_memRdOffset == 8) {
                    // No need to offset anything. We can forward the current word
                    soTRIF_Data.write(mrd_axiWord);
                    // Jump to stream merged since there's no joining to be performed
                    mrd_fsmState = MRD_STREAMUNMERGED;
               
                }
                else if (mrd_memRdOffset < 8) {
                    // The last word of the 1st segment-part is not full.
                    // Don't output anything here and go to MRD_JOIN.
                    // There, we will fetch more data to fill in the current word.
                    mrd_fsmState = MRD_JOIN;
                }
            }
            else if (mrd_axiWord.tlast == 1 && mrd_doubleAccessFlag == FLAG_OFF)  {
                // This is the 1st and last data word of this segment.
                // We are done the 1st segment. Stay in this default idle state.
                soTRIF_Data.write(mrd_axiWord);
            }
            else {
                // The 1st segment-part contains more than one word.
                // Forward the current word and goto MRD_STREAM to process the others.
                soTRIF_Data.write(mrd_axiWord);
                mrd_fsmState = MRD_STREAM;
            }
        }
        break;

    case MRD_STREAM:
        //-- Handles all but the 1st data words of a 1st memory access of a segment
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            siMEM_RxP_Data.read(mrd_axiWord);
            // Count the number of valid bytes in this data word
            mrd_memRdOffset = keepMapping(mrd_axiWord.tkeep);
            if (mrd_axiWord.tlast == 1 && mrd_doubleAccessFlag == FLAG_ON) {
                // This is the last word and this access was broken down in 2.
                // Therefore, clear the last flag in the current axiWord
                mrd_axiWord.tlast = ~mrd_doubleAccessFlag;
                if (mrd_memRdOffset == 8) {
                    // No need to offset anything
                    soTRIF_Data.write(mrd_axiWord);
                    // Jump to stream merged since there's no joining to be performed.
                    mrd_fsmState = MRD_STREAMUNMERGED;
                }
                else if (mrd_memRdOffset < 8) {
                    // The last word of the 1st segment-part is not full.
                    // Don't output anything here and go to MRD_JOIN.
                    // There, we will fetch more data to fill in the current word.
                    mrd_fsmState = MRD_JOIN;
                }
            }
            else if (mrd_axiWord.tlast == 1 && mrd_doubleAccessFlag == FLAG_OFF) {
                // This is the 1st and last data word of this segment and no
                // memory access breakdown occurred.
                soTRIF_Data.write(mrd_axiWord);
                // We are done. Goto default idle state.
                mrd_fsmState = MRD_IDLE;
            }
            else {
                // There is more data to read for this segment
                soTRIF_Data.write(mrd_axiWord);
            }
        }
        break;

    case MRD_STREAMUNMERGED:
        //-- Handle the 2nd memory read access when no realignment is required
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            AxiWord currWord = siMEM_RxP_Data.read();
            soTRIF_Data.write(currWord);
            if (currWord.tlast == 1) {
                // We are done with 2nd segment part. Go back to default idle
                mrd_fsmState = MRD_IDLE;
            }
        }
        break;

    case MRD_JOIN:
        //-- Perform the hand over from the 1st to the 2nd memory access when
        //--  the 1st memory read access has already occurred its content is
        //--  stored in 'mrd_axiWord'.
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            AxiWord currWord = AxiWord(0, 0xFF, 0);
            // In any case, backup the content of the previous data word.
            // Here we don't pay attention to the exact number of bytes in the
            // new data word. In case they don't fill the entire remaining gap,
            // there will be garbage in the output but it doesn't matter since
            // the 'tkeep' field indicates the valid bytes.
            currWord.tdata.range((mrd_memRdOffset.to_uint64() * 8) - 1, 0) = \
            		mrd_axiWord.tdata.range(((mrd_memRdOffset.to_uint64() * 8) - 1), 0);
            // Read the next word into static variable
            siMEM_RxP_Data.read(mrd_axiWord);
            // Fill-in the remaining byte of 'currWord'
            currWord.tdata.range(63, (mrd_memRdOffset * 8)) = \
            		mrd_axiWord.tdata.range(((8 - mrd_memRdOffset.to_uint64()) * 8) - 1, 0);
            // Determine how any bytes are valid in the new data word. It might
            // be that this is the only data word of the 2nd segment.
            ap_uint<4> byteCounter = keepMapping(mrd_axiWord.tkeep);
            // Calculate the number of bytes to go into the next & final data word
            mrd_offsetBuffer = byteCounter - (8 - mrd_memRdOffset);

            if (mrd_axiWord.tlast == 1) {
                if ((byteCounter + mrd_memRdOffset) <= 8) {
                    // The residue from the 1st segment plus the bytes of the
                    //  1st word of the 2nd segment fill this data word.
                    // We can set 'tkeep' to the sum of the 2 data word's bytes
                    //  as well as 'tlast' because this is the last word of 2nd segment.
                    currWord.tkeep = returnKeep(byteCounter + mrd_memRdOffset);
                    currWord.tlast = 1;
                    // We are done with 2nd segment part. Go back to default idle
                    mrd_fsmState = MRD_IDLE;
                }
                else {
                    // The total number of bytes from the 1st segment and the
                    //  1st word of the 2nd segment is > 8.
                    // Goto MRD_RESIDUE to output the remaining data words.
                    mrd_fsmState = MRD_RESIDUE;
                }
            }
            else {
                // We have not reached the end of the 2nd segment-part.
                // Goto MRD_STREAMMERGED to output the remaining data words.
                mrd_fsmState = MRD_STREAMMERGED;
            }
            // Always forward the current word
            soTRIF_Data.write(currWord);
        }
        break;

    case MRD_STREAMMERGED:
        //-- Handle all the remaining data of the 2nd memory access.
        //-- Data are realigned cess, which resulted from a data word
        //-- This state outputs all of the remaining, realigned data words of the 2nd mem.access, which resulted from a data word
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            AxiWord currWord = AxiWord(0, 0xFF, 0);
            currWord.tdata.range((mrd_memRdOffset.to_uint64() * 8) - 1, 0) = \
            		mrd_axiWord.tdata.range(63, ((8 - mrd_memRdOffset.to_uint64()) * 8));
            // Read the next word into static variable
            siMEM_RxP_Data.read(mrd_axiWord);
            // Fill-in the remaining byte of 'currWord'
            currWord.tdata.range(63, (mrd_memRdOffset * 8)) = \
            		mrd_axiWord.tdata.range(((8 - mrd_memRdOffset.to_uint64()) * 8) - 1, 0);
            // Determine how any bytes are valid in the new data word. It might
            // be that this is the only data word of the 2nd segment.
            ap_uint<4> byteCounter = keepMapping(mrd_axiWord.tkeep);
            // Calculate the number of bytes to go into the next & final data word
            mrd_offsetBuffer = byteCounter - (8 - mrd_memRdOffset);
            if (mrd_axiWord.tlast == 1) {
                if ((byteCounter + mrd_memRdOffset) <= 8) {
                    // The residue from the 1st memory access and the data in the
                	// 1st data word of the 2nd memory access fill this data word.
                    // We can set 'tkeep' to the sum of the 2 data word's bytes
                    //  as well as 'tlast' because this is the last word of the
                	// 2nd memory access.
                    currWord.tkeep = returnKeep(byteCounter + mrd_memRdOffset);
                    currWord.tlast = 1;
                    // We are done with 2nd memory access. Go back to default idle
                    mrd_fsmState = MRD_IDLE;
                }
                else {
                	// This is not the last word because it doesn't fit in the
                	//  available space of the current word.
                	// Goto the MRD_RESIDUE to handle the remainder of this data word
                	mrd_fsmState = MRD_RESIDUE;
                }
            }
            soTRIF_Data.write(currWord);
        }
        break;

    case MRD_RESIDUE:
    	//--
        if (!soTRIF_Data.full()) {
            AxiWord currWord = AxiWord(0, returnKeep(mrd_offsetBuffer), 1);
            currWord.tdata.range((mrd_memRdOffset.to_uint64() * 8) - 1, 0) = \
            		mrd_axiWord.tdata.range(63, ((8 - mrd_memRdOffset.to_uint64()) * 8));
            soTRIF_Data.write(currWord);
            // We are done with the very last bytes of a segment.
            // Go back to default idle state.
            mrd_fsmState = MRD_IDLE;
        }
        break;
    }
}

/*****************************************************************************
 * @brief Listen application interface (Lai)
 *
 * @param[in]  siTRIF_LsnReq,         TCP listen port request from TRIF.
 * @param[out] soTRIF_LsnAck,         TCP listen port acknowledge to TRIF.
 * @param[out] soPRt_LsnReq,          Listen port request to PortTable (PRt).
 * @param[in]  siPRt_LsnAck,          Listen port acknowledge from [PRt].
 *
 * @details
 *  This process is used to open/close passive connections.
 *
 * [TODO - this does not seem to be very necessary]
 *
 ******************************************************************************/
void pLsnAppInterface(
        stream<TcpPort>     &siTRIF_LsnReq,
        stream<AckBit>      &soTRIF_LsnAck,
        stream<TcpPort>     &soPRt_LsnReq,
        stream<AckBit>      &siPRt_LsnAck)
        // [TODO] The tear down of a connection is not implemented yet
        //stream<TcpPort>   &siTRIF_StopLsnReq,
        //stream<TcpPort>   &soPRt_CloseReq,)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Lai");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                lai_waitForAck=false;
    #pragma HLS reset variable=lai_waitForAck

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TcpPort            listenPort;
    AckBit             listenAck;

    if (!siTRIF_LsnReq.empty() && !lai_waitForAck) {
        siTRIF_LsnReq.read(listenPort);
        soPRt_LsnReq.write(listenPort);
        lai_waitForAck = true;
    }
    else if (!siPRt_LsnAck.empty() && lai_waitForAck) {
        siPRt_LsnAck.read(listenAck);
        soTRIF_LsnAck.write(listenAck);
        lai_waitForAck = false;
    }

    /*** Close listening port [TODO] ***
    if (!siTRIF_StopLsnReq.empty()) {
        soPRt_CloseReq.write(siTRIF_StopLsnReq.read());
    }
    ***/
}

/*****************************************************************************
 * @brief Rx Application Interface (RAi).
 *
 * @param[out] soTRIF_Notif,    Notification to TcpRoleInterface (TRIF) about available new data in TCP Rx buffer.
 * @param[in]  siTRIF_DataReq,  Request for data from [TRIF].

 * @param[out] soTRIF_Data,     TCP data stream to TRIF.
 * @param[out] soTRIF_Meta,     Metadata to TRIF.
 * @param[in]  siTRIF_LsnReq,   TCP listen port request from [TRIF].
 * @param[out] soTRIF_LsnAck,   TCP listen port acknowledge to [TRIF].
 * @param[out] soPRt_LsnReq,    TCP listen port acknowledge from [PRt].
 * @param[in]  siPRt_LsnAck,    Port state reply from [PRt].
 * @param[in]  siRXe_Notif,     Notification from [RXe].
 * @param
 * @param
 * @param
 * @param[out] soMEM_RxP_RdCmd,       Rx memory read command to MEM.
 * @param[in]  siMEM_RxP_Data,        Rx memory data from MEM.
 *
 * @details
 *
 ******************************************************************************/
void rx_app_interface(
        //-- TRIF / Handshake Interfaces
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
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Rx Application Stream (Ras) ------------------------------------------
	static stream<DmCmd>        ssRasToRma_MemRdCmd ("ssRasToRma_MemRdCmd");
    #pragma HLS stream variable=ssRasToRma_MemRdCmd depth=16

    //-- Rx Memory Access (Rma) -----------------------------------------------
    static stream<StsBit>       ssRmaToMrd_SplitSeg ("ssRmaToMrd_SplitSeg");
    #pragma HLS stream variable=ssRmaToMrd_SplitSeg depth=16

    pRxAppStream(
            siTRIF_DataReq,
            soTRIF_Meta,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            ssRasToRma_MemRdCmd);

    pRxMemAccess(
            ssRasToRma_MemRdCmd,
            soMEM_RxP_RdCmd,
            ssRmaToMrd_SplitSeg);

    pMemReader(
            siMEM_RxP_Data,
            soTRIF_Data,
            ssRmaToMrd_SplitSeg);

    pLsnAppInterface(
            siTRIF_LsnReq,
            soTRIF_LsnAck,
            soPRt_LsnReq,
            siPRt_LsnAck);

    // Notification Mux (Nmx) based on template stream Mux
    //  Notice order --> RXe has higher priority that TIm
    pStreamMux(
            siRXe_Notif,
            siTIm_Notif,
            soTRIF_Notif);

}
