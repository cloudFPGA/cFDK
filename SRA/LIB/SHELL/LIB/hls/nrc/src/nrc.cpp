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

/*!
 * @file nrc.cpp
 * @brief 
 * \ingroup NRC
 * \addtogroup NRC
 * \{
 */

#include "nrc.hpp"

uint16_t tableCopyVariable = 0;

ap_uint<8>   openPortWaitTime = 100;
ap_uint<8>   udp_lsn_watchDogTimer = 100;
#ifndef __SYNTHESIS__
ap_uint<16>  mmio_stabilize_counter = 1;
#else
ap_uint<16>  mmio_stabilize_counter = NRC_MMIO_STABILIZE_TIME;
#endif
bool Udp_RX_metaWritten = false;

FsmStateUdp fsmStateRX_Udp = FSM_RESET;
FsmStateUdp fsmStateTX_Udp = FSM_RESET;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];


ap_uint<32> mrt_version_processed = 0;

ap_uint<32> udp_rx_ports_processed = 0;
ap_uint<32> udp_rx_ports_to_close = 0;
bool need_udp_port_req = false;
ap_uint<16> new_relative_port_to_req_udp = 0;
ClsFsmStates clsFsmState_Udp = CLS_IDLE;
ap_uint<16> newRelativePortToClose = 0;
ap_uint<16> newAbsolutePortToClose = 0;

NetworkMetaStream out_meta_udp = NetworkMetaStream(); //DON'T FORGET to initilize!
NetworkMetaStream in_meta_udp = NetworkMetaStream(); //ATTENTION: don't forget initilizer...

ap_uint<32> node_id_missmatch_RX_cnt = 0;
NodeId last_rx_node_id = 0;
NrcPort last_rx_port = 0;
ap_uint<32> node_id_missmatch_TX_cnt = 0;
NodeId last_tx_node_id = 0;
NrcPort last_tx_port = 0;
ap_uint<16> port_corrections_TX_cnt = 0;
ap_uint<32> unauthorized_access_cnt = 0;
ap_uint<32> authorized_access_cnt = 0;

ap_uint<32> packet_count_RX = 0;
ap_uint<32> packet_count_TX = 0;

UdpAppDLen udpTX_packet_length = 0;
UdpAppDLen udpTX_current_packet_length = 0;


ap_uint<64> tripleList[MAX_NRC_SESSIONS];
ap_uint<17> sessionIdList[MAX_NRC_SESSIONS];
ap_uint<1>  usedRows[MAX_NRC_SESSIONS];
ap_uint<1>  rowsToDelete[MAX_NRC_SESSIONS];
bool tables_initalized = false;
#define UNUSED_TABLE_ENTRY_VALUE 0x111000
#define UNUSED_SESSION_ENTRY_VALUE 0xFFFE


//FROM TCP

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
extern bool gTraceEvent;
#endif

#define THIS_NAME "NRC"

#define TRACE_OFF  0x0000
#define TRACE_RDP 1 <<  1
#define TRACE_WRP 1 <<  2
#define TRACE_SAM 1 <<  3
#define TRACE_LSN 1 <<  4
#define TRACE_CON 1 <<  5
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

enum DropCmd {KEEP_CMD=false, DROP_CMD};

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  TCP Role Interface, unless the user specifies new ones
//--  via TBD.
//--  FYI --> 8803 is the ZIP code of Ruschlikon ;-)
//---------------------------------------------------------
//#define DEFAULT_FPGA_LSN_PORT   0x2263      // TOE    listens on port = 8803 (static  ports must be     0..32767)
//#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
//#define DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571


AppOpnReq     leHostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER
AppOpnSts     newConn;
ap_uint<32>  watchDogTimer_pcon = 0;
ap_uint<8>   watchDogTimer_plisten = 0;

// Set a startup delay long enough to account for the initialization
// of TOE's listen port table which takes 32,768 cycles after reset.
//  [FIXME - StartupDelay must be replaced by a piSHELL_Reday signal]
#ifdef __SYNTHESIS_
ap_uint<16>         startupDelay = 0x8000;
#else 
ap_uint<16>         startupDelay = 30;
#endif
OpnFsmStates opnFsmState = OPN_IDLE;
ClsFsmStates clsFsmState_Tcp = CLS_IDLE;

LsnFsmStates lsnFsmState = LSN_IDLE;

RrhFsmStates rrhFsmState = RRH_WAIT_NOTIF;
AppNotif notif_pRrh;

RdpFsmStates rdpFsmState = RDP_WAIT_META;

WrpFsmStates wrpFsmState = WRP_WAIT_META;

ap_uint<32> tcp_rx_ports_processed = 0;
bool need_tcp_port_req = false;
ap_uint<16> new_relative_port_to_req_tcp = 0;

ap_uint<16> processed_FMC_listen_port = 0;
bool fmc_port_opened = false;

NetworkMetaStream out_meta_tcp = NetworkMetaStream(); //DON'T FORGET to initilize!
NetworkMetaStream in_meta_tcp = NetworkMetaStream(); //ATTENTION: don't forget initilizer...
bool Tcp_RX_metaWritten = false;
ap_uint<64>  tripple_for_new_connection = 0;
bool tcp_need_new_connection_request = false;
bool tcp_new_connection_failure = false;
ap_uint<16> tcp_new_connection_failure_cnt = 0;

SessionId session_toFMC = 0;
SessionId session_fromFMC = 0;
bool expect_FMC_response = false;

NetworkDataLength tcpTX_packet_length = 0;
NetworkDataLength tcpTX_current_packet_length = 0;


//returns the ZERO-based bit position (so 0 for LSB)
ap_uint<32> getRightmostBitPos(ap_uint<32> num) 
{ 
//#pragma HLS inline
  //return (ap_uint<32>) log2((ap_fixed<32,2>) (num & -num));
  ap_uint<32> pos = 0; 
  for (int i = 0; i < 32; i++) {
//#pragma HLS unroll factor=8
    if (!(num & (1 << i)))
    {
      pos++; 
    }
    else {
      break; 
    }
  } 
  return pos; 
}


NodeId getNodeIdFromIpAddress(ap_uint<32> ipAddr)
{
//#pragma HLS inline
  //bool foundIt = false;
  //NodeId ret = 0xFFFF;
  //Loop unroll pragma needs int as variable...
  for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
  {
//#pragma HLS unroll //factor=8
    //if(!foundIt && localMRT[i] == ipAddr)
    if(localMRT[i] == ipAddr)
    {
      return (NodeId) i;
      //foundIt = true;
      //ret = (NodeId) i;
    }
  }
  //if(!foundIt)
  //{
  //  node_id_missmatch_RX_cnt++;
  //}
  return 0xFFFF;
  //return ret;
}


ap_uint<64> newTripple(Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort)
{
//#pragma HLS inline
  ap_uint<64> new_entry = (((ap_uint<64>) ipRemoteAddres) << 32) | (((ap_uint<32>) tcpRemotePort) << 16) | tcpLocalPort;
  return new_entry;
}


Ip4Addr getRemoteIpAddrFromTripple(ap_uint<64> tripple)
{
//#pragma HLS inline
  ap_uint<32> ret = ((ap_uint<32>) (tripple >> 32)) & 0xFFFFFFFF;
  return (Ip4Addr) ret;
}


TcpPort getRemotePortFromTripple(ap_uint<64> tripple)
{
//#pragma HLS inline
  ap_uint<16> ret = ((ap_uint<16>) (tripple >> 16)) & 0xFFFF;
  return (TcpPort) ret;
}


TcpPort getLocalPortFromTripple(ap_uint<64> tripple)
{
//#pragma HLS inline
  ap_uint<16> ret = (ap_uint<16>) (tripple & 0xFFFF);
  return (TcpPort) ret;
}


