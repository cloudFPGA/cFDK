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
 * @file       : tx_app_stream.cpp
 * @brief      : Tx Application Stream (Tas) management
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "tx_app_stream.hpp"
#include "../../test/test_toe_utils.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TAS_TRACE | TAS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TAi/Tas"

#define TRACE_OFF  0x0000
#define TRACE_MDL 1 <<  1
#define TRACE_SLG 1 <<  2
#define TRACE_SMW 1 <<  3
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Segment Length Generator (slg)
 *
 * @param[in]  siTRIF_Data,      TCP data stream from TRIF.
 * @param[out] soSmw_Data,       TCP data stream to SegmentMemoryWriter (smw).
 * @param[out] soMdl_SegLen,     The length of the TCP segment to MetaDataLoader (mdl);
 *
 * @details
 *   Generates the length of the TCP segment received from the [APP] while
 *   forwarding that same data stream to the Segment Memory Writer (smw).
 *******************************************************************************/
void pSegmentLengthGenerator(
        stream<AppData>           &siTRIF_Data,
        stream<AxiWord>           &soSmw_Data,
        stream<ap_uint<16> >      &soMdl_SegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "slg");

    static TcpSegLen   tcpSegLen = 0;

    AxiWord            currWord = AxiWord(0, 0xFF, 0);

    if (!siTRIF_Data.empty()) {
        siTRIF_Data.read(currWord);

        soSmw_Data.write(currWord);
        tcpSegLen += keepMapping(currWord.tkeep);
        if (currWord.tlast) {
            soMdl_SegLen.write(tcpSegLen);
            if (DEBUG_LEVEL & TRACE_SLG) {
                printInfo(myName, "Received end-of-segment. SegLen=%d\n",  tcpSegLen.to_int());
            }
            tcpSegLen = 0;
        }
    }
}

/******************************************************************************
 * @brief Meta Data Loader (Mdl)
 *
 * @param[in]  siTRIF_Meta,        The TCP session Id this segment belongs to.
 * @param[out] soTRIF_DSts,        The TCP write transfer status to TRIF.
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from StateTable (STt).
 * @param[out] soTat_AccessReq,    Access request to TxAppTable (Tat).
 * @parma[in]  siTat_AccessRep,    Access reply from [Tat]
 * @param[in]  siSlg_SegLen,       The TCP segment length from SegmentLengthGenerator (slg).
 * @param[out] soSmw_SegMeta,      Segment memory metadata to SegmentMemoryWriter (smw).
 * @param[out] soEVe_Event,        Event to EventEngine (EVe).
 *
 * @details
 *  Reads the request from the application and loads the necessary metadata for
 *  transmitting this segment.
 *  The FSM decides if the segment is written to the TX buffer or is discarded.
 *
 *  [FIXME-TODO: Implement TCP_NODELAY]
 *
 ******************************************************************************/
