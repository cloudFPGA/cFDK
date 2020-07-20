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

/*******************************************************************************
 * @file       : rx_app_interface.cpp
 * @brief      : Rx Application Interface (RAi) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "rx_app_interface.hpp"


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

/*******************************************************************************
 * @brief A 2-to-1 generic Stream Multiplexer
 *
 *  @param[in]  si1     The input stream #1.
 *  @param[in]  si2     The input stream #2.
 *  @param[out] so      The output stream.
 *
 * @details
 *  This multiplexer behaves like an arbiter. It takes two streams as inputs and
 *   forwards one of them to the output channel. The stream connected to the
 *   first input always takes precedence over the second one.
 *******************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so)
{
   //-- DIRECTIVES FOR THIS PROCESS --------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}

/*******************************************************************************
 * @brief Rx Application Stream (RAs)
 *
 * @param[in]  siTAIF_DataReq  Data request from TcpApplicationInterface (TAIF).
 * @param[out] soTAIF_Meta     Metadata to [TAIF].
 * @param[out] soRSt_RxSarQry  Query to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep  Reply from [RSt].
 * @param[out] soRma_MemRdCmd  Rx memory read command to RxMemoryAccess (Rma).
 *
 * @detail
 *  This process waits for a valid data read request from the TcpAppInterface
 *   (TAIF) and generates a corresponding read command for the TCP Rx buffer
 *   memory via the RxMemoryAccess (Rma) process. Next, a request to update the
 *   RxApp pointer of the session is forwarded to the RxSarTable (RSt) and a
 *   meta-data (.i.e the current session-id) is sent back to [TAIF] to signal
 *   that the request has been processed.
 *******************************************************************************/
