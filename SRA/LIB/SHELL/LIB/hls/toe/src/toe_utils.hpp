/*****************************************************************************
 * @file       : toe_utils.hpp
 * @brief      : Utilities for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef TOE_UTILS_H_
#define TOE_UTILS_H_

#include <stdio.h>
#include <string>

using namespace std;


/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
class SockAddr;
class SocketPair;
class AxiSockAddr;
class AxiSocketPair;
class AxiWord;
class DmCmd;
class Ip4overAxi;

/******************************************************************************
 * PROTOTYPE DEFINITIONS
 *******************************************************************************/
ap_uint<16> swapWord   (ap_uint<16> inpWord);       // [FIXME - To be replaced w/ byteSwap16]
ap_uint<16> byteSwap16 (ap_uint<16> inputVector);
ap_uint<32> swapDWord  (ap_uint<32> inpDWord);      // [FIXME - To be replaced w/ byteSwap32]
ap_uint<32> byteSwap32 (ap_uint<32> inputVector);

ap_uint<8>  lenToKeep  (ap_uint<4> noValidBytes);
ap_uint<8>  returnKeep (ap_uint<4> length);
ap_uint<4>  keepToLen  (ap_uint<8> keepValue);
ap_uint<4>  keepMapping(ap_uint<8> keepValue);

#endif
