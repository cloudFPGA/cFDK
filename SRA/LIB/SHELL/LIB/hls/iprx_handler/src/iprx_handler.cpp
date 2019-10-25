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
 * @file       : iprx_handler.cpp
 * @brief      : IP receiver frame handler (IPRX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *-----------------------------------------------------------------------------
 *
 * @details    : This process connects to the Rx side of the Ethernet MAC core.
 *               It extracts the IPv4 and ARP packets from the Ethernet frame
 *               and forwards them the TCP-offload-engine (TOE), the Internet
 *               Control Message Protocol (ICMP) engine, the UDP engine (UDP)
 *               or the Address Resolution Protocol (ARP) server.
 *
 * @[TODO]     - Re-align the internal flow of IPv4 packets in order to re-use
 *               the methods of the class 'Ip4overMac'.
 *             - Why do we need an IP input buffer? Can we get ride of it?
 *
 *****************************************************************************/

#include "iprx_handler.hpp"
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
#define THIS_NAME "IPRX"

#define TRACE_OFF  0x0000
#define TRACE_IBUF 1 << 1
#define TRACE_MPD  1 << 2
#define TRACE_ILC  1 << 3
#define TRACE_ICA  1 << 4
#define TRACE_ICC  1 << 5
#define TRACE_IID  1 << 6
#define TRACE_ICL  1 << 7
#define TRACE_IPR  1 << 8
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


/*****************************************************************************
 * Input Buffer (IBuf)
 *
 * @param[in]  siETH_Data,	Data steam from the ETHeernet interface.
 * @param[out] soMPd_Data,	Data stream to Mac Protocol Detector (MPd).
 *
 * @details
 *  This process enqueues the incoming data traffic into FiFo.
 *    [FIXME - Do we really need this?].
 *
 *****************************************************************************/
void pInputBuffer(
        stream<AxiWord>     &siETH_Data,
        stream<AxiWord>     &soMPd_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IBuf");

    if(!siETH_Data.empty() && !soMPd_Data.full()){
        soMPd_Data.write(siETH_Data.read());
    }
}

/*****************************************************************************
 * Mac Protocol Detector (MPd)
 *
 * @param[in]  siIBuf_Data, Data steam from the Input Buffer (IBuf).
 * @param[out] soARP_Data,  Data stream to ARP.
 * @param[out] soILc_Data,  Data stream to the IPv4 Length Checket (ILc).
 * @param[in]  piMMIO_MacAddr, The MAC address from MMIO.
 *
 * @details
 *  This process parses the Ethernet header to detect ARP and IPv4 frames.
 *  These two types of frames are forwarded accordingly, while other types of
 *  frames are dropped.
 *****************************************************************************/
void pMacProtocolDetector(
        LE_EthAddr           piMMIO_MacAddr,
        stream<AxiWord>     &siIBuf_Data,
        stream<AxiWord>     &soARP_Data,
        stream<AxiWord>     &soILc_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "MPd");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { S0=0, S1 } fsmState=S0;
    #pragma HLS reset         variable=fsmState
    static ap_uint<2>                  wordCount=0;
    #pragma HLS reset         variable=wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static EtherType    etherType;
    static AxiWord      prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    EthoverMac          currWord;

    switch (fsmState) {
    case S0:
        if (!siIBuf_Data.empty() && !soARP_Data.full() && !soILc_Data.full()) {
            siIBuf_Data.read(currWord);
            switch (wordCount) {
            case 0:
                // Compare MAC_DA with our MMIO_MacAddress and with BROADCAST
                //  [TODO - Is this really needed given ARP ?]
                //OBSOLETE-20191015 if (currWord.tdata.range(47, 0) != piMMIO_MacAddr &&
                //OBSOLETE-20191015     currWord.tdata.range(47, 0) !=0xFFFFFFFFFFFF) {
                if (currWord.getLE_EthDstAddr() != piMMIO_MacAddr &&
                    currWord.getLE_EthDstAddr() != 0xFFFFFFFFFFFF) {
                    etherType = DROP;
                    printInfo(myName, "Requesting current frame with MAC destination address = 0x%16.16lX (0x%16.16lX) to be dropped\n",
                                      currWord.getEthDstAddr().to_long(), currWord.getLE_EthDstAddr().to_long());
                }
                else {
                    etherType = FORWARD;
                }
                wordCount++;
                break;
            default:
                if (wordCount == 1) {
                    if (etherType != DROP) {
                        //OBSOLETE-2191015 macType = byteSwap16(currWord.tdata(47, 32));
                        etherType = currWord.getEtherType();
                    }
                    wordCount++;
                }
                if (etherType == ARP) {
                    soARP_Data.write(prevWord);
                }
                else if (etherType == IPv4) {
                    soILc_Data.write(prevWord);
                }
                break;
            }
            prevWord = currWord;
            if (currWord.tlast) {
                wordCount = 0;
                fsmState = S1;
            }
        }
        break;
    case 1:
        if( !soARP_Data.full() && !soILc_Data.full()){
            if (etherType == ARP) {
                soARP_Data.write(prevWord);
            }
            else if (etherType == IPv4) {
                soILc_Data.write(prevWord);
            }
            fsmState = S0;
        }
        break;

    } // End of: switch
}

