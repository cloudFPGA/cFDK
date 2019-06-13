/*****************************************************************************
 * @file       : tx_app_stream_if.cpp
 * @brief      : Tx Application Stream Interface (Tas) // [FIXME - Need a better name]
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *****************************************************************************/

#include "tx_app_stream_if.hpp"

using namespace hls;

/*******************************************************************************
 * @brief Segment Length Generator (Slg)
 *
 * @param[in]  siTRIF_Data,      TCP data stream from TRIF.
 * @param[out] soSmw_Data,       TCP data stream to SegmentMemoryWriter (Smw).
 * @param[out] soMdl_SegLen,     The length of the TCP segment to MetaDataLoader (Mdl);
 *
 * @details
 *   Generates the length of the TCP segment received from the [APP] while
 *   forwarding that same data stream to the Segment Memory Writer (Smw).
 *******************************************************************************/
void pSegmentLengthGenerator(
        stream<AppData>           &siTRIF_Data,
        stream<AxiWord>           &soSmw_Data,
        stream<ap_uint<16> >      &soMdl_SegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    static TcpSegLen   tcpSegLen = 0;

    AxiWord            currWord = AxiWord(0, 0xFF, 0);

    if (!siTRIF_Data.empty()) {
        siTRIF_Data.read(currWord);

        soSmw_Data.write(currWord);
        tcpSegLen += keepMapping(currWord.tkeep);
        if (currWord.tlast) {
            soMdl_SegLen.write(tcpSegLen);
            tcpSegLen = 0;
        }
    }
}

/******************************************************************************
 * @brief Meta Data Loader (Mdl)
 *
 * @param[in]  siTRIF_Meta,        The TCP session Id this segment belongs to.
 * @param[in]  siSTt_SessStateRep, Session state reply from StateTable (STt).
 * @
 * @param[in]  siSlg_SegLen,       The TCP segment length from SegmentLengthGenerator (Slg).
 * @
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @
 * @param[out] soSmw_SegMeta,      Segment memory metadata to SegmentMemoryWriter (Smw).
 *
 *
 * @details
 *  Reads the request from the application and loads the necessary metadata,
 *  the FSM decides if the packet is written to the TX buffer or discarded.
 *
 *  [FIXME-TODO: Implement TCP_NODELAY]
 *
 ******************************************************************************/
void pMetaDataLoader(
        stream<AppMeta>             &siTRIF_Meta,
        stream<sessionState>        &siSTt_SessStateRep,
        stream<txAppTxSarReply>     &txSar2txApp_upd_rsp,
        stream<TcpSegLen>           &siSlg_SegLen,
        stream<ap_int<17> >         &appTxDataRsp,
        stream<TcpSessId>           &soSTt_SessStateReq,
        stream<txAppTxSarQuery>     &txApp2txSar_upd_req,
        stream<SegMemMeta>          &sMdlToSmw_SegMeta,
        stream<event>               &txAppStream2eventEng_setEvent)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS pipeline II=1

    //OBSOLETE-20190611 enum tai_states {READ_REQUEST, READ_META};
    //OBSOLETE-20190611 static tai_states tai_state = READ_REQUEST;
    static enum MdlFsmStates { READ_REQUEST,  READ_META } mdlFsmState = READ_REQUEST;

    static TcpSessId       tcpSessId;
    static ap_uint<16>     tasi_maxWriteLength = 0;
    static txAppTxSarReply tasi_writeSar;

    sessionState           sessState;
    TcpSegLen              segLen = 0;

    // FSM requests metadata, decides if packet goes to buffer or not
    switch(mdlFsmState) {
    case READ_REQUEST:
        if (!siTRIF_Meta.empty()) {
            // Read the session ID
            siTRIF_Meta.read(tcpSessId);
            // Request state of the session
            soSTt_SessStateReq.write(tcpSessId);
            // Get Ack pointer
            txApp2txSar_upd_req.write(txAppTxSarQuery(tcpSessId));
            mdlFsmState = READ_META;
        }
        break;
    case READ_META:
        if (!txSar2txApp_upd_rsp.empty() && !siSTt_SessStateRep.empty() && !siSlg_SegLen.empty()) {
            siSTt_SessStateRep.read(sessState);
            txSar2txApp_upd_rsp.read(tasi_writeSar);
            siSlg_SegLen.read(segLen);
            tasi_maxWriteLength = (tasi_writeSar.ackd - tasi_writeSar.mempt) - 1;
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
                // Notify app about fail
                appTxDataRsp.write(ERROR_NOCONNCECTION);
            }
            else if(segLen > tasi_maxWriteLength)
            {
                sMdlToSmw_SegMeta.write(SegMemMeta(true));
                // Notify app about fail
                appTxDataRsp.write(ERROR_NOSPACE);
            }
            else //if (sessState == ESTABLISHED && pkgLen <= tasi_maxWriteLength)
            {
                // TODO there seems some redundancy
                sMdlToSmw_SegMeta.write(SegMemMeta(tcpSessId, tasi_writeSar.mempt, segLen));
                appTxDataRsp.write(segLen);
                //tasi_eventCacheFifo.write(eventMeta(tasi_writeSessionID, tasi_writeSar.mempt, segLen));
                txAppStream2eventEng_setEvent.write(event(TX, tcpSessId, tasi_writeSar.mempt, segLen));
                txApp2txSar_upd_req.write(txAppTxSarQuery(tcpSessId, tasi_writeSar.mempt + segLen));
            }
            mdlFsmState = READ_REQUEST;
        }
        break;
    } //switch
}

