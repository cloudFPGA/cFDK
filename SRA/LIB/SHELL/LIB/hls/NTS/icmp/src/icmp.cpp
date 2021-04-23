/************************************************
Copyright (c) 2016-2020, IBM Research.
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
 * @file       : icmp.cpp
 * @brief      : Internet Control Message Protocol (ICMP) Server
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_ICMP
 * \{
 *******************************************************************************/

#include "icmp.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPd | TRACE_IBUF)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "ICMP"

#define TRACE_OFF  0x0000
#define TRACE_CMB  1 << 1
#define TRACE_ICC  1 << 2
#define TRACE_ICI  1 << 3
#define TRACE_IHA  1 << 4
#define TRACE_IPD  1 << 5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * ICMP header Checksum accumulator and Checker (ICc)
 *
 * @param[in]  siIPRX_Data    The data stream from the IP Rx handler (IPRX).
 * @param[out] soIPd_Data     The data stream to IcmpPacketDropper (IPd)
 * @param[out] soIPd_DropCmd  THe drop command information for [IPd].
 * @param[out] soICi_Csum     ICMP checksum to IcmpChecksumInserter (ICi).
 *
 * @details
 *   This process handles the incoming data stream from the IPRX. It assesses
 *   the validity of the ICMP checksum while forwarding the data stream to the
 *   InvalidPacketDropper (IPd).
 *   If the incoming packet is an ICMP_REQUEST, the packet forwarded to [IPd] is
 *   turned into an ICMP_REPLY by overwriting the 'Type' field. To avoid the
 *   recomputing of the entire ICMP checksum, an incremental checksum update is
 *   performed as described in RFC-1624. The resulting new header checksum is
 *   then forwarded to the IcmpChecksumInserter (ICi).
 *
 * @warning
 *   It is expected that the IP header does not have any option.
 *
 *  The format of the incoming ICMP message embedded into an IPv4 packet is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Checksum            |      Code     |     Type      |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                    Rest of Header (content varies based on the ICMP Type and Code)                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/
