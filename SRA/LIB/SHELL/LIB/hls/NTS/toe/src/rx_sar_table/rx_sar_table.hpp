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
 * @file       : rx_sar_table.hpp
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

#ifndef _TOE_RST_H_
#define _TOE_RST_H_

#include "../toe.hpp"

using namespace hls;

/*******************************************************************************
 * Rx SAR Table (RSt)
 *  Structure to manage the received data stream in the TCP Rx buffer memory.
 *  Every session is allocated with a static Rx buffer of 64KB to store the
 *  stream of bytes received from the network layer, until the application layer
 *  consumes (.i.e read) them out. The Rx buffer is managed as a circular buffer
 *  with an insertion and an extraction pointer.
 *   - 'rcvd' holds the sequence number of the last received and acknowledged
 *            byte from the network layer,
 *   - 'appd' holds the sequence number of the next byte ready to be read (.i.e
 *            consumed) by the application layer.
 *
 *               appd                                    rcvd
 *                |                                       |
 *               \|/                                     \|/
 *        --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *          |269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|
 *        --+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--
 *
 *******************************************************************************/
class RxSarEntry {
  public:
    RxSeqNum    rcvd;  // Bytes RCV'ed and ACK'ed (same as Receive Next)
    RxSeqNum    appd;  // Bytes READ (.i.e consumed) by the application
    RxSarEntry() {}
};

/*******************************************************************************
 *
 * @brief ENTITY - Rx SAR Table (RSt)
 *
 *******************************************************************************/
void rx_sar_table(
        //-- Rx Engine Interfaces
        stream<RXeRxSarQuery>      &siRXe_RxSarQry,
        stream<RxSarReply>         &soRXe_RxSarRep,
        //-- Rx Application Interfaces
        stream<RAiRxSarQuery>      &siRAi_RxSarQry,
        stream<RAiRxSarReply>      &soRAi_RxSarRep,
        //-- Tx Engine Interfaces
        stream<SessionId>          &siTXe_RxSarReq,
        stream<RxSarReply>         &soTxe_RxSarRep
);

#endif

/*! \} */
