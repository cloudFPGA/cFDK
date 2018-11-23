/*****************************************************************************
 * @file       : rx_engine.hpp
 * @brief      : Rx Engine (RXE) of the TCP Offload Engine (TOE).
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
 * @details    : Data structures, types and prototypes definitions for the
 *               TCP Rx Engine.
 *
 *****************************************************************************/

#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;


/********************************************
 * RXe - MetaData Interface
 ********************************************/
struct rxEngineMetaData
{
	TcpSeqNum   seqNumb;    // TCP Sequence Number
	TcpAckNum   ackNumb;    // TCP Acknowledgment Number
	TcpWindow   winSize;    // TCP Window Size
    TcpSegLen   length;     // TCP Segment Length
    ap_uint<1>  ack;
    ap_uint<1>  rst;
    ap_uint<1>  syn;
    ap_uint<1>  fin;
    //ap_uint<16> dstPort;
};

/********************************************
 * RXe - FsmMetaData Interface
 ********************************************/
struct rxFsmMetaData
{
    ap_uint<16>         sessionID;
    ap_uint<32>         srcIpAddress;
    ap_uint<16>         dstIpPort;
    rxEngineMetaData    meta; //check if all needed
    rxFsmMetaData() {}
    rxFsmMetaData(ap_uint<16> id, ap_uint<32> ipAddr, ap_uint<16> ipPort, rxEngineMetaData meta) :
        sessionID(id), srcIpAddress(ipAddr), dstIpPort(ipPort), meta(meta) {}
};


/*****************************************************************************
 * @brief   Main process of the TCP Rx Engine (RXe).
 *
 * @ingroup rx_engine
 *****************************************************************************/
void rx_engine(
        stream<Ip4Word>                 &siIPRX_Pkt,
        stream<sessionLookupReply>      &siSLc_SessLookupRep,
        stream<sessionState>            &siSTt_SessStateRep,
        stream<StsBit>                  &siPRt_PortSts,
        stream<rxSarEntry>              &siRSt_SessRxSarRep,
        stream<rxTxSarReply>            &siTSt_SessTxSarRep,
        stream<mmStatus>                &siMEM_WrSts,
        stream<axiWord>                 &soMemWrData,
        stream<stateQuery>              &soSessStateReq,
        stream<AxiTcpPort>              &soGetPortState,
        stream<sessionLookupQuery>      &soSessLookupReq,
        stream<rxSarRecvd>              &soSessRxSarReq,
        stream<rxTxSarQuery>            &soSessTxSarReq,
        stream<rxRetransmitTimerUpdate> &soClearReTxTimer,
        stream<ap_uint<16> >            &soClearProbeTimer,
        stream<ap_uint<16> >            &soCloseTimer,
        stream<openStatus>              &soSessOpnStatus,
        stream<extendedEvent>           &soSetEvent,
        stream<mmCmd>                   &soMemWrCmd,
        stream<appNotification>         &soRxNotification
);