/*******************************************************************************
 * @brief Segment Memory Writer (Smw)
 *
 * @param[in]  siSlg_Data,     TCP data stream from SegmentLengthGenerator (Slg).
 * @param[in] siMdl_SegMeta,   Segment memory metadata from MetaDataLoader (Mdl).
 *
 *
 * @details
 *   Writes the incoming TCP segment into the external DRAM, unless the state
 *   machine of the MetaDataLoader (Mdl) decides to drop the segment.
 *
 *******************************************************************************/
void pSegmentMemoryWriter(
        stream<AxiWord>         &tasi_pkgBuffer,
        stream<SegMemMeta>      &siMdl_SegMeta,
        stream<DmCmd>           &txBufferWriteCmd,
        stream<AxiWord>         &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush
    #pragma HLS INLINE off

    static ap_uint<3>    tasiPkgPushState = 0;
    static SegMemMeta    tasi_pushMeta;
    static DmCmd         txAppTempCmd = DmCmd(0, 0);
    static ap_uint<16>   txAppBreakTemp = 0;
    static uint8_t       lengthBuffer = 0;
    static ap_uint<3>    accessResidue = 0;
    static bool          txAppBreakdown = false;
    static AxiWord       pushWord = AxiWord(0, 0xFF, 0);

    static uint16_t txAppPktCounter = 0;
    static uint32_t txAppWordCounter = 0;

    switch (tasiPkgPushState) {
    case 0:
        if (!siMdl_SegMeta.empty() && !txBufferWriteCmd.full()) {
            siMdl_SegMeta.read(tasi_pushMeta);
            if (!tasi_pushMeta.drop) {
                ap_uint<32> pkgAddr = 0x40000000;
                pkgAddr(29, 16) = tasi_pushMeta.sessId(13, 0);
                pkgAddr(15, 0) = tasi_pushMeta.addr;
                txAppTempCmd = DmCmd(pkgAddr, tasi_pushMeta.len);
                DmCmd tempCmd = txAppTempCmd;
                if ((txAppTempCmd.saddr.range(15, 0) + txAppTempCmd.bbt) > 65536) {
                    txAppBreakTemp = 65536 - txAppTempCmd.saddr;
                    txAppTempCmd.bbt -= txAppBreakTemp;
                    tempCmd = DmCmd(txAppTempCmd.saddr, txAppBreakTemp);
                    txAppBreakdown = true;
                }
                else
                    txAppBreakTemp = txAppTempCmd.bbt;
                txBufferWriteCmd.write(tempCmd);
                //txAppPktCounter++;
                //std::cerr <<  "1st Cmd: " << std::dec << txAppPktCounter << " - " << std::hex << tempCmd.saddr << " - " << tempCmd.bbt << std::endl;
            }
            tasiPkgPushState = 1;
        }
        break;
    case 1:
        if (!tasi_pkgBuffer.empty()) {
            tasi_pkgBuffer.read(pushWord);
            AxiWord outputWord = pushWord;
            ap_uint<4> byteCount = keepMapping(pushWord.tkeep);
            if (!tasi_pushMeta.drop) {
                if (txAppBreakTemp > 8)
                    txAppBreakTemp -= 8;
                else {
                    if (txAppBreakdown == true) {               /// Changes are to go in here
                        if (txAppTempCmd.saddr.range(15, 0) % 8 != 0) // If the word is not perfectly alligned then there is some magic to be worked.
                            outputWord.tkeep = returnKeep(txAppBreakTemp);
                        outputWord.tlast = 1;
                        tasiPkgPushState = 2;
                        accessResidue = byteCount - txAppBreakTemp;
                        lengthBuffer = txAppBreakTemp;  // Buffer the number of bits consumed.
                    }
                    else
                        tasiPkgPushState = 0;
                }
                txAppWordCounter++;
                //std::cerr <<  std::dec << cycleCounter << " - " << txAppWordCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
                AxiWord FixMe = outputWord;
                soMEM_TxP_Data.write(FixMe);
            }
            else {
                if (pushWord.tlast == 1)
                    tasiPkgPushState = 0;
            }
        }
        break;
    case 2:
        if (!txBufferWriteCmd.full()) {
            if (txAppTempCmd.saddr.range(15, 0) % 8 == 0)
                tasiPkgPushState = 3;
            //else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
            else if (txAppTempCmd.bbt - accessResidue > 0)
                tasiPkgPushState = 4;
            else
                tasiPkgPushState = 5;
            txAppTempCmd.saddr.range(15, 0) = 0;
            txAppBreakTemp = txAppTempCmd.bbt;
            txBufferWriteCmd.write(DmCmd(txAppTempCmd.saddr, txAppBreakTemp));
            //std::cerr <<  "2nd Cmd: " << std::dec << txAppPktCounter << " - " << std::hex << txAppTempCmd.saddr << " - " << txAppTempCmd.bbt << std::endl;
            txAppBreakdown = false;

        }
        break;
    case 3: // This is the non-realignment state
        if (!tasi_pkgBuffer.empty() & !soMEM_TxP_Data.full()) {
            tasi_pkgBuffer.read(pushWord);
            if (!tasi_pushMeta.drop) {
                txAppWordCounter++;
                //std::cerr <<  std::dec << cycleCounter << " - " << txAppWordCounter << " - " << std::hex << pushWord.data << " - " << pushWord.keep << " - " << pushWord.last << std::endl;
                AxiWord FixMe = pushWord;
                soMEM_TxP_Data.write(FixMe);
            }
            if (pushWord.tlast == 1)
                tasiPkgPushState = 0;
        }
        break;
    case 4: // We go into this state when we need to realign things
        if (!tasi_pkgBuffer.empty() && !soMEM_TxP_Data.full()) {
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            pushWord = tasi_pkgBuffer.read();
            outputWord.tdata.range(63, (8-lengthBuffer)*8) = pushWord.tdata.range((lengthBuffer * 8), 0 );

            if (!tasi_pushMeta.drop) {
                if (pushWord.tlast == 1) {
                    if (txAppBreakTemp - accessResidue > lengthBuffer)  { // In this case there's residue to be handled
                        txAppBreakTemp -=8;
                        tasiPkgPushState = 5;
                    }
                    else {
                        tasiPkgPushState = 0;
                        outputWord.tkeep = returnKeep(txAppBreakTemp);
                        outputWord.tlast = 1;
                    }
                }
                else
                    txAppBreakTemp -= 8;
                //txAppWordCounter++;
                //std::cerr <<  std::dec << cycleCounter << " - " << txAppWordCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
                AxiWord FixMe = outputWord;
                soMEM_TxP_Data.write(FixMe);
            }
            else {
                if (pushWord.tlast == 1)
                    tasiPkgPushState = 0;
            }
        }
        break;
    case 5:
        if (!soMEM_TxP_Data.full()) {
            if (!tasi_pushMeta.drop) {
                AxiWord outputWord = AxiWord(0, returnKeep(txAppBreakTemp), 1);
                outputWord.tdata.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
                //txAppWordCounter++;
                //std::cerr <<  std::dec << cycleCounter << " - " << txAppWordCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
                AxiWord FixMe = outputWord;
                soMEM_TxP_Data.write(FixMe);
                tasiPkgPushState = 0;
            }
        }
        break;
    } //switch
}

