/*****************************************************************************
 * @file       : rx_engine.hpp
 * @brief      : Rx Engine (RXE) of the TCP Offload Engine (TOE).
 **
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

using namespace hls;

/** @ingroup rx_engine
 *  @TODO check if same as in Tx engine
 */
struct rxEngineMetaData
{
    //ap_uint<16> sessionID;
    ap_uint<32> seqNumb;
    ap_uint<32> ackNumb;
    ap_uint<16> winSize;
    ap_uint<16> length;
    ap_uint<1>  ack;
    ap_uint<1>  rst;
    ap_uint<1>  syn;
    ap_uint<1>  fin;
    //ap_uint<16> dstPort;
};

/** @ingroup rx_engine
 *
 */
struct rxFsmMetaData
{
    ap_uint<16>         sessionID;
    ap_uint<32>         srcIpAddress;
    ap_uint<16>         dstIpPort;
    rxEngineMetaData    meta; //check if all needed
    rxFsmMetaData() {}
    rxFsmMetaData(ap_uint<16> id, ap_uint<32> ipAddr, ap_uint<16> ipPort, rxEngineMetaData meta)
                :sessionID(id), srcIpAddress(ipAddr), dstIpPort(ipPort), meta(meta) {}
};

/** @defgroup rx_engine RX Engine
 *  @ingroup tcp_module
 *  RX Engine
 */
void rx_engine(
		stream<Ip4Word>						&siIPRX_Data,
		stream<sessionLookupReply>&         sLookup2rxEng_rsp,
		stream<sessionState>&               stateTable2rxEng_upd_rsp,
		stream<bool>&                       portTable2rxEng_rsp,
		stream<rxSarEntry>&                 rxSar2rxEng_upd_rsp,
		stream<rxTxSarReply>&               txSar2rxEng_upd_rsp,
		stream<mmStatus>&                   rxBufferWriteStatus,
		stream<axiWord>&                    rxBufferWriteData,
		stream<sessionLookupQuery>&         rxEng2sLookup_req,
		stream<stateQuery>&                 rxEng2stateTable_upd_req,
		stream<ap_uint<16> >&               rxEng2portTable_req,
		stream<rxSarRecvd>&                 rxEng2rxSar_upd_req,
		stream<rxTxSarQuery>&               rxEng2txSar_upd_req,
		stream<rxRetransmitTimerUpdate>&    rxEng2timer_clearRetransmitTimer,
		stream<ap_uint<16> >&               rxEng2timer_clearProbeTimer,
		stream<ap_uint<16> >&               rxEng2timer_setCloseTimer,
		stream<openStatus>&                 openConStatusOut, //TODO remove
		stream<extendedEvent>&              rxEng2eventEng_setEvent,
		stream<mmCmd>&                      rxBufferWriteCmd,
		stream<appNotification>&            rxEng2rxApp_notification
);
