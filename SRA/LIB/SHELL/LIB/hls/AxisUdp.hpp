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
 * @file       : AxisUdp.hpp
 * @brief      : A class to access UDP header fields within data chunks
 *               transmitted over an AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details : The User Datagram Protocol (UDP) fields defined in this class
 *  refer to the format generated by the 10GbE MAC of Xilinx which organizes
 *  its two 64-bit Rx and Tx interfaces into 8 lanes (see PG157).
 *  The result of this division into lanes, is that the UDP fields end up being
 *  stored in LITTLE-ENDIAN order instead of the initial big-endian order used
 *  to transmit bytes over the physical media.
 *  As an example, assume that the 16 bits of the UDP "Checksum" datagram has a
 *  value of '0xA1B2'. This field will be transmitted on the media in big-endian
 *  order .i.e, a '0xA1' followed by '0xB2'. However, this field will end up
 *  being ordered in little-endian mode (.i.e, 0xB2A1) by the AXI4-Stream
 *  interface of the 10GbE MAC.
 *
 * @warning : This class is to be used when an UDP datagram is aligned to a
 *  64-bit quadword. Refer to the methods of 'AxisIp4.hpp' to access the fields
 *  of a UDP datagram that is embedded into an IPv4 packet, or the the methods
 *  of 'AxisEth.hpp' to access the fields of an UDP datagram that is embedded
 *  into an IPv4 packet further embedded into an Ethernet frame.
 *
 * @info :
 *  The format of an UDP datagram transferred over an AXI4-Stream interface of
 *  quadwords is done in LITTLE-ENDIAN and is mapped as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Checksum            |            Length             |       Destination Port        |          Source Port          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************************/

#ifndef AXIS_UDP_H_
#define AXIS_UDP_H_

/*********************************************************
 * UDP - HEADER SIZE.
 *  All UDP datagrams have a fix-sized header of 8 bytes
 *  and a variable-sized data section.
 *********************************************************/
#define UDP_HEADER_SIZE    8       // In bytes

/*********************************************************
 * UDP - HEADER FIELDS IN LITTLE_ENDIAN (LE) ORDER.
 *********************************************************/
typedef ap_uint<16> LE_UdpSrcPort;  // UDP Source Port
typedef ap_uint<16> LE_UdpDstPort;  // UDP Destination Port
typedef ap_uint<16> LE_UdpPort;     // UDP source or destination Port
typedef ap_uint<16> LE_UdpLen;      // UDP header and data Length
typedef ap_uint<16> LE_UdpCsum;     // UDP header and data Checksum

/*********************************************************
 * UDP - HEADER FIELDS IN NETWORK BYTE ORDER.
 *   Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<16> UdpSrcPort;     // UDP Source Port
typedef ap_uint<16> UdpDstPort;     // UDP Destination Port
typedef ap_uint<16> UdpPort;        // UDP source or destination Port
typedef ap_uint<16> UdpLen;         // UDP header and data Length
typedef ap_uint<16> UdpCsum;        // UDP header and data Checksum

/*********************************************************
 * UDP Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 *********************************************************/
class AxisUdp: public AxiWord {

  public:
    AxisUdp() {}
    AxisUdp(AxiWord axiWord) :
      AxiWord(axiWord.tdata, axiWord.tkeep, axiWord.tlast) {}
    AxisUdp(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
      AxiWord(tdata, tkeep, tlast) {}

    // Set-Get the UDP Source Port
    void          setUdpSrcPort(UdpPort port)   {                  tdata.range(15,  0) = swapWord(port);  }
    UdpPort       getUdpSrcPort()               { return swapWord (tdata.range(15,  0));                  }
    LE_UdpPort getLE_UdpSrcPort()               {           return tdata.range(15,  0) ;                  }

    // Set-Get the UDP Destination Port
    void          setUdpDstPort(UdpPort port)   {                  tdata.range(31, 16) = swapWord(port);  }
    UdpPort       getUdpPort()                  { return swapWord (tdata.range(31, 16));                  }
    LE_UdpPort getLE_UdpDstPort()               {           return tdata.range(31, 16);                   }

    // Set-Get the UDP length field
    void          setUdpLen(UdpLen length)      {                  tdata.range(47, 32) = swapWord(length);}
    UdpPort       getUdpLen()                   { return swapWord (tdata.range(47, 32));                  }
    LE_UdpPort getLE_UdpLen()                   {           return tdata.range(47, 32);                   }

    // Set-Get the UDP Checksum field
    void          setUdpCsum(UdpCsum csum)      {                  tdata.range(63, 48) = swapWord(csum);  }
    UdpCsum       getUdpCsum()                  { return swapWord (tdata.range(63, 48));                  }

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

}; // End of: AxisUdp

#endif
