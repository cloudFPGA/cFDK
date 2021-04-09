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
 * @file       : iptx.cpp
 * @brief      : IP Transmitter packet handler (IPTX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_IPTX
 * \{
 ******************************************************************************/

#include "iptx.hpp"

using namespace hls;

//OBSOLETE_20210409 #define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MAI | TRACE_ICA)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "IPTX"

#define TRACE_OFF  0x0000
#define TRACE_ICA  1 << 1
#define TRACE_ICI  1 << 2
#define TRACE_IAE  1 << 3
#define TRACE_MAI  1 << 4
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * IPv4 Header Checksum Accumulator (HCa)
 *
 * @param[in]  siL3MUX_Data Data stream from the L3 Multiplexer (L3MUX).
 * @param[out] soICi_Data   Data stream to the IpChecksumInserter (ICi).
 * @param[out] soICi_Csum   IP header checksum.
 *
 * @details
 *  This process computes the IPv4 header checksum and forwards it to the
 *  next process which will insert it into the header of the incoming packet.
 *
 * @Warning
 *  The IP header is formatted for transmission over a 64-bits interface which
 *  is logically divided into 8 lanes, with lane[0]=bits(7:0) and
 *  lane[7]=bits(63:56). The format of the incoming IPv4 header is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |   Ident (L)   |   Ident (H)   | Total Len (L) | Total Len (H) |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    | Hd Chksum (L) | Hd Chksum (H) |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            Options                            |     DA (LL)   |     DA (L)    |     DA (H)    |    DA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pHeaderChecksumAccumulator(
        stream<AxisIp4>     &siL3MUX_Data,
        stream<AxisIp4>     &soICi_Data,
        stream<Ip4HdrCsum>  &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "HCa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                             hca_csumWritten=true;
    #pragma HLS RESET              variable=hca_csumWritten
    static enum FsmState {S0=0, S1, S2, S3} hca_fsmState=S0;
    #pragma HLS RESET              variable=hca_fsmState
    static ap_uint<17>                      hca_ipHdrCsum[4]={0, 0, 0, 0};
    #pragma HLS RESET              variable=hca_ipHdrCsum
    #pragma HLS ARRAY_PARTITION    variable=hca_ipHdrCsum complete dim=1
    static ap_uint<3>                       hca_chunkCount=0;
    #pragma HLS RESET              variable=hca_chunkCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<4>                       hca_ipHdrLen;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4     currChunk;
    Ip4HdrCsum  tempHdrCsum;

    currChunk.setLE_TLast(0);
    if(!hca_csumWritten) {
        switch (hca_fsmState) {
        case S0:
            hca_ipHdrCsum[0] += hca_ipHdrCsum[2];
            hca_ipHdrCsum[1] += hca_ipHdrCsum[3];
            hca_ipHdrCsum[0] = (hca_ipHdrCsum[0] + (hca_ipHdrCsum[0] >> 16)) & 0xFFFF;
            hca_ipHdrCsum[1] = (hca_ipHdrCsum[1] + (hca_ipHdrCsum[1] >> 16)) & 0xFFFF;
            hca_fsmState = S1;
            break;
        case S1:
            hca_ipHdrCsum[0] += hca_ipHdrCsum[1];
            hca_ipHdrCsum[0] = (hca_ipHdrCsum[0] + (hca_ipHdrCsum[0] >> 16)) & 0xFFFF;
            hca_fsmState = S2;
            break;
        case S2:
            hca_ipHdrCsum[0] = ~hca_ipHdrCsum[0];
            soICi_Csum.write(hca_ipHdrCsum[0](15, 0));
            hca_fsmState = S3;
            break;
        case S3:
            hca_ipHdrCsum[0] = 0;
            hca_ipHdrCsum[1] = 0;
            hca_ipHdrCsum[2] = 0;
            hca_ipHdrCsum[3] = 0;
            hca_csumWritten = true;
            hca_fsmState = S0;
            break;
        }
    }
    else if (!siL3MUX_Data.empty() and !soICi_Data.full() and hca_csumWritten) {
        siL3MUX_Data.read(currChunk);
        // Process the IPv4 header.
        //  Remember that the Internet Header Length (IHL) field contains the
        //  size of the IPv4 header specified in number of 32-bit words, and
        //  its default minimum value is 5.
        switch (hca_chunkCount) {
        case 0:
            hca_ipHdrLen = currChunk.getIp4HdrLen();
            //-- Accumulate 1st IPv4 header qword [Frag|Ident|TotLen|ToS|IHL]
            for (int i=0; i<4; i++) {
              #pragma HLS UNROLL
                tempHdrCsum( 7, 0) = currChunk.getLE_TData().range(i*16+15, i*16+8);
                tempHdrCsum(15, 8) = currChunk.getLE_TData().range(i*16+7,  i*16);
                hca_ipHdrCsum[i]  += tempHdrCsum;
                hca_ipHdrCsum[i]   = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
            }
            hca_chunkCount++;
            hca_ipHdrLen -= 2;
            break;
        case 1:
            //-- Accumulate 2nd IPv4 header qword [SA|HdCsum|Prot|TTL]
            for (int i=0; i<4; i++) {
              #pragma HLS UNROLL
                if (i != 1) {
                    // Skip 3rd and 4th bytes. They contain the IP header checksum.
                    tempHdrCsum( 7, 0) = currChunk.getLE_TData().range(i*16+15, i*16+8);
                    tempHdrCsum(15, 8) = currChunk.getLE_TData().range(i*16+7,  i*16);
                    hca_ipHdrCsum[i]  += tempHdrCsum;
                    hca_ipHdrCsum[i]  = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
            }
            hca_chunkCount++;
            hca_ipHdrLen -= 2;
            break;
        default:
            switch (hca_ipHdrLen) {
            case 0:
                //-- We are done with the checksum header processing
                break;
            case 1:
                //-- Accumulate half of the 3rd IPv4 header qword [DA] or
                //-- Accumulate the last IPv4 option dword
                for (int i = 0; i < 2; i++) {
                  #pragma HLS UNROLL
                    tempHdrCsum( 7, 0) = currChunk.getLE_TData().range(i*16+15, i*16+8);
                    tempHdrCsum(15, 8) = currChunk.getLE_TData().range(i*16+7,  i*16);
                    hca_ipHdrCsum[i]  += tempHdrCsum;
                    hca_ipHdrCsum[i]   = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
                hca_ipHdrLen = 0;
                hca_csumWritten = false;
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                  #pragma HLS UNROLL
                    tempHdrCsum( 7, 0) = currChunk.getLE_TData().range(i*16+15, i*16+8);
                    tempHdrCsum(15, 8) = currChunk.getLE_TData().range(i*16+7,  i*16);
                    hca_ipHdrCsum[i] += tempHdrCsum;
                    hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
                hca_ipHdrLen -= 2;
                if (hca_ipHdrLen == 0) {
                    hca_csumWritten = false;
                }
                break;
            } // End-of: switch ipHeaderLen
            break;
        } // End-of: switch(hca_chunkCount)
        soICi_Data.write(currChunk);
        if (currChunk.getLE_TLast()) {
            hca_chunkCount = 0;
        }
    }
}

/*******************************************************************************
 * IP Checksum Inserter (ICi)
 *
 * @param[in]  siHCa_Data The data stream from HeaderChecksumAccumulator (HCa).
 * @param[in]  siHCa_Csum The computed IP header checksum from [HCa].
 * @param[out] soIAe_Data The data stream to IpAddressExtratcor (IAe).
 *
 * @details
 *  This process inserts the computed IP header checksum in the IPv4 packet
 *  being streamed on the data interface.
 *
 *******************************************************************************/
void pIpChecksumInsert(
        stream<AxisIp4>     &siHCa_Data,
        stream<Ip4HdrCsum>  &siHCa_Csum,
        stream<AxisIp4>     &soIAe_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICi");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { S0=0, S1, S2 } ici_fsmState=S0;
    #pragma HLS RESET             variable=ici_fsmState

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4     currChunk;
    Ip4HdrCsum  ipHdrChecksum;

    currChunk.setLE_TLast(0);
    switch (ici_fsmState) {
    case S0:
        if (!siHCa_Data.empty() and !soIAe_Data.full()) {
            siHCa_Data.read(currChunk);
            soIAe_Data.write(currChunk);
            ici_fsmState = S1;
        }
        break;
    case S1:
        if (!siHCa_Data.empty() and !siHCa_Csum.empty() and !soIAe_Data.full()) {
            siHCa_Data.read(currChunk);
            siHCa_Csum.read(ipHdrChecksum);
            // Insert the computed ipHeaderChecksum in the incoming stream
            currChunk.setIp4HdrCsum(ipHdrChecksum);
            soIAe_Data.write(currChunk);
            ici_fsmState = S2;
        }
        break;
    default:
        if (!siHCa_Data.empty() and !soIAe_Data.full()) {
            siHCa_Data.read(currChunk);
            soIAe_Data.write(currChunk);
        }
        break;
    } // End-of: switch()

    if (currChunk.getLE_TLast()) {
        ici_fsmState = S0;
    }
}

/*******************************************************************************
 * IPv4 Address Extractor (IAe)
 *
 * @param[in]  piMMIO_SubNetMask The sub-network-mask from [MMIO].
 * @param[in]  piMMIO_GatewayAddrThe default gateway address from [MMIO].
 * @param[in]  siICi_Data        The data stream from IpChecksumInserter ICi).
 * @param[out] soMAi_Data        The data stream to MacAddressInserter (MAi).
 * @param[out] soARP_LookupReq   IPv4 address lookup request to [ARP].
 *
 * @details
 *  This process extracts the IP destination address from the incoming stream
 *  and forwards it to the Address Resolution Protocol server (ARP) in order to
 *  look up the corresponding MAC address.
 *******************************************************************************/
void pIp4AddressExtractor(
        Ip4Addr              piMMIO_SubNetMask,
        Ip4Addr              piMMIO_GatewayAddr,
        stream<AxisIp4>     &siICi_Data,
        stream<AxisIp4>     &soMAi_Data,
        stream<Ip4Addr>     &soARP_LookupReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IAe");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<2>          iae_chunkCount=0;
    #pragma HLS RESET variable=iae_chunkCount

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    Ip4Addr ipDestAddr;

    if (!siICi_Data.empty() and !soMAi_Data.full()) {
        AxisIp4 currChunk = siICi_Data.read();
        switch (iae_chunkCount) {
        case 2:  // Extract destination IP address
            ipDestAddr = currChunk.getIp4DstAddr();
            if ((ipDestAddr & piMMIO_SubNetMask) == (piMMIO_GatewayAddr & piMMIO_SubNetMask)
              || (ipDestAddr == 0xFFFFFFFF)) {
                soARP_LookupReq.write(ipDestAddr);
            }
            else {
                soARP_LookupReq.write(piMMIO_GatewayAddr);
            }
            iae_chunkCount++;
            break;
        default:
            if (currChunk.getLE_TLast()) {
                iae_chunkCount = 0;
            }
            else if (iae_chunkCount < 3) {
                iae_chunkCount++;
            }
            break;
        } // End-of: switch()
        soMAi_Data.write(currChunk);
    }
}

/*******************************************************************************
 * MAC Address Inserter (MAi)
 *
 * @param[in]  piMMIO_MacAddress My Ethernet MAC address from [MMIO].
 * @param[in]  siIAe_Data        The data stream from IpAddressExtractor (IAe).
 * @param[out] siARP_LookupRsp   MAC address looked-up from [ARP].
 * @param[in]  soL2MUX_Data      The data stream to [L2MUX].
 *
 * @details
 *  This process prepends the appropriate Ethernet header to the outgoing IPv4
 *  packet.
 *******************************************************************************/
void pMacAddressInserter(
        EthAddr                  piMMIO_MacAddress,
        stream<AxisIp4>         &siIAe_Data,
        stream<ArpLkpReply>     &siARP_LookupRsp,
        stream<AxisEth>         &soL2MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "MAi");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates {FSM_MAI_WAIT_LOOKUP=0, FSM_MAI_DROP, FSM_MAI_WRITE,
                           FSM_MAI_WRITE_FIRST,   FSM_MAI_WRITE_LAST} \
                               mai_fsmState=FSM_MAI_WAIT_LOOKUP;
    #pragma HLS RESET variable=mai_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static AxisIp4  mai_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth     sendChunk;
    AxisIp4     currChunk;
    ArpLkpReply arpResponse;
    EthAddr     macDstAddr;

    currChunk.setLE_TLast(0);
    switch (mai_fsmState) {
    case FSM_MAI_WAIT_LOOKUP:
        if (!siARP_LookupRsp.empty() and !soL2MUX_Data.full()) {
            siARP_LookupRsp.read(arpResponse);
            macDstAddr = arpResponse.macAddress;
            if (arpResponse.hit) {
                sendChunk.setEthDstAddr(macDstAddr);
                sendChunk.setEthSrcAddrHi(piMMIO_MacAddress);
                sendChunk.setLE_TKeep(0xff);
                sendChunk.setLE_TLast(0);
                soL2MUX_Data.write(sendChunk);
                mai_fsmState = FSM_MAI_WRITE_FIRST;
                if (DEBUG_LEVEL & TRACE_MAI) {
                    printInfo(myName, "FSM_MAI_WAIT_LOOKUP - Lookup=HIT - Received MAC = 0x%12.12lX\n",
                              arpResponse.macAddress.to_ulong());
                    printAxisRaw(myName, "Forwarding AxisChunk to [L2MUX]: ", sendChunk);
                }
            }
            else {  // Drop it all, wait for RT
                mai_fsmState = FSM_MAI_DROP;
                if (DEBUG_LEVEL & TRACE_MAI) {
                    printInfo(myName, "FSM_MAI_WAIT_LOOKUP - Lookup=NO-HIT. \n");
                }
            }
        }
        break;
    case FSM_MAI_WRITE_FIRST:
        if (DEBUG_LEVEL & TRACE_MAI) { printInfo(myName, "FSM_MAI_WRITE_FIRST - \n"); }
        if (!siIAe_Data.empty() and !soL2MUX_Data.full()) {
            siIAe_Data.read(currChunk);
            sendChunk.setEthSrcAddrLo(piMMIO_MacAddress);
            sendChunk.setEthTypeLen(0x0800);
            sendChunk.setIp4HdrLen(currChunk.getIp4HdrLen());
            sendChunk.setIp4Version(currChunk.getIp4Version());
            sendChunk.setIp4ToS(currChunk.getIp4ToS());
            sendChunk.setLE_TKeep(0xff);
            sendChunk.setLE_TLast(0);
            soL2MUX_Data.write(sendChunk);
            mai_prevChunk = currChunk;
            mai_fsmState = FSM_MAI_WRITE;
        }
        break;
    case FSM_MAI_WRITE:
        if (DEBUG_LEVEL & TRACE_MAI) { printInfo(myName, "FSM_MAI_WRITE - \n"); }
        if (!siIAe_Data.empty() and !soL2MUX_Data.full()) {
            siIAe_Data.read(currChunk);
            sendChunk.setLE_TData(mai_prevChunk.getLE_TData(63, 16), 47,  0);
            sendChunk.setLE_TData(    currChunk.getLE_TData(15,  0), 63, 48);
            sendChunk.setLE_TKeep(mai_prevChunk.getLE_TKeep( 7,  2),  5,  0);
            sendChunk.setLE_TKeep(    currChunk.getLE_TKeep( 1,  0),  7,  6);
            if (currChunk.getLE_TKeep()[2] == 0) {
                sendChunk.setLE_TLast(TLAST);
            }
            else {
                 sendChunk.setLE_TLast(0);
            }
            soL2MUX_Data.write(sendChunk);
            if (DEBUG_LEVEL & TRACE_MAI) {
                printAxisRaw(myName, "Forwarding AxisChunk to [L2MUX]: ", sendChunk);
            }
            mai_prevChunk = currChunk;
            if (currChunk.getLE_TLast()) {
                if (currChunk.getLE_TKeep()[2] == 0) {
                    mai_fsmState = FSM_MAI_WAIT_LOOKUP;
                }
                else {
                   mai_fsmState = FSM_MAI_WRITE_LAST;
                }
            }
            else {
                    mai_fsmState = FSM_MAI_WRITE;
                }
        }
        break;
    case FSM_MAI_WRITE_LAST:
        if (DEBUG_LEVEL & TRACE_MAI) { printInfo(myName, "FSM_MAI_WRITE_LAST - \n"); }
        if (!soL2MUX_Data.full()) {
            sendChunk.setLE_TData(mai_prevChunk.getLE_TData(63, 16), 47,  0);
            sendChunk.setLE_TData(                                0, 63, 48);
            sendChunk.setLE_TKeep(mai_prevChunk.getLE_TKeep( 7,  2),  5,  0);
            sendChunk.setLE_TKeep(                                0,  7,  6);
            sendChunk.setLE_TLast(TLAST);
            soL2MUX_Data.write(sendChunk);
            if (DEBUG_LEVEL & TRACE_MAI) {
                printAxisRaw(myName, "Forwarding AxisChunk to [L2MUX]: ", sendChunk);
            }
            mai_fsmState = FSM_MAI_WAIT_LOOKUP;
        }
        break;
    case FSM_MAI_DROP:
        if (DEBUG_LEVEL & TRACE_MAI) { printInfo(myName, "FSM_MAI_DROP - \n"); }
        if (!siIAe_Data.empty() and !soL2MUX_Data.full()) {
            siIAe_Data.read(currChunk);
            if (currChunk.getLE_TLast()) {
                mai_fsmState = FSM_MAI_WAIT_LOOKUP;
            }
        }
        break;

    } // End-of: switch()

} // End-of: pMacAddressInserter

/*******************************************************************************
 * @brief   Main process of the IP Transmitter Handler (IPTX).
 *
 * @param[in]  piMMIO_MacAddress  The MAC address from MMIO (in network order).
 * @param[in]  piMMIO_SubNetMask  The sub-network-mask from [MMIO].
 * @param[in]  piMMIO_GatewayAddr The default gateway address from [MMIO].
 * @param[in]  siL3MUX_Data       The IP4 data stream from the L3 Multiplexer (L3MUX).
 * @param[out] soL2MUX_Data       The ETH data stream to the L2 Multiplexer (L2MUX).
 * @param[out] soARP_LookupReq    The IP4 address lookup request to AddressResolutionProtocol (ARP).
 * @param[in]  siARP_LookupRep    The MAC address looked-up from [ARP].
 *
 * @details: This process receives IP packets from the TCP-offload-engine (TOE),
 *  the Internet Control Message Protocol (ICMP) engine or the UDP Offload
 *  Engine (UOE). It first computes the IP header checksum and inserts it into
 *  the IP packet. Next, it extracts the IP_DA from the incoming data stream and
 *  forwards it to the Address Resolution Protocol server (ARP) in order to
 *  look up the corresponding MAC address. Final, an Ethernet header is created
 *  and is prepended to the outgoing IPv4 packet.
 *
 *******************************************************************************/
void iptx(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,
        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siL3MUX_Data,
        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxisEth>         &soL2MUX_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Header Checksum Accumulator (HCa)
    static stream<AxisIp4>         ssHCaToICi_Data    ("ssHCaToICi_Data");
    #pragma HLS STREAM    variable=ssHCaToICi_Data    depth=1024 // Must hold one IP header for checksum computation
    #pragma HLS DATA_PACK variable=ssHCaToICi_Data               // [FIXME -We only need to store 20 to 24+ bytes]

    static stream<Ip4HdrCsum>      ssHCaToICi_Csum    ("ssHCaToICi_Csum");
    #pragma HLS STREAM    variable=ssHCaToICi_Csum    depth=16

    //-- IP Checksum Inserter (ICi)
    static stream<AxisIp4>         ssICiToIAe_Data    ("ssICiToIAe_Data");
    #pragma HLS STREAM    variable=ssICiToIAe_Data    depth=16
    #pragma HLS DATA_PACK variable=ssICiToIAe_Data

    //-- IP Address Extractor (IAe)
    static stream<AxisIp4>         ssIAeToMAi_Data    ("ssIAeToMAi_Data");
    #pragma HLS STREAM    variable=ssIAeToMAi_Data    depth=16
    #pragma HLS DATA_PACK variable=ssIAeToMAi_Data

    //-- PROCESS FUNCTIONS -----------------------------------------------------

    pHeaderChecksumAccumulator(
            siL3MUX_Data,
            ssHCaToICi_Data,
            ssHCaToICi_Csum);

    pIpChecksumInsert(
            ssHCaToICi_Data,
            ssHCaToICi_Csum,
            ssICiToIAe_Data);

    pIp4AddressExtractor(
            piMMIO_SubNetMask,
            piMMIO_GatewayAddr,
            ssICiToIAe_Data,
            ssIAeToMAi_Data,
            soARP_LookupReq);

    pMacAddressInserter(
            piMMIO_MacAddress,
            ssIAeToMAi_Data,
            siARP_LookupRep,
            soL2MUX_Data);
}

/*******************************************************************************
 * @brief  Top of IP Transmitter Handler (IPTX)
 *
 * @param[in]  piMMIO_MacAddress  The MAC address from MMIO (in network order).
 * @param[in]  piMMIO_SubNetMask  The sub-network-mask from [MMIO].
 * @param[in]  piMMIO_GatewayAddr The default gateway address from [MMIO].
 * @param[in]  siL3MUX_Data       The IP4 data stream from the L3 Multiplexer (L3MUX).
 * @param[out] soL2MUX_Data       The ETH data stream to the L2 Multiplexer (L2MUX).
 * @param[out] soARP_LookupReq    The IP4 address lookup request to AddressResolutionProtocol (ARP).
 * @param[in]  siARP_LookupRep    The MAC address looked-up from [ARP].
 *
 *******************************************************************************/
#if HLS_VERSION == 2017
    void iptx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,
        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxisIp4>         &siL3MUX_Data,
        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxisEth>         &soL2MUX_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable          port=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable          port=piMMIO_SubNetMask
    #pragma HLS INTERFACE ap_stable          port=piMMIO_GatewayAddr

    #pragma HLS resource core=AXI4Stream variable=siL3MUX_Data    metadata="-bus_bundle siL3MUX_Data"
    #pragma HLS resource core=AXI4Stream variable=soL2MUX_Data    metadata="-bus_bundle soL2MUX_Data"

    #pragma HLS resource core=AXI4Stream variable=soARP_LookupReq metadata="-bus_bundle soARP_LookupReq"
    #pragma HLS resource core=AXI4Stream variable=siARP_LookupRep metadata="-bus_bundle siARP_LookupRep"
    #pragma HLS                DATA_PACK variable=siARP_LookupRep

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- MAIN IPTX PROCESS -----------------------------------------------------
    iptx(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_SubNetMask,
        piMMIO_GatewayAddr,
        //-- L3MUX Interface
        siL3MUX_Data,
        //-- L2MUX Interface
        soL2MUX_Data,
        //-- ARP Interface
        soARP_LookupReq,
        siARP_LookupRep);

}
#else
    void iptx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,
        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxisRaw>         &siL3MUX_Data,
        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxisRaw>         &soL2MUX_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE   ap_stable        port=piMMIO_MacAddress
    #pragma HLS INTERFACE   ap_stable        port=piMMIO_SubNetMask
    #pragma HLS INTERFACE   ap_stable        port=piMMIO_GatewayAddr

    #pragma HLS INTERFACE   axis off         port=siL3MUX_Data
    #pragma HLS INTERFACE   axis off         port=soL2MUX_Data

    #pragma HLS INTERFACE   axis off         port=soARP_LookupReq

    #pragma HLS INTERFACE   axis off         port=siARP_LookupRep

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW disable_start_propagation

    //-- LOCAL INPUT and OUTPUT STREAMS ----------------------------------------
    static stream<AxisIp4>          ssiL3MUX_Data ("ssiL3MUX_Data");
    #pragma HLS STREAM     variable=ssiL3MUX_Data depth=8
    static stream<AxisEth>          ssoL2MUX_Data ("ssoL2MUX_Data");

    //-- INPUT STREAM CASTING --------------------------------------------------
    pAxisRawCast(siL3MUX_Data, ssiL3MUX_Data);

    //-- MAIN IPTX PROCESS -----------------------------------------------------
    iptx(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_SubNetMask,
        piMMIO_GatewayAddr,
        //-- L3MUX Interface
        ssiL3MUX_Data,
        //-- L2MUX Interface
        ssoL2MUX_Data,
        //-- ARP Interface
        soARP_LookupReq,
        siARP_LookupRep);

    //-- OUTPUT STREAM CASTING -------------------------------------------------
    pAxisRawCast(ssoL2MUX_Data, soL2MUX_Data);

}
#endif  // HLS_VERSION

/*! \} */
