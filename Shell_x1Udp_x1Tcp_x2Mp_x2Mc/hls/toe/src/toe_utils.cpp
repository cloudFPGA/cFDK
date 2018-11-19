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
 *----------------------------------------------------------------------------
 *
 * @details    :
 * @note       :
 * @remark     :
 * @warning    :
 * @todo       :
 *
 * @see        :
 *
 *****************************************************************************/

// #include <cstring>

//#include "toe_utils.hpp"
#include "toe.hpp"


//*** [ FIXME - Consider adding byteSwap16 and byteSwap32 here]  


/*****************************************************************************
 * @brief Concatenate two char constants (used for debugging).
 *
 * @param[in] first,  the first chat constant. (e.g. "Tle").
 * @param[in] second, the second char constant ((e.g. "RXE").
 *****************************************************************************/
/*char * concat(const char *first, const char *second)
{
    char buf[100];
    strcpy(buf, first);
    strcat(buf, "/");
    strcat(buf, second);
    return buf;
}*/


/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk)
{
    printf("[%s] AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
           callerName, chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}





