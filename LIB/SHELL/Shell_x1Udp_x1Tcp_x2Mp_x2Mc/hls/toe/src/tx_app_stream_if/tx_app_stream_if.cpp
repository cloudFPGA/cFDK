#include "tx_app_stream_if.hpp"

using namespace hls;

/** @ingroup tx_app_stream_if
 *  Computes the size of the packet which the application wants to send
 */
void tasi_compute_pkg_size( stream<AxiWord>&            appTxDataReq,
                            stream<AxiWord>&            tasi_pkgBuffer,
                            stream<ap_uint<16> >&       tasi_pkgLenFifo) {
#pragma HLS pipeline II=1

    static ap_uint<16> tasi_pkgLen = 0;
    AxiWord currWord = AxiWord(0, 0xFF, 0);

    if (!appTxDataReq.empty()) {
        appTxDataReq.read(currWord);
        tasi_pkgBuffer.write(currWord);
        tasi_pkgLen += keepMapping(currWord.tkeep);
        if (currWord.tlast) {
            tasi_pkgLenFifo.write(tasi_pkgLen);
            tasi_pkgLen = 0;
        }
    }
}

/** @ingroup tx_app_stream_if
 *  Reads the request from the application and loads the necessary metadata,
 *  the FSM decides if the packet is written to the TX buffer or discarded.
 */
void tasi_metaLoader(   stream<ap_uint<16> >&           appTxDataReqMetaData,
                        stream<sessionState>&           stateTable2txApp_rsp,
                        stream<txAppTxSarReply>&        txSar2txApp_upd_rsp,
                        stream<ap_uint<16> >&           tasi_pkgLenFifo,
                        stream<ap_int<17> >&            appTxDataRsp,
                        stream<ap_uint<16> >&           txApp2stateTable_req,
                        stream<txAppTxSarQuery>&        txApp2txSar_upd_req,
                        stream<pkgPushMeta>&            tasi_writeToBufFifo,
                        stream<event>&                  txAppStream2eventEng_setEvent)
{
#pragma HLS pipeline II=1

    enum tai_states {READ_REQUEST, READ_META};
    static tai_states tai_state = READ_REQUEST;

    static ap_uint<16> tasi_writeSessionID;
    static ap_uint<16> tasi_maxWriteLength = 0;
    static txAppTxSarReply tasi_writeSar;

    sessionState state;
    ap_uint<16> pkgLen = 0;

    // FSM requests metadata, decides if packet goes to buffer or not
    switch(tai_state)
    {
    case READ_REQUEST:
        if (!appTxDataReqMetaData.empty())
        {
            // Read sessionID
            appTxDataReqMetaData.read(tasi_writeSessionID);
            // Get session state
            txApp2stateTable_req.write(tasi_writeSessionID);
            // Get Ack pointer
            txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeSessionID));
            tai_state = READ_META;
        }
        break;
    case READ_META:
        if (!txSar2txApp_upd_rsp.empty() && !stateTable2txApp_rsp.empty() && !tasi_pkgLenFifo.empty())
        {
            stateTable2txApp_rsp.read(state);
            txSar2txApp_upd_rsp.read(tasi_writeSar);
            tasi_pkgLenFifo.read(pkgLen);
            tasi_maxWriteLength = (tasi_writeSar.ackd - tasi_writeSar.mempt) - 1;
            if (state != ESTABLISHED)
            {
                tasi_writeToBufFifo.write(pkgPushMeta(true));
                // Notify app about fail
                appTxDataRsp.write(ERROR_NOCONNCECTION);
            }
            else if(pkgLen > tasi_maxWriteLength)
            {
                tasi_writeToBufFifo.write(pkgPushMeta(true));
                // Notify app about fail
                appTxDataRsp.write(ERROR_NOSPACE);
            }
            else //if (state == ESTABLISHED && pkgLen <= tasi_maxWriteLength)
            {
                // TODO there seems some redundancy
                tasi_writeToBufFifo.write(pkgPushMeta(tasi_writeSessionID, tasi_writeSar.mempt, pkgLen));
                appTxDataRsp.write(pkgLen);
                //tasi_eventCacheFifo.write(eventMeta(tasi_writeSessionID, tasi_writeSar.mempt, pkgLen));
                txAppStream2eventEng_setEvent.write(event(TX, tasi_writeSessionID, tasi_writeSar.mempt, pkgLen));
                txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeSessionID, tasi_writeSar.mempt + pkgLen));
            }
            tai_state = READ_REQUEST;
        }
        break;
    } //switch
}

/** @ingroup tx_app_stream_if
 *  In case the @tasi_metaLoader decides to write the packet to the memory,
 *  it writes the memory command and pushes the data to the DataMover,
 *  otherwise the packet is dropped.
 */
