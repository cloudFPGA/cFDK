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
 * @file       : rx_sar_table.cpp
 * @brief      : Rx Segmentation And Re-assembly Table (RSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

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


/*****************************************************************************
 * @brief Rx SAR Table (RSt) - Stores the data structures for managing the
 *         TCP Rx buffer and the Rx sliding window.
 *
 * @param[in]  siRXe_RxSarQry,    Query from RxEngine (RXe).
 * @param[out] soRXe_RxSarRep,    Reply to RXe.
 * @param[in]  siRAi_RxSarQry,    Query from RxAppInterface (RAi).
 * @param[out] soRAi_RxSarRep,    Reply to [RAi].
 * @param[in]  siTXe_RxSarReq,    Read request from TxEngine (TXe).
 * @param[out] soTxe_RxSarRep,    Read reply to [TXe].
 *
 * @details
 *  This process is concurrently accessed by the RxEngine (RXe), the
 *   RxApplicationInterface (RAi) and the TxEngine (TXe).
 *  The access by the TXe is read-only.
 *  
 *****************************************************************************/
void rx_sar_table(
        stream<RXeRxSarQuery>      &siRXe_RxSarQry,
        stream<RxSarEntry>         &soRXe_RxSarRep,
        stream<RAiRxSarQuery>      &siRAi_RxSarQry,
        stream<RAiRxSarReply>      &soRAi_RxSarRep,
        stream<SessionId>          &siTXe_RxSarReq,
        stream<RxSarEntry>         &soTxe_RxSarRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName = THIS_NAME;

    //-- STATIC ARRAYS --------------------------------------------------------
    static RxSarEntry               RX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RX_SAR_TABLE core=RAM_1P_BRAM
    #pragma HLS DEPENDENCE variable=RX_SAR_TABLE inter false
    #pragma HLS RESET      variable=RX_SAR_TABLE





    if(!siTXe_RxSarReq.empty()) {
        SessionId sessId;
        // Read only access from the Tx Engine
        siTXe_RxSarReq.read(sessId);
        soTxe_RxSarRep.write(RX_SAR_TABLE[sessId]);
    }
    else if(!siRAi_RxSarQry.empty()) {
        RAiRxSarQuery RAiQry;
        // Read or write access from the Rx App I/F to update the application pointer
        siRAi_RxSarQry.read(RAiQry);
        if(RAiQry.write) {
            RX_SAR_TABLE[RAiQry.sessionID].appd = RAiQry.appd;
        }
        else {
            soRAi_RxSarRep.write(RAiRxSarReply(RAiQry.sessionID, RX_SAR_TABLE[RAiQry.sessionID].appd));
        }
    }
    else if(!siRXe_RxSarQry.empty()) {
        RXeRxSarQuery RXeQry;
        // Read or write access from the RxEngine
        siRXe_RxSarQry.read(RXeQry);
        if (RXeQry.write) {
            RX_SAR_TABLE[RXeQry.sessionID].rcvd = RXeQry.rcvd;
            if (RXeQry.init) {
                RX_SAR_TABLE[RXeQry.sessionID].appd = RXeQry.rcvd;
            }
        }
        else {
            soRXe_RxSarRep.write(RX_SAR_TABLE[RXeQry.sessionID]);
        }
    }
}
