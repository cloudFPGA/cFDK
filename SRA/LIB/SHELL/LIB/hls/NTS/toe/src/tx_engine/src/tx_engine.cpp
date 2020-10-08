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
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file       : tx_engine.cpp
 * @brief      : Tx Engine (TXe) of the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "tx_engine.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_PHC | TRACE_MDL)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOE/TXe"

#define TRACE_OFF 0x0000
#define TRACE_MDL 1 << 1
#define TRACE_IHC 1 << 2
#define TRACE_SPS 1 << 3
#define TRACE_PHC 1 << 4
#define TRACE_MRD 1 << 5
#define TRACE_TSS 1 << 6
#define TRACE_SCA 1 << 7
#define TRACE_TCA 1 << 8
#define TRACE_IPS 1 << 9
#define TRACE_ALL 0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * @brief Meta Data Loader (Mdl)
 *
 * @param[in]  siAKd_Event         Event from Ack Delayer (AKd).
 * @param[out] soRSt_RxSarReq      Read request to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep      Read reply from [RSt].
 * @param[out] soTSt_TxSarQry      TxSar query to Tx SAR Table (TSt).
 * @param[in]  siTSt_TxSarRep,     TxSar reply from [TSt].
 * @param[out] soTIm_ReTxTimerEvent Send retransmit timer event to Timers (TIm).
 * @param[out] soTIm_SetProbeTimer  Set the probe timer to [TIm].
 * @param[out] soIhc_TcpSegLen     TCP segment length to Ip Header Constructor(Ihc).
 * @param[out] soPhc_TxeMeta       Tx Engine metadata to Pseudo Header Constructor(Phc).
 * @param[out] soMrd_BufferRdCmd   Buffer read command to Memory Reader (Mrd).
 * @param[out] soSLc_ReverseLkpReq Reverse lookup request to Session Lookup Controller (SLc).
 * @param[out] soSps_IsLookup      Tells the Socket Pair Splitter (Sps) that a reverse lookup is to be expected.
 * @param[out] soTODO_IsDdrBypass  [TODO]
 * @param[out] soSps_RstSockPair   Tells the [Sps] about the socket pair to reset.
 * @param[out] soEVe_RxEventSig    Signals the reception of an event to EventEngine (EVe).
 *
 * @details
 *  The meta data loader reads the events from the Event Engine (EVe) and loads
 *   the necessary data from the metadata structures (Rx & Tx SAR Tables).
 *  Depending on the event type, it generates the necessary metadata for the
 *   'pIpHeaderConstructor' and the 'pPseudoHeaderConstructor'.
 * Additionally it requests the IP tuples from Session Lookup Controller (SLc).
 * In some special cases the IP tuple is delivered directly from the RxEngine
 *  (RXe) and it does not have to be loaded from the SLc. The 'isLookUpFifo'
 *  indicates this special cases.
 * Depending on the Event Type the retransmit or/and probe Timer is set.
 *
 *******************************************************************************/
void pMetaDataLoader(
        stream<ExtendedEvent>           &siAKd_Event,
        stream<SessionId>               &soRSt_RxSarReq,
        stream<RxSarReply>              &siRSt_RxSarRep,
        stream<TXeTxSarQuery>           &soTSt_TxSarQry,
        stream<TXeTxSarReply>           &siTSt_TxSarRep,
        stream<TXeReTransTimerCmd>      &soTIm_ReTxTimerEvent,
        stream<ap_uint<16> >            &soTIm_SetProbeTimer,
        stream<TcpSegLen>               &soIhc_TcpSegLen,
        stream<TXeMeta>                 &soPhc_TxeMeta,
        stream<DmCmd>                   &soMrd_BufferRdCmd,
        stream<SessionId>               &soSLc_ReverseLkpReq,
        stream<StsBool>                 &soSps_IsLookup,
#if (TCP_NODELAY)
        stream<bool>                    &soTODO_IsDdrBypass,
#endif
        stream<LE_SocketPair>           &soSps_RstSockPair,
        stream<SigBit>                  &soEVe_RxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { S0=0, S1 } \
                                 mdl_fsmState=S0;
    #pragma HLS RESET   variable=mdl_fsmState
    static FlagBool              mdl_sarLoaded=false;
    #pragma HLS RESET   variable=mdl_sarLoaded
    static ap_uint<2>            mdl_segmentCount=0;  // [FIXME - Too small for re-transmit?]
    #pragma HLS RESET   variable=mdl_segmentCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ExtendedEvent  mdl_curEvent;
    static RxSarReply     mdl_rxSar;
    static TXeTxSarReply  mdl_txSar;
    static ap_uint<32>    mdl_randomValue= 0x562301af; // [FIXME - Add a random Initial Sequence Number in EMIF]
    static TXeMeta        mdl_txeMeta;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpWindow             windowSize;
    TcpWindow             usableWindow;
    TcpSegLen             currLength;
    ap_uint<16>           slowstart_threshold;
    rstEvent              resetEvent;

    switch (mdl_fsmState) {
    case S0:
        if (!siAKd_Event.empty()) {
            siAKd_Event.read(mdl_curEvent);
            if (DEBUG_LEVEL & TRACE_MDL) {
                printInfo(myName, "Received event '%s' from [AKd].\n", getEventName(mdl_curEvent.type));
            }
            mdl_sarLoaded = false;
            assessSize(myName, soEVe_RxEventSig, "soEVe_RxEventSig", 2); // [FIXME-Use constant for the length]
            soEVe_RxEventSig.write(1);

            // NOT necessary for SYN/SYN_ACK only needs one
            switch(mdl_curEvent.type) {
            case RT_EVENT:
            case TX_EVENT:
            case SYN_ACK_EVENT:
            case FIN_EVENT:
            case ACK_EVENT:
            case ACK_NODELAY_EVENT:
                assessSize(myName, soRSt_RxSarReq, "soRSt_RxSarReq", 2); // [FIXME-Use constant for the length]
                soRSt_RxSarReq.write(mdl_curEvent.sessionID);
                assessSize(myName, soTSt_TxSarQry, "soTSt_TxSarQry", 2); // [FIXME-Use constant for the length]
                soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                break;
            case RST_EVENT:
                // Get txSar for SEQ numb
                resetEvent = mdl_curEvent;
                if (resetEvent.hasSessionID()) {
                    assessSize(myName, soTSt_TxSarQry, "soTSt_TxSarQry", 2); // [FIXME-Use constant for the length]
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                }
                break;
            case SYN_EVENT:
                if (mdl_curEvent.rt_count != 0) {
                    assessSize(myName, soTSt_TxSarQry, "soTSt_TxSarQry", 2); // [FIXME-Use constant for the length]
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                }
                break;
            default:
                break;
            }
            mdl_fsmState = S1;
            // [FIXME - Add a random Initial Sequence Number in EMIF and move this out of here]
            mdl_randomValue++;
        }
        mdl_segmentCount = 0;
        break;
    case S1:
        switch(mdl_curEvent.type) {
        // When Nagle's algorithm disabled; Can bypass DDR
#if (TCP_NODELAY)
        case TX:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got TX event.\n");
            if ((!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty()) || ml_sarLoaded) {
                if (!ml_sarLoaded) {
                    rxSar2txEng_rsp.read(rxSar);
                    txSar2txEng_upd_rsp.read(mdl_txSar);
                }

                // Compute our space, Advertise at least a quarter/half, otherwise 0
                windowSize = (rxSar.appd - ((ap_uint<16>)rxSar.recvd)) - 1; // This works even for wrap around
                meta.ackNumb = rxSar.recvd;
                meta.seqNumb = mdl_txSar.not_ackd;
                meta.window_size = windowSize;
                meta.ack = 1; // ACK is always set when established
                meta.rst = 0;
                meta.syn = 0;
                meta.fin = 0;

                //TODO this is hack, makes sure that probeTimer is never set.
                if (0x7FFF < ml_curEvent.length) {
                    txEng2timer_setProbeTimer.write(ml_curEvent.sessionID);
                }

                meta.length = ml_curEvent.length;

                // TODO some checking
                mdl_txSar.not_ackd += ml_curEvent.length;

                txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, mdl_txSar.not_ackd, 1));
                ml_FsmState = 0;

                // Send a packet only if there is data or we want to send an empty probing message
                if (meta.length != 0) { // || ml_curEvent.retransmit) //TODO retransmit boolean currently not set, should be removed
                    txEng_ipMetaFifoOut.write(meta.length);
                    txEng_tcpMetaFifoOut.write(meta);
                    txEng_isLookUpFifoOut.write(true);
                    txEng_isDDRbypass.write(true);
                    txEng2sLookup_rev_req.write(ml_curEvent.sessionID);

                    // Only set RT timer if we actually send sth, TODO only set if we change state and sent sth
                    txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID));
                } //TODO if probe send msg length 1
                ml_sarLoaded = true;
            }
            break;
