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
 * @param[in]  siTRIF_DataReq,   Data request from TcpRoleInterface (TRIF).
 * @param[out] soTRIF_Meta,      Metadata to TRIF.
 * @param[out] soRSt_RxSarQry,   Query to RxSarTable (RSt)
 * @param[in]  siRSt_RxSarRep,   Reply from [RSt].
 * @param[out] soMEM_RxP_RdCmd,  Rx memory read command to MEM.
 *
 * @detail
 *  This application interface is used to receive data streams of established
 *  connections. The Application polls data from the buffer by sending a
 *  readRequest. The module checks if the readRequest is valid then it sends a
 *  read request to the memory. After processing the request the MetaData
 *  containing the Session-ID is also written back.
 *
 *****************************************************************************/
void rx_app_stream_if(
        stream<AppRdReq>            &siTRIF_DataReq,
        stream<SessionId>           &soTRIF_Meta,
        stream<RAiRxSarQuery>       &soRSt_RxSarQry,
        stream<RAiRxSarReply>       &siRSt_RxSarRep,
        stream<DmCmd>               &soMEM_RxP_RdCmd)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static ap_uint<16>    rasi_readLength;
    static uint16_t       rxAppBreakTemp  = 0;
    static uint16_t       rxRdCounter = 0; // For Debug

    static enum FsmStates { S0=0, S1 } fsmState=S0;

    switch (fsmState) {
        case S0:
            if (!siTRIF_DataReq.empty() && !soRSt_RxSarQry.full()) {
                AppRdReq  appReadRequest = siTRIF_DataReq.read();
                if (appReadRequest.length != 0) {
                    // Make sure length is not 0, otherwise Data Mover will hang up
                    soRSt_RxSarQry.write(RAiRxSarQuery(appReadRequest.sessionID));
                    // Get app pointer
                    rasi_readLength = appReadRequest.length;
                    fsmState = S1;
                }
            }
            break;

        case S1:
            if (!siRSt_RxSarRep.empty() && !soTRIF_Meta.full() &&
                !soMEM_RxP_RdCmd.full() && !soRSt_RxSarQry.full()) {
                RAiRxSarReply rxSar = siRSt_RxSarRep.read();
                RxMemPtr      rxMemAddr = 0;
                rxMemAddr(29, 16) = rxSar.sessionID(13, 0);
                rxMemAddr(15,  0) = rxSar.appd;
                soTRIF_Meta.write(rxSar.sessionID);
                rxRdCounter++;
                DmCmd rxAppTempCmd = DmCmd(rxMemAddr, rasi_readLength);
                soMEM_RxP_RdCmd.write(rxAppTempCmd);
                // Update the APP read pointer
                soRSt_RxSarQry.write(RAiRxSarQuery(rxSar.sessionID, rxSar.appd+rasi_readLength));
                fsmState = S0;
            }
            break;
    }
}
