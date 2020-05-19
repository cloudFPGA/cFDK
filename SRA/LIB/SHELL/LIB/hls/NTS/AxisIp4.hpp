/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 * @file       : AxisIp4.hpp
 * @brief      : A class to access an IPv4 data chunk transmitted over an
 *                AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details : The Internet Protocol version 4 (IPv4) fields defined in this
 *  class refer to the format generated by the 10GbE MAC of Xilinx which
 *  organizes its two 64-bit Rx and Tx interfaces into 8 lanes (see PG157).
 *  The result of this division into lanes, is that the ETH fields end up
 *  being stored in LITTLE-ENDIAN order instead of the initial big-endian
 *  order used to transmit bytes over the physical media.
 *  As an example, assume that the 16-bits field "TotalLength" of an IPv4
 *  packet has a value of '0x1234'. This field will be transmitted on the
 *  media in big-endian order .i.e, a '0x12' followed by '0x34'. However,
 *  this field will end up being ordered in little-endian mode (.i.e, 0x3412)
 *  by the AXI4-Stream interface of the 10GbE MAC.
 *
 *  Therefore, the mapping of an IPv4 packet onto the AXI4-Stream interface
 *  encoded in LITTLE-ENDIAN is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag. Offset  |Flags|         |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                Options (if IHL>5) or Data                     |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 *  And the format of a TCP segment over an IPV4 packet (w/o options) is as follow:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag. Offset  |Flags|         |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Destination Port        |          Source Port          |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Acknowledgment Number                      |                        Sequence Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               |                               |                               |   |U|A|P|R|S|F|  Data |       |
 *  |         Urgent Pointer        |           Checksum            |            Window             |   |R|C|S|S|Y|I| Offset|  Res  |
 *  |                               |                               |                               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  And the format of a UDP datagram over an IPv4 packet (w/o options) is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag Ofst (L) |Flags|  FO(H)  |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Destination Port        |          Source Port          |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             Data                              |           Checksum            |           Length              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  And the format of an ICMP message over an IPv4 packet (w/o options) is as follows:
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
 * * @info: What's this Little-Endian(LE) vs Big-Endian(BE) anyhow.
 *  FYI - The original source from Xilinx (available at:
 *   https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip) is
 *   entirely coded with respect to the above mapping of the media network
 *   stream  over a 64-bits interface in little-endian oder. This makes the initial
 *   code particularly difficult to read, maintain and test. Therefore, this
 *   class implements a set of methods to hide this complexity by accessing a
 *   raw Axis data streams as if it was encoded in the expected big-endian order.
 *
 * @warning : This class is to be used when a IPv4 packet is aligned to a
 *  64-bit quadword. Refer to the methods of 'AxisEth.hpp' to access the fields
 *  of an IPv4 embedded into an Ethernet frame.
 *****************************************************************************/

#ifndef AXIS_IP4_H_
#define AXIS_IP4_H_

#include "AxisRaw.hpp"
#include "AxisIcmp.hpp"
#include "AxisTcp.hpp"
#include "AxisUdp.hpp"


/*********************************************************
 * IP4 - DEFINITIONS
 *********************************************************/

// An IP4 packet has a default header size of 20 bytes. This length may vary in
// the presence of option(s) and is indicated by the field IHL>5.
#define IP4_HEADER_LEN      20      // In bytes

/*********************************************************
 * IPv4 - HEADER FIELDS IN LITTLE-ENDIAN (LE) ORDER.
 *   As received or transmitted by the 10GbE MAC.
 *********************************************************/
typedef ap_uint< 4> LE_Ip4Version;  // IPv4 Version from the MAC
typedef ap_uint< 4> LE_Ip4HdrLen;   // IPv4 Internet Header Length from the MAC
typedef ap_uint< 8> LE_Ip4ToS;      // IPv4 Type of Service from the MAC
typedef ap_uint<16> LE_Ip4TotalLen; // IPv4 Total Length from the MAC
typedef ap_uint< 8> LE_Ip4TtL;      // IPv4 Time to Live
typedef ap_uint< 8> LE_Ip4Prot;     // IPv4 Protocol
typedef ap_uint<16> LE_Ip4HdrCsum;  // IPv4 Header Checksum from the MAC.
typedef ap_uint<32> LE_Ip4SrcAddr;  // IPv4 Source Address from the MAC
typedef ap_uint<32> LE_Ip4DstAddr;  // IPv4 Destination Address from the MAC
typedef ap_uint<32> LE_Ip4Address;  // IPv4 Source or Destination Address from the MAC
typedef ap_uint<32> LE_Ip4Addr;     // IPv4 Source or Destination Address from the MAC
typedef ap_uint<64> LE_IpData;      // IPv4 Data stream from the MAC