#else
        case TX_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) {
                printInfo(myName, "Got TX event.\n");
            }
            // Send everything between txSar.not_ackd and txSar.app
            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(mdl_rxSar);
                    siTSt_TxSarRep.read(mdl_txSar);
                }
                // Compute our space, Advertise at least a quarter/half, otherwise 0
                windowSize = (mdl_rxSar.appd - ((RxBufPtr)mdl_rxSar.rcvd)) - 1; // This works even for wrap around
                mdl_txeMeta.ackNumb = mdl_rxSar.rcvd;
                mdl_txeMeta.seqNumb = mdl_txSar.not_ackd;
                mdl_txeMeta.winSize = windowSize;
                mdl_txeMeta.ack = 1; // ACK is always set when ESTABISHED
                mdl_txeMeta.rst = 0;
                mdl_txeMeta.syn = 0;
                mdl_txeMeta.fin = 0;
                mdl_txeMeta.length = 0;
                currLength = (mdl_txSar.app - ((TxBufPtr)mdl_txSar.not_ackd));
                TxBufPtr usedLength = ((TxBufPtr)mdl_txSar.not_ackd - mdl_txSar.ackd);
                if (mdl_txSar.min_window > usedLength) {
                    // FYI: min_window = Min(txSar.recv_window, mdl_txSar.cong_window)
                    usableWindow = mdl_txSar.min_window - usedLength;
                }
                else {
                    usableWindow = 0;
                }
                // Construct address before modifying mdl_txSar.not_ackd
                //  FYI - The size of the DDR4 memory of the FMKU60 is 8GB.
                //        The tx path uses the upper 4GB.
                TxMemPtr memSegAddr;  // OBSOLETE-0x40000000
                memSegAddr(31, 30) = 0x01;
                // [TODO-TODO] memSegAddr(32, 30) = 0x4;  // [FIXME- Must be configurable]
                memSegAddr(29, 16) = mdl_curEvent.sessionID(13, 0);
                memSegAddr(15,  0) = mdl_txSar.not_ackd(15, 0); //ml_curEvent.address;
                // Check length, if bigger than Usable Window or MMS
                if (currLength <= usableWindow) {
                    if (currLength >= ZYC2_MSS) {
                        //-- Start IP Fragmentation ----------------------------
                        //--  We stay in this state
                        mdl_txSar.not_ackd += ZYC2_MSS;
                        mdl_txeMeta.length  = ZYC2_MSS;
                    }
                    else {
                        //-- No IP Fragmentation or End of Fragmentation -------
                        //--  If we sent all data, we might also need to send a FIN
                        if (mdl_txSar.finReady && (mdl_txSar.ackd == mdl_txSar.not_ackd || currLength == 0)) {
                            mdl_curEvent.type = FIN_EVENT;
                        }
                        else {
                            mdl_txSar.not_ackd += currLength;
                            mdl_txeMeta.length  = currLength;
                            mdl_fsmState = S0;
                        }
                        // Update the 'mdl_txSar.not_ackd' pointer
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, mdl_txSar.not_ackd, QUERY_WR));
                    }
                }
                else {
                    // Code duplication, but better timing.
                    if (usableWindow >= ZYC2_MSS) {
                        //-- Start IP Fragmentation ----------------------------
                        //--  We stay in this state
                        mdl_txSar.not_ackd += ZYC2_MSS;
                        mdl_txeMeta.length  = ZYC2_MSS;
                    }
                    else {
                        // Check if we sent >= MSS data
                        if (mdl_txSar.ackd == mdl_txSar.not_ackd) {
                            mdl_txSar.not_ackd += usableWindow;
                            mdl_txeMeta.length = usableWindow;
                        }
                        // Set probe Timer to try again later
                        soTIm_SetProbeTimer.write(mdl_curEvent.sessionID);
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, mdl_txSar.not_ackd, QUERY_WR));
                        mdl_fsmState = S0;
                    }
                }
                if (mdl_txeMeta.length != 0) {
                    soMrd_BufferRdCmd.write(DmCmd(memSegAddr, mdl_txeMeta.length));
                }
                // Send a packet only if there is data or we want to send an empty probing message
                if (mdl_txeMeta.length != 0) { // || mdl_curEvent.retransmit) //TODO retransmit boolean currently not set, should be removed
                    soIhc_TcpSegLen.write(mdl_txeMeta.length);
                    soPhc_TxeMeta.write(mdl_txeMeta);
                    soSps_IsLookup.write(true);
                    soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                    // Only set RT timer if we actually send sth,
                    // [TODO - Only set if we change state and sent sth]
                    soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID));
                } // [TODO - if probe send msg length 1]
                mdl_sarLoaded = true;
            }
            break;
