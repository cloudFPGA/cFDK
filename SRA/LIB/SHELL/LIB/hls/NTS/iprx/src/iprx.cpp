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
 * @file       : iprx.cpp
 * @brief      : IP Receiver packet handler (IPRX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_IPRX
 * \{
 *******************************************************************************/

#include "iprx.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_MPD | TRACE_IBUF)
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

#define DEBUG_LEVEL (TRACE_OFF)


/*******************************************************************************
 * Input Buffer (IBuf)
 *
 * @param[in]  siETH_Data  Data steam from the ETHernet interface (ETH).
 * @param[out] soMPd_Data  Data stream to MAC Protocol Detector (MPd).
 *
 * @details
 *  This process enqueues the incoming data traffic into a FiFo.
 *    [TOOD-FIXME - Do we really need this buffer? Can we get ride of it?].
 *
 *******************************************************************************/
void pInputBuffer(
        stream<AxisEth>     &siETH_Data,
        stream<AxisEth>     &soMPd_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IBuf");

    if (!siETH_Data.empty() && !soMPd_Data.full()) {
        AxisEth  axisEth;
        siETH_Data.read(axisEth);
        if (not axisEth.isValid()) {
            if (DEBUG_LEVEL & TRACE_IBUF) {
                 printWarn(myName, "Received an AxisChunk with an unexpected \'tkeep\' or \'tlast\' value.\n");
                 printAxisRaw(myName, "Aborting the frame after: ", AxisRaw(axisEth.getLE_TData(), axisEth.getLE_TKeep(), axisEth.getLE_TLast()));
            }
            soMPd_Data.write(AxisEth(axisEth.getLE_TData(), 0x00, 1));
        }
        else {
            soMPd_Data.write(axisEth);
        }
    }
}

/*******************************************************************************
 * MAC Protocol Detector (MPd)
 *
 * @param[in]  piMMIO_MacAddr The MAC address from MMIO.
 * @param[in]  siIBuf_Data    Data steam from the Input Buffer (IBuf).
 * @param[out] soARP_Data     Data stream to ARP.
 * @param[out] soILc_Data     Data stream to the IPv4 Length Checker (ILc).
 *
 * @details
 *  This process parses the Ethernet header frame to detect embedded ARP and
 *  IPv4 packets. These two types of packets are forwarded accordingly, while
 *  other types of packets are dropped.
 *******************************************************************************/
void pMacProtocolDetector(
        EthAddr              piMMIO_MacAddr,
        stream<AxisEth>     &siIBuf_Data,
        stream<AxisArp>     &soARP_Data,
        stream<AxisEth>     &soILc_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "MPd");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { S0=0, S1 } mpd_fsmState=S0;
    #pragma HLS RESET         variable=mpd_fsmState
    static ap_uint<2>                  mpd_chunkCount=0;
    #pragma HLS RESET         variable=mpd_chunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static EtherType    mpd_etherType;
    static AxisEth      mpd_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth    currChunk;

    switch (mpd_fsmState) {
    case S0:
        if (!siIBuf_Data.empty() && !soARP_Data.full() && !soILc_Data.full()) {
            siIBuf_Data.read(currChunk);
            switch (mpd_chunkCount) {
            case 0:
                // Compare MAC_DA with our MMIO_MacAddress and with BROADCAST
                //  [TODO - Is this really needed given ARP ?]
                if (currChunk.getEthDstAddr() != piMMIO_MacAddr &&
                    currChunk.getEthDstAddr() != 0xFFFFFFFFFFFF) {
                    mpd_etherType = CMD_DROP;
                    printInfo(myName, "Requesting current frame with MAC destination address = 0x%16.16lX (0x%16.16lX) to be dropped\n",
                                      currChunk.getEthDstAddr().to_long(), currChunk.getLE_EthDstAddr().to_long());
                }
                else {
                    mpd_etherType = CMD_KEEP;
                }
                mpd_chunkCount++;
                break;
            default:
                if (mpd_chunkCount == 1) {
                    if (mpd_etherType != CMD_DROP) {
                        mpd_etherType = currChunk.getEtherType();
                    }
                    mpd_chunkCount++;
                }
                if (mpd_etherType == ETH_ETHERTYPE_ARP) {
                    soARP_Data.write(mpd_prevChunk);
                }
                else if (mpd_etherType == ETH_ETHERTYPE_IP4) {
                    soILc_Data.write(mpd_prevChunk);
                }
                break;
            }
            mpd_prevChunk = currChunk;
            if (currChunk.getLE_TLast()) {
                mpd_chunkCount = 0;
                mpd_fsmState = S1;
            }
        }
        break;
    case S1:
        if( !soARP_Data.full() && !soILc_Data.full()){
            if (mpd_etherType == ETH_ETHERTYPE_ARP) {
                soARP_Data.write(mpd_prevChunk);
            }
            else if (mpd_etherType == ETH_ETHERTYPE_IP4) {
                soILc_Data.write(mpd_prevChunk);
            }
            mpd_fsmState = S0;
        }
        break;

    } // End of: switch
}

/*******************************************************************************
 * IP Length Checker (ILc)
 *
 * @param[in]  siMPd_Data  Data steam from the MAC Protocol Detector (MPd).
 * @param[out] soICa_Data  Data stream to IP Checksum Accumulator (ICa).
 *
 * @details
 *  This process checks the length of the IPv4 packet and drops it if it is too
 *  long.
 *  [TODO:Remove this process. It makes no sense since we do not support fragmentation.]
 *
 *******************************************************************************/
void pIpLengthChecker(
        stream<AxisEth>     &siMPd_Data,
        stream<AxisEth>     &soICa_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ILc");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static ap_uint<2>          ilc_leftToWrite=0;
    #pragma HLS RESET variable=ilc_leftToWrite
    static enum FsmStates {FSM_IDLE=0, FSM_SIZECHECK, FSM_STREAM} ilc_fsmState=FSM_IDLE;
    #pragma HLS RESET variable=ilc_fsmState
    static ap_uint<2>          ilc_chunkCount=0;
    #pragma HLS RESET variable=ilc_chunkCount
    static ap_uint<1>          ilc_filterPacket=0;
    #pragma HLS RESET variable=ilc_filterPacket

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static ap_shift_reg<AxisEth, 2> chunkBuffer; // A shift reg. holding 2 AxisChunks

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth     tempChunk;
    Ip4TotalLen ip4TotLen;

    if (ilc_leftToWrite == 0) {
        if (!siMPd_Data.empty() && !soICa_Data.full()) {
            AxisEth currChunk = siMPd_Data.read();
            switch (ilc_fsmState) {
            case FSM_IDLE:
                // Read head of shift reg, shift up one, and load current chunk into location 0
                chunkBuffer.shift(currChunk);
                if (ilc_chunkCount == 1) {
                    ilc_fsmState = FSM_SIZECHECK;
                }
                ilc_chunkCount++;
                break;
            case FSM_SIZECHECK:
                // Check the IPv4/TotalLength field ([FIXME - Consider removing.])
                ip4TotLen = currChunk.getIp4TotalLen();
                if (ip4TotLen > MaxDatagramSize) {
                    ilc_filterPacket = 1;
                    printWarn(myName, "Current frame will be discarded because frame is too long!\n");
                }
                else {
                    ilc_filterPacket = 0;
                    if (DEBUG_LEVEL & TRACE_ILC) {
                        printInfo(myName, "Start of IPv4 packet - Total length = %d bytes.\n", ip4TotLen.to_int());
                    }
                }
                // Read head of shift reg, shift up one, and load current chunk into location 0
                tempChunk = chunkBuffer.shift(currChunk);
                if (ilc_filterPacket == 0) {
                    soICa_Data.write(tempChunk);
                }
                ilc_fsmState = FSM_STREAM;
                break;
           case FSM_STREAM:
                tempChunk = chunkBuffer.shift(currChunk);
                if (ilc_filterPacket == 0) {
                    soICa_Data.write(tempChunk);
                }
                break;
            } // End of: switch

            if (currChunk.getLE_TLast()) {
                ilc_chunkCount   = 0;
                ilc_leftToWrite = 2;
                ilc_fsmState    = FSM_IDLE;
            }
       }
    }
    else if (ilc_leftToWrite != 0 && !soICa_Data.full() ) {
        tempChunk = chunkBuffer.shift(AxisEth(0, 0, 0));
        if (ilc_filterPacket == 0) {
            soICa_Data.write(tempChunk);
        }
        ilc_leftToWrite--;
    }
}

/*******************************************************************************
 * IP Checksum Checker (ICa)
 *
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siILc_Data        Data stream from IpLengthChecker (ILc).
 * @param[out] soIId_Data        Data stream to IpInvalidDropper (IId).
 * @param[out] soIId_IpVer       The IP version to [IId].
 * @param[out] soIId_DropCmd     Tell [IId] to drop this IP packet.
 * @param[out] soICc_SubSums     Four sub-checksums to IpChecksumChecker (ICc).
 *
 * @details
 *  This process computes four sub-sums in parallel to later assess the checksum
 *  of the IPv4 header.
 *******************************************************************************/
void pIpChecksumAccumulator(
        Ip4Addr               piMMIO_Ip4Address,
        stream<AxisEth>      &siILc_Data,
        stream<AxisIp4>      &soIId_Data,
        stream<Ip4Version>   &soIId_IpVer,
        stream<CmdBit>       &soIId_DropCmd,
        stream<SubSums>      &soICc_SubSums)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICa");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool                ica_wasLastChunk=false;
    #pragma HLS RESET variable=ica_wasLastChunk
    static ap_uint<3>          ica_chunkCount=0;
    #pragma HLS RESET variable=ica_chunkCount

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static ap_uint<17> ica_ipHdrSums[4];
    static ap_uint<8>  ica_ipHdrLen;
    static LE_Ip4Addr  ica_dstIpAddress;
    static AxisEth     ica_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisEth     currChunk;
    AxisIp4     sendChunk;
    ap_uint<16> ip4FlagsAndFragOff;
    bool        ipAddrMatch;

    currChunk.setLE_TLast(0);
    if (!siILc_Data.empty() && !ica_wasLastChunk && !soIId_IpVer.full() &&
        !soIId_DropCmd.full() && !soICc_SubSums.full() && !soIId_Data.full()) {
        siILc_Data.read(currChunk);

        switch (ica_chunkCount) {
        case 0:
            // Skip because chunk contains MAC_SA[1:0] | MAC_DA[5:0]
            for (int i=0; i<4 ;++i) {
                ica_ipHdrSums[i] = 0;
            }
            ica_chunkCount++;
            break;
        case 1:
            // Retrieve the IP version field
            soIId_IpVer.write(currChunk.getIp4Version());
            // Retrieve the Internet Header Length
            ica_ipHdrLen = currChunk.getIp4HdrLen();
            // Retrieve the first two bytes of the IPv4 header
            ica_ipHdrSums[3].range(15,12) = currChunk.getIp4Version();
            ica_ipHdrSums[3].range(11, 8) = currChunk.getIp4HdrLen();
            ica_ipHdrSums[3].range( 7, 0) = currChunk.getIp4ToS();
            ica_chunkCount++;
            break;
        case 2:
            // Check for fragmentation in this cycle. Need to check the MF flag.
            // If set, then drop. If not set, then check the fragment offset and
            // if it is non-zero then drop.
            ip4FlagsAndFragOff = byteSwap16(currChunk.getLE_TData().range(47, 32));
            if (ip4FlagsAndFragOff.range(12, 0) != 0 || ip4FlagsAndFragOff.bit(13) != 0) {
                soIId_DropCmd.write(CMD_DROP);
                if (ip4FlagsAndFragOff.bit(13) != 0) {
                    printWarn(myName, "The More Fragments (MF) is set but IP fragmentation is not supported (yet).\n");
                }
                if (ip4FlagsAndFragOff.range(12, 0) != 0) {
                    printWarn(myName, "Fragment offset is set but not supported (yet) !!!\n");
                }
            }
            else {
                soIId_DropCmd.write(CMD_KEEP);
            }
            for (int i = 0; i < 4; i++) {
              #pragma HLS unroll
                ica_ipHdrSums[i] += byteSwap16(currChunk.getLE_TData().range(i*16+15, i*16));
                ica_ipHdrSums[i] = (ica_ipHdrSums[i] + (ica_ipHdrSums[i] >> 16)) & 0xFFFF;
            }
            ica_ipHdrLen -= 2;
            ica_chunkCount++;
            break;
        case 3:
            ica_dstIpAddress.range(31, 16) = currChunk.getIp4DstAddrHi();
            for (int i = 0; i < 4; i++) {
              #pragma HLS unroll
                ica_ipHdrSums[i] += byteSwap16(currChunk.getLE_TData().range(i*16+15, i*16));
                ica_ipHdrSums[i] = (ica_ipHdrSums[i] + (ica_ipHdrSums[i] >> 16)) & 0xFFFF;
            }
            ica_ipHdrLen -= 2;
            ica_chunkCount++;
            break;

        default:
            if (ica_chunkCount == 4) {
                ica_dstIpAddress(15, 0) = currChunk.getIp4DstAddrLo();
            }
            switch (ica_ipHdrLen) {
            case 0:
                break;
            case 1:
                // Sum up part0
                ica_ipHdrSums[0] += currChunk.getIp4DstAddrLo();
                ica_ipHdrSums[0] = (ica_ipHdrSums[0] + (ica_ipHdrSums[0] >> 16)) & 0xFFFF;
                ica_ipHdrLen = 0;
                // Assess destination IP address (FYI - IP Broadcast addresses are dropped)
                if (ica_dstIpAddress == piMMIO_Ip4Address) {
                    ipAddrMatch = true;
                }
                else {
                    ipAddrMatch = false;
                }
                soICc_SubSums.write(SubSums(ica_ipHdrSums, ipAddrMatch));
                break;
            case 2:
                // Sum up parts 0-2
                for (int i = 0; i < 4; i++) {
                  #pragma HLS unroll
                    ica_ipHdrSums[i] += byteSwap16(currChunk.getLE_TData().range(i*16+15, i*16));
                    ica_ipHdrSums[i] = (ica_ipHdrSums[i] + (ica_ipHdrSums[i] >> 16)) & 0xFFFF;
                }
                ica_ipHdrLen = 0;
                // Assess destination IP address (FYI - IP Broadcast addresses are dropped)
                if (ica_dstIpAddress == piMMIO_Ip4Address) {
                    // || ica_dstIpAddress == 0xFFFFFFFF) {
                ipAddrMatch = true;
                }
                else {
                    ipAddrMatch = false;
                }
                soICc_SubSums.write(SubSums(ica_ipHdrSums, ipAddrMatch));
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                  #pragma HLS unroll
                    ica_ipHdrSums[i] += byteSwap16(currChunk.getLE_TData().range(i*16+15, i*16));
                    ica_ipHdrSums[i] = (ica_ipHdrSums[i] + (ica_ipHdrSums[i] >> 16)) & 0xFFFF;
                }
                ica_ipHdrLen -= 2;
                break;
            }
            break;
        } // End of: switch(ica_chunkCount)f

        if (ica_chunkCount > 2) {
            // Send AxisIp4 chunks while re-aligning the outgoing chunks
            sendChunk = AxisIp4((currChunk.getLE_TData().range(47, 0), ica_prevChunk.getLE_TData().range(63, 48)),
                                (currChunk.getLE_TKeep().range( 5, 0), ica_prevChunk.getLE_TKeep().range( 7,  6)),
                                (currChunk.getLE_TKeep()[6] == 0));
            soIId_Data.write(sendChunk);
        }
        ica_prevChunk = currChunk;
        if (currChunk.getLE_TLast()) {
            ica_chunkCount = 0;
            ica_wasLastChunk = !sendChunk.getLE_TLast();
        }
    }
    else if(ica_wasLastChunk && !soIId_Data.full()) {
        // Send remaining Chunk;
        sendChunk = AxisIp4(ica_prevChunk.getLE_TData().range(63, 48),
                            ica_prevChunk.getLE_TKeep().range( 7,  6),
                            TLAST);
        soIId_Data.write(sendChunk);
        ica_wasLastChunk = false;
    }
}

