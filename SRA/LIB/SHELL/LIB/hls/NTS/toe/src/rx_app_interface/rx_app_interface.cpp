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
#define TRACE_ASS  1 <<  2
#define TRACE_NMX  1 <<  3
#define TRACE_RAS  1 <<  4
#define TRACE_MRD  1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

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
 * @param[out] soMrd_MemRdCmd  Rx memory read command to Rx MemoryReader (Mrd).
 *
 * @detail
 *  This process waits for a valid data read request from the TcpAppInterface
 *   (TAIF) and generates a corresponding read command for the TCP Rx buffer
 *   memory via the Rx MemoryReader (Mrd) process. Next, a request to update the
 *   RxApp pointer of the session is forwarded to the RxSarTable (RSt) and a
 *   meta-data (.i.e the current session-id) is sent back to [TAIF] to signal
 *   that the request has been processed.
 *******************************************************************************/
void pRxAppStream(
    stream<TcpAppRdReq>         &siTAIF_DataReq,
    stream<TcpAppMeta>          &soTAIF_Meta,
    stream<RAiRxSarQuery>       &soRSt_RxSarQry,
    stream<RAiRxSarReply>       &siRSt_RxSarRep,
    stream<DmCmd>               &soMrd_MemRdCmd)
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
                !soMrd_MemRdCmd.full() && !soRSt_RxSarQry.full()) {
            RAiRxSarReply rxSarRep = siRSt_RxSarRep.read();
            // Signal that the data request has been processed by sending
            // the SessId back to [TAIF]
            soTAIF_Meta.write(rxSarRep.sessionID);
            // Generate a memory buffer read command
            RxMemPtr memSegAddr = TOE_RX_MEMORY_BASE;
            memSegAddr(29, 16) = rxSarRep.sessionID(13, 0);
            memSegAddr(15,  0) = rxSarRep.appd;
            soMrd_MemRdCmd.write(DmCmd(memSegAddr, ras_readLength));
            // Update the APP read pointer
            soRSt_RxSarQry.write(RAiRxSarQuery(rxSarRep.sessionID, rxSarRep.appd+ras_readLength));
            ras_fsmState = S0;
        }
        break;
    }
}

/*******************************************************************************
 * @brief Rx Memory Reader (Mrd)
 *
 * @param[in]  siRas_MemRdCmd   Rx memory read command from RxAppStream (Ras).
 * @param[out] soMEM_RxpRdCmd   Rx memory read command to [MEM].
 * @param[out] soAss_SplitSeg   Split segment to AppSegmentStitcher (Ass).
 *
 * @details
 *  This process takes the memory read command assembled by RxAppStream (Ras)
 *   an forwards it to the memory sub-system (MEM). While doing so, it checks if
 *   the TCP Rx memory buffer wraps around and accordingly generates two memory
 *   read commands out of the initial command received from [Ras].
 *  Because the AppSegmetStitcher (Ass) process needs to be aware of this split,
 *   a signal is sent to [Ass] telling whether a data segment was broken in two
 *   Rx memory buffers or is provided as a single buffer.
 *******************************************************************************/
