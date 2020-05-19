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
 * @file       : AxisTcp.hpp
 * @brief      : A class to access TCP header fields within data chunks
 *               transmitted over an AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details : The Transmission Control Protocol (TCP) fields defined in this
 *  class refer to the format generated by the 10GbE MAC of Xilinx which
 *  organizes its two 64-bit Rx and Tx interfaces into 8 lanes (see PG157).
 *  The result of this division into lanes, is that the TCP fields end up being
 *  stored in LITTLE-ENDIAN order instead of the initial big-endian order used
 *  to transmit bytes over the physical media.
 *  As an example, assume that the 16 bits of the TCP "SourcePort" has a value
 *  of '0xABCD'. This field will be transmitted on the media in big-endian
 *  order .i.e, a '0xAB' byte followed by a '0xCD' byte. However, this field
 *  will ends up being ordered in little-endian mode (.i.e, 0xCDAB) by the
 *  AXI4-Stream interface of the 10GbE MAC.
 *
 * @warning : This class is to be used when an TCP segment is aligned to a
 *  64-bit quadword. Refer to the methods of 'AxisIp4.hpp' to access the fields
 *  of a TCP segment that is embedded into an IPv4 packet, or the the methods
 *  of 'AxisEth.hpp' to access the fields of a TCP segment that is embedded
 *  into an IPv4 packet further embedded into an Ethernet frame.
 *
 * @info :
 *  The format of a TCP segment transferred over an AXI4-Stream interface of
 *  quadwords is done in LITTLE-ENDIAN and is mapped as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Sequence Number                        |       Destination Port        |          Source Port          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               |C|E|U|A|P|R|S|F|  Data |     |N|                                                               |
 *  |             Window            |W|C|R|C|S|S|Y|I| Offset| Res |S|                    Acknowledgment Number                      |
 *  |                               |R|E|G|K|H|T|N|N|       |     | |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Data (or Options, if DataOffset>5)                  |         Urgent Pointer        |           Checksum            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************************/

#ifndef AXIS_TCP_H_
#define AXIS_TCP_H_

#include "AxisRaw.hpp"

/*********************************************************
 * TCP - DEFINITIONS
 *********************************************************/

// A TCP segment has a default header len of 20 bytes. This length may vary in
// the presence of option(s) and is indicated by the field "DataOffset" > 5.
#define TCP_HEADER_LEN      20      // In bytes

/*********************************************************
 * TCP - HEADER FIELDS IN LITTLE_ENDIAN (LE) ORDER.
 *********************************************************/
typedef ap_uint<16> LE_TcpSrcPort;  // TCP Source Port from the MAC
typedef ap_uint<16> LE_TcpDstPort;  // TCP Destination Port from the MAC
typedef ap_uint<16> LE_TcpPort;     // TCP Source or Destination Port from the MAC
typedef ap_uint<32> LE_TcpSeqNum;   // TCP Sequence Number from the MAC
typedef ap_uint<32> LE_TcpAckNum;   // TCP Acknowledgment Number from the MAC
typedef ap_uint<4>  LE_TcpDataOff;  // TCP Data Offset from the MAC
typedef ap_uint<6>  LE_TcpCtrlBits; // TCP Control Bits from the MAC
typedef ap_uint<16> LE_TcpWindow;   // TCP Window from the MAC
typedef ap_uint<16> LE_TcpChecksum; // TCP Checksum
typedef ap_uint<16> LE_TcpUrgPtr;   // TCP Urgent Pointer from the MAC
typedef ap_uint<64> LE_TcpData;     // TCP Data stream from the MAC

/*********************************************************
 * TCP - HEADER FIELDS IN NETWORK BYTE ORDER.
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<16> TcpSrcPort;     // TCP Source Port
typedef ap_uint<16> TcpDstPort;     // TCP Destination Port
typedef ap_uint<16> TcpPort;        // TCP Source or Destination Port Number
typedef ap_uint<32> TcpSeqNum;      // TCP Sequence Number
typedef ap_uint<32> TcpAckNum;      // TCP Acknowledge Number
typedef ap_uint<4>  TcpDataOff;     // TCP Data Offset
typedef ap_uint<6>  TcpCtrlBits;    // TCP Control Bits
typedef ap_uint<1>  TcpCtrlBit;     // TCP Control Bit
typedef ap_uint<16> TcpWindow;      // TCP Window
typedef ap_uint<16> TcpChecksum;    // TCP Checksum
typedef ap_uint<16> TcpCSum;        // TCP Checksum (alias for TcpChecksum)
typedef ap_uint<16> TcpUrgPtr;      // TCP Urgent Pointer

typedef ap_uint< 8> TcpOptKind;     // TCP Option Kind
typedef ap_uint<16> TcpOptMss;      // TCP Option Maximum Segment Size

typedef ap_uint<16> TcpSegLen;      // TCP Segment Length in octets (same as Ip4DatLen)
typedef ap_uint< 8> TcpHdrLen;      // TCP Header  Length in octets
typedef ap_uint<16> TcpDatLen;      // TCP Data    Length in octets (same as TcpSegLen minus TcpHdrLen)

/*********************************************************
 * TCP Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 *********************************************************/
