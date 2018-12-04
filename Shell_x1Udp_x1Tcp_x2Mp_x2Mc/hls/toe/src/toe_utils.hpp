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

#include "toe.hpp"

// Forward declarations
//-----------------------
class AxiWord;
class Ip4overAxi;
class AxiSocketPair;

/*************************************************************************
 * MACRO DEFINITIONS
 *************************************************************************/

// Concatenate two char constants
#define concat2(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat3(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst

/*****************************************************************************
 * @brief A macro to print an information message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 *****************************************************************************/
#define printInfo(callerName , format, ...) \
    do { gTraceEvent = true; printf("[%s] INFO - " format, callerName, ##__VA_ARGS__); } while (0)

/*****************************************************************************
 * @brief A macro to print a warning message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 *****************************************************************************/
#define printWarn(callerName , format, ...) \
    do { gTraceEvent = true; printf("[%s] WARNING - " format, callerName, ##__VA_ARGS__); } while (0)

/*****************************************************************************
 * @brief A macro to print an error message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 *****************************************************************************/
#define printError(callerName , format, ...) \
    do { gTraceEvent = true; printf("[%s] ERROR - " format, callerName, ##__VA_ARGS__); } while (0)


/*************************************************************************
 * PROTOTYPE DEFINITIONS
 *************************************************************************/

#ifndef __SYNTHESIS__

void printAxiWord    (const char *callerName, AxiWord chunk);
void printIpPktStream(const char *callerName, std::deque<Ip4overAxi> &pktChunk);
void printSockPair   (const char *callerName, AxiSocketPair sockPair);

#endif



ap_uint<16> swapWord  (ap_uint<16> inpWord);        // [FIXME - To be replaced w/ byteSwap16]
ap_uint<16> byteSwap16(ap_uint<16> inputVector);

ap_uint<32> swapDWord (ap_uint<32> inpDWord);      // [FIXME - To be replaced w/ byteSwap32]
ap_uint<32> byteSwap32(ap_uint<32> inputVector);

ap_uint<8>  lenToKeep (ap_uint<4> noValidBytes);
ap_uint<8>  returnKeep(ap_uint<4> length);

ap_uint<4>  keepToLen  (ap_uint<8> keepValue);
ap_uint<4>  keepMapping(ap_uint<8> keepValue);

#endif