void pRxAppStream(
    stream<TcpAppRdReq>         &siTAIF_DataReq,
    stream<TcpAppMeta>          &soTAIF_Meta,
    stream<RAiRxSarQuery>       &soRSt_RxSarQry,
    stream<RAiRxSarReply>       &siRSt_RxSarRep,
    stream<DmCmd>               &soRma_MemRdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Ras");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { S0=0, S1 } \
                                 ras_fsmState=S0;
    #pragma HLS RESET   variable=ras_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TcpSegLen    ras_readLength;

    switch (ras_fsmState) {
    case S0:
        if (!siTAIF_DataReq.empty() && !soRSt_RxSarQry.full()) {
            TcpAppRdReq  appReadRequest = siTAIF_DataReq.read();
            if (appReadRequest.length != 0) {
                // Make sure length is not 0, otherwise Data Mover will hang
                soRSt_RxSarQry.write(RAiRxSarQuery(appReadRequest.sessionID));
                ras_readLength = appReadRequest.length;
                ras_fsmState = S1;
            }
        }
        break;
    case S1:
        if (!siRSt_RxSarRep.empty() && !soTAIF_Meta.full() &&
                !soRma_MemRdCmd.full() && !soRSt_RxSarQry.full()) {
            RAiRxSarReply rxSarRep = siRSt_RxSarRep.read();
            // Signal that the data request has been processed by sending
            // the SessId back to [TAIF]
            soTAIF_Meta.write(rxSarRep.sessionID);
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

/*******************************************************************************
 * @brief Rx Memory Access (Rma)
 *
 * @param[in]  siRas_MemRdCmd   Rx memory read command from RxAppStream (Ras).
 * @param[out] soMEM_RxP_RdCmd  Rx memory read command to [MEM].
 * @param[out] soMrd_SplitSeg   Split segment to RxMemRead (Mrd).
 *
 * @details
 *  This process takes the memory read command assembled by RxAppStream (Ras)
 *   an forwards it to the memory sub-system (MEM). While doing so, it checks if
 *   the TCP Rx memory buffer wraps around and accordingly generates two memory
 *   read commands out of the initial command received from [Ras].
 *  Because the MemoryReader (Mrd) process needs to be aware of this split, a
 *   signal is sent to [Mrd] telling whether a data segment is broken in two Rx
 *   memory buffers or is provided as a single buffer.
 *******************************************************************************/
void pRxMemAccess(
        stream<DmCmd>   &siRas_MemRdCmd,
        stream<DmCmd>   &soMEM_RxP_RdCmd,
        stream<StsBit>  &soMrd_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Rma");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static FlagBit             rma_doubleAccessFlag=FLAG_OFF;
    #pragma HLS reset variable=rma_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd               rma_memRdCmd;
    static TcpSegLen           rma_memRdLen;

    if (rma_doubleAccessFlag == FLAG_OFF) {
        if (!siRas_MemRdCmd.empty() && !soMEM_RxP_RdCmd.full()) {
            rma_memRdCmd = siRas_MemRdCmd.read();
            if ((rma_memRdCmd.saddr.range(15, 0) + rma_memRdCmd.bbt) > RXMEMBUF) {
                // Break this memory access in two because TCP Rx memory buffer wraps around
                rma_memRdLen = RXMEMBUF - rma_memRdCmd.saddr;
                DmCmd memRdCmd = DmCmd(rma_memRdCmd.saddr, rma_memRdLen);
                soMEM_RxP_RdCmd.write(memRdCmd);
                rma_doubleAccessFlag = FLAG_ON;
                if (DEBUG_LEVEL & TRACE_RMA) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This is the 1st memory access.\n");
                    printDmCmd(myName, memRdCmd);
                }
            }
            else {
                soMEM_RxP_RdCmd.write(rma_memRdCmd);
                if (DEBUG_LEVEL & TRACE_RMA) {
                    printDmCmd(myName, rma_memRdCmd);
                }
            }
            soMrd_SplitSeg.write(rma_doubleAccessFlag);
        }
    }
    else if (rma_doubleAccessFlag == FLAG_ON) {
        if (!soMEM_RxP_RdCmd.full()) {
            rma_memRdCmd.saddr.range(15, 0) = 0;
            rma_memRdLen = rma_memRdCmd.bbt - rma_memRdLen;
            DmCmd memRdCmd = DmCmd(rma_memRdCmd.saddr, rma_memRdLen);
            soMEM_RxP_RdCmd.write(memRdCmd);
            rma_doubleAccessFlag = FLAG_OFF;
            if (DEBUG_LEVEL & TRACE_RMA) {
                printInfo(myName, "TCP Rx memory buffer wraps around: This is the 2nd memory access.\n");
                printDmCmd(myName, memRdCmd);
            }
        }
    }
}

/*******************************************************************************
 * @brief Memory Reader (Mrd)
 *
 * @param[in]  siMEM_RxP_Data  Rx data segment from [MEM].
 * @param[out] soTAIF_Data     Rx data stream to TcpAppInterface (TAIF).
 * @param[in]  siRma_SplitSeg  Split segment from RxMemAccess (Rma).
 *
 * @details
 *  This process is the data read front-end of the memory sub-system (MEM). It
 *  reads in the data from the TCP Rx buffer memory and forwards them to the
 *  application [APP] via the TcpAppInterface (TAIF).
 *  If the received data segment were split and stored as two Rx memory buffers
 *  as indicated by the signal flag 'siRma_SplitSeg', this process will merge
 *  them back into a single stream of bytes before delivering them to the [APP].
 *******************************************************************************/
void pMemReader(
        stream<AxisApp>     &siMEM_RxP_Data,
        stream<TcpAppData>  &soTAIF_Data,
        stream<StsBit>      &siRma_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

	const char *myName = concat3(THIS_NAME, "/", "Mrd");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { MRD_IDLE=0, MRD_STREAM, MRD_JOIN, MRD_STREAMMERGED, \
                            MRD_STREAMUNMERGED, MRD_RESIDUE} \
                                mrd_fsmState=MRD_IDLE;
    #pragma HLS RESET  variable=mrd_fsmState
    static FlagBit              mrd_doubleAccessFlag=FLAG_OFF;
    #pragma HLS RESET  variable=mrd_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp     mrd_currChunk;
    static ap_uint<4>  mrd_memRdOffset;
    static ap_uint<8>  mrd_offsetBuffer;

    switch(mrd_fsmState) {
    case MRD_IDLE:
        if (!siRma_SplitSeg.empty() and !siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            siRma_SplitSeg.read(mrd_doubleAccessFlag);
            siMEM_RxP_Data.read(mrd_currChunk);
            // Count the number of valid bytes in this data chunk
            //OBSOLETE_20200715 mrd_memRdOffset = keepMapping(mrd_currChunk.tkeep);
            mrd_memRdOffset = mrd_currChunk.getLen();
            if (mrd_currChunk.getTLast() and mrd_doubleAccessFlag == FLAG_ON) {
                // This is the last chunk and this access was broken down in 2.
                // Therefore, clear the last flag in the current chunk
                mrd_currChunk.setTLast(~mrd_doubleAccessFlag);
                if (mrd_memRdOffset == 8) {
                    // No need to offset anything. We can forward the current chunk
                    soTAIF_Data.write(mrd_currChunk);
                    if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
                    // Jump to stream merged since there's no joining to be performed
                    mrd_fsmState = MRD_STREAMUNMERGED;
                }
                else if (mrd_memRdOffset < 8) {
                    // The last chunk of the 1st segment-part is not full.
                    // Don't output anything here and go to 'MRD_JOIN'.
                    // There, we will fetch more data to fill in the current chunk.
                    mrd_fsmState = MRD_JOIN;
                }
            }
            else if (mrd_currChunk.getTLast() and mrd_doubleAccessFlag == FLAG_OFF) {
                // This is the 1st and last data chunk of this segment.
                // We are done with the 1st segment. Stay in this default idle state.
                soTAIF_Data.write(mrd_currChunk);
                if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
            }
            else {
                // The 1st segment-part contains more than one chunk.
                // Forward the current chunk and goto 'MRD_STREAM' and process the others.
                soTAIF_Data.write(mrd_currChunk);
                if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
                mrd_fsmState = MRD_STREAM;
            }
        }
        break;
    case MRD_STREAM:
        //-- Handle all but the 1st data chunk of a 1st memory access
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            siMEM_RxP_Data.read(mrd_currChunk);
            // Count the number of valid bytes in this data chunk
            mrd_memRdOffset = mrd_currChunk.getLen();
            if (mrd_currChunk.getTLast() and mrd_doubleAccessFlag == FLAG_ON) {
                // This is the last chunk and this access was broken down in 2.
                // Therefore, clear the last flag in the current chunk
                mrd_currChunk.setTLast(~mrd_doubleAccessFlag);
                if (mrd_memRdOffset == 8) {
                    // No need to offset anything
                    soTAIF_Data.write(mrd_currChunk);
                    if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
                    // Jump to 'STREAMUNMERGED' since there's no joining to be done.
                    mrd_fsmState = MRD_STREAMUNMERGED;
                }
                else if (mrd_memRdOffset < 8) {
                    // The last chunk of the 1st segment-part is not full.
                    // Don't output anything here and go to 'MRD_JOIN'.
                    // There, we will fetch more data to fill in the current chunk.
                    mrd_fsmState = MRD_JOIN;
                }
            }
            else if (mrd_currChunk.getTLast() and mrd_doubleAccessFlag == FLAG_OFF) {
                // This is the 1st and last data chunk of this segment and no
                // memory access breakdown occurred.
                soTAIF_Data.write(mrd_currChunk);
                if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
                // We are done. Goto default idle state.
                mrd_fsmState = MRD_IDLE;
            }
            else {
                // There is more data to read for this segment
                soTAIF_Data.write(mrd_currChunk);
                if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
            }
        }
        break;
    case MRD_STREAMUNMERGED:
        //-- Handle the 2nd memory read access when no realignment is required
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = siMEM_RxP_Data.read();
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
            if (currChunk.getTLast()) {
                // We are done with 2nd segment part. Go back to default idle
                mrd_fsmState = MRD_IDLE;
            }
        }
        break;
    case MRD_JOIN:
        //-- Perform the hand over from the 1st to the 2nd memory access when
        //-- the 1st memory read access has already occurred and its content is
        //-- stored in 'mrd_currChunk'.
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0xFF, 0);
            // In any case, backup the content of the previous data chunk.
            // Here we don't pay attention to the exact number of bytes in the
            // new data chunk. In case they don't fill the entire remaining gap,
            // there will be garbage in the output but it doesn't matter since
            // the 'tkeep' field indicates the valid bytes.
            //OBSOLETE_20200715 currWord.tdata.range(                            (mrd_memRdOffset.to_uint64() * 8) - 1,  0) = \
            //OBSOLETE_20200715                       mrd_currChunk.tdata.range(((mrd_memRdOffset.to_uint64() * 8) - 1), 0);
            currChunk.setLE_TData(mrd_currChunk.getLE_TData((mrd_memRdOffset.to_uint()*8)-1, 0), (mrd_memRdOffset.to_uint()*8)-1, 0);
            // Read the next chunk into static variable
            siMEM_RxP_Data.read(mrd_currChunk);
            // Fill-in the remaining byte of 'currChunk'
            //OBSOLETE_20200715 currWord.tdata.range(63, (mrd_memRdOffset * 8)) = \
            //OBSOLETE_20200715                      mrd_currChunk.tdata.range(((8 - mrd_memRdOffset.to_uint64()) * 8) - 1, 0);
            currChunk.setLE_TData(mrd_currChunk.getLE_TData(((8-mrd_memRdOffset.to_uint())*8)-1, 0), 63, (mrd_memRdOffset*8));
            // Determine how many bytes are valid in the new data chunk.
            // Warning, this might be the only data chunk of the 2nd segment.
            ap_uint<4> byteCounter = mrd_currChunk.getLen();
            // Calculate the number of bytes to go into the next & final data chunk
            mrd_offsetBuffer = byteCounter - (8 - mrd_memRdOffset);
            if (mrd_currChunk.getTLast()) {
                if ((byteCounter + mrd_memRdOffset) <= 8) {
                    // The residue from the 1st segment plus the bytes of the
                    //  1st chunk of the 2nd segment fill this data chunk.
                    // We can set 'tkeep' to the sum of the 2 data chunks's bytes
                    //  as well as 'tlast' because this is the last chunk of 2nd segment.
                    //OBSOELTE_20200715 currChunk.tkeep = returnKeep(byteCounter + mrd_memRdOffset);
                    currChunk.setLE_TKeep(lenToLE_tKeep(byteCounter + mrd_memRdOffset));
                    currChunk.setLE_TLast(TLAST);
                    // We are done with the 2nd segment part. Go back to default idle
                    mrd_fsmState = MRD_IDLE;
                }
                else {
                    // The total number of bytes from the 1st segment and the
                    //  1st chunk of the 2nd segment is > 8.
                    // Goto 'MRD_RESIDUE' and output the remaining data chunks.
                    mrd_fsmState = MRD_RESIDUE;
                }
            }
            else {
                // We have not reached the end of the 2nd segment-part.
                // Goto 'MRD_STREAMMERGED' and output the remaining data chunks.
                mrd_fsmState = MRD_STREAMMERGED;
            }
            // Always forward the current chunk
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
        }
        break;
    case MRD_STREAMMERGED:
        //-- Handle all the remaining data of the 2nd memory access while realigning data.
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0xFF, 0);
            //OBSOELTE_20200715 currWord.tdata.range((mrd_memRdOffset.to_uint64() * 8) - 1, 0) = \
            //OBSOELTE_20200715                       mrd_currChunk.tdata.range(63, ((8 - mrd_memRdOffset.to_uint64()) * 8));
            currChunk.setLE_TData(mrd_currChunk.getLE_TData(63, ((8-mrd_memRdOffset.to_uint())*8)), (mrd_memRdOffset.to_uint()*8)-1, 0);
            // Read the next chunk into static variable
            siMEM_RxP_Data.read(mrd_currChunk);
            // Fill-in the remaining byte of 'currChunk'
            //OBSOELTE_20200715 currWord.tdata.range(63, (mrd_memRdOffset * 8)) = \
            //OBSOELTE_20200715         mrd_currChunk.tdata.range(((8 - mrd_memRdOffset.to_uint64()) * 8) - 1, 0);
            currChunk.setLE_TData(mrd_currChunk.getLE_TData(((8-mrd_memRdOffset.to_uint())*8)-1, 0), 63, (mrd_memRdOffset*8));
            // Determine how any bytes are valid in the new data chunk.
            // Warning, this might be the only data chunk of the 2nd segment.
            ap_uint<4> byteCounter = mrd_currChunk.getLen();
            // Calculate the number of bytes to go into the next & final data chunk
            mrd_offsetBuffer = byteCounter - (8 - mrd_memRdOffset);
            if (mrd_currChunk.getTLast()) {
                if ((byteCounter + mrd_memRdOffset) <= 8) {
                    // The residue from the 1st memory access and the data in the
                    // 1st data chunk of the 2nd memory access fill this data chunk.
                    // We can set 'tkeep' to the sum of the 2 data chunk's bytes
                    //  as well as 'tlast' because this is the last chunk of the
                    // 2nd memory access.
                    currChunk.setLE_TKeep(lenToLE_tKeep(byteCounter + mrd_memRdOffset));
                    currChunk.setLE_TLast(TLAST);
                    // We are done with the 2nd memory access. Go back to default idle
                    mrd_fsmState = MRD_IDLE;
                }
                else {
                    // This is not the last chunk because it doesn't fit in the
                    // available space of the current chunk.
                    // Goto the 'MRD_RESIDUE' and handle the remainder of this data chunk
                    mrd_fsmState = MRD_RESIDUE;
                }
            }
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
        }
        break;
    case MRD_RESIDUE:
        //-- Output the remaining data chunks.
        if (!soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0, TLAST);
            currChunk.setLE_TKeep(lenToLE_tKeep(mrd_offsetBuffer));
            currChunk.setLE_TData(mrd_currChunk.getLE_TData(63, ((8-mrd_memRdOffset.to_uint())*8)), (mrd_memRdOffset.to_uint()*8)-1, 0);
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_MRD) { printAxisRaw(myName, "soTAIF_Data =", mrd_currChunk); }
            // We are done with the very last bytes of a segment.
            // Go back to default idle state.
            mrd_fsmState = MRD_IDLE;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Listen Application Interface (Lai)
 *
 * @param[in]  siTAIF_LsnReq  TCP listen port request from TcpAppInterface (TAIF).
 * @param[out] soTAIF_LsnRep  TCP listen port reply to [TAIF].
 * @param[out] soPRt_LsnReq   Listen port request to PortTable (PRt).
 * @param[in]  siPRt_LsnRep   Listen port reply from [PRt].
 *
 * @details
 *  This process is used to open/close a port in listen mode. This puts the TOE
 *  in passive listening mode and ready to accept an incoming connection on the
 *  socket {MY_IP, THIS_PORT}.
 *  [TODO-FIXME] The tear down of a connection is not implemented yet.
 *
 *******************************************************************************/
