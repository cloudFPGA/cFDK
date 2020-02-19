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

 * @file       : AxisArp.hpp
 * @brief      : A class to access ARP header fields within data chunks
 *               transmitted over an AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details : The Address Resolution Protocol (ARP) fields defined in this class
 *  refer to the format generated by the 10GbE MAC of Xilinx which organizes its
 *  two 64-bit Rx and Tx interfaces into 8 lanes (see PG157). The result of this
 *  division into lanes, is that the ARP fields end up being stored in LITTLE-
 *  ENDIAN order instead of the initial big-endian order used to transmit bytes
 *  over the physical media.
 *  As an example, consider the 16-bit field "Hardware Type (HTYPE)" of the ARP
 *  packet which value is '0x0001' when the media is Ethernet. This field will
 *  be transmitted on the media in big-endian order .i.e, a '0x00' followed by a
 *  '0x01'. However, this field will end up being ordered in little-endian mode
 *  (.i.e, 0x0100) by the AXI4-Stream interface of the 10GbE MAC.
 *
 * @warning : This class is to be used when an ARP packet is aligned to a 64-bit
 *  quadword. Refer to the methods of 'AxisEth.hpp' to access the fields of an
 *  ARP packet that is embedded into an Ethernet frame.
 *
 * @info :
 *  The format of an ARP packet transferred over an AXI4-Stream interface of
 *  quadwords is done in LITTLE-ENDIAN and is mapped as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    OPER=0x0001 (or 0x0002)    |   PLEN=0x04   |   HLEN=0x06   |          PTYPE=0x0800         |          HTYPE=0x0001         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SPA[1]     |    SPA[0]     |    SHA[5]     |    SHA[4]     |    SHA[3]     |    SHA[2]     |    SHA[1]     |    SHA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    THA[5]     |    THA[4]     |    THA[3]     |    THA[2]     |    THA[1]     |    THA[0]     |    SPA[3]     |    SPA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |    TPA[3]     |    TPA[2]     |    TPA[1]     |    TPA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************************/

#ifndef AXIS_ARP_H_
#define AXIS_ARP_H_

/*********************************************************
 * ARP - HEADER FIELDS IN LITTLE_ENDIAN (LE) ORDER.
 *********************************************************/
typedef ap_uint<16> LE_ArpHwType;       // ARP Hardware Type (HTYPE)
typedef ap_uint<16> LE_ArpProtType;     // ARP Protocol Type (PTYPE)
typedef ap_uint< 8> LE_ArpHwLen;        // ARP Hardware address Length (HLEN)
typedef ap_uint< 8> LE_ArpProtLen;      // ARP Protocol address Length (PLEN)
typedef ap_uint<16> LE_ArpOper;         // ARP Operation (1=request; 2=reply)
typedef ap_uint<48> LE_ArpSendHwAddr;   // ARP Sender Hardware Address (SHA)
typedef ap_uint<32> LE_ArpSendProtAddr; // ARP Sender Protocol Address (SPA)
typedef ap_uint<48> LE_ArpTargHwAddr;   // ARP Target Hardware Address (THA)
typedef ap_uint<32> LE_ArpTargProtAddr; // ARP Target Protocol Address (TPA)

typedef ap_uint<16> LE_ArpShaHi;        // ARP SHA 16-MSbits (.i.e 47:32)
typedef ap_uint<32> LE_ArpShaLo;        // ARP SHA 32-LSbits (.i.e 31: 0)
typedef ap_uint<16> LE_ArpTpaHi;        // ARP TPA 16-MSbits (.i.e 47:32)
typedef ap_uint<16> LE_ArpTpaLo;        // ARP TPA 16-LSbits (.i.e 31: 0)

/*********************************************************
 * ARP - HEADER FIELDS IN NETWORK BYTE ORDER.
 *   Default Type Definitions (as used by HLS)
 *********************************************************/
typedef ap_uint<16> ArpHwType;          // ARP Hardware Type (HTYPE)
typedef ap_uint<16> ArpProtType;        // ARP Protocol Type (PTYPE)
typedef ap_uint< 8> ArpHwLen;           // ARP Hardware address Length (HLEN)
typedef ap_uint< 8> ArpProtLen;         // ARP Protocol address Length (PLEN)
typedef ap_uint<16> ArpOper;            // ARP Operation (1=request; 2=reply)
typedef ap_uint<48> ArpSendHwAddr;      // ARP Sender Hardware Address (SHA)
typedef ap_uint<32> ArpSendProtAddr;    // ARP Sender Protocol Address (SPA)
typedef ap_uint<48> ArpTargHwAddr;      // ARP Target Hardware Address (THA)
typedef ap_uint<32> ArpTargProtAddr;    // ARP Target Protocol Address (TPA)

