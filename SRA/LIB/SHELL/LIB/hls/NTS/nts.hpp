/*
 * Copyright 2016 -- 2020 IBM Corporation
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
 */

/*******************************************************************************
 * @file       : nts.hpp
 * @brief      : Definition of the Network Transport Stack (NTS) component
 *               as if it was an HLS IP core.
 *
 * System:     : cloudFPGA
 * Component   : Shell
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _NTS_H_
#define _NTS_H_

#include "nts_types.hpp"

using namespace hls;


/**********************************************************
 * INTERFACE - TCP APPLICATION INTERFACE (TAIF)
 **********************************************************/


/**********************************************************
 * INTERFACE - UDP APPLICATION INTERFACE (UAIF)
 **********************************************************/
//-- UAIF / Control Interfaces
typedef UdpPort     UdpAppLsnReq;
typedef StsBool     UdpAppLsnRep;
typedef UdpPort     UdpAppClsReq; // [FIXME-What about creating a class 'AppLsnReq' with a member 'start/stop']
//-- UAIF / Datagram Interfaces
typedef AxisApp     UdpAppData;
typedef SocketPair  UdpAppMeta;
typedef UdpLen      UdpAppDLen;


/*******************************************************************************
 *
 * ENTITY - NETWORK TRANSPORT STACK (NTS)
 *
 *******************************************************************************/
void nts(

        //------------------------------------------------------
        //-- TAIF / Received Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>     &soTAIF_Notif,
        stream<TcpAppRdReq>     &siTAIF_DReq,
        stream<TcpAppData>      &soTAIF_Data,
        stream<TcpAppMeta>      &soTAIF_Meta,

        //------------------------------------------------------
        //-- TAIF / Listen Port Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>    &siTAIF_LsnReq,
        stream<TcpAppLsnRep>    &soTAIF_LsnRep,

        //------------------------------------------------------
        //-- TAIF / Transmit Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppData>      &siTAIF_Data,
        stream<TcpAppMeta>      &siTAIF_Meta,
        stream<TcpAppWrSts>     &soTAIF_DSts,

        //------------------------------------------------------
        //-- TAIF / Open Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>    &siTAIF_OpnReq,
        stream<TcpAppOpnRep>    &soTAIF_OpnRep,

        //------------------------------------------------------
        //-- TAIF / Close Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>    &siTAIF_ClsReq,
        //-- Not USed           &soTAIF_ClsSts,

        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpAppLsnReq>    &siUAIF_LsnReq,
        stream<UdpAppLsnRep>    &soUAIF_LsnRep,
        stream<UdpAppClsReq>    &siUAIF_ClsReq,

        //------------------------------------------------------
        //-- UAIF / Received Datagram Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soUAIF_Data,
        stream<UdpAppMeta>      &soUAIF_Meta,

        //------------------------------------------------------
        //-- UAIF / Transmit Datatagram Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen

);

#endif

/*! \} */















