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
#define TRACE_TBD  1 << 1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ON)

//OBSOLETE-20200220 ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
//OBSOLETE-20200220     return (inputVector.range(7,0), inputVector(15, 8));
//OBSOLETE-20200220 }

/*****************************************************************************
 * ICMP header Checksum accumulator and Checker (ICc)
 *
 * @param[in]  siIPRX_Data,     The data stream from the IP Rx handler (IPRX).
 * @param[out] soIPd_Data,      The data stream to IcmpPacketDropper (IPd)
 * @param[out] soIPd_CsumValid, IP header checksum valid information.
 * @param[out] soICi_Csum,      ICMP checksum to IcmpChecksumInserter (ICi).
 *
 * @details
 *   This process handles the incoming data stream from the IPRX.
 *   It computes ... Assumption no options in IP header
 *
 *  The format of the incoming ICMP message embedded into an IPv4 packet is as follows:
 *
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
        stream<bool>            &soIPd_CsumValid,  // [TODO- VAlBit]
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
    //OBSOLETE_20200221 static ap_uint<2>          icc_state=0;
    static enum FsmStates { S0=0, S1, S2, S3 } icc_csumState=S0;
    #pragma HLS RESET                 variable=icc_csumState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxisIp4  icc_prevWord;
    static Sum17    icc_subSums[4];
    static Sum17    icc_sum;
    static IcmpType icc_icmpType;
    static IcmpCode icc_icmpCode;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    //OBSOLETE_202200221 static ap_uint<8>   newTTL = 0x40;
    //OBSOLETE_202020221 AxiWord currWord;
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
            icc_sum = ~icc_sum;
            icc_csumState = S1;
            break;
        case S1:
            icc_subSums[0] += icc_subSums[1];
            icc_subSums[0] = (icc_subSums[0] + (icc_subSums[0] >> 16)) & 0xFFFF;
            icc_sum -= ICMP_ECHO_REQUEST;
            icc_sum = (icc_sum - (icc_sum >> 16)) & 0xFFFF;
            icc_csumState = S2;
            break;
        case S2:
            icc_subSums[0] = ~icc_subSums[0];
            icc_sum = ~icc_sum;
            icc_csumState = S3;
            break;
        case S3:
            if ((icc_subSums[0](15, 0) == 0) &&         // Checksum valid
                (icc_icmpType == ICMP_ECHO_REQUEST) &&  // Message type is Echo Request (used to ping)
                (icc_icmpCode == 0)) {
                soIPd_CsumValid.write(true);
                soICi_Csum.write(icc_sum.range(15, 0));
            }
            else
                soIPd_CsumValid.write(false);
            icc_computeCs = false;
            break;
        }
        //OBSOLETE-20200221 icc_state++;
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
            //OBSOLETE-20200221 icc_icmpType        = currWord.tdata(39, 32);
            //OBSOLETE-20200221 icc_icmpCode        = currWord.tdata(47, 40);
            //OBSOLETE-20200221 icc_checksum(15, 0) = currWord.tdata(63, 48);

            // Save ICMP Type, Code and Checksum
            icc_icmpType   = currWord.getIcmpType();
            icc_icmpCode   = currWord.getIcmpCode();
            icc_sum(15, 0) = currWord.getLE_IcmpCsum(); // [FIXME - Why LE_?]
            icc_sum[16] = 1;
            // Forward data stream while swapping IP_SA & IP_DA
            //OBSOLETE-20200221 sendWord.tdata(31,  0) = icc_prevWord.tdata(31, 0);
            //OBSOLETE-20200221 sendWord.tdata(63, 32) = currWord.tdata(31, 0);
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
            //OBSOLETE-20200221 currWord.tdata(31, 0) = icc_prevWord.tdata(63, 32);
            currWord.setIp4DstAddr(icc_prevWord.getIp4SrcAddr());
            // Replace ECHO_REQUEST field with ECHO_REPLY
            //OBSOLETE-20200221 currWord.tdata.range(39, 32) = ECHO_REPLY;
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
 *  This process ...
 *   *  If the TTL of the incoming IPv4 packet has expired, the packet is routed
 *  to the ICMP (over the 'soICMP_Derr' stream) in order for the ICMP engine
 *  to build the error messages which data section must include a copy of the
 *  Erroneous IPv4 header plus at least the first eight bytes of data from the
 *  IPv4 packet that caused the error message.
 *
 *****************************************************************************/