void tasi_pkg_pusher(
        stream<AxiWord>         &tasi_pkgBuffer,
        stream<pkgPushMeta>     &tasi_writeToBufFifo,
        stream<DmCmd>           &txBufferWriteCmd,
        stream<AxiWord>         &soMEM_TxP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1 enable_flush
    #pragma HLS INLINE off

    static ap_uint<3> tasiPkgPushState = 0;
    static pkgPushMeta tasi_pushMeta;
    static DmCmd txAppTempCmd = DmCmd(0, 0);
    static ap_uint<16> txAppBreakTemp = 0;
    static uint8_t lengthBuffer = 0;
    static ap_uint<3> accessResidue = 0;
    static bool txAppBreakdown = false;
    static AxiWord pushWord = AxiWord(0, 0xFF, 0);

    static uint16_t txAppPktCounter = 0;
    static uint32_t txAppWordCounter = 0;

    switch (tasiPkgPushState) {
    case 0:
        if (!tasi_writeToBufFifo.empty() && !txBufferWriteCmd.full()) {
            tasi_writeToBufFifo.read(tasi_pushMeta);
            if (!tasi_pushMeta.drop) {
                ap_uint<32> pkgAddr = 0x40000000;
                pkgAddr(29, 16) = tasi_pushMeta.sessionID(13, 0);
                pkgAddr(15, 0) = tasi_pushMeta.address;
                txAppTempCmd = DmCmd(pkgAddr, tasi_pushMeta.length);
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

/** @ingroup tx_app_stream_if
 *  This application interface is used to transmit data streams of established connections.
 *  The application sends the Session-ID on through @p writeMetaDataIn and the data stream
 *  on @p writeDataIn. The interface checks then the state of the connection and loads the
 *  application pointer into the memory. It then writes the data into the memory. The application
 *  is notified through @p writeReturnStatusOut if the write to the buffer succeeded. In case
 *  of success the length of the write is returned, otherwise -1;
 *  @param[in]      appTxDataReqMetaData
 *  @param[in]      appTxDataReq
 *  @param[in]      stateTable2txApp_rsp
 *  @param[in]      txSar2txApp_upd_rsp
 *  @param[out]     appTxDataRsp
 *  @param[out]     txApp2stateTable_req
 *  @param[out]     txApp2txSar_upd_req
 *  @param[out]     txBufferWriteCmd
 *  @param[out]     txBufferWriteData
 *  @param[out]     txAppStream2eventEng_setEvent
 */
void tx_app_stream_if(
        stream<ap_uint<16> >       &appTxDataReqMetaData,
        stream<AxiWord>            &appTxDataReq,
        stream<sessionState>       &stateTable2txApp_rsp,
        stream<txAppTxSarReply>    &txSar2txApp_upd_rsp, //TODO rename
        stream<ap_int<17> >        &appTxDataRsp,
        stream<ap_uint<16> >       &txApp2stateTable_req,
        stream<txAppTxSarQuery>    &txApp2txSar_upd_req, //TODO rename
        stream<DmCmd>              &txBufferWriteCmd,
        stream<AxiWord>            &soMEM_TxP_Data,
        stream<event>              &txAppStream2eventEng_setEvent)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE

    // FIFOs
    static stream<AxiWord>         tasi_pkgBuffer("tasi_pkgBuffer");
    #pragma HLS stream    variable=tasi_pkgBuffer depth=256
    #pragma HLS DATA_PACK variable=tasi_pkgBuffer

    static stream<ap_uint<16> >    tasi_pkgLenFifo("tasi_pkgLenFifo");
    #pragma HLS stream    variable=tasi_pkgLenFifo depth=32

    static stream<pkgPushMeta>     tasi_writeToBufFifo("tasi_writeToBufFifo");
    #pragma HLS stream    variable=tasi_writeToBufFifo depth=128
    #pragma HLS DATA_PACK variable=tasi_writeToBufFifo

    tasi_compute_pkg_size(
            appTxDataReq,
            tasi_pkgBuffer,
            tasi_pkgLenFifo);

    tasi_metaLoader(    appTxDataReqMetaData,
                        stateTable2txApp_rsp,
                        txSar2txApp_upd_rsp,
                        tasi_pkgLenFifo,
                        appTxDataRsp,
                        txApp2stateTable_req,
                        txApp2txSar_upd_req,
                        tasi_writeToBufFifo,
                        txAppStream2eventEng_setEvent);

    tasi_pkg_pusher(
            tasi_pkgBuffer,
            tasi_writeToBufFifo,
            txBufferWriteCmd,
            soMEM_TxP_Data);

}
