/*****************************************************************************
 * @file       : toe.cpp
 * @brief      : TCP Offload Engine (TOE)
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
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

#include "toe.hpp"

#include "session_lookup_controller/session_lookup_controller.hpp"
#include "state_table/state_table.hpp"
#include "rx_sar_table/rx_sar_table.hpp"
#include "tx_sar_table/tx_sar_table.hpp"
#include "retransmit_timer/retransmit_timer.hpp"
#include "probe_timer/probe_timer.hpp"
#include "close_timer/close_timer.hpp"
#include "event_engine/event_engine.hpp"
#include "ack_delay/ack_delay.hpp"
#include "port_table/port_table.hpp"

#include "rx_engine/src/rx_engine.hpp"
#include "tx_engine/tx_engine.hpp"

#include "rx_app_if/rx_app_if.hpp"
#include "rx_app_stream_if/rx_app_stream_if.hpp"
#include "tx_app_interface/tx_app_interface.hpp"

//#include "tx_app_if/tx_app_if.hpp"
//#include "tx_app_stream_if/tx_app_stream_if.hpp"


/*****************************************************************************
 * @brief A 2-to-1 Stream multiplexer.
 * ***************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so) {

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}


/*****************************************************************************
 * @brief This a wrapper for the timer processes (TIm).
 *
 * @param[in]  siRXe_ReTxTimerCmd,  Retransmission timer command from [RxEngine].
 * @param[in]  siTXe_ReTxTimerevent,Retransmission timer event from [TxEngine].
 * @param[in]  siRXe_ClrProbeTimer, Clear probe timer from [RXe].
 * @param[in]  siTXe_SetProbeTimer, Set probe timer from [TXe].
 * @param[in]  siRXe_CloseTimer,    Close timer from [RXe].
 * @param[out] soEVe_Event,         Event to [EventEngine].
 * @param[out] soSTt_SessCloseCmd,  Close session command to StateTable (STt).
 * @param[out] soTAi_Notif,         Notification to [TxApplicationInterface].
   @param[out] soRAi_Notif,         Notification to [RxApplicationInterface].
 *
 * @details
 *
 * @todo [TODO - Consider creating a dedicated file.]
 *
 * @ingroup toe
 *****************************************************************************/
void pTimers(
        stream<ReTxTimerCmd>      &siRXe_ReTxTimerCmd,
        stream<ReTxTimerEvent>    &siTXe_ReTxTimerevent,
        stream<ap_uint<16> >      &siRXe_ClrProbeTimer,
        stream<ap_uint<16> >      &siTXe_SetProbeTimer,
        stream<ap_uint<16> >      &siRXe_CloseTimer,
        stream<SessionId>         &soSTt_SessCloseCmd,
        stream<event>             &soEVe_Event,
        stream<OpenStatus>        &soTAi_Notif,
        stream<AppNotif>          &soRAi_Notif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    static stream<ap_uint<16> > sCloseTimer2Mux_ReleaseState ("sCloseTimer2Mux_ReleaseState");
    #pragma HLS stream variable=sCloseTimer2Mux_ReleaseState depth=2

    static stream<ap_uint<16> > sRttToSmx_SessCloseCmd       ("sRttToSmx_SessCloseCmd");
    #pragma HLS stream variable=sRttToSmx_SessCloseCmd       depth=2

    static stream<event>        sRttToEmx_Event              ("sRttToEmx_Event");
    #pragma HLS stream variable=sRttToEmx_Event              depth=2

    static stream<event>        sPbToEmx_Event               ("sPbToEmx_Event");
    #pragma HLS stream variable=sPbToEmx_Event               depth=2

    // Event Mux (Emx) based on template stream Mux
    //  Notice order --> RetransmitTimer comes before ProbeTimer
    pStreamMux(
            sRttToEmx_Event,
            sPbToEmx_Event,
            soEVe_Event);

    // ReTransmit  Timer (Rtt)
    pRetransmitTimer(
            siRXe_ReTxTimerCmd,
            siTXe_ReTxTimerevent,
            sRttToEmx_Event,
            sRttToSmx_SessCloseCmd,
            soTAi_Notif,
            soRAi_Notif);

    // Probe Timer (Pbt)
    probe_timer(
            siRXe_ClrProbeTimer,
            siTXe_SetProbeTimer,
            sPbToEmx_Event);

    close_timer(
            siRXe_CloseTimer,
            sCloseTimer2Mux_ReleaseState);

    // State table release Mux (Smx) based on template stream Mux
    pStreamMux(
            sCloseTimer2Mux_ReleaseState,
            sRttToSmx_SessCloseCmd,
            soSTt_SessCloseCmd);

}

void rxAppMemAccessBreakdown(
        stream<DmCmd>        &inputMemAccess,
        stream<DmCmd>        &outputMemAccess,
        stream<ap_uint<1> >  &rxAppDoubleAccess)
{
#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS INLINE off

    static bool rxAppBreakdown = false;
    static DmCmd rxAppTempCmd;
    static ap_uint<16> rxAppAccLength = 0;

    if (rxAppBreakdown == false) {
        if (!inputMemAccess.empty() && !outputMemAccess.full()) {
            rxAppTempCmd = inputMemAccess.read();
            if ((rxAppTempCmd.saddr.range(15, 0) + rxAppTempCmd.bbt) > 65536) {
                rxAppAccLength = 65536 - rxAppTempCmd.saddr;
                outputMemAccess.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
                rxAppBreakdown = true;
            }
            else
                outputMemAccess.write(rxAppTempCmd);
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt << std::endl;
            rxAppDoubleAccess.write(rxAppBreakdown);
        }
    }
    else if (rxAppBreakdown == true) {
        if (!outputMemAccess.full()) {
            rxAppTempCmd.saddr.range(15, 0) = 0;
            rxAppAccLength = rxAppTempCmd.bbt - rxAppAccLength;
            outputMemAccess.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
            rxAppBreakdown = false;
        }
    }
}