/*******************************************************************************
 * IP Invalid Dropper (IId)
 *
 * @param[in]  siICa_Data      Data stream from IpChecksumAccumulator (ICa).
 * @param[in]  siICa_IpVer     The IP version from [ICa].
 * @param[in]  siICa_DropFrag  Drop this IP fragment from [ICA].
 * @param[out] siICc_CsumValid Checksum is valid from IpChecksumChecker (ICc).
 * @param[out] soICl_Data      Data stream to IP Cut Length (ICl).
 *
 * @details
 *  Drops an IP packet when its version is not '4' (siICa_IpVer), when it is a
 *   fragmented packet (siICa_DropFrag) or when its IP header checksum is not
 *   valid (siICc_CsumValid). Otherwise, the IPv4 packet is passed on.
 *******************************************************************************/
void pIpInvalidDropper(
        stream<AxisIp4>      &siICa_Data,
        stream<Ip4Version>   &siICa_IpVer,
        stream<ValBit>       &siICa_DropFrag,
        stream<ValBit>       &siICc_CsumValid,
        stream<AxisIp4>      &soICl_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IId");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates {FSM_IDLE=0, FSM_FWD, FSM_DROP} iid_fsmState=FSM_IDLE;
    #pragma HLS RESET                            variable=iid_fsmState

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4 currChunk = AxisIp4(0, 0, 0);

    switch(iid_fsmState) {
    case FSM_IDLE:
        if (!siICc_CsumValid.empty() && !siICa_IpVer.empty() && !siICa_DropFrag.empty() &&
            !siICa_Data.empty() && !soICl_Data.full()) {
            siICa_Data.read(currChunk);
            // Assess validity of the current IPv4 packet
            ValBit csumValid = (siICc_CsumValid.read() == 1);        // CSUM is valid
            ValBit isIpv4Pkt = (siICa_IpVer.read()     == 4);        // IP Version is 4
            ValBit doKeepPkt = (siICa_DropFrag.read()  == CMD_KEEP); // Do not drop
            ap_uint<2> valid = (csumValid + isIpv4Pkt + doKeepPkt);
            if (valid != 3) {
                iid_fsmState = FSM_DROP;
                printWarn(myName, "The current IP packet will be dropped because:\n");
                printWarn(myName, "  csumValid=%d | isIpv4Pkt=%d | doKeepPkt=%d\n",
                          csumValid.to_int(), isIpv4Pkt.to_int(), doKeepPkt.to_int());
            }
            else {
                soICl_Data.write(currChunk);
                iid_fsmState = FSM_FWD;
            }
        }
        break;
    case FSM_FWD:
        if(!siICa_Data.empty() && !soICl_Data.full()) {
            siICa_Data.read(currChunk);
            soICl_Data.write(currChunk);
            if (currChunk.getLE_TLast()) {
                iid_fsmState = FSM_IDLE;
            }
        }
        break;
    case FSM_DROP:
        if(!siICa_Data.empty()){
            siICa_Data.read(currChunk);
            if (currChunk.getLE_TLast()) {
                iid_fsmState = FSM_IDLE;
            }
        }
        break;
    }
}

