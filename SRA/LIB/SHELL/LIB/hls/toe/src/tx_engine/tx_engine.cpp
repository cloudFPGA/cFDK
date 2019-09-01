/************************************************
Copyright (c) 2015, Xilinx, Inc.
Copyright (c) 2016-2019, IBM Research.

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
 * @file       : tx_engine.cpp
 * @brief      : Tx Engine (TXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.

 *****************************************************************************/

#include "../../src/tx_engine/tx_engine.hpp"
#include "../../test/test_toe_utils.hpp"

#include <algorithm>

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | TRACE_IPS)
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

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * @brief Meta Data Loader (Mdl)
 *
 * @param[in]  siAKd_Event,        Event from Ack Delayer (AKd).
 * @param[out] soRSt_RxSarReq,     Read request to RxSarTable (RSt).
 * @param[in]  siRSt_RxSarRep,     Read reply from [RSt].
 * @param[out] soTSt_TxSarQry,     TxSar query to Tx SAR Table (TSt).
 * @param[in]  siTSt_TxSarRep,     TxSar reply from [TSt].
 * @param[out] soTIm_ReTxTimerEvent, Send retransmit timer event to Timers (TIm).
 * @param[out] soTIm_SetProbeTimer,Set the probe timer to [TIm].
 * @param[out] soIhc_TcpSegLen,    TCP segment length to Ip Header Constructor(Ihc).
 * @param[out] soPhc_TxeMeta,      Tx Engine metadata to Pseudo Header Constructor(Phc).
 * @param[out] soMrd_BufferRdCmd,  Buffer read command to Memory Reader (Mrd).
 * @param[out] soSLc_ReverseLkpReq,Reverse lookup request to Session Lookup Controller (SLc).
 * @param[out] soSps_IsLookup,     Tells the Socket Pair Splitter (Sps) that a reverse lookup is to be expected.
 * @param[out] soTODO_IsDdrBypass, [TODO]
 * @param[out] soSps_RstSockPair,  Tells the [Sps] about the socket pair to reset.
 * @param[out] soEVe_RxEventSig,   Signals the reception of an event to EventEngine (EVe).
 *
 * @details
 *  The meta data loader reads the events from the Event Engine (EVe) and loads
 *   the necessary data from the metadata structures (RX & TX Sar Tables).
 *  Depending on the event type, it generates the necessary metadata for the
 *   'pIpHeaderConstruction' and the 'pPseudoHeaderConstruction'.
 * Additionally it requests the IP tuples from Session Lookup Controller (SLc).
 * In some special cases the IP tuple is delivered directly from the Rx Engine
 *  (RXe) and it does not have to be loaded from the SLc. The 'isLookUpFifo'
 *  indicates this special cases.
 * Depending on the Event Type the retransmit or/and probe Timer is set.
 *
 *****************************************************************************/