/*******************************************************************************
 * @brief [TODO]
 *  This application interface is used to transmit data streams of established connections.
 *
 * @param[in]  siTRIF_Data,        TCP data stream from TRIF.
 * @param[in]  siTRIF_Meta,        TCP metadata stream from TRIF.
 * @param[in]  siSTt_SessStateRep, Session state reply from StateTable (STt).
 * @param[in]      txSar2txApp_upd_rsp
 * @param[out]     appTxDataRsp
 * @param[out] soSTt_SessStateReq, Session state request to StateTable (STt).
 * @param[out]     txApp2txSar_upd_req
 * @param[out]     txBufferWriteCmd
 * @param[out]     txBufferWriteData
 * @param[out]     txAppStream2eventEng_setEvent
 *
 *  The application sends the Session-ID on through @p writeMetaDataIn and the data stream
 *  on @p writeDataIn. The interface checks then the state of the connection and loads the
 *  application pointer into the memory. It then writes the data into the memory. The application
 *  is notified through @p writeReturnStatusOut if the write to the buffer succeeded. In case
 *  of success the length of the write is returned, otherwise -1;
 *
 ******************************************************************************/
void tx_app_stream_if(
        stream<AppData>            &siTRIF_Data,
        stream<AppMeta>            &siTRIF_Meta,
        stream<sessionState>       &siSTt_SessStateRep,
        stream<txAppTxSarReply>    &txSar2txApp_upd_rsp, //TODO rename
        stream<ap_int<17> >        &appTxDataRsp,
        stream<TcpSessId>          &soSTt_SessStateReq,
        stream<txAppTxSarQuery>    &txApp2txSar_upd_req, //TODO rename
        stream<DmCmd>              &txBufferWriteCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<event>              &txAppStream2eventEng_setEvent)
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
            siSTt_SessStateRep,
            txSar2txApp_upd_rsp,
            sSlgToMdl_SegLen,
            appTxDataRsp,
            soSTt_SessStateReq,
            txApp2txSar_upd_req,
            sMdlToSmw_SegMeta,
            txAppStream2eventEng_setEvent);

    pSegmentMemoryWriter(
            sSlgToSmw_Data,
            sMdlToSmw_SegMeta,
            txBufferWriteCmd,
            soMEM_TxP_Data);

}