void rxAppMemDataRead(
        stream<AxiWord>     &rxBufferReadData,
        stream<AxiWord>     &rxDataRsp,
        stream<ap_uint<1> > &rxAppDoubleAccess)
{
#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS INLINE off

    static AxiWord rxAppMemRdRxWord = AxiWord(0, 0, 0);
    static ap_uint<1> rxAppDoubleAccessFlag = 0;
    static enum rAstate {RXAPP_IDLE = 0, RXAPP_STREAM, RXAPP_JOIN, RXAPP_STREAMMERGED, RXAPP_STREAMUNMERGED, RXAPP_RESIDUE} rxAppState;
    static ap_uint<4> rxAppMemRdOffset = 0;
    static ap_uint<8> rxAppOffsetBuffer = 0;
    switch(rxAppState) {
    case RXAPP_IDLE:
        if (!rxAppDoubleAccess.empty() && !rxBufferReadData.empty() && !rxDataRsp.full()) {
            //rxAppMemRdOffset = 0;
            rxAppDoubleAccessFlag = rxAppDoubleAccess.read();
            rxBufferReadData.read(rxAppMemRdRxWord);
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    rxDataRsp.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0)  { // If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                rxDataRsp.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
            else {                                                              // Finally if there are more words in this memory access,
                rxAppState = RXAPP_STREAM;                                      // then go to RXAPP_STREAM to read them
                rxDataRsp.write(rxAppMemRdRxWord);                              // and output the current word
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }

        }
        break;
    case RXAPP_STREAM:                                                          // This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
        if (!rxBufferReadData.empty() && !rxDataRsp.full()) {                   // Verify that there's data in the input and space in the output
            rxBufferReadData.read(rxAppMemRdRxWord);                            // Read the data word in
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    rxDataRsp.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0) {// If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                rxDataRsp.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                rxAppState = RXAPP_IDLE;                                        // and go back to the idle state
            }
            else {                                                              // If the segment data hasn't finished yet
                rxDataRsp.write(rxAppMemRdRxWord);                              // output them and stay in this state
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
        }
        break;
    case RXAPP_STREAMUNMERGED:                                                  // This state handles 2nd mem.access data when no realignment is required
        if (!rxBufferReadData.empty() && !rxDataRsp.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = rxBufferReadData.read();                                 // If so read the data in a tempVariable
            if (temp.tlast == 1)                                                     // If this is the last data word...
                rxAppState = RXAPP_IDLE;                                        // Go back to the output state. Everything else is perfectly fine as is
            rxDataRsp.write(temp);                                              // Finally, output the data word before changing states
            std::cerr << "Mem.Data: " << std::hex << temp.tdata << " - " << temp.tkeep << " - " << temp.tlast << std::endl;
        }
        break;
    case RXAPP_JOIN:                                                            // This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
        if (!rxBufferReadData.empty() && !rxDataRsp.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(((rxAppMemRdOffset.to_uint64() * 8) - 1), 0);    // In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
            rxAppMemRdRxWord = rxBufferReadData.read();
            temp.tdata.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.tdata.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);                 // Buffer & realign temp into rxAppmemRdRxWord (which is a static variable)
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.tkeep);                    // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.tlast == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                        // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.tkeep = returnKeep(tempCounter + rxAppMemRdOffset);     // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.tlast = 1;                                  // also set the LAST to 1, since this is going to be the final word of this segment
                    rxAppState = RXAPP_IDLE;                                    // And go back to idle when finished with this state
                }
                else
                    rxAppState = RXAPP_RESIDUE;                                     // then go to the RXAPP_RESIDUE to output the remaining data words
            }
            else
                rxAppState = RXAPP_STREAMMERGED;                                    // then go to the RXAPP_STREAMMERGED to output the remaining data words
            rxDataRsp.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_STREAMMERGED:                                                    // This state outputs all of the remaining, realigned data words of the 2nd mem.access, which resulted from a data word
        if (!rxBufferReadData.empty() && !rxDataRsp.full()) {                   // Verify that there's data at the input and that the output is ready to receive data
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxAppMemRdRxWord = rxBufferReadData.read();                             // Read the new data word in
            temp.tdata.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.tdata.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.tkeep);            // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.tlast == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                            // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.tkeep = returnKeep(tempCounter + rxAppMemRdOffset);             // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.tlast = 1;                                                  // also set the LAST to 1, since this is going to be the final word of this segment
                    rxAppState = RXAPP_IDLE;                                        // And go back to idle when finished with this state
                }
                else                                                                // If this not the last word, because it doesn't fit in the available space in this data word
                    rxAppState = RXAPP_RESIDUE;                                         // then go to the RXAPP_RESIDUE to output the remainder of this data word
            }
            rxDataRsp.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_RESIDUE:
        if (!rxDataRsp.full()) {
            AxiWord temp = AxiWord(0, returnKeep(rxAppOffsetBuffer), 1);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxDataRsp.write(temp);                                              // And finally write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
            rxAppState = RXAPP_IDLE;                                            // And go back to the idle stage
        }
        break;
    }
}


/*****************************************************************************
 * @brief Rc Application Interface (RAi).
 *          [TODO - Consider merging with rx_app_if]
 *
 *  @param[in]  siTRIF_DataReq,        Data request from [TcpRoleInterface].
 *  @param[in]
 *  @param[in]
 *  @param[out] soPRt_LsnPortStateReq, Port state request to [PortTable].
 *  @param[in]  siPRt_LsnPortStateRep, Port state reply from [PRt].
 *  @param[in]  siRXe_Notif,           Notification from [RXe].
 *  @param[in]@
 *  @
 *
 * @ingroup rx_app_interface
 ******************************************************************************/
