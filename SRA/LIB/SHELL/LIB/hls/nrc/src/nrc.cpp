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

#include "nrc.hpp"

stream<UdpPLen>        sPLen_Udp     ("sPLen_Udp");
stream<UdpWord>        sFifo_Udp_Data("sFifo_Udp_Data");
stream<NetworkMetaStream>  sFifo_Udp_Meta("sFifo_Udp_Meta");

ap_uint<8>   openPortWaitTime = 10;
bool Udp_RX_metaWritten = false;

FsmStateUdp fsmStateRX_Udp = FSM_RESET;

UdpPLen    pldLen_Udp = 0;
FsmStateUdp fsmStateTXenq_Udp = FSM_RESET;

FsmStateUdp fsmStateTXdeq_Udp = FSM_RESET;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

ap_uint<32> udp_rx_ports_processed = 0;
bool need_udp_port_req = false;
ap_uint<16> new_relative_port_to_req_udp = 0;

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

NetworkDataLength udpTX_packet_length = 0;
NetworkDataLength udpTX_current_packet_length = 0;


ap_uint<64> tripleList[MAX_NRC_SESSIONS];
ap_uint<17> sessionIdList[MAX_NRC_SESSIONS];
ap_uint<1>  usedRows[MAX_NRC_SESSIONS];
bool tables_initalized = false;
#define UNUSED_TABLE_ENTRY_VALUE 0x111000


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
ap_uint<12>  watchDogTimer_pcon = 0;
ap_uint<8>   watchDogTimer_plisten = 0;

// Set a startup delay long enough to account for the initialization
// of TOE's listen port table which takes 32,768 cycles after reset.
//  [FIXME - StartupDelay must be replaced by a piSHELL_Reday signal]
#ifdef __SYNTHESIS_
ap_uint<16>         startupDelay = 0x8000;
#else 
ap_uint<16>         startupDelay = 30;
#endif
OpnFsmStates opnFsmState=OPN_IDLE;

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

NetworkDataLength tcpTX_packet_length = 0;
NetworkDataLength tcpTX_current_packet_length = 0;

/*****************************************************************************
 * @brief Update the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_role_if
 *
 * @param[in]     axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen_Udp, a pointer to the payload length of the corresponding
 *                     AXI4-Stream.
 * @return Nothing.
 ******************************************************************************/
void updatePayloadLength(UdpWord *axisChunk, UdpPLen *pldLen_Udp) {
  if (axisChunk->tlast) {
    int bytCnt = 0;
    for (int i = 0; i < 8; i++) {
      if (axisChunk->tkeep.bit(i) == 1) {
        bytCnt++;
      }
    }
    *pldLen_Udp += bytCnt;
  } else
    *pldLen_Udp += 8;
}

