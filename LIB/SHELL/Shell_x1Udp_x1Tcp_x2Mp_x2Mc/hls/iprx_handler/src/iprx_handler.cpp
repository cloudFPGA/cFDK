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
 *----------------------------------------------------------------------------
 *
 * @details    : This process connects to the Ethernet MAC core. It extracts
 *  the IP frame from the Ethernet packet and forwards that frame to the
 *  TCP-offload-engine (TOE), the Internet Control Message Protocol (ICMP)
 *  engine, the UDP engine or the Address Resolution Protocol (ARP) server.
 *
 * @note       :
 * @remark     : { remark text }
 * @warning    :
 * @todo       : Why do we need an IP input buffer? Can we get ride of it?
 *
 * @see        : https://www.stack.nl/~dimitri/doxygen/manual/commands.html
 *
 *****************************************************************************/

#include "iprx_handler.hpp"

using namespace hls;

#define USE_DEPRECATED_DIRECTIVES


ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8));
}

ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8), inputVector.range(23,16), inputVector(31, 24));
}



ap_uint<8> returnKeep(ap_uint<4> length)
{
    ap_uint<8> keep = 0;

    switch(length){
            case 1: keep = 0x01; break;
            case 2: keep = 0x03; break;
            case 3: keep = 0x07; break;
            case 4: keep = 0x0F; break;
            case 5: keep = 0x1F; break;
            case 6: keep = 0x3F; break;
            case 7: keep = 0x7F; break;
            case 8: keep = 0xFF; break;
        }

    return keep;
}

ap_uint<4> keepMapping(ap_uint<8> keepValue)        // This function counts the number of 1s in an 8-bit value
{
    ap_uint<4> counter = 0;

    switch(keepValue){
        case 0x01: counter = 1; break;
        case 0x03: counter = 2; break;
        case 0x07: counter = 3; break;
        case 0x0F: counter = 4; break;
        case 0x1F: counter = 5; break;
        case 0x3F: counter = 6; break;
        case 0x7F: counter = 7; break;
        case 0xFF: counter = 8; break;
    }
    return counter;
}



void detect_mac_protocol(     stream<axiWord> &dataIn,
                            stream<axiWord> &ARPdataOut,
                            stream<axiWord> &IPdataOut,
                            ap_uint<48> myMacAddress)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<1> dmp_fsmState = 0;
    static ap_uint<2> dmp_wordCount = 0;
    static ap_uint<16> dmp_macType;
    static axiWord dmp_prevWord;
    axiWord currWord;

    switch (dmp_fsmState) {
    case 0:
        if (!dataIn.empty() && !ARPdataOut.full() && !IPdataOut.full()) {
            dataIn.read(currWord);
            switch (dmp_wordCount) {
            case 0:
                if (currWord.data.range(47, 0) != myMacAddress && currWord.data.range(47, 0) !=0xFFFFFFFFFFFF) // Verify that this node is the destination of this Ethenet frame
                    dmp_macType = DROP;
                else
                    dmp_macType = FORWARD;

                dmp_wordCount++;
            break;
            default:
                if (dmp_wordCount == 1) {
                    if (dmp_macType != DROP)
                        dmp_macType = byteSwap16(currWord.data(47, 32));
                    dmp_wordCount++;
                }
                if (dmp_macType == ARP)
                    ARPdataOut.write(dmp_prevWord);
                else if (dmp_macType == IPv4)
                    IPdataOut.write(dmp_prevWord);
                break;
            }
            dmp_prevWord = currWord;
            if (currWord.last) {
                dmp_wordCount = 0;
                dmp_fsmState = 1;
            }
        }
    break;
    case 1:
        if( !ARPdataOut.full() && !IPdataOut.full()){
            if (dmp_macType == ARP)
                ARPdataOut.write(dmp_prevWord);
            else if (dmp_macType == IPv4)
                IPdataOut.write(dmp_prevWord);
            dmp_fsmState = 0;
        }
    break;
    } //switch
}




void length_check(stream<axiWord> &lengthCheckIn, stream<axiWord> &ipDataFifo)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

   static enum lcState {LC_IDLE = 0, LC_SIZECHECK, LC_STREAM} lengthCheckState;
   static ap_uint<2> dip_wordCount      = 0;
   static ap_uint<2> dip_leftToWrite    = 0;
   static ap_uint<1> filterPacket   = 0;
   static ap_shift_reg<axiWord, 2> wordBuffer;
   axiWord temp;

   if (dip_leftToWrite == 0) {
            if (!lengthCheckIn.empty() && !ipDataFifo.full()) {
                axiWord currWord = lengthCheckIn.read();
            switch (lengthCheckState) {
                case LC_IDLE:
                    wordBuffer.shift(currWord);
                    if (dip_wordCount == 1)
                        lengthCheckState = LC_SIZECHECK;
                    dip_wordCount++;
                    break;
                case LC_SIZECHECK:
                    byteSwap16(currWord.data.range(15, 0)) > MaxDatagramSize ? filterPacket = 1 : filterPacket = 0;
                    temp = wordBuffer.shift(currWord);
                    if (filterPacket == 0)
                        ipDataFifo.write(temp);
                    lengthCheckState = LC_STREAM;
                    break;
                case LC_STREAM:
                    temp = wordBuffer.shift(currWord);
                    if (filterPacket == 0)
                        ipDataFifo.write(temp);
                    break;
             }
             if (currWord.last) {
                dip_wordCount   = 0;
                dip_leftToWrite = 2;
                lengthCheckState    = LC_IDLE;
             }
            }
   } else if (dip_leftToWrite != 0 && !ipDataFifo.full() ) {
      //axiWord nullAxiWord = {0, 0, 0};
      temp = wordBuffer.shift(axiWord(0, 0, 0));
      if (filterPacket == 0)
          ipDataFifo.write(temp);
      dip_leftToWrite--;
   }
}





