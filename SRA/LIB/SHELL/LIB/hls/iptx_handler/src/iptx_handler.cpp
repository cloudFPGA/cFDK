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

/*****************************************************************************
 * @file       : iptx_handler.cpp
 * @brief      : IP transmitter frame handler (IPTX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "iptx_handler.hpp"
#include "../../toe/test/test_toe_utils.hpp"

using namespace hls;

#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPd | TRACE_IBUF)
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


/*****************************************************************************
 * IPv4 Header Checksum Accumulator (HCa)
 *
 * @param[in]  siL3MUX_Data, Data stream from the L3 Multiplexer (L3MUX).
 * @param[out] soICi_Data,   Data stream to the IpChecksumInserter (ICi).
 * @param[out] soICi_Csum,   IP header checksum.
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
 *****************************************************************************/
void pHeaderChecksumAccumulator(
        stream<AxiWord>     &siL3MUX_Data,
        stream<AxiWord>     &soICi_Data,
        stream<Ip4HdrCsum>  &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "HCa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                             hca_csumWritten=true;
    #pragma HLS RESET              variable=hca_csumWritten
    static enum FsmState {S0=0, S1, S2, S3} hca_fsmState=S0;
    #pragma HLS RESET              variable=hca_fsmState
    static ap_uint<17>                      hca_ipHdrCsum[4]={0, 0, 0, 0};
    #pragma HLS RESET              variable=hca_ipHdrCsum
    #pragma HLS ARRAY_PARTITION    variable=hca_ipHdrCsum complete dim=1
    static ap_uint<3>                       hca_axiWordCount=0;
    #pragma HLS RESET              variable=hca_axiWordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<4>                       hca_ipHdrLen;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     currWord;
    Ip4HdrCsum  temp;

    currWord.tlast = 0;
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
    else if (!siL3MUX_Data.empty() && !soICi_Data.full() && hca_csumWritten) {
        siL3MUX_Data.read(currWord);
        // Process the IPv4 header.
        //  Remember that the Internet Header Length (IHL) field contains the
        //  size of the IPv4 header specified in number of 32-bit words, and
        //  its default minimum value is 5.
        switch (hca_axiWordCount) {
        case 0:
            hca_ipHdrLen = currWord.tdata.range(3, 0);
            //-- Accumulate bytes [0:7] of the IPv4 header
            for (int i = 0; i < 4; i++) {
              #pragma HLS UNROLL
                temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                temp(15, 8) = currWord.tdata.range(i*16+7,  i*16);
                hca_ipHdrCsum[i] += temp;
                hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
            }
            hca_axiWordCount++;
            // [FIXME - This line was missing]
            hca_ipHdrLen -= 2;
            break;
        case 1:
            //-- Accumulate bytes [8:15] of the IPv4 header.
            for (int i = 0; i < 4; i++) {
              #pragma HLS UNROLL
                if (i != 1) {
                    // Skip 3rd and 4th bytes. They contain the IP header checksum.
                    temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.tdata.range(i*16+7,  i*16);
                    hca_ipHdrCsum[i] += temp;
                    hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
            }
            hca_axiWordCount++;
            hca_ipHdrLen -= 2;
            break;
        default:
            switch (hca_ipHdrLen) {
            case 0:
                //-- We are done with the checksum header processing
                break;
            case 1:
                //-- Process bytes [16:19] --> Handle 1 option max.
                for (int i = 0; i < 2; i++) {
                  #pragma HLS UNROLL
                    temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.tdata.range(i*16+7,  i*16);
                    hca_ipHdrCsum[i] += temp;
                    hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
                hca_ipHdrLen = 0;
                hca_csumWritten = false;
                break;
            case 4:
                for (int i = 0; i < 4; i++) {
                  #pragma HLS UNROLL
                    temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.tdata.range(i*16+7,  i*16);
                    hca_ipHdrCsum[i] += temp;
                    hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
                hca_ipHdrLen = 0;
                hca_csumWritten = false;
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                  #pragma HLS UNROLL
                    temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.tdata.range(i*16+7,  i*16);
                    hca_ipHdrCsum[i] += temp;
                    hca_ipHdrCsum[i] = (hca_ipHdrCsum[i] + (hca_ipHdrCsum[i] >> 16)) & 0xFFFF;
                }
                hca_ipHdrLen -= 2;
                break;
            } // switch ipHeaderLen
            break;
        } // End of switch(hca_axiWordCount)
        soICi_Data.write(currWord);
        if (currWord.tlast) {
            hca_axiWordCount = 0;
        }
    }
}

/*****************************************************************************
 * IP Checksum Inserter (ICi)
 *
 * @param[in]  siHCa_Data, The data stream from HeaderChecksumAccumulator (HCa).
 * @param[in]  siHCa_Csum, The computed IP header checksum from [HCa].
 * @param[out] soIAe_Data, The data stream to IpAddressExtratcor (IAe).
 *
 * @details
 *  This process inserts the computed IP header checksum in the IPv4 packet
 *  being streamed on the data interface.
 *
 *****************************************************************************/
void pIpChecksumInsert(
        stream<AxiWord>     &siHCa_Data,
        stream<Ip4HdrCsum>  &siHCa_Csum,
        stream<AxiWord>     &soIAe_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICi");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0=0, S1, S2 } ici_fsmState=S0;
    #pragma HLS RESET             variable=ici_fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     currWord;
    Ip4HdrCsum  ipHdrChecksum;

	currWord.tlast = 0;
    switch (ici_fsmState) {
    case 0:
        if (!siHCa_Data.empty() && !soIAe_Data.full()) {
            siHCa_Data.read(currWord);
            soIAe_Data.write(currWord);
            ici_fsmState = S1;
        }
        break;
    case 1:
        if (!siHCa_Data.empty() && !siHCa_Csum.empty() && !soIAe_Data.full()) {
            siHCa_Data.read(currWord);
            siHCa_Csum.read(ipHdrChecksum);
            // Insert the computed ipHeaderChecksum in the incoming stream
            currWord.tdata(23, 16) = ipHdrChecksum(15, 8);
            currWord.tdata(31, 24) = ipHdrChecksum( 7, 0);
            soIAe_Data.write(currWord);
            ici_fsmState = S2;
        }
        break;
    default:
        if (!siHCa_Data.empty() && !soIAe_Data.full()) {
            siHCa_Data.read(currWord);
            soIAe_Data.write(currWord);
        }
        break;
    } // End-of: switch()

    if (currWord.tlast) {
        ici_fsmState = S0;
    }
}

/*****************************************************************************
 * IPv4 Address Extractor (IAe)
 *
 * @param[in]  piMMIO_SubNetMask, The sub-network-mask from [MMIO].
 * @param[in]  piMMIO_GatewayAddr,The default gateway address from [MMIO].
 * @param[in]  siICi_Data,        The data stream from IpChecksumInserter ICi).
 * @param[out] soMAi_Data,        The data stream to MacAddressInserter (MAe).
 * @param[out] soARP_LookupReq,   IPv4 address lookup request to [ARP].
 *
 * @details
 *  This process extracts the IP destination address from the incoming stream
 *  and forwards it to the Address Resolution Server in order to look up the
 *  corresponding MAC address.
 *****************************************************************************/
void pIp4AddressExtractor(
        Ip4Addr              piMMIO_SubNetMask,
        Ip4Addr              piMMIO_GatewayAddr,
        stream<AxiWord>     &siICi_Data,
        stream<AxiWord>     &soMAi_Data,
        stream<Ip4Addr>     &soARP_LookupReq)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IAe");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<2>          iae_wordCount=0;
    #pragma HLS RESET variable=iae_wordCount

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    Ip4Addr ipDestAddr;

    if (!siICi_Data.empty() && !soMAi_Data.full()) {
        Ip4overMac currWord = siICi_Data.read();
        switch (iae_wordCount) {
        case 2:  // Extract destination IP address
            ipDestAddr = currWord.getIp4DstAddr();
            // OBSOLETE ipDestAddr = currWord.tdata.range(31, 0);
            if ((ipDestAddr & piMMIO_SubNetMask) == (piMMIO_GatewayAddr & piMMIO_SubNetMask)
              || (ipDestAddr == 0xFFFFFFFF)) {
                soARP_LookupReq.write(ipDestAddr);
            }
            else {
                soARP_LookupReq.write(piMMIO_GatewayAddr);
            }
            iae_wordCount++;
            break;
        default:
            if (currWord.tlast) {
                iae_wordCount = 0;
            }
            else if (iae_wordCount < 3) {
                iae_wordCount++;
            }
            break;
        } // End-of: switch()

        soMAi_Data.write(currWord);
    }
}

/*****************************************************************************
 * MAC Address Inserter (MAi)
 *
 * @param[in]  piMMIO_MacAddress,My Ethernet MAC address from [MMIO].
 * @param[in]  siIAe_Data,       The data stream from IpAddressExtractor (IAe).
 * @param[out] siARP_LookupRsp,  MAC address lookup from [ARP].
 * @param[in]  soL2MUX_Data,     The data stream to [L2MUX].
 *
 * @details
 *  This process inserts the Ethernet MAC address corresponding to the IPv4
 *  destination address that was looked-up by the ARP server.
 *****************************************************************************/
void pMacAddressInserter(
        EthAddr                  piMMIO_MacAddress,
        stream<AxiWord>         &siIAe_Data,
        stream<ArpLkpReply>     &siARP_LookupRsp,
        stream<AxiWord>         &soL2MUX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "MAi");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {WAIT_LOOKUP=0, DROP, WRITE_FIRST, WRITE, WRITE_LAST} \
                               mai_fsmState=WAIT_LOOKUP;
    #pragma HLS RESET variable=mai_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static EthoverMac mai_prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    EthoverMac  sendWord;
    EthoverMac  currWord;
    ArpLkpReply arpResponse;
    EthAddr     macDstAddr;

    currWord.tlast = 0;
    switch (mai_fsmState) {
    case WAIT_LOOKUP:
        if (!siARP_LookupRsp.empty() && !soL2MUX_Data.full()) {
            siARP_LookupRsp.read(arpResponse);
            macDstAddr = arpResponse.macAddress;
            if (arpResponse.hit) {
                sendWord.setEthDstAddr(macDstAddr);
                sendWord.setEthSrcAddrHi(piMMIO_MacAddress);
                sendWord.tkeep = 0xff;
                sendWord.tlast = 0;
                soL2MUX_Data.write(sendWord);
                mai_fsmState = WRITE_FIRST;
                if (DEBUG_LEVEL & TRACE_MAI) {
                    printInfo(myName, "IP lookup = HIT - Received MAC = 0x%12.12lX\n",
                              arpResponse.macAddress.to_ulong());
                }
            }
            else {  // Drop it all, wait for RT
                mai_fsmState = DROP;
                printWarn(myName, "IP lookup = NO-HIT. \n");
            }
        }
        break;
    case WRITE_FIRST:
        if (!siIAe_Data.empty() && !soL2MUX_Data.full()) {
            siIAe_Data.read(currWord);
            sendWord.setEthSrcAddrLo(piMMIO_MacAddress);
            sendWord.setEthTypeLen(0x0800);
            sendWord.tdata(63, 48) = currWord.tdata(15, 0);
            sendWord.tkeep = 0xff;
            sendWord.tlast = 0;
            soL2MUX_Data.write(sendWord);
            mai_prevWord = currWord;
            mai_fsmState = WRITE;
        }
        break;
    case WRITE:
        if (!siIAe_Data.empty() && !soL2MUX_Data.full()) {
            siIAe_Data.read(currWord);
            sendWord.tdata(47,  0) = mai_prevWord.tdata(63, 16);
            sendWord.tdata(63, 48) =     currWord.tdata(15,  0);
            sendWord.tkeep( 5,  0) = mai_prevWord.tkeep( 7,  2);
            sendWord.tkeep( 7,  6) =     currWord.tkeep( 1,  0);
            sendWord.tlast = (currWord.tkeep[2] == 0);
            soL2MUX_Data.write(sendWord);
            if (DEBUG_LEVEL & TRACE_MAI) {
                printAxiWord(myName, "Forwarding AxiWord to [L2MUX]: ", sendWord);
            }
            mai_prevWord = currWord;
        }
        break;
    case WRITE_LAST:
        if (!soL2MUX_Data.full()) {
            sendWord.tdata(47,  0) = mai_prevWord.tdata(63, 16);
            sendWord.tdata(63, 48) = 0;
            sendWord.tkeep( 5,  0) = mai_prevWord.tkeep(7, 2);
            sendWord.tkeep( 7,  6) = 0;
            sendWord.tlast = 1;
            soL2MUX_Data.write(sendWord);
            if (DEBUG_LEVEL & TRACE_MAI) {
                printAxiWord(myName, "Forwarding AxiWord to [L2MUX]: ", sendWord);
            }
            mai_fsmState = WAIT_LOOKUP;
        }
        break;
    case DROP:
        if (!siIAe_Data.empty() && !soL2MUX_Data.full()) {
            siIAe_Data.read(currWord);
            sendWord.tlast = 1;
        }
        break;
    } // End-of: switch()

    if (currWord.tlast) {
        if (currWord.tkeep[2] == 0) {
              mai_fsmState = WAIT_LOOKUP;
        }
        else {
              mai_fsmState = WRITE_LAST;
        }
    } // End-of: else
}