void pMetaDataLoader(
        stream<extendedEvent>           &siAKd_Event,
        stream<SessionId>               &soRSt_RxSarReq,
        stream<RxSarEntry>              &siRSt_RxSarRep,
        stream<TXeTxSarQuery>           &soTSt_TxSarQry,
        stream<TXeTxSarReply>           &siTSt_TxSarRep,
        stream<TXeReTransTimerCmd>      &soTIm_ReTxTimerEvent,
        stream<ap_uint<16> >            &soTIm_SetProbeTimer,
        stream<TcpSegLen>               &soIhc_TcpSegLen,
        stream<TXeMeta>                 &soPhc_TxeMeta,
        stream<DmCmd>                   &soMrd_BufferRdCmd,
        stream<ap_uint<16> >            &soSLc_ReverseLkpReq,
        stream<bool>                    &soSps_IsLookup,
#if (TCP_NODELAY)
        stream<bool>                    &soTODO_IsDdrBypass,
#endif
        stream<AxiSocketPair>           &soSps_RstSockPair,
        stream<SigBit>                  &soEVe_RxEventSig)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Mdl");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0, S1 } fsmState=S0;
    #pragma HLS reset       variable=fsmState
    static bool                      mdl_sarLoaded = false;
    #pragma HLS reset       variable=mdl_sarLoaded

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static extendedEvent  mdl_curEvent;
    static RxSarEntry     rxSar;
    static TXeTxSarReply  txSar;

    // [FIXME - Add a random Initial Sequence Number in EMIF]
    static ap_uint<32>    mdl_randomValue= 0x562301af;

    static ap_uint<2>     mdl_segmentCount = 0;
    static TXeMeta        txeMeta;

    TcpWindow             windowSize;
    TcpWindow             usableWindow;
    TcpSegLen             currLength;
    ap_uint<16>           slowstart_threshold;

    rstEvent              resetEvent;

    static uint16_t       txEngCounter = 0;

    switch (fsmState) {

    case S0:
        if (!siAKd_Event.empty()) {
            siAKd_Event.read(mdl_curEvent);
            mdl_sarLoaded = false;

            assessFull(myName, soEVe_RxEventSig, "soEVe_RxEventSig");
            //OBSOLETE-20190901 if (soEVe_RxEventSig.full())
            //OBSOLETE-20190901     printFatal(myName, "Trying to write stream \'soEVe_RxEventSig\' while it is full.");
            //OBSOLETE-20190901 else
            soEVe_RxEventSig.write(1);
            // NOT necessary for SYN/SYN_ACK only needs one
            switch(mdl_curEvent.type) {
            case RT:
            case TX:
            case SYN_ACK:
            case FIN:
            case ACK:
            case ACK_NODELAY:
                assessFull(myName, soRSt_RxSarReq, "soRSt_RxSarReq");
                soRSt_RxSarReq.write(mdl_curEvent.sessionID);
                assessFull(myName, soTSt_TxSarQry, "soTSt_TxSarQry");
                soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                break;
            case RST:
                // Get txSar for SEQ numb
                resetEvent = mdl_curEvent;
                if (resetEvent.hasSessionID()) {
                    assessFull(myName, soTSt_TxSarQry, "soTSt_TxSarQry");
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                }
                break;
            case SYN:
                if (mdl_curEvent.rt_count != 0) {
                    assessFull(myName, soTSt_TxSarQry, "soTSt_TxSarQry");
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID));
                }
                break;
            default:
                break;
            }
            fsmState = S1;
            // [FIXME - Add a random Initial Sequence Number in EMIF and move thsi out of here]
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
                    txSar2txEng_upd_rsp.read(txSar);
                }

                // Compute our space, Advertise at least a quarter/half, otherwise 0
                windowSize = (rxSar.appd - ((ap_uint<16>)rxSar.recvd)) - 1; // This works even for wrap around
                meta.ackNumb = rxSar.recvd;
                meta.seqNumb = txSar.not_ackd;
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
                txSar.not_ackd += ml_curEvent.length;

                txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
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
        case TX:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got TX event.\n");
            // Send everything between txSar.not_ackd and txSar.app
            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(rxSar);
                    siTSt_TxSarRep.read(txSar);
                }

                // Compute our space, Advertise at least a quarter/half, otherwise 0
                windowSize = (rxSar.appd - ((RxBufPtr)rxSar.rcvd)) - 1; // This works even for wrap around
                txeMeta.ackNumb = rxSar.rcvd;
                txeMeta.seqNumb = txSar.not_ackd;
                txeMeta.winSize = windowSize;
                txeMeta.ack = 1; // ACK is always set when ESTABISHED
                txeMeta.rst = 0;
                txeMeta.syn = 0;
                txeMeta.fin = 0;
                //OBSOLETE-20190707 txeMeta.length = 0;

                currLength = (txSar.app - ((TxBufPtr)txSar.not_ackd));
                TxBufPtr usedLength = ((TxBufPtr)txSar.not_ackd - txSar.ackd);
                if (txSar.min_window > usedLength) {
                    // FYI: min_window = Min(txSar.recv_window, txSar.cong_window)
                    usableWindow = txSar.min_window - usedLength;
                }
                else {
                    usableWindow = 0;
                }

                // Construct address before modifying txSar.not_ackd
                TxMemPtr memSegAddr;  // 0x40000000
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = mdl_curEvent.sessionID(13, 0);
                memSegAddr(15,  0) = txSar.not_ackd(15, 0); //ml_curEvent.address;

                // Check length, if bigger than Usable Window or MMS
                if (currLength <= usableWindow) {
                    if (currLength >= MSS) {
                        //-- Start IP Fragmentation ----------------------------
                        //--  We stay in this state
                        txSar.not_ackd += MSS;
                        txeMeta.length  = MSS;
                    }
                    else {
                        //-- No IP Fragmentation or End of Fragmentation -------
                        //--  If we sent all data, we might also need to send a FIN
                        if (txSar.finReady && (txSar.ackd == txSar.not_ackd || currLength == 0))
                            mdl_curEvent.type = FIN;
                        else {
                            txSar.not_ackd += currLength;
                            txeMeta.length  = currLength;
                            fsmState = S0;
                        }

                        //OBSOLETE-20190707 // Check if small segment and if unacknowledged data in pipe (Nagle)
                        //OBSOLETE-20190707 if (txSar.ackd == txSar.not_ackd) {
                        //OBSOLETE-20190707     txSar.not_ackd += currLength;
                        //OBSOLETE-20190707     txeMeta.length = currLength;
                        //OBSOLETE-20190707 }
                        //OBSOLETE-20190707 else {
                        //OBSOLETE-20190707     soTIm_SetProbeTimer.write(mdl_curEvent.sessionID);
                        //OBSOLETE-20190707 }

                        // Update the 'txSar.not_ackd' pointer
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, txSar.not_ackd, QUERY_WR));

                    }
                }
                else {
                    // Code duplication, but better timing.
                    if (usableWindow >= MSS) {
                        //-- Start IP Fragmentation ----------------------------
                        //--  We stay in this state
                        txSar.not_ackd += MSS;
                        txeMeta.length  = MSS;
                    }
                    else {
                        // Check if we sent >= MSS data
                        if (txSar.ackd == txSar.not_ackd) {
                            txSar.not_ackd += usableWindow;
                            txeMeta.length = usableWindow;
                        }
                        // Set probe Timer to try again later
                        soTIm_SetProbeTimer.write(mdl_curEvent.sessionID);
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, txSar.not_ackd, QUERY_WR));
                        fsmState = S0;
                    }
                }

                if (txeMeta.length != 0) {
                    soMrd_BufferRdCmd.write(DmCmd(memSegAddr, txeMeta.length));
                }
                // Send a packet only if there is data or we want to send an empty probing message
                if (txeMeta.length != 0) { // || mdl_curEvent.retransmit) //TODO retransmit boolean currently not set, should be removed
                    soIhc_TcpSegLen.write(txeMeta.length);
                    soPhc_TxeMeta.write(txeMeta);
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

        case RT:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got RT event.\n");
            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(rxSar);
                    siTSt_TxSarRep.read(txSar);
                }

                // Compute our window size
                windowSize = (rxSar.appd - ((RxBufPtr)rxSar.rcvd)) - 1; // This works even for wrap around
                if (!txSar.finSent) // No FIN sent
                    currLength = ((TxBufPtr) txSar.not_ackd - txSar.ackd);
                else // FIN already sent
                    currLength = ((TxBufPtr) txSar.not_ackd - txSar.ackd)-1;

                txeMeta.ackNumb = rxSar.rcvd;
                txeMeta.seqNumb = txSar.ackd;
                txeMeta.winSize = windowSize;
                txeMeta.ack = 1; // ACK is always set when session is established
                txeMeta.rst = 0;
                txeMeta.syn = 0;
                txeMeta.fin = 0;

                // Construct address before modifying 'txSar.ackd'
                TxMemPtr memSegAddr;  // 0x40000000
                memSegAddr(31, 30) = 0x01;
                memSegAddr(29, 16) = mdl_curEvent.sessionID(13, 0);
                memSegAddr(15,  0) = txSar.ackd(15, 0); // mdl_curEvent.address;

                // Decrease Slow Start Threshold, only on first RT from retransmitTimer
                if (!mdl_sarLoaded && (mdl_curEvent.rt_count == 1)) {
                    if (currLength > (4*MSS)) // max(FlightSize/2, 2*MSS) RFC:5681
                        slowstart_threshold = currLength/2;
                    else
                        slowstart_threshold = (2 * MSS);
                    soTSt_TxSarQry.write(TXeTxSarRtQuery(mdl_curEvent.sessionID, slowstart_threshold));
                }

                // Since we are retransmitting from 'txSar.ackd' to 'txSar.not_ackd', this data is already inside the usableWindow
                //  => No check is required
                // Only check if length is bigger than MMS
                if (currLength > MSS) {
                    // We stay in this state and sent immediately another packet
                    txeMeta.length = MSS;
                    txSar.ackd    += MSS;
                    // [TODO - replace with dynamic count, remove this]
                    if (mdl_segmentCount == 3) {
                        // Should set a probe or sth??
                        //txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
                        fsmState = S0;
                    }
                    mdl_segmentCount++;
                }
                else {
                    txeMeta.length = currLength;
                    if (txSar.finSent)
                        mdl_curEvent.type = FIN;
                    else
                        fsmState = S0;
                }

                // Only send a packet if there is data
                if (txeMeta.length != 0) {
                    soMrd_BufferRdCmd.write(DmCmd(memSegAddr, txeMeta.length));
                    soIhc_TcpSegLen.write(txeMeta.length);
                    soPhc_TxeMeta.write(txeMeta);
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

        case ACK:
        case ACK_NODELAY:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got ACK event.\n");
            if (!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) {
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                windowSize = (rxSar.appd - ((RxMemPtr)rxSar.rcvd)) - 1;
                txeMeta.ackNumb = rxSar.rcvd;
                txeMeta.seqNumb = txSar.not_ackd; //Always send SEQ
                txeMeta.winSize = windowSize;
                txeMeta.length  = 0;
                txeMeta.ack = 1;
                txeMeta.rst = 0;
                txeMeta.syn = 0;
                txeMeta.fin = 0;
                soIhc_TcpSegLen.write(txeMeta.length);
                soPhc_TxeMeta.write(txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                fsmState = S0;
            }
            break;

        case SYN:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got SYN event.\n");
            if (((mdl_curEvent.rt_count != 0) && !siTSt_TxSarRep.empty()) || (mdl_curEvent.rt_count == 0)) {
                if (mdl_curEvent.rt_count != 0) {
                    siTSt_TxSarRep.read(txSar);
                    txeMeta.seqNumb = txSar.ackd;
                }
                else {
                    txSar.not_ackd = mdl_randomValue; // [FIXME - Use a register from EMIF]
                    mdl_randomValue = (mdl_randomValue* 8) xor mdl_randomValue;
                    txeMeta.seqNumb = txSar.not_ackd;
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, txSar.not_ackd+1, QUERY_WR, QUERY_INIT));
                }

                txeMeta.ackNumb = 0;
                //txeMeta.seqNumb = txSar.not_ackd;
                txeMeta.winSize = 0xFFFF;
                txeMeta.length = 4; // For MSS Option, 4 bytes
                txeMeta.ack = 0;
                txeMeta.rst = 0;
                txeMeta.syn = 1;
                txeMeta.fin = 0;

                soIhc_TcpSegLen.write(4);  // For MSS Option, 4 bytes
                soPhc_TxeMeta.write(txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                // Set retransmit timer
                soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID, SYN_EVENT));
                fsmState = S0;
            }
            break;

        case SYN_ACK:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got SYN_ACK event.\n");

            if (!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) {
                siRSt_RxSarRep.read(rxSar);
                siTSt_TxSarRep.read(txSar);

                // Construct SYN_ACK message
                txeMeta.ackNumb = rxSar.rcvd;
                txeMeta.winSize = 0xFFFF;
                txeMeta.length  = 4;    // For MSS Option, 4 bytes
                txeMeta.ack     = 1;
                txeMeta.rst     = 0;
                txeMeta.syn     = 1;
                txeMeta.fin     = 0;
                if (mdl_curEvent.rt_count != 0) {
                    txeMeta.seqNumb = txSar.ackd;
                }
                else {
                    txSar.not_ackd = mdl_randomValue; // FIXME better rand();
                    mdl_randomValue = (mdl_randomValue* 8) xor mdl_randomValue;
                    txeMeta.seqNumb = txSar.not_ackd;
                    soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID,
                                         txSar.not_ackd+1, QUERY_WR, QUERY_INIT));
                }

                soIhc_TcpSegLen.write(txeMeta.length);
                soPhc_TxeMeta.write(txeMeta);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);

                // Set retransmit timer
                soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID, SYN_ACK_EVENT));
                fsmState = S0;
            }
            break;

        case FIN:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got FIN event.\n");

            if ((!siRSt_RxSarRep.empty() && !siTSt_TxSarRep.empty()) || mdl_sarLoaded) {
                if (!mdl_sarLoaded) {
                    siRSt_RxSarRep.read(rxSar);
                    siTSt_TxSarRep.read(txSar);
                }
                // Construct FIN message
                windowSize = (rxSar.appd - ((RxMemPtr)rxSar.rcvd)) - 1;
                txeMeta.ackNumb = rxSar.rcvd;
                //meta.seqNumb = txSar.not_ackd;
                txeMeta.winSize = windowSize;
                txeMeta.length = 0;
                txeMeta.ack = 1; // has to be set for FIN message as well
                txeMeta.rst = 0;
                txeMeta.syn = 0;
                txeMeta.fin = 1;

                // Check if retransmission, in case of RT, we have to reuse 'not_ackd' number
                if (mdl_curEvent.rt_count != 0)
                    txeMeta.seqNumb = txSar.not_ackd-1; // Special case, or use ackd?
                else {
                    txeMeta.seqNumb = txSar.not_ackd;
                    // Check if all data is sent, otherwise we have to delay FIN message
                    // Set FIN flag, such that probeTimer is informed
                    if (txSar.app == txSar.not_ackd(15, 0))
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, txSar.not_ackd+1,
                                             QUERY_WR, ~QUERY_INIT, true, true));
                    else
                        soTSt_TxSarQry.write(TXeTxSarQuery(mdl_curEvent.sessionID, txSar.not_ackd,
                                             QUERY_WR, ~QUERY_INIT, true, false));
                }

                // Check if there is a FIN to be sent // [TODO - maybe restrict this]
                if (txeMeta.seqNumb(15, 0) == txSar.app) {
                    soIhc_TcpSegLen.write(txeMeta.length);
                    soPhc_TxeMeta.write(txeMeta);
                    soSps_IsLookup.write(true);
                    soSLc_ReverseLkpReq.write(mdl_curEvent.sessionID);
                    // Set retransmit timer
                    soTIm_ReTxTimerEvent.write(TXeReTransTimerCmd(mdl_curEvent.sessionID));
                }
                fsmState = S0;
            }
            break;

        case RST:
            if (DEBUG_LEVEL & TRACE_MDL)
                printInfo(myName, "Got RST event.\n");

            // Assumption RST length == 0
            resetEvent = mdl_curEvent;
            if (!resetEvent.hasSessionID()) {
                soIhc_TcpSegLen.write(0);
                soPhc_TxeMeta.write(TXeMeta(0, resetEvent.getAckNumb(), 1, 1, 0, 0));
                soSps_IsLookup.write(false);
                soSps_RstSockPair.write(mdl_curEvent.tuple);
                fsmState = S0;
            }
            else if (!siTSt_TxSarRep.empty()) {
                siTSt_TxSarRep.read(txSar);
                soIhc_TcpSegLen.write(0);
                soSps_IsLookup.write(true);
                soSLc_ReverseLkpReq.write(resetEvent.sessionID); //there is no sessionID??
                soPhc_TxeMeta.write(TXeMeta(txSar.not_ackd, resetEvent.getAckNumb(), 1, 1, 0, 0));

                fsmState = S0;
            }
            break;

        } // End of: switch
        break;

    } // End of: switch

} // End of: pMetaDataLoader


