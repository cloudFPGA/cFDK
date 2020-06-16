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

/*****************************************************************************
 * @file       : SimNtsUtils.hpp
 * @brief      : Utilities for the simulation of the Network-Transport-Stack
 *               (NTS) components.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#ifndef SIM_NTS_UTILS_H_
#define SIM_NTS_UTILS_H_

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "../MEM/mem.hpp"
#include "../NTS/nts.hpp"
#include "../NTS/nts_utils.hpp"
#include "../NTS/SimNtsUtils.hpp"
#include "../NTS/AxisApp.hpp"
//#include "AxisArp.hpp"
//#include "../../AxisEth.hpp"
//#include "../../AxisIp4.hpp"
//#include "../../AxisIcmp.hpp"

using namespace std;

/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
//OBSOLETE
//class ArpBindPair;
//class SockAddr;
//class SocketPair;
//class LE_SockAddr;
//class LE_SocketPair;
//class AxiWord;
//class DmCmd;
//class Ip4overMac;
//OBSOLETE_20200415 class SLcFourTuple;

/******************************************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 ******************************************************************************/
//OBSOLETE - WAS MOVED TO nts_utils.hpp
//#ifndef __SYNTHESIS__
//  extern bool         gTraceEvent;
//  extern bool         gFatalError;
//  extern unsigned int gSimCycCnt;
//#endif

/******************************************************************************
 * SIMULATION UTILITY HELPERS -  PROTOTYPE DEFINITIONS
 *******************************************************************************/
#ifndef __SYNTHESIS__
  bool           isDatFile       (string      fileName);
  bool           isDottedDecimal (string      ipStr);
  bool           isHexString     (string      str);
  ap_uint<32>    myDottedDecimalIpToUint32(string ipStr);
  vector<string> myTokenizer     (string      strBuff, char delimiter);
  string         myUint64ToStrHex(ap_uint<64> inputNumber);
  string         myUint8ToStrHex (ap_uint<8>  inputNumber);
  ap_uint<64>    myStrHexToUint64(string      dataString);
  ap_uint<8>     myStrHexToUint8 (string      keepString);
  int            myDiffTwoFiles  (string      dataFileName, string goldFileName);
#endif

/******************************************************************************
 * SIMULATION LINE READ & WRITE HELPERS - PROTOTYPE DEFINITIONS
 ******************************************************************************/
#ifndef __SYNTHESIS__
  bool readAxisRawFromLine    (AxisRaw  &axiRaw,   string stringBuffer);
  bool readFpgaSocketFromLine (SockAddr &hostSock, string stringBuffer);
  bool readHostSocketFromLine (SockAddr &hostSock, string stringBuffer);
  bool readFpgaSndPortFromLine(Ly4Port  &port,     string stringBuffer);
#endif

/******************************************************************************
 * SIMULATION FILE READ & WRITE HELPERS - PROTOTYPE DEFINITIONS
 ******************************************************************************/
#ifndef __SYNTHESIS__
  bool readAxisRawFromFile  (AxisRaw    &axisRaw,    ifstream &inpFileStream);
  bool writeAxisRawToFile   (AxisRaw    &axisRaw,    ofstream &outFileStream);
  bool writeSocketPairToFile(SocketPair &socketPair, ofstream &outFileStream);
  template <int D> \
  bool writeApUintToFile    (ap_uint<D> &data,       ofstream &outFileStream);
#endif

/******************************************************************************
 * SIMULATION STREAM WRITER HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
#ifndef __SYNTHESIS__
  bool feedAxisRawStreamFromFile(stream<AxisRaw> &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  bool drainAxisRawStreamToFile (stream<AxisRaw> &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  template <class AXIS_T> \
  bool feedAxisFromFile(stream<AXIS_T>           &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  template <class AXIS_T> \
  bool drainAxisToFile(stream<AXIS_T>            &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
#endif

#endif
