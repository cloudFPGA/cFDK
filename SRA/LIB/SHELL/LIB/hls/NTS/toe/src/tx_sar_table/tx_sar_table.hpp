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
 * @file       : tx_sar_table.hpp
 * @brief      : Tx Segmentation and re-assembly Table (TSt)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_TST_H_
#define _TOE_TST_H_

#include "../toe.hpp"

using namespace hls;

/*******************************************************************************
 * Tx SAR Table (TSt)
 *  Structure to manage the transmitted data stream in the TCP Tx buffer memory.
 *  Every session is allocated with a static Tx buffer of 64KB to store the
 *  stream of bytes received from the application layer, until the network
 *  layer consumes (.i.e read) them out and acknowledge them.
 *  The Tx buffer is managed as a circular buffer with three pointers:
 *   - 'appw' points to the last byte written by the application layer.
 *   - 'ackd' holds the sequence number of the last transmitted and
 *            acknowledged byte by the network layer (aka SND.NXT),
 *   - 'unak' holds the sequence number of the last transmitted but not yet
 *            acknowledged byte by the network layer (aka SND.UNA).
 *
 *                   ackd                            unak           appw
 *                    |                               |               |
 *                   \|/                             \|/             \|/
 *        --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *          |269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|
 *        --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *
 * [TODO - The structure is also used to manage the send window...]
 *******************************************************************************/
class TxSarEntry {
  public:
    TxBufPtr        appw;        // Points to last written byte by APP
    TxAckNum        unak;        // Bytes TX'ed but not ACK'ed
    TxAckNum        ackd;        // Bytes TX'ed and ACK'ed
    RemotWinSize    recv_window; // Remote receiver's buffer size (their)
    LocalWinSize    cong_window; // Local  receiver's buffer size (mine)
    TcpWindow       slowstart_threshold;
    ap_uint<2>      count;
    bool            fastRetransmitted;
    bool            finReady;
    bool            finSent;
    TxSarEntry() {}
};


/*******************************************************************************
 *
 * @brief ENTITY - Tx SAR Table (TSt)
 *
 *******************************************************************************/
void tx_sar_table(
        //-- Rx engine Interfaces
        stream<RXeTxSarQuery>      &siRXe_TxSarQry,
        stream<RXeTxSarReply>      &soRXe_TxSarRep,
        //-- Tx Engine Interfaces
        stream<TXeTxSarQuery>      &siTXe_TxSarQry,
        stream<TXeTxSarReply>      &soTXe_TxSarRep,
        //-- TCP Application Interfaces
        stream<TAiTxSarPush>       &siTAi_AppPush,
        stream<TStTxSarPush>       &soTAi_AckPush
);

#endif

/*! \} */