#endif
        case RT_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'RT' processing.\n"); }
            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(mdl_rxSar);
                    siTSt_TxSarRep.read(mdl_txSar);
                }
                // Compute our window size
                windowSize = (mdl_rxSar.appd - ((RxBufPtr)mdl_rxSar.rcvd)) - 1; // This works even for wrap around
                if (!mdl_txSar.finSent) // No FIN sent
                    currLength = ((TxBufPtr) mdl_txSar.not_ackd - mdl_txSar.ackd);
                else // FIN already sent
                    currLength = ((TxBufPtr) mdl_txSar.not_ackd - mdl_txSar.ackd)-1;
                mdl_txeMeta.ackNumb = mdl_rxSar.rcvd;
                mdl_txeMeta.seqNumb = mdl_txSar.ackd;
                mdl_txeMeta.winSize = windowSize;
                mdl_txeMeta.ack = 1; // ACK is always set when session is established
                mdl_txeMeta.rst = 0;
                mdl_txeMeta.syn = 0;
                mdl_txeMeta.fin = 0;
                // Construct address before modifying 'mdl_txSar.ackd'
                TxMemPtr memSegAddr;  // 0x40000000
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = mdl_curEvent.sessionID(13, 0);
                memSegAddr(15,  0) = mdl_txSar.ackd(15, 0); // mdl_curEvent.address;
                // Decrease Slow Start Threshold, only on first RT from retransmitTimer
                if (!mdl_sarLoaded && (mdl_curEvent.rt_count == 1)) {
                    if (currLength > (4*ZYC2_MSS)) // max(FlightSize/2, 2*MSS) RFC:5681
                        slowstart_threshold = currLength/2;
                    else
                        slowstart_threshold = (2 * ZYC2_MSS);
                    soTSt_TxSarQry.write(TXeTxSarRtQuery(mdl_curEvent.sessionID, slowstart_threshold));
                }
                // Since we are retransmitting from 'txSar.ackd' to 'txSar.not_ackd', this data is already inside the usableWindow
                //  => No check is required
                // Only check if length is bigger than MMS
                if (currLength > ZYC2_MSS) {
                    // We stay in this state and sent immediately another packet
                    mdl_txeMeta.length = ZYC2_MSS;
                    mdl_txSar.ackd    += ZYC2_MSS;
                    // [TODO - replace with dynamic count, remove this]
                    if (mdl_segmentCount == 3) {
                        // Should set a probe or sth??
                        //txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, mdl_txSar.not_ackd, 1));
                        mdl_fsmState = S0;
                    }
                    mdl_segmentCount++;
                }
                else {
                    mdl_txeMeta.length = currLength;
                    if (mdl_txSar.finSent)
                        mdl_curEvent.type = FIN_EVENT;
                    else
                        mdl_fsmState = S0;
                }

                // Only send a packet if there is data
                if (mdl_txeMeta.length != 0) {
                    soMrd_BufferRdCmd.write(DmCmd(memSegAddr, mdl_txeMeta.length));
                    soIhc_TcpSegLen.write(mdl_txeMeta.length);
                    soPhc_TxeMeta.write(mdl_txeMeta);
                    soSps_IsLookup.write(true);
#if (TCP_NODELAY)
                    soTODO_isDDRbypass.write(false);
#endif
                    soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                    // Only set RT timer if we actually send sth
                    soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID));
                }
                mdl_sarLoaded = true;
            }
            break;
        case ACK_EVENT:
        case ACK_NODELAY_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'ACK' processing.\n"); }
            if (!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) {
                siRSt_RxSarRep.read(mdl_rxSar);
                siTSt_TxSarRep.read(mdl_txSar);

                windowSize = (mdl_rxSar.appd - ((RxMemPtr)mdl_rxSar.rcvd)) - 1;
                mdl_txeMeta.ackNumb = mdl_rxSar.rcvd;
                mdl_txeMeta.seqNumb = mdl_txSar.not_ackd; //Always send SEQ
                mdl_txeMeta.winSize = windowSize;
                mdl_txeMeta.length  = 0;
                mdl_txeMeta.ack = 1;
                mdl_txeMeta.rst = 0;
                mdl_txeMeta.syn = 0;
                mdl_txeMeta.fin = 0;
                soIhc_TcpSegLen.write(mdl_txeMeta.length);
                soPhc_TxeMeta.write(mdl_txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                mdl_fsmState = S0;
            }
            break;
        case SYN_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'SYN' processing.\n"); }
            if (((mdl_curEvent.rt_count != 0) && !siTSt_TxSarRep.empty()) || (mdl_curEvent.rt_count == 0)) {
                if (mdl_curEvent.rt_count != 0) {
                    siTSt_TxSarRep.read(mdl_txSar);
                    mdl_txeMeta.seqNumb = mdl_txSar.ackd;
                }
                else {
                    mdl_txSar.not_ackd = mdl_randomValue; // [FIXME - Use a register from EMIF]
                    mdl_randomValue = (mdl_randomValue* 8) xor mdl_randomValue;
                    mdl_txeMeta.seqNumb = mdl_txSar.not_ackd;
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, mdl_txSar.not_ackd+1, QUERY_WR, QUERY_INIT));
                }
                mdl_txeMeta.ackNumb = 0;
                //mdl_txeMeta.seqNumb = mdl_txSar.not_ackd;
                mdl_txeMeta.winSize = 0xFFFF;
                //OBSOLETE_20200924 mdl_txeMeta.length = 0; // FYI - MSS option (will be added in Phc)
                mdl_txeMeta.length = 4; // FYI - MSS adds 4 option bytes
                mdl_txeMeta.ack = 0;
                mdl_txeMeta.rst = 0;
                mdl_txeMeta.syn = 1;
                mdl_txeMeta.fin = 0;
                soIhc_TcpSegLen.write(mdl_txeMeta.length);
                soPhc_TxeMeta.write(mdl_txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                // Set retransmit timer
                soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID, SYN_EVENT));
                mdl_fsmState = S0;
            }
            break;
        case SYN_ACK_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'SYN_ACK' processing.\n"); }
            if (!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) {
                siRSt_RxSarRep.read(mdl_rxSar);
                siTSt_TxSarRep.read(mdl_txSar);
                // Construct SYN_ACK message
                mdl_txeMeta.ackNumb = mdl_rxSar.rcvd;
                mdl_txeMeta.winSize = 0xFFFF;
                mdl_txeMeta.length  = 4; // FYI - MSS adds 4 option bytes
                mdl_txeMeta.ack     = 1;
                mdl_txeMeta.rst     = 0;
                mdl_txeMeta.syn     = 1;
                mdl_txeMeta.fin     = 0;
                if (mdl_curEvent.rt_count != 0) {
                    mdl_txeMeta.seqNumb = mdl_txSar.ackd;
                }
                else {
                    mdl_txSar.not_ackd = mdl_randomValue; // FIXME better rand();
                    mdl_randomValue = (mdl_randomValue* 8) xor mdl_randomValue;
                    mdl_txeMeta.seqNumb = mdl_txSar.not_ackd;
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID,
                                         mdl_txSar.not_ackd+1, QUERY_WR, QUERY_INIT));
                }
                soIhc_TcpSegLen.write(mdl_txeMeta.length);
                soPhc_TxeMeta.write(mdl_txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                // Set retransmit timer
                soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID, SYN_ACK_EVENT));
                mdl_fsmState = S0;
            }
            break;
        case FIN_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'FIN' processing.\n"); }
            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(mdl_rxSar);
                    siTSt_TxSarRep.read(mdl_txSar);
                }
                // Construct FIN message
                windowSize = (mdl_rxSar.appd - ((RxMemPtr)mdl_rxSar.rcvd)) - 1;
                mdl_txeMeta.ackNumb = mdl_rxSar.rcvd;
                //meta.seqNumb = mdl_txSar.not_ackd;
                mdl_txeMeta.winSize = windowSize;
                mdl_txeMeta.length = 0;
                mdl_txeMeta.ack = 1; // has to be set for FIN message as well
                mdl_txeMeta.rst = 0;
                mdl_txeMeta.syn = 0;
                mdl_txeMeta.fin = 1;

                // Check if retransmission, in case of RT, we have to reuse 'not_ackd' number
                if (mdl_curEvent.rt_count != 0)
                    mdl_txeMeta.seqNumb = mdl_txSar.not_ackd-1; // Special case, or use ackd?
                else {
                    mdl_txeMeta.seqNumb = mdl_txSar.not_ackd;
                    // Check if all data is sent, otherwise we have to delay FIN message
                    // Set FIN flag, such that probeTimer is informed
                    if (mdl_txSar.app == mdl_txSar.not_ackd(15, 0))
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, mdl_txSar.not_ackd+1,
                                             QUERY_WR, ~QUERY_INIT, true, true));
                    else
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, mdl_txSar.not_ackd,
                                             QUERY_WR, ~QUERY_INIT, true, false));
                }

                // Check if there is a FIN to be sent // [TODO - maybe restrict this]
                if (mdl_txeMeta.seqNumb(15, 0) == mdl_txSar.app) {
                    soIhc_TcpSegLen.write(mdl_txeMeta.length);
                    soPhc_TxeMeta.write(mdl_txeMeta);
                    soSps_IsLookup.write(true);
                    soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                    // Set retransmit timer
                    soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID));
                }
                mdl_fsmState = S0;
            }
            break;
        case RST_EVENT:
            if (DEBUG_LEVEL & TRACE_MDL) { printInfo(myName, "Entering the 'RST' processing.\n"); }
            // Assumption RST length == 0
            resetEvent = mdl_curEvent;
            if (!resetEvent.hasSessionID()) {
                soIhc_TcpSegLen.write(0);
                soPhc_TxeMeta.write(TXeMeta(0, resetEvent.getAckNumb(), 1, 1, 0, 0));
                soSps_IsLookup.write(false);
                soSps_RstSockPair.write(mdl_curEvent.tuple);
                mdl_fsmState = S0;
            }
            else if (!siTSt_TxSarRep.empty()) {
                siTSt_TxSarRep.read(mdl_txSar);
                soIhc_TcpSegLen.write(0);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(resetEvent.sessionID); //there is no sessionID??
                soPhc_TxeMeta.write(TXeMeta(mdl_txSar.not_ackd, resetEvent.getAckNumb(), 1, 1, 0, 0));

                mdl_fsmState = S0;
            }
            break;
        } // End of: switch
        break;
    } // End of: switch

} // End of: pMetaDataLoader

/*******************************************************************************
 * Socket Pair Splitter (Sps)
 *
 * @param[in]  siSLc_ReverseLkpRsp Reverse lookup response from SessionLookupController (SLc).
 * @param[in]  siMdl_RstSockPair   The socket pair to reset from MetaDataLoader (Mdh).
 * @param[in]  siMdl_IsLookup      Status from [Mdl] indicating that a reverse lookup is to be expected.
 * @param[out] soIhc_IpAddrPair    IP_SA and IP_DA to IpHeaderConstructor (Ihc).
 * @param[out] soPhc_SocketPair    The socket pair to PseudoHeaderConstructor (Phc).
 *
 * @details
 *  This process forwards the incoming socket-pair from the CAM or the RxEngine
 *   to both the IpHeaderConstructor (Ihc) and the PseudoHeaderConstructor (Phc).
 *******************************************************************************/
void pSocketPairSplitter(
        stream<fourTuple>       &siSLc_ReverseLkpRsp,
        stream<LE_SocketPair>   &siMdl_RstSockPair,    // [FIXME]
        stream<StsBool>         &siMdl_IsLookup,
        stream<IpAddrPair>      &soIhc_IpAddrPair,
        stream<SocketPair>      &soPhc_SocketPair)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Sps");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                sps_getMeta=false;
    #pragma HLS RESET variable=sps_getMeta

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static StsBool             sps_isLookUp;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    fourTuple      tuple;

    if (not sps_getMeta) {
        if (!siMdl_IsLookup.empty()) {
            siMdl_IsLookup.read(sps_isLookUp);
            sps_getMeta = true;
        }
    }
    else {
        if (!siSLc_ReverseLkpRsp.empty() && sps_isLookUp) {
            siSLc_ReverseLkpRsp.read(tuple);
            SocketPair socketPair(SockAddr(byteSwap32(tuple.srcIp),
                                           byteSwap16(tuple.srcPort)),
                                  SockAddr(byteSwap32(tuple.dstIp),
                                           byteSwap16(tuple.dstPort)));
            if (DEBUG_LEVEL & TRACE_SPS) {
               printInfo(myName, "Received the following socket-pair from [SLc]: \n");
               printSockPair(myName, socketPair);
            }
            soIhc_IpAddrPair.write(IpAddrPair(socketPair.src.addr, socketPair.dst.addr));
            soPhc_SocketPair.write(socketPair);
            sps_getMeta = false;
        }
        else if(!siMdl_RstSockPair.empty() && !sps_isLookUp) {
            LE_SocketPair  leSocketPair;
            siMdl_RstSockPair.read(leSocketPair);
            SocketPair     socketPair = SocketPair(SockAddr(byteSwap32(leSocketPair.src.addr),
                                                           byteSwap16(leSocketPair.src.port)),
                                                  SockAddr(byteSwap32(leSocketPair.dst.addr),
                                                           byteSwap16(leSocketPair.dst.port)));
            if (DEBUG_LEVEL & TRACE_SPS) {
               printInfo(myName, "Received the following socket-pair from [Mdl]: \n");
               printSockPair(myName, socketPair);
            }
            //OBSOLETE_202020720 soIhc_IpAddrPair.write(LE_IpAddrPair(leSockPair.src.addr, leSockPair.dst.addr));
            soIhc_IpAddrPair.write(IpAddrPair(socketPair.src.addr, socketPair.dst.addr));
            soPhc_SocketPair.write(socketPair);
            sps_getMeta = false;
        }
    }
}

