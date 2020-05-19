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
 * @file       : AxisEth.hpp
 * @brief      : A class to access an ETHernet data chunk transmitted over an
 *                AXI4-Stream interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details : The ETHernet (ETH) fields defined in this class refer to the
 *  format generated by the 10GbE MAC of Xilinx which organizes its two 64-bit
 *  Rx and Tx interfaces into 8 lanes (see PG157). The result of this division
 *  into lanes, is that the ETH fields end up being stored in LITTLE-ENDIAN
 *  order instead of the initial big-endian order used to transmit bytes over
 *  the physical media.
 *  As an example, consider the 16-bit field "EtherType" of the ETH frame
 *  which value is '0x0800' when the Ethernet frame contains an IPv4 packet.
 *  This field will be transmitted on the media in big-endian order .i.e, a
 *  '0x08' followed by a '0x00'. However, this field will end up being ordered
 *  in little-endian mode (.i.e, 0x0080) by the AXI4-Stream interface of the
 *  10GbE MAC.
 *
 *  Therefore, the mapping of an ETH frame onto the 64-bits of an AXI4-Stream
 *  interface encoded in LITTLE-ENDIAN is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Data      |     Data      |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Data      |     Data      |     Data      |     Data      |      Data     |     Data      |     Data      |     Data      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  And the format of an ARP packet over an ETHERNET frame is as follow:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          HTYPE=0x0001         |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SHA[1]     |    SHA[0]     |    OPER=0x0001 (or 0x0002)    |   PLEN=0x04   |   HLEN=0x06   |          PTYPE=0x0800         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    SPA[3]     |    SPA[2]     |    SPA[1]     |    SPA[0]     |    SHA[5]     |    SHA[4]     |    SHA[3]     |    SHA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    TPA[1]     |    TPA[0]     |    THA[5]     |    THA[4]     |    THA[3]     |    THA[2]     |    THA[1]     |    THA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               |               |               |               |               |               |    TPA[3]     |    TPA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  And the format of an IPv4 packet over an ETHERNET frame is as follows:
 *
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Type of Service|Version|  IHL  |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Protocol   |  Time to Live | Frag. Offset  |Flags|         |         Identification        |          Total Length         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Destination Address (Hi-Word) |                       Source Address                          |         Header Checksum       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |              Data             |                Options (if IHL>5) or Data                     | Destination Address (Lo-Word) |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************************/

#ifndef AXIS_ETH_H_
#define AXIS_ETH_H_

#include "AxisRaw.hpp"
#include "AxisArp.hpp"
#include "AxisIp4.hpp"

/*********************************************************
 * ETH - HEADER FIELDS IN LITTLE-ENDIAN (LE) ORDER.
 *   As received or transmitted by the 10GbE MAC.
 *********************************************************/
typedef ap_uint<48> LE_EthSrcAddr;     // Ethernet Source Address
typedef ap_uint<48> LE_EthDstAddr;     // Ethernet Destination Address
typedef ap_uint<48> LE_EthAddress;     // Ethernet Source or Destination Address
typedef ap_uint<48> LE_EthAddr;        // Ethernet Source or Destination Address (a shorter version)
typedef ap_uint<16> LE_EthTypeLen;     // Ethernet Type or Length field
typedef ap_uint<16> LE_EtherType;      // Ethernet Type field
typedef ap_uint<16> LE_EtherLen;       // Ethernet Length field

/*********************************************************
 * ETH - HEADER FIELDS IN NETWORK BYTE ORDER.
 *   Default type definitions (as used by HLS).
 *********************************************************/
typedef ap_uint<48> EthSrcAddr;        // Ethernet Source Address
typedef ap_uint<48> EthDstAddr;        // Ethernet Destination Address
typedef ap_uint<48> EthAddress;        // Ethernet Source or Destination Address
typedef ap_uint<48> EthAddr;           // Ethernet Source or Destination Address
typedef ap_uint<16> EthTypeLen;        // Ethernet Type or Length field
typedef ap_uint<16> EtherType;         // Ethernet Type field
typedef ap_uint<16> EtherLen;          // Ethernet Length field