/*********************************************************
 * IPv4 - HEADER FIELDS IN NETWORK BYTE ORDER.
 *  Default Type Definitions (as used by HLS).
 *********************************************************/
typedef ap_uint< 4> Ip4Version;     // IPv4 Version
typedef ap_uint< 4> Ip4HdrLen;      // IPv4 Header Length in octets (same as 4*Ip4HeaderLen)
typedef ap_uint< 8> Ip4ToS;         // IPv4 Type of Service
typedef ap_uint<16> Ip4TotalLen;    // IPv4 Total  Length
typedef ap_uint<16> Ip4Ident;       // IPv4 Identification
typedef ap_uint<13> Ip4FragOff;     // IPv4 Fragment Offset
typedef ap_uint< 3> Ip4Flags;       // IPv4 Flags
typedef ap_uint< 8> Ip4TtL;         // IPv4 Time to Live
typedef ap_uint< 8> Ip4Prot;        // IPv4 Protocol
typedef ap_uint<16> Ip4HdrCsum;     // IPv4 Header Checksum
typedef ap_uint<32> Ip4SrcAddr;     // IPv4 Source Address
typedef ap_uint<32> Ip4DstAddr;     // IPv4 Destination Address
typedef ap_uint<32> Ip4Address;     // IPv4 Source or Destination Address
typedef ap_uint<32> Ip4Addr;        // IPv4 Source or Destination Address
typedef ap_uint<64> Ip4Data;        // IPv4 Data unit of transfer

typedef ap_uint<16> Ip4PktLen;      // IP4 Packet Length in octets (same as Ip4TotalLen)
typedef ap_uint<16> Ip4DatLen;      // IP4 Data   Length in octets (same as Ip4PktLen minus Ip4HdrLen)


/******************************************************************************
 * IP4 Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 *******************************************************************************/
class AxisIp4: public AxisRaw {

  public:
    AxisIp4() {}
    AxisIp4(AxisRaw axisRaw) :
        AxisRaw(axisRaw.getLE_TData(), axisRaw.getLE_TKeep(), axisRaw.getLE_TLast()) {}
    AxisIp4(LE_tData tdata, LE_tKeep tkeep, LE_tLast tlast) :
        AxisRaw(tdata, tkeep, tlast) {}
    AxisIp4(const AxisIp4 &axisRaw) :
        AxisRaw(axisRaw.tdata, axisRaw.tkeep, axisRaw.tlast) {}

    //-----------------------------------------------------
    //-- IP4 PACKET FIELDS - Constant Definitions
    //-----------------------------------------------------
    // IP protocol numbers
    static const unsigned char  ICMP_PROTOCOL = 0x01;
    static const unsigned char  TCP_PROTOCOL  = 0x06;
    static const unsigned char  UDP_PROTOCOL  = 0x11;