void check_ip_checksum( stream<axiWord>&      dataIn,
                        ap_uint<32>                 myIpAddress,
                        stream<axiWord>&        dataOut,
                        stream<subSums>&        iph_subSumsFifoIn,
                        stream<ap_uint<1> >&    ipValidFifoVersionNo,
                        stream<ap_uint<1> >&    ipFragmentDrop)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
    static ap_uint<8> cics_ipHeaderLen = 0;
    static axiWord cics_prevWord;
    static bool cics_wasLast = false;
    static ap_uint<3> cics_wordCount = 0;
    static ap_uint<32> cics_dstIpAddress = 0;

    axiWord currWord;
    axiWord sendWord;
    ap_uint<16> temp;
    bool ipMatch    = false;

    currWord.last = 0;
    if (!dataIn.empty() && !cics_wasLast && !ipValidFifoVersionNo.full() && !ipFragmentDrop.full() && !iph_subSumsFifoIn.full() && !dataOut.full()) {
        dataIn.read(currWord);

        switch (cics_wordCount) {
        case 0:
            // This is MAC only we throw it out
            for (uint8_t i=0;i<4;++i)
                cics_ip_sums[i] = 0;
            cics_wordCount++;
            break;
        case 1:
            if (currWord.data.range(55, 52) != 4)   // Verify that the IP protocol version number is what it should be
                ipValidFifoVersionNo.write(0);      // If not, drop the packet
            else
                ipValidFifoVersionNo.write(1);

            cics_ip_sums[3] += byteSwap16(currWord.data.range(63, 48));
            cics_ip_sums[3] = (cics_ip_sums[3] + (cics_ip_sums[3] >> 16)) & 0xFFFF;
            cics_ipHeaderLen = currWord.data.range(51, 48);

            cics_wordCount++;
            break;
        case 2: // Check for fragmentation in this cycle. Need to check the MF flag. If that's set then drop. If it's not set then check the fragment offset and if its non-zero then drop.
        {
            ap_uint<16> temp = byteSwap16(currWord.data.range(47, 32));

            if (temp.range(12, 0) != 0 || temp.bit(13) != 0)
                ipFragmentDrop.write(0);
            else
                ipFragmentDrop.write(1);

            for (int i = 0; i < 4; i++)     {
            #pragma HLS unroll
                cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
            }
            cics_ipHeaderLen -= 2;
            cics_wordCount++;
            break;
        }
        case 3: //maybe merge with WORD_2
            //cics_srcMacIpTuple.ipAddress = currWord.data(47, 16);
            cics_dstIpAddress.range(15, 0) = currWord.data.range(63, 48);

            for (int i = 0; i < 4; i++) {
            #pragma HLS unroll
                cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
            }

            // write tcp len out
            cics_ipHeaderLen -= 2;
            cics_wordCount++;
            break;
        default:
            if (cics_wordCount == 4)
                cics_dstIpAddress(31, 16) = currWord.data(15, 0);

            switch (cics_ipHeaderLen) {
            case 0:
                break;
            case 1:
                // Sum up part0
                cics_ip_sums[0] += byteSwap16(currWord.data.range(15, 0));
                cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
                cics_ipHeaderLen = 0;

                if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                    ipMatch = true;

                iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                break;
            case 2:
                // Sum up part 0-2
                for (int i = 0; i < 4; i++) {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                cics_ipHeaderLen = 0;

                if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                    ipMatch = true;

                iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                cics_ipHeaderLen -= 2;
                break;
            }
            break;
        } // switch WORD_N

        if (cics_wordCount > 2) {
            // Send Word
            sendWord = axiWord((currWord.data(47, 0), cics_prevWord.data(63, 48)), (currWord.keep(5, 0), cics_prevWord.keep(7,6)), (currWord.keep[6] == 0));
            dataOut.write(sendWord);
        }

        cics_prevWord = currWord;
        if (currWord.last) {
            cics_wordCount = 0;
            cics_wasLast = !sendWord.last;
            //cics_wasLast = true;
        }
    } else if(cics_wasLast && !dataOut.full()) {
        // Send remaining Word;
        sendWord = axiWord(cics_prevWord.data.range(63, 48), cics_prevWord.keep.range(7, 6), 1);
        dataOut.write(sendWord);
        cics_wasLast = false;
    }
}


void check_ip_checksum_2(   stream<axiWord>&        dataIn,
                            ap_uint<32>                 myIpAddress,
                            stream<axiWord>&        dataOut,
                            stream<subSums>&        iph_subSumsFifoIn,
                            stream<ap_uint<1> >&    ipValidFifoVersionNo,
                            stream<ap_uint<1> >&    ipFragmentDrop)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
    static ap_uint<8> cics_ipHeaderLen = 0;
    static axiWord cics_prevWord;
    static bool cics_wasLast = false;
    static ap_uint<3> cics_wordCount = 0;
    static ap_uint<32> cics_dstIpAddress = 0;

    axiWord currWord;
    axiWord sendWord;
    ap_uint<16> temp;
    bool ipMatch    = false;

    static enum check_ip_checksum_engine {CICES_IDLE = 0, CICES_WORD_1, CICES_WORD_2, CICES_WORD_3, CICES_WORD_DEF, CICES_LAST} check_ip_checksum_engine_state;

    switch (check_ip_checksum_engine_state) {
        case CICES_IDLE:
            if (!dataIn.empty()) {
                dataIn.read(currWord);
                cics_prevWord = currWord;
                cics_ip_sums[0] = cics_ip_sums[1] = cics_ip_sums[2] = cics_ip_sums[3] = 0;
                cics_wordCount++;
                check_ip_checksum_engine_state = CICES_WORD_1;
            }
        break;
        case CICES_WORD_1:
            if (!dataIn.empty() && !ipValidFifoVersionNo.full()) {
                dataIn.read(currWord);
                cics_prevWord = currWord;

                if (currWord.data.range(55, 52) != 4)   // Verify that the IP protocol version number is what it should be
                    ipValidFifoVersionNo.write(0);      // If not, drop the packet
                else
                    ipValidFifoVersionNo.write(1);

                cics_ip_sums[3]    += byteSwap16(currWord.data.range(63, 48));
                cics_ip_sums[3]     = (cics_ip_sums[3] + (cics_ip_sums[3] >> 16)) & 0xFFFF;
                cics_ipHeaderLen    = currWord.data.range(51, 48);
                cics_wordCount++;

                check_ip_checksum_engine_state = CICES_WORD_2;
            }
        break;
        case CICES_WORD_2:
            if (!dataIn.empty() && !ipFragmentDrop.full()) {
                dataIn.read(currWord);
                cics_prevWord = currWord;

                ap_uint<16> temp = byteSwap16(currWord.data.range(47, 32));
                if (temp.range(12, 0) != 0 || temp.bit(13) != 0)
                    ipFragmentDrop.write(0);
                else
                    ipFragmentDrop.write(1);

                for (int i = 0; i < 4; i++)     {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                cics_ipHeaderLen -= 2;
                cics_wordCount++;

                check_ip_checksum_engine_state = CICES_WORD_3;
            }
        break;
        case CICES_WORD_3:
            if (!dataIn.empty() && !dataOut.full()) {
                dataIn.read(currWord);
                cics_prevWord = currWord;

                cics_dstIpAddress.range(15, 0) = currWord.data.range(63, 48);

                for (int i = 0; i < 4; i++) {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                // write tcp len out
                cics_ipHeaderLen -= 2;

                if(cics_wordCount > 2){
                    sendWord = axiWord((currWord.data(47, 0), cics_prevWord.data(63, 48)), (currWord.keep(5, 0), cics_prevWord.keep(7,6)), (currWord.keep[6] == 0));
                    dataOut.write(sendWord);
                }

                if (currWord.last) {
                    //cics_wordCount = 0;

                    cics_wasLast = !sendWord.last;
                    if(cics_wasLast)
                        check_ip_checksum_engine_state = CICES_LAST;
                }


                cics_wordCount++;

                check_ip_checksum_engine_state = CICES_WORD_DEF;
            }
        break;
        case CICES_WORD_DEF:
            if (!dataIn.empty() && !dataOut.full() && !iph_subSumsFifoIn.full()) {
                dataIn.read(currWord);
                cics_prevWord = currWord;

                if (cics_wordCount == 4)
                    cics_dstIpAddress(31, 16) = currWord.data(15, 0);

                    switch (cics_ipHeaderLen) {
                    case 0:
                        break;
                    case 1:
                    // Sum up part0
                        cics_ip_sums[0] += byteSwap16(currWord.data.range(15, 0));
                        cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
                        cics_ipHeaderLen = 0;

                        if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                            ipMatch = true;
                        iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                    break;
                    case 2:
                        // Sum up part 0-2
                        for (int i = 0; i < 4; i++) {
                        #pragma HLS unroll
                            cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                            cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                        }

                        cics_ipHeaderLen = 0;

                        if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                            ipMatch = true;

                        iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                    break;
                    default:
                        // Sum up everything
                        for (int i = 0; i < 4; i++) {
                        #pragma HLS unroll
                            cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                            cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                        }
                        cics_ipHeaderLen -= 2;
                    break;
                }

                if(cics_wordCount > 2){
                    sendWord = axiWord((currWord.data(47, 0), cics_prevWord.data(63, 48)), (currWord.keep(5, 0), cics_prevWord.keep(7,6)), (currWord.keep[6] == 0));
                    dataOut.write(sendWord);
                }

                if (currWord.last) {
                    //cics_wordCount = 0;

                    cics_wasLast = !sendWord.last;
                    if(cics_wasLast)
                        check_ip_checksum_engine_state = CICES_LAST;
                }
            }
        break;
        case CICES_LAST:
            if (!dataOut.full()) {
                sendWord = axiWord(cics_prevWord.data.range(63, 48), cics_prevWord.keep.range(7, 6), 1);
                dataOut.write(sendWord);
                cics_wasLast = false;
                check_ip_checksum_engine_state = CICES_IDLE;
                cics_wordCount = 0;
            }
        break;

    }
}





void ip_invalid_dropper(  stream<axiWord>&        dataIn,
                            stream<ap_uint<1> >&    ipValidFifoIn,
                            // Add a stream here for fragment drop
                            stream<ap_uint<1> >         &ipValidFifoVersionNo,
                            stream<ap_uint<1> >         &ipFragmentDrop,
                            stream<axiWord>&        dataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static bool iid_firstWord = true;
    static bool iid_drop = false;

    static enum ip_invalid_dropper_engine {IIDES_IDLE = 0, IIDES_FWD, IIDES_DROP} ip_invalid_dropper_engine_state;

    axiWord currWord = axiWord(0, 0, 0);


    switch(ip_invalid_dropper_engine_state) {
    case IIDES_IDLE:
        if (!ipValidFifoIn.empty() && !ipValidFifoVersionNo.empty() && !ipFragmentDrop.empty() && !dataIn.empty() && !dataOut.full()) {
            ap_uint<2> valid = ipValidFifoIn.read() + ipValidFifoVersionNo.read() + ipFragmentDrop.read(); // Check the fragment drop queue
            dataIn.read(currWord);

            if (valid != 3)
                ip_invalid_dropper_engine_state = IIDES_DROP;
            else {
                dataOut.write(currWord);
                ip_invalid_dropper_engine_state = IIDES_FWD;
            }
        }
    break;
    case IIDES_FWD:
        if(!dataIn.empty() && !dataOut.full()) {
            dataIn.read(currWord);
            dataOut.write(currWord);

            if (currWord.last == 1)
                ip_invalid_dropper_engine_state = IIDES_IDLE;
        }
    break;
    case IIDES_DROP:
        if(!dataIn.empty()){
            dataIn.read(currWord);
            if (currWord.last == 1)
                ip_invalid_dropper_engine_state = IIDES_IDLE;
        }
    break;
    }
}




void cut_length(  stream<axiWord> &dataIn,
                    stream<axiWord> &dataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static enum cut_length_engine {CLES_IDLE = 0, CLES_LAST} cut_length_engine_state;

    static ap_uint<16> cl_wordCount = 0;
    static ap_uint<16> cl_totalLength = 0;
    static bool cl_drop = false;

    switch(cut_length_engine_state){
    case CLES_IDLE:
        if (!dataIn.empty() && !dataOut.full()) {
                axiWord currWord = dataIn.read();
                switch (cl_wordCount) {
                case 0:
                    cl_totalLength = byteSwap16(currWord.data(31, 16));
                    break;
                default:
                    if (((cl_wordCount+1)*8) >= cl_totalLength) {   //last real world
                        if (currWord.last == 0)
                            cut_length_engine_state = CLES_LAST;
                            //cl_drop = true;
                        currWord.last = 1;
                        ap_uint<4> leftLength = cl_totalLength - (cl_wordCount*8);
                        currWord.keep = returnKeep(leftLength);
                    }
                    break;
                }
                dataOut.write(currWord);
                cl_wordCount++;
                if (currWord.last)
                    cl_wordCount = 0;
            }
    break;
    case CLES_LAST:
        if (!dataIn.empty()) {
            axiWord currWord = dataIn.read();
            if (currWord.last)
                cut_length_engine_state = CLES_IDLE;
                //cl_drop = false;
        }
    break;

    }
}


void iph_check_ip_checksum(     stream<subSums>&        iph_subSumsFifoOut,
                            stream<ap_uint<1> >&    iph_validFifoOut)
{

#pragma HlS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    if (!iph_subSumsFifoOut.empty() && !iph_validFifoOut.full()) {
        subSums icic_ip_sums = iph_subSumsFifoOut.read();
        icic_ip_sums.sum0 += icic_ip_sums.sum2;
        icic_ip_sums.sum1 += icic_ip_sums.sum3;
        icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
        icic_ip_sums.sum1 = (icic_ip_sums.sum1 + (icic_ip_sums.sum1 >> 16)) & 0xFFFF;
        icic_ip_sums.sum0 += icic_ip_sums.sum1;
        icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
        icic_ip_sums.sum0 = ~icic_ip_sums.sum0;
        iph_validFifoOut.write((icic_ip_sums.sum0(15, 0) == 0x0000) && icic_ip_sums.ipMatch);
    }
}