void pIcmpChecksumChecker(
        stream<AxisIp4>         &siIPRX_Data,
        stream<AxisIp4>         &soIPd_Data,
        stream<ValBool>         &soIPd_DropCmd,
        stream<IcmpCsum>        &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    const char *myName  = concat3(THIS_NAME, "/", "ICc");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                                icc_writeLastOne=false;
    #pragma HLS RESET                 variable=icc_writeLastOne
    static bool                                icc_computeCs=false;
    #pragma HLS RESET                 variable=icc_computeCs
    static ap_uint<7>                          icc_chunkCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET                 variable=icc_chunkCount
    static enum FsmStates { S0=0, S1, S2, S3 } icc_csumState=S0;
    #pragma HLS RESET                 variable=icc_csumState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIp4  icc_prevChunk;
    static Sum17    icc_subSums[4];
    static IcmpCsum icc_oldHCsum;
    static Sum17    icc_newHCsum;
    static IcmpType icc_icmpType;
    static IcmpCode icc_icmpCode;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4 currChunk;
    AxisIp4 sendChunk;

    enum { CHUNK_0=0, CHUNK_1, CHUNK_2, CHUNK_3, CHUNK_4, CHUNK_5 };

    currChunk.setLE_TLast(0);

    if (icc_writeLastOne) {
        // Forward the very last AxisChunk
        soIPd_Data.write(icc_prevChunk);
        icc_writeLastOne = false;
    }
    else if (icc_computeCs) {
        switch (icc_csumState) {
        case S0:
            icc_subSums[0] += icc_subSums[2];
            icc_subSums[0] = (icc_subSums[0] + (icc_subSums[0] >> 16)) & 0xFFFF;
            icc_subSums[1] += icc_subSums[3];
            icc_subSums[1] = (icc_subSums[1] + (icc_subSums[1] >> 16)) & 0xFFFF;
            //-- [RFC-1624] -->  HC' = ~(~HC + ~m + m')
            icc_newHCsum = ~icc_oldHCsum & 0xFFFF;
            icc_csumState = S1;
            break;
        case S1:
            icc_subSums[0] += icc_subSums[1];
            icc_subSums[0] = (icc_subSums[0] + (icc_subSums[0] >> 16)) & 0xFFFF;
            if ((icc_icmpType == ICMP_ECHO_REQUEST) && (icc_icmpCode == 0)) {
                // Message is a PING -> Perform incremental update of chechsum
                //-- [RFC-1624] -->  HC' = ~(~HC + ~m + m')
                icc_newHCsum = icc_newHCsum + (~0x0800 & 0xFFFF) + 0x0000;
                icc_newHCsum = (icc_newHCsum + (icc_newHCsum >> 16)) & 0xFFFF;
            }
            icc_csumState = S2;
            break;
        case S2:
            icc_subSums[0] = ~icc_subSums[0];
            //-- [RFC-1624] -->  HC' = ~(~HC + ~m + m')
            icc_newHCsum = ~icc_newHCsum & 0xFFFF;
            icc_csumState = S3;
            break;
        case S3:
            if (icc_subSums[0](15, 0) == 0) {
                if (DEBUG_LEVEL & TRACE_ICC) {
                    printInfo(myName, "\tThe checksum is valid.\n");
                }
                if ((icc_icmpType == ICMP_ECHO_REQUEST) && (icc_icmpCode == 0)) {
                    soICi_Csum.write(icc_newHCsum.range(15, 0));
                    soIPd_DropCmd.write(CMD_KEEP);
                    if (DEBUG_LEVEL & TRACE_ICC) {
                        printInfo(myName, "\tThe control message is ECHO-REQUEST (.i.e Ping).\n");
                        printInfo(myName, "\t\tThe computed new checksum for ECHO-REPLY is 0x%4.4X.\n",
                                  icc_newHCsum.range(15, 0).to_uint());
                    }
                }
                else {
                    soIPd_DropCmd.write(CMD_DROP);
                    if (DEBUG_LEVEL & TRACE_ICC) {
                        printWarn(myName, "\tThis control message is not supported.\n");
                        printInfo(myName, "\t\tThe message-type is %d and message-code is %d.\n",
                                  icc_icmpType.to_int(), icc_icmpCode.to_int());
                    }
                }
            }
            else {
                soIPd_DropCmd.write(CMD_DROP);
                if (DEBUG_LEVEL & TRACE_ICC) {
                    printWarn(myName, "\tThe checksum is invalid.\n");
                }
            }
            icc_csumState = S0;
            icc_computeCs = false;
            break;
        }
    }
    else if (!siIPRX_Data.empty()) {
        siIPRX_Data.read(currChunk);
        switch (icc_chunkCount) {
        case CHUNK_0: // The current chunk contains [ FO | Id | TotLen | ToS | IHL ]
            icc_subSums[0] = 0;
            icc_subSums[1] = 0;
            icc_subSums[2] = 0;
            icc_subSums[3] = 0;
            break;
        case CHUNK_1: // The current chunk contains [ SA | HdCsum | Prot | TTL ]
            sendChunk = icc_prevChunk;
            soIPd_Data.write(sendChunk);
            break;
        case CHUNK_2: // The current chunk contains [ Csum | Code | Type | DA ]
            // Save ICMP Type, Code and Checksum
            icc_icmpType = currChunk.getIcmpType();
            icc_icmpCode = currChunk.getIcmpCode();
            icc_oldHCsum = currChunk.getIcmpCsum();
            // Forward data stream while swapping IP_SA & IP_DA
            sendChunk.setIp4TtL(icc_prevChunk.getIp4TtL());
            sendChunk.setIp4Prot(icc_prevChunk.getIp4Prot());
            sendChunk.setIp4HdrCsum(icc_prevChunk.getIp4HdrCsum());
            sendChunk.setIp4SrcAddr(currChunk.getIp4DstAddr());
            sendChunk.setLE_TKeep(0xFF);
            sendChunk.setLE_TLast(0);
            soIPd_Data.write(sendChunk);
            // Accumulate [ Csum | Code | Type ]
            for (int i=2; i<4; i++) {
              #pragma HLS UNROLL
                ap_uint<16> temp;
                temp( 7, 0) = currChunk.getLE_TData(i*16+15, i*16+8);
                temp(15, 8) = currChunk.getLE_TData(i*16+ 7, i*16+0);
                icc_subSums[i] += temp;
                icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
            }
            // Replace the IP_DA field with IP_SA
            currChunk.setIp4DstAddr(icc_prevChunk.getIp4SrcAddr());
            // Replace ECHO_REQUEST field with ECHO_REPLY
            currChunk.setIcmpType(ICMP_ECHO_REPLY);
            break;
        default:
            // Accumulate quadword
            for (int i=0; i<4; i++) {
              #pragma HLS UNROLL
                ap_uint<16> temp;
                if (currChunk.getLE_TKeep(i*2+1, i*2) == 0x3) {
                    temp( 7, 0) = currChunk.getLE_TData(i*16+15, i*16+8);
                    temp(15, 8) = currChunk.getLE_TData(i*16+ 7, i*16+0);
                    icc_subSums[i] += temp;
                    icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
                }
                else if (currChunk.getLE_TKeep()[i*2] == 0x1) {
                    temp( 7, 0) = 0;
                    temp(15, 8) = currChunk.getLE_TData(i*16+7, i*16);
                    icc_subSums[i] += temp;
                    icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
                }
            }
            sendChunk = icc_prevChunk;
            soIPd_Data.write(sendChunk);
            break;
        } // End-of: switch (icc_chunkCount)

        icc_prevChunk = currChunk;
        icc_chunkCount++;

        if (currChunk.getLE_TLast()) {
            icc_chunkCount = 0;
            icc_writeLastOne = true;
            icc_computeCs = true;
            if (DEBUG_LEVEL & TRACE_ICC) {
                printInfo(myName, "Received a new message (checksum=0x%4.4X)\n",
                          icc_oldHCsum.to_uint());
            }
        }
    }
}