/*******************************************************************************
 * @brief IPv4 Header Constructor (Ihc)
 *
 * @param[in]  siMdl_TcpSegLen   TCP segment length from Meta Data Loader (Mdl).
 * @param[in]  siSps_Ip4AddrPair The IP_SA and IP_DA from Socket Pair Splitter (Sps).
 * @param[out] soIps_IpHeader    IP4 header stream to Ip Packet Stitcher (Ips).
 *
 * @details
 *  Constructs an IPv4 header and forwards it to the IP Packet Stitcher (Ips).

 * @Warning
 *  The IP header is formatted for transmission to the Ethernet MAC. Remember
 *   that this 64-bits interface is logically divided into lane #0 (7:0) to
 *   lane #7 (63:56). The format of the outgoing IPv4 header is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |   Ident (L)   |   Ident (H)   | Total Len (L) | Total Len (H) |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    | Hd Chksum (L) | Hd Chksum (H) |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pIpHeaderConstructor(
        stream<TcpSegLen>       &siMdl_TcpSegLen,
        stream<IpAddrPair>      &siSps_IpAddrPair,
        stream<AxisIp4>         &soIPs_IpHeader)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Ihc");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<2>          ihc_chunkCounter=0;
    #pragma HLS RESET variable=ihc_chunkCounter

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static IpAddrPair          ihc_ipAddrPair;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4                    currIpHdrChunk;
    Ip4TotalLen                ip4TotLen = 0;
    TcpSegLen                  tcpSegLen;

    switch(ihc_chunkCounter) {
    case CHUNK_0:
        if (!siMdl_TcpSegLen.empty()) {
            siMdl_TcpSegLen.read(tcpSegLen);
            currIpHdrChunk.setIp4Version(4);
            currIpHdrChunk.setIp4HdrLen(5);
            currIpHdrChunk.setIp4ToS(0);
            ip4TotLen = tcpSegLen + 40;
            currIpHdrChunk.setIp4TotalLen(ip4TotLen);
            currIpHdrChunk.setIp4Ident(0);
            currIpHdrChunk.setIp4Flags(0);
            currIpHdrChunk.setIp4FragOff(0);
            currIpHdrChunk.setLE_TKeep(0xFF);
            currIpHdrChunk.setLE_TLast(0);
            if (DEBUG_LEVEL & TRACE_IHC) {
                printAxisRaw(myName, currIpHdrChunk);
            }
            soIPs_IpHeader.write(currIpHdrChunk);
            ihc_chunkCounter++;
        }
        break;
    case CHUNK_1:
        if (!siSps_IpAddrPair.empty()) {
            siSps_IpAddrPair.read(ihc_ipAddrPair);
            currIpHdrChunk.setIp4TtL(0x40);
            currIpHdrChunk.setIp4Prot((ap_uint<8>)IP4_PROT_TCP);
            currIpHdrChunk.setIp4HdrCsum(0);
            //OBSOLETE_20200706 currIpHdrChunk.tdata.range(63, 32) = ihc_leIpAddrPair.src; // Source Address
            currIpHdrChunk.setIp4SrcAddr(ihc_ipAddrPair.src);
            currIpHdrChunk.setLE_TKeep(0xFF);
            currIpHdrChunk.setLE_TLast(0);
            if (DEBUG_LEVEL & TRACE_IHC) {
                printAxisRaw(myName, currIpHdrChunk);
            }
            soIPs_IpHeader.write(currIpHdrChunk);
            ihc_chunkCounter++;
        }
        break;
    case CHUNK_2:
       //OBSOLETE_20200706 currIpHdrChunk.tdata.range(31,  0) = ihc_leIpAddrPair.dst; // Destination Address
        currIpHdrChunk.setIp4DstAddr(ihc_ipAddrPair.dst);
        currIpHdrChunk.setLE_TKeep(0x0F);
        currIpHdrChunk.setLE_TLast(TLAST);
        if (DEBUG_LEVEL & TRACE_IHC) {
            printAxisRaw(myName, currIpHdrChunk);
        }
        soIPs_IpHeader.write(currIpHdrChunk);
        ihc_chunkCounter = 0;
        break;
    }

} // End of: pIpHeaderConstructor

/*******************************************************************************
 * @brief Pseudo Header Constructor (Phc)
 *
 * @param[in]  siMdl_TxeMeta   Meta data from MetaDataLoader (Mdl).
 * @param[in]  siSps_SockPair  Socket pair from SocketPairSplitter (Sps).
 * @param[out] soTss_PseudoHdr TCP pseudo header to TcpSegmentStitcher (Tss).
 *
 * @details
 *  Reads the TCP header metadata and the IP tuples and generates a TCP pseudo
 *   header from it. Result is streamed out to the TcpSegmentStitcher (Tss).
 *
  * @Warning
 *  The pseudo header is prepared as if it was to be transmitted over the
 *   Ethernet MAC. Therefore, remember that this 64-bits interface is logically
 *   divided into lane #0 (7:0) to lane #7 (63:56). The format of the outgoing
 *   pseudo header is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |         Segment Len           |      0x06     |    0x00       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |C|E|U|A|P|R|S|F|  Data |     |N|
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |W|C|R|C|S|S|Y|I| Offset| Res |S|
 *  |               |               |               |               |               |               |R|E|G|K|H|T|N|N|       |     | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 3     |    Data 2     |    Data 1     |    Data 0     |      Opt-Data (.i.e MSS)      |   Opt-Length  |   Opt-Kind    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pPseudoHeaderConstructor(
        stream<TXeMeta>             &siMdl_TxeMeta,
        stream<SocketPair>          &siSps_SockPair,
        stream<AxisPsd4>            &soTss_PseudoHdr)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Phc");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<3>          phc_chunkCount=0;
    #pragma HLS RESET variable=phc_chunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static TXeMeta             phc_meta;
    static SocketPair          phc_socketPair;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisPsd4                   currChunk(0, 0xFF, 0);
    TcpSegLen                  pseudoHdrLen = 0;

    switch(phc_chunkCount) {
    case CHUNK_0:
        // Build and forward [ IP_DA | IP_SA ]
        if (!siSps_SockPair.empty() && !siMdl_TxeMeta.empty()) {
            siSps_SockPair.read(phc_socketPair);
            siMdl_TxeMeta.read(phc_meta);
            currChunk.setPsd4SrcAddr(phc_socketPair.src.addr);
            currChunk.setPsd4DstAddr(phc_socketPair.dst.addr);
            assessSize(myName, soTss_PseudoHdr, "soTss_PseudoHdr", 32); // [FIXME-Use constant for the length]
            soTss_PseudoHdr.write(currChunk);
            if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soTss_PseudoHdr =", currChunk); }
            phc_chunkCount++;
        }
        break;
    case CHUNK_1:
        // Build and forward [ TCP_DP | TCP_SP | SegLen | 0x06 | 0x00 ]
        currChunk.setPsd4ResBits(0x00);
        currChunk.setPsd4Prot(IP4_PROT_TCP);
        // Compute the length of the TCP segment. This includes both the header and the payload.
        //OBSOLETE_20200924 pseudoHdrLen = phc_meta.length + 20 + (phc_meta.syn << 2); // 20 + (4 for MSS)
        pseudoHdrLen = phc_meta.length + TCP_HEADER_LEN; // [FIXME]
        currChunk.setPsd4Len(pseudoHdrLen);
        currChunk.setTcpSrcPort(phc_socketPair.src.port);
        currChunk.setTcpDstPort(phc_socketPair.dst.port);
        assessSize(myName, soTss_PseudoHdr, "soTss_PseudoHdr", 32); // [FIXME-Use constant for the length]
        soTss_PseudoHdr.write(currChunk);
        if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soTss_PseudoHdr =", currChunk); }
        phc_chunkCount++;
        break;
    case CHUNK_2:
        //  Build and forward [ AckNum | SeqNum ]
        currChunk.setTcpSeqNum(phc_meta.seqNumb);
        currChunk.setTcpAckNum(phc_meta.ackNumb);
        assessSize(myName, soTss_PseudoHdr, "soTss_PseudoHdr", 32); // [FIXME-Use constant for the length]
        soTss_PseudoHdr.write(currChunk);
        if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soTss_PseudoHdr =", currChunk); }
        phc_chunkCount++;
        break;
    case CHUNK_3:
        // Build and forward  [ UrgPtr | CSum | Win | Flags | DataOffset & Res & NS ]
        currChunk.setTcpCtrlNs(0);
        currChunk.setTcpResBits(0);
        currChunk.setTcpDataOff(5 + (int)phc_meta.syn); // 5x32bits (+ 1x32bits for MSS)
        currChunk.setTcpCtrlFin(phc_meta.fin);
        currChunk.setTcpCtrlSyn(phc_meta.syn);
        currChunk.setTcpCtrlRst(phc_meta.rst);
        currChunk.setTcpCtrlPsh(0);
        currChunk.setTcpCtrlAck(phc_meta.ack);
        currChunk.setTcpCtrlUrg(0);
        currChunk.setTcpCtrlEce(0);
        currChunk.setTcpCtrlCwr(0);
        currChunk.setTcpWindow(phc_meta.winSize);
        currChunk.setTcpChecksum(0);
        currChunk.setTcpUrgPtr(0);
        currChunk.setTLast((phc_meta.length == 0) and (phc_meta.syn == 0));
        assessSize(myName, soTss_PseudoHdr, "soTss_PseudoHdr", 32); // [FIXME-Use constant for the length]
        soTss_PseudoHdr.write(currChunk);
        if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soTss_PseudoHdr =", currChunk); }
        if (!phc_meta.syn) {
            phc_chunkCount = 0;
        }
        else {
            phc_chunkCount++;
        }
        break;
    case CHUNK_4:
        // Only used for SYN and MSS negotiation
        // Build and forward [ Data 3:0 | Opt-Data | Opt-Length | Opt-Kind ]
        currChunk.setTcpOptKind(0x02);  // Option Kind = Maximum Segment Size
        currChunk.setTcpOptLen(0x04);   // Option length = 4 bytes
        currChunk.setTcpOptMss(MY_MSS); // Our Maximum Segment Size (1456)
        currChunk.setLE_TKeep(0x0F);
        currChunk.setLE_TLast(TLAST);
        assessSize(myName, soTss_PseudoHdr, "soTss_PseudoHdr", 32); // [FIXME-Use constant for the length]
        soTss_PseudoHdr.write(currChunk);
        if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soTss_PseudoHdr =", currChunk); }
        phc_chunkCount = 0;
        break;
    } // End of: switch

} // End of: pPseudoHeaderConstructor (Phc)


/*******************************************************************************
 * TCP Segment Stitcher (Tss)
 *
 * @param[in]  siPhc_PseudoHdr   Incoming chunk from PseudoHeaderConstructor (Phc).
 * @param[in]  siMEM_TxP_Data    TCP data payload from DRAM Memory (MEM).
 * @param[out] soSca_PseudoPkt   Pseudo TCP/IP packet to SubChecksumAccumulator (Sca).
 * @param[in]  siMrd_SplitSegSts Indicates that the current segment has been
 *                                splitted and stored in 2 memory buffers.
 * @details
 *  Reads in the TCP pseudo header stream from PseudoHeaderConstructor (Phc) and
 *   appends the corresponding payload stream retrieved from the memory.
 *  Note that a TCP segment might have been splitted and stored as two memory
 *   segment units. This typically happens when the address of the physical
 *   memory buffer ring wraps around.
 *******************************************************************************/
