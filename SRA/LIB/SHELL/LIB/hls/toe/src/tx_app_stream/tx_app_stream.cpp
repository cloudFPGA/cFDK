/*****************************************************************************
 * @file       : tx_app_stream.cpp
 * @brief      : Tx Application Stream (Tas) management
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
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

#define DEBUG_LEVEL (TRACE_ALL)


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
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from StateTable (STt).
 * @param[out] soTat_AccessReq,    Access request to TxAppTable (Tat).
 * @parma[in]  siTat_AccessRep,    Access reply from [Tat]
 * @param[in]  siSlg_SegLen,       The TCP segment length from SegmentLengthGenerator (slg).
 * @param[out] soTRIF_DSts,        TCP data status to TRIF.
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
        stream<TcpSessId>           &soSTt_SessStateReq,
        stream<SessionState>        &siSTt_SessStateRep,
        stream<TxAppTableRequest>   &soTat_AccessReq,
        stream<TxAppTableReply>     &siTat_AccessRep,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<ap_int<17> >         &soTRIF_DSts,
        stream<SegMemMeta>          &sMdlToSmw_SegMeta,
        stream<event>               &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "mdl");

    static enum MdlFsmStates { READ_REQUEST,  READ_META } mdlFsmState = READ_REQUEST;

    static TcpSessId       tcpSessId;
    static TxAppTableReply txAppTableReply;

    SessionState           sessState;
    TcpSegLen              segLen = 0;

    switch(mdlFsmState) {
    case READ_REQUEST:
        if (!siTRIF_Meta.empty()) {
            // Read the session ID
            siTRIF_Meta.read(tcpSessId);
            // Request state of the session
            soSTt_SessStateReq.write(tcpSessId);
            // Request the value of ACK and MemPtr from TxAppTable
            soTat_AccessReq.write(TxAppTableRequest(tcpSessId));
            if (DEBUG_LEVEL & TRACE_MDL) {
                printInfo(myName, "Received new Tx request for session %d.\n", tcpSessId.to_int());
            }
            mdlFsmState = READ_META;
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
                sMdlToSmw_SegMeta.write(SegMemMeta(true));
                // Notify App about the fail
                soTRIF_DSts.write(ERROR_NOCONNCECTION);
                printError(myName, "Session %d is not established. Current session state is %d.\n", \
                           tcpSessId.to_uint(), sessState);

                // HERE MAJOR DIFF
                // appTxDataRsp.write(appTxRsp(tasi_writeMeta.length, maxWriteLength, ERROR_NOCONNCECTION));
                // tai_state = READ_REQUEST;

            }
            else if(segLen > maxWriteLength) { // || usableWindow < tasi_writeMeta.length)
                sMdlToSmw_SegMeta.write(SegMemMeta(true));
                // Notify app about fail
                soTRIF_DSts.write(ERROR_NOSPACE);
            }
            else {
                // Here --> (sessState == ESTABLISHED && pkgLen <= maxWriteLength)
                // TODO there seems some redundancy
                sMdlToSmw_SegMeta.write(SegMemMeta(tcpSessId, txAppTableReply.mempt, segLen));
                soTRIF_DSts.write(segLen);
                //tasi_eventCacheFifo.write(eventMeta(tasi_writeSessionID, tasi_writeSar.mempt, segLen));
                soEVe_Event.write(event(TX, tcpSessId, txAppTableReply.mempt, segLen));
                // Write new MemPtr in the TxAppTable
                soTat_AccessReq.write(TxAppTableRequest(tcpSessId, txAppTableReply.mempt + segLen));
            }
            mdlFsmState = READ_REQUEST;
        }
        break;
    } //switch
}

/*******************************************************************************
 * @brief Segment Memory Writer (Smw)
 *
 * @param[in]  siSlg_Data,      TCP data stream from SegmentLengthGenerator (slg).
 * @param[in]  siMdl_SegMeta,   Segment memory metadata from MetaDataLoader (mdl).
 * @param[out] soMEM_TxP_WrCmd, Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data,  Tx memory data to MEM.
 *
 * @details
 *   Writes the incoming TCP segment into the external DRAM, unless the state
 *   machine of the MetaDataLoader (mdl) decides to drop the segment.
 *
 *******************************************************************************/