void detect_ip_protocol_1(stream<axiWord>&      dataIn,
                          stream<axiWord>&      ICMPdataOut,
                          stream<axiWord>&      ICMPexpDataOut,
                          stream<axiWord>&      UDPdataOut,
                          stream<axiWord>&      TCPdataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<8>   dip_ipProtocol  = 0;
    static ap_uint<2>   dip_wordCount   = 0;
    static ap_uint<1>   dip_ttlExpired  = 0;
    static axiWord      dip_prevWord;
    static bool         dip_leftToWrite = false;
    axiWord             currWord;

     static enum detect_ip_protocol_engine {DIPES_IDLE = 0, DIPES_LAST} detect_ip_protocol_engine_state;

     switch (detect_ip_protocol_engine_state) {
     case DIPES_IDLE:
         if (!dataIn.empty() && !ICMPexpDataOut.full() && !ICMPdataOut.full() && !UDPdataOut.full() && !TCPdataOut.full()) {
             dataIn.read(currWord);
                switch (dip_wordCount) {
                case 0:
                    dip_wordCount++;
                    break;
                default:
                    if (dip_wordCount == 1) {
                        if (currWord.data.range(7, 0) == 1)
                            dip_ttlExpired = 1;
                        else
                            dip_ttlExpired = 0;
                        dip_ipProtocol = currWord.data.range(15, 8);
                        dip_wordCount++;
                    }
                    // There is not default, if package does not match any case it is automatically dropped
                    if (dip_ttlExpired == 1) {
                        ICMPexpDataOut.write(dip_prevWord);
                    } else {
                        switch (dip_ipProtocol) {
                        case ICMP:
                            ICMPdataOut.write(dip_prevWord);
                            break;
                        case UDP:
                            UDPdataOut.write(dip_prevWord);
                            break;
                        case TCP:
                            TCPdataOut.write(dip_prevWord);
                            break;
                        }
                    }
                    break;
                }
                dip_prevWord = currWord;
                if (currWord.last) {
                    dip_wordCount = 0;
                    dip_leftToWrite = true;
                    detect_ip_protocol_engine_state = DIPES_LAST;
                }
            }
     break;
     case DIPES_LAST:
         if (!ICMPexpDataOut.full() && !ICMPdataOut.full() && !UDPdataOut.full() && !TCPdataOut.full() ) {
                uint8_t bitCounter = 0;
                bitCounter = keepMapping(dip_prevWord.keep);
                /*for (uint8_t i=0;i<8;++i) {
                    if (dip_prevWord.keep.bit(i) == 1)
                        bitCounter++;
                }*/
                if (dip_prevWord.keep != 0xFF)
                    dip_prevWord.data.range(63, 64-((8-bitCounter)*8)) = 0;

                if (dip_ttlExpired == 1) {
                    ICMPexpDataOut.write(dip_prevWord);
                } else {
                    switch (dip_ipProtocol) {
                    case ICMP:
                        ICMPdataOut.write(dip_prevWord);
                        break;
                    case UDP:
                        UDPdataOut.write(dip_prevWord);
                        break;
                    case TCP:
                        TCPdataOut.write(dip_prevWord);
                        break;
                    }
                }
                //dip_leftToWrite = false;
                detect_ip_protocol_engine_state = DIPES_IDLE;
         }
     break;
     }
}







