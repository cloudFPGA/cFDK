/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : toe.cpp
 * @brief      : TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "toe.hpp"
#include "../test/test_toe_utils.hpp"

#include "session_lookup_controller/session_lookup_controller.hpp"
#include "state_table/state_table.hpp"
#include "rx_sar_table/rx_sar_table.hpp"
#include "tx_sar_table/tx_sar_table.hpp"
//OBSOLETE_20191202 #include "retransmit_timer/retransmit_timer.hpp"
//OBSOLETE_20191202 #include "probe_timer/probe_timer.hpp"
//OBSOLETE_20191202 #include "close_timer/close_timer.hpp"
#include "timers/timers.hpp"
#include "event_engine/event_engine.hpp"
#include "ack_delay/ack_delay.hpp"
#include "port_table/port_table.hpp"

#include "rx_engine/src/rx_engine.hpp"
#include "tx_engine/src/tx_engine.hpp"
//OBSOLETE_20191206 #include "rx_app_if/rx_app_if.hpp"
//OBSOLETE_20191206 #include "rx_app_stream_if/rx_app_stream_if.hpp"
#include "rx_app_interface/rx_app_interface.hpp"
#include "tx_app_interface/tx_app_interface.hpp"

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_OFF | TRACE_RDY)
 ************************************************/
#define THIS_NAME "TOE"

#define TRACE_OFF  0x0000
#define TRACE_RDY 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

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

/*** OBSOLETE_20191209 ****************
void rxAppMemAccessBreakdown(
        stream<DmCmd>        &inputMemAccess,
        stream<DmCmd>        &soMEM_RxP_RdCmd,
        stream<ap_uint<1> >  &rxAppDoubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                rxAppBreakdown = false;
    #pragma HLS reset variable=rxAppBreakdown

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static DmCmd rxAppTempCmd;
    static ap_uint<16> rxAppAccLength = 0;

    if (rxAppBreakdown == false) {
        if (!inputMemAccess.empty() && !soMEM_RxP_RdCmd.full()) {
            rxAppTempCmd = inputMemAccess.read();
            if ((rxAppTempCmd.saddr.range(15, 0) + rxAppTempCmd.bbt) > 65536) {
                rxAppAccLength = 65536 - rxAppTempCmd.saddr;
                soMEM_RxP_RdCmd.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
                rxAppBreakdown = true;
            }
            else {
                soMEM_RxP_RdCmd.write(rxAppTempCmd);
            }
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt << std::endl;
            rxAppDoubleAccess.write(rxAppBreakdown);
        }
    }
    else if (rxAppBreakdown == true) {
        if (!soMEM_RxP_RdCmd.full()) {
            rxAppTempCmd.saddr.range(15, 0) = 0;
            rxAppAccLength = rxAppTempCmd.bbt - rxAppAccLength;
            soMEM_RxP_RdCmd.write(DmCmd(rxAppTempCmd.saddr, rxAppAccLength));
            //std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
            rxAppBreakdown = false;
        }
    }
}
*****************************/