/*******************************************************************************
 * Control Message Builder (CMb)
 *
 * @param[in]  siUOE_Data  The data stream from the UDP offload engine (UDP).
 * @param[in]  siIPRX_Derr Erroneous IP data stream from IpRxHandler (IPRX).
 * @param[out] soIHa_Data  Data stream to IpHeaderAppender (IHa).
 * @param[out] soIHa_Hdr   Header data stream to IpHeaderAppender (IHa).
 * @param[out] soICi_Csum  ICMP checksum to ICi.
 *
 * @details
 *  This process is dedicated to the building of 2 types of error messages:
 *  1) It creates an error message of type "Time Exceeded" upon reception of an
 *     IPv4 packet which TTL has expired.
 *  2) It creates an error message of type 'Destination Port Unreachable' if an
 *     UDP datagram is received for a port that is not opened in listening mode.
 *******************************************************************************/
void pControlMessageBuilder(
        stream<AxisIcmp>        &siUOE_Data,
        stream<AxisIp4>         &siIPRX_Derr,
        stream<AxisIcmp>        &soIHa_Data,
        stream<AxisIp4>         &soIHa_IpHdr,
        stream<IcmpCsum>        &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "CMb");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { BCM_IDLE,   BCM_IP, \
                            BCM_STREAM, BCM_CS } bcm_fsmState=BCM_IDLE;
    #pragma HLS reset                   variable=bcm_fsmState

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_uint<3>   ipChunkCounter;  // Up to 64-bytes IP packet
    static ap_uint<20>  checksumAcc;

    static ap_uint<1>   streamSource    = 0; // 0 is UDP, 1 is IP Handler (Destination unreachable & TTL exceeded respectively)

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    StsBit       udpInEmpty;
    StsBit       iprxInEmpty;

    udpInEmpty  = siUOE_Data.empty();
    iprxInEmpty = siIPRX_Derr.empty();

    switch(bcm_fsmState) {
        case BCM_IDLE:
            //-- Assemble an ICMP header = [Type | Code | Checksum]
            if ((udpInEmpty == 0 || iprxInEmpty == 0) && !soIHa_Data.full()) {
                // Data are available in at lease one of the input queue(s).
                // Don't read them yet but start assembling a ICMP header
                ipChunkCounter = 0;
                AxisIcmp axisIcmp(0, 0xFF, 0);
                if (udpInEmpty == 0) {
                    axisIcmp.setIcmpType(ICMP_DESTINATION_UNREACHABLE);
                    axisIcmp.setIcmpCode(ICMP_DESTINATION_PORT_UNREACHABLE);
                    streamSource = 0;
                }
                else if (iprxInEmpty == 0) {
                    axisIcmp.setIcmpType(ICMP_TIME_EXCEEDED);
                    axisIcmp.setIcmpCode(ICMP_TTL_EXPIRED_IN_TRANSIT);
                    streamSource = 1;
                }
                checksumAcc = (((axisIcmp.getLE_TData(63, 48)  + axisIcmp.getLE_TData(47, 32)) +
                                 axisIcmp.getLE_TData(31, 16)) + axisIcmp.getLE_TData(15,  0));
                soIHa_Data.write(axisIcmp);
                bcm_fsmState = BCM_IP;
            }
            break;
        case BCM_IP:
            //-- Forward the IP header to [IHa]
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && iprxInEmpty == 0)) &&
                !soIHa_Data.full() && !soIHa_IpHdr.full()) {
                // Start reading data from one of the input queues.
                AxisIcmp axisIcmp(0, 0, 0);
                if (streamSource == 0) {
                    axisIcmp = siUOE_Data.read();
                }
                else if (streamSource == 1) {
                    axisIcmp = siIPRX_Derr.read();
                }
                checksumAcc = (checksumAcc +
                              ((axisIcmp.getLE_TData(63, 48) + axisIcmp.getLE_TData(47, 32)) +
                                axisIcmp.getLE_TData(31, 16) + axisIcmp.getLE_TData(15,  0)));
                soIHa_Data.write(axisIcmp);
                soIHa_IpHdr.write(axisIcmp);
                if (ipChunkCounter == 2) {
                    bcm_fsmState = BCM_STREAM;
                }
                else {
                    ipChunkCounter++;
                }
            }
            break;  
        case BCM_STREAM:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && iprxInEmpty == 0)) && !soIHa_Data.full()) { // If there are data in the queue start reading them
                AxisIcmp axisIcmp(0, 0, 0);
                if (streamSource == 0) {
                    axisIcmp = siUOE_Data.read();
                }
                else if (streamSource == 1) {
                    axisIcmp = siIPRX_Derr.read();
                }
                checksumAcc = (checksumAcc +
                              ((axisIcmp.getLE_TData(63, 48) + axisIcmp.getLE_TData(47, 32)) +
                                axisIcmp.getLE_TData(31, 16) + axisIcmp.getLE_TData(15,  0)));
                soIHa_Data.write(axisIcmp);
                if (axisIcmp.getLE_TLast()) {
                    bcm_fsmState = BCM_CS;
                }
            }
            break;
        case BCM_CS:
            if (!soICi_Csum.full()) {
                checksumAcc = (checksumAcc & 0xFFFF) + (checksumAcc >> 16);
                checksumAcc = (checksumAcc & 0xFFFF) + (checksumAcc >> 16);
                // Reverse the bits and forward to [ICi]
                checksumAcc = ~checksumAcc;
                soICi_Csum.write(checksumAcc.range(15, 0));
                bcm_fsmState = BCM_IDLE;
            }
            break;
    }
}