void ip_buffering(stream<axiWord>&      dataIn,
                  stream<axiWord>&      dataOut     )
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    if(!dataIn.empty() && !dataOut.full()){
        dataOut.write(dataIn.read());
    }


}

/*added buffering layer to the refactored version*/

/*****************************************************************************
 * @brief   Main process of the IP Receiver HAndler.
 * @ingroup iprx_handler
 *
 * @param[in]  piMMIO_This_MacAddress, MAC address from MMIO.
 * @param[in]  piMMIO_This_IpAddress,  IPv4 address from MMIO.
 * @param[in]  siETH_This_Data,        Data stream from ETH MAC layer.
 * @param[out] soTHIS_Arp_Data,        Data stream to ARP server.
 * @param[out] soTHIS_Icmp_Data,	   Data stream to ICMP.
 * @param[out] soTHIS_Icmp_Derr,       Data error** stream to ICMP.
 *
 * @note:
 *  ** This data-error stream is used instead of the data stream when the TTL
 *     field of the incoming IPv4 frame is found the be zero. In such a case,
 *     the erroneous frame is forwarded to the ICMP module over the data-error
 *     stream. The ICMP module uses this information to build an ICMP error
 *     datagram (of type #11 --Time Exceeded) that is sent back to the sender.
 *
 * @return Nothing.
 *****************************************************************************/
void iprx_handler(

	    //------------------------------------------------------
	    //-- MMIO Interfaces
	    //------------------------------------------------------
		ap_uint<48>          piMMIO_This_MacAddress,
        ap_uint<32>          piMMIO_This_Ip4Address,


        //------------------------------------------------------
        //-- ETH0 / This / Interface
        //------------------------------------------------------
        stream<axiWord>		&siETH_This_Data,

		//------------------------------------------------------
		//-- ARP Interface
		//------------------------------------------------------
		stream<axiWord>		&soTHIS_Arp_Data,

	    //------------------------------------------------------
	    //-- ICMP Interfaces
	    //------------------------------------------------------
        stream<axiWord>		&soTHIS_Icmp_Data,
        stream<axiWord>		&soTHIS_Icmp_Derr,

	    //------------------------------------------------------
	    //-- UDP Interface
	    //------------------------------------------------------
        stream<axiWord>		&soTHIS_Udp_Data,

		//------------------------------------------------------
		//-- TOE Interface
		//------------------------------------------------------
        stream<axiWord>		&soTHIS_Tcp_Data)

{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
	#pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

	#pragma HLS INTERFACE ap_stable port=piMMIO_This_MacAddress name=piMMIO_This_MacAddress
	#pragma HLS INTERFACE ap_stable port=piMMIO_This_Ip4Address name=piMMIO_This_Ip4Address

    #pragma  HLS resource core=AXI4Stream variable=siETH_This_Data  metadata="-bus_bundle siETH_This_Data"

	#pragma  HLS resource core=AXI4Stream variable=soTHIS_Arp_Data  metadata="-bus_bundle soTHIS_Arp_Data"

	#pragma  HLS resource core=AXI4Stream variable=soTHIS_Icmp_Data metadata="-bus_bundle soTHIS_Icmp_Data"
	#pragma  HLS resource core=AXI4Stream variable=soTHIS_Icmp_Derr metadata="-bus_bundle soTHIS_Icmp_Derr"

	#pragma  HLS resource core=AXI4Stream variable=soTHIS_Udp_Data  metadata="-bus_bundle soTHIS_Udp_Data"
	#pragma  HLS resource core=AXI4Stream variable=soTHIS_Tcp_Data  metadata="-bus_bundle soTHIS_Tcp_Data"

#else

	#pragma HLS INTERFACE ap_none register port=piMMIO_This_MacAddress name=piMMIO_This_MacAddress
	#pragma HLS INTERFACE ap_none register port=piMMIO_This_Ip4Address name=piMMIO_This_Ip4Address

	#pragma HLS INTERFACE port=siETH_This_Data	axis

	#pragma HLS INTERFACE port=soTHIS_Arp_Data	axis

	#pragma HLS INTERFACE port=soTHIS_Icmp_Data axis
	#pragma HLS INTERFACE port=soTHIS_Icmp_Derr axis

	#pragma HLS INTERFACE port=soTHIS_Udp_Data  axis
	#pragma HLS INTERFACE port=soTHIS_Tcp_Data  axis

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW //interval=1

	//-- LOCAL STREAMS --------------------------------------------------------
    static stream<axiWord>          lengthCheckIn	("lengthCheckIn");
    #pragma HLS STREAM     variable=lengthCheckIn   depth=32

    static stream<axiWord>          ipDataFifo		("ipDataFifo");
    #pragma HLS DATA_PACK  variable=ipDataFifo
    #pragma HLS STREAM     variable=ipDataFifo      depth=32

    static stream<axiWord>      	ipDataCheckFifo	("ipDataCheckFifo");
    #pragma HLS DATA_PACK  variable=ipDataCheckFifo
    #pragma HLS STREAM 	   variable=ipDataCheckFifo depth=1024 //must hold IP header for checksum checking

    static stream<axiWord>      	ipDataDropFifo	("ipDataDropFifo");
    #pragma HLS DATA_PACK  variable=ipDataDropFifo
    #pragma HLS STREAM 	   variable=ipDataDropFifo  depth=32

    static stream<axiWord>      	ipDataCutFifo	("ipDataCutFifo");
    #pragma HLS DATA_PACK  variable=ipDataCutFifo
    #pragma HLS STREAM 	   variable=ipDataCutFifo	depth=32

    static stream<subSums>      	iph_subSumsFifoOut("iph_subSumsFifoOut");
    #pragma HLS DATA_PACK  variable=iph_subSumsFifoOut
    #pragma HLS STREAM     variable=iph_subSumsFifoOut	depth=32

    static stream<ap_uint<1> >		ipValidFifo		("ipValidFifo");
    #pragma HLS STREAM     variable=ipValidFifo     depth=32

    static stream<ap_uint<1> >     ipValidFifoVersionNo("ipValidFifoVersionNo");
    #pragma HLS STREAM     variable=ipValidFifoVersionNo	depth=32

    static stream<ap_uint<1> >      ipFragmentDrop("ipFragmentDrop");
    #pragma HLS STREAM     variable=ipFragmentDrop	depth=32

    static stream<axiWord> 			dataToDetectMac("dataToDetectMac");
    #pragma HLS STREAM     variable=dataToDetectMac	depth=8000

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    ip_buffering(
    			siETH_This_Data,  dataToDetectMac);

    detect_mac_protocol(
    			dataToDetectMac, soTHIS_Arp_Data, lengthCheckIn, piMMIO_This_MacAddress);

    length_check(
    			lengthCheckIn, ipDataFifo);

    check_ip_checksum(
    			ipDataFifo, piMMIO_This_Ip4Address, ipDataCheckFifo, iph_subSumsFifoOut, ipValidFifoVersionNo, ipFragmentDrop);

    iph_check_ip_checksum(
    			iph_subSumsFifoOut, ipValidFifo);

    ip_invalid_dropper(
    			ipDataCheckFifo, ipValidFifo, ipValidFifoVersionNo, ipFragmentDrop, ipDataDropFifo);

    cut_length(
    		ipDataDropFifo, ipDataCutFifo);

    detect_ip_protocol_1(
    		ipDataCutFifo, soTHIS_Icmp_Data, soTHIS_Icmp_Derr, soTHIS_Udp_Data, soTHIS_Tcp_Data);

}
































/*********************************************************************************
 *
 * [TODO - REMOVE EVERYTHING BELOW HERE. THIS IS ALL OBSOLETE STUFF FROM JAGATH.
 *
 **********************************************************************************/