void pLsnAppInterface(
        stream<TcpAppLsnReq>    &siTAIF_LsnReq,
        stream<TcpAppLsnRep>    &soTAIF_LsnAck,
        stream<TcpPort>         &soPRt_LsnReq,
        stream<AckBit>          &siPRt_LsnRep)
        //stream<TcpPort>       &siTAIF_StopLsnReq,
        //stream<TcpPort>       &soPRt_CloseReq,)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Lai");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                lai_waitForAck=false;
    #pragma HLS reset variable=lai_waitForAck

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpPort     listenPort;
    AckBit      listenAck;

    if (!siTAIF_LsnReq.empty() and !lai_waitForAck) {
        siTAIF_LsnReq.read(listenPort);
        soPRt_LsnReq.write(listenPort);
        lai_waitForAck = true;
    }
    else if (!siPRt_LsnRep.empty() and lai_waitForAck) {
        siPRt_LsnRep.read(listenAck);
        soTAIF_LsnAck.write((StsBool)listenAck);
        lai_waitForAck = false;
    }

    /*** Close listening port [TODO] ***
    if (!siTAIF_StopLsnReq.empty()) {
        soPRt_CloseReq.write(siTAIF_StopLsnReq.read());
    }
    ***/
}

/*******************************************************************************
 * @brief Rx Application Interface (RAi)
 *
 * @param[out] soTAIF_Notif    Data availability notification to TcpAppInterface (TAIF).
 * @param[in]  siTAIF_DataReq  Request to retrieve data from [TAIF].
 * @param[out] soTAIF_Data     TCP data stream to [TAIF].
 * @param[out] soTAIF_Meta     Metadata to [TAIF].
 * @param[in]  siTAIF_LsnReq   TCP listen port request from [TAIF].
 * @param[out] soTAIF_LsnRep   TCP listen port reply to [TAIF].
 * @param[out] soPRt_LsnReq    TCP listen port request to PortTable (PRt).
 * @param[in]  siPRt_LsnAck    TCP listen port acknowledge from [PRt].
 * @param[in]  siRXe_Notif     Data availability notification from Rx Engine (RXe).
 * @param[in]  siTIm_Notif     Data availability notification from Timers (TIm).
 * @param[out] soRSt_RxSarReq  Rx SAR request to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep  Rx SAR reply from [RSt].
 * @param[out] soMEM_RxP_RdCmd Rx memory read command to Memory sub-system (MEM).
 * @param[in]  siMEM_RxP_Data  Rx memory data stream from [MEM].
 *
 * @details
 *  The Rx Application Interface (Rai) retrieves the data of an established
 *   connection from the TCP Rx buffer memory and forwards them to the
 *   application via the TcpAppInterface (TAIF).
 *  This process is also in charge of opening/closing TCP ports in listen mode
 *   for TOE to be ready for accepting passive incoming connections.
 *******************************************************************************/
