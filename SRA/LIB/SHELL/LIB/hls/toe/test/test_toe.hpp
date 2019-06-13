/*****************************************************************************
 * @file       : test_toe.hpp
 * @brief      : Data structures, types & prototypes definitions for test of
 *                the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef TEST_TOE_H_
#define TEST_TOE_H_



using namespace std;



/*******************************************************************
 * @brief Class Testbench Socket Address
 *  This class differs from the class 'AxiSockAddr' used by TOE from
 *  an ENDIANESS point of view. This class is ENDIAN independent as
 *  opposed to the one used by TOE which stores its data members in
 *  LITTLE-ENDIAN order.
 *******************************************************************/
class TbSockAddr {  // Testbench Socket Address
  public:
    unsigned int addr;  // IPv4 address
    unsigned int port;  // TCP  port
    TbSockAddr() {}
    TbSockAddr(unsigned int addr, unsigned int port) :
        addr(addr), port(port) {}
};

/*******************************************************************
 * @brief Class Testbench Socket Pair
 *  This class differs from the class 'AxiSockAddr' used by TOE from
 *  an ENDIANESS point of view. This class is ENDIAN independent as
 *  opposed to the one used by TOE which stores its data members in
 *  LITTLE-ENDIAN order.
 *******************************************************************/
class TbSocketPair {    // Socket Pair Association
  public:
    TbSockAddr  src;    // Source socket address in LITTLE-ENDIAN order !!!
    TbSockAddr  dst;    // Destination socket address in LITTLE-ENDIAN order !!!
    TbSocketPair() {}
    TbSocketPair(TbSockAddr src, TbSockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (TbSocketPair const &s1, TbSocketPair const &s2) {
        return ((s1.dst.addr < s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}


#endif