/*** OBSOLETE_20191209 ****************
void rxAppMemDataRead(
        stream<AxiWord>     &siMEM_RxP_Data,
        stream<AxiWord>     &soTRIF_Data,
        stream<ap_uint<1> > &rxAppDoubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    static AxiWord rxAppMemRdRxWord = AxiWord(0, 0, 0);
    static ap_uint<1> rxAppDoubleAccessFlag = 0;
    static enum rAstate {RXAPP_IDLE = 0, RXAPP_STREAM, RXAPP_JOIN, RXAPP_STREAMMERGED, RXAPP_STREAMUNMERGED, RXAPP_RESIDUE} rxAppState;
    static ap_uint<4> rxAppMemRdOffset = 0;
    static ap_uint<8> rxAppOffsetBuffer = 0;

    switch(rxAppState) {
    case RXAPP_IDLE:
        if (!rxAppDoubleAccess.empty() && !siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {
            //rxAppMemRdOffset = 0;
            rxAppDoubleAccessFlag = rxAppDoubleAccess.read();
            siMEM_RxP_Data.read(rxAppMemRdRxWord);
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    soTRIF_Data.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0)  { // If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                soTRIF_Data.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
            else {                                                              // Finally if there are more words in this memory access,
                rxAppState = RXAPP_STREAM;                                      // then go to RXAPP_STREAM to read them
                soTRIF_Data.write(rxAppMemRdRxWord);                              // and output the current word
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }

        }
        break;
    case RXAPP_STREAM:                                                          // This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // Verify that there's data in the input and space in the output
            siMEM_RxP_Data.read(rxAppMemRdRxWord);                            // Read the data word in
            rxAppMemRdOffset = keepMapping(rxAppMemRdRxWord.tkeep);                      // Count the number of valid bytes in this data word
            if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 1) {         // If this is the last word and this access was broken down
                rxAppMemRdRxWord.tlast = ~rxAppDoubleAccessFlag;                     // Negate the last flag inn the axiWord and determine if there's an offset
                if (rxAppMemRdOffset == 8) {                                    // No need to offset anything
                    soTRIF_Data.write(rxAppMemRdRxWord);                          // Output the word directly
                    //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                    rxAppState = RXAPP_STREAMUNMERGED;                          // Jump to stream merged since there's no joining to be performed.
                }
                else if (rxAppMemRdOffset < 8) {                                // If this data word is not full
                    rxAppState = RXAPP_JOIN;                                    // Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
                }
            }
            else if (rxAppMemRdRxWord.tlast == 1 && rxAppDoubleAccessFlag == 0) {// If this is the 1st and last data word of this segment and no mem. access breakdown occured,
                soTRIF_Data.write(rxAppMemRdRxWord);                              // then output the data word and stay in this state to read the next segment data
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
                rxAppState = RXAPP_IDLE;                                        // and go back to the idle state
            }
            else {                                                              // If the segment data hasn't finished yet
                soTRIF_Data.write(rxAppMemRdRxWord);                              // output them and stay in this state
                //std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
            }
        }
        break;
    case RXAPP_STREAMUNMERGED:                                                  // This state handles 2nd mem.access data when no realignment is required
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = siMEM_RxP_Data.read();                                 // If so read the data in a tempVariable
            if (temp.tlast == 1)                                                     // If this is the last data word...
                rxAppState = RXAPP_IDLE;                                        // Go back to the output state. Everything else is perfectly fine as is
            soTRIF_Data.write(temp);                                              // Finally, output the data word before changing states
            std::cerr << "Mem.Data: " << std::hex << temp.tdata << " - " << temp.tkeep << " - " << temp.tlast << std::endl;
        }
        break;
    case RXAPP_JOIN:                                                            // This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // First determine that there's data to input and space in the output
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(((rxAppMemRdOffset.to_uint64() * 8) - 1), 0);    // In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
            rxAppMemRdRxWord = siMEM_RxP_Data.read();
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
            soTRIF_Data.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_STREAMMERGED:                                                    // This state outputs all of the remaining, realigned data words of the 2nd mem.access, which resulted from a data word
        if (!siMEM_RxP_Data.empty() && !soTRIF_Data.full()) {                   // Verify that there's data at the input and that the output is ready to receive data
            AxiWord temp = AxiWord(0, 0xFF, 0);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            rxAppMemRdRxWord = siMEM_RxP_Data.read();                             // Read the new data word in
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
            soTRIF_Data.write(temp);                                              // Finally, write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
        }
        break;
    case RXAPP_RESIDUE:
        if (!soTRIF_Data.full()) {
            AxiWord temp = AxiWord(0, returnKeep(rxAppOffsetBuffer), 1);
            temp.tdata.range((rxAppMemRdOffset.to_uint64() * 8) - 1, 0) = rxAppMemRdRxWord.tdata.range(63, ((8 - rxAppMemRdOffset.to_uint64()) * 8));
            soTRIF_Data.write(temp);                                              // And finally write the data word to the output
            //std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
            rxAppState = RXAPP_IDLE;                                            // And go back to the idle stage
        }
        break;
    }
}
*** OBSOLETE_20191209 ****************/

/*** OBSOLETE_20191202 *******************
void rx_app_interface(
        stream<AppNotif>            &soTRIF_Notif,
        stream<AppRdReq>            &siTRIF_DataReq,
        stream<AxiWord>             &soTRIF_Data,
        stream<SessionId>           &soTRIF_Meta,
        stream<AppLsnReq>           &siTRIF_LsnReq,
        stream<AppLsnAck>           &soTRIF_LsnAck,
        stream<TcpPort>             &soPRt_LsnReq,
        stream<AckBit>              &siPRt_LsnAck,
        stream<AppNotif>            &siRXe_Notif,
        stream<AppNotif>            &siTIm_Notif,
        stream<RAiRxSarQuery>       &soRSt_RxSarReq,
        stream<RAiRxSarReply>       &siRSt_RxSarRep,
        stream<DmCmd>               &soMEM_RxP_RdCmd,
        stream<AxiWord>             &siMEM_RxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	//OBSOLETE-2019111 #pragma HLS INLINE
    #pragma HLS INLINE // off

    static stream<DmCmd>        rxAppStreamIf2memAccessBreakdown ("rxAppStreamIf2memAccessBreakdown");
    #pragma HLS stream variable=rxAppStreamIf2memAccessBreakdown depth=16

    static stream<ap_uint<1> >  rxAppDoubleAccess                ("rxAppDoubleAccess");
    #pragma HLS stream variable=rxAppDoubleAccess                depth=16

    // RX Application Stream Interface
    rx_app_stream_if(
            siTRIF_DataReq,
            soTRIF_Meta,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            rxAppStreamIf2memAccessBreakdown);

    rxAppMemAccessBreakdown(
            rxAppStreamIf2memAccessBreakdown,
            soMEM_RxP_RdCmd,
            rxAppDoubleAccess);

    rxAppMemDataRead(
            siMEM_RxP_Data,
            soTRIF_Data,
            rxAppDoubleAccess);

    // RX Application Interface
    rx_app_if(
            siTRIF_LsnReq,
            soTRIF_LsnAck,
            soPRt_LsnReq,
            siPRt_LsnAck);

    // Multiplex the notifications
    pStreamMux(
            siRXe_Notif,
            siTIm_Notif,
            soTRIF_Notif);
}
*****************************************/

/******************************************************************************
 * @brief The Ready (Rdy) process generates the ready signal of the TOE.
 *
 *  @param[in]  piPRt_Ready, The ready signal from PortTable (PRt).
 *  @param[in]  piTBD_Ready, The ready signal from TBD.
 *  @param[out] poNTS_Ready, The ready signal of the TOE.
 *
 ******************************************************************************/