/*****************************************************************************
 * Socket Pair Splitter (Sps)
 *
 * @param[in]  siSLc_ReverseLkpRep, Reverse lookup reply from Session Lookup Controller (SLc).
 * @param[in]  siMdl_RstSockPair,   The socket pair to reset from Meta Data Loader (Mdh).
 * @param[in]  txEng_isLookUpFifoIn
 * @param[out] soIhc_IpAddrPair,    IP_SA and IP_DA to Ip Header Constructor (Ihc).
 * @param[out] soPhc_SocketPair,    The socket pair to Pseudo Header Constructor (Phc).
 *
 * @details
 *  Forwards the incoming tuple from the SmartCam or RX Engine to the 2 header
 *   construction modules.
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pSocketPairSplitter(
        stream<fourTuple>       &siSLc_ReverseLkpRep,
        stream<AxiSocketPair>   &siMdl_RstSockPair,
        stream<bool>            &txEng_isLookUpFifoIn,
        stream<IpAddrPair>      &soIhc_IpAddrPair,
        stream<AxiSocketPair>   &soPhc_SocketPair)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Sps");

	static bool ts_getMeta = true;
    static bool ts_isLookUp;

    fourTuple      tuple;  // [FIXME - Update to SocketPair]
    AxiSocketPair  sockPair;

    if (ts_getMeta) {
        if (!txEng_isLookUpFifoIn.empty()) {
            txEng_isLookUpFifoIn.read(ts_isLookUp);
            ts_getMeta = false;
        }
    }
    else {
        if (!siSLc_ReverseLkpRep.empty() && ts_isLookUp) {
            siSLc_ReverseLkpRep.read(tuple);
            AxiSocketPair axiSocketPair(AxiSockAddr(tuple.srcIp, tuple.srcPort),
                                        AxiSockAddr(tuple.dstIp, tuple.dstPort));
            if (DEBUG_LEVEL & TRACE_SPS) {
               printInfo(myName, "Received the following socket-pair from [SLc]: \n");
               //OBSOLETE-20190620 printAxiSockPair(myName, axiSocketPair);
               printSockPair(myName, axiSocketPair);
            }
            soIhc_IpAddrPair.write(IpAddrPair(axiSocketPair.src.addr, axiSocketPair.dst.addr));
            soPhc_SocketPair.write(axiSocketPair);
            ts_getMeta = true;
        }
        else if(!siMdl_RstSockPair.empty() && !ts_isLookUp) {
            siMdl_RstSockPair.read(sockPair);
            soIhc_IpAddrPair.write(IpAddrPair(sockPair.src.addr, sockPair.dst.addr));
            soPhc_SocketPair.write(sockPair);
            ts_getMeta = true;
        }
    }
}


/*****************************************************************************
 * @brief IPv4 Header Construction (Ihc)
 *
 * @param[in]  siMdl_TcpSegLen, TCP segment length from Meta Data Laoder (Mdl).
 * @param[in]  siSps_Ip4Tuple,  The IP_SA and IP_DA from Socket Pair Splitter (Sps).
 * @param[out] soIps_Ip4Word,   IP4 word to Ip packet Stitcher (Ips).
 *
 * @details
 *  Constructs an IPv4 header and forwards it to the IP Packet Stitcher (Ips).

 * @Warning
 *  The IP header is formatted for transmission to the Ethernet MAC. Remember
 *   that this 64-bits interface is logically divided into lane #0 (7:0) to
 *   lane #7 (63:56). The format of te outgoing IPv4 header is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |   Ident (L)   |   Ident (H)   | Total Len (L) | Total Len (H) |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    | Hd Chksum (L) | Hd Chksum (H) |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pIpHeaderConstructor(
        stream<TcpSegLen>       &siMdl_TcpSegLen,
        stream<IpAddrPair>      &siSps_IpAddrPair,
        stream<Ip4Word>         &soIPs_Ip4Word)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Ihc");

    static ap_uint<2>   ihc_wordCounter = 0;
    static IpAddrPair   ihc_ipAddrPair;

    Ip4Word             ip4HdrWord;
    Ip4TotalLen         ip4TotLen = 0;
    TcpSegLen           tcpSegLen;

    switch(ihc_wordCounter) {

    case WORD_0:
        if (!siMdl_TcpSegLen.empty()) {
            siMdl_TcpSegLen.read(tcpSegLen);

            ip4HdrWord.tdata.range( 7,  0) = 0x45;    // Version+IHL
            ip4HdrWord.tdata.range(15,  8) = 0;       // ToS
            ip4TotLen = tcpSegLen + 40;
            ip4HdrWord.tdata.range(31, 16) = byteSwap16(ip4TotLen); // Total Length //OBSOLETE-20181127 ip4HdrWord.data.range(23, 16) = ip4TotLen(15, 8); //OBSOLETE-20181127 ip4HdrWord.data.range(31, 24) = ip4TotLen( 7, 0);
            ip4HdrWord.tdata.range(47, 32) = 0;       // Identification
            ip4HdrWord.tdata.range(50, 48) = 0;       //Flags
            ip4HdrWord.tdata.range(63, 51) = 0x0;     //Fragment Offset
            ip4HdrWord.tkeep = 0xFF;
            ip4HdrWord.tlast = 0;
            if (DEBUG_LEVEL & TRACE_IHC) printAxiWord(myName, ip4HdrWord);

            soIPs_Ip4Word.write(ip4HdrWord);
            ihc_wordCounter++;
        }
        break;

    case WORD_1:
        if (!siSps_IpAddrPair.empty()) {
            siSps_IpAddrPair.read(ihc_ipAddrPair);

            ip4HdrWord.tdata.range( 7,  0) = 0x40;    // Time to Live
            ip4HdrWord.tdata.range(15,  8) = 0x06;    // Protocol (TCP)
            ip4HdrWord.tdata.range(31, 16) = 0;       // Header Checksum
            ip4HdrWord.tdata.range(63, 32) = ihc_ipAddrPair.src; // Source Address
            ip4HdrWord.tkeep = 0xFF;
            ip4HdrWord.tlast = 0;
            if (DEBUG_LEVEL & TRACE_IHC) printAxiWord(myName, ip4HdrWord);

            soIPs_Ip4Word.write(ip4HdrWord);
            ihc_wordCounter++;
        }
        break;

    case WORD_2:
        ip4HdrWord.tdata.range(31,  0) = ihc_ipAddrPair.dst; // Destination Address
        ip4HdrWord.tkeep = 0x0F;
        ip4HdrWord.tlast = 1;
        if (DEBUG_LEVEL & TRACE_IHC) printAxiWord(myName, ip4HdrWord);

        soIPs_Ip4Word.write(ip4HdrWord);
        ihc_wordCounter = 0;
        break;

    }
} // End of: pIpHeaderConstructor


/*****************************************************************************
 * @brief Pseudo Header Constructor (Phc)
 *
 * @param[in]  siMdl_TxeMeta,  Meta data from Meta Data Loader (Mdl).
 * @param[in]  siSps_SockPair, Socket pair from Socket Pair Splitter (Sps).
 * @param[out] soTss_TcpWord,  Outgoing TCP word to TCP Segment Stitcher (Tss).
 *
 * @details
 *  Reads the TCP header metadata and the IP tuples and generates a TCP pseudo
 *   header from it. Result is streamed out to the TCP Segment Stitcher (Tss).
 *
  * @Warning
 *  The pseudo header is prepared for transmission to the Ethernet MAC.
 *   Remember that this 64-bits interface is logically divided into lane #0
 *   (7:0) to lane #7 (63:56). The format of the outgoing psudo header is then:
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
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pPseudoHeaderConstructor(
        stream<TXeMeta>             &siMdl_TxeMeta,
        stream<AxiSocketPair>       &siSps_SockPair,
        stream<AxiWord>             &soTss_TcpWord)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Phc");

    static uint16_t       phc_wordCount = 0;
    TcpWord               sendWord;
    static TXeMeta        phc_meta;
    static AxiSocketPair  phc_sockPair;
    TcpSegLen             pseudoHdrLen = 0;

    //OBSOLETE-20181130 if (phc_done && !siMdl_TxeMeta.empty()) {
    //OBSOLETE-20181130     siMdl_TxeMeta.read(phc_meta);
    //OBSOLETE-20181130     phc_done = false;
    //OBSOLETE-20181130 }
    //OBSOLETE-20181130 else if (!phc_done) {
        switch(phc_wordCount) {

        case WORD_0:
            if (!siSps_SockPair.empty() && !siMdl_TxeMeta.empty()) {
                siSps_SockPair.read(phc_sockPair);
                siMdl_TxeMeta.read(phc_meta);

                sendWord.tdata.range(31,  0) = phc_sockPair.src.addr;  // IP4 Source Address
                sendWord.tdata.range(63, 32) = phc_sockPair.dst.addr;  // IP4 Destination Addr
                sendWord.tkeep = 0xFF;
                sendWord.tlast = 0;

                soTss_TcpWord.write(sendWord);
                if (DEBUG_LEVEL & TRACE_PHC) printAxiWord(myName, sendWord);
                phc_wordCount++;
            }
            break;

        case WORD_1:
            sendWord.tdata.range( 7, 0) = 0x00;     // TCP Time to Live
            sendWord.tdata.range(15, 8) = 0x06;     // TCP Protocol
            pseudoHdrLen = phc_meta.length + 0x14;  // Add 20 bytes for the TCP header

            sendWord.tdata.range(31,16) = byteSwap16(pseudoHdrLen);  //OBSOLETE-20181128 sendWord.data.range(23, 16) = length(15, 8);
                                                                     //OBSOLETE-20181128 sendWord.data.range(31, 24) = length(7, 0);
            sendWord.tdata.range(47, 32) = phc_sockPair.src.port;   // Source Port
            sendWord.tdata.range(63, 48) = phc_sockPair.dst.port;   // Destination Port
            sendWord.tkeep = 0xFF;
            sendWord.tlast = 0;

            soTss_TcpWord.write(sendWord);
            if (DEBUG_LEVEL & TRACE_PHC) printAxiWord(myName, sendWord);
            phc_wordCount++;
            break;

        case WORD_2:
            // Insert SEQ number
        	//OBSOLETE-20181128 sendWord.data(7, 0)   = phc_meta.seqNumb(31, 24);
        	//OBSOLETE-20181128 sendWord.data(15, 8)  = phc_meta.seqNumb(23, 16);
        	//OBSOLETE-20181128 sendWord.data(23, 16) = phc_meta.seqNumb(15, 8);
        	//OBSOLETE-20181128 sendWord.data(31, 24) = phc_meta.seqNumb(7, 0);
            sendWord.tdata(31,  0) = byteSwap32(phc_meta.seqNumb);
            // Insert ACK number
            //OBSOLETE-20181128 sendWord.data(39, 32) = phc_meta.ackNumb(31, 24);
            //OBSOLETE-20181128 sendWord.data(47, 40) = phc_meta.ackNumb(23, 16);
            //OBSOLETE-20181128 sendWord.data(55, 48) = phc_meta.ackNumb(15, 8);
            //OBSOLETE-20181128 sendWord.data(63, 56) = phc_meta.ackNumb(7, 0);
            sendWord.tdata(63, 32) = byteSwap32(phc_meta.ackNumb);
            sendWord.tkeep = 0xFF;
            sendWord.tlast = 0;

            soTss_TcpWord.write(sendWord);
            if (DEBUG_LEVEL & TRACE_PHC) printAxiWord(myName, sendWord);
            phc_wordCount++;
            break;

        case WORD_3:
            sendWord.tdata(3, 1) = 0; // Reserved
            sendWord.tdata(7, 4) = (0x5 + phc_meta.syn); // Data Offset (+1 for MSS)
            /* Control bits:
             * [ 8] == FIN
             * [ 9] == SYN
             * [10] == RST
             * [11] == PSH
             * [12] == ACK
             * [13] == URG
             */
            sendWord.tdata[8]  = phc_meta.fin;  // Control bits
            sendWord.tdata[9]  = phc_meta.syn;
            sendWord.tdata[10] = phc_meta.rst;
            sendWord.tdata[11] = 0;
            sendWord.tdata[12] = phc_meta.ack;
            sendWord.tdata(15, 13) = 0;
            //OBSOLETE-20181128 sendWord.data.range(23, 16) = phc_meta.winSize(15, 8);
            //OBSOLETE-20181128 sendWord.data.range(31, 24) = phc_meta.winSize( 7, 0);
            sendWord.tdata.range(31, 16) = byteSwap16(phc_meta.winSize);
            sendWord.tdata.range(63, 32) = 0; // Urgent pointer & Checksum
            sendWord.tkeep = 0xFF;
            sendWord.tlast = (phc_meta.length == 0);

            soTss_TcpWord.write(sendWord);
            if (DEBUG_LEVEL & TRACE_PHC) printAxiWord(myName, sendWord);

            if (!phc_meta.syn)
                phc_wordCount = 0;
            else
                phc_wordCount++;
            //OBSOLETE-20181130 phc_done = true;
            break;

        case WORD_4: // Only used for SYN and MSS negotiation
            sendWord.tdata( 7,  0) = 0x02;   // Option Kind = Maximum Segment Size
            sendWord.tdata(15,  8) = 0x04;   // Option length
            sendWord.tdata(31, 16) = 0xB405; // 0x05B4 = 1460
            sendWord.tdata(63, 32) = 0;
            sendWord.tkeep = 0x0F;
            sendWord.tlast = 1; // (phc_meta.length == 0x04); // OR JUST SET TO 1

            soTss_TcpWord.write(sendWord);
            if (DEBUG_LEVEL & TRACE_PHC) printAxiWord(myName, sendWord);
            phc_wordCount = 0;
        break;

        } // End of: switch
        //OBSOLETE-20181130    }
} // End of: pPseudoHeaderConstructor


