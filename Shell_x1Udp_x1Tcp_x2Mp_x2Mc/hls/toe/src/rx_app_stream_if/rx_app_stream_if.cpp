/*****************************************************************************
 * @file       : rx_app_stream_if.cpp
 * @brief      : Rx Application Stream Interface (RAsi) of the TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "rx_app_stream_if.hpp"

using namespace hls;

/*****************************************************************************
 * @brief The Rx Application Stream Interface (RAsi) retrieves the data of an
 *         established connection from the buffer memory and forwards it to
 *         the application.
 *
 * @param[in]      appRxDataReq
 * @param[in]      rxSar2rxApp_upd_rsp
 * @param[out]     appRxDataRspMetadata
 * @param[out]     rxApp2rxSar_upd_req
 * @param[out]     rxBufferReadCmd
 *
 * @detail
 *  This application interface is used to receive data streams of established
 *  connections. The Application polls data from the buffer by sending a
 *  readRequest. The module checks if the readRequest is valid then it sends a
 *  read request to the memory. After processing the request the MetaData
 *  containing the Session-ID is also written back.
 *
 * @ingroup rx_app_stream_if
 *****************************************************************************/
void rx_app_stream_if(
        stream<appReadRequest>      &siTRIF_DataReq,
        stream<rxSarAppd>           &rxSar2rxApp_upd_rsp,
        stream<SessionId>           &appRxDataRspMetadata,
        stream<rxSarAppd>           &rxApp2rxSar_upd_req,
        stream<DmCmd>               &rxBufferReadCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static ap_uint<16>              rasi_readLength;
    static ap_uint<2>               rasi_fsmState   = 0;
    static uint16_t                 rxAppBreakTemp  = 0;
    static uint16_t                 rxRdCounter = 0;

    switch (rasi_fsmState) {
        case 0:
            if (!siTRIF_DataReq.empty() && !rxApp2rxSar_upd_req.full()) {
                appReadRequest  app_read_request = siTRIF_DataReq.read();
                if (app_read_request.length != 0) {
                    // Make sure length is not 0, otherwise Data Mover will hang up
                    rxApp2rxSar_upd_req.write(rxSarAppd(app_read_request.sessionID));
                    // Get app pointer
                    rasi_readLength = app_read_request.length;
                    rasi_fsmState = 1;
                }
            }
            break;

        case 1:
            if (!rxSar2rxApp_upd_rsp.empty() && !appRxDataRspMetadata.full() &&
                !rxBufferReadCmd.full() && !rxApp2rxSar_upd_req.full()) {
                rxSarAppd   rxSar = rxSar2rxApp_upd_rsp.read();
                ap_uint<32> pkgAddr = 0;
                pkgAddr(29, 16) = rxSar.sessionID(13, 0);
                pkgAddr(15,  0) = rxSar.appd;
                appRxDataRspMetadata.write(rxSar.sessionID);
                rxRdCounter++;
                DmCmd rxAppTempCmd = DmCmd(pkgAddr, rasi_readLength);
                rxBufferReadCmd.write(rxAppTempCmd);
                rxApp2rxSar_upd_req.write(rxSarAppd(rxSar.sessionID, rxSar.appd+rasi_readLength)); // Update app read pointer
                rasi_fsmState = 0;
            }
            break;
    }
}