#if 0
ap_uint<8> returnKeep(ap_uint<4> length) {
    ap_uint<8> keep = 0;
    for (uint8_t i=0;i<8;++i) {
        if (i < length)
            keep.bit(i) = 1;
    }
    return keep;
}
#endif


#if 0
void iph_check_ip_checksum_0(   stream<subSums>&        iph_subSumsFifoOut,
                                stream<ap_uint<1> >&    iph_validFifoOut)
{

#pragma HlS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<32> csum = 0;
    static subSums icic_ip_sums;
    ap_uint<16> temp_csum = 0;

    static enum iph_check_ip_checksum_engine {ICICES_IDLE = 0, ICICES_CSUM} iph_check_ip_checksum_engine_state;

    switch (iph_check_ip_checksum_engine_state) {
        case ICICES_IDLE:
            if (!iph_subSumsFifoOut.empty()) {
                icic_ip_sums = iph_subSumsFifoOut.read();
                csum = icic_ip_sums.sum0 + icic_ip_sums.sum1 + icic_ip_sums.sum2 + icic_ip_sums.sum3;
                iph_check_ip_checksum_engine_state = ICICES_CSUM;
            }
        break;
        case ICICES_CSUM:
            if (!iph_validFifoOut.full()){
                if(csum(31,16) > 0){
                    csum = csum(31,16) + csum(15,0);
                } else {
                    temp_csum = ~csum(15,0);
                    iph_validFifoOut.write((temp_csum == 0x0000) && icic_ip_sums.ipMatch);
                    iph_check_ip_checksum_engine_state = ICICES_IDLE;
                }
            }
        break;
    }
}
#endif


#if 0
/** @ingroup iprx_handler
 *  Detects the MAC protocol in the header of the packet, ARP and IP packets are forwared accordingly,
 *  packets of other protocols are discarded
 *  @param[in]      dataIn
 *  @param[out]         ARPdataOut
 *  @param[out]         IPdataOut
 */
void detect_mac_protocol_0(   stream<axiWord> &dataIn,
                            stream<axiWord> &ARPdataOut,
                            stream<axiWord> &IPdataOut,
                            ap_uint<48> myMacAddress)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<1> dmp_fsmState = 0;
    static ap_uint<2> dmp_wordCount = 0;
    static ap_uint<16> dmp_macType;
    static axiWord dmp_prevWord;
    axiWord currWord;

    switch (dmp_fsmState) {
    case 0:
        if (!dataIn.empty()) {
            dataIn.read(currWord);
            switch (dmp_wordCount) {
            case 0:
                if (currWord.data.range(47, 0) != myMacAddress && currWord.data.range(47, 0) !=0xFFFFFFFFFFFF) // Verify that this node is the destination of this Ethenet frame
                    dmp_macType = DROP;
                else
                    dmp_macType = FORWARD;

                dmp_wordCount++;
            break;
            default:
                if (dmp_wordCount == 1) {
                    if (dmp_macType != DROP)
                        dmp_macType = byteSwap16(currWord.data(47, 32));
                    dmp_wordCount++;
                }
                if (dmp_macType == ARP)
                    ARPdataOut.write(dmp_prevWord);
                else if (dmp_macType == IPv4)
                    IPdataOut.write(dmp_prevWord);
                break;
            }
            dmp_prevWord = currWord;
            if (currWord.last) {
                dmp_wordCount = 0;
                dmp_fsmState = 1;
            }
        }
    break;
    case 1:
        if (dmp_macType == ARP)
            ARPdataOut.write(dmp_prevWord);
        else if (dmp_macType == IPv4)
            IPdataOut.write(dmp_prevWord);
        dmp_fsmState = 0;
    break;
    } //switch
}
#endif


#if 0
/** @ingroup iprx_handler
 *  Checks IP checksum and removes MAC wrapper, writes valid into @param ipValidBuffer
 *  @param[in]      dataIn, incoming data stream
 *  @param[in]      myIpAddress, our IP address which is set externally
 *  @param[out]         dataOut, outgoing data stream
 *  @param[out]         ipValidFifoOut
 */
void check_ip_checksum_0( stream<axiWord>&        dataIn,
                        ap_uint<32>                 myIpAddress,
                        stream<axiWord>&        dataOut,
                        stream<subSums>&        iph_subSumsFifoIn,
                        stream<ap_uint<1> >&    ipValidFifoVersionNo,
                        stream<ap_uint<1> >&    ipFragmentDrop)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
    static ap_uint<8> cics_ipHeaderLen = 0;
    static axiWord cics_prevWord;
    static bool cics_wasLast = false;
    static ap_uint<3> cics_wordCount = 0;
    static ap_uint<32> cics_dstIpAddress = 0;

    axiWord currWord;
    axiWord sendWord;
    ap_uint<16> temp;
    bool ipMatch    = false;

    currWord.last = 0;
    if (!dataIn.empty() && !cics_wasLast) {
        dataIn.read(currWord);
        switch (cics_wordCount) {
        case 0:
            // This is MAC only we throw it out
            for (uint8_t i=0;i<4;++i)
                cics_ip_sums[i] = 0;
            cics_wordCount++;
            break;
        case 1:
            if (currWord.data.range(55, 52) != 4)   // Verify that the IP protocol version number is what it should be
                ipValidFifoVersionNo.write(0);      // If not, drop the packet
            else
                ipValidFifoVersionNo.write(1);
            cics_ip_sums[3] += byteSwap16(currWord.data.range(63, 48));
            cics_ip_sums[3] = (cics_ip_sums[3] + (cics_ip_sums[3] >> 16)) & 0xFFFF;
            cics_ipHeaderLen = currWord.data.range(51, 48);
            cics_wordCount++;
            break;
        case 2: // Check for fragmentation in this cycle. Need to check the MF flag. If that's set then drop. If it's not set then check the fragment offset and if its non-zero then drop.
        {
            ap_uint<16> temp = byteSwap16(currWord.data.range(47, 32));
            if (temp.range(12, 0) != 0 || temp.bit(13) != 0)
                ipFragmentDrop.write(0);
            else
                ipFragmentDrop.write(1);
            for (int i = 0; i < 4; i++)     {
            #pragma HLS unroll
                cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
            }
            cics_ipHeaderLen -= 2;
            cics_wordCount++;
            break;
        }
        case 3: //maybe merge with WORD_2
            //cics_srcMacIpTuple.ipAddress = currWord.data(47, 16);
            cics_dstIpAddress.range(15, 0) = currWord.data.range(63, 48);
            for (int i = 0; i < 4; i++) {
            #pragma HLS unroll
                cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
            }
            // write tcp len out
            cics_ipHeaderLen -= 2;
            cics_wordCount++;
            break;
        default:
            if (cics_wordCount == 4)
                cics_dstIpAddress(31, 16) = currWord.data(15, 0);
            switch (cics_ipHeaderLen) {
            case 0:
                break;
            case 1:
                // Sum up part0
                cics_ip_sums[0] += byteSwap16(currWord.data.range(15, 0));
                cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
                cics_ipHeaderLen = 0;
                if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                    ipMatch = true;
                iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                break;
            case 2:
                // Sum up part 0-2
                for (int i = 0; i < 4; i++) {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                cics_ipHeaderLen = 0;
                if (cics_dstIpAddress == myIpAddress || cics_dstIpAddress == 0xFFFFFFFF)
                    ipMatch = true;
                iph_subSumsFifoIn.write(subSums(cics_ip_sums, ipMatch));
                break;
            default:
                // Sum up everything
                for (int i = 0; i < 4; i++) {
                #pragma HLS unroll
                    cics_ip_sums[i] += byteSwap16(currWord.data.range(i*16+15, i*16));
                    cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
                }
                cics_ipHeaderLen -= 2;
                break;
            }
            break;
        } // switch WORD_N

        if (cics_wordCount > 2) {
            // Send Word
            sendWord = axiWord((currWord.data(47, 0), cics_prevWord.data(63, 48)), (currWord.keep(5, 0), cics_prevWord.keep(7,6)), (currWord.keep[6] == 0));
            dataOut.write(sendWord);
        }

        cics_prevWord = currWord;
        if (currWord.last) {
            cics_wordCount = 0;
            cics_wasLast = !sendWord.last;
            //cics_wasLast = true;
        }
    } else if(cics_wasLast) {
        // Send remaining Word;
        sendWord = axiWord(cics_prevWord.data.range(63, 48), cics_prevWord.keep.range(7, 6), 1);
        dataOut.write(sendWord);
        cics_wasLast = false;
    }
}
#endif