void rx_app_interface(
        //-- TAIF / Handshake Interfaces
        stream<TcpAppNotif>         &soTAIF_Notif,
        stream<TcpAppRdReq>         &siTAIF_DataReq,
        //-- TAIF / Data Stream Interfaces
        stream<TcpAppData>          &soTAIF_Data,
        stream<TcpAppMeta>          &soTAIF_Meta,
        //-- TAIF / Listen Interfaces
        stream<TcpAppLsnReq>        &siTAIF_LsnReq,
        stream<TcpAppLsnRep>        &soTAIF_LsnRep,
        //-- PRt / Port Table Interfaces
        stream<TcpPort>             &soPRt_LsnReq,
        stream<AckBit>              &siPRt_LsnAck,
        //-- RXe / Rx Engine Notification Interface
        stream<TcpAppNotif>         &siRXe_Notif,
        //-- TIm / Timers Notification Interface
        stream<TcpAppNotif>         &siTIm_Notif,
        //-- Rx SAR Table Interface
        stream<RAiRxSarQuery>       &soRSt_RxSarReq,
        stream<RAiRxSarReply>       &siRSt_RxSarRep,
        //-- MEM / DDR4 Memory Interface
        stream<DmCmd>               &soMEM_RxP_RdCmd,
        stream<AxisApp>             &siMEM_RxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Rx Application Stream (Ras) -------------------------------------------
	static stream<DmCmd>        ssRasToRma_MemRdCmd ("ssRasToRma_MemRdCmd");
    #pragma HLS stream variable=ssRasToRma_MemRdCmd depth=16

    //-- Rx Memory Access (Rma) ------------------------------------------------
    static stream<StsBit>       ssRmaToMrd_SplitSeg ("ssRmaToMrd_SplitSeg");
    #pragma HLS stream variable=ssRmaToMrd_SplitSeg depth=16

    pRxAppStream(
            siTAIF_DataReq,
            soTAIF_Meta,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            ssRasToRma_MemRdCmd);

    pRxMemAccess(
            ssRasToRma_MemRdCmd,
            soMEM_RxP_RdCmd,
            ssRmaToMrd_SplitSeg);

    pMemReader(
            siMEM_RxP_Data,
            soTAIF_Data,
            ssRmaToMrd_SplitSeg);

    pLsnAppInterface(
            siTAIF_LsnReq,
            soTAIF_LsnRep,
            soPRt_LsnReq,
            siPRt_LsnAck);

    // Notification Mux (Nmx) based on template stream Mux
    //  Notice order --> RXe has higher priority that TIm
    pStreamMux(
            siRXe_Notif,
            siTIm_Notif,
            soTAIF_Notif);

}

/*! \} */