/*******************************************************************************
 * IPv4 Cut Length (ICl)
 *
 * @param[in]  siIId_Data  Data stream from IpInvalidDropper (IId).
 * @param[out] soIPr_Data  Data stream to IpPacketRouter (IPr).
 *
 * @details
 *  This process intercepts and drops packets which are longer than the
 *   announced 'Ip4TotalLenght' field.
 *******************************************************************************/
void pIpCutLength(
        stream<AxisIp4>  &siIId_Data,
        stream<AxisIp4>  &soIPr_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "ICl");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates {FSM_FWD=0, FSM_DROP} icl_fsmState=FSM_FWD;
    #pragma HLS RESET                  variable=icl_fsmState
    static ap_uint<13>                          icl_chunkCount=0; // Ip4TotalLen/8
    #pragma HLS RESET                  variable=icl_chunkCount

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static Ip4TotalLen  icl_ip4TotalLength;

    switch(icl_fsmState){
    case FSM_FWD:
        if (!siIId_Data.empty() && !soIPr_Data.full()) {
            AxisIp4 currChunk = siIId_Data.read();
            switch (icl_chunkCount) {
            case 0:
                icl_ip4TotalLength = currChunk.getIp4TotalLen();
                break;
            default:
                if (((icl_chunkCount+1)*8) >= icl_ip4TotalLength) {
                    if (currChunk.getLE_TLast() == 0) {
                        icl_fsmState = FSM_DROP;
                    }
                    currChunk.setLE_TLast(TLAST);
                    ap_uint<4> leftLength = icl_ip4TotalLength - (icl_chunkCount*8);
                    currChunk.setLE_TKeep(lenToLE_tKeep(leftLength));
                }
                break;
            } // End of: switch(icl_chunkCount)
            soIPr_Data.write(currChunk);
            icl_chunkCount++;
            if (currChunk.getLE_TLast()) {
                icl_chunkCount = 0;
            }
        }
        break;
    case FSM_DROP:
        if (!siIId_Data.empty()) {
            AxisIp4 currChunk = siIId_Data.read();
            if (currChunk.getLE_TLast()) {
                icl_fsmState = FSM_FWD;
            }
        }
        break;

    } // End of: switch(icl_fsmState)
}

