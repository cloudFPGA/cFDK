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

/*****************************************************************************
 * @file       : nal.hpp
 * @brief      : The cloudFPGA Network Abstraction Layer (NAL) between NTS and ROlE.
 *               The NAL core manages the NTS Stack and maps the network streams to 
 *               the user's ROLE or the FMC while hiding some complexity of UOE and 
 *               TOE.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/


#ifndef _NAL_H_
#define _NAL_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>


// CTRL LINK DEFINITONS
// =====================

//#define NAL_AXI_CTRL_REGISTER 0

//// MRT size 1024
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff
//#define NAL_CTRL_LINK_SIZE (XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH/4)
//#define NAL_CTRL_LINK_CONFIG_START_ADDR (0x2000/4)
//#define NAL_CTRL_LINK_CONFIG_END_ADDR (0x203F/4)
//#define NAL_CTRL_LINK_STATUS_START_ADDR (0x2040/4)
//#define NAL_CTRL_LINK_STATUS_END_ADDR (0x207F/4)
//#define NAL_CTRL_LINK_MRT_START_ADDR (0x2080/4)
//#define NAL_CTRL_LINK_MRT_END_ADDR (0x307F/4)


//// MRT size 128
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x400
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x7ff
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_WIDTH_CTRLLINK_V     32
//#define XNAL_PIFMC_NAL_CTRLLINK_AXI_DEPTH_CTRLLINK_V     160
//#define NAL_CTRL_LINK_SIZE (XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH/4)
//#define NAL_CTRL_LINK_CONFIG_START_ADDR (0x400/4)
//#define NAL_CTRL_LINK_CONFIG_END_ADDR (0x43F/4)
//#define NAL_CTRL_LINK_STATUS_START_ADDR (0x440/4)
//#define NAL_CTRL_LINK_STATUS_END_ADDR (0x47F/4)
//#define NAL_CTRL_LINK_MRT_START_ADDR (0x480/4)
//#define NAL_CTRL_LINK_MRT_END_ADDR (0x67F/4)

// MRT size 64
#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x200
#define XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3ff
#define XNAL_PIFMC_NAL_CTRLLINK_AXI_WIDTH_CTRLLINK_V     32
#define XNAL_PIFMC_NAL_CTRLLINK_AXI_DEPTH_CTRLLINK_V     96
#define NAL_CTRL_LINK_SIZE (XNAL_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH/4)
#define NAL_CTRL_LINK_CONFIG_START_ADDR (0x200/4)
#define NAL_CTRL_LINK_CONFIG_END_ADDR (0x23F/4)
#define NAL_CTRL_LINK_STATUS_START_ADDR (0x240/4)
#define NAL_CTRL_LINK_STATUS_END_ADDR (0x27F/4)
#define NAL_CTRL_LINK_MRT_START_ADDR (0x280/4)
#define NAL_CTRL_LINK_MRT_END_ADDR (0x37F/4)

// (CtrlLink definitions end)


#include "../../FMC/src/fmc.hpp"

#include "../../../../../hls/network.hpp"
#include "../../network_utils.hpp"
//#include "../../memory_utils.hpp"
#include "../../simulation_utils.hpp"


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
extern bool gTraceEvent;
#endif

#define THIS_NAME "NAL"

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CON 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};


/************************************************
 * INTERFACE SYNTHESIS DIRECTIVES
 *  For the time being, we continue designing
 *  with the DEPRECATED directives because the
 *  new PRAGMAs do not work for us.
 ************************************************/
//#define USE_DEPRECATED_DIRECTIVES

using namespace hls;

#define DEFAULT_TX_PORT 2718
#define DEFAULT_RX_PORT 2718