/*****************************************************************************
 * TCP Segment Stitcher (Tss)
 *
 * @param[in]  siPhc_TcpWord,  Incoming TCP word from Pseudo Header Constructor (Phc).
 * @param[in]  siMEM_TxP_Data, TCP data payload from DRAM Memory (MEM).
 * @param[out] soSca_TcpWord,  Outgoing TCP word to Sub Checksum Accumulator (Sca).
 * @param[in]  siMrd_SplitSegSts, Indicates that the current segment has been
 *                              splitted and stored in 2 memory buffers.
 * @details
 *  Reads in the TCP pseudo header stream and appends the corresponding
 *   payload stream retrieved from the memory.
 *  Note that a TCP segment might have been splitted and stored as two memory
 *   segment units. This typically happens when the address of the physical
 *   memory buffer ring wraps around.
 *
 * @ingroup tx_engine*
 *****************************************************************************/
void pTcpSegStitcher(
        stream<TcpWord>         &siPhc_TcpWord,
        stream<AxiWord>         &siMEM_TxP_Data,
        #if (TCP_NODELAY)
        stream<bool>            &txEng_isDDRbypass,
        stream<axiWord>         &txApp2txEng_data_stream,
        #endif
        stream<AxiWord>         &soSca_TcpWord,
        stream<ap_uint<1> >     &siMrd_SplitSegSts)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tss");

    static ap_uint<3>   tss_wordCount    = 0;
    static ap_uint<3>   tss_state        = 0;
    static AxiWord      currWord         = AxiWord(0, 0, 0);
    static ap_uint<1>   tss_splitSegSts  = 0;
    static bool         didRdSplitSegSts = false;
    static ap_uint<4>   shiftBuffer      = 0;

    bool isShortCutData = false;

    static enum FsmState {PSEUDO_HDR=0,   SYN_OPT, FIRST_SEG_UNIT, FIRST_TO_SECOND_SEG_UNIT,
                          SECOND_SEG_UNIT} fsmState;

    switch (tss_state) {

    case 0: // Read the 4 first header words from Pseudo Header Constructor (Phc)
        if (!siPhc_TcpWord.empty()) {
            siPhc_TcpWord.read(currWord);
            soSca_TcpWord.write(currWord);
            didRdSplitSegSts = false;
            if (tss_wordCount == 3) {
                if (currWord.tdata[9] == 1) {
                    // Segment is a SYN
                    tss_state = 1;
                }
                else {
                    #if (TCP_NODELAY)
                    tss_state = 7;
                    #else
                    tss_state = 2;
                    #endif
                }
                tss_wordCount = 0;
            }
            else {
                tss_wordCount++;
            }
            if (currWord.tlast) {
                tss_state = 0;
                tss_wordCount = 0;
            }
        }
        break;

    case 1: // Read one more word from Phc because segment includes an option (.e.g, MSS)
        if (!siPhc_TcpWord.empty()) {
            siPhc_TcpWord.read(currWord);
            soSca_TcpWord.write(currWord);
            #if (TCP_NODELAY)
            tss_state = 7;
            #else
            tss_state = 2;
            #endif
            if (currWord.tlast) {
                tss_state = 0;
            }
            tss_wordCount = 0;
        }
        break;

    case 2: // Read the 1st memory segment unit
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full() &&
            ((didRdSplitSegSts == true) ||
             (didRdSplitSegSts == false && !siMrd_SplitSegSts.empty()))) {
            if (didRdSplitSegSts == false) {
                // Read the Split Segment Status information
                //  If 'true', the TCP segment was splitted and stored as 2 memory buffers.
                tss_splitSegSts = siMrd_SplitSegSts.read();
                didRdSplitSegSts = true;
            }
            siMEM_TxP_Data.read(currWord);
            if (currWord.tlast) {
                if (tss_splitSegSts == 0) {
                    // The TCP segment was not splitted; go back to init state
                    tss_state = 0;
                    soSca_TcpWord.write(currWord);
                }
                else {
                    //  The TCP segment was splitted in two parts.
                    shiftBuffer = keepToLen(currWord.tkeep);
                    tss_splitSegSts = 0;
                    currWord.tlast  = 0;
                    if (currWord.tkeep != 0xFF) {
                        // The last word contains some invalid bytes; These must be aligned.
                        tss_state = 3;
                    }
                    else {
                        // The last word is populated with 8 valid bytes and is therefore also
                        // aligned. We are done with this TCP segment.
                        soSca_TcpWord.write(currWord);
                    }
            	}
            }
            else
                soSca_TcpWord.write(currWord);
        }
        break;

    case 3:
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full()) {
            // Read the first word of the second (.i.e, splitted) and non_aligned segment unit
            currWord = siMEM_TxP_Data.read();
            ap_uint<4> keepCounter = keepToLen(currWord.tkeep);

            AxiWord outputWord = AxiWord(currWord.tdata, 0xFF, 0);
            outputWord.tdata(63, (shiftBuffer * 8)) = currWord.tdata(((8 - shiftBuffer) * 8) - 1, 0);

            if (keepCounter < 8 - shiftBuffer) {
                // The entirety of this word fits into the reminder of the previous one.
                // We are done with this TCP segment.
                outputWord.tkeep = lenToKeep(keepCounter + shiftBuffer);
                outputWord.tlast = 1;
                tss_state = 0;
            }
            else if (currWord.tlast == 1)
                tss_state = 5;
            else
                tss_state = 4;
            soSca_TcpWord.write(outputWord);
        }
        break;

    case 4:
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full()) {
            // Read the 2nd memory segment unit
            currWord = siMEM_TxP_Data.read();
            ap_uint<4> keepCounter = keepToLen(currWord.tkeep);

            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata((shiftBuffer * 8) - 1, 0) = currWord.tdata(63, (8 - shiftBuffer) * 8);
            outputWord.tdata(63, (8 * shiftBuffer)) = currWord.tdata(((8 - shiftBuffer) * 8) - 1, 0);

            if (keepCounter < 8 - shiftBuffer) {
                // The entirety of this word fits into the reminder of the previous one.
                // We are done with this TCP segment.
                outputWord.tkeep = lenToKeep(keepCounter + shiftBuffer);
                outputWord.tlast = 1;
                tss_state = 0;
            }
            else if (currWord.tlast == 1)
                tss_state = 5;
            soSca_TcpWord.write(outputWord);
        }
        break;

    case 5:
        if (!soSca_TcpWord.full()) {
            ap_uint<4> keepCounter = keepToLen(currWord.tkeep) - (8 - shiftBuffer);
            AxiWord outputWord = AxiWord(0, lenToKeep(keepCounter), 1);

            outputWord.tdata((shiftBuffer * 8) - 1, 0) = currWord.tdata(63, (8 - shiftBuffer) * 8);
            soSca_TcpWord.write(outputWord);
            tss_state = 0;
        }
        break;

    #if (TCP_NODELAY)
    case 6:
        if (!txApp2txEng_data_stream.empty() && !soSca_TcpWord.full()) {
            txApp2txEng_data_stream.read(currWord);
            soSca_TcpWord.write(currWord);
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
                tss_state = 2;
            }
        }
        break;
    #endif

    } // End of: switch

} // End of: pTcpSegStitcher