void pTcpSegStitcher(
        stream<AxisPsd4>        &siPhc_PseudoHdr,
        stream<AxisApp>         &siMEM_TxP_Data,
        #if (TCP_NODELAY)
        stream<bool>            &txEng_isDDRbypass,
        stream<axiWord>         &txApp2txEng_data_stream,
        #endif
        stream<AxisPsd4>        &soSca_PseudoPkt,
        stream<StsBit>          &siMrd_SplitSegSts)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tss");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmState {TSS_PSD_HDR=0, TSS_SYN_OPT, TSS_1ST_BUF, TSS_2ND_BUF,
                          TSS_ALIGN_2ND_BUF, TSS_RESIDUE, TSS_2ND_SEG } fsmState;
    #pragma HLS RESET             variable=fsmState
    static ap_uint<3>                      tss_state = TSS_PSD_HDR;
    #pragma HLS RESET             variable=tss_state
    static ap_uint<3>                      tss_psdHdrChunkCount = 0;
    #pragma HLS RESET             variable=tss_psdHdrChunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisApp      tss_currChunk;
    static StsBool      tss_didRdSplitSegSts;
    static ap_uint<4>   tss_memRdOffset;
    static StsBit       tss_splitSegSts;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    bool                isShortCutData = false;

    switch (tss_state) {
    case TSS_PSD_HDR:
        //-- Read first 4 chunks from [Phc]
        if (!siPhc_PseudoHdr.empty() and !soSca_PseudoPkt.full()) {
            AxisPsd4 currHdrChunk = siPhc_PseudoHdr.read();
            soSca_PseudoPkt.write(currHdrChunk);
            if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", currHdrChunk); }
            tss_didRdSplitSegSts = false;
            if (tss_psdHdrChunkCount == 3) {
                if (currHdrChunk.getTcpCtrlSyn()) {
                    tss_state = TSS_SYN_OPT;  // Segment is a SYN
                }
                else {
                    #if (TCP_NODELAY)
                        tss_state = 7;
                    #else
                        tss_state = TSS_1ST_BUF;
                    #endif
                }
                tss_psdHdrChunkCount = 0;
            }
            else {
                tss_psdHdrChunkCount++;
            }
            if (currHdrChunk.getTLast()) {
                tss_state = TSS_PSD_HDR;
                tss_psdHdrChunkCount = 0;
            }
        }
        break;
    case TSS_SYN_OPT:
        //-- Read fifth chunk from [Phc] because segment includes a TCP option (.i.e, MSS)
        if (!siPhc_PseudoHdr.empty() and !soSca_PseudoPkt.full()) {
            AxisPsd4 currHdrChunk = siPhc_PseudoHdr.read();
            soSca_PseudoPkt.write(currHdrChunk);
            if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", currHdrChunk); }
            #if (TCP_NODELAY)
                tss_state = 7;
            #else
                tss_state = TSS_1ST_BUF;
            #endif
            if (currHdrChunk.getTLast()) {
                tss_state = TSS_PSD_HDR;
            }
            tss_psdHdrChunkCount = 0;
        }
        break;
    case TSS_1ST_BUF:
        //-- Read the data chunks from the 1st memory part
        if (!siMEM_TxP_Data.empty() and !soSca_PseudoPkt.full() and
            ((tss_didRdSplitSegSts) or (!tss_didRdSplitSegSts and !siMrd_SplitSegSts.empty()))) {
            if (tss_didRdSplitSegSts == false) {
                // Read the Split Segment Status information
                //  If 'true', the TCP segment was splitted and stored as 2 memory buffers.
                tss_didRdSplitSegSts = true;
                tss_splitSegSts = siMrd_SplitSegSts.read();
            }
            siMEM_TxP_Data.read(tss_currChunk);
            if (tss_currChunk.getTLast()) {
                if (!tss_splitSegSts) {
                    // The TCP segment was not splitted and this is the last
                    // chunk of the 1st memory buffer. We are done. Go back to 'TSS_PSD_HDR'.
                    tss_state = TSS_PSD_HDR;
                    soSca_PseudoPkt.write((AxisPsd4)tss_currChunk);
                    if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", tss_currChunk); }
                }
                else {
                    //  The TCP segment was splitted in two parts.
                    tss_memRdOffset = tss_currChunk.getLen();
                    tss_splitSegSts = 0;
                    tss_currChunk.setTLast(0);
                    //OBSOLETE_20200722 if (tss_currChunk.getLE_TKeep() != 0xFF) {
                    if (tss_memRdOffset != 8) {
                        // The last chunk of the 1st memory buffer is not fully
                        // populated. Don't output anything here and go to 'TSS_JOIN_2ND'.
                        // There, we will fetch more data to fill in the current chunk.
                        tss_state = TSS_ALIGN_2ND_BUF;
                    }
                    else {
                        // The last chunk of the 1st memory buffer is populated with
                        // 8 valid bytes and is therefore also aligned.
                        // We are done with the 1st memory buffer.
                        soSca_PseudoPkt.write((AxisPsd4)tss_currChunk);
                        if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", tss_currChunk); }
                        tss_state = TSS_2ND_BUF;
                    }
                }
            }
            else {
                // Continue streaming the 1st memory buffer
                soSca_PseudoPkt.write((AxisPsd4)tss_currChunk);
                if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", tss_currChunk); }
            }
        }
        break;
    case TSS_ALIGN_2ND_BUF:
        //-- Join the bytes from the 2nd memory buffer to the stream of bytes of
        //-- the 1st memory buffer when the 1st buffer was not fully populated.
        //-- The re-alignment occurs between the previously read chunk stored
        //-- in 'prevChunk' and the latest chunk stored in 'tss_currChunk'.
        if (!siMEM_TxP_Data.empty() && !soSca_PseudoPkt.full()) {
            // Save the previous data chunk
            AxisPsd4 prevChunk = AxisPsd4(tss_currChunk.getLE_TData(), 0xFF, 0);
            // Read the next chunk from the second memory buffer
            tss_currChunk = siMEM_TxP_Data.read();
            //OBSOLETE_20200706 currPktChunk.tdata(63, (tss_memRdOffset * 8)) = segChunk.tdata(((8 - tss_memRdOffset) * 8) - 1, 0);
            prevChunk.setLE_TData(tss_currChunk.getLE_TData(((8-(int)tss_memRdOffset)*8)-1, 0),
                                                            63, (int)tss_memRdOffset*8);
            //OBSOLETE_20200706 ap_uint<4> byteCounter = keepToLen(tss_currChunk.tkeep);
            ap_uint<4> byteCounter = tss_currChunk.getLen();
            //OBSOLETE_20200722 if (byteCounter < (8 - tss_memRdOffset)) {
            if ((byteCounter + tss_memRdOffset) <= 8) {
                // The entire current chunk fits into the reminder of the previous chunk.
                // We are done with this 2nd memory buffer.
                //OBSOLETE_20200706 prevChunk.tkeep = lenToKeep(byteCounter + tss_memRdOffset);
                prevChunk.setLE_TKeep(lenToLE_tKeep(byteCounter + tss_memRdOffset));
                prevChunk.setLE_TLast(TLAST);
                tss_state = TSS_PSD_HDR;
            }
            else if (tss_currChunk.getLE_TLast()) {
                // This cannot be the last chunk because it doesn't fit into the
                // available space of the current chunk. Goto the 'TSS_RESIDUE'
                // and handle the remainder of this data chunk
                tss_state = TSS_RESIDUE;
            }
            //OBSOLETE_20200721 else {
            //OBSOLETE_20200721     tss_state = 4;
            //OBSOLETE_20200721 }
            soSca_PseudoPkt.write(prevChunk);
            if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", tss_currChunk); }
        }
        break;
    /*** OBSOLETE_20200722 ***
    case 4:
        if (!siMEM_TxP_Data.empty() && !soSca_PseudoPkt.full()) {
            // Save the current segment chunk
            AxisPsd4 currPktChunk = AxisPsd4(0, 0xFF, 0);
            //OBSOLETE_20200706 currPktChunk.tdata((tss_memRdOffset * 8) - 1, 0) = segChunk.tdata(63, (8 - tss_memRdOffset) * 8);
            currPktChunk.setLE_TData(tss_currChunk.getLE_TData(63, (8 - (int)tss_memRdOffset)*8),
                                     (int)(tss_memRdOffset * 8 - 1), 0);
            // Read the 2nd memory segment unit
            tss_currChunk = siMEM_TxP_Data.read();
            //OBSOLETE_20200706 currPktChunk.tdata(63, (8 * tss_memRdOffset)) = segChunk.tdata(((8 - tss_memRdOffset) * 8) - 1, 0);
            currPktChunk.setLE_TData(tss_currChunk.getLE_TData(((8 - (int)tss_memRdOffset) * 8) - 1, 0),
                                     63, (int)(8 * tss_memRdOffset));
            //OBSOLETE_20200706 ap_uint<4> keepCounter = keepToLen(tss_currChunk.tkeep);
            ap_uint<4> keepCounter = tss_currChunk.getLen();
            if (keepCounter < 8 - tss_memRdOffset) {
                // The entirety of this chunk fits into the reminder of the previous one.
                // We are done with this TCP segment.
                //OBSOLETE_20200706 currPktChunk.tkeep = lenToKeep(keepCounter + tss_memRdOffset);
                currPktChunk.setLE_TKeep(lenToLE_tKeep(keepCounter + tss_memRdOffset));
                currPktChunk.setLE_TLast(TLAST);
                tss_state = TSS_PSD_HDR;
            }
            else if (tss_currChunk.getLE_TLast()) {
                tss_state = 5;
            }
            soSca_PseudoPkt.write(currPktChunk);
        }
        break;
    *** OBSOLETE_20200722 ***/
    case TSS_RESIDUE:
        //-- Output the very last unaligned chunk
        if (!soSca_PseudoPkt.full()) {
            //OBSOLETE_20200706 ap_uint<4> keepCounter = keepToLen(segChunk.tkeep) - (8 - tss_memRdOffset);
            ap_uint<4> keepCounter = tss_currChunk.getLen() - (8 - tss_memRdOffset);
            AxisPsd4 lastChunk = AxisPsd4(0, lenToLE_tKeep(keepCounter), TLAST);
            //OBSOLETE_20200706 currPktChunk.tdata((tss_memRdOffset * 8) - 1, 0) = segChunk.tdata(63, (8 - tss_memRdOffset) *8);
            lastChunk.setLE_TData(tss_currChunk.getLE_TData(63, (8-(int)tss_memRdOffset)*8),
                                  ((int)tss_memRdOffset*8)-1, 0);
            soSca_PseudoPkt.write(lastChunk);
            if (DEBUG_LEVEL & TRACE_PHC) { printAxisRaw(myName, "soSca_PseudoPkt =", tss_currChunk); }
            tss_state = TSS_PSD_HDR;
        }
        break;
    #if (TCP_NODELAY)
    case 6:
        if (!txApp2txEng_data_stream.empty() && !soSca_PseudoPkt.full()) {
            txApp2txEng_data_stream.read(currWord);
            soSca_PseudoPkt.write(currWord);
            if (currWord.last) {
            	tps_state = 0;
            }
        }
        break;
    #endif
    #if (TCP_NODELAY)
    case 7:
        if (!txEng_isDDRbypass.empty()) {
            txEng_isDDRbypass.read(isShortCutData);
            if (isShortCutData) {
                tss_state = 6;
            }
            else {
                TSS_1ST_BUF;
            }
        }
        break;
    #endif
    } // End of: switch

} // End of: pTcpSegStitcher