void pMetaDataLoader(
        stream<AppMeta>             &siTRIF_Meta,
        stream<AppWrSts>            &soTRIF_DSts,
        stream<TcpSessId>           &soSTt_SessStateReq,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<TxAppTableRequest>   &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<SegMemMeta>          &sMdlToSmw_SegMeta,
        stream<Event>               &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { READ_REQUEST=0,  READ_META } fsmState = READ_REQUEST;
    #pragma HLS reset                         variable = fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static TcpSessId tcpSessId;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    TxAppTableReply  txAppTableReply;
    SessionState     sessState;
    TcpSegLen        segLen;

    switch(fsmState) {

    case READ_REQUEST:
        if (!siTRIF_Meta.empty()) {
            // Read the session ID
            siTRIF_Meta.read(tcpSessId);
            // Request state of the session
            assessFull(myName, soSTt_SessStateReq, "soSTt_SessStateReq");  // [FIXME-]
            soSTt_SessStateReq.write(tcpSessId);
            // Request the value of ACK and MemPtr from TxAppTable
            assessFull(myName, soTat_AccessReq, "soTat_AccessReq");  // [FIXME-]
            soTat_AccessReq.write(TxAppTableRequest(tcpSessId));
            if (DEBUG_LEVEL & TRACE_MDL) {
                printInfo(myName, "Received new Tx request for session %d.\n", tcpSessId.to_int());
            }
            fsmState = READ_META;
        }
        break;

    case READ_META:
        if (!siTat_AccessRep.empty() && !siSTt_SessStateRep.empty() && !siSlg_SegLen.empty()) {
            siSTt_SessStateRep.read(sessState);
            siTat_AccessRep.read(txAppTableReply);
            siSlg_SegLen.read(segLen);
            TcpSegLen maxWriteLength = (txAppTableReply.ackd - txAppTableReply.mempt) - 1;

            /*** [TODO - TCP_NODELAY] ******************
            #if (TCP_NODELAY)
                //tasi_writeSar.mempt and txSar.not_ackd are supposed to be equal (with a few cycles delay)
                ap_uint<16> usedLength = ((ap_uint<16>) writeSar.mempt - writeSar.ackd);
                ap_uint<16> usableWindow = 0;
                if (writeSar.min_window > usedLength) {
                    usableWindow = writeSar.min_window - usedLength;
                }
            #endif
            *******************************************/

            if (sessState != ESTABLISHED) {
                sMdlToSmw_SegMeta.write(SegMemMeta(CMD_DROP));
                // Notify App about the fail
                soTRIF_DSts.write(AppWrSts(STS_KO, ERROR_NOCONNCECTION));
                printError(myName, "Session %d is not established. Current session state is \'%s\'.\n",
                           tcpSessId.to_uint(), SessionStateString[sessState].c_str());
            }
            else if (segLen > maxWriteLength) {
                sMdlToSmw_SegMeta.write(SegMemMeta(CMD_DROP));
                // Notify app about fail
                soTRIF_DSts.write(AppWrSts(STS_KO, ERROR_NOSPACE));
                printError(myName, "There is no TxBuf memory space available for session %d.\n",
                           tcpSessId.to_uint());
            }
            else {
                // Here --> (sessState == ESTABLISHED && pkgLen <= maxWriteLength)
                // TODO there seems some redundancy
                sMdlToSmw_SegMeta.write(SegMemMeta(tcpSessId, txAppTableReply.mempt, segLen));
                soTRIF_DSts.write(AppWrSts(STS_OK, segLen));
                soEVe_Event.write(Event(TX_EVENT, tcpSessId, txAppTableReply.mempt, segLen));
                // Write new MemPtr in the TxAppTable
                soTat_AccessReq.write(TxAppTableRequest(tcpSessId, txAppTableReply.mempt + segLen));
            }
            fsmState = READ_REQUEST;
        }
        break;
    } //switch
}

/*******************************************************************************
 * @brief Segment Memory Writer (Smw)
 *
 * @param[in]  siSlg_Data,      TCP data stream from SegmentLengthGenerator (Slg).
 * @param[in]  siMdl_SegMeta,   Segment memory metadata from MetaDataLoader (Mdl).
 * @param[out] soMEM_TxP_WrCmd, Tx memory write command to [MEM].
 * @param[out] soMEM_TxP_Data,  Tx memory data to [MEM].
 *
 * @details
 *   Writes the incoming TCP segment into the external DRAM, unless the state
 *   machine of the MetaDataLoader (Mdl) decides to drop the segment.
 *
 *******************************************************************************/
