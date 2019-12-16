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
 * @file       : tx_sar_table.cpp
 * @brief      : Tx Segmentation And Re-assembly Table (TSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
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
#include "../../test/test_toe_utils.hpp"

#define TRACE_OFF  0x0000
#define TRACE_TST 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*******************************************************************************
 * @brief Tx Sar Table (TSt) - Stores the data structures for managing the
 *         TCP Tx buffer and Tx sliding window.
 *
 * @param[in]  siRXe_TxSarQry, Query from RxEngine (RXe).
 * @param[out] soRXe_TxSarRep, Reply to [RXe].
 * @param[in]  siTXe_TxSarQry, Tx SAR query from TxEngine (TXe).
 * @param[out] soTXe_TxSarRep, Tx SAR reply to [TXe].
 * @param[in]  siTAi_PushCmd,  Push command from TxAppInterface (TAi).
 * @param[out] soTAi_PushCmd,  Push command to [TAi].
 *
 * @details
 *  This process is accessed by the RxEngine (RXe), the TxEngine (TXe) and the
 *   TxAppInterface (TAi).
 *
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
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS --------------------------------------------------------
    static TxSarEntry               TX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS DEPENDENCE variable=TX_SAR_TABLE inter false
    #pragma HLS RESOURCE   variable=TX_SAR_TABLE core=RAM_T2P_BRAM
    #pragma HLS RESET      variable=TX_SAR_TABLE


    if (!siTXe_TxSarQry.empty()) {
        TXeTxSarQuery sTXeQry;
        //----------------------------------------
        //-- Query from TX Engine
        //----------------------------------------
        siTXe_TxSarQry.read(sTXeQry);
        if (sTXeQry.write) {
            //-- Write Query
            if (!sTXeQry.isRtQuery) {
                TX_SAR_TABLE[sTXeQry.sessionID].not_ackd = sTXeQry.not_ackd;
                if (sTXeQry.init) {
                    TX_SAR_TABLE[sTXeQry.sessionID].app         = sTXeQry.not_ackd;
                    TX_SAR_TABLE[sTXeQry.sessionID].ackd        = sTXeQry.not_ackd-1;
                    TX_SAR_TABLE[sTXeQry.sessionID].cong_window = 0x3908; // 10 x 1460(MSS)
                    TX_SAR_TABLE[sTXeQry.sessionID].slowstart_threshold = 0xFFFF;
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady    = sTXeQry.finReady;
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent     = sTXeQry.finSent;
                    // Init ACK on the TxAppInterface side
                    soTAi_PushCmd.write(TStTxSarPush(sTXeQry.sessionID,
                                                     sTXeQry.not_ackd, CMD_INIT));
                }
                if (sTXeQry.finReady) {
                    TX_SAR_TABLE[sTXeQry.sessionID].finReady = sTXeQry.finReady;
                }
                if (sTXeQry.finSent) {
                    TX_SAR_TABLE[sTXeQry.sessionID].finSent  = sTXeQry.finSent;
                }
            }
            else {
                //-- Write RtQuery
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
            //-- Read Query
            TcpWindow   minWindow;
            if (TX_SAR_TABLE[sTXeQry.sessionID].cong_window < TX_SAR_TABLE[sTXeQry.sessionID].recv_window) {
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].cong_window;
            }
            else {
                minWindow = TX_SAR_TABLE[sTXeQry.sessionID].recv_window;
            }
            soTXe_TxSarRep.write(TXeTxSarReply(TX_SAR_TABLE[sTXeQry.sessionID].ackd,
                                               TX_SAR_TABLE[sTXeQry.sessionID].not_ackd,
                                               minWindow,
                                               TX_SAR_TABLE[sTXeQry.sessionID].app,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finReady,
                                               TX_SAR_TABLE[sTXeQry.sessionID].finSent));
        }
    }

    else if (!siTAi_PushCmd.empty()) {
        TAiTxSarPush sTAiCmd;
        //---------------------------------------
        //-- Update the 'txAppPtr' in the table
        //---------------------------------------
        siTAi_PushCmd.read(sTAiCmd);
        TX_SAR_TABLE[sTAiCmd.sessionID].app = sTAiCmd.app;
    }

    else if (!siRXe_TxSarQry.empty()) {
        RXeTxSarQuery sRXeQry;
        //---------------------------------------
        //-- RX Engine
        //---------------------------------------
        siRXe_TxSarQry.read(sRXeQry);
        if (sRXeQry.write) {
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
                                               TX_SAR_TABLE[sRXeQry.sessionID].not_ackd,
                                               TX_SAR_TABLE[sRXeQry.sessionID].cong_window,
                                               TX_SAR_TABLE[sRXeQry.sessionID].slowstart_threshold,
                                               TX_SAR_TABLE[sRXeQry.sessionID].count,
                                               TX_SAR_TABLE[sRXeQry.sessionID].fastRetransmitted));
        }
    }
}
