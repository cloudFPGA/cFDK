/*****************************************************************************
 * @file       : udp_role_if.cpp
 * @brief      : UDP Role Interface
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This process exposes the UDP data path to the role.
 *
 * @note       : { text }
 * @remark     : { remark text }
 * @warning    : { warning message }
 * @todo       : { paragraph describing what is to be done }
 *
 * @see        : https://www.stack.nl/~dimitri/doxygen/manual/commands.html
 *
 *****************************************************************************/

#include "udp_role_if.hpp"


/*****************************************************************************
 * @brief Receive path from UDP core to ROLE.
 * @ingroup udp_role_if
 *
 * @param[in]  siUdp_Data, data from UDP core.
 * @param[in]  siUdp_Meta, meta-data from UDP core.
 * @param[out] soUdp_OpnReq, open port request to UDP core.
 * @param[in]  siUdp_OpnAck, open port acknowledgment from UDP core.
 * @param[out] soRol_Data, data to ROLE.
 * @param[out] soTxP_Len, UDP packet length to TxPath process.
 * @param[out] soTxP_Meta, UDP meta-data to TxPath process.
 *
 * @return Nothing.
 ******************************************************************************/
void pRxPath(
        stream<Axis<64> >    &siUdp_Data,
        stream<Metadata>     &siUdp_Meta,
        stream<Axis<16> >    &soUdp_OpnReq,
        stream<Axis<1> >     &siUdp_OpnAck,
        stream<Axis<64> >    &soRol_Data,
        stream<Axis<16> >    &soTxP_Len,
        stream<Metadata>     &soTxP_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    static enum FsmState {FSM_IDLE = 0, FSM_W8FORPORT, FSM_ACC_FIRST, FSM_ACC} fsmState;

    static uint16_t             lbPacketLength       = 0;
    static ap_uint<8>           openPortWaitTime     = 10;

    switch(fsmState) {

        case FSM_IDLE:
            if ( !soUdp_OpnReq.full() && openPortWaitTime == 0 ) {
                // Request to open UDP port 80 -- [FIXME - Port# should be a parameter]
                soUdp_OpnReq.write(Axis<16>(0x50));
                fsmState = FSM_W8FORPORT;
            }
            else 
                openPortWaitTime--;
            break;
        case FSM_W8FORPORT:
            if ( !siUdp_OpnAck.empty() ) {
                // Read the acknowledgment
                Axis<1> sOpenAck = siUdp_OpnAck.read();
                fsmState = FSM_ACC_FIRST;
            }
            break;
        case FSM_ACC_FIRST:
            if ( !siUdp_Data.empty() && !soRol_Data.full() &&
                  !siUdp_Meta.empty() && !soTxP_Meta.full()) {

                // Swap the source and destination socket addresses and forward result to pTxPath
                Metadata    tempMetadata   = siUdp_Meta.read();
                SocketAddr    tempSockAddr   = tempMetadata.src;
                tempMetadata.src = tempMetadata.dst;
                tempMetadata.dst = tempSockAddr;
                soTxP_Meta.write(tempMetadata);

                // Forward data chunk to ROLE and update byte counter
                Axis<64>  tmpAxis64 = siUdp_Data.read();
                soRol_Data.write(tmpAxis64);
                ap_uint<4> counter = 0;
                for (uint8_t i=0; i<8; ++i) {
                    if (tmpAxis64.tkeep.bit(i) == 1)
                        counter++;
                }
                lbPacketLength += counter;

                // Forward the packet length to pTxPath
                if (tmpAxis64.tlast) {
                    soTxP_Len.write(Axis<16>(lbPacketLength));
                    lbPacketLength = 0;
                }
                else
                    fsmState = FSM_ACC;
            }
            break;
        case FSM_ACC:
            if ( !siUdp_Data.empty() && !soRol_Data.full() ) {

                // Forward data chunk to ROLE and update byte counter
                Axis<64>  tempWord = siUdp_Data.read();
                soRol_Data.write(tempWord);
                ap_uint<4> counter = 0;
                for (uint8_t i=0; i<8; ++i) {
                    if (tempWord.tkeep.bit(i) == 1)
                        counter++;
                }

                // Forward the packet length to pTxPath
                lbPacketLength += counter;
                if (tempWord.tlast) {
                    soTxP_Len.write(Axis<16>(lbPacketLength));
                    lbPacketLength = 0;
                    fsmState = FSM_ACC_FIRST;
                }
            }
            break;
    }
}


