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
 * @param[in]  siRXe_RxSarUpdReq, Update request from Rx Engine (RXe).
 * @param[in]  siRAi_RxSarUpdReq, Update request from Rx App. I/F (RAi).
 * @param[in]  siTXe_RxSarRdReq,  Read request from Tx Engine (TXe).
 * @param[out] soRXe_RxSarUpdRep, Update reply to RXe.
 * @param[out] soRAi_RxSarUpdReq, Update reply to RAi.
 * @param[out] soTxe_RxSarRdRep,  Read reply to TXe.
 *
 * @details
 *  This process is concurrently accessed by the Rx Engine (RXe), the Rx
 *   Application Interface (RAi) and the Tx Engine (TXe).
 *  The access by the TXe id read-only.
 *  
 *****************************************************************************/
void rx_sar_table(
        stream<rxSarRecvd>         &siRXe_RxSarUpdReq,
        stream<rxSarAppd>          &siRAi_RxSarUpdReq,
        stream<ap_uint<16> >       &siTXe_RxSarRdReq, // Read only
        stream<RxSarEntry>         &soRXe_RxSarUpdRep,
        stream<rxSarAppd>          &soRAi_RxSarUpdReq,
        stream<RxSarEntry>         &soTxe_RxSarRdRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static RxSarEntry               RX_SAR_TABLE[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=RX_SAR_TABLE core=RAM_1P_BRAM
    #pragma HLS DEPENDENCE variable=RX_SAR_TABLE inter false

    ap_uint<16>         addr;
    rxSarRecvd          in_recvd;
    rxSarAppd           in_appd;

    if(!siTXe_RxSarRdReq.empty()) {
        // Read only access from the Tx Engine
        siTXe_RxSarRdReq.read(addr);
        soTxe_RxSarRdRep.write(RX_SAR_TABLE[addr]);
    }
    else if(!siRAi_RxSarUpdReq.empty()) {
        // Read or Write access from the Rx App I/F to update the application pointer
        siRAi_RxSarUpdReq.read(in_appd);
        if(in_appd.write)
            RX_SAR_TABLE[in_appd.sessionID].appd = in_appd.appd;
        else
            soRAi_RxSarUpdReq.write(rxSarAppd(in_appd.sessionID, RX_SAR_TABLE[in_appd.sessionID].appd));
    }
    else if(!siRXe_RxSarUpdReq.empty()) {
        // Read or Write access from the Rx Engine
        siRXe_RxSarUpdReq.read(in_recvd);
        if (in_recvd.write) {
            RX_SAR_TABLE[in_recvd.sessionID].recvd = in_recvd.recvd;
            if (in_recvd.init)
                RX_SAR_TABLE[in_recvd.sessionID].appd = in_recvd.recvd;
        }
        else
            soRXe_RxSarUpdRep.write(RX_SAR_TABLE[in_recvd.sessionID]);
    }
}
