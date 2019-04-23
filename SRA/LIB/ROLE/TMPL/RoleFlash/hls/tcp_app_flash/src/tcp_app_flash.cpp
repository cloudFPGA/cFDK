/*****************************************************************************
 * @file       : tcp_app_flash.cpp
 * @brief      : TCP Application Flash (TAF)
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This application implements a set of TCP-oriented tests and
 *  functions which are embedded into the Flash of the cloudFPGA role.
 *
 * @note       : For the time being, we continue designing with the DEPRECATED
 *				  directives because the new PRAGMAs do not work for us.
 * @remark     :
 * @warning    :
 *
 * @see        :
 *
 *****************************************************************************/

#include "tcp_app_flash.hpp"

#define USE_DEPRECATED_DIRECTIVES

#define MTU		1500	// Maximum Transmission Unit in bytes [TODO:Move to a common place]


/*****************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the TCP connection.
 *         The echo is said to operate in "pass-through" mode because every
 *         received packet is sent back without being stored by this process.
 * @ingroup tcp_app_flash
 *
 * @param[in]  siRxp_Data,	data from pRXPath
 * @param[in]  soTxp_Data,	data to pTXPath.
 *
 * @return Nothing.
 ******************************************************************************/
/*** OBSOLETE ***
void pEchoPassThrough(
		stream<TcpWord>		&siRxp_Data,
		stream<TcpWord>     &soTxp_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	static stream<TcpWord>	sFifo ("sFifo");
	#pragma HLS stream      variable=sFifo depth=MTU

	//-- FiFo Push
	if ( !siRxp_Data.empty() && !sFifo.full() )
		sFifo.write(siRxp_Data.read());

	//-- FiFo Pop
	if ( !sFifo.empty() && !soTxp_Data.full() )
		soTxp_Data.write(sFifo.read());
}
*****/


/*****************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the TCP connection.
 *         The echo is said to operate in "store-and-forward" mode because
 *         every received packet is stored into the DDR4 memory before being
 *         read again and and sent back.
 * @ingroup tcp_app_flash
 *
 * @param[in]  siRxp_Data,	data from pRXPath
 * @param[in]  soTxp_Data,	data to pTXPath.
 *
 * @return Nothing.
 ******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as store-and-forward]
		stream<TcpWord>		&siRxp_Data,
		stream<TcpWord>     &soTxp_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	static stream<TcpWord>	sFifo ("sFifo");
	#pragma HLS stream      variable=sFifo depth=8

	//-- FiFo Push
	if ( !siRxp_Data.empty() && !sFifo.full() )
		sFifo.write(siRxp_Data.read());

	//-- FiFo Pop
	if ( !sFifo.empty() && !soTxp_Data.full() )
		soTxp_Data.write(sFifo.read());
}


/*****************************************************************************
 * @brief Transmit Path - From THIS to SHELL.
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siEpt_Data,         data from pEchoPassTrough.
 * @param[in]  siEsf_Data,         data from pEchoStoreAndForward.
 * @param[out] soSHL_Data,         data to SHELL.
 *
 * @return Nothing.
 *****************************************************************************/
void pTXPath(
		ap_uint<2>           piSHL_MmioEchoCtrl,
        stream<TcpWord>     &siEpt_Data,
		stream<TcpWord>     &siEsf_Data,
		stream<TcpWord>     &soSHL_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	TcpWord	tcpWord;

   	//-- Forward incoming chunk to SHELL
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
        	// Read data chunk from pEchoPassThrough
        	if ( !siEpt_Data.empty() ) {
        		tcpWord = siEpt_Data.read();
        	}
        	else
        		return;
        	break;

        case ECHO_STORE_FWD:
        	// Read data chunk from pEchoStoreAndForward
        	if ( !siEsf_Data.empty() )
        		tcpWord = siEsf_Data.read();
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
        soSHL_Data.write(tcpWord);
}


/*****************************************************************************
 * @brief Receive Path - From SHELL to THIS.
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siSHL_Data,         data from SHELL.
 * @param[out] soEpt_Data,         data to pEchoPassTrough.
 * @param[out] soEsf_Data,         data to pEchoStoreAndForward.
 *
 * @return Nothing.
 ******************************************************************************/
