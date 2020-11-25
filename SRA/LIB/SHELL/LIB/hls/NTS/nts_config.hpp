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
 * @file     : nts_config.hpp
 * @brief    : Configuration parameters for the Network Transport Stack (NTS)
 *             component and sub-components.
 *
 * System:   : cloudFPGA
 * Component : Shell
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#ifndef _NTS_CONFIG_H_
#define _NTS_CONFIG_H_

#include <stdint.h>
#include <math.h>

/*******************************************************************************
 * LOCAL HELPER FUNCTION
 *******************************************************************************/
static unsigned long log2ceil(unsigned long val) {
    return (unsigned long)(ceil(log2((double)val)));
}


/*******************************************************************************
 * CONFIGURATION - DATA-LINK LAYER-2 - ETHERNET & ARP
 *******************************************************************************
 * This section defines the configurable parameters related to the data-link
 * layer-2 of the NTS.
 ******************************************************************************/

//--------------------------------------------------------------------
//-- ETHERNET - MAXIMUM TRANSMISSION UNIT
//--------------------------------------------------------------------
static const uint16_t MTU = 1500;

//--------------------------------------------------------------------
//-- ETHERNET - MTU in ZYC2
//--   ZYC2_MTU is 1450 bytes (1500-20-8-8-6-6-2) due to the use of
//--   VXLAN overlay
//--    [OutIpHdr][OutUdpHdr][VxlanHdr][InMacDa][InMacSa][EtherType]
//--------------------------------------------------------------------
static const uint16_t MTU_ZYC2 = 1450;


/*******************************************************************************
 * CONFIGURATION - TRANSPORT LAYER-4 - TCP
 *******************************************************************************
 * This section defines the configurable parameters related to the TCP
 * transport layer-4 of the NTS.
 *******************************************************************************/

//------------------------------------------------------------------
//-- TCP - MAXIMUM SEGMENT SIZE (modulo 8 for efficiency)
//--  FYI: A specific MSS might be advertised by every connection
//--   during the 3-way handshake process. For the time being, we
//--   assume and use a single ZYC2_MSS value for all connections.
//--   This MSS is rounded modulo 8 bytes for better efficiency.
//------------------------------------------------------------------
static const uint16_t ZYC2_MSS  = (MTU_ZYC2-92) & ~0x7; // 1358 & ~0x7 = 1352

//------------------------------------------------------------------
//-- TCP OFFLOAD ENGINE - CONFIGURATION PARAMETERS
//------------------------------------------------------------------
static const uint16_t TOE_MAX_SESSIONS    = 16; // The number of session you want to support (Max. is 10.000)
//OBSOLETE static const uint16_t TOE_TX_MAX_SESSIONS = 10; // Number of Tx Sessions to open for testing

static const uint16_t TOE_WINDOW_BITS     = 16;

static const uint32_t TOE_BUFFER_SIZE     = (1 << TOE_WINDOW_BITS); // 65536
static const uint32_t TOE_RX_BUFFER_SIZE  = TOE_BUFFER_SIZE; // 65536
static const uint32_t TOE_TX_BUFFER_SIZE  = TOE_BUFFER_SIZE; // 65536

#define               TOE_MEMORY_SIZE       0x80000000 // 2GB
#define               TOE_MEMORY_BASE       0x00000000 // Base address
static const uint16_t TOE_MEMORY_BITS     = log2ceil(TOE_MEMORY_SIZE); // 2GB = 2^31

static const uint64_t TOE_RX_MEMORY_SIZE  = TOE_MEMORY_SIZE >> 1; // 1GB
static const uint64_t TOE_TX_MEMORY_SIZE  = TOE_MEMORY_SIZE >> 1; // 1GB
static const uint64_t TOE_RX_MEMORY_BASE = (TOE_MEMORY_BASE);                    // 0x00000000 (Base)
static const uint64_t TOE_TX_MEMORY_BASE = (TOE_MEMORY_SIZE+TOE_RX_MEMORY_SIZE); // 0x40000000 (Base+1GB)
static const uint16_t TOE_RX_MEMORY_BITS  = log2ceil(TOE_RX_MEMORY_SIZE); // 1GB = 2^30
static const uint16_t TOE_TX_MEMORY_BITS  = log2ceil(TOE_TX_MEMORY_SIZE); // 1GB = 2^30


/*******************************************************************************
 * CONFIGURATION - TRANSPORT LAYER-4 - UDP
 *******************************************************************************
 * This section defines the configurable parameters related to the UDP
 * transport layer-4 of the NTS.
 *******************************************************************************/


#endif

/*! \} */