    /****************************************************************
     * AXIS_UDP - BIG-ENDIAN HELPERS (specific to UDP-over-IPv4)
     ****************************************************************/
    /* Set higher-half part of the 'tdata' field with a data encoded in BE order
     *        +---------------+---------------+---------------+---------------+
     * LITTLE |63        Lower-Half         32|31       Higher-Half          0|
     *        +---------------+---------------+---------------+---------------+
     */
    void setTDataHi(tDataHalf data) {
        tdata.range(31,  0) = swapDWord(data);
    }
    /* Get higher-half part of the 'tdata' field and return it in BE order
     *        +---------------+---------------+---------------+---------------+
     * LITTLE |63        Lower-Half         32|31       Higher-Half          0|
     *       +---------------+---------------+---------------+---------------+
     */
    tDataHalf getTDataHi() {
        return swapDWord(tdata.range(31, 0));
    }
    /* Set lower-half part of the 'tdata' field with a data encoded in BE order
     *        +---------------+---------------+---------------+---------------+
     * LITTLE |63        Lower-Half         32|31       Higher-Half          0|
     *        +---------------+---------------+---------------+---------------+
     */
    void setTDataLo(tDataHalf data) {
        tdata.range(63, 32) = swapDWord(data);
    }
    /* Get lower-half part of the 'tdata' field and return it in BE order
     *        +---------------+---------------+---------------+---------------+
     * LITTLE |63        Lower-Half         32|31       Higher-Half          0|
     *        +---------------+---------------+---------------+---------------+
     */
    tDataHalf getTDataLo() {
        return swapDWord(tdata.range(63,32));
    }
    // Set higher-half part of the 'tkeep' field with a data encoded in BE order
    void setTKeepHi(tKeepHalf keep) {
        tkeep(3,0) = swapNibble(keep);
    }
    // Get higher-half part of the 'tkeep' field and return it in BE order
    tKeepHalf getTKeepHi() {
        return swapNibble(tkeep.range(3,0));
    }
    // Set lower-half part of the 'tkeep' field with a data encoded in BE order
    void setTKeepLo(tKeepHalf keep) {
        tkeep(7,4) = swapNibble(keep);
    }
    // Get lower-half part of the 'tkeep' field and return it in BE order
    tKeepHalf getTKeepLo() {
        return swapNibble(tkeep.range(7,4));
    }

    /****************************************************************
     * IP4 HEADER HELPERS - SETTERS AND GETTERS
     ****************************************************************/
    // Set-Get the IP4 Version
    void        setIp4Version(Ip4Version ver)   {                  tdata.range( 7,  4) = ver;             }
    Ip4Version  getIp4Version()                 {           return tdata.range( 7,  4);                   }
    // Set-Get the IP4 Internet Header Length
    void        setIp4HdrLen(Ip4HdrLen ihl)     {                  tdata.range( 3,  0) = ihl;             }
    Ip4HdrLen   getIp4HdrLen()                  {           return tdata.range( 3,  0);                   }
    // Set-Get the IP4 Type of Service
    void        setIp4ToS(Ip4ToS tos)           {                  tdata.range(15,  8) = tos;             }
    Ip4ToS      getIp4ToS()                     {           return tdata.range(15,  8);                   }
    // Set the IP4 Total Length
    void        setIp4TotalLen(Ip4TotalLen len) {                  tdata.range(31, 16) = swapWord(len);   }
    Ip4TotalLen getIp4TotalLen()                { return swapWord (tdata.range(31, 16));                  }
    // Set the IP4 Identification
    void        setIp4Ident(Ip4Ident id)        {                  tdata.range(47, 32) = swapWord(id);    }
    Ip4Ident    getIp4Ident()                   { return swapWord (tdata.range(47, 32));                  }
    // Set-get the IP4 Fragment Offset
    void        setIp4FragOff(Ip4FragOff offset){                  tdata.range(63, 56) = offset( 7, 0);
                                                                   tdata.range(52, 48) = offset(12, 8);   }
    Ip4FragOff  getIp4FragOff()                 {          return (tdata.range(52, 48) << 8 |
                                                                   tdata.range(63, 56));                  }
    // Set the IP4 Flags
    void        setIp4Flags(Ip4Flags flags)     {                  tdata.range(55, 53) = flags;           }
    // Set-Get the IP4 Time to Live
    void        setIp4TtL(Ip4TtL ttl)           {                  tdata.range( 7,  0) = ttl;             }
    Ip4TtL      getIp4Ttl()                     {           return tdata.range( 7,  0);                   }
    // Set-Get the IP4 Protocol
    void        setIp4Prot(Ip4Prot prot)        {                  tdata.range(15,  8) = prot;            }
    Ip4Prot     getIp4Prot()                    {           return tdata.range(15,  8);                   }
    // Set-Get the IP4 Header Checksum
    void        setIp4HdrCsum(Ip4HdrCsum csum)  {                  tdata.range(31, 16) = swapWord(csum);  }
    Ip4HdrCsum  getIp4HdrCsum()                 { return swapWord (tdata.range(31, 16));                  }
    // Set-Get the IP4 Source Address
    void        setIp4SrcAddr(Ip4Addr addr)     {                  tdata.range(63, 32) = swapDWord(addr); }
    Ip4Addr     getIp4SrcAddr()                 { return swapDWord(tdata.range(63, 32));                  }
    // Set-Get the IP4 Destination Address
    void        setIp4DstAddr(Ip4Addr addr)     {                  tdata.range(31,  0) = swapDWord(addr); }
    Ip4Addr     getIp4DstAddr()                 { return swapDWord(tdata.range(31,  0));                  }
    LE_Ip4TtL  getLE_Ip4Ttl()                   {           return tdata.range( 7,  0);                   }
    LE_Ip4Prot getLE_Ip4Prot()                  {           return tdata.range(15,  8);                   }
    LE_Ip4HdrCsum getLE_Ip4HdrCsum()            {           return tdata.range(31, 16);                  }
    LE_Ip4Addr getLE_Ip4SrcAddr()               {           return tdata.range(63, 32);                   }
    LE_Ip4Addr getLE_Ip4DstAddr()               {           return tdata.range(31,  0);                   }