//#define FSM_RESET 0
//#define FSM_IDLE  1
//#define FSM_W8FORPORT 2
//#define FSM_FIRST_ACC 3
//#define FSM_ACC 4
//#define FSM_LAST_ACC 5
//#define FSM_W8FORMETA 6
//#define FSM_WRITE_META 7
//#define FSM_DROP_PACKET 8
//#define FsmStateUdp uint8_t
enum FsmStateUdp {FSM_RESET = 0, FSM_W8FORMETA, FSM_W8FORREQS, FSM_FIRST_ACC, FSM_ACC, \
  FSM_WRITE_META, FSM_DROP_PACKET};

//#define OpnFsmStates uint8_t
//#define OPN_IDLE 0
//#define OPN_REQ 1
//#define OPN_REP 2
//#define OPN_DONE 3
enum OpnFsmStates {OPN_IDLE = 0, OPN_REQ, OPN_REP, OPN_DONE};

//#define LsnFsmStates uint8_t
//#define LSN_IDLE 0
//#define LSN_SEND_REQ 1
//#define LSN_WAIT_ACK 2
//#define LSN_DONE 3
enum LsnFsmStates {LSN_IDLE = 0, LSN_SEND_REQ, LSN_WAIT_ACK, LSN_DONE};

//#define RrhFsmStates uint8_t
//#define RRH_WAIT_NOTIF 0
//#define RRH_SEND_DREQ 1
enum RrhFsmStates {RRH_RESET = 0, RRH_WAIT_NOTIF, RRH_PROCESS_NOTIF, RRH_START_REQUEST, RRH_PROCESS_REQUEST, RRH_WAIT_WRITE_ACK};

//#define RdpFsmStates uint8_t
//#define RDP_WAIT_META 0
//#define RDP_STREAM_ROLE 1
//#define RDP_STREAM_FMC 2
//#define RDP_WRITE_META_ROLE 3
//#define RDP_WRITE_META_FMC 4
//#define RDP_DROP_PACKET 5
enum RdpFsmStates {RDP_WAIT_META = 0, RDP_W8FORREQS_1, RDP_W8FORREQS_2, RDP_FILTER_META, RDP_STREAM_ROLE, RDP_STREAM_FMC, \
  RDP_WRITE_META_ROLE, RDP_WRITE_META_FMC, RDP_DROP_PACKET};

//#define WrpFsmStates uint8_t
//#define WRP_WAIT_META 0
//#define WRP_STREAM_FMC 1
//#define WRP_WAIT_CONNECTION 2
//#define WRP_STREAM_ROLE 3
//#define WRP_DROP_PACKET 4
enum WrpFsmStates {WRP_WAIT_META = 0, WRP_STREAM_FMC, WRP_W8FORREQS_1,  WRP_W8FORREQS_11, WRP_W8FORREQS_2, WRP_W8FORREQS_22, WRP_WAIT_CONNECTION, \
  WRP_STREAM_ROLE, WRP_DROP_PACKET};

enum WbuFsmStates {WBU_WAIT_META = 0, WBU_SND_REQ, WBU_WAIT_REP, WBU_STREAM, WBU_DROP, WBU_DRAIN};

enum FiveStateFsm {FSM_STATE_0 = 0, FSM_STATE_1, FSM_STATE_2, FSM_STATE_3, FSM_STATE_4};

enum CacheInvalFsmStates {CACHE_WAIT_FOR_VALID = 0, CACHE_VALID, CACHE_INV_SEND_0, CACHE_INV_SEND_1, CACHE_INV_SEND_2, CACHE_INV_SEND_3};

//#define ClsFsmStates uint8_t
//#define CLS_IDLE 0
//#define CLS_NEXT 1
//#define CLS_WAIT4RESP 2
enum ClsFsmStates {CLS_IDLE = 0, CLS_NEXT, CLS_WAIT4RESP};

enum DeqFsmStates {DEQ_WAIT_META = 0, DEQ_STREAM_DATA, DEQ_SEND_NOTIF};

enum TableFsmStates {TAB_FSM_READ = 0, TAB_FSM_WRITE};