/*******************************************************************************
 * IP Header Appender (IHa)
 *
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siCMb_Data        Data stream from ControlMessageBuilder (CMb).
 * @param[in]  siIHa_IpHdr       The IP header part of a data stream.
 * @param[out] soICi_Data        Data stream to IcmpChecksumInserter (ICi).
 *
 *******************************************************************************/
void pIpHeaderAppender(
        Ip4Addr                  piMMIO_Ip4Address,
        stream<AxisIcmp>        &siCMb_Data,
        stream<AxisIp4>         &siIHa_IpHdr,
        stream<AxisIp4>         &soICi_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { AIH_IDLE=0, AIH_IP, AIH_MERGE, \
                            AIH_STREAM, AIH_RESIDUE } aih_fsmState=AIH_IDLE;
    #pragma HLS RESET                        variable=aih_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIcmp iha_icmpData(0, 0, 0);
    static AxisIp4  iha_ipHdrChunk(0, 0, 0);
    static Ip4Addr  iha_remoteIpHostAddr;

    switch(aih_fsmState) {
        case AIH_IDLE:
            if (!siIHa_IpHdr.empty() && !soICi_Data.full()) {
                //-- Forward the 1st IP header chunk
                siIHa_IpHdr.read(iha_ipHdrChunk);
                Ip4TotalLen ip4TotLen = iha_ipHdrChunk.getIp4TotalLen() + 28;
                iha_ipHdrChunk.setIp4TotalLen(ip4TotLen);
                iha_ipHdrChunk.setLE_TKeep(0xFF);
                iha_ipHdrChunk.setLE_TLast(0);
                soICi_Data.write(iha_ipHdrChunk);
                aih_fsmState = AIH_IP;
            }
            break;
        case AIH_IP:
            if (!siIHa_IpHdr.empty() && !soICi_Data.full()) {
                //-- Forward the 2nd IP header chunk
                siIHa_IpHdr.read(iha_ipHdrChunk);
                iha_ipHdrChunk.setIp4TtL(0x80);
                iha_ipHdrChunk.setIp4Prot(ICMP_PROTOCOL);
                // Save the remote host address and set the new IP_SA
                iha_remoteIpHostAddr = iha_ipHdrChunk.getIp4SrcAddr();
                iha_ipHdrChunk.setIp4SrcAddr(piMMIO_Ip4Address);
                iha_ipHdrChunk.setLE_TKeep(0xFF);
                iha_ipHdrChunk.setLE_TLast(0);
                soICi_Data.write(iha_ipHdrChunk);
                aih_fsmState = AIH_MERGE;
            }
            break;
        case AIH_MERGE:
            if (!siIHa_IpHdr.empty() && !siCMb_Data.empty() &&
                !soICi_Data.full()) {
                siIHa_IpHdr.read(); // Drain and drop this chunk
                //-- Merge 3rd IP header chunk with 1st ICMP datagram
                iha_icmpData = siCMb_Data.read();
                AxisIp4 thirdChunk(0, 0xFF, 0);
                thirdChunk.setIp4DstAddr(iha_remoteIpHostAddr);
                thirdChunk.setIcmpType(iha_icmpData.getIcmpType());
                thirdChunk.setIcmpCode(iha_icmpData.getIcmpCode());
                thirdChunk.setIcmpCsum(iha_icmpData.getIcmpCsum());
                soICi_Data.write(thirdChunk);
                aih_fsmState = AIH_STREAM;
            }
            break;
        case AIH_STREAM:
            if (!siCMb_Data.empty() && !soICi_Data.full()) {
                AxisIp4  outputChunk(0, 0xFF, 0);
                outputChunk.setLE_TData(iha_icmpData.getLE_TData(63, 32), 31,  0);
                iha_icmpData = siCMb_Data.read();
                outputChunk.setLE_TData(iha_icmpData.getLE_TData(31,  0), 63, 32);
                if (iha_icmpData.getLE_TLast()) {
                    if (iha_icmpData.getLE_TKeep(7, 4) == 0) {
                        outputChunk.setLE_TLast(TLAST);
                        outputChunk.setLE_TKeep(iha_icmpData.getLE_TKeep(3,0), 7, 4);
                        aih_fsmState = AIH_IDLE;
                    }
                    else {
                        aih_fsmState = AIH_RESIDUE;
                    }
                }
                soICi_Data.write(outputChunk);
            }
            break;
        case AIH_RESIDUE:
            if (!soICi_Data.full()) {
                AxisIp4 outputChunk(0, 0, 1);
                outputChunk.setLE_TData(iha_icmpData.getLE_TData(63, 32), 31, 0);
                outputChunk.setLE_TKeep(iha_icmpData.getLE_TKeep( 7,  4),  3, 0);
                soICi_Data.write(outputChunk);
                aih_fsmState = AIH_IDLE;
            }
            break;
    }
}