void rx_app_interface(
        stream<AppRdReq>            &siTRIF_DataReq,
        stream<rxSarAppd>           &siRSt_RxSarUpdRep,
        stream<AppLsnReq>           &siTRIF_ListenPortReq,
        stream<RepBit>              &soTRIF_ListenPortRep,
        stream<TcpPort>             &soPRt_LsnPortStateReq,
        stream<RepBit>              &siPRt_LsnPortStateRep,
        stream<AppNotif>            &siRXe_Notif,
        stream<AppNotif>            &siTIm_Notif,
        stream<SessionId>           &appRxDataRspMetadata,
        stream<rxSarAppd>           &soRSt_RxSarUpdRep,
        stream<DmCmd>               &rxBufferReadCmd,
        stream<AppNotif>            &soAPP_Notif,
        stream<AxiWord>             &rxBufferReadData,
        stream<AxiWord>             &rxDataRsp)
{
    #pragma HLS INLINE

    static stream<DmCmd>        rxAppStreamIf2memAccessBreakdown ("rxAppStreamIf2memAccessBreakdown");
    #pragma HLS stream variable=rxAppStreamIf2memAccessBreakdown depth=16

    static stream<ap_uint<1> >  rxAppDoubleAccess                ("rxAppDoubleAccess");
    #pragma HLS stream variable=rxAppDoubleAccess                depth=16

    // RX Application Stream Interface
    rx_app_stream_if(
            siTRIF_DataReq,
            siRSt_RxSarUpdRep,
            appRxDataRspMetadata,
            soRSt_RxSarUpdRep,
            rxAppStreamIf2memAccessBreakdown);

    rxAppMemAccessBreakdown(
            rxAppStreamIf2memAccessBreakdown,
            rxBufferReadCmd,
            rxAppDoubleAccess);

    rxAppMemDataRead(
            rxBufferReadData,
            rxDataRsp,
            rxAppDoubleAccess);

    // RX Application Interface
    rx_app_if(
            siTRIF_ListenPortReq,
            soTRIF_ListenPortRep,
            soPRt_LsnPortStateReq,
            siPRt_LsnPortStateRep
);

    // Multiplex the notifications
    pStreamMux(
            siRXe_Notif,
            siTIm_Notif,
            soAPP_Notif);
}