void pRxMemoryReader(
        stream<DmCmd>     &siRas_MemRdCmd,
        stream<DmCmd>     &soMEM_RxpRdCmd,
        stream<FlagBool>  &soAss_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Mrd");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmState { MRD_1ST_ACCESS=0, MRD_2ND_ACCESS } \
                               mrd_fsmState=MRD_1ST_ACCESS;
    #pragma HLS RESET variable=mrd_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd    mrd_memRdCmd;
    static RxBufPtr mrd_firstAccLen;
    static uint16_t mrd_debugCounter=1;

    switch (mrd_fsmState) {
    case MRD_1ST_ACCESS:
        if (!siRas_MemRdCmd.empty() and !soAss_SplitSeg.full() and !soMEM_RxpRdCmd.full() ) {
            siRas_MemRdCmd.read(mrd_memRdCmd);

            if ((mrd_memRdCmd.saddr.range(TOE_WINDOW_BITS-1, 0) + mrd_memRdCmd.bbt) > TOE_RX_BUFFER_SIZE) {
                // This segment was broken in two memory accesses because TCP Rx memory buffer wrapped around
                mrd_firstAccLen = TOE_RX_BUFFER_SIZE - mrd_memRdCmd.saddr;
                mrd_fsmState = MRD_2ND_ACCESS;

                soMEM_RxpRdCmd.write(DmCmd(mrd_memRdCmd.saddr, mrd_firstAccLen));
                soAss_SplitSeg.write(true);

                if (DEBUG_LEVEL & TRACE_MRD) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This segment is broken in two memory accesses.\n");
                    printInfo(myName, "Issuing 1st memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              mrd_debugCounter, mrd_memRdCmd.saddr.to_uint(), mrd_firstAccLen.to_uint());
                }
            }
            else {
                soMEM_RxpRdCmd.write(mrd_memRdCmd);
                soAss_SplitSeg.write(false);

                if (DEBUG_LEVEL & TRACE_MRD) {
                    printInfo(myName, "Issuing memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              mrd_debugCounter, mrd_memRdCmd.saddr.to_uint(), mrd_memRdCmd.bbt.to_uint());
                    mrd_debugCounter++;
                }
            }
        }
        break;
    case MRD_2ND_ACCESS:
        if (!soMEM_RxpRdCmd.full()) {
            // Update the command to account for the Rx buffer wrap around
            mrd_memRdCmd.saddr(TOE_WINDOW_BITS-1, 0) = 0;
            soMEM_RxpRdCmd.write(DmCmd(mrd_memRdCmd.saddr, mrd_memRdCmd.bbt - mrd_firstAccLen));

            mrd_fsmState = MRD_1ST_ACCESS;

            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "Issuing 2nd memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                          mrd_debugCounter, mrd_memRdCmd.saddr.to_ulong(),
                         (mrd_memRdCmd.bbt - mrd_firstAccLen).to_uint());
                mrd_debugCounter++;
            }
        }
        break;
    }

    /*** OBSOLETE_20201014 *****************************************************

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static FlagBit             mrd_doubleAccessFlag=FLAG_OFF;
    #pragma HLS reset variable=mrd_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd               mrd_memRdCmd;
    static TcpSegLen           mrd_memRdLen;
    static uint16_t            mrd_debugCounter=1;

    if (mrd_doubleAccessFlag == FLAG_OFF) {
        if (!siRas_MemRdCmd.empty() && !soMEM_RxpRdCmd.full()) {
            mrd_memRdCmd = siRas_MemRdCmd.read();
            if ((mrd_memRdCmd.saddr.range(15, 0) + mrd_memRdCmd.bbt) > RXMEMBUF) {
                // Break this memory access in two because TCP Rx memory buffer wraps around
                mrd_memRdLen = RXMEMBUF - mrd_memRdCmd.saddr;
                DmCmd memRdCmd = DmCmd(mrd_memRdCmd.saddr, mrd_memRdLen);
                soMEM_RxpRdCmd.write(memRdCmd);
                mrd_doubleAccessFlag = FLAG_ON;
                if (DEBUG_LEVEL & TRACE_MRD) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This is the first memory accesses.\n");
                    printInfo(myName, "Issuing 1st memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              mrd_debugCounter, mrd_memRdCmd.saddr.to_uint(), mrd_memRdLen.to_uint());
                }
            }
            else {
                soMEM_RxpRdCmd.write(mrd_memRdCmd);
                if (DEBUG_LEVEL & TRACE_MRD) {
                    printDmCmd(myName, mrd_memRdCmd);
                }
            }
            soAss_SplitSeg.write(mrd_doubleAccessFlag);
        }
    }
    else if (mrd_doubleAccessFlag == FLAG_ON) {
        if (!soMEM_RxpRdCmd.full()) {
            mrd_memRdCmd.saddr.range(15, 0) = 0;
            mrd_memRdLen = mrd_memRdCmd.bbt - mrd_memRdLen;
            DmCmd memRdCmd = DmCmd(mrd_memRdCmd.saddr, mrd_memRdLen);
            soMEM_RxpRdCmd.write(memRdCmd);
            mrd_doubleAccessFlag = FLAG_OFF;
            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "TCP Rx memory buffer wraps around: This is the 2nd memory access.\n");
                printDmCmd(myName, memRdCmd);
            }
        }
    }
    ***************************************************************************/
}