/*******************************************************************************
 * IPv4 Checksum Checker (ICc)
 *
 * @param[in]  siICa_SubSums    Four sub-checksums from IpChecksumAccumulator (ICa).
 * @param[out] soIId_CsumValid  Checksum valid information.
 *
 * @details
 *  This process computes the final IP header checksum by adding the 4 subsums.
 *
 *******************************************************************************/
void pIpChecksumChecker(
        stream<SubSums>   &siICa_SubSums,
        stream<ValBit>    &soIId_CsumValid)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
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

/*******************************************************************************
 * IPv4 Packet Router (IPr)
 *
 * @param[in]  siICl_Data  Data stream from IpCutLength (ICl).
 * @param[out] soICMP_Data ICMP/IP data stream to ICMP.
 * @param[out] soICMP_Derr Erroneous IP data stream to ICMP.
 * @param[out] soUOE_Data  UDP/IP data stream to UDP offload engine (UOE).
 * @param[out] soTOE_Data  TCP/IP data stream to TCP offload engine (TOE).
 *
 * @details
 *  This process routes the IPv4 packets to one of the 3 following engines:
 *  ICMP, TCP or UDP.
 *  If the TTL of the incoming IPv4 packet has expired, the packet is routed
 *  to the ICMP (over the 'soICMP_Derr' stream) in order for the ICMP engine
 *  to build the error messages which data section must include a copy of the
 *  erroneous IPv4 header plus at least the first eight bytes of data from the
 *  IPv4 packet that caused the error message.
 *
 *******************************************************************************/
