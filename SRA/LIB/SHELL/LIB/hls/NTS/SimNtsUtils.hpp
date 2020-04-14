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
 * @file       : sim_nts_utils.hpp
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

//#include "../src/toe.hpp"
//#include "../src/toe_utils.hpp"
//#include "../../AxisApp.hpp"
//#include "../../AxisArp.hpp"
//#include "../../AxisEth.hpp"
//#include "../../AxisIp4.hpp"
//#include "../../AxisIcmp.hpp"

using namespace std;

/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
//class ArpBindPair;
//class SockAddr;
//class SocketPair;
//class LE_SockAddr;
//class LE_SocketPair;
//class AxiWord;
//class DmCmd;
//class Ip4overMac;
//class SLcFourTuple;

/******************************************************************************
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
void printSockPair     (const char *callerName, int  src,     SLcFourTuple fourTuple);
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




/******************************************************************************
 * LINE READ & WRITER HELPERS - PROTOTYPE DEFINITIONS
 ******************************************************************************/
bool readAxisRawFromLine    (AxisRaw  &axiRaw,  string stringBuffer);
// bool readFpgaSndPortFromLine(Ly4Port  &port,     string stringBuffer);
// bool readHostSocketFromLine (SockAddr &hostSock, string stringBuffer);

/******************************************************************************
 * FILE READ & WRITER HELPERS - PROTOTYPE DEFINITIONS
 ******************************************************************************/
bool readAxisRawFromFile(AxisRaw &axisRaw, ifstream &inpFileStream);
bool writeAxisRawToFile (AxisRaw &axisRaw, ofstream &outFileStream);

/******************************************************************************
 * STREAM WRITER HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
bool feedAxiWordStreamFromFile(stream<AxiWord> &ss,    const string ssName,
                               string      datFile,    int       &nrChunks,
                               int       &nrFrames,    int        &nrBytes);
bool drainAxiWordStreamToFile (stream<AxiWord> &ss,    const string ssName,
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
