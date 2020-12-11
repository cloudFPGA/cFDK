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

#include "nal.hpp"

ap_uint<8>   openPortWaitTime = 100;
//ap_uint<8>   udp_lsn_watchDogTimer = 100;
#ifndef __SYNTHESIS__
ap_uint<16>  mmio_stabilize_counter = 1;
#else
ap_uint<16>  mmio_stabilize_counter = NAL_MMIO_STABILIZE_TIME;
#endif

FsmStateUdp fsmStateRX_Udp = FSM_RESET;
FsmStateUdp fsmStateTX_Udp = FSM_RESET;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];


ap_uint<32> udp_rx_ports_processed = 0;
ap_uint<32> udp_rx_ports_to_close = 0;
bool need_udp_port_req = false;
ap_uint<16> new_relative_port_to_req_udp = 0;
ClsFsmStates clsFsmState_Udp = CLS_IDLE;
//ap_uint<16> newRelativePortToClose = 0;
//ap_uint<16> newAbsolutePortToClose = 0;

NetworkMetaStream out_meta_udp = NetworkMetaStream(); //DON'T FORGET to initilize!
//NetworkMetaStream in_meta_udp = NetworkMetaStream(); //ATTENTION: don't forget initilizer...
//
//ap_uint<32> node_id_missmatch_RX_cnt = 0;
//NodeId last_rx_node_id = 0;
//NrcPort last_rx_port = 0;
//ap_uint<32> node_id_missmatch_TX_cnt = 0;
//NodeId last_tx_node_id = 0;
//NrcPort last_tx_port = 0;
//ap_uint<16> port_corrections_TX_cnt = 0;
//ap_uint<32> unauthorized_access_cnt = 0;
//ap_uint<32> authorized_access_cnt = 0;
//ap_uint<32> fmc_tcp_bytes_cnt = 0;
//
//ap_uint<32> packet_count_RX = 0;
//ap_uint<32> packet_count_TX = 0;

//UdpAppDLen udpTX_packet_length = 0;
//UdpAppDLen udpTX_current_packet_length = 0;


ap_uint<64> tripleList[MAX_NAL_SESSIONS];
ap_uint<17> sessionIdList[MAX_NAL_SESSIONS];
ap_uint<1>  usedRows[MAX_NAL_SESSIONS];
ap_uint<1>  rowsToDelete[MAX_NAL_SESSIONS];
bool tables_initalized = false;

ap_uint<1>  privilegedRows[MAX_NAL_SESSIONS];

//NodeId cached_udp_rx_id = 0;
//Ip4Addr cached_udp_rx_ipaddr = 0;

SessionId cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE; //pTcpRrh and pTcpRDp need this
//ap_uint<64> cached_tcp_rx_tripple = UNUSED_TABLE_ENTRY_VALUE;
//SessionId cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
//ap_uint<64> cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;

bool pr_was_done_flag = false;

//FROM TCP

//TcpAppOpnReq     HostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER
//TcpAppOpnRep  newConn;
//ap_uint<32>  watchDogTimer_pcon = 0;
ap_uint<8>   watchDogTimer_plisten = 0;

// Set a startup delay long enough to account for the initialization
// of TOE's listen port table which takes 32,768 cycles after reset.
//  [FIXME - StartupDelay must be replaced by a piSHELL_Reday signal]
//#ifdef __SYNTHESIS_
//ap_uint<16>         startupDelay = 0x8000;
//#else
//ap_uint<16>         startupDelay = 30;
//#endif
OpnFsmStates opnFsmState = OPN_IDLE;
ClsFsmStates clsFsmState_Tcp = CLS_IDLE;

LsnFsmStates lsnFsmState = LSN_IDLE;

RrhFsmStates rrhFsmState = RRH_WAIT_NOTIF;
//TcpAppNotif notif_pRrh;

RdpFsmStates rdpFsmState = RDP_WAIT_META;

WrpFsmStates wrpFsmState = WRP_WAIT_META;

ap_uint<32> tcp_rx_ports_processed = 0;
bool need_tcp_port_req = false;
ap_uint<16> new_relative_port_to_req_tcp = 0;

ap_uint<16> processed_FMC_listen_port = 0;
bool fmc_port_opened = false;

//NetworkMetaStream out_meta_tcp = NetworkMetaStream(); //DON'T FORGET to initilize!
//NetworkMetaStream in_meta_tcp = NetworkMetaStream(); //ATTENTION: don't forget initilizer...
//bool Tcp_RX_metaWritten = false;
ap_uint<64>  tripple_for_new_connection = 0; //pTcpWrp and CON need this
bool tcp_need_new_connection_request = false; //pTcpWrp and CON need this
bool tcp_new_connection_failure = false; //pTcpWrp and CON need this
ap_uint<16> tcp_new_connection_failure_cnt = 0;

//SessionId session_toFMC = 0;
//SessionId session_fromFMC = 0;
bool expect_FMC_response = false; //pTcpRDP and pTcpWRp need this