/*** OBSOLETE_20201014 *********************************************************
 * @brief Rx Memory Access (Rma)
 *
 * @param[in]  siRas_MemRdCmd   Rx memory read command from RxAppStream (Ras).
 * @param[out] soMEM_RxpRdCmd   Rx memory read command to [MEM].
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
/*** OBSOLETE_20201014 *********************************************************
void pRxMemAccess(
        stream<DmCmd>   &siRas_MemRdCmd,
        stream<DmCmd>   &soMEM_RxpRdCmd,
        stream<StsBit>  &soMrd_SplitSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Rma");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmState { RMA_1ST_ACCESS=0, RMA_2ND_ACCESS } \
                               rma_fsmState=RMA_1ST_ACCESS;
    #pragma HLS RESET variable=rma_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd    rma_memRdCmd;
    static TxBufPtr rma_firstAccLen;
    static uint16_t rma_debugCounter=1;

    switch (rma_fsmState) {
    case RMA_1ST_ACCESS:
        if (!siRas_MemRdCmd.empty() and !soMrd_SplitSeg.full() and !soMEM_RxpRdCmd.full() ) {
            siRas_MemRdCmd.read(rma_memRdCmd);
            if ((rma_memRdCmd.saddr.range(TOE_WINDOW_BITS-1, 0) + rma_memRdCmd.bbt) > TOE_RX_BUFFER_SIZE) {
                // This segment was broken in two memory accesses because TCP Rx memory buffer wrapped around
                rma_firstAccLen = TOE_RX_BUFFER_SIZE - rma_memRdCmd.saddr;
                rma_fsmState = RMA_2ND_ACCESS;

                soMEM_RxpRdCmd.write(DmCmd(rma_memRdCmd.saddr, rma_firstAccLen));
                soMrd_SplitSeg.write(true);

                if (DEBUG_LEVEL & TRACE_RMA) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This segment is broken in two memory accesses.\n");
                    printInfo(myName, "Issuing 1st memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              rma_debugCounter, rma_memRdCmd.saddr.to_uint(), rma_firstAccLen.to_uint());
                }
                else {
                    soMEM_RxpRdCmd.write(rma_memRdCmd);
                    soMrd_SplitSeg.write(false);

                    if (DEBUG_LEVEL & TRACE_RMA) {
                        printInfo(myName, "Issuing memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                                  rma_debugCounter, rma_memRdCmd.saddr.to_uint(), rma_memRdCmd.bbt.to_uint());
                        rma_debugCounter++;
                    }
                }
            }
        }
        break;
    case RMA_2ND_ACCESS:
        if (!soMEM_RxpRdCmd.full()) {
            soMEM_RxpRdCmd.write(DmCmd(TOE_RX_MEMORY_BASE, rma_memRdCmd.bbt - rma_firstAccLen));
            rma_fsmState = RMA_1ST_ACCESS;

            if (DEBUG_LEVEL & TRACE_RMA) {
                printInfo(myName, "Issuing 2nd memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                          rma_debugCounter, 0, (rma_memRdCmd.bbt - rma_firstAccLen).to_uint());
                rma_debugCounter++;
            }
        }
        break;
    }

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static FlagBit             rma_doubleAccessFlag=FLAG_OFF;
    #pragma HLS reset variable=rma_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd               rma_memRdCmd;
    static TcpSegLen           rma_memRdLen;
    static uint16_t            rma_debugCounter=1;

    if (rma_doubleAccessFlag == FLAG_OFF) {
        if (!siRas_MemRdCmd.empty() && !soMEM_RxpRdCmd.full()) {
            rma_memRdCmd = siRas_MemRdCmd.read();
            if ((rma_memRdCmd.saddr.range(15, 0) + rma_memRdCmd.bbt) > RXMEMBUF) {
                // Break this memory access in two because TCP Rx memory buffer wraps around
                rma_memRdLen = RXMEMBUF - rma_memRdCmd.saddr;
                DmCmd memRdCmd = DmCmd(rma_memRdCmd.saddr, rma_memRdLen);
                soMEM_RxpRdCmd.write(memRdCmd);
                rma_doubleAccessFlag = FLAG_ON;
                if (DEBUG_LEVEL & TRACE_RMA) {
                    printInfo(myName, "TCP Rx memory buffer wraps around: This is the first memory accesses.\n");
                    printInfo(myName, "Issuing 1st memory read command #%d - SADDR=0x%9.9x - BBT=%d\n",
                              rma_debugCounter, rma_memRdCmd.saddr.to_uint(), rma_memRdLen.to_uint());
                }
            }
            else {
                soMEM_RxpRdCmd.write(rma_memRdCmd);
                if (DEBUG_LEVEL & TRACE_RMA) {
                    printDmCmd(myName, rma_memRdCmd);
                }
            }
            soMrd_SplitSeg.write(rma_doubleAccessFlag);
        }
    }
    else if (rma_doubleAccessFlag == FLAG_ON) {
        if (!soMEM_RxpRdCmd.full()) {
            rma_memRdCmd.saddr.range(15, 0) = 0;
            rma_memRdLen = rma_memRdCmd.bbt - rma_memRdLen;
            DmCmd memRdCmd = DmCmd(rma_memRdCmd.saddr, rma_memRdLen);
            soMEM_RxpRdCmd.write(memRdCmd);
            rma_doubleAccessFlag = FLAG_OFF;
            if (DEBUG_LEVEL & TRACE_RMA) {
                printInfo(myName, "TCP Rx memory buffer wraps around: This is the 2nd memory access.\n");
                printDmCmd(myName, memRdCmd);
            }
        }
    }
}
********************************************************************************/