/*******************************************************************************
 * Invalid Packet Dropper (IPd)
 *
 * @param[in]  siICc_Data    Data stream from IcmpChecksumChecker (ICc).
 * @param[in]  siICc_DropCmd The drop command information from [ICc].
 * @param[out] soICi_Data    Data stream to IcmpChecksumInserter (ICi).
 *
 * @details
 *  This process drops the messages which arrive with a bad checksum. All the
 *  other ICMP messages are forwarded to the IcmpChecksumInserter (ICi).
 *
 *******************************************************************************/
void pInvalidPacketDropper(
        stream<AxisIp4>     &siICc_Data,
        stream<ValBool>     &siICc_DropCmd,
        stream<AxisIp4>     &soICi_Data)
{
    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                ipd_isFirstChunk=true;
    #pragma HLS RESET variable=ipd_isFirstChunk

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static bool ipd_drop;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    bool dropCmd;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4  currChunk;

    if (!siICc_Data.empty()) {
        if(ipd_isFirstChunk) {
            if (!siICc_DropCmd.empty()) {
                siICc_Data.read(currChunk);
                siICc_DropCmd.read(dropCmd);
                if(dropCmd == CMD_KEEP) {
                    soICi_Data.write(currChunk);
                }
                else {
                    ipd_drop = true;
                }
                ipd_isFirstChunk = false;
            }
        }
        else if (ipd_drop) {
            // Drain and drop current packet
            siICc_Data.read(currChunk);
        }
        else {
            // Forward packet
            siICc_Data.read(currChunk);
            soICi_Data.write(currChunk);
        }
        if (currChunk.getLE_TLast()) {
            ipd_drop = false;
            ipd_isFirstChunk = true;
        }
    }
}

