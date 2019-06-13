/*****************************************************************************
 * @file       : tcp_app_flash.cpp
 * @brief      : TCP Application Flash (TAF)
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
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
 *  directives because the new PRAGMAs do not work for us.
 *
 *****************************************************************************/

#include "tcp_app_flash.hpp"

#define USE_DEPRECATED_DIRECTIVES

/*****************************************************************************
 * @brief Echo loopback with store and forward in DDR4.
 * @ingroup tcp_app_flash
 *
 * @param[in]  siRXp_Data,   data from pRXPath (RXp)
 * @param[in]  siRXp_SessId, session Id from [RXp]
 * @param[out] soTXp_Data,   data to pTXPath (TXp).
 * @param[out] soTXp_SessId, session Id to [TXp].
 *
 * @details
 *  Loopback between the Rx and Tx ports of the TCP connection. The echo is
 *  said to operate in "store-and-forward" mode because every received segment
 *  is stored into the DDR4 memory before being read again and and sent back.
 *
 * @return Nothing.
 ******************************************************************************/
void pEchoStoreAndForward( // [TODO - Implement this process as a real store-and-forward]
        stream<AxiWord>     &siRXp_Data,
        stream<TcpSessId>   &siRXp_SessId,
        stream<AxiWord>     &soTXp_Data,
        stream<TcpSessId>   &soTXp_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL VARIABLES ------------------------------------------------------
    static stream<AxiWord>      sDataFifo ("sDataFifo");
    #pragma HLS stream variable=sDataFifo depth=2048
    static stream<TcpSessId>    sMetaFifo ("sMetaFifo");
    #pragma HLS stream variable=sDataFifo depth=64

    //-- FiFo Push ----------
    if ( !siRXp_Data.empty() && !sDataFifo.full() )
        sDataFifo.write(siRXp_Data.read());
    if ( !siRXp_SessId.empty() && !sMetaFifo.full() )
        sMetaFifo.write(siRXp_SessId.read());

    //-- FiFo Pop -----------
    if ( !sDataFifo.empty() && !soTXp_Data.full() )
        soTXp_Data.write(sDataFifo.read());
    if ( !sMetaFifo.empty() && !soTXp_SessId.full() )
        soTXp_SessId.write(sMetaFifo.read());
}


/*****************************************************************************
 * @brief Transmit Path (.i.e Data and metadata to SHELL.
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siEPt_Data,         data from EchoPassTrough (EPt).
 * @param[in]  siEPt_SessId,       metadata from [EPt].
 * @param[in]  siESf_Data,         data from EchoStoreAndForward (ESf).
 * @param[in]  siESf_SessId,       metadata from [ESf].
 * @param[out] soSHL_Data,         data to SHELL (SHL).
 * @param[out] soSHL_SessId,       metadata to [SHL].
 *
 * @return Nothing.
 *****************************************************************************/