#if 0
void length_check_0(stream<axiWord> &lengthCheckIn, stream<axiWord> &ipDataFifo)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

   static enum lcState {LC_IDLE = 0, LC_SIZECHECK, LC_STREAM} lengthCheckState;
   static ap_uint<2> dip_wordCount      = 0;
   static ap_uint<2> dip_leftToWrite    = 0;
   static ap_uint<1> filterPacket   = 0;
   static ap_shift_reg<axiWord, 2> wordBuffer;
   axiWord temp;

   if (dip_leftToWrite == 0) {
            if (!lengthCheckIn.empty()) {
                axiWord currWord = lengthCheckIn.read();
            switch (lengthCheckState) {
                case LC_IDLE:
                    wordBuffer.shift(currWord);
                    if (dip_wordCount == 1)
                        lengthCheckState = LC_SIZECHECK;
                    dip_wordCount++;
                    break;
                case LC_SIZECHECK:
                    byteSwap16(currWord.data.range(15, 0)) > MaxDatagramSize ? filterPacket = 1 : filterPacket = 0;
                    temp = wordBuffer.shift(currWord);
                    if (filterPacket == 0)
                        ipDataFifo.write(temp);
                    lengthCheckState = LC_STREAM;
                    break;
                case LC_STREAM:
                    temp = wordBuffer.shift(currWord);
                    if (filterPacket == 0)
                        ipDataFifo.write(temp);
                    break;
             }
             if (currWord.last) {
                dip_wordCount   = 0;
                dip_leftToWrite = 2;
                lengthCheckState    = LC_IDLE;
             }
            }
   } else if (dip_leftToWrite != 0) {
      //axiWord nullAxiWord = {0, 0, 0};
      temp = wordBuffer.shift(axiWord(0, 0, 0));
      if (filterPacket == 0)
          ipDataFifo.write(temp);
      dip_leftToWrite--;
   }
}
#endif


#if 0
/** @ingroup iprx_handler
 *  Reads a packed and its valid flag in, if the packet is valid it is forwarded,
 *  otherwise it is dropped
 *  @param[in]      inData, incoming data stream
 *  @param[in]      ipValidFifoIn, FIFO containing valid flag to indicate if packet is valid,
 *  @param[out]         outData, outgoing data stream
 */
void ip_invalid_dropper_0(stream<axiWord>&        dataIn,
                        stream<ap_uint<1> >&    ipValidFifoIn,
                        // Add a stream here for fragment drop
                        stream<ap_uint<1> >         &ipValidFifoVersionNo,
                        stream<ap_uint<1> >         &ipFragmentDrop,
                        stream<axiWord>&        dataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static bool iid_firstWord = true;
    static bool iid_drop = false;

    axiWord currWord = axiWord(0, 0, 0);

    if (iid_drop) {
        if(!dataIn.empty())
            dataIn.read(currWord);
    }
    else if (iid_firstWord) {
        if (!ipValidFifoIn.empty() && !ipValidFifoVersionNo.empty() && !ipFragmentDrop.empty() && !dataIn.empty()) {
            ap_uint<2> valid = ipValidFifoIn.read() + ipValidFifoVersionNo.read() + ipFragmentDrop.read(); // Check the fragment drop queue
            dataIn.read(currWord);
            if (valid != 3)
                iid_drop = true;
            else
                dataOut.write(currWord);
            iid_firstWord = false;
        }
    }
    else if(!dataIn.empty()) {
        dataIn.read(currWord);
        dataOut.write(currWord);
    }
    if (currWord.last == 1) {
        iid_drop = false;
        iid_firstWord = true;
    }
}
#endif


#if 0
/** @ingroup ip_hanlder
 *
 */
void cut_length_0(stream<axiWord> &dataIn, stream<axiWord> &dataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<16> cl_wordCount = 0;
    static ap_uint<16> cl_totalLength = 0;
    static bool cl_drop = false;
    if (cl_drop) {
        if (!dataIn.empty()) {
            axiWord currWord = dataIn.read();
            if (currWord.last)
                cl_drop = false;
        }
    }
    else if (!dataIn.empty() && !dataOut.full()) {
        axiWord currWord = dataIn.read();
        switch (cl_wordCount) {
        case 0:
            cl_totalLength = byteSwap16(currWord.data(31, 16));
            break;
        default:
            if (((cl_wordCount+1)*8) >= cl_totalLength) {   //last real world
                if (currWord.last == 0)
                    cl_drop = true;
                currWord.last = 1;
                ap_uint<4> leftLength = cl_totalLength - (cl_wordCount*8);
                currWord.keep = returnKeep(leftLength);
            }
            break;
        }
        dataOut.write(currWord);
        cl_wordCount++;
        if (currWord.last)
            cl_wordCount = 0;
    }
}
#endif

#if 0
/** @ingroup iprx_handler
 *  Detects IP protocol in the packet. ICMP, UDP and TCP packets are forwarded, packets of other IP protocols are discarded.
 *  @param[in]      dataIn, incoming data stream
 *  @param[out]         ICMPdataOut, outgoing ICMP (Ping) data stream
 *  @param[out]         UDPdataOut, outgoing UDP data stream
 *  @param[out]         TCPdataOut, outgoing TCP data stream
 */
void detect_ip_protocol_0(stream<axiWord> &dataIn,
                        stream<axiWord> &ICMPdataOut,
                        stream<axiWord>     &ICMPexpDataOut,
                        stream<axiWord> &UDPdataOut,
                        stream<axiWord> &TCPdataOut)
{

#pragma HLS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

    static ap_uint<8>   dip_ipProtocol  = 0;
    static ap_uint<2>   dip_wordCount   = 0;
    static ap_uint<1>   dip_ttlExpired  = 0;
    static axiWord          dip_prevWord;
    static bool         dip_leftToWrite = false;

    axiWord currWord;

    if (dip_leftToWrite) {
        uint8_t bitCounter = 0;
        for (uint8_t i=0;i<8;++i) {
            if (dip_prevWord.keep.bit(i) == 1)
                bitCounter++;
        }
        if (dip_prevWord.keep != 0xFF)
            dip_prevWord.data.range(63, 64-((8-bitCounter)*8)) = 0;
        if (dip_ttlExpired == 1) {
            ICMPexpDataOut.write(dip_prevWord);
        }
        else {
            switch (dip_ipProtocol) {
            case ICMP:
                ICMPdataOut.write(dip_prevWord);
                break;
            case UDP:
                UDPdataOut.write(dip_prevWord);
                break;
            case TCP:
                TCPdataOut.write(dip_prevWord);
                break;
            }
        }
        dip_leftToWrite = false;
    }
    else if (!dataIn.empty()) {
        dataIn.read(currWord);
        switch (dip_wordCount) {
        case 0:
            dip_wordCount++;
            break;
        default:
            if (dip_wordCount == 1) {
                if (currWord.data.range(7, 0) == 1)
                    dip_ttlExpired = 1;
                else
                    dip_ttlExpired = 0;
                dip_ipProtocol = currWord.data.range(15, 8);
                dip_wordCount++;
            }
            // There is not default, if package does not match any case it is automatically dropped
            if (dip_ttlExpired == 1) {
                ICMPexpDataOut.write(dip_prevWord);
            }
            else {
                switch (dip_ipProtocol) {
                case ICMP:
                    ICMPdataOut.write(dip_prevWord);
                    break;
                case UDP:
                    UDPdataOut.write(dip_prevWord);
                    break;
                case TCP:
                    TCPdataOut.write(dip_prevWord);
                    break;
                }
            }
            break;
        }
        dip_prevWord = currWord;
        if (currWord.last) {
            dip_wordCount = 0;
            dip_leftToWrite = true;
        }
    }
}
#endif


