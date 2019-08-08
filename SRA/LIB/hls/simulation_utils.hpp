//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Aug 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains memory types and functions that are shared accross HLS cores. 
//  *
//  *

#ifndef _CF_SIMULATION_UTILS_
#define _CF_SIMULATION_UTILS_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include <stdint.h>
#include <vector>

#include "ap_int.h"

using namespace hls;


#ifndef __SYNTHESIS__
  // HowTo - You should adjust the value of 'TIME_1s' such that the testbench
  //   works with your longest segment. In other words, if 'TIME_1s' is too short
  //   and/or your segment is too long, you may experience retransmission events
  //   (RT) which will break the test. You may want to use 'appRx_OneSeg.dat' or
  //   'appRx_TwoSeg.dat' to tune this parameter.
  static const ap_uint<32> TIME_1s        =   25;

  static const ap_uint<32> TIME_1ms       = (((ap_uint<32>)(TIME_1s/1000) > 1) ? (ap_uint<32>)(TIME_1s/1000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_5ms       = (((ap_uint<32>)(TIME_1s/ 200) > 1) ? (ap_uint<32>)(TIME_1s/ 200) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_25ms      = (((ap_uint<32>)(TIME_1s/  40) > 1) ? (ap_uint<32>)(TIME_1s/  40) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_50ms      = (((ap_uint<32>)(TIME_1s/  20) > 1) ? (ap_uint<32>)(TIME_1s/  20) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_100ms     = (((ap_uint<32>)(TIME_1s/  10) > 1) ? (ap_uint<32>)(TIME_1s/  10) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_250ms     = (((ap_uint<32>)(TIME_1s/   4) > 1) ? (ap_uint<32>)(TIME_1s/   4) : (ap_uint<32>)1);

  static const ap_uint<32> TIME_5s        = (  5*TIME_1s);
  static const ap_uint<32> TIME_7s        = (  7*TIME_1s);
  static const ap_uint<32> TIME_10s       = ( 10*TIME_1s);
  static const ap_uint<32> TIME_15s       = ( 15*TIME_1s);
  static const ap_uint<32> TIME_20s       = ( 20*TIME_1s);
  static const ap_uint<32> TIME_30s       = ( 30*TIME_1s);
  static const ap_uint<32> TIME_60s       = ( 60*TIME_1s);
  static const ap_uint<32> TIME_120s      = (120*TIME_1s);
#else
  static const ap_uint<32> TIME_1ms       = (  0.001/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5ms       = (  0.005/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_25ms      = (  0.025/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_50ms      = (  0.050/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_100ms     = (  0.100/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_250ms     = (  0.250/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_1s        = (  1.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5s        = (  5.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_7s        = (  7.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_10s       = ( 10.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_15s       = ( 15.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_20s       = ( 20.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_30s       = ( 30.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_60s       = ( 60.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_120s      = (120.000/0.0000000064/MAX_SESSIONS) + 1;
#endif

#ifndef __SYNTHESIS__
    const std::string  SessionStateString[] = {
                   "CLOSED",    "SYN_SENT",  "SYN_RECEIVED", "ESTABLISHED", \
                   "FIN_WAIT_1","FIN_WAIT_2","CLOSING",      "TIME_WAIT",   \
                   "LAST_ACK" };
#endif


#endif