void pTXPath(
        ap_uint<2>           piSHL_MmioEchoCtrl,
        stream<AxiWord>     &siEPt_Data,
        stream<TcpSessId>   &siEPt_SessId,
        stream<AxiWord>     &siESf_Data,
        stream<TcpSessId>   &siESf_SessId,
        stream<AxiWord>     &soSHL_Data,
        stream<TcpSessId>   &soSHL_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL VARIABLES ------------------------------------------------------
    AxiWord     tcpWord;
    TcpSessId   tcpSessId;

    static enum TxpFsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } txpFsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=txpFsmState

    switch (txpFsmState ) {
      case START_OF_SEGMENT:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            // Read session Id from pEchoPassThrough
            if ( !siEPt_SessId.empty() and !soSHL_SessId.full()) {
                siEPt_SessId.read(tcpSessId);
                soSHL_SessId.write(tcpSessId);
                txpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read session Id from pEchoStoreAndForward
            if ( !siESf_SessId.empty() and !soSHL_SessId.full()) {
                siESf_SessId.read(tcpSessId);
                soSHL_SessId.write(tcpSessId);
                txpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the session Id
            if ( !siEPt_SessId.empty() )
                siEPt_SessId.read();
            else if ( !siESf_SessId.empty() )
                siESf_SessId.read();
            txpFsmState = CONTINUATION_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
      case CONTINUATION_OF_SEGMENT:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data from pEchoPathThrough
            if ( !siEPt_Data.empty() and !soSHL_Data.full()) {
                siEPt_Data.read(tcpWord);
                soSHL_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    txpFsmState = START_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming data from pEchoStoreAndForward
            if ( !siESf_Data.empty() and !soSHL_Data.full()) {
                siESf_Data.read(tcpWord);
                soSHL_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    txpFsmState = START_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segments
            if ( !siEPt_Data.empty() )
                siEPt_Data.read(tcpWord);
            else if ( !siESf_Data.empty() )
                siESf_Data.read(tcpWord);
            // Always alternate between START and CONTINUATION to drain all streams
            txpFsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (rxpFsmState ) {
}


/*****************************************************************************
 * @brief Receive Path (.i.e Data and metadata from SHELL).
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl, configuration of the echo function.
 * @param[in]  siSHL_Data,         data from SHELL (SHL).
 * @param[in]  siSHL_SessId,       metadata from [SHL].
 * @param[out] soEPt_Data,         data to EchoPassTrough (EPt).
 * @param[out] soEPt_SessId,       metadata to [EPt].
 * @param[out] soESf_Data,         data to EchoStoreAndForward (ESf).
 * @param[out] soESF_SessId,       metadata to [ESf].
 *
 * @return Nothing.
 ******************************************************************************/
void pRXPath(
        ap_uint<2>            piSHL_MmioEchoCtrl,
        stream<AxiWord>      &siSHL_Data,
        stream<TcpSessId>    &siSHL_SessId,
        stream<AxiWord>      &soEPt_Data,
        stream<TcpSessId>    &soEPt_SessId,
        stream<AxiWord>      &soESf_Data,
        stream<TcpSessId>    &soESf_SessId)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL VARIABLES ------------------------------------------------------
    AxiWord     tcpWord;
    TcpSessId   tcpSessId;

    static enum RxpFsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } rxpFsmState = START_OF_SEGMENT;
    #pragma HLS reset variable=rxpFsmState

    switch (rxpFsmState ) {
      case START_OF_SEGMENT:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming session Id and forward to pEchoPathThrough
            if ( !siSHL_SessId.empty() and !soEPt_SessId.full()) {
                siSHL_SessId.read(tcpSessId);
                soEPt_SessId.write(tcpSessId);
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming session Id and forward to pEchoStoreAndForward
            if ( !siSHL_SessId.empty() and !soESf_SessId.full()) {
                siSHL_SessId.read(tcpSessId);
                soESf_SessId.write(tcpSessId);
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_SessId.empty() ) {
                siSHL_SessId.read(tcpSessId);
                rxpFsmState = CONTINUATION_OF_SEGMENT;
            }
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
      case CONTINUATION_OF_SEGMENT:
        switch(piSHL_MmioEchoCtrl) {
          case ECHO_PATH_THRU:
            //-- Read incoming data and forward to pEchoPathThrough
            if ( !siSHL_Data.empty() and !soEPt_Data.full()) {
                siSHL_Data.read(tcpWord);
                soEPt_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    rxpFsmState = START_OF_SEGMENT;
            }
            break;
          case ECHO_STORE_FWD:
            //-- Read incoming data and forward to pEchoStoreAndForward
            if ( !siSHL_Data.empty() and !soESf_Data.full()) {
                siSHL_Data.read(tcpWord);
                soESf_Data.write(tcpWord);
                // Update FSM state
                if (tcpWord.tlast)
                    rxpFsmState = START_OF_SEGMENT;
            }
            break;
          case ECHO_OFF:
          default:
            // Drain and drop the segment
            if ( !siSHL_Data.empty() ) {
                siSHL_Data.read(tcpWord);
            }
            // Always alternate between START and CONTINUATION to drain all streams
            rxpFsmState = START_OF_SEGMENT;
            break;
        }  // End-of: switch(piSHL_MmioEchoCtrl) {
        break;
    }  // End-of: switch (rxpFsmState ) {
}

/*****************************************************************************
 * @brief   Main process of the TCP Application Flash
 * @ingroup tcp_app_flash
 *
 * @param[in]  piSHL_MmioEchoCtrl,  configures the echo function.
 * @param[in]  piSHL_MmioPostSegEn, enables posting of a TCP segment by [MMIO].
 * @param[in]  piSHL_MmioCaptSegEn, enables capture of a TCP segment by [MMIO].
 * @param[in]  siSHL_Data,          TCP data stream from the SHELL [SHL].
 * @param[in]  siSHL_SessId,        TCP session Id from [SHL].
 * @param[out] soSHL_Data           TCP data stream to [SHL].
 *
 * @return Nothing.
 *****************************************************************************/
void tcp_app_flash (

        //------------------------------------------------------
        //-- SHELL / MMIO / Configuration Interfaces
        //------------------------------------------------------
        ap_uint<2>          piSHL_MmioEchoCtrl,
        //[TODO] ap_uint<1> piSHL_MmioPostSegEn,
        //[TODO] ap_uint<1> piSHL_MmioCaptSegEn,

        //------------------------------------------------------
        //-- SHELL / TCP Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siSHL_Data,
        stream<AppMeta>     &siSHL_SessId,

        //------------------------------------------------------
        //-- SHELL / TCP Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soSHL_Data,
        stream<AppMeta>     &soSHL_SessId)
{

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable    port=piSHL_MmioEchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable     port=piSHL_MmioCaptSegEn

    #pragma HLS resource core=AXI4Stream variable=siSHL_Data   metadata="-bus_bundle siSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=siSHL_SessId metadata="-bus_bundle siSHL_SessId"
    #pragma HLS resource core=AXI4Stream variable=soSHL_Data   metadata="-bus_bundle soSHL_Data"
    #pragma HLS resource core=AXI4Stream variable=soSHL_SessId metadata="-bus_bundle soSHL_SessId"

#else

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable          port=piSHL_MmioEchoCtrl
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_MmioPostSegEn
    //[TODO] #pragma HLS INTERFACE ap_stable port=piSHL_MmioCapSegtEn

    // [INFO] Always add "register off" because (default value is "both")
    #pragma HLS INTERFACE axis register off port=siSHL_Data
    #pragma HLS INTERFACE axis register off port=siSHL_SessId
    #pragma HLS INTERFACE axis register off port=soSHL_Data
    #pragma HLS INTERFACE axis register off port=soSHL_SessId

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)

    //-------------------------------------------------------------------------
    //-- Rx Path (RXp)
    //-------------------------------------------------------------------------
    static stream<AxiWord>      sRXpToTXp_Data    ("sRXpToTXp_Data");
    #pragma HLS STREAM variable=sRXpToTXp_Data    depth=2048
    static stream<TcpSessId>    sRXpToTXp_SessId  ("sRXpToTXp_SessId");
    #pragma HLS STREAM variable=sRXpToTXp_SessId  depth=64

    static stream<AxiWord>      sRXpToESf_Data    ("sRXpToESf_Data");
    #pragma HLS STREAM variable=sRXpToESf_Data    depth=2
    static stream<TcpSessId>    sRXpToESf_SessId  ("sRXpToESf_SessId");
    #pragma HLS STREAM variable=sRXpToESf_SessId  depth=2

    //-------------------------------------------------------------------------
    //-- Echo Store and Forward (ESf)
    //-------------------------------------------------------------------------
    static stream<AxiWord>      sESfToTXp_Data    ("sESfToTXp_Data");
    #pragma HLS STREAM variable=sESfToTXp_Data    depth=2
    static stream<TcpSessId>    sESfToTXp_SessId  ("sESfToTXp_SessId");
    #pragma HLS STREAM variable=sESfToTXp_SessId  depth=2

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pRXPath(
            piSHL_MmioEchoCtrl,
            siSHL_Data,
            siSHL_SessId,
            sRXpToTXp_Data,
            sRXpToTXp_SessId,
            sRXpToESf_Data,
            sRXpToESf_SessId);

    pEchoStoreAndForward(
            sRXpToESf_Data,
            sRXpToESf_SessId,
            sESfToTXp_Data,
            sESfToTXp_SessId);

    pTXPath(
            piSHL_MmioEchoCtrl,
            sRXpToTXp_Data,
            sRXpToTXp_SessId,
            sESfToTXp_Data,
            sESfToTXp_SessId,
            soSHL_Data,
            soSHL_SessId);

}

