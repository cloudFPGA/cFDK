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

/*****************************************************************************
 * @file       : nts.hpp
 * @brief      : Definition of the Network Transport Stack (NTS) component
 *               as if it was an HLS IP core.
 *
 * System:     : cloudFPGA
 * Component   : Shell
 * Language    : Vivado HLS
 *
 * \ingroup NTS_UOE
 * \addtogroup NTS_UOE
 * \{
 *****************************************************************************/

#ifndef NTS_H_
#define NTS_H_

#include "nts_types.hpp"

//#include <hls_stream.h>

//#include "AxisEth.hpp"   // ETHernet
//#include "AxisIp4.hpp"   // IPv4

using namespace hls;


/**********************************************************
 * INTERFACE - UDP APPLICATION INTERFACE (UAIF)
 **********************************************************/
typedef AxisApp      UdpAppData;
typedef SocketPair   UdpAppMeta;
typedef UdpLen       UdpAppDLen;

/**********************************************************
 * INTERFACE - TCP APPLICATION INTERFACE (TAIF)
 **********************************************************/


/*************************************************************************
 *
 * ENTITY - NETWORK TRANSPORT STACK (NTS)
 *
 *
 *************************************************************************/
void nts(

        //------------------------------------------------------
        //-- UAIF / Control Port Interfaces
        //------------------------------------------------------
        stream<UdpPort>         &siUAIF_LsnReq,
        stream<StsBool>         &soUAIF_LsnRep,
        stream<UdpPort>         &siUAIF_ClsReq,

        //------------------------------------------------------
        //-- UAIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &soUAIF_Data,
        stream<UdpAppMeta>      &soUAIF_Meta,

        //------------------------------------------------------
        //-- UAIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<UdpAppData>      &siUAIF_Data,
        stream<UdpAppMeta>      &siUAIF_Meta,
        stream<UdpAppDLen>      &siUAIF_DLen

);

#endif

/*! \} */