    //-----------------------------------------------------
    //-- ENCAPSULATED TCP SEGMENT - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the TCP Source Port
    void        setTcpSrcPort(TcpPort port)     {                  tdata.range(47, 32) = swapWord(port);  }
    TcpPort     getTcpSrcPort()                 { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the TCP Destination Port
    void        setTcpDstPort(TcpPort port)     {                  tdata.range(63, 48) = swapWord(port);  }
    TcpPort     getTcpDstPort()                 { return swapWord (tdata.range(63, 48));                  }
    // Set-Get the TCP Sequence Number
    void        setTcpSeqNum(TcpSeqNum num)     {                  tdata.range(31,  0) = swapDWord(num);  }
    TcpSeqNum   getTcpSeqNum()                  { return swapDWord(tdata.range(31,  0));                  }
    // Set-Get the TCP Acknowledgment Number
    void        setTcpAckNum(TcpAckNum num)     {                  tdata.range(63, 32) = swapDWord(num);  }
    TcpAckNum   getTcpAckNum()                  { return swapDWord(tdata.range(63, 32));                  }
    // Set-Get the TCP Data Offset
    void        setTcpDataOff(TcpDataOff offset){                  tdata.range( 7,  4) = offset;          }
    TcpDataOff  getTcpDataOff()                 { return           tdata.range( 7,  4);                   }
    // Set-Get the TCP Control Bits
    void setTcpCtrlFin(TcpCtrlBit bit)          {                  tdata.bit( 8) = bit;                   }
    TcpCtrlBit  getTcpCtrlFin()                 {           return tdata.bit( 8);                         }
    void setTcpCtrlSyn(TcpCtrlBit bit)          {                  tdata.bit( 9) = bit;                   }
    TcpCtrlBit  getTcpCtrlSyn()                 {           return tdata.bit( 9);                         }
    void setTcpCtrlRst(TcpCtrlBit bit)          {                  tdata.bit(10) = bit;                   }
    TcpCtrlBit  getTcpCtrlRst()                 {           return tdata.bit(10);                         }
    void setTcpCtrlPsh(TcpCtrlBit bit)          {                  tdata.bit(11) = bit;                   }
    TcpCtrlBit  getTcpCtrlPsh()                 {           return tdata.bit(11);                         }
    void setTcpCtrlAck(TcpCtrlBit bit)          {                  tdata.bit(12) = bit;                   }
    TcpCtrlBit  getTcpCtrlAck()                 {           return tdata.bit(12);                         }
    void setTcpCtrlUrg(TcpCtrlBit bit)          {                  tdata.bit(13) = bit;                   }
    TcpCtrlBit  getTcpCtrlUrg()                 {           return tdata.bit(13);                         }
    // Set-Get the TCP Window
    void        setTcpWindow(TcpWindow win)     {                  tdata.range(31, 16) = swapWord(win);   }
    TcpWindow   getTcpWindow()                  { return swapWord (tdata.range(31, 16));                  }
    // Set-Get the TCP Checksum
    void        setTcpChecksum(TcpChecksum csum){                  tdata.range(47, 32) = swapWord(csum);  }
    TcpChecksum getTcpChecksum()                { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the TCP Urgent Pointer
    void        setTcpUrgPtr(TcpUrgPtr ptr)     {                  tdata.range(63, 48) = swapWord(ptr);   }
    TcpUrgPtr   getTcpUrgPtr()                  { return swapWord (tdata.range(63, 48));                  }
    // Set-Get the TCP Options
    void        setTcpOptKind(TcpOptKind val)   {                  tdata.range( 7,  0);                   }
    TcpOptKind  getTcpOptKind()                 { return           tdata.range( 7,  0);                   }
    void        setTcpOptMss(TcpOptMss val)     {                  tdata.range(31, 16);                   }
    TcpOptMss   getTcpOptMss()                  { return swapWord (tdata.range(31, 16));                  }

    LE_TcpPort  getLE_TcpSrcPort()              {           return tdata.range(47, 32) ;                  }
    LE_TcpPort  getLE_TcpDstPort()              {           return tdata.range(63, 48);                   }

    //-----------------------------------------------------
    //-- ENCAPSULATED UDP DATAGRAM - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the UDP Source Port
    void        setUdpSrcPort(UdpPort port)     {                  tdata.range(47, 32) = swapWord(port);  }
    UdpPort     getUdpSrcPort()                 { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the UDP Destination Port
    void        setUdpDstPort(UdpPort port)     {                  tdata.range(63, 48) = swapWord(port);  }
    UdpPort     getUdpDstPort()                 { return swapWord (tdata.range(63, 48));                  }

    // Set-Get the UDP Length
    void        setUdpLen(UdpLen len)           {                  tdata.range(15,  0) = swapWord(len);   }
    UdpLen      getUdpLen(int Lo=0)             { return swapWord (tdata.range(Lo+16-1, Lo));             }
    // Set-Get the UDP Checksum
    void        setUdpCsum(UdpCsum csum)        {                  tdata.range(31, 16) = swapWord(csum);  }
    UdpCsum     getUdpCsum(int Lo=16)           { return swapWord (tdata.range(Lo+16-1, Lo));             }

    //-----------------------------------------------------
    //-- ENCAPSULATED ICMP MESSAGE - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the message Type field
    void          setIcmpType(IcmpType type)    {                  tdata.range(32+ 7, 32+ 0) = type;            }
    IcmpType      getIcmpType()                 {           return tdata.range(32+ 7, 32+ 0);                   }
    // Set-Get the message Code field
    void          setIcmpCode(IcmpCode code)    {                  tdata.range(32+15, 32+ 8) = code;            }
    IcmpCode      getIcmpCode()                 {           return tdata.range(32+15, 32+ 8);                   }
    // Set-Get the Checksum field
    void          setIcmpCsum(IcmpCsum csum)    {                  tdata.range(32+31, 32+16) = swapWord(csum);  }
    IcmpCsum      getIcmpCsum()                 { return swapWord (tdata.range(32+31, 32+16));                  }

    LE_IcmpType   getLE_IcmpType()              {             return tdata.range(32+ 7, 32+ 0);                   }
    LE_IcmpCode   getLE_IcmpCode()              {             return tdata.range(32+15, 32+ 8);                   }
    LE_IcmpCsum   getLE_IcmpCsum()              {             return tdata.range(32+31, 32+16);                   }


  private:
    // Reverse the bits within a nibble.
    ap_uint<4> swapNibble(ap_uint<4> nibble) {
        return (nibble.range(0,3));
    }
    // Reverse the bits within a byte.
    ap_uint<8> swapByte(ap_uint<8> byte) {
        return (byte.range(0,7));
    }
    // Swap the two bytes of a word (.i.e, 16 bits)
    ap_uint<16> swapWord(ap_uint<16> word) {
      return (word.range(7,0), word.range(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits)
    ap_uint<32> swapDWord(ap_uint<32> dword) {
      return (dword.range( 7, 0), dword.range(15,  8),
              dword.range(23,16), dword.range(31, 24));
    }

}; // End of: AxisIp4

#endif