/*******************************************************************************
 * @brief Sub-Checksum Accumulator (Sca)
 *
 * @param[in]  siTss_PseudoPkt Incoming data stream from Tc pSegment Stitcher (Tss).
 * @param[out] soIps_PseudoPkt Outgoing data stream to IP Packet Stitcher (Ips).
 * @param[out] soTca_4SubCsums Four sub-checksums to Tcp Checksum Accumulator (Tca).
 *
 * @details
 *  This process takes a TCP pseudo packet as input from the TcpSegmentStitcher
 *   (Tss) and forwards it to the IpPacketStitcher (Ips) while accumulating 4
 *    sub-checksums on the fly. When the the end-of-packet is detected, these 4
 *    sub-checksums are forwarded to the TcpChecksumAccumulator (Tca) for final
 *    checksum computation.
 *******************************************************************************/
void pSubChecksumAccumulators(
        stream<AxisPsd4>    &siTss_PseudoPkt,
        stream<AxisPsd4>    &soIps_PseudoPkt,
        stream<SubCSums>    &soTca_4SubCsums)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                sca_doForwardChunk=false;
    #pragma HLS RESET variable=sca_doForwardChunk
    static ap_uint<17>         sca_4CSums[4]={0, 0, 0, 0};
    #pragma HLS RESET variable=sca_4CSums
    #pragma HLS ARRAY_PARTITION \
                      variable=sca_4CSums complete dim=1

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisPsd4 currPktChunk;

    if (!siTss_PseudoPkt.empty()) {
        siTss_PseudoPkt.read(currPktChunk);
        if (sca_doForwardChunk) {
            // We do not forward the first chunk of the pseudo-header
            //  [FIXME] Consider forwarding all chunks and drop the 1st in the next process.
            soIps_PseudoPkt.write(currPktChunk);
        }
        for (int i = 0; i < 4; i++) {
          #pragma HLS UNROLL
            TcpCsum temp;
            if (currPktChunk.getLE_TKeep(i*2+1, i*2) == 0x3) {
                temp( 7, 0) = currPktChunk.getLE_TData(i*16+15, i*16+8);
                temp(15, 8) = currPktChunk.getLE_TData(i*16+ 7, i*16);
                sca_4CSums[i] += temp;
                sca_4CSums[i] = (sca_4CSums[i] + (sca_4CSums[i] >> 16)) & 0xFFFF;
            }
            else if (currPktChunk.getLE_TKeep()[i*2] == 0x1) {
                temp( 7, 0) = 0;
                temp(15, 8) = currPktChunk.getLE_TData(i*16+7, i*16);
                sca_4CSums[i] += temp;
                sca_4CSums[i] = (sca_4CSums[i] + (sca_4CSums[i] >> 16)) & 0xFFFF;
            }
        }

        sca_doForwardChunk = true;

        if(currPktChunk.getTLast()) {
            soTca_4SubCsums.write(sca_4CSums);
            sca_doForwardChunk = false;
            sca_4CSums[0] = 0;
            sca_4CSums[1] = 0;
            sca_4CSums[2] = 0;
            sca_4CSums[3] = 0;
        }
    }
} // End-of: Sca


/*******************************************************************************
 * @brief TCP Checksum Accumulator (Tca)
 *
 * @param[in]  siSca_FourSubCsums The 4 sub-csums from the Sub-Checksum Accumulator (Sca).
 * @param[out] soIps_TcpCsum      The computed checksum to IP Packet Stitcher (Ips).
 *
 * @details
 *  Computes the final TCP checksum from the 4 accumulated sub-checksums and
 *   forwards the results to the  IP Packet Stitcher (Ips).
 *
 *******************************************************************************/
void pTcpChecksumAccumulator(
        stream<SubCSums>       &siSca_FourSubCsums,
        stream<TcpChecksum>    &soIps_TcpCsum)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tca");

    if (!siSca_FourSubCsums.empty()) {
        SubCSums tca_4CSums = siSca_FourSubCsums.read();

        tca_4CSums.sum0 += tca_4CSums.sum2;
        tca_4CSums.sum1 += tca_4CSums.sum3;
        tca_4CSums.sum0 = (tca_4CSums.sum0 + (tca_4CSums.sum0 >> 16)) & 0xFFFF;
        tca_4CSums.sum1 = (tca_4CSums.sum1 + (tca_4CSums.sum1 >> 16)) & 0xFFFF;
        tca_4CSums.sum0 += tca_4CSums.sum1;
        tca_4CSums.sum0 = (tca_4CSums.sum0 + (tca_4CSums.sum0 >> 16)) & 0xFFFF;
        tca_4CSums.sum0 = ~tca_4CSums.sum0;

        if (DEBUG_LEVEL & TRACE_TCA) {
            printInfo(myName, "Checksum =0x%4.4X\n", tca_4CSums.sum0.to_uint());
        }
        soIps_TcpCsum.write(tca_4CSums.sum0);
    }
}


