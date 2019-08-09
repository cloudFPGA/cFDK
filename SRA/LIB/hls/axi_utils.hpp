//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains AXI types and functions that are shared between SHELL and ROLE.
//  *

#ifndef _CF_AXI_USER_UTILS_
#define _CF_AXI_USER_UTILS_


#include <stdint.h>
#include <stdio.h>

#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>


using namespace hls;

/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
 template<int D>
   struct Axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
   };


#endif
