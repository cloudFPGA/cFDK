/*****************************************************************************
 * @file       : rx_engine.cpp
 * @brief      : Rx Engine (RXE) of the TCP Offload Engine (TOE)
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
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

#include "rx_engine.hpp"

#define THIS_NAME "TOE/RXe"

using namespace hls;

#define DEBUG_LEVEL 1

enum DropCmd {KEEP_CMD=false, DROP_CMD};





/*****************************************************************************
 * @brief TCP length extraction (Tle).
 *
 * @param[in]  siIPRX_Pkt,    IP4 packet stream form IPRX.
 * @param[out] soTcpSeg,      A pseudo TCP segment (.i.e, IP-SA + IP-DA + TCP)
 * @param[out] soTcpSegLen,   The length of the pseudo TCP segment.
 *
 * @details
 *   This is the process that handles the incoming data stream from the IPRX.
 *   It extracts the TCP length field from the IP header, removes that IP
 *   header but keeps the IP source and destination addresses in front of the
 *   TCP segment so that the output can be used by the next process to build
 *   the 12-byte TCP pseudo header.
 *   The length of the IPv4 data (.i.e. the TCP segment length) is also
 *   computed from the IPv4 total length and the IPv4 header length.
 *
 *   The data received from the Ethernet MAC are logically divided into lane #0
 *   (7:0) to lane #7 (63:56). The format of an incoming IPv4 packet is then:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |   Ident (L)   |   Ident (H)   | Total Len (L) | Total Len (H) |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    | Hd Chksum (L) | Hd Chksum (H) |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *    The format of the outgoing segment is the following:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                      0x0000000000000000                                                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |   |U|A|P|R|S|F|  Data |       |               |               |               |               |
 *  |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |
 *  |               |               |   |G|K|H|T|N|N|       |       |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |               |               |
 *  |    Data 3     |    Data 2     |    Data 1     |    Data 0     |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |
 *  |               |               |               |               |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               NU                              |    Data 7     |    Data 6     |    Data 5     |    Data 4     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pTcpLengthExtract(
        stream<Ip4Word>        &siIPRX_Pkt,
        stream<TcpWord>        &soTcpSeg,
        stream<TcpSegLen>      &soTcpSegLen)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Tle");

    static Ip4Hdr_HdrLen    tle_ipHeaderLen = 0;
    static Ip4Hdr_TotalLen  tle_ipTotalLen  = 0;
    static Ip4DatLen        tle_ipDataLen   = 0;
    static ap_uint<4>       tle_wordCount   = 0;
    static bool             tle_insertWord  = false;
    static bool             tle_wasLast     = false;
    static bool             tle_shift       = true;
    static Ip4Word          tle_prevWord;
    TcpWord                 sendWord;

    if (tle_insertWord) {
        sendWord = TcpWord(0, 0xFF, 0);
        soTcpSeg.write(sendWord);
        tle_insertWord = false;
        if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
    }
    else if (!siIPRX_Pkt.empty() && !tle_wasLast) {
        Ip4Word currWord = siIPRX_Pkt.read();
        switch (tle_wordCount) {
        case 0:
            tle_ipHeaderLen  = currWord.tdata(3, 0);
            tle_ipTotalLen   = byteSwap16(currWord.tdata(31, 16));
            // Compute length of IPv4 data (.i.e. the TCP segment length)
            tle_ipDataLen    = tle_ipTotalLen - (tle_ipHeaderLen * 4);
            tle_ipHeaderLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case 1:
            // Forward length of IPv4 data
            soTcpSegLen.write(tle_ipDataLen);
            tle_ipHeaderLen -= 2; // We just processed 8 bytes
            tle_wordCount++;
            break;
        case 2:
            // Forward destination IP address
            // Warning, half of this address is now in 'prevWord'
            sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                               (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                               (currWord.tkeep[4] == 0));
            soTcpSeg.write(sendWord);
            tle_ipHeaderLen -= 1;  // We just processed the last 8 bytes of the IP header
            tle_insertWord = true;
            tle_wordCount++;
            if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
            break;
        case 3:
            switch (tle_ipHeaderLen) {
            case 0: // Half of prevWord contains valuable data and currWord is full of valuable
                sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                   (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                   (currWord.tkeep[4] == 0));
                soTcpSeg.write(sendWord);
                tle_shift = true;
                tle_ipHeaderLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
                break;
            case 1: // The prevWord contains garbage data, but currWord is valuable
                sendWord = TcpWord(currWord.tdata, currWord.tkeep, currWord.tlast);
                soTcpSeg.write(sendWord);
                tle_shift = false;
                tle_ipHeaderLen = 0;
                tle_wordCount++;
                if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
                break;
            default: // The prevWord contains garbage data, currWord at least half garbage
                //Drop this shit
                tle_ipHeaderLen -= 2;
                break;
            }
            break;
        default:
            if (tle_shift) {
                sendWord = TcpWord((currWord.tdata(31, 0), tle_prevWord.tdata(63, 32)),
                                   (currWord.tkeep( 3, 0), tle_prevWord.tkeep( 7,  4)),
                                   (currWord.tkeep[4] == 0));
                soTcpSeg.write(sendWord);
                if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
            }
            else {
                sendWord = TcpWord(currWord.tdata, currWord.tkeep, currWord.tlast);
                soTcpSeg.write(sendWord);
                if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
            }
            break;

        } // End of: switch (tle_wordCount)

        tle_prevWord = currWord;
        if (currWord.tlast) {
            tle_wordCount = 0;
            tle_wasLast = !sendWord.tlast;
        }

    } // End of: else if (!siIPRX_Data.empty() && !tle_wasLast) {

    else if (tle_wasLast) { //Assumption has to be shift
        // Send remaining data
        AxiWord sendWord = AxiWord(0, 0, 1);
        sendWord.tdata(31, 0) = tle_prevWord.tdata(63, 32);
        sendWord.tkeep( 3, 0) = tle_prevWord.tkeep( 7,  4);
        soTcpSeg.write(sendWord);
        tle_wasLast = false;
        if (DEBUG_LEVEL > 0) printAxiWord(myName, sendWord);
    }
}


/*****************************************************************************
 * @brief Insert pseudo header (Iph).
 *
 * @param[in]  siTle_TcpSeg,    A pseudo TCP segment from TCP Length Extraction.
 * @param[in]  siTle_TcpSegLen, The length of the incoming pseudo TCP segment.
 * @param[out] soTcpSeg,        An updated pseudo TCP segment.
 *
 * @details
 *  Constructs a TCP pseudo header and prepends it to the TCP payload.
 *
 *  The format of the incoming segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                      0x0000000000000000                                                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |   |U|A|P|R|S|F|  Data |       |               |               |               |               |
 *  |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |
 *  |               |               |   |G|K|H|T|N|N|       |       |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |               |               |
 *  |    Data 3     |    Data 2     |    Data 1     |    Data 0     |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |
 *  |               |               |               |               |               |               |               |               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               NU                              |    Data 7     |    Data 6     |    Data 5     |    Data 4     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 *    The format of the outgoing pseudo TCP segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |         Segment Len           |      0x06     |    0x00       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pInsertPseudoHeader(
        stream<TcpWord>     &siTle_TcpSeg,
        stream<TcpSegLen>   &siTle_TcpSegLen,
        stream<TcpWord>     &soTcpSeg)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static bool         iph_wasLast = false;
    static ap_uint<2>   iph_wordCount = 0;
    ap_uint<1>          valid;
    TcpSegLen           tcpSegLen;
    TcpWord             currWord;
    TcpWord             sendWord;
    static TcpWord      iph_prevWord;

    const char *myName  = concat3(THIS_NAME, "/", "Iph");

    currWord.tlast = 0;

    if (iph_wasLast) {
        sendWord.tdata(31, 0) = iph_prevWord.tdata(63,32);
        sendWord.tkeep( 3, 0) = iph_prevWord.tkeep( 7, 4);
        sendWord.tkeep( 7, 4) = 0x0;
        sendWord.tlast        = 0x1;
        soTcpSeg.write(sendWord);
        iph_wasLast = false;
        if (DEBUG_LEVEL > 1) printAxiWord(myName, sendWord);
    }
    else if(!siTle_TcpSeg.empty()) {
        switch (iph_wordCount) {
        case 0:
            siTle_TcpSeg.read(currWord);
            iph_wordCount++;
            break;
        case 1:
            siTle_TcpSeg.read(currWord);
            sendWord = iph_prevWord;
            // Forward IP-DA & IP-SA
            soTcpSeg.write(sendWord);
            iph_wordCount++;
            if (DEBUG_LEVEL > 1) printAxiWord(myName, sendWord);
            break;
        case 2:
            if (!siTle_TcpSegLen.empty()) {
                siTle_TcpSeg.read(currWord);
                siTle_TcpSegLen.read(tcpSegLen);
                // Forward Protocol and Segment length
                sendWord.tdata(15,  0) = 0x0600;        // 06 is for TCP
                sendWord.tdata(23, 16) = tcpSegLen(15, 8);
                sendWord.tdata(31, 24) = tcpSegLen( 7, 0);
                // Forward TCP-SP & TCP-DP
                sendWord.tdata(63, 32) = currWord.tdata(31, 0);
                sendWord.tkeep         = 0xFF;
                sendWord.tlast         = 0;
                soTcpSeg.write(sendWord);
                iph_wordCount++;
                if (DEBUG_LEVEL > 1) printAxiWord(myName, sendWord);
            }
            break;
        default:
            siTle_TcpSeg.read(currWord);
            // Forward { Sequence Number, Acknowledgment Number } or
            //         { Flags, Window, Checksum, UrgentPointer } or
            //         { Data }
            sendWord.tdata.range(31,  0) = iph_prevWord.tdata.range(63, 32);
            sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
            sendWord.tkeep.range( 3,  0) = iph_prevWord.tkeep.range(7, 4);
            sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
            sendWord.tlast               = (currWord.tkeep[4] == 0); // see format of the incoming segment
            soTcpSeg.write(sendWord);
            if (DEBUG_LEVEL > 1) printAxiWord(myName, sendWord);
            break;
        }
        iph_prevWord = currWord;
        if (currWord.tlast == 1) {
            iph_wordCount = 0;
            iph_wasLast = !sendWord.tlast;
        }
    }
}


/*****************************************************************************
 * @brief TCP checksum accumulator (csa).
 *
 * @param[in]  siIph_TcpSeg,A pseudo TCP segment from Insert Pseudo Header (Iph).
 * @param[out] soData,      TCP data stream.
 * @param[out] soDataValid, TCP data valid.
 * @param[out] soMeta,      TCP metadata.
 * @param[out] soSockPair,  TCP socket pair.
 * @param[out] soDstPort    TCP destination port number.

 *
 * @details
 *  This process extracts the data (i.e. text octets) from the incoming segment
 *   and forwards them to the TCP Invalid Dropper (Tid) together with a valid
 *   bit indicating the result of the checksum validation.
 *  It also extracts the socket pair information and some metadata information
 *   from the TCP segment and forwards them to the MetaData Handler (Mdh).
 *  Next, the TCP destination port number is extracted and forwarded to the
 *   Port Table (PRt) process to check if the port is open.
 *
 *  The format of the incoming pseudo TCP segment is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DA (LL)   |     DA (L)    |      DA (H)   |    DA (HH)    |     SA (LL)   |     SA (L)    |     SA (H)    |    SA (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     DP (L)    |     DP (H)    |     SP (L)    |    SP (H)     |         Segment Len           |      0x06     |    0x00       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Ack (LL)   |    Ack (L)    |    Ack (H)    |   Ack (HH)    |    Seq (LL)   |    Seq (L)    |     Seq (H)   |   Seq (HH)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |   |U|A|P|R|S|F|  Data |       |
 *  |  UrgPtr(L)    |  UrgPtr(H)    |   CSum (L)    |  CSum (H)     |    Win (L)    |    Win (H)    |Res|R|C|S|S|Y|I| Offset| Res   |
 *  |               |               |               |               |               |               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Data 7     |    Data 6     |    Data 5     |    Data 4     |    Data 3     |    Data 2     |    Data 1     |    Data 0     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
 *
 * @ingroup rx_engine
 ******************************************************************************/
