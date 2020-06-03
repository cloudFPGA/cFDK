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
 *  stored in LITTLE-ENDIAN order instead of the initial BIG-ENDIAN order used
 *  to transmit bytes over the physical media.
 *  As an example, assume that the 16 bits of the UDP "Checksum" datagram has a
 *  value of '0xA1B2'. This field will be transmitted on the media in big-endian
 *  order .i.e, a '0xA1' followed by '0xB2'. However, this field will end up
 *  being ordered in little-endian mode (.i.e, 0xB2A1) by the AXI4-Stream
 *  interface of the 10GbE MAC.
 *
 *  Therefore, the format of a UDP datagram transferred over an AXI4-Stream
 *  interface of quadwords is done in LITTLE-ENDIAN and is mapped as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Checksum            |            Length             |       Destination Port        |          Source Port          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @info: What's this Little-Endian(LE) vs Big-Endian(BE) anyhow.
 *  FYI - The original source from Xilinx (available at:
 *   https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip) is
 *   entirely coded with respect to the above mapping of the media network
 *   stream  over a 64-bits interface in little-endian oder. This makes the initial
 *   code particularly difficult to read, maintain and test. Therefore, this
 *   class implements a set of methods to hide this complexity by accessing a
 *   raw Axis data streams as if it was encoded in the expected big-endian order.
 *
 * @warning : This class is to be used when a UDP datagram is aligned to a
 *  64-bit quadword. Refer to the methods of 'AxisIp4.hpp' to access the fields
 *  of a UDP datagram that is embedded into an IPv4 packet, or the the methods
 *  of 'AxisEth.hpp' to access the fields of an UDP datagram that is embedded
 *  into an IPv4 packet further embedded into an Ethernet frame.
 *
 *****************************************************************************/

#ifndef AXIS_UDP_H_
#define AXIS_UDP_H_

#include "AxisRaw.hpp"

/*********************************************************
 * UDP - HEADER SIZE.
 *  All UDP datagrams have a fix-sized header of 8 bytes
 *  and a variable-sized data section.
 *********************************************************/
#define UDP_HEADER_LEN  8           // In bytes

/*********************************************************
 * UDP - HEADER FIELDS IN LITTLE_ENDIAN (LE) ORDER.
 *********************************************************/
typedef ap_uint<16> LE_UdpSrcPort;  // UDP Source Port
typedef ap_uint<16> LE_UdpDstPort;  // UDP Destination Port
typedef ap_uint<16> LE_UdpPort;     // UDP source or destination Port
typedef ap_uint<16> LE_UdpLen;      // UDP header and data Length
typedef ap_uint<16> LE_UdpCsum;     // UDP header and data Checksum
typedef LE_tData    LE_UdpData;     // UDP Data unit of transfer

/*********************************************************
 * UDP - HEADER FIELDS IN NETWORK BYTE ORDER.
 *   Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<16> UdpSrcPort;     // UDP Source Port
typedef ap_uint<16> UdpDstPort;     // UDP Destination Port
typedef ap_uint<16> UdpPort;        // UDP source or destination Port
typedef ap_uint<16> UdpLen;         // UDP header and data Length
typedef ap_uint<16> UdpCsum;        // UDP header and data Checksum
typedef tData       UdpData;        // UDP Data unit of transfer
typedef tDataHalf   UdpDataHi;      // UDP High part of a data unit of transfer
typedef tDataHalf   UdpDataLo;      // UDP Low part of a data unit of transfer

/******************************************************************************
 * UDP Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 ******************************************************************************/
class AxisUdp: public AxisRaw {

  public:
    AxisUdp() {}
    AxisUdp(AxisRaw axisRaw) :
        AxisRaw(axisRaw.getLE_TData(), axisRaw.getLE_TKeep(), axisRaw.getLE_TLast()) {}
    AxisUdp(LE_tData tdata, LE_tKeep tkeep, LE_tLast tlast) :
        AxisRaw(tdata, tkeep, tlast) {}
    AxisUdp(const AxisUdp &axisUdp) :
        AxisRaw(axisUdp.tdata, axisUdp.tkeep, axisUdp.tlast) {}

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
    // Get the length of the higher-half part of the this chunk (in bytes)
    int getLenHi() {
        int len = this->keepToLen();
        if (len > 4) {
            return (4);
        }
        else {
            return len;
        }
    }
    // Get the length of the lower-half part of the this chunk (in bytes)
    int getLenLo() {
        int len = this->keepToLen();
        if (len > 4) {
            return (len-4);
        }
        else {
            return 0;
        }
    }

    /****************************************************************
     * UDP HEADER HELPERS - SETTERS AND GETTERS
     ****************************************************************/

    // Set-Get the UDP Source Port
    void          setUdpSrcPort(UdpPort port)   {                  tdata.range(15,  0) = swapWord(port);  }
    UdpPort       getUdpSrcPort()               { return swapWord (tdata.range(15,  0));                  }
    LE_UdpPort getLE_UdpSrcPort()               {           return tdata.range(15,  0) ;                  }

    // Set-Get the UDP Destination Port
    void          setUdpDstPort(UdpPort port)   {                  tdata.range(31, 16) = swapWord(port);  }
    UdpPort       getUdpDstPort()               { return swapWord (tdata.range(31, 16));                  }
    LE_UdpPort getLE_UdpDstPort()               {           return tdata.range(31, 16);                   }

    // Set-Get the UDP length field
    void          setUdpLen(UdpLen length)      {                  tdata.range(47, 32) = swapWord(length);}
    UdpLen        getUdpLen()                   { return swapWord (tdata.range(47, 32));                  }
    LE_UdpLen     getLE_UdpLen()                   {           return tdata.range(47, 32);                   }

    // Set-Get the UDP Checksum field
    void          setUdpCsum(UdpCsum csum)      {                  tdata.range(63, 48) = swapWord(csum);  }
    UdpCsum       getUdpCsum()                  { return swapWord (tdata.range(63, 48));                  }

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

}; // End of: AxisUdp

#endif