/*****************************************************************************
 * IP Length Checker (ILc) - [FIXME:Consider removing this process]
 *
 * @param[in]  siMPd_Data,  Data steam from the MAC Protocol Detector (MPd).
 * @param[out] soICa_Data,  Data stream to IP Checksum Accumulator (ICa).
 *
 * @details
 *  This process parses the checks the IPv4/TotalLength field.
 *
 *****************************************************************************/
void pIpLengthChecker(
        stream<AxiWord>     &siMPd_Data,
        stream<AxiWord>     &soICa_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ILc");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static ap_uint<2>          leftToWrite=0;
    #pragma HLS reset variable=leftToWrite
    static enum FsmStates {FSM_IDLE=0, FSM_SIZECHECK, FSM_STREAM} fsmState=FSM_IDLE;
    #pragma HLS reset variable=fsmState
    static ap_uint<2>          wordCount=0;
    #pragma HLS reset variable=wordCount
    static ap_uint<1>          filterPacket=0;
    #pragma HLS reset variable=filterPacket

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_shift_reg<AxiWord, 2> wordBuffer;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     tempWord;
    Ip4TotalLen ip4TotLen;

    if (leftToWrite == 0) {
        if (!siMPd_Data.empty() && !soICa_Data.full()) {
            AxiWord currWord = siMPd_Data.read();
            switch (fsmState) {
            case FSM_IDLE:
                wordBuffer.shift(currWord);
                if (wordCount == 1) {
                    fsmState = FSM_SIZECHECK;
                }
                wordCount++;
                break;
            case FSM_SIZECHECK:
                // Check the IPv4/TotalLength filed ([FIXME - Consider removing])
                ip4TotLen = byteSwap16(currWord.tdata.range(15, 0));
                //OBSOLETE-20191025 byteSwap16(currWord.tdata.range(15, 0)) > MaxDatagramSize ? filterPacket = 1 : filterPacket = 0;
                if (ip4TotLen > MaxDatagramSize) {
                    filterPacket = 1;
                    printWarn(myName, "Current frame will be discarded because frame is too long!\n");
                }
                else {
                    filterPacket = 0;
                    if (DEBUG_LEVEL & TRACE_ILC) {
                        printInfo(myName, "Start of IPv4 packet - Total length = %d bytes.\n", ip4TotLen.to_int());
                    }
                }
                tempWord = wordBuffer.shift(currWord);
                if (filterPacket == 0) {
                    soICa_Data.write(tempWord);
                }
                fsmState = FSM_STREAM;
                break;
           case FSM_STREAM:
                tempWord = wordBuffer.shift(currWord);
                if (filterPacket == 0) {
                    soICa_Data.write(tempWord);
                }
                break;
            } // End of: switch

            if (currWord.tlast) {
                wordCount   = 0;
                leftToWrite = 2;
                fsmState    = FSM_IDLE;
            }
       }
    }
    else if (leftToWrite != 0 && !soICa_Data.full() ) {
        tempWord = wordBuffer.shift(AxiWord(0, 0, 0));
        if (filterPacket == 0) {
            soICa_Data.write(tempWord);
        }
        leftToWrite--;
    }

//   if (leftToWrite == 0) {
//            if (!siMPd_Data.empty() && !ipDataFifo.full()) {
//                AxiWord currWord = siMPd_Data.read();
//            switch (lengthCheckState) {
//                case LC_IDLE:
//                    wordBuffer.shift(currWord);
//                    if (dip_wordCount == 1)
//                        lengthCheckState = LC_SIZECHECK;
//                    dip_wordCount++;
//                    break;
//                case LC_SIZECHECK:
//                    byteSwap16(currWord.tdata.range(15, 0)) > MaxDatagramSize ? filterPacket = 1 : filterPacket = 0;
//                    temp = wordBuffer.shift(currWord);
//                    if (filterPacket == 0)
//                        ipDataFifo.write(temp);
//                    lengthCheckState = LC_STREAM;
//                    break;
//                case LC_STREAM:
//                    temp = wordBuffer.shift(currWord);
//                    if (filterPacket == 0)
//                        ipDataFifo.write(temp);
//                    break;
//             }
//             if (currWord.tlast) {
//                dip_wordCount   = 0;
//                leftToWrite = 2;
//                lengthCheckState    = LC_IDLE;
//             }
//            }
//   } else if (leftToWrite != 0 && !ipDataFifo.full() ) {
//      //AxiWord nullAxiWord = {0, 0, 0};
//      temp = wordBuffer.shift(AxiWord(0, 0, 0));
//      if (filterPacket == 0)
//          ipDataFifo.write(temp);
//      leftToWrite--;
//   }

}

