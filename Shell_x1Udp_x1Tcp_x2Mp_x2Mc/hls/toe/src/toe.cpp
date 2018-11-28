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

#include "rx_engine/rx_engine.hpp"
#include "tx_engine/tx_engine.hpp"

#include "rx_app_if/rx_app_if.hpp"
#include "rx_app_stream_if/rx_app_stream_if.hpp"
#include "tx_app_interface/tx_app_interface.hpp"

//#include "tx_app_if/tx_app_if.hpp"
//#include "tx_app_stream_if/tx_app_stream_if.hpp"

//OBSOLETE-20181120 ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
//OBSOLETE-20181120     return (inputVector.range(7,0), inputVector(15, 8));
//OBSOLETE-20181120 }

//OBSOLETE-20181120 ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
//OBSOLETE-20181120     return (inputVector.range( 7, 0), inputVector(15,  8),
//OBSOLETE-20181120         inputVector.range(23,16), inputVector(31, 24));
//OBSOLETE-20181120 }

/*
ap_uint<4> keepMapping(ap_uint<8> keepValue) {          // This function counts the number of 1s in an 8-bit value
    ap_uint<4> counter = 0;
    for (ap_uint<4> i=0;i<8;++i) {
        if (keepValue.bit(i) == 1)
            counter++;
    }
    return counter;
}
*/


/*****************************************************************************
 * @brief A function to count the number of 1s in an 8-bit value.
 *****************************************************************************/
ap_uint<4> keepMapping(ap_uint<8> keepValue) {
    ap_uint<4> counter = 0;

    switch(keepValue){
        case 0x01: counter = 1; break;
        case 0x03: counter = 2; break;
        case 0x07: counter = 3; break;
        case 0x0F: counter = 4; break;
        case 0x1F: counter = 5; break;
        case 0x3F: counter = 6; break;
        case 0x7F: counter = 7; break;
        case 0xFF: counter = 8; break;
    }
    return counter;
}


/*****************************************************************************
 * @brief A function to set a number of 1s in an 8-bit value.
 *****************************************************************************/
ap_uint<8> returnKeep(ap_uint<4> count) {
    ap_uint<8> keep = 0;

    switch(count){
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }

    return keep;
}


template<typename T> void mergeFunction(stream<T>& in1, stream<T>& in2, stream<T>& out) {
#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS INLINE off
    if (!in1.empty())
        out.write(in1.read());
    else if (!in2.empty())
        out.write(in2.read());
}


/*****************************************************************************
 * @brief This a wrapper for the timer processes (TIm).
 *
 * @param[in]  siRXe_ClrReTxTimer,  Clear retransmission timer from Rx Engine (RXe).
 * @param[in]  siTXe_SetReTxTimer,  Set   retransmission timer from Tx Engine (TXe).
 * @param[]
 * @param[in]  siTXe_SetProbeTimer, Set probe timer from Tx Engine (TXe).
 * @param[out]
 *
 * @details
 *
 * @todo [TODO - Consider creating a dedicated file.]
 *
 * @ingroup toe
 *****************************************************************************/
void pTimers(
        stream<rxRetransmitTimerUpdate> &siRXe_ClrReTxTimer,
        stream<txRetransmitTimerSet>    &siTXe_SetReTxTimer,
        stream<ap_uint<16> >            &rxEng2timer_clearProbeTimer,
        stream<ap_uint<16> >            &siTXe_SetProbeTimer,
        stream<ap_uint<16> >            &rxEng2timer_setCloseTimer,
        stream<ap_uint<16> >            &timer2stateTable_releaseState,
        stream<event>                   &timer2eventEng_setEvent,
        stream<openStatus>              &rtTimer2txApp_notification,
        stream<appNotification>         &rtTimer2rxApp_notification)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    static stream<ap_uint<16> > closeTimer2stateTable_releaseState("closeTimer2stateTable_releaseState");
    static stream<ap_uint<16> > rtTimer2stateTable_releaseState("rtTimer2stateTable_releaseState");
    #pragma HLS stream variable=closeTimer2stateTable_releaseState      depth=2
    #pragma HLS stream variable=rtTimer2stateTable_releaseState             depth=2

    static stream<event> rtTimer2eventEng_setEvent("rtTimer2eventEng_setEvent");
    static stream<event> probeTimer2eventEng_setEvent("probeTimer2eventEng_setEvent");
    #pragma HLS stream variable=rtTimer2eventEng_setEvent       depth=2
    #pragma HLS stream variable=probeTimer2eventEng_setEvent    depth=2

    // Merge Events, Order: rtTimer has to be before probeTimer
    //eventMerger(rtTimer2eventEng_setEvent, probeTimer2eventEng_setEvent, timer2eventEng_setEvent);
    mergeFunction(rtTimer2eventEng_setEvent, probeTimer2eventEng_setEvent, timer2eventEng_setEvent);
    retransmit_timer(
            siRXe_ClrReTxTimer,
            siTXe_SetReTxTimer,
            rtTimer2eventEng_setEvent,
            rtTimer2stateTable_releaseState,
            rtTimer2txApp_notification,
            rtTimer2rxApp_notification);

    probe_timer(
            rxEng2timer_clearProbeTimer,
            siTXe_SetProbeTimer,
            probeTimer2eventEng_setEvent);

    close_timer(
            rxEng2timer_setCloseTimer,
            closeTimer2stateTable_releaseState);

    mergeFunction(
            closeTimer2stateTable_releaseState,
            rtTimer2stateTable_releaseState,
            timer2stateTable_releaseState);

    /***
    timerCloseMerger(
           closeTimer2stateTable_releaseState,
           rtTimer2stateTable_releaseState,
           timer2stateTable_releaseState);
    ***/
}

