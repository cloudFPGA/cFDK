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
 * @file       : arp_server.cpp
 * @brief      : Address Resolution Protocol (ARP) Server
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_ARP
 * \{
 *******************************************************************************/

#include "arp.hpp"

using namespace hls;

#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_APR | TRACE_APS)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "ARP"

#define TRACE_OFF  0x0000
#define TRACE_APR  1 << 1
#define TRACE_APS  1 << 2
#define TRACE_ACC  1 << 3
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * ARP Packet Receiver (APr)
 *
 * @param[in]  piMMIO_IpAddress My IPv4 address from [MMIO].
 * @param[in]  siIPRX_Data      Data stream from the IP Rx Handler (IPRX).
 * @param[out] soAPs_Meta       Meta stream to ARP Packet Sender (APs).
 * @param[out] soACc_UpdateReq  New {MAC,IP} update request to ArpCamController (ACc).
 *
 * @details
 *  This process parses the incoming ARP packet and extracts the relevant ARP
 *  fields as a metadata structure.
 *  If the 'OPERation' field is an ARP-REQUEST, the extracted metadata structure
 *  is forwarded to the ArpPacketSender (APs) which will use that information to
 *  to build an ARP-REPLY packet in response to the incoming ARP-REQUEST.
 *  Next, the {MAC,IP} binding of the ARP-Sender is systematically forwarded to
 *  the ARP-CAM for insertion.
 *
 * @warning
 *  The format of the incoming ARP-over-ETHERNET is as follow:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          HTYPE=0x0001         |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SHA[1]     |    SHA[0]     |    OPER=0x0001 (or 0x0002)    |   PLEN=0x04   |   HLEN=0x06   |          PTYPE=0x0800         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SPA[3]     |    SPA[2]     |    SPA[1]     |    SPA[0]     |    SHA[5]     |    SHA[4]     |    SHA[3]     |    SHA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    TPA[1]     |    TPA[0]     |    THA[5]     |    THA[4]     |    THA[3]     |    THA[2]     |    THA[1]     |    THA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |    TPA[3]     |    TPA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pArpPacketReceiver(
        Ip4Addr                  piMMIO_IpAddress,
        stream<AxisEth>         &siIPRX_Data,
        stream<ArpMeta>         &soAPs_Meta,
        stream<ArpBindPair>     &soACc_UpdateReq)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "APr");

    // An ETH frame w/ an ARP packet but w/o FCS is only 14+28 bytes.
    // However, let's plan for an entire MTU frame.
    const int cW = 8; // (int)(ceil(log2((1500+18)/8)))

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<cW>         apr_chunkCount=0;
    #pragma HLS RESET variable=apr_chunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ArpMeta            apr_meta;
    static ArpOper            apr_opCode;
    static ArpTargProtAddr    arp_targProtAddr;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth currChunk(0, 0, 0);

    enum { CHUNK_0=0, CHUNK_1, CHUNK_2, CHUNK_3, CHUNK_4, CHUNK_5 };

    if (!siIPRX_Data.empty()) {
        siIPRX_Data.read(currChunk);
        switch(apr_chunkCount) {
            case CHUNK_0:
                // Extract MAC_SA[1:0]
                apr_meta.remoteMacAddr(47,32) = currChunk.getEthSrcAddrHi();
                break;
            case CHUNK_1:
                apr_meta.remoteMacAddr(31, 0) = currChunk.getEthSrcAddrLo();
                apr_meta.etherType            = currChunk.getEtherType();
                apr_meta.arpHwType            = currChunk.getArpHwType();
                break;
            case CHUNK_2:
                apr_meta.arpProtType          = currChunk.getArpProtType();
                apr_meta.arpHwLen             = currChunk.getArpHwLen();
                apr_meta.arpProtLen           = currChunk.getArpProtLen();
                apr_opCode                    = currChunk.getArpOper();
                apr_meta.arpSendHwAddr(47,32) = currChunk.getArpShaHi();
                break;
            case CHUNK_3:
                apr_meta.arpSendHwAddr(31, 0) = currChunk.getArpShaLo();
                apr_meta.arpSendProtAddr      = currChunk.getArpSpa();
                break;
            case CHUNK_4:
                arp_targProtAddr(31,16)       = currChunk.getArpTpaHi();
                break;
            case CHUNK_5:
                arp_targProtAddr(15, 0)      = currChunk.getArpTpaLo();
                if (DEBUG_LEVEL & TRACE_APR) {
                    printInfo(myName, "Received ARP packet from IPRX\n");
                    printInfo(myName, "\t srcMacAddr   = 0x%12.12lX \n", apr_meta.remoteMacAddr.to_ulong());
                    printInfo(myName, "\t etherType    = 0x%4.4X    \n", apr_meta.etherType.to_ushort());
                    printInfo(myName, "\t HwType       = 0x%4.4X    \n", apr_meta.arpHwType.to_ushort());
                    printInfo(myName, "\t ProtType     = 0x%4.4X    \n", apr_meta.arpProtType.to_ushort());
                    printInfo(myName, "\t HwLen        = 0x%2.2X    \n", apr_meta.arpHwLen.to_uchar());
                    printInfo(myName, "\t ProtLen      = 0x%2.2X    \n", apr_meta.arpProtLen.to_uchar());
                    printInfo(myName, "\t Operation    = 0x%4.4X    \n", apr_opCode.to_ushort());
                    printInfo(myName, "\t SendHwAddr   = 0x%12.12lX \n", apr_meta.arpSendHwAddr.to_ulong());
                    printInfo(myName, "\t SendProtAddr = 0x%8.8X    \n", apr_meta.arpSendProtAddr.to_uint());
                    printInfo(myName, "\t TargProtAddr = 0x%8.8X    \n", arp_targProtAddr.to_uint());
                }
                break;
            default:
                break;
        } // End of: switch

        if (currChunk.getLE_TLast() == 1) {
            if (apr_opCode == ARP_OPER_REQUEST) {
                if (arp_targProtAddr == piMMIO_IpAddress) {
                    soAPs_Meta.write(apr_meta);
                }
                else if (DEBUG_LEVEL & TRACE_APR) {
                    printInfo(myName,"Skip ARP reply because requested TPA does not match our IP address.\n");
                }
            }
            // Always ask the CAM to add the {MAC,IP} binding of the ARP-Sender
            soACc_UpdateReq.write(ArpBindPair(apr_meta.arpSendHwAddr,
                                              apr_meta.arpSendProtAddr));
            apr_chunkCount = 0;
        }
        else {
            apr_chunkCount++;
        }
    }
}

