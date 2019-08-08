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
#include "../../../../../hls/network_utils.hpp"
//#include "hls_math.h"
//#include "ap_fixed.h"

stream<UdpPLen>        sPLen_Udp     ("sPLen_Udp");
stream<UdpWord>        sFifo_Udp_Data("sFifo_Udp_Data");
stream<NetworkMetaStream>  sFifo_Udp_Meta("sFifo_Udp_Meta");

ap_uint<8>   openPortWaitTime = 10;
static bool metaWritten = false;

static FsmStateUdp fsmStateRX_Udp = FSM_RESET;

static UdpPLen    pldLen_Udp = 0;
static FsmStateUdp fsmStateTXenq_Udp = FSM_RESET;

static FsmStateUdp fsmStateTXdeq_Udp = FSM_RESET;

static ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

static ap_uint<32> udp_rx_ports_processed = 0;
static bool need_udp_port_req = false;
static ap_uint<16> new_relative_port_to_req_udp = 0;

static NetworkMetaStream out_meta_udp = NetworkMetaStream(); //DON'T FORGET to initilize!
static NetworkMetaStream in_meta_udp = NetworkMetaStream(); //ATTENTION: don't forget initilizer...

static ap_uint<32> node_id_missmatch_RX_cnt = 0;
static NodeId last_rx_node_id = 0;
static NrcPort last_rx_port = 0;
static ap_uint<32> node_id_missmatch_TX_cnt = 0;
static NodeId last_tx_node_id = 0;
static NrcPort last_tx_port = 0;
static ap_uint<32> port_corrections_TX_cnt = 0;

static ap_uint<32> packet_count_RX = 0;
static ap_uint<32> packet_count_TX = 0;

NetworkDataLength udpTX_packet_length = 0;
NetworkDataLength udpTX_current_packet_length = 0;


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

    //-- ROLE / This / Network Interfaces
    ap_uint<32>                 *pi_udp_rx_ports,
    stream<UdpWord>             &siUdp_data,
    stream<UdpWord>             &soUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<NetworkMetaStream>   &soUdp_meta,
    ap_uint<32>                 *myIpAddress,

    //-- UDMX / This / Open-Port Interfaces
    stream<AxisAck>     &siUDMX_This_OpnAck,
    stream<UdpPort>     &soTHIS_Udmx_OpnReq,

    //-- UDMX / This / Data & MetaData Interfaces
    stream<UdpWord>     &siUDMX_This_Data,
    stream<UdpMeta>     &siUDMX_This_Meta,
    stream<UdpWord>     &soTHIS_Udmx_Data,
    stream<UdpMeta>     &soTHIS_Udmx_Meta,
    stream<UdpPLen>     &soTHIS_Udmx_PLen
    )
{

  //-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
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
#pragma HLS INTERFACE ap_vld register port=pi_udp_rx_ports name=piROL_NRC_Udp_Rx_ports

#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_NRC_ctrlLink_AXI
#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_NRC_ctrlLink_AXI



  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
  //#pragma HLS DATAFLOW interval=1
#pragma HLS STREAM variable=sPLen_Udp depth=1
#pragma HLS STREAM variable=sFifo_Udp_Meta depth=1
#pragma HLS STREAM variable=sFifo_Udp_Data depth=2048    // Must be able to contain MTU


  //=================================================================================================
  // Reset global variables 

#pragma HLS reset variable=metaWritten
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



  //===========================================================
  // MRT

  //copy MRT axi Interface
  //MRT data are after possible config DATA
  for(int i = 0; i < MAX_MRT_SIZE; i++)
  {
    //localMRT[i] = MRT[i];
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
    printf("port diff: %#04x\n",(int) diff);
    if(diff != 0)
    {//we have to open new ports, one after another
      new_relative_port_to_req_udp = getRightmostBitPos(diff);
      need_udp_port_req = true;
    }
  } else {
    need_udp_port_req = false;
  }

  //-- PROCESS FUNCTIONS ----------------------------------------------------
  //pTxP(siUdp_data,  siIP,
  //     soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen, myIpAddress);
  //=================================================================================================
  // TX 


  //-- PROCESS FUNCTIONS ----------------------------------------------------
  //pTxP_Enqueue(siUdp_data,
  //             sFifo_Udp_Data, sPLen_Udp);
  //-------------------------------------------------------------------------------------------------
  // TX Enqueue

  switch(fsmStateTXenq_Udp) {

    default:
    case FSM_RESET:
      pldLen_Udp = 0;
      //fsmStateTXenq_Udp = FSM_ACC;
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
        //fsmStateTXenq_Udp = FSM_ACC;
        fsmStateTXenq_Udp = FSM_W8FORMETA;
      }

      break;
  }



  //pTxP_Dequeue(sFifo_Udp_Data,  siIP,  sPLen_Udp,
  //             soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen, myIpAddress);
  //-------------------------------------------------------------------------------------------------
  // TX Dequeue


  switch(fsmStateTXdeq_Udp) {

    default:
    case FSM_RESET: 
      fsmStateTXdeq_Udp = FSM_W8FORMETA;
      //NO break! --> to be same as FSM_W8FORMETA
    case FSM_W8FORMETA: //TODO: can be done also in FSM_FIRST_ACC, but leave it here for now

      // The very first time, wait until the Rx path provides us with the
      // socketPair information before continuing
      //if ( !siUdp_meta.empty() ) {
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
            //fsmStateTXdeq_Udp = FSM_FIRST_ACC;
            fsmStateTXdeq_Udp = FSM_W8FORMETA;
          }
        }

        break;
      }

      //---- From UDMX to ROLE
      //pRxP(siUDMX_This_Data,  siUDMX_This_Meta,
      //    soTHIS_Udmx_OpnReq, siUDMX_This_OpnAck,
      //    soUdp_data,    soIP);
      //=================================================================================================
      // RX 



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
              //soTHIS_Udmx_OpnReq.write(DEFAULT_RX_PORT);
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

              metaWritten = false; //don't put the meta stream in the critical path
              if (!udpWord.tlast) {
                fsmStateRX_Udp = FSM_ACC;
              } else { 
                //fsmStateRX_Udp = FSM_FIRST_ACC; //wait for next packet = stay here
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
          if ( !metaWritten && !soUdp_meta.full() )
          {
            soUdp_meta.write(in_meta_udp);
            packet_count_RX++;
            metaWritten = true;
            if ( fsmStateRX_Udp == FSM_WRITE_META)
            {//was a small packet
              fsmStateRX_Udp = FSM_FIRST_ACC;
            }
          }
          break;
      }


  }