/*** OBSOLETE-pTcpSegStitcher-20181129 ****************************************
void pTcpSegStitcher(
        stream<TcpWord>         &siPhc_TcpWord,
        stream<AxiWord>         &siMEM_TxP_Data,
        stream<AxiWord>         &soSca_TcpWord,
        stream<ap_uint<1> >     &siMrd_SplitBuf)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tss");

    static ap_uint<3>   tss_wordCount = 0;
    static AxiWord      currWord = AxiWord(0, 0, 0);
    static ap_uint<1>   accBreakdownSts = 0;
    static ap_uint<4>   shiftBuffer = 0;
    static bool         hasReadAccBreakdown = false;

    if (tss_wordCount == 0) {
        if (!siPhc_TcpWord.empty()) {
            soSca_TcpWord.write(siPhc_TcpWord.read());
            // A segment will always consists of more than a single TcpWord
            hasReadAccBreakdown = false;
            tss_wordCount++;
        }
    }
    else if (tss_wordCount < 4) {
        if (!siPhc_TcpWord.empty() && !soSca_TcpWord.full()) {
            siPhc_TcpWord.read(currWord);
            soSca_TcpWord.write(currWord);
            if (currWord.tlast)
                tss_wordCount = 0;
            else
                tss_wordCount++;
        }
    }
    else if (tss_wordCount == 4) {
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full() &&
            (hasReadAccBreakdown == true || (hasReadAccBreakdown == false && !siMrd_SplitBuf.empty()))) {
            // Always read the first part of the buffer
            siMEM_TxP_Data.read(currWord);
            if (hasReadAccBreakdown == false) {
                accBreakdownSts = siMrd_SplitBuf.read();
                hasReadAccBreakdown = true;
            }
            if (currWord.tlast) {
                // This this mem. access is finished
                if (accBreakdownSts == 0) {
                    // Check if it was broken down in two. If not...
                    // go back to the init state and wait for the next segment.
                    tss_wordCount = 0;
                    soSca_TcpWord.write(currWord);
                }
                else if (accBreakdownSts == 1) {
                    // If yes, several options present themselves:
                    shiftBuffer = keepMapping(currWord.tkeep);
                    accBreakdownSts = 0;
                    currWord.tlast = 0;
                    if (currWord.tkeep != 0xFF)         // If the last word is complete, this means that the data are aligned correctly & nothing else needs to be done. If not we need to align them.
                        tss_wordCount = 5;              // Go to the next state to do just that
                    else
                        soSca_TcpWord.write(currWord);
                }
            }
            else
                soSca_TcpWord.write(currWord);
        }
    }
    else if (tss_wordCount == 5) { // 0x8F908348249AB4F8
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full()) {
            // Read the first word of a non_aligned second mem. access
            AxiWord outputWord = AxiWord(currWord.tdata, 0xFF, 0);
            currWord = siMEM_TxP_Data.read();
            outputWord.tdata.range(63, (shiftBuffer * 8)) = currWord.tdata.range(((8 - shiftBuffer.to_uint()) * 8) - 1, 0);
            ap_uint<4> keepCounter = keepMapping(currWord.tkeep);
            if (keepCounter < 8 - shiftBuffer) {
                // The entirety of the 2nd mem. access fits in this data word.
                outputWord.tkeep = returnKeep(keepCounter + shiftBuffer);
                outputWord.tlast = 1;
                tss_wordCount = 0;   // then go back to idle
            }
            else if (currWord.tlast == 1)
                tss_wordCount = 7;
            else
                tss_wordCount = 6;
            soSca_TcpWord.write(outputWord);
            //std::cerr <<  std::dec << cycleCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
        }
    }
    else if (tss_wordCount == 6) {
        if (!siMEM_TxP_Data.empty() && !soSca_TcpWord.full()) {
            // Read the first word of a non_aligned second mem. access
            AxiWord outputWord = AxiWord(0, 0xFF, 0);
            outputWord.tdata.range((shiftBuffer.to_uint() * 8) - 1, 0) = currWord.tdata.range(63, (8 - shiftBuffer.to_uint()) * 8);
            currWord = siMEM_TxP_Data.read();
            outputWord.tdata.range(63, (8 * shiftBuffer)) = currWord.tdata.range(((8 - shiftBuffer.to_uint()) * 8) - 1, 0);
            ap_uint<4> keepCounter = keepMapping(currWord.tkeep);
            if (keepCounter < 8 - shiftBuffer) {
                // The entirety of the 2nd mem. access fits in this data word..
                outputWord.tkeep = returnKeep(keepCounter + shiftBuffer);
                outputWord.tlast = 1;
                tss_wordCount = 0;   // then go back to idle
            }
            else if (currWord.tlast == 1)
                tss_wordCount = 7;
            soSca_TcpWord.write(outputWord);
            //std::cerr <<  std::dec << cycleCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
        }
    }
    else if (tss_wordCount == 7) {
        if (!soSca_TcpWord.full()) {
            ap_uint<4> keepCounter = keepMapping(currWord.tkeep) - (8 - shiftBuffer);                            // This is how many bits are valid in this word
            AxiWord outputWord = AxiWord(0, returnKeep(keepCounter), 1);
            outputWord.tdata.range((shiftBuffer.to_uint() * 8) - 1, 0) = currWord.tdata.range(63, (8 - shiftBuffer.to_uint()) * 8);
            soSca_TcpWord.write(outputWord);
            //std::cerr <<  std::dec << cycleCounter << " - " << std::hex << outputWord.data << " - " << outputWord.keep << " - " << outputWord.last << std::endl;
            tss_wordCount = 0;
        }
    }
}
******************************************************************************/