void pIpPacketRouter(
        stream<AxisIp4>     &siICl_Data,
        stream<AxisIp4>     &soICMP_Data,
        stream<AxisIp4>     &soICMP_Derr,
        stream<AxisIp4>     &soUOE_Data,
        stream<AxisIp4>     &soTOE_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 enable_flush

    const char *myName  = concat3(THIS_NAME, "/", "IPr");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { FSM_IDLE=0, FSM_LAST} ipr_fsmState=FSM_IDLE;
    #pragma HLS RESET                    variable=ipr_fsmState
    static ap_uint<2>                             ipr_chunkCount=0;
    #pragma HLS RESET                    variable=ipr_chunkCount
    static bool                                   ipr_leftToWrite=false;
    #pragma HLS RESET                    variable=ipr_leftToWrite

    //-- STATIC DATAFLOW VARIABLES ---------------------------------------------
    static StsBit       ipr_ttlExpired;
    static Ip4Prot      ipr_ipProtocol;
    static AxisIp4      ipr_prevChunk;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4             currChunk;

     switch (ipr_fsmState) {
     case FSM_IDLE:
         if (!siICl_Data.empty() &&
             !soICMP_Derr.full() && !soICMP_Data.full() &&
             !soUOE_Data.full()  && !soTOE_Data.full()) {
             siICl_Data.read(currChunk);
             switch (ipr_chunkCount) {
                case 0:
                    ipr_chunkCount++;
                    break;
                default:
                    if (ipr_chunkCount == 1) {
                        if (currChunk.getIp4TtL() == 1) {
                            ipr_ttlExpired = 1;
                        }
                        else {
                            ipr_ttlExpired = 0;
                        }
                        ipr_ipProtocol = currChunk.getIp4Prot();
                        ipr_chunkCount++;
                    }
                    if (ipr_ttlExpired == 1) {
                        // Forward the current IP packet to ICMP for building the error message
                        soICMP_Derr.write(ipr_prevChunk);
                    }
                    else {
                        switch (ipr_ipProtocol) {
                        // FYI - There is no default case. If the current packet
                        //  does not match any case, it is automatically dropped.
                        case IP4_PROT_ICMP:
                            soICMP_Data.write(ipr_prevChunk);
                            break;
                        case IP4_PROT_UDP:
                            soUOE_Data.write(ipr_prevChunk);
                            break;
                        case IP4_PROT_TCP:
                            soTOE_Data.write(ipr_prevChunk);
                            break;
                        }
                    }
                    break;
                }
                ipr_prevChunk = currChunk;
                if (currChunk.getLE_TLast()) {
                    ipr_chunkCount = 0;
                    ipr_leftToWrite = true;
                    ipr_fsmState = FSM_LAST;
                }
            }
     break;
     case FSM_LAST:
         if (!soICMP_Derr.full() &&
             !soICMP_Data.full() && !soUOE_Data.full() && !soTOE_Data.full() ) {
            ap_uint<log2Ceil<8>::val> bitCounter = 0;
            bitCounter = ipr_prevChunk.getLen();
            if (ipr_prevChunk.getLE_TKeep() != 0xFF) {
                ipr_prevChunk.setLE_TData(0, 63, 64-((8-bitCounter.to_int())*8));
            }
            if (ipr_ttlExpired == 1) {
                soICMP_Derr.write(ipr_prevChunk);
            }
            else {
                switch (ipr_ipProtocol) {
                case IP4_PROT_ICMP:
                    soICMP_Data.write(ipr_prevChunk);
                    break;
                case IP4_PROT_UDP:
                    soUOE_Data.write(ipr_prevChunk);
                    break;
                case IP4_PROT_TCP:
                    soTOE_Data.write(ipr_prevChunk);
                    break;
                }
            }
            ipr_leftToWrite = false;
            ipr_fsmState = FSM_IDLE;
         }
         break;
     }
}