//NetworkDataLength tcpTX_packet_length = 0;
//NetworkDataLength tcpTX_current_packet_length = 0;

stream<NalEventNotif> internal_event_fifo ("internal_event_fifo");

Ip4Addr getIpFromRank(NodeId rank)
{
#pragma HLS INLINE off
  return localMRT[rank];
  //should return 0 on failure (since MRT is initialized with 0 -> ok)
}

NodeId getOwnRank()
{
#pragma HLS INLINE
  return (NodeId) config[NAL_CONFIG_OWN_RANK];
}

//returns the ZERO-based bit position (so 0 for LSB)
ap_uint<32> getRightmostBitPos(ap_uint<32> num) 
{ 
#pragma HLS INLINE
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
#pragma HLS INLINE off
  for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
  {
//#pragma HLS unroll //factor=8
    if(localMRT[i] == ipAddr)
    {
      return (NodeId) i;
    }
  }
  return UNUSED_SESSION_ENTRY_VALUE;
}


ap_uint<64> newTripple(Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort)
{
#pragma HLS INLINE
  ap_uint<64> new_entry = (((ap_uint<64>) ipRemoteAddres) << 32) | (((ap_uint<32>) tcpRemotePort) << 16) | tcpLocalPort;
  return new_entry;
}


Ip4Addr getRemoteIpAddrFromTripple(ap_uint<64> tripple)
{
#pragma HLS INLINE
  ap_uint<32> ret = ((ap_uint<32>) (tripple >> 32)) & 0xFFFFFFFF;
  return (Ip4Addr) ret;
}


TcpPort getRemotePortFromTripple(ap_uint<64> tripple)
{
#pragma HLS INLINE
  ap_uint<16> ret = ((ap_uint<16>) (tripple >> 16)) & 0xFFFF;
  return (TcpPort) ret;
}


TcpPort getLocalPortFromTripple(ap_uint<64> tripple)
{
#pragma HLS INLINE
  ap_uint<16> ret = (ap_uint<16>) (tripple & 0xFFFF);
  return (TcpPort) ret;
}