/*******************************************************************************
 * ARP Packet Sender (APs)
 *
 * @param[in]  piMMIO_MacAddress My MAC address from MemoryMappedIOs (MMIO).
 * @param[in]  piMMIO_IpAddress  My IPv4 address from [MMIO].
 * @param[in]  siAPr_Meta        Meta stream from ArpPacketReceiver (APr).
 * @param[in]  siACc_Meta        Meta stream from ArpCamController (ACc).
 * @param[out] soETH_Data        Data stream to Ethernet (ETH).
 *
  * @details
 *  This process builds an ARP-REPLY packet upon request from the process
 *  ArpPacketReceiver (APr), or an ARP-REQUEST packet upon request from the
 *  process ArpCamController ACc).
 *  The generated ARP packet is then encapsulated into an Ethernet frame.
 *
 * @warning
 *  The format of the outgoing ARP-over-ETHERNET is as follow:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          HTYPE=0x0001         |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SHA[1]     |    SHA[0]     |    OPER=0x0001 (or 0x0002)    |   PLEN=0x04   |   HLEN=0x06   |          PTYPE=0x0800         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SPA[3]     |    SPA[2]     |    SPA[1]     |    SPA[0]     |    SHA[5]     |    SHA[4]     |    SHA[3]     |    SHA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    TPA[1]     |    TPA[0]     |    THA[5]     |    THA[4]     |    THA[3]     |    THA[2]     |    THA[1]     |    THA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |    TPA[3]     |    TPA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pArpPacketSender(
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_IpAddress,
        stream<ArpMeta>     &siAPr_Meta,
        stream<Ip4Addr>     &siACc_Meta,
        stream<AxisEth>     &soETH_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "APr");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { APR_IDLE, APR_REPLY, APR_SENTRQ } aps_fsmState=APR_IDLE;
    #pragma HLS RESET                                variable=aps_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_uint<8> aps_sendCount; // ETH+ARP is only 44 bytes (.i.e 6 quadwords).
    static ArpMeta    aps_aprMeta;
    static Ip4Addr    aps_tpaReq; // Target Protocol Address Request

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth sendCunk(0, 0xFF, 0);

    switch (aps_fsmState) {
    case APR_IDLE:
        aps_sendCount = 0;
        if (!siAPr_Meta.empty()) {
            siAPr_Meta.read(aps_aprMeta);
            aps_fsmState = APR_REPLY;
        }
        else if (!siACc_Meta.empty()) {
            siACc_Meta.read(aps_tpaReq);
            aps_fsmState = APR_SENTRQ;
        }
        break;
    case APR_SENTRQ:
        switch(aps_sendCount) {
        case 0:
            sendCunk.setEthDstAddr(ETH_BROADCAST_ADDR);
            sendCunk.setEthSrcAddrHi(piMMIO_MacAddress);
            break;
        case 1:
            sendCunk.setEthSrcAddrLo(piMMIO_MacAddress);
            sendCunk.setEthertType(ETH_ETHERTYPE_ARP);
            sendCunk.setArpHwType(ARP_HTYPE_ETHERNET);
            break;
        case 2:
            sendCunk.setArpProtType(ARP_PTYPE_IPV4);
            sendCunk.setArpHwLen(ARP_HLEN_ETHERNET);
            sendCunk.setArpProtLen(ARP_PLEN_IPV4);
            sendCunk.setArpOper(ARP_OPER_REQUEST);
            sendCunk.setArpShaHi(piMMIO_MacAddress);
            break;
        case 3:
            sendCunk.setArpShaLo(piMMIO_MacAddress);
            sendCunk.setArpSpa(piMMIO_IpAddress);
            break;
        case 4:
            // MEMO - The THA field is ignored in an ARP request
            sendCunk.setArpTpaHi(aps_tpaReq);
            break;
        case 5:
            sendCunk.setArpTpaLo(aps_tpaReq);
            sendCunk.setLE_TKeep(0x03);
            sendCunk.setLE_TLast(TLAST);
            aps_fsmState = APR_IDLE;
            break;
        } // End-of: switch(aps_sendCount)
        soETH_Data.write(sendCunk);
        aps_sendCount++;
        break;
    case APR_REPLY:
        switch(aps_sendCount) {
        case 0:
            sendCunk.setEthDstAddr(aps_aprMeta.remoteMacAddr);
            sendCunk.setEthSrcAddrHi(piMMIO_MacAddress);
            break;
        case 1:
            sendCunk.setEthSrcAddrLo(piMMIO_MacAddress);
            sendCunk.setEthertType(aps_aprMeta.etherType);
            sendCunk.setArpHwType(aps_aprMeta.arpHwType);
            break;
        case 2:
            sendCunk.setArpProtType(aps_aprMeta.arpProtType);
            sendCunk.setArpHwLen(aps_aprMeta.arpHwLen);
            sendCunk.setArpProtLen(aps_aprMeta.arpProtLen);
            sendCunk.setArpOper(ARP_OPER_REPLY);
            sendCunk.setArpShaHi(piMMIO_MacAddress);
            break;
        case 3:
            sendCunk.setArpShaLo(piMMIO_MacAddress);
            sendCunk.setArpSpa(piMMIO_IpAddress);
            break;
        case 4:
            sendCunk.setArpTha(aps_aprMeta.arpSendHwAddr);
            sendCunk.setArpTpaHi(aps_aprMeta.arpSendProtAddr);
            break;
        case 5:
            sendCunk.setArpTpaLo(aps_aprMeta.arpSendProtAddr);
            sendCunk.setLE_TKeep(0x03);
            sendCunk.setLE_TLast(TLAST);
            aps_fsmState = APR_IDLE;
            break;
        } // End-of: switch(aps_sendCount)
        soETH_Data.write(sendCunk);
        aps_sendCount++;
        break;

    } // End-of: switch (aps_fsmState)
}

/*******************************************************************************
 * ARP CAM Controller (ACc)
 *
 * @param[in]  siAPr_UpdateReq  New {MAC,IP} update request from ArpPacketReceiver (APr).
 * @param[out] soAPs_Meta       Meta stream to ArpPacketSender (APs).
 * @param[in]  siIPTX_MacLkpReq MAC lookup request from [IPTX].
 * @param[out] soIPTX_MacLkpRep MAC lookup reply to [IPTX].
 * @param[out] soCAM_MacLkpReq  MAC lookup request to [CAM].
 * @param[in]  siCAM_MacLkpRep  MAC lookup reply from [CAM].
 * @param[out] soCAM_MacUpdReq  MAC update request to [CAM].
 * @param[in]  siCAM_MacUpdRep  MAC update reply from [CAM].
 *
 * @details
 *  This is the front-end process of the RTL ContentAddressableMemory (CAM). It
 *  serves the MAC lookup requests from the IpTxHandler (IPTX) and MAC update
 *  requests from the ArpPacketReceiver (APr).
 *
 *******************************************************************************/
