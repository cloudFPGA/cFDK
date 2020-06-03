/************************************************
Copyright (c) 2016-2019, IBM Research.

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
 * @file       : tcp_shell_if.hpp
 * @brief      : TCP Shell Interface (TSIF).
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP shell interface.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include "ap_int.h"

#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/toe/src/toe.hpp"
#include "../../../../cFDK/SRA/LIB/SHELL/LIB/hls/toe/test/test_toe_utils.hpp"

using namespace hls;


/*************************************************************************
 *
 * ENTITY - TCP SHELL INTERFACE (TSIF)
 *
 *************************************************************************/
void tcp_shell_if(

        //------------------------------------------------------
        //-- SHELL / Mmio Interface
        //------------------------------------------------------
        CmdBit                *piSHL_Mmio_En,

        //------------------------------------------------------
        //-- TAF / TxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &siTAF_Data,
        stream<AppMetaAxis>   &siTAF_SessId,

        //------------------------------------------------------
        //-- TAF / RxP Data Interface
        //------------------------------------------------------
        stream<AppData>       &soTAF_Data,
        stream<AppMetaAxis>   &soTAF_SessId,

        //------------------------------------------------------
        //-- SHELL / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>      &siSHL_Notif,
        stream<AppRdReq>      &soSHL_DReq,
        stream<AppData>       &siSHL_Data,
        stream<AppMetaAxis>   &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReqAxis> &soSHL_LsnReq,
        stream<AppLsnAckAxis> &siSHL_LsnAck,

        //------------------------------------------------------
        //-- SHELL / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>       &soSHL_Data,
        stream<AppMetaAxis>   &soSHL_SessId,
        stream<AppWrSts>      &siSHL_DSts,

        //------------------------------------------------------
        //-- SHELL / Tx Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>     &soSHL_OpnReq,
        stream<AppOpnRep>     &siSHL_OpnRep,

        //------------------------------------------------------
        //-- SHELL / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReqAxis> &soSHL_ClsReq,

        //------------------------------------------------------
        //-- TAF / Session Connect Id Interface
        //------------------------------------------------------
        SessionId             *poTAF_SConId

);