/*******************************************************************************
 * IP Checksum Inserter (ICi)
 *
 *  @param[in]  siXYz_Data  Data stream array from IPd and IHa.
 *  @param[in]  siUVw_Csum  ICMP checksum array from ICc and CMb.
 *  @param[out] soIPTX_Data The data stream to the IpTxHandler (IPTX).
 *
 * @details
 *  This process inserts both the IP header checksum and the ICMP checksum into
 *  the outgoing packet.
 *
 *******************************************************************************/
void pIcmpChecksumInserter(
        stream<AxisIp4>      siXYz_Data[2],
        stream<IcmpCsum>     siUVw_Csum[2],
        stream<AxisIp4>      &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "ICi");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<7>          ici_chunkCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET variable=ici_chunkCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<1>   ici_dataStreamSource;  // siIPd_Data or siIHa_Data

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4             inputChunk(0, 0, 0);
    LE_IcmpCsum         icmpChecksum;

    switch(ici_chunkCount) {
    case 0:
        bool streamEmptyStatus[2]; // Status of the input data streams
        for (int i=0; i<2; ++i) {
            streamEmptyStatus[i] = siXYz_Data[i].empty();
        }
        for (int i=0; i<2; ++i) {
            if(!streamEmptyStatus[i]) {
                ici_dataStreamSource = i;
                inputChunk = siXYz_Data[i].read();
                soIPTX_Data.write(inputChunk);
                ici_chunkCount++;
                break;
            }
        }
        break;
    case 2:
        if (!siXYz_Data[ici_dataStreamSource].empty() &&
            !siUVw_Csum[ici_dataStreamSource].empty()) {
            siXYz_Data[ici_dataStreamSource].read(inputChunk);
            icmpChecksum = siUVw_Csum[ici_dataStreamSource].read();
            inputChunk.setIcmpCsum(icmpChecksum);
            soIPTX_Data.write(inputChunk);
            ici_chunkCount++;
        }
        break;
    default:
        if (!siXYz_Data[ici_dataStreamSource].empty()) {
            siXYz_Data[ici_dataStreamSource].read(inputChunk);
            soIPTX_Data.write(inputChunk);
            if (inputChunk.getLE_TLast()) {
                ici_chunkCount = 0;
            }
            else {
                ici_chunkCount++;
            }
        }
        break;
    }
}