/*******************************************************************************
 * @brief IPv4 Packet Stitcher (Ips)
 *
 * @param[in]  siIhc_IpHeader  IP4 header stream from IP Header Constructor (Ihc).
 * @param[in]  siSca_PseudoPkt TCP pseudo packet stream from Sub Checksum Accumulator (Sca).
 * @param[in]  siTca_TcpCsum   TCP checksum from TCP Checksum Accumulator (Tca).
 * @param[out] soL3MUX_Data    IPv4 packet stream to layer-3 packet mux (L3MUX).
 *
 * @details
 *  Assembles an IPv4 packet from the incoming IP header stream and the TCP
 *   pseudo data packet stream from Sub Checksum Accumulator (Sca).
 *  This process also inserts the IPv4 header checksum and the TCP checksum.
 *  This is the actual IPv4 packet being streamed out by the TCP Offload Engine.
 *******************************************************************************/
void pIpPktStitcher(
        stream<AxisIp4>         &siIhc_IpHeader,
        stream<AxisPsd4>        &siSca_PseudoPkt,
        stream<TcpChecksum>     &siTca_TcpCsum,
        stream<AxisIp4>         &soL3MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Ips");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<3>          ips_chunkCount=0;
    #pragma HLS RESET variable=ips_chunkCount

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4    ip4HdrChunk;
    AxisPsd4   tcpPsdChunk;
    AxisIp4    currChunk(0, 0xFF, 0);
    TcpCsum    tcpCsum;

    switch (ips_chunkCount) {
    case CHUNK_0:
    case CHUNK_1:
        if (!siIhc_IpHeader.empty() and !soL3MUX_Data.full()) {
            siIhc_IpHeader.read(ip4HdrChunk);
            currChunk = ip4HdrChunk;
            soL3MUX_Data.write(currChunk);
            ips_chunkCount++;
            if (DEBUG_LEVEL & TRACE_IPS) { printAxisRaw(myName, "soL3MUX_Data =", currChunk); }
        }
        break;
    case CHUNK_2:
        // Start concatenating IPv4 header and TCP segment
        if (!siIhc_IpHeader.empty() && !siSca_PseudoPkt.empty() and !soL3MUX_Data.full()) {
            siIhc_IpHeader.read(ip4HdrChunk);
            siSca_PseudoPkt.read(tcpPsdChunk);
            currChunk.setIp4DstAddr(ip4HdrChunk.getIp4DstAddr());
            currChunk.setTcpSrcPort(tcpPsdChunk.getTcpSrcPort());
            currChunk.setTcpDstPort(tcpPsdChunk.getTcpDstPort());
            soL3MUX_Data.write(currChunk);
            ips_chunkCount++;
            if (DEBUG_LEVEL & TRACE_IPS) { printAxisRaw(myName, "soL3MUX_Data =", currChunk); }
        }
        break;
    case CHUNK_3:
        if (!siSca_PseudoPkt.empty() and !soL3MUX_Data.full()) {
            siSca_PseudoPkt.read(tcpPsdChunk);
            currChunk.setTcpSeqNum(tcpPsdChunk.getTcpSeqNum());
            currChunk.setTcpAckNum(tcpPsdChunk.getTcpAckNum());
            soL3MUX_Data.write(currChunk);
            ips_chunkCount++;
            if (DEBUG_LEVEL & TRACE_IPS) { printAxisRaw(myName, "soL3MUX_Data =", currChunk); }
        }
        break;
    case CHUNK_4:
        if (!siSca_PseudoPkt.empty() and !siTca_TcpCsum.empty() and !soL3MUX_Data.full()) {
            siSca_PseudoPkt.read(tcpPsdChunk);
            siTca_TcpCsum.read(tcpCsum);
            // Get CtrlBits & Window & TCP UrgPtr & Checksum
            currChunk = tcpPsdChunk;
            // Now overwrite TCP checksum
            currChunk.setTcpChecksum(tcpCsum);
            soL3MUX_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_IPS) { printAxisRaw(myName, "soL3MUX_Data =", currChunk); }
            ips_chunkCount++;
            if (tcpPsdChunk.getTLast()) {
                // This is the last chunk if/when there is no data payload
                ips_chunkCount = 0;
            }
        }
        break;
    default:
        if (!siSca_PseudoPkt.empty() and !soL3MUX_Data.full()) {
            siSca_PseudoPkt.read(tcpPsdChunk);  // TCP Data
            currChunk = tcpPsdChunk;
            soL3MUX_Data.write(currChunk);
            if (DEBUG_LEVEL & TRACE_IPS) { printAxisRaw(myName, "soL3MUX_Data =", currChunk); }
            if (tcpPsdChunk.getTLast()) {
                ips_chunkCount = 0;
            }
        }
        break;
    }
}

/*******************************************************************************
 * @brief Memory Reader (Mrd)
 *
 * @param[in]  siMdl_BufferRdCmd  Buffer read command from Meta Data Loader (Mdl).
 * @param[out] soMEM_Txp_RdCmd    Memory read command to the DRAM Memory (MEM).
 * @param[out] soTss_SplitMemAcc  Splitted memory access to Tcp Segment Stitcher (Tss).
 *
 * @details
 *  Front end memory controller for reading data from the external DRAM.
 *  This process receives a transfer read command and forwards it to the AXI4
 *   Data Mover. This command might be split in two memory accesses if the
 *   address of the data buffer to read wraps in the external memory. Such a
 *   split memory access is indicated by the signal 'soTss_SplitMemAcc'.
 *
 *******************************************************************************/
void pMemoryReader(
        stream<DmCmd>       &siMdl_BufferRdCmd,
        stream<DmCmd>       &soMEM_Txp_RdCmd,
        stream<StsBit>      &soTss_SplitMemAcc)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mrd");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static StsBool                mrd_accessBreakdown=false;
    #pragma HLS RESET    variable=mrd_accessBreakdown
    static uint16_t               mrd_dbgTxPktCounter=0;
    #pragma HLS RESET    variable=mrd_dbgTxPktCounter

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static DmCmd    mrd_memRdCmd;
    static TxBufPtr mrd_curAccLen;

    if (mrd_accessBreakdown == false) {
        if (!siMdl_BufferRdCmd.empty() && !soMEM_Txp_RdCmd.full()) { // OBSOLETE-20190909 } && soTss_SplitMemAcc.empty()) {
            mrd_memRdCmd = siMdl_BufferRdCmd.read();
            DmCmd memRdCmd = mrd_memRdCmd;
            if ((mrd_memRdCmd.saddr.range(15, 0) + mrd_memRdCmd.bbt) > 65536) {
                // This segment is broken in two memory accesses because TCP Tx memory buffer wraps around
                mrd_curAccLen = 65536 - mrd_memRdCmd.saddr;
                memRdCmd = DmCmd(mrd_memRdCmd.saddr, mrd_curAccLen);
                mrd_accessBreakdown = true;
                if (DEBUG_LEVEL & TRACE_MRD) {
                    printInfo(myName, "TCP Tx memory buffer wraps around: This segment is broken in two memory buffers.\n");
                }
            }

            soMEM_Txp_RdCmd.write(memRdCmd);
            assessSize(myName, soTss_SplitMemAcc, "soTss_SplitMemAcc", 32); // [FIXME-Use constant for the length]
            soTss_SplitMemAcc.write(mrd_accessBreakdown);
            mrd_dbgTxPktCounter++;
            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "Issuing memory read command #%d - SADDR=0x%x - BBT=0x%x\n",
                          mrd_dbgTxPktCounter, memRdCmd.saddr.to_uint(), memRdCmd.bbt.to_uint());
            }
        }
    }
    else if (mrd_accessBreakdown == true) {
        if (!soMEM_Txp_RdCmd.full()) {
            mrd_memRdCmd.saddr.range(15, 0) = 0;
            soMEM_Txp_RdCmd.write(DmCmd(mrd_memRdCmd.saddr, mrd_memRdCmd.bbt - mrd_curAccLen));
            mrd_accessBreakdown = false;
            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "Memory access breakdown: Issuing 2nd read command #%d - SADDR=0x%x - BBT=0x%x\n",
                          mrd_dbgTxPktCounter, mrd_memRdCmd.saddr.to_uint(), mrd_memRdCmd.bbt.to_uint());
            }
        }
    }
}