/*******************************************************************************
 * @brief   Main process of the IP Receive handler (IPRX).
 *
 * @param[in]  piMMIO_MacAddress The MAC address from MMIO (in network order).
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siETH_Data        Data stream from ETHernet MAC layer (ETH).
 * @param[out] soARP_Data        Data stream to Address Resolution Protocol (ARP) server.
 * @param[out] soICMP_Data       Data stream to Internet Control Message Protocol (ICMP) engine.
 * @param[out] soICMP_Derr       Data stream in error to [ICMP].
 * @param[out] soUOE_Data        Data stream to UDP Offload Engine (UOE).
 * @param[out] soTOE_Data        Data stream to TCP Offload Engine (TOE).
 *
 * @details: This process connects to the Rx side of the Ethernet MAC core.
 *   It extracts the IPv4 and ARP packets from the incoming Ethernet frame and
 *   forwards them the TCP-offload-engine (TOE), the Internet Control Message
 *   Protocol (ICMP) engine, the UDP Offload Engine (UOE) or the Address
 *   Resolution Protocol (ARP) server.
 *
 * @note:
 *  ** The data-error stream is used instead of the data stream when the TTL
 *     field of the incoming IPv4 packet is found the be zero. In such a case,
 *     the erroneous packet is forwarded to the ICMP module over the data-error
 *     stream. The ICMP module uses this information to build an ICMP error
 *     datagram (of type #11 --Time Exceeded) that is sent back to the sender.
 *
 *******************************************************************************/