/*******************************************************************************
 * @brief  Main process of the Internet Control Message Protocol (ICMP) Server.
 *
 * @param[in]  piMMIO_MacAddress The MAC  address from MMIO (in network order).
 * @param[in]  siIPRX_Data       The data stream from the IP Rx handler (IPRX).
 * @param[in]  siIPRX_Derr       Erroneous IP data stream from [IPRX].
 * @param[in]  siUOE_Data        A copy of the first IPv4 bytes that caused the error.
 * @param[out] soIPTX_Data       The data stream to the IpTxHandler (IPTX).
 *
 * @details
 *  This process is in charge of building and sending control and error messages
 *  back to the IP address that initiated the control request or that caused an
 *  error in the first place.
 *  The process performs the 3 following main tasks:
 *  1) It implements the ICMP Echo Reply message used by the ping command.
 *  2) It creates an error message of type "Time Exceeded" upon reception of an
 *     IPv4 packet which TTL has expired.
 *  3) It creates an error message of type 'Destination Port Unreachable' if an
 *     UDP datagram is received for a port that is not opened in listening mode.
 *******************************************************************************/
void icmp(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr              piMMIO_Ip4Address,

        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &siIPRX_Data,
        stream<AxisIp4>     &siIPRX_Derr,

        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisIcmp>    &siUOE_Data,

        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- IP Checksum Checker (ICc)
    static stream<AxisIp4>           ssICcToIPd_Data        ("ssICcToIPd_Data");
    #pragma HLS stream      variable=ssICcToIPd_Data        depth=64
    #pragma HLS DATA_PACK   variable=ssICcToIPd_Data

    static stream<ValBool>           ssICcToIPd_DropCmd     ("ssICcToIPd_DropCmd");
    #pragma HLS stream      variable=ssICcToIPd_DropCmd     depth=8

    //-- Control Message Builder (CMb)
    static stream<AxisIcmp>          ssCMbToIHa_Data        ("ssCMbToIHa_Data");
    #pragma HLS stream      variable=ssCMbToIHa_Data        depth=192
    static stream<AxisIp4>           ssCMbToIHa_IpHdr       ("ssCMbToIHa_IpHdr");
    #pragma HLS stream      variable=ssCMbToIHa_IpHdr       depth=64

    //-- XYz = [InvalidPacketDropper (IPd)| IpHeaderAppender (IHa)]
    static stream<AxisIp4>           ssXYzToICi_Data[2];
    #pragma HLS STREAM      variable=ssXYzToICi_Data        depth=16

    //-- UVw = [ICc|CMb]
    static stream<IcmpCsum>          ssUVwToICi_Csum[2];
    #pragma HLS STREAM      variable=ssUVwToICi_Csum        depth=16

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pIcmpChecksumChecker(
            siIPRX_Data,
            ssICcToIPd_Data,
            ssICcToIPd_DropCmd,
            ssUVwToICi_Csum[0]);

    pInvalidPacketDropper(
            ssICcToIPd_Data,
            ssICcToIPd_DropCmd,
            ssXYzToICi_Data[0]);

    pControlMessageBuilder(
            siUOE_Data,
            siIPRX_Derr,
            ssCMbToIHa_Data,
            ssCMbToIHa_IpHdr,
            ssUVwToICi_Csum[1]);

    pIpHeaderAppender(
            piMMIO_Ip4Address,
            ssCMbToIHa_Data,
            ssCMbToIHa_IpHdr,
            ssXYzToICi_Data[1]);

    pIcmpChecksumInserter(
            ssXYzToICi_Data,
            ssUVwToICi_Csum,
            soIPTX_Data);

}

