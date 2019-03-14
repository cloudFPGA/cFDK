/*****************************************************************************
 * @file       : toe_uitls.cpp
 * @brief      : Utilities for the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <queue>

#include "toe_utils.hpp"



/*****************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord, the 16-bit unsigned data to swap.
 *
 * @return a 16-bit unsigned data.
 *****************************************************************************/
ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
    return (inputVector.range(7,0), inputVector(15, 8));
}

/*****************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord, a 32-bit unsigned data.
 *
 * @return a 32-bit unsigned data.
 *****************************************************************************/
ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
    return (inputVector.range( 7, 0), inputVector(15,  8),
        inputVector.range(23,16), inputVector(31, 24));
}

/*****************************************************************************
 * @brief Returns the number of valid bytes in an AxiWord.
 * @param[in] The 'tkeep' field of the AxiWord.
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepValue) {
    ap_uint<4> count = 0;
    switch(keepValue){
        case 0x01: count = 1; break;
        case 0x03: count = 2; break;
        case 0x07: count = 3; break;
        case 0x0F: count = 4; break;
        case 0x1F: count = 5; break;
        case 0x3F: count = 6; break;
        case 0x7F: count = 7; break;
        case 0xFF: count = 8; break;
    }
    return count;
}

/*****************************************************************************
 * @brief A function to count the number of 1s in an 8-bit value.
 *        [FIXME - To replace by keepToLen]
 *****************************************************************************/
ap_uint<4> keepMapping(ap_uint<8> keepValue) {
    ap_uint<4> counter = 0;

    switch(keepValue){
        case 0x01: counter = 1; break;
        case 0x03: counter = 2; break;
        case 0x07: counter = 3; break;
        case 0x0F: counter = 4; break;
        case 0x1F: counter = 5; break;
        case 0x3F: counter = 6; break;
        case 0x7F: counter = 7; break;
        case 0xFF: counter = 8; break;
    }
    return counter;
}

/*****************************************************************************
 * @brief Returns the 'tkeep' field of an AxiWord as a function of the number
 *  of valid bytes in that word.
 * @param[in] The number of valid bytes in an AxiWord.
 *****************************************************************************/
ap_uint<8> lenToKeep(ap_uint<4> noValidBytes) {
    ap_uint<8> keep = 0;

    switch(noValidBytes) {
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}


/*****************************************************************************
 * @brief A function to set a number of 1s in an 8-bit value.
 *         [FIXME - To replace by lenToKeep]
 *****************************************************************************/
ap_uint<8> returnKeep(ap_uint<4> count) {
    ap_uint<8> keep = 0;

    switch(count){
        case 1: keep = 0x01; break;
        case 2: keep = 0x03; break;
        case 3: keep = 0x07; break;
        case 4: keep = 0x0F; break;
        case 5: keep = 0x1F; break;
        case 6: keep = 0x3F; break;
        case 7: keep = 0x7F; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}


/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk)
{
    printInfo(callerName, "AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}

/*****************************************************************************
 * @brief Prints the details of a Data Mover Command (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mwr").
 * @param[in] dmCmd,        the data mover command to display.
 *****************************************************************************/
void printDmCmd(const char *callerName, DmCmd dmCmd)
{
    printInfo(callerName, "DmCmd = {BBT=0x%6.6X, TYPE=0x%1.1X DSA=0x%2.2X, EOF=0x%1.1X, DRR=0x%1.1X, SADDR=0x%8.8X, TAG=0x%1.1X} \n",
              dmCmd.bbt.to_uint(), dmCmd.type.to_uint(), dmCmd.dsa.to_uint(), dmCmd.eof.to_uint(), dmCmd.drr.to_uint(), dmCmd.saddr.to_uint(), dmCmd.tag.to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association encoded in LITTLE-ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printAxiSockPair(const char *callerName, AxiSocketPair sockPair)
{
    printInfo(callerName, "MacSocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(),
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association encoded in NETWORK-BYTE order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockPair(const char *callerName, SocketPair sockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(),
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket address encoded in LITTLE_ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printAxiSockAddr(const char *callerName, AxiSockAddr sockAddr)
{
    printInfo(callerName, "MacSocketAddr {IpAddr,TcpPort} = {0x%8.8X,0x%4.4X} \n",
        sockAddr.addr.to_uint(), sockAddr.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket address encoded in NETWORK-BYTE order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockAddr(const char *callerName, SockAddr sockAddr)
{
    printInfo(callerName, "SocketAddr {IpAddr,TcpPort} = {0x%8.8X,0x%4.4X} \n",
        sockAddr.addr.to_uint(), sockAddr.port.to_uint());
}