void pRXPath(
        ap_uint<2>            piSHL_MmioEchoCtrl,
        stream<TcpWord>      &siSHL_Data,
        stream<TcpWord>      &soEpt_Data,
		stream<TcpWord>      &soEsf_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	TcpWord	tcpWord;

    //-- Read incoming data chunk
    if ( !siSHL_Data.empty() )
    	tcpWord = siSHL_Data.read();
    else
    	return;

   	// Forward data chunk to Echo function
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
        	// Forward data chunk to pEchoPathThrough
        	if ( !soEpt_Data.full() )
        		soEpt_Data.write(tcpWord);
            break;

        case ECHO_STORE_FWD:
        	// Forward data chunk to pEchoStoreAndForward
        	if ( !soEsf_Data.full() )
        		soEsf_Data.write(tcpWord);
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
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_This_MmioEchoCtrl,  configures the echo function.
 * @param[in]  piSHL_This_MmioPostPktEn, enables posting of UDP packets.
 * @param[in]  piSHL_This_MmioCaptPktEn, enables capture of UDP packets.
 * @param[in]  siSHL_This_Data           UDP data stream from the SHELL.
 * @param[out] soTHIS_Shl_Data           UDP data stream to the SHELL.
 *
 * @return Nothing.
 *****************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / This / Mmio / Config Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_This_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_This_MmioPostPktEn,
        //[TODO] ap_uint<1> piSHL_This_MmioCaptPktEn,

        //------------------------------------------------------
        //-- SHELL / This / Udp Interfaces
        //------------------------------------------------------
        stream<TcpWord>     &siSHL_This_Data,
        stream<TcpWord>     &soTHIS_Shl_Data)
{

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

	//-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
	#pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable 	 port=piSHL_This_MmioEchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable 	 port=piSHL_This_MmioPostPktEn
	//[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_This_MmioCaptPktEn

    #pragma HLS resource core=AXI4Stream variable=siSHL_This_Data metadata="-bus_bundle siSHL_This_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Shl_Data metadata="-bus_bundle soTHIS_Shl_Data"

#else

	//-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
	#pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable     port=piSHL_This_MmioEchoCtrl
	//[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_This_MmioPostPktEn
	//[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_This_MmioCaptPktEn

	// [INFO] Always add "register off" because (default value is "both")
    #pragma HLS INTERFACE axis register off port=siSHL_This_Data
    #pragma HLS INTERFACE axis register off port=soTHIS_Shl_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL STREAMS --------------------------------------------------------
    //OBSOLETE-20180918 static stream<TcpWord>      sRxpToEpt_Data("sRxpToEpt_Data");
	//OBSOLETE-20180918 static stream<TcpWord>      sEptToTxp_Data("sEptToTxp_Data");

	static stream<TcpWord>      sRxpToTxp_Data("sRxpToTxP_Data");

    static stream<TcpWord>      sRxpToEsf_Data("sRxpToEsf_Data");
    static stream<TcpWord>      sEsfToTxp_Data("sEsfToTxp_Data");

    //OBSOLETE-20180918 #pragma HLS STREAM    variable=sRxpToEpt_Data depth=512
    //OBSOLETE-20180918 #pragma HLS STREAM    variable=sEptToTxp_Data depth=512

    #pragma HLS STREAM variable=sRxpToTxp_Data off depth=1500
	#pragma HLS STREAM variable=sRxpToEsf_Data depth=2
    #pragma HLS STREAM variable=sEsfToTxp_Data depth=2

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pRXPath(piSHL_This_MmioEchoCtrl,
            siSHL_This_Data, sRxpToTxp_Data, sRxpToEsf_Data);

    //OBSOLETE-20180918 pEchoPassThrough(sRxpToEpt_Data, sEptToTxp_Data);

    pEchoStoreAndForward(sRxpToEsf_Data, sEsfToTxp_Data);

    pTXPath(piSHL_This_MmioEchoCtrl,
    		sRxpToTxp_Data, sEsfToTxp_Data, soTHIS_Shl_Data);

}