/*****************************************************************************
 * IP Checksum Checker (ICa)
 *
 * @param[in]  piMMIO_Ip4Address, the IPv4 address from MMIO (in network order). [FIXME - Not yet]
 * @param[in]  siILc_Data,        Data stream from IpLengthChecker (ILc).
 * @param[out] soIId_Data,        Data stream to IpInvalidDropper (IId).
 * @param[out] soIId_IpVer,       The IP version to [IId].
 * @param[out] soIId_DropFrag,    Tell [IId] to drop this IP fragment.
 * @param[out] soICc_SubSums,     Four sub-checksums to IpChecksumChecker (ICc).


 * @details
 *  This process computes four sub-sums to assess the IPv4 header checksum.
 *
 *****************************************************************************/
void pIpChecksumAccumulator(
        LE_Ip4Addr            piMMIO_Ip4Address,
        stream<AxiWord>      &siILc_Data,
        stream<AxiWord>      &soIId_Data,
        stream<Ip4Version>   &soIId_IpVer,
        stream<CmdBit>       &soIId_DropFrag,
        stream<SubSums>      &soICc_SubSums)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICa");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static bool                wasLastWord=false;
    #pragma HLS reset variable=wasLastWord
    static ap_uint<3>          wordCount=0;
    #pragma HLS reset variable=wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<17> ipHdrSums[4];
    static ap_uint<8>  ipHdrLen;
    static LE_Ip4Addr  dstIpAddress;
    static AxiWord     prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord     currWord;
    AxiWord     sendWord;
    ap_uint<16> ip4FlagsAndFragOff;
    bool        ipAddrMatch;

    currWord.tlast = 0;
    if (!siILc_Data.empty() && !wasLastWord && !soIId_IpVer.full() &&
        !soIId_DropFrag.full() && !soICc_SubSums.full() && !soIId_Data.full()) {
        siILc_Data.read(currWord);

        switch (wordCount) {
        case 0:
            // Skip because word contains MAC_SA[1:0] | MAC_DA[5:0]
            for (uint8_t i=0; i<4 ;++i) {
                ipHdrSums[i] = 0;
            }
            wordCount++;
            break;

        case 1:
            // Retrieve the IP version field
            soIId_IpVer.write(currWord.tdata.range(55, 52));
            // Retrieve the Internet Header Length
            ipHdrLen = currWord.tdata.range(51, 48);
            // Retrieve the first two octets of the IPv4 header
            ipHdrSums[3] += byteSwap16(currWord.tdata.range(63, 48));
            ipHdrSums[3] = (ipHdrSums[3] + (ipHdrSums[3] >> 16)) & 0xFFFF;
            wordCount++;
            break;

        case 2:
            // Check for fragmentation in this cycle. Need to check the MF flag.
            // If set, then drop. If not set, then check the fragment offset and
            // if it is non-zero then drop.
            ip4FlagsAndFragOff = byteSwap16(currWord.tdata.range(47, 32));

            if (ip4FlagsAndFragOff.range(12, 0) != 0 || ip4FlagsAndFragOff.bit(13) != 0) {
                soIId_DropFrag.write(CMD_DROP);
                if (ip4FlagsAndFragOff.bit(13) != 0) {
                    printWarn(myName, "The More Fragments (MF) is set but IP fragmentation is not supported (yet).\n");
                }
                if (ip4FlagsAndFragOff.range(12, 0) != 0) {
                    printWarn(myName, "Fragment offset is set but not supported (yet) !!!\n");
                }
            }
            else {
                soIId_DropFrag.write(CMD_KEEP);
            }

            for (int i = 0; i < 4; i++) {
              #pragma HLS unroll
                ipHdrSums[i] += byteSwap16(currWord.tdata.range(i*16+15, i*16));
                ipHdrSums[i] = (ipHdrSums[i] + (ipHdrSums[i] >> 16)) & 0xFFFF;
            }
            ipHdrLen -= 2;
            wordCount++;
            break;

        case 3:
            dstIpAddress.range(15, 0) = currWord.tdata.range(63, 48);
            for (int i = 0; i < 4; i++) {
              #pragma HLS unroll
                ipHdrSums[i] += byteSwap16(currWord.tdata.range(i*16+15, i*16));
                ipHdrSums[i] = (ipHdrSums[i] + (ipHdrSums[i] >> 16)) & 0xFFFF;
            }
            ipHdrLen -= 2;
            wordCount++;
            break;

        default:
            if (wordCount == 4)
                dstIpAddress(31, 16) = currWord.tdata(15, 0);

            switch (ipHdrLen) {
            case 0:
                break;
            case 1:
                // Sum up part0
                ipHdrSums[0] += byteSwap16(currWord.tdata.range(15, 0));
                ipHdrSums[0] = (ipHdrSums[0] + (ipHdrSums[0] >> 16)) & 0xFFFF;
                ipHdrLen = 0;
                // Assess destination IP address
                if (dstIpAddress == piMMIO_Ip4Address || dstIpAddress == 0xFFFFFFFF) {
                    ipAddrMatch = true;
                }
                else {
                    ipAddrMatch = false;
                }
                soICc_SubSums.write(SubSums(ipHdrSums, ipAddrMatch));
                break;
            case 2:
                // Sum up part 0-2
                for (int i = 0; i < 4; i++) {
                  #pragma HLS unroll
                    ipHdrSums[i] += byteSwap16(currWord.tdata.range(i*16+15, i*16));
                    ipHdrSums[i] = (ipHdrSums[i] + (ipHdrSums[i] >> 16)) & 0xFFFF;
                }
                ipHdrLen = 0;

                if (dstIpAddress == piMMIO_Ip4Address || dstIpAddress == 0xFFFFFFFF) {
                    ipAddrMatch = true;
                }
                else {
                    ipAddrMatch = false;
                }
                soICc_SubSums.write(SubSums(ipHdrSums, ipAddrMatch));
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                  #pragma HLS unroll
                    ipHdrSums[i] += byteSwap16(currWord.tdata.range(i*16+15, i*16));
                    ipHdrSums[i] = (ipHdrSums[i] + (ipHdrSums[i] >> 16)) & 0xFFFF;
                }
                ipHdrLen -= 2;
                break;
            }
            break;

        } // End of: switch(wordCount)

        if (wordCount > 2) {
            // Send AxiWords while re-aligning the outgoing chunks
            sendWord = AxiWord((currWord.tdata(47, 0), prevWord.tdata(63, 48)),
                               (currWord.tkeep( 5, 0), prevWord.tkeep( 7,  6)),
                               (currWord.tkeep[6] == 0));
            soIId_Data.write(sendWord);
        }

        prevWord = currWord;
        if (currWord.tlast) {
            wordCount = 0;
            wasLastWord = !sendWord.tlast;
        }
    }
    else if(wasLastWord && !soIId_Data.full()) {
        // Send remaining Word;
        sendWord = AxiWord(prevWord.tdata.range(63, 48), prevWord.tkeep.range(7, 6), 1);
        soIId_Data.write(sendWord);
        wasLastWord = false;
    }
}