#if 0
/** @ingroup iprx_handler
 *  @param[in]      dataIn, incoming data stream
 *  @param[in]      regIpAddress, our IP address
 *  @param[out]         ARPdataOut, outgoing ARP data stream
 *  @param[out]         ICMPdataOut, outgoing ICMP (Ping) data stream
 *  @param[out]         UDPdataOut, outgoing UDP data stream
 *  @param[out]         TCPdataOut, outgoing TCP data stream
 */
/******************************************************************************/
void iprx_handler_0(
        stream<axiWord>     &dataIn,
        stream<axiWord>     &ARPdataOut,
        stream<axiWord>     &ICMPdataOut,
        stream<axiWord>     &ICMPexpDataOut,
        stream<axiWord>     &UDPdataOut,
        stream<axiWord>     &TCPdataOut,
        ap_uint<32>         regIpAddress,
        ap_uint<48>         myMacAddress)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none port=return

//#pragma HLS INTERFACE port=dataIn             axis
//#pragma HLS INTERFACE port=ARPdataOut         register axis
//#pragma HLS INTERFACE port=ICMPdataOut        register axis
//#pragma HLS INTERFACE port=ICMPexpDataOut     register axis
//#pragma HLS INTERFACE port=UDPdataOut         register axis
//#pragma HLS INTERFACE port=TCPdataOut         register axis

    #pragma  HLS resource core=AXI4Stream variable=dataIn           metadata="-bus_bundle s_dataIn"
    #pragma  HLS resource core=AXI4Stream variable=ARPdataOut       metadata="-bus_bundle m_ARPdataOut"
    #pragma  HLS resource core=AXI4Stream variable=ICMPdataOut          metadata="-bus_bundle m_ICMPdataOut"
    #pragma  HLS resource core=AXI4Stream variable=ICMPexpDataOut   metadata="-bus_bundle m_ICMPexpDataOut"
    #pragma  HLS resource core=AXI4Stream variable=UDPdataOut       metadata="-bus_bundle m_UDPdataOut"
    #pragma  HLS resource core=AXI4Stream variable=TCPdataOut       metadata="-bus_bundle m_TCPdataOut"

    //If you need to run co-sim exchange the ap_none pragma for the ap_stable one in the 2 lines below. This will allow co-sim to go through ad
    //does ot affect co-sim functionality as the IP adddress is anyhow fixed in the TB.
    //#pragma HLS INTERFACE ap_stable port=regIpAddress

    #pragma HLS INTERFACE ap_none register port=regIpAddress
    #pragma HLS INTERFACE ap_stable port=myMacAddress

    static stream<axiWord>              lengthCheckIn("lengthCheckIn");
    #pragma HLS STREAM variable=lengthCheckIn           depth=4

    static stream<axiWord>              ipDataFifo("ipDataFifo");
    #pragma HLS DATA_PACK variable=ipDataFifo
    #pragma HLS STREAM variable=ipDataFifo                  depth=4

    static stream<axiWord>              ipDataCheckFifo("ipDataCheckFifo");
    #pragma HLS DATA_PACK variable=ipDataCheckFifo
    #pragma HLS STREAM variable=ipDataCheckFifo         depth=128 //must hold IP header for checksum checking

    static stream<axiWord>              ipDataDropFifo("ipDataDropFifo");
    #pragma HLS DATA_PACK variable=ipDataDropFifo
    #pragma HLS STREAM variable=ipDataDropFifo              depth=4

    static stream<axiWord>              ipDataCutFifo("ipDataCutFifo");
    #pragma HLS DATA_PACK variable=ipDataCutFifo
    #pragma HLS STREAM variable=ipDataCutFifo           depth=4

    static stream<subSums>              iph_subSumsFifoOut("iph_subSumsFifoOut");
    #pragma HLS DATA_PACK variable=iph_subSumsFifoOut
    #pragma HLS STREAM variable=iph_subSumsFifoOut          depth=4

    static stream<ap_uint<1> >          ipValidFifo("ipValidFifo");
    #pragma HLS STREAM variable=ipValidFifo             depth=4

    static stream<ap_uint<1> >          ipValidFifoVersionNo("ipValidFifoVersionNo");
    #pragma HLS STREAM variable=ipValidFifoVersionNo    depth=4

    static stream<ap_uint<1> >          ipFragmentDrop("ipFragmentDrop");
    #pragma HLS STREAM variable=ipFragmentDrop              depth=4

    detect_mac_protocol(dataIn, ARPdataOut, lengthCheckIn, myMacAddress);
    length_check(lengthCheckIn, ipDataFifo);
    check_ip_checksum(ipDataFifo, regIpAddress, ipDataCheckFifo, iph_subSumsFifoOut, ipValidFifoVersionNo, ipFragmentDrop);
    iph_check_ip_checksum(iph_subSumsFifoOut, ipValidFifo);
    ip_invalid_dropper(ipDataCheckFifo, ipValidFifo, ipValidFifoVersionNo, ipFragmentDrop, ipDataDropFifo);
    cut_length(ipDataDropFifo, ipDataCutFifo);
    detect_ip_protocol(ipDataCutFifo, ICMPdataOut, ICMPexpDataOut, UDPdataOut, TCPdataOut);
}
#endif


#if 0
/** @ingroup iprx_handler
 *  @param[in]      dataIn, incoming data stream
 *  @param[in]      regIpAddress, our IP address
 *  @param[out]         ARPdataOut, outgoing ARP data stream
 *  @param[out]         ICMPdataOut, outgoing ICMP (Ping) data stream
 *  @param[out]         UDPdataOut, outgoing UDP data stream
 *  @param[out]         TCPdataOut, outgoing TCP data stream
 */
