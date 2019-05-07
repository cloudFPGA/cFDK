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

stream<UdpPLen>        sPLen     ("sPLen");
stream<UdpWord>        sFifo_Data("sFifo_Data");

ap_uint<8>   openPortWaitTime = 10;
bool metaWritten = false;

FsmState fsmStateRX = FSM_RESET;

UdpPLen    pldLen = 0;
FsmState fsmStateTXenq = FSM_RESET;

FsmState fsmStateTXdeq = FSM_RESET;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

ap_uint<32> udp_rx_ports_processed = 0;
bool need_udp_port_req = false;
ap_uint<16> new_relative_port_to_req = 0;

ap_uint<32> node_id_missmatch_RX_cnt = 0;
NodeId last_rx_node_id = 0;
NrcPort last_rx_port = 0;
ap_uint<32> node_id_missmatch_TX_cnt = 0;
NodeId last_tx_node_id = 0;
NrcPort last_tx_port = 0;
ap_uint<32> port_corrections_TX_cnt = 0;

/*****************************************************************************
 * @brief Update the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_role_if
 *
 * @param[in]     axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen, a pointer to the payload length of the corresponding
 *                     AXI4-Stream.
 * @return Nothing.
 ******************************************************************************/
void updatePayloadLength(UdpWord *axisChunk, UdpPLen *pldLen) {
    if (axisChunk->tlast) {
        int bytCnt = 0;
        for (int i = 0; i < 8; i++) {
            if (axisChunk->tkeep.bit(i) == 1) {
                bytCnt++;
            }
        }
        *pldLen += bytCnt;
    } else
        *pldLen += 8;
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
    // ----- system reset ---
    ap_uint<1> sys_reset,
    // ----- link to SMC -----
    ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

    //-- ROLE / This / Network Interfaces
    ap_uint<32>             *pi_udp_rx_ports,
    stream<UdpWord>         &siUdp_data,
    stream<UdpWord>         &soUdp_data,
    //stream<NrcMeta>   &siNrc_meta,
    stream<NrcMetaStream>   &siNrc_meta,
    //stream<NrcMeta>   &soNrc_meta,
    stream<NrcMetaStream>   &soNrc_meta,
    ap_uint<32>             *myIpAddress,

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

#pragma HLS INTERFACE axis register both port=siNrc_meta
#pragma HLS INTERFACE axis register both port=soNrc_meta

#pragma HLS INTERFACE ap_vld register port=myIpAddress name=piMyIpAddress
#pragma HLS INTERFACE ap_vld register port=pi_udp_rx_ports name=piROL_NRC_Udp_Rx_ports
#pragma HLS INTERFACE ap_vld register port=sys_reset name=piSysReset

#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_NRC_ctrlLink_AXI
#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_NRC_ctrlLink_AXI



//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
//#pragma HLS DATAFLOW interval=1
#pragma HLS STREAM variable=sPLen depth=1
#pragma HLS STREAM variable=sFifo_Data depth=2048    // Must be able to contain MTU


//=================================================================================================
// Reset global variables 

#pragma HLS reset variable=metaWritten
#pragma HLS reset variable=fsmStateRX
#pragma HLS reset variable=fsmStateTXenq
#pragma HLS reset variable=fsmStateTXdeq
#pragma HLS reset variable=pldLen


  if(sys_reset == 1)
  {
    for(int i = 0; i < MAX_MRT_SIZE; i++)
    {
      localMRT[i] = 0;
    }
    for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    {
      config[i] = 0;
    }
    for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
    {
      status[i] = 0;
    }

    openPortWaitTime = 10;
    metaWritten = false; 
    fsmStateRX = FSM_RESET;
    fsmStateTXenq = FSM_RESET;
    fsmStateTXdeq = FSM_RESET;
    pldLen = 0;

    //TODO: reset processed ports if ports can also be closed 
    // or does a double request not harm?
    udp_rx_ports_processed = 0;
    need_udp_port_req = false;
    new_relative_port_to_req = 0;
    node_id_missmatch_RX_cnt = 0;
    node_id_missmatch_TX_cnt = 0;
    port_corrections_TX_cnt = 0;
    last_rx_node_id = 0;
    last_rx_port = 0;
    last_tx_node_id = 0;
    last_tx_port = 0;
    return;
  }

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

  status[NRC_STATUS_SEND_STATE] = (ap_uint<32>) fsmStateRX;
  status[NRC_STATUS_RECEIVE_STATE] = (ap_uint<32>) fsmStateTXenq;
  status[NRC_STATUS_GLOBAL_STATE] = (ap_uint<32>) fsmStateTXdeq;
  status[NRC_STATUS_RX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_RX_cnt;
  status[NRC_STATUS_LAST_RX_NODE_ID] = (ap_uint<32>) (( (ap_uint<32>) last_rx_port) << 16) | ( (ap_uint<32>) last_rx_node_id);
  status[NRC_STATUS_TX_NODEID_ERROR] = (ap_uint<32>) node_id_missmatch_TX_cnt;
  status[NRC_STATUS_LAST_TX_NODE_ID] = (ap_uint<32>) (((ap_uint<32>) last_tx_port) << 16) | ((ap_uint<32>) last_tx_node_id);
  status[NRC_STATUS_TX_PORT_CORRECTIONS] = (ap_uint<32>) port_corrections_TX_cnt;

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
      new_relative_port_to_req = getRightmostBitPos(diff);
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
  //             sFifo_Data, sPLen);
//-------------------------------------------------------------------------------------------------
// TX Enqueue

  switch(fsmStateTXenq) {

    case FSM_RESET:
      pldLen = 0;
      fsmStateTXenq = FSM_ACC;
      break;

    case FSM_ACC:

      // Default stream handling
      if ( !siUdp_data.empty() && !sFifo_Data.full() ) {

        // Read incoming data chunk
        UdpWord aWord = siUdp_data.read();

        // Increment the payload length
        updatePayloadLength(&aWord, &pldLen);

        // Enqueue data chunk into FiFo
        sFifo_Data.write(aWord);

        // Until LAST bit is set
        if (aWord.tlast)
        {
          fsmStateTXenq = FSM_LAST_ACC;
        }
      }
      break;

    case FSM_LAST_ACC:

      // Forward the payload length
      sPLen.write(pldLen);

      // Reset the payload length
      pldLen = 0;

      // Start over
      fsmStateTXenq = FSM_ACC;

      break;
  }



  //pTxP_Dequeue(sFifo_Data,  siIP,  sPLen,
  //             soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen, myIpAddress);
//-------------------------------------------------------------------------------------------------
// TX Dequeue

  NrcMetaStream out_meta = NrcMetaStream(); //DON'T FORGET to initilize!

  switch(fsmStateTXdeq) {

    case FSM_RESET: 
      fsmStateTXdeq = FSM_W8FORMETA;
      //NO break! --> to be same as FSM_W8FORMETA
    case FSM_W8FORMETA: //TODO: can be done also in FSM_FIRST_ACC, but leave it here for now

      // The very first time, wait until the Rx path provides us with the
      // socketPair information before continuing
      if ( !siNrc_meta.empty() ) {
        out_meta = siNrc_meta.read();
        fsmStateTXdeq = FSM_FIRST_ACC;
      }
      //printf("waiting for NrcMeta.\n");
      break;

    case FSM_FIRST_ACC:

      // Send out the first data together with the metadata and payload length information
      if ( !sFifo_Data.empty() && !sPLen.empty() ) {
        if ( !soTHIS_Udmx_Data.full() && !soTHIS_Udmx_Meta.full() && !soTHIS_Udmx_PLen.full() ) {
          // Forward data chunk, metadata and payload length
          UdpWord    aWord = sFifo_Data.read();
          //if (!aWord.tlast) { //TODO?? we ignore packets smaller 64Bytes?
            soTHIS_Udmx_Data.write(aWord);

            ap_uint<32> ipAddrLE = 0;
            ipAddrLE  = (*myIpAddress >> 24) & 0xFF;
            ipAddrLE |= (*myIpAddress >> 8) & 0xFF00;
            ipAddrLE |= (*myIpAddress << 8) & 0xFF0000;
            ipAddrLE |= (*myIpAddress << 24) & 0xFF000000;
            //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
            
            NodeId dst_rank = out_meta.tdata.dst_rank;
            if(dst_rank > MAX_CF_NODE_ID)
            {
              node_id_missmatch_TX_cnt++;
              dst_rank = 0;
            }
            ap_uint<32> dst_ip_addr = localMRT[dst_rank];
            last_tx_node_id = dst_rank;
            NrcPort src_port = out_meta.tdata.src_port; //TODO: DEBUG
            if (src_port == 0)
            {
              src_port = DEFAULT_RX_PORT;
            }
            NrcPort dst_port = out_meta.tdata.dst_port; //TODO: DEBUG
            if (dst_port == 0)
            {
              dst_port = DEFAULT_RX_PORT;
              port_corrections_TX_cnt++;
            }
            last_tx_port = dst_port;
            // {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
            UdpMeta txMeta = {{src_port, ipAddrLE}, {dst_port, dst_ip_addr}};
            soTHIS_Udmx_Meta.write(txMeta);

            soTHIS_Udmx_PLen.write(sPLen.read());
          if (aWord.tlast) { 
            fsmStateTXdeq = FSM_W8FORMETA;
          } else {
            fsmStateTXdeq = FSM_ACC;
          }
        }
      }

      break;

    case FSM_ACC:

      // Default stream handling
      if ( !sFifo_Data.empty() && !soTHIS_Udmx_Data.full() ) {
        // Forward data chunk
        UdpWord    aWord = sFifo_Data.read();
        soTHIS_Udmx_Data.write(aWord);
        // Until LAST bit is set
        if (aWord.tlast) 
        {
          //fsmStateTXdeq = FSM_FIRST_ACC;
          fsmStateTXdeq = FSM_W8FORMETA;
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


  NrcMetaStream in_meta = NrcMetaStream(); //ATTENTION: don't forget initilizer...

  switch(fsmStateRX) {

    case FSM_RESET:
      openPortWaitTime = 10;
      fsmStateRX = FSM_IDLE;
      break;

    case FSM_IDLE:
      if(openPortWaitTime == 0) { 
        if ( !soTHIS_Udmx_OpnReq.full() && need_udp_port_req) {
          ap_uint<16> new_absolute_port = UDP_RX_MIN_PORT + new_relative_port_to_req;
          //soTHIS_Udmx_OpnReq.write(DEFAULT_RX_PORT);
          soTHIS_Udmx_OpnReq.write(new_absolute_port);
          fsmStateRX = FSM_W8FORPORT;
        } else if(udp_rx_ports_processed > 0)
        { // we have already at least one open port 
          //don't hang after reset
          fsmStateRX = FSM_FIRST_ACC;
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
        fsmStateRX = FSM_FIRST_ACC;
        //port acknowleded
        need_udp_port_req = false;
        udp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req);
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
          if (!udpWord.tlast) {
            fsmStateRX = FSM_ACC;
          } else { 
            fsmStateRX = FSM_FIRST_ACC; //wait for next packet = stay here
          }

          //extrac src ip address
          UdpMeta udpRxMeta = siUDMX_This_Meta.read();
          NodeId src_id = getNodeIdFromIpAddress(udpRxMeta.src.addr);
          last_rx_node_id = src_id;
          last_rx_port = udpRxMeta.dst.port;
          NrcMeta tmp_meta = NrcMeta(config[NRC_CONFIG_OWN_RANK], udpRxMeta.dst.port, src_id, udpRxMeta.src.port);
          in_meta = NrcMetaStream(tmp_meta);

          metaWritten = false;
        } 
      }
      if(need_udp_port_req)
      {
        fsmStateRX = FSM_IDLE;
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
          fsmStateRX = FSM_FIRST_ACC;
        }
      }
      if ( !metaWritten && !soNrc_meta.full() )
      {
        soNrc_meta.write(in_meta);
        metaWritten = true;
      }
      break;
  }


}

