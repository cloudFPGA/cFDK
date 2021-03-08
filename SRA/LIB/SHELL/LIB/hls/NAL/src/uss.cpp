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
 * @file       : uss.cpp
 * @brief      : The UDP Sub System (USS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/

#include "uss.hpp"
#include "nal.hpp"

using namespace hls;


/*****************************************************************************
 * @brief Processes the outgoing UDP packets (i.e. ROLE -> Network).
 *
 * @param[in]   siUdp_data,            UDP Data from the Role
 * @param[in]   siUdp_meta,            UDP Metadata from the Role
 * @param[out]  soUOE_Data,            UDP Data for the UOE
 * @param[out]  soUOE_Meta,            UDP Metadata (i.e. HostSockAddr) for the UOE
 * @param[out]  soUOE_DLen,            UDP Packet length for the UOE
 * @param[out]  sGetIpReq_UdpTx,       Request stream for the the MRT Agency
 * @param[in]   sGetIpRep_UdpTx,       Reply stream from the MRT Agency
 * @param[in]   ipAddrBE,              IP address of the FPGA (from MMIO)
 * @param[in]   cache_inval_sig,       Signal from the Cache Invalidation Logic
 * @param[out]  internal_event_fifo,   Fifo for event reporting
 *
 ******************************************************************************/
void pUdpTX(
    stream<NetworkWord>         &siUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen,
    stream<NodeId>              &sGetIpReq_UdpTx,
    stream<Ip4Addr>             &sGetIpRep_UdpTx,
    const ap_uint<32>           *ipAddrBE,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char   *myName  = concat3(THIS_NAME, "/", "Udp_TX");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static FsmStateUdp fsmStateTX_Udp = FSM_RESET;
  static NodeId cached_nodeid_udp_tx = UNUSED_SESSION_ENTRY_VALUE;
  static Ip4Addr cached_ip4addr_udp_tx = 0;
  static bool cache_init = false;
  static uint8_t evs_loop_i = 0;

#pragma HLS RESET variable=fsmStateTX_Udp
#pragma HLS RESET variable=cached_nodeid_udp_tx
#pragma HLS RESET variable=cached_ip4addr_udp_tx
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=evs_loop_i

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static UdpAppDLen udpTX_packet_length = 0;
  static UdpAppDLen udpTX_current_packet_length = 0;
  static ap_uint<32> dst_ip_addr = 0;
  static NetworkMetaStream udp_meta_in;
  static NodeId dst_rank = 0;
  static UdpAppMeta txMeta;
  static NrcPort src_port;
  static NrcPort dst_port;

  static stream<NalEventNotif> evsStreams[6];

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NalEventNotif new_ev_not;

  switch(fsmStateTX_Udp) {

    default:
    case FSM_RESET:
      fsmStateTX_Udp = FSM_W8FORMETA;
      udpTX_packet_length = 0;
      udpTX_current_packet_length = 0;
      cache_init = false;
      break;

    case FSM_W8FORMETA:
      if( !cache_inval_sig.empty() )
      {
        if(cache_inval_sig.read())
        {
          cache_init = false; 
          cached_nodeid_udp_tx = UNUSED_SESSION_ENTRY_VALUE;
          cached_ip4addr_udp_tx = 0;
        }
        break;
      }
      else if ( !siUdp_meta.empty() &&
          !sGetIpReq_UdpTx.full() )
      {
        udp_meta_in = siUdp_meta.read();
        udpTX_packet_length = udp_meta_in.tdata.len;
        udpTX_current_packet_length = 0;

        // Send out the first data together with the metadata and payload length information

        dst_rank = udp_meta_in.tdata.dst_rank;
        if(dst_rank > MAX_CF_NODE_ID)
        {
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          evsStreams[0].write_nb(new_ev_not);
          //SINK packet
          fsmStateTX_Udp = FSM_DROP_PACKET;
          break;
        }

        src_port = udp_meta_in.tdata.src_port;
        if (src_port == 0)
        {
          src_port = DEFAULT_RX_PORT;
        }
        dst_port = udp_meta_in.tdata.dst_port;
        if (dst_port == 0)
        {
          dst_port = DEFAULT_RX_PORT;
          new_ev_not = NalEventNotif(PCOR_TX, 1);
          evsStreams[3].write_nb(new_ev_not);
        }

        //to create here due to timing...
        txMeta = SocketPair(SockAddr(*ipAddrBE, src_port), SockAddr(0, dst_port));

        //request ip if necessary
        if(cache_init && cached_nodeid_udp_tx == dst_rank)
        {
          dst_ip_addr = cached_ip4addr_udp_tx;
          fsmStateTX_Udp = FSM_FIRST_ACC;
          printf("used UDP TX id cache\n");
        } else {

          sGetIpReq_UdpTx.write(dst_rank);
          fsmStateTX_Udp = FSM_W8FORREQS;
          //break;
        }
      }
      break;

    case FSM_W8FORREQS:
      if(!sGetIpRep_UdpTx.empty())
      {
        dst_ip_addr = sGetIpRep_UdpTx.read();
        //TODO: need new FSM states
        cached_nodeid_udp_tx = dst_rank;
        cached_ip4addr_udp_tx = dst_ip_addr;
        cache_init = true;
        fsmStateTX_Udp = FSM_FIRST_ACC;
      }
      break;

    case FSM_FIRST_ACC:
      if ( !soUOE_Meta.full()
          && !soUOE_DLen.full() )
      {
        if(dst_ip_addr == 0)
        {
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          evsStreams[1].write_nb(new_ev_not);
          //SINK packet
          fsmStateTX_Udp = FSM_DROP_PACKET;
          break;
        }
        new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        evsStreams[2].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        evsStreams[4].write_nb(new_ev_not);
        //UdpMeta txMeta = {{src_port, ipAddrBE}, {dst_port, dst_ip_addr}};
        //txMeta = SocketPair(SockAddr(*ipAddrBE, src_port), SockAddr(dst_ip_addr, dst_port));
        txMeta.dst.addr = dst_ip_addr;

        // Forward data chunk, metadata and payload length
        soUOE_Meta.write(txMeta);

        //we can forward the length, even if 0
        //the UOE handles this as streaming mode
        soUOE_DLen.write(udpTX_packet_length);
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        evsStreams[5].write_nb(new_ev_not);

        fsmStateTX_Udp = FSM_ACC;
      }
      break;

    case FSM_ACC:
      // Default stream handling
      if ( !siUdp_data.empty() && !soUOE_Data.full() )
      {
        // Forward data chunk
        UdpAppData    aWord = siUdp_data.read();
        udpTX_current_packet_length += extractByteCnt((Axis<64>) aWord);
        if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
        {//we need to set tlast manually
          aWord.setTLast(1);
        }

        soUOE_Data.write(aWord);
        // Until LAST bit is set
        if (aWord.getTLast() == 1)
        {
          fsmStateTX_Udp = FSM_W8FORMETA;
        }
      }
      break;

    case FSM_DROP_PACKET:
      if ( !siUdp_data.empty() )
      {
        UdpAppData    aWord = siUdp_data.read();
        udpTX_current_packet_length += extractByteCnt((Axis<64>) aWord);

        if( (udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
            || aWord.getTLast() == 1 )
        {
          fsmStateTX_Udp = FSM_W8FORMETA;
        }
      }
      break;
  } //switch

  //-- ALWAYS -----------------------
  if(!internal_event_fifo.full())
  {
    if(!evsStreams[evs_loop_i].empty())
    {
      internal_event_fifo.write(evsStreams[evs_loop_i].read());
    }
    evs_loop_i++;
    if(evs_loop_i >= 6)
    {
      evs_loop_i = 0;
    }
  }

}



/*****************************************************************************
 * @brief Terminates the internal UDP TX FIFOs and forwards packets to the UOE.
 *
 ******************************************************************************/
void pUoeUdpTxDeq(
    ap_uint<1>                  *layer_4_enabled,
    ap_uint<1>                  *piNTS_ready,
    stream<UdpAppData>          &sUoeTxBuffer_Data,
    stream<UdpAppMeta>          &sUoeTxBuffer_Meta,
    stream<UdpAppDLen>          &sUoeTxBuffer_DLen,
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static DeqFsmStates deqFsmState = DEQ_WAIT_META;
#pragma HLS RESET variable=deqFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  UdpAppData cur_word;
  UdpAppMeta cur_meta;
  UdpAppDLen cur_dlen;
  bool uoe_disabled = (*layer_4_enabled == 0 || *piNTS_ready == 0);

  switch(deqFsmState)
  {
    default:
    case DEQ_WAIT_META:
      if(!sUoeTxBuffer_Data.empty() && !sUoeTxBuffer_Meta.empty() && !sUoeTxBuffer_DLen.empty()
          && ( (!soUOE_Data.full() && !soUOE_Meta.full() && !soUOE_DLen.full() ) || //UOE can read
            (uoe_disabled) //UOE is disabled -> drain FIFOs
            )
        )
      {
        cur_word = sUoeTxBuffer_Data.read();
        cur_meta = sUoeTxBuffer_Meta.read();
        cur_dlen = sUoeTxBuffer_DLen.read();
        if(!uoe_disabled)
        {
          soUOE_Data.write(cur_word);
          soUOE_Meta.write(cur_meta);
          soUOE_DLen.write(cur_dlen);
        }
        if(cur_word.getTLast() == 0)
        {
          deqFsmState = DEQ_STREAM_DATA;
        }
      }
      break;
    case DEQ_STREAM_DATA:
      if(!sUoeTxBuffer_Data.empty()
          && (!soUOE_Data.full() || uoe_disabled)
        )
      {
        cur_word = sUoeTxBuffer_Data.read();
        if(!uoe_disabled)
        {
          soUOE_Data.write(cur_word);
        }
        if(cur_word.getTLast() == 1)
        {
          deqFsmState = DEQ_WAIT_META;
        }
      }
      break;
  }

}



/*****************************************************************************
 * @brief Asks the UOE to open new UDP ports for listening, based on the 
 *        request from pPortLogic.
 *
 * @param[out]  soUOE_LsnReq,            open port request for UOE
 * @param[in]   siUOE_LsnRep,            response of the UOE
 * @param[in]   sUdpPortsToOpen,         port open request from pPortLogic
 * @param[out]  sUdpPortsOpenFeedback,   feedback to pPortLogic
 *
 ******************************************************************************/
void pUdpLsn(
    stream<UdpPort>             &soUOE_LsnReq,
    stream<StsBool>             &siUOE_LsnRep,
    stream<UdpPort>             &sUdpPortsToOpen,
    stream<bool>                &sUdpPortsOpenFeedback
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Udp_LSn");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static LsnFsmStates lsnFsmState = LSN_IDLE;
#ifdef __SYNTHESIS_
  static ap_uint<16>         startupDelay = 0x8000;
#else
  static ap_uint<16>         startupDelay = 10;
#endif

#pragma HLS RESET variable=lsnFsmState
#pragma HLS RESET variable=startupDelay
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<8>   watchDogTimer_plisten = 0;
  static UdpPort new_absolute_port = 0;

  switch (lsnFsmState) {

    default:
    case LSN_IDLE:
      if (startupDelay > 0)
      {
        startupDelay--;
      } else {
        lsnFsmState = LSN_SEND_REQ;
      }
      break;

    case LSN_SEND_REQ:
      if ( !soUOE_LsnReq.full() && !sUdpPortsToOpen.empty())
      {
        new_absolute_port = sUdpPortsToOpen.read();
        soUOE_LsnReq.write(new_absolute_port);
        if (DEBUG_LEVEL & TRACE_LSN) {
          printInfo(myName, "Requesting to listen on port #%d (0x%4.4X).\n",
              (int) new_absolute_port, (int) new_absolute_port);
#ifndef __SYNTHESIS__
          watchDogTimer_plisten = 10;
#else
          watchDogTimer_plisten = 100;
#endif
          lsnFsmState = LSN_WAIT_ACK;
        }
      }
      //else {
      //  printWarn(myName, "Cannot send a listen port request to [UOE] because stream is full!\n");
      //}
      break;

    case LSN_WAIT_ACK:
      if (!siUOE_LsnRep.empty() && !sUdpPortsOpenFeedback.full())
      {
        bool    listenDone;
        siUOE_LsnRep.read(listenDone);
        if (listenDone) {
          printInfo(myName, "Received listen acknowledgment from [UOE].\n");
          lsnFsmState = LSN_SEND_REQ;
          sUdpPortsOpenFeedback.write(true);
        }
        else {
          printWarn(myName, "UOE denied listening on port %d (0x%4.4X).\n",
              (int) new_absolute_port, (int) new_absolute_port);
          sUdpPortsOpenFeedback.write(false);
          lsnFsmState = LSN_SEND_REQ;
        }
      } else if (watchDogTimer_plisten == 0 && !sUdpPortsOpenFeedback.full() )
      {
        printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
            (int)  new_absolute_port, (int) new_absolute_port);
        sUdpPortsOpenFeedback.write(false);
        lsnFsmState = LSN_SEND_REQ;
      } else {
        watchDogTimer_plisten--;
      }
      break;

  }
}



/*****************************************************************************
 * @brief Processes the incoming UDP packets (i.e. Network -> ROLE ).
 *
 * @param[out]  soUdp_data,            UDP Data for the Role
 * @param[out]  soUdp_meta,            UDP Metadata for the Role
 * @param[in]   siUOE_Data,            UDP Data from the UOE
 * @param[in]   siUOE_Meta,            UDP Metadata from the UOE
 * @param[in]   sConfigUpdate,         Updates from axi4liteProcessing (own rank)
 * @param[out]  sGetNidReq_UdpRx,      Request stream for the the MRT Agency
 * @param[in]   sGetNidRep_UdpRx,      Reply stream from the MRT Agency
 * @param[in]   cache_inval_sig,       Signal from the Cache Invalidation Logic
 * @param[out]  internal_event_fifo,   Fifo for event reporting
 *
 ******************************************************************************/
void pUdpRx(
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &soUdp_meta,
    stream<UdpAppData>          &siUOE_Data,
    stream<UdpAppMeta>          &siUOE_Meta,
    stream<NalConfigUpdate>     &sConfigUpdate,
    stream<Ip4Addr>             &sGetNidReq_UdpRx,
    stream<NodeId>              &sGetNidRep_UdpRx,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char   *myName  = concat3(THIS_NAME, "/", "Udp_RX");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static FsmStateUdp fsmStateRX_Udp = FSM_W8FORMETA;
  static NodeId cached_udp_rx_id = 0;
  static Ip4Addr cached_udp_rx_ipaddr = 0;
  static NodeId own_rank = 0;
  static bool cache_init = false;
  static uint8_t evs_loop_i = 0;

#pragma HLS RESET variable=fsmStateRX_Udp
#pragma HLS RESET variable=cached_udp_rx_id
#pragma HLS RESET variable=cached_udp_rx_ipaddr
#pragma HLS RESET variable=own_rank
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=evs_loop_i

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static UdpMeta udpRxMeta;
  static NodeId src_id = INVALID_MRT_VALUE;
  static NetworkMeta in_meta;

  static stream<NalEventNotif> evsStreams[4];

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NalEventNotif new_ev_not;
  NetworkMetaStream in_meta_udp_stream;


  switch(fsmStateRX_Udp) {

    default:
      //case FSM_RESET:
      //fsmStateRX_Udp = FSM_W8FORMETA;
      //NO break;
    case FSM_W8FORMETA:
      if( !cache_inval_sig.empty() )
      {
        if(cache_inval_sig.read())
        {
          cached_udp_rx_id = 0;
          cached_udp_rx_ipaddr = 0;
          cache_init = false;
        }
        break;
      } else if(!sConfigUpdate.empty())
      {
        NalConfigUpdate ca = sConfigUpdate.read();
        if(ca.config_addr == NAL_CONFIG_OWN_RANK)
        {
          own_rank = (NodeId) ca.update_value;
          cache_init = false;
          cached_udp_rx_id = 0;
          cached_udp_rx_ipaddr = 0;
        }
        break;
      } else if ( !siUOE_Meta.empty()
          && !sGetNidReq_UdpRx.full())
      {
        //extract src ip address
        siUOE_Meta.read(udpRxMeta);
        src_id = (NodeId) INVALID_MRT_VALUE;

        //to create here due to timing...
        in_meta = NetworkMeta(own_rank, udpRxMeta.dst.port, 0, udpRxMeta.src.port, 0);

        //FIXME: add length here as soon as available from the UOE
        //ask cache
        if(cache_init && cached_udp_rx_ipaddr == udpRxMeta.src.addr)
        {
          printf("used UDP RX id cache\n");
          src_id = cached_udp_rx_id;
          fsmStateRX_Udp = FSM_FIRST_ACC;
        } else {
          sGetNidReq_UdpRx.write(udpRxMeta.src.addr);
          fsmStateRX_Udp = FSM_W8FORREQS;
          //break;
        }
      }
      break;

    case FSM_W8FORREQS:
      if(!sGetNidRep_UdpRx.empty())
      {
        src_id = sGetNidRep_UdpRx.read();
        cached_udp_rx_ipaddr = udpRxMeta.src.addr;
        cached_udp_rx_id = src_id;
        cache_init = true;
        fsmStateRX_Udp = FSM_FIRST_ACC;
      }
      break;

    case FSM_FIRST_ACC:
      if( !soUdp_meta.full() )
      {
        if(src_id == ((NodeId) INVALID_MRT_VALUE))
        {
          //SINK packet
          new_ev_not = NalEventNotif(NID_MISS_RX, 1);
          evsStreams[0].write_nb(new_ev_not);
          printf("[UDP-RX:ERROR]invalid src_id, packet will be dropped.\n");
          fsmStateRX_Udp = FSM_DROP_PACKET;
          cache_init = false;
          break;
        }
        //status
        new_ev_not = NalEventNotif(LAST_RX_NID, src_id);
        evsStreams[1].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(LAST_RX_PORT, udpRxMeta.dst.port);
        evsStreams[2].write_nb(new_ev_not);
        //in_meta = NetworkMeta(own_rank, udpRxMeta.dst.port, src_id, udpRxMeta.src.port, 0);
        //FIXME: add length here as soon as available from the UOE
        in_meta.src_rank = src_id;

        //write metadata
        in_meta_udp_stream = NetworkMetaStream(in_meta);
        soUdp_meta.write(in_meta_udp_stream);
        //soUdp_meta.write(in_meta_udp);
        new_ev_not = NalEventNotif(PACKET_RX, 1);
        evsStreams[3].write_nb(new_ev_not);
        // Forward data chunk to ROLE
        fsmStateRX_Udp = FSM_ACC;
      }
      break;

    case FSM_ACC:
      // Default stream handling
      if ( !siUOE_Data.empty() && !soUdp_data.full() )
      {
        // Forward data chunk to ROLE
        NetworkWord    udpWord = siUOE_Data.read();
        soUdp_data.write(udpWord);
        // Until LAST bit is set
        if (udpWord.tlast == 1)
        {
          fsmStateRX_Udp = FSM_W8FORMETA;
        }
      }
      break;

    case FSM_DROP_PACKET:
      if( !siUOE_Data.empty() )
      {
        UdpAppData    udpWord = siUOE_Data.read();
        // Until LAST bit is set
        if (udpWord.getTLast() == 1)
        {
          fsmStateRX_Udp = FSM_W8FORMETA;
        }
      }
      break;
  } //switch

  //-- ALWAYS -----------------------
  if(!internal_event_fifo.full())
  {
    if(!evsStreams[evs_loop_i].empty())
    {
      internal_event_fifo.write(evsStreams[evs_loop_i].read());
    }
    evs_loop_i++;
    if(evs_loop_i >= 4)
    {
      evs_loop_i = 0;
    }
  }

}


/*****************************************************************************
 * @brief Terminates the internal UDP RX FIFOs and forwards packets to the Role.
 *
 ******************************************************************************/
void pRoleUdpRxDeq(
    ap_uint<1>                  *layer_7_enabled,
    ap_uint<1>                  *role_decoupled,
    stream<NetworkWord>         &sRoleUdpDataRx_buffer,
    stream<NetworkMetaStream>   &sRoleUdpMetaRx_buffer,
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &soUdp_meta
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static DeqFsmStates deqFsmState = DEQ_WAIT_META;
#pragma HLS RESET variable=deqFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord cur_word = NetworkWord();
  NetworkMetaStream cur_meta = NetworkMetaStream();
  bool role_disabled = (*layer_7_enabled == 0 && *role_decoupled == 1);

  switch(deqFsmState)
  {
    default:
    case DEQ_WAIT_META:
      if(!sRoleUdpDataRx_buffer.empty() && !sRoleUdpMetaRx_buffer.empty()
          && ( (!soUdp_data.full() && !soUdp_meta.full()) ||  //user can read
            (role_disabled) //role is disabled -> drain FIFOs
            )
        )
      {
        cur_word = sRoleUdpDataRx_buffer.read();
        cur_meta = sRoleUdpMetaRx_buffer.read();
        if(!role_disabled)
        {
          soUdp_data.write(cur_word);
          soUdp_meta.write(cur_meta);
        }
        if(cur_word.tlast == 0)
        {
          deqFsmState = DEQ_STREAM_DATA;
        }
      }
      break;
    case DEQ_STREAM_DATA:
      if(!sRoleUdpDataRx_buffer.empty()
          && (!soUdp_data.full() || role_disabled)
        )
      {
        cur_word = sRoleUdpDataRx_buffer.read();
        if(!role_disabled)
        {
          soUdp_data.write(cur_word);
        }
        if(cur_word.tlast == 1)
        {
          deqFsmState = DEQ_WAIT_META;
        }
      }
      break;
  }

}



/*****************************************************************************
 * @brief Asks the UOE to close UDP ports, based on the request from pPortLogic.
 *
 * @param[out]  soUOE_ClsReq,            close port request for UOE
 * @param[in]   siUOE_ClsRep,            response of the UOE
 * @param[in]   sUdpPortsToClose,        port close request from pPortLogic
 *
 ******************************************************************************/
void pUdpCls(
    stream<UdpPort>             &soUOE_ClsReq,
    stream<StsBool>             &siUOE_ClsRep,
    stream<UdpPort>             &sUdpPortsToClose
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char *myName  = concat3(THIS_NAME, "/", "Udp_Cls");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static ClsFsmStates clsFsmState_Udp = CLS_IDLE;

#pragma HLS RESET variable=clsFsmState_Udp
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //static ap_uint<16> newRelativePortToClose = 0;
  static ap_uint<16> newAbsolutePortToClose = 0;


  switch (clsFsmState_Udp) 
  {
    default:
    case CLS_IDLE:
      clsFsmState_Udp = CLS_NEXT;
    case CLS_NEXT:
      if(!sUdpPortsToClose.empty() && !soUOE_ClsReq.full() )
      {
        //we have to close opened ports, one after another
        newAbsolutePortToClose = sUdpPortsToClose.read();
        soUOE_ClsReq.write(newAbsolutePortToClose);
        clsFsmState_Udp = CLS_WAIT4RESP;
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


/*! \} */