enum AxiLiteFsmStates {A4L_RESET = 0, A4L_STATUS_UPDATE, A4L_COPY_CONFIG, A4L_COPY_CONFIG_2, \
  //A4L_BROADCAST_CONFIG_1, A4L_BROADCAST_CONFIG_2,
  A4L_COPY_MRT, A4L_COPY_STATUS, A4L_COPY_FINISH, A4L_WAIT_FOR_SUB_FSMS};

enum ConfigBcastStates {CB_WAIT = 0, CB_START, CB_1, CB_2, CB_3_0, CB_3_1, CB_3_2};


enum PortFsmStates {PORT_RESET = 0, PORT_IDLE, PORT_L4_RESET, PORT_NEW_UDP_REQ, PORT_NEW_UDP_REP, \
                PORT_NEW_FMC_REQ, PORT_NEW_FMC_REP, PORT_NEW_TCP_REQ, PORT_NEW_TCP_REP, PORT_L7_RESET, \
                PORT_WAIT_PR, PORT_START_TCP_CLS_0, PORT_START_TCP_CLS_1, PORT_START_TCP_CLS_2, PORT_START_UDP_CLS, PORT_SEND_UPDATE};

enum PortType {FMC = 0, UDP, TCP};

enum NalCntIncType {NID_MISS_RX = 0, NID_MISS_TX, PCOR_TX, TCP_CON_FAIL, LAST_RX_PORT, \
  LAST_RX_NID, LAST_TX_PORT, LAST_TX_NID, PACKET_RX, PACKET_TX, UNAUTH_ACCESS, \
    AUTH_ACCESS, FMC_TCP_BYTES};


#define MAX_NAL_SESSIONS (TOE_MAX_SESSIONS)
#define NAL_STREAMING_SPLIT_TCP (ZYC2_MSS)
//#define NAL_STREAMING_SPLIT_TCP (ZYC2_MSS - 8)

//#define MAX_MRT_SIZE 1024
//#define MAX_MRT_SIZE 128
#define MAX_MRT_SIZE 64
#define NAL_MAX_FIFO_DEPTHS_BYTES 2000 //does apply for ROLE and FMC FIFOs
#define NUMBER_CONFIG_WORDS 16
#define NUMBER_STATUS_WORDS 16
#define NAL_NUMBER_CONFIG_WORDS NUMBER_CONFIG_WORDS
#define NAL_NUMBER_STATUS_WORDS NUMBER_STATUS_WORDS
//#define NAL_READ_TIMEOUT 160000000 //is a little more than one second with 156Mhz
#define NAL_CONNECTION_TIMEOUT 160000000 //is a little more than one second with 156Mhz
#define NAL_TCP_RX_DATA_DELAY_CYCLES  80  //based on chipscope

#define NAL_MMIO_STABILIZE_TIME 150 //based on chipscope...

#define UNUSED_TABLE_ENTRY_VALUE 0x111000
#define UNUSED_SESSION_ENTRY_VALUE 0xFFFE
#define INVALID_MRT_VALUE 0xFFFFF

/*
 * ctrlLINK Structure:
 * 1.         0 --            NUMBER_CONFIG_WORDS -1 :  possible configuration from FMC to NAL
 * 2. NUMBER_CONFIG_WORDS --  NUMBER_STATUS_WORDS -1 :  possible status from NAL to FMC
 * 3. NUMBER_STATUS_WORDS 
 *     + NUMBER_STATUS_WORDS --  MAX_MRT_SIZE +
 *                              NUMBER_CONFIG_WORDS +
 *                              NUMBER_STATUS_WORDS    : Message Routing Table (MRT)
 *
 */
#define NAL_CONFIG_OWN_RANK 0
#define NAL_CONFIG_MRT_VERSION 1
#define NAL_CONFIG_SAVED_UDP_PORTS 2
#define NAL_CONFIG_SAVED_TCP_PORTS 3
#define NAL_CONFIG_SAVED_FMC_PORTS 4