void iprx_handler_0(stream<axiWord>&        dataIn,
                stream<axiWord>&        ARPdataOut,
                stream<axiWord>&        ICMPdataOut,
                stream<axiWord>&        ICMPexpDataOut,
                stream<axiWord>&        UDPdataOut,
                stream<axiWord>&        TCPdataOut,
                ap_uint<32>                 regIpAddress,
                ap_uint<48>                 myMacAddress)
{
#pragma HLS DATAFLOW interval=1
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE port=dataIn           axis
#pragma HLS INTERFACE port=ARPdataOut       register axis
#pragma HLS INTERFACE port=ICMPdataOut          register axis
#pragma HLS INTERFACE port=ICMPexpDataOut   register axis
#pragma HLS INTERFACE port=UDPdataOut       register axis
#pragma HLS INTERFACE port=TCPdataOut       register axis

    /*#pragma  HLS resource core=AXI4Stream variable=dataIn metadata="-bus_bundle s_axis_raw"
    #pragma  HLS resource core=AXI4Stream variable=ARPdataOut metadata="-bus_bundle m_axis_ARP"
    #pragma  HLS resource core=AXI4Stream variable=ICMPdataOut metadata="-bus_bundle m_axis_ICMP"
    #pragma  HLS resource core=AXI4Stream variable=ICMPexpDataOut metadata="-bus_bundle m_axis_ICMPexp"
    #pragma  HLS resource core=AXI4Stream variable=UDPdataOut metadata="-bus_bundle m_axis_UDP"
    #pragma  HLS resource core=AXI4Stream variable=TCPdataOut metadata="-bus_bundle m_axis_TCP"*/

    //If you need to run co-sim exchange the ap_none pragma for the ap_stable one in the 2 lines below. This will allow co-sim to go through ad
    //does ot affect co-sim functionality as the IP adddress is anyhow fixed in the TB.
    //#pragma HLS INTERFACE ap_stable port=regIpAddress

    #pragma HLS INTERFACE ap_none register port=regIpAddress
    #pragma HLS INTERFACE ap_stable port=myMacAddress

    static stream<axiWord>              lengthCheckIn("lengthCheckIn");
    #pragma HLS STREAM variable=lengthCheckIn           depth=4

    static stream<axiWord>              ipDataFifo("ipDataFifo");
    #pragma HLS DATA_PACK variable=ipDataFifo
    #pragma HLS STREAM variable=ipDataFifo                  depth=4

    static stream<axiWord>              ipDataCheckFifo("ipDataCheckFifo");
    #pragma HLS DATA_PACK variable=ipDataCheckFifo
    #pragma HLS STREAM variable=ipDataCheckFifo         depth=128 //must hold IP header for checksum checking

    static stream<axiWord>              ipDataDropFifo("ipDataDropFifo");
    #pragma HLS DATA_PACK variable=ipDataDropFifo
    #pragma HLS STREAM variable=ipDataDropFifo              depth=4

    static stream<axiWord>              ipDataCutFifo("ipDataCutFifo");
    #pragma HLS DATA_PACK variable=ipDataCutFifo
    #pragma HLS STREAM variable=ipDataCutFifo           depth=4

    static stream<subSums>              iph_subSumsFifoOut("iph_subSumsFifoOut");
    #pragma HLS DATA_PACK variable=iph_subSumsFifoOut
    #pragma HLS STREAM variable=iph_subSumsFifoOut          depth=4

    static stream<ap_uint<1> >          ipValidFifo("ipValidFifo");
    #pragma HLS STREAM variable=ipValidFifo             depth=4

    static stream<ap_uint<1> >          ipValidFifoVersionNo("ipValidFifoVersionNo");
    #pragma HLS STREAM variable=ipValidFifoVersionNo    depth=4

    static stream<ap_uint<1> >          ipFragmentDrop("ipFragmentDrop");
    #pragma HLS STREAM variable=ipFragmentDrop              depth=4

    detect_mac_protocol(dataIn, ARPdataOut, lengthCheckIn, myMacAddress);
    length_check(lengthCheckIn, ipDataFifo);
    check_ip_checksum(ipDataFifo, regIpAddress, ipDataCheckFifo, iph_subSumsFifoOut, ipValidFifoVersionNo, ipFragmentDrop);
    iph_check_ip_checksum(iph_subSumsFifoOut, ipValidFifo);
    ip_invalid_dropper(ipDataCheckFifo, ipValidFifo, ipValidFifoVersionNo, ipFragmentDrop, ipDataDropFifo);
    cut_length(ipDataDropFifo, ipDataCutFifo);
    detect_ip_protocol_0(ipDataCutFifo, ICMPdataOut, ICMPexpDataOut, UDPdataOut, TCPdataOut);
}

#endif

/*refactored version*/
/******************************************************************************
void iprx_handler_1(stream<axiWord>&        dataIn,
                stream<axiWord>&        ARPdataOut,
                stream<axiWord>&        ICMPdataOut,
                stream<axiWord>&        ICMPexpDataOut,
                stream<axiWord>&        UDPdataOut,
                stream<axiWord>&        TCPdataOut,
                ap_uint<32>                 regIpAddress,
                ap_uint<48>                 myMacAddress)
{
#pragma HLS DATAFLOW //interval=1
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE port=dataIn           axis
#pragma HLS INTERFACE port=ARPdataOut       axis
#pragma HLS INTERFACE port=ICMPdataOut          axis
#pragma HLS INTERFACE port=ICMPexpDataOut   axis
#pragma HLS INTERFACE port=UDPdataOut       axis
#pragma HLS INTERFACE port=TCPdataOut       axis

    #pragma HLS INTERFACE ap_none register port=regIpAddress
    #pragma HLS INTERFACE ap_none register port=myMacAddress

    //#pragma HLS INTERFACE port=regIpAddress axis
    //#pragma HLS INTERFACE port=myMacAddress axis

    //#pragma HLS INTERFACE ap_stable port=myMacAddress

    static stream<axiWord>              lengthCheckIn("lengthCheckIn");
    #pragma HLS STREAM variable=lengthCheckIn           depth=4

    static stream<axiWord>              ipDataFifo("ipDataFifo");
    #pragma HLS DATA_PACK variable=ipDataFifo
    #pragma HLS STREAM variable=ipDataFifo                  depth=4

    static stream<axiWord>              ipDataCheckFifo("ipDataCheckFifo");
    #pragma HLS DATA_PACK variable=ipDataCheckFifo
    #pragma HLS STREAM variable=ipDataCheckFifo         depth=128 //must hold IP header for checksum checking

    static stream<axiWord>              ipDataDropFifo("ipDataDropFifo");
    #pragma HLS DATA_PACK variable=ipDataDropFifo
    #pragma HLS STREAM variable=ipDataDropFifo              depth=4

    static stream<axiWord>              ipDataCutFifo("ipDataCutFifo");
    #pragma HLS DATA_PACK variable=ipDataCutFifo
    #pragma HLS STREAM variable=ipDataCutFifo           depth=4

    static stream<subSums>              iph_subSumsFifoOut("iph_subSumsFifoOut");
    #pragma HLS DATA_PACK variable=iph_subSumsFifoOut
    #pragma HLS STREAM variable=iph_subSumsFifoOut          depth=4

    static stream<ap_uint<1> >          ipValidFifo("ipValidFifo");
    #pragma HLS STREAM variable=ipValidFifo             depth=4

    static stream<ap_uint<1> >          ipValidFifoVersionNo("ipValidFifoVersionNo");
    #pragma HLS STREAM variable=ipValidFifoVersionNo    depth=4

    static stream<ap_uint<1> >          ipFragmentDrop("ipFragmentDrop");
    #pragma HLS STREAM variable=ipFragmentDrop              depth=4

    detect_mac_protocol(dataIn, ARPdataOut, lengthCheckIn, myMacAddress);
    length_check(lengthCheckIn, ipDataFifo);
    check_ip_checksum(ipDataFifo, regIpAddress, ipDataCheckFifo, iph_subSumsFifoOut, ipValidFifoVersionNo, ipFragmentDrop);
    //check_ip_checksum_1(ipDataFifo, regIpAddress, ipDataCheckFifo, iph_subSumsFifoOut, ipValidFifoVersionNo, ipFragmentDrop);
    //iph_check_ip_checksum(iph_subSumsFifoOut, ipValidFifo);
    iph_check_ip_checksum_1(iph_subSumsFifoOut, ipValidFifo);
    //ip_invalid_dropper(ipDataCheckFifo, ipValidFifo, ipValidFifoVersionNo, ipFragmentDrop, ipDataDropFifo);
    ip_invalid_dropper_1(ipDataCheckFifo, ipValidFifo, ipValidFifoVersionNo, ipFragmentDrop, ipDataDropFifo);
    //cut_length(ipDataDropFifo, ipDataCutFifo);
    cut_length_1(ipDataDropFifo, ipDataCutFifo);
    //detect_ip_protocol(ipDataCutFifo, ICMPdataOut, ICMPexpDataOut, UDPdataOut, TCPdataOut);
    detect_ip_protocol_1(ipDataCutFifo, ICMPdataOut, ICMPexpDataOut, UDPdataOut, TCPdataOut);
    //detect_ip_protocol_2(ipDataCutFifo, ICMPdataOut, ICMPexpDataOut, UDPdataOut, TCPdataOut);
}
******************************************************************************/