void pControlMessageBuilder(
        stream<AxiWord>         &siUDP_Data,
        stream<AxiWord>         &siIPRX_Derr,
		stream<AxiWord>         &soIHa_Data,
		stream<ap_uint<64> >    &soIHa_Hdr,
		stream<ap_uint<16> >    &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "CMb");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { BCM_IDLE, BCM_IP, BCM_STREAM, BCM_CS } bcm_fsmState=BCM_IDLE;
    #pragma HLS reset                                     variable=bcm_fsmState

    //OBSOLETE_20200220 static enum uState{UDP_IDLE, UDP_IP, UDP_STREAM, UDP_CS} udpState;

    static ap_uint<20>  udpChecksum     = 0;
    static ap_uint<3>   ipWordCounter   = 0;
    static ap_uint<1>   streamSource    = 0; // 0 is UDP, 1 is IP Handler (Destination unreachable & TTL exceeded respectively)

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    StsBit       udpInEmpty = 0;
    StsBit       ttlInEmpty = 0;


    udpInEmpty = siUDP_Data.empty();
    ttlInEmpty = siIPRX_Derr.empty();

    switch(bcm_fsmState) {
        case BCM_IDLE:
            if ((udpInEmpty == 0 || ttlInEmpty == 0) && !soIHa_Data.full()) {
                // If there are data in the queue, don't read them in but start assembling the ICMP header
                ipWordCounter = 0;
                AxiWord tempWord(0, 0xFF, 0);
                if (udpInEmpty == 0) {
                    tempWord.tdata = 0x0000000000000303;
                    streamSource = 0;
                }
                else if (ttlInEmpty == 0) {
                    tempWord.tdata = 0x000000000000000B;
                    streamSource = 1;
                }
                udpChecksum = (((tempWord.tdata.range(63, 48) + tempWord.tdata.range(47, 32)) + tempWord.tdata.range(31, 16)) + tempWord.tdata.range(15, 0));
                soIHa_Data.write(tempWord);
                bcm_fsmState = BCM_IP;
            }
            break;
        case BCM_IP:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !soIHa_Data.full() && !soIHa_Hdr.full()) { // If there are data in the queue start reading them
                AxiWord tempWord(0, 0, 0);
                if (streamSource == 0)
                    tempWord = siUDP_Data.read();
                else if (streamSource == 1)
                    tempWord = siIPRX_Derr.read();
                udpChecksum = (udpChecksum + ((tempWord.tdata.range(63, 48) + tempWord.tdata.range(47, 32)) + tempWord.tdata.range(31, 16) + tempWord.tdata.range(15, 0)));
                soIHa_Data.write(tempWord);
                soIHa_Hdr.write(tempWord.tdata);
                if (ipWordCounter == 2)
                    bcm_fsmState = BCM_STREAM;
                else 
                    ipWordCounter++;
            }
            break;  
        case BCM_STREAM:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !soIHa_Data.full()) { // If there are data in the queue start reading them
                AxiWord tempWord(0, 0, 0);
                if (streamSource == 0)
                    tempWord = siUDP_Data.read();
                else if (streamSource == 1)
                    tempWord = siIPRX_Derr.read();
                udpChecksum = (udpChecksum + ((tempWord.tdata.range(63, 48) + tempWord.tdata.range(47, 32)) + tempWord.tdata.range(31, 16) + tempWord.tdata.range(15, 0)));
                soIHa_Data.write(tempWord);
                if (tempWord.tlast == 1)
                    bcm_fsmState = BCM_CS;
            }
            break;
        case BCM_CS:
            if (!soICi_Csum.full()) {
                udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
                udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
                udpChecksum = ~udpChecksum;         // Reverse the bits of the result
                soICi_Csum.write(udpChecksum.range(15, 0));   // and write it into the output
                bcm_fsmState = BCM_IDLE;        // When all is complete, move to idle
            }
            break;
    }
}

/*****************************************************************************
 * IP Header Appender (IHa)
 *
 * @param[in] siCMb_Data, Data stream from ControlMessageBuilder (CMb).
 *
 * @param[in] soICi_Data,
 *
 *****************************************************************************/
