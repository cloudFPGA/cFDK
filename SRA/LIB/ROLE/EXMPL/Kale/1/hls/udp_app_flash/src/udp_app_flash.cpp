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
 * @file       : udp_app_flash.cpp
 * @brief      : UDP Application Flash (UAF)
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This application implements a set of UDP-oriented tests and
 *  functions which are embedded into the Flash of the cloudFPGA role.
 *
 * @note       : For the time being, we continue designing with the DEPRECATED
 *               directives because the new PRAGMAs do not work for us.
 *
 *****************************************************************************/

#include "udp_app_flash.hpp"

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we may continue to design
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not always work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

#define MTU    1500    // Maximum Transmission Unit in bytes [TODO:Move to a common place]


/*****************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the UDP connection.
 *         The echo is said to operate in "store-and-forward" mode because
 *         every received packet is stored into the DDR4 memory before being
 *         read again and and sent back.
 *
 * @param[in]  siRXp_Data,  data from the Rx Path (RXp).
 * @param[out] soTXp_Data,  data to the Tx PAth (TXp).
 *
 * @return Nothing.
 ******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as store-and-forward]
        stream<UdpWord>     &siRXp_Data,
        stream<UdpWord>     &soTXp_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
    static stream<UdpWord>  sFifo ("sFifo");
    #pragma HLS stream      variable=sFifo depth=8

    //-- FiFo Push
    if ( !siRXp_Data.empty() && !sFifo.full() )
        sFifo.write(siRXp_Data.read());

    //-- FiFo Pop
    if ( !sFifo.empty() && !soTXp_Data.full() )
        soTXp_Data.write(sFifo.read());
}


/*****************************************************************************
 * @brief Transmit Path - From THIS to SHELL.
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siEPt_Data,         data from pEchoPassTrough.
 * @param[in]  siESf_Data,         data from pEchoStoreAndForward.
 * @param[out] soSHL_Data,         data to SHELL.
 *
 * @return Nothing.
 *****************************************************************************/
void pTXPath(
        ap_uint<2>           piSHL_MmioEchoCtrl,
        stream<UdpWord>     &siEPt_Data,
        stream<UdpWord>     &siESf_Data,
        stream<UdpWord>     &soSHL_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
    UdpWord    udpWord;

    //-- Forward incoming chunk to SHELL
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
            // Read data chunk from pEchoPassThrough
            if ( !siEPt_Data.empty() ) {
                udpWord = siEPt_Data.read();
            }
            else
                return;
            break;

        case ECHO_STORE_FWD:
            // Read data chunk from pEchoStoreAndForward
            if ( !siESf_Data.empty() )
                udpWord = siESf_Data.read();
            break;

        case ECHO_OFF:
            // Read data chunk from TBD
            break;

        default:
            // Reserved configuration ==> Do nothing
            break;
    }

    //-- Forward data chunk to SHELL
    if ( !soSHL_Data.full() )
        soSHL_Data.write(udpWord);
}


/*****************************************************************************
 * @brief Receive Path - From SHELL to THIS.
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siSHL_Data,         data from SHELL.
 * @param[out] soEPt_Data,         data to pEchoPassTrough.
 * @param[out] soESf_Data,         data to pEchoStoreAndForward.
 *
 * @return Nothing.
 ******************************************************************************/
void pRXPath(
        ap_uint<2>            piSHL_MmioEchoCtrl,
        stream<UdpWord>      &siSHL_Data,
        stream<UdpWord>      &soEPt_Data,
        stream<UdpWord>      &soESf_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
    UdpWord    udpWord;

    //-- Read incoming data chunk
    if ( !siSHL_Data.empty() )
        udpWord = siSHL_Data.read();
    else
        return;

    // Forward data chunk to Echo function
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
            // Forward data chunk to pEchoPathThrough
            if ( !soEPt_Data.full() )
                soEPt_Data.write(udpWord);
            break;

        case ECHO_STORE_FWD:
            // Forward data chunk to pEchoStoreAndForward
            if ( !soESf_Data.full() )
                soESf_Data.write(udpWord);
            break;

        case ECHO_OFF:
            // Drop the packet
            break;

        default:
            // Drop the packet
            break;
    }
}