/*******************************************************************************
 * @brief  Top of the Internet Control Message Protocol (ICMP) Server.
 *
 * @param[in]  piMMIO_MacAddress The MAC  address from MMIO (in network order).
 * @param[in]  siIPRX_Data       The data stream from the IP Rx handler (IPRX).
 * @param[in]  siIPRX_Derr       Erroneous IP data stream from [IPRX].
 * @param[in]  siUOE_Data        A copy of the first IPv4 bytes that caused the error.
 * @param[out] soIPTX_Data       The data stream to the IpTxHandler (IPTX).
 *
 *******************************************************************************/
#if HLS_VERSION == 2017
    void icmp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr              piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &siIPRX_Data,
        stream<AxisIp4>     &siIPRX_Derr,
        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisIcmp>    &siUOE_Data,
        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soIPTX_Data)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable          port=piMMIO_Ip4Address

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data   metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Derr   metadata="-bus_bundle siIPRX_Derr"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_Data    metadata="-bus_bundle siUOE_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_Data   metadata="-bus_bundle soIPTX_Data"

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- MAIN ICMP PROCESS -----------------------------------------------------
	icmp(
	    //-- MMIO Interfaces
	    piMMIO_Ip4Address,
	    //-- IPRX Interfaces
	    siIPRX_Data,
	    siIPRX_Derr,
	    //-- UOE Interface
	    siUOE_Data,
	    //-- IPTX Interface
	    soIPTX_Data);
}
#else
    void icmp_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr              piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &siIPRX_Data,
        stream<AxisRaw>     &siIPRX_Derr,
        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisRaw>     &siUOE_Data,
        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soIPTX_Data)
{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable             port=piMMIO_Ip4Address

    #pragma HLS INTERFACE axis off              port=siIPRX_Data
    #pragma HLS INTERFACE axis register forward port=siIPRX_Derr
    #pragma HLS INTERFACE axis off              port=siUOE_Data
    #pragma HLS INTERFACE axis register forward port=soIPTX_Data

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW disable_start_propagation

    //-- LOCAL INPUT and OUTPUT STREAMS ----------------------------------------
    static stream<AxisIp4>      ssiIPRX_Data ("ssiIPRX_Data");
    #pragma HLS STREAM variable=ssiIPRX_Data depth=32
    static stream<AxisIp4>      ssiIPRX_Derr ("ssiIPRX_Derr");
    static stream<AxisIcmp>     ssiUOE_Data  ("ssiUOE_Data");
    static stream<AxisIp4>      ssoIPTX_Data ("ssoIPTX_Data");

    //-- INPUT STREAM CASTING --------------------------------------------------
    pAxisRawCast(siIPRX_Data, ssiIPRX_Data);
    pAxisRawCast(siIPRX_Derr, ssiIPRX_Derr);
    pAxisRawCast(siUOE_Data,  ssiUOE_Data);

    //-- MAIN ICMP PROCESS -----------------------------------------------------
	icmp(
	    //-- MMIO Interfaces
	    piMMIO_Ip4Address,
	    //-- IPRX Interfaces
	    ssiIPRX_Data,
	    ssiIPRX_Derr,
	    //-- UOE Interface
	    ssiUOE_Data,
	    //-- IPTX Interface
	    ssoIPTX_Data);

    //-- OUTPUT STREAM CASTING -------------------------------------------------
    pAxisRawCast(ssoIPTX_Data, soIPTX_Data);

}
#endif

 /*! \} */
