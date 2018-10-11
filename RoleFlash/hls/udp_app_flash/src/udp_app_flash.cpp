/*****************************************************************************
 * @file       : udp_app_flash.cpp
 * @brief      : UDP Application Flash (UAF)
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
 * @details    : This application implements a set of UDP-oriented tests and
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

#include "udp_app_flash.hpp"

#define USE_DEPRECATED_DIRECTIVES

#define MTU		1500	// Maximum Transmission Unit in bytes [TODO:Move to a common place]

/*****************************************************************************
 * @brief Update the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_app_flash
 *
 * @param[in]     axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen, a pointer to the payload length of the corresponding
 *                     AXI4-Stream.
 * @return Nothing.
 ******************************************************************************/
//void updatePayloadLength(UdpWord *axisChunk, UdpPLen *pldLen) {
//    if (axisChunk->tlast) {
//        int bytCnt = 0;
//        for (int i = 0; i < 8; i++) {
//            if (axisChunk->tkeep.bit(i) == 1) {
//                bytCnt++;
//            }
//        }
//        *pldLen += bytCnt;
//    } else
//        *pldLen += 8;
//}


/*****************************************************************************
 * @brief TxPath - Enqueue data onto a FiFo stream while computing the
 *         payload length of incoming frame on the fly.
 * @ingroup udp_app_flash
 *
 * @param[in]  siROLE_Data, the data stream to enqueue.
 * @param[out] soFifo_Data, the FiFo stream to write.
 * @param[out] soPLen,      the length of the enqueued payload.
 *
 * @return Nothing.
 *****************************************************************************/
// void pTxP_Enqueue (
//        stream<UdpWord>    &siROLE_Data,
//        stream<UdpWord>    &soFifo_Data,
//        stream<UdpPLen>    &soPLen)
//{
//    //-- LOCAL VARIABLES ------------------------------------------------------
//    static UdpPLen    pldLen;
//
//    static enum FsmState {FSM_RST=0, FSM_ACC, FSM_LAST_ACC} fsmState;
//
//    switch(fsmState) {
//
//        case FSM_RST:
//            pldLen = 0;
//            fsmState = FSM_ACC;
//            break;
//
//        case FSM_ACC:
//
//            // Default stream handling
//            if ( !siROLE_Data.empty() && !soFifo_Data.full() ) {
//
//                // Read incoming data chunk
//                UdpWord aWord = siROLE_Data.read();
//
//                // Increment the payload length
//                updatePayloadLength(&aWord, &pldLen);
//
//                // Enqueue data chunk into FiFo
//                soFifo_Data.write(aWord);
//
//                // Until LAST bit is set
//                if (aWord.tlast)
//                    fsmState = FSM_LAST_ACC;
//            }
//            break;
//
//        case FSM_LAST_ACC:
//
//            // Forward the payload length
//            soPLen.write(pldLen);
//
//            // Reset the payload length
//            pldLen = 0;
//
//            // Start over
//            fsmState = FSM_ACC;
//
//            break;
//    }
//}


/*****************************************************************************
 * @brief TxPath - Dequeue data from a FiFo stream and forward the metadata
 *         when the LAST bit is set.
 * @ingroup udp_app_flash
 *
 * @param[in]  siFifo, the FiFo stream to read from.
 * @param[in]  siMeta, the socket pair information provided by the Rx path.
 * @param[in]  siPLen, the length of the enqueued payload.
 * @param[out] soUDMX_Data, the output data stream.
 * @param[out] soUDMX_Meta, the output metadata stream.
 * @param[out] soUDMX_PLen, the output payload length.
 *
 * @return Nothing.
 *****************************************************************************/
//void pTxP_Dequeue (
//        stream<UdpWord>  &siFifo,
//        stream<UdpMeta>  &siMeta,
//        stream<UdpPLen>  &siPLen,
//        stream<UdpWord>  &soUDMX_Data,
//        stream<UdpMeta>  &soUDMX_Meta,
//        stream<UdpPLen>  &soUDMX_PLen)
//{
//    //-- LOCAL VARIABLES ------------------------------------------------------
//    static UdpMeta txSocketPairReg;
//
//    static enum FsmState {FSM_W8FORMETA=0, FSM_FIRST_ACC, FSM_ACC} fsmState;
//
//    switch(fsmState) {
//
//        case FSM_W8FORMETA:
//
//            // The very first time, wait until the Rx path provides us with the
//            // socketPair information before continuing
//            if ( !siMeta.empty() ) {
//                txSocketPairReg = siMeta.read();
//                fsmState = FSM_FIRST_ACC;
//            }
//            break;
//
//        case FSM_FIRST_ACC:
//
//            // Send out the first data together with the metadata and payload length information
//            if ( !siFifo.empty() && !siPLen.empty() ) {
//                if ( !soUDMX_Data.full() && !soUDMX_Meta.full() && !soUDMX_PLen.full() ) {
//                    // Forward data chunk, metadata and payload length
//                    UdpWord    aWord = siFifo.read();
//                    if (!aWord.tlast) {
//                        soUDMX_Data.write(aWord);
//                        soUDMX_Meta.write(txSocketPairReg);
//                        soUDMX_PLen.write(siPLen.read());
//                        fsmState = FSM_ACC;
//                    }
//                }
//            }
//
//            // Always drain the 'siMeta' stream to avoid any blocking on the Rx side
//            if ( !siMeta.empty() )
//                txSocketPairReg = siMeta.read();
//
//            break;
//
//        case FSM_ACC:
//
//            // Default stream handling
//            if ( !siFifo.empty() && !soUDMX_Data.full() ) {
//                // Forward data chunk
//                UdpWord    aWord = siFifo.read();
//                soUDMX_Data.write(aWord);
//                // Until LAST bit is set
//                if (aWord.tlast)
//                    fsmState = FSM_FIRST_ACC;
//            }
//
//            // Always drain the 'siMeta' stream to avoid any blocking on the Rx side
//            if ( !siMeta.empty() )
//                txSocketPairReg = siMeta.read();
//
//            break;
//    }
//}