void pIpHeaderAppender(
        stream<AxiWord>         &siCMb_Data,
        stream<ap_uint<64> >    &soIHa_Hdr,
        stream<AxiWord>         &soICi_Data,
        LE_Ip4Addr               piMMIO_IpAddress)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    //OBSOLETE_20200220 static enum aState{AIP_IDLE, AIP_IP, AIP_MERGE, AIP_STREAM, AIP_RESIDUE} addIpState;
    static enum FsmStates { AIH_IDLE=0, AIH_IP, AIH_MERGE, \
                            AIH_STREAM, AIH_RESIDUE } aih_fsmState=AIH_IDLE;
    #pragma HLS RESET                        variable=aih_fsmState

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static AxiWord tempWord(0, 0, 0);
    static ap_int<32> sourceIP  = 0;

    switch(aih_fsmState) {
        case AIH_IDLE:
            if (!soIHa_Hdr.empty() && !soICi_Data.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
                tempWord.tdata = soIHa_Hdr.read();
                ap_uint<16> tempLength = byteSwap16(tempWord.tdata.range(31, 16));
                tempWord.tdata.range(31, 16) = byteSwap16(tempLength + 28);
                tempWord.tkeep = 0xFF;
                tempWord.tlast = 0;
                soICi_Data.write(tempWord);
                aih_fsmState = AIH_IP;
            }
            break;
        case AIH_IP:
            if (!soIHa_Hdr.empty() && !soICi_Data.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
                tempWord.tdata               = soIHa_Hdr.read();
                tempWord.tdata.range( 7, 0)  = 0x80;                              // Set the TTL to 128
                tempWord.tdata.range(15, 8)  = 0x01;                              // Swap the protocol from whatever it was to ICMP
                sourceIP                    = tempWord.tdata.range(63, 32); //OBSOLETE-20181112 tempWord.tdata.range(63, 32) =   0x01010101;
                tempWord.tdata.range(63, 32) = piMMIO_IpAddress;
                tempWord.tkeep               = 0xFF;
                tempWord.tlast               = 0;
                soICi_Data.write(tempWord);
                aih_fsmState = AIH_MERGE;
            }
            break;
        case AIH_MERGE:
            if (!soIHa_Hdr.empty() && !siCMb_Data.empty() && !soICi_Data.full()) {
                soIHa_Hdr.read();
                ap_uint<64> tempData = sourceIP;
                tempWord             = siCMb_Data.read();
                AxiWord outputWord(0, 0xFF, 0);
                outputWord.tdata      = tempData;
                outputWord.tdata.range(63, 32) = tempWord.tdata.range(31,  0);
                soICi_Data.write(outputWord);
                aih_fsmState = AIH_STREAM;
            }
            break;
        case AIH_STREAM:
            if (!siCMb_Data.empty() && !soICi_Data.full()) {
                AxiWord outputWord(0, 0xFF, 0);
                outputWord.tdata.range(31, 0) = tempWord.tdata.range(63,  32);
                tempWord = siCMb_Data.read();
                outputWord.tdata.range(63, 32) = tempWord.tdata.range(31,  0);
                if (tempWord.tlast == 1) {
                    if (tempWord.tkeep.range(7, 4) == 0) {
                        outputWord.tlast = 1;
                        outputWord.tkeep.range(7, 4) = tempWord.tkeep.range(3,0);
                        aih_fsmState = AIH_IDLE;
                    }
                    else
                        aih_fsmState = AIH_RESIDUE;
                }
                soICi_Data.write(outputWord);
            }
            break;
        case AIH_RESIDUE:
            if (!soICi_Data.full()) {
                AxiWord outputWord(0, 0, 1);
                outputWord.tdata.range(31, 0)    = tempWord.tdata.range(63, 32);
                outputWord.tkeep.range(3, 0)     = tempWord.tkeep.range(7, 4);
                soICi_Data.write(outputWord);
                aih_fsmState = AIH_IDLE;
            }
            break;
    }
}

/*****************************************************************************
 * Invalid Packet Dropper (IPd)
 *
 * @param[in]  siICc_Data,      Data stream from IcmpChecksumChecker (ICc).
 * @param[in]  siICc_CsumValid, IP header checksum valid information.
 * @param[out] soICi_Data,      Data stream to IcmpChecksumInserter (ICi).
 *
 * @details
 *  This process....Reads valid bit from validBufffer, if package is invalid it is dropped otherwise it is forwarded
 *
 *****************************************************************************/
