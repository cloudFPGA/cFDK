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



/*************************************************************************
 * MACRO DEFINITIONS
 *************************************************************************/

// Concatenate two char constants
#define concat2(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat3(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst


/*************************************************************************
 * PROTOTYPE DEFINITIONS
 *************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk);

void printInfo(const char *callerName, const char *message);

void printIpPktStream(const char *callerName, std::deque<Ip4Word> &pktChunk);

void printSockPair(const char *callerName, SocketPair sockPair);

void printWarn(const char *callerName, const char *message);

ap_uint<16> swapWord(ap_uint<16> inpWord);

ap_uint<32> swapDWord(ap_uint<32> inpDWord);

#endif
