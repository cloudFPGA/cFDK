/*******************************************************************************
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
*******************************************************************************/

//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Aug 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains utiliy functions for HLS simulations.
//  *
//  *


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include <stdint.h>
#include <vector>

#include "ap_int.h"

#include "simulation_utils.hpp"

using namespace hls;
using namespace std;


/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxisRaw chunk)
{
    printInfo(callerName, "AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.getTData().to_ulong(), chunk.getTKeep().to_int(), chunk.getTLast().to_int());
}

void printAxiWord(const char *callerName, NetworkWord chunk)
{
    printInfo(callerName, "AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}




