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
 * @file       : rlb.hpp
 * @brief      : Defines and prototypes related to the Ready Logic Barrier.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_RLB
 * \{
 *******************************************************************************/

#ifndef _RLB_H
#define _RLB_H

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"

using namespace hls;


/*******************************************************************************
 *
 * ENTITY - READY LOGIC BARRIER (RLB)
 *
 *******************************************************************************/
void rlb(
        //------------------------------------------------------
        //-- MMIO Interface
        //------------------------------------------------------
        StsBit                          *poMMIO_Ready,

        //------------------------------------------------------
        //-- UOE / Data Stream Interface
        //------------------------------------------------------
        stream<StsBool>                 &siUOE_Ready,

        //------------------------------------------------------
        //-- TOE / Data Stream Interface
        //------------------------------------------------------
        stream<StsBool>                 &siTOE_Ready
);

#endif

/*! \} */