/******************************************************************************
 * IP Invalid Dropper (IId)
 *
 * @param[in]  siICa_Data,      Data stream from IpChecksumAccumulator (ICa).
 * @param[in]  siICa_IpVer,     The IP version from [ICa].
 * @param[in]  siICa_DropFrag,  Drop this IP fragment from [ICA].
 * @param[out] siICc_CsumValid, Checksum is valid from IpChecksumChecker (ICc).
 * @param[out] soICl_Data,      Data stream to IP Cut Length (ICl).
 *
 * @details
 *  Drops an IP packet when its version is not '4' (siICa_IpVer), when it is a
 *   fragmented packet (siICa_DropFrag) or when its IP header checksum is not
 *   valid (siICc_CsumValid). Otherwise, the IPv4 packet is passed on.
 *
 *****************************************************************************/
void pIpInvalidDropper(
        stream<AxiWord>      &siICa_Data,
        stream<Ip4Version>   &siICa_IpVer,
        stream<ValBit>       &siICa_DropFrag,
        stream<ValBit>       &siICc_CsumValid,
        stream<AxiWord>      &soICl_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IId");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {FSM_IDLE=0, FSM_FWD, FSM_DROP} fsmState=FSM_IDLE;
    #pragma HLS reset                           variable=fsmState

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    AxiWord currWord = AxiWord(0, 0, 0);

    switch(fsmState) {
    case FSM_IDLE:
        if (!siICc_CsumValid.empty() && !siICa_IpVer.empty() && !siICa_DropFrag.empty() &&
            !siICa_Data.empty() && !soICl_Data.full()) {
            siICa_Data.read(currWord);
            // Assess validity of the current IPv4 packet
            //OBSOLETE-20191021 ap_uint<2> valid = siICc_CsumValid.read() + ipValidFifoVersionNo.read() + siICa_DropFrag.read(); // Check the fragment drop queue
            ValBit csumValid = (siICc_CsumValid.read() == 1);        // CSUM is valid
            ValBit isIpv4Pkt = (siICa_IpVer.read()     == 4);        // IP Version is 4
            ValBit doKeepPkt = (siICa_DropFrag.read()  == CMD_KEEP); // Do not drop

            ap_uint<2> valid = (csumValid + isIpv4Pkt + doKeepPkt);
            if (valid != 3) {
                fsmState = FSM_DROP;
                printWarn(myName, "The current IP packet will be dropped because:\n");
                printWarn(myName, "  csumValid=%d | isIpv4Pkt=%d | doKeepPkt=%d\n",
                          csumValid.to_int(), isIpv4Pkt.to_int(), doKeepPkt.to_int());
                // [FIXME]  printAxiWord(myName, "Dropping:", currWord);
            }
            else {
                soICl_Data.write(currWord);
                fsmState = FSM_FWD;
            }
        }
        break;
    case FSM_FWD:
        if(!siICa_Data.empty() && !soICl_Data.full()) {
            siICa_Data.read(currWord);
            soICl_Data.write(currWord);
            if (currWord.tlast == 1) {
                fsmState = FSM_IDLE;
            }
        }
        break;
    case FSM_DROP:
        if(!siICa_Data.empty()){
            siICa_Data.read(currWord);
            // [FIXME] printAxiWord(myName, "Dropping:", currWord);
            if (currWord.tlast == 1) {
                fsmState = FSM_IDLE;
            }
        }
        break;
    }
}