ap_uint<64> getTrippleFromSessionId(SessionId sessionID)
{
#pragma HLS inline OFF
  printf("searching for session: %d\n", (int) sessionID);
  uint32_t i = 0;
  for(i = 0; i < MAX_NAL_SESSIONS; i++)
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
#pragma HLS inline OFF
  printf("Searching for tripple: %llu\n", (unsigned long long) tripple);
  uint32_t i = 0;
  for(i = 0; i < MAX_NAL_SESSIONS; i++)
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
  for(i = 0; i < MAX_NAL_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(usedRows[i] == 0)
    {//next free one, tables stay in sync
      sessionIdList[i] = sessionID;
      tripleList[i] = new_entry;
      usedRows[i] = 1;
      privilegedRows[i] = 0;
      printf("stored tripple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) new_entry);
      return;
    }
  }
  //we run out of sessions...TODO
  printf("ERROR: no free space in table left!\n");
}


void addnewSessionToTable(SessionId sessionID, Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort)
{
//#pragma HLS inline off
  ap_uint<64> new_entry = newTripple(ipRemoteAddres, tcpRemotePort, tcpLocalPort);
  addnewTrippleToTable(sessionID, new_entry);
}


void deleteSessionFromTables(SessionId sessionID)
{
#pragma HLS inline off
  printf("try to delete session: %d\n", (int) sessionID);
  for(uint32_t i = 0; i < MAX_NAL_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(sessionIdList[i] == sessionID && usedRows[i] == 1)
    {
      usedRows[i] = 0;
      printf("found and deleting session: %d\n", (int) sessionID);
      //printf("invalidating TCP RX cache\n");
      //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
      return;
    }
  }
  //nothing to delete, nothing to do...
}

void markSessionAsPrivileged(SessionId sessionID)
{
#pragma HLS inline off
  printf("mark session as privileged: %d\n", (int) sessionID);
  for(uint32_t i = 0; i < MAX_NAL_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(sessionIdList[i] == sessionID && usedRows[i] == 1)
    {
      privilegedRows[i] = 1;
      rowsToDelete[i] = 0;
      return;
    }
  }
  //nothing found, nothing to do...
}

void markCurrentRowsAsToDelete_unprivileged()
{
//#pragma HLS inline
  for(uint32_t i = 0; i< MAX_NAL_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(privilegedRows[i] == 1)
    {
      continue;
    } else {
      rowsToDelete[i] = usedRows[i];
    }
  }
}


SessionId getAndDeleteNextMarkedRow()
{
#pragma HLS INLINE off
  for(uint32_t i = 0; i< MAX_NAL_SESSIONS; i++)
  {
//#pragma HLS unroll factor=8
    if(rowsToDelete[i] == 1)
    {
      SessionId ret = sessionIdList[i];
      //sessionIdList[i] = 0x0; //not necessary
      //tripleList[i] = 0x0;
      usedRows[i] = 0;
      rowsToDelete[i] = 0;
      //privilegedRows[i] = 0; //not necessary
      printf("Closing session %d at table row %d.\n",(int) ret, (int) i);
      return ret;
    }
  }
  //Tables are empty
  printf("TCP tables are empty\n");
  return (SessionId) UNUSED_SESSION_ENTRY_VALUE;
}

void eventStatusHousekeeping(
      ap_uint<1>        		*layer_7_enabled,
      ap_uint<1>     			*role_decoupled,
	  ap_uint<32> 				*mrt_version_processed,
      stream<NalEventNotif>  	&internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static ap_uint<32> node_id_missmatch_RX_cnt = 0;
  static NodeId last_rx_node_id = 0;
  static NrcPort last_rx_port = 0;
  static ap_uint<32> node_id_missmatch_TX_cnt = 0;
  static NodeId last_tx_node_id = 0;
  static NrcPort last_tx_port = 0;
  static ap_uint<16> port_corrections_TX_cnt = 0;
  static ap_uint<32> unauthorized_access_cnt = 0;
  static ap_uint<32> authorized_access_cnt = 0;
  static ap_uint<32> fmc_tcp_bytes_cnt = 0;

  static ap_uint<32> packet_count_RX = 0;
  static ap_uint<32> packet_count_TX = 0;

#pragma HLS reset variable=node_id_missmatch_RX_cnt
#pragma HLS reset variable=node_id_missmatch_TX_cnt
#pragma HLS reset variable=port_corrections_TX_cnt
#pragma HLS reset variable=packet_count_RX
#pragma HLS reset variable=packet_count_TX
#pragma HLS reset variable=last_rx_node_id
#pragma HLS reset variable=last_rx_port
#pragma HLS reset variable=last_tx_node_id
#pragma HLS reset variable=last_tx_port
#pragma HLS reset variable=unauthorized_access_cnt
#pragma HLS reset variable=authorized_access_cnt
#pragma HLS reset variable=fmc_tcp_bytes_cnt

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------

   if(*layer_7_enabled == 0 || *role_decoupled == 1 )
    {
    //reset counters
    packet_count_TX = 0x0;
    packet_count_RX = 0x0;
    last_rx_port = 0x0;
    last_rx_node_id = 0x0;
    last_tx_port = 0x0;
    last_tx_node_id = 0x0;
  return;
    }

  for(int i = 0; i<NAL_PARALEL_EVENT_PROCESSING_FACTOR; i++)
{
#pragma HLS unroll
  //remove dependencies (yes, risking race conditions,
  //but they should be very unlikely and are only affecting statistics data)
#pragma HLS dependence variable=node_id_missmatch_RX_cnt  inter false
#pragma HLS dependence variable=node_id_missmatch_TX_cnt  inter false
#pragma HLS dependence variable=port_corrections_TX_cnt   inter false
#pragma HLS dependence variable=packet_count_RX           inter false
#pragma HLS dependence variable=packet_count_TX           inter false
#pragma HLS dependence variable=last_rx_node_id           inter false
#pragma HLS dependence variable=last_rx_port              inter false
#pragma HLS dependence variable=last_tx_node_id           inter false
#pragma HLS dependence variable=last_tx_port              inter false
#pragma HLS dependence variable=unauthorized_access_cnt   inter false
#pragma HLS dependence variable=authorized_access_cnt     inter false
#pragma HLS dependence variable=fmc_tcp_bytes_cnt         inter false

  if(!internal_event_fifo.empty())
  {
    NalEventNotif ne = internal_event_fifo.read();
    switch(ne.type)
    {
    case NID_MISS_RX:
      node_id_missmatch_RX_cnt += ne.update_value;
      break;
    case NID_MISS_TX:
      node_id_missmatch_TX_cnt += ne.update_value;
      break;
    case PCOR_TX:
      port_corrections_TX_cnt += ne.update_value;
      break;
    case TCP_CON_FAIL:
      tcp_new_connection_failure_cnt += ne.update_value;
      break;
    case LAST_RX_PORT:
      last_rx_port = ne.update_value;
      break;
    case LAST_RX_NID:
      last_rx_node_id = ne.update_value;
      break;
    case LAST_TX_PORT:
      last_tx_port = ne.update_value;
      break;
    case LAST_TX_NID:
      last_tx_node_id = ne.update_value;
      break;
    case PACKET_RX:
      packet_count_RX += ne.update_value;
      break;
    case PACKET_TX:
      packet_count_TX += ne.update_value;
      break;
    case UNAUTH_ACCESS:
      unauthorized_access_cnt += ne.update_value;
      break;
    case AUTH_ACCESS:
      authorized_access_cnt += ne.update_value;
      break;
    case FMC_TCP_BYTES:
      fmc_tcp_bytes_cnt += ne.update_value;
      break;
    default:
      printf("[ERROR] Internal Event Processing received invalid event %d with update value %d\n", \
          (int) ne.type, (int) ne.update_value);
      break;
    }
  } else {
    break;
  }
}


    //update status entries
    status[NAL_STATUS_MRT_VERSION] = *mrt_version_processed;
    status[NAL_STATUS_OPEN_UDP_PORTS] = udp_rx_ports_processed;
    status[NAL_STATUS_OPEN_TCP_PORTS] = tcp_rx_ports_processed;
    status[NAL_STATUS_FMC_PORT_PROCESSED] = (ap_uint<32>) processed_FMC_listen_port;
    status[NAL_STATUS_OWN_RANK] = config[NAL_CONFIG_OWN_RANK];

    //udp
    //status[NAL_STATUS_SEND_STATE] = (ap_uint<32>) fsmStateRX_Udp;
    //status[NAL_STATUS_RECEIVE_STATE] = (ap_uint<32>) fsmStateTXenq_Udp;
    //status[NAL_STATUS_GLOBAL_STATE] = (ap_uint<32>) fsmStateTXdeq_Udp;

    //tcp
    status[NAL_STATUS_SEND_STATE] = (ap_uint<32>) wrpFsmState;
    status[NAL_STATUS_RECEIVE_STATE] = (ap_uint<32>) rdpFsmState;
    //status[NAL_STATUS_GLOBAL_STATE] = (ap_uint<32>) opnFsmState;

    status[NAL_STATUS_GLOBAL_STATE] = fmc_tcp_bytes_cnt;

    //status[NAL_STATUS_RX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_RX_cnt;
    status[NAL_STATUS_RX_NODEID_ERROR] = (((ap_uint<32>) port_corrections_TX_cnt) << 16) | ( 0xFFFF & ((ap_uint<16>) node_id_missmatch_RX_cnt));
    status[NAL_STATUS_LAST_RX_NODE_ID] = (ap_uint<32>) (( (ap_uint<32>) last_rx_port) << 16) | ( (ap_uint<32>) last_rx_node_id);
    //status[NAL_STATUS_TX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_TX_cnt;
    status[NAL_STATUS_TX_NODEID_ERROR] = (((ap_uint<32>) tcp_new_connection_failure_cnt) << 16) | ( 0xFFFF & ((ap_uint<16>) node_id_missmatch_TX_cnt));
    status[NAL_STATUS_LAST_TX_NODE_ID] = (ap_uint<32>) (((ap_uint<32>) last_tx_port) << 16) | ((ap_uint<32>) last_tx_node_id);
    //status[NAL_STATUS_TX_PORT_CORRECTIONS] = (((ap_uint<32>) tcp_new_connection_failure_cnt) << 16) | ((ap_uint<16>) port_corrections_TX_cnt);
    status[NAL_STATUS_PACKET_CNT_RX] = (ap_uint<32>) packet_count_RX;
    status[NAL_STATUS_PACKET_CNT_TX] = (ap_uint<32>) packet_count_TX;

    status[NAL_UNAUTHORIZED_ACCESS] = (ap_uint<32>) unauthorized_access_cnt;
    status[NAL_AUTHORIZED_ACCESS] = (ap_uint<32>) authorized_access_cnt;
}

void axi4liteProcessing(
		ap_uint<32> 	ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
		ap_uint<32> 	*mrt_version_processed
		)
{

	static uint16_t tableCopyVariable = 0;

#pragma HLS reset variable=tableCopyVariable

    //TODO: necessary? Or does this AXI4Lite anyways "in the background"?
    //or do we need to copy it explicetly, but could do this also every ~2 seconds?
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
      ap_uint<32> new_mrt_version = config[NAL_CONFIG_MRT_VERSION];
     // if(new_mrt_version > mrt_version_processed)
     // {
        //invalidate cache
        //cached_udp_rx_ipaddr = 0;
        //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
        //cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
     //   detected_cache_invalidation = true;
      //moved to outer process
      //}
      *mrt_version_processed = new_mrt_version;
    }  else {
      tableCopyVariable++;
    }
}