#define NAL_STATUS_MRT_VERSION 0
#define NAL_STATUS_OPEN_UDP_PORTS 1
#define NAL_STATUS_OPEN_TCP_PORTS 2
#define NAL_STATUS_FMC_PORT_PROCESSED 3
#define NAL_STATUS_OWN_RANK 13

//#define NAL_STATUS_WRITE_ERROR_CNT 4
//#define NAL_STATUS_UNUSED_1 4
#define NAL_UNAUTHORIZED_ACCESS 4
//#define NAL_STATUS_READ_ERROR_CNT 5
#define NAL_AUTHORIZED_ACCESS 5
//#define NAL_STATUS_UNUSED_2 5
#define NAL_STATUS_SEND_STATE 6
#define NAL_STATUS_RECEIVE_STATE 7
#define NAL_STATUS_GLOBAL_STATE 8
#define NAL_STATUS_LAST_RX_NODE_ID 9
#define NAL_STATUS_RX_NODEID_ERROR 10
#define NAL_STATUS_LAST_TX_NODE_ID 11
#define NAL_STATUS_TX_NODEID_ERROR 12
//#define NAL_STATUS_TX_PORT_CORRECTIONS 13
#define NAL_STATUS_PACKET_CNT_RX 14
#define NAL_STATUS_PACKET_CNT_TX 15


// New UOE types
// here until merged into one HLS hpp [TODO]

typedef bool StsBool;  // Status     : Noun or verb indicating a status (.e.g isOpen). Does not  have to go back to source of stimulus.
typedef ap_uint<16> UdpSrcPort;     // UDP Source Port
typedef ap_uint<16> UdpDstPort;     // UDP Destination Port
typedef ap_uint<16> UdpPort;        // UDP source or destination Port
typedef ap_uint<16> UdpLen;         // UDP header and data Length
typedef SocketPair   UdpAppMeta;
//typedef Axis<64>     UdpAppData;
//typedef NetworkWord  UdpAppData;
//typedef UdpMeta      UdpAppMeta;
typedef UdpLen       UdpAppDLen;

typedef UdpAppMeta  UdpMeta;
typedef UdpAppDLen  UdpPLen;

typedef ap_uint<16> PacketLen;



struct NalEventNotif {
  NalCntIncType type;
  ap_uint<32>   update_value;
  //in case of LAST_* types, the update_value is the new value
  //on other cases, it is an increment value
  NalEventNotif() {}
  NalEventNotif(NalCntIncType nt, ap_uint<32> uv): type(nt), update_value(uv) {}
};
//typedef NalEventNotif NalEventNotifType;

typedef ap_uint<64> NalTriple;

struct NalNewTableEntry {
  NalTriple  new_triple;
  SessionId sessId;
  NalNewTableEntry() {}
  NalNewTableEntry(NalTriple nt, SessionId sid): new_triple(nt), sessId(sid) {}
};

struct NalNewTcpConRep {
  NalTriple new_triple;
  SessionId newSessionId;
  bool    failure;
  NalNewTcpConRep() {}
  NalNewTcpConRep(NalTriple nt, SessionId ns, bool fail): new_triple(nt), newSessionId(ns), failure(fail) {}
};

struct NalConfigUpdate {
  ap_uint<16>   config_addr;
  ap_uint<32>   update_value;
  NalConfigUpdate() {}
  NalConfigUpdate(ap_uint<16> ca, ap_uint<32> uv): config_addr(ca), update_value(uv) {}
};

struct NalMrtUpdate {
  NodeId    nid;
  Ip4Addr   ip4a;
  NalMrtUpdate() {}
  NalMrtUpdate(NodeId node, Ip4Addr addr): nid(node), ip4a(addr) {}
};

struct NalStatusUpdate {
  ap_uint<16>   status_addr;
  ap_uint<32>   new_value;
  NalStatusUpdate() {}
  NalStatusUpdate(ap_uint<16> sa, ap_uint<32> nv): status_addr(sa), new_value(nv) {}
};