/*****************************************************************************
 * @brief Transmit path from ROLE to UDP core.
 * @ingroup udp_role_if
 *
 * @param[in]  siRol_Data,  data from ROLE.
 * @param[in]  siRxP_Len,   UDP packet length from RxPath process.
 * @param[in]  siRxP_Meta,  UDP meta-data from RxPath process.
 * @param[out] soUdp_Data,  data to UDP core.
 * @param[out] soUdmx_Meta, UDP meta-data to UDP core.
 * @param[out] soUdmx_Len,  UDP packet length to UDP core.
 *
 * @return Nothing.
 *****************************************************************************/
void pTxPath(
        stream<Axis<64> >    &siRol_Data,
        stream<Axis<16> >    &siRxP_Len,
        stream<Metadata>     &siRxP_Meta,
        stream<Axis<64> >    &soUdp_Data,
        stream<Metadata>     &soUdmx_Meta,
        stream<Axis<16> >    &soUdmx_Len)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    if ( !siRol_Data.empty() && !soUdp_Data.full() )
        soUdp_Data.write(siRol_Data.read());

    if (!siRxP_Meta.empty()  && !soUdmx_Meta.full())
        soUdmx_Meta.write(siRxP_Meta.read());

    if (!siRxP_Len.empty() && !soUdmx_Len.full())
        soUdmx_Len.write(siRxP_Len.read());
}


/*****************************************************************************
 * @brief Transfer data in between two streams. The communication channel is
 *        handled with non-blocking read and write operations.
 * @ingroup udp_role_if
 *
 * @param[in]  si, the incoming data stream.
 * @param[out] so, the outgoing data stream.
 *
 * @return Nothing.
 *****************************************************************************/
void channel (
        stream<Axis<64> >    &si,
        stream<Axis<64> >    &so)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1

    if (!si.empty() && !so.full()) {
        so.write(si.read());
    }
}


/*****************************************************************************
 * @brief Enqueue data on a FiFo stream.
 * @ingroup udp_role_if
 *
 * @param[in]  si, the incoming data stream.
 * @param[out] so, the outgoing data stream.
 *
 * @return Nothing.
 *****************************************************************************/
void pEnqueue (
        stream<Axis<64> >    &si,
        stream<Axis<64> >    &so)
{
    channel(si, so);
}


/*****************************************************************************
 * @brief Dequeue data from a FiFo stream.
 * @ingroup udp_role_if
 *
 * @param[in]  si, the incoming data stream.
 * @param[out] so, the outgoing data stream.
 *
 * @return Nothing.
 *****************************************************************************/
void pDequeue (
        stream<Axis<64> >    &si,
        stream<Axis<64> >    &so)
{
    channel(si, so);
}


/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 * @ingroup udp_role_if
 *
 * @param[in]  siROL_This_Data,     UDP data stream from the ROLE.
 * @param[out] soTHIS_Rol_Data,     UDP data stream to the ROLE.
 * @param[in]  siUDMX_This_OpnAck   Open port request from the UDP-Mux. 
 * @param[out] soTHIS_Udmx_OpnReq   Open port acknowledgment to the UDP-Mux.
 * @param[in]  siUDMX_This_Data     Data path from the UDP-Mux.
 * @param[in]  siUDMX_This_Meta     Metadata from the UDP-Mux.
 * @param[out] soTHIS_Udmx_Data     Data path to the UDP-Mux.
 * @param[out] soTHIS_Udmx_Meta     Metadata to the UDP-Mux.
 * @param[out] soTHIS_Udmx_Len      Length of last Rx packet to the UDP-Mux. 
 *
 * @return Nothing.
 *****************************************************************************/
