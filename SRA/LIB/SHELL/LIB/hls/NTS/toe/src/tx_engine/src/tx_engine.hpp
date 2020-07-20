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

/*******************************************************************************
 * @file       : tx_engine.hpp
 * @brief      : Tx Engine (TXe) of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_TXE_H_
#define _TOE_TXE_H_


#include "../../../../../NTS/nts.hpp"
#include "../../../../../NTS/nts_utils.hpp"
#include "../../../../../NTS/toe/src/toe.hpp"
#include "../../../../../NTS/toe/src/toe_utils.hpp"
#include "../../../../../NTS/SimNtsUtils.hpp"
#include "../../../../../NTS/AxisIp4.hpp"
#include "../../../../../NTS/AxisPsd4.hpp"

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
class LE_IpAddrPair {  // OBSOLETE-20191004 struct IpAddrPair
  public:
    LE_Ip4Addr  src;
    LE_Ip4Addr  dst;
    LE_IpAddrPair() {}
    LE_IpAddrPair(LE_Ip4Addr src, LE_Ip4Addr dst) :
        src(src), dst(dst) {}
};

/*******************************************************************************
 *
 * @brief ENTITY - Tx Engine (TXe)
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
        stream<AxisIp4>                 &soL3MUX_Data
);

#endif

/*! \} */
