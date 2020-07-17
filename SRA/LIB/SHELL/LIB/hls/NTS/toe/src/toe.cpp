/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*******************************************************************************
 * @file     : toe.cpp
 * @brief    : TCP Offload Engine (TOE)
 *
 * System:   : cloudFPGA
 * Component : Network Transport Stack (NTS)
 * Language  : Vivado HLS
 *
 * @remarks  :
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network. A PDU is
 *   therefore composed of a protocol specific control information (e.g a header)
 *   and a user data section.
 *  This source code uses the following terminology:
 *   - a SEGMENT (or TCP Packet) refers to the TCP protocol data unit.
 *   - a PACKET  (or IP  Packet) refers to the IP protocol data unit.
 *   - a FRAME   (or MAC Frame)  refers to the Ethernet data link layer.
 *
 * \ingroup NTS_TOE
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#include "toe.hpp"
#include "toe_utils.hpp"
#include "./session_lookup_controller/session_lookup_controller.hpp"
#include "./state_table/state_table.hpp"
#include "./rx_sar_table/rx_sar_table.hpp"
#include "./tx_sar_table/tx_sar_table.hpp"
#include "./timers/timers.hpp"
#include "./event_engine/event_engine.hpp"
#include "./ack_delay/ack_delay.hpp"
#include "./port_table/port_table.hpp"
#include "./rx_app_interface/rx_app_interface.hpp"
#include "./tx_app_interface/tx_app_interface.hpp"
#include "./rx_engine/src/rx_engine.hpp"
#include "./tx_engine/src/tx_engine.hpp"

/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
#define USE_DEPRECATED_DIRECTIVES

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (TRACE_OFF | TRACE_RDY)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif
#define THIS_NAME "TOE"

#define TRACE_OFF  0x0000
#define TRACE_RDY 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief A 2-to-1 generic Stream Multiplexer
 *
 *  @param[in]  si1     The input stream #1.
 *  @param[in]  si2     The input stream #2.
 *  @param[out] so      The output stream.
 *
 * @details
 *  This multiplexer behaves like an arbiter. It takes two streams as inputs and
 *   forwards one of them to the output channel. The stream connected to the
 *   first input always takes precedence over the second one.
 *******************************************************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so) {

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    if (!si1.empty())
        so.write(si1.read());
    else if (!si2.empty())
        so.write(si2.read());
}


/*******************************************************************************
 * @brief Ready (Rdy)
 *
 * @param[in]  piPRt_Ready  The ready signal from PortTable (PRt).
 * @param[in]  piTBD_Ready  The ready signal from TBD.
 * @param[out] poNTS_Ready  The ready signal of the TOE.
 *
 * @details
 *  This process merges multiple ready signals into a single global ready
 *   signal for the TOE.
 *******************************************************************************/
void pReady(
    StsBool     &piPRt_Ready,
    StsBit      &poNTS_Ready)
{
    const char *myName = concat3(THIS_NAME, "/", "Rdy");

    poNTS_Ready = (piPRt_Ready == true) ? 1 : 0;

    if (DEBUG_LEVEL & TRACE_RDY) {
        if (poNTS_Ready)
            printInfo(myName, "Process [TOE] is ready.\n");
    }
}

/*******************************************************************************
 * @brief Testbench Simulation Counter
 *
 * @param[out] poSimCycCount  The incremented simulation counter.
 *
 * @details
 *  This process is provided for debugging only. It allows to implement the
 *   simulation a counter of the testbench within the TOE itself.
 *******************************************************************************/
void pTbSimCount(
    volatile ap_uint<32>    &poSimCycCount)
{
    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    #ifdef __SYNTHESIS__
        static ap_uint<32>         sCounter = 0xFFFFFFF9;
    #else
        static ap_uint<32>         sCounter = 0x00000000;
    #endif
    #pragma HLS reset variable=sCounter

    sCounter += 1;
    // [Un-comment this line ]poSimCycCount = sCounter;
}