/*****************************************************************************
 * @brief   Main process of the IP Transmitter Handler.
 *
 ******************************************************************************/
void iptx_handler(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                  piMMIO_MacAddress,
        Ip4Addr                  piMMIO_SubNetMask,
        Ip4Addr                  piMMIO_GatewayAddr,

        //------------------------------------------------------
        //-- L3MUX Interface
        //------------------------------------------------------
        stream<AxiWord>         &siL3MUX_Data,

        //------------------------------------------------------
        //-- L2MUX Interface
        //------------------------------------------------------
        stream<AxiWord>         &soL2MUX_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<Ip4Addr>         &soARP_LookupReq,
        stream<ArpLkpReply>     &siARP_LookupRep)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

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

#else

    #pragma HLS INTERFACE   ap_stable        port=piMMIO_MacAddress
    #pragma HLS INTERFACE   ap_stable        port=piMMIO_SubNetMask
    #pragma HLS INTERFACE   ap_stable        port=piMMIO_GatewayAddr

    #pragma HLS INTERFACE   axis             port=siL3MUX_Data
    #pragma HLS INTERFACE   axis             port=soL2MUX_Data

    #pragma HLS INTERFACE   axis             port=soARP_LookupReq

    #pragma HLS INTERFACE   axis             port=siARP_LookupRep

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Header Checksum Accumulator (HCa)
    static stream<AxiWord>         ssHCaToICi_Data    ("ssHCaToICi_Data");
    #pragma HLS STREAM    variable=ssHCaToICi_Data    depth=1024 // Must hold one IP header for checksum computation
    #pragma HLS DATA_PACK variable=ssHCaToICi_Data               // [FIXME -We only need to store 20 to 24+ bytes]

    static stream<Ip4HdrCsum>      ssHCaToICi_Csum    ("ssHCaToICi_Csum");
    #pragma HLS STREAM    variable=ssHCaToICi_Csum    depth=16

    //-- IP Checksum Inserter (ICi)
    static stream<AxiWord>         ssICiToIAe_Data    ("ssICiToIAe_Data");
    #pragma HLS STREAM    variable=ssICiToIAe_Data    depth=16
    #pragma HLS DATA_PACK variable=ssICiToIAe_Data

    //-- IP Address Extractor (IAe)
    static stream<AxiWord>         ssIAeToMAi_Data    ("ssIAeToMAi_Data");
    #pragma HLS STREAM    variable=ssIAeToMAi_Data    depth=16
    #pragma HLS DATA_PACK variable=ssIAeToMAi_Data

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