/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 *
 *****************************************************************************/
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
    stream<NetworkWord>             &siUdp_data,
    stream<NetworkWord>             &soUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<NetworkMetaStream>   &soUdp_meta,

    // -- ROLE TCP connection
    ap_uint<32>                 *pi_tcp_rx_ports,
    stream<NetworkWord>          &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<NetworkWord>          &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,

    // -- FMC TCP connection
    stream<TcpAppData>          &siFMC_Tcp_data,
    stream<TcpAppMeta>          &siFMC_Tcp_SessId,
    ap_uint<1>                  *piFMC_Tcp_data_FIFO_prog_full,
    stream<TcpAppData>          &soFMC_Tcp_data,
    ap_uint<1>                  *piFMC_Tcp_sessid_FIFO_prog_full,
    stream<TcpAppMeta>          &soFMC_Tcp_SessId,

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
    stream<TcpAppData>     &soTOE_Data,
    stream<TcpAppMeta>     &soTOE_SessId,
    //stream<AppWrSts>    &siTOE_DSts,
    //-- TOE / Open Interfaces
    stream<TcpAppOpnReq>      &soTOE_OpnReq,
    stream<TcpAppOpnRep>   &siTOE_OpnRep,
    //-- TOE / Close Interfaces
    stream<TcpAppClsReq>   &soTOE_ClsReq
  )
{

// ----- directives for AXI buses (AXI4 stream, AXI4 Lite) -----
#ifdef USE_DEPRECATED_DIRECTIVES
  #pragma HLS RESOURCE core=AXI4LiteS variable=ctrlLink metadata="-bus_bundle piFMC_NAL_ctrlLink_AXI"
  
  #pragma HLS RESOURCE core=AXI4Stream variable=siUdp_data    metadata="-bus_bundle siUdp_data"
  #pragma HLS RESOURCE core=AXI4Stream variable=soUdp_data    metadata="-bus_bundle soUdp_data"

  #pragma HLS RESOURCE core=AXI4Stream variable=siUdp_meta    metadata="-bus_bundle siUdp_meta"
  #pragma HLS DATA_PACK          variable=siUdp_meta
  #pragma HLS RESOURCE core=AXI4Stream variable=soUdp_meta    metadata="-bus_bundle soUdp_meta"
  #pragma HLS DATA_PACK          variable=soUdp_meta

  #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_LsnReq  metadata="-bus_bundle soUOE_LsnReq"
  #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_LsnRep  metadata="-bus_bundle siUOE_LsnRep"
  #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_ClsReq  metadata="-bus_bundle soUOE_ClsReq"
  #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_ClsRep  metadata="-bus_bundle siUOE_ClsRep"

  #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_Data    metadata="-bus_bundle siUOE_Data"
  #pragma HLS RESOURCE core=AXI4Stream variable=siUOE_Meta    metadata="-bus_bundle siUOE_Meta"
  #pragma HLS DATA_PACK                variable=siUOE_Meta

  #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_Data    metadata="-bus_bundle soUOE_Data"
  #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_Meta    metadata="-bus_bundle soUOE_Meta"
  #pragma HLS DATA_PACK                variable=soUOE_Meta
  #pragma HLS RESOURCE core=AXI4Stream variable=soUOE_DLen    metadata="-bus_bundle soUOE_DLen"

  #pragma HLS RESOURCE core=AXI4Stream variable=siTcp_data    metadata="-bus_bundle siTcp_data"
  #pragma HLS RESOURCE core=AXI4Stream variable=soTcp_data    metadata="-bus_bundle soTcp_data"
  #pragma HLS RESOURCE core=AXI4Stream variable=siTcp_meta    metadata="-bus_bundle siTcp_meta"
  #pragma HLS DATA_PACK          variable=siTcp_meta
  #pragma HLS RESOURCE core=AXI4Stream variable=soTcp_meta    metadata="-bus_bundle soTcp_meta"
  #pragma HLS DATA_PACK          variable=soTcp_meta

  #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_Notif   metadata="-bus_bundle siTOE_Notif"
  #pragma HLS DATA_PACK                variable=siTOE_Notif
  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_DReq    metadata="-bus_bundle soTOE_DReq"
  #pragma HLS DATA_PACK                variable=soTOE_DReq
  #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_Data    metadata="-bus_bundle siTOE_Data"
  #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_SessId  metadata="-bus_bundle siTOE_SessId"
  
  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_LsnReq  metadata="-bus_bundle soTOE_LsnReq"
  #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_LsnRep  metadata="-bus_bundle siTOE_LsnRep"

  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_Data    metadata="-bus_bundle soTOE_Data"
  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_SessId  metadata="-bus_bundle soTOE_SessId"
  //#pragma HLS RESOURCE core=AXI4Stream variable=siTOE_DSts  metadata="-bus_bundle "

  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_OpnReq  metadata="-bus_bundle soTOE_OpnReq"
  #pragma HLS DATA_PACK                variable=soTOE_OpnReq
  #pragma HLS RESOURCE core=AXI4Stream variable=siTOE_OpnRep  metadata="-bus_bundle siTOE_OpnRep"
  #pragma HLS DATA_PACK                variable=siTOE_OpnRep
  
  #pragma HLS RESOURCE core=AXI4Stream variable=soTOE_ClsReq  metadata="-bus_bundle soTOE_ClsReq"

#else
  #pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piFMC_NAL_ctrlLink_AXI

  #pragma HLS INTERFACE axis register both port=siUdp_data
  #pragma HLS INTERFACE axis register both port=soUdp_data

  #pragma HLS INTERFACE axis register both port=siUdp_meta
  #pragma HLS INTERFACE axis register both port=soUdp_meta

  #pragma HLS INTERFACE axis register both port=soUOE_LsnReq
  #pragma HLS INTERFACE axis register both port=siUOE_LsnRep
  #pragma HLS INTERFACE axis register both port=soUOE_ClsReq
  #pragma HLS INTERFACE axis register both port=siUOE_ClsRep

  #pragma HLS INTERFACE axis register both port=siUOE_Data
  #pragma HLS INTERFACE axis register both port=siUOE_Meta
  #pragma HLS DATA_PACK                variable=siUOE_Meta

  #pragma HLS INTERFACE axis register both port=soUOE_Data
  #pragma HLS INTERFACE axis register both port=soUOE_Meta
  #pragma HLS DATA_PACK                variable=soUOE_Meta
  #pragma HLS INTERFACE axis register both port=soUOE_DLen

  #pragma HLS INTERFACE axis register both port=siTcp_data
  #pragma HLS INTERFACE axis register both port=soTcp_data
  #pragma HLS INTERFACE axis register both port=siTcp_meta
  #pragma HLS INTERFACE axis register both port=soTcp_meta

  #pragma HLS INTERFACE axis register both port=siTOE_Notif
  #pragma HLS DATA_PACK                variable=siTOE_Notif
  #pragma HLS INTERFACE axis register both port=soTOE_DReq
  #pragma HLS DATA_PACK                variable=soTOE_DReq
  #pragma HLS INTERFACE axis register both port=siTOE_Data
  #pragma HLS INTERFACE axis register both port=siTOE_SessId
  
  #pragma HLS INTERFACE axis register both port=soTOE_LsnReq
  #pragma HLS INTERFACE axis register both port=siTOE_LsnRep
  
  #pragma HLS INTERFACE axis register both port=soTOE_Data
  #pragma HLS INTERFACE axis register both port=soTOE_SessId
  //#pragma HLS INTERFACE axis register both port=siTOE_DSts
  
  #pragma HLS INTERFACE axis register both port=soTOE_OpnReq
  #pragma HLS DATA_PACK                variable=soTOE_OpnReq
  #pragma HLS INTERFACE axis register both port=siTOE_OpnRep
  #pragma HLS DATA_PACK                variable=siTOE_OpnRep
  
  #pragma HLS INTERFACE axis register both port=soTOE_ClsReq

#endif

// ----- common directives -----

#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE ap_vld register port=layer_4_enabled name=piLayer4enabled
#pragma HLS INTERFACE ap_vld register port=layer_7_enabled name=piLayer7enabled
#pragma HLS INTERFACE ap_vld register port=role_decoupled  name=piRoleDecoup_active
#pragma HLS INTERFACE ap_vld register port=piNTS_ready name=piNTS_ready

#pragma HLS INTERFACE ap_vld register port=myIpAddress name=piMyIpAddress
#pragma HLS INTERFACE ap_vld register port=pi_udp_rx_ports name=piROL_Udp_Rx_ports
#pragma HLS INTERFACE ap_vld register port=piMMIO_FmcLsnPort name=piMMIO_FmcLsnPort
#pragma HLS INTERFACE ap_vld register port=piMMIO_CfrmIp4Addr name=piMMIO_CfrmIp4Addr

#pragma HLS INTERFACE ap_vld register port=pi_tcp_rx_ports name=piROL_Tcp_Rx_ports

#pragma HLS INTERFACE ap_fifo port=siFMC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=soFMC_Tcp_data
#pragma HLS INTERFACE ap_fifo port=siFMC_Tcp_SessId
#pragma HLS INTERFACE ap_fifo port=soFMC_Tcp_SessId
#pragma HLS INTERFACE ap_vld register port=piFMC_Tcp_data_FIFO_prog_full name=piFMC_Tcp_data_FIFO_prog_full
#pragma HLS INTERFACE ap_vld register port=piFMC_Tcp_sessid_FIFO_prog_full name=piFMC_Tcp_sessid_FIFO_prog_full

  //TODO: add internal streams

#pragma HLS DATAFLOW
//#pragma HLS PIPELINE II=1 //FIXME

//=================================================================================================
// Variable Pragmas

#pragma HLS ARRAY_PARTITION variable=tripleList cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=sessionIdList cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=usedRows cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=privilegedRows cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=rowsToDelete cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=localMRT complete dim=1

#pragma HLS ARRAY_PARTITION variable=status cyclic factor=4 dim=1
#pragma HLS STREAM variable=internal_event_fifo depth=128

	  //===========================================================
	  //  core wide STATIC variables

	static ap_uint<32> mrt_version_processed = 0;
	static ap_uint<32> mrt_version_old = 0; //no reset needed

//=================================================================================================
// Reset global variables

#pragma HLS reset variable=fsmStateRX_Udp
#pragma HLS reset variable=fsmStateTX_Udp
//#pragma HLS reset variable=openPortWaitTime
#pragma HLS reset variable=mmio_stabilize_counter
//#pragma HLS reset variable=udp_lsn_watchDogTimer
#pragma HLS reset variable=mrt_version_processed
#pragma HLS reset variable=udp_rx_ports_processed
#pragma HLS reset variable=udp_rx_ports_to_close
#pragma HLS reset variable=need_udp_port_req
#pragma HLS reset variable=new_relative_port_to_req_udp
//#pragma HLS reset variable=clsFsmState_Udp
//#pragma HLS reset variable=newRelativePortToClose
//#pragma HLS reset variable=newAbsolutePortToClose
//#pragma HLS reset variable=node_id_missmatch_RX_cnt
//#pragma HLS reset variable=node_id_missmatch_TX_cnt
//#pragma HLS reset variable=port_corrections_TX_cnt
//#pragma HLS reset variable=packet_count_RX
//#pragma HLS reset variable=packet_count_TX
//#pragma HLS reset variable=last_rx_node_id
//#pragma HLS reset variable=last_rx_port
//#pragma HLS reset variable=last_tx_node_id
//#pragma HLS reset variable=last_tx_port
////#pragma HLS reset variable=out_meta_udp
////#pragma HLS reset variable=in_meta_udp
////#pragma HLS reset variable=udpTX_packet_length
////#pragma HLS reset variable=udpTX_current_packet_length
//#pragma HLS reset variable=unauthorized_access_cnt
//#pragma HLS reset variable=authorized_access_cnt
//#pragma HLS reset variable=fmc_tcp_bytes_cnt

#pragma HLS reset variable=startupDelay
#pragma HLS reset variable=opnFsmState
#pragma HLS reset variable=clsFsmState_Tcp
#pragma HLS reset variable=lsnFsmState
//#pragma HLS reset variable=rrhFsmState
#pragma HLS reset variable=rdpFsmState
#pragma HLS reset variable=wrpFsmState
#pragma HLS reset variable=tcp_rx_ports_processed
#pragma HLS reset variable=need_tcp_port_req
#pragma HLS reset variable=new_relative_port_to_req_tcp
#pragma HLS reset variable=processed_FMC_listen_port
#pragma HLS reset variable=fmc_port_opened
#pragma HLS reset variable=tables_initalized
//#pragma HLS reset variable=out_meta_tcp
//#pragma HLS reset variable=in_meta_tcp
//#pragma HLS reset variable=session_toFMC
//#pragma HLS reset variable=session_fromFMC
//#pragma HLS reset variable=Tcp_RX_metaWritten
//#pragma HLS reset variable=tcpTX_packet_length
//#pragma HLS reset variable=tcpTX_current_packet_length
#pragma HLS reset variable=tripple_for_new_connection
#pragma HLS reset variable=tcp_need_new_connection_request
#pragma HLS reset variable=tcp_new_connection_failure
#pragma HLS reset variable=tcp_new_connection_failure_cnt

#pragma HLS reset variable=expect_FMC_response

//#pragma HLS reset variable=cached_udp_rx_id
//#pragma HLS reset variable=cached_udp_rx_ipaddr
//#pragma HLS reset variable=cached_tcp_rx_session_id
//#pragma HLS reset variable=cached_tcp_rx_tripple
//#pragma HLS reset variable=cached_tcp_tx_session_id
//#pragma HLS reset variable=cached_tcp_tx_tripple
#pragma HLS reset variable=pr_was_done_flag




  //===========================================================
  //  core wide variables (for one iteration)

  ap_uint<32> ipAddrBE = *myIpAddress;
  bool nts_ready_and_enabled = (*piNTS_ready == 1 && *layer_4_enabled == 1);
  bool detected_cache_invalidation = false;
  bool start_udp_cls_fsm = false;
  bool start_tcp_cls_fsm = false;


  //===========================================================
  // restore saved states
  if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC &&
      rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
      wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
  { //so we are not in a critical data path

    // > to avoid loop at 0
    if(config[NAL_CONFIG_SAVED_FMC_PORTS] > processed_FMC_listen_port)
    {
      processed_FMC_listen_port = (ap_uint<16>) config[NAL_CONFIG_SAVED_FMC_PORTS];
    }

    if(*layer_7_enabled == 1 && *role_decoupled == 0)
    { // looks like only we were reset
      // since the user cannot close ports (up to now), the > should work...
      if(config[NAL_CONFIG_SAVED_UDP_PORTS] > udp_rx_ports_processed)
      {
        udp_rx_ports_processed = config[NAL_CONFIG_SAVED_UDP_PORTS];
      }

      if(config[NAL_CONFIG_SAVED_TCP_PORTS] > tcp_rx_ports_processed)
      {
        tcp_rx_ports_processed = config[NAL_CONFIG_SAVED_TCP_PORTS];
      }
    }

  }

  //===========================================================
  // check for resets

  if(mrt_version_old != mrt_version_processed)
  {
	  mrt_version_old = mrt_version_processed;
	  detected_cache_invalidation = true;
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
    //invalidate cache
    //cached_udp_rx_ipaddr = 0;
    //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    //cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    detected_cache_invalidation = true;
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
        //clsFsmState_Udp = CLS_NEXT;
        start_udp_cls_fsm = true;
      }

      if(tcp_rx_ports_processed > 0)
      {
        //mark all TCP ports as to be deleted
        markCurrentRowsAsToDelete_unprivileged();
        if( *role_decoupled == 0 )
        {//start closing FSM TCP
          //clsFsmState_Tcp = CLS_NEXT;
          start_tcp_cls_fsm = true;
        } else {
          //FMC is using TCP!
          pr_was_done_flag = true;
        }
      }
    }
    //in all cases
    udp_rx_ports_processed = 0x0;
    tcp_rx_ports_processed = 0x0;
    if( *role_decoupled == 0)
    { //invalidate cache
      //cached_udp_rx_ipaddr = 0;
      //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
      //cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
      detected_cache_invalidation = true;
    } else {
      //FMC is using TCP!
      pr_was_done_flag = true;
    }
  }
  if(pr_was_done_flag && *role_decoupled == 0)
  {//so, after the PR was done
    //invalidate cache
    //cached_udp_rx_ipaddr = 0;
    //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    //cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    //start closing FSM TCP
    //ports have been marked earlier
    //clsFsmState_Tcp = CLS_NEXT;
    start_tcp_cls_fsm = true;
    //FSM will wait until RDP and WRP are done
    pr_was_done_flag = false;
  }
  //===========================================================
  // MRT init

  //TODO: some consistency check for tables? (e.g. every IP address only once...)

  if (!tables_initalized)
  {
    printf("init tables...\n");
    for(int i = 0; i<MAX_NAL_SESSIONS; i++)
    {
      //#pragma HLS unroll
      sessionIdList[i] = 0;
      tripleList[i] = 0;
      usedRows[i]  =  0;
      rowsToDelete[i] = 0;
      privilegedRows[i] = 0;
    }
    udp_rx_ports_to_close = 0x0;
    
    tables_initalized = true;
    //printf("Table layout:\n");
    //for(int i = 0; i<MAX_NAL_SESSIONS; i++)
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
        mmio_stabilize_counter = NAL_MMIO_STABILIZE_TIME;
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
  //TODO: remove unused global variables
  //TODO: add disable signal? (NTS_ready, layer4 enabled)
  pUdpTX(siUdp_data, siUdp_meta, soUOE_Data, soUOE_Meta, soUOE_DLen, &ipAddrBE, &nts_ready_and_enabled, internal_event_fifo);

  //=================================================================================================
  // RX UDP
  //TODO: remove unused global variables
  //TODO: add disable signal? (NTS_ready, layer4 enabled)
  //TODO: add cache invalidate mechanism
  pUdpRx(soUOE_LsnReq, siUOE_LsnRep, soUdp_data, soUdp_meta, siUOE_Data, siUOE_Meta, &need_udp_port_req, \
      &new_relative_port_to_req_udp, &udp_rx_ports_processed, &nts_ready_and_enabled, &detected_cache_invalidation, internal_event_fifo);

  //=================================================================================================
  // UDP Port Close

  pUdpCls(soUOE_ClsReq, siUOE_ClsRep, &udp_rx_ports_to_close, &start_udp_cls_fsm, &nts_ready_and_enabled);

    //=================================================================================================
    // TCP pListen
   pTcpLsn(piMMIO_FmcLsnPort, soTOE_LsnReq, siTOE_LsnRep, &tcp_rx_ports_processed, &need_tcp_port_req, \
       &new_relative_port_to_req_tcp, &processed_FMC_listen_port, &nts_ready_and_enabled);

   //=================================================================================================
   // TCP Read Request Handler

   //TODO: remove unused global variables
     //TODO: add disable signal? (NTS_ready, layer4 enabled)
     //TODO: add cache invalidate mechanism
   pTcpRRh(siTOE_Notif, soTOE_DReq, piFMC_Tcp_data_FIFO_prog_full, piFMC_Tcp_sessid_FIFO_prog_full, &cached_tcp_rx_session_id, &nts_ready_and_enabled);

    //=================================================================================================
    // TCP Read Path
    pTcpRDp(siTOE_Data, siTOE_SessId, soFMC_Tcp_data, soFMC_Tcp_SessId, soTcp_data, soTcp_meta, piMMIO_CfrmIp4Addr, \
        &processed_FMC_listen_port, layer_7_enabled, role_decoupled, &cached_tcp_rx_session_id, &expect_FMC_response, \
      &nts_ready_and_enabled, &detected_cache_invalidation, internal_event_fifo);

    //=================================================================================================
    // TCP Write Path
    pTcpWRp(siFMC_Tcp_data, siFMC_Tcp_SessId, siTcp_data, siTcp_meta, soTOE_Data, soTOE_SessId, &expect_FMC_response, \
        &tripple_for_new_connection, &tcp_need_new_connection_request, &tcp_new_connection_failure, &nts_ready_and_enabled, \
      &detected_cache_invalidation, internal_event_fifo);

    //=================================================================================================
    // TCP start remote connection
    pTcpCOn(soTOE_OpnReq, siTOE_OpnRep, &tripple_for_new_connection, &tcp_need_new_connection_request, &tcp_new_connection_failure,\
        &nts_ready_and_enabled);

    //=================================================================================================
    // TCP connection close
    pTcpCls(soTOE_ClsReq, &start_tcp_cls_fsm, &nts_ready_and_enabled);

    //===========================================================
    //  update status, config, MRT

    eventStatusHousekeeping(layer_7_enabled, role_decoupled, &mrt_version_old, internal_event_fifo);

    axi4liteProcessing(ctrlLink, &mrt_version_processed);

//    if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC &&
//        rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
//        wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
//    { //so we are not in a critical data path
//
//    }

}


  /*! \} */