/*****************************************************************************
 * @brief Sub-Checksum Accumulator (Sca)
 *
 * @param[in]  siTss_TcpWord  Incoming data stream from Tcp Segment Stitcher (Tss).
 * @param[out] soIps_TcpWord, Outgoing data stream to IP Packet Stitcher (Ips).
 * @param[out] soTca_FourSubCsumsOut, the computed subchecksums are stored into this FIFO
 *
 * @details
 *  Accumulates 4 sub-checksums and forwards them to TCP Checksum Accumulator
 *   (Tca) for final checksum computation.
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pSubChecksumAccumulators(
        stream<TcpWord>     &siTss_TcpWord,
        stream<TcpWord>     &soIps_TcpWord,
        stream<SubCSums>    &soTca_4SubCsums)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static ap_uint<17> sca_4CSums[4] = {0, 0, 0, 0};
    static bool        sca_firstWord = true;

    TcpWord tcpWord;

    if (!siTss_TcpWord.empty()) {
        siTss_TcpWord.read(tcpWord);
        if (!sca_firstWord) {
            // We do not forward the first TcpWord' of the pseudo-header
            soIps_TcpWord.write(tcpWord);
        }
        for (int i = 0; i < 4; i++) {
        #pragma HLS unroll
            ap_uint<16> temp;
            if (tcpWord.tkeep.range(i*2+1, i*2) == 0x3) {
                temp( 7, 0) = tcpWord.tdata.range(i*16+15, i*16+8);
                temp(15, 8) = tcpWord.tdata.range(i*16+ 7, i*16);
                sca_4CSums[i] += temp;
                sca_4CSums[i] = (sca_4CSums[i] + (sca_4CSums[i] >> 16)) & 0xFFFF;
            }
            else if (tcpWord.tkeep[i*2] == 0x1) {
                temp( 7, 0) = 0;
                temp(15, 8) = tcpWord.tdata.range(i*16+7, i*16);
                sca_4CSums[i] += temp;
                sca_4CSums[i] = (sca_4CSums[i] + (sca_4CSums[i] >> 16)) & 0xFFFF;
            }
        }
        sca_firstWord = false;
        if(tcpWord.tlast == 1) {
            sca_firstWord = true;
            soTca_4SubCsums.write(sca_4CSums);
            sca_4CSums[0] = 0;
            sca_4CSums[1] = 0;
            sca_4CSums[2] = 0;
            sca_4CSums[3] = 0;
        }
    }
}


/*****************************************************************************
 * @brief TCP Checksum Accumulator (Tca)
 *
 * @param[in]  siSca_FourSubCsums, The 4 sub-csums from the Sub-Checksum Accumulator (Sca).
 * @param[out] soIps_TcpCsum,      The computed checksum to IP Packet Stitcher (Ips).
 *
 * @details
 *  Computes the final TCP checksum from the 4 accumulated sub-checksums and
 *   forwards the results to the  IP Packet Stitcher (Ips).
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pTcpChecksumAccumulator(
        stream<SubCSums>        &siSca_FourSubCsums,
        stream<ap_uint<16> >    &soIps_TcpCsum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

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

        if (DEBUG_LEVEL & TRACE_TCA) printInfo(myName, "Checksum =0x%4.4X\n", tca_4CSums.sum0.to_uint());
        soIps_TcpCsum.write(tca_4CSums.sum0);
    }
}
/*  static ap_uint<3> tctc_cc_state = 0;
    static subSums tctc_tcp_sums;

    switch (tctc_cc_state) {
    case 0:
        if (!txEng_subChecksumsFifoIn.empty()) {
            txEng_subChecksumsFifoIn.read(tctc_tcp_sums);
            tctc_cc_state++;
        }
        break;
    case 1:
        tctc_tcp_sums.sum0 += tctc_tcp_sums.sum2;
        tctc_tcp_sums.sum1 += tctc_tcp_sums.sum3;
        tctc_tcp_sums.sum0 = (tctc_tcp_sums.sum0 + (tctc_tcp_sums.sum0 >> 16)) & 0xFFFF;
        tctc_tcp_sums.sum1 = (tctc_tcp_sums.sum1 + (tctc_tcp_sums.sum1 >> 16)) & 0xFFFF;
        tctc_cc_state++;
        break;
    case 2:
        tctc_tcp_sums.sum0 += tctc_tcp_sums.sum1;
        tctc_tcp_sums.sum0 = (tctc_tcp_sums.sum0 + (tctc_tcp_sums.sum0 >> 16)) & 0xFFFF;
        tctc_cc_state++;
        break;
    case 3:
        tctc_tcp_sums.sum0 = ~tctc_tcp_sums.sum0;
        tctc_cc_state++;
        break;
    case 4: //TODO try to remove one state
        txEng_tcpChecksumFifoOut.write(tctc_tcp_sums.sum0);
        tctc_cc_state = 0;
        break;
    }
}*/