//returns the ZERO-based bit position (so 0 for LSB)
ap_uint<32> getRightmostBitPos(ap_uint<32> num) 
{ 
  //return (ap_uint<32>) log2((ap_fixed<32,2>) (num & -num));
  ap_uint<32> pos = 0; 
  for (int i = 0; i < 32; i++) { 
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
  for(NodeId i = 0; i< MAX_MRT_SIZE; i++)
  {
    if(localMRT[i] == ipAddr)
    {
      return i;
    }
  }
  node_id_missmatch_RX_cnt++;
  return 0xFFFF;
}

ap_uint<64> newTripple(Ip4Addr ipRemoteAddres, TcpPort tcpRemotePort, TcpPort tcpLocalPort)
{
      ap_uint<64> new_entry = (((ap_uint<64>) ipRemoteAddres) << 32) | (((ap_uint<32>) tcpRemotePort) << 16) | tcpLocalPort;
      return new_entry;
}

Ip4Addr getRemoteIpAddrFromTripple(ap_uint<64> tripple)
{
  ap_uint<32> ret = ((ap_uint<32>) (tripple >> 32)) & 0xFFFFFFFF;
  return (Ip4Addr) ret;
}

TcpPort getRemotePortFromTripple(ap_uint<64> tripple)
{
  ap_uint<16> ret = ((ap_uint<16>) (tripple >> 16)) & 0xFFFF;
  return (TcpPort) ret;
}

TcpPort getLocalPortFromTripple(ap_uint<64> tripple)
{
  ap_uint<16> ret = (ap_uint<16>) (tripple & 0xFFFF);
  return (TcpPort) ret;
}

ap_uint<64> getTrippleFromSessionId(SessionId sessionID)
{
  printf("searching for session: %d\n", (int) sessionID);
  uint32_t i = 0;
  for(i = 0; i < MAX_NRC_SESSIONS; i++)
  {
    if(sessionIdList[i] == sessionID)
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
  printf("Searching for tripple: %llu\n", (unsigned long long) tripple);
  uint32_t i = 0;
  for(i = 0; i < MAX_NRC_SESSIONS; i++)
  {
    if(tripleList[i] == tripple)
    {
      return sessionIdList[i];
    }
  }
  //there is (not yet) a connection
  printf("ERROR: unkown tripple\n");
  return (SessionId) UNUSED_TABLE_ENTRY_VALUE;
}


void addnewSessionToTableWithTripple(SessionId sessionID, ap_uint<64> new_entry)
{
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
    //if(sessionIdList[i] == UNUSED_TABLE_ENTRY_VALUE)
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
    ap_uint<64> new_entry = newTripple(ipRemoteAddres, tcpRemotePort, tcpLocalPort);
    addnewSessionToTableWithTripple(sessionID, new_entry);
}


/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 *
 *****************************************************************************/
void nrc_main(
    // ----- link to FMC -----
    ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
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

    //-- UDMX / This / Open-Port Interfaces
    stream<AxisAck>     &siUDMX_This_OpnAck,
    stream<UdpPort>     &soTHIS_Udmx_OpnReq,

    //-- UDMX / This / Data & MetaData Interfaces
    stream<UdpWord>     &siUDMX_This_Data,
    stream<UdpMeta>     &siUDMX_This_Meta,
    stream<UdpWord>     &soTHIS_Udmx_Data,
    stream<UdpMeta>     &soTHIS_Udmx_Meta,
    stream<UdpPLen>     &soTHIS_Udmx_PLen, 

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

#pragma HLS INTERFACE ap_vld register port=piNTS_ready name=piNTS_ready

#pragma HLS INTERFACE axis register both port=siUdp_data
#pragma HLS INTERFACE axis register both port=soUdp_data

#pragma HLS INTERFACE axis register both port=siUDMX_This_OpnAck
#pragma HLS INTERFACE axis register both port=soTHIS_Udmx_OpnReq

#pragma HLS INTERFACE axis register both port=siUDMX_This_Data
#pragma HLS INTERFACE axis register both port=siUDMX_This_Meta
#pragma HLS DATA_PACK                variable=siUDMX_This_Meta instance=siUDMX_This_Meta

#pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Data
#pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Meta
#pragma HLS DATA_PACK                variable=soTHIS_Udmx_Meta instance=soTHIS_Udmx_Meta
#pragma HLS INTERFACE axis register both port=soTHIS_Udmx_PLen

#pragma HLS INTERFACE axis register both port=siUdp_meta
#pragma HLS INTERFACE axis register both port=soUdp_meta

#pragma HLS INTERFACE ap_vld register port=myIpAddress name=piMyIpAddress
#pragma HLS INTERFACE ap_vld register port=pi_udp_rx_ports name=piROL_Udp_Rx_ports
#pragma HLS INTERFACE ap_vld register port=piMMIO_FmcLsnPort name=piMMIO_FmcLsnPort
#pragma HLS INTERFACE ap_vld register port=piMMIO_CfrmIp4Addr name=piMMIO_CfrmIp4Addr

#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piFMC_NRC_ctrlLink_AXI
#pragma HLS INTERFACE s_axilite port=return bundle=piFMC_NRC_ctrlLink_AXI

#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE ap_vld register port=pi_tcp_rx_ports name=piROL_Tcp_Rx_ports

#pragma HLS INTERFACE axis register both port=siFMC_Tcp_data
#pragma HLS INTERFACE axis register both port=soFMC_Tcp_data
#pragma HLS INTERFACE axis register both port=siFMC_Tcp_SessId
#pragma HLS INTERFACE axis register both port=soFMC_Tcp_SessId


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
  //#pragma HLS DATAFLOW interval=1
  //#pragma HLS PIPELINE II=1 TODO/FIXME: is this necessary??
#pragma HLS STREAM variable=sPLen_Udp depth=1
#pragma HLS STREAM variable=sFifo_Udp_Meta depth=1
#pragma HLS STREAM variable=sFifo_Udp_Data depth=2048    // Must be able to contain MTU


  //=================================================================================================
  // Reset global variables 

#pragma HLS reset variable=Udp_RX_metaWritten
#pragma HLS reset variable=fsmStateRX_Udp
#pragma HLS reset variable=fsmStateTXenq_Udp
#pragma HLS reset variable=fsmStateTXdeq_Udp
#pragma HLS reset variable=pldLen_Udp
#pragma HLS reset variable=openPortWaitTime
#pragma HLS reset variable=udp_rx_ports_processed
#pragma HLS reset variable=need_udp_port_req
#pragma HLS reset variable=new_relative_port_to_req_udp
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

  //DO NOT reset MRT and config. This should be done explicitly by the FMC
#pragma HLS reset variable=localMRT off
#pragma HLS reset variable=config off
//Disable ARRAY reset...just to be sure, and the init will be reset anyhow
#pragma HLS reset variable=tripleList off
#pragma HLS reset variable=sessionIdList off
#pragma HLS reset variable=usedRows off
  //#pragma HLS reset variable=status --> I think to reset an array is hard, it is also uninitalized in C itself...
  // the status array is anyhow written every IP core iteration and the inputs are reset --> so not necessary

#pragma HLS reset variable=startupDelay
#pragma HLS reset variable=opnFsmState
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


  //===========================================================
  // MRT

  //copy MRT axi Interface
  //MRT data are after possible config DATA
  for(int i = 0; i < MAX_MRT_SIZE; i++)
  {
    localMRT[i] = ctrlLink[i + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
  }
  for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
  {
    config[i] = ctrlLink[i];
  }

  //DEBUG
  //ctrlLink[3 + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS] = 42;

  //copy routing nodes 0 - 2 FOR DEBUG
  status[0] = localMRT[0];
  status[1] = localMRT[1];
  status[2] = localMRT[2];

  status[NRC_STATUS_SEND_STATE] = (ap_uint<32>) fsmStateRX_Udp;
  status[NRC_STATUS_RECEIVE_STATE] = (ap_uint<32>) fsmStateTXenq_Udp;
  status[NRC_STATUS_GLOBAL_STATE] = (ap_uint<32>) fsmStateTXdeq_Udp;
  status[NRC_STATUS_RX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_RX_cnt;
  status[NRC_STATUS_LAST_RX_NODE_ID] = (ap_uint<32>) (( (ap_uint<32>) last_rx_port) << 16) | ( (ap_uint<32>) last_rx_node_id);
  status[NRC_STATUS_TX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_TX_cnt;
  status[NRC_STATUS_LAST_TX_NODE_ID] = (ap_uint<32>) (((ap_uint<32>) last_tx_port) << 16) | ((ap_uint<32>) last_tx_node_id);
  status[NRC_STATUS_TX_PORT_CORRECTIONS] = (((ap_uint<32>) tcp_new_connection_failure_cnt) << 16) | ((ap_uint<16>) port_corrections_TX_cnt);
  status[NRC_STATUS_PACKET_CNT_RX] = (ap_uint<32>) packet_count_RX;
  status[NRC_STATUS_PACKET_CNT_TX] = (ap_uint<32>) packet_count_TX;

  status[NRC_UNAUTHORIZED_ACCESS] = (ap_uint<32>) unauthorized_access_cnt;
  status[NRC_AUTHORIZED_ACCESS] = (ap_uint<32>) authorized_access_cnt;

  //TODO: some consistency check for tables? (e.g. every IP address only once...)

  if (!tables_initalized)
  {
    printf("init tables...\n");
    for(int i = 0; i<MAX_NRC_SESSIONS; i++)
    {
      //sessionIdList[i] = UNUSED_TABLE_ENTRY_VALUE;
      sessionIdList[i] = 0;
      tripleList[i] = 0;
      usedRows[i]  =  0;
    }
    tables_initalized = true;
    //printf("Table layout:\n");
    //for(int i = 0; i<MAX_NRC_SESSIONS; i++)
    //{
    //  printf("%d | %d |  %llu\n",(int) i, (int) sessionIdList[i], (unsigned long long) tripleList[i]);
    //}
  }

  
  //===========================================================
  //  core wide variables (for one iteration)

  ap_uint<32> ipAddrLE = 0;
  ipAddrLE  = (ap_uint<32>) ((*myIpAddress >> 24) & 0xFF);
  ipAddrLE |= (ap_uint<32>) ((*myIpAddress >> 8) & 0xFF00);
  ipAddrLE |= (ap_uint<32>) ((*myIpAddress << 8) & 0xFF0000);
  ipAddrLE |= (ap_uint<32>) ((*myIpAddress << 24) & 0xFF000000);

  //===========================================================
  //  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
  }

//DON'T DO ANYTHING WITH NTS BEFORE IT'S NOT READY
  
  if(*piNTS_ready != 1)
  {
    return;
  }


  //===========================================================
  //  port requests
  if(udp_rx_ports_processed != *pi_udp_rx_ports)
  {
    //we can't close, so only look for newly opened
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
  } else {
    need_udp_port_req = false;
  }
 
  if(processed_FMC_listen_port != *piMMIO_FmcLsnPort)
  {
    fmc_port_opened = false;
    need_tcp_port_req = true;
    printf("Need FMC port request: %#02x\n",(unsigned int) *piMMIO_FmcLsnPort);

  } else if(tcp_rx_ports_processed != *pi_tcp_rx_ports)
  {
    //we can't close, so only look for newly opened
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
  } else {
    need_tcp_port_req = false;
  }

  //=================================================================================================
  // TX UDP

  //-------------------------------------------------------------------------------------------------
  // TX UDP Enqueue

  switch(fsmStateTXenq_Udp) {

    default:
    case FSM_RESET:
      pldLen_Udp = 0;
      fsmStateTXenq_Udp = FSM_W8FORMETA;
      udpTX_packet_length = 0;
      udpTX_current_packet_length = 0;
      //NO break! --> to be same as FSM_W8FORMETA

    case FSM_W8FORMETA:
      // The very first time, wait until the Rx path provides us with the
      // socketPair information before continuing
      if ( !siUdp_meta.empty() && !sFifo_Udp_Meta.full() ) {
        NetworkMetaStream tmp_meta_in = siUdp_meta.read();
        sFifo_Udp_Meta.write(tmp_meta_in);
        udpTX_packet_length = tmp_meta_in.tdata.len;
        udpTX_current_packet_length = 0;
        fsmStateTXenq_Udp = FSM_ACC;
      }
      //printf("waiting for NetworkMeta.\n");
      break;

    case FSM_ACC:

      // Default stream handling
      if ( !siUdp_data.empty() && !sFifo_Udp_Data.full() ) {

        // Read incoming data chunk
        UdpWord aWord = siUdp_data.read();
        udpTX_current_packet_length++;
        if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
        {//we need to set tlast manually
          aWord.tlast = 1;
        }

        // Increment the payload length
        updatePayloadLength(&aWord, &pldLen_Udp);

        // Enqueue data chunk into FiFo
        sFifo_Udp_Data.write(aWord);

        // Until LAST bit is set
        if (aWord.tlast)
        {
          fsmStateTXenq_Udp = FSM_LAST_ACC;
        }
      }
      break;

    case FSM_LAST_ACC:
      if( !sPLen_Udp.full() ) {
        // Forward the payload length
        sPLen_Udp.write(pldLen_Udp);
        // Reset the payload length
        pldLen_Udp = 0;
        // Start over
        fsmStateTXenq_Udp = FSM_W8FORMETA;
      }

      break;
  }

  //-------------------------------------------------------------------------------------------------
  // TX UDP Dequeue

  switch(fsmStateTXdeq_Udp) {

    default:
    case FSM_RESET: 
      fsmStateTXdeq_Udp = FSM_W8FORMETA;
      //NO break! --> to be same as FSM_W8FORMETA
    case FSM_W8FORMETA: //TODO: can be done also in FSM_FIRST_ACC, but leave it here for now

      // The very first time, wait until the Rx path provides us with the
      // socketPair information before continuing
      if ( !sFifo_Udp_Meta.empty() ) {
        out_meta_udp  = sFifo_Udp_Meta.read();
        fsmStateTXdeq_Udp = FSM_FIRST_ACC;
      }
      //printf("waiting for Internal Meta.\n");
      break;

    case FSM_FIRST_ACC:

      // Send out the first data together with the metadata and payload length information
      if ( !sFifo_Udp_Data.empty() && !sPLen_Udp.empty() ) 
      {
        if ( !soTHIS_Udmx_Data.full() && !soTHIS_Udmx_Meta.full() && !soTHIS_Udmx_PLen.full() ) 
        {
          
          //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
          NodeId dst_rank = out_meta_udp.tdata.dst_rank;
          if(dst_rank > MAX_CF_NODE_ID)
          {
            node_id_missmatch_TX_cnt++;
            //dst_rank = 0;
            //SINK packet
            sPLen_Udp.read();
            fsmStateTXdeq_Udp = FSM_DROP_PACKET;
            break;
          }
          ap_uint<32> dst_ip_addr = localMRT[dst_rank];
          if(dst_ip_addr == 0)
          {
            node_id_missmatch_TX_cnt++;
            //dst_ip_addr = localMRT[0];
            //SINK packet
            sPLen_Udp.read();
            fsmStateTXdeq_Udp = FSM_DROP_PACKET;
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
          UdpMeta txMeta = {{src_port, ipAddrLE}, {dst_port, dst_ip_addr}};
          soTHIS_Udmx_Meta.write(txMeta);
          packet_count_TX++;
          
          // Forward data chunk, metadata and payload length
          UdpWord    aWord = sFifo_Udp_Data.read();
          //if (!aWord.tlast) { //TODO?? we ignore packets smaller 64Bytes?
          soTHIS_Udmx_Data.write(aWord);

          soTHIS_Udmx_PLen.write(sPLen_Udp.read());
          if (aWord.tlast == 1) { 
            fsmStateTXdeq_Udp = FSM_W8FORMETA;
          } else {
            fsmStateTXdeq_Udp = FSM_ACC;
          }
        }
      }

        break;

        case FSM_ACC:
          // Default stream handling
          if ( !sFifo_Udp_Data.empty() && !soTHIS_Udmx_Data.full() ) {
            // Forward data chunk
            UdpWord    aWord = sFifo_Udp_Data.read();
            soTHIS_Udmx_Data.write(aWord);
            // Until LAST bit is set
            if (aWord.tlast == 1) 
            {
              fsmStateTXdeq_Udp = FSM_W8FORMETA;
            }
          }
        break;

        case FSM_DROP_PACKET:
          if ( !sFifo_Udp_Data.empty()) {
            // Forward data chunk
            UdpWord    aWord = sFifo_Udp_Data.read();
            // Until LAST bit is set
            if (aWord.tlast == 1) 
            {
              fsmStateTXdeq_Udp = FSM_W8FORMETA;
            }
          }
        break;


      }

      //=================================================================================================
      // RX UDP

      switch(fsmStateRX_Udp) {

        default:
        case FSM_RESET:
          openPortWaitTime = 10;
          fsmStateRX_Udp = FSM_IDLE;
          break;

        case FSM_IDLE:
          if(openPortWaitTime == 0) { 
            if ( !soTHIS_Udmx_OpnReq.full() && need_udp_port_req) {
              ap_uint<16> new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_udp;
              soTHIS_Udmx_OpnReq.write(new_absolute_port);
              fsmStateRX_Udp = FSM_W8FORPORT;
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
          if ( !siUDMX_This_OpnAck.empty() ) {
            // Read the acknowledgment
            AxisAck sOpenAck = siUDMX_This_OpnAck.read();
            fsmStateRX_Udp = FSM_FIRST_ACC;
            //port acknowleded
            need_udp_port_req = false;
            udp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_udp);
            printf("new udp_rx_ports_processed: %#03x\n",(int) udp_rx_ports_processed);
          }
          break;

        case FSM_FIRST_ACC:
          // Wait until both the first data chunk and the first metadata are received from UDP
          if ( !siUDMX_This_Data.empty() && !siUDMX_This_Meta.empty() ) {
            if ( !soUdp_data.full() ) {

              //extrac src ip address
              UdpMeta udpRxMeta = siUDMX_This_Meta.read();
              NodeId src_id = getNodeIdFromIpAddress(udpRxMeta.src.addr);
              if(src_id == 0xFFFF)
              {
                //SINK packet
                node_id_missmatch_RX_cnt++;
                fsmStateRX_Udp = FSM_DROP_PACKET;
                break;
              }
              last_rx_node_id = src_id;
              last_rx_port = udpRxMeta.dst.port;
              NetworkMeta tmp_meta = NetworkMeta(config[NRC_CONFIG_OWN_RANK], udpRxMeta.dst.port, src_id, udpRxMeta.src.port, 0);
              in_meta_udp = NetworkMetaStream(tmp_meta);
              // Forward data chunk to ROLE
              UdpWord    udpWord = siUDMX_This_Data.read();
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
          if ( !siUDMX_This_Data.empty() && !soUdp_data.full() ) {
            // Forward data chunk to ROLE
            UdpWord    udpWord = siUDMX_This_Data.read();
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
          if( !siUDMX_This_Data.empty() )
          {
            UdpWord    udpWord = siUDMX_This_Data.read();
            // Until LAST bit is set
            if (udpWord.tlast)
            {
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          break;
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
    
    char   *myName  = concat3(THIS_NAME, "/", "LSn");

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
        }
        else {
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

    switch(rrhFsmState) {
    case RRH_WAIT_NOTIF:
        if (!siTOE_Notif.empty()) {
            siTOE_Notif.read(notif_pRrh);
            if (notif_pRrh.tcpSegLen != 0) {
                // Always request the data segment to be received
                rrhFsmState = RRH_SEND_DREQ;
                //remember the session ID
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

    switch (rdpFsmState ) {

      default:
      case RDP_WAIT_META:
        if (!siTOE_SessId.empty()) {
          siTOE_SessId.read(sessId);

          ap_uint<64> tripple_in = getTrippleFromSessionId(sessId);
          printf("tripple_in: %llu\n",(unsigned long long) tripple_in);
          Ip4Addr remoteAddr = getRemoteIpAddrFromTripple(tripple_in);
          TcpPort dstPort = getLocalPortFromTripple(tripple_in);
          TcpPort srcPort = getRemotePortFromTripple(tripple_in);
          printf("remote Addr: %d; dstPort: %d; srcPort %d\n", (int) remoteAddr, (int) dstPort, (int) srcPort);

          if(dstPort == processed_FMC_listen_port) //take processed...just to be sure
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
          if(src_id == 0xFFFF)
          {
            //SINK packet
            node_id_missmatch_RX_cnt++;
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
        if (!siTOE_Data.empty() && !soFMC_Tcp_data.full()) 
        {
          siTOE_Data.read(currWord);
          //if (DEBUG_LEVEL & TRACE_RDP) { TODO: type management
          //  printAxiWord(myName, (AxiWord) currWord);
          //}
          soFMC_Tcp_data.write(currWord);
          if (currWord.tlast == 1)
          {
            rdpFsmState  = RDP_WAIT_META;
          }
        }
        // NO break;
      case RDP_WRITE_META_FMC:
        if( !Tcp_RX_metaWritten && !soFMC_Tcp_SessId.full())
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

    switch (wrpFsmState) {
    case WRP_WAIT_META:
        //FMC must come first
        if (!siFMC_Tcp_SessId.empty() && !soTOE_SessId.full()) {
            tcpSessId = (AppMeta) siFMC_Tcp_SessId.read().tdata;
            soTOE_SessId.write(tcpSessId);
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
          if(sessId == (SessionId) UNUSED_TABLE_ENTRY_VALUE)
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
                watchDogTimer_pcon = 10000;
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
                addnewSessionToTableWithTripple(newConn.sessionID, tripple_for_new_connection);
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
                #ifndef __SYNTHESIS__
                  watchDogTimer_pcon = 10;
                #else
                  watchDogTimer_pcon = 10000;
                #endif
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
