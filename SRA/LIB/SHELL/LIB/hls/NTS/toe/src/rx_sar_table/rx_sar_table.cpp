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
 * @file       : rx_sar_table.cpp
 * @brief      : Rx Segmentation And Re-assembly Table (RSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "rx_sar_table.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_RST)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/RSt"

#define TRACE_OFF  0x0000
#define TRACE_RST 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Rx SAR Table (RSt)
 *
 * @param[in]  siRXe_RxSarQry  Query from RxEngine (RXe).
 * @param[out] soRXe_RxSarRep  Reply to RXe.
 * @param[in]  siRAi_RxSarQry  Query from RxAppInterface (RAi).
 * @param[out] soRAi_RxSarRep  Reply to [RAi].
 * @param[in]  siTXe_RxSarReq  Read request from TxEngine (TXe).
 * @param[out] soTxe_RxSarRep  Read reply to [TXe].
 *
 * @details
 *  This process stores the structures for managing the received data stream in
 *  the TCP Rx buffer memory. An Rx buffer of 64KB is allocated to every session
 *  and the stream of bytes within every session is managed with two pointers:
 *   - 'rcvd' holds the sequence number of the last received and acknowledged
 *             byte from the network layer,
 *   - 'appd' holds a pointer to the next byte ready to be read (.i.e, consumed)
 *            by the application layer.
 *
 *            appd        rcvd           oooTail             oooHead
 *             |           |               |                   |
 *            \|/         \|/             \|/                 \|/
 *     --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *       |269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|
 *     --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *
 *  This process is concurrently accessed by the RxEngine (RXe), the TxEngine
 *   (TXe) and the RxApplicationInterface (RAi), but TXe access is read-only.
 *******************************************************************************/
void rx_sar_table(
        stream<RXeRxSarQuery>      &siRXe_RxSarQry,
        stream<RxSarReply>         &soRXe_RxSarRep,
        stream<RAiRxSarQuery>      &siRAi_RxSarQry,
        stream<RAiRxSarReply>      &soRAi_RxSarRep,
        stream<SessionId>          &siTXe_RxSarReq,
        stream<RxSarReply>         &soTxe_RxSarRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 //OBSOLETE_20210819 enable_flush
	//OBSOLETE_20210819 #pragma HLS INLINE off

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS ---------------------------------------------------------
    static RxSarEntry               RX_SAR_TABLE[TOE_MAX_SESSIONS];
    //OBSOLETE_20210819 #pragma HLS RESOURCE   variable=RX_SAR_TABLE core=RAM_1P_BRAM
    #pragma HLS RESOURCE   variable=RX_SAR_TABLE core=RAM_2P
    #pragma HLS DEPENDENCE variable=RX_SAR_TABLE inter false
    //OBSOLETE_20210819 #pragma HLS RESET      variable=RX_SAR_TABLE

    if(!siTXe_RxSarReq.empty()) {
        SessionId sessId;
        //---------------------------------------
        // [TXe] - Read only access
        //---------------------------------------
        siTXe_RxSarReq.read(sessId);
        soTxe_RxSarRep.write(RxSarReply(RX_SAR_TABLE[sessId].appd,
                                        RX_SAR_TABLE[sessId].rcvd,
                                        RX_SAR_TABLE[sessId].ooo,
                                        RX_SAR_TABLE[sessId].oooHead,
                                        RX_SAR_TABLE[sessId].oooTail));
        if (DEBUG_LEVEL & TRACE_RST) {
            RxBufPtr free_space = ((RX_SAR_TABLE[sessId].appd -
                          (RxBufPtr)RX_SAR_TABLE[sessId].oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
            printInfo(myName, "[TXe] Read  S%d - FreeSpace=%d\n",
                              sessId.to_uint(), free_space.to_uint());
        }
    }
    else if(!siRAi_RxSarQry.empty()) {
        RAiRxSarQuery raiQry;
        //----------------------------------------------------------------
        // [RAi] - Read or write access to update the application pointer
        //----------------------------------------------------------------
        siRAi_RxSarQry.read(raiQry);
        if(raiQry.write) {
            RX_SAR_TABLE[raiQry.sessionID].appd = raiQry.appd;
            if (DEBUG_LEVEL & TRACE_RST) {
                RxBufPtr free_space = ((RX_SAR_TABLE[raiQry.sessionID].appd -
                              (RxBufPtr)RX_SAR_TABLE[raiQry.sessionID].oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
                printInfo(myName, "[RAi] Write S%d - FreeSpace=%d\n",
                                  raiQry.sessionID.to_uint(), free_space.to_uint());
            }
        }
        else {
            soRAi_RxSarRep.write(RAiRxSarReply(raiQry.sessionID,
                                               RX_SAR_TABLE[raiQry.sessionID].appd));
            if (DEBUG_LEVEL & TRACE_RST) {
                RxBufPtr free_space = ((RX_SAR_TABLE[raiQry.sessionID].appd -
                              (RxBufPtr)RX_SAR_TABLE[raiQry.sessionID].oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
                printInfo(myName, "[RAi] Read  S%d - FreeSpace=%d\n",
                                  raiQry.sessionID.to_uint(), free_space.to_uint());
            }
        }
    }
    else if(!siRXe_RxSarQry.empty()) {
        RXeRxSarQuery rxeQry = siRXe_RxSarQry.read();
        //---------------------------------------
        // [RXe] - Write access
        //---------------------------------------
        if (rxeQry.write) {
            RX_SAR_TABLE[rxeQry.sessionID].rcvd = rxeQry.rcvd;
            if (rxeQry.init) {
                RX_SAR_TABLE[rxeQry.sessionID].appd    = rxeQry.rcvd;
            }
            if (rxeQry.ooo) {
                RX_SAR_TABLE[rxeQry.sessionID].ooo     = true;
                RX_SAR_TABLE[rxeQry.sessionID].oooHead = rxeQry.oooHead;
                RX_SAR_TABLE[rxeQry.sessionID].oooTail = rxeQry.oooTail;
            }
            else {
                RX_SAR_TABLE[rxeQry.sessionID].ooo     = false;
                RX_SAR_TABLE[rxeQry.sessionID].oooHead = rxeQry.rcvd;
                RX_SAR_TABLE[rxeQry.sessionID].oooTail = rxeQry.rcvd;
            }
            if (DEBUG_LEVEL & TRACE_RST) {
                RxBufPtr free_space = ((RX_SAR_TABLE[rxeQry.sessionID].appd -
                              (RxBufPtr)RX_SAR_TABLE[rxeQry.sessionID].oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
                printInfo(myName, "[RXe] Write S%d - FreeSpace=%d\n",
                                  rxeQry.sessionID.to_uint(), free_space.to_uint());
            }
        }
        else {
            // Read access from [RXe]
            soRXe_RxSarRep.write(RxSarReply(RX_SAR_TABLE[rxeQry.sessionID].appd,
                                            RX_SAR_TABLE[rxeQry.sessionID].rcvd,
                                            RX_SAR_TABLE[rxeQry.sessionID].ooo,
                                            RX_SAR_TABLE[rxeQry.sessionID].oooHead,
                                            RX_SAR_TABLE[rxeQry.sessionID].oooTail));
            if (DEBUG_LEVEL & TRACE_RST) {
                RxBufPtr free_space = ((RX_SAR_TABLE[rxeQry.sessionID].appd -
                              (RxBufPtr)RX_SAR_TABLE[rxeQry.sessionID].oooHead(TOE_WINDOW_BITS-1, 0)) - 1);
                printInfo(myName, "[RXe] Read  S%d - FreeSpace=%d\n",
                                  rxeQry.sessionID.to_uint(), free_space.to_uint());
            }
        }
    }
}

/*! \} */
