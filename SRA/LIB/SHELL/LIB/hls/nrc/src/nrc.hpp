//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        A interface between the UDP stack and the NRC (mainly a buffer).
//  *

#ifndef _NRC_H_
#define _NRC_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

#include "../../../../../hls/network_utils.hpp"


using namespace hls;

#define DEFAULT_TX_PORT 2718
#define DEFAULT_RX_PORT 2718

//enum FsmState {FSM_RESET=0, FSM_IDLE, FSM_W8FORPORT, FSM_FIRST_ACC, FSM_ACC} fsmState;
//static enum FsmState {FSM_RST=0, FSM_ACC, FSM_LAST_ACC} fsmState;
//static enum FsmState {FSM_W8FORMETA=0, FSM_FIRST_ACC, FSM_ACC} fsmState;
#define FSM_RESET 0
#define FSM_IDLE  1
#define FSM_W8FORPORT 2
#define FSM_FIRST_ACC 3
#define FSM_ACC 4
#define FSM_LAST_ACC 5
#define FSM_W8FORMETA 6
#define FsmState uint8_t



void nrc(
    // ----- system reset ---
    ap_uint<1> sys_reset,

    //------------------------------------------------------
    //-- ROLE / This / Udp Interfaces
    //------------------------------------------------------
    stream<UdpWord>     &siROL_This_Data,
    stream<UdpWord>     &soTHIS_Rol_Data,
    stream<IPMeta>      &siIP,
    stream<IPMeta>      &soIP,
    ap_uint<32>         *myIpAddress,

    //------------------------------------------------------
    //-- UDMX / This / Open-Port Interfaces
    //------------------------------------------------------
    stream<AxisAck>     &siUDMX_This_OpnAck,
    stream<UdpPort>     &soTHIS_Udmx_OpnReq,

    //------------------------------------------------------
    //-- UDMX / This / Data & MetaData Interfaces
    //------------------------------------------------------
    stream<UdpWord>     &siUDMX_This_Data,
    stream<UdpMeta>     &siUDMX_This_Meta,
    stream<UdpWord>     &soTHIS_Udmx_Data,
    stream<UdpMeta>     &soTHIS_Udmx_Meta,
    stream<UdpPLen>     &soTHIS_Udmx_Len
);

#endif


