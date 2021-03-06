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
 * @file       : ack_delay.hpp
 * @brief      : ACK Delayer (AKd) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_AKD_H_
#define _TOE_AKD_H_

#include "../../../../../NTS/nts_utils.hpp"
#include "../../../../../NTS/toe/src/toe.hpp"

using namespace hls;

/*******************************************************************************
 * ACK Table (RSt)
 *  Structure to manage the transmission rate of the ACKs.
 *
 * [TODO - The type of 'AckEntry.delay' can be configured as a functions of 'TIME_XXX' and 'MAX_SESSIONS']
 *   [ E.g. - TIME_64us  = ( 64.000/0.0064/32) + 1 =  313 = 0x139 ( 9-bits)
 *   [ E.g. - TIME_64us  = ( 64.000/0.0064/ 8) + 1 = 1251 = 0x4E3 (11-bits)
 *   [ E.g. - TIME_128us = (128.000/0.0064/32) + 1 =  626 = 0x271 (10-bits)
 *******************************************************************************/
class AckEntry {
  public:
    ap_uint<12> delay;  // Keeps track of the elapsed time
    ap_uint<4>  count;  // Counts the number of received ACKs

    AckEntry() :
        delay(0), count(0) {}
    AckEntry(ap_uint<12> delay, ap_uint<4> count) :
        delay(delay), count(count) {}

};

/*******************************************************************************
 *
 * @brief ENTITY - Acknowledgment Delayer (AKd)
 *
 *******************************************************************************/
void ack_delay(
        //-- Event Engine Interfaces
        stream<ExtendedEvent>   &siEVe_Event,
        stream<SigBit>          &soEVe_RxEventSig,
        stream<SigBit>          &soEVe_TxEventSig,
        //-- Tx Engine Interface
        stream<ExtendedEvent>   &soTXe_Event
);

#endif

/*! \} */