void pSegmentMemoryWriter(
        stream<AxiWord>         &siSlg_Data,
        stream<SegMemMeta>      &siMdl_SegMeta,
        stream<DmCmd>           &soMEM_TxP_WrCmd,
        stream<AxiWord>         &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush
    #pragma HLS INLINE off

	const char *myName  = concat3(THIS_NAME, "/", "smw");

    //OBSOLETE-20190614 static ap_uint<3>    tasiPkgPushState = 0;
    static enum SmwFsmStates { S0, S1, S2, S3, S4, S5 } smwFsmState = S0;

    static SegMemMeta    segMemMeta;
    static DmCmd         dmCmd = DmCmd(0, 0);
    static ap_uint<16>   breakLen = 0;
    static bool          segIsSplitted = false;
    static AxiWord       pushWord = AxiWord(0, 0xFF, 0);
    static uint8_t       lengthBuffer  = 0;
    static ap_uint<3>    accessResidue = 0;

    //OBSOLETE-20190614 static uint16_t txAppPktCounter  = 0;
    static uint32_t txAppWordCounter = 0;

    switch (smwFsmState) {
    case S0:
        if (!siMdl_SegMeta.empty() && !soMEM_TxP_WrCmd.full()) {
            siMdl_SegMeta.read(segMemMeta);
            if (!segMemMeta.drop) {
                // Build memory address for this segment in the upper 2GB
                ap_uint<32> memSegAddr; // [TODO-typedef]
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = segMemMeta.sessId(13, 0);
                memSegAddr(15,  0) = segMemMeta.addr;
                dmCmd = DmCmd(memSegAddr, segMemMeta.len);
                DmCmd tempDmCmd = dmCmd;
                if ((dmCmd.saddr.range(15, 0) + dmCmd.bbt) > 65536) {
                    breakLen = 65536 - dmCmd.saddr;
                    dmCmd.bbt -= breakLen;
                    tempDmCmd = DmCmd(dmCmd.saddr, breakLen);
                    segIsSplitted = true;
                }
                else {
                    breakLen = dmCmd.bbt;
                }
                soMEM_TxP_WrCmd.write(tempDmCmd);
            }
            smwFsmState = S1;
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
                        smwFsmState = S2;
                        accessResidue = byteCount - breakLen;
                        lengthBuffer = breakLen;  // Buffer the number of bits consumed.
                    }
                    else {
                        smwFsmState = S0;
                    }
                }
                txAppWordCounter++;
                soMEM_TxP_Data.write(outputWord);
            }
            else {
                if (pushWord.tlast == 1)
                    smwFsmState = S0;
            }
        }
        break;
    case S2:
        if (!soMEM_TxP_WrCmd.full()) {
            if (dmCmd.saddr.range(15, 0) % 8 == 0)
                smwFsmState = S3;
            //else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
            else if (dmCmd.bbt - accessResidue > 0)
                smwFsmState = S4;
            else
                smwFsmState = S5;
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
                smwFsmState = S0;
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
                        smwFsmState = S5;
                    }
                    else {
                        smwFsmState = S0;
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
                    smwFsmState = S0;
            }
        }
        break;
    case S5:
        if (!soMEM_TxP_Data.full()) {
            if (!segMemMeta.drop) {
                AxiWord outputWord = AxiWord(0, lenToKeep(breakLen), 1);
                outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
                soMEM_TxP_Data.write(outputWord);
                smwFsmState = S0;
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
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[in]  siSTt_SessStateRep, Session state reply from [STt].
 * @param[out] soTat_AcessReq,     Access request to TxAppTable (Tat).
 * @param[in]  siTat_AcessRep,     Access reply from [Tat].
 * @param[out] soTRIF_DSts,        TCP data status to TRIF.
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
        stream<TcpSessId>          &soSTt_SessStateReq,
        stream<SessionState>       &siSTt_SessStateRep,
        stream<TxAppTableRequest>  &soTat_AcessReq,
        stream<TxAppTableReply>    &siTat_AcessRep,
        stream<ap_int<17> >        &soTRIF_DSts,
        stream<DmCmd>              &soMEM_TxP_WrCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<event>              &soEVe_Event)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Segment Length Generator (Slg) ---------------------------------------
    static stream<AxiWord>         sSlgToSmw_Data    ("sSlgToSmw_Data");
    #pragma HLS stream    variable=sSlgToSmw_Data    depth=256
    #pragma HLS DATA_PACK variable=sSlgToSmw_Data

    static stream<TcpSegLen>       sSlgToMdl_SegLen  ("sSlgToMdl_SegLen");
    #pragma HLS stream    variable=sSlgToMdl_SegLen  depth=32

    //-- Meta Data Loader (Mdl) -----------------------------------------------
    static stream<SegMemMeta>      sMdlToSmw_SegMeta ("sMdlToSmw_SegMeta");
    #pragma HLS stream    variable=sMdlToSmw_SegMeta depth=128
    #pragma HLS DATA_PACK variable=sMdlToSmw_SegMeta

    pSegmentLengthGenerator(
            siTRIF_Data,
            sSlgToSmw_Data,
            sSlgToMdl_SegLen);

    pMetaDataLoader(
            siTRIF_Meta,
            soSTt_SessStateReq,
            siSTt_SessStateRep,
            soTat_AcessReq,
            siTat_AcessRep,
            sSlgToMdl_SegLen,
            soTRIF_DSts,
            sMdlToSmw_SegMeta,
            soEVe_Event);

    pSegmentMemoryWriter(
            sSlgToSmw_Data,
            sMdlToSmw_SegMeta,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data);

}
