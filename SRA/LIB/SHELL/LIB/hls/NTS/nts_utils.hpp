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
 * Component   : Shell, Network Transport Session (NTS)
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
#include "../NTS/nts_utils.hpp"

using namespace std;

/*******************************************************************************
 * FORWARD DECLARATIONS
 *******************************************************************************/
//OBSOLETE_20200618 class ArpBindPair;
//OBSOLETE_20200618 class SockAddr;*
//OBSOLETE_20200618 class SocketPair;
//OBSOLETE_20200618 class LE_SockAddr;
//OBSOLETE_20200618 class LE_SocketPair;
//OBSOLETE_20200618 class AxiWord;
//OBSOLETE_20200618 class DmCmd;
//OBSOLETE_20200618 class Ip4overMac;
//OBSOLETE_20200415 class SLcFourTuple;

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
#ifndef __LOG2CEIL__
#define __LOG2CEIL__
  /********************************************************
   * A synthesizable version of the C++ log2ceil function.
   * @param[in] n  The input value to compute.
   * @return ceil(log(n)).
   *
   * Usage: ap_uint<Log2Ceil<N>::val> counter;
   ********************************************************/
  template<int n> struct Log2Ceil {
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
          n <=     1 ?  1 : \
          n <=     2 ?  1 : \
          n <=     4 ?  2 : \
          n <=     8 ?  3 : \
          n <=    16 ?  4 : \
          n <=    32 ?  5 : \
          n <=    64 ?  6 : \
          n <=   128 ?  7 : \
          n <=   256 ?  8 : \
          n <=   512 ?  9 : \
          n <=  1024 ? 10 : \
          n <=  2048 ? 11 : \
          n <=  4096 ? 12 : \
          n <=  8192 ? 13 : \
          n <= 16384 ? 14 : \
          n <= 32768 ? 15 : \
          n <= 65536 ? 16 :
          32
      };
  };
#endif

/*******************************************************************************
 * AXIS CHUNK UTILITIES - PROTOTYPE DEFINITIONS
 *******************************************************************************/
tKeep          lenTotKeep(ap_uint<4>  noValidBytes);
LE_tKeep       lenToLE_tKeep (ap_uint<4>  noValidBytes);

//OBSOLETE_20200711 ap_uint<4>     keepToLen (ap_uint<8>  keepValue);
ap_uint<16>    byteSwap16(ap_uint<16> inputValue);
ap_uint<32>    byteSwap32(ap_uint<32> inputValue);
ap_uint<48>    byteSwap48(ap_uint<48> inputValue);
ap_uint<64>    byteSwap64(ap_uint<64> inputValue);

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
//OBSOLETE_20200415 void printSockPair     (const char *callerName, int  src,     SLcFourTuple fourTuple);
void printLE_SockAddr  (const char *callerName, LE_SockAddr   leSockAddr);
void printLE_SockPair  (const char *callerName, LE_SocketPair leSockPair);
void printIp4Addr      (const char *callerName, \
                        const char *message,    Ip4Addr       ip4Addr);
void printIp4Addr      (const char *callerName, Ip4Addr       ip4Addr);
void printIp4Addr      (                        Ip4Addr       ip4Addr);
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
    do { gTraceEvent = true; printf("(@%5.5d) [%s] INFO - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
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
    do { gTraceEvent = true; printf("(@%5.5d) [%s] WARNING - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
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
    do { gTraceEvent = true; printf("(@%5.5d) [%s] ERROR - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
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
    do { gTraceEvent = true; gFatalError = true; printf("\n(@%5.5d) [%s] FATAL - " format, gSimCycCnt, callerName, ##__VA_ARGS__); printf("\n\n"); exit(99); } while (0)
#else
  #define printFatal(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro that checks if a stream is full.
 * @param[in] callerName  The name of the caller process (e.g. "TB/IPRX").
 * @param[in] stream      The stream to test.
 * @param[in] streamName  The name of the stream (e.g. "soEVe_RxEventSig").
 **********************************************************/
/*** [FIXME - MUST BE REMOVED] ********
#ifndef __SYNTHESIS__
  #define assessFull(callerName , stream , streamName) \
    do { if (stream.full()) printFatal(callerName, "Stream \'%s\' is full: Cannot write.", streamName); } while (0)
#else
  #define assessFull(callerName , stream, streamName) \
    do {} while (0);
#endif
***************************************/

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
