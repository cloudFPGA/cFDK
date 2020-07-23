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
 * @file       : toe_utils.cpp
 * @brief      : Utilities and helpers for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "toe_utils.hpp"

/*******************************************************************************
 * DEBUG PRINT HELPERS
 *******************************************************************************/
#define THIS_NAME "ToeUtils"

/*******************************************************************************
 * @brief Returns the name of an enum-based event as a user friendly string.
 *
 * @param[in]   evType  An enumerated type of event.
 * @returns the event type as a string.
 *******************************************************************************/
const char *getEventName(EventType evType) {
    switch (evType) {
    case TX_EVENT:
        return "TX";
    case RT_EVENT:
        return "RT";
    case ACK_EVENT:
        return "ACK";
    case SYN_EVENT:
        return "SYN";
    case SYN_ACK_EVENT:
        return "SYN_ACK";
    case FIN_EVENT:
        return "";
    case RST_EVENT:
        return "";
    case ACK_NODELAY_EVENT:
        return "ACK_NODELAY";
    default:
        return "ERROR: UNKNOWN EVENT!";
    }
}

/*******************************************************************************
 * @brief Print a socket pair association from an internal FourTuple encoding.
 *
 * @param[in] callerName  The name of the caller process (e.g. "TAi").
 * @param[in] source      The source of the internal 4-tuple information.
 * @param[in] fourTuple   The internal 4-tuple encoding of the socket pair.
 *******************************************************************************/
void printFourTuple(const char *callerName, int src, FourTuple fourTuple) {
    SocketPair socketPair;
    switch (src) {
        case FROM_RXe:
            socketPair.src.addr = byteSwap32(fourTuple.theirIp);
            socketPair.src.port = byteSwap16(fourTuple.theirPort);
            socketPair.dst.addr = byteSwap32(fourTuple.myIp);
            socketPair.dst.port = byteSwap16(fourTuple.myPort);
            break;
        case FROM_TAi:
            socketPair.src.addr = byteSwap32(fourTuple.myIp);
            socketPair.src.port = byteSwap16(fourTuple.myPort);
            socketPair.dst.addr = byteSwap32(fourTuple.theirIp);
            socketPair.dst.port = byteSwap16(fourTuple.theirPort);
            break;
        default:
            printFatal(callerName, "Unknown request source %d.\n", src);
            break;
    }
    printSockPair(callerName, socketPair);
}

/*! \} */
