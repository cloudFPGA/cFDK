/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 * @file       : SimNtsUtils.hpp
 * @brief      : Utilities for the simulation of the Network-Transport-Stack
 *               (NTS) components.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{
 *******************************************************************************/

#ifndef _SIM_NTS_UTILS_H_
#define _SIM_NTS_UTILS_H_

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "../MEM/mem.hpp"
#include "../NTS/nts.hpp"
#include "../NTS/nts_utils.hpp"
#include "../NTS/SimNtsUtils.hpp"

using namespace std;

/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/

/******************************************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 ******************************************************************************/

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
  bool readFpgaSndPortFromLine(Ly4Port  &port,     string stringBuffer);
  bool readHostSocketFromLine (SockAddr &hostSock, string stringBuffer);
#endif

/******************************************************************************
 * SIMULATION FILE READ & WRITE HELPERS - PROTOTYPE DEFINITIONS
 ******************************************************************************/
#ifndef __SYNTHESIS__
  bool readAxisRawFromFile  (AxisRaw    &axisRaw,    ifstream &inpFileStream);
  bool readTbParamFromFile  (const string paramName, const string datFile,
                             unsigned int &paramVal);
  int  writeAxisAppToFile   (AxisApp    &axisApp,    ofstream &outFileStream);
  int  writeAxisAppToFile   (AxisApp    &axisApp,    ofstream &outFileStream, int &wrCount);
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

/*! \} */