void iprx(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxisEth>     &siETH_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxisArp>     &soARP_Data,
        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &soICMP_Data,
        stream<AxisIp4>     &soICMP_Derr,
        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soUOE_Data,
        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soTOE_Data)
{
    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW
    #pragma HLS INLINE
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //--------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //--------------------------------------------------------------------------

    //-- Input Buffer (IBuf)
    static stream<AxisEth>          ssIBufToMPd_Data    ("ssIBufToMPd_Data");
    #pragma HLS STREAM     variable=ssIBufToMPd_Data    depth=8192

    //-- MAC Protocol Detector (MPd)
    static stream<AxisEth>          ssMPdToILc_Data     ("ssMPdToILc_Data");
    #pragma HLS STREAM     variable=ssMPdToILc_Data     depth=32

    //-- IPv4 Length Checker (ILc)
    static stream<AxisEth>          ssILcToICa_Data     ("ssILcToICa_Data");
    #pragma HLS STREAM     variable=ssILcToICa_Data     depth=32

    //-- IPv4 Checksum Accumulator (ICa)
    static stream<AxisIp4>          ssICaToIId_Data     ("ssICaToIId_Data");
    #pragma HLS STREAM     variable=ssICaToIId_Data     depth=1024 // Must hold IP header for checksum checking
    static stream<Ip4Version>       ssICaToIId_IpVer    ("ssICaToIId_IpVer");
    #pragma HLS STREAM     variable=ssICaToIId_IpVer    depth=32
    static stream<CmdBit>           ssICaToIId_DropFrag ("ssICaToIId_DropFrag");
    #pragma HLS STREAM     variable=ssICaToIId_DropFrag depth=32
    static stream<SubSums>          ssICaToICc_SubSums  ("ssICaToICc_SubSums");
    #pragma HLS DATA_PACK  variable=ssICaToICc_SubSums
    #pragma HLS STREAM     variable=ssICaToICc_SubSums  depth=32

    //-- IPv4 Invalid Dropper (IId)
    static stream<AxisIp4>          ssIIdToICl_Data     ("ssIIdToICl_Data");
    #pragma HLS DATA_PACK  variable=ssIIdToICl_Data
    #pragma HLS STREAM     variable=ssIIdToICl_Data     depth=32

    //-- IPv4 Cut Length (ICl)
    static stream<AxisIp4>          ssIClToIPr_Data     ("ssIClToIPr_Data");
    #pragma HLS DATA_PACK  variable=ssIClToIPr_Data
    #pragma HLS STREAM     variable=ssIClToIPr_Data     depth=32

    //-- IPv4 Checksum Checker
    static stream<ValBit>           ssICcToIId_CsumVal  ("ssICcToIId_CsumVal");
    #pragma HLS STREAM     variable=ssICcToIId_CsumVal  depth=32

    //-- PROCESS FUNCTIONS -----------------------------------------------------

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
            soUOE_Data,
            soTOE_Data);

}

/*******************************************************************************
 * @brief Top of IP Receive handler (IPRX)
 *
 * @param[in]  piMMIO_MacAddress The MAC address from MMIO (in network order).
 * @param[in]  piMMIO_Ip4Address The IPv4 address from MMIO (in network order).
 * @param[in]  siETH_Data        Data stream from ETHernet MAC layer (ETH).
 * @param[out] soARP_Data        Data stream to Address Resolution Protocol (ARP) server.
 * @param[out] soICMP_Data       Data stream to Internet Control Message Protocol (ICMP) engine.
 * @param[out] soICMP_Derr       Data stream in error to [ICMP].
 * @param[out] soUOE_Data        Data stream to UDP Offload Engine (UOE).
 * @param[out] soTOE_Data        Data stream to TCP Offload Engine (TOE).
 *
 *******************************************************************************/