/*******************************************************************************
 * @brief   Main process of the TCP Offload Engine.
 *
 * -- MMIO Interfaces
 * @param[in]  piMMIO_IpAddr    IP4 Address from MMIO.
 * -- NTS Interfaces
 * @param[out] poNTS_Ready      Ready signal of the TOE.
 * -- IPRX / IP Rx / Data Interface
 * @param[in]  siIPRX_Data      IP4 data stream from IPRX.
 * -- L3MUX / IP Tx / Data Interface
 * @param[out] soL3MUX_Data     IP4 data stream to L3MUX.
 * -- TAIF / Rx Segment Interfaces
 * @param[out] soTAIF_Notif     TCP notification to TAIF.
 * @param[in]  siTAIF_DReq      TCP data request from TAIF.
 * @param[out] soTAIF_Data      TCP data stream to TAIF.
 * @param[out] soTAIF_Meta      TCP metadata stream to TAIF.
 * -- TAIF / Listen Port Interfaces
 * @param[in]  siTAIF_LsnReq    TCP listen port request from TAIF.
 * @param[out] soTAIF_LsnRep    TCP listen port reply to TAIF.
 * -- TAIF / Tx Data Interfaces
 * @param[in]  siTAIF_Data      TCP data stream from TAIF.
 * @param[in]  siTAIF_Meta      TCP metadata stream from TAIF.
 * @param[out] soTAIF_DSts      TCP data write status to TAIF.
 * -- TAIF / Open Interfaces
 * @param[in]  siTAIF_OpnReq    TCP open port request from TAIF.
 * @param[out] soTAIF_OpnRep    TCP open port reply to TAIF.
 * -- TAIF / Close Interfaces
 * @param[in]  siTAIF_ClsReq    TCP close connection request from TAIF.
 * @warning:   Not-Used         TCP close connection status to TAIF.
 * -- MEM / Rx PATH / S2MM Interface
 * @warning:   Not-Used         Rx memory read status from MEM.
 * @param[out] soMEM_RxP_RdCmd  Rx memory read command to MEM.
 * @param[in]  siMEM_RxP_Data   Rx memory data from MEM.
 * @param[in]  siMEM_RxP_WrSts  Rx memory write status from MEM.
 * @param[out] soMEM_RxP_WrCmd  Rx memory write command to MEM.
 * @param[out] soMEM_RxP_Data   Rx memory data to MEM.
 * -- MEM / Tx PATH / S2MM Interface
 * @warning:   Not-Used         Tx memory read status from MEM.
 * @param[out] soMEM_TxP_RdCmd  Tx memory read command to MEM.
 * @param[in]  siMEM_TxP_Data   Tx memory data from MEM.
 * @param[in]  siMEM_TxP_WrSts  Tx memory write status from MEM.
 * @param[out] soMEM_TxP_WrCmd  Tx memory write command to MEM.
 * @param[out] soMEM_TxP_Data   Tx memory data to MEM.
 * -- CAM / Session Lookup & Update Interfaces
 * @param[in]  siCAM_SssLkpRep  Session lookup reply from CAM.
 * @param[in]  siCAM_SssUpdRep  Session update reply from CAM.
 * @param[out] soCAM_SssLkpReq  Session lookup request to CAM.
 * @param[out] soCAM_SssUpdReq  Session update request to CAM.
 * -- DEBUG / Session Statistics Interfaces
 * @param[out] poDBG_SssRelCnt  Session release count to DEBUG.  // [FIXMS-Remove]
 * @param[out] poDBG_SssRegCnt  Session register count to DEBUG. // [FIXMS-Remove]
 *******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr                              piMMIO_IpAddr,

        //------------------------------------------------------
        //-- NTS Interfaces
        //------------------------------------------------------
        StsBit                              &poNTS_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                     &siIPRX_Data,

        //------------------------------------------------------
        //-- L3MUX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                     &soL3MUX_Data,

        //------------------------------------------------------
        //-- TAIF / Rx Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>                 &soTAIF_Notif,
        stream<TcpAppRdReq>                 &siTAIF_DReq,
        stream<TcpAppData>                  &soTAIF_Data,
        stream<TcpAppMeta>                  &soTAIF_Meta,

        //------------------------------------------------------
        //-- TAIF / Listen Port Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>                &siTAIF_LsnReq,
        stream<TcpAppLsnRep>                &soTAIF_LsnRep,

        //------------------------------------------------------
        //-- TAIF / Tx Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppData>                  &siTAIF_Data,
        stream<TcpAppMeta>                  &siTAIF_Meta,
        stream<TcpAppWrSts>                 &soTAIF_DSts,

        //------------------------------------------------------
        //-- TAIF / Open connection Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>                &siTAIF_OpnReq,
        stream<TcpAppOpnRep>                &soTAIF_OpnRep,

        //------------------------------------------------------
        //-- TAIF / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>                &siTAIF_ClsReq,
        //-- Not USed                       &soTAIF_ClsSts,

        //------------------------------------------------------
        //-- MEM / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_RxP_RdSts,
        stream<DmCmd>                       &soMEM_RxP_RdCmd,
        stream<AxisApp>                     &siMEM_RxP_Data,
        stream<DmSts>                       &siMEM_RxP_WrSts,
        stream<DmCmd>                       &soMEM_RxP_WrCmd,
        stream<AxisApp>                     &soMEM_RxP_Data,

        //------------------------------------------------------
        //-- MEM / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                       &siMEM_TxP_RdSts,
        stream<DmCmd>                       &soMEM_TxP_RdCmd,
        stream<AxisApp>                     &siMEM_TxP_Data,
        stream<DmSts>                       &siMEM_TxP_WrSts,
        stream<DmCmd>                       &soMEM_TxP_WrCmd,
        stream<AxisApp>                     &soMEM_TxP_Data,

        //------------------------------------------------------
        //-- CAM / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<CamSessionLookupRequest>     &soCAM_SssLkpReq,
        stream<CamSessionLookupReply>       &siCAM_SssLkpRep,
        stream<CamSessionUpdateRequest>     &soCAM_SssUpdReq,
        stream<CamSessionUpdateReply>       &siCAM_SssUpdRep,

        //------------------------------------------------------
        //-- DEBUG Interfaces
        //------------------------------------------------------
        ap_uint<16>                         &poDBG_SssRelCnt,
        ap_uint<16>                         &poDBG_SssRegCnt
        //-- NU-DEBUG / ap_uint<32>         &poSimCycCount
        )
{

    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

#if defined(USE_DEPRECATED_DIRECTIVES)

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_stable          port=piMMIO_IpAddr
    //-- NTS Interfaces
    #pragma HLS INTERFACE ap_none register   port=poNTS_Ready
    //-- IPRX / IP Rx Data Interface ------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siIPRX_Data     metadata="-bus_bundle siIPRX_Data"
    //-- L3MUX / IP Tx Data Interface -----------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soL3MUX_Data    metadata="-bus_bundle soL3MUX_Data"
    //-- TAIF / ROLE Rx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTAIF_DReq     metadata="-bus_bundle siTAIF_DReq"
    #pragma HLS DATA_PACK                variable=siTAIF_DReq
    #pragma HLS resource core=AXI4Stream variable=soTAIF_Notif    metadata="-bus_bundle soTAIF_Notif"
    #pragma HLS DATA_PACK                variable=soTAIF_Notif
    #pragma HLS resource core=AXI4Stream variable=soTAIF_Data     metadata="-bus_bundle soTAIF_Data"
    #pragma HLS resource core=AXI4Stream variable=soTAIF_Meta     metadata="-bus_bundle soTAIF_Meta"
     //-- TAIF / ROLE Rx Listen Interface -------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTAIF_LsnReq   metadata="-bus_bundle siTAIF_LsnReq"
    #pragma HLS resource core=AXI4Stream variable=soTAIF_LsnRep   metadata="-bus_bundle soTAIF_LsnRep"
    //-- TAIF / ROLE Tx Data Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTAIF_Data     metadata="-bus_bundle siTAIF_Data"
    #pragma HLS resource core=AXI4Stream variable=siTAIF_Meta     metadata="-bus_bundle siTAIF_Meta"
    #pragma HLS resource core=AXI4Stream variable=soTAIF_DSts     metadata="-bus_bundle soTAIF_DSts"
    #pragma HLS DATA_PACK                variable=soTAIF_DSts
    //-- TAIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS resource core=AXI4Stream variable=siTAIF_OpnReq   metadata="-bus_bundle siTAIF_OpnReq"
    #pragma HLS DATA_PACK                variable=siTAIF_OpnReq
    #pragma HLS resource core=AXI4Stream variable=soTAIF_OpnRep   metadata="-bus_bundle soTAIF_OpnRep"
    #pragma HLS DATA_PACK                variable=soTAIF_OpnRep
    #pragma HLS resource core=AXI4Stream variable=siTAIF_ClsReq   metadata="-bus_bundle siTAIF_ClsReq"
    //-- MEM / Nts0 / RxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_RdCmd metadata="-bus_bundle soMEM_RxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soMEM_RxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_RxP_Data  metadata="-bus_bundle siMEM_RxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_RxP_WrSts metadata="-bus_bundle siMEM_RxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_RxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_WrCmd metadata="-bus_bundle soMEM_RxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soMEM_RxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soMEM_RxP_Data  metadata="-bus_bundle soMEM_RxP_Data"
    //-- MEM / Nts0 / TxP Interface -------------------------------------------
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_RdCmd metadata="-bus_bundle soMEM_TxP_RdCmd"
    #pragma HLS DATA_PACK                variable=soMEM_TxP_RdCmd
    #pragma HLS resource core=AXI4Stream variable=siMEM_TxP_Data  metadata="-bus_bundle siMEM_TxP_Data"
    #pragma HLS resource core=AXI4Stream variable=siMEM_TxP_WrSts metadata="-bus_bundle siMEM_TxP_WrSts"
    #pragma HLS DATA_PACK                variable=siMEM_TxP_WrSts
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_WrCmd metadata="-bus_bundle soMEM_TxP_WrCmd"
    #pragma HLS DATA_PACK                variable=soMEM_TxP_WrCmd
    #pragma HLS resource core=AXI4Stream variable=soMEM_TxP_Data  metadata="-bus_bundle soMEM_TxP_Data"
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS resource core=AXI4Stream variable=siCAM_SssLkpRep metadata="-bus_bundle siCAM_SssLkpRep"
    #pragma HLS DATA_PACK                variable=siCAM_SssLkpRep
    #pragma HLS resource core=AXI4Stream variable=siCAM_SssUpdRep metadata="-bus_bundle siCAM_SssUpdRep"
    #pragma HLS DATA_PACK                variable=siCAM_SssUpdRep
    #pragma HLS resource core=AXI4Stream variable=soCAM_SssLkpReq metadata="-bus_bundle soCAM_SssLkpReq"
    #pragma HLS DATA_PACK                variable=soCAM_SssLkpReq
    #pragma HLS resource core=AXI4Stream variable=soCAM_SssUpdReq metadata="-bus_bundle soCAM_SssUpdReq"
    #pragma HLS DATA_PACK                variable=soCAM_SssUpdReq
    //-- DEBUG / Session Statistics Interfaces
    #pragma HLS INTERFACE ap_none register port=poDBG_SssRelCnt
    #pragma HLS INTERFACE ap_none register port=poDBG_SssRegCnt
    //-- DEBUG / Simulation Counter Interfaces
    #pragma HLS INTERFACE ap_none register port=poSimCycCount

#else

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_vld  register   port=piMMIO_IpAddr   name=piMMIO_IpAddr
    //-- NTS Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poNTS_Ready     name=poNTS_Ready
    //-- IPRX / IP Rx Data Interface ------------------------------------------
    #pragma HLS INTERFACE axis off           port=siIPRX_Data name=siIPRX_Data
    //-- L3MUX / IP Tx Data Interface -----------------------------------------
    #pragma HLS INTERFACE axis off           port=soL3MUX_Data    name=soL3MUX_Data
    //-- TAIF / ROLE Rx Data Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTAIF_DReq     name=siTAIF_DReq
    #pragma HLS DATA_PACK                variable=siTAIF_DReq
    #pragma HLS INTERFACE axis off           port=soTAIF_Notif    name=soTAIF_Notif
    #pragma HLS DATA_PACK                variable=soTAIF_Notif
    #pragma HLS INTERFACE axis off           port=soTAIF_Data     name=soTAIF_Data
    #pragma HLS INTERFACE axis off           port=soTAIF_Meta     name=soTAIF_Meta
    //-- TAIF / ROLE Rx Listen Interface -------------------------------------
    #pragma HLS INTERFACE axis off           port=siTAIF_LsnReq   name=siTAIF_LsnReq
    #pragma HLS INTERFACE axis off           port=soTAIF_LsnRep   name=soTAIF_LsnRep
    //-- TAIF / ROLE Tx Data Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTAIF_Data     name=siTAIF_Data
    #pragma HLS INTERFACE axis off           port=siTAIF_Meta     name=siTAIF_Meta
    #pragma HLS INTERFACE axis off           port=soTAIF_DSts     name=soTAIF_DSts
    #pragma HLS DATA_PACK                variable=soTAIF_DSts
    //-- TAIF / ROLE Tx Ctrl Interfaces ---------------------------------------
    #pragma HLS INTERFACE axis off           port=siTAIF_OpnReq   name=siTAIF_OpnReq
    #pragma HLS DATA_PACK                variable=siTAIF_OpnReq
    #pragma HLS INTERFACE axis off           port=soTAIF_OpnRep   name=soTAIF_OpnRep
    #pragma HLS DATA_PACK                variable=soTAIF_OpnRep
    #pragma HLS INTERFACE axis off           port=siTAIF_ClsReq   name=siTAIF_ClsReq
    //-- MEM / Nts0 / RxP Interface -------------------------------------------
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_RdCmd name=soMEM_RxP_RdCmd
    #pragma HLS DATA_PACK                variable=soMEM_RxP_RdCmd
    #pragma HLS INTERFACE axis off           port=siMEM_RxP_Data  name=siMEM_RxP_Data
    #pragma HLS INTERFACE axis off           port=siMEM_RxP_WrSts name=siMEM_RxP_WrSts
    #pragma HLS DATA_PACK                variable=siMEM_RxP_WrSts
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_WrCmd name=soMEM_RxP_WrCmd
    #pragma HLS DATA_PACK                variable=soMEM_RxP_WrCmd
    #pragma HLS INTERFACE axis off           port=soMEM_RxP_Data  name=soMEM_RxP_Data
    //-- MEM / Nts0 / TxP Interface -------------------------------------------
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_RdCmd name=soMEM_TxP_RdCmd
    #pragma HLS DATA_PACK                variable=soMEM_TxP_RdCmd
    #pragma HLS INTERFACE axis off           port=siMEM_TxP_Data  name=siMEM_TxP_Data
    #pragma HLS INTERFACE axis off           port=siMEM_TxP_WrSts name=siMEM_TxP_WrSts
    #pragma HLS DATA_PACK                variable=siMEM_TxP_WrSts
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_WrCmd name=soMEM_TxP_WrCmd
    #pragma HLS DATA_PACK                variable=soMEM_TxP_WrCmd
    #pragma HLS INTERFACE axis off           port=soMEM_TxP_Data  name=soMEM_TxP_Data
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS INTERFACE axis off           port=siCAM_SssLkpRep name=siCAM_SssLkpRep
    #pragma HLS DATA_PACK                variable=siCAM_SssLkpRep
    #pragma HLS INTERFACE axis off           port=siCAM_SssUpdRep name=siCAM_SssUpdRep
    #pragma HLS DATA_PACK                variable=siCAM_SssUpdRep
    #pragma HLS INTERFACE axis off           port=soCAM_SssLkpReq name=soCAM_SssLkpReq
    #pragma HLS DATA_PACK                variable=soCAM_SssLkpReq
    #pragma HLS INTERFACE axis off           port=soCAM_SssUpdReq name=soCAM_SssUpdReq
    #pragma HLS DATA_PACK                variable=soCAM_SssUpdReq
    //-- DEBUG / Session Statistics Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poDBG_SssRelCnt name=poDBG_SssRelCnt
    #pragma HLS INTERFACE ap_ovld register   port=poDBG_SssRegCnt name=poDBG_SssRegCnt
    //-- DEBUG / Simulation Counter Interfaces
    #pragma HLS INTERFACE ap_ovld register   port=poSimCycCount   name=poSimCycCount

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS AND SIGNALS
    //--   (Sorted by the name of the modules which generate them)
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    //-- ACK Delayer (AKd)
    //-------------------------------------------------------------------------
    static stream<ExtendedEvent>      ssAKdToTXe_Event           ("ssAKdToTXe_Event");
    #pragma HLS stream       variable=ssAKdToTXe_Event           depth=16
    #pragma HLS DATA_PACK    variable=ssAKdToTXe_Event

    static stream<SigBit>             ssAKdToEVe_RxEventSig      ("ssAKdToEVe_RxEventSig");
    #pragma HLS stream       variable=ssAKdToEVe_RxEventSig      depth=2

    static stream<SigBool>            ssAKdToEVe_TxEventSig      ("ssAKdToEVe_TxEventSig");
    #pragma HLS stream       variable=ssAKdToEVe_TxEventSig      depth=2

    //-------------------------------------------------------------------------
    //-- Event Engine (EVe)
    //-------------------------------------------------------------------------
    static stream<ExtendedEvent>      ssEVeToAKd_Event           ("ssEVeToAKd_Event");
    #pragma HLS stream       variable=ssEVeToAKd_Event           depth=4
    #pragma HLS DATA_PACK    variable=ssEVeToAKd_Event

    //-------------------------------------------------------------------------
    //-- Port Table (PRt)
    //-------------------------------------------------------------------------
    StsBool                           sPRtToRdy_Ready;

    static stream<RepBit>             ssPRtToRXe_PortStateRep    ("ssPRtToRXe_PortStateRep");
    #pragma HLS stream       variable=ssPRtToRXe_PortStateRep    depth=4

    static stream<AckBit>             ssPRtToRAi_OpnLsnPortRep   ("ssPRtToRAi_OpnLsnPortRep");
    #pragma HLS stream       variable=ssPRtToRAi_OpnLsnPortRep   depth=4

    static stream<TcpPort>            ssPRtToTAi_GetFreePortRep  ("ssPRtToTAi_GetFreePortRep");
    #pragma HLS stream       variable=ssPRtToTAi_GetFreePortRep  depth=4

    //-- Rx Application Interface (RAi) ---------------------------------------
    static stream<TcpPort>            ssRAiToPRt_OpnLsnPortReq   ("ssRAiToPRt_OpnLsnPortReq");
    #pragma HLS stream       variable=ssRAiToPRt_OpnLsnPortReq   depth=4

    static stream<RAiRxSarQuery>      ssRAiToRSt_RxSarQry        ("ssRAiToRSt_RxSarQry");
    #pragma HLS stream       variable=ssRAiToRSt_RxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRAiToRSt_RxSarQry

    //-------------------------------------------------------------------------
    //-- Rx Engine (RXe)
    //-------------------------------------------------------------------------
    static stream<SessionLookupQuery> ssRXeToSLc_SessLkpReq      ("ssRXeToSLc_SessLkpReq");
    #pragma HLS stream       variable=ssRXeToSLc_SessLkpReq      depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToSLc_SessLkpReq

    static stream<TcpPort>            ssRXeToPRt_PortStateReq    ("ssRXeToPRt_PortStateReq");
    #pragma HLS stream       variable=ssRXeToPRt_PortStateReq    depth=4

    static stream<StateQuery>         ssRXeToSTt_SessStateQry    ("ssRXeToSTt_SessStateQry");
    #pragma HLS stream       variable=ssRXeToSTt_SessStateQry    depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToSTt_SessStateQry

    static stream<RXeRxSarQuery>      ssRXeToRSt_RxSarQry        ("ssRXeToRSt_RxSarQry");
    #pragma HLS stream       variable=ssRXeToRSt_RxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToRSt_RxSarQry

    static stream<RXeTxSarQuery>      ssRXeToTSt_TxSarQry        ("ssRXeToTSt_TxSarQry");
    #pragma HLS stream       variable=ssRXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToTSt_TxSarQry

    static stream<RXeReTransTimerCmd> ssRXeToTIm_ReTxTimerCmd    ("ssRXeToTIm_ReTxTimerCmd");
    #pragma HLS stream       variable=ssRXeToTIm_ReTxTimerCmd    depth=2
    #pragma HLS DATA_PACK    variable=ssRXeToTIm_ReTxTimerCmd

    static stream<SessionId>          ssRXeToTIm_CloseTimer      ("ssRXeToTIm_CloseTimer");
    #pragma HLS stream       variable=ssRXeToTIm_CloseTimer      depth=2

    static stream<SessionId>          ssRXeToTIm_ClrProbeTimer   ("ssRXeToTIm_ClrProbeTimer");
    #pragma HLS stream       variable=ssRXeToTIm_ClrProbeTimer   depth=2

    static stream<TcpAppNotif>        ssRXeToRAi_Notif           ("ssRXeToRAi_Notif");
    #pragma HLS stream       variable=ssRXeToRAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToRAi_Notif

    static stream<SessState>          ssRXeToTAi_SessOpnSts      ("ssRXeToTAi_SessOpnSts");
    #pragma HLS stream       variable=ssRXeToTAi_SessOpnSts      depth=4
    #pragma HLS DATA_PACK    variable=ssRXeToTAi_SessOpnSts

    static stream<ExtendedEvent>      ssRXeToEVe_Event           ("ssRXeToEVe_Event");
    #pragma HLS stream       variable=ssRXeToEVe_Event           depth=512
    #pragma HLS DATA_PACK    variable=ssRXeToEVe_Event

    //-- Rx SAR Table (RSt) ---------------------------------------------------
    static stream<RxSarEntry>         ssRStToRXe_RxSarRep        ("ssRStToRXe_RxSarRep");
    #pragma HLS stream       variable=ssRStToRXe_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToRXe_RxSarRep

    static stream<RAiRxSarReply>      ssRStToRAi_RxSarRep        ("ssRStToRAi_RxSarRep");
    #pragma HLS stream       variable=ssRStToRAi_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToRAi_RxSarRep

    static stream<RxSarEntry>         ssRStToTXe_RxSarRep        ("ssRStToTXe_RxSarRep");
    #pragma HLS stream       variable=ssRStToTXe_RxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssRStToTXe_RxSarRep

    //-------------------------------------------------------------------------
    //-- Session Lookup Controller (SLc)
    //-------------------------------------------------------------------------
    static stream<SessionLookupReply> ssSLcToRXe_SessLkpRep      ("ssSLcToRXe_SessLkpRep");
    #pragma HLS stream       variable=ssSLcToRXe_SessLkpRep      depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToRXe_SessLkpRep

    static stream<SessionLookupReply> ssSLcToTAi_SessLookupRep   ("ssSLcToTAi_SessLookupRep");
    #pragma HLS stream       variable=ssSLcToTAi_SessLookupRep   depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToTAi_SessLookupRep

    static stream<TcpPort>            ssSLcToPRt_ReleasePort     ("ssSLcToPRt_ReleasePort");
    #pragma HLS stream       variable=ssSLcToPRt_ReleasePort     depth=4

    static stream<fourTuple>          ssSLcToTXe_ReverseLkpRep   ("ssSLcToTXe_ReverseLkpRep");
    #pragma HLS stream       variable=ssSLcToTXe_ReverseLkpRep   depth=4
    #pragma HLS DATA_PACK    variable=ssSLcToTXe_ReverseLkpRep

    //-------------------------------------------------------------------------
    //-- State Table (STt)
    //-------------------------------------------------------------------------
    static stream<TcpState>           ssSTtToRXe_SessStateRep    ("ssSTtToRXe_SessStateRep");
    #pragma HLS stream       variable=ssSTtToRXe_SessStateRep    depth=2

    static stream<TcpState>           ssSTtToTAi_AcceptStateRep  ("ssSTtToTAi_AcceptStateRep");
    #pragma HLS stream       variable=ssSTtToTAi_AcceptStateRep  depth=2

    static stream<TcpState>           ssSTtToTAi_SessStateRep    ("ssSTtToTAi_SessStateRep");
    #pragma HLS stream       variable=ssSTtToTAi_SessStateRep    depth=2

    static stream<SessionId>          ssSTtToSLc_SessReleaseCmd  ("ssSTtToSLc_SessReleaseCmd");
    #pragma HLS stream       variable=ssSTtToSLc_SessReleaseCmd  depth=2

    //-------------------------------------------------------------------------
    //-- Tx Application Interface (TAi)
    //-------------------------------------------------------------------------
    static stream<ReqBit>             ssTAiToPRt_GetFeePortReq   ("ssTAiToPRt_GetFeePortReq");
    #pragma HLS stream       variable=ssTAiToPRt_GetFeePortReq   depth=4

    static stream<SocketPair>         ssTAiToSLc_SessLookupReq   ("ssTAiToSLc_SessLookupReq");
    #pragma HLS DATA_PACK    variable=ssTAiToSLc_SessLookupReq
    #pragma HLS stream       variable=ssTAiToSLc_SessLookupReq   depth=4

    static stream<Event>              ssTAiToEVe_Event           ("ssTAiToEVe_Event");
    #pragma HLS stream       variable=ssTAiToEVe_Event           depth=4
    #pragma HLS DATA_PACK    variable=ssTAiToEVe_Event

    static stream<TAiTxSarPush>       ssTAiToTSt_PushCmd         ("ssTAiToTSt_PushCmd");
    #pragma HLS stream       variable=ssTAiToTSt_PushCmd         depth=2
    #pragma HLS DATA_PACK    variable=ssTAiToTSt_PushCmd

    static stream<StateQuery>         ssTAiToSTt_AcceptStateQry  ("ssTAiToSTt_AcceptStateQry");
    #pragma HLS stream       variable=ssTAiToSTt_AcceptStateQry  depth=2
    #pragma HLS DATA_PACK    variable=ssTAiToSTt_AcceptStateQry

    static stream<TcpSessId>          ssTAiToSTt_SessStateReq    ("ssTAiToSTt_SessStateReq");
    #pragma HLS stream       variable=ssTAiToSTt_SessStateReq    depth=2

    //-------------------------------------------------------------------------
    //-- Timers (TIm)
    //-------------------------------------------------------------------------
    static stream<Event>              ssTImToEVe_Event           ("ssTImToEVe_Event");
    #pragma HLS stream       variable=ssTImToEVe_Event           depth=4 //TODO maybe reduce to 2, there should be no evil cycle
    #pragma HLS DATA_PACK    variable=ssTImToEVe_Event

    static stream<SessionId>          ssTImToSTt_SessCloseCmd    ("ssTImToSTt_SessCloseCmd");
    #pragma HLS stream       variable=ssTImToSTt_SessCloseCmd    depth=2

    static stream<SessState>          ssTImToTAi_Notif           ("ssTImToTAi_Notif");
    #pragma HLS stream       variable=ssTImToTAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssTImToTAi_Notif

    static stream<TcpAppNotif>        ssTImToRAi_Notif           ("ssTImToRAi_Notif");
    #pragma HLS stream       variable=ssTImToRAi_Notif           depth=4
    #pragma HLS DATA_PACK    variable=ssTImToRAi_Notif

    //-------------------------------------------------------------------------
    //-- Tx Engine (TXe)
    //-------------------------------------------------------------------------
    static stream<SigBit>             ssTXeToEVe_RxEventSig      ("ssTXeToEVe_RxEventSig");
    #pragma HLS stream       variable=ssTXeToEVe_RxEventSig      depth=2

    static stream<SessionId>          ssTXeToRSt_RxSarReq        ("ssTXeToRSt_RxSarReq");
    #pragma HLS stream       variable=ssTXeToRSt_RxSarReq        depth=2

    static stream<TXeTxSarQuery>      ssTXeToTSt_TxSarQry        ("ssTXeToTSt_TxSarQry");
    #pragma HLS stream       variable=ssTXeToTSt_TxSarQry        depth=2
    #pragma HLS DATA_PACK    variable=ssTXeToTSt_TxSarQry

    static stream<SessionId>          ssTXeToSLc_ReverseLkpReq   ("ssTXeToSLc_ReverseLkpReq");
    #pragma HLS stream       variable=ssTXeToSLc_ReverseLkpReq   depth=4

    static stream<TXeReTransTimerCmd> ssTXeToTIm_SetReTxTimer    ("ssTXeToTIm_SetReTxTimer");
    #pragma HLS stream       variable=ssTXeToTIm_SetReTxTimer    depth=2
    #pragma HLS DATA_PACK    variable=ssTXeToTIm_SetReTxTimer

    static stream<SessionId>          ssTXeToTIm_SetProbeTimer   ("ssTXeToTIm_SetProbeTimer");
    #pragma HLS stream       variable=ssTXeToTIm_SetProbeTimer   depth=2

    //-------------------------------------------------------------------------
    //-- Tx SAR Table (TSt)
    //-------------------------------------------------------------------------
    static stream<RXeTxSarReply>      ssTStToRXe_TxSarRep        ("ssTStToRXe_TxSarRep");
    #pragma HLS stream       variable=ssTStToRXe_TxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssTStToRXe_TxSarRep

    static stream<TXeTxSarReply>      ssTStToTXe_TxSarRep        ("ssTStToTXe_TxSarRep");
    #pragma HLS stream       variable=ssTStToTXe_TxSarRep        depth=2
    #pragma HLS DATA_PACK    variable=ssTStToTXe_TxSarRep

    static stream<TStTxSarPush>       ssTStToTAi_PushCmd         ("ssTStToTAi_PushCmd");
    #pragma HLS stream       variable=ssTStToTAi_PushCmd         depth=2
    #pragma HLS DATA_PACK    variable=ssTStToTAi_PushCmd

    /**********************************************************************
     * PROCESSES: TCP STATE-KEEPING DATA STRUCTURES
     **********************************************************************/

    //-- Session Lookup Controller (SLc) -----------------------------------
    session_lookup_controller(
            ssRXeToSLc_SessLkpReq,
            ssSLcToRXe_SessLkpRep,
            ssSTtToSLc_SessReleaseCmd,
            ssSLcToPRt_ReleasePort,
            ssTAiToSLc_SessLookupReq,
            ssSLcToTAi_SessLookupRep,
            ssTXeToSLc_ReverseLkpReq,
            ssSLcToTXe_ReverseLkpRep,
            soCAM_SssLkpReq,
            siCAM_SssLkpRep,
            soCAM_SssUpdReq,
            siCAM_SssUpdRep,
            poDBG_SssRelCnt,
            poDBG_SssRegCnt);

    //-- State Table (STt) -------------------------------------------------
    state_table(
            ssRXeToSTt_SessStateQry,
            ssSTtToRXe_SessStateRep,
            ssTAiToSTt_AcceptStateQry,
            ssSTtToTAi_AcceptStateRep,
            ssTAiToSTt_SessStateReq,
            ssSTtToTAi_SessStateRep,
            ssTImToSTt_SessCloseCmd,
            ssSTtToSLc_SessReleaseCmd);

    //-- RX SAR Table (RSt) ------------------------------------------------
    rx_sar_table(
            ssRXeToRSt_RxSarQry,
            ssRStToRXe_RxSarRep,
            ssRAiToRSt_RxSarQry,
            ssRStToRAi_RxSarRep,
            ssTXeToRSt_RxSarReq,
            ssRStToTXe_RxSarRep);

    //-- TX SAR Table (TSt) ------------------------------------------------
    tx_sar_table(
            ssRXeToTSt_TxSarQry,
            ssTStToRXe_TxSarRep,
            ssTXeToTSt_TxSarQry,
            ssTStToTXe_TxSarRep,
            ssTAiToTSt_PushCmd,
            ssTStToTAi_PushCmd);

    //-- Port Table (PRt) --------------------------------------------------
    port_table(
            sPRtToRdy_Ready,
            ssRXeToPRt_PortStateReq,
            ssPRtToRXe_PortStateRep,
            ssRAiToPRt_OpnLsnPortReq,
            ssPRtToRAi_OpnLsnPortRep,
            ssTAiToPRt_GetFeePortReq,
            ssPRtToTAi_GetFreePortRep,
            ssSLcToPRt_ReleasePort);

    //-- Timers (TIm) ------------------------------------------------------
    timers(
            ssRXeToTIm_ReTxTimerCmd,
            ssRXeToTIm_ClrProbeTimer,
            ssRXeToTIm_CloseTimer,
            ssTXeToTIm_SetReTxTimer,
            ssTXeToTIm_SetProbeTimer,
            ssTImToSTt_SessCloseCmd,
            ssTImToEVe_Event,
            ssTImToTAi_Notif,
            ssTImToRAi_Notif);

    //-- Event Engine (EVe) ------------------------------------------------
    event_engine(
            ssTAiToEVe_Event,
            ssRXeToEVe_Event,
            ssTImToEVe_Event,
            ssEVeToAKd_Event,
            ssAKdToEVe_RxEventSig,
            ssAKdToEVe_TxEventSig,
            ssTXeToEVe_RxEventSig);

    //-- Ack Delayer (AKd)) ----------------------------------------------
     ack_delay(
            ssEVeToAKd_Event,
            ssAKdToEVe_RxEventSig,
            ssAKdToEVe_TxEventSig,
            ssAKdToTXe_Event);


    /**********************************************************************
     * PROCESSES: RX & TX ENGINES
     **********************************************************************/

    //-- RX Engine (RXe) --------------------------------------------------
    rx_engine(
            siIPRX_Data,
            ssRXeToSLc_SessLkpReq,
            ssSLcToRXe_SessLkpRep,
            ssRXeToSTt_SessStateQry,
            ssSTtToRXe_SessStateRep,
            ssRXeToPRt_PortStateReq,
            ssPRtToRXe_PortStateRep,
            ssRXeToRSt_RxSarQry,
            ssRStToRXe_RxSarRep,
            ssRXeToTSt_TxSarQry,
            ssTStToRXe_TxSarRep,
            ssRXeToTIm_ReTxTimerCmd,
            ssRXeToTIm_ClrProbeTimer,
            ssRXeToTIm_CloseTimer,
            ssRXeToEVe_Event,
            ssRXeToTAi_SessOpnSts,
            ssRXeToRAi_Notif,
            soMEM_RxP_WrCmd,
            soMEM_RxP_Data,
            siMEM_RxP_WrSts);

    //-- TX Engine (TXe) --------------------------------------------------
    tx_engine(
            ssAKdToTXe_Event,
            ssTXeToEVe_RxEventSig,
            ssTXeToRSt_RxSarReq,
            ssRStToTXe_RxSarRep,
            ssTXeToTSt_TxSarQry,
            ssTStToTXe_TxSarRep,
            soMEM_TxP_RdCmd,
            siMEM_TxP_Data,
            ssTXeToTIm_SetReTxTimer,
            ssTXeToTIm_SetProbeTimer,
            ssTXeToSLc_ReverseLkpReq,
            ssSLcToTXe_ReverseLkpRep,
            soL3MUX_Data);


    /**********************************************************************
     * PROCESSSES: APPLICATION INTERFACES
     **********************************************************************/

    //-- Rx Application Interface (RAi) -----------------------------------
     rx_app_interface(
             soTAIF_Notif,
             siTAIF_DReq,
             soTAIF_Data,
             soTAIF_Meta,
             siTAIF_LsnReq,
             soTAIF_LsnRep,
             ssRAiToPRt_OpnLsnPortReq,
             ssPRtToRAi_OpnLsnPortRep,
             ssRXeToRAi_Notif,
             ssTImToRAi_Notif,
             ssRAiToRSt_RxSarQry,
             ssRStToRAi_RxSarRep,
             soMEM_RxP_RdCmd,
             siMEM_RxP_Data);

    //-- Tx Application Interface (TAi) ------------------------------------
    tx_app_interface(
            siTAIF_OpnReq,
            soTAIF_OpnRep,
            siTAIF_ClsReq,
            siTAIF_Data,
            siTAIF_Meta,
            soTAIF_DSts,
            soMEM_TxP_WrCmd,
            soMEM_TxP_Data,
            siMEM_TxP_WrSts,
            ssTAiToSTt_SessStateReq,
            ssSTtToTAi_SessStateRep,
            ssTAiToSTt_AcceptStateQry,   // [FIXME- ReEname ConnectStateQry]
            ssSTtToTAi_AcceptStateRep,
            ssTAiToSLc_SessLookupReq,
            ssSLcToTAi_SessLookupRep,
            ssTAiToPRt_GetFeePortReq,
            ssPRtToTAi_GetFreePortRep,
            ssTStToTAi_PushCmd,
            ssTAiToTSt_PushCmd,
            ssRXeToTAi_SessOpnSts,
            ssTAiToEVe_Event,
            ssTImToTAi_Notif,
            piMMIO_IpAddr);

    /**********************************************************************
     * PROCESSES: CONTROL AND DEBUG INTERFACES
     **********************************************************************/

    //-- Ready signal generator -------------------------------------------
    pReady(
            sPRtToRdy_Ready,
            poNTS_Ready);

    #if TOE_FEATURE_USED_FOR_DEBUGGING
    //-- Testbench counter incrementer (for debugging) --------------------
    pTbSimCount(
        poSimCycCount);
    #endif

}

/*! \} */
