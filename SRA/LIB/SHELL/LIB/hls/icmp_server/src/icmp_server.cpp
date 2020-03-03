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
 * @file       : icmp_server.cpp
 * @brief      : Internet Control Message Protocol server (ICMP)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "icmp_server.hpp"
#include "../../toe/src/toe_utils.hpp"
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
#define THIS_NAME "ICMP"

#define TRACE_OFF  0x0000
#define TRACE_CMB  1 << 1
#define TRACE_ICC  1 << 2
#define TRACE_ICI  1 << 3
#define TRACE_IHA  1 << 4
#define TRACE_IPD  1 << 5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * ICMP header Checksum accumulator and Checker (ICc)
 *
 * @param[in]  siIPRX_Data,   The data stream from the IP Rx handler (IPRX).
 * @param[out] soIPd_Data,    The data stream to IcmpPacketDropper (IPd)
 * @param[out] soIPd_DropCmd, THe drop command information for [IPd].
 * @param[out] soICi_Csum,    ICMP checksum to IcmpChecksumInserter (ICi).
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
 *   Assumption is no options in IP header.
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
 *****************************************************************************/
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
    static ap_uint<7>                          icc_wordCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET                 variable=icc_wordCount
    static enum FsmStates { S0=0, S1, S2, S3 } icc_csumState=S0;
    #pragma HLS RESET                 variable=icc_csumState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIp4  icc_prevWord;
    static Sum17    icc_subSums[4];
    static IcmpCsum icc_oldHCsum;
    static Sum17    icc_newHCsum;
    static IcmpType icc_icmpType;
    static IcmpCode icc_icmpCode;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4 currWord;
    AxisIp4 sendWord;

    currWord.tlast = 0;

    if (icc_writeLastOne) {
        // Forward the very last AxiWord
        soIPd_Data.write(icc_prevWord);
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
        siIPRX_Data.read(currWord);
        switch (icc_wordCount) {
        case WORD_0: // The current word contains [ FO | Id | TotLen | ToS | IHL ]
            icc_subSums[0] = 0;
            icc_subSums[1] = 0;
            icc_subSums[2] = 0;
            icc_subSums[3] = 0;
            break;
        case WORD_1: // The current word contains [ SA | HdCsum | Prot | TTL ]
            sendWord = icc_prevWord;
            soIPd_Data.write(sendWord);
            break;
        case WORD_2: // The current word contains [ Csum | Code | Type | DA ]
            // Save ICMP Type, Code and Checksum
            icc_icmpType = currWord.getIcmpType();
            icc_icmpCode = currWord.getIcmpCode();
            icc_oldHCsum = currWord.getIcmpCsum();

            // Forward data stream while swapping IP_SA & IP_DA
            sendWord.setIp4TtL(icc_prevWord.getIp4Ttl());
            sendWord.setIp4Prot(icc_prevWord.getIp4Prot());
            sendWord.setIp4HdrCsum(icc_prevWord.getIp4HdrCsum());
            sendWord.setIp4SrcAddr(currWord.getIp4DstAddr());
            sendWord.tkeep = 0xFF;
            sendWord.tlast = 0;
            soIPd_Data.write(sendWord);
            // Accumulate [ Csum | Code | Type ]
            for (int i=2; i<4; i++) {
              #pragma HLS UNROLL
                ap_uint<16> temp;
                temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                temp(15, 8) = currWord.tdata.range(i*16+ 7, i*16+0);
                icc_subSums[i] += temp;
                icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
            }
            // Replace the IP_DA field with IP_SA
            currWord.setIp4DstAddr(icc_prevWord.getIp4SrcAddr());
            // Replace ECHO_REQUEST field with ECHO_REPLY
            currWord.setIcmpType(ICMP_ECHO_REPLY);
            break;
        default:
            // Accumulate quadword
            for (int i=0; i<4; i++) {
              #pragma HLS UNROLL
                ap_uint<16> temp;
                if (currWord.tkeep.range(i*2+1, i*2) == 0x3) {
                    temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.tdata.range(i*16+ 7, i*16+0);
                    icc_subSums[i] += temp;
                    icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
                }
                else if (currWord.tkeep[i*2] == 0x1) {
                    temp( 7, 0) = 0;
                    temp(15, 8) = currWord.tdata.range(i*16+7, i*16);
                    icc_subSums[i] += temp;
                    icc_subSums[i] = (icc_subSums[i] + (icc_subSums[i] >> 16)) & 0xFFFF;
                }
            }
            sendWord = icc_prevWord;
            soIPd_Data.write(sendWord);
            break;
        } // End-of: switch (icc_wordCount)

        icc_prevWord = currWord;
        icc_wordCount++;

        if (currWord.tlast == 1) {
            icc_wordCount = 0;
            icc_writeLastOne = true;
            icc_computeCs = true;
            if (DEBUG_LEVEL & TRACE_ICC) {
                printInfo(myName, "Received a new message (checksum=0x%4.4X)\n",
                          icc_oldHCsum.to_uint());
            }
        }
    }
}

