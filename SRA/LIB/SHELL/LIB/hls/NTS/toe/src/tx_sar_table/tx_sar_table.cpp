/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/************************************************
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
 * @file       : tx_sar_table.cpp
 * @brief      : Tx Segmentation and re-assembly Table (TSt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *****************************************************************************/

#include "tx_sar_table.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_TST)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TSt"

#define TRACE_OFF  0x0000
#define TRACE_TST 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Tx Sar Table (TSt). Stores the data structures for managing the
 *         TCP Tx buffer and Tx sliding window.
 *
 * @param[in]  siRXe_TxSarQry  Query from RxEngine (RXe).
 * @param[out] soRXe_TxSarRep  Reply to [RXe].
 * @param[in]  siTXe_TxSarQry  Tx SAR query from TxEngine (TXe).
 * @param[out] soTXe_TxSarRep  Tx SAR reply to [TXe].
 * @param[in]  siTAi_PushCmd   Push command from TxAppInterface (TAi).
 * @param[out] soTAi_PushCmd   Push command to [TAi].
 *
 * @details
 *  This process is accessed by the RxEngine (RXe), the TxEngine (TXe) and the
 *   TxAppInterface (TAi).
 *******************************************************************************/
void tx_sar_table(
        stream<RXeTxSarQuery>      &siRXe_TxSarQry,
        stream<RXeTxSarReply>      &soRXe_TxSarRep,
        stream<TXeTxSarQuery>      &siTXe_TxSarQry,
        stream<TXeTxSarReply>      &soTXe_TxSarRep,
        stream<TAiTxSarPush>       &siTAi_PushCmd,
        stream<TStTxSarPush>       &soTAi_PushCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS ---------------------------------------------------------
    static TxSarEntry               TX_SAR_TABLE[TOE_MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_SAR_TABLE inter false
    #pragma HLS RESOURCE   variable=TX_SAR_TABLE core=RAM_2P

    if (!siTXe_TxSarQry.empty()) {
        TXeTxSarQuery sTXeQry;
        //----------------------------------------
        //-- Rd/Wr Query from TX Engine
        //----------------------------------------
        siTXe_TxSarQry.read(sTXeQry);
        if (sTXeQry.write) {
            //-- TXe Write Query
            if (not sTXeQry.isRtQuery) {
                TX_SAR_TABLE[sTXeQry.sessionID].unak = sTXeQry.not_ackd;
                if (sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].appw        = sTXeQry.not_ackd;
                    TX_SAR_TABLE[sTXeQry.sessionID].ackd        = sTXeQry.not_ackd-1;
                    TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = 0xFFFF;
                    // Avoid initializing 'finReady' and 'finSent' at two different
                    // places because it will translate into II=2 and DRC message:
                    // 'Unable to schedule store operation on array due to limited memory ports'.

                    // Init ACK on the TxAppInterface side
                    soTAi_PushCmd.write(TStTxSarPush(sTXeQry.sessionID,
                                                     sTXeQry.not_ackd, CMD_INIT));
                }
                if (sTXeQry.finReady or sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady = sTXeQry.finReady;
                }
                if (sTXeQry.finSent or sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent  = sTXeQry.finSent;
                }
            }
            else {
                //-- TXe Write RtQuery
                TXeTxSarRtQuery sTXeRtQry = sTXeQry;
                TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = sTXeRtQry.getThreshold();
                TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS) TODO is this correct or less, eg. 1/2 * MSS
                if (DEBUG_LEVEL & TRACE_TST) {
                    printInfo(myName, "Received a Retry-Write query from TXe for session #%d.\n",
                            sTXeQry.sessionID.to_int());
                }
            }
        }
        else {
            //-- TXe Read Query
            TxSarEntry txSarEntry = TX_SAR_TABLE[sTXeQry.sessionID];

            TcpWindow   minWindow;
            if (txSarEntry.cong_window < txSarEntry.recv_window) {
                minWindow = txSarEntry.cong_window;
            }
            else {
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].recv_window;
            }
            soTXe_TxSarRep.write(TXeTxSarReply(TX_SAR_TABLE[sTXeQry.sessionID].ackd,
                                               TX_SAR_TABLE[sTXeQry.sessionID].unak,
                                               minWindow,
                                               TX_SAR_TABLE[sTXeQry.sessionID].appw,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finReady,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finSent));
        }
    }
    else if (!siTAi_PushCmd.empty()) {
        TAiTxSarPush sTAiCmd;
        //---------------------------------------
        //-- Wr Command from TX APP Interface
        //---------------------------------------
        siTAi_PushCmd.read(sTAiCmd);
        //--  Update the 'txAppWrPtr'
        TX_SAR_TABLE[sTAiCmd.sessionID].appw = sTAiCmd.app;
    }
    else if (!siRXe_TxSarQry.empty()) {
        RXeTxSarQuery sRXeQry;
        //---------------------------------------
        //-- Rd/Wr Query from RX Engine
        //---------------------------------------
        siRXe_TxSarQry.read(sRXeQry);
        if (sRXeQry.write == QUERY_WR) {
            TX_SAR_TABLE[sRXeQry.sessionID].ackd        = sRXeQry.ackd;
            TX_SAR_TABLE[sRXeQry.sessionID].recv_window = sRXeQry.recv_window;
            TX_SAR_TABLE[sRXeQry.sessionID].cong_window = sRXeQry.cong_window;
            TX_SAR_TABLE[sRXeQry.sessionID].count       = sRXeQry.count;
            TX_SAR_TABLE[sRXeQry.sessionID].fastRetransmitted = sRXeQry.fastRetransmitted;
            // Push ACK to txAppInterface
            soTAi_PushCmd.write(TStTxSarPush(sRXeQry.sessionID, sRXeQry.ackd));
        }
        else {
            //-- Read Query
            soRXe_TxSarRep.write(RXeTxSarReply(TX_SAR_TABLE[sRXeQry.sessionID].ackd,
                                               TX_SAR_TABLE[sRXeQry.sessionID].unak,
                                               TX_SAR_TABLE[sRXeQry.sessionID].cong_window,
                                               TX_SAR_TABLE[sRXeQry.sessionID].slowstart_threshold,
                                               TX_SAR_TABLE[sRXeQry.sessionID].count,
                                               TX_SAR_TABLE[sRXeQry.sessionID].fastRetransmitted));
        }
    }
}

/*! \} */
