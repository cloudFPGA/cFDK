/*****************************************************************************
 * @file       : rx_sar_table.cpp
 * @brief      : Rx Segment And reassemble Table (RSt) of the TCP Offload Engine (TOE)
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

#define THIS_NAME "TOE/RSt"

using namespace hls;

#define DEBUG_LEVEL 1


/*****************************************************************************
 * @brief The Rx SAR Table (RSt) stores the This data structure stores the Rx
 *         sliding window information.
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
 * @ingroup rx_sar_table
 *****************************************************************************/
void rx_sar_table(
        stream<rxSarRecvd>         &siRXe_RxSarUpdReq,
        stream<rxSarAppd>          &siRAi_RxSarUpdReq,
        stream<ap_uint<16> >       &siTXe_RxSarRdReq, // Read only
        stream<rxSarEntry>         &soRXe_RxSarUpdRep,
        stream<rxSarAppd>          &soRAi_RxSarUpdReq,
        stream<rxSarEntry>         &soTxe_RxSarRdRep)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static rxSarEntry               rx_table[MAX_SESSIONS];
    #pragma HLS RESOURCE   variable=rx_table core=RAM_1P_BRAM
    #pragma HLS DEPENDENCE variable=rx_table inter false

    ap_uint<16>         addr;
    rxSarRecvd          in_recvd;
    rxSarAppd           in_appd;

    if(!siTXe_RxSarRdReq.empty()) {
        // Read only access from the Tx Engine
        siTXe_RxSarRdReq.read(addr);
        soTxe_RxSarRdRep.write(rx_table[addr]);
    }
    else if(!siRAi_RxSarUpdReq.empty()) {
        // Read or Write access from the Rx App I/F to update the application pointer
        siRAi_RxSarUpdReq.read(in_appd);
        if(in_appd.write)
            rx_table[in_appd.sessionID].appd = in_appd.appd;
        else
            soRAi_RxSarUpdReq.write(rxSarAppd(in_appd.sessionID, rx_table[in_appd.sessionID].appd));
    }
    else if(!siRXe_RxSarUpdReq.empty()) {
        // Read or Write access from the Rx Engine
        siRXe_RxSarUpdReq.read(in_recvd);
        if (in_recvd.write) {
            rx_table[in_recvd.sessionID].recvd = in_recvd.recvd;
            if (in_recvd.init)
                rx_table[in_recvd.sessionID].appd = in_recvd.recvd;
        }
        else
            soRXe_RxSarUpdRep.write(rx_table[in_recvd.sessionID]);
    }
}