/*****************************************************************************
 * @brief Echo loopback between the Rx and Tx ports of the UDP connection.
 *         The echo is said to operate in "pass-through" mode because every
 *         received packet is sent back without being stored by this process.
 * @ingroup udp_app_flash
 *
 * @param[in]  siRxp_Data,	data from pRXPath
 * @param[in]  soTxp_Data,	data to pTXPath.
 *
 * @return Nothing.
 ******************************************************************************/
/*** OBSOLETE ***
void pEchoPassThrough(
		stream<UdpWord>		&siRxp_Data,
		stream<UdpWord>     &soTxp_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	static stream<UdpWord>	sFifo ("sFifo");
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
 * @brief Echo loopback between the Rx and Tx ports of the UDP connection.
 *         The echo is said to operate in "store-and-forward" mode because
 *         every received packet is stored into the DDR4 memory before being
 *         read again and and sent back.
 * @ingroup udp_app_flash
 *
 * @param[in]  siRxp_Data,	data from pRXPath
 * @param[in]  soTxp_Data,	data to pTXPath.
 *
 * @return Nothing.
 ******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as store-and-forward]
		stream<UdpWord>		&siRxp_Data,
		stream<UdpWord>     &soTxp_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	static stream<UdpWord>	sFifo ("sFifo");
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
 * @ingroup udp_app_flash
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
        stream<UdpWord>     &siEpt_Data,
		stream<UdpWord>     &siEsf_Data,
		stream<UdpWord>     &soSHL_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	UdpWord	udpWord;

   	//-- Forward incoming chunk to SHELL
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
        	// Read data chunk from pEchoPassThrough
        	if ( !siEpt_Data.empty() ) {
        		udpWord = siEpt_Data.read();
        	}
        	else
        		return;
        	break;

        case ECHO_STORE_FWD:
        	// Read data chunk from pEchoStoreAndForward
        	if ( !siEsf_Data.empty() )
        		udpWord = siEsf_Data.read();
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
 * @ingroup udp_app_flash
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
        stream<UdpWord>      &siSHL_Data,
        stream<UdpWord>      &soEpt_Data,
		stream<UdpWord>      &soEsf_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    //-- LOCAL VARIABLES ------------------------------------------------------
	UdpWord	udpWord;

    //-- Read incoming data chunk
    if ( !siSHL_Data.empty() )
    	udpWord = siSHL_Data.read();
    else
    	return;

   	// Forward data chunk to Echo function
    switch(piSHL_MmioEchoCtrl) {

        case ECHO_PATH_THRU:
        	// Forward data chunk to pEchoPathThrough
        	if ( !soEpt_Data.full() )
        		soEpt_Data.write(udpWord);
            break;

        case ECHO_STORE_FWD:
        	// Forward data chunk to pEchoStoreAndForward
        	if ( !soEsf_Data.full() )
        		soEsf_Data.write(udpWord);
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
 * @param[in]  piSHL_This_MmioEchoCtrl,  configures the echo function.
 * @param[in]  piSHL_This_MmioPostPktEn, enables posting of UDP packets.
 * @param[in]  piSHL_This_MmioCaptPktEn, enables capture of UDP packets.
 * @param[in]  siSHL_This_Data           UDP data stream from the SHELL.
 * @param[out] soTHIS_Shl_Data           UDP data stream to the SHELL.
 *
 * @return Nothing.
 *****************************************************************************/
void udp_app_flash (

        //------------------------------------------------------
        //-- SHELL / This / Mmio / Config Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_This_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_This_MmioPostPktEn,
        //[TODO] ap_uint<1> piSHL_This_MmioCaptPktEn,

        //------------------------------------------------------
        //-- SHELL / This / Udp Interfaces
        //------------------------------------------------------
        stream<UdpWord>     &siSHL_This_Data,
        stream<UdpWord>     &soTHIS_Shl_Data)
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
    //OBSOLETE-20180918 static stream<UdpWord>      sRxpToEpt_Data("sRxpToEpt_Data");
	//OBSOLETE-20180918 static stream<UdpWord>      sEptToTxp_Data("sEptToTxp_Data");

	static stream<UdpWord>      sRxpToTxp_Data("sRxpToTxP_Data");

    static stream<UdpWord>      sRxpToEsf_Data("sRxpToEsf_Data");
    static stream<UdpWord>      sEsfToTxp_Data("sEsfToTxp_Data");

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