void rxAppMemAccessBreakdown(
        stream<mmCmd> &inputMemAccess,
        stream<mmCmd> &outputMemAccess,
        stream<ap_uint<1> > &rxAppDoubleAccess)
{
#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS INLINE off

    static bool rxAppBreakdown = false;
    static mmCmd rxAppTempCmd;
    static ap_uint<16> rxAppAccLength = 0;

    if (rxAppBreakdown == false) {
        if (!inputMemAccess.empty() && !outputMemAccess.full()) {
            rxAppTempCmd = inputMemAccess.read();
            if ((rxAppTempCmd.saddr.range(15, 0) + rxAppTempCmd.bbt) > 65536) {
                rxAppAccLength = 65536 - rxAppTempCmd.saddr;
                outputMemAccess.write(mmCmd(rxAppTempCmd.saddr, rxAppAccLength));
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
            outputMemAccess.write(mmCmd(rxAppTempCmd.saddr, rxAppAccLength));
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
            rxAppBreakdown = false;
        }
    }
}

void rxAppMemDataRead(
        stream<axiWord> &rxBufferReadData,
        stream<axiWord> &rxDataRsp,
        stream<ap_uint<1> > &rxAppDoubleAccess)
{
#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS INLINE off

    static axiWord rxAppMemRdRxWord = axiWord(0, 0, 0);
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
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.keep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    rxDataRsp.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 0)  { // If this is the 1st and last data word of this segment and no mem. access breakdown occured,
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
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.keep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    rxDataRsp.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 0) {// If this is the 1st and last data word of this segment and no mem. access breakdown occured,
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
            axiWord temp = rxBufferReadData.read();                                 // If so read the data in a tempVariable
            if (temp.last == 1)                                                     // If this is the last data word...
                rxAppState = RXAPP_IDLE;                                        // Go back to the output state. Everything else is perfectly fine as is
            rxDataRsp.write(temp);                                              // Finally, output the data word before changing states
            std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_JOIN:                                                            // This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
        if (!rxBufferReadData.empty() && !rxDataRsp.full()) {                   // First determine that there's data to input and space in the output
            axiWord temp = axiWord(0, 0xFF, 0);
            temp.data.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.data.range(((rxAppMemRdOffset.to_uint64() * 8) - 1), 0);    // In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
            rxAppMemRdRxWord = rxBufferReadData.read();
            temp.data.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);                 // Buffer & realign temp into rxAppmemRdRxWord (which is a static variable)
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.keep);                    // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.last == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                        // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.keep = returnKeep(tempCounter + rxAppMemRdOffset);     // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.last = 1;                                  // also set the LAST to 1, since this is going to be the final word of this segment
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
            axiWord temp = axiWord(0, 0xFF, 0);
            temp.data.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.data.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxAppMemRdRxWord = rxBufferReadData.read();                             // Read the new data word in
            temp.data.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset.to_uint64()) * 8) - 1, 0);
            ap_uint<4> tempCounter = keepMapping(rxAppMemRdRxWord.keep);            // Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
            rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);               // Calculate the number of bytes to go into the next & final data word
            if (rxAppMemRdRxWord.last == 1) {
                if ((tempCounter + rxAppMemRdOffset) <= 8) {                            // Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
                    temp.keep = returnKeep(tempCounter + rxAppMemRdOffset);             // then set the KEEP value of the output to the sum of the 2 data word's bytes
                    temp.last = 1;                                                  // also set the LAST to 1, since this is going to be the final word of this segment
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
            axiWord temp = axiWord(0, returnKeep(rxAppOffsetBuffer), 1);
            temp.data.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.data.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxDataRsp.write(temp);                                              // And finally write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
            rxAppState = RXAPP_IDLE;                                            // And go back to the idle stage
        }
        break;
    }
}


// TODO - Rename RxAppWrapper into RxAppInterface (RAi)
void rxAppWrapper(
        stream<appReadRequest>      &siTRIF_DataReq,
        stream<rxSarAppd>           &siRSt_RxSarUpdRep,
        stream<TcpPort>             &siTRIF_ListenPortReq,
        stream<bool>                &sPRtToRAi_LsnPortStateRep,
        stream<appNotification>     &rxEng2rxApp_notification,
        stream<appNotification>     &timer2rxApp_notification,
        stream<ap_uint<16> >        &appRxDataRspMetadata,
        stream<rxSarAppd>           &soRSt_RxSarUpdRep,
        stream<mmCmd>               &rxBufferReadCmd,
        stream<bool>                &appListenPortRsp,
        stream<TcpPort>             &soPRt_ListenReq,
        stream<appNotification>     &appNotification,
        stream<axiWord>             &rxBufferReadData,
        stream<axiWord>             &rxDataRsp)
{
    #pragma HLS INLINE
    static stream<mmCmd> rxAppStreamIf2memAccessBreakdown("rxAppStreamIf2memAccessBreakdown");
    static stream<ap_uint<1> > rxAppDoubleAccess("rxAppDoubleAccess");
    #pragma HLS stream variable=rxAppStreamIf2memAccessBreakdown    depth=16
    #pragma HLS stream variable=rxAppDoubleAccess                   depth=16

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
            sPRtToRAi_LsnPortStateRep,
            appListenPortRsp,
            soPRt_ListenReq);

    //notificationMerger(rxEng2rxApp_notification, timer2rxApp_notification, appNotification);
    mergeFunction(
            rxEng2rxApp_notification,
            timer2rxApp_notification,
            appNotification);
}


