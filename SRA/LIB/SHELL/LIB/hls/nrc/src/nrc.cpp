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
//#include "../../../../../hls/network_utils.hpp"
//#include "../../../../../hls/memory_utils.hpp"
//#include "../../../../../hls/simulation_utils.hpp"

//#include "hls_math.h"
//#include "ap_fixed.h"

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
ap_uint<32> port_corrections_TX_cnt = 0;

ap_uint<32> packet_count_RX = 0;
ap_uint<32> packet_count_TX = 0;

NetworkDataLength udpTX_packet_length = 0;
NetworkDataLength udpTX_current_packet_length = 0;


//FROM TCP

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TRIF"

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
#define DEFAULT_FPGA_LSN_PORT   0x2263      // TOE    listens on port = 8803 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // HOST's IP Address      = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   8803+0x8000 // HOST   listens on port = 41571


AppOpnReq     leHostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER
AppOpnSts     newConn;
ap_uint<12>  watchDogTimer;
ap_uint<8>   watchDogTimer_plisten = 0;

// Set a startup delay long enough to account for the initialization
// of TOE's listen port table which takes 32,768 cycles after reset.
//  [FIXME - StartupDelay must be replaced by a piSHELL_Reday signal]
ap_uint<16>         startupDelay = 0x8000;
OpnFsmStates opnFsmState=OPN_IDLE;

LsnFsmStates lsnFsmState = LSN_IDLE;

RrhFsmStates rrhFsmState = RRH_WAIT_NOTIF;
AppNotif notif_pRrh;

RdpFsmStates rdpFsmState = RDP_WAIT_META;

WrpFsmStates wrpFsmState = WRP_WAIT_META;

ap_uint<32> tcp_rx_ports_processed = 0;
bool need_tcp_port_req = false;
ap_uint<16> new_relative_port_to_req_tcp = 0;



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

/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 *
 *****************************************************************************/