void udp_role_if (

        //------------------------------------------------------
        //-- ROLE / This / Udp Interfaces
        //------------------------------------------------------
        stream<Axis<64> >   &siROL_This_Data,
        stream<Axis<64> >   &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- UDMX / This / Open-Port Interfaces
        //------------------------------------------------------
        stream<Axis<1> >    &siUDMX_This_OpnAck,
        stream<Axis<16> >   &soTHIS_Udmx_OpnReq,

        //------------------------------------------------------
        //-- UDMX / This / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<Axis<64> >   &siUDMX_This_Data,
        stream<Metadata>    &siUDMX_This_Meta,
        stream<Axis<64> >   &soTHIS_Udmx_Data,
        stream<Metadata>    &soTHIS_Udmx_Meta,
        stream<Axis<16> >   &soTHIS_Udmx_Len)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE axis register forward port=siROL_This_Data
    #pragma HLS INTERFACE axis register forward port=soTHIS_Rol_Data

    #pragma HLS INTERFACE axis register forward port=siUDMX_This_OpnAck
    #pragma HLS INTERFACE axis register forward port=soTHIS_Udmx_OpnReq

    #pragma HLS INTERFACE axis register forward port=siUDMX_This_Data
    #pragma HLS INTERFACE axis register forward port=siUDMX_This_Meta
    #pragma HLS DATA_PACK                   variable=siUDMX_This_Meta instance=siUDMX_This_Meta
    #pragma HLS INTERFACE axis register forward port=soTHIS_Udmx_Data
    #pragma HLS INTERFACE axis register forward port=soTHIS_Udmx_Meta
    #pragma HLS DATA_PACK                   variable=soTHIS_Udmx_Meta instance=soTHIS_Udmx_Meta
    #pragma HLS INTERFACE axis register forward port=soTHIS_Udmx_Len

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=siROL_This_Data     Metadata="-bus_bundle siROL_This_Data"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=soTHIS_Rol_Data     Metadata="-bus_bundle soTHIS_Rol_Data"

    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=siUDMX_This_OpnAck   // OBSOLETE-20180704 Metadata="-bus_bundle siUDMX_This_OpnAck"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Data     // OBSOLETE-20180704 Metadata="-bus_bundle siUDMX_This_Data"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Meta     // OBSOLETE-20180704 Metadata="-bus_bundle siUDMX_This_Meta"

    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_OpnReq   // OBSOLETE-20180704 Metadata="-bus_bundle soTHIS_Udmx_OpnReq"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Data     // OBSOLETE-20180704 Metadata="-bus_bundle soTHIS_Udmx_Data"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Meta     // OBSOLETE-20180704 Metadata="-bus_bundle soTHIS_Udmx_Meta"
    // OBSOLETE-20180705 #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Len      // OBSOLETE-20180704 Metadata="-bus_bundle soTHIS_Udmx_Len"

    //-- LOCAL VARIABLES ------------------------------------------------------
    static stream<Axis<64> >    sRolToUrifFifo  ("sRolToUrifFifo");
    #pragma HLS STREAM variable=sRolToUrifFifo depth=1024 dim=1

    static stream<Axis<64> >    sUrifToRolFifo  ("sUrifToRolFifo");
    #pragma HLS STREAM variable=sUrifToRolFifo depth=1024 dim=1

    static stream<Axis<16> >    sRxToTx_Len     ("sRxToTx_Len");
    static stream<Metadata>     sRxToTx_Meta    ("sRxToTx_Meta");

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    //---- From ROLE
    pEnqueue(siROL_This_Data, sRolToUrifFifo);
    pTxPath(sRolToUrifFifo,
            sRxToTx_Len,      sRxToTx_Meta,
            soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_Len);

    //---- To ROLE
    pDequeue(sUrifToRolFifo, soTHIS_Rol_Data);
    pRxPath(siUDMX_This_Data, siUDMX_This_Meta,
            soTHIS_Udmx_OpnReq, siUDMX_This_OpnAck,
            sUrifToRolFifo,
            sRxToTx_Len, sRxToTx_Meta);

}