ap_uint<64> getTrippleFromSessionId(SessionId sessionID)
{
//#pragma HLS inline off
  printf("searching for session: %d\n", (int) sessionID);
  uint32_t i = 0;
  for(i = 0; i < MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(sessionIdList[i] == sessionID && usedRows[i] == 1 && rowsToDelete[i] == 0)
    {
      ap_uint<64> ret = tripleList[i];
      printf("found tripple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) ret);
      return ret;
    }
  }
  //unkown session TODO
  printf("ERROR: unkown session\n");
  return (ap_uint<64>) UNUSED_TABLE_ENTRY_VALUE;
}


SessionId getSessionIdFromTripple(ap_uint<64> tripple)
{
//#pragma HLS inline off
  printf("Searching for tripple: %llu\n", (unsigned long long) tripple);
  uint32_t i = 0;
  for(i = 0; i < MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(tripleList[i] == tripple && usedRows[i] == 1 && rowsToDelete[i] == 0)
    {
      return sessionIdList[i];
    }
  }
  //there is (not yet) a connection
  printf("ERROR: unkown tripple\n");
  return (SessionId) UNUSED_SESSION_ENTRY_VALUE;
}


void addnewTrippleToTable(SessionId sessionID, ap_uint<64> new_entry)
{
//#pragma HLS inline off
  printf("new tripple entry: %d |  %llu\n",(int) sessionID, (unsigned long long) new_entry);
  //first check for duplicates!
  ap_uint<64> test_tripple = getTrippleFromSessionId(sessionID);
  if(test_tripple == new_entry)
  {
    printf("session/tripple already known, skipping. \n");
    return;
  }

  uint32_t i = 0;
  for(i = 0; i < MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(usedRows[i] == 0)
    {//next free one, tables stay in sync
      sessionIdList[i] = sessionID;
      tripleList[i] = new_entry;
      usedRows[i] = 1;
      printf("stored tripple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) new_entry);
      return;
    }
  }
  //we run out of sessions...TODO
  printf("ERROR: no free space in table left!\n");
}


void addnewSessionToTable(SessionId sessionID, Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort)
{
//#pragma HLS inline
  ap_uint<64> new_entry = newTripple(ipRemoteAddres, tcpRemotePort, tcpLocalPort);
  addnewTrippleToTable(sessionID, new_entry);
}


void deleteSessionFromTables(SessionId sessionID)
{
//#pragma HLS inline off
  printf("try to delete session: %d\n", (int) sessionID);
  for(uint32_t i = 0; i < MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(sessionIdList[i] == sessionID && usedRows[i] == 1)
    {
      usedRows[i] = 0;
      printf("found and deleting session: %d\n", (int) sessionID);
      return;
    }
  }
  //nothing to delete, nothing to do...
}


void markCurrentRowsAsToDelete()
{
//#pragma HLS inline
  for(uint32_t i = 0; i< MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    rowsToDelete[i] = usedRows[i];
  }
}


SessionId getAndDeleteNextMarkedRow()
{
//#pragma HLS inline off
  for(uint32_t i = 0; i< MAX_NRC_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(rowsToDelete[i] == 1)
    {
      SessionId ret = sessionIdList[i];
      //sessionIdList[i] = 0x0; //not necessary
      //tripleList[i] = 0x0;
      usedRows[i] = 0;
      rowsToDelete[i] = 0;
      printf("Closing session %d at table row %d.\n",(int) ret, (int) i);
      return ret;
    }
  }
  //Tables are empty
  printf("TCP tables are empty\n");
  return (SessionId) UNUSED_SESSION_ENTRY_VALUE;
}


/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 *
 *****************************************************************************/
void nrc_main(
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
    stream<UdpWord>             &siUdp_data,
    stream<UdpWord>             &soUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<NetworkMetaStream>   &soUdp_meta,

    // -- ROLE TCP connection
    ap_uint<32>                 *pi_tcp_rx_ports,
    stream<TcpWord>             &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<TcpWord>             &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,

    // -- FMC TCP connection
    stream<TcpWord>             &siFMC_Tcp_data,
    stream<Axis<16> >           &siFMC_Tcp_SessId,
    stream<TcpWord>             &soFMC_Tcp_data,
    stream<Axis<16> >           &soFMC_Tcp_SessId,

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
    stream<AppNotif>    &siTOE_Notif,
    stream<AppRdReq>    &soTOE_DReq,
    stream<NetworkWord> &siTOE_Data,
    stream<AppMeta>     &siTOE_SessId,
    //-- TOE / Listen Interfaces
    stream<AppLsnReq>   &soTOE_LsnReq,
    stream<AppLsnAck>   &siTOE_LsnAck,
    //-- TOE / Tx Data Interfaces
    stream<NetworkWord> &soTOE_Data,
    stream<AppMeta>     &soTOE_SessId,
    stream<AppWrSts>    &siTOE_DSts,
    //-- TOE / Open Interfaces
    stream<AppOpnReq>   &soTOE_OpnReq,
    stream<AppOpnSts>   &siTOE_OpnRep,
    //-- TOE / Close Interfaces
    stream<AppClsReq>   &soTOE_ClsReq
    )
{

#pragma HLS INTERFACE ap_vld register port=layer_4_enabled name=piLayer4enabled
#pragma HLS INTERFACE ap_vld register port=layer_7_enabled name=piLayer7enabled
#pragma HLS INTERFACE ap_vld register port=role_decoupled  name=piRoleDecoup_active
#pragma HLS INTERFACE ap_vld register port=piNTS_ready name=piNTS_ready

#pragma HLS INTERFACE axis register both port=siUdp_data
#pragma HLS INTERFACE axis register both port=soUdp_data

#pragma HLS INTERFACE axis register both port=siUdp_meta
#pragma HLS INTERFACE axis register both port=soUdp_meta


#pragma HLS INTERFACE axis register both port=soUOE_LsnReq   name=soUOE_Udp_LsnReq
#pragma HLS INTERFACE axis register both port=siUOE_LsnRep   name=siUOE_Udp_LsnRep
#pragma HLS INTERFACE axis register both port=soUOE_ClsReq   name=soUOE_Udp_ClsReq
#pragma HLS INTERFACE axis register both port=siUOE_ClsRep   name=siUOE_Udp_ClsRep

#pragma HLS INTERFACE axis register both port=siUOE_Data     name=siUOE_Udp_Data
#pragma HLS INTERFACE axis register both port=siUOE_Meta     name=siUOE_Udp_Meta
#pragma HLS DATA_PACK                variable=siUOE_Meta

#pragma HLS INTERFACE axis register both port=soUOE_Data     name=soUOE_Udp_Data
#pragma HLS INTERFACE axis register both port=soUOE_Meta     name=soUOE_Udp_Meta
#pragma HLS DATA_PACK                variable=soUOE_Meta
#pragma HLS INTERFACE axis register both port=soUOE_DLen     name=soUOE_Udp_DLen

#pragma HLS INTERFACE ap_vld register port=myIpAddress name=piMyIpAddress
#pragma HLS INTERFACE ap_vld register port=pi_udp_rx_ports name=piROL_Udp_Rx_ports
#pragma HLS INTERFACE ap_vld register port=piMMIO_FmcLsnPort name=piMMIO_FmcLsnPort
#pragma HLS INTERFACE ap_vld register port=piMMIO_CfrmIp4Addr name=piMMIO_CfrmIp4Addr

#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piFMC_NRC_ctrlLink_AXI
  //#pragma HLS INTERFACE s_axilite port=return bundle=piFMC_NRC_ctrlLink_AXI
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE ap_vld register port=pi_tcp_rx_ports name=piROL_Tcp_Rx_ports

#pragma HLS INTERFACE ap_fifo port=siFMC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=soFMC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=siFMC_Tcp_SessId
#pragma HLS INTERFACE ap_fifo port=soFMC_Tcp_SessId


#pragma HLS INTERFACE axis register both port=siTOE_Notif
#pragma HLS DATA_PACK                variable=siTOE_Notif
#pragma HLS INTERFACE axis register both port=soTOE_DReq
#pragma HLS DATA_PACK                variable=soTOE_DReq
#pragma HLS INTERFACE axis register both port=siTOE_Data
#pragma HLS INTERFACE axis register both port=siTOE_SessId

#pragma HLS INTERFACE axis register both port=soTOE_LsnReq
#pragma HLS INTERFACE axis register both port=siTOE_LsnAck

#pragma HLS INTERFACE axis register both port=soTOE_Data
#pragma HLS INTERFACE axis register both port=soTOE_SessId
#pragma HLS INTERFACE axis register both port=siTOE_DSts

#pragma HLS INTERFACE axis register both port=soTOE_OpnReq
#pragma HLS DATA_PACK                variable=soTOE_OpnReq
#pragma HLS INTERFACE axis register both port=siTOE_OpnRep
#pragma HLS DATA_PACK                variable=siTOE_OpnRep

#pragma HLS INTERFACE axis register both port=soTOE_ClsReq

  // Pragmas for internal variables
#pragma HLS DATAFLOW interval=1
  //#pragma HLS PIPELINE II=1 //TODO/FIXME: is this necessary??


  //=================================================================================================
  // Reset global variables 

#pragma HLS reset variable=Udp_RX_metaWritten
#pragma HLS reset variable=fsmStateRX_Udp
#pragma HLS reset variable=fsmStateTX_Udp
#pragma HLS reset variable=openPortWaitTime
#pragma HLS reset variable=mmio_stabilize_counter
#pragma HLS reset variable=udp_lsn_watchDogTimer
#pragma HLS reset variable=mrt_version_processed
#pragma HLS reset variable=udp_rx_ports_processed
#pragma HLS reset variable=udp_rx_ports_to_close
#pragma HLS reset variable=need_udp_port_req
#pragma HLS reset variable=new_relative_port_to_req_udp
#pragma HLS reset variable=clsFsmState_Udp
#pragma HLS reset variable=newRelativePortToClose
#pragma HLS reset variable=newAbsolutePortToClose
#pragma HLS reset variable=node_id_missmatch_RX_cnt
#pragma HLS reset variable=node_id_missmatch_TX_cnt
#pragma HLS reset variable=port_corrections_TX_cnt
#pragma HLS reset variable=packet_count_RX
#pragma HLS reset variable=packet_count_TX
#pragma HLS reset variable=last_rx_node_id
#pragma HLS reset variable=last_rx_port
#pragma HLS reset variable=last_tx_node_id
#pragma HLS reset variable=last_tx_port
#pragma HLS reset variable=out_meta_udp
#pragma HLS reset variable=in_meta_udp
#pragma HLS reset variable=udpTX_packet_length
#pragma HLS reset variable=udpTX_current_packet_length
#pragma HLS reset variable=unauthorized_access_cnt
#pragma HLS reset variable=authorized_access_cnt

//to reset arrays has weird side effects...
//#pragma HLS reset variable=localMRT //off
//#pragma HLS reset variable=config //off
//#pragma HLS reset variable=tripleList //off
//#pragma HLS reset variable=sessionIdList //off
//#pragma HLS reset variable=usedRows //off
//#pragma HLS reset variable=rowsToDelete//off

#pragma HLS reset variable=startupDelay
#pragma HLS reset variable=opnFsmState
#pragma HLS reset variable=clsFsmState_Tcp
#pragma HLS reset variable=lsnFsmState
#pragma HLS reset variable=rrhFsmState
#pragma HLS reset variable=rdpFsmState
#pragma HLS reset variable=wrpFsmState
#pragma HLS reset variable=tcp_rx_ports_processed
#pragma HLS reset variable=need_tcp_port_req
#pragma HLS reset variable=new_relative_port_to_req_tcp
#pragma HLS reset variable=processed_FMC_listen_port
#pragma HLS reset variable=fmc_port_opened
#pragma HLS reset variable=tables_initalized
#pragma HLS reset variable=out_meta_tcp
#pragma HLS reset variable=in_meta_tcp
#pragma HLS reset variable=session_toFMC
#pragma HLS reset variable=session_fromFMC
#pragma HLS reset variable=Tcp_RX_metaWritten
#pragma HLS reset variable=tcpTX_packet_length
#pragma HLS reset variable=tcpTX_current_packet_length
#pragma HLS reset variable=tripple_for_new_connection
#pragma HLS reset variable=tcp_need_new_connection_request
#pragma HLS reset variable=tcp_new_connection_failure
#pragma HLS reset variable=tcp_new_connection_failure_cnt

#pragma HLS reset variable=tableCopyVariable
#pragma HLS reset variable=expect_FMC_response


  //===========================================================
  //  core wide variables (for one iteration)

  //ap_uint<32> ipAddrLE = 0;
  //ipAddrLE  = (ap_uint<32>) ((*myIpAddress >> 24) & 0xFF);
  //ipAddrLE |= (ap_uint<32>) ((*myIpAddress >> 8) & 0xFF00);
  //ipAddrLE |= (ap_uint<32>) ((*myIpAddress << 8) & 0xFF0000);
  //ipAddrLE |= (ap_uint<32>) ((*myIpAddress << 24) & 0xFF000000);
  ap_uint<32> ipAddrBE = *myIpAddress;


  //if(*piNTS_ready != 1)
  //{
  //  return;
  //}

  //===========================================================
  // restore saved states
  
  // > to avoid loop at 0
  if(config[NRC_CONFIG_SAVED_FMC_PORTS] > processed_FMC_listen_port)
  {
    processed_FMC_listen_port = (ap_uint<16>) config[NRC_CONFIG_SAVED_FMC_PORTS];
  }

  if(*layer_7_enabled == 1)
  { // looks like only we were reset
    // since the user cannot close ports (up to now), the > should work...
    if(config[NRC_CONFIG_SAVED_UDP_PORTS] > udp_rx_ports_processed)
    {
      udp_rx_ports_processed = config[NRC_CONFIG_SAVED_UDP_PORTS];
    }

    if(config[NRC_CONFIG_SAVED_TCP_PORTS] > tcp_rx_ports_processed)
    {
      tcp_rx_ports_processed = config[NRC_CONFIG_SAVED_TCP_PORTS];
    }
  }

  //if layer 4 is reset, ports will be closed 
  if(*layer_4_enabled == 0)
  {
    processed_FMC_listen_port = 0x0;
    udp_rx_ports_processed = 0x0;
    tcp_rx_ports_processed = 0x0;
    //also, all sessions should be lost 
    tables_initalized = false;
    //we don't need to close ports any more
    clsFsmState_Tcp = CLS_IDLE;
    clsFsmState_Udp = CLS_IDLE;
    //and we shouldn't expect smth
    expect_FMC_response = false;
  }

  if(*layer_7_enabled == 0 || *role_decoupled == 1 )
  {
    if(*layer_4_enabled == 1 && *piNTS_ready == 1 && tables_initalized)
    {
      if(udp_rx_ports_processed > 0)
      {
        //mark all UDP ports as to be deleted
        udp_rx_ports_to_close = udp_rx_ports_processed;
        //start closing FSM UDP
        clsFsmState_Udp = CLS_NEXT;
      }

      if(tcp_rx_ports_processed > 0)
      {
        //mark all TCP ports as to be deleted
        markCurrentRowsAsToDelete();
        //start closing FSM TCP
        clsFsmState_Tcp = CLS_NEXT;
      }
    }
    //in all cases
    udp_rx_ports_processed = 0x0;
    tcp_rx_ports_processed = 0x0;
  }
  //===========================================================
  // MRT init

  //TODO: some consistency check for tables? (e.g. every IP address only once...)

  if (!tables_initalized)
  {
    printf("init tables...\n");
    for(int i = 0; i<MAX_NRC_SESSIONS; i++)
    {
      //#pragma HLS unroll
      sessionIdList[i] = 0;
      tripleList[i] = 0;
      usedRows[i]  =  0;
      rowsToDelete[i] = 0;
    }
    udp_rx_ports_to_close = 0x0;
    
    tables_initalized = true;
    //printf("Table layout:\n");
    //for(int i = 0; i<MAX_NRC_SESSIONS; i++)
    //{
    //  printf("%d | %d |  %llu\n",(int) i, (int) sessionIdList[i], (unsigned long long) tripleList[i]);
    //}
  }

  //remaining MRT handling moved to the bottom

  //only if NTS is ready
  if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  {
    //===========================================================
    //  port requests
    //  only if a user application is running...
    if((udp_rx_ports_processed != *pi_udp_rx_ports) && *layer_7_enabled == 1
        && *role_decoupled == 0 && !need_udp_port_req )
    {
      //we close ports only if layer 7 is reset, so only look for new ports
      ap_uint<32> tmp = udp_rx_ports_processed | *pi_udp_rx_ports;
      ap_uint<32> diff = udp_rx_ports_processed ^ tmp;
      //printf("rx_ports IN: %#04x\n",(int) *pi_udp_rx_ports);
      //printf("udp_rx_ports_processed: %#04x\n",(int) udp_rx_ports_processed);
      printf("UDP port diff: %#04x\n",(unsigned int) diff);
      if(diff != 0)
      {//we have to open new ports, one after another
        new_relative_port_to_req_udp = getRightmostBitPos(diff);
        need_udp_port_req = true;
      }
    } //else {
      //need_udp_port_req = false;
    //}

    if(processed_FMC_listen_port != *piMMIO_FmcLsnPort
        && !need_tcp_port_req )
    {
      if(mmio_stabilize_counter == 0)
      {
        fmc_port_opened = false;
        need_tcp_port_req = true;
#ifndef __SYNTHESIS__
        mmio_stabilize_counter = 1;
#else
        mmio_stabilize_counter = NRC_MMIO_STABILIZE_TIME;
#endif
        printf("Need FMC port request: %#02x\n",(unsigned int) *piMMIO_FmcLsnPort);
      } else {
        mmio_stabilize_counter--;
      }

    } else if((tcp_rx_ports_processed != *pi_tcp_rx_ports) && *layer_7_enabled == 1
        && *role_decoupled == 0 && !need_tcp_port_req )
    { //  only if a user application is running...
      //we close ports only if layer 7 is reset, so only look for new ports
      ap_uint<32> tmp = tcp_rx_ports_processed | *pi_tcp_rx_ports;
      ap_uint<32> diff = tcp_rx_ports_processed ^ tmp;
      //printf("rx_ports IN: %#04x\n",(int) *pi_tcp_rx_ports);
      //printf("tcp_rx_ports_processed: %#04x\n",(int) tcp_rx_ports_processed);
      printf("TCP port diff: %#04x\n",(unsigned int) diff);
      if(diff != 0)
      {//we have to open new ports, one after another
        new_relative_port_to_req_tcp = getRightmostBitPos(diff);
        need_tcp_port_req = true;
      }
    } //else {
      //need_tcp_port_req = false;
    //}
  }

  //=================================================================================================
  // TX UDP

  //only if NTS is ready
  if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  {
    switch(fsmStateTX_Udp) {

      default:
      case FSM_RESET: 
        fsmStateTX_Udp = FSM_W8FORMETA;
        udpTX_packet_length = 0;
        udpTX_current_packet_length = 0;
        //NO break! --> to be same as FSM_W8FORMETA
      case FSM_W8FORMETA:
        // The very first time, wait until the Rx path provides us with the
        // socketPair information before continuing
        if ( !siUdp_meta.empty() && !soUOE_Meta.full() &&
            !siUdp_data.empty() && !soUOE_Data.full() &&
            !soUOE_DLen.full() )
        {
          NetworkMetaStream tmp_meta_in = siUdp_meta.read();
          udpTX_packet_length = tmp_meta_in.tdata.len;
          udpTX_current_packet_length = 0;

          // Send out the first data together with the metadata and payload length information

          //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
          NodeId dst_rank = tmp_meta_in.tdata.dst_rank;
          if(dst_rank > MAX_CF_NODE_ID)
          {
            node_id_missmatch_TX_cnt++;
            //SINK packet
            fsmStateTX_Udp = FSM_DROP_PACKET;
            break;
          }
          ap_uint<32> dst_ip_addr = localMRT[dst_rank];
          if(dst_ip_addr == 0)
          {
            node_id_missmatch_TX_cnt++;
            //SINK packet
            fsmStateTX_Udp = FSM_DROP_PACKET;
            break;
          }
          last_tx_node_id = dst_rank;
          NrcPort src_port = out_meta_udp.tdata.src_port; //TODO: DEBUG
          if (src_port == 0)
          {
            src_port = DEFAULT_RX_PORT;
          }
          NrcPort dst_port = out_meta_udp.tdata.dst_port; //TODO: DEBUG
          if (dst_port == 0)
          {
            dst_port = DEFAULT_RX_PORT;
            port_corrections_TX_cnt++;
          }
          last_tx_port = dst_port;
          // {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
          //UdpMeta txMeta = {{src_port, ipAddrBE}, {dst_port, dst_ip_addr}};
          UdpMeta txMeta = SocketPair(SockAddr(ipAddrBE, src_port), SockAddr(dst_ip_addr, dst_port));
          soUOE_Meta.write(txMeta);
          //we can forward the length, even if 0
          //the UOE handles this as streaming mode
          soUOE_DLen.write(udpTX_packet_length);
          packet_count_TX++;

          // Forward data chunk, metadata and payload length
          UdpWord    aWord = siUdp_data.read();
          udpTX_current_packet_length++;
          if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
          {//we need to set tlast manually
            aWord.tlast = 1;
          }

          soUOE_Data.write(aWord);
          if (aWord.tlast == 1)
          {
            fsmStateTX_Udp = FSM_W8FORMETA;
          } else {
            fsmStateTX_Udp = FSM_ACC;
          }
        }
        break;

      case FSM_ACC:
        // Default stream handling
        if ( !siUdp_data.empty() && !soUOE_Data.full() )
        {
          // Forward data chunk
          UdpWord    aWord = siUdp_data.read();
          udpTX_current_packet_length++;
          if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
          {//we need to set tlast manually
            aWord.tlast = 1;
          }

          soUOE_Data.write(aWord);
          // Until LAST bit is set
          if (aWord.tlast == 1)
          {
            fsmStateTX_Udp = FSM_W8FORMETA;
          }
        }
        break;

      case FSM_DROP_PACKET:
        if ( !siUdp_data.empty() ) {
          UdpWord    aWord = siUdp_data.read();
          udpTX_current_packet_length++;
          if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
          {//we need to set tlast manually
            aWord.tlast = 1;
          }
          // Until LAST bit is set (with length or with tlast)
          if (aWord.tlast == 1)
          {
            fsmStateTX_Udp = FSM_W8FORMETA;
          }
        }
        break;
    }
  }

    //=================================================================================================
    // RX UDP
    char   *myName  = concat3(THIS_NAME, "/", "Udp_RX");

    //only if NTS is ready
    //we DO NOT need to check for layer_7_enabled or role_decoupled, because then Ports should be closed
    if(*piNTS_ready == 1 && *layer_4_enabled == 1)
    {
      switch(fsmStateRX_Udp) {

        default:
        case FSM_RESET:
            #ifndef __SYNTHESIS__
                openPortWaitTime = 10;
            #else
                openPortWaitTime = 100;
            #endif
          fsmStateRX_Udp = FSM_IDLE;
          break;

        case FSM_IDLE:
          if(openPortWaitTime == 0) { 
            if ( !soUOE_LsnReq.full() && need_udp_port_req) {
              ap_uint<16> new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_udp;
              soUOE_LsnReq.write(new_absolute_port);
              fsmStateRX_Udp = FSM_W8FORPORT;
              #ifndef __SYNTHESIS__
                udp_lsn_watchDogTimer = 10;
              #else
                udp_lsn_watchDogTimer = 100;
              #endif
                if (DEBUG_LEVEL & TRACE_LSN) {
                  printInfo(myName, "SHELL/UOE is requested to listen on port #%d (0x%4.4X).\n",
                      (int) new_absolute_port, (int) new_absolute_port);
                }
            } else if(udp_rx_ports_processed > 0)
            { // we have already at least one open port 
              //don't hang after reset
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          else {
            openPortWaitTime--;
          }
          break;

        case FSM_W8FORPORT:
          udp_lsn_watchDogTimer--;
          if ( !siUOE_LsnRep.empty() ) {
            // Read the acknowledgment
            StsBool sOpenAck = siUOE_LsnRep.read();
            printf("new udp_rx_ports_processed: %#03x\n",(int) udp_rx_ports_processed);
            if (sOpenAck) {
              printInfo(myName, "Received listen acknowledgment from [UOE].\n");
              fsmStateRX_Udp = FSM_FIRST_ACC;
              //port acknowleded
              need_udp_port_req = false;
              udp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_udp);
            }
            else {
              printWarn(myName, "UOE denied listening on port %d (0x%4.4X).\n",
                        (int) (NRC_RX_MIN_PORT + new_relative_port_to_req_udp),
                        (int) (NRC_RX_MIN_PORT + new_relative_port_to_req_udp));
              fsmStateRX_Udp = FSM_IDLE;
            }
          } else {
            if (udp_lsn_watchDogTimer == 0) {
              printError(myName, "Timeout: Server failed to listen on new udp port.\n");
              fsmStateRX_Udp = FSM_IDLE;
            }
          }
          break;

        case FSM_FIRST_ACC:
          // Wait until both the first data chunk and the first metadata are received from UDP
          if ( !siUOE_Data.empty() && !siUOE_Meta.empty() ) {
            if ( !soUdp_data.full() && !soUdp_meta.full() ) {

              //extrac src ip address
              UdpMeta udpRxMeta = siUOE_Meta.read();
              NodeId src_id = getNodeIdFromIpAddress(udpRxMeta.src.addr);
              if(src_id == 0xFFFF)
              {
                //SINK packet
                //node_id_missmatch_RX_cnt++; is done by getNodeIdFromIpAddress
                fsmStateRX_Udp = FSM_DROP_PACKET;
                break;
              }
              last_rx_node_id = src_id;
              last_rx_port = udpRxMeta.dst.port;
              NetworkMeta tmp_meta = NetworkMeta(config[NRC_CONFIG_OWN_RANK], udpRxMeta.dst.port, src_id, udpRxMeta.src.port, 0);
              //FIXME: add length here as soon as available from the UOE
              in_meta_udp = NetworkMetaStream(tmp_meta);
              // Forward data chunk to ROLE
              UdpWord    udpWord = siUOE_Data.read();
              soUdp_data.write(udpWord);

              Udp_RX_metaWritten = false; //don't put the meta stream in the critical path
              if (!udpWord.tlast) {
                fsmStateRX_Udp = FSM_ACC;
              } else { 
                fsmStateRX_Udp = FSM_WRITE_META; 
              }
            } 
          }
          if(need_udp_port_req)
          {
            fsmStateRX_Udp = FSM_IDLE;
          }
          break;

        case FSM_ACC:
          // Default stream handling
          if ( !siUOE_Data.empty() && !soUdp_data.full() ) {
            // Forward data chunk to ROLE
            UdpWord    udpWord = siUOE_Data.read();
            soUdp_data.write(udpWord);
            // Until LAST bit is set
            if (udpWord.tlast)
            {
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          //no break!
        case FSM_WRITE_META:
          if ( !Udp_RX_metaWritten && !soUdp_meta.full() )
          {
            soUdp_meta.write(in_meta_udp);
            packet_count_RX++;
            Udp_RX_metaWritten = true;
            if ( fsmStateRX_Udp == FSM_WRITE_META)
            {//was a small packet
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          break;

        case FSM_DROP_PACKET:
          if( !siUOE_Data.empty() )
          {
            UdpWord    udpWord = siUOE_Data.read();
            // Until LAST bit is set
            if (udpWord.tlast)
            {
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          break;
      }
    }


    //=================================================================================================
    // UDP Port Close
    
    //redefinition
    myName  = concat3(THIS_NAME, "/", "Udp_Cls");
    
    //only if NTS is ready
    //and if we have valid tables
    if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
    {
      switch (clsFsmState_Udp) {
        default:
        case CLS_IDLE:
          //we wait until we are activated;
          break;
        case CLS_NEXT:
          if( udp_rx_ports_to_close != 0 )
          {
            //we have to close opened ports, one after another
            newRelativePortToClose = getRightmostBitPos(udp_rx_ports_to_close);
            newAbsolutePortToClose = NRC_RX_MIN_PORT + newRelativePortToClose;
            if(!soUOE_ClsReq.full()) {
              soUOE_ClsReq.write(newAbsolutePortToClose);
              clsFsmState_Udp = CLS_WAIT4RESP;
            } //else: just tay here
          } else {
            clsFsmState_Udp = CLS_IDLE;
          }
          break;
        case CLS_WAIT4RESP:
          if(!siUOE_ClsRep.empty())
          {
            StsBool isOpened;
            siUOE_ClsRep.read(isOpened);
            if (not isOpened)
            {
                printInfo(myName, "Received close acknowledgment from [UOE].\n");
                //update ports to close
                ap_uint<32> one_cold_closed_port = ~(((ap_uint<32>) 1) << (newRelativePortToClose));
                udp_rx_ports_to_close &= one_cold_closed_port;
                printf("new UDP port ports to close: %#04x\n",(unsigned int) udp_rx_ports_to_close);
            }
            else {
                printWarn(myName, "UOE denied closing the port %d (0x%4.4X) which is still opened.\n",
                          (int) newAbsolutePortToClose, (int) newAbsolutePortToClose);
            }
            //in all cases
            clsFsmState_Udp = CLS_NEXT;
          }
          break;
      }
    }

    //=================================================================================================
    // TCP pListen
    /*****************************************************************************
     * @brief Request the TOE to start listening (LSn) for incoming connections
     *  on a specific port (.i.e open connection for reception mode).
     *
     * @param[out] soTOE_LsnReq,   listen port request to TOE.
     * @param[in]  siTOE_LsnAck,   listen port acknowledge from TOE.
     *
     * @warning
     *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
     *   32,767) which are used for listening ports, and one for dynamically
     *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
     *   connections. Therefore, listening port numbers must be in the range 0
     *   to 32,767.
     ******************************************************************************/

    myName  = concat3(THIS_NAME, "/", "LSn");

    //only if NTS is ready
    if(*piNTS_ready == 1 && *layer_4_enabled == 1)
    {
      switch (lsnFsmState) {

        case LSN_IDLE:
          if (startupDelay > 0)
          {
            startupDelay--;
          } else {
            if(need_tcp_port_req == true)
            {
              lsnFsmState = LSN_SEND_REQ;
            } else {
              lsnFsmState = LSN_DONE;
            }
          }
          break;

        case LSN_SEND_REQ: //we arrive here only if need_tcp_port_req == true
          if (!soTOE_LsnReq.full()) {
            ap_uint<16> new_absolute_port = 0;
            //always process FMC first
            if(fmc_port_opened == false)
            {
              new_absolute_port = *piMMIO_FmcLsnPort;
            } else {
              new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
            }

            AppLsnReq    tcpListenPort = new_absolute_port;
            soTOE_LsnReq.write(tcpListenPort);
            if (DEBUG_LEVEL & TRACE_LSN) {
              printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                  (int) new_absolute_port, (int) new_absolute_port);
#ifndef __SYNTHESIS__
              watchDogTimer_plisten = 10;
#else
              watchDogTimer_plisten = 100;
#endif
              lsnFsmState = LSN_WAIT_ACK;
            }
          }
          else {
            printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
          }
          break;

        case LSN_WAIT_ACK:
          watchDogTimer_plisten--;
          if (!siTOE_LsnAck.empty()) {
            bool    listenDone;
            siTOE_LsnAck.read(listenDone);
            if (listenDone) {
              printInfo(myName, "Received listen acknowledgment from [TOE].\n");
              lsnFsmState = LSN_DONE;

              need_tcp_port_req = false;
              if(fmc_port_opened == false)
              {
                fmc_port_opened = true;
                processed_FMC_listen_port = *piMMIO_FmcLsnPort;
              } else {
                tcp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_tcp);
                printf("new tcp_rx_ports_processed: %#03x\n",(int) tcp_rx_ports_processed);
              }
            }
            else {
              ap_uint<16> new_absolute_port = 0;
              //always process FMC first
              if(fmc_port_opened == false)
              {
                new_absolute_port = *piMMIO_FmcLsnPort;
              } else {
                new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
              }
              printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                  (int) new_absolute_port, (int) new_absolute_port);
              lsnFsmState = LSN_SEND_REQ;
            }
          } else {
            if (watchDogTimer_plisten == 0) {
              ap_uint<16> new_absolute_port = 0;
              //always process FMC first
              if(fmc_port_opened == false)
              {
                new_absolute_port = *piMMIO_FmcLsnPort;
              } else {
                new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
              }
              printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                  (int)  new_absolute_port, (int) new_absolute_port);
              lsnFsmState = LSN_SEND_REQ;
            }
          }
          break;

        case LSN_DONE:
          if(need_tcp_port_req == true)
          {
            lsnFsmState = LSN_SEND_REQ;
          }
          break;
      }
    }

    /*****************************************************************************
     * @brief ReadRequestHandler (RRh).
     *  Waits for a notification indicating the availability of new data for
     *  the ROLE. If the TCP segment length is greater than 0, the notification
     *  is accepted.
     *
     * @param[in]  siTOE_Notif, a new Rx data notification from TOE.
     * @param[out] soTOE_DReq,  a Rx data request to TOE.
     *
     ******************************************************************************/
    //update myName
    myName  = concat3(THIS_NAME, "/", "RRh");

    //only if NTS is ready
    if(*piNTS_ready == 1 && *layer_4_enabled == 1)
    {
      switch(rrhFsmState) {
        case RRH_WAIT_NOTIF:
          if (!siTOE_Notif.empty()) {
            siTOE_Notif.read(notif_pRrh);
            if (notif_pRrh.tcpSegLen != 0) {
              // Always request the data segment to be received
              rrhFsmState = RRH_SEND_DREQ;
              //remember the session ID if not yet known
              addnewSessionToTable(notif_pRrh.sessionID, notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort);
            }
          }
          break;
        case RRH_SEND_DREQ:
          if (!soTOE_DReq.full()) {
            soTOE_DReq.write(AppRdReq(notif_pRrh.sessionID, notif_pRrh.tcpSegLen));
            rrhFsmState = RRH_WAIT_NOTIF;
          }
          break;
      }
    }

      /*****************************************************************************
       * @brief Read Path (RDp) - From TOE to ROLE.
       *  Process waits for a new data segment to read and forwards it to the ROLE.
       *
       * @param[in]  siTOE_Data,   Data from [TOE].
       * @param[in]  siTOE_SessId, Session Id from [TOE].
       * @param[out] soTcp_data,   Data to [ROLE].
       * @param[out] soTcp_meta,   Meta Data to [ROLE].
       *
       *****************************************************************************/
      //update myName
      myName  = concat3(THIS_NAME, "/", "RDp");

      //"local" variables
      NetworkWord currWord;
      AppMeta     sessId;

    //only if NTS is ready
    //we NEED for layer_7_enabled or role_decoupled, because the
    // 1. FMC is still active
    // 2. TCP ports cant be closed up to now [FIXME]
    if(*piNTS_ready == 1 && *layer_4_enabled == 1)
    {
      switch (rdpFsmState ) {

        default:
        case RDP_WAIT_META:
          if (!siTOE_SessId.empty()) {
            siTOE_SessId.read(sessId);

            ap_uint<64> tripple_in = getTrippleFromSessionId(sessId);
            //since we requested the session, we shoul know it -> no error handling
            printf("tripple_in: %llu\n",(unsigned long long) tripple_in);
            Ip4Addr remoteAddr = getRemoteIpAddrFromTripple(tripple_in);
            TcpPort dstPort = getLocalPortFromTripple(tripple_in);
            TcpPort srcPort = getRemotePortFromTripple(tripple_in);
            printf("remote Addr: %d; dstPort: %d; srcPort %d\n", (int) remoteAddr, (int) dstPort, (int) srcPort);

            if(dstPort == processed_FMC_listen_port) 
            {
              if(remoteAddr == *piMMIO_CfrmIp4Addr)
              {//valid connection to FMC
                printf("found valid FMC connection.\n");
                Tcp_RX_metaWritten = false;
                session_toFMC = sessId;
                rdpFsmState = RDP_STREAM_FMC;
                authorized_access_cnt++;
                break;
              } else {
                unauthorized_access_cnt++;
                printf("unauthorized access to FMC!\n");
                rdpFsmState = RDP_DROP_PACKET;
                printf("NRC drops the packet...\n");
                break;
              }
            }
            //no we think this is valid...
            NodeId src_id = getNodeIdFromIpAddress(remoteAddr);
            //printf("TO ROLE: src_rank: %d\n", (int) src_id);
            //Role packet
            if(src_id == 0xFFFF 
                || *layer_7_enabled == 0 || *role_decoupled == 1)
            {
              //SINK packet
              //node_id_missmatch_RX_cnt++; is done by getNodeIdFromIpAddress
              rdpFsmState = RDP_DROP_PACKET;
              printf("NRC drops the packet...\n");
              break;
            }
            last_rx_node_id = src_id;
            last_rx_port = dstPort;
            NetworkMeta tmp_meta = NetworkMeta(config[NRC_CONFIG_OWN_RANK], dstPort, src_id, srcPort, 0);
            in_meta_tcp = NetworkMetaStream(tmp_meta);
            Tcp_RX_metaWritten = false;
            rdpFsmState  = RDP_STREAM_ROLE;
          }
          break;

        case RDP_STREAM_ROLE:
          if (!siTOE_Data.empty() && !soTcp_data.full()) 
          {
            siTOE_Data.read(currWord);
            //if (DEBUG_LEVEL & TRACE_RDP) { TODO: type management
            //  printAxiWord(myName, (AxiWord) currWord);
            //}
            soTcp_data.write(currWord);
            //printf("writing to ROLE...\n\n");
            if (currWord.tlast == 1)
            {
              rdpFsmState  = RDP_WAIT_META;
            }
          }
          // NO break;
        case RDP_WRITE_META_ROLE:
          if( !Tcp_RX_metaWritten && !soTcp_meta.full())
          {
            soTcp_meta.write(in_meta_tcp);
            packet_count_RX++;
            Tcp_RX_metaWritten = true;
          }
          break;

        case RDP_STREAM_FMC:
          //if (!siTOE_Data.empty() && !soFMC_Tcp_data.full()) 
          if (!siTOE_Data.empty() ) //&& !soFMC_Tcp_data.full()) 
          {
            siTOE_Data.read(currWord);
            //if (DEBUG_LEVEL & TRACE_RDP) { TODO: type management
            //  printAxiWord(myName, (AxiWord) currWord);
            //}
            soFMC_Tcp_data.write(currWord);
            if (currWord.tlast == 1)
            {
              expect_FMC_response = true;
              rdpFsmState  = RDP_WAIT_META;
            }
          }
          // NO break;
        case RDP_WRITE_META_FMC:
          //if( !Tcp_RX_metaWritten && !soFMC_Tcp_SessId.full())
          if( !Tcp_RX_metaWritten )
          {
            soFMC_Tcp_SessId.write(session_toFMC);
            //TODO: is tlast set?
            //TODO: count incoming FMC packets?
            Tcp_RX_metaWritten = true;
          }
          break;

        case RDP_DROP_PACKET:
          if( !siTOE_Data.empty())
          {
            siTOE_Data.read(currWord);
            if (currWord.tlast == 1)
            {
              rdpFsmState  = RDP_WAIT_META;
            }
          }
          break;
      }
    }

      /*****************************************************************************
       * @brief Write Path (WRp) - From ROLE to TOE.
       *  Process waits for a new data segment to write and forwards it to TOE.
       *
       * @param[in]  siROL_Data,   Tx data from [ROLE].
       * @param[in]  siROL_SessId, the session Id from [ROLE].
       * @param[out] soTOE_Data,   Tx data to [TOE].
       * @param[out] soTOE_SessId, Tx session Id to to [TOE].
       * @param[in]  siTOE_DSts,   Tx data write status from [TOE].
       *
       ******************************************************************************/
      //update myName
      myName  = concat3(THIS_NAME, "/", "WRp");

      //"local" variables
      AppMeta       tcpSessId;
      NetworkWord   currWordIn;
      NetworkWord   currWordOut;

    //only if NTS is ready
    if(*piNTS_ready == 1 && *layer_4_enabled == 1)
    {
      switch (wrpFsmState) {
        case WRP_WAIT_META:
          //FMC must come first
          //if (!siFMC_Tcp_SessId.empty() && !soTOE_SessId.full())
          //to not wait for ever here for the FIFOs
          if (expect_FMC_response && !siFMC_Tcp_SessId.empty() && !soTOE_SessId.full())
          {
            tcpSessId = (AppMeta) siFMC_Tcp_SessId.read().tdata;
            soTOE_SessId.write(tcpSessId);
            //delete the session id, we don't need it any longer
            deleteSessionFromTables(tcpSessId);
            expect_FMC_response = false;

            if (DEBUG_LEVEL & TRACE_WRP) {
              printInfo(myName, "Received new session ID #%d from [FMC].\n",
                  tcpSessId.to_uint());
            }
            wrpFsmState = WRP_STREAM_FMC;
            break;
          }
          //now ask the ROLE 
          if (!siTcp_meta.empty() && !soTOE_SessId.full()) 
          {
            out_meta_tcp = siTcp_meta.read();
            tcpTX_packet_length = out_meta_tcp.tdata.len;
            tcpTX_current_packet_length = 0;

            NodeId dst_rank = out_meta_tcp.tdata.dst_rank;
            if(dst_rank > MAX_CF_NODE_ID)
            {
              node_id_missmatch_TX_cnt++;
              //dst_rank = 0;
              //SINK packet
              wrpFsmState = WRP_DROP_PACKET;
              printf("NRC drops the packet...\n");
              break;
            }
            Ip4Addr dst_ip_addr = localMRT[dst_rank];
            if(dst_ip_addr == 0)
            {
              node_id_missmatch_TX_cnt++;
              //dst_ip_addr = localMRT[0];
              //SINK packet
              wrpFsmState = WRP_DROP_PACKET;
              printf("NRC drops the packet...\n");
              break;
            }
            NrcPort src_port = out_meta_tcp.tdata.src_port; //TODO: DEBUG
            if (src_port == 0)
            {
              src_port = DEFAULT_RX_PORT;
            }
            NrcPort dst_port = out_meta_tcp.tdata.dst_port; //TODO: DEBUG
            if (dst_port == 0)
            {
              dst_port = DEFAULT_RX_PORT;
              port_corrections_TX_cnt++;
            }

            //check if session is present
            ap_uint<64> new_tripple = newTripple(dst_ip_addr, dst_port, src_port);
            printf("From ROLE: remote Addr: %d; dstPort: %d; srcPort %d; (rank: %d)\n", (int) dst_ip_addr, (int) dst_port, (int) src_port, (int) dst_rank);
            SessionId sessId = getSessionIdFromTripple(new_tripple);
            printf("session id found: %d\n", (int) sessId);
            if(sessId == (SessionId) UNUSED_SESSION_ENTRY_VALUE)
            {//we need to create one first
              tripple_for_new_connection = new_tripple;
              tcp_need_new_connection_request = true;
              tcp_new_connection_failure = false;
              wrpFsmState = WRP_WAIT_CONNECTION;
              printf("requesting new connection.\n");
              break;
            }
            last_tx_port = dst_port;
            last_tx_node_id = dst_rank;
            packet_count_TX++;

            soTOE_SessId.write(sessId);
            if (DEBUG_LEVEL & TRACE_WRP) {
              printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                  sessId.to_uint());
            }
            wrpFsmState = WRP_STREAM_ROLE;
          }
          break;

        case WRP_WAIT_CONNECTION:
          if( !tcp_need_new_connection_request && !soTOE_SessId.full() && !tcp_new_connection_failure )
          {
            SessionId sessId = getSessionIdFromTripple(tripple_for_new_connection);

            last_tx_port = getRemotePortFromTripple(tripple_for_new_connection);
            last_tx_node_id = getNodeIdFromIpAddress(getRemoteIpAddrFromTripple(tripple_for_new_connection));
            packet_count_TX++;

            soTOE_SessId.write(sessId);
            if (DEBUG_LEVEL & TRACE_WRP) {
              printInfo(myName, "Received new session ID #%d from [ROLE].\n",
                  sessId.to_uint());
            }
            wrpFsmState = WRP_STREAM_ROLE;

          } else if (tcp_new_connection_failure)
          {
            tcp_new_connection_failure_cnt++;
            // we sink the packet, because otherwise the design will hang 
            // and the user is notified with the flight recorder status
            wrpFsmState = WRP_DROP_PACKET;
            printf("NRC drops the packet...\n");
            break;
          }
          break;

        case WRP_STREAM_FMC:
          if (!siFMC_Tcp_data.empty() && !soTOE_Data.full()) {
            siFMC_Tcp_data.read(currWordIn);
            //if (DEBUG_LEVEL & TRACE_WRP) {
            //     printAxiWord(myName, currWordIn);
            //}
            soTOE_Data.write(currWordIn);
            if(currWordIn.tlast == 1) {
              wrpFsmState = WRP_WAIT_META;
            }
          }
          break;

        case WRP_STREAM_ROLE:
          if (!siTcp_data.empty() && !soTOE_Data.full()) {
            siTcp_data.read(currWordIn);
            tcpTX_current_packet_length++;
            //if (DEBUG_LEVEL & TRACE_WRP) {
            //     printAxiWord(myName, currWordIn);
            //}
            printf("streaming from ROLE to TOE: tcpTX_packet_length: %d, tcpTX_current_packet_length: %d \n", (int) tcpTX_packet_length, (int) tcpTX_current_packet_length);
            if(tcpTX_packet_length > 0 && tcpTX_current_packet_length >= tcpTX_packet_length)
            {
              currWordIn.tlast = 1;
            }
            soTOE_Data.write(currWordIn);
            if(currWordIn.tlast == 1) {
              wrpFsmState = WRP_WAIT_META;
            }
          } else {
            printf("ERROR: can't stream to TOE!\n");
          }
          break;

        case WRP_DROP_PACKET: 
          if( !siTcp_data.empty()) 
          {
            siTcp_data.read(currWordIn);
            tcpTX_current_packet_length++;
            //until Tlast or length
            if(tcpTX_packet_length > 0 && tcpTX_current_packet_length >= tcpTX_packet_length)
            {
              currWordIn.tlast = 1;
            }
            if(currWordIn.tlast == 1) {
              wrpFsmState = WRP_WAIT_META;
            }
          }
          break;
      }

      //-- ALWAYS -----------------------
      if (!siTOE_DSts.empty()) {
        siTOE_DSts.read();  // [TODO] Checking.
      }
    }

      /*****************************************************************************
       * @brief Client connection to remote HOST or FPGA socket (COn).
       *
       * @param[out] soTOE_OpnReq,  open connection request to TOE.
       * @param[in]  siTOE_OpnRep,  open connection reply from TOE.
       * @param[out] soTOE_ClsReq,  close connection request to TOE.
       *
       ******************************************************************************/
      //update myName
      myName  = concat3(THIS_NAME, "/", "COn");

    //only if NTS is ready
    //and if we have valid tables
    if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
    {
      switch (opnFsmState) {

        case OPN_IDLE:
          if (startupDelay > 0) {
            startupDelay--;
            if (!siTOE_OpnRep.empty()) {
              // Drain any potential status data
              siTOE_OpnRep.read(newConn);
              printInfo(myName, "Requesting to close sessionId=%d.\n", newConn.sessionID.to_uint());
              soTOE_ClsReq.write(newConn.sessionID);
            }
          }
          else {
            if(tcp_need_new_connection_request)
            {
              opnFsmState = OPN_REQ;
            } else { 
              opnFsmState = OPN_DONE;
            }
          }
          break;

        case OPN_REQ:
          if (tcp_need_new_connection_request && !soTOE_OpnReq.full()) {
            Ip4Addr remoteIp = getRemoteIpAddrFromTripple(tripple_for_new_connection);
            TcpPort remotePort = getRemotePortFromTripple(tripple_for_new_connection);

            SockAddr    hostSockAddr(remoteIp, remotePort);
            leHostSockAddr.addr = byteSwap32(hostSockAddr.addr);
            leHostSockAddr.port = byteSwap16(hostSockAddr.port);
            soTOE_OpnReq.write(leHostSockAddr);
            if (DEBUG_LEVEL & TRACE_CON) {
              printInfo(myName, "Client is requesting to connect to remote socket:\n");
              printSockAddr(myName, leHostSockAddr);
            }
#ifndef __SYNTHESIS__
            watchDogTimer_pcon = 10;
#else
            watchDogTimer_pcon = NRC_CONNECTION_TIMEOUT;
#endif
            opnFsmState = OPN_REP;
          }
          break;

        case OPN_REP:
          watchDogTimer_pcon--;
          if (!siTOE_OpnRep.empty()) {
            // Read the reply stream
            siTOE_OpnRep.read(newConn);
            if (newConn.success) {
              if (DEBUG_LEVEL & TRACE_CON) {
                printInfo(myName, "Client successfully connected to remote socket:\n");
                printSockAddr(myName, leHostSockAddr);
              }
              addnewTrippleToTable(newConn.sessionID, tripple_for_new_connection);
              opnFsmState = OPN_DONE;
              tcp_need_new_connection_request = false;
              tcp_new_connection_failure = false;
            }
            else {
              printError(myName, "Client failed to connect to remote socket:\n");
              printSockAddr(myName, leHostSockAddr);
              opnFsmState = OPN_DONE;
              tcp_need_new_connection_request = false;
              tcp_new_connection_failure = true;
            }
          }
          else {
            if (watchDogTimer_pcon == 0) {
              if (DEBUG_LEVEL & TRACE_CON) {
                printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
                printSockAddr(myName, leHostSockAddr);
              }
              tcp_need_new_connection_request = false;
              tcp_new_connection_failure = true;
              //the packet will be dropped, so we are done
              opnFsmState = OPN_DONE;
            }

          }
          break;
        case OPN_DONE:
          //if(tcp_need_new_connection_request)
          //{ 
          //No neccisarity to wait...
          opnFsmState = OPN_REQ;
          //}
          break;
      }
    }

    /*****************************************************************************
     * @brief Closes unused sessions (Cls).
     *
     * @param[out] soTOE_ClsReq,  close connection request to TOE.
     *
     ******************************************************************************/
    //update myName
    //myName  = concat3(THIS_NAME, "/", "Cls");

    //only if NTS is ready
    //and if we have valid tables
    if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
    {
      switch (clsFsmState_Tcp) {
        default:
        case CLS_IDLE:
          //we wait until we are activated;
          break;
        case CLS_NEXT:
          SessionId nextToDelete = getAndDeleteNextMarkedRow();
          if(nextToDelete != (SessionId) UNUSED_SESSION_ENTRY_VALUE)
          {
            soTOE_ClsReq.write(nextToDelete);
          } else {
            clsFsmState_Tcp = CLS_IDLE;
          }
          break;
      }

    }
    //===========================================================
    // MRT init

    //copy MRT axi Interface
    //MRT data are after possible config DATA
    //for(int i = 0; i < MAX_MRT_SIZE; i++)
    //{
    //  localMRT[i] = ctrlLink[i + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
    //}
    //for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    //{
    //  config[i] = ctrlLink[i];
    //}


    //DEBUG
    //ctrlLink[3 + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS] = 42;

    //copy routing nodes 0 - 2 FOR DEBUG
    //status[0] = localMRT[0];
    //status[1] = localMRT[1];
    //status[2] = localMRT[2];
    status[NRC_STATUS_MRT_VERSION] = mrt_version_processed;
    status[NRC_STATUS_OPEN_UDP_PORTS] = udp_rx_ports_processed;
    status[NRC_STATUS_OPEN_TCP_PORTS] = tcp_rx_ports_processed;
    status[NRC_STATUS_FMC_PORT_PROCESSED] = (ap_uint<32>) processed_FMC_listen_port;
    status[NRC_STATUS_OWN_RANK] = config[NRC_CONFIG_OWN_RANK];

    //udp
    //status[NRC_STATUS_SEND_STATE] = (ap_uint<32>) fsmStateRX_Udp;
    //status[NRC_STATUS_RECEIVE_STATE] = (ap_uint<32>) fsmStateTXenq_Udp;
    //status[NRC_STATUS_GLOBAL_STATE] = (ap_uint<32>) fsmStateTXdeq_Udp;

    //tcp
    status[NRC_STATUS_SEND_STATE] = (ap_uint<32>) wrpFsmState;
    status[NRC_STATUS_RECEIVE_STATE] = (ap_uint<32>) rdpFsmState;
    status[NRC_STATUS_GLOBAL_STATE] = (ap_uint<32>) opnFsmState;

    status[NRC_STATUS_RX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_RX_cnt;
    status[NRC_STATUS_LAST_RX_NODE_ID] = (ap_uint<32>) (( (ap_uint<32>) last_rx_port) << 16) | ( (ap_uint<32>) last_rx_node_id);
    //status[NRC_STATUS_TX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_TX_cnt;
    status[NRC_STATUS_TX_NODEID_ERROR] = (((ap_uint<32>) tcp_new_connection_failure_cnt) << 16) | ( 0xFFFF & ((ap_uint<16>) node_id_missmatch_TX_cnt));
    status[NRC_STATUS_LAST_TX_NODE_ID] = (ap_uint<32>) (((ap_uint<32>) last_tx_port) << 16) | ((ap_uint<32>) last_tx_node_id);
    //status[NRC_STATUS_TX_PORT_CORRECTIONS] = (((ap_uint<32>) tcp_new_connection_failure_cnt) << 16) | ((ap_uint<16>) port_corrections_TX_cnt);
    status[NRC_STATUS_PACKET_CNT_RX] = (ap_uint<32>) packet_count_RX;
    status[NRC_STATUS_PACKET_CNT_TX] = (ap_uint<32>) packet_count_TX;

    status[NRC_UNAUTHORIZED_ACCESS] = (ap_uint<32>) unauthorized_access_cnt;
    status[NRC_AUTHORIZED_ACCESS] = (ap_uint<32>) authorized_access_cnt;

    //for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
    //{
    //  ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
    //}

    //===========================================================
    //  update status, config, MRT


    if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC && 
        rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
        wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
    { //so we are not in a critical data path

      //TODO: necessary?
      if(tableCopyVariable < NUMBER_CONFIG_WORDS)
      {
        config[tableCopyVariable] = ctrlLink[tableCopyVariable];
      }
      if(tableCopyVariable < MAX_MRT_SIZE)
      {
        localMRT[tableCopyVariable] = ctrlLink[tableCopyVariable + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
      }

      if(tableCopyVariable < NUMBER_STATUS_WORDS)
      {
        ctrlLink[NUMBER_CONFIG_WORDS + tableCopyVariable] = status[tableCopyVariable];
      }

      if(tableCopyVariable >= MAX_MRT_SIZE)
      {
        tableCopyVariable = 0;
        //acknowledge the processed version 
        mrt_version_processed = config[NRC_CONFIG_MRT_VERSION];
      }  else {
        tableCopyVariable++;
      }
    }

}


  /*! \} */