/*********************************************************
 * IPv4 - UNALIGNED FIELDS IN LITTLE-ENDIAN (LE) ORDER.
 *   As received or transmitted by the 10GbE MAC.
 *********************************************************/
typedef ap_uint<16> LE_Ip4DstAddrHi;   // IPv4 Destination Address 16-MSbits (.i.e 31:16)
typedef ap_uint<16> LE_Ip4DstAddrLo;   // IPv4 Destination Address 16-LSbits (.i.e 15:00)

/*********************************************************
 * IPv4 - UNALIGNED FIELDS IN NETWORK BYTE ORDER.
 *  Default Type Definitions (as used by HLS).
 *********************************************************/
typedef ap_uint<16> Ip4DstAddrHi;      // IPv4 Destination Address 16-MSbits (.i.e 31:16)
typedef ap_uint<16> Ip4DstAddrLo;      // IPv4 Destination Address 16-LSbits (.i.e 15:00)

/******************************************************************************
 * ETH Data over AXI4-STREAMING
 *  As Encoded by the 10GbE MAC (.i.e LITTLE-ENDIAN order).
 *******************************************************************************/
class AxisEth: public AxisRaw {

  public:
    AxisEth() {}
    AxisEth(AxisRaw axisRaw) :
        AxisRaw(axisRaw.getLE_TData(), axisRaw.getLE_TKeep(), axisRaw.getLE_TLast()) {}
    AxisEth(LE_tData tdata, LE_tKeep tkeep, LE_tLast tlast) :
      AxisRaw(tdata, tkeep, tlast) {}
    AxisEth(const AxisEth &axisEth) :
      AxisRaw(axisEth.tdata, axisEth.tkeep, axisEth.tlast) {}

    //-----------------------------------------------------
    //-- ETHERNET FRAME HEADER - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the ETH Destination Address
    void               setEthDstAddr(EthAddr addr)       {                    tdata.range(47,  0) = swapMacAddr(addr); }
    EthAddr            getEthDstAddr()                   { return swapMacAddr(tdata.range(47,  0));                    }
    // Set-Get the 16-MSbits of the ETH Source Address
    void               setEthSrcAddrHi(EthAddr addr)     {                    tdata.range(63, 48) = swapMacAddr(addr).range(15,  0); }
    ap_uint<16>        getEthSrcAddrHi()                 {    return swapWord(tdata.range(63, 48));                    }
    // Set-Get the 32-LSbits of the ETH Source Address
    void               setEthSrcAddrLo(EthAddr addr)     {                    tdata.range(31,  0) = swapMacAddr(addr).range(47, 16); }
    ap_uint<32>        getEthSrcAddrLo()                 {   return swapDWord(tdata.range(31,  0));                    }
    // Set-get the ETH Type/Length
    void               setEthTypeLen(EthTypeLen eTyLe)   {                    tdata.range(47, 32) = swapWord(eTyLe);   }
    EthTypeLen         getEthTypelen()                   {    return swapWord(tdata.range(47, 32));                    }
    void               setEthertType(EtherType  eType)   {                    tdata.range(47, 32) = swapWord(eType);   }
    EtherType          getEtherType()                    {    return swapWord(tdata.range(47, 32));                    }
    void               setEtherLen(EtherLen   eLength)   {                    tdata.range(47, 32) = swapWord(eLength); }
    EtherLen           getEtherLen()                     {    return swapWord(tdata.range(47, 32));                    }

    LE_EthAddr         getLE_EthDstAddr()                {             return tdata.range(47,  0);                     }
    ap_uint<16>        getLE_EthSrcAddrHi()              {             return tdata.range(63, 48);                     }
    ap_uint<32>        getLE_EthSrcAddrLo()              {             return tdata.range(31,  0);                     }
    LE_EtherType       getLE_EtherType()                 {             return tdata.range(47, 32);                     }