/*****************************************************************************
 * @brief IPv4 Packet Stitcher (Ips)
 *
 * @param[in]  siIhc_Ip4Word, IPv4 stream from IP Header Constructor (Ihc).
 * @param[in]  siSca_TcpWord, TCP stream from Sub Checksum Accumulator (Sca).
 * @param[in]  siTca_TcpCsum, TCP checksum from TCP Checksum Accumulator (Tca).
 * @param[out] soL3MUX_Data,  Outgoing data stream to layer-3 pkt mux.
 *
 * @details
 *  Assembles an IPv4 packet from the incoming IP header stream and the data
 *   payload stream.
 *  Inserts the IPv4 and the TCP checksum.
 *  IPv4 packet is streamed out of the TCP Offload Engine.
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pIpPktStitcher(
        stream<Ip4Word>         &siIhc_Ip4Word,
        stream<TcpWord>         &siSca_TcpWord,
        stream<TcpCSum>         &siTca_TcpCsum,
        stream<Ip4overAxi>      &soL3MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //#pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Ips");

    static ap_uint<3> ps_wordCount = 0;
    Ip4overAxi ip4HdrWord;
    Ip4overAxi sendWord;
    TcpCSum    tcpCsum;
    TcpWord    tcpDatWord;

    switch (ps_wordCount) {

    case WORD_0:
    case WORD_1:
        if (!siIhc_Ip4Word.empty()) {
            siIhc_Ip4Word.read(ip4HdrWord);
            sendWord = ip4HdrWord;

            soL3MUX_Data.write(sendWord);
            ps_wordCount++;
            if (DEBUG_LEVEL & TRACE_IPS) printAxiWord(myName, sendWord);
        }
        break;

    case WORD_2: // Start concatenating IPv4 header and TCP segment
        if (!siIhc_Ip4Word.empty() && !siSca_TcpWord.empty()) {
            siIhc_Ip4Word.read(ip4HdrWord);
            siSca_TcpWord.read(tcpDatWord);

            sendWord.tdata(31,  0) = ip4HdrWord.tdata(31,  0);  // IPv4 Destination Address
            sendWord.tdata(63, 32) = tcpDatWord.tdata(63, 32);  // TCP DstPort & SrcPort
            sendWord.tkeep         = 0xFF;
            sendWord.tlast         = 0;

            soL3MUX_Data.write(sendWord);
            ps_wordCount++;
            if (DEBUG_LEVEL & TRACE_IPS) printAxiWord(myName, sendWord);
        }
        break;

    case WORD_3:
        if (!siSca_TcpWord.empty()) {
            siSca_TcpWord.read(tcpDatWord);  // TCP SeqNum & AckNum
            sendWord = Ip4overAxi(tcpDatWord.tdata, tcpDatWord.tkeep, tcpDatWord.tlast);

            soL3MUX_Data.write(sendWord);
            ps_wordCount++;
            if (DEBUG_LEVEL & TRACE_IPS) printAxiWord(myName, sendWord);
        }
        break;

    case WORD_4:
        if (!siSca_TcpWord.empty() && !siTca_TcpCsum.empty()) {
            siSca_TcpWord.read(tcpDatWord);
            siTca_TcpCsum.read(tcpCsum);  // TCP UrgPtr & Checksum & Window & CtrlBits

            sendWord = Ip4overAxi(tcpDatWord.tdata, tcpDatWord.tkeep, tcpDatWord.tlast);
            // Now overwrite TCP checksum
            sendWord.setTcpChecksum(tcpCsum);
            //OBSOLETE-20181202 sendWord.tdata(47, 32) = byteSwap16(tcpCsum);
            //OBSOLETE-20181128 sendWord.tdata(39, 32) = tcpCsum(15,  8);
            //OBSOLETE-20181128 sendWord.tdata(47, 40) = tcpCsum( 7,  0);

            soL3MUX_Data.write(sendWord);
            ps_wordCount++;
            if (tcpDatWord.tlast) {
                // Might be last word when data payload is empty
                ps_wordCount = 0;
                if (DEBUG_LEVEL & TRACE_IPS) printAxiWord(myName, sendWord);
            }
        }
        break;

    default:
        if (!siSca_TcpWord.empty()) {
            siSca_TcpWord.read(tcpDatWord);  // TCP Data
            sendWord = Ip4overAxi(tcpDatWord.tdata, tcpDatWord.tkeep, tcpDatWord.tlast);

            soL3MUX_Data.write(sendWord);
            if (tcpDatWord.tlast) {
                ps_wordCount = 0;
            }
            if (DEBUG_LEVEL & TRACE_IPS) printAxiWord(myName, sendWord);
        }
        break;

    }

}


/*****************************************************************************
 * @brief Memory Reader (Mrd)
 *
 * @param[in]  siMdl_BufferRdCmd, Buffer read command from Meta Data Loader (Mdl).
 * @param[out] soMEM_Txp_RdCmd,   Memory read command to the DRAM Memory (MEM).
 * @param[out] soTss_SplitMemAcc, Splitted memory access to Tcp Segment Stitcher (Tss).
 *
 * @details
 *  Front end memory controller for reading data from the external DRAM.
 *  This process receives a transfer read command and forwards it to the AXI4
 *   Data Mover. This command might be split in two memory accesses if the
 *   address of the data buffer to read wraps in the external memory. Such a
 *   split memory access is indicated by the signal 'soTss_SplitMemAcc'.
 *
 * @ingroup tx_engine
 *****************************************************************************/
void pMemoryReader(
        stream<DmCmd>       &siMdl_BufferRdCmd,
        stream<DmCmd>       &soMEM_Txp_RdCmd,
        stream<StsBit>      &soTss_SplitMemAcc)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    const char *myName  = concat3(THIS_NAME, "/", "Mrd");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                   txEngBreakdown = false;
    #pragma HLS reset    variable=txEngBreakdown
    static uint16_t               txPktCounter = 0;
    #pragma HLS reset    variable=txPktCounter

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static DmCmd    txMemReaderCmd;
    static uint16_t txEngBreakTemp;

    if (txEngBreakdown == false) {
        if (!siMdl_BufferRdCmd.empty() && !soMEM_Txp_RdCmd.full() && soTss_SplitMemAcc.empty()) {
            txMemReaderCmd = siMdl_BufferRdCmd.read();
            DmCmd tempCmd = txMemReaderCmd;
            if ((txMemReaderCmd.saddr.range(15, 0) + txMemReaderCmd.bbt) > 65536) {
                // This segment is broken in two memory accesses because TCP Tx memory buffer wraps around
                txEngBreakTemp = 65536 - txMemReaderCmd.saddr;
                tempCmd = DmCmd(txMemReaderCmd.saddr, txEngBreakTemp);
                txEngBreakdown = true;
            }
            soMEM_Txp_RdCmd.write(tempCmd);
            soTss_SplitMemAcc.write(txEngBreakdown);
            txPktCounter++;
            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "Issuing memory read command #%d - SADDR=0x%x - BBT=0x%x\n",
                          txPktCounter, tempCmd.saddr.to_uint(), tempCmd.bbt.to_uint());
            }
        }
    }
    else if (txEngBreakdown == true) {
        if (!soMEM_Txp_RdCmd.full()) {
            txMemReaderCmd.saddr.range(15, 0) = 0;
            soMEM_Txp_RdCmd.write(DmCmd(txMemReaderCmd.saddr, txMemReaderCmd.bbt - txEngBreakTemp));
            txEngBreakdown = false;
            if (DEBUG_LEVEL & TRACE_MRD) {
                printInfo(myName, "Issuing memory read command #%d - SADDR=0x%x - BBT=0x%x\n",
                          txPktCounter, txMemReaderCmd.saddr.to_uint(), txMemReaderCmd.bbt.to_uint());
            }
        }
    }
}


/*****************************************************************************
 * @brief The tx_engine (TXe) builds the IPv4 packets to be sent to L3MUX.
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
 *   packet contains any payload, the data is retrieved from the DDR4 memory
 *   and is put as a TCP segment into the IPv4 packet. The complete packet is
 *   then streamed out over the IPv4 Tx interface of the TOE (.i.e, soL3MUX).
 *
 * @ingroup tx_engine
 ******************************************************************************/