typedef ap_uint<16> ArpShaHi;           // ARP SHA 16-MSbits (.i.e 47:32)
typedef ap_uint<32> ArpShaLo;           // ARP SHA 32-LSbits (.i.e 31: 0)
typedef ap_uint<16> ArpTpaHi;           // ARP TPA 16-MSbits (.i.e 47:32)
typedef ap_uint<16> ArpTpaLo;           // ARP TPA 16-LSbits (.i.e 31: 0)


/************************************************
 * ARP BIND PAIR - {MAC,IPv4} ASSOCIATION
 ************************************************/
class ArpBindPair {
  public:
    EthAddr  macAddr;
    Ip4Addr  ip4Addr;
    ArpBindPair() {}
    ArpBindPair(EthAddr newMac, Ip4Addr newIp4) :
        macAddr(newMac), ip4Addr(newIp4) {}
};

/*********************************************************
 * ARP Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 *********************************************************/
class AxisArp: public AxiWord {

  public:
    AxisArp() {}
    AxisArp(AxiWord axiWord) :
      AxiWord(axiWord.tdata, axiWord.tkeep, axiWord.tlast) {}
    AxisArp(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
      AxiWord(tdata, tkeep, tlast) {}

    // Set-Get the Hardware Type (HTYPE) field
    void          setHardwareType(ArpHwType htype)   {                    tdata.range(15,  0) = swapWord(htype); }
    ArpHwType     getHardwareType()                  {   return swapWord (tdata.range(15,  0));                  }
    // Set-Get the Protocol type (PTYPE) field
    void          setProtocolType(ArpProtType ptype) {                    tdata.range(31, 16) = swapWord(ptype); }
    ArpProtType   getProtocolType()                  {   return swapWord (tdata.range(31, 16));                  }
    // Set the Hardware Address Length (HLEN) field
    void          setHardwareLength(ArpHwLen hlen)   {                    tdata.range(39, 32) = hlen;            }
    ArpHwLen      getHardwareLength()                {             return tdata.range(39, 32);                   }
    // Set-Get Protocol Address length (PLEN) field
    void          setProtocolLength(ArpProtLen plen) {                    tdata.range(47, 40) = plen;            }
    ArpProtLen    getProtocolLength()                {             return tdata.range(47, 40);                   }
    // Set-Get the Operation code (OPER) field
    void          setOperation(ArpOper oper)         {                    tdata.range(63, 48) = swapWord(oper);  }
    ArpOper       getOperation()                     {   return swapWord (tdata.range(63, 48));                  }
    // Set-Get the Sender Hardware Address (SHA)
    void          setSenderHwAddr(ArpSendHwAddr sha) {                    tdata.range(47,  0) = swapMacAddr(sha);}
    ArpSendHwAddr getSenderHwAddr()                  { return swapMacAddr(tdata.range(47,  0));                  }
    // Set-Get the 16-MSbits of the Sender Protocol Address (SPA)
    void          setSenderProtAddrHi(ArpSendProtAddr spa) {              tdata.range(63, 48) = swapDWord(spa).range(15, 0); }
    ap_uint<16>   getSenderProtAddrHi()              {    return swapWord(tdata.range(63, 48));                  }
    // Set-Get the 16-LSbits of the Sender Protocol Address (SPA)
    void          setSenderProtAddrLo(ArpSendProtAddr spa) {              tdata.range(15,  0) = swapDWord(spa).range(31,16); }
    ap_uint<32>   getSenderProtAddrLo()              {    return swapWord(tdata.range(15,  0));                  }
    // Set-Get the Target Hardware Address (SHA)
    void          setTargetHwAddr(ArpTargHwAddr tha) {                    tdata.range(63, 16) = swapMacAddr(tha);}
    ArpTargHwAddr getTargetHwAddr()                  { return swapMacAddr(tdata.range(63, 16));                  }
    // Set-Get the Target Protocol Address (TPA)
    void            setTargetProtAddr(ArpTargProtAddr tpa)   {              tdata.range(31,  0) = swapDWord(tpa);}
    ArpTargProtAddr getTargetProtAddr()              {     return swapDWord(tdata.range(31,  0));                }

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
    // Swap the six bytes of a MAC address (.i.e, 48 bits)
    ap_uint<48> swapMacAddr(ap_uint<48> macAddr) {
      return (macAddr.range( 7,  0), macAddr.range(15,  8),
              macAddr.range(23, 16), macAddr.range(31, 24),
              macAddr.range(39, 32), macAddr.range(47, 40));
    }

}; // End of: AxisArp

#endif
