/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*******************************************************************************
 * @file       : nts_utils.hpp
 * @brief      : Utilities and helpers for the Network-Transport-Stack (NTS)
 *               components.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _NTS_UTILS_H_
#define _NTS_UTILS_H_

#include "../MEM/mem.hpp"
#include "../NTS/nts.hpp"

using namespace std;

/*******************************************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 *******************************************************************************/
#ifndef __SYNTHESIS__
  extern bool         gTraceEvent;
  extern bool         gFatalError;
  extern unsigned int gSimCycCnt;
#endif


/*******************************************************************************
 * HELPER DESIGN CLASSES
 *******************************************************************************/
#ifndef __SYNTH_LOG2CEIL__
#define __SYNTH_LOG2CEIL__
  /********************************************************
   * A synthesizable version of the C++ log2ceil function.
   * @param[in] n  The input value to compute.
   * @return ceil(log(n)).
   *
   * Usage:
   *   ap_uint<log2Ceil<N>::val> counter;
   *   #define NUM_BITS 8  #define CTRL_BITS LOG2_CEIL<NUM_BITS>::val+1
   ********************************************************/
  template<uint64_t n> struct log2Ceil {
      //-- Code extracted from the book:
      //--  "High-level Synthesis: Blue Book" by By Michael Fingeroff.
      //--  Since the parameter 'n' is usually based on a template parameter,
      //--   it requires the use of enumerated types to perform the computation
      //--   so that the result is statically determinable at compile time.
      //--  One important point to note about using "brute force" approach is
      //--   that there must be sufficient enumerations to cover all of the
      //--   possible values.
      enum {
          val = \
          n <=         0x1 ?  1 : \
          n <=         0x2 ?  1 : \
          n <=         0x4 ?  2 : \
          n <=         0x8 ?  3 : \
          n <=        0x10 ?  4 : \
          n <=        0x20 ?  5 : \
          n <=        0x40 ?  6 : \
          n <=        0x80 ?  7 : \
          n <=       0x100 ?  8 : \
          n <=       0x200 ?  9 : \
          n <=       0x400 ? 10 : \
          n <=       0x800 ? 11 : \
          n <=      0x1000 ? 12 : \
          n <=      0x2000 ? 13 : \
          n <=      0x4000 ? 14 : \
          n <=      0x8000 ? 15 : \
          n <=     0x10000 ? 16 : \
          n <=     0x20000 ? 17 : \
          n <=     0x40000 ? 18 : \
          n <=     0x80000 ? 19 : \
          n <=    0x100000 ? 20 : \
          n <=    0x200000 ? 21 : \
          n <=    0x400000 ? 22 : \
          n <=    0x800000 ? 23 : \
          n <=   0x1000000 ? 24 : \
          n <=   0x2000000 ? 25 : \
          n <=   0x4000000 ? 26 : \
          n <=   0x8000000 ? 27 : \
          n <=  0x10000000 ? 28 : \
          n <=  0x20000000 ? 29 : \
          n <=  0x40000000 ? 30 : \
          n <=  0x80000000 ? 31 : \
          n <= 0x100000000 ? 32 : \
          33
      };
  };
#endif

/*******************************************************************************
 * AXIS CHUNK UTILITIES - PROTOTYPE DEFINITIONS
 *******************************************************************************/
tKeep          lenTotKeep(ap_uint<4>  noValidBytes);
LE_tKeep       lenToLE_tKeep (ap_uint<4>  noValidBytes);

ap_uint<16>    byteSwap16(ap_uint<16> inputValue);
ap_uint<32>    byteSwap32(ap_uint<32> inputValue);
ap_uint<48>    byteSwap48(ap_uint<48> inputValue);
ap_uint<64>    byteSwap64(ap_uint<64> inputValue);

/*******************************************************************************
 * ENUM TO STRING HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
const char *getTcpStateName(TcpState tcpState);

/*******************************************************************************
 * PRINT HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
void printAxisRaw      (const char *callerName, AxisRaw       chunk);
void printAxisRaw      (const char *callerName, \
                        const char *message,    AxisRaw       chunk);
void printDmCmd        (const char *callerName, DmCmd         dmCmd);
void printArpBindPair  (const char *callerName, ArpBindPair   arpBind);
void printSockAddr     (const char *callerName, SockAddr      sockAddr);
void printSockAddr     (const char *callerName, LE_SockAddr   leSockAddr);
void printSockAddr     (                        SockAddr      sockAddr);
void printSockPair     (const char *callerName, SocketPair    sockPair);
void printSockPair     (const char *callerName, LE_SocketPair leSockPair);
void printLE_SockAddr  (const char *callerName, LE_SockAddr   leSockAddr);
void printLE_SockPair  (const char *callerName, LE_SocketPair leSockPair);
void printIp4Addr      (const char *callerName, \
                        const char *message,    Ip4Addr       ip4Addr);
void printIp4Addr      (const char *callerName, Ip4Addr       ip4Addr);
void printEthAddr      (const char *callerName, \
                        const char *message,    EthAddr       ethAddr);
void printEthAddr      (const char *callerName, EthAddr       ethAddr);
void printEthAddr      (                        EthAddr       ethAddr);
void printTcpPort      (const char *callerName, TcpPort       tcpPort);
void printTcpPort      (                        TcpPort       tcpPort);

/*******************************************************************************
 * MACRO DEFINITIONS
 *******************************************************************************/
// Concatenate two char constants
#define concat2(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat3(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst

/*********************************************************
 * @brief A macro to print an information message.
 * @param[in] callerName The name of the caller process (e.g. "TB/IPRX").
 * @param[in] message    The message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printInfo(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%-20s] INFO - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printInfo(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a warning message.
 * @param[in] callerName  The name of the caller process (e.g. "TB/IPRX").
 * @param[in] message     The message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printWarn(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%-20s] WARNING - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printWarn(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print an error message.
 * @param[in] callerName  The name of the caller process (e.g. "TB/IPRX").
 * @param[in] message     The message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printError(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%-20s] ERROR - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printError(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a fatal error message and exit.
 * @param[in] callerName  The name of the caller process (e.g. "TB/IPRX").
 * @param[in] message     The message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printFatal(callerName , format, ...) \
    do { gTraceEvent = true; gFatalError = true; printf("\n(@%5.5d) [%-20s] FATAL - " format, gSimCycCnt, callerName, ##__VA_ARGS__); printf("\n\n"); exit(99); } while (0)
#else
  #define printFatal(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro that checks if a stream is full.
 * @param[in] callerName  The name of the caller process (e.g. "TB/IPRX").
 * @param[in] stream      The stream to test.
 * @param[in] streamName  The name of the stream (e.g. "soEVe_RxEventSig").
 * @param[in] depth       The depth of the implemented FIFO.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define assessSize(callerName , stream , streamName, depth) \
    do { if (stream.size() >= depth) printFatal(callerName, "Stream \'%s\' is full: Cannot write.", streamName); } while (0)
#else
  #define assessSize(callerName , stream, streamName, depth) \
    do {} while (0);
#endif

#endif

/*! \} */