void pSegmentMemoryWriter(
        stream<AxiWord>         &siSlg_Data,
        stream<SegMemMeta>      &siMdl_SegMeta,
        stream<DmCmd>           &soMEM_TxP_WrCmd,
        stream<AxiWord>         &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "smw");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0=0, S0bis, S1, S2, S3, S4, S5 } smwState = S0;
    #pragma HLS reset                              variable = smwState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static SegMemMeta    segMemMeta;
    static ap_uint<16>   breakLen;  // [TODO-typedef]
    static DmCmd         dmCmd;     // OBSOLETE-20190907 = DmCmd(0, 0);
    static bool          segIsSplitted;
    static AxiWord       pushWord;         // OBSOLETE-20190907 = AxiWord(0, 0xFF, 0);
    static uint8_t       lengthBuffer;     // OBSOLETE-20190907  = 0;
    static ap_uint<3>    accessResidue;    // OBSOLETE-20190907 = 0;
    static uint32_t      txAppWordCounter; // OBSOLETE-20190907 = 0;

    switch (smwState) {
    case S0:
        if (!siMdl_SegMeta.empty() && !soMEM_TxP_WrCmd.full()) {
            siMdl_SegMeta.read(segMemMeta);

            if (!segMemMeta.drop) {
                // Build memory address for this segment in the upper 2GB [FIXME-Must be configurable]
                ap_uint<32> memSegAddr; // [TODO-typedef+33-bits]
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = segMemMeta.sessId(13, 0);
                memSegAddr(15,  0) = segMemMeta.addr;

                dmCmd = DmCmd(memSegAddr, segMemMeta.len);
                //OBSOLETE-20190907 DmCmd tempDmCmd = dmCmd;
                if ((dmCmd.saddr.range(15, 0) + dmCmd.bbt) > 65536) {
                    /*** OBSOLETE-20190907 ****************
                    // was moved to S0bis because of timing issues
                    breakLen = 65536 - dmCmd.saddr;
                    dmCmd.bbt -= breakLen;
                    tempDmCmd = DmCmd(dmCmd.saddr, breakLen);
                    segIsSplitted = true;
                    ****************************************/
                    segIsSplitted = true;
                    smwState = S0bis;
                    break;
                }
                else {
                    segIsSplitted = false;
                    breakLen = dmCmd.bbt;
                    //OBSOLETE-20190907 soMEM_TxP_WrCmd.write(tempDmCmd);
                    soMEM_TxP_WrCmd.write(dmCmd);
                    smwState = S1;
                    if (DEBUG_LEVEL & TRACE_SMW) {
                        printDmCmd(myName, dmCmd);
                    }
                    break;
                }
            }
            smwState = S1;
        }
        break;

    case S0bis:
        breakLen   = 65536 - dmCmd.saddr;
        dmCmd.bbt -= breakLen;
        soMEM_TxP_WrCmd.write(DmCmd(dmCmd.saddr, breakLen));
        smwState = S1;
        if (DEBUG_LEVEL & TRACE_SMW) {
            printDmCmd(myName, dmCmd);
        }
        break;

    case S1:
        if (!siSlg_Data.empty()) {
            siSlg_Data.read(pushWord);
            AxiWord outputWord = pushWord;
            ap_uint<4> byteCount = keepMapping(pushWord.tkeep);
            if (!segMemMeta.drop) {
                if (breakLen > 8)
                    breakLen -= 8;
                else {
                    if (segIsSplitted == true) {
                        if (dmCmd.saddr.range(15, 0) % 8 != 0) {
                            // If the word is not perfectly aligned then there is some magic to be worked.
                            outputWord.tkeep = lenToKeep(breakLen);
                        }
                        outputWord.tlast = 1;
                        smwState = S2;
                        accessResidue = byteCount - breakLen;
                        lengthBuffer = breakLen;  // Buffer the number of bits consumed.
                    }
                    else {
                        smwState = S0;
                    }
                }
                txAppWordCounter++;
                soMEM_TxP_Data.write(outputWord);
            }
            else {
                if (pushWord.tlast == 1)
                    smwState = S0;
            }
        }
        break;
    case S2:
        if (!soMEM_TxP_WrCmd.full()) {
            if (dmCmd.saddr.range(15, 0) % 8 == 0)
                smwState = S3;
            //else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
            else if (dmCmd.bbt - accessResidue > 0)
                smwState = S4;
            else
                smwState = S5;
            dmCmd.saddr.range(15, 0) = 0;
            breakLen = dmCmd.bbt;
            soMEM_TxP_WrCmd.write(DmCmd(dmCmd.saddr, breakLen));
            segIsSplitted = false;
        }
        break;
    case S3: // This is the non-realignment state
        if (!siSlg_Data.empty() & !soMEM_TxP_Data.full()) {
            siSlg_Data.read(pushWord);
            if (!segMemMeta.drop) {
                txAppWordCounter++;
                soMEM_TxP_Data.write(pushWord);
            }
            if (pushWord.tlast == 1)
                smwState = S0;
        }
        break;
    case S4: // We go into this state when we need to realign things
        if (!siSlg_Data.empty() && !soMEM_TxP_Data.full()) {
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            pushWord = siSlg_Data.read();
            outputWord.tdata.range(63, (8-lengthBuffer)*8) = pushWord.tdata.range((lengthBuffer * 8), 0 );

            if (!segMemMeta.drop) {
                if (pushWord.tlast == 1) {
                    if (breakLen - accessResidue > lengthBuffer)  {
                        // In this case there's residue to be handled
                        breakLen -=8;
                        smwState = S5;
                    }
                    else {
                        smwState = S0;
                        outputWord.tkeep = lenToKeep(breakLen);
                        outputWord.tlast = 1;
                    }
                }
                else
                    breakLen -= 8;
                soMEM_TxP_Data.write(outputWord);
            }
            else {
                if (pushWord.tlast == 1)
                    smwState = S0;
            }
        }
        break;
    case S5:
        if (!soMEM_TxP_Data.full()) {
            if (!segMemMeta.drop) {
                AxiWord outputWord = AxiWord(0, lenToKeep(breakLen), 1);
                outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
                soMEM_TxP_Data.write(outputWord);
                smwState = S0;
            }
        }
        break;
    } // End-of switch
}

