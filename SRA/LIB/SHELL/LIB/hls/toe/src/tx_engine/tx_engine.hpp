/*****************************************************************************
 * @file       : tx_engine.hpp
 * @brief      : Tx Engine (TXe) of the TCP Offload Engine (TOE).
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
 *               TCP Tx Engine.
 *
 *****************************************************************************/

#include "../toe.hpp"
#include "../toe_utils.hpp"

using namespace hls;


/********************************************
 * TXe - MetaData Interface
 ********************************************/
class TXeMeta {
  public:
    TcpSeqNum   seqNumb;
    TcpAckNum   ackNumb;
    TcpWindow   winSize;
    TcpSegLen   length;
    ap_uint<1>  ack;
    ap_uint<1>  rst;
    ap_uint<1>  syn;
    ap_uint<1>  fin;
    TXeMeta() {}
    TXeMeta(ap_uint<1> ack, ap_uint<1> rst, ap_uint<1> syn, ap_uint<1> fin)
        : seqNumb(0), ackNumb(0), winSize(0), length(0), ack(ack), rst(rst), syn(syn), fin(fin) {}
    TXeMeta(TcpSeqNum seqNumb, TcpAckNum ackNumb, ap_uint<1> ack, ap_uint<1> rst, ap_uint<1> syn, ap_uint<1> fin)
        : seqNumb(seqNumb), ackNumb(ackNumb), winSize(0), length(0), ack(ack), rst(rst), syn(syn), fin(fin) {}
    TXeMeta(TcpSeqNum seqNumb, TcpAckNum ackNumb, TcpWindow winSize, ap_uint<1> ack, ap_uint<1> rst, ap_uint<1> syn, ap_uint<1> fin)
        : seqNumb(seqNumb), ackNumb(ackNumb), winSize(winSize), length(0), ack(ack), rst(rst), syn(syn), fin(fin) {}
};


/********************************************
 * TXe - A set of 4 TCP Sub-Checksums
 ********************************************/
struct SubCSums
{
    ap_uint<17>     sum0;
    ap_uint<17>     sum1;
    ap_uint<17>     sum2;
    ap_uint<17>     sum3;
    SubCSums() {}
    SubCSums(ap_uint<17> sums[4]) :
        sum0(sums[0]), sum1(sums[1]), sum2(sums[2]), sum3(sums[3]) {}
    SubCSums(ap_uint<17> s0, ap_uint<17> s1, ap_uint<17> s2, ap_uint<17> s3) :
        sum0(s0), sum1(s1), sum2(s2), sum3(s3) {}
};


/********************************************
 * TXe - Pair of {Src,Dst} IPv4 Addresses
 ********************************************/
struct IpAddrPair
{
	AxiIp4Addr  src;
	AxiIp4Addr  dst;
    IpAddrPair() {}
    IpAddrPair(AxiIp4Addr src, AxiIp4Addr dst) :
        src(src), dst(dst) {}
};


/*****************************************************************************
 * @brief   Main process of the TCP Tx Engine (TXe).
 *
 * @ingroup tx_engine
 *****************************************************************************/
void tx_engine(
        stream<extendedEvent>           &siAKd_Event,
        stream<ap_uint<16> >            &soRSt_RxSarRdReq,
        stream<RxSarEntry>              &siRSt_RxSarRdRep,
        stream<txTxSarQuery>            &soTSt_TxSarUpdReq,
        stream<txTxSarReply>            &siTSt_TxSarUpdRep,
        stream<AxiWord>                 &siMEM_TxP_Data,
        stream<ReTxTimerEvent>          &soTIm_ReTxTimerEvent,
        stream<ap_uint<16> >            &soTIm_SetProbeTimer,
        stream<DmCmd>                   &soMEM_Txp_RdCmd,
        stream<ap_uint<16> >            &soSLc_ReverseLkpReq,
        stream<fourTuple>               &siSLc_ReverseLkpRep,
        stream<SigBit>                  &soEVe_RxEventSig,
        stream<Ip4overAxi>              &soL3MUX_Data
);