class AxisTcp: public AxisRaw {

  public:
    AxisTcp() {}
    AxisTcp(AxisRaw axisRaw) :
        AxisRaw(axisRaw.getLE_TData(), axisRaw.getLE_TKeep(), axisRaw.getLE_TLast()) {}
    AxisTcp(LE_tData tdata, LE_tKeep tkeep, LE_tLast tlast) :
        AxisRaw(tdata, tkeep, tlast) {}
    AxisTcp(const AxisTcp &axisTcp) :
        AxisRaw(axisTcp.tdata, axisTcp.tkeep, axisTcp.tlast) {}

    // Set-Get the TCP Source Port
    void          setTcpSrcPort(TcpPort port)   {                  tdata.range(15,  0) = swapWord(port);  }
    TcpPort       getTcpSrcPort()               { return swapWord (tdata.range(15,  0));                  }
    LE_TcpPort getLE_TcpSrcPort()               {           return tdata.range(15,  0) ;                  }

    // Set-Get the TCP Destination Port
    void          setTcpDstPort(TcpPort port)   {                  tdata.range(31, 16) = swapWord(port);  }
    TcpPort       getTcpDstPort()               { return swapWord (tdata.range(31, 16));                  }
    LE_TcpPort getLE_TcpDstPort()               {           return tdata.range(31, 16);                   }

    // Set-Get the TCP Sequence Number
    void       setTcpSeqNum(TcpSeqNum num)      {                  tdata.range(63, 32) = swapDWord(num);  }
    TcpSeqNum  getTcpSeqNum()                   { return swapDWord(tdata.range(63, 32));                  }

    // Set-Get the TCP Acknowledgment Number
    void       setTcpAckNum(TcpAckNum num)      {                  tdata.range(31,  0) = swapDWord(num);  }
    TcpAckNum  getTcpAckNum()                   { return swapDWord(tdata.range(31,  0));                  }

    // Set-Get the TCP Data Offset
    void       setTcpDataOff(TcpDataOff offset) {                  tdata.range(39, 36) = offset;          }
    TcpDataOff getTcpDataOff()                  { return           tdata.range(39, 36);                   }

    // Set-Get the TCP Control Bits
    void setTcpCtrlFin(TcpCtrlBit bit)          {                  tdata.bit(40) = bit;                   }
    TcpCtrlBit getTcpCtrlFin()                  {           return tdata.bit(40);                         }
    void setTcpCtrlSyn(TcpCtrlBit bit)          {                  tdata.bit(41) = bit;                   }
    TcpCtrlBit getTcpCtrlSyn()                  {           return tdata.bit(41);                         }
    void setTcpCtrlRst(TcpCtrlBit bit)          {                  tdata.bit(42) = bit;                   }
    TcpCtrlBit getTcpCtrlRst()                  {           return tdata.bit(42);                         }
    void setTcpCtrlPsh(TcpCtrlBit bit)          {                  tdata.bit(43) = bit;                   }
    TcpCtrlBit getTcpCtrlPsh()                  {           return tdata.bit(43);                         }
    void setTcpCtrlAck(TcpCtrlBit bit)          {                  tdata.bit(44) = bit;                   }
    TcpCtrlBit getTcpCtrlAck()                  {           return tdata.bit(44);                         }
    void setTcpCtrlUrg(TcpCtrlBit bit)          {                  tdata.bit(45) = bit;                   }
    TcpCtrlBit getTcpCtrlUrg()                  {           return tdata.bit(45);                         }

    // Set-Get the TCP Window
    void        setTcpWindow(TcpWindow win)     {                  tdata.range(63, 48) = swapWord(win);   }
    TcpWindow   getTcpWindow()                  { return swapWord (tdata.range(63, 48));                  }

    // Set-Get the TCP Checksum
    void        setTcpChecksum(TcpChecksum csum){                  tdata.range(15,  0) = swapWord(csum);                   }
    TcpChecksum getTcpChecksum()                { return swapWord (tdata.range(15,  0));                  }

    // Set-Get the TCP Urgent Pointer
    void        setTcpUrgPtr(TcpUrgPtr ptr)     {                  tdata.range(31, 16) = swapWord(ptr);   }
    TcpUrgPtr   getTcpUrgPtr()                  { return swapWord (tdata.range(31, 16));                  }

    // Set-Get the TCP Options
    void        setTcpOptKind(TcpOptKind val)   {                  tdata.range(39, 32);                   }
    TcpOptKind  getTcpOptKind()                 { return           tdata.range(39, 32);                   }
    void        setTcpOptMss(TcpOptMss val)     {                  tdata.range(63, 48);                   }
    TcpOptMss   getTcpOptMss()                  { return swapWord (tdata.range(63, 48));                  }

  private:
    // Swap the two bytes of a word (.i.e, 16 bits)
    ap_uint<16> swapWord(ap_uint<16> inpWord) {
      return (inpWord.range(7,0), inpWord.range(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits)
    ap_uint<32> swapDWord(ap_uint<32> inpDWord) {
      return (inpDWord.range( 7, 0), inpDWord.range(15,  8),
              inpDWord.range(23,16), inpDWord.range(31, 24));
    }

}; // End of: AxisTcp

#endif