void pArpCamController(
    stream<ArpBindPair>         &siAPr_UpdateReq,
    stream<Ip4Addr>             &soAPs_Meta,
    stream<Ip4Addr>             &siIPTX_MacLkpReq,
    stream<ArpLkpReply>         &soIPTX_MacLkpRep,
    stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
    stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
    stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
    stream<RtlMacUpdateReply>   &siCAM_MacUpdRep)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ACc");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { ACC_IDLE, ACC_UPDATE, ACC_LOOKUP } acc_fsmState=ACC_IDLE;
    #pragma HLS RESET                                 variable=acc_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static Ip4Addr    acc_ipLkpKey;

    switch(acc_fsmState) {
    case ACC_IDLE:
        if (!siIPTX_MacLkpReq.empty()) {
            acc_ipLkpKey = siIPTX_MacLkpReq.read();
            if (acc_ipLkpKey == IP4_BROADCAST_ADDR) {
                // Reply with Ethernet broadcast address and return immediately
                soIPTX_MacLkpRep.write(ArpLkpReply(ETH_BROADCAST_ADDR, LKP_HIT));
                acc_fsmState = ACC_IDLE;
            }
            else {
                if (DEBUG_LEVEL & TRACE_ACC) {
                    printInfo(myName, "FSM=ACC_IDLE - Request CAM to lookup MAC address binded to:\n");
                    printIp4Addr(myName, acc_ipLkpKey);
                }
                soCAM_MacLkpReq.write(RtlMacLookupRequest(acc_ipLkpKey));
                acc_fsmState = ACC_LOOKUP;
            }
        }
        else if (!siAPr_UpdateReq.empty()) {
            ArpBindPair arpBind = siAPr_UpdateReq.read();
            if (DEBUG_LEVEL & TRACE_ACC) {
                printInfo(myName, "FSM=ACC_IDLE - Request CAM to update:\n");
                printArpBindPair(myName, ArpBindPair(arpBind.macAddr, arpBind.ip4Addr));
            }
            soCAM_MacUpdReq.write(RtlMacUpdateRequest(arpBind.ip4Addr, arpBind.macAddr, ARP_INSERT));
            acc_fsmState = ACC_UPDATE;
        }
        break;
    case ACC_UPDATE:
        if(!siCAM_MacUpdRep.empty()) {
            siCAM_MacUpdRep.read();  // Consume the reply without actually acting on it
            if (DEBUG_LEVEL & TRACE_ACC) {
                printInfo(myName, "FSM=ACC_UPDATE - Done with CAM update.\n");
            }
            acc_fsmState = ACC_IDLE;
        }
        break;
    case ACC_LOOKUP:
        if(!siCAM_MacLkpRep.empty()) {
            RtlMacLookupReply macLkpReply = siCAM_MacLkpRep.read();
            if (DEBUG_LEVEL & TRACE_ACC) {
                printInfo(myName, "FSM=ACC_LOOKUP\n");
            }
            soIPTX_MacLkpRep.write(ArpLkpReply(macLkpReply.value, macLkpReply.hit));
            if (!macLkpReply.hit) {
                if (DEBUG_LEVEL & TRACE_ACC) {
                    printInfo(myName, "NO-HIT. Go and fire an ARP-REQUEST for:\n");
                    printIp4Addr(myName, acc_ipLkpKey);
                }
                soAPs_Meta.write(acc_ipLkpKey);
            }
            else {
                if (DEBUG_LEVEL & TRACE_ACC) {
                    printInfo(myName, "HIT. Retrieved following address from CAM:\n");
                    printEthAddr(myName, macLkpReply.value);
                }
            }
            acc_fsmState = ACC_IDLE;
        }
        break;
    }
}