void nrc_main(
    // ----- link to FMC -----
    ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
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
    stream<NetworkMetaStream>   &siFMC_Tcp_meta,
    stream<TcpWord>             &soFMC_Tcp_data,
    stream<NetworkMetaStream>   &soFMC_Tcp_meta,

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
    stream<AppData>     &siTOE_Data,
    stream<AppMeta>     &siTOE_SessId,
    //-- TOE / Listen Interfaces
    stream<AppLsnReq>   &soTOE_LsnReq,
    stream<AppLsnAck>   &siTOE_LsnAck,
    //-- TOE / Tx Data Interfaces
    stream<AppData>     &soTOE_Data,
    stream<AppMeta>     &soTOE_SessId,
    stream<AppWrSts>    &siTOE_DSts,
    //-- TOE / Open Interfaces
    stream<AppOpnReq>   &soTOE_OpnReq,
    stream<AppOpnSts>   &siTOE_OpnRep,
    //-- TOE / Close Interfaces
    stream<AppClsReq>   &soTOE_ClsReq
  )
{

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

#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_NRC_ctrlLink_AXI
#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_NRC_ctrlLink_AXI

#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE ap_vld register port=pi_tcp_rx_ports name=piROL_Tcp_Rx_ports

#pragma HLS INTERFACE axis register both port=siFMC_Tcp_data
#pragma HLS INTERFACE axis register both port=siFMC_Tcp_data
#pragma HLS INTERFACE axis register both port=soFMC_Tcp_meta
#pragma HLS INTERFACE axis register both port=soFMC_Tcp_meta


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

  //DO NOT reset MRT and config. This should be done explicitly by the SMC
#pragma HLS reset variable=localMRT off
#pragma HLS reset variable=config off
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
  status[NRC_STATUS_TX_PORT_CORRECTIONS] = (ap_uint<32>) port_corrections_TX_cnt;
  status[NRC_STATUS_PACKET_CNT_RX] = (ap_uint<32>) packet_count_RX;
  status[NRC_STATUS_PACKET_CNT_TX] = (ap_uint<32>) packet_count_TX;

  status[NRC_STATUS_UNUSED_1] = 0x0;
  status[NRC_STATUS_UNUSED_2] = 0x0;

  //TODO: some consistency check for tables? (e.g. every IP address only once...)


  //===========================================================
  //  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
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
    printf("UDP port diff: %#04x\n",(int) diff);
    if(diff != 0)
    {//we have to open new ports, one after another
      new_relative_port_to_req_udp = getRightmostBitPos(diff);
      need_udp_port_req = true;
    }
  } else {
    need_udp_port_req = false;
  }
  
  if(tcp_rx_ports_processed != *pi_tcp_rx_ports)
  {
    //we can't close, so only look for newly opened
    ap_uint<32> tmp = tcp_rx_ports_processed | *tcp_udp_rx_ports;
    ap_uint<32> diff = tcp_rx_ports_processed ^ tmp;
    //printf("rx_ports IN: %#04x\n",(int) *pi_tcp_rx_ports);
    //printf("tcp_rx_ports_processed: %#04x\n",(int) tcp_rx_ports_processed);
    printf("TCP port diff: %#04x\n",(int) diff);
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
      if ( !sFifo_Udp_Data.empty() && !sPLen_Udp.empty() ) {
        if ( !soTHIS_Udmx_Data.full() && !soTHIS_Udmx_Meta.full() && !soTHIS_Udmx_PLen.full() ) {
          // Forward data chunk, metadata and payload length
          UdpWord    aWord = sFifo_Udp_Data.read();
          //if (!aWord.tlast) { //TODO?? we ignore packets smaller 64Bytes?
          soTHIS_Udmx_Data.write(aWord);


          ap_uint<32> ipAddrLE = 0;
          ipAddrLE  = (*myIpAddress >> 24) & 0xFF;
          ipAddrLE |= (*myIpAddress >> 8) & 0xFF00;
          ipAddrLE |= (*myIpAddress << 8) & 0xFF0000;
          ipAddrLE |= (*myIpAddress << 24) & 0xFF000000;
          //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};

          NodeId dst_rank = out_meta_udp.tdata.dst_rank;
          if(dst_rank > MAX_CF_NODE_ID)
          {
            node_id_missmatch_TX_cnt++;
            dst_rank = 0;
          }
          ap_uint<32> dst_ip_addr = localMRT[dst_rank];
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
          if (aWord.tlast) 
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
              // Forward data chunk to ROLE
              UdpWord    udpWord = siUDMX_This_Data.read();
              soUdp_data.write(udpWord);

              //extrac src ip address
              UdpMeta udpRxMeta = siUDMX_This_Meta.read();
              NodeId src_id = getNodeIdFromIpAddress(udpRxMeta.src.addr);
              last_rx_node_id = src_id;
              last_rx_port = udpRxMeta.dst.port;
              NetworkMeta tmp_meta = NetworkMeta(config[NRC_CONFIG_OWN_RANK], udpRxMeta.dst.port, src_id, udpRxMeta.src.port, 0);
              in_meta_udp = NetworkMetaStream(tmp_meta);

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
            ap_uint<16> new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
            AppLsnReq    tcpListenPort = new_absolute_port;
            soTOE_LsnReq.write(tcpListenPort);
            if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                          new_absolute_port, new_absolute_port);
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
                tcp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_tcp);
                printf("new tcp_rx_ports_processed: %#03x\n",(int) tcp_rx_ports_processed);
            }
            else {
                ap_uint<16> new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          new_absolute_port, new_absolute_port);
                lsnFsmState = LSN_SEND_REQ;
            }
        }
        else {
            if (watchDogTimer_plisten == 0) {
                ap_uint<16> new_absolute_port = NRC_RX_MIN_PORT + new_relative_port_to_req_tcp;
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                           new_absolute_port, new_absolute_port);
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

