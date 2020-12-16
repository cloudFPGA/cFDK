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
 * @file       : tss.cpp
 * @brief      : The TCP Sub System (TSS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/

#include "tss.hpp"
#include "nal.hpp"

using namespace hls;

/*****************************************************************************
 * @brief Request the TOE to start listening (LSn) for incoming connections
 *  on a specific port (.i.e open connection for reception mode).
 *
 * @param[out] soTOE_LsnReq,   listen port request to TOE.
 * @param[in]  siTOE_LsnRep,   listen port acknowledge from TOE.
 *
 * @warning
 *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
 *   32,767) which are used for listening ports, and one for dynamically
 *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
 *   connections. Therefore, listening port numbers must be in the range 0
 *   to 32,767.
 ******************************************************************************/
void pTcpLsn(
    stream<TcpAppLsnReq>      &soTOE_LsnReq,
    stream<TcpAppLsnRep>      &siTOE_LsnRep,
    stream<TcpPort>       &sTcpPortsToOpen,
    stream<bool>        &sTcpPortsOpenFeedback,
    const bool          *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
  //#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Tcp_LSn");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static LsnFsmStates lsnFsmState = LSN_IDLE;
#ifdef __SYNTHESIS_
  static ap_uint<16>         startupDelay = 0x8000;
#else
  static ap_uint<16>         startupDelay = 30;
#endif

#pragma HLS RESET variable=lsnFsmState
#pragma HLS RESET variable=startupDelay
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<8>   watchDogTimer_plisten = 0;
  ap_uint<16> new_absolute_port = 0;


  if(!*nts_ready_and_enabled)
  {
    lsnFsmState = LSN_IDLE;
  }


  //only if NTS is ready
  // if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  // {
  //   if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC )
  //   { //so we are not in a critical UDP path
  switch (lsnFsmState) {

    case LSN_IDLE:
      if (startupDelay > 0)
      {
        startupDelay--;
      } else {
        //if(*need_tcp_port_req == true)
        if(!sTcpPortsToOpen.empty())
        {
          lsnFsmState = LSN_SEND_REQ;
        } else {
          lsnFsmState = LSN_DONE;
        }
      }
      break;

    case LSN_SEND_REQ: //we arrive here only if need_tcp_port_req == true
      if (!soTOE_LsnReq.full() && !sTcpPortsToOpen.empty()) {
        new_absolute_port = sTcpPortsToOpen.read();
        //always process FMC first
        //              if(fmc_port_opened == false)
        //              {
        //                new_absolute_port = *piMMIO_FmcLsnPort;
        //              } else {
        //                new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
        //              }

        TcpAppLsnReq    tcpListenPort = new_absolute_port;
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
      if (!siTOE_LsnRep.empty() && !sTcpPortsOpenFeedback.full()) {
        bool    listenDone;
        siTOE_LsnRep.read(listenDone);
        if (listenDone) {
          printInfo(myName, "Received listen acknowledgment from [TOE].\n");
          lsnFsmState = LSN_DONE;

          //                *need_tcp_port_req = false;
          //                if(fmc_port_opened == false)
          //                {
          //                  fmc_port_opened = true;
          //                  *processed_FMC_listen_port = *piMMIO_FmcLsnPort;
          //                } else {
          //                  *tcp_rx_ports_processed |= ((ap_uint<32>) 1) << (*new_relative_port_to_req_tcp);
          //                  printf("new tcp_rx_ports_processed: %#03x\n",(int) *tcp_rx_ports_processed);
          //                }
          sTcpPortsOpenFeedback.write(true);
        }
        else {
          //ap_uint<16> new_absolute_port = 0;
          //always process FMC first
          //                if(fmc_port_opened == false)
          //                {
          //                  new_absolute_port = *piMMIO_FmcLsnPort;
          //                } else {
          //                  new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
          //                }
          printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
              (int) new_absolute_port, (int) new_absolute_port);
          sTcpPortsOpenFeedback.write(false);
          lsnFsmState = LSN_SEND_REQ;
        }
      } else {
        if (watchDogTimer_plisten == 0) {
          ap_uint<16> new_absolute_port = 0;
          //always process FMC first
          //                if(fmc_port_opened == false)
          //                {
          //                  new_absolute_port = *piMMIO_FmcLsnPort;
          //                } else {
          //                  new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
          //                }
          printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
              (int)  new_absolute_port, (int) new_absolute_port);
          sTcpPortsOpenFeedback.write(false);
          lsnFsmState = LSN_SEND_REQ;
        }
      }
      break;

    case LSN_DONE:
      if(!sTcpPortsToOpen.empty())
      {
        lsnFsmState = LSN_SEND_REQ;
      }
      break;
  }
  //  }
  // }

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
void pTcpRRh(
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppRdReq>       &soTOE_DReq,
    ap_uint<1>                  *piFMC_Tcp_data_FIFO_prog_full,
    ap_uint<1>                  *piFMC_Tcp_sessid_FIFO_prog_full,
    SessionId         *cached_tcp_rx_session_id,
    const bool          *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Tcp_RRh");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RrhFsmStates rrhFsmState = RRH_WAIT_NOTIF;
  //todo: DReq table

#pragma HLS RESET variable=rrhFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static TcpAppNotif notif_pRrh;

  static stream<TcpAppNotif> notifBuffer  ("pTcpRRh_NotifBuffer");
#pragma HLS STREAM variable=notifBuffer depth=2048

  //TODO?
  if(!*nts_ready_and_enabled)
  {
    rrhFsmState = RRH_WAIT_NOTIF;
    notif_pRrh = TcpAppNotif();
  }



  //only if NTS is ready
  //   if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  //   {
  //        //so we are not in a critical UDP path
  //        if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC &&
  //            //and only if the FMC FIFO would have enough space
  //            (*piFMC_Tcp_data_FIFO_prog_full == 0 && *piFMC_Tcp_sessid_FIFO_prog_full == 0 )
  //          )
  //        {
  switch(rrhFsmState) {
    case RRH_WAIT_NOTIF:
      if (!siTOE_Notif.empty()) {
        siTOE_Notif.read(notif_pRrh);
        if (notif_pRrh.tcpDatLen != 0) {
          // Always request the data segment to be received
          rrhFsmState = RRH_SEND_DREQ;
          //remember the session ID if not yet known
          if(notif_pRrh.sessionID != *cached_tcp_rx_session_id)
          {
            addnewSessionToTable(notif_pRrh.sessionID, notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort);
          } else {
            printf("session/tripple id already in cache.\n");
          }
        } else if(notif_pRrh.tcpState == FIN_WAIT_1 || notif_pRrh.tcpState == FIN_WAIT_2
            || notif_pRrh.tcpState == CLOSING || notif_pRrh.tcpState == TIME_WAIT
            || notif_pRrh.tcpState == LAST_ACK || notif_pRrh.tcpState == CLOSED)
        {
          // we were notified about a closing connection
          deleteSessionFromTables(notif_pRrh.sessionID);
        }
      }
      break;
    case RRH_SEND_DREQ:
      //TODO increment tcpDatLen if necessary
      if(*piFMC_Tcp_data_FIFO_prog_full == 0 && *piFMC_Tcp_sessid_FIFO_prog_full == 0 )
      {
        if (!soTOE_DReq.full()) {
          soTOE_DReq.write(TcpAppRdReq(notif_pRrh.sessionID, notif_pRrh.tcpDatLen));
          rrhFsmState = RRH_WAIT_NOTIF;
        }
      }
      break;
  }
  //    }
  //  }

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
void pTcpRDp(
    stream<TcpAppData>        &siTOE_Data,
    stream<TcpAppMeta>        &siTOE_SessId,
    stream<TcpAppData>          &soFMC_Tcp_data,
    stream<TcpAppMeta>          &soFMC_Tcp_SessId,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<Ip4Addr>       &sGetNidReq_TcpRx,
    stream<NodeId>        &sGetNidRep_TcpRx,
    ap_uint<32>         *piMMIO_CfrmIp4Addr,
    const ap_uint<16>       *processed_FMC_listen_port,
    ap_uint<1>          *layer_7_enabled,
    ap_uint<1>          *role_decoupled,
    //SessionId         *cached_tcp_rx_session_id,
    bool            *expect_FMC_response,
    const bool          *nts_ready_and_enabled,
    const bool          *detected_cache_invalidation,
    stream<NalEventNotif>     &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char* myName  = concat3(THIS_NAME, "/", "Tcp_RDp");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RdpFsmStates rdpFsmState = RDP_WAIT_META;
  static ap_uint<64> cached_tcp_rx_tripple = UNUSED_TABLE_ENTRY_VALUE;
  static bool Tcp_RX_metaWritten = false;
  static SessionId cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  static NodeId own_rank = 0;
  static NodeId cached_src_id = INVALID_MRT_VALUE;
  static bool cache_init = false;

#pragma HLS RESET variable=rdpFsmState
#pragma HLS RESET variable=cached_tcp_rx_tripple
#pragma HLS RESET variable=Tcp_RX_metaWritten
#pragma HLS RESET variable=cached_tcp_rx_session_id
#pragma HLS RESET variable=cached_src_id
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=own_rank
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static SessionId session_toFMC = 0;
  static NetworkMetaStream in_meta_tcp = NetworkMetaStream(); //ATTENTION: don't forget to initialize...
  static bool first_word_written = false;
  static TcpAppData first_word = TcpAppData();
  static NodeId src_id = INVALID_MRT_VALUE;
  static ap_uint<64> tripple_in = UNUSED_TABLE_ENTRY_VALUE;
  static Ip4Addr remoteAddr = Ip4Addr();
  static TcpPort dstPort = 0x0;
  static TcpPort srcPort = 0x0;
  static bool found_in_cache = false;
  static  AppMeta     sessId = 0x0;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppData currWord;
  ap_uint<32> fmc_tcp_bytes_cnt = 0; //just temporary
  NalEventNotif new_ev_not;
  NetworkMeta tmp_meta;


  if(!*nts_ready_and_enabled)
  {
    rdpFsmState = RDP_WAIT_META;
    cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    cached_tcp_rx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    cached_src_id = INVALID_MRT_VALUE;
    cache_init = false;
  }

  if(*detected_cache_invalidation)
  {
    cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    cached_tcp_rx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    cached_src_id = INVALID_MRT_VALUE;
    cache_init = false;
  }

  if(!sConfigUpdate.empty())
  {
    NalConfigUpdate ca = sConfigUpdate.read();
    if(ca.config_addr == NAL_CONFIG_OWN_RANK)
    {
      own_rank = ca.update_value;
    }
  }

  //default actions
  *expect_FMC_response = false;

  //only if NTS is ready
  //we NEED for layer_7_enabled or role_decoupled, because the
  // 1. FMC is still active
  // 2. TCP ports cant be closed up to now [FIXME]
  // if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  //  {
  //     if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC )
  //     { //so we are not in a critical UDP path
  switch (rdpFsmState ) {

    default:
    case RDP_WAIT_META:
      if (!siTOE_SessId.empty()
          && !sGetNidReq_TcpRx.full()
         )
      {
        siTOE_SessId.read(sessId);

        tripple_in = UNUSED_TABLE_ENTRY_VALUE;
        found_in_cache = false;
        if(cache_init && sessId == cached_tcp_rx_session_id)
        {
          printf("used TCP RX tripple and NID cache.\n");
          tripple_in = cached_tcp_rx_tripple;
          src_id = cached_src_id;
          found_in_cache = true;
          rdpFsmState = RDP_FILTER_META;
        } else {
          //TODO request tripple
          rdpFsmState = RDP_W8FORREQS_1;
          cache_init = false;
          printf("[Tcp-RDP:INFO] Need to request session and node id.\n");
          break;
        }
      } else {
        break;
      }
      //NO break;
    case RDP_W8FORREQS_1:
      if(!cache_init)
      {
        if(true && !sGetNidReq_TcpRx.full())
        {
          tripple_in = getTrippleFromSessionId(sessId);
          remoteAddr = getRemoteIpAddrFromTripple(tripple_in);
          dstPort = getLocalPortFromTripple(tripple_in);
          if(dstPort != *processed_FMC_listen_port)
          {
            sGetNidReq_TcpRx.write(remoteAddr);
            printf("[TCP-RX:INFO] need to ask for Node ID.\n");
            rdpFsmState = RDP_W8FORREQS_2;
            break;
          } else {
            printf("[TCP-RX:INFO] found possible FMC connection, write to cache.\n");
            cache_init = true;
            cached_src_id = INVALID_MRT_VALUE;
            cached_tcp_rx_session_id = sessId;
            cached_tcp_rx_tripple = tripple_in;
            rdpFsmState = RDP_FILTER_META;
          }
          rdpFsmState = RDP_W8FORREQS_2;
        } else {
          break;
        }

      } else {
        rdpFsmState = RDP_W8FORREQS_2;
      }
      //NO break;
    case RDP_W8FORREQS_2:
      if(!cache_init)
      {
        if(!sGetNidRep_TcpRx.empty())
        {

          //NodeId src_id = getNodeIdFromIpAddress(remoteAddr);
          src_id = sGetNidRep_TcpRx.read();
          cache_init = true;
          cached_src_id = src_id;
          cached_tcp_rx_session_id = sessId;
          cached_tcp_rx_tripple = tripple_in;
          rdpFsmState = RDP_FILTER_META;
        } else {
          break;
        }
      } else {
        rdpFsmState = RDP_FILTER_META;
      }
      //NO break;
    case RDP_FILTER_META:
      printf("tripple_in: %llu\n",(unsigned long long) tripple_in);
      //since we requested the session, we should know it -> no error handling
      dstPort = getLocalPortFromTripple(tripple_in);
      srcPort = getRemotePortFromTripple(tripple_in);
      remoteAddr = getRemoteIpAddrFromTripple(tripple_in);
      printf("remote Addr: %d; dstPort: %d; srcPort %d\n", (int) remoteAddr, (int) dstPort, (int) srcPort);

      if(dstPort == *processed_FMC_listen_port)
      {
        if(remoteAddr == *piMMIO_CfrmIp4Addr)
        {//valid connection to FMC
          printf("found valid FMC connection.\n");
          Tcp_RX_metaWritten = false;
          session_toFMC = sessId;
          rdpFsmState = RDP_STREAM_FMC;
          //authorized_access_cnt++;
          new_ev_not = NalEventNotif(AUTH_ACCESS, 1);
          internal_event_fifo.write(new_ev_not);
          if(!found_in_cache)
          {
            markSessionAsPrivileged(sessId);
          }
          break;
        } else {
          //unauthorized_access_cnt++;
          new_ev_not = NalEventNotif(UNAUTH_ACCESS, 1);
          internal_event_fifo.write(new_ev_not);
          printf("unauthorized access to FMC!\n");
          rdpFsmState = RDP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          cache_init = false;
          break;
        }
      }

      printf("TO ROLE: src_rank: %d\n", (int) src_id);
      //Role packet
      if(src_id == INVALID_MRT_VALUE
          || *layer_7_enabled == 0 || *role_decoupled == 1)
      {
        //SINK packet
        //node_id_missmatch_RX_cnt++;
        new_ev_not = NalEventNotif(NID_MISS_RX, 1);
        internal_event_fifo.write(new_ev_not);
        rdpFsmState = RDP_DROP_PACKET;
        printf("NRC drops the packet...\n");
        cache_init = false;
        break;
      }
      //last_rx_node_id = src_id;
      new_ev_not = NalEventNotif(LAST_RX_NID, src_id);
      internal_event_fifo.write_nb(new_ev_not);
      //last_rx_port = dstPort;
      new_ev_not = NalEventNotif(LAST_RX_PORT, dstPort);
      internal_event_fifo.write_nb(new_ev_not);
      tmp_meta = NetworkMeta(own_rank, dstPort, src_id, srcPort, 0);
      in_meta_tcp = NetworkMetaStream(tmp_meta);
      Tcp_RX_metaWritten = false;
      rdpFsmState  = RDP_STREAM_ROLE;

      break;
    case RDP_STREAM_ROLE:
      if (!siTOE_Data.empty() && !soTcp_data.full())
      {
        siTOE_Data.read(currWord);
        //if (DEBUG_LEVEL & TRACE_RDP) { TODO: type management
        //  printAxiWord(myName, (AxiWord) currWord);
        //}
        NetworkWord tcpWord = currWord;
        //soTcp_data.write(currWord);
        soTcp_data.write(tcpWord);
        //printf("writing to ROLE...\n\n");
        if (currWord.getTLast() == 1)
        {
          rdpFsmState  = RDP_WAIT_META;
        }
      }
      // NO break;
    case RDP_WRITE_META_ROLE:
      if( !Tcp_RX_metaWritten && !soTcp_meta.full())
      {
        soTcp_meta.write(in_meta_tcp);
        //packet_count_RX++;
        NalEventNotif new_ev_not = NalEventNotif(PACKET_RX, 1);
        internal_event_fifo.write_nb(new_ev_not);
        Tcp_RX_metaWritten = true;
      }
      break;

    case RDP_STREAM_FMC:
      if (!siTOE_Data.empty() )
      {
        siTOE_Data.read(currWord);
        //if (DEBUG_LEVEL & TRACE_RDP) { TODO: type management
        //  printAxiWord(myName, (AxiWord) currWord);
        //}
        //blocking write, because it is a FIFO
        soFMC_Tcp_data.write(currWord);
        if (currWord.getTLast() == 1)
        {
          *expect_FMC_response = true;
          rdpFsmState  = RDP_WAIT_META;
        }
        switch (currWord.getTKeep()) {
          case 0b11111111:
            fmc_tcp_bytes_cnt = 8;
            break;
          case 0b01111111:
            fmc_tcp_bytes_cnt = 7;
            break;
          case 0b00111111:
            fmc_tcp_bytes_cnt = 6;
            break;
          case 0b00011111:
            fmc_tcp_bytes_cnt = 5;
            break;
          case 0b00001111:
            fmc_tcp_bytes_cnt = 4;
            break;
          case 0b00000111:
            fmc_tcp_bytes_cnt = 3;
            break;
          case 0b00000011:
            fmc_tcp_bytes_cnt = 2;
            break;
          default:
          case 0b00000001:
            fmc_tcp_bytes_cnt = 1;
            break;
        }
        NalEventNotif new_ev_not = NalEventNotif(FMC_TCP_BYTES, fmc_tcp_bytes_cnt);
        internal_event_fifo.write(new_ev_not);
      }
      // NO break;
    case RDP_WRITE_META_FMC:
      if( !Tcp_RX_metaWritten )
      {
        //blocking write, because it is a FIFO
        soFMC_Tcp_SessId.write(session_toFMC);
        //TODO: count incoming FMC packets?
        Tcp_RX_metaWritten = true;
      }
      break;

    case RDP_DROP_PACKET:
      if( !siTOE_Data.empty())
      {
        siTOE_Data.read(currWord);
        if (currWord.getTLast() == 1)
        {
          rdpFsmState  = RDP_WAIT_META;
          cache_init = false;
        }
      }
      break;
  }
  //    }
  // }


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
void pTcpWRp(
    stream<TcpAppData>          &siFMC_Tcp_data,
    stream<TcpAppMeta>          &siFMC_Tcp_SessId,
    stream<NetworkWord>         &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<TcpAppData>        &soTOE_Data,
    stream<TcpAppMeta>        &soTOE_SessId,
    stream<NodeId>        &sGetIpReq_TcpTx,
    stream<Ip4Addr>       &sGetIpRep_TcpTx,
    stream<Ip4Addr>       &sGetNidReq_TcpTx,
    stream<NodeId>        &sGetNidRep_TcpTx,
    const bool          *expect_FMC_response,
    ap_uint<64>         *tripple_for_new_connection,
    bool            *tcp_need_new_connection_request,
    bool            *tcp_new_connection_failure,
    const bool          *nts_ready_and_enabled,
    const bool          *detected_cache_invalidation,
    stream<NalEventNotif>     &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char* myName  = concat3(THIS_NAME, "/", "Tcp_WRp");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static WrpFsmStates wrpFsmState = WRP_WAIT_META;

  static SessionId cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  static ap_uint<64> cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
  static NodeId cached_dst_rank = INVALID_MRT_VALUE;
  static Ip4Addr cached_dst_ip_addr = 0x0;
  static bool cache_init = false;

#pragma HLS RESET variable=wrpFsmState
#pragma HLS RESET variable=cached_tcp_tx_session_id
#pragma HLS RESET variable=cached_tcp_tx_tripple
#pragma HLS RESET variable=cached_dst_rank
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=cached_dst_ip_addr
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static NetworkMetaStream out_meta_tcp = NetworkMetaStream(); //DON'T FORGET to initilize!
  static NetworkDataLength tcpTX_packet_length = 0;
  static NetworkDataLength tcpTX_current_packet_length = 0;
  static   TcpAppMeta   tcpSessId = TcpAppMeta();
  static NodeId dst_rank = INVALID_MRT_VALUE;
  static Ip4Addr dst_ip_addr = 0x0;
  static NrcPort src_port = 0x0;
  static NrcPort dst_port = 0x0;
  static ap_uint<64> new_tripple = UNUSED_TABLE_ENTRY_VALUE;
  static SessionId sessId = UNUSED_SESSION_ENTRY_VALUE;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord  currWordIn;
  //TcpAppData   currWordOut;
  NalEventNotif new_ev_not;



  if(!*nts_ready_and_enabled)
  {
    wrpFsmState = WRP_WAIT_META;
    cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    cached_dst_rank = INVALID_MRT_VALUE;
    cache_init = false;
    cached_dst_ip_addr = 0x0;
    //return;
  }

  if(*detected_cache_invalidation)
  {
    cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
    cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
    cached_dst_rank = INVALID_MRT_VALUE;
    cached_dst_ip_addr = 0x0;
    cache_init = false;
  }


  //only if NTS is ready
  //  if(*piNTS_ready == 1 && *layer_4_enabled == 1)
  //  {
  //    if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC  //so we are not in a critical UDP path
  //        && (rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE) //deactivate if we are receiving smth
  //      )
  //   {
  switch (wrpFsmState) {
    case WRP_WAIT_META:
      //FMC must come first
      if (*expect_FMC_response && !siFMC_Tcp_SessId.empty() && !soTOE_SessId.full())
      {
        tcpSessId = (AppMeta) siFMC_Tcp_SessId.read();
        soTOE_SessId.write(tcpSessId);
        //delete the session id, we don't need it any longer
        // (this expects an HTTP mode of request-response)
        //TODO
        //deleteSessionFromTables(tcpSessId);
        //*expect_FMC_response = false;
        //*detected_cache_invalidation = true;

        if (DEBUG_LEVEL & TRACE_WRP) {
          printInfo(myName, "Received new session ID #%d from [FMC].\n",
              tcpSessId.to_uint());
        }
        wrpFsmState = WRP_STREAM_FMC;
        break;
      }
      //now ask the ROLE
      if (!siTcp_meta.empty() && !soTOE_SessId.full() && !sGetIpReq_TcpTx.full())
      {
        out_meta_tcp = siTcp_meta.read();
        tcpTX_packet_length = out_meta_tcp.tdata.len;
        tcpTX_current_packet_length = 0;

        dst_rank = out_meta_tcp.tdata.dst_rank;
        if(dst_rank > MAX_CF_NODE_ID)
        {
          //node_id_missmatch_TX_cnt++;
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          internal_event_fifo.write(new_ev_not);
          //SINK packet
          wrpFsmState = WRP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          break;
        }

        if(cache_init && dst_rank == cached_dst_rank)
        {
          dst_ip_addr = cached_dst_ip_addr;
        } else {
          //need request both...
          //Ip4Addr dst_ip_addr = getIpFromRank(dst_rank);
          sGetIpReq_TcpTx.write(dst_rank);
          cache_init = false;
          wrpFsmState = WRP_W8FORREQS_1;
          break;
        }


      } else {
        break;
      }
      // NO break;

    case WRP_W8FORREQS_1:
      if(!cache_init)
      {
        if(true && !sGetIpRep_TcpTx.empty())
        {
          dst_ip_addr = sGetIpRep_TcpTx.read();
        } else {
          break;
        }

      }
      //both cases
      if(true)
      {
        //not else, in both cases
        wrpFsmState = WRP_W8FORREQS_2;
        if(dst_ip_addr == 0)
        {
          //node_id_missmatch_TX_cnt++;
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          internal_event_fifo.write(new_ev_not);
          //SINK packet
          wrpFsmState = WRP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          break;
        }
        src_port = out_meta_tcp.tdata.src_port;
        if (src_port == 0)
        {
          src_port = DEFAULT_RX_PORT;
        }
        dst_port = out_meta_tcp.tdata.dst_port;
        if (dst_port == 0)
        {
          dst_port = DEFAULT_RX_PORT;
          //port_corrections_TX_cnt++;
          new_ev_not = NalEventNotif(PCOR_TX, 1);
          internal_event_fifo.write(new_ev_not);
        }

        //check if session is present
        new_tripple = newTripple(dst_ip_addr, dst_port, src_port);
        printf("From ROLE: remote Addr: %d; dstPort: %d; srcPort %d; (rank: %d)\n", (int) dst_ip_addr, (int) dst_port, (int) src_port, (int) dst_rank);
        sessId = UNUSED_SESSION_ENTRY_VALUE;
        if(cache_init && new_tripple == cached_tcp_tx_tripple)
        {
          printf("used TCP TX tripple chache.\n");
          sessId = cached_tcp_tx_session_id;
        } else {
          //need request
          cache_init = false;
          break;
        }
      } else {
        break;
      }
      //NO break;
    case WRP_W8FORREQS_2:
      if(!cache_init )
      {
        if(true)
        {
          sessId = getSessionIdFromTripple(new_tripple);
          cached_tcp_tx_tripple = new_tripple;
          cached_tcp_tx_session_id = sessId;
          cached_dst_ip_addr = dst_ip_addr;
          cached_dst_rank = dst_rank;
          cache_init = true;
        } else {
          break;
        }
      }
      //both cases
      //"final" preprocessing
      printf("session id found: %d\n", (int) sessId);
      if(sessId == (SessionId) UNUSED_SESSION_ENTRY_VALUE)
      {//we need to create one first
        *tripple_for_new_connection = new_tripple;
        *tcp_need_new_connection_request = true;
        *tcp_new_connection_failure = false;
        wrpFsmState = WRP_WAIT_CONNECTION;
        cache_init = false;
        printf("requesting new connection.\n");
        break;
      }
      //last_tx_port = dst_port;
      //last_tx_node_id = dst_rank;
      new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
      internal_event_fifo.write_nb(new_ev_not); //TODO: blocking?
      new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
      internal_event_fifo.write_nb(new_ev_not);
      //packet_count_TX++;
      new_ev_not = NalEventNotif(PACKET_TX, 1);
      internal_event_fifo.write_nb(new_ev_not);

      soTOE_SessId.write(sessId);
      if (DEBUG_LEVEL & TRACE_WRP) {
        printInfo(myName, "Received new session ID #%d from [ROLE].\n",
            sessId.to_uint());
      }
      wrpFsmState = WRP_STREAM_ROLE;
      break;

    case WRP_WAIT_CONNECTION:
      if( !*tcp_need_new_connection_request && !soTOE_SessId.full() && !*tcp_new_connection_failure )
      {
        SessionId sessId = getSessionIdFromTripple(*tripple_for_new_connection);

        NrcPort dst_port = getRemotePortFromTripple(*tripple_for_new_connection);
        //NodeId dst_rank = getNodeIdFromIpAddress(getRemoteIpAddrFromTripple(*tripple_for_new_connection));
        //sGetNidReq_TcpTx.write(getRemoteIpAddrFromTripple(*tripple_for_new_connection));
        //NodeId dst_rank = sGetNidRep_TcpTx.read();
        //TODO new FSM states
        NalEventNotif new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        internal_event_fifo.write_nb(new_ev_not); //TODO: blocking?
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        internal_event_fifo.write_nb(new_ev_not);
        //packet_count_TX++;
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        internal_event_fifo.write_nb(new_ev_not);
        soTOE_SessId.write(sessId);
        if (DEBUG_LEVEL & TRACE_WRP) {
          printInfo(myName, "Received new session ID #%d from [ROLE].\n",
              sessId.to_uint());
        }
        wrpFsmState = WRP_STREAM_ROLE;
        cached_tcp_tx_tripple = new_tripple;
        cached_tcp_tx_session_id = sessId;
        cached_dst_ip_addr = dst_ip_addr;
        cached_dst_rank = dst_rank;
        cache_init = true;

      } else if (*tcp_new_connection_failure)
      {
        //tcp_new_connection_failure_cnt++;
        NalEventNotif new_ev_not = NalEventNotif(TCP_CON_FAIL, 1);
        internal_event_fifo.write(new_ev_not);
        // we sink the packet, because otherwise the design will hang
        // and the user is notified with the flight recorder status
        wrpFsmState = WRP_DROP_PACKET;
        printf("NRC drops the packet...\n");
        break;
      }
      break;

    case WRP_STREAM_FMC:
      if (!siFMC_Tcp_data.empty() && !soTOE_Data.full()) {
        TcpAppData tcpWord = siFMC_Tcp_data.read();
        //if (DEBUG_LEVEL & TRACE_WRP) {
        //     printAxiWord(myName, currWordIn);
        //}
        soTOE_Data.write(tcpWord);
        if(tcpWord.getTLast() == 1) {
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
        soTOE_Data.write((TcpAppData) currWordIn);
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
  // if (!siTOE_DSts.empty()) {
  //   siTOE_DSts.read();  // [TODO] Checking.
  // }
  //     }
  //   }


}


/*****************************************************************************
 * @brief Client connection to remote HOST or FPGA socket (COn).
 *
 * @param[out] soTOE_OpnReq,  open connection request to TOE.
 * @param[in]  siTOE_OpnRep,  open connection reply from TOE.
 * @param[out] soTOE_ClsReq,  close connection request to TOE.
 *
 ******************************************************************************/
void pTcpCOn(
    stream<TcpAppOpnReq>        &soTOE_OpnReq,
    stream<TcpAppOpnRep>      &siTOE_OpnRep,
    //stream<TcpAppClsReq>      &soTOE_ClsReq,
    ap_uint<64>         *tripple_for_new_connection,
    bool            *tcp_need_new_connection_request,
    bool            *tcp_new_connection_failure,
    const bool          *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
  //#pragma HLS pipeline II=1


  char *myName  = concat3(THIS_NAME, "/", "Tcp_COn");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static OpnFsmStates opnFsmState = OPN_IDLE;
  // Set a startup delay long enough to account for the initialization
  // of TOE's listen port table which takes 32,768 cycles after reset.
  //  [FIXME - StartupDelay must be replaced by a piSHELL_Reday signal]
#ifdef __SYNTHESIS_
  static ap_uint<16>         startupDelay = 0x8000;
#else
  static ap_uint<16>         startupDelay = 30;
#endif

#pragma HLS RESET variable=opnFsmState
#pragma HLS RESET variable=startupDelay
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<32>  watchDogTimer_pcon = 0;
  static TcpAppOpnReq     HostSockAddr;  // Socket Address stored in LITTLE-ENDIAN ORDER

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppOpnRep     newConn;


  if(!*nts_ready_and_enabled)
  {
    opnFsmState = OPN_IDLE;
    return; //TODO
  }

  //only if NTS is ready
  //and if we have valid tables
  // if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
  // {
  //    if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC )
  //    { //so we are not in a critical UDP path
  switch (opnFsmState) {

    case OPN_IDLE:
      if (startupDelay > 0) {
        startupDelay--;
        if (!siTOE_OpnRep.empty()) {
          // Drain any potential status data
          siTOE_OpnRep.read(newConn);
          //printInfo(myName, "Requesting to close sessionId=%d.\n", newConn.sessId.to_uint());
          //soTOE_ClsReq.write(newConn.sessId);
        }
      } else {
        //if(*tcp_need_new_connection_request)
        // {
        opnFsmState = OPN_REQ;
        //  printf("\n\tCON starting request\n");
        // } else {
        //   opnFsmState = OPN_DONE;
        // }
      }
      break;

    case OPN_REQ:
      if (*tcp_need_new_connection_request && !soTOE_OpnReq.full()) {
        Ip4Addr remoteIp = getRemoteIpAddrFromTripple(*tripple_for_new_connection);
        TcpPort remotePort = getRemotePortFromTripple(*tripple_for_new_connection);

        SockAddr    hostSockAddr(remoteIp, remotePort);
        HostSockAddr.addr = hostSockAddr.addr;
        HostSockAddr.port = hostSockAddr.port;
        soTOE_OpnReq.write(HostSockAddr);
        if (DEBUG_LEVEL & TRACE_CON) {
          printInfo(myName, "Client is requesting to connect to remote socket:\n");
          printSockAddr(myName, HostSockAddr);
        }
#ifndef __SYNTHESIS__
        watchDogTimer_pcon = 10;
#else
        watchDogTimer_pcon = NAL_CONNECTION_TIMEOUT;
#endif
        opnFsmState = OPN_REP;
      }
      break;

    case OPN_REP:
      watchDogTimer_pcon--;
      if (!siTOE_OpnRep.empty()) {
        // Read the reply stream
        siTOE_OpnRep.read(newConn);
        if (newConn.tcpState == ESTABLISHED) {
          if (DEBUG_LEVEL & TRACE_CON) {
            printInfo(myName, "Client successfully connected to remote socket:\n");
            printSockAddr(myName, HostSockAddr);
          }
          addnewTrippleToTable(newConn.sessId, *tripple_for_new_connection);
          opnFsmState = OPN_DONE;
          *tcp_need_new_connection_request = false;
          *tcp_new_connection_failure = false;
        }
        else {
          printError(myName, "Client failed to connect to remote socket:\n");
          printSockAddr(myName, HostSockAddr);
          opnFsmState = OPN_DONE;
          *tcp_need_new_connection_request = false;
          *tcp_new_connection_failure = true;
        }
      }
      else {
        if (watchDogTimer_pcon == 0) {
          if (DEBUG_LEVEL & TRACE_CON) {
            printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
            printSockAddr(myName, HostSockAddr);
          }
          *tcp_need_new_connection_request = false;
          *tcp_new_connection_failure = true;
          //the packet will be dropped, so we are done
          opnFsmState = OPN_DONE;
        }

      }
      break;
    case OPN_DONE:
      //No need to wait...
      opnFsmState = OPN_REQ;
      break;
  }
  //   }
  //  }

}

/*****************************************************************************
 * @brief Closes unused sessions (Cls).
 *
 * @param[out] soTOE_ClsReq,  close connection request to TOE.
 *
 ******************************************************************************/
void pTcpCls(
    stream<TcpAppClsReq>      &soTOE_ClsReq,
    bool            *start_tcp_cls_fsm,
    const bool            *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
  //#pragma HLS pipeline II=1


  char *myName  = concat3(THIS_NAME, "/", "Tcp_Cls");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static ClsFsmStates clsFsmState_Tcp = CLS_IDLE;

#pragma HLS RESET variable=clsFsmState_Tcp
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------


  if(!*nts_ready_and_enabled)
  {
    clsFsmState_Tcp = CLS_IDLE;
    return; //TODO
  }

  //only if NTS is ready
  //and if we have valid tables
  //  if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
  //   {
  //      if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC  //so we are not in a critical UDP path
  //          && (rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE) //deactivate if we are receiving smth
  //         && (wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE) //deactivate if we are sending
  //       )
  //      { //so we are not in a critical UDP path
  switch (clsFsmState_Tcp) {
    default:
    case CLS_IDLE:
      //we wait until we are activated;
      if(*start_tcp_cls_fsm)
      {
        clsFsmState_Tcp = CLS_NEXT;
        *start_tcp_cls_fsm = false;
      }
      break;
    case CLS_NEXT:
      if(!soTOE_ClsReq.full())
      {
        SessionId nextToDelete = getAndDeleteNextMarkedRow();
        if(nextToDelete != (SessionId) UNUSED_SESSION_ENTRY_VALUE)
        {
          soTOE_ClsReq.write(nextToDelete);
        } else {
          clsFsmState_Tcp = CLS_IDLE;
        }
      }
      break;
  }
  //    }
  //    }
}



/*! \} */