/*****************************************************************************
 * @brief   Main process of the Address Resolution Protocol (ARP) Server.
 *
 * @param[in]  piMMIO_MacAddress The MAC  address from MMIO (in network order).
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siIPRX_Data       Data stream from the IP Rx Handler (IPRX).
 * @param[out] soETH_Data        Data stream to Ethernet (ETH).
 * @param[in]  siIPTX_MacLkpReq  MAC lookup request from [IPTX].
 * @param[out] soIPTX_MacLkpRep  MAC lookup reply to [IPTX].
 * @param[out] soCAM_MacLkpReq   MAC lookup request to [CAM].
 * @param[in]  siCAM_MacLkpRep   MAC lookup reply from [CAM].
 * @param[out] soCAM_MacUpdReq   MAC update request to [CAM].
 * @param[in]  siCAM_MacUpdRep   MAC update reply from [CAM].
 *
 * @details
 *  This process maintains a Content Addressable Memory (CAM) which holds the
 *  association of a layer-2 MAC address with a layer-3 IP address.
 *  The content of the CAM is continuously updated with any new {MAC_SA, IP_SA}
 *  binding seen by the IP Rx packet handler (IPRX) at the receive side of the
 *  FPGA.
 *  When an IP packet is transmitted by the FPGA, the 'IP_DA' is used as key to
 *  lookup the CAM and retrieve the associated 'MAC_DA' value to be used for
 *  building the outgoing Ethernet frame. If there is not a known binding for
 *  a given 'IP_DA', this process will build a broadcast ARP-REQUEST message
 *  that will be sent to every node of the local network, and will wait for the
 *  corresponding ARP-RESPONSE message to come back before forwarding the IP
 *  packet that is currently waiting for its address to be resolved.
 *  Conversely, if a broadcast ARP-REQUEST message that matches the ARP binding
 *  of the current FPGA, this process is in charge of building the corresponding
 *  ARP-REPLY message that is to be sent back in response to the incoming ARP
 *  request.
 ******************************************************************************/
