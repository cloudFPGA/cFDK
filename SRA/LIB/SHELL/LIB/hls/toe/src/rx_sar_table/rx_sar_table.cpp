/*****************************************************************************
 * @file       : rx_sar_table.cpp
 * @brief      : Rx Segmentation And Re-assembly Table (RSt).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "rx_sar_table.hpp"

using namespace hls;

#define THIS_NAME "TOE/RSt"


/*****************************************************************************
 * @brief Rx SAR Table (RSt) - Stores the data structures for managing the
 *         TCP Rx buffer and the Rx sliding window.
 *
 * @param[in]  siRXe_RxSarQry,    Query from RxEngine (RXe).
 * @param[out] soRXe_RxSarRep,    Reply to RXe.
 * @param[in]  siRAi_RxSarQry,    Query from RxAppInterface (RAi).
 * @param[out] soRAi_RxSarRep,    Reply to [RAi].
 * @param[in]  siTXe_RxSarReq,    Read request from TxEngine (TXe).
 * @param[out] soTxe_RxSarRep,    Read reply to [TXe].
 *
 * @details
 *  This process is concurrently accessed by the RxEngine (RXe), the
 *   RxApplicationInterface (RAi) and the TxEngine (TXe).
 *  The access by the TXe is read-only.
 *  
 *****************************************************************************/
void rx_sar_table(
        stream<RXeRxSarQuery>      &siRXe_RxSarQry,
        stream<RxSarEntry>         &soRXe_RxSarRep,
        stream<RAiRxSarQuery>      &siRAi_RxSarQry,
        stream<RAiRxSarReply>      &soRAi_RxSarRep,
        stream<SessionId>          &siTXe_RxSarReq, // Read only
        stream<RxSarEntry>         &soTxe_RxSarRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static RxSarEntry               RX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RX_SAR_TABLE core=RAM_1P_BRAM
    #pragma HLS DEPENDENCE variable=RX_SAR_TABLE inter false

    SessionId           sessId;
    RXeRxSarQuery         in_recvd;
    RAiRxSarQuery         in_appd;

    if(!siTXe_RxSarReq.empty()) {
        // Read only access from the Tx Engine
        siTXe_RxSarReq.read(sessId);
        soTxe_RxSarRep.write(RX_SAR_TABLE[sessId]);
    }
    else if(!siRAi_RxSarQry.empty()) {
        // Read or Write access from the Rx App I/F to update the application pointer
        siRAi_RxSarQry.read(in_appd);
        if(in_appd.write)
            RX_SAR_TABLE[in_appd.sessionID].appd = in_appd.appd;
        else
            soRAi_RxSarRep.write(RAiRxSarReply(in_appd.sessionID, RX_SAR_TABLE[in_appd.sessionID].appd));
    }
    else if(!siRXe_RxSarQry.empty()) {
        // Read or Write access from the RxEngine
        siRXe_RxSarQry.read(in_recvd);
        if (in_recvd.write) {
            RX_SAR_TABLE[in_recvd.sessionID].recvd = in_recvd.recvd;
            if (in_recvd.init)
                RX_SAR_TABLE[in_recvd.sessionID].appd = in_recvd.recvd;
        }
        else
            soRXe_RxSarRep.write(RX_SAR_TABLE[in_recvd.sessionID]);
    }
}