/******************************************************************************
 * IPv4 Cut Length (ICl)
 *
 * @param[in]  siIId_Data,  Data stream from IpInvalidDropper (IId).
 * @param[out] soIPr_Data,  Data stream to Ip PAcket Router (IPr).
 *
 * @details
 *  This process intercepts and drops packets which are longer than the
 *   announced 'Ip4TotalLenght' field.
 *
 *****************************************************************************/
void pIpCutLength(
        stream<AxiWord>  &siIId_Data,
        stream<AxiWord>  &soIPr_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICl");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates {FSM_FWD=0, FSM_DROP} fsmState=FSM_FWD;
    #pragma HLS reset                 variable=fsmState
    static ap_uint<13>                         wordCount=0; // Ip4TotalLen/8
    #pragma HLS reset                 variable=wordCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static Ip4TotalLen  ip4TotalLength;

    switch(fsmState){
    case FSM_FWD:
        if (!siIId_Data.empty() && !soIPr_Data.full()) {
            AxiWord currWord = siIId_Data.read();
            switch (wordCount) {
            case 0:
                ip4TotalLength = byteSwap16(currWord.tdata(31, 16)); // why not (15:0)???
                break;
            default:
                if (((wordCount+1)*8) >= ip4TotalLength) {
                    if (currWord.tlast == 0) {
                        fsmState = FSM_DROP;
                    }
                    currWord.tlast = 1;
                    ap_uint<4> leftLength = ip4TotalLength - (wordCount*8);
                    currWord.tkeep = returnKeep(leftLength);
                }
                break;
            } // End of: switch(wordCount)

            soIPr_Data.write(currWord);
            wordCount++;
            if (currWord.tlast) {
                wordCount = 0;
            }
        }
        break;

    case FSM_DROP:
        if (!siIId_Data.empty()) {
            AxiWord currWord = siIId_Data.read();
            if (currWord.tlast) {
                fsmState = FSM_FWD;
            }
        }
        break;

    } // End of: switch(fsmState)
}