void arp(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_Ip4Address,

        //------------------------------------------------------
        //-- IPRX Interface
        //------------------------------------------------------
        stream<AxisEth>             &siIPRX_Data,

        //------------------------------------------------------
        //-- ETH Interface
        //------------------------------------------------------
        stream<AxisEth>             &soETH_Data,

        //------------------------------------------------------
        //-- IPTX Interfaces
        //------------------------------------------------------
        stream<Ip4Addr>             &siIPTX_MacLkpReq,
        stream<ArpLkpReply>         &soIPTX_MacLkpRep,

        //------------------------------------------------------
        //-- CAM Interfaces
        //------------------------------------------------------
        stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
        stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
        stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
        stream<RtlMacUpdateReply>   &siCAM_MacUpdRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- ARP Packet Receiver (APr)
    static stream<ArpMeta>         ssAPrToAPs_Meta      ("ssAPrToAPs_Meta");
    #pragma HLS STREAM    variable=ssAPrToAPs_Meta      depth=4
    #pragma HLS DATA_PACK variable=ssAPrToAPs_Meta
    static stream<ArpBindPair>     ssAPrToACc_UpdateReq ("ssAPrToACc_UpdateReq");
    #pragma HLS STREAM    variable=ssAPrToACc_UpdateReq depth=4
    #pragma HLS DATA_PACK variable=ssAPrToACc_UpdateReq

    //-- ARP CAM Controller (ACc)
    static stream<LE_Ip4Addr>      ssACcToAPs_Meta      ("ssACcToAPs_Meta");
    #pragma HLS STREAM    variable=ssACcToAPs_Meta      depth=4

    pArpPacketReceiver(
        piMMIO_Ip4Address,
        siIPRX_Data,
        ssAPrToAPs_Meta,
        ssAPrToACc_UpdateReq);

    pArpPacketSender(
        piMMIO_MacAddress,
        piMMIO_Ip4Address,
        ssAPrToAPs_Meta,
        ssACcToAPs_Meta,
        soETH_Data);

    pArpCamController(
        ssAPrToACc_UpdateReq,
        ssACcToAPs_Meta,
        siIPTX_MacLkpReq,
        soIPTX_MacLkpRep,
        soCAM_MacLkpReq,
        siCAM_MacLkpRep,
        soCAM_MacUpdReq,
        siCAM_MacUpdRep);
}