void tx_engine(
        stream<extendedEvent>           &siAKd_Event,
        stream<SessionId>               &soRSt_RxSarReq,
        stream<RxSarEntry>              &siRSt_RxSarRep,
        stream<TXeTxSarQuery>           &soTSt_TxSarQry,
        stream<TXeTxSarReply>           &siTSt_TxSarRep,
        stream<AxiWord>                 &siMEM_TxP_Data,
        stream<TXeReTransTimerCmd>      &soTIm_ReTxTimerEvent,
        stream<ap_uint<16> >            &soTIm_SetProbeTimer,
        stream<DmCmd>                   &soMEM_Txp_RdCmd,
        stream<ap_uint<16> >            &soSLc_ReverseLkpReq,
        stream<fourTuple>               &siSLc_ReverseLkpRep,
        stream<SigBit>                  &soEVe_RxEventSig,
        stream<Ip4overAxi>              &soL3MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return  // [FIXME - No needed here?]
   //#pragma HLS PIPELINE II=1
    #pragma HLS INLINE //off

    //=========================================================================
    //== LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //=========================================================================

    //-------------------------------------------------------------------------
    //-- Meta Data Loader (Mdl)
    //-------------------------------------------------------------------------
    static stream<TcpSegLen>            sMdlToIhc_TcpSegLen     ("sMdlToIhc_TcpSegLen");
    #pragma HLS stream         variable=sMdlToIhc_TcpSegLen     depth=16

    static stream<TXeMeta>              sMdlToPhc_TxeMeta       ("sMdlToPhc_TxeMeta");
    #pragma HLS stream         variable=sMdlToPhc_TxeMeta       depth=16
    #pragma HLS DATA_PACK      variable=sMdlToPhc_TxeMeta

    static stream<bool>                 sMdlToSpS_IsLookup      ("sMdlToSpS_IsLookup");
    #pragma HLS stream         variable=sMdlToSpS_IsLookup      depth=4

    static stream<DmCmd>                sMdlToMrd_BufferRdCmd   ("sMdlToMrd_BufferRdCmd");
    #pragma HLS stream         variable=sMdlToMrd_BufferRdCmd   depth=32
    #pragma HLS DATA_PACK      variable=sMdlToMrd_BufferRdCmd

    static stream<AxiSocketPair>        sMdlToSps_RstSockPair   ("sMdlToSps_RstSockPair");
    #pragma HLS stream         variable=sMdlToSps_RstSockPair   depth=2
    #pragma HLS DATA_PACK      variable=sMdlToSps_RstSockPair

    //-------------------------------------------------------------------------
    //-- Ip Header Construction (Ihc)
    //-------------------------------------------------------------------------
    static stream<Ip4Word>              sIhcToIps_Ip4Word       ("sIhcToIps_Ip4Word");
    #pragma HLS stream         variable=sIhcToIps_Ip4Word       depth=32 // [INFO] Ip header is 3 words, keep at least 8 headers
    #pragma HLS DATA_PACK      variable=sIhcToIps_Ip4Word

    //-------------------------------------------------------------------------
     //-- Pseudo Header Construction (Phc)
     //-------------------------------------------------------------------------
    static stream<TcpWord>              sPhcToTss_TcpWord       ("sPhcToTss_TcpWord");
    #pragma HLS stream         variable=sPhcToTss_TcpWord       depth=32 // TCP pseudo header is 4 words, keep at least 8 headers
    #pragma HLS DATA_PACK      variable=sPhcToTss_TcpWord


    //-------------------------------------------------------------------------
    //-- Socket Pair Splitter (Sps)
    //-------------------------------------------------------------------------
    static stream<IpAddrPair>           sSpsToIhc_IpAddrPair    ("sSpsToIhc_IpAddrPair");
    #pragma HLS stream         variable=sSpsToIhc_IpAddrPair    depth=4
    #pragma HLS DATA_PACK      variable=sSpsToIhc_IpAddrPair

    static stream<AxiSocketPair>        sSpsToPhc_SockPair      ("sSpsToPhc_SockPair");
    #pragma HLS stream         variable=sSpsToPhc_SockPair      depth=4
    #pragma HLS DATA_PACK      variable=sSpsToPhc_SockPair

    //-------------------------------------------------------------------------
    //-- TCP Segment Stitcher (Tss)
    //-------------------------------------------------------------------------
    static stream<TcpWord>              sTssToSca_TcpWord       ("sTssToSca_TcpWord");
    #pragma HLS stream         variable=sTssToSca_TcpWord       depth=16   // is forwarded immediately, size is not critical
    #pragma HLS DATA_PACK      variable=sTssToSca_TcpWord

    //-------------------------------------------------------------------------
    //-- TCP Checksum Accumulator (Tca)
    //-------------------------------------------------------------------------
    static stream<TcpCSum>              sTcaToIps_TcpCsum       ("sTcaToIps_TcpCsum");
    #pragma HLS stream         variable=sTcaToIps_TcpCsum       depth=4

    //-------------------------------------------------------------------------
    //-- Sub-Checksum Accumulator (Sca)
    //-------------------------------------------------------------------------
    static stream<TcpWord>              sScaToIps_TcpWord       ("sScaToIps_TcpWord");
    #pragma HLS stream         variable=sScaToIps_TcpWord       depth=256  // WARNING: Critical; has to keep complete packet for checksum computation
    #pragma HLS DATA_PACK      variable=sScaToIps_TcpWord

    static stream<SubCSums>             sScaToTca_FourSubCsums  ("sScaToTca_FourSubCsums");
    #pragma HLS stream         variable=sScaToTca_FourSubCsums  depth=2
    #pragma HLS DATA_PACK      variable=sScaToTca_FourSubCsums

    //------------------------------------------------------------------------
    //-- Memory Reader (Mrd)
    //------------------------------------------------------------------------
    static stream<StsBit>               sMrdToTss_SplitMemAcc   ("sMrdToTss_SplitMemAcc");
    #pragma HLS stream         variable=sMrdToTss_SplitMemAcc   depth=32



    //OBSOLETE-20181127 #pragma HLS DATA_PACK variable=txBufferReadData
    //OBSOLETE-20181125 #pragma HLS DATA_PACK variable=soRSt_RxSarRdReq
    //OBSOLETE-20181126 #pragma HLS DATA_PACK variable=soSLc_ReverseLkpReq

    //OBSOLETE 20181128 #pragma HLS resource core=AXI4Stream variable=soL3MUX_Data metadata="-bus_bundle m_axis_tcp_data"
    //OBSOLETE 20181128 #pragma HLS DATA_PACK variable=soL3MUX_Data

    // NOT-USED Memory Read delay around 76 cycles, 10 cycles/packet, so keep meta of at least 8 packets
    // NOT-USED static stream<tx_engine_meta>       txEng_metaDataFifo("txEng_metaDataFifo");
    // NOT-USED #pragma HLS stream variable=txEng_metaDataFifo depth=16
    // NOT-USED #pragma HLS DATA_PACK variable=txEng_metaDataFifo

    
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
            sMdlToIhc_TcpSegLen,
            sMdlToPhc_TxeMeta,
            sMdlToMrd_BufferRdCmd,
            soSLc_ReverseLkpReq,
            sMdlToSpS_IsLookup,
            sMdlToSps_RstSockPair,
            soEVe_RxEventSig);
    
    pMemoryReader(
            sMdlToMrd_BufferRdCmd,
            soMEM_Txp_RdCmd,
            sMrdToTss_SplitMemAcc);

    pSocketPairSplitter(
            siSLc_ReverseLkpRep,
            sMdlToSps_RstSockPair,
            sMdlToSpS_IsLookup,
            sSpsToIhc_IpAddrPair,
            sSpsToPhc_SockPair);

    pIpHeaderConstructor(
            sMdlToIhc_TcpSegLen,
            sSpsToIhc_IpAddrPair,
            sIhcToIps_Ip4Word);

    pIpPktStitcher(
            sIhcToIps_Ip4Word,
            sScaToIps_TcpWord,
            sTcaToIps_TcpCsum,
            soL3MUX_Data);

    pPseudoHeaderConstructor(
            sMdlToPhc_TxeMeta,
            sSpsToPhc_SockPair,
            sPhcToTss_TcpWord);

    pTcpSegStitcher(
            sPhcToTss_TcpWord,
            siMEM_TxP_Data,
            sTssToSca_TcpWord,
            sMrdToTss_SplitMemAcc);

    pSubChecksumAccumulators(
            sTssToSca_TcpWord,
            sScaToIps_TcpWord,
            sScaToTca_FourSubCsums);

    pTcpChecksumAccumulator(
            sScaToTca_FourSubCsums,
            sTcaToIps_TcpCsum);



}