struct NalPortUpdate {
  PortType    port_type;
  ap_uint<32> new_value;
  NalPortUpdate () {}
  NalPortUpdate(PortType pt, ap_uint<32> nv): port_type(pt), new_value(nv) {}
};


//INLINE METHODS
ap_uint<32> getRightmostBitPos(ap_uint<32> num);
NalTriple newTriple(Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort);
Ip4Addr getRemoteIpAddrFromTriple(NalTriple triple);
TcpPort getRemotePortFromTriple(NalTriple triple);
TcpPort getLocalPortFromTriple(NalTriple triple);
uint8_t extractByteCnt(Axis<64> currWord);
uint8_t extractByteCntNW(NetworkWord currWord);


#include "uss.hpp"
#include "tss.hpp"
#include "hss.hpp"
#include "cam8.hpp"
//#include "cam16.hpp"

void nal_main(
    // ----- link to FMC -----
    ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    //state of the FPGA
    ap_uint<1> *layer_4_enabled,
    ap_uint<1> *layer_7_enabled,
    ap_uint<1> *role_decoupled,
    // ready signal from NTS
    ap_uint<1>  *piNTS_ready,
    // ----- link to MMIO ----
    ap_uint<16> *piMMIO_FmcLsnPort,
    ap_uint<32> *piMMIO_CfrmIp4Addr,
    // -- my IP address 
    ap_uint<32>                 *myIpAddress,

    //-- ROLE UDP connection
    ap_uint<32>                 *pi_udp_rx_ports,
    stream<NetworkWord>         &siUdp_data,
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<NetworkMetaStream>   &soUdp_meta,

    // -- ROLE TCP connection
    ap_uint<32>                 *pi_tcp_rx_ports,
    stream<NetworkWord>         &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,

    // -- FMC TCP connection
    stream<TcpAppData>          &siFMC_data,
    stream<TcpAppMeta>          &siFMC_SessId,
    //ap_uint<1>                  *piFMC_data_FIFO_prog_full,
    stream<TcpAppData>          &soFMC_data,
    //ap_uint<1>                  *piFMC_sessid_FIFO_prog_full,
    stream<TcpAppMeta>          &soFMC_SessId,

    //-- UOE / Control Port Interfaces
    stream<UdpPort>             &soUOE_LsnReq,
    stream<StsBool>             &siUOE_LsnRep,
    stream<UdpPort>             &soUOE_ClsReq,
    stream<StsBool>             &siUOE_ClsRep,

    //-- UOE / Rx Data Interfaces
    stream<UdpAppData>          &siUOE_Data,
    stream<UdpAppMeta>          &siUOE_Meta,

    //-- UOE / Tx Data Interfaces
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen,

    //-- TOE / Rx Data Interfaces
    stream<TcpAppNotif>    &siTOE_Notif,
    stream<TcpAppRdReq>    &soTOE_DReq,
    stream<TcpAppData>     &siTOE_Data,
    stream<TcpAppMeta>     &siTOE_SessId,
    //-- TOE / Listen Interfaces
    stream<TcpAppLsnReq>   &soTOE_LsnReq,
    stream<TcpAppLsnRep>   &siTOE_LsnRep,
    //-- TOE / Tx Data Interfaces
    stream<TcpAppData>      &soTOE_Data,
    stream<TcpAppSndReq>    &soTOE_SndReq,
    stream<TcpAppSndRep>    &siTOE_SndRep,
    //stream<AppWrSts>    &siTOE_DSts,
    //-- TOE / Open Interfaces
    stream<TcpAppOpnReq>      &soTOE_OpnReq,
    stream<TcpAppOpnRep>   &siTOE_OpnRep,
    //-- TOE / Close Interfaces
    stream<TcpAppClsReq>   &soTOE_ClsReq
    );

#endif


    /*! \} */

