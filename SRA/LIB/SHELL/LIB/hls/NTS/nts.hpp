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
 * @file       : nts.hpp
 * @brief      : Network Transport Stack (NTS) for the cloudFPGA shell.
 *
 * System:     : cloudFPGA
 * Component   : Shell)
 * Language    : Vivado HLS
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *               Network Transport Stack.
 *
 *****************************************************************************/

#ifndef NTS_H_
#define NTS_H_

//OBSOLETE #include <stdio.h>
//OBSOLETE #include <iostream>
//OBSOLETE #include <fstream>
//OBSOLETE #include <string>
//OBSOLETE #include <math.h>
//OBSOLETE #include <hls_stream.h>
//OBSOLETE #include <stdint.h>
//OBSOLETE #include <vector>

//OBSOLETE #include "ap_int.h"

#include "AxisIp4.hpp"    // IPv4 DEFINITIONS (NETWORK LAYER-3)

using namespace hls;

/******************************************************************************
 * GENERIC TYPES and CLASSES USED BY NTS
 ******************************************************************************
 * Some Terminology & Conventions:
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network.
 *  A PDU is therefore composed of a protocol specific control information
 *   (e.g, a header) and a user data section. This source code uses the
 *   following terminology:
 *   - a FRAME    (or MAC Frame)    refers to the Ethernet data link layer.
 *   - a PACKET   (or IP  Packet)   refers to the IP or ICMP protocol data unit.
 *   - a SEGMENT  (or TCP Segment)  refers to the TCP protocol data unit.
 *   - a DATAGRAM (or UDP Datagram) refers to the UDP protocol data unit.
 ******************************************************************************/









/******************************************************************************
 * NETWORK LAYER-4 - UDP & TCP TRANSPORT LAYER
 * ****************************************************************************
 * Terminology & Conventions
 *  - a SEGMENT  (or TCP Segment)  refers to the TCP protocol data unit.
 *  - a DATAGRAM (or UDP Datagram) refers to the UDP protocol data unit.
 ******************************************************************************/

/*********************************************************
 * LAYER-4 - COMMON TCP and UDP HEADER FIELDS
 *********************************************************/
typedef ap_uint<16> LE_Ly4Port; // Layer-4 Port in LE order
typedef ap_uint<16> LE_Ly4Len;  // Layer-4 Length in LE_order
typedef ap_uint<16> Ly4Port;    // Layer-4 Port
typedef ap_uint<16> Ly4Len;     // Layer-4 header plus data Length

/*********************************************************
 * TCP SESSION IDENTIFIER
 *********************************************************/
typedef ap_uint<16> SessionId;

/*********************************************************
 * SOCKET ADDRESS
 *********************************************************/
class LE_SockAddr {  // Socket Address stored in LITTLE-ENDIAN order !!!
  public:
    LE_Ip4Address  addr;  // IPv4 address in LITTLE-ENDIAN order !!!
    LE_Ly4Port     port;  // Layer-4 port in LITTLE-ENDIAN order !!!
    LE_SockAddr() {}
    LE_SockAddr(LE_Ip4Address addr, LE_Ly4Port port) :
        addr(addr), port(port) {}
};

/***********************************************
 * SOCKET ADDRESS
 ***********************************************/
class SockAddr {  // Socket Address stored in NETWORK BYTE ORDER
   public:
    Ip4Addr        addr;   // IPv4 address in NETWORK BYTE ORDER
    Ly4Port        port;   // Layer-4 port in NETWORK BYTE ORDER
    SockAddr() {}
    SockAddr(Ip4Addr ip4Addr, Ly4Port layer4Port) :
        addr(ip4Addr), port(layer4Port) {}
};

/***********************************************
 * SOCKET PAIR ASSOCIATION
 ***********************************************/
class LE_SocketPair { // Socket Pair Association in LITTLE-ENDIAN order !!!
  public:
    LE_SockAddr  src;  // Source socket address in LITTLE-ENDIAN order !!!
    LE_SockAddr  dst;  // Destination socket address in LITTLE-ENDIAN order !!!
    LE_SocketPair() {}
    LE_SocketPair(LE_SockAddr src, LE_SockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (LE_SocketPair const &s1, LE_SocketPair const &s2) {
        return ((s1.dst.addr < s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}

class SocketPair { // Socket Pair Association in NETWORK-BYTE order !!!
  public:
    SockAddr  src;  // Source socket address in NETWORK-BYTE order !!!
    SockAddr  dst;  // Destination socket address in NETWORK-BYTE order !!!
    SocketPair() {}
    SocketPair(SockAddr src, SockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (SocketPair const &s1, SocketPair const &s2) {
        return ((s1.dst.addr <  s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}



#endif

