/*****************************************************************************
 * Control Message Builder (CMb)
 *
 * @param[in]  siUDP_Data,  The data stream from the UDP offload engine (UDP).
 * @param[in]  siIPRX_Derr, Erroneous IP data stream from IpRxHandler (IPRX).
 * @param[out] soIHa_Data,  Data stream to IpHeaderAppender (IHa).
 * @param[out] soIHa_Hdr,   Header data stream to IpHeaderAppender (IHa).
 * @param[out] soICi_Csum,  ICMP checksum to ICi.
 *
 * @details
 *  [TODO-Not tested yet] This process ...
 *   *  If the TTL of the incoming IPv4 packet has expired, the packet is routed
 *  to the ICMP (over the 'soICMP_Derr' stream) in order for the ICMP engine
 *  to build the error messages which data section must include a copy of the
 *  Erroneous IPv4 header plus at least the first eight bytes of data from the
 *  IPv4 packet that caused the error message.
 *
 *****************************************************************************/
void pControlMessageBuilder(
        stream<AxiWord>         &siUDP_Data,    // [TODO-AxisUdp]
        stream<AxisIp4>         &siIPRX_Derr,
        stream<AxisIcmp>        &soIHa_Data,
        stream<AxisIp4>         &soIHa_IpHdr,
        stream<IcmpCsum>        &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "CMb");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { BCM_IDLE,   BCM_IP, \
                            BCM_STREAM, BCM_CS } bcm_fsmState=BCM_IDLE;
    #pragma HLS reset                   variable=bcm_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<3>   ipWordCounter;  // Up to 64-bytes IP packet
    static ap_uint<20>  checksumAcc;

    static ap_uint<1>   streamSource    = 0; // 0 is UDP, 1 is IP Handler (Destination unreachable & TTL exceeded respectively)

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    StsBit       udpInEmpty;
    StsBit       iprxInEmpty;

    udpInEmpty  = siUDP_Data.empty();
    iprxInEmpty = siIPRX_Derr.empty();

    switch(bcm_fsmState) {
        case BCM_IDLE:
            //-- Assemble an ICMP header = [Type | Code | Checksum]
            if ((udpInEmpty == 0 || iprxInEmpty == 0) && !soIHa_Data.full()) {
                // Data are available in at lease one of the input queue(s).
                // Don't read them yet but start assembling a ICMP header
                ipWordCounter = 0;
                AxisIcmp tempWord(0, 0xFF, 0);
                if (udpInEmpty == 0) {
                    tempWord.setIcmpType(ICMP_DESTINATION_UNREACHABLE);
                    tempWord.setIcmpCode(ICMP_DESTINATION_PORT_UNREACHABLE);
                    streamSource = 0;
                }
                else if (iprxInEmpty == 0) {
                    tempWord.setIcmpType(ICMP_TIME_EXCEEDED);
                    tempWord.setIcmpCode(ICMP_TTL_EXPIRED_IN_TRANSIT);
                    streamSource = 1;
                }
                checksumAcc = (((tempWord.tdata.range(63, 48)  + tempWord.tdata.range(47, 32)) +
                                 tempWord.tdata.range(31, 16)) + tempWord.tdata.range(15,  0));
                soIHa_Data.write(tempWord);
                bcm_fsmState = BCM_IP;
            }
            break;
        case BCM_IP:
            //-- Forward the IP header to [IHa]
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && iprxInEmpty == 0)) &&
                !soIHa_Data.full() && !soIHa_IpHdr.full()) {
                // Start reading data from one of the input queues.
                AxiWord tempWord(0, 0, 0);
                if (streamSource == 0) {
                    tempWord = siUDP_Data.read();
                }
                else if (streamSource == 1) {
                    tempWord = siIPRX_Derr.read();
                }
                checksumAcc = (checksumAcc +
                              ((tempWord.tdata.range(63, 48) + tempWord.tdata.range(47, 32)) +
                                tempWord.tdata.range(31, 16) + tempWord.tdata.range(15,  0)));
                soIHa_Data.write(tempWord);
                soIHa_IpHdr.write(tempWord);
                if (ipWordCounter == 2) {
                    bcm_fsmState = BCM_STREAM;
                }
                else {
                    ipWordCounter++;
                }
            }
            break;  
        case BCM_STREAM:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && iprxInEmpty == 0)) && !soIHa_Data.full()) { // If there are data in the queue start reading them
                AxiWord tempWord(0, 0, 0);
                if (streamSource == 0) {
                    tempWord = siUDP_Data.read();
                }
                else if (streamSource == 1) {
                    tempWord = siIPRX_Derr.read();
                }
                checksumAcc = (checksumAcc +
                              ((tempWord.tdata.range(63, 48) + tempWord.tdata.range(47, 32)) +
                                tempWord.tdata.range(31, 16) + tempWord.tdata.range(15,  0)));
                soIHa_Data.write(tempWord);
                if (tempWord.tlast == 1) {
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

/*****************************************************************************
 * IP Header Appender (IHa)
 *
 * @param[in]  piMMIO_IpAddress, the IPv4 address from MMIO (in network order).
 * @param[in]  siCMb_Data, Data stream from ControlMessageBuilder (CMb).
 * @param[in]  siIHa_IpHdr,The IP header part of a data stream.
 * @param[out] soICi_Data, Data stream to IcmpChecksumInserter (ICi).
 *
 *****************************************************************************/
void pIpHeaderAppender(
        Ip4Addr                  piMMIO_IpAddress,
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
    static AxisIp4  iha_ipHdrWord(0, 0, 0);
    static Ip4Addr  iha_remoteIpHostAddr;

    switch(aih_fsmState) {
        case AIH_IDLE:
            if (!siIHa_IpHdr.empty() && !soICi_Data.full()) {
                //-- Forward the 1st IP header word
                siIHa_IpHdr.read(iha_ipHdrWord);
                Ip4TotalLen ip4TotLen = iha_ipHdrWord.getIp4TotalLen() + 28;
                iha_ipHdrWord.setIp4TotalLen(ip4TotLen);
                iha_ipHdrWord.tkeep = 0xFF;
                iha_ipHdrWord.tlast = 0;
                soICi_Data.write(iha_ipHdrWord);
                aih_fsmState = AIH_IP;
            }
            break;
        case AIH_IP:
            if (!siIHa_IpHdr.empty() && !soICi_Data.full()) {
                //-- Forward the 2nd IP header word
                siIHa_IpHdr.read(iha_ipHdrWord);
                iha_ipHdrWord.setIp4TtL(0x80);
                iha_ipHdrWord.setIp4Prot(ICMP_PROTOCOL);
                // Save the remote host address and set the new IP_SA
                iha_remoteIpHostAddr = iha_ipHdrWord.getIp4SrcAddr();
                iha_ipHdrWord.setIp4SrcAddr(piMMIO_IpAddress);
                iha_ipHdrWord.tkeep = 0xFF;
                iha_ipHdrWord.tlast = 0;
                soICi_Data.write(iha_ipHdrWord);
                aih_fsmState = AIH_MERGE;
            }
            break;
        case AIH_MERGE:
            if (!siIHa_IpHdr.empty() && !siCMb_Data.empty() &&
                !soICi_Data.full()) {
                siIHa_IpHdr.read(); // Drain and drop this word
                //-- Merge 3rd IP header word with 1st ICMP datagram
                iha_icmpData = siCMb_Data.read();
                AxisIp4 thirdWord(0, 0xFF, 0);
                thirdWord.setIp4DstAddr(iha_remoteIpHostAddr);
                thirdWord.setIcmpType(iha_icmpData.getIcmpType());
                thirdWord.setIcmpCode(iha_icmpData.getIcmpCode());
                thirdWord.setIcmpCsum(iha_icmpData.getIcmpCsum());
                soICi_Data.write(thirdWord);
                aih_fsmState = AIH_STREAM;
            }
            break;
        case AIH_STREAM:
            if (!siCMb_Data.empty() && !soICi_Data.full()) {
                AxisIp4  outputWord(0, 0xFF, 0);
                outputWord.tdata.range(31, 0) = iha_icmpData.tdata.range(63, 32);
                iha_icmpData = siCMb_Data.read();
                outputWord.tdata.range(63, 32) = iha_icmpData.tdata.range(31,  0);
                if (iha_icmpData.tlast == 1) {
                    if (iha_icmpData.tkeep.range(7, 4) == 0) {
                        outputWord.tlast = 1;
                        outputWord.tkeep.range(7, 4) = iha_icmpData.tkeep.range(3,0);
                        aih_fsmState = AIH_IDLE;
                    }
                    else {
                        aih_fsmState = AIH_RESIDUE;
                    }
                }
                soICi_Data.write(outputWord);
            }
            break;
        case AIH_RESIDUE:
            if (!soICi_Data.full()) {
                AxisIp4 outputWord(0, 0, 1);
                outputWord.tdata.range(31, 0) = iha_icmpData.tdata.range(63, 32);
                outputWord.tkeep.range( 3, 0) = iha_icmpData.tkeep.range(7, 4);
                soICi_Data.write(outputWord);
                aih_fsmState = AIH_IDLE;
            }
            break;
    }
}

/*****************************************************************************
 * Invalid Packet Dropper (IPd)
 *
 * @param[in]  siICc_Data,    Data stream from IcmpChecksumChecker (ICc).
 * @param[in]  siICc_DropCmd, The drop command information from [ICc].
 * @param[out] soICi_Data,    Data stream to IcmpChecksumInserter (ICi).
 *
 * @details
 *  This process drops the packets which arrive with a bad checksum. All the
 *  other ICMP packets are forwarded to the IcmpChecksumInserter (ICi).
 *
 *****************************************************************************/
void pInvalidPacketDropper(
        stream<AxisIp4>     &siICc_Data,
        stream<ValBool>     &siICc_DropCmd,
        stream<AxisIp4>     &soICi_Data)
{
    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                ipd_isFirstWord=true;
    #pragma HLS RESET variable=ipd_isFirstWord

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static bool ipd_drop;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    bool dropCmd;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4  currWord;

    if (!siICc_Data.empty()) {
        if(ipd_isFirstWord) {
            if (!siICc_DropCmd.empty()) {
                siICc_Data.read(currWord);
                siICc_DropCmd.read(dropCmd);
                if(dropCmd == CMD_KEEP) {
                    soICi_Data.write(currWord);
                }
                else {
                    ipd_drop = true;
                }
                ipd_isFirstWord = false;
            }
        }
        else if (ipd_drop) {
            // Drain and drop current packet
            siICc_Data.read(currWord);
        }
        else {
            // Forward packet
            siICc_Data.read(currWord);
            soICi_Data.write(currWord);
        }
        if (currWord.tlast == 1) {
            ipd_drop = false;
            ipd_isFirstWord = true;
        }
    }
}

/*****************************************************************************
 * IP Checksum Inserter (ICi)
 *
 *  @param[in]  siXYz_Data,  Data stream array from IPd and IHa.
 *  @param[in]  siUVw_Csum,  ICMP checksum array from ICc and CMb.
 *  @param[out] soIPTX_Data, The data stream to the IpTxHandler (IPTX).
 *
 * @details
 *  This process inserts both the IP header checksum and the ICMP checksum in
 *  the outgoing packet.
 *
 *****************************************************************************/
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
    static ap_uint<7>          ici_wordCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET variable=ici_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<1>   ici_dataStreamSource;  // siIPd_Data or siIHa_Data

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4             inputWord(0, 0, 0);
    LE_IcmpCsum         icmpChecksum;

    switch(ici_wordCount) {
    case 0:
        bool streamEmptyStatus[2]; // Status of the input data streams
        for (uint8_t i=0; i<2; ++i) {
            streamEmptyStatus[i] = siXYz_Data[i].empty();
        }
        for (uint8_t i=0; i<2; ++i) {
            if(!streamEmptyStatus[i]) {
                ici_dataStreamSource = i;
                inputWord = siXYz_Data[i].read();
                soIPTX_Data.write(inputWord);
                ici_wordCount++;
                break;
            }
        }
        break;
    case 2:
        if (!siXYz_Data[ici_dataStreamSource].empty() &&
            !siUVw_Csum[ici_dataStreamSource].empty()) {
            siXYz_Data[ici_dataStreamSource].read(inputWord);
            icmpChecksum = siUVw_Csum[ici_dataStreamSource].read();
            inputWord.setIcmpCsum(icmpChecksum);
            soIPTX_Data.write(inputWord);
            ici_wordCount++;
        }
        break;
    default:
        if (!siXYz_Data[ici_dataStreamSource].empty()) {
            siXYz_Data[ici_dataStreamSource].read(inputWord);
            soIPTX_Data.write(inputWord);
            if (inputWord.tlast == 1) {
                ici_wordCount = 0;
            }
            else {
                ici_wordCount++;
            }
        }
        break;
    }
}


 /*****************************************************************************
  * @brief   Main process of the ICMP server.
  *
  *****************************************************************************/
 void icmp_server(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr              piMMIO_IpAddress,

        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &siIPRX_Data,
        stream<AxisIp4>     &siIPRX_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxiWord>     &siUDP_Data,

        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soIPTX_Data)
{
        //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
        #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable           port=piMMIO_IpAddress

    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Data   metadata="-bus_bundle siIPRX_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=siIPRX_Derr   metadata="-bus_bundle siIPRX_Derr"
    #pragma HLS RESOURCE core=AXI4Stream variable=siUDP_Data    metadata="-bus_bundle siUDP_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soIPTX_Data   metadata="-bus_bundle soIPTX_Data"

#else

    #pragma HLS INTERFACE ap_stable           port=piMMIO_IpAddress
    #pragma HLS INTERFACE axis                port=siIPRX_Data
    #pragma HLS INTERFACE axis                port=siIPRX_Derr
    #pragma HLS _NTERFACE axis                port=siUDP_Data
    #pragma HLS INTERFACE axis                port=soIPTX_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

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
    static stream<AxisIp4>           ssToICi_Data[2];
    #pragma HLS STREAM      variable=ssToICi_Data           depth=16

    //-- UVw = [ICc|CMb]
    static stream<IcmpCsum>          ssUVwToICi_Csum[2];
    #pragma HLS STREAM      variable=ssUVwToICi_Csum    depth=16

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pIcmpChecksumChecker(
            siIPRX_Data,
            ssICcToIPd_Data,
            ssICcToIPd_DropCmd,
            ssUVwToICi_Csum[0]);

    pInvalidPacketDropper(
            ssICcToIPd_Data,
            ssICcToIPd_DropCmd,
            ssToICi_Data[0]);

    pControlMessageBuilder(
            siUDP_Data,
            siIPRX_Derr,
            ssCMbToIHa_Data,
            ssCMbToIHa_IpHdr,
            ssUVwToICi_Csum[1]);

    pIpHeaderAppender(
            piMMIO_IpAddress,
            ssCMbToIHa_Data,
            ssCMbToIHa_IpHdr,
            ssToICi_Data[1]);

    pIcmpChecksumInserter(
            ssToICi_Data,
            ssUVwToICi_Csum,
            soIPTX_Data);

}