/*******************************************************************************
 * @brief Top of Address Resolution Protocol (ARP) Server.
 *
 * @param[in]  piMMIO_MacAddress The MAC  address from MMIO (in network order).
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siIPRX_Data       Data stream from the IP Rx Handler (IPRX).
 * @param[out] soETH_Data        Data stream to Ethernet (ETH).
 * @param[in]  siIPTX_MacLkpReq  MAC lookup request from [IPTX].
 * @param[out] soIPTX_MacLkpRep  MAC lookup reply to [IPTX].
 * @param[out] soCAM_MacLkpReq   MAC lookup request to [CAM].
 * @param[in]  siCAM_MacLkpRep   MAC lookup reply from [CAM].
 * @param[out] soCAM_MacUpdReq   MAC update request to [CAM].
 * @param[in]  siCAM_MacUpdRep   MAC update reply from [CAM].
 *
 *******************************************************************************/
#if HLS_VERSION == 2017
    void arp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interface
        //------------------------------------------------------
        stream<AxisEth>             &siIPRX_Data,
        //------------------------------------------------------
        //-- ETH Interface
        //------------------------------------------------------
        stream<AxisEth>             &soETH_Data,
        //------------------------------------------------------
        //-- IPTX Interfaces
        //------------------------------------------------------
        stream<Ip4Addr>             &siIPTX_MacLkpReq,
        stream<ArpLkpReply>         &soIPTX_MacLkpRep,
        //------------------------------------------------------
        //-- CAM Interfaces
        //------------------------------------------------------
        stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
        stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
        stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
        stream<RtlMacUpdateReply>   &siCAM_MacUpdRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable          port=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable          port=piMMIO_Ip4Address

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data        metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soETH_Data         metadata="-bus_bundle soETH_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPTX_MacLkpReq   metadata="-bus_bundle siIPTX_MacLkpReq"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_MacLkpRep   metadata="-bus_bundle soIPTX_MacLkpRep"
    #pragma HLS DATA_PACK                variable=soIPTX_MacLkpRep

    #pragma HLS RESOURCE core=AXI4Stream variable=soCAM_MacLkpReq    metadata="-bus_bundle soCAM_MacLkpReq"
    #pragma HLS DATA_PACK                variable=soCAM_MacLkpReq
    #pragma HLS RESOURCE core=AXI4Stream variable=siCAM_MacLkpRep    metadata="-bus_bundle siCAM_MacLkpRep"
    #pragma HLS DATA_PACK                variable=siCAM_MacLkpRep
    #pragma HLS RESOURCE core=AXI4Stream variable=soCAM_MacUpdReq    metadata="-bus_bundle soCAM_MacUpdReq"
    #pragma HLS DATA_PACK                variable=soCAM_MacUpdReq
    #pragma HLS RESOURCE core=AXI4Stream variable=siCAM_MacUpdRep    metadata="-bus_bundle siCAM_MacUpdRep"
    #pragma HLS DATA_PACK                variable=siCAM_MacUpdRep

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- MAIN ARP PROCESS ------------------------------------------------------
    arp(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_Ip4Address,
        //-- IPRX Interface
        siIPRX_Data,
        //-- ETH Interface
        soETH_Data,
        //-- IPTX Interfaces
        siIPTX_MacLkpReq,
        soIPTX_MacLkpRep,
        //-- CAM Interfaces
        soCAM_MacLkpReq,
        siCAM_MacLkpRep,
        soCAM_MacUpdReq,
        siCAM_MacUpdRep);
}
#else
    void arp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr                      piMMIO_MacAddress,
        Ip4Addr                      piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interface
        //------------------------------------------------------
        stream<AxisRaw>             &siIPRX_Data,
        //------------------------------------------------------
        //-- ETH Interface
        //------------------------------------------------------
        stream<AxisRaw>             &soETH_Data,
        //------------------------------------------------------
        //-- IPTX Interfaces
        //------------------------------------------------------
        stream<Ip4Addr>             &siIPTX_MacLkpReq,
        stream<ArpLkpReply>         &soIPTX_MacLkpRep,
        //------------------------------------------------------
        //-- CAM Interfaces
        //------------------------------------------------------
        stream<RtlMacLookupRequest> &soCAM_MacLkpReq,
        stream<RtlMacLookupReply>   &siCAM_MacLkpRep,
        stream<RtlMacUpdateRequest> &soCAM_MacUpdReq,
        stream<RtlMacUpdateReply>   &siCAM_MacUpdRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable          port=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable          port=piMMIO_Ip4Address

    #pragma HLS INTERFACE axis off           port=siIPRX_Data
    #pragma HLS INTERFACE axis off           port=soETH_Data

    #pragma HLS INTERFACE axis register both port=siIPTX_MacLkpReq
    #pragma HLS INTERFACE axis register both port=soIPTX_MacLkpRep
    #pragma HLS DATA_PACK                variable=soIPTX_MacLkpRep

    #pragma HLS INTERFACE axis register both port=soCAM_MacLkpReq
    #pragma HLS DATA_PACK                variable=soCAM_MacLkpReq
    #pragma HLS INTERFACE axis register both port=siCAM_MacLkpRep
    #pragma HLS DATA_PACK                variable=siCAM_MacLkpRep
    #pragma HLS INTERFACE axis register both port=soCAM_MacUpdReq
    #pragma HLS DATA_PACK                variable=soCAM_MacUpdReq
    #pragma HLS INTERFACE axis register both port=siCAM_MacUpdRep
    #pragma HLS DATA_PACK                variable=siCAM_MacUpdRep

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW disable_start_propagation

    //-- LOCAL INPUT and OUTPUT STREAMS ----------------------------------------
    static stream<AxisEth>  ssiIPRX_Data;
    static stream<AxisEth>  ssoETH_Data;

    //-- INPUT STREAM CASTING --------------------------------------------------
    pAxisRawCast(siIPRX_Data, ssiIPRX_Data);

    //-- MAIN ARP PROCESS ------------------------------------------------------
    arp(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_Ip4Address,
        //-- IPRX Interface
        ssiIPRX_Data,
        //-- ETH Interface
        ssoETH_Data,
        //-- IPTX Interfaces
        siIPTX_MacLkpReq,
        soIPTX_MacLkpRep,
        //-- CAM Interfaces
        soCAM_MacLkpReq,
        siCAM_MacLkpRep,
        soCAM_MacUpdReq,
        siCAM_MacUpdRep);

    //-- OUTPUT STREAM CASTING -------------------------------------------------
    pAxisRawCast(ssoETH_Data,  soETH_Data);
}

#endif

/*! \} */
