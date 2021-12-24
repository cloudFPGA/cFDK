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
 * @file       : rx_app_interface.hpp
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

#ifndef _TOE_RAI_H_
#define _TOE_RAI_H_

#include "../../../../NTS/nts_utils.hpp"
#include "../../../../NTS/toe/src/toe.hpp"

using namespace hls;

/*******************************************************************************
 *
 * @brief ENTITY - Rx Application Interface (RAi)
 *
 *******************************************************************************/
void rx_app_interface(
        //-- TAIF /  Rx Segment Interfaces
        stream<TcpAppNotif>         &soTAIF_Notif,
        stream<TcpAppRdReq>         &siTAIF_DataReq,
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
        stream<AxisApp>             &siMEM_RxP_Data,
        //-- MMIO Interface
        stream<ap_uint<8> >         &soMMIO_NotifDropCnt,
        stream<ap_uint<8> >         &soMMIO_MetaDropCnt,
        stream<ap_uint<8> >         &soMMIO_DataDropCnt
);

#endif

/*! \} */