/******************************************************************************
 * IPv4 Checksum Checker (ICc)
 *
 * @param[in]  siICa_SubSums,   Four sub-checksums from IpChecksumAccumulator (ICa).
 * @param[out] soIId_CsumValid, Checksum valid information.
 *
 * @details
 *  This process .........
 *
 *****************************************************************************/
void pIpChecksumChecker(
        stream<SubSums>   &siICa_SubSums,
        stream<ValBit>    &soIId_CsumValid)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HlS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICc");

    if (!siICa_SubSums.empty() && !soIId_CsumValid.full()) {
        SubSums ipHdrSums = siICa_SubSums.read();

        ipHdrSums.sum0 += ipHdrSums.sum2;
        ipHdrSums.sum1 += ipHdrSums.sum3;
        ipHdrSums.sum0 = (ipHdrSums.sum0 + (ipHdrSums.sum0 >> 16)) & 0xFFFF;
        ipHdrSums.sum1 = (ipHdrSums.sum1 + (ipHdrSums.sum1 >> 16)) & 0xFFFF;
        ipHdrSums.sum0 += ipHdrSums.sum1;
        ipHdrSums.sum0 = (ipHdrSums.sum0 + (ipHdrSums.sum0 >> 16)) & 0xFFFF;
        ipHdrSums.sum0 = ~ipHdrSums.sum0;
        if (ipHdrSums.sum0(15, 0) != 0x0000) {
            printError(myName, "Bad IP header checksum: Expected 0x0000 - Computed 0x%4.4X\n", ipHdrSums.sum0(15, 0).to_int());
        }
        soIId_CsumValid.write((ipHdrSums.sum0(15, 0) == 0x0000) && ipHdrSums.ipMatch);
    }
}

/******************************************************************************
 * IPv4 Packet Router (IPr)
 *
 * @param[in]  siICl_Data,  Data stream from IpCutLength (ICl).
 * @param[out] soICMP_Data, Data stream to ICMP.
 * @param[out] soICMP_Derr, Data error stream(**) to ICMP.
 * @param[out] soUDP_Data,  Data stream to UDP engine.
 * @param[out] soTCP_Data,  Data stream to TCP offload engine.
 *
 * @details
 *  This process routes the IPv4 packets to one of the 3 following engines:
 *  ICMP, TCP or UDP.
 *
 *****************************************************************************/
