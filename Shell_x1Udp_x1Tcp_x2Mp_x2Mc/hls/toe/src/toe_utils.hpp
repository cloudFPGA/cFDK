/*****************************************************************************
 * @file       : toe_utils.hpp
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

#ifndef TOE_UTILS_H_
#define TOE_UTILS_H_

#include <stdio.h>



/*************************************************************************
 * MACRO DEFINITIONS
 *************************************************************************/

// Concatenate two char constants
#define concat(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst


/*************************************************************************
 * PROTOTYPE DEFINITIONS
 *************************************************************************/
//char * concat(const char *first, const char *second);
void printAxiWord(const char *callerName, AxiWord chunk);


#endif