/*******************************************************************************
 * @brief Transmit Engine (TXe)
 *
 * @param[in]  siAKd_Event,        Event from Ack Delayer (AKd).
 * @param[out] soRSt_RxSarReq,     Read request to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep,     Read reply from [RSt].
 * @param[out] soTSt_TxSarQry,     TxSar query to TxSarTable (TSt).
 * @param[in]  siTSt_TxSarRep,     TxSar reply from [TSt].
 * @param[in]  siMEM_TxP_Data,     Data payload from the DRAM Memory (MEM).
 * @param[out] soTIm_ReTxTimerEvent, Send retransmit timer event to [Timers].
 * @param[out] soTIm_SetProbeTimer,Set probe timer to Timers (TIm).
 * @param[out] soMEM_Txp_RdCmd,    Memory read command to the DRAM Memory (MEM).
 * @param[out] soSLc_ReverseLkpReq,Reverse lookup request to Session Lookup Controller (SLc).
 * @param[in]  siSLc_ReverseLkpRep,Reverse lookup reply from SLc.
 * @param[out] soEVe_RxEventSig,   Signals the reception of an event to [EventEngine].
 * @param[out] soL3MUX_Data,       Outgoing data stream.
 *
 * @details
 *  The Tx Engine (TXe) is responsible for the generation and transmission of
 *   IPv4 packets with embedded TCP payload. It contains a state machine with
 *   a state for each Event Type. Upon reception of an event, it loads and
 *   generates the necessary metadata to construct an IPv4 packet. If that
 *   packet contains any payload, the data are retrieved from the DDR4 memory
 *   and are put as a TCP segment into the IPv4 packet. The complete packet is
 *   then streamed out over the IPv4 Tx interface of the TOE (.i.e, soL3MUX).
 *
 *******************************************************************************/
void tx_engine(
        //-- Ack Delayer & Event Engine Interfaces
        stream<ExtendedEvent>           &siAKd_Event,
        stream<SigBit>                  &soEVe_RxEventSig,
        //-- Rx SAR Table Interface
        stream<SessionId>               &soRSt_RxSarReq,
        stream<RxSarReply>              &siRSt_RxSarRep,
        //-- Tx SAR Table Interface
        stream<TXeTxSarQuery>           &soTSt_TxSarQry,
        stream<TXeTxSarReply>           &siTSt_TxSarRep,
        //-- MEM / Tx Read Path Interface
        stream<DmCmd>                   &soMEM_Txp_RdCmd,
        stream<AxisApp>                 &siMEM_TxP_Data,
        //-- Timers Interface
        stream<TXeReTransTimerCmd>      &soTIm_ReTxTimerEvent,
        stream<ap_uint<16> >            &soTIm_SetProbeTimer,
        //-- Session Lookup Controller Interface
        stream<SessionId>               &soSLc_ReverseLkpReq,
        stream<fourTuple>               &siSLc_ReverseLkpRep,
        //-- IP Tx Interface
        stream<AxisIp4>                 &soL3MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //==========================================================================
    //== LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //==========================================================================

    //--------------------------------------------------------------------------
    //-- Meta Data Loader (Mdl)
    //--------------------------------------------------------------------------
    static stream<TcpSegLen>            ssMdlToIhc_TcpSegLen    ("ssMdlToIhc_TcpSegLen");
    #pragma HLS stream         variable=ssMdlToIhc_TcpSegLen    depth=16

    static stream<TXeMeta>              ssMdlToPhc_TxeMeta      ("ssMdlToPhc_TxeMeta");
    #pragma HLS stream         variable=ssMdlToPhc_TxeMeta      depth=16
    #pragma HLS DATA_PACK      variable=ssMdlToPhc_TxeMeta

    static stream<bool>                 ssMdlToSpS_IsLookup     ("ssMdlToSpS_IsLookup");
    #pragma HLS stream         variable=ssMdlToSpS_IsLookup     depth=4

    static stream<DmCmd>                ssMdlToMrd_BufferRdCmd  ("ssMdlToMrd_BufferRdCmd");
    #pragma HLS stream         variable=ssMdlToMrd_BufferRdCmd  depth=32
    #pragma HLS DATA_PACK      variable=ssMdlToMrd_BufferRdCmd

    static stream<LE_SocketPair>        ssMdlToSps_RstSockPair  ("ssMdlToSps_RstSockPair");
    #pragma HLS stream         variable=ssMdlToSps_RstSockPair  depth=2
    #pragma HLS DATA_PACK      variable=ssMdlToSps_RstSockPair

    //--------------------------------------------------------------------------
    //-- Ip Header Construction (Ihc)
    //--------------------------------------------------------------------------
    static stream<AxisIp4>              ssIhcToIps_IpHeader     ("ssIhcToIps_IpHeader");
    #pragma HLS stream         variable=ssIhcToIps_IpHeader     depth=32
    #pragma HLS DATA_PACK      variable=ssIhcToIps_IpHeader

    //-------------------------------------------------------------------------
     //-- Pseudo Header Construction (Phc)
     //-------------------------------------------------------------------------
    static stream<AxisPsd4>             ssPhcToTss_PseudoHdr    ("ssPhcToTss_PseudoHdr");
    #pragma HLS stream         variable=ssPhcToTss_PseudoHdr    depth=32
    #pragma HLS DATA_PACK      variable=ssPhcToTss_PseudoHdr

    //-------------------------------------------------------------------------
    //-- Socket Pair Splitter (Sps)
    //-------------------------------------------------------------------------
    static stream<IpAddrPair>           ssSpsToIhc_IpAddrPair   ("ssSpsToIhc_IpAddrPair");
    #pragma HLS stream         variable=ssSpsToIhc_IpAddrPair   depth=4
    #pragma HLS DATA_PACK      variable=ssSpsToIhc_IpAddrPair

    static stream<SocketPair>           ssSpsToPhc_SockPair     ("ssSpsToPhc_SockPair");
    #pragma HLS stream         variable=ssSpsToPhc_SockPair     depth=4
    #pragma HLS DATA_PACK      variable=ssSpsToPhc_SockPair

    //-------------------------------------------------------------------------
    //-- TCP Segment Stitcher (Tss)
    //-------------------------------------------------------------------------
    static stream<AxisPsd4>             ssTssToSca_PseudoPkt    ("ssTssToSca_PseudoPkt");
    #pragma HLS stream         variable=ssTssToSca_PseudoPkt    depth=16
    #pragma HLS DATA_PACK      variable=ssTssToSca_PseudoPkt

    //-------------------------------------------------------------------------
    //-- TCP Checksum Accumulator (Tca)
    //-------------------------------------------------------------------------
    static stream<TcpChecksum>          ssTcaToIps_TcpCsum      ("ssTcaToIps_TcpCsum");
    #pragma HLS stream         variable=ssTcaToIps_TcpCsum      depth=4

    //-------------------------------------------------------------------------
    //-- Sub-Checksum Accumulator (Sca)
    //-------------------------------------------------------------------------
    static stream<AxisPsd4>             ssScaToIps_PseudoPkt    ("ssScaToIps_PseudoPkt");
    #pragma HLS stream         variable=ssScaToIps_PseudoPkt    depth=256  // WARNING: Critical; has to keep complete packet for checksum computation
    #pragma HLS DATA_PACK      variable=ssScaToIps_PseudoPkt

    static stream<SubCSums>             ssScaToTca_FourSubCsums ("ssScaToTca_FourSubCsums");
    #pragma HLS stream         variable=ssScaToTca_FourSubCsums depth=2
    #pragma HLS DATA_PACK      variable=ssScaToTca_FourSubCsums

    //------------------------------------------------------------------------
    //-- Memory Reader (Mrd)
    //------------------------------------------------------------------------
    static stream<StsBit>               ssMrdToTss_SplitMemAcc  ("ssMrdToTss_SplitMemAcc");
    #pragma HLS stream         variable=ssMrdToTss_SplitMemAcc  depth=32

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pMetaDataLoader(
            siAKd_Event,
            soRSt_RxSarReq,
            siRSt_RxSarRep,
            soTSt_TxSarQry,
            siTSt_TxSarRep,
            soTIm_ReTxTimerEvent,
            soTIm_SetProbeTimer,
            ssMdlToIhc_TcpSegLen,
            ssMdlToPhc_TxeMeta,
            ssMdlToMrd_BufferRdCmd,
            soSLc_ReverseLkpReq,
            ssMdlToSpS_IsLookup,
            ssMdlToSps_RstSockPair,
            soEVe_RxEventSig);
    
    pMemoryReader(
            ssMdlToMrd_BufferRdCmd,
            soMEM_Txp_RdCmd,
            ssMrdToTss_SplitMemAcc);

    pSocketPairSplitter(
            siSLc_ReverseLkpRep,
            ssMdlToSps_RstSockPair,
            ssMdlToSpS_IsLookup,
            ssSpsToIhc_IpAddrPair,
            ssSpsToPhc_SockPair);

    pIpHeaderConstructor(
            ssMdlToIhc_TcpSegLen,
            ssSpsToIhc_IpAddrPair,
            ssIhcToIps_IpHeader);

    pIpPktStitcher(
            ssIhcToIps_IpHeader,
            ssScaToIps_PseudoPkt,
            ssTcaToIps_TcpCsum,
            soL3MUX_Data);

    pPseudoHeaderConstructor(
            ssMdlToPhc_TxeMeta,
            ssSpsToPhc_SockPair,
            ssPhcToTss_PseudoHdr);

    pTcpSegStitcher(
            ssPhcToTss_PseudoHdr,
            siMEM_TxP_Data,
            ssTssToSca_PseudoPkt,
            ssMrdToTss_SplitMemAcc);

    pSubChecksumAccumulators(
            ssTssToSca_PseudoPkt,
            ssScaToIps_PseudoPkt,
            ssScaToTca_FourSubCsums);

    pTcpChecksumAccumulator(
            ssScaToTca_FourSubCsums,
            ssTcaToIps_TcpCsum);

}

/*! \} */