    //-----------------------------------------------------
    //-- ENCAPSULATED ARP PACKET - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the Hardware Type (HTYPE) field
    void               setArpHwType(ArpHwType htype)     {                 tdata.range(63, 48) = swapWord(htype);      }
    ArpHwType          getArpHwType()                    { return swapWord(tdata.range(63, 48));                       }
    // Set-Get the Protocol type (PTYPE) field
    void               setArpProtType(ArpProtType ptype) {                 tdata.range(15,  0) = swapWord(ptype);      }
    ArpProtType        getArpProtType()                  { return swapWord(tdata.range(15,  0));                       }
    // Set the Hardware Address Length (HLEN) field
    void               setArpHwLen(ArpHwLen hlen)        {                 tdata.range(23, 16) = hlen;                 }
    ArpHwLen           getArpHwLen()                     {          return tdata.range(23, 16);                        }
    // Set-Get Protocol Address length (PLEN) field
    void               setArpProtLen(ArpProtLen plen)    {                 tdata.range(31, 24) = plen;                 }
    ArpProtLen         getArpProtLen()                   {          return tdata.range(31, 24);                        }
    // Set-Get the Operation code (OPER) field
    void               setArpOper(ArpOper oper)          {                 tdata.range(47, 32) = swapWord(oper);       }
    ArpOper            getArpOper()                      { return swapWord(tdata.range(47, 32));                       }
    // Set-Get the 16-MSbits of the Sender Hardware Address (SHA)
    void               setArpShaHi(ArpSendHwAddr sha)    {                 tdata.range(63, 48) = swapMacAddr(sha).range(15,  0);}
    ArpShaHi           getArpShaHi()                     { return swapWord(tdata.range(63, 48));                       }
    // Set-Get the 32-LSbits of the Sender Hardware Address (SHA)
    void               setArpShaLo(ArpSendHwAddr sha)    {                 tdata.range(31,  0) = swapMacAddr(sha).range(47, 16);}
    ArpShaLo           getArpShaLo()                     {return swapDWord(tdata.range(31,  0));                       }
    // Set-Get the the Sender Protocol Address (SPA)
    void               setArpSpa(ArpSendProtAddr spa)    {                 tdata.range(63, 32) = swapDWord(spa);       }
    ArpSendProtAddr    getArpSpa()                       {return swapDWord(tdata.range(63, 32));                       }
    // Set-Get the Target Hardware Address (THA)
    void               setArpTha(ArpTargHwAddr tha)      {                   tdata.range(47,  0) = swapMacAddr(tha);   }
    ArpTargHwAddr      getArpTha()                       {return swapMacAddr(tdata.range(47,  0));                     }
    // Set-Get the 16-MSbits of the Target Protocol Address (TPA)
    void               setArpTpaHi(ArpTargProtAddr tpa)  {                 tdata.range(63, 48) = swapDWord(tpa).range(15,  0);  }
    ArpTpaHi           getArpTpaHi()                     { return swapWord(tdata.range(63, 48));                       }
    // Set-Get the 16-LSbits of the Target Protocol Address (TPA)
    void               setArpTpaLo(ArpTargProtAddr tpa)  {                 tdata.range(15,  0) = swapDWord(tpa).range(31, 16);  }
    ArpTpaLo           getArpTpaLo()                     { return swapWord(tdata.range(15,  0));                       }

    LE_ArpHwType       getLE_ArpHwType()                 {          return tdata.range(63, 48);                        }
    LE_ArpProtType     getLE_ArpProtType()               {          return tdata.range(15,  0);                        }
    LE_ArpOper         getLE_ArpOper()                   {          return tdata.range(47, 32);                        }
    LE_ArpShaHi        getLE_ArpShaHi()                  {          return tdata.range(63, 48);                        }
    LE_ArpShaLo        getLE_ArpShaLo()                  {          return tdata.range(31,  0);                        }
    LE_ArpSendProtAddr getLE_ArpSpa()                    {          return tdata.range(63, 32);                        }
    LE_ArpTargHwAddr   getLE_ArpTha()                    {          return tdata.range(47,  0);                        }
    LE_ArpTpaHi        getLE_ArpTpaHi()                  {          return tdata.range(63, 48);                        }
    LE_ArpTpaLo        getLE_ArpTpaLo()                  {          return tdata.range(15,  0);                        }