/*******************************************************************************
 * @brief Application Segment Stitcher (Ass)
 *
 * @param[in]  siMEM_RxP_Data  Rx data segment from [MEM].
 * @param[out] soTAIF_Data     Rx data stream to TcpAppInterface (TAIF).
 * @param[in]  siMrd_SplitSeg  Split segment from Rx MemoryReader (MRd).
 *
 * @details
 *  This process is the data read front-end of the memory sub-system (MEM). It
 *  reads in the data from the TCP Rx buffer memory and forwards them to the
 *  application [APP] via the TcpAppInterface (TAIF).
 *  If a received data segment was splitted and stored as two Rx memory buffers
 *  as indicated by the signal flag 'siMrd_SplitSeg', this process will stitch
 *  them back into a single stream of bytes before delivering them to the [APP].
 *******************************************************************************/
void pAppSegmentStitcher(
        stream<AxisApp>     &siMEM_RxP_Data,
        stream<TcpAppData>  &soTAIF_Data,
        stream<FlagBool>    &siMrd_SplitSegFlag)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    const char *myName = concat3(THIS_NAME, "/", "Ass");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmState {ASS_IDLE,
                          ASS_FWD_1ST_BUF,  ASS_FWD_2ND_BUF,
                          ASS_JOIN_2ND_BUF, ASS_RESIDUE } \
                               ass_fsmState=ASS_IDLE;
    #pragma HLS RESET variable=ass_fsmState
    static ap_uint<3>          ass_psdHdrChunkCount = 0;
    #pragma HLS RESET variable=ass_psdHdrChunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp      ass_prevChunk;
    static ap_uint<4>   ass_memRdOffset;
    static FlagBool     ass_mustJoin;

    switch(ass_fsmState) {
    case ASS_IDLE:
        //-- Handle the very 1st data chunk from the 1st memory buffer
        if (!siMEM_RxP_Data.empty() and !siMrd_SplitSegFlag.empty() and !soTAIF_Data.full()) {
            siMrd_SplitSegFlag.read(ass_mustJoin);
            AxisApp currAppChunk = siMEM_RxP_Data.read();

            if (currAppChunk.getTLast()) {
                // We are done with the 1st memory buffer
                if (ass_mustJoin == false) {
                    // The TCP segment was not splitted.
                    // We are done with this segment. Stay in this state.
                    soTAIF_Data.write(currAppChunk);
                    ass_fsmState = ASS_IDLE;
                    if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
                }
                else {
                    // The TCP segment was splitted in two parts
                    ass_memRdOffset = currAppChunk.getLen();
                    if (ass_memRdOffset != 8) {
                        // The last chunk of the 1st memory buffer is not fully populated.
                        // Don't output anything here. Save the current chunk and goto 'ASS_JOIN_2ND'.
                        // There, we will fetch more data to fill in the current chunk.
                        ass_prevChunk = currAppChunk;
                        ass_fsmState = ASS_JOIN_2ND_BUF;
                    }
                    else {
                        // The last chunk of the 1st memory buffer is populated with
                        // 8 valid bytes and is therefore also aligned.
                        // Forward this chunk and goto 'ASS_FWD_2ND_BUF'.
                        soTAIF_Data.write(currAppChunk);
                        if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
                        ass_fsmState = ASS_FWD_2ND_BUF;
                    }
                }
            }
            else {
               // The 1st memory buffer contains more than one chunk
               soTAIF_Data.write(currAppChunk);
               if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
               ass_fsmState = ASS_FWD_1ST_BUF;
            }
        }
        break;
    case ASS_FWD_1ST_BUF:
        //-- Forward all the data chunks of the 1st memory buffer
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currAppChunk = siMEM_RxP_Data.read();

            if (currAppChunk.getTLast()) {
                // We are done with the 1st memory buffer
                if (ass_mustJoin == false) {
                    // The TCP segment was not splitted.
                    // We are done with this segment. Go back to 'ASS_IDLE'.
                    soTAIF_Data.write(currAppChunk);
                    ass_fsmState = ASS_IDLE;
                    if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
                }
                else {
                    // The TCP segment was splitted in two parts
                    ass_memRdOffset = currAppChunk.getLen();
                    // Always clear the last bit of the last chunk of 1st part
                    currAppChunk.setTLast(0);
                    if (ass_memRdOffset != 8) {
                        // The last chunk of the 1st memory buffer is not fully populated.
                        // Don't output anything here. Save the current chunk and goto 'TSS_JOIN_2ND'.
                        // There, we will fetch more data to fill in the current chunk.
                        ass_prevChunk = currAppChunk;
                        ass_fsmState = ASS_JOIN_2ND_BUF;
                    }
                    else {
                        // The last chunk of the 1st memory buffer is populated with
                        // 8 valid bytes and is therefore also aligned.
                        // Forward this chunk and goto 'TSS_FWD_2ND_BUF'.
                        soTAIF_Data.write(currAppChunk);
                        if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
                        ass_fsmState = ASS_FWD_2ND_BUF;
                    }
                }
            }
            else {
                // Remain in this state and continue streaming the 1st memory buffer
                soTAIF_Data.write(currAppChunk);
                if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
             }
        }
        break;
    case ASS_FWD_2ND_BUF:
        //-- Forward all the data chunks of the 2nd memory buffer
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currAppChunk = siMEM_RxP_Data.read();

            soTAIF_Data.write(currAppChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", currAppChunk); }
            if (currAppChunk.getTLast()) {
                // We are done with the 2nd memory buffer
                ass_fsmState = ASS_IDLE;
            }
        }
        break;
    case ASS_JOIN_2ND_BUF:
        //-- Join the bytes from the 2nd memory buffer to the stream of bytes of
        //-- the 1st memory buffer when the 1st buffer was not fully populated.
        //-- The re-alignment occurs between the previously read chunk stored
        //-- in 'tss_prevChunk' and the latest chunk stored in 'currAppChunk',
        //-- and 'tss_memRdOffset' specifies the number of valid bytes in 'tss_prevChunk'.
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currAppChunk = siMEM_RxP_Data.read();

            AxisApp joinedChunk(0,0,0);  // [FIXME-Create a join method in AxisRaw]
            // Set lower-part of the joined chunk with the last bytes of the previous chunk
            joinedChunk.setLE_TData(ass_prevChunk.getLE_TData(((int)ass_memRdOffset*8)-1, 0),
                                                              ((int)ass_memRdOffset*8)-1, 0);
            joinedChunk.setLE_TKeep(ass_prevChunk.getLE_TKeep( (int)ass_memRdOffset   -1, 0),
                                                               (int)ass_memRdOffset   -1, 0);
            // Set higher part of the joined chunk with the first bytes of the current chunk
            joinedChunk.setLE_TData(currAppChunk.getLE_TData( ARW   -1-((int)ass_memRdOffset*8), 0),
                                                              ARW   -1,((int)ass_memRdOffset*8));
            joinedChunk.setLE_TKeep(currAppChunk.getLE_TKeep((ARW/8)-1- (int)ass_memRdOffset, 0),
                                                             (ARW/8)-1, (int)ass_memRdOffset);
            if (currAppChunk.getLE_TKeep()[8-(int)ass_memRdOffset] == 0) {
                // The entire current chunk fits into the remainder of the previous chunk.
                // We are done with this 2nd memory buffer.
                joinedChunk.setLE_TLast(TLAST);
                ass_fsmState = ASS_IDLE;
            }
            else if (currAppChunk.getLE_TLast()) {
                // This cannot be the last chunk because it doesn't fit into the
                // available space of the previous chunk. Goto the 'ASS_RESIDUE'
                // and handle the remainder of this data chunk
                ass_fsmState = ASS_RESIDUE;
            }

            soTAIF_Data.write(joinedChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", joinedChunk); }

            // Move remainder of current chunk to previous chunk
            ass_prevChunk.setLE_TData(currAppChunk.getLE_TData(64-1, 64-(int)ass_memRdOffset*8),
                                                               ((int)ass_memRdOffset*8)-1, 0);
            ass_prevChunk.setLE_TKeep(currAppChunk.getLE_TKeep(8-1, 8-(int)ass_memRdOffset),
                                                               (int)ass_memRdOffset-1, 0);
        }
        break;
    case ASS_RESIDUE:
        //-- Output the very last unaligned chunk
        if (!soTAIF_Data.full()) {
            AxisApp lastChunk = AxisApp(0, 0, TLAST);
            lastChunk.setLE_TData(ass_prevChunk.getLE_TData(((int)ass_memRdOffset*8)-1, 0),
                                                            ((int)ass_memRdOffset*8)-1, 0);
            lastChunk.setLE_TKeep(ass_prevChunk.getLE_TKeep((int)ass_memRdOffset-1, 0),
                                                            (int)ass_memRdOffset-1, 0);

            soTAIF_Data.write(lastChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", lastChunk); }
            ass_fsmState = ASS_IDLE;
        }
        break;
    }

    /*** OBSOLETE_20201016 *****************************************************
    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ASS_IDLE=0, ASS_STREAM, ASS_JOIN, ASS_STREAMMERGED, \
                            ASS_STREAMUNMERGED, ASS_RESIDUE} \
                                ass_fsmState=ASS_IDLE;
    #pragma HLS RESET  variable=ass_fsmState
    static FlagBit              ass_doubleAccessFlag=FLAG_OFF;
    #pragma HLS RESET  variable=ass_doubleAccessFlag

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp     ass_currChunk;
    static ap_uint<4>  ass_memRdOffset;
    static ap_uint<8>  ass_offsetBuffer;

    switch(ass_fsmState) {
    case ASS_IDLE:
        if (!siMrd_SplitSeg.empty() and !siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            siMrd_SplitSeg.read(ass_doubleAccessFlag);
            siMEM_RxP_Data.read(ass_currChunk);
            // Count the number of valid bytes in this data chunk
            ass_memRdOffset = ass_currChunk.getLen();
            if (ass_currChunk.getTLast() and ass_doubleAccessFlag == FLAG_ON) {
                // This is the last chunk and this access was broken down in 2.
                // Therefore, clear the last flag in the current chunk
                ass_currChunk.setTLast(~ass_doubleAccessFlag);
                if (ass_memRdOffset == 8) {
                    // No need to offset anything. We can forward the current chunk
                    soTAIF_Data.write(ass_currChunk);
                    if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
                    // Jump to stream merged since there's no joining to be performed
                    ass_fsmState = ASS_STREAMUNMERGED;
                }
                else if (ass_memRdOffset < 8) {
                    // The last chunk of the 1st segment-part is not full.
                    // Don't output anything here and go to 'ASS_JOIN'.
                    // There, we will fetch more data to fill in the current chunk.
                    ass_fsmState = ASS_JOIN;
                }
            }
            else if (ass_currChunk.getTLast() and ass_doubleAccessFlag == FLAG_OFF) {
                // This is the last data chunk of this segment.
                // We are done with the 1st segment. Stay in this default idle state.
                soTAIF_Data.write(ass_currChunk);
                if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
            }
            else {
                // The 1st segment-part contains more than one chunk.
                // Forward the current chunk and goto 'ASS_STREAM' and process the others.
                soTAIF_Data.write(ass_currChunk);
                if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
                ass_fsmState = ASS_STREAM;
            }
        }
        break;
    case ASS_STREAM:
        //-- Handle all but the 1st data chunk of a 1st memory access
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            siMEM_RxP_Data.read(ass_currChunk);
            // Count the number of valid bytes in this data chunk
            ass_memRdOffset = ass_currChunk.getLen();
            if (ass_currChunk.getTLast() and ass_doubleAccessFlag == FLAG_ON) {
                // This is the last chunk and this access was broken down in 2.
                // Therefore, clear the last flag in the current chunk
                ass_currChunk.setTLast(~ass_doubleAccessFlag);
                if (ass_memRdOffset == 8) {
                    // No need to offset anything
                    soTAIF_Data.write(ass_currChunk);
                    if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
                    // Jump to 'STREAMUNMERGED' since there's no joining to be done.
                    ass_fsmState = ASS_STREAMUNMERGED;
                }
                else if (ass_memRdOffset < 8) {
                    // The last chunk of the 1st segment-part is not full.
                    // Don't output anything here and go to 'ASS_JOIN'.
                    // There, we will fetch more data to fill in the current chunk.
                    ass_fsmState = ASS_JOIN;
                }
            }
            else if (ass_currChunk.getTLast() and ass_doubleAccessFlag == FLAG_OFF) {
                // This is the 1st and last data chunk of this segment and no
                // memory access breakdown occurred.
                soTAIF_Data.write(ass_currChunk);
                if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
                // We are done. Goto default idle state.
                ass_fsmState = ASS_IDLE;
            }
            else {
                // There is more data to read for this segment
                soTAIF_Data.write(ass_currChunk);
                if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
            }
        }
        break;
    case ASS_STREAMUNMERGED:
        //-- Handle the 2nd memory read access when no realignment is required
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = siMEM_RxP_Data.read();
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
            if (currChunk.getTLast()) {
                // We are done with 2nd segment part. Go back to default idle
                ass_fsmState = ASS_IDLE;
            }
        }
        break;
    case ASS_JOIN:
        //-- Perform the hand over from the 1st to the 2nd memory access when
        //-- the 1st memory read access has already occurred and its content is
        //-- stored in 'ass_currChunk'.
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0xFF, 0);
            // In any case, backup the content of the previous data chunk.
            // Here we don't pay attention to the exact number of bytes in the
            // new data chunk. In case they don't fill the entire remaining gap,
            // there will be garbage in the output but it doesn't matter since
            // the 'tkeep' field indicates the valid bytes.
            currChunk.setLE_TData(ass_currChunk.getLE_TData((ass_memRdOffset.to_uint()*8)-1, 0), (ass_memRdOffset.to_uint()*8)-1, 0);
            // Read the next chunk into static variable
            siMEM_RxP_Data.read(ass_currChunk);
            // Fill-in the remaining byte of 'currChunk'
            currChunk.setLE_TData(ass_currChunk.getLE_TData(((8-ass_memRdOffset.to_uint())*8)-1, 0), 63, (ass_memRdOffset*8));
            // Determine how many bytes are valid in the new data chunk.
            // Warning, this might be the only data chunk of the 2nd segment.
            ap_uint<4> byteCounter = ass_currChunk.getLen();
            // Calculate the number of bytes to go into the next & final data chunk
            ass_offsetBuffer = byteCounter - (8 - ass_memRdOffset);
            if (ass_currChunk.getTLast()) {
                if ((byteCounter + ass_memRdOffset) <= 8) {
                    // The residue from the 1st segment plus the bytes of the
                    //  1st chunk of the 2nd segment fill this data chunk.
                    // We can set 'tkeep' to the sum of the 2 data chunks's bytes
                    //  as well as 'tlast' because this is the last chunk of 2nd segment.
                    //OBSOELTE_20200715 currChunk.tkeep = returnKeep(byteCounter + ass_memRdOffset);
                    currChunk.setLE_TKeep(lenToLE_tKeep(byteCounter + ass_memRdOffset));
                    currChunk.setLE_TLast(TLAST);
                    // We are done with the 2nd segment part. Go back to default idle
                    ass_fsmState = ASS_IDLE;
                }
                else {
                    // The total number of bytes from the 1st segment and the
                    //  1st chunk of the 2nd segment is > 8.
                    // Goto 'ASS_RESIDUE' and output the remaining data chunks.
                    ass_fsmState = ASS_RESIDUE;
                }
            }
            else {
                // We have not reached the end of the 2nd segment-part.
                // Goto 'ASS_STREAMMERGED' and output the remaining data chunks.
                ass_fsmState = ASS_STREAMMERGED;
            }
            // Always forward the current chunk
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
        }
        break;
    case ASS_STREAMMERGED:
        //-- Handle all the remaining data of the 2nd memory access while realigning data.
        if (!siMEM_RxP_Data.empty() and !soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0xFF, 0);
            currChunk.setLE_TData(ass_currChunk.getLE_TData(63, ((8-ass_memRdOffset.to_uint())*8)), (ass_memRdOffset.to_uint()*8)-1, 0);
            // Read the next chunk into static variable
            siMEM_RxP_Data.read(ass_currChunk);
            // Fill-in the remaining byte of 'currChunk'
            currChunk.setLE_TData(ass_currChunk.getLE_TData(((8-ass_memRdOffset.to_uint())*8)-1, 0), 63, (ass_memRdOffset*8));
            // Determine how any bytes are valid in the new data chunk.
            // Warning, this might be the only data chunk of the 2nd segment.
            ap_uint<4> byteCounter = ass_currChunk.getLen();
            // Calculate the number of bytes to go into the next & final data chunk
            ass_offsetBuffer = byteCounter - (8 - ass_memRdOffset);
            if (ass_currChunk.getTLast()) {
                if ((byteCounter + ass_memRdOffset) <= 8) {
                    // The residue from the 1st memory access and the data in the
                    // 1st data chunk of the 2nd memory access fill this data chunk.
                    // We can set 'tkeep' to the sum of the 2 data chunk's bytes
                    //  as well as 'tlast' because this is the last chunk of the
                    // 2nd memory access.
                    currChunk.setLE_TKeep(lenToLE_tKeep(byteCounter + ass_memRdOffset));
                    currChunk.setLE_TLast(TLAST);
                    // We are done with the 2nd memory access. Go back to default idle
                    ass_fsmState = ASS_IDLE;
                }
                else {
                    // This is cannot be the last chunk because it doesn't fit in the
                    // available space of the current chunk.
                    // Goto the 'ASS_RESIDUE' and handle the remainder of this data chunk
                    ass_fsmState = ASS_RESIDUE;
                }
            }
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
        }
        break;
    case ASS_RESIDUE:
        //-- Output the remaining data chunks.
        if (!soTAIF_Data.full()) {
            AxisApp currChunk = AxisApp(0, 0, TLAST);
            currChunk.setLE_TKeep(lenToLE_tKeep(ass_offsetBuffer));
            currChunk.setLE_TData(ass_currChunk.getLE_TData(63, ((8-ass_memRdOffset.to_uint())*8)), (ass_memRdOffset.to_uint()*8)-1, 0);
            soTAIF_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_ASS) { printAxisRaw(myName, "soTAIF_Data =", ass_currChunk); }
            // We are done with the very last bytes of a segment.
            // Go back to default idle state.
            ass_fsmState = ASS_IDLE;
        }
        break;
    }
    ****************************************************************************/
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
	static stream<DmCmd>        ssRasToMrd_MemRdCmd ("ssRasToMrd_MemRdCmd");
    #pragma HLS stream variable=ssRasToMrd_MemRdCmd depth=16

    //-- Rx Memory Reader (Mrd) ------------------------------------------------
    static stream<FlagBool>     ssMrdToAss_SplitSeg ("ssMrdToAss_SplitSeg");
    #pragma HLS stream variable=ssMrdToAss_SplitSeg depth=16

    pRxAppStream(
            siTAIF_DataReq,
            soTAIF_Meta,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            ssRasToMrd_MemRdCmd);

    pRxMemoryReader(
            ssRasToMrd_MemRdCmd,
            soMEM_RxP_RdCmd,
            ssMrdToAss_SplitSeg);

    pAppSegmentStitcher(
            siMEM_RxP_Data,
            soTAIF_Data,
            ssMrdToAss_SplitSeg);

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