/*****************************************************************************
 * @brief   Main process of the TCP Role Interface
 *
 * @ingroup toe
 *
 * -- MMIO Interfaces
 * @param[in]  piMMIO_This_IpAddr,  IP4 Address from MMIO.
 * -- IPRX / This / IP Rx / Data Interface
 * @param[in]  siIPRX_This_Data,    IP4 data stream from IPRX.
 * -- L3MUX / This / IP Tx / Data Interface
 * @param[out] soTHIS_L3mux_Data,   IP4 data stream to L3MUX.
 * -- TRIF / This / Rx PATH / Data Interfaces
 * @param[in]  siTRIF_This_DReq,    TCP data request from TRIF.
 * @param[out] soTHIS_Trif_Notif,   TCP notification to TRIF.
 * @param[out] soTHIS_Trif_Data,    TCP data stream to TRIF.
 * @param[out] soTHIS_Trif_Meta,    TCP metadata stream to TRIF.
 * -- TRIF / This / Rx PATH / Ctrl Interfaces
 * @param[in]  siTRIF_This_LsnReq,  TCP listen port request from TRIF.
 * @param[out] soTHIS_Trif_LsnAck,  TCP listen port acknowledge to TRIF.
 * -- TRIF / This / Tx PATH / Data Interfaces
 * @param[in]  siTRIF_This_Data,    TCP data stream from TRIF.
 * @param[in]  siTRIF_This_Meta,    TCP metadata stream from TRIF.
 * @param[out] soTHIS_Trif_DSts,    TCP data status to TRIF.
 * -- TRIF / This / Tx PATH / Ctrl Interfaces
 * @param[in]  siTRIF_This_OpnReq,  TCP open port request from TRIF.
 * @param[out] soTHIS_Trif_OpnSts,  TCP open port status to TRIF.
 * @param[in]  siTRIF_This_ClsReq,  TCP close connection request from TRIF.
 * @warning:   Not-Used,            TCP close connection status to TRIF.
 * -- MEM / This / Rx PATH / S2MM Interface
 * @warning:   Not-Used,            Rx memory read status from MEM.
 * @param[out] soTHIS_Mem_RxP_RdCmd,Rx memory read command to MEM.
 * @param[in]  siMEM_This_RxP_Data, Rx memory data from MEM.
 * @param[in]  siMEM_This_RxP_WrSts,Rx memory write status from MEM.
 * @param[out] soTHIS_Mem_RxP_WrCmd,Rx memory write command to MEM.
 * @param[out] soTHIS_Mem_RxP_Data, Rx memory data to MEM.
 * -- MEM / This / Tx PATH / S2MM Interface
 * @warning:   Not-Used,            Tx memory read status from MEM.
 * @param[out] soTHIS_Mem_TxP_RdCmd,Tx memory read command to MEM.
 * @param[in]  siMEM_This_TxP_Data, Tx memory data from MEM.
 * @param[out] siMEM_This_TxP_WrSts,Tx memory write status from MEM.
 * @param[out] soTHIS_Mem_TxP_WrCmd,Tx memory write command to MEM.
 * @param[out] soTHIS_Mem_TxP_Data, Tx memory data to MEM.
 * -- CAM / This / Session Lookup & Update Interfaces
 * @param[in]  siCAM_This_SssLkpRep,Session lookup reply from CAM.
 * @param[in]  siCAM_This_SssUpdRpl,Session update reply from CAM.
 * @param[out] soTHIS_Cam_SssLkpReq,Session lookup request to CAM.
 * @param[out] soTHIS_Cam_SssUpdReq,Session update request to CAM.
 * -- DEBUG / Session Statistics Interfaces
 * @param[out] poTHIS_Dbg_SssRelCnt,Session release count to DEBUG.
 * @param[out] poTHIS_Dbg_SssRegCnt,Session register count to DEBUG.
 ******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        ap_uint<32>                         piMMIO_This_IpAddr,

        //------------------------------------------------------
        //-- IPRX / This / IP Rx / Data Interface
        //------------------------------------------------------
        stream<Ip4Word>                     &siIPRX_This_Data,

        //------------------------------------------------------
        //-- L3MUX / This / IP Tx / Data Interface
        //------------------------------------------------------
        stream<Ip4Word>                     &soTHIS_L3mux_Data,

        //------------------------------------------------------
        //-- TRIF / This / Rx PATH / Data Interfaces
        //------------------------------------------------------
        stream<appReadRequest>              &siTRIF_This_DReq,
        stream<appNotification>             &soTHIS_Trif_Notif,
        stream<axiWord>                     &soTHIS_Trif_Data,
        stream<ap_uint<16> >                &soTHIS_Trif_Meta,

        //------------------------------------------------------
        //-- TRIF / This / Rx PATH / Ctrl Interfaces
        //------------------------------------------------------
        stream<TcpPort>                     &siTRIF_This_LsnReq,
        stream<bool>                        &soTHIS_Trif_LsnAck,

        //------------------------------------------------------
        //-- TRIF / This / Tx PATH / Data Interfaces
        //------------------------------------------------------
        stream<axiWord>                     &siTRIF_This_Data,
        stream<ap_uint<16> >                &siTRIF_This_Meta,
        stream<ap_int<17> >                 &soTHIS_Trif_DSts,

        //------------------------------------------------------
        //-- TRIF / This / Tx PATH / Ctrl Interfaces
        //------------------------------------------------------
        stream<ipTuple>                     &siTRIF_This_OpnReq,
        stream<openStatus>                  &soTHIS_Trif_OpnSts,
        stream<ap_uint<16> >                &siTRIF_This_ClsReq,
        //-- Not USed                       &soTHIS_Trif_ClsSts,

        //------------------------------------------------------
        //-- MEM / This / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_This_RxP_RdSts,
        stream<mmCmd>                       &soTHIS_Mem_RxP_RdCmd,
        stream<axiWord>                     &siMEM_This_RxP_Data,
        stream<mmStatus>                    &siMEM_This_RxP_WrSts,
        stream<mmCmd>                       &soTHIS_Mem_RxP_WrCmd,
        stream<axiWord>                     &soTHIS_Mem_RxP_Data,

        //------------------------------------------------------
        //-- MEM / This / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_This_TxP_RdSts,
        stream<mmCmd>                       &soTHIS_Mem_TxP_RdCmd,
        stream<AxiWord>                     &siMEM_This_TxP_Data,
        stream<mmStatus>                    &siMEM_This_TxP_WrSts,
        stream<mmCmd>                       &soTHIS_Mem_TxP_WrCmd,
        stream<AxiWord>                     &soTHIS_Mem_TxP_Data,

        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<rtlSessionLookupReply>       &siCAM_This_SssLkpRep,
        stream<rtlSessionUpdateReply>       &siCAM_This_SssUpdRpl,
        stream<rtlSessionLookupRequest>     &soTHIS_Cam_SssLkpReq,
        stream<rtlSessionUpdateRequest>     &soTHIS_Cam_SssUpdReq,

        //------------------------------------------------------
        //-- To DEBUG / Session Statistics Interfaces
        //------------------------------------------------------
        ap_uint<16>                         &poTHIS_Dbg_SssRelCnt,
        ap_uint<16>                         &poTHIS_Dbg_SssRegCnt)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_stable port=piMMIO_This_IpAddr
    //-- IPRX / IP Rx Data Interface ------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siIPRX_This_Data     metadata="-bus_bundle siIPRX_This_Data"
    //-- L3MUX / IP Tx Data Interface -----------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soTHIS_L3mux_Data    metadata="-bus_bundle soTHIS_L3mux_Data"
    //-- TRIF / ROLE Rx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_DReq     metadata="-bus_bundle siTRIF_This_DReq"
    #pragma HLS DATA_PACK                variable=siTRIF_This_DReq
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_Notif    metadata="-bus_bundle soTHIS_Trif_Notif"
    #pragma HLS DATA_PACK                variable=soTHIS_Trif_Notif
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_Data     metadata="-bus_bundle soTHIS_Trif_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_Meta     metadata="-bus_bundle soTHIS_Trif_Meta"
     //-- TRIF / ROLE Rx Listen Interface -------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_LsnReq   metadata="-bus_bundle siTRIF_This_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_LsnAck   metadata="-bus_bundle soTHIS_Trif_LsnAck"
    //-- TRIF / ROLE Tx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_Data     metadata="-bus_bundle siTRIF_This_Data"
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_Meta     metadata="-bus_bundle siTRIF_This_Meta"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_DSts     metadata="-bus_bundle soTHIS_Trif_DSts"
    //-- TRIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_OpnReq   metadata="-bus_bundle siTRIF_This_OpnReq"
    #pragma HLS DATA_PACK                variable=siTRIF_This_OpnReq
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Trif_OpnSts   metadata="-bus_bundle soTHIS_Trif_OpnSts"
    #pragma HLS DATA_PACK                variable=soTHIS_Trif_OpnSts
    #pragma HLS resource core=AXI4Stream variable=siTRIF_This_ClsReq   metadata="-bus_bundle siTRIF_This_ClsReq"
    //-- MEM / Nts0 / RxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_RxP_RdCmd metadata="-bus_bundle soTHIS_Mem_RxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soTHIS_Mem_RxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_This_RxP_Data  metadata="-bus_bundle siMEM_This_RxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_This_RxP_WrSts metadata="-bus_bundle siMEM_This_RxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_This_RxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_RxP_WrCmd metadata="-bus_bundle soTHIS_Mem_RxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soTHIS_Mem_RxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_RxP_Data  metadata="-bus_bundle soTHIS_Mem_RxP_Data"
    //-- MEM / Nts0 / TxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_TxP_RdCmd metadata="-bus_bundle soTHIS_Mem_TxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soTHIS_Mem_TxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_This_TxP_Data  metadata="-bus_bundle siMEM_This_TxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_This_TxP_WrSts metadata="-bus_bundle siMEM_This_TxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_This_TxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_TxP_WrCmd metadata="-bus_bundle soTHIS_Mem_TxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soTHIS_Mem_TxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Mem_TxP_Data  metadata="-bus_bundle soTHIS_Mem_TxP_Data"
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS resource core=AXI4Stream variable=siCAM_This_SssLkpRep metadata="-bus_bundle siCAM_This_SssLkpRep"
    #pragma HLS DATA_PACK                variable=siCAM_This_SssLkpRep
    #pragma HLS resource core=AXI4Stream variable=siCAM_This_SssUpdRpl metadata="-bus_bundle siCAM_This_SssUpdRpl"
    #pragma HLS DATA_PACK                variable=siCAM_This_SssUpdRpl
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Cam_SssLkpReq metadata="-bus_bundle soTHIS_Cam_SssLkpReq"
    #pragma HLS DATA_PACK                variable=soTHIS_Cam_SssLkpReq
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Cam_SssUpdReq metadata="-bus_bundle soTHIS_Cam_SssUpdReq"
    #pragma HLS DATA_PACK                variable=soTHIS_Cam_SssUpdReq
    //-- DEBUG / Session Statistics Interfaces
    #pragma HLS INTERFACE ap_none register port=poTHIS_Dbg_SssRelCnt
    #pragma HLS INTERFACE ap_none register port=poTHIS_Dbg_SssRegCnt

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW
    //OBSOLETE-20181002 #pragma HLS PIPELINE II=1
    //OBSOLETE-20181002 #pragma HLS INLINE off


    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- ACK Delayer (AKd) ----------------------------------------------------
    static stream<extendedEvent>        sAKdToTXe_Event           ("sAKdToTXe_Event");
    #pragma HLS stream         variable=sAKdToTXe_Event           depth=16
    #pragma HLS DATA_PACK      variable=sAKdToTXe_Event

    //-- Event Engine (EVe) ---------------------------------------------------
    static stream<extendedEvent>        sEVeToAKd_Event           ("sEVeToAKd_Event");
    #pragma HLS stream         variable=sEVeToAKd_Event           depth=4
    #pragma HLS DATA_PACK      variable=sEVeToAKd_Event

    static stream<ap_uint<1> >          sAKdToEVe_RxEventSig      ("sAKdToEVe_RxEventSig");
    #pragma HLS stream         variable=sAKdToEVe_RxEventSig      depth=2

    static stream<ap_uint<1> >          sAKdToEVe_TxEventSig      ("sAKdToEVe_TxEventSig");
    #pragma HLS stream         variable=sAKdToEVe_TxEventSig      depth=2

    //-- Port Table (PRt) -----------------------------------------------------
    static stream<StsBit>               sPRtToRXe_PortStateRep    ("sPRtToRXe_PortStateRep");
    #pragma HLS stream         variable=sPRtToRXe_PortStateRep    depth=4

    static stream<bool>                 sPRtToRAi_LsnPortStateRep ("sPRtToRAi_LsnPortStateRep");
    #pragma HLS stream         variable=sPRtToRAi_LsnPortStateRep depth=4

    static stream<ap_uint<16> >         sPRtToTAi_ActPortStateRsp ("sPRtToTAi_ActPortStateRsp");
    #pragma HLS stream         variable=sPRtToTAi_ActPortStateRsp depth=4

    //-- Rx Application Interface (RAi) ---------------------------------------
    static stream<TcpPort>              sRAiToPRt_LsnPortStateReq ("sRAiToPRt_LsnPortStateReq");
    #pragma HLS stream         variable=sRAiToPRt_LsnPortStateReq depth=4

    static stream<rxSarAppd>            sRAiToRSt_RxSarUpdRep     ("sRAiToRSt_RxSarUpdRep");
    #pragma HLS stream         variable=sRAiToRSt_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRAiToRSt_RxSarUpdRep

    //-- Rx Engine (RXe) ------------------------------------------------------
    static stream<sessionLookupQuery>   sRXeToSLc_SessLkpReq      ("sRXeToSLc_SessLkpReq");
    #pragma HLS stream         variable=sRXeToSLc_SessLkpReq      depth=4
    #pragma HLS DATA_PACK      variable=sRXeToSLc_SessLkpReq

    static stream<AxiTcpPort>           sRXeToPRt_PortStateReq    ("sRXeToPRt_PortStateReq");
    #pragma HLS stream         variable=sRXeToPRt_PortStateReq    depth=4

    static stream<stateQuery>           sRXeToSTt_SessStateReq    ("sRXeToSTt_SessStateReq");
    #pragma HLS stream         variable=sRXeToSTt_SessStateReq    depth=2
    #pragma HLS DATA_PACK      variable=sRXeToSTt_SessStateReq

    static stream<rxSarRecvd>           sRXeToRSt_RxSarUpdReq     ("sRXeToRSt_RxSarUpdReq");
    #pragma HLS stream         variable=sRXeToRSt_RxSarUpdReq     depth=2
    #pragma HLS DATA_PACK      variable=sRXeToRSt_RxSarUpdReq

    static stream<rxTxSarQuery>         sRXeToTSt_TxSarUpdReq     ("sRXeToTSt_TxSarUpdReq");
    #pragma HLS stream         variable=sRXeToTSt_TxSarUpdReq     depth=2
    #pragma HLS DATA_PACK      variable=sRXeToTSt_TxSarUpdReq

    static stream<rxRetransmitTimerUpdate> sRXeToTIm_ClearReTxTimer("sRXeToTIm_ClearReTxTimer");
    #pragma HLS stream         variable=sRXeToTIm_ClearReTxTimer   depth=2
    #pragma HLS DATA_PACK      variable=sRXeToTIm_ClearReTxTimer

    static stream<ap_uint<16> >         sRXeToTIm_CloseTimer      ("sRXeToTIm_CloseTimer");
    #pragma HLS stream         variable=sRXeToTIm_CloseTimer      depth=2

    static stream<ap_uint<16> >         sRXeToTIm_ClrProbeTimer   ("sRXeToTIm_ClrProbeTimer");
    // FIXME - No depth for this stream ?

    static stream<appNotification>      sRXeToRXa_Notification    ("sRXeToRXa_Notification");
    #pragma HLS stream         variable=sRXeToRXa_Notification    depth=4
    #pragma HLS DATA_PACK      variable=sRXeToRXa_Notification

    static stream<openStatus>           sRXeToTAi_SessOpnSts      ("sRXeToTAi_SessOpnSts");
    #pragma HLS stream         variable=sRXeToTAi_SessOpnSts      depth=4
    #pragma HLS DATA_PACK      variable=sRXeToTAi_SessOpnSts

    static stream<extendedEvent>        sRXeToEVe_Event           ("sRXeToEVe_Event");
    #pragma HLS stream         variable=sRXeToEVe_Event           depth=512
    #pragma HLS DATA_PACK      variable=sRXeToEVe_Event

    //-- Rx SAR Table (RSt) ---------------------------------------------------
    static stream<rxSarEntry>           sRStToRXe_RxSarUpdRep     ("sRStToRXe_RxSarUpdRep");
    #pragma HLS stream         variable=sRStToRXe_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRStToRXe_RxSarUpdRep

    static stream<rxSarAppd>            sRStToRAi_RxSarUpdRep     ("sRStToRAi_RxSarUpdRep");
    #pragma HLS stream         variable=sRStToRAi_RxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sRStToRAi_RxSarUpdRep

    static stream<rxSarEntry>           sRStToTXe_RxSarRdRep      ("sRStToTXe_RxSarRdRep");
    #pragma HLS stream         variable=sRStToTXe_RxSarRdRep      depth=2
    #pragma HLS DATA_PACK      variable=sRStToTXe_RxSarRdRep

    //-- Session Lookup Controller (SLc) --------------------------------------
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

    //-- State Table (STt) ----------------------------------------------------
    static stream<sessionState>         sSTtToRXe_SessStateRep    ("sSTtToRXe_SessStateRep");
    #pragma HLS stream         variable=sSTtToRXe_SessStateRep    depth=2

    //-- Tx Application Interface (TAi)
    static stream<ap_uint<1> >          sTAiToPRt_ActPortStateReq ("sTAiToPRt_ActPortStateReq");
    #pragma HLS stream         variable=sTAiToPRt_ActPortStateReq depth=4

    static stream<fourTuple>            sTAiToSLc_SessLookupReq   ("sTAiToSLc_SessLookupReq");
    #pragma HLS DATA_PACK      variable=sTAiToSLc_SessLookupReq
    #pragma HLS stream         variable=sTAiToSLc_SessLookupReq   depth=4

    static stream<event>                sTAiToEVe_Event           ("sTAiToEVe_Event");
    #pragma HLS stream         variable=sTAiToEVe_Event           depth=4
    #pragma HLS DATA_PACK      variable=sTAiToEVe_Event

    //-- Timers (TIm) ---------------------------------------------------------
    static stream<event>                sTImToEVe_Event           ("sTImToEVe_Event");
    #pragma HLS stream         variable=sTImToEVe_Event           depth=4 //TODO maybe reduce to 2, there should be no evil cycle
    #pragma HLS DATA_PACK      variable=sTImToEVe_Event

    //-------------------------------------------------------------------------
    //-- Tx Engine (TXe)
    //-------------------------------------------------------------------------
    static stream<ap_uint<1> >          sTXeToEVe_RxEventSig      ("sTXeToEVe_RxEventSig");
    #pragma HLS stream         variable=sTXeToEVe_RxEventSig      depth=2

    static stream<ap_uint<16> >         sTXeToRSt_RxSarRdReq      ("sTXeToRSt_RxSarRdReq");
    #pragma HLS stream         variable=sTXeToRSt_RxSarRdReq      depth=2

    static stream<txTxSarQuery>         sTXeToTSt_TxSarUpdReq     ("sTXeToTSt_TxSarUpdReq");
    #pragma HLS stream         variable=sTXeToTSt_TxSarUpdReq     depth=2
    #pragma HLS DATA_PACK      variable=sTXeToTSt_TxSarUpdReq

    static stream<ap_uint<16> >         sTXeToSLc_ReverseLkpReq   ("sTXeToSLc_ReverseLkpReq");
    #pragma HLS stream         variable=sTXeToSLc_ReverseLkpReq   depth=4

    static stream<txRetransmitTimerSet> sTXeToTIm_SetReTxTimer    ("sTXeToTIm_SetReTxTimer");
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

    static stream<txTxSarReply>         sTStToTXe_TxSarUpdRep     ("sTStToTXe_TxSarUpdRep");
    #pragma HLS stream         variable=sTStToTXe_TxSarUpdRep     depth=2
    #pragma HLS DATA_PACK      variable=sTStToTXe_TxSarUpdRep



    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


    // State Table


    static stream<stateQuery>           txApp2stateTable_upd_req("txApp2stateTable_upd_req");
    static stream<sessionState>             stateTable2txApp_upd_rsp("stateTable2txApp_upd_rsp");
    static stream<ap_uint<16> >             txApp2stateTable_req("txApp2stateTable_req");
    static stream<sessionState>             stateTable2txApp_rsp("stateTable2txApp_rsp");
    static stream<ap_uint<16> >             stateTable2sLookup_releaseSession("stateTable2sLookup_releaseSession");


    #pragma HLS stream variable=txApp2stateTable_upd_req            depth=2
    #pragma HLS stream variable=stateTable2txApp_upd_rsp            depth=2
    #pragma HLS stream variable=txApp2stateTable_req                depth=2
    #pragma HLS stream variable=stateTable2txApp_rsp                depth=2
    #pragma HLS stream variable=stateTable2sLookup_releaseSession   depth=2

    #pragma HLS DATA_PACK variable=txApp2stateTable_upd_req
    //#pragma HLS DATA_PACK variable=txApp2stateTable_req

    // RX Sar Table















    // TX Sar Table


    //static stream<txAppTxSarQuery>        txApp2txSar_upd_req("txApp2txSar_upd_req");
    //static stream<txAppTxSarReply>        txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");


    static stream<txSarAckPush>             txSar2txApp_ack_push("txSar2txApp_ack_push");
    static stream<txAppTxSarPush>       txApp2txSar_push("txApp2txSar_push");


    //#pragma HLS stream variable=txApp2txSar_upd_req       depth=2
    //#pragma HLS stream variable=txSar2txApp_upd_rsp       depth=2


    #pragma HLS stream variable=txSar2txApp_ack_push    depth=2
    #pragma HLS stream variable=txApp2txSar_push        depth=2


    //#pragma HLS DATA_PACK variable=txApp2txSar_upd_req
    //#pragma HLS DATA_PACK variable=txSar2txApp_upd_rsp


    #pragma HLS DATA_PACK variable=txSar2txApp_ack_push
    #pragma HLS DATA_PACK variable=txApp2txSar_push

    // Retransmit Timer






    // Probe Timer



    // Close Timer


    // Timer Session release Fifos
    static stream<ap_uint<16> >                     timer2stateTable_releaseState("timer2stateTable_releaseState");
    #pragma HLS stream variable=timer2stateTable_releaseState           depth=2

    // Event Engine


    //static stream<event>                  appStreamEventFifo("appStreamEventFifo");
    //static stream<event>                  retransmitEventFifo("retransmitEventFifo");


    // Application Interface



    static stream<appNotification>          timer2rxApp_notification("timer2rxApp_notification");

    #pragma HLS stream variable=timer2rxApp_notification        depth=4

    #pragma HLS DATA_PACK variable=timer2rxApp_notification

    // Port Table



    static stream<openStatus>               rtTimer2txApp_notification("rtTimer2txApp_notifcation");
    #pragma HLS stream variable=rtTimer2txApp_notification depth=4
    #pragma HLS DATA_PACK variable=rtTimer2txApp_notification

    /**********************************************************************
     * DATA STRUCTURES
     **********************************************************************/

    //-- Session Lookup Controller (SLc) -----------------------------------
    session_lookup_controller(
            sRXeToSLc_SessLkpReq,
            sSLcToRXe_SessLkpRep,
            stateTable2sLookup_releaseSession,
            sSLcToPRt_ReleasePort,
            sTAiToSLc_SessLookupReq,
            sSLcToTAi_SessLookupRep,
            sTXeToSLc_ReverseLkpReq,
            sSLcToTXe_ReverseLkpRep,
            soTHIS_Cam_SssLkpReq,
            siCAM_This_SssLkpRep,
            soTHIS_Cam_SssUpdReq,
            //sessionInsert_req,
            //sessionDelete_req,
            siCAM_This_SssUpdRpl,
            poTHIS_Dbg_SssRelCnt,
            poTHIS_Dbg_SssRegCnt);

    //-- State Table (STt) -------------------------------------------------
    state_table(
            sRXeToSTt_SessStateReq,
            txApp2stateTable_upd_req,
            txApp2stateTable_req,
            timer2stateTable_releaseState,
            sSTtToRXe_SessStateRep,
            stateTable2txApp_upd_rsp,
            stateTable2txApp_rsp,
            stateTable2sLookup_releaseSession);

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
            sRXeToTSt_TxSarUpdReq,
            //txApp2txSar_upd_req,
            sTXeToTSt_TxSarUpdReq,
            txApp2txSar_push,
            sTStToRXe_SessTxSarRep,
            //txSar2txApp_upd_rsp,
            sTStToTXe_TxSarUpdRep,
            txSar2txApp_ack_push);

    //-- Port Table (PRt) --------------------------------------------------
    port_table(
            sRXeToPRt_PortStateReq,
            sPRtToRXe_PortStateRep,
            sRAiToPRt_LsnPortStateReq,
            sPRtToRAi_LsnPortStateRep,
            sTAiToPRt_ActPortStateReq,
            sPRtToTAi_ActPortStateRsp,
            sSLcToPRt_ReleasePort);

    //-- Timers (TIm) ------------------------------------------------------
    pTimers(
            sRXeToTIm_ClearReTxTimer,
            sTXeToTIm_SetReTxTimer,
            sRXeToTIm_ClrProbeTimer,
            sTXeToTIm_SetProbeTimer,
            sRXeToTIm_CloseTimer,
            timer2stateTable_releaseState,
            sTImToEVe_Event,
            rtTimer2txApp_notification,
            timer2rxApp_notification);

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
            siIPRX_This_Data,
            sSLcToRXe_SessLkpRep,
            sSTtToRXe_SessStateRep,
            sPRtToRXe_PortStateRep,
            sRStToRXe_RxSarUpdRep,
            sTStToRXe_SessTxSarRep,
            siMEM_This_RxP_WrSts,
            soTHIS_Mem_RxP_Data,
            sRXeToSTt_SessStateReq,
            sRXeToPRt_PortStateReq,
            sRXeToSLc_SessLkpReq,
            sRXeToRSt_RxSarUpdReq,
            sRXeToTSt_TxSarUpdReq,
            sRXeToTIm_ClearReTxTimer,
            sRXeToTIm_ClrProbeTimer,
            sRXeToTIm_CloseTimer,
            sRXeToTAi_SessOpnSts,
            sRXeToEVe_Event,
            soTHIS_Mem_RxP_WrCmd,
            sRXeToRXa_Notification);

    //-- TX Engine (TXe) --------------------------------------------------
    tx_engine(
            sAKdToTXe_Event,
            sTXeToRSt_RxSarRdReq,
            sRStToTXe_RxSarRdRep,
            sTXeToTSt_TxSarUpdReq,
            sTStToTXe_TxSarUpdRep,
            siMEM_This_TxP_Data,
            sTXeToTIm_SetReTxTimer,
            sTXeToTIm_SetProbeTimer,
            soTHIS_Mem_TxP_RdCmd,
            sTXeToSLc_ReverseLkpReq,
            sSLcToTXe_ReverseLkpRep,
            sTXeToEVe_RxEventSig,
            soTHIS_L3mux_Data);


    /**********************************************************************
     * APPLICATION INTERFACES
     **********************************************************************/

    //-- Rx Application Wrapper (RAi) -------------------------------------
     rxAppWrapper(
             siTRIF_This_DReq,
             sRStToRAi_RxSarUpdRep,
             siTRIF_This_LsnReq,
             sPRtToRAi_LsnPortStateRep,
             sRXeToRXa_Notification,
             timer2rxApp_notification,
             soTHIS_Trif_Meta,
             sRAiToRSt_RxSarUpdRep,
             soTHIS_Mem_RxP_RdCmd,
             soTHIS_Trif_LsnAck,
             sRAiToPRt_LsnPortStateReq,
             soTHIS_Trif_Notif,
             siMEM_This_RxP_Data,
             soTHIS_Trif_Data);

    //-- Tx Application Interface (TAi) ------------------------------------
    tx_app_interface(
            siTRIF_This_Meta,
            siTRIF_This_Data,
            stateTable2txApp_rsp,
            //txSar2txApp_upd_rsp,
            txSar2txApp_ack_push,
            siMEM_This_TxP_WrSts,
            siTRIF_This_OpnReq,
            siTRIF_This_ClsReq,
            sSLcToTAi_SessLookupRep,
            sPRtToTAi_ActPortStateRsp,
            stateTable2txApp_upd_rsp,
            sRXeToTAi_SessOpnSts,
            soTHIS_Trif_DSts,
            txApp2stateTable_req,
            //txApp2txSar_upd_req,
            soTHIS_Mem_TxP_WrCmd,
            soTHIS_Mem_TxP_Data,
            txApp2txSar_push,
            soTHIS_Trif_OpnSts,
            sTAiToSLc_SessLookupReq,
            sTAiToPRt_ActPortStateReq,
            txApp2stateTable_upd_req,
            sTAiToEVe_Event,
            rtTimer2txApp_notification,
            piMMIO_This_IpAddr);

}