void pIpPacketRouter(
		stream<AxiWord>  &siICl_Data,
		stream<AxiWord>  &soICMP_Data,
		stream<AxiWord>  &soICMP_Derr,
		stream<AxiWord>  &soUDP_Data,
		stream<AxiWord>  &soTCP_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS INLINE off
	#pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IPr");

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum FsmStates { FSM_IDLE=0, FSM_LAST} fsmState=FSM_IDLE;
	#pragma HLS reset                    variable=fsmState
    static ap_uint<2>                             wordCount=0;
    #pragma HLS reset                    variable=wordCount
    static bool                                   leftToWrite=false;
    #pragma HLS reset                    variable=leftToWrite

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static StsBit		ttlExpired;
    static Ip4Prot		ipProtocol;
    static Ip4overMac	prevWord;

    //-- DYNAMIC VARIABLES ----------------------------------------------------
    Ip4overMac			currWord;

     switch (fsmState) {
     case FSM_IDLE:
         if (!siICl_Data.empty() &&
        	 !soICMP_Derr.full() && !soICMP_Data.full() &&
			 !soUDP_Data.full()  && !soTCP_Data.full()) {
             siICl_Data.read(currWord);
             switch (wordCount) {
                case 0:
                    wordCount++;
                    break;
                default:
                    if (wordCount == 1) {
                        //OBSOLETE-20191025 if (currWord.tdata.range(7, 0) == 1)
                        if (currWord.getIp4Ttl() == 1) {
                            ttlExpired = 1;
                        }
                        else {
                            ttlExpired = 0;
                        }
                        //OBSOLETE-20191025 dip_ipProtocol = currWord.tdata.range(15, 8);
                        ipProtocol = currWord.getIp4Prot();
                        wordCount++;
                    }
                    if (ttlExpired == 1) {
                        soICMP_Derr.write(prevWord);
                    }
                    else {
                        switch (ipProtocol) {
                        // FYI - There is not default case. If the current packet
                        //  does not match any case, it is automatically dropped.
                        case ICMP:
                            soICMP_Data.write(prevWord);
                            break;
                        case UDP:
                            soUDP_Data.write(prevWord);
                            break;
                        case TCP:
                            soTCP_Data.write(prevWord);
                            break;
                        }
                    }
                    break;
                }
                prevWord = currWord;
                if (currWord.tlast) {
                    wordCount = 0;
                    leftToWrite = true;
                    fsmState = FSM_LAST;
                }
            }
     break;

     case FSM_LAST:
         if (!soICMP_Derr.full() &&
        	 !soICMP_Data.full() && !soUDP_Data.full() && !soTCP_Data.full() ) {
        	 uint8_t bitCounter = 0;
        	 bitCounter = keepMapping(prevWord.tkeep);
        	 if (prevWord.tkeep != 0xFF) {
        		 prevWord.tdata.range(63, 64-((8-bitCounter)*8)) = 0;
        	 }
        	 if (ttlExpired == 1) {
        		 soICMP_Derr.write(prevWord);
        	}
        	else {
        		switch (ipProtocol) {
        		case ICMP:
        			soICMP_Data.write(prevWord);
        			break;
        		 case UDP:
        			soUDP_Data.write(prevWord);
        			break;
        		 case TCP:
        			soTCP_Data.write(prevWord);
        			break;
        		}
        	}
            leftToWrite = false;
            fsmState = FSM_IDLE;
         }
         break;
     }
}

/*****************************************************************************
 * @brief   Main process of the IP Receiver Handler.
 *
 * @param[in]  piMMIO_MacAddress, the MAC address from MMIO (in network order). [FIXME - Not yet]
 * @param[in]  piMMIO_Ip4Address, the IPv4 address from MMIO (in network order). [FIXME - Not yet]
 * @param[in]  siETH_Data,        Data stream from ETHernet MAC layer.
 * @param[out] soARP_Data,        Data stream to ARP server.
 * @param[out] soICMP_Data,       Data stream to ICMP.
 * @param[out] soICMP_Derr,       Data error stream(**) to ICMP.
 * @param[out] soUDP_Data,        Data stream to UDP engine.
 * @param[out] soTCP_Data,        Data stream to TCP offload engine.
 *
 * @note:
 *  ** The data-error stream is used instead of the data stream when the TTL
 *     field of the incoming IPv4 packet is found the be zero. In such a case,
 *     the erroneous packet is forwarded to the ICMP module over the data-error
 *     stream. The ICMP module uses this information to build an ICMP error
 *     datagram (of type #11 --Time Exceeded) that is sent back to the sender.
 *
 *****************************************************************************/
void iprx_handler(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        LE_EthAddr           piMMIO_MacAddress,
        LE_Ip4Addr           piMMIO_Ip4Address,

        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxiWord>     &siETH_Data,

        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxiWord>     &soARP_Data,

        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxiWord>     &soICMP_Data,
        stream<AxiWord>     &soICMP_Derr,

        //------------------------------------------------------
        //-- UDP Interface
        //------------------------------------------------------
        stream<AxiWord>     &soUDP_Data,

        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxiWord>     &soTCP_Data)