void pReady(
    StsBool     &piPRt_Ready,
    StsBit      &poNTS_Ready)
{
    const char *myName = concat3(THIS_NAME, "/", "Rdy");

    poNTS_Ready = (piPRt_Ready == true) ? 1 : 0;

    if (DEBUG_LEVEL & TRACE_RDY) {
        if (poNTS_Ready)
            printInfo(myName, "Process [TOE] is ready.\n");
    }
}


/******************************************************************************
 * @brief Increments the simulation counter of the testbench (for debugging).
 *
 *  @param[out] poSimCycCount, The incremented simulation counter.
 *
 ******************************************************************************/
void pTbSimCount(
    volatile ap_uint<32>    &poSimCycCount)
{
    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    #ifdef __SYNTHESIS__
        static ap_uint<32>         sCounter = 0xFFFFFFF9;
    #else
        static ap_uint<32>         sCounter = 0x00000000;
    #endif
    #pragma HLS reset variable=sCounter

    sCounter += 1;
    poSimCycCount = sCounter;
}


/*****************************************************************************
 * @brief   Main process of the TCP Offload Engine.
 *
 * -- MMIO Interfaces
 * @param[in]  piMMIO_IpAddr,    IP4 Address from MMIO.
 * -- NTS Interfaces
 * @param[out] poNTS_Ready,      Ready signal of the TOE.
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data,      IP4 data stream from IPRX.
 * -- L3MUX / IP Tx / Data Interface
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
 * @param[out] soTRIF_OpnRep,    TCP open port reply to TRIF.
 * -- TRIF / Close Interfaces
 * @param[in]  siTRIF_ClsReq,    TCP close connection request from TRIF.
 * @warning:   Not-Used,         TCP close connection status to TRIF.
 * -- MEM / Rx PATH / S2MM Interface
 * @warning:   Not-Used,         Rx memory read status from MEM.
 * @param[out] soMEM_RxP_RdCmd,  Rx memory read command to MEM.
 * @param[in]  siMEM_RxP_Data,   Rx memory data from MEM.
 * @param[in]  siMEM_RxP_WrSts,  Rx memory write status from MEM.
 * @param[out] soMEM_RxP_WrCmd,  Rx memory write command to MEM.
 * @param[out] soMEM_RxP_Data,   Rx memory data to MEM.
 * -- MEM / Tx PATH / S2MM Interface
 * @warning:   Not-Used,         Tx memory read status from MEM.
 * @param[out] soMEM_TxP_RdCmd,  Tx memory read command to MEM.
 * @param[in]  siMEM_TxP_Data,   Tx memory data from MEM.
 * @param[in]  siMEM_TxP_WrSts,  Tx memory write status from MEM.
 * @param[out] soMEM_TxP_WrCmd,  Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,   Tx memory data to MEM.
 * -- CAM / Session Lookup & Update Interfaces
 * @param[in]  siCAM_SssLkpRep,  Session lookup reply from CAM.
 * @param[in]  siCAM_SssUpdRep,  Session update reply from CAM.
 * @param[out] soCAM_SssLkpReq,  Session lookup request to CAM.
 * @param[out] soCAM_SssUpdReq,  Session update request to CAM.
 * -- DEBUG / Session Statistics Interfaces
 * @param[out] poDBG_SssRelCnt,  Session release count to DEBUG.
 * @param[out] poDBG_SssRegCnt,  Session register count to DEBUG.
 * -- DEBUG / SimCycCounter
 * @param[out] poSimCycCount,    Cycle simulation counter to testbench.
 ******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        LE_Ip4Addr                           piMMIO_IpAddr,

        //------------------------------------------------------
        //-- NTS Interfaces
        //------------------------------------------------------
        StsBit                              &poNTS_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<Ip4overMac>                  &siIPRX_Data,

        //------------------------------------------------------
        //-- L3MUX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<Ip4overMac>                  &soL3MUX_Data,

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
        stream<AppOpnRep>                   &soTRIF_OpnRep,

        //------------------------------------------------------
        //-- TRIF / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReq>                   &siTRIF_ClsReq,
        //-- Not USed                       &soTRIF_ClsSts,

        //------------------------------------------------------
        //-- MEM / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_RxP_RdSts,
        stream<DmCmd>                       &soMEM_RxP_RdCmd,
        stream<AxiWord>                     &siMEM_RxP_Data,
        stream<DmSts>                       &siMEM_RxP_WrSts,
        stream<DmCmd>                       &soMEM_RxP_WrCmd,
        stream<AxiWord>                     &soMEM_RxP_Data,

        //------------------------------------------------------
        //-- MEM / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_TxP_RdSts,
        stream<DmCmd>                       &soMEM_TxP_RdCmd,
        stream<AxiWord>                     &siMEM_TxP_Data,
        stream<DmSts>                       &siMEM_TxP_WrSts,
        stream<DmCmd>                       &soMEM_TxP_WrCmd,
        stream<AxiWord>                     &soMEM_TxP_Data,

        //------------------------------------------------------
        //-- CAM / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<RtlSessionLookupRequest>     &soCAM_SssLkpReq,
        stream<RtlSessionLookupReply>       &siCAM_SssLkpRep,
        stream<RtlSessionUpdateRequest>     &soCAM_SssUpdReq,
        stream<RtlSessionUpdateReply>       &siCAM_SssUpdRep,

        //------------------------------------------------------
        //-- DEBUG Interfaces
        //------------------------------------------------------
        ap_uint<16>                         &poDBG_SssRelCnt,
        ap_uint<16>                         &poDBG_SssRegCnt,
        //--
        ap_uint<32>                         &poSimCycCount)
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_stable          port=piMMIO_IpAddr
    //-- NTS Interfaces
    #pragma HLS INTERFACE ap_none register   port=poNTS_Ready
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
    #pragma HLS DATA_PACK                variable=soTRIF_DSts
    //-- TRIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTRIF_OpnReq   metadata="-bus_bundle siTRIF_OpnReq"
    #pragma HLS DATA_PACK                variable=siTRIF_OpnReq
    #pragma HLS resource core=AXI4Stream variable=soTRIF_OpnRep   metadata="-bus_bundle soTRIF_OpnRep"
    #pragma HLS DATA_PACK                variable=soTRIF_OpnRep
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
    //-- DEBUG / Simulation Counter Interfaces
    #pragma HLS INTERFACE ap_none register port=poSimCycCount

#else

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_vld  register   port=piMMIO_IpAddr   name=piMMIO_IpAddr
    //-- NTS Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poNTS_Ready     name=poNTS_Ready
    //-- IPRX / IP Rx Data Interface ------------------------------------------
    #pragma HLS INTERFACE axis off           port=siIPRX_Data name=siIPRX_Data
    //-- L3MUX / IP Tx Data Interface -----------------------------------------
    #pragma HLS INTERFACE axis off           port=soL3MUX_Data    name=soL3MUX_Data
    //-- TRIF / ROLE Rx Data Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTRIF_DReq     name=siTRIF_DReq
    #pragma HLS DATA_PACK                variable=siTRIF_DReq
    #pragma HLS INTERFACE axis off           port=soTRIF_Notif    name=soTRIF_Notif
    #pragma HLS DATA_PACK                variable=soTRIF_Notif
    #pragma HLS INTERFACE axis off           port=soTRIF_Data     name=soTRIF_Data
    #pragma HLS INTERFACE axis off           port=soTRIF_Meta     name=soTRIF_Meta
    //-- TRIF / ROLE Rx Listen Interface -------------------------------------
    #pragma HLS INTERFACE axis off           port=siTRIF_LsnReq   name=siTRIF_LsnReq
    #pragma HLS INTERFACE axis off           port=soTRIF_LsnAck   name=soTRIF_LsnAck
    //-- TRIF / ROLE Tx Data Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTRIF_Data     name=siTRIF_Data
    #pragma HLS INTERFACE axis off           port=siTRIF_Meta     name=siTRIF_Meta
    #pragma HLS INTERFACE axis off           port=soTRIF_DSts     name=soTRIF_DSts
    #pragma HLS DATA_PACK                variable=soTRIF_DSts
    //-- TRIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTRIF_OpnReq   name=siTRIF_OpnReq
    #pragma HLS DATA_PACK                variable=siTRIF_OpnReq
    #pragma HLS INTERFACE axis off           port=soTRIF_OpnRep   name=soTRIF_OpnRep
    #pragma HLS DATA_PACK                variable=soTRIF_OpnRep
    #pragma HLS INTERFACE axis off           port=siTRIF_ClsReq   name=siTRIF_ClsReq
    //-- MEM / Nts0 / RxP Interface -------------------------------------------
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_RdCmd name=soMEM_RxP_RdCmd
    #pragma HLS DATA_PACK                variable=soMEM_RxP_RdCmd
    #pragma HLS INTERFACE axis off           port=siMEM_RxP_Data  name=siMEM_RxP_Data
    #pragma HLS INTERFACE axis off           port=siMEM_RxP_WrSts name=siMEM_RxP_WrSts
    #pragma HLS DATA_PACK                variable=siMEM_RxP_WrSts
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_WrCmd name=soMEM_RxP_WrCmd
    #pragma HLS DATA_PACK                variable=soMEM_RxP_WrCmd
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_Data  name=soMEM_RxP_Data
    //-- MEM / Nts0 / TxP Interface -------------------------------------------
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_RdCmd name=soMEM_TxP_RdCmd
    #pragma HLS DATA_PACK                variable=soMEM_TxP_RdCmd
    #pragma HLS INTERFACE axis off           port=siMEM_TxP_Data  name=siMEM_TxP_Data
    #pragma HLS INTERFACE axis off           port=siMEM_TxP_WrSts name=siMEM_TxP_WrSts
    #pragma HLS DATA_PACK                variable=siMEM_TxP_WrSts
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_WrCmd name=soMEM_TxP_WrCmd
    #pragma HLS DATA_PACK                variable=soMEM_TxP_WrCmd
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_Data  name=soMEM_TxP_Data
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS INTERFACE axis off           port=siCAM_SssLkpRep name=siCAM_SssLkpRep
    #pragma HLS DATA_PACK                variable=siCAM_SssLkpRep
    #pragma HLS INTERFACE axis off           port=siCAM_SssUpdRep name=siCAM_SssUpdRep
    #pragma HLS DATA_PACK                variable=siCAM_SssUpdRep
    #pragma HLS INTERFACE axis off           port=soCAM_SssLkpReq name=soCAM_SssLkpReq
    #pragma HLS DATA_PACK                variable=soCAM_SssLkpReq
    #pragma HLS INTERFACE axis off           port=soCAM_SssUpdReq name=soCAM_SssUpdReq
    #pragma HLS DATA_PACK                variable=soCAM_SssUpdReq
    //-- DEBUG / Session Statistics Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poDBG_SssRelCnt name=poDBG_SssRelCnt
    #pragma HLS INTERFACE ap_ovld register   port=poDBG_SssRegCnt name=poDBG_SssRegCnt
    //-- DEBUG / Simulation Counter Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poSimCycCount   name=poSimCycCount

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS AND SIGNALS
    //--   (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    //-- ACK Delayer (AKd)
    //-------------------------------------------------------------------------
    static stream<ExtendedEvent>      ssAKdToTXe_Event           ("ssAKdToTXe_Event");
    #pragma HLS stream       variable=ssAKdToTXe_Event           depth=16
    #pragma HLS DATA_PACK    variable=ssAKdToTXe_Event

    static stream<SigBit>             ssAKdToEVe_RxEventSig      ("ssAKdToEVe_RxEventSig");
    #pragma HLS stream       variable=ssAKdToEVe_RxEventSig      depth=2

    static stream<SigBool>            ssAKdToEVe_TxEventSig      ("ssAKdToEVe_TxEventSig");
    #pragma HLS stream       variable=ssAKdToEVe_TxEventSig      depth=2

    //-------------------------------------------------------------------------
    //-- Event Engine (EVe)
    //-------------------------------------------------------------------------
    static stream<ExtendedEvent>      ssEVeToAKd_Event           ("ssEVeToAKd_Event");
    #pragma HLS stream       variable=ssEVeToAKd_Event           depth=4
    #pragma HLS DATA_PACK    variable=ssEVeToAKd_Event

    //-------------------------------------------------------------------------
    //-- Port Table (PRt)
    //-------------------------------------------------------------------------
    StsBool                           sPRtToRdy_Ready;

    static stream<RepBit>             ssPRtToRXe_PortStateRep    ("ssPRtToRXe_PortStateRep");
    #pragma HLS stream       variable=ssPRtToRXe_PortStateRep    depth=4

    static stream<AckBit>             ssPRtToRAi_OpnLsnPortAck   ("ssPRtToRAi_OpnLsnPortAck");
    #pragma HLS stream       variable=ssPRtToRAi_OpnLsnPortAck   depth=4

    static stream<TcpPort>            ssPRtToTAi_GetFreePortRep  ("ssPRtToTAi_GetFreePortRep");
    #pragma HLS stream       variable=ssPRtToTAi_GetFreePortRep  depth=4

    //-- Rx Application Interface (RAi) ---------------------------------------
    static stream<TcpPort>            ssRAiToPRt_OpnLsnPortReq   ("ssRAiToPRt_OpnLsnPortReq");
    #pragma HLS stream       variable=ssRAiToPRt_OpnLsnPortReq   depth=4

    static stream<RAiRxSarQuery>      ssRAiToRSt_RxSarQry        ("ssRAiToRSt_RxSarQry");
    #pragma HLS stream       variable=ssRAiToRSt_RxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRAiToRSt_RxSarQry

    //-------------------------------------------------------------------------
    //-- Rx Engine (RXe)
    //-------------------------------------------------------------------------
    static stream<SessionLookupQuery> ssRXeToSLc_SessLkpReq      ("ssRXeToSLc_SessLkpReq");
    #pragma HLS stream       variable=ssRXeToSLc_SessLkpReq      depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToSLc_SessLkpReq

    static stream<TcpPort>            ssRXeToPRt_PortStateReq    ("ssRXeToPRt_PortStateReq");
    #pragma HLS stream       variable=ssRXeToPRt_PortStateReq    depth=4

    static stream<StateQuery>         ssRXeToSTt_SessStateQry    ("ssRXeToSTt_SessStateQry");
    #pragma HLS stream       variable=ssRXeToSTt_SessStateQry    depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToSTt_SessStateQry

    static stream<RXeRxSarQuery>      ssRXeToRSt_RxSarQry        ("ssRXeToRSt_RxSarQry");
    #pragma HLS stream       variable=ssRXeToRSt_RxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToRSt_RxSarQry

    static stream<RXeTxSarQuery>      ssRXeToTSt_TxSarQry        ("ssRXeToTSt_TxSarQry");
    #pragma HLS stream       variable=ssRXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToTSt_TxSarQry

    static stream<RXeReTransTimerCmd> ssRXeToTIm_ReTxTimerCmd    ("ssRXeToTIm_ReTxTimerCmd");
    #pragma HLS stream       variable=ssRXeToTIm_ReTxTimerCmd    depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToTIm_ReTxTimerCmd

    static stream<SessionId>          ssRXeToTIm_CloseTimer      ("ssRXeToTIm_CloseTimer");
    #pragma HLS stream       variable=ssRXeToTIm_CloseTimer      depth=2

    static stream<SessionId>          ssRXeToTIm_ClrProbeTimer   ("ssRXeToTIm_ClrProbeTimer");
    #pragma HLS stream       variable=ssRXeToTIm_ClrProbeTimer   depth=2

    static stream<AppNotif>           ssRXeToRAi_Notif           ("ssRXeToRAi_Notif");
    #pragma HLS stream       variable=ssRXeToRAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToRAi_Notif

    static stream<OpenStatus>         ssRXeToTAi_SessOpnSts      ("ssRXeToTAi_SessOpnSts");
    #pragma HLS stream       variable=ssRXeToTAi_SessOpnSts      depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToTAi_SessOpnSts

    static stream<ExtendedEvent>      ssRXeToEVe_Event           ("ssRXeToEVe_Event");
    #pragma HLS stream       variable=ssRXeToEVe_Event           depth=512
    #pragma HLS DATA_PACK    variable=ssRXeToEVe_Event

    //-- Rx SAR Table (RSt) ---------------------------------------------------
    static stream<RxSarEntry>         ssRStToRXe_RxSarRep        ("ssRStToRXe_RxSarRep");
    #pragma HLS stream       variable=ssRStToRXe_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToRXe_RxSarRep

    static stream<RAiRxSarReply>      ssRStToRAi_RxSarRep        ("ssRStToRAi_RxSarRep");
    #pragma HLS stream       variable=ssRStToRAi_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToRAi_RxSarRep

    static stream<RxSarEntry>         ssRStToTXe_RxSarRep        ("ssRStToTXe_RxSarRep");
    #pragma HLS stream       variable=ssRStToTXe_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToTXe_RxSarRep

    //-------------------------------------------------------------------------
    //-- Session Lookup Controller (SLc)
    //-------------------------------------------------------------------------
    static stream<SessionLookupReply> ssSLcToRXe_SessLkpRep      ("ssSLcToRXe_SessLkpRep");
    #pragma HLS stream       variable=ssSLcToRXe_SessLkpRep      depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToRXe_SessLkpRep

    static stream<SessionLookupReply> ssSLcToTAi_SessLookupRep   ("ssSLcToTAi_SessLookupRep");
    #pragma HLS stream       variable=ssSLcToTAi_SessLookupRep   depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToTAi_SessLookupRep

    static stream<TcpPort>            ssSLcToPRt_ReleasePort     ("ssSLcToPRt_ReleasePort");
    #pragma HLS stream       variable=ssSLcToPRt_ReleasePort     depth=4

    static stream<fourTuple>          ssSLcToTXe_ReverseLkpRep   ("ssSLcToTXe_ReverseLkpRep");
    #pragma HLS stream       variable=ssSLcToTXe_ReverseLkpRep   depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToTXe_ReverseLkpRep

    //-------------------------------------------------------------------------
    //-- State Table (STt)
    //-------------------------------------------------------------------------
    static stream<SessionState>       ssSTtToRXe_SessStateRep    ("ssSTtToRXe_SessStateRep");
    #pragma HLS stream       variable=ssSTtToRXe_SessStateRep    depth=2

    static stream<SessionState>       ssSTtToTAi_AcceptStateRep  ("ssSTtToTAi_AcceptStateRep");
    #pragma HLS stream       variable=ssSTtToTAi_AcceptStateRep  depth=2

    static stream<SessionState>       ssSTtToTAi_SessStateRep    ("ssSTtToTAi_SessStateRep");
    #pragma HLS stream       variable=ssSTtToTAi_SessStateRep    depth=2

    static stream<SessionId>          ssSTtToSLc_SessReleaseCmd  ("ssSTtToSLc_SessReleaseCmd");
    #pragma HLS stream       variable=ssSTtToSLc_SessReleaseCmd  depth=2

    //-------------------------------------------------------------------------
    //-- Tx Application Interface (TAi)
    //-------------------------------------------------------------------------
    static stream<ReqBit>             ssTAiToPRt_GetFeePortReq   ("ssTAiToPRt_GetFeePortReq");
    #pragma HLS stream       variable=ssTAiToPRt_GetFeePortReq   depth=4

    static stream<LE_SocketPair>      ssTAiToSLc_SessLookupReq   ("ssTAiToSLc_SessLookupReq");
    #pragma HLS DATA_PACK    variable=ssTAiToSLc_SessLookupReq
    #pragma HLS stream       variable=ssTAiToSLc_SessLookupReq   depth=4

    static stream<Event>              ssTAiToEVe_Event           ("ssTAiToEVe_Event");
    #pragma HLS stream       variable=ssTAiToEVe_Event           depth=4
    #pragma HLS DATA_PACK    variable=ssTAiToEVe_Event

    static stream<TAiTxSarPush>       ssTAiToTSt_PushCmd         ("ssTAiToTSt_PushCmd");
    #pragma HLS stream       variable=ssTAiToTSt_PushCmd         depth=2
    #pragma HLS DATA_PACK    variable=ssTAiToTSt_PushCmd

    static stream<StateQuery>         ssTAiToSTt_AcceptStateQry  ("ssTAiToSTt_AcceptStateQry");
    #pragma HLS stream       variable=ssTAiToSTt_AcceptStateQry  depth=2
    #pragma HLS DATA_PACK    variable=ssTAiToSTt_AcceptStateQry

    static stream<TcpSessId>          ssTAiToSTt_SessStateReq    ("ssTAiToSTt_SessStateReq");
    #pragma HLS stream       variable=ssTAiToSTt_SessStateReq    depth=2

    //-------------------------------------------------------------------------
    //-- Timers (TIm)
    //-------------------------------------------------------------------------
    static stream<Event>              ssTImToEVe_Event           ("ssTImToEVe_Event");
    #pragma HLS stream       variable=ssTImToEVe_Event           depth=4 //TODO maybe reduce to 2, there should be no evil cycle
    #pragma HLS DATA_PACK    variable=ssTImToEVe_Event

    static stream<SessionId>          ssTImToSTt_SessCloseCmd    ("ssTImToSTt_SessCloseCmd");
    #pragma HLS stream       variable=ssTImToSTt_SessCloseCmd    depth=2

    static stream<OpenStatus>         ssTImToTAi_Notif           ("ssTImToTAi_Notif");
    #pragma HLS stream       variable=ssTImToTAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssTImToTAi_Notif

    static stream<AppNotif>           ssTImToRAi_Notif           ("ssTImToRAi_Notif");
    #pragma HLS stream       variable=ssTImToRAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssTImToRAi_Notif

    //-------------------------------------------------------------------------
    //-- Tx Engine (TXe)
    //-------------------------------------------------------------------------
    static stream<SigBit>             ssTXeToEVe_RxEventSig      ("ssTXeToEVe_RxEventSig");
    #pragma HLS stream       variable=ssTXeToEVe_RxEventSig      depth=2

    static stream<SessionId>          ssTXeToRSt_RxSarReq        ("ssTXeToRSt_RxSarReq");
    #pragma HLS stream       variable=ssTXeToRSt_RxSarReq        depth=2

    static stream<TXeTxSarQuery>      ssTXeToTSt_TxSarQry        ("ssTXeToTSt_TxSarQry");
    #pragma HLS stream       variable=ssTXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssTXeToTSt_TxSarQry

    static stream<SessionId>          ssTXeToSLc_ReverseLkpReq   ("ssTXeToSLc_ReverseLkpReq");
    #pragma HLS stream       variable=ssTXeToSLc_ReverseLkpReq   depth=4

    static stream<TXeReTransTimerCmd> ssTXeToTIm_SetReTxTimer    ("ssTXeToTIm_SetReTxTimer");
    #pragma HLS stream       variable=ssTXeToTIm_SetReTxTimer    depth=2
    #pragma HLS DATA_PACK    variable=ssTXeToTIm_SetReTxTimer

    static stream<SessionId>          ssTXeToTIm_SetProbeTimer   ("ssTXeToTIm_SetProbeTimer");
    #pragma HLS stream       variable=ssTXeToTIm_SetProbeTimer   depth=2

    //-------------------------------------------------------------------------
    //-- Tx SAR Table (TSt)
    //-------------------------------------------------------------------------
    static stream<RXeTxSarReply>      ssTStToRXe_TxSarRep        ("ssTStToRXe_TxSarRep");
    #pragma HLS stream       variable=ssTStToRXe_TxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssTStToRXe_TxSarRep

    static stream<TXeTxSarReply>      ssTStToTXe_TxSarRep        ("ssTStToTXe_TxSarRep");
    #pragma HLS stream       variable=ssTStToTXe_TxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssTStToTXe_TxSarRep

    static stream<TStTxSarPush>       ssTStToTAi_PushCmd         ("ssTStToTAi_PushCmd");
    #pragma HLS stream       variable=ssTStToTAi_PushCmd         depth=2
    #pragma HLS DATA_PACK    variable=ssTStToTAi_PushCmd

    /**********************************************************************
     * PROCESSES: TCP STATE-KEEPING DATA STRUCTURE
     **********************************************************************/

    //-- Session Lookup Controller (SLc) -----------------------------------
    session_lookup_controller(
            ssRXeToSLc_SessLkpReq,
            ssSLcToRXe_SessLkpRep,
            ssSTtToSLc_SessReleaseCmd,
            ssSLcToPRt_ReleasePort,
            ssTAiToSLc_SessLookupReq,
            ssSLcToTAi_SessLookupRep,
            ssTXeToSLc_ReverseLkpReq,
            ssSLcToTXe_ReverseLkpRep,
            soCAM_SssLkpReq,
            siCAM_SssLkpRep,
            soCAM_SssUpdReq,
            siCAM_SssUpdRep,
            poDBG_SssRelCnt,
            poDBG_SssRegCnt);

    //-- State Table (STt) -------------------------------------------------
    state_table(
            ssRXeToSTt_SessStateQry,
            ssSTtToRXe_SessStateRep,
            ssTAiToSTt_AcceptStateQry,
            ssSTtToTAi_AcceptStateRep,
            ssTAiToSTt_SessStateReq,
            ssSTtToTAi_SessStateRep,
            ssTImToSTt_SessCloseCmd,
            ssSTtToSLc_SessReleaseCmd);

    //-- RX SAR Table (RSt) ------------------------------------------------
    rx_sar_table(
            ssRXeToRSt_RxSarQry,
            ssRStToRXe_RxSarRep,
            ssRAiToRSt_RxSarQry,
            ssRStToRAi_RxSarRep,
            ssTXeToRSt_RxSarReq,
            ssRStToTXe_RxSarRep);

    //-- TX SAR Table (TSt) ------------------------------------------------
    tx_sar_table(
            ssRXeToTSt_TxSarQry,
            ssTStToRXe_TxSarRep,
            ssTXeToTSt_TxSarQry,
            ssTStToTXe_TxSarRep,
            ssTAiToTSt_PushCmd,
            ssTStToTAi_PushCmd);

    //-- Port Table (PRt) --------------------------------------------------
    port_table(
            sPRtToRdy_Ready,
            ssRXeToPRt_PortStateReq,
            ssPRtToRXe_PortStateRep,
            ssRAiToPRt_OpnLsnPortReq,
            ssPRtToRAi_OpnLsnPortAck,
            ssTAiToPRt_GetFeePortReq,
            ssPRtToTAi_GetFreePortRep,
            ssSLcToPRt_ReleasePort);

    //-- Timers (TIm) ------------------------------------------------------
    //OBSOLETE_20191202 pTimers(
    timers(
            ssRXeToTIm_ReTxTimerCmd,
            ssTXeToTIm_SetReTxTimer,
            ssRXeToTIm_ClrProbeTimer,
            ssTXeToTIm_SetProbeTimer,
            ssRXeToTIm_CloseTimer,
            ssTImToSTt_SessCloseCmd,
            ssTImToEVe_Event,
            ssTImToTAi_Notif,
            ssTImToRAi_Notif);

    //-- Event Engine (EVe) ------------------------------------------------
    event_engine(
            ssTAiToEVe_Event,
            ssRXeToEVe_Event,
            ssTImToEVe_Event,
            ssEVeToAKd_Event,
            ssAKdToEVe_RxEventSig,
            ssAKdToEVe_TxEventSig,
            ssTXeToEVe_RxEventSig);

    //-- Ack Delayer (AKd)) ----------------------------------------------
     ack_delay(
            ssEVeToAKd_Event,
            ssAKdToTXe_Event,
            ssAKdToEVe_RxEventSig,
            ssAKdToEVe_TxEventSig);


    /**********************************************************************
     * RX & TX ENGINES
     **********************************************************************/

    //-- RX Engine (RXe) --------------------------------------------------
    rx_engine(
            siIPRX_Data,
            ssRXeToSLc_SessLkpReq,
            ssSLcToRXe_SessLkpRep,
            ssRXeToSTt_SessStateQry,
            ssSTtToRXe_SessStateRep,
            ssRXeToPRt_PortStateReq,
            ssPRtToRXe_PortStateRep,
            ssRXeToRSt_RxSarQry,
            ssRStToRXe_RxSarRep,
            ssRXeToTSt_TxSarQry,
            ssTStToRXe_TxSarRep,
            ssRXeToTIm_ReTxTimerCmd,
            ssRXeToTIm_ClrProbeTimer,
            ssRXeToTIm_CloseTimer,
            ssRXeToEVe_Event,
            ssRXeToTAi_SessOpnSts,
            ssRXeToRAi_Notif,
            soMEM_RxP_WrCmd,
            soMEM_RxP_Data,
            siMEM_RxP_WrSts);

    //-- TX Engine (TXe) --------------------------------------------------
    tx_engine(
            ssAKdToTXe_Event,
            ssTXeToEVe_RxEventSig,
            ssTXeToRSt_RxSarReq,
            ssRStToTXe_RxSarRep,
            ssTXeToTSt_TxSarQry,
            ssTStToTXe_TxSarRep,
            soMEM_TxP_RdCmd,
            siMEM_TxP_Data,
            ssTXeToTIm_SetReTxTimer,
            ssTXeToTIm_SetProbeTimer,
            ssTXeToSLc_ReverseLkpReq,
            ssSLcToTXe_ReverseLkpRep,
            soL3MUX_Data);


    /**********************************************************************
     * APPLICATION INTERFACES
     **********************************************************************/

    //-- Rx Application Interface (RAi) -----------------------------------
     rx_app_interface(
             soTRIF_Notif,
             siTRIF_DReq,
             soTRIF_Data,
             soTRIF_Meta,
             siTRIF_LsnReq,
             soTRIF_LsnAck,
             ssRAiToPRt_OpnLsnPortReq,
             ssPRtToRAi_OpnLsnPortAck,
             ssRXeToRAi_Notif,
             ssTImToRAi_Notif,
             ssRAiToRSt_RxSarQry,
             ssRStToRAi_RxSarRep,
             soMEM_RxP_RdCmd,
             siMEM_RxP_Data);

    //-- Tx Application Interface (TAi) ------------------------------------
    tx_app_interface(
            siTRIF_OpnReq,
            soTRIF_OpnRep,
            siTRIF_ClsReq,
            siTRIF_Data,
            siTRIF_Meta,
            soTRIF_DSts,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            siMEM_TxP_WrSts,
            ssTAiToSTt_SessStateReq,
            ssSTtToTAi_SessStateRep,
            ssTAiToSTt_AcceptStateQry,
            ssSTtToTAi_AcceptStateRep,
            ssTAiToSLc_SessLookupReq,
            ssSLcToTAi_SessLookupRep,
            ssTAiToPRt_GetFeePortReq,
            ssPRtToTAi_GetFreePortRep,
            ssTStToTAi_PushCmd,
            ssTAiToTSt_PushCmd,
            ssRXeToTAi_SessOpnSts,
            ssTAiToEVe_Event,
            ssTImToTAi_Notif,




            piMMIO_IpAddr);

    /**********************************************************************
     * CONTROL AND DEBUG INTERFACES
     **********************************************************************/

    //-- Ready signal generator -------------------------------------------
    pReady(
            sPRtToRdy_Ready,
            poNTS_Ready);

    //-- Testbench counter incrementer (for debugging) --------------------
    pTbSimCount(
        poSimCycCount);

}