void pCheckSumAccumulator(
        stream<TcpWord>             &siIph_TcpSeg,
        stream<AxiWord>             &soData,
        stream<ValBit>              &soDataValid,
        stream<rxEngineMetaData>    &soMeta,
        stream<fourTuple>           &soSockPair,
        stream<TcpPort>             &soDstPort)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Csa");
    char message[256];

    static ap_uint<17>      csa_tcp_sums[4] = {0, 0, 0, 0};
    static ap_uint<8>       csa_dataOffset = 0xFF;
    static ap_uint<16>      csa_wordCount = 0;
    static fourTuple        csa_sessionTuple;
    static ap_uint<36>      halfWord;
    TcpWord                 currWord;
    TcpWord                 sendWord;
    static rxEngineMetaData csa_meta;
    static ap_uint<16>      csa_dstPort;
    static TcpHdr_Checksum  csa_tcpHdr_CSum;

    static bool             csa_doShift     = false;
    static bool             csa_wasLast     = false;
    static bool             csa_doCSumVerif = false;

    static ap_uint<3>       csa_cc_state = 0;

    //OBSOLETE-20181031 currWord.last = 0; // might no be necessary any more FIXME to you want to risk it ;)

    if (!siIph_TcpSeg.empty() && !csa_doCSumVerif) {
        siIph_TcpSeg.read(currWord);
        switch (csa_wordCount) {
        case 0:
            csa_dataOffset = 0xFF;
            csa_doShift = false;
                // Get IP-SA & IP-DA
                //  Warning: IP addresses are stored w/ the Most Significant Byte Last
                csa_sessionTuple.srcIp = currWord.tdata(31,  0);
                csa_sessionTuple.dstIp = currWord.tdata(63, 32);
                sendWord.tlast         = currWord.tlast;
            break;
        case 1:
            // Get SEGMENT length
            //OBSOLETE-20181031 csa_meta.length( 7, 0)   = currWord.tdata(31, 24);
            //OBSOLETE-20181031 csa_meta.length(15, 8)   = currWord.tdata(23, 16);
            csa_meta.length = byteSwap16(currWord.tdata(31, 16));
            // Get TCP-SP & TCP-DP
            //  Warning: TCP ports are stored w/ Most Significant Byte Last
            csa_sessionTuple.srcPort = currWord.tdata(47, 32);
            csa_sessionTuple.dstPort = currWord.tdata(63, 48);
            csa_dstPort              = currWord.tdata(63, 48);
            sendWord.tlast           = currWord.tlast;
            break;
        case 2:
            // Get Sequence and Acknowledgment Numbers
            //OBSOLETE-20181031 csa_meta.seqNumb( 7,  0) = currWord.data(31, 24);
            //OBSOLETE-20181031 csa_meta.seqNumb(15,  8) = currWord.data(23, 16);
            //OBSOLETE-20181031 csa_meta.seqNumb(23, 16) = currWord.data(15,  8);
            //OBSOLETE-20181031 csa_meta.seqNumb(31, 24) = currWord.data( 7,  0);
            csa_meta.seqNumb = byteSwap32(currWord.tdata(31, 0));
            //OBSOLETE-20181031 csa_meta.ackNumb( 7,  0) = currWord.data(63, 56);
            //OBSOLETE-20181031 csa_meta.ackNumb(15,  8) = currWord.data(55, 48);
            //OBSOLETE-20181031 csa_meta.ackNumb(23, 16) = currWord.data(47, 40);
            //OBSOLETE-20181031 csa_meta.ackNumb(31, 24) = currWord.data(39, 32);
            csa_meta.ackNumb = byteSwap32(currWord.tdata(63, 32));
            sendWord.tlast   = currWord.tlast;
            break;
        case 3:
            // Get Data Offset
            csa_dataOffset   = currWord.tdata.range(7, 4);
            csa_meta.length -= (csa_dataOffset * 4);
            //csa_dataOffset -= 5; //FIXME, do -5
            // Get Control Bits
            /*  [ 8] == FIN
             *  [ 9] == SYN
             *  [10] == RST
             *  [11] == PSH
             *  [12] == ACK
             *  [13] == URG
             */
            csa_meta.ack = currWord.tdata[12];
            csa_meta.rst = currWord.tdata[10];
            csa_meta.syn = currWord.tdata[ 9];
            csa_meta.fin = currWord.tdata[ 8];
            // Get Window Size
            //OBSOLETE-20181031 csa_meta.winSize(7, 0) = currWord.data(31, 24);
            //OBSOLETE-20181031 csa_meta.winSize(15, 8) = currWord.data(23, 16);
            csa_meta.winSize = byteSwap16(currWord.tdata(31, 16));
            // Get the checksum of the pseudo-header (only for debug purposes)
            csa_tcpHdr_CSum = currWord.tdata(47, 32);
            sendWord.tlast = currWord.tlast;
            break;
        default:
            if (csa_dataOffset > 6) {
                // Drain the unsupported options
                csa_dataOffset -= 2;
            }
            else if (csa_dataOffset == 6) {
                // The header has 32 bits of options and padding out of 64 bits
                // Get the first four Data bytes
                csa_dataOffset = 5;
                csa_doShift = true;
                halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                halfWord.range(35, 32) = currWord.tkeep.range( 7,  4);
                //OBSOLETE-20181031 halfWord[36] = currWord.last;
                sendWord.tlast = (currWord.tkeep[4] == 0);
            }
            else {    // csa_dataOffset == 5 (or less)
                if (!csa_doShift) {
                    sendWord = currWord;
                    soData.write(sendWord);
                }
                else {
                    sendWord.tdata.range(31,  0) = halfWord.range(31, 0);
                    sendWord.tdata.range(63, 32) = currWord.tdata.range(31, 0);
                    sendWord.tkeep.range( 3,  0) = halfWord.range(35, 32);
                    sendWord.tkeep.range( 7,  4) = currWord.tkeep.range(3, 0);
                    sendWord.tlast = (currWord.tkeep[4] == 0);
                    //OBSOLETE-20181031 /*if (currWord.last && currWord.strb.range(7, 4) != 0)
                    //OBSOLETE-20181031 {
                    //OBSOLETE-20181031     sendWord.last = 0;
                    //OBSOLETE-20181031 }*/
                    soData.write(sendWord);
                    halfWord.range(31,  0) = currWord.tdata.range(63, 32);
                    halfWord.range(35, 32) = currWord.tkeep.range(7, 4);
                    //OBSOLETE-20181031 //halfWord[36] = currWord.last; //FIXME not needed
                }
            }
            break;
        } // End of: switch

        // Accumulate TCP checksum
        for (int i = 0; i < 4; i++) {
            #pragma HLS UNROLL
            ap_uint<16> temp;
            if (currWord.tkeep.range(i*2+1, i*2) == 0x3) {
                temp( 7, 0) = currWord.tdata.range(i*16+15, i*16+8);
                temp(15, 8) = currWord.tdata.range(i*16+ 7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
            else if (currWord.tkeep[i*2] == 0x1) {
                temp( 7, 0) = 0;
                temp(15, 8) = currWord.tdata.range(i*16+7, i*16);
                csa_tcp_sums[i] += temp;
                csa_tcp_sums[i] = (csa_tcp_sums[i] + (csa_tcp_sums[i] >> 16)) & 0xFFFF;
            }
        }

        csa_wordCount++;

        if(currWord.tlast == 1) {  // FIXME - Can we get ride of this cycle (see ETHZ version)
            csa_wordCount = 0;
            csa_wasLast = !sendWord.tlast;
            csa_doCSumVerif = true;
        }

    } // End of: if (!siIph_Data.empty() && !csa_doCSumVerif)

    else if(csa_wasLast) {
        if (csa_meta.length != 0) {
            sendWord.tdata.range(31,  0) = halfWord.range(31,  0);
            sendWord.tdata.range(63, 32) = 0;
            sendWord.tkeep.range( 3,  0) = halfWord.range(35, 32);
            sendWord.tkeep.range( 7,  4) = 0;
            sendWord.tlast               = 1;
            soData.write(sendWord);
        }
        csa_wasLast = false;
    }
    
    else if (csa_doCSumVerif) {
        switch (csa_cc_state) {
            case 0:
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_tcp_sums[1] = (csa_tcp_sums[1] + (csa_tcp_sums[1] >> 16)) & 0xFFFF;
                csa_tcp_sums[2] = (csa_tcp_sums[2] + (csa_tcp_sums[2] >> 16)) & 0xFFFF;
                csa_tcp_sums[3] = (csa_tcp_sums[3] + (csa_tcp_sums[3] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 1:
                csa_tcp_sums[0] += csa_tcp_sums[2];
                csa_tcp_sums[1] += csa_tcp_sums[3];
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_tcp_sums[1] = (csa_tcp_sums[1] + (csa_tcp_sums[1] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 2:
                csa_tcp_sums[0] += csa_tcp_sums[1];
                csa_tcp_sums[0] = (csa_tcp_sums[0] + (csa_tcp_sums[0] >> 16)) & 0xFFFF;
                csa_cc_state++;
                break;
            case 3:
                csa_tcp_sums[0] = ~csa_tcp_sums[0];
                csa_cc_state++;
                break;
            case 4:
                // If summation == 0 then checksum is correct
                if (csa_tcp_sums[0](15, 0) == 0) {
                    // TCP segment is valid, write out metadata, 4-tuple and check port
                    soMeta.write(csa_meta);
                    soDstPort.write(csa_dstPort);
                    soSockPair.write(csa_sessionTuple);
                    if (csa_meta.length != 0) {
                        soDataValid.write(true);
                    }
                }
                else if(csa_meta.length != 0) {
                    soDataValid.write(false);
                    if (DEBUG_LEVEL > 0) {
                        sprintf(message, "BAD CHECKSUM (0x%4.4X).", csa_tcpHdr_CSum.to_uint());
                        printWarn(myName, message);
                        sprintf(message, "SocketPair={{0x%8.8X, 0x%4.4X},{0x%8.8X, 0x%4.4X}",
                                csa_sessionTuple.srcIp.to_uint(), csa_sessionTuple.srcPort.to_uint(),
                                csa_sessionTuple.dstIp.to_uint(), csa_sessionTuple.dstPort.to_uint());
                        printInfo(myName, message);
                    }
                }
                csa_doCSumVerif = false;
                csa_tcp_sums[0] = 0;
                csa_tcp_sums[1] = 0;
                csa_tcp_sums[2] = 0;
                csa_tcp_sums[3] = 0;
                csa_cc_state = 0;
                break;

        } // End of: switch
    }
}


/*****************************************************************************
 * @brief TCP Invalid checksum Dropper (Tid)
 *
 * @param[in]  siCsa_Data,      TCP data stream from Checksum Accumulator (Csa).
 * @param[in]  siCsa_DataVal,   TCP data valid.
 * @param[out] soData,          TCP data stream.
 *
 * @details
 *  Drops the TCP data when they are flagged with an invalid checksum by
 *   'siDataValid'. Otherwise, the TCP data is passed on.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pTcpInvalidDropper(
        stream<AxiWord>     &siCsa_Data,
        stream<ValBit>      &siCsa_DataVal,
        stream<AxiWord>     &soData)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static bool tid_isFirstWord = true;
    static bool tid_doDrop      = false;

    AxiWord currWord;
    bool     isValid;

    currWord.tlast = 0;

    if (tid_doDrop) {
        if(!siCsa_Data.empty())
            siCsa_Data.read(currWord);
    }
    else if (tid_isFirstWord) {
        if (!siCsa_DataVal.empty() && !siCsa_Data.empty()) {
            siCsa_DataVal.read(isValid);
            siCsa_Data.read(currWord);
            if (!isValid)
                tid_doDrop = true;
            else
                soData.write(currWord);
            tid_isFirstWord = false;
        }
    }
    else if(!siCsa_Data.empty()) {
        siCsa_Data.read(currWord);
        soData.write(currWord);
    }

    if (currWord.tlast == 1) {
        tid_doDrop      = false;
        tid_isFirstWord = true;
    }
}


/*****************************************************************************
 * @brief MetaData Handler (Mdh)
 *
 * @param[in]  siCsa_Meta,      TCP metadata from CheckSum Accumulator (Csa).
 * @param[in]  siCsa_SockPair,  TCP socket pair from CheckSum Accumulator.
 * @param[in]  siSLc_SessLookupRep, Session lookup reply from Session Lookup Controller (SLc).
 * @param[in]  siPRt_PortSts,   Port state (open/close) from Port Table (PRt).
 * @param[out] soSessLookupReq, Session lookup request.
 * @param[out] soSetEvent,      Set event.
 * @param[out] soDropCmd,       Drop command.
 * @param[out] soMeta,          Metadata for the central FSM of the Rx engine.
 *
 * @details
 *  This process waits until it gets a response from the Port Table (PRt).
 *   It then loads the metadata and socket pair generated by the Checksum
 *   Accumulator (Csa) process and evaluates them. Next, if destination
 *   port is open, it requests the Session Lookup Controller (SLc) to
 *   perform a session lookup and waits for its reply. If a session is open
 *   for this socket pair, a new metadata structure is generated and
 *   forwarded to the Finite State Machine (FSm) of the Rx engine.
 *   If the target destination port is not open, the process creates an
 *    event requesting a RST+ACK message to be sent back to the initiating
 *    host.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pMetaDataHandler(
        stream<rxEngineMetaData>    &siCsa_Meta,
        stream<fourTuple>           &siCsa_SockPair,
        stream<sessionLookupReply>  &siSLc_SessLookupRep,
        stream<StsBit>              &siPRt_PortSts,
        stream<sessionLookupQuery>  &soSessLookupReq,
        stream<extendedEvent>       &soSetEvent,
        stream<CmdBit>              &soDropCmd,
        stream<rxFsmMetaData>       &soMeta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName = concat3(THIS_NAME, "/", "Mdh");
    char message[256];

    static rxEngineMetaData     mdh_meta;
    static sessionLookupReply   mdh_sessLookupReply;
    static Ip4Addr              mdh_srcIp4Addr;
    static TcpPort              mdh_dstTcpPort;

    fourTuple    tuple;            // TODO - Upgrade to SocketPair
    StsBit       isPortOpen;

    //OBSOLETE-20181101 enum mhStateType {META, LOOKUP};
    //OBSOLETE-20181101 static mhStateType mdh_state = META;
    static enum FsmState {META=0, LOOKUP} mdh_fsmState;

    switch (mdh_fsmState) {

    case META:
        // Wait until we get a reply from the Port Table (PRt)
        if (!siPRt_PortSts.empty()) {
            //  Read metadata and socket pair
            if (!siCsa_Meta.empty() && !siCsa_SockPair.empty()) {

                siPRt_PortSts.read(isPortOpen);
                siCsa_Meta.read(mdh_meta);
                siCsa_SockPair.read(tuple);

                mdh_srcIp4Addr( 7,  0) = tuple.srcIp(31, 24);
                mdh_srcIp4Addr(15,  8) = tuple.srcIp(23, 16);
                mdh_srcIp4Addr(23, 16) = tuple.srcIp(15,  8);
                mdh_srcIp4Addr(31, 24) = tuple.srcIp( 7,  0);
                mdh_dstTcpPort( 7,  0) = tuple.dstPort(15, 8);
                mdh_dstTcpPort(15,  8) = tuple.dstPort( 7, 0);

                if (!isPortOpen) {
                    // The destination port is closed
                    if (!mdh_meta.rst) {
                        // Reply with RST+ACK and send necessary socket-pair through event
                        fourTuple switchedTuple;
                        switchedTuple.srcIp   = tuple.dstIp;
                        switchedTuple.dstIp   = tuple.srcIp;
                        switchedTuple.srcPort = tuple.dstPort;
                        switchedTuple.dstPort = tuple.srcPort;
                        if (mdh_meta.syn || mdh_meta.fin) {
                            soSetEvent.write(extendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length+1),
                                    switchedTuple)); //always 0
                        }
                        else {
                            soSetEvent.write(extendedEvent(rstEvent(mdh_meta.seqNumb+mdh_meta.length),
                                    switchedTuple));
                        }
                    }
                    else {
                        // The RST bit is set. Ignore => do nothing
                    }

                    if (mdh_meta.length != 0) {
                        soDropCmd.write(DROP_CMD);
                    }

                    if (DEBUG_LEVEL > 1) {
                        sprintf(message, "Port %d is not open.", tuple.dstPort.to_uint());
                        printWarn(myName, message);
                    }
                }
                else {
                    // Query session lookup. Only allow creation of a new entry when SYN or SYN_ACK
                    soSessLookupReq.write(sessionLookupQuery(tuple,
                                                         (mdh_meta.syn && !mdh_meta.rst && !mdh_meta.fin)));
                    mdh_fsmState = LOOKUP;
                    if (DEBUG_LEVEL > 1) {
                        printSockPair(myName,
                            SocketPair(SockAddr(tuple.srcIp, tuple.srcPort),
                                       SockAddr(tuple.dstIp, tuple.dstPort)));
                    }
                }
            }
        }
        break;
    case LOOKUP:
        // Wait until we get a reply from the Port Table (PRt).
        //  Warning: There may be a large delay for the lookup to complete
        if (!siSLc_SessLookupRep.empty()) {
            siSLc_SessLookupRep.read(mdh_sessLookupReply);
            if (mdh_sessLookupReply.hit) {
                //Write out lup and meta
                soMeta.write(rxFsmMetaData(mdh_sessLookupReply.sessionID,
                                           mdh_srcIp4Addr,
                                           mdh_dstTcpPort,
                                           mdh_meta));
            }
            if (mdh_meta.length != 0) {
                soDropCmd.write(!mdh_sessLookupReply.hit);
            }
            mdh_fsmState = META;
        }
        break;

    } // End of: switch
}


/*****************************************************************************
 * @brief Finite State machine (Fsm)
 *
 * @param[in]  siMdh_Meta,        Metadata from MetData Handler (Mdh).
 * @param[in]  siSTt_SessStateRep,Session state reply from State Table (STt).
 * @param[in]  siRSt_SessRxSarRep,Session Rx SAR reply from Rx SAR Table (RSt).
 * @param[in]  siTSt_SessTxSarRep,Session Tx SAR reply from Tx SAR Table (TSt).
 * @param[out] soSessStateReq,    Request to read the session state.
 * @param[out] soSessRxSarReq,    Request to read the session Rx SAR.
 * @param[out] soSessTxSarReq,    Request to read the session Tx SAR.
 * @param[out] soClearReTxTimer,  Clear the retransmit timer.
 * @param[out] soClearProbeTimer, Clear the probing timer.
 * @param[out] soCloseTimer,      Close session timer.
 * @param[out] soSessOpnSts,	  Open status of the session.
 * @param[out] soSetEvent,        Set an event.
 * @param[out] soDropCmd,         Drop command for the process having the segment data.
 * @param[out] soMemWrCmd,        Memory write command.
 * @param[out] soRxNotif,         Rx data notification for the application.
 *
 * @details
 *  This process implements the typical TCP state and metadata management. It
 *   contains all the logic that updates the metadata and keeps track of the
 *   events related to the reception of segments and their handshaking. This is
 *   the key central part of the Rx engine.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pFiniteStateMachine(
        stream<rxFsmMetaData>               &siMdh_Meta,
        stream<sessionState>                &siSTt_SessStateRep,
        stream<rxSarEntry>                  &siRSt_SessRxSarRep,
        stream<rxTxSarReply>                &siTSt_SessTxSarRep,
        stream<stateQuery>                  &soSessStateReq,
        stream<rxSarRecvd>                  &soSessRxSarReq,
        stream<rxTxSarQuery>                &soSessTxSarReq,
        stream<rxRetransmitTimerUpdate>     &soClearReTxTimer,
        stream<ap_uint<16> >                &soClearProbeTimer,
        stream<ap_uint<16> >                &soCloseTimer,
        stream<openStatus>                  &soSessOpnSts, //TODO merge with eventEngine
        stream<event>                       &soSetEvent,
        stream<CmdBit>                      &soDropCmd,
        stream<mmCmd>                       &soMemWrCmd,
        stream<appNotification>             &soRxNotif)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    const char *myName  = concat3(THIS_NAME, "/", "Fsm");

    //OBSOLETE-20181103 enum fsmStateType {LOAD, TRANSITION};
    //OBSOLETE-20181103 static fsmStateType fsm_state = LOAD;

    static rxFsmMetaData fsm_meta;
    static bool          fsm_txSarRequest = false;

    static uint16_t rxEngSynCounter = 0;

    ap_uint<4>         control_bits = 0;
    sessionState     tcpState;
    rxSarEntry         rxSar;
    rxTxSarReply     txSar;

    static enum State {LOAD=0, TRANSITION} fsm_state;

    switch(fsm_state) {
    case LOAD:
        if (!siMdh_Meta.empty()) {
            siMdh_Meta.read(fsm_meta);
            // Request the current state of the session
            soSessStateReq.write(stateQuery(fsm_meta.sessionID));
            // Always request the rxSar, even though not required for SYN-ACK
            soSessRxSarReq.write(rxSarRecvd(fsm_meta.sessionID));

            if (fsm_meta.meta.ack) {
                // Only request the txSar when (ACK+ANYTHING); not for SYN
                soSessTxSarReq.write(rxTxSarQuery(fsm_meta.sessionID));
                fsm_txSarRequest = true;
            }
            fsm_state = TRANSITION;
        }
        break;
    case TRANSITION:
        // Check if transition to LOAD occurs
        if (!siSTt_SessStateRep.empty() && !siRSt_SessRxSarRep.empty() &&
            !(fsm_txSarRequest && siTSt_SessTxSarRep.empty())) {
            fsm_state = LOAD;
            fsm_txSarRequest = false;
        }

        control_bits[0] = fsm_meta.meta.ack;
        control_bits[1] = fsm_meta.meta.syn;
        control_bits[2] = fsm_meta.meta.fin;
        control_bits[3] = fsm_meta.meta.rst;

        switch (control_bits) {
        case 1: // ACK
            if (fsm_state == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_SessRxSarRep.read(rxSar);
                siTSt_SessTxSarRep.read(txSar);
                soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, (fsm_meta.meta.ackNumb == txSar.nextByte)));
                if ( (tcpState == ESTABLISHED) || (tcpState == SYN_RECEIVED) ||
                     (tcpState == FIN_WAIT_1)  || (tcpState == CLOSING)      ||
                     (tcpState == LAST_ACK) ) {
                    // Check if new ACK arrived
                    if ( (fsm_meta.meta.ackNumb == txSar.prevAck) &&
                         (txSar.prevAck != txSar.nextByte) ) {
                        // Not new ACK increase counter
                        txSar.count++;
                    }
                    else {
                        // Notify probeTimer about new ACK
                        soClearProbeTimer.write(fsm_meta.sessionID);

                        // Check for SlowStart & Increase Congestion Window (TODO-CheckDiff)
                        if (txSar.cong_window <= (txSar.slowstart_threshold-MMS)) {
                            txSar.cong_window += MMS;
                        }
                        else if (txSar.cong_window <= 0xF7FF) {
                            txSar.cong_window += 365; //TODO replace by approx. of (MSS x MSS) / cong_window
                        }
                        txSar.count = 0;
                    }

                    // TX SAR (TODO-CheckDiff)
                    if ( (  (txSar.prevAck <= fsm_meta.meta.ackNumb) && (fsm_meta.meta.ackNumb <= txSar.nextByte) ) ||
                         ( ((txSar.prevAck <= fsm_meta.meta.ackNumb) || (fsm_meta.meta.ackNumb <= txSar.nextByte) ) && (txSar.nextByte < txSar.prevAck) ) ) {
                        soSessTxSarReq.write((rxTxSarQuery(fsm_meta.sessionID,
                                                fsm_meta.meta.ackNumb,
                                                fsm_meta.meta.winSize,
                                                txSar.cong_window,
                                                txSar.count,
                                                0)));
                    }

                    // Check if packet contains payload
                    if (fsm_meta.meta.length != 0) {
                        ap_uint<32> newRecvd = fsm_meta.meta.seqNumb + fsm_meta.meta.length;
                        // Second part makes sure that app pointer is not overtaken
                        ap_uint<16> free_space = ((rxSar.appd - rxSar.recvd(15, 0)) - 1);
                        // Check if segment in order and if enough free space is available
                        if ( (fsm_meta.meta.seqNumb == rxSar.recvd) &&
                             (free_space > fsm_meta.meta.length) ) {
                            soSessRxSarReq.write(rxSarRecvd(fsm_meta.sessionID, newRecvd, 1));
                            // Build memory address for this segment
                            ap_uint<32> memSegAddr;
                            memSegAddr(31, 30) = 0x0;
                            memSegAddr(29, 16) = fsm_meta.sessionID(13, 0);
                            memSegAddr(15,  0) = fsm_meta.meta.seqNumb.range(15, 0);
                            soMemWrCmd.write(mmCmd(memSegAddr, fsm_meta.meta.length));
                            // Only notify about new data available
                            soRxNotif.write(appNotification(fsm_meta.sessionID,    fsm_meta.meta.length,
                                                            fsm_meta.srcIpAddress, fsm_meta.dstIpPort));
                            soDropCmd.write(KEEP_CMD);
                        }
                        else {
                            soDropCmd.write(DROP_CMD);
                        }

                        // OBSOLETE-Sent ACK
                        // OBSOLETE-soSetEvent.write(event(ACK, fsm_meta.sessionID));
                    }
                    if (txSar.count == 3) {
                        soSetEvent.write(event(RT, fsm_meta.sessionID));
                    }
                    else if (fsm_meta.meta.length != 0) {
                        soSetEvent.write(event(ACK, fsm_meta.sessionID));
                    }

                    // Reset Retransmit Timer
                    // OBSOLET soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, (mdh_meta.ackNumb == txSarNextByte)));
                    if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                        switch (tcpState) {
                        case SYN_RECEIVED:  //TODO MAYBE REARRANGE
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, ESTABLISHED, 1));
                            break;
                        case CLOSING:
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, TIME_WAIT, 1));
                            soCloseTimer.write(fsm_meta.sessionID);
                            break;
                        case LAST_ACK:
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSED, 1));
                            break;
                        default:
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                            break;
                        }
                    }
                    else { // we have to release the lock
                        // reset rtTimer
                        // OBSOLETE rtTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID));
                        soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1)); // or ESTABLISHED
                    }
                } // End of : if ( (tcpState...

                // TODO if timewait just send ACK, can it be time wait??
                else { // state == (CLOSED || SYN_SENT || CLOSE_WAIT || FIN_WAIT_2 || TIME_WAIT)
                    // SENT RST, RFC 793: fig.11
                    soSetEvent.write(rstEvent(fsm_meta.sessionID, fsm_meta.meta.seqNumb+fsm_meta.meta.length)); // noACK ?
                    // if data is in the pipe it needs to be droppped
                    if (fsm_meta.meta.length != 0) {
                        soDropCmd.write(DROP_CMD);
                    }
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, QUERY_WR));
                }
                //fsm_state = LOAD;
            }
            break;

        case 2: // SYN
            // OBBSOLETE if (!siSTt_SessStateRep.empty())
            if (fsm_state == LOAD) {
                rxEngSynCounter++;
                //std::cerr << "SYN Counter: " << rxEngSynCounter << std::endl;
                siSTt_SessStateRep.read(tcpState);
                siRSt_SessRxSarRep.read(rxSar);
                if (tcpState == CLOSED || tcpState == SYN_SENT) {
                	// Actually this is LISTEN || SYN_SENT
                    // Initialize rxSar, SEQ + phantom byte, last '1' for makes sure appd is initialized
                    soSessRxSarReq.write(rxSarRecvd(fsm_meta.sessionID, fsm_meta.meta.seqNumb + 1, 1, 1));
                    // Initialize receive window
                    soSessTxSarReq.write((rxTxSarQuery(fsm_meta.sessionID, 0, fsm_meta.meta.winSize,
                                          txSar.cong_window, 0, 1))); //TODO maybe include count check
                    // Set SYN_ACK event
                    soSetEvent.write(event(SYN_ACK, fsm_meta.sessionID));
                    // Change State to SYN_RECEIVED
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, SYN_RECEIVED, 1));
                }
                else if (tcpState == SYN_RECEIVED) { // && mdh_meta.seqNumb+1 == rxSar.recvd) // Maybe Check for seq
                    // If it is the same SYN, we resent SYN-ACK, almost like quick RT, we could also wait for RT timer
                    if (fsm_meta.meta.seqNumb+1 == rxSar.recvd) {
                        // Retransmit SYN_ACK
                        soSetEvent.write(event(SYN_ACK, fsm_meta.sessionID, 1));
                        soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                    }
                    else { // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                        soSetEvent.write(rstEvent(fsm_meta.sessionID, fsm_meta.meta.seqNumb+1)); //length == 0
                        soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSED, 1));
                    }
                }
                else { // Any synchronized state
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soSetEvent.write(event(ACK_NODELAY, fsm_meta.sessionID));
                    // TODo send RST, has no ACK??
                    // Respond with RST, no ACK, seq ==
                    //eventEngine.write(rstEvent(mdh_meta.seqNumb, mh_meta.length, true));
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                }
            }
            break;

        case 3: // SYN_ACK
            //OBSOLETE if (!siSTt_SessStateRep.empty() && !siTSt_SessTxSarRep.empty())
            if (fsm_state == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_SessRxSarRep.read(rxSar);
                siTSt_SessTxSarRep.read(txSar);
                soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID,
                                                               (fsm_meta.meta.ackNumb == txSar.nextByte)));
                if ( (tcpState == SYN_SENT) && (fsm_meta.meta.ackNumb == txSar.nextByte) ) { // && !mh_lup.created)
                    //initialize rx_sar, SEQ + phantom byte, last '1' for appd init
                    soSessRxSarReq.write(rxSarRecvd(fsm_meta.sessionID,
                                                    fsm_meta.meta.seqNumb + 1, 1, 1));
                    soSessTxSarReq.write((rxTxSarQuery(fsm_meta.sessionID,
                                                       fsm_meta.meta.ackNumb,
                                                       fsm_meta.meta.winSize,
                                                       txSar.cong_window, 0, 1))); //CHANGE this was added //TODO maybe include count check
                    // Set ACK event
                    soSetEvent.write(event(ACK_NODELAY, fsm_meta.sessionID));

                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, ESTABLISHED, 1));
                    soSessOpnSts.write(openStatus(fsm_meta.sessionID, true));
                }
                else if (tcpState == SYN_SENT) { //TODO correct answer?
                    // Sent RST, RFC 793: fig.9 (old) duplicate SYN(+ACK)
                    soSetEvent.write(rstEvent(fsm_meta.sessionID,
                                              fsm_meta.meta.seqNumb+fsm_meta.meta.length+1));
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSED, 1));
                }
                else {
                    // Unexpected SYN arrived, reply with normal ACK, RFC 793: fig.10
                    soSetEvent.write(event(ACK_NODELAY, fsm_meta.sessionID));
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                }
            }
            break;

        case 5: //FIN (_ACK)
            //OBSOLETE if (!siRSt_SessRxSarRep.empty() && !siSTt_SessStateRep.empty() && !siTSt_SessTxSarRep.empty())
            if (fsm_state == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_SessRxSarRep.read(rxSar);
                siTSt_SessTxSarRep.read(txSar);
                soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID,
                                       (fsm_meta.meta.ackNumb == txSar.nextByte)));
                // Check state and if FIN in order, Current out of order FINs are not accepted
                if ( (tcpState == ESTABLISHED || tcpState == FIN_WAIT_1 ||
                      tcpState == FIN_WAIT_2) && (rxSar.recvd == fsm_meta.meta.seqNumb) ) {
                    soSessTxSarReq.write((rxTxSarQuery(fsm_meta.sessionID,
                                          fsm_meta.meta.ackNumb, fsm_meta.meta.winSize,
                                          txSar.cong_window, txSar.count, 0))); //TODO include count check

                    // +1 for phantom byte, there might be data too
                    soSessRxSarReq.write(rxSarRecvd(fsm_meta.sessionID, fsm_meta.meta.seqNumb+fsm_meta.meta.length+1, 1)); //diff to ACK

                    // Clear the probe timer
                    soClearProbeTimer.write(fsm_meta.sessionID);

                    // Check if there is payload
                    if (fsm_meta.meta.length != 0) {
                        ap_uint<32> pkgAddr;
                        pkgAddr(31, 30) = 0x0;
                        pkgAddr(29, 16) = fsm_meta.sessionID(13, 0);
                        pkgAddr(15,  0) = fsm_meta.meta.seqNumb(15, 0);
                        soMemWrCmd.write(mmCmd(pkgAddr, fsm_meta.meta.length));
                        // Tell Application new data is available and connection got closed
                        soRxNotif.write(appNotification(fsm_meta.sessionID,    fsm_meta.meta.length,
                                                        fsm_meta.srcIpAddress, fsm_meta.dstIpPort, true));
                        soDropCmd.write(KEEP_CMD);
                    }
                    else if (tcpState == ESTABLISHED) {
                        // Tell Application connection got closed
                        soRxNotif.write(appNotification(fsm_meta.sessionID, fsm_meta.srcIpAddress,
                                                        fsm_meta.dstIpPort, true)); //CLOSE
                    }

                    // Update state
                    if (tcpState == ESTABLISHED) {
                        soSetEvent.write(event(FIN, fsm_meta.sessionID));
                        soSessStateReq.write(stateQuery(fsm_meta.sessionID, LAST_ACK, 1));
                    }
                    else { //FIN_WAIT_1 || FIN_WAIT_2
                        if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                            // check if final FIN is ACK'd -> LAST_ACK
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, TIME_WAIT, 1));
                            soCloseTimer.write(fsm_meta.sessionID);
                        }
                        else {
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSING, 1));
                        }
                        soSetEvent.write(event(ACK, fsm_meta.sessionID));
                    }
                }
                else { // NOT (ESTABLISHED || FIN_WAIT_1 || FIN_WAIT_2)
                    soSetEvent.write(event(ACK, fsm_meta.sessionID));
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                    // If there is payload we need to drop it
                    if (fsm_meta.meta.length != 0) {
                        soDropCmd.write(DROP_CMD);
                    }
                }
            }
            break;

        default: //TODO MAYBE load everything all the time
            // stateTable is locked, make sure it is released in at the end
            // If there is an ACK we read txSar
            // We always read rxSar
            if (fsm_state == LOAD) {
                siSTt_SessStateRep.read(tcpState);
                siRSt_SessRxSarRep.read(rxSar); //TODO not sure nb works
                siTSt_SessTxSarRep.read_nb(txSar);
            }
            if (fsm_state == LOAD) {
                // Handle if RST
                if (fsm_meta.meta.rst) {
                    if (tcpState == SYN_SENT) { //TODO this would be a RST,ACK i think
                    	// Check if matching SYN
                        if (fsm_meta.meta.ackNumb == txSar.nextByte) {
                            // Tell application, could not open connection
                            soSessOpnSts.write(openStatus(fsm_meta.sessionID, false));
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSED, 1));
                            soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, true));
                        }
                        else {
                            // Ignore since not matching
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                        }
                    }
                    else {
                        // Check if in window
                        if (fsm_meta.meta.seqNumb == rxSar.recvd) {
                            //tell application, RST occurred, abort
                            soRxNotif.write(appNotification(fsm_meta.sessionID, fsm_meta.srcIpAddress, fsm_meta.dstIpPort, true)); //RESET
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, CLOSED, 1)); //TODO maybe some TIME_WAIT state
                            soClearReTxTimer.write(rxRetransmitTimerUpdate(fsm_meta.sessionID, true));
                        }
                        else {
                            // Ignore since not matching window
                            soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                        }
                    }
                }
                else { // Handle non RST bogus packages
                    //TODO maybe sent RST ourselves, or simply ignore
                    // For now ignore, sent ACK??
                    //eventsOut.write(rstEvent(mh_meta.seqNumb, 0, true));
                    soSessStateReq.write(stateQuery(fsm_meta.sessionID, tcpState, 1));
                } // End of: if (fsm_meta.meta.rst)
            } // if fsm_stat
            break;

        } // End of: switch control_bits

        break;

    } // End of: switch state
}


/*****************************************************************************
 * @brief TCP Segment Dropper (Tsd)
 *
 * @param[in]  siTid_Data,      TCP data stream from Tcp Invalid Dropper (Tid).
 * @param[in]  siMdh_DropCmd,   Drop command from MetaData Handler (Mdh).
 * @param[in]  siFsm_DropCmd,   Drop command from FiniteState Machine (Fsm).
 * @param[out] soData,          TCP data stream.
 *
 * @details
 *  Drops TCP segments when their metadata did not match and/or is invalid.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pTcpSegmentDropper(
        stream<AxiWord>     &siTid_Data,
        stream<CmdBit>      &siMdh_DropCmd,
        stream<CmdBit>      &siFsm_DropCmd,
        stream<AxiWord>     &soData)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    static enum FsmState {FSM_RD_DROP_CMD1=0, FSM_RD_DROP_CMD2, FSM_FWD, FSM_DROP} tsd_fsmState = FSM_RD_DROP_CMD1;

    static ap_uint<10>  rxBuffWrAccessLength = 0;
    static ap_uint<1>   rxBuffWriteDoubleAccess = 0;

    switch (tsd_fsmState) {

    case FSM_RD_DROP_CMD1:
        if (!siMdh_DropCmd.empty()) {
            CmdBit dropCmd = siMdh_DropCmd.read();
            (dropCmd) ? tsd_fsmState = FSM_DROP : tsd_fsmState = FSM_RD_DROP_CMD2;
        }
        break;
    case FSM_RD_DROP_CMD2:
        if (!siFsm_DropCmd.empty()) {
            CmdBit dropCmd = siFsm_DropCmd.read();
            (dropCmd) ? tsd_fsmState = FSM_DROP : tsd_fsmState = FSM_FWD;
        }
        break;
    case FSM_FWD:
        if(!siTid_Data.empty() && !soData.full()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast)
                tsd_fsmState = FSM_RD_DROP_CMD1;
            soData.write(currWord);
        }
        break;
    case FSM_DROP:
        if(!siTid_Data.empty()) {
            AxiWord currWord = siTid_Data.read();
            if (currWord.tlast)
                tsd_fsmState = FSM_RD_DROP_CMD1;
        }
        break;

    } // End of: switch
}


/*****************************************************************************
 * @brief Rx Application Notifier (Ran)
 *
 * @param[in]  siMEM_WrSts,    The memory write status from MEM.
 * @param[in]  siFsm_Notif,    Rx data notification from Finite State Machine (Fsm).
 * @param[out] soRxNotif,      Outgoing Rx notification for the application.
 *
 * @details
 *  Delays the notifications to the application until the data is actually
 *   written into the physical DRAM memory.
 *
 * @todo
 *  Handle unsuccessful write to memory.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pRxAppNotifier(
        stream<mmStatus>            &siMEM_WrSts,
        stream<appNotification>     &siFsm_Notif,
        stream<appNotification>     &soRxNotif,
        stream<ap_uint<1> >         &doubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
    #pragma HLS pipeline II=1

    //-- LOCAL STREAMS
    static stream<appNotification> sRxNotifFifo("sRxNotifFifo");
    #pragma HLS STREAM    variable=sRxNotifFifo depth=32 // Depends on memory delay
    #pragma HLS DATA_PACK variable=sRxNotifFifo

    static ap_uint<1>       rxAppNotificationDoubleAccessFlag = false;
    static ap_uint<5>       rand_fifoCount = 0;
    static mmStatus         rxAppNotificationStatus1, rxAppNotificationStatus2;
    static appNotification  rxAppNotification;

    if (rxAppNotificationDoubleAccessFlag == true) {
        if(!siMEM_WrSts.empty()) {
            siMEM_WrSts.read(rxAppNotificationStatus2);
            rand_fifoCount--;
            if (rxAppNotificationStatus1.okay && rxAppNotificationStatus2.okay)
                soRxNotif.write(rxAppNotification);
            rxAppNotificationDoubleAccessFlag = false;
        }
    }
    else if (rxAppNotificationDoubleAccessFlag == false) {
        if(!siMEM_WrSts.empty() && !sRxNotifFifo.empty() && !doubleAccess.empty()) {
            siMEM_WrSts.read(rxAppNotificationStatus1);
            sRxNotifFifo.read(rxAppNotification);
            // Read the double notification flag. If one then go and w8 for the second status
            rxAppNotificationDoubleAccessFlag = doubleAccess.read();
            if (rxAppNotificationDoubleAccessFlag == 0) {
                // The memory access was not broken down in two for this segment
                rand_fifoCount--;
                if (rxAppNotificationStatus1.okay) {
                    // Output the notification
                    soRxNotif.write(rxAppNotification);
                }
            }
            //TODO else, we are screwed since the ACK is already sent
        }
        else if (!siFsm_Notif.empty() && (rand_fifoCount < 31)) {
            siFsm_Notif.read(rxAppNotification);
            if (rxAppNotification.length != 0) {
                sRxNotifFifo.write(rxAppNotification);
                rand_fifoCount++;
            }
            else
                soRxNotif.write(rxAppNotification);
        }
    }
}

/*****************************************************************************
 * @brief Event Multiplexer (Evm)
 *
 * @param[in]  siCsa_Data,      TCP data stream from Checksum Accumulator (Csa).
 * @param[in]  siCsa_DataVal,   TCP data valid.
 * @param[out] soData,          TCP data stream.
 *
 * @details
 *  Takes two extended events as inputs and mux them on a single output. Note
 *   that the first input has priority over the second one.
 *
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pEventMultiplexer(
        stream<extendedEvent>    &siEvent1,
        stream<event>            &siEvent2,
        stream<extendedEvent>    &soEvent)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE

    if (!siEvent1.empty()) {
        soEvent.write(siEvent1.read());
    }
    else if (!siEvent2.empty()) {
        soEvent.write(siEvent2.read());
    }
}

/*****************************************************************************
 * @brief Memory Writer (Mwr)
 *
 * @param[in]  siTsd_MemWrData, Memory data write from the Tcp Segment Dropper (Tid).
 * @param[in]  siFsm_MemWrCmd,  Memory write command from the Finite State Machine (Fsm).
 * @param[out] soMemWrCmd,      Memory write command to DRAM controller.
 * @param[out] soMemWrData,     Memory data write stream to DRAM controller.
 * @param[out] soDoubleAccess,  TODO
 *
 * @details
 *  Front memory controller process for writing data into the external DRAM.
 *
 * @ingroup rx_engine
 *****************************************************************************/
void pMemWriter(
        stream<AxiWord>     &siTid_MemWrData,
        stream<mmCmd>       &siFsm_MemWrCmd,
        stream<mmCmd>       &soMemWrCmd,
        stream<axiWord>     &soMemWrData,
        stream<ap_uint<1> > &soDoubleAccess)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS pipeline II=1
    #pragma HLS INLINE off

    static mmCmd       rxMemWriterCmd = mmCmd(0, 0);
    static ap_uint<16> rxEngBreakTemp = 0;
    static uint8_t     lengthBuffer   = 0;
    static ap_uint<3>  rxEngAccessResidue = 0;
    static bool        txAppBreakdown = false;

    static AxiWord     pushWord = AxiWord(0, 0xFF, 0);

    //OBSOLETE static uint16_t txAppPktCounter = 0;
    //OBSOLETE static uint16_t txAppWordCounter = 0;

    static enum FsmState {RXMEMWR_IDLE,       RXMEMWR_WRFIRST,
                          RXMEMWR_EVALSECOND, RXMEMWR_WRSECONDSTR,
                          RXMEMWR_ALIGNED,    RXMEMWR_RESIDUE} mwr_fsmState;

    switch (mwr_fsmState) {

    case RXMEMWR_IDLE:
        if (!siFsm_MemWrCmd.empty() && !soMemWrCmd.full() && !soDoubleAccess.full()) {
            rxMemWriterCmd = siFsm_MemWrCmd.read();
            mmCmd tempCmd = rxMemWriterCmd;
            if ((rxMemWriterCmd.saddr.range(15, 0) + rxMemWriterCmd.bbt) > 65536) {
                rxEngBreakTemp = 65536 - rxMemWriterCmd.saddr;
                rxMemWriterCmd.bbt -= rxEngBreakTemp;
                tempCmd = mmCmd(rxMemWriterCmd.saddr, rxEngBreakTemp);
                txAppBreakdown = true;
            }
            else
                rxEngBreakTemp = rxMemWriterCmd.bbt;
            soMemWrCmd.write(tempCmd);
            soDoubleAccess.write(txAppBreakdown);
            //txAppPktCounter++;
            //std::cerr <<  "Cmd: " << std::dec << txAppPktCounter << " - " << std::hex << tempCmd.saddr << " - " << tempCmd.bbt << std::endl;
            mwr_fsmState = RXMEMWR_WRFIRST;
        }
        break;

    case RXMEMWR_WRFIRST:
        if (!siTid_MemWrData.empty() && !soMemWrData.full()) {
            siTid_MemWrData.read(pushWord);
            axiWord outputWord = axiWord(pushWord.tdata, pushWord.tkeep, pushWord.tlast);
            ap_uint<4> byteCount = keepMapping(pushWord.tkeep);
            if (rxEngBreakTemp > 8)
                rxEngBreakTemp -= 8;
            else {
                if (txAppBreakdown == true) {
                    // Changes are to go in here
                    // If the word is not perfectly aligned then there is some magic to be worked.
                    if (rxMemWriterCmd.saddr.range(15, 0) % 8 != 0)
                        outputWord.keep = returnKeep(rxEngBreakTemp);
                    outputWord.last = 1;
                    mwr_fsmState = RXMEMWR_EVALSECOND;
                    rxEngAccessResidue = byteCount - rxEngBreakTemp;
                    lengthBuffer = rxEngBreakTemp;  // Buffer the number of bits consumed.
                }
                else
                    mwr_fsmState = RXMEMWR_IDLE;
            }
            //txAppWordCounter++;
            //std::cerr <<  std::dec << cycleCounter << " - " << txAppWordCounter << " - " << std::hex << pushWord.data << std::endl;
            soMemWrData.write(outputWord);
        }
        break;

    case RXMEMWR_EVALSECOND:
        if (!soMemWrCmd.full()) {
            if (rxMemWriterCmd.saddr.range(15, 0) % 8 == 0)
                mwr_fsmState = RXMEMWR_ALIGNED;
            //else if (rxMemWriterCmd.bbt + rxEngAccessResidue > 8 || rxEngAccessResidue > 0)
            else if (rxMemWriterCmd.bbt - rxEngAccessResidue > 0)
                mwr_fsmState = RXMEMWR_WRSECONDSTR;
            else
                mwr_fsmState = RXMEMWR_RESIDUE;
            rxMemWriterCmd.saddr.range(15, 0) = 0;
            rxEngBreakTemp = rxMemWriterCmd.bbt;
            soMemWrCmd.write(mmCmd(rxMemWriterCmd.saddr, rxEngBreakTemp));
            //std::cerr <<  "Cmd: " << std::dec << txAppPktCounter << " - " << std::hex << txAppTempCmd.saddr << " - " << txAppTempCmd.bbt << std::endl;
            txAppBreakdown = false;
        }
        break;

    case RXMEMWR_ALIGNED:   // This is the non-realignment state
        if (!siTid_MemWrData.empty() & !soMemWrData.full()) {
            siTid_MemWrData.read(pushWord);
            soMemWrData.write(axiWord(pushWord.tdata, pushWord.tkeep, pushWord.tlast));
            if (pushWord.tlast == 1)
                mwr_fsmState = RXMEMWR_IDLE;
        }
        break;

    case RXMEMWR_WRSECONDSTR: // We go into this state when we need to realign things
        if (!siTid_MemWrData.empty() && !soMemWrData.full()) {
            axiWord outputWord = axiWord(0, 0xFF, 0);
            outputWord.data.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            pushWord = siTid_MemWrData.read();
            outputWord.data.range(63, (8-lengthBuffer)*8) = pushWord.tdata.range((lengthBuffer * 8), 0 );

            if (pushWord.tlast == 1) {
                if (rxEngBreakTemp - rxEngAccessResidue > lengthBuffer) { // In this case there's residue to be handled
                    rxEngBreakTemp -= 8;
                    mwr_fsmState = RXMEMWR_RESIDUE;
                }
                else {
                    outputWord.keep = returnKeep(rxEngBreakTemp);
                    outputWord.last = 1;
                    mwr_fsmState = RXMEMWR_IDLE;
                }
            }
            else
                rxEngBreakTemp -= 8;
            soMemWrData.write(outputWord);
        }
        break;

    case RXMEMWR_RESIDUE:
        if (!soMemWrData.full()) {
            axiWord outputWord = axiWord(0, returnKeep(rxEngBreakTemp), 1);
            outputWord.data.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.tdata.range(63, lengthBuffer*8);
            soMemWrData.write(outputWord);
            mwr_fsmState = RXMEMWR_IDLE;
        }
        break;

    }
}


/*****************************************************************************
 * @brief The rx_engine processes the data packets received from IPRX.
 *    When a new packet enters the engine its TCP checksum is tested, the
 *    header is parsed and some more checks are done. Next, it is evaluated
 *    by the main TCP state machine which triggers events and updates the data
 *    structures according to the type of received packet. Finally, if the
 *    packet contains valid payload, it is stored in external DDR4 memory and
 *    the application is notified about the arrival of new data.
 *
 *
 *
 * @param[in]  siIPRX_Data,         IP4 data stream form IPRX.
 * @param[in]  siSLc_SessLookupRep, Session lookup lookup reply from SLc.
 * @param[in]  siSTt_SessStateRep,  Session state reply
 * @param[in]  siPRt_PortSts,       Port state (open/close) from Port Table (PRt).
 * @param[in]  siRSt_SessRxSarRep   Session Rx SAR reply form Rx SAR Table (RSt).
 * @param[in]  siTSt_SessTxSarRep   Session Tx SAR reply from Tx SAR Table (TSt).
 * @param[in]  siMEM_WrSts,         Memory write status from MEM.
 * @param[out] soMemWrData,         Memory data write stream to MEM controller.
 * @param[out] soSessStateReq,      Session state request.
 * @param[out] soDstPort,           TCP destination port number.
 * @param[out] soSessLookupReq,     Session lookup request.
 * @param[out] soSessRxSarReq,      Request to read the session Rx SAR.
 * @param[out] soSessTxSarReq,      Request to read the session Tx SAR.
 * @param[out] soClearReTxTimer,    Clear the retransmit timer.
 * @param[out] soClearProbeTimer,   Clear the probe timer.
 * @param[out] soCloseTimer,        Close session timer.
 * @param[out] soSessOpnStatus,     Open status of the session.
 * @param[out] soSetEvent,          Set an event.
 * @param[out] soMemWrCmd,          Memory write command,
 * @param[out] soRxNotification,    Rx data notification for the application.
 *
 * @return Nothing.
 *
 * @ingroup rx_engine
 ******************************************************************************/
void rx_engine(
        stream<Ip4Word>                 &siIPRX_Pkt,
        stream<sessionLookupReply>      &siSLc_SessLookupRep,
        stream<sessionState>            &siSTt_SessStateRep,
        stream<StsBit>                  &siPRt_PortSts,
        stream<rxSarEntry>              &siRSt_SessRxSarRep,
        stream<rxTxSarReply>            &siTSt_SessTxSarRep,
        stream<mmStatus>                &siMEM_WrSts,
        stream<axiWord>                 &soMemWrData,
        stream<stateQuery>              &soSessStateReq,
        stream<TcpPort>                 &soDstPort,
        stream<sessionLookupQuery>      &soSessLookupReq,
        stream<rxSarRecvd>              &soSessRxSarReq,
        stream<rxTxSarQuery>            &soSessTxSarReq,
        stream<rxRetransmitTimerUpdate> &soClearReTxTimer,
        stream<ap_uint<16> >            &soClearProbeTimer,
        stream<ap_uint<16> >            &soCloseTimer,
        stream<openStatus>              &soSessOpnStatus,
        stream<extendedEvent>           &soSetEvent,
        stream<mmCmd>                   &soMemWrCmd,
        stream<appNotification>         &soRxNotification)
{

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    //#pragma HLS DATAFLOW
    //#pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INLINE

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-- Tcp Length Extract (Tle) ---------------------------------------------
    static stream<TcpWord>          sTleToIph_TcpSeg       ("sTleToIph_TcpSeg");
    #pragma HLS stream     variable=sTleToIph_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=sTleToIph_TcpSeg

    static stream<TcpSegLen>        sTleToIph_TcpSegLen    ("sTleToIph_TcpSegLen");
    #pragma HLS stream     variable=sTleToIph_TcpSegLen    depth=2

    //-- Insert Pseudo Header (Iph) -------------------------------------------
    static stream<TcpWord>          sIphToCsa_TcpSeg       ("sIphToCsa_TcpSeg");
    #pragma    HLS stream  variable=sIphToCsa_TcpSeg       depth=8
    #pragma HLS DATA_PACK  variable=sIphToCsa_TcpSeg

    //-- CheckSum Accumulator (Csa) -------------------------------------------
    static stream<TcpWord>          sCsaToTid_Data         ("sCsaToTid_Data");
    #pragma HLS stream     variable=sCsaToTid_Data         depth=256 //critical, tcp checksum computation
    #pragma HLS DATA_PACK  variable=sCsaToTid_Data

    static stream<bool>             sCsaToTid_DataValid    ("sCsaToTid_DataValid");
    #pragma HLS stream     variable=sCsaToTid_DataValid    depth=2

    static stream<rxEngineMetaData> sCsaToMdh_Meta         ("sCsaToTid_Meta");
    #pragma HLS stream     variable=sCsaToMdh_Meta         depth=2
    #pragma HLS DATA_PACK  variable=sCsaToMdh_Meta

    static stream<fourTuple>        sCsaToMdh_SockPair     ("rx_tupleBuffer");
    #pragma HLS stream     variable=sCsaToMdh_SockPair     depth=2
    #pragma HLS DATA_PACK  variable=sCsaToMdh_SockPair

    static stream<AxiWord>          sTidToTsd_Data         ("sTidToTsd_Data");
    #pragma HLS stream     variable=sTidToTsd_Data         depth=8
    #pragma HLS DATA_PACK  variable=sTidToTsd_Data

    //-- Tcp Invalid dropper (Tid) --------------------------------------------
    static stream<AxiWord>          sTsdToMwr_Data         ("sTsdToMwr_Data");
    #pragma HLS stream     variable=sTsdToMwr_Data         depth=16
    #pragma HLS DATA_PACK  variable=sTsdToMwr_Data

    //-- MetaData Handler (Mdh) -----------------------------------------------
    static stream<extendedEvent>    sMdhToEvm_SetEvent     ("sMdhToEvm_SetEvent");
    #pragma HLS stream     variable=sMdhToEvm_SetEvent     depth=2
    #pragma HLS DATA_PACK  variable=sMdhToEvm_SetEvent

    static stream<CmdBit>           sMdhToTsd_DropCmd      ("sMdhToTsd_DropCmd");
    #pragma HLS stream     variable=sMdhToTsd_DropCmd      depth=2
    #pragma HLS DATA_PACK  variable=sMdhToTsd_DropCmd

    static stream<rxFsmMetaData>    sMdhToFsm_Meta         ("sMdhToFsm_Meta");  // FIXME- Did we loose FiFo_Depth?

    //-- Finite State Machine (Fsm) -------------------------------------------
    static stream<CmdBit>           sFsmToTsd_DropCmd      ("sFsmToTsd_DropCmd");
    #pragma HLS stream     variable=sFsmToTsd_DropCmd      depth=2
    #pragma HLS DATA_PACK  variable=sFsmToTsd_DropCmd

    static stream<appNotification>  sFsmToRan_Notif        ("sFsmToRan_Notif");
    #pragma HLS stream     variable=sFsmToRan_Notif        depth=8  // This depends on the memory delay
    #pragma HLS DATA_PACK  variable=sFsmToRan_Notif

    static stream<event>            sFsmToEvm_SetEvent     ("sFsmToEvm_SetEvent");
    #pragma HLS stream     variable=sFsmToEvm_SetEvent     depth=2
    #pragma HLS DATA_PACK  variable=sFsmToEvm_SetEvent

    static stream<mmCmd>            sFsmToMwr_WrCmd        ("sFsmToMwr_WrCmd");
    #pragma HLS stream     variable=sFsmToMwr_WrCmd        depth=8
    #pragma HLS DATA_PACK  variable=sFsmToMwr_WrCmd

    //-- Memory Writer (Mwr) --------------------------------------------------
    static stream<ap_uint<1> >      sMwrToRan_DoubleAccess ("sMwrToRan_DoubleAccess");
    #pragma HLS stream     variable=sMwrToRan_DoubleAccess depth=8

    //-------------------------------------------------------------------------
    //-- PROCESS FUNCTIONS
    //-------------------------------------------------------------------------

    pTcpLengthExtract(
            siIPRX_Pkt,
            sTleToIph_TcpSeg,
            sTleToIph_TcpSegLen);

    pInsertPseudoHeader(
            sTleToIph_TcpSeg,
            sTleToIph_TcpSegLen,
            sIphToCsa_TcpSeg);

    pCheckSumAccumulator(
            sIphToCsa_TcpSeg,
            sCsaToTid_Data,
            sCsaToTid_DataValid,
            sCsaToMdh_Meta,
            sCsaToMdh_SockPair,
            soDstPort);

    pTcpInvalidDropper(
            sCsaToTid_Data,
            sCsaToTid_DataValid,
            sTidToTsd_Data);

    pMetaDataHandler(
            sCsaToMdh_Meta,
            sCsaToMdh_SockPair,
            siSLc_SessLookupRep,
            siPRt_PortSts,
            soSessLookupReq,
            sMdhToEvm_SetEvent,
            sMdhToTsd_DropCmd,
            sMdhToFsm_Meta);

    pFiniteStateMachine(
            sMdhToFsm_Meta,
            siSTt_SessStateRep,
            siRSt_SessRxSarRep,
            siTSt_SessTxSarRep,
            soSessStateReq,
            soSessRxSarReq,
            soSessTxSarReq,
            soClearReTxTimer,
            soClearProbeTimer,
            soCloseTimer,
            soSessOpnStatus,
            sFsmToEvm_SetEvent,
            sFsmToTsd_DropCmd,
            sFsmToMwr_WrCmd,
            sFsmToRan_Notif);

    pTcpSegmentDropper(
            sTidToTsd_Data,
            sMdhToTsd_DropCmd,
            sFsmToTsd_DropCmd,
            sTsdToMwr_Data);

    pMemWriter(
            sTsdToMwr_Data,
            sFsmToMwr_WrCmd,
            soMemWrCmd,
            soMemWrData,
            sMwrToRan_DoubleAccess);

    pRxAppNotifier(
            siMEM_WrSts,
            sFsmToRan_Notif,
            soRxNotification,
            sMwrToRan_DoubleAccess);

    pEventMultiplexer(
            sMdhToEvm_SetEvent,
            sFsmToEvm_SetEvent,
            soSetEvent);

}
