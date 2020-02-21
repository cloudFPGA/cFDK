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
        stream<axiWord>         &siIPRX_Data,
        stream<axiWord>         &soIPd_Data,
        stream<bool>            &soIPd_CsumValid,
        stream<ap_uint<16> >    &soICi_Csum)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "ICc");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                icc_writeLastOne=false;
    #pragma HLS RESET variable=icc_writeLastOne
    static bool                icc_computeCs=false;
    #pragma HLS RESET variable=icc_computeCs
    static ap_uint<7>          icc_wordCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET variable=icc_wordCount


    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static axiWord     icc_prevWord;
    static ap_uint<17> icc_sums[4];




    static ap_uint<2> cics_state = 0;

    static ap_uint<8>   newTTL = 0x40;
    static ap_uint<17>  icmpChecksum = 0;
    static ap_uint<8>   icmpType;
    static ap_uint<8>   icmpCode;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    axiWord currWord;  // [TODO-AxisIp4]
    axiWord sendWord;  // [TODO-AxisIp4]

    currWord.last = 0;

    if (icc_writeLastOne) {
        soIPd_Data.write(icc_prevWord);
        icc_writeLastOne = false;
    }
    else if (icc_computeCs) {
        switch (cics_state) {
        case 0:
            icc_sums[0] += icc_sums[2];
            icc_sums[0] = (icc_sums[0] + (icc_sums[0] >> 16)) & 0xFFFF;
            icc_sums[1] += icc_sums[3];
            icc_sums[1] = (icc_sums[1] + (icc_sums[1] >> 16)) & 0xFFFF;
            icmpChecksum = ~icmpChecksum;
            break;
        case 1:
            icc_sums[0] += icc_sums[1];
            icc_sums[0] = (icc_sums[0] + (icc_sums[0] >> 16)) & 0xFFFF;
            icmpChecksum -= ECHO_REQUEST;
            icmpChecksum = (icmpChecksum - (icmpChecksum >> 16)) & 0xFFFF;
            break;
        case 2:
            icc_sums[0] = ~icc_sums[0];
            icmpChecksum = ~icmpChecksum;
            break;
        case 3:
            // Check for 0
            if ((icc_sums[0](15, 0) == 0) && (icmpType == ECHO_REQUEST) && (icmpCode == 0)) {
                soIPd_CsumValid.write(true);
                soICi_Csum.write(icmpChecksum);
            }
            else
                soIPd_CsumValid.write(false);
            icc_computeCs = false;
            break;
        }
        cics_state++;
    }
    else if (!siIPRX_Data.empty()) {
        siIPRX_Data.read(currWord);
        switch (icc_wordCount) {
        case WORD_0:
            // The current word contains [ FO | Id | TotLen | ToS | IHL ]
            icc_sums[0] = 0;
            icc_sums[1] = 0;
            icc_sums[2] = 0;
            icc_sums[3] = 0;
            break;
        case WORD_1:
            // The current word contains [ SA | HdCsum | Prot | TTL ]
            sendWord = icc_prevWord;
            soIPd_Data.write(sendWord);
            break;
        case WORD_2:
            // The current word contains [ Csum | Code | Type | DA ]
            sendWord.data(31,  0) = icc_prevWord.data(31, 0);
            sendWord.data(63, 32) = currWord.data(31, 0);
            icmpType = currWord.data(39, 32);
            icmpCode = currWord.data(47, 40);
            icmpChecksum(15, 0) = currWord.data(63, 48);
            icmpChecksum[16] = 1;
            sendWord.keep = 0xFF;
            sendWord.last = 0;
            soIPd_Data.write(sendWord);
            for (int i = 2; i < 4; i++) {
              #pragma HLS UNROLL
                ap_uint<16> temp;
                    temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.data.range(i*16+7, i*16);
                    icc_sums[i] += temp;
                    icc_sums[i] = (icc_sums[i] + (icc_sums[i] >> 16)) & 0xFFFF;
            }
            currWord.data(31, 0) = icc_prevWord.data(63, 32);
            currWord.data.range(39, 32) = ECHO_REPLY;
            break;
        default:
            for (int i = 0; i < 4; i++) {
    #pragma HLS UNROLL
                ap_uint<16> temp;
                if (currWord.keep.range(i*2+1, i*2) == 0x3) {
                    temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
                    temp(15, 8) = currWord.data.range(i*16+7, i*16);
                    icc_sums[i] += temp;
                    icc_sums[i] = (icc_sums[i] + (icc_sums[i] >> 16)) & 0xFFFF;
                }
                else if (currWord.keep[i*2] == 0x1) {
                    temp(7, 0) = 0;
                    temp(15, 8) = currWord.data.range(i*16+7, i*16);
                    icc_sums[i] += temp;
                    icc_sums[i] = (icc_sums[i] + (icc_sums[i] >> 16)) & 0xFFFF;
                }
            }
            sendWord = icc_prevWord;
            soIPd_Data.write(sendWord);
            break;
        } // switch

        icc_prevWord = currWord;
        icc_wordCount++;

        if (currWord.last == 1) {
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
        stream<axiWord>         &siUDP_Data,
        stream<axiWord>         &siIPRX_Derr,
		stream<axiWord>         &soIHa_Data,
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
                axiWord tempWord = {0, 0xFF, 0};
                if (udpInEmpty == 0) {
                    tempWord.data = 0x0000000000000303;
                    streamSource = 0;
                }
                else if (ttlInEmpty == 0) {
                    tempWord.data = 0x000000000000000B;
                    streamSource = 1;
                }
                udpChecksum = (((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16)) + tempWord.data.range(15, 0));
                soIHa_Data.write(tempWord);
                bcm_fsmState = BCM_IP;
            }
            break;
        case BCM_IP:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !soIHa_Data.full() && !soIHa_Hdr.full()) { // If there are data in the queue start reading them
                axiWord tempWord = {0, 0, 0};
                if (streamSource == 0)
                    tempWord = siUDP_Data.read();
                else if (streamSource == 1)
                    tempWord = siIPRX_Derr.read();
                udpChecksum = (udpChecksum + ((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16) + tempWord.data.range(15, 0)));
                soIHa_Data.write(tempWord);
                soIHa_Hdr.write(tempWord.data);
                if (ipWordCounter == 2)
                    bcm_fsmState = BCM_STREAM;
                else 
                    ipWordCounter++;
            }
            break;  
        case BCM_STREAM:
            if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !soIHa_Data.full()) { // If there are data in the queue start reading them
                axiWord tempWord = {0, 0, 0};
                if (streamSource == 0)
                    tempWord = siUDP_Data.read();
                else if (streamSource == 1)
                    tempWord = siIPRX_Derr.read();
                udpChecksum = (udpChecksum + ((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16) + tempWord.data.range(15, 0)));
                soIHa_Data.write(tempWord);
                if (tempWord.last == 1)
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
        stream<axiWord>         &siCMb_Data,
        stream<ap_uint<64> >    &soIHa_Hdr,
        stream<axiWord>         &soICi_Data,
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
    static axiWord tempWord     = {0, 0, 0};
    static ap_int<32> sourceIP  = 0;

    switch(aih_fsmState) {
        case AIH_IDLE:
            if (!soIHa_Hdr.empty() && !soICi_Data.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
                tempWord.data = soIHa_Hdr.read();
                ap_uint<16> tempLength = byteSwap16(tempWord.data.range(31, 16));
                tempWord.data.range(31, 16) = byteSwap16(tempLength + 28);
                tempWord.keep = 0xFF;
                tempWord.last = 0;
                soICi_Data.write(tempWord);
                aih_fsmState = AIH_IP;
            }
            break;
        case AIH_IP:
            if (!soIHa_Hdr.empty() && !soICi_Data.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
                tempWord.data               = soIHa_Hdr.read();
                tempWord.data.range( 7, 0)  = 0x80;                              // Set the TTL to 128
                tempWord.data.range(15, 8)  = 0x01;                              // Swap the protocol from whatever it was to ICMP
                sourceIP                    = tempWord.data.range(63, 32); //OBSOLETE-20181112 tempWord.data.range(63, 32) =   0x01010101;
                tempWord.data.range(63, 32) = piMMIO_IpAddress;
                tempWord.keep               = 0xFF;
                tempWord.last               = 0;
                soICi_Data.write(tempWord);
                aih_fsmState = AIH_MERGE;
            }
            break;
        case AIH_MERGE:
            if (!soIHa_Hdr.empty() && !siCMb_Data.empty() && !soICi_Data.full()) {
                soIHa_Hdr.read();
                ap_uint<64> tempData = sourceIP;
                tempWord             = siCMb_Data.read();
                axiWord outputWord   = {0, 0xFF, 0};
                outputWord.data      = tempData;
                outputWord.data.range(63, 32) = tempWord.data.range(31,  0);
                soICi_Data.write(outputWord);
                aih_fsmState = AIH_STREAM;
            }
            break;
        case AIH_STREAM:
            if (!siCMb_Data.empty() && !soICi_Data.full()) {
                axiWord outputWord = {0, 0xFF, 0};
                outputWord.data.range(31, 0) = tempWord.data.range(63,  32);
                tempWord = siCMb_Data.read();
                outputWord.data.range(63, 32) = tempWord.data.range(31,  0);
                if (tempWord.last == 1) {
                    if (tempWord.keep.range(7, 4) == 0) {
                        outputWord.last = 1;
                        outputWord.keep.range(7, 4) = tempWord.keep.range(3,0);
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
                axiWord outputWord              = {0, 0, 1};
                outputWord.data.range(31, 0)    = tempWord.data.range(63, 32);
                outputWord.keep.range(3, 0)     = tempWord.keep.range(7, 4);
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
        stream<axiWord>     &siICc_Data,
        stream<bool>        &siICc_CsumValid,
        stream<axiWord>     &soICi_Data)
{
    static bool d_isFirstWord   = true;
    static bool d_drop          = false;
    bool d_valid;
    axiWord currWord;

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
        if (currWord.last == 1) {
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
 *  @param[out] soIPTX_Data, The data stream to the IP Tx handler (IPTX).
 *
 * @details
 *  This process inserts both the IP header checksum and the ICMP checksum in
 *  the outgoing packet.
 *
 *****************************************************************************/
void pIcmpChecksumInserter(
        stream<axiWord>      siXYz_Data[2],
        stream<ap_uint<16> > siUVw_Csum[2],
        stream<axiWord>      &soIPTX_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "ICi");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<7>          ici_wordCount=0; // The max len of ICMP messages is 576 bytes
    #pragma HLS RESET variable=ici_wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<1>   streamSource;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    axiWord             inputWord    = {0, 0, 0};
    ap_uint<16>         icmpChecksum = 0;

    switch(ici_wordCount) {
    case 0:
        bool streamEmptyStatus[2]; // Status of the input data streams
        for (uint8_t i=0; i<2; ++i) {
            streamEmptyStatus[i] = siXYz_Data[i].empty();
        }
        for (uint8_t i=0; i<2; ++i) {
            if(!streamEmptyStatus[i]) {
                streamSource = i;
                inputWord = siXYz_Data[i].read();
                soIPTX_Data.write(inputWord);
                ici_wordCount++;
                break;
            }
        }
        break;
    case 2:
        if (!siXYz_Data[streamSource].empty() &&
            !siUVw_Csum[streamSource].empty()) {
            siXYz_Data[streamSource].read(inputWord);
            icmpChecksum = siUVw_Csum[streamSource].read();
            inputWord.data(63, 48) = icmpChecksum;
            soIPTX_Data.write(inputWord);
            ici_wordCount++;
        }
        break;
    default:
        if (!siXYz_Data[streamSource].empty()) {
            siXYz_Data[streamSource].read(inputWord);
            soIPTX_Data.write(inputWord);
            if (inputWord.last == 1) {
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
		stream<axiWord>     &siIPRX_Data,
        stream<axiWord>     &siIPRX_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<axiWord>     &siUDP_Data,

        //------------------------------------------------------
        //-- IPTX Interface
        //------------------------------------------------------
        stream<axiWord>     &soIPTX_Data)
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
    static stream<axiWord>           ssICcToIPd_Data        ("ssICcToIPd_Data");
    #pragma HLS stream      variable=ssICcToIPd_Data        depth=64
    #pragma HLS DATA_PACK   variable=ssICcToIPd_Data

    static stream<bool>              ssICcToIPd_CsumValid   ("ssICcToIPd_CsumValid");
    #pragma HLS stream      variable=ssICcToIPd_CsumValid   depth=8

    //-- Control Message Builder (CMb)
    static stream<axiWord>           ssCMbToIHa_Data         ("ssCMbToIHa_Data");
    #pragma HLS stream      variable=ssCMbToIHa_Data         depth=192
    static stream<ap_uint<64> >      ssCMbToIHa_Hdr          ("ssCMbToIHa_Hdr");
    #pragma HLS stream      variable=ssCMbToIHa_Hdr          depth=64

    //-- XYz = [InvalidPacketDropper (IPd)| IpHeaderAppender (IHa)]
    static stream<axiWord>           ssToICi_Data[2];
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