/*****************************************************************************
 * @brief   Main process of the UDP Application Flash
 * @ingroup udp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl,  configures the echo function.
 * @param[in]  piSHL_MmioPostPktEn, enables posting of UDP packets.
 * @param[in]  piSHL_MmioCaptPktEn, enables capture of UDP packets.
 * @param[in]  siSHL_Data           UDP data stream from the SHELL.
 * @param[out] soSHL_Data           UDP data stream to the SHELL.
 *
 * @return Nothing.
 *****************************************************************************/
void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_MmioPostPktEn,
        //[TODO] ap_uint<1> piSHL_MmioCaptPktEn,

        //------------------------------------------------------
        //-- SHELL / UDP Rx Interfaces
        //------------------------------------------------------
        stream<UdpWord>     &siSHL_Data,

        //------------------------------------------------------
        //-- SHELL / UDP Tx Interfaces
        //------------------------------------------------------
        stream<UdpWord>     &soSHL_Data)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable               port=piSHL_MmioEchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable      port=piSHL_MmioPostPktEn
    //[TODO] #pragma HLS INTERFACE ap_stable      port=piSHL_MmioCaptPktEn

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Data metadata="-bus_bundle soSHL_Data"

#else

    #pragma HLS INTERFACE ap_stable               port=piSHL_MmioEchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable      port=piSHL_MmioPostPktEn
    //[TODO] #pragma HLS INTERFACE ap_stable      port=piSHL_MmioCaptPktEn

    #pragma HLS INTERFACE axis register both      port=siSHL_This_Data
    #pragma HLS INTERFACE axis register both      port=soTHIS_Shl_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)

    //-------------------------------------------------------------------------
    //-- Rx Path (RXp)
    //-------------------------------------------------------------------------
    //static stream<AppData>      sRXpToTXp_Data    ("sRXpToTXp_Data");
    static stream<UdpWord>      sRXpToTXp_Data    ("sRXpToTXp_Data");
    #pragma HLS STREAM variable=sRXpToTXp_Data    depth=2048
    //static stream<AppMeta>      sRXpToTXp_Meta    ("sRXpToTXp_Meta");
    //#pragma HLS STREAM variable=sRXpToTXp_Meta    depth=64

    //static stream<AppData>      sRXpToESf_Data    ("sRXpToESf_Data");
    static stream<UdpWord>        sRXpToESf_Data  ("sRXpToESf_Data");
    #pragma HLS STREAM variable=sRXpToESf_Data    depth=2
    //static stream<AppMeta>      sRXpToESf_Meta    ("sRXpToESf_Meta");
    //#pragma HLS STREAM variable=sRXpToESf_Meta    depth=2

    //-------------------------------------------------------------------------
    //-- Echo Store and Forward (ESf)
    //-------------------------------------------------------------------------
    //static stream<AppData>      sESfToTXp_Data    ("sESfToTXp_Data");
    static stream<UdpWord>      sESfToTXp_Data    ("sESfToTXp_Data");
    #pragma HLS STREAM variable=sESfToTXp_Data    depth=2
    //static stream<AppMeta>      sESfToTXp_Meta    ("sESfToTXp_Meta");
    //#pragma HLS STREAM variable=sESfToTXp_Meta    depth=2

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    //
    //       +-+                                   | |
    //       |S|    1      +----------+            |S|
    //       |i| +-------->|   pESf   |----------+ |r|
    //       |n| |         +----------+          | |c|
    //       |k| |    2     --------+            | +++
    //       /|\ |  +--------> sEPt |---------+  |  |
    //       0|  |  |       --------+         |  | \|/
    //     +--+--+--+--+                   +--+--+--+--+
    //     |   pRXp    |                   |   pTXp    |
    //     +------+----+                   +-----+-----+
    //          /|\                              |
    //           |                               |
    //           |                               |
    //           |                              \|/
    //
    //-------------------------------------------------------------------------
    pRXPath(
            piSHL_MmioEchoCtrl,
            siSHL_Data,
            sRXpToTXp_Data,
            sRXpToESf_Data);

    pEchoStoreAndForward(
            sRXpToESf_Data,
            sESfToTXp_Data);

    pTXPath(
            piSHL_MmioEchoCtrl,
            sRXpToTXp_Data,
            sESfToTXp_Data,
            soSHL_Data);

}