#if HLS_VERSION == 2017
    void iprx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxisEth>     &siETH_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxisArp>     &soARP_Data,
        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxisIp4>     &soICMP_Data,
        stream<AxisIp4>     &soICMP_Derr,
        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soUOE_Data,
        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxisIp4>     &soTOE_Data)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/
    #pragma HLS INTERFACE ap_stable register  port=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable register  port=piMMIO_Ip4Address

    #pragma HLS RESOURCE core=AXI4Stream variable=siETH_Data  metadata="-bus_bundle siETH_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=soARP_Data  metadata="-bus_bundle soARP_Data"

    #pragma HLS RESOURCE core=AXI4Stream variable=soICMP_Data metadata="-bus_bundle soICMP_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soICMP_Derr metadata="-bus_bundle soICMP_Derr"

    #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_Data  metadata="-bus_bundle soUOE_Data"
    #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_Data  metadata="-bus_bundle soTOE_Data"

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW

    //-- MAIN IPRX PROCESS -----------------------------------------------------
    iprx(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_Ip4Address,
        //-- ETHernet MAC Layer Interface
        siETH_Data,
        //-- ARP Interface
        soARP_Data,
        //-- ICMP Interfaces
        soICMP_Data,
        soICMP_Derr,
        //-- UDP Interface
        soUOE_Data,
        //-- TOE Interface
        soTOE_Data);

}
#else
    void iprx_top(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        EthAddr              piMMIO_MacAddress,
        Ip4Addr              piMMIO_Ip4Address,
        //------------------------------------------------------
        //-- ETHernet MAC Layer Interface
        //------------------------------------------------------
        stream<AxisRaw>     &siETH_Data,
        //------------------------------------------------------
        //-- ARP Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soARP_Data,
        //------------------------------------------------------
        //-- ICMP Interfaces
        //------------------------------------------------------
        stream<AxisRaw>     &soICMP_Data,
        stream<AxisRaw>     &soICMP_Derr,
        //------------------------------------------------------
        //-- UOE Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soUOE_Data,
        //------------------------------------------------------
        //-- TOE Interface
        //------------------------------------------------------
        stream<AxisRaw>     &soTOE_Data)
{

    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE ap_stable             port=piMMIO_MacAddress name=piMMIO_MacAddress
    #pragma HLS INTERFACE ap_stable             port=piMMIO_Ip4Address name=piMMIO_Ip4Address

    #pragma HLS INTERFACE axis off              port=siETH_Data        name=siETH_Data

    #pragma HLS INTERFACE axis off              port=soARP_Data        name=soARP_Data

    #pragma HLS INTERFACE axis off              port=soICMP_Data       name=soICMP_Data
    #pragma HLS INTERFACE axis register both    port=soICMP_Derr       name=soICMP_Derr

    #pragma HLS INTERFACE axis off              port=soUOE_Data        name=soUOE_Data
    #pragma HLS INTERFACE axis off              port=soTOE_Data        name=soTOE_Data

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW disable_start_propagation

    //-- LOCAL INPUT and OUTPUT STREAMS ----------------------------------------
    static stream<AxisEth>     ssiETH_Data;
    static stream<AxisArp>     ssoARP_Data;
    static stream<AxisIp4>     ssoICMP_Data;
    static stream<AxisIp4>     ssoICMP_Derr;
    static stream<AxisIp4>     ssoUOE_Data;
    static stream<AxisIp4>     ssoTOE_Data;

    //-- INPUT STREAM CASTING --------------------------------------------------
    pAxisRawCast(siETH_Data, ssiETH_Data);

    //-- MAIN IPRX PROCESS -----------------------------------------------------
    iprx(
        //-- MMIO Interfaces
        piMMIO_MacAddress,
        piMMIO_Ip4Address,
        //-- ETHernet MAC Layer Interface
        ssiETH_Data,
        //-- ARP Interface
        ssoARP_Data,
        //-- ICMP Interfaces
        ssoICMP_Data,
        ssoICMP_Derr,
        //-- UDP Interface
        ssoUOE_Data,
        //-- TOE Interface
        ssoTOE_Data);

    //-- OUTPUT STREAM CASTING -------------------------------------------------
    pAxisRawCast(ssoARP_Data,  soARP_Data);
    pAxisRawCast(ssoICMP_Data, soICMP_Data);
    pAxisRawCast(ssoICMP_Derr, soICMP_Derr);
    pAxisRawCast(ssoUOE_Data,  soUOE_Data);
    pAxisRawCast(ssoTOE_Data,  soTOE_Data);

}
#endif  //  HLS_VERSION

/*! \} */