{
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

    #pragma HLS INTERFACE ap_stable           port=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable           port=piMMIO_Ip4Address

    #pragma  HLS resource core=AXI4Stream variable=siETH_Data  metadata="-bus_bundle siETH_Data"

    #pragma  HLS resource core=AXI4Stream variable=soARP_Data  metadata="-bus_bundle soARP_Data"

    #pragma  HLS resource core=AXI4Stream variable=soICMP_Data metadata="-bus_bundle soICMP_Data"
    #pragma  HLS resource core=AXI4Stream variable=soICMP_Derr metadata="-bus_bundle soICMP_Derr"

    #pragma  HLS resource core=AXI4Stream variable=soUDP_Data  metadata="-bus_bundle soUDP_Data"
    #pragma  HLS resource core=AXI4Stream variable=soTCP_Data  metadata="-bus_bundle soTCP_Data"

#else

    #pragma HLS INTERFACE ap_stable port=piMMIO_MacAddress     name=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable port=piMMIO_Ip4Address     name=piMMIO_Ip4Address

    #pragma HLS INTERFACE axis      port=siETH_Data            name=siETH_Data

    #pragma HLS INTERFACE axis      port=soARP_Data            name=soARP_Data

    #pragma HLS INTERFACE axis      port=soICMP_Data           name=soICMP_Data
    #pragma HLS INTERFACE axis      port=soICMP_Derr           name=soICMP_Data

    #pragma HLS INTERFACE axis      port=soUDP_Data            name=soUDP_Data
    #pragma HLS INTERFACE axis      port=soTCP_Data            name=soTCP_Data

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Input Buffer (IBuf)
    static stream<AxiWord>          ssIBufToMPd_Data    ("ssIBufToMPd_Data");
    #pragma HLS STREAM     variable=ssIBufToMPd_Data    depth=8000

    //-- MAC Protocol Detector (MPd)
    static stream<AxiWord>          ssMPdToILc_Data     ("ssMPdToILc_Data");
    #pragma HLS STREAM     variable=ssMPdToILc_Data     depth=32

    //-- IPv4 Length Checker (ILc)
    static stream<AxiWord>          ssILcToICa_Data     ("ssILcToICa_Data");
    #pragma HLS STREAM     variable=ssILcToICa_Data     depth=32

    //-- IPv4 Checksum Accumulator (ICa)
    static stream<AxiWord>          ssICaToIId_Data     ("ssICaToIId_Data");
    #pragma HLS STREAM     variable=ssICaToIId_Data     depth=1024 // Must hold IP header for checksum checking
    static stream<Ip4Version>       ssICaToIId_IpVer    ("ssICaToIId_IpVer");
    #pragma HLS STREAM     variable=ssICaToIId_IpVer    depth=32
    static stream<CmdBit>           ssICaToIId_DropFrag ("ssICaToIId_DropFrag");
    #pragma HLS STREAM     variable=ssICaToIId_DropFrag depth=32
    static stream<SubSums>          ssICaToICc_SubSums  ("ssICaToICc_SubSums");
    #pragma HLS DATA_PACK  variable=ssICaToICc_SubSums
    #pragma HLS STREAM     variable=ssICaToICc_SubSums  depth=32

    //-- IPv4 Invalid Dropper (IId)
    static stream<AxiWord>          ssIIdToICl_Data     ("ssIIdToICl_Data");
    #pragma HLS DATA_PACK  variable=ssIIdToICl_Data
    #pragma HLS STREAM     variable=ssIIdToICl_Data     depth=32

    //-- IPv4 Cut Length (ICl)
    static stream<AxiWord>          ssIClToIPr_Data     ("ssIClToIPr_Data");
    #pragma HLS DATA_PACK  variable=ssIClToIPr_Data
    #pragma HLS STREAM     variable=ssIClToIPr_Data     depth=32

    //-- IPv4 Checksum Checker
    static stream<ValBit>           ssICcToIId_CsumVal  ("ssICcToIId_CsumVal");
    #pragma HLS STREAM     variable=ssICcToIId_CsumVal  depth=32

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    pInputBuffer(
            siETH_Data,
            ssIBufToMPd_Data);

    pMacProtocolDetector(
            piMMIO_MacAddress,
            ssIBufToMPd_Data,
            soARP_Data,
            ssMPdToILc_Data);

    pIpLengthChecker(
            ssMPdToILc_Data,
            ssILcToICa_Data);

    pIpChecksumAccumulator(
            piMMIO_Ip4Address,
            ssILcToICa_Data,
            ssICaToIId_Data,
            ssICaToIId_IpVer,
            ssICaToIId_DropFrag,
            ssICaToICc_SubSums);

    pIpChecksumChecker(
            ssICaToICc_SubSums,
            ssICcToIId_CsumVal);

    pIpInvalidDropper(
            ssICaToIId_Data,
            ssICaToIId_IpVer,
            ssICaToIId_DropFrag,
            ssICcToIId_CsumVal,
            ssIIdToICl_Data);

    pIpCutLength(
            ssIIdToICl_Data,
            ssIClToIPr_Data);

    pIpPacketRouter(
            ssIClToIPr_Data,
            soICMP_Data,
            soICMP_Derr,
            soUDP_Data,
            soTCP_Data);

}

