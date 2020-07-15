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
 * @file       : port_table.hpp
 * @brief      : Port Table (PRt) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_PRT_H_
#define _TOE_PRT_H_

#include "../../../../NTS/nts_utils.hpp"
#include "../../../../NTS/toe/src/toe.hpp"

using namespace hls;


typedef AckBit PortState;
#define ACT_FREE_PORT false
#define ACT_USED_PORT true

#define ACTIVE_PORT   false
#define LISTEN_PORT   true

#define PortRange     bool


/*******************************************************************************
 *
 * @brief ENTITY - Port Table (PRt)
 *
 *******************************************************************************/
void port_table(
        //-- Ready Signal
        StsBool                 &poTOE_Ready,
        //-- RXe / Rx Engine Interface
        stream<TcpPort>         &siRXe_GetPortStateReq,
        stream<RepBit>          &soRXe_GetPortStateRep,
        //-- RAi / Rx Application Interface
        stream<TcpPort>         &siRAi_OpenLsnPortReq,
        stream<AckBit>          &soRAi_OpenLsnPortAck,
        //-- TAi / Tx Application Interface
        stream<ReqBit>          &siTAi_GetFreePortReq,
        stream<TcpPort>         &siTAi_GetFreePortRep,
        //-- SLc / Session Lookup Controller Interface
        stream<TcpPort>         &siSLc_ClosePortCmd
);

#endif

/*! \} */