/*******************************************************************************
 * @brief Tx App Stream (Tas)
 *
 * @param[in]  siTRIF_Data,        TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,        TCP metadata stream from TRIF.
 * @param[out] soTRIF_DSts,        TCP data status to TRIF.
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from [STt].
 * @param[out] soTat_AcessReq,     Access request to TxAppTable (Tat).
 * @param[in]  siTat_AcessRep,     Access reply from [Tat].
 * @param[out] soMEM_TxP_WrCmd,    Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,     Tx memory data to MEM.
 * @param[out] soEVe_Event,        Event to EventEngine (EVe).
 *
 * @details
 *  This process transmits the data streams of established connections.
 *   After data and metadata are received from [TRIF], the state of the
 *  connection is checked and the segment memory pointer is loaded into the
 *  ACK pointer table.
 *   The data It then written into the DDR4 buffer memory. The application is
 *  notified through [TODO] if the write to the buffer succeeded. In case
 *  of success the length of the write is returned, otherwise -1;
 *
 ******************************************************************************/
void tx_app_stream(
        stream<AppData>            &siTRIF_Data,
        stream<AppMeta>            &siTRIF_Meta,
        stream<AppWrSts>           &soTRIF_DSts,
        stream<TcpSessId>          &soSTt_SessStateReq,
        stream<SessionState>       &siSTt_SessStateRep,
        stream<TxAppTableRequest>  &soTat_AcessReq,
        stream<TxAppTableReply>    &siTat_AcessRep,
        stream<DmCmd>              &soMEM_TxP_WrCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<Event>              &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Segment Length Generator (Slg) ---------------------------------------
    static stream<AxiWord>         ssSlgToSmw_Data    ("ssSlgToSmw_Data");
    #pragma HLS stream    variable=ssSlgToSmw_Data    depth=256
    #pragma HLS DATA_PACK variable=ssSlgToSmw_Data

    static stream<TcpSegLen>       ssSlgToMdl_SegLen  ("ssSlgToMdl_SegLen");
    #pragma HLS stream    variable=ssSlgToMdl_SegLen  depth=32

    //-- Meta Data Loader (Mdl) -----------------------------------------------
    static stream<SegMemMeta>      ssMdlToSmw_SegMeta ("ssMdlToSmw_SegMeta");
    #pragma HLS stream    variable=ssMdlToSmw_SegMeta depth=128
    #pragma HLS DATA_PACK variable=ssMdlToSmw_SegMeta

    pSegmentLengthGenerator(
            siTRIF_Data,
            ssSlgToSmw_Data,
            ssSlgToMdl_SegLen);

    pMetaDataLoader(
            siTRIF_Meta,
            soTRIF_DSts,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            soTat_AcessReq,
            siTat_AcessRep,
            ssSlgToMdl_SegLen,
            ssMdlToSmw_SegMeta,
            soEVe_Event);

    pSegmentMemoryWriter(
            ssSlgToSmw_Data,
            ssMdlToSmw_SegMeta,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data);

}