void pInvalidPacketDropper(
        stream<AxisIp4>     &siICc_Data,
        stream<bool>        &siICc_CsumValid,
        stream<AxiWord>     &soICi_Data)
{
    static bool d_isFirstWord   = true;
    static bool d_drop          = false;
    bool d_valid;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxisIp4  currWord;

    if (!siICc_Data.empty()) {
        if(d_isFirstWord) {
            if (!siICc_CsumValid.empty()) {
                siICc_Data.read(currWord);
                siICc_CsumValid.read(d_valid);
                if(d_valid)
                    soICi_Data.write(currWord);
                else
                    d_drop = true;
                d_isFirstWord = false;
            }
        }
        else if (d_drop)
            siICc_Data.read(currWord);
        else {
            siICc_Data.read(currWord);
            soICi_Data.write(currWord);
        }
        if (currWord.tlast == 1) {
            d_drop = false;
            d_isFirstWord = true;
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
        stream<AxiWord>      siXYz_Data[2],
        stream<ap_uint<16> > siUVw_Csum[2],
        stream<AxiWord>      &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "ICi");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<7>          ici_wordCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET variable=ici_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<1>   dataStreamSource;  // siIPd_Data or siIHa_Data

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord             inputWord(0, 0, 0);
    ap_uint<16>         icmpChecksum = 0;

    switch(ici_wordCount) {
    case 0:
        bool streamEmptyStatus[2]; // Status of the input data streams
        for (uint8_t i=0; i<2; ++i) {
            streamEmptyStatus[i] = siXYz_Data[i].empty();
        }
        for (uint8_t i=0; i<2; ++i) {
            if(!streamEmptyStatus[i]) {
                dataStreamSource = i;
                inputWord = siXYz_Data[i].read();
                soIPTX_Data.write(inputWord);
                ici_wordCount++;
                break;
            }
        }
        break;
    case 2:
        if (!siXYz_Data[dataStreamSource].empty() &&
            !siUVw_Csum[dataStreamSource].empty()) {
            siXYz_Data[dataStreamSource].read(inputWord);
            icmpChecksum = siUVw_Csum[dataStreamSource].read();
            inputWord.tdata(63, 48) = icmpChecksum;
            soIPTX_Data.write(inputWord);
            ici_wordCount++;
        }
        break;
    default:
        if (!siXYz_Data[dataStreamSource].empty()) {
            siXYz_Data[dataStreamSource].read(inputWord);
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
        LE_Ip4Addr           piMMIO_IpAddress,

        //------------------------------------------------------
        //-- IPRX Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &siIPRX_Data,
        stream<AxiWord>     &siIPRX_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxiWord>     &siUDP_Data,

        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<AxiWord>     &soIPTX_Data)
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

    static stream<bool>              ssICcToIPd_CsumValid   ("ssICcToIPd_CsumValid");
    #pragma HLS stream      variable=ssICcToIPd_CsumValid   depth=8

    //-- Control Message Builder (CMb)
    static stream<AxiWord>           ssCMbToIHa_Data         ("ssCMbToIHa_Data");
    #pragma HLS stream      variable=ssCMbToIHa_Data         depth=192
    static stream<ap_uint<64> >      ssCMbToIHa_Hdr          ("ssCMbToIHa_Hdr");
    #pragma HLS stream      variable=ssCMbToIHa_Hdr          depth=64

    //-- XYz = [InvalidPacketDropper (IPd)| IpHeaderAppender (IHa)]
    static stream<AxiWord>           ssToICi_Data[2];
    #pragma HLS STREAM      variable=ssToICi_Data           depth=16

    //-- UVw = [ICc|CMb]
    static stream<ap_uint<16> >      ssUVwToICi_Csum[2];
    #pragma HLS STREAM      variable=ssUVwToICi_Csum    depth=16

    //-- PROCESS FUNCTIONS ----------------------------------------------------
    pIcmpChecksumChecker(
            siIPRX_Data,
            ssICcToIPd_Data,
            ssICcToIPd_CsumValid,
            ssUVwToICi_Csum[0]);

    pInvalidPacketDropper(
            ssICcToIPd_Data,
            ssICcToIPd_CsumValid,
            ssToICi_Data[0]);

    pControlMessageBuilder(
            siUDP_Data,
            siIPRX_Derr,
            ssCMbToIHa_Data,
            ssCMbToIHa_Hdr,
            ssUVwToICi_Csum[1]);

    pIpHeaderAppender(
            ssCMbToIHa_Data,
            ssCMbToIHa_Hdr,
            ssToICi_Data[1],
            piMMIO_IpAddress);

    pIcmpChecksumInserter(
            ssToICi_Data,
            ssUVwToICi_Csum,
            soIPTX_Data);

}