/*****************************************************************************
 * @brief   Main process of the TCP Offload Engine.
 *
 * @ingroup toe
 *
 * -- MMIO Interfaces
 * @param[in]  piMMIO_IpAddr,    IP4 Address from MMIO.
 * -- IPRX / This / IP Rx / Data Interface
 * @param[in]  siIPRX_Data,      IP4 data stream from IPRX.
 * -- L3MUX / This / IP Tx / Data Interface
 * @param[out] soL3MUX_Data,     IP4 data stream to L3MUX.
 * -- TRIF / Tx Data Interfaces
 * @param[out] soTRIF_Notif,     TCP notification to TRIF.
 * @param[in]  siTRIF_DReq,      TCP data request from TRIF.
 * @param[out] soTRIF_Data,      TCP data stream to TRIF.
 * @param[out] soTRIF_Meta,      TCP metadata stream to TRIF.
 * -- TRIF / Listen Interfaces
 * @param[in]  siTRIF_LsnReq,    TCP listen port request from TRIF.
 * @param[out] soTRIF_LsnAck,    TCP listen port acknowledge to TRIF.
 * -- TRIF / Rx Data Interfaces
 * @param[in]  siTRIF_Data,      TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,      TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,      TCP data status to TRIF.
 * -- TRIF / Open Interfaces
 * @param[in]  siTRIF_OpnReq,    TCP open port request from TRIF.
 * @param[out] soTRIF_OpnSts,    TCP open port status to TRIF.
 * -- TRIF / Close Interfaces
 * @param[in]  siTRIF_ClsReq,    TCP close connection request from TRIF.
 * @warning:   Not-Used,         TCP close connection status to TRIF.
 * -- MEM / This / Rx PATH / S2MM Interface
 * @warning:   Not-Used,         Rx memory read status from MEM.
 * @param[out] soMEM_RxP_RdCmd,  Rx memory read command to MEM.
 * @param[in]  siMEM_RxP_Data,   Rx memory data from MEM.
 * @param[in]  siMEM_RxP_WrSts,  Rx memory write status from MEM.
 * @param[out] soMEM_RxP_WrCmd,  Rx memory write command to MEM.
 * @param[out] soMEM_RxP_Data,   Rx memory data to MEM.
 * -- MEM / This / Tx PATH / S2MM Interface
 * @warning:   Not-Used,         Tx memory read status from MEM.
 * @param[out] soMEM_TxP_RdCmd,  Tx memory read command to MEM.
 * @param[in]  siMEM_TxP_Data,   Tx memory data from MEM.
 * @param[in]  siMEM_TxP_WrSts,  Tx memory write status from MEM.
 * @param[out] soMEM_TxP_WrCmd,  Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,   Tx memory data to MEM.
 * -- CAM / This / Session Lookup & Update Interfaces
 * @param[in]  siCAM_SssLkpRep,  Session lookup reply from CAM.
 * @param[in]  siCAM_SssUpdRep,  Session update reply from CAM.
 * @param[out] soCAM_SssLkpReq,  Session lookup request to CAM.
 * @param[out] soCAM_SssUpdReq,  Session update request to CAM.
 * -- DEBUG / Session Statistics Interfaces
 * @param[out] poDBG_SssRelCnt,  Session release count to DEBUG.
 * @param[out] poDBG_SssRegCnt,  Session register count to DEBUG.
 ******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        AxiIp4Addr                           piMMIO_IpAddr,

        //------------------------------------------------------
        //-- IPRX / This / IP Rx / Data Interface
        //------------------------------------------------------
        stream<Ip4overAxi>                  &siIPRX_Data,

        //------------------------------------------------------
        //-- L3MUX / This / IP Tx / Data Interface
        //------------------------------------------------------
        stream<Ip4overAxi>                  &soL3MUX_Data,

        //------------------------------------------------------
        //-- TRIF / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>                    &soTRIF_Notif,
        stream<AppRdReq>                    &siTRIF_DReq,
        stream<AppData>                     &soTRIF_Data,
        stream<AppMeta>                     &soTRIF_Meta,

        //------------------------------------------------------
        //-- TRIF / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReq>                   &siTRIF_LsnReq,
        stream<AppLsnAck>                   &soTRIF_LsnAck,

        //------------------------------------------------------
        //-- TRIF / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppData>                     &siTRIF_Data,
        stream<AppMeta>                     &siTRIF_Meta,
        stream<AppWrSts>                    &soTRIF_DSts,

        //------------------------------------------------------
        //-- TRIF / Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>                   &siTRIF_OpnReq,
        stream<AppOpnSts>                   &soTRIF_OpnSts,

        //------------------------------------------------------
        //-- TRIF / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReq>                   &siTRIF_ClsReq,
        //-- Not USed                       &soTRIF_ClsSts,

        //------------------------------------------------------
        //-- MEM / This / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_RxP_RdSts,
        stream<DmCmd>                       &soMEM_RxP_RdCmd,
        stream<AxiWord>                     &siMEM_RxP_Data,
        stream<DmSts>                       &siMEM_RxP_WrSts,
        stream<DmCmd>                       &soMEM_RxP_WrCmd,
        stream<AxiWord>                     &soMEM_RxP_Data,

        //------------------------------------------------------
        //-- MEM / This / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_TxP_RdSts,
        stream<DmCmd>                       &soMEM_TxP_RdCmd,
        stream<AxiWord>                     &siMEM_TxP_Data,
        stream<DmSts>                       &siMEM_TxP_WrSts,
        stream<DmCmd>                       &soMEM_TxP_WrCmd,
        stream<AxiWord>                     &soMEM_TxP_Data,

        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<rtlSessionLookupRequest>     &soCAM_SssLkpReq,
        stream<rtlSessionLookupReply>       &siCAM_SssLkpRep,
        stream<rtlSessionUpdateRequest>     &soCAM_SssUpdReq,
        stream<rtlSessionUpdateReply>       &siCAM_SssUpdRep,

        //------------------------------------------------------
        //-- To DEBUG / Session Statistics Interfaces
        //------------------------------------------------------
        ap_uint<16>                         &poDBG_SssRelCnt,
        ap_uint<16>                         &poDBG_SssRegCnt)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_stable port=piMMIO_IpAddr
    //-- IPRX / IP Rx Data Interface ------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siIPRX_Data     metadata="-bus_bundle siIPRX_Data"
    //-- L3MUX / IP Tx Data Interface -----------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soL3MUX_Data    metadata="-bus_bundle soL3MUX_Data"
    //-- TRIF / ROLE Rx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_DReq     metadata="-bus_bundle siTRIF_DReq"
    #pragma HLS DATA_PACK                variable=siTRIF_DReq
    #pragma HLS resource core=AXI4Stream variable=soTRIF_Notif    metadata="-bus_bundle soTRIF_Notif"
    #pragma HLS DATA_PACK                variable=soTRIF_Notif
    #pragma HLS resource core=AXI4Stream variable=soTRIF_Data     metadata="-bus_bundle soTRIF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTRIF_Meta     metadata="-bus_bundle soTRIF_Meta"
     //-- TRIF / ROLE Rx Listen Interface -------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_LsnReq   metadata="-bus_bundle siTRIF_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=soTRIF_LsnAck   metadata="-bus_bundle soTRIF_LsnAck"
    //-- TRIF / ROLE Tx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_Data     metadata="-bus_bundle siTRIF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTRIF_Meta     metadata="-bus_bundle siTRIF_Meta"
    #pragma HLS resource core=AXI4Stream variable=soTRIF_DSts     metadata="-bus_bundle soTRIF_DSts"
    //-- TRIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_OpnReq   metadata="-bus_bundle siTRIF_OpnReq"
    #pragma HLS DATA_PACK                variable=siTRIF_OpnReq
    #pragma HLS resource core=AXI4Stream variable=soTRIF_OpnSts   metadata="-bus_bundle soTRIF_OpnSts"
    #pragma HLS DATA_PACK                variable=soTRIF_OpnSts
    #pragma HLS resource core=AXI4Stream variable=siTRIF_ClsReq   metadata="-bus_bundle siTRIF_ClsReq"
    //-- MEM / Nts0 / RxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_RdCmd metadata="-bus_bundle soMEM_RxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soMEM_RxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_RxP_Data  metadata="-bus_bundle siMEM_RxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_RxP_WrSts metadata="-bus_bundle siMEM_RxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_RxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_WrCmd metadata="-bus_bundle soMEM_RxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soMEM_RxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_Data  metadata="-bus_bundle soMEM_RxP_Data"
    //-- MEM / Nts0 / TxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_RdCmd metadata="-bus_bundle soMEM_TxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soMEM_TxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_TxP_Data  metadata="-bus_bundle siMEM_TxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_TxP_WrSts metadata="-bus_bundle siMEM_TxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_TxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_WrCmd metadata="-bus_bundle soMEM_TxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soMEM_TxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_Data  metadata="-bus_bundle soMEM_TxP_Data"
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS resource core=AXI4Stream variable=siCAM_SssLkpRep metadata="-bus_bundle siCAM_SssLkpRep"
    #pragma HLS DATA_PACK                variable=siCAM_SssLkpRep
    #pragma HLS resource core=AXI4Stream variable=siCAM_SssUpdRep metadata="-bus_bundle siCAM_SssUpdRep"
    #pragma HLS DATA_PACK                variable=siCAM_SssUpdRep
    #pragma HLS resource core=AXI4Stream variable=soCAM_SssLkpReq metadata="-bus_bundle soCAM_SssLkpReq"
    #pragma HLS DATA_PACK                variable=soCAM_SssLkpReq
    #pragma HLS resource core=AXI4Stream variable=soCAM_SssUpdReq metadata="-bus_bundle soCAM_SssUpdReq"
    #pragma HLS DATA_PACK                variable=soCAM_SssUpdReq
    //-- DEBUG / Session Statistics Interfaces
    #pragma HLS INTERFACE ap_none register port=poDBG_SssRelCnt
    #pragma HLS INTERFACE ap_none register port=poDBG_SssRegCnt

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW
    //OBSOLETE-20181002 #pragma HLS PIPELINE II=1
    //OBSOLETE-20181002 #pragma HLS INLINE off


    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    //-- ACK Delayer (AKd)
    //-------------------------------------------------------------------------
    static stream<extendedEvent>        sAKdToTXe_Event           ("sAKdToTXe_Event");
    #pragma HLS stream         variable=sAKdToTXe_Event           depth=16
    #pragma HLS DATA_PACK      variable=sAKdToTXe_Event

    static stream<SigBit>               sAKdToEVe_RxEventSig      ("sAKdToEVe_RxEventSig");
    #pragma HLS stream         variable=sAKdToEVe_RxEventSig      depth=2

    static stream<SigBit>               sAKdToEVe_TxEventSig      ("sAKdToEVe_TxEventSig");
    #pragma HLS stream         variable=sAKdToEVe_TxEventSig      depth=2

    //-------------------------------------------------------------------------
    //-- Event Engine (EVe)
    //-------------------------------------------------------------------------
    static stream<extendedEvent>        sEVeToAKd_Event           ("sEVeToAKd_Event");
    #pragma HLS stream         variable=sEVeToAKd_Event           depth=4
    #pragma HLS DATA_PACK      variable=sEVeToAKd_Event

    //-------------------------------------------------------------------------
    //-- Port Table (PRt)
    //-------------------------------------------------------------------------
    static stream<RepBit>               sPRtToRXe_PortStateRep    ("sPRtToRXe_PortStateRep");
    #pragma HLS stream         variable=sPRtToRXe_PortStateRep    depth=4

    static stream<RepBit>               sPRtToRAi_OpnLsnPortRep   ("sPRtToRAi_OpnLsnPortRep");
    #pragma HLS stream         variable=sPRtToRAi_OpnLsnPortRep   depth=4

    static stream<TcpPort>              sPRtToTAi_ActPortStateRep ("sPRtToTAi_ActPortStateRep");
    #pragma HLS stream         variable=sPRtToTAi_ActPortStateRep depth=4

    //-- Rx Application Interface (RAi) ---------------------------------------
    static stream<TcpPort>              sRAiToPRt_OpnLsnPortReq   ("sRAiToPRt_OpnLsnPortReq");
    #pragma HLS stream         variable=sRAiToPRt_OpnLsnPortReq   depth=4

    static stream<rxSarAppd>            sRAiToRSt_RxSarUpdRep     ("sRAiToRSt_RxSarUpdRep");
    #pragma HLS stream         variable=sRAiToRSt_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRAiToRSt_RxSarUpdRep

    //-------------------------------------------------------------------------
    //-- Rx Engine (RXe)
    //-------------------------------------------------------------------------
    static stream<sessionLookupQuery>   sRXeToSLc_SessLkpReq      ("sRXeToSLc_SessLkpReq");
    #pragma HLS stream         variable=sRXeToSLc_SessLkpReq      depth=4
    #pragma HLS DATA_PACK      variable=sRXeToSLc_SessLkpReq

    static stream<TcpPort>              sRXeToPRt_PortStateReq    ("sRXeToPRt_PortStateReq");
    #pragma HLS stream         variable=sRXeToPRt_PortStateReq    depth=4

    static stream<StateQuery>           sRXeToSTt_SessStateQry    ("sRXeToSTt_SessStateQry");
    #pragma HLS stream         variable=sRXeToSTt_SessStateQry    depth=2
    #pragma HLS DATA_PACK      variable=sRXeToSTt_SessStateQry

    static stream<rxSarRecvd>           sRXeToRSt_RxSarUpdReq     ("sRXeToRSt_RxSarUpdReq");
    #pragma HLS stream         variable=sRXeToRSt_RxSarUpdReq     depth=2
    #pragma HLS DATA_PACK      variable=sRXeToRSt_RxSarUpdReq

    static stream<rxTxSarQuery>         sRXeToTSt_TxSarQry        ("sRXeToTSt_TxSarQry");
    #pragma HLS stream         variable=sRXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK      variable=sRXeToTSt_TxSarQry

    static stream<ReTxTimerCmd>         sRXeToTIm_ReTxTimerCmd    ("sRXeToTIm_ReTxTimerCmd");
    #pragma HLS stream         variable=sRXeToTIm_ReTxTimerCmd    depth=2
    #pragma HLS DATA_PACK      variable=sRXeToTIm_ReTxTimerCmd

    static stream<ap_uint<16> >         sRXeToTIm_CloseTimer      ("sRXeToTIm_CloseTimer");
    #pragma HLS stream         variable=sRXeToTIm_CloseTimer      depth=2

    static stream<ap_uint<16> >         sRXeToTIm_ClrProbeTimer   ("sRXeToTIm_ClrProbeTimer");
    // FIXME - No depth for this stream ?

    static stream<AppNotif>             sRXeToRAi_Notif           ("sRXeToRAi_Notif");
    #pragma HLS stream         variable=sRXeToRAi_Notif           depth=4
    #pragma HLS DATA_PACK      variable=sRXeToRAi_Notif

    static stream<OpenStatus>           sRXeToTAi_SessOpnSts      ("sRXeToTAi_SessOpnSts");
    #pragma HLS stream         variable=sRXeToTAi_SessOpnSts      depth=4
    #pragma HLS DATA_PACK      variable=sRXeToTAi_SessOpnSts

    static stream<extendedEvent>        sRXeToEVe_Event           ("sRXeToEVe_Event");
    #pragma HLS stream         variable=sRXeToEVe_Event           depth=512
    #pragma HLS DATA_PACK      variable=sRXeToEVe_Event

    //-- Rx SAR Table (RSt) ---------------------------------------------------
    static stream<RxSarEntry>           sRStToRXe_RxSarUpdRep     ("sRStToRXe_RxSarUpdRep");
    #pragma HLS stream         variable=sRStToRXe_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRStToRXe_RxSarUpdRep

    static stream<rxSarAppd>            sRStToRAi_RxSarUpdRep     ("sRStToRAi_RxSarUpdRep");
    #pragma HLS stream         variable=sRStToRAi_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRStToRAi_RxSarUpdRep

    static stream<RxSarEntry>           sRStToTXe_RxSarRdRep      ("sRStToTXe_RxSarRdRep");
    #pragma HLS stream         variable=sRStToTXe_RxSarRdRep      depth=2
    #pragma HLS DATA_PACK      variable=sRStToTXe_RxSarRdRep

    //-------------------------------------------------------------------------
    //-- Session Lookup Controller (SLc)
    //-------------------------------------------------------------------------
    static stream<sessionLookupReply>   sSLcToRXe_SessLkpRep      ("sSLcToRXe_SessLkpRep");
    #pragma HLS stream         variable=sSLcToRXe_SessLkpRep      depth=4
    #pragma HLS DATA_PACK      variable=sSLcToRXe_SessLkpRep

    static stream<sessionLookupReply>   sSLcToTAi_SessLookupRep   ("sSLcToTAi_SessLookupRep");
    #pragma HLS stream         variable=sSLcToTAi_SessLookupRep   depth=4
    #pragma HLS DATA_PACK      variable=sSLcToTAi_SessLookupRep

    static stream<ap_uint<16> >         sSLcToPRt_ReleasePort     ("sSLcToPRt_ReleasePort");
    #pragma HLS stream         variable=sSLcToPRt_ReleasePort     depth=4

    static stream<fourTuple>            sSLcToTXe_ReverseLkpRep   ("sSLcToTXe_ReverseLkpRep");
    #pragma HLS stream         variable=sSLcToTXe_ReverseLkpRep   depth=4
    #pragma HLS DATA_PACK      variable=sSLcToTXe_ReverseLkpRep

    //-------------------------------------------------------------------------
    //-- State Table (STt)
    //-------------------------------------------------------------------------
    static stream<SessionState>         sSTtToRXe_SessStateRep    ("sSTtToRXe_SessStateRep");
    #pragma HLS stream         variable=sSTtToRXe_SessStateRep    depth=2

    static stream<SessionState>         sSTtToTAi_Taa_StateRep    ("sSTtToTAi_Taa_StateRep");
    #pragma HLS stream         variable=sSTtToTAi_Taa_StateRep    depth=2

    static stream<SessionState>         sSTtToTAi_Tas_StateRep    ("sSTtToTAi_Tas_StateRep");
    #pragma HLS stream         variable=sSTtToTAi_Tas_StateRep    depth=2

    static stream<SessionId>            sSTtToSLc_SessReleaseCmd  ("sSTtToSLc_SessReleaseCmd");
    #pragma HLS stream         variable=sSTtToSLc_SessReleaseCmd  depth=2

    //-------------------------------------------------------------------------
    //-- Tx Application Interface (TAi)
    //-------------------------------------------------------------------------
    static stream<ReqBit>               sTAiToPRt_ActPortStateReq ("sTAiToPRt_ActPortStateReq");
    #pragma HLS stream         variable=sTAiToPRt_ActPortStateReq depth=4

    static stream<AxiSocketPair>        sTAiToSLc_SessLookupReq   ("sTAiToSLc_SessLookupReq");
    #pragma HLS DATA_PACK      variable=sTAiToSLc_SessLookupReq
    #pragma HLS stream         variable=sTAiToSLc_SessLookupReq   depth=4

    static stream<event>                sTAiToEVe_Event           ("sTAiToEVe_Event");
    #pragma HLS stream         variable=sTAiToEVe_Event           depth=4
    #pragma HLS DATA_PACK      variable=sTAiToEVe_Event

    static stream<TxSarTableAppPush>    sTAiToTSt_AppPush         ("sTAiToTSt_AppPush");
    #pragma HLS stream         variable=sTAiToTSt_AppPush         depth=2
    #pragma HLS DATA_PACK      variable=sTAiToTSt_AppPush

    static stream<StateQuery>           sTAiToSTt_Taa_StateQry    ("sTAiToSTt_Taa_StateQry");
    #pragma HLS stream         variable=sTAiToSTt_Taa_StateQry    depth=2
    #pragma HLS DATA_PACK      variable=sTAiToSTt_Taa_StateQry

    static stream<TcpSessId>            sTAiToSTt_Tas_StateReq    ("sTAiToSTt_Tas_StateReq");
    #pragma HLS stream         variable=sTAiToSTt_Tas_StateReq    depth=2

    //-------------------------------------------------------------------------
    //-- Timers (TIm)
    //-------------------------------------------------------------------------
    static stream<event>                sTImToEVe_Event           ("sTImToEVe_Event");
    #pragma HLS stream         variable=sTImToEVe_Event           depth=4 //TODO maybe reduce to 2, there should be no evil cycle
    #pragma HLS DATA_PACK      variable=sTImToEVe_Event

    static stream<SessionId>            sTImToSTt_SessCloseCmd    ("sTImToSTt_SessCloseCmd");
    #pragma HLS stream         variable=sTImToSTt_SessCloseCmd    depth=2

    static stream<OpenStatus>           sTImToTAi_Notif           ("sTImToTAi_Notif");
    #pragma HLS stream         variable=sTImToTAi_Notif           depth=4
    #pragma HLS DATA_PACK      variable=sTImToTAi_Notif

    static stream<AppNotif>             sTImToRAi_Notif           ("sTImToRAi_Notif");
    #pragma HLS stream         variable=sTImToRAi_Notif           depth=4
    #pragma HLS DATA_PACK      variable=sTImToRAi_Notif

    //-------------------------------------------------------------------------
    //-- Tx Engine (TXe)
    //-------------------------------------------------------------------------
    static stream<SigBit>               sTXeToEVe_RxEventSig      ("sTXeToEVe_RxEventSig");
    #pragma HLS stream         variable=sTXeToEVe_RxEventSig      depth=2

    static stream<ap_uint<16> >         sTXeToRSt_RxSarRdReq      ("sTXeToRSt_RxSarRdReq");
    #pragma HLS stream         variable=sTXeToRSt_RxSarRdReq      depth=2

    static stream<txTxSarQuery>         sTXeToTSt_TxSarQry        ("sTXeToTSt_TxSarQry");
    #pragma HLS stream         variable=sTXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK      variable=sTXeToTSt_TxSarQry

    static stream<ap_uint<16> >         sTXeToSLc_ReverseLkpReq   ("sTXeToSLc_ReverseLkpReq");
    #pragma HLS stream         variable=sTXeToSLc_ReverseLkpReq   depth=4

    static stream<ReTxTimerEvent>       sTXeToTIm_SetReTxTimer    ("sTXeToTIm_SetReTxTimer");
    #pragma HLS stream         variable=sTXeToTIm_SetReTxTimer    depth=2
    #pragma HLS DATA_PACK      variable=sTXeToTIm_SetReTxTimer

    static stream<ap_uint<16> >         sTXeToTIm_SetProbeTimer   ("sTXeToTIm_SetProbeTimer");
    #pragma HLS stream         variable=sTXeToTIm_SetProbeTimer   depth=2

    //-------------------------------------------------------------------------
    //-- Tx SAR Table (TSt)
    //-------------------------------------------------------------------------
    static stream<rxTxSarReply>         sTStToRXe_SessTxSarRep    ("sTStToRXe_SessTxSarRep");
    #pragma HLS stream         variable=sTStToRXe_SessTxSarRep    depth=2
    #pragma HLS DATA_PACK      variable=sTStToRXe_SessTxSarRep

    static stream<txTxSarReply>         sTStToTXe_TxSarRep        ("sTStToTXe_TxSarRep");
    #pragma HLS stream         variable=sTStToTXe_TxSarRep        depth=2
    #pragma HLS DATA_PACK      variable=sTStToTXe_TxSarRep

    static stream<txSarAckPush>         sTStToTAi_AckPush         ("sTStToTAi_AckPush");
    #pragma HLS stream         variable=sTStToTAi_AckPush         depth=2
    #pragma HLS DATA_PACK      variable=sTStToTAi_AckPush

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


    //OBSOLETE static stream<txAppTxSarQuery>        txApp2txSar_upd_req("txApp2txSar_upd_req");
    //OBSOLETE static stream<txAppTxSarReply>        txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");

    //OBSOLETE #pragma HLS stream variable=txApp2txSar_upd_req       depth=2
    //OBSOLETE #pragma HLS stream variable=txSar2txApp_upd_rsp       depth=2

    //OBSOLETE #pragma HLS DATA_PACK variable=txApp2txSar_upd_req
    //OBSOLETE #pragma HLS DATA_PACK variable=txSar2txApp_upd_rsp

    //OBSOLETE static stream<event>                  appStreamEventFifo("appStreamEventFifo");
    //OBSOLETE static stream<event>                  retransmitEventFifo("retransmitEventFifo");

    /**********************************************************************
     * DATA STRUCTURES
     **********************************************************************/

    //-- Session Lookup Controller (SLc) -----------------------------------
    session_lookup_controller(
            sRXeToSLc_SessLkpReq,
            sSLcToRXe_SessLkpRep,
            sSTtToSLc_SessReleaseCmd,
            sSLcToPRt_ReleasePort,
            sTAiToSLc_SessLookupReq,
            sSLcToTAi_SessLookupRep,
            sTXeToSLc_ReverseLkpReq,
            sSLcToTXe_ReverseLkpRep,
            soCAM_SssLkpReq,
            siCAM_SssLkpRep,
            soCAM_SssUpdReq,
            siCAM_SssUpdRep,
            poDBG_SssRelCnt,
            poDBG_SssRegCnt);

    //-- State Table (STt) -------------------------------------------------
    state_table(
            sRXeToSTt_SessStateQry,
            sSTtToRXe_SessStateRep,
            sTAiToSTt_Taa_StateQry,
            sSTtToTAi_Taa_StateRep,
            sTAiToSTt_Tas_StateReq,
            sSTtToTAi_Tas_StateRep,
            sTImToSTt_SessCloseCmd,
            sSTtToSLc_SessReleaseCmd);

    //-- RX SAR Table (RSt) ------------------------------------------------
    rx_sar_table(
            sRXeToRSt_RxSarUpdReq,
            sRAiToRSt_RxSarUpdRep,
            sTXeToRSt_RxSarRdReq,
            sRStToRXe_RxSarUpdRep,
            sRStToRAi_RxSarUpdRep,
            sRStToTXe_RxSarRdRep);

    //-- TX SAR Table (TSt) ------------------------------------------------
    tx_sar_table(
            sRXeToTSt_TxSarQry,
            //txApp2txSar_upd_req,
            sTXeToTSt_TxSarQry,
            sTStToTXe_TxSarRep,
            sTAiToTSt_AppPush,
            sTStToRXe_SessTxSarRep,
            //txSar2txApp_upd_rsp,
            sTStToTAi_AckPush);

    //-- Port Table (PRt) --------------------------------------------------
    port_table(
            sRXeToPRt_PortStateReq,
            sPRtToRXe_PortStateRep,
            sRAiToPRt_OpnLsnPortReq,
            sPRtToRAi_OpnLsnPortRep,
            sTAiToPRt_ActPortStateReq,
            sPRtToTAi_ActPortStateRep,
            sSLcToPRt_ReleasePort);

    //-- Timers (TIm) ------------------------------------------------------
    pTimers(
            sRXeToTIm_ReTxTimerCmd,
            sTXeToTIm_SetReTxTimer,
            sRXeToTIm_ClrProbeTimer,
            sTXeToTIm_SetProbeTimer,
            sRXeToTIm_CloseTimer,
            sTImToSTt_SessCloseCmd,
            sTImToEVe_Event,
            sTImToTAi_Notif,
            sTImToRAi_Notif);

    //-- Event Engine (EVe) ------------------------------------------------
    event_engine(
            sTAiToEVe_Event,
            sRXeToEVe_Event,
            sTImToEVe_Event,
            sEVeToAKd_Event,
            sAKdToEVe_RxEventSig,
            sAKdToEVe_TxEventSig,
            sTXeToEVe_RxEventSig);

    //-- Ack Delayer (AKd)) ----------------------------------------------
     ack_delay(
            sEVeToAKd_Event,
            sAKdToTXe_Event,
            sAKdToEVe_RxEventSig,
            sAKdToEVe_TxEventSig);


    /**********************************************************************
     * RX & TX ENGINES
     **********************************************************************/

    //-- RX Engine (RXe) --------------------------------------------------
    rx_engine(
            siIPRX_Data,
            sRXeToSLc_SessLkpReq,
            sSLcToRXe_SessLkpRep,
            sRXeToSTt_SessStateQry,
            sSTtToRXe_SessStateRep,
            sRXeToPRt_PortStateReq,
            sPRtToRXe_PortStateRep,
            sRXeToRSt_RxSarUpdReq,
            sRStToRXe_RxSarUpdRep,
            sRXeToTSt_TxSarQry,
            sTStToRXe_SessTxSarRep,
            sRXeToTIm_ReTxTimerCmd,
            sRXeToTIm_ClrProbeTimer,
            sRXeToTIm_CloseTimer,
            sRXeToEVe_Event,
            sRXeToTAi_SessOpnSts,
            sRXeToRAi_Notif,
            soMEM_RxP_WrCmd,
            soMEM_RxP_Data,
            siMEM_RxP_WrSts);

    //-- TX Engine (TXe) --------------------------------------------------
    tx_engine(
            sAKdToTXe_Event,
            sTXeToRSt_RxSarRdReq,
            sRStToTXe_RxSarRdRep,
            sTXeToTSt_TxSarQry,
            sTStToTXe_TxSarRep,
            siMEM_TxP_Data,
            sTXeToTIm_SetReTxTimer,
            sTXeToTIm_SetProbeTimer,
            soMEM_TxP_RdCmd,
            sTXeToSLc_ReverseLkpReq,
            sSLcToTXe_ReverseLkpRep,
            sTXeToEVe_RxEventSig,
            soL3MUX_Data);


    /**********************************************************************
     * APPLICATION INTERFACES
     **********************************************************************/

    //-- Rx Application Interface (RAi) -----------------------------------
     rx_app_interface(
             siTRIF_DReq,
             sRStToRAi_RxSarUpdRep,
             siTRIF_LsnReq,
             soTRIF_LsnAck,
             sRAiToPRt_OpnLsnPortReq,
             sPRtToRAi_OpnLsnPortRep,
             sRXeToRAi_Notif,
             sTImToRAi_Notif,
             soTRIF_Meta,
             sRAiToRSt_RxSarUpdRep,
             soMEM_RxP_RdCmd,
             soTRIF_Notif,
             siMEM_RxP_Data,
             soTRIF_Data);

    //-- Tx Application Interface (TAi) ------------------------------------
    tx_app_interface(
            siTRIF_Data,
            siTRIF_Meta,
            sTAiToSTt_Tas_StateReq,
            sSTtToTAi_Tas_StateRep,
            sTStToTAi_AckPush,
            siMEM_TxP_WrSts,
            siTRIF_OpnReq,
            siTRIF_ClsReq,
            sSLcToTAi_SessLookupRep,
            sPRtToTAi_ActPortStateRep,
            sRXeToTAi_SessOpnSts,
            soTRIF_DSts,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            sTAiToTSt_AppPush,
            soTRIF_OpnSts,
            sTAiToSLc_SessLookupReq,
            sTAiToPRt_ActPortStateReq,
            sTAiToSTt_Taa_StateQry,
            sSTtToTAi_Taa_StateRep,
            sTAiToEVe_Event,
            sTImToTAi_Notif,
            piMMIO_IpAddr);

}