    //-----------------------------------------------------
    //-- ENCAPSULATED IP4 PACKET - Setters and Getters
    //-----------------------------------------------------
    // Set-Get the IP4 Version
    void              setIp4Version(Ip4Version ver)      {                 tdata.range(55, 52) = ver;                  }
    Ip4Version        getIp4Version()                    {          return tdata.range(55, 52);                        }
    // Set-Get the IP4 Internet Header Length
    void              setIp4HdrLen(Ip4HdrLen ihl)        {                 tdata.range(51, 48) = ihl;                  }
    Ip4HdrLen         getIp4HdrLen()                     {          return tdata.range(51, 48);                        }
    // Set-Get the IP4 Type of Service
    void              setIp4ToS(Ip4ToS tos)              {                 tdata.range(63, 56) = tos;                  }
    Ip4ToS            getIp4ToS()                        {          return tdata.range(63, 56);                        }
    // Set the IP4 Total Length
    void              setIp4TotalLen(Ip4TotalLen len)    {                 tdata.range(15,  0) = swapWord(len);        }
    Ip4TotalLen       getIp4TotalLen()                   { return swapWord(tdata.range(15,  0));                       }
    // Set-Get the IP4 Identification
    void              setIp4Ident(Ip4Ident id)           {                 tdata.range(31, 16) = swapWord(id);         }
    Ip4Ident          getIp4Ident()                      { return swapWord(tdata.range(31, 16));                       }
    // Set-Get the IP4 Fragment Offset
    void              setIp4FragOff(Ip4FragOff offset)   {                 tdata.range(47, 40) = offset( 7, 0);
                                                                           tdata.range(36, 32) = offset(12, 8);        }
    Ip4FragOff        getIp4FragOff()                    {         return (tdata.range(47, 40) << 8 |
                                                                           tdata.range(36, 32));                       }
    // Set the IP4 Flags
    void              setIp4Flags(Ip4Flags flags)        {                 tdata.range(39, 37) = flags;                }
    // Set-Get the IP4 Time to Live
    void              setIp4TtL(Ip4TtL ttl)              {                 tdata.range(55, 48) = ttl;                  }
    Ip4TtL            getIp4Ttl()                        {          return tdata.range(55, 48);                        }
    // Set-Get the IP4 Protocol
    void              setIp4Prot(Ip4Prot prot)           {                 tdata.range(63, 56) = prot;                 }
    Ip4Prot           getIp4Prot()                       {          return tdata.range(63, 56);                        }
    // Set-Get the IP4 Header Checksum
    void              setIp4HdrCsum(Ip4HdrCsum csum)     {                 tdata.range(15,  0) = swapWord(csum);       }
    Ip4HdrCsum        getIp4HdrCsum()                    {return  swapWord(tdata.range(15,  0));                       }
    // Set-Get the IP4 Source Address
    void              setIp4SrcAddr(Ip4Addr addr)        {                 tdata.range(47, 16) = swapDWord(addr);      }
    Ip4Addr           getIp4SrcAddr()                    {return swapDWord(tdata.range(47, 16));                       }
    // Set-Get the IP4 Destination Address
    void              setIp4DstAddrHi(Ip4Addr addr)      {                 tdata.range(63, 48) = swapWord(addr).range(15, 0); }
    Ip4DstAddrHi      getIp4DstAddrHi()                  { return swapWord(tdata.range(63, 48));                       }
    void              setIp4DstAddrLo(Ip4Addr addr)      {                 tdata.range(15,  0) = swapWord(addr).range(31,16); }
    Ip4DstAddrLo      getIp4DstAddrLo()                  { return swapWord(tdata.range(15,  0));                       }

    LE_Ip4Addr        getLE_Ip4SrcAddr()                 {          return tdata.range(63, 32);                        }
    LE_Ip4DstAddrHi   getLE_Ip4DstAddrHi()               {          return tdata.range(63, 48);                        }
    LE_Ip4DstAddrLo   getLE_Ip4DstAddrLo()               {          return tdata.range(15,  0);                        }


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

}; // End of: AxisEth

#endif