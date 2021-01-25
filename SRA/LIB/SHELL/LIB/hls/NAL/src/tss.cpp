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
    stream<bool>        &sTcpPortsOpenFeedback
    //const bool          *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Tcp_LSn");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static LsnFsmStates lsnFsmState = LSN_IDLE;
#ifdef __SYNTHESIS_
  static ap_uint<16>         startupDelay = 0x8000;
#else
  static ap_uint<16>         startupDelay = 5;
#endif

#pragma HLS RESET variable=lsnFsmState
#pragma HLS RESET variable=startupDelay
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<8>   watchDogTimer_plisten = 0;
  ap_uint<16> new_absolute_port = 0;


  //  if(!*nts_ready_and_enabled)
  //  {
  //    lsnFsmState = LSN_IDLE;
  //#ifdef __SYNTHESIS_
  //  startupDelay = 0x8000;
  //#else
  //  startupDelay = 30;
  //#endif
  //  } else {
  //

  switch (lsnFsmState) {

    default:
    case LSN_IDLE:
      if (startupDelay > 0)
      {
        startupDelay--;
      } else {
        //if(*need_tcp_port_req == true)
        //if(!sTcpPortsToOpen.empty())
        //{
        lsnFsmState = LSN_SEND_REQ;
        //} else {
        //  lsnFsmState = LSN_DONE;
        //}
      }
      break;

    case LSN_SEND_REQ: //we arrive here only if need_tcp_port_req == true
      if (!soTOE_LsnReq.full() && !sTcpPortsToOpen.empty()) 
      {
        new_absolute_port = sTcpPortsToOpen.read();
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
      //else {
      //  printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
      //}
      break;

    case LSN_WAIT_ACK:
      if (!siTOE_LsnRep.empty() && !sTcpPortsOpenFeedback.full())
      {
        bool    listenDone;
        siTOE_LsnRep.read(listenDone);
        if (listenDone) {
          printInfo(myName, "Received listen acknowledgment from [TOE].\n");
          //lsnFsmState = LSN_DONE;
          lsnFsmState = LSN_SEND_REQ;
          sTcpPortsOpenFeedback.write(true);
        }
        else {
          printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
              (int) new_absolute_port, (int) new_absolute_port);
          sTcpPortsOpenFeedback.write(false);
          lsnFsmState = LSN_SEND_REQ;
        }
      } else if (watchDogTimer_plisten == 0 && !sTcpPortsOpenFeedback.full() )
      {
        ap_uint<16> new_absolute_port = 0;
        printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
            (int)  new_absolute_port, (int) new_absolute_port);
        sTcpPortsOpenFeedback.write(false);
        lsnFsmState = LSN_SEND_REQ;
      } else {
        watchDogTimer_plisten--;
      }
      break;

      //case LSN_DONE:
      //  if(!sTcpPortsToOpen.empty())
      //  {
      //    lsnFsmState = LSN_SEND_REQ;
      //  }
      //  break;
  } //switch
  //} //else
}

void pTcpRxNotifEnq(
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppNotif>     &sTcpNotif_buffer
    //const bool                *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------


  // if(*nts_ready_and_enabled)
  // {
  if(!siTOE_Notif.empty() && !sTcpNotif_buffer.full())
  {
    sTcpNotif_buffer.write(siTOE_Notif.read());
  }
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
    stream<NalNewTableEntry>  &sAddNewTripple_TcpRrh,
    stream<SessionId>     &sDeleteEntryBySid,
    stream<TcpAppRdReq>       &sRDp_ReqNotif,
    ap_uint<1>                *piFMC_Tcp_data_FIFO_prog_full,
    ap_uint<1>                *piFMC_Tcp_sessid_FIFO_prog_full,
    //stream<bool>                &role_fifo_full_sig,
    stream<bool>                &role_fifo_empty_sig
    //const bool          *role_fifo_empty,
    //const bool                *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Tcp_RRh");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RrhFsmStates rrhFsmState = RRH_RESET;
  static ap_uint<8>   read_request_watchDogTimer = 0;
  static bool role_fifo_still_empty = true; //after reset --> true

#pragma HLS RESET variable=rrhFsmState
#pragma HLS RESET variable=read_request_watchDogTimer
#pragma HLS RESET variable=role_fifo_still_empty
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static Cam8<SessionId,TcpDatLen> sessionLength = Cam8<SessionId,TcpDatLen>();
#ifndef __SYNTHESIS__
  if(MAX_NAL_SESSIONS != 8)
  {
    printf("\n\t\tERROR: pTcpRRh is currently configured to support only 8 parallel sessions! Abort.\n(Currently, the use of \'Cam8\' must be updated accordingly by hand.)\n");
    exit(-1);
  }
#endif

  static stream<SessionId> waitingSessions ("sTcpRRh_WaitingSessions");
  static stream<SessionId> session_reinsert ("sTcpRRh_sessions_to_reinsert");
#pragma HLS STREAM variable=waitingSessions  depth=8
#pragma HLS STREAM variable=session_reinsert  depth=8

  static TcpDatLen waiting_length = 0;
  static bool already_waiting = false;
  static TcpAppNotif notif_pRrh = TcpAppNotif();

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------


  // if(!*nts_ready_and_enabled)
  // {
  //   rrhFsmState = RRH_WAIT_NOTIF;
  //   sessionLength.reset();
  //   read_request_watchDogTimer = 0;
  //   //drain stream
  //   if(!waitingSessions.empty())
  //   {
  //     waitingSessions.read();
  //   }
  //   if(!session_reinsert.empty())
  //   {
  //     session_reinsert.read();
  //   }
  // } else {

  switch(rrhFsmState) 
  {
    default:
    case RRH_RESET:
      rrhFsmState = RRH_WAIT_NOTIF;
      sessionLength.reset();
      read_request_watchDogTimer = 0;
      //TODO drain internal streams?
      break;

    case RRH_WAIT_NOTIF:
      if(!siTOE_Notif.empty()  && !sDeleteEntryBySid.full()
          //&& !waitingSessions.full() && !sAddNewTripple_TcpRrh.full()
        )
      {
        //siTOE_Notif.read(notif_pRrh);
        siTOE_Notif.read(notif_pRrh);
        if (notif_pRrh.tcpDatLen != 0)
        {

          waiting_length = 0;
          already_waiting = sessionLength.lookup(notif_pRrh.sessionID, waiting_length);
          //if(already_waiting)
          //{
          //  //KeyValuePair<SessionId,TcpDatLen> new_KVp = KeyValuePair<SessionId,TcpDatLen>(
          //  //    notif_pRrh.sessionID, notif_pRrh.tcpDatLen+waiting_length);
          //  sessionLength.update(notif_pRrh.sessionID, notif_pRrh.tcpDatLen + waiting_length);
          //  printf("[TCP-RRH] adding %d to waiting sessions for session %d.\n",(int) notif_pRrh.tcpDatLen, (int) notif_pRrh.sessionID);

          //} else {
          //  bool found_slot = sessionLength.insert(notif_pRrh.sessionID, notif_pRrh.tcpDatLen);
          //  if(!found_slot)
          //  {
          //    //we have a problem...
          //    //but shouldn't happen actually since we have the same size as the table in TOE...
          //    printf("[TCP-RRH:PANIC] We don't have space left in the waiting table...\n");
          //    break;
          //  } else {
          //    NalNewTableEntry ne_struct = NalNewTableEntry(newTriple(notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort),
          //        notif_pRrh.sessionID);
          //    sAddNewTripple_TcpRrh.write(ne_struct);
          //    waitingSessions.write(notif_pRrh.sessionID);
          //    printf("[TCP-RRH] adding %d with %d bytes as new waiting session.\n", (int) notif_pRrh.sessionID, (int) notif_pRrh.tcpDatLen);
          //  }
          //}

          //rrhFsmState = RRH_SEND_DREQ;
          rrhFsmState = RRH_PROCESS_NOTIF;

        } else if(notif_pRrh.tcpState == FIN_WAIT_1 || notif_pRrh.tcpState == FIN_WAIT_2
            || notif_pRrh.tcpState == CLOSING || notif_pRrh.tcpState == TIME_WAIT
            || notif_pRrh.tcpState == LAST_ACK || notif_pRrh.tcpState == CLOSED)
        {
          // we were notified about a closing connection
          //deleteSessionFromTables(notif_pRrh.sessionID);
          sDeleteEntryBySid.write(notif_pRrh.sessionID);
        }
      }
      else if(!role_fifo_empty_sig.empty())
      {
        role_fifo_still_empty = role_fifo_empty_sig.read();
      }
      else if(!session_reinsert.empty() && !waitingSessions.full())
      {
        waitingSessions.write(session_reinsert.read());
        rrhFsmState = RRH_SEND_DREQ;

      } else if(!waitingSessions.empty())
      {
        rrhFsmState = RRH_SEND_DREQ;
      }

      //always
      if(read_request_watchDogTimer > 0)
      {
        //we are waiting here too
        read_request_watchDogTimer--;
      }
      break;
    case RRH_PROCESS_NOTIF:
      if( !waitingSessions.full() && !sAddNewTripple_TcpRrh.full() )
      {
        if(already_waiting)
        {
          //KeyValuePair<SessionId,TcpDatLen> new_KVp = KeyValuePair<SessionId,TcpDatLen>(
          //    notif_pRrh.sessionID, notif_pRrh.tcpDatLen+waiting_length);
          sessionLength.update(notif_pRrh.sessionID, notif_pRrh.tcpDatLen + waiting_length);
          printf("[TCP-RRH] adding %d to waiting sessions for session %d.\n",(int) notif_pRrh.tcpDatLen, (int) notif_pRrh.sessionID);

        } else {
          bool found_slot = sessionLength.insert(notif_pRrh.sessionID, notif_pRrh.tcpDatLen);
          if(!found_slot)
          {
            //we have a problem...
            //but shouldn't happen actually since we have the same size as the table in TOE...
            printf("[TCP-RRH:PANIC] We don't have space left in the waiting table...\n");
            break;
          } else {
            NalNewTableEntry ne_struct = NalNewTableEntry(newTriple(notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort),
                notif_pRrh.sessionID);
            sAddNewTripple_TcpRrh.write(ne_struct);
            waitingSessions.write(notif_pRrh.sessionID);
            printf("[TCP-RRH] adding %d with %d bytes as new waiting session.\n", (int) notif_pRrh.sessionID, (int) notif_pRrh.tcpDatLen);
          }
        }
        //in all cases
        rrhFsmState = RRH_SEND_DREQ;
      }

      //always
      if(read_request_watchDogTimer > 0)
      {
        //we are waiting here too
        read_request_watchDogTimer--;
      }
      break;
    case RRH_SEND_DREQ:
      if((*piFMC_Tcp_data_FIFO_prog_full == 0 && *piFMC_Tcp_sessid_FIFO_prog_full == 0 )
          //&& *role_fifo_empty
          //&& !role_fifo_empty_sig.empty()
          && role_fifo_still_empty
          && !soTOE_DReq.full() && !sRDp_ReqNotif.full()
          && !waitingSessions.empty() && !session_reinsert.full()
          && read_request_watchDogTimer == 0
        )
      {
        //bool role_empty = role_fifo_empty_sig.read();
        //if(!role_empty)
        //{
        //  //shouldn't happen actually since the stream is filled by ourselve
        //  printf("[TCP-RRH:PANIC] pRoleTcpRxDeq is signalling strange things...\n");
        //  break;
        //}
        SessionId found_ID = waitingSessions.read();
        TcpDatLen found_length = 0;

        bool found_smth = sessionLength.lookup(found_ID, found_length);
        if(!found_smth)
        {
          //we have a problem...
          //but shouldn't happen actually since the stream is filled by ourselve
          printf("[TCP-RRH:PANIC] We received a sessionID that is not in the CAM...\n");
          break;
        } else {
          TcpDatLen requested_length = 0;
          if(found_length > NAL_MAX_FIFO_DEPTHS_BYTES)
          {
            requested_length = NAL_MAX_FIFO_DEPTHS_BYTES;
            sessionLength.update(found_ID, found_length - NAL_MAX_FIFO_DEPTHS_BYTES);
            session_reinsert.write(found_ID);
          } else {
            requested_length = found_length;
            sessionLength.deleteEntry(found_ID);
          }
          //both cases
          printf("[TCP-RRH] requesting data for #%d with length %d\n", (int) found_ID, (int) requested_length);
          TcpAppRdReq new_req = TcpAppRdReq(found_ID, requested_length);
          soTOE_DReq.write(new_req);
          sRDp_ReqNotif.write(new_req);

          //add delay cnt to not request sessions after sessions...
          //#ifndef __SYNTHESIS__
          //            read_request_watchDogTimer = 10;
          //#else
          //            read_request_watchDogTimer = 100;
          //#endif
          //we need this delay so that the FIFO fillstands can be updated...
          read_request_watchDogTimer = (requested_length/(8*2)) + 1; //8 -> bytes/word, 2 -> by then, the fillstands should be up-to-date

        }
      } else if(read_request_watchDogTimer > 0)
      {
        read_request_watchDogTimer--;
        printf("[TCP-RRH] Waiting for watchDog (%d)\n.", read_request_watchDogTimer.to_uint());
      }
      //always
      rrhFsmState = RRH_WAIT_NOTIF;
      break;
  } //switch
  //} //else
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
    stream<TcpAppRdReq>       &sRDp_ReqNotif,
    stream<TcpAppData>        &siTOE_Data,
    stream<TcpAppMeta>        &siTOE_SessId,
    stream<TcpAppData>          &soFMC_Tcp_data,
    stream<TcpAppMeta>          &soFMC_Tcp_SessId,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<Ip4Addr>       &sGetNidReq_TcpRx,
    stream<NodeId>        &sGetNidRep_TcpRx,
    stream<SessionId>   &sGetTripleFromSid_Req,
    stream<NalTriple>   &sGetTripleFromSid_Rep,
    stream<SessionId>   &sMarkAsPriv,
    ap_uint<32>         *piMMIO_CfrmIp4Addr,
    ap_uint<16>       *piMMIO_FmcLsnPort,
    //const ap_uint<16>       *processed_FMC_listen_port,
    ap_uint<1>          *layer_7_enabled,
    ap_uint<1>          *role_decoupled,
    //SessionId         *cached_tcp_rx_session_id,
    //bool            *expect_FMC_response,
    //const bool          *nts_ready_and_enabled,
    //const bool          *detected_cache_invalidation,
    stream<bool>          &cache_inval_sig,
    stream<NalEventNotif>     &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char* myName  = concat3(THIS_NAME, "/", "Tcp_RDp");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RdpFsmStates rdpFsmState = RDP_WAIT_META;
  static ap_uint<64> cached_tcp_rx_triple = UNUSED_TABLE_ENTRY_VALUE;
  //static bool Tcp_RX_metaWritten = false;
  static SessionId cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  static NodeId own_rank = 0;
  static NodeId cached_src_id = INVALID_MRT_VALUE;
  static bool cache_init = false;
  static uint8_t evs_loop_i = 0;


#pragma HLS RESET variable=rdpFsmState
#pragma HLS RESET variable=cached_tcp_rx_triple
//#pragma HLS RESET variable=Tcp_RX_metaWritten
#pragma HLS RESET variable=cached_tcp_rx_session_id
#pragma HLS RESET variable=cached_src_id
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=own_rank
#pragma HLS RESET variable=evs_loop_i

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static SessionId session_toFMC = 0;
  static NetworkMetaStream in_meta_tcp = NetworkMetaStream(); //ATTENTION: don't forget to initialize...
  static bool first_word_written = false;
  static TcpAppData first_word = TcpAppData();
  static NodeId src_id = INVALID_MRT_VALUE;
  static ap_uint<64> triple_in = UNUSED_TABLE_ENTRY_VALUE;
  static Ip4Addr remoteAddr = Ip4Addr();
  static TcpPort dstPort = 0x0;
  static TcpPort srcPort = 0x0;
  static bool found_in_cache = false;
  static  AppMeta     sessId = 0x0;
  static NetworkDataLength current_length = 0;

  static stream<NalEventNotif> evsStreams[7];

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppData currWord;
  ap_uint<32> fmc_tcp_bytes_cnt = 0; //just temporary
  NalEventNotif new_ev_not;
  NetworkMeta tmp_meta;


  // if(!*nts_ready_and_enabled)
  // {
  //   rdpFsmState = RDP_WAIT_META;
  //   cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  //   cached_tcp_rx_triple = UNUSED_TABLE_ENTRY_VALUE;
  //   cached_src_id = INVALID_MRT_VALUE;
  //   cache_init = false;
  // } else if(*detected_cache_invalidation)
  // {
  //   cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  //   cached_tcp_rx_triple = UNUSED_TABLE_ENTRY_VALUE;
  //   cached_src_id = INVALID_MRT_VALUE;
  //   cache_init = false;
  // } else if(!sConfigUpdate.empty())
  // {
  //   NalConfigUpdate ca = sConfigUpdate.read();
  //   if(ca.config_addr == NAL_CONFIG_OWN_RANK)
  //   {
  //     own_rank = (NodeId) ca.update_value;
  //     cache_init = false;
  //   }
  // } else {


  //default actions
  // *expect_FMC_response = false;

  switch (rdpFsmState ) 
  {

    default:
    case RDP_WAIT_META:
      if(!cache_inval_sig.empty())
      {
        if(cache_inval_sig.read())
        {
          cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
          cached_tcp_rx_triple = UNUSED_TABLE_ENTRY_VALUE;
          cached_src_id = INVALID_MRT_VALUE;
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
        }
        break;
      }
      else if (!sRDp_ReqNotif.empty()
          //!siTOE_SessId.empty()
          //&& !sGetNidReq_TcpRx.full() 
          && !sGetTripleFromSid_Req.full() 
          //&& !sMarkAsPriv.full()
          )
      {
        //siTOE_SessId.read(sessId);
        TcpAppRdReq new_req = sRDp_ReqNotif.read();
        sessId = new_req.sessionID;
        current_length = new_req.length;

        triple_in = UNUSED_TABLE_ENTRY_VALUE;
        found_in_cache = false;
        if(cache_init && sessId == cached_tcp_rx_session_id)
        {
          printf("used TCP RX tripple and NID cache.\n");
          triple_in = cached_tcp_rx_triple;
          src_id = cached_src_id;
          found_in_cache = true;
          rdpFsmState = RDP_FILTER_META;
        } else {
          sGetTripleFromSid_Req.write(sessId);
          rdpFsmState = RDP_W8FORREQS_1;
          cache_init = false;
          printf("[Tcp-RDP:INFO] Need to request session and node id.\n");
          //break;
        }
      }
      //else {
        break;
      //}
      ////NO break;
    case RDP_W8FORREQS_1:
      //if(!cache_init)
      //{
        if(!sGetTripleFromSid_Rep.empty() && !sGetNidReq_TcpRx.full())
        {
          //triple_in = getTrippleFromSessionId(sessId);
          triple_in = sGetTripleFromSid_Rep.read();
          remoteAddr = getRemoteIpAddrFromTriple(triple_in);
          dstPort = getLocalPortFromTriple(triple_in);
          if(dstPort != *piMMIO_FmcLsnPort)
          {
            sGetNidReq_TcpRx.write(remoteAddr);
            printf("[TCP-RX:INFO] need to ask for Node ID.\n");
            rdpFsmState = RDP_W8FORREQS_2;
            //break;
          } else {
            printf("[TCP-RX:INFO] found possible FMC connection, write to cache.\n");
            cache_init = true;
            cached_src_id = INVALID_MRT_VALUE;
            cached_tcp_rx_session_id = sessId;
            cached_tcp_rx_triple = triple_in;
            rdpFsmState = RDP_FILTER_META;
          }
          //rdpFsmState = RDP_W8FORREQS_2;
        }
        //else {
          break;
        //}

      //} else {
        //rdpFsmState = RDP_W8FORREQS_2;
      //}
      //NO break;
    case RDP_W8FORREQS_2:
      //if(!cache_init)
      //{
        if(!sGetNidRep_TcpRx.empty())
        {

          //NodeId src_id = getNodeIdFromIpAddress(remoteAddr);
          src_id = sGetNidRep_TcpRx.read();
          cache_init = true;
          cached_src_id = src_id;
          cached_tcp_rx_session_id = sessId;
          cached_tcp_rx_triple = triple_in;
          rdpFsmState = RDP_FILTER_META;
        }
        //else {
          break;
        //}
      //} else {
      //  rdpFsmState = RDP_FILTER_META;
      //}
      ////NO break;
    case RDP_FILTER_META:
      if(!sMarkAsPriv.full() && !siTOE_SessId.empty()
        && !soTcp_meta.full() //&& !soFMC_Tcp_SessId.full()
        )
      {
        TcpAppMeta controll_id = siTOE_SessId.read();
        if(controll_id != sessId)
        {//actually, should not happen
          printf("[TCP-RDp:PANIC] We received data that we didn't expect...\n");
          //SINK packet
          //node_id_missmatch_RX_cnt++;
          //new_ev_not = NalEventNotif(NID_MISS_RX, 1);
          //internal_event_fifo.write(new_ev_not);
          //evsStreams[9].write(new_ev_not);
          rdpFsmState = RDP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          cache_init = false;
          break;
        }

        printf("tripple_in: %llu\n",(unsigned long long) triple_in);
        //since we requested the session, we should know it -> no error handling
        dstPort = getLocalPortFromTriple(triple_in);
        srcPort = getRemotePortFromTriple(triple_in);
        remoteAddr = getRemoteIpAddrFromTriple(triple_in);
        printf("remote Addr: %d; dstPort: %d; srcPort %d\n", (int) remoteAddr, (int) dstPort, (int) srcPort);

        if(dstPort == *piMMIO_FmcLsnPort)
        {
          if(remoteAddr == *piMMIO_CfrmIp4Addr)
          {//valid connection to FMC
            printf("found valid FMC connection.\n");
            //Tcp_RX_metaWritten = false;
            session_toFMC = sessId;
            //blocking write, because it is a FIFO
            soFMC_Tcp_SessId.write(session_toFMC);
            rdpFsmState = RDP_STREAM_FMC;
            //rdpFsmState = RDP_WRITE_META_FMC;
            //authorized_access_cnt++;
            new_ev_not = NalEventNotif(AUTH_ACCESS, 1);
            //internal_event_fifo.write(new_ev_not);
            evsStreams[0].write(new_ev_not);
            if(!found_in_cache)
            {
              //markSessionAsPrivileged(sessId);
              sMarkAsPriv.write(sessId);
            }
            break;
          } else {
            //unauthorized_access_cnt++;
            new_ev_not = NalEventNotif(UNAUTH_ACCESS, 1);
            //internal_event_fifo.write(new_ev_not);
            evsStreams[1].write(new_ev_not);
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
          //internal_event_fifo.write(new_ev_not);
          evsStreams[2].write(new_ev_not);
          rdpFsmState = RDP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          cache_init = false;
          break;
        }
        //last_rx_node_id = src_id;
        new_ev_not = NalEventNotif(LAST_RX_NID, src_id);
        //internal_event_fifo.write(new_ev_not);
        evsStreams[3].write(new_ev_not);
        //last_rx_port = dstPort;
        new_ev_not = NalEventNotif(LAST_RX_PORT, dstPort);
        //internal_event_fifo.write(new_ev_not);
        evsStreams[4].write(new_ev_not);
        tmp_meta = NetworkMeta(own_rank, dstPort, src_id, srcPort, current_length);
        in_meta_tcp = NetworkMetaStream(tmp_meta);
        //Tcp_RX_metaWritten = false;
        
        //write meta
        soTcp_meta.write(in_meta_tcp);
        //packet_count_RX++;
        new_ev_not = NalEventNotif(PACKET_RX, 1);
        //internal_event_fifo.write(new_ev_not);
        evsStreams[5].write(new_ev_not);
        rdpFsmState  = RDP_STREAM_ROLE;
        //rdpFsmState = RDP_WRITE_META_ROLE;
      }
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
      break;
      //// NO break;
    //case RDP_WRITE_META_ROLE:
    //  if(!soTcp_meta.full())
    //  {
    //    soTcp_meta.write(in_meta_tcp);
    //    //packet_count_RX++;
    //    new_ev_not = NalEventNotif(PACKET_RX, 1);
    //    //internal_event_fifo.write(new_ev_not);
    //    evsStreams[5].write(new_ev_not);
    //    //Tcp_RX_metaWritten = true;
    //    rdpFsmState = RDP_STREAM_ROLE;
    //  }
    //  break;
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
          //*expect_FMC_response = true;
          rdpFsmState  = RDP_WAIT_META;
        }
        fmc_tcp_bytes_cnt = extractByteCnt((Axis<64>) currWord);
        new_ev_not = NalEventNotif(FMC_TCP_BYTES, fmc_tcp_bytes_cnt);
        //internal_event_fifo.write(new_ev_not);
        evsStreams[6].write(new_ev_not);
      }
      break;
      //// NO break;
    //case RDP_WRITE_META_FMC:
    //  if(!soFMC_Tcp_SessId.full())
    //  {
    //    //blocking write, because it is a FIFO
    //    soFMC_Tcp_SessId.write(session_toFMC);
    //    //TODO: count incoming FMC packets?
    //    rdpFsmState = RDP_STREAM_FMC;
    //  }
    //  break;
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
  } // switch case

  //-- ALWAYS -----------------------
  if(!internal_event_fifo.full())
  {
    if(!evsStreams[evs_loop_i].empty())
    {
      internal_event_fifo.write(evsStreams[evs_loop_i].read());
    }
    evs_loop_i++;
    if(evs_loop_i >= 7)
    {
      evs_loop_i = 0;
    }
  }
  //} //else

}


void pRoleTcpRxDeq(
    ap_uint<1>          *layer_7_enabled,
    ap_uint<1>          *role_decoupled,
    stream<NetworkWord>       &sRoleTcpDataRx_buffer,
    stream<NetworkMetaStream>   &sRoleTcpMetaRx_buffer,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    //bool                        *role_fifo_empty
    stream<bool>                &role_fifo_empty_sig
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static DeqFsmStates deqFsmState = DEQ_SEND_NOTIF; //we start with an empty fifo...
#pragma HLS RESET variable=deqFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord cur_word = NetworkWord();
  NetworkMetaStream cur_meta = NetworkMetaStream();
  bool role_disabled = (*layer_7_enabled == 0 && *role_decoupled == 1);

  switch(deqFsmState)
  {
    default:
    case DEQ_SEND_NOTIF:
      if(!role_fifo_empty_sig.full())
      {
        role_fifo_empty_sig.write(true);
        deqFsmState = DEQ_WAIT_META;
      }
      break;
    case DEQ_WAIT_META:
      if(!sRoleTcpDataRx_buffer.empty() && !sRoleTcpMetaRx_buffer.empty()
          && !role_fifo_empty_sig.full()
          && ( (!soTcp_data.full() && !soTcp_meta.full()) ||  //user can read
            (role_disabled) //role is disabled -> drain FIFOs
            )
        )
      {
        cur_word = sRoleTcpDataRx_buffer.read();
        cur_meta = sRoleTcpMetaRx_buffer.read();
        if(!role_disabled)
        {
          soTcp_data.write(cur_word);
          soTcp_meta.write(cur_meta);
        }
        if(cur_word.tlast == 0)
        {
          deqFsmState = DEQ_STREAM_DATA;
          role_fifo_empty_sig.write(false);
        }
      }
      break;
    case DEQ_STREAM_DATA:
      if(!sRoleTcpDataRx_buffer.empty()
          && (!soTcp_data.full() || role_disabled)
        )
      {
        cur_word = sRoleTcpDataRx_buffer.read();
        if(!role_disabled)
        {
          soTcp_data.write(cur_word);
        }
        if(cur_word.tlast == 1)
        {
          //deqFsmState = DEQ_WAIT_META;
          deqFsmState = DEQ_SEND_NOTIF;
        }
      }
      break;
  }

  // -- always ------ 

  //  *role_fifo_empty = sRoleTcpDataRx_buffer.empty();

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
    stream<TcpDatLen>     &soTOE_len,
    stream<NodeId>        &sGetIpReq_TcpTx,
    stream<Ip4Addr>       &sGetIpRep_TcpTx,
    //stream<Ip4Addr>       &sGetNidReq_TcpTx,
    //stream<NodeId>        &sGetNidRep_TcpTx,
    stream<NalTriple>   &sGetSidFromTriple_Req,
    stream<SessionId>     &sGetSidFromTriple_Rep,
    stream<NalTriple>     &sNewTcpCon_Req,
    stream<NalNewTcpConRep>    &sNewTcpCon_Rep,
    //const bool          *expect_FMC_response,
    //const bool          *nts_ready_and_enabled,
    //const bool          *detected_cache_invalidation,
    stream<bool>          &cache_inval_sig,
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
  static ap_uint<64> cached_tcp_tx_triple = UNUSED_TABLE_ENTRY_VALUE;
  static NodeId cached_dst_rank = INVALID_MRT_VALUE;
  static Ip4Addr cached_dst_ip_addr = 0x0;
  static bool cache_init = false;

  //static uint8_t evs2snd_watermark = 0;
  static uint8_t evs_loop_i = 0;

#pragma HLS RESET variable=wrpFsmState
#pragma HLS RESET variable=cached_tcp_tx_session_id
#pragma HLS RESET variable=cached_tcp_tx_triple
#pragma HLS RESET variable=cached_dst_rank
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=cached_dst_ip_addr
  //#pragma HLS RESET variable=evs2snd_watermark
#pragma HLS RESET variable=evs_loop_i

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static NetworkMetaStream out_meta_tcp = NetworkMetaStream(); //DON'T FORGET to initilize!
  static NetworkDataLength tcpTX_packet_length = 0;
  static NetworkDataLength tcpTX_current_packet_length = 0;
  static TcpAppMeta   tcpSessId = TcpAppMeta();
  static NodeId dst_rank = INVALID_MRT_VALUE;
  static Ip4Addr dst_ip_addr = 0x0;
  static NrcPort src_port = 0x0;
  static NrcPort dst_port = 0x0;
  static ap_uint<64> new_triple = UNUSED_TABLE_ENTRY_VALUE;
  static SessionId sessId = UNUSED_SESSION_ENTRY_VALUE;
  static bool streaming_mode = false;

  static stream<NalEventNotif> evsStreams[10];

  //#pragma HLS dependence variable=events_to_send intra false
  //#pragma HLS dependence variable=evs2snd_valid intra false
  //#pragma HLS ARRAY_PARTITION variable=evs2snd_valid complete dim=1
  //#pragma HLS ARRAY_PARTITION variable=events_to_send complete dim=1
  //#pragma HLS RESOURCE variable=evs2snd_valid core=RAM_S2P_LUTRAM


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord  currWordIn;
  //TcpAppData   currWordOut;
  NalEventNotif new_ev_not;
  //bool skip_events_read = false;


  //  if(!*nts_ready_and_enabled)
  //  {
  //    wrpFsmState = WRP_WAIT_META;
  //    cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  //    cached_tcp_tx_triple = UNUSED_TABLE_ENTRY_VALUE;
  //    cached_dst_rank = INVALID_MRT_VALUE;
  //    cache_init = false;
  //    cached_dst_ip_addr = 0x0;
  //
  //    //    for(uint8_t i = 0; i < 11; i++)
  //    //    {
  //    //#pragma HLS pipeline off
  //    //      evs2snd_valid[i] = false;
  //    //    }
  //    //return;
  //  } else if(*detected_cache_invalidation)
  //  {
  //    cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  //    cached_tcp_tx_triple = UNUSED_TABLE_ENTRY_VALUE;
  //    cached_dst_rank = INVALID_MRT_VALUE;
  //    cached_dst_ip_addr = 0x0;
  //    cache_init = false;
  //    //return;
  //  } else
  //  {

  switch (wrpFsmState)
  {
    case WRP_WAIT_META:
      if(!cache_inval_sig.empty())
      {
        if(cache_inval_sig.read())
        {
          cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
          cached_tcp_tx_triple = UNUSED_TABLE_ENTRY_VALUE;
          cached_dst_rank = INVALID_MRT_VALUE;
          cached_dst_ip_addr = 0x0;
          cache_init = false;
        }
        break;
      }
      //else if (*expect_FMC_response && !siFMC_Tcp_SessId.empty() && !soTOE_SessId.full() && !soTOE_len.full() )
      else if (!siFMC_Tcp_SessId.empty() 
          && !soTOE_SessId.full() //&& !soTOE_len.full() 
          )
      {
        //FMC must come first
        tcpSessId = (AppMeta) siFMC_Tcp_SessId.read();
        soTOE_SessId.write(tcpSessId);
        //soTOE_len.write(0); //always streaming mode
        streaming_mode = true;
        tcpTX_current_packet_length = 0;
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
      else if (!siTcp_meta.empty()
          //&& !soTOE_SessId.full()
          && !sGetIpReq_TcpTx.full()
          //&& !soTOE_len.full() 
          )
      {
        //now ask the ROLE
        out_meta_tcp = siTcp_meta.read();
        tcpTX_packet_length = out_meta_tcp.tdata.len;
        tcpTX_current_packet_length = 0;

        dst_rank = out_meta_tcp.tdata.dst_rank;
        if(dst_rank > MAX_CF_NODE_ID)
        {
          //node_id_missmatch_TX_cnt++;
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          //internal_event_fifo.write(new_ev_not);
          //events_to_send[evs_arr_offset + 0] = NalEventNotif(NID_MISS_TX, 1);
          //evs2snd_valid[evs_arr_offset + 0] = true;
          evsStreams[0].write(new_ev_not);
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
          //internal_event_fifo.write(new_ev_not);
          //events_to_send[evs_arr_offset + 2] =  NalEventNotif(PCOR_TX, 1);
          //evs2snd_valid[evs_arr_offset + 0] = true;
          evsStreams[2].write(new_ev_not);
        }

        if(cache_init && dst_rank == cached_dst_rank)
        {
          dst_ip_addr = cached_dst_ip_addr;
          wrpFsmState = WRP_W8FORREQS_11;
        } else {
          //need request both...
          //Ip4Addr dst_ip_addr = getIpFromRank(dst_rank);
          sGetIpReq_TcpTx.write(dst_rank);
          cache_init = false;
          wrpFsmState = WRP_W8FORREQS_1;
          //break;
        }
      }
      //else {
        break;
      //}
      // NO break;

    case WRP_W8FORREQS_1:
      //if(!cache_init)
      //{
        if(!sGetIpRep_TcpTx.empty())
        {
          dst_ip_addr = sGetIpRep_TcpTx.read();
          wrpFsmState = WRP_W8FORREQS_11;
        }
        break;

      //}
      // NO break;
    case WRP_W8FORREQS_11:
      //both cases
      if(//cache_init || i
         !sGetSidFromTriple_Req.full())
      {
        //not else, in both cases
        wrpFsmState = WRP_W8FORREQS_2;
        if(dst_ip_addr == 0)
        {
          //node_id_missmatch_TX_cnt++;
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          //internal_event_fifo.write(new_ev_not);
          //events_to_send[evs_arr_offset + 1] = NalEventNotif(NID_MISS_TX, 1);
          //evs2snd_valid[evs_arr_offset + 1] = true;
          evsStreams[1].write(new_ev_not);
          //SINK packet
          wrpFsmState = WRP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          break;
        }

        //check if session is present
        new_triple = newTriple(dst_ip_addr, dst_port, src_port);
        printf("From ROLE: remote Addr: %d; dstPort: %d; srcPort %d; (rank: %d)\n", (int) dst_ip_addr, (int) dst_port, (int) src_port, (int) dst_rank);
        sessId = UNUSED_SESSION_ENTRY_VALUE;
        if(cache_init && new_triple == cached_tcp_tx_triple)
        {
          printf("used TCP TX tripple chache.\n");
          sessId = cached_tcp_tx_session_id;
          wrpFsmState = WRP_W8FORREQS_22;
        } else {
          //need request
          sGetSidFromTriple_Req.write(new_triple);
          cache_init = false;
          wrpFsmState = WRP_W8FORREQS_2;
          //break;
        }
      } //else {
        break;
      //}
      ////NO break;
    case WRP_W8FORREQS_2:
      //if(!cache_init )
      //{
        if(!sGetSidFromTriple_Rep.empty())
        {
          //sessId = getSessionIdFromTriple(new_triple);
          sessId = sGetSidFromTriple_Rep.read();
          cached_tcp_tx_triple = new_triple;
          cached_tcp_tx_session_id = sessId;
          cached_dst_ip_addr = dst_ip_addr;
          cached_dst_rank = dst_rank;
          cache_init = true;
          wrpFsmState = WRP_W8FORREQS_22;
        }
        // else {
          break;
        //}
      //}
    case WRP_W8FORREQS_22:
      if(!soTOE_SessId.full() && !sNewTcpCon_Req.full() && !soTOE_len.full() )
      {
        //both cases
        //"final" preprocessing
        printf("session id found: %d\n", (int) sessId);
        if(sessId == (SessionId) UNUSED_SESSION_ENTRY_VALUE)
        {//we need to create one first
          //*tripple_for_new_connection = new_triple;
          //*tcp_need_new_connection_request = true;
          //*tcp_new_connection_failure = false;
          sNewTcpCon_Req.write(new_triple);
          wrpFsmState = WRP_WAIT_CONNECTION;
          cache_init = false;
          printf("requesting new connection.\n");
          break;
        }
        //last_tx_port = dst_port;
        //last_tx_node_id = dst_rank;
        //events_to_send[evs_arr_offset + 3]
        new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        //evs2snd_valid[evs_arr_offset + 3] = true;
        evsStreams[3].write(new_ev_not);
        //events_to_send[evs_arr_offset + 4]
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        //evs2snd_valid[evs_arr_offset + 4] = true;
        evsStreams[4].write(new_ev_not);
        //packet_count_TX++;
        //events_to_send[evs_arr_offset + 5]
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        //evs2snd_valid[evs_arr_offset + 5] = true;
        evsStreams[5].write(new_ev_not);

        soTOE_SessId.write(sessId);
        tcpTX_current_packet_length = 0;
        if(tcpTX_packet_length == 0)
        {
          streaming_mode = true;
        } else {
          streaming_mode = false;
          soTOE_len.write(tcpTX_packet_length);
        }
        if (DEBUG_LEVEL & TRACE_WRP) {
          printInfo(myName, "Received new session ID #%d from [ROLE] with length %d.\n",
              sessId.to_uint(), tcpTX_packet_length.to_uint());
        }
        wrpFsmState = WRP_STREAM_ROLE;
      }
      break;

    case WRP_WAIT_CONNECTION:
      if( !soTOE_SessId.full() && !sNewTcpCon_Rep.empty() && !soTOE_len.full() )
      {
        //new_triple = *tripple_for_new_connection;
        //SessionId sessId = getSessionIdFromTripple(*tripple_for_new_connection);
        NalNewTcpConRep con_rep = sNewTcpCon_Rep.read();
        if(con_rep.failure == true)
        {
          new_ev_not = NalEventNotif(TCP_CON_FAIL, 1);
          //internal_event_fifo.write(new_ev_not);
          //events_to_send[evs_arr_offset + 6] = NalEventNotif(TCP_CON_FAIL, 1);
          //evs2snd_valid[evs_arr_offset + 6] = true;
          evsStreams[6].write(new_ev_not);
          // we sink the packet, because otherwise the design will hang
          // and the user is notified with the flight recorder status
          wrpFsmState = WRP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          break;
        }
        new_triple = con_rep.new_triple;
        sessId = con_rep.newSessionId;

        //NrcPort dst_port = getRemotePortFromTriple(*tripple_for_new_connection);
        //NodeId dst_rank = getNodeIdFromIpAddress(getRemoteIpAddrFromTripple(*tripple_for_new_connection));
        //sGetNidReq_TcpTx.write(getRemoteIpAddrFromTripple(*tripple_for_new_connection));
        //NodeId dst_rank = sGetNidRep_TcpTx.read();

        new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        //internal_event_fifo.write(new_ev_not); //TODO: blocking?
        //events_to_send[evs_arr_offset + 7] = NalEventNotif(LAST_TX_NID, dst_rank);
        //evs2snd_valid[evs_arr_offset + 7] = true;
        evsStreams[7].write(new_ev_not);
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        //internal_event_fifo.write(new_ev_not);
        //events_to_send[evs_arr_offset + 8] =  NalEventNotif(LAST_TX_NID, dst_rank);
        //evs2snd_valid[evs_arr_offset + 8] = true;
        evsStreams[8].write(new_ev_not);
        //packet_count_TX++;
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        //internal_event_fifo.write(new_ev_not);
        //events_to_send[evs_arr_offset + 9] =  NalEventNotif(LAST_TX_NID, dst_rank);
        //evs2snd_valid[evs_arr_offset + 9] = true;
        evsStreams[9].write(new_ev_not);
        //skip_events_read = true;

        soTOE_SessId.write(sessId);
        tcpTX_current_packet_length = 0;
        if(tcpTX_packet_length == 0)
        {
          streaming_mode = true;
        } else {
          streaming_mode = false;
          soTOE_len.write(tcpTX_packet_length);
        }
        if (DEBUG_LEVEL & TRACE_WRP) {
          printInfo(myName, "Received new session ID #%d from [ROLE].\n",
              sessId.to_uint());
        }
        wrpFsmState = WRP_STREAM_ROLE;
        cached_tcp_tx_triple = new_triple;
        cached_tcp_tx_session_id = sessId;
        cached_dst_ip_addr = dst_ip_addr;
        cached_dst_rank = dst_rank;
        cache_init = true;

      }
      break;

    case WRP_STREAM_FMC:
      if (!siFMC_Tcp_data.empty() && !soTOE_Data.full() && !soTOE_len.full() )
      {
        TcpAppData tcpWord = siFMC_Tcp_data.read();
        tcpTX_current_packet_length += extractByteCnt((Axis<64>) tcpWord);
        //if (DEBUG_LEVEL & TRACE_WRP) {
        //     printAxiWord(myName, currWordIn);
        //}
        soTOE_Data.write(tcpWord);
        if(tcpWord.getTLast() == 1) {
          wrpFsmState = WRP_WAIT_META;
          soTOE_len.write(tcpTX_current_packet_length);
        } else {
          //always streaming
          if(tcpTX_current_packet_length >= NAL_STREAMING_SPLIT_TCP)
          {
            soTOE_len.write(tcpTX_current_packet_length);
            tcpTX_current_packet_length = 0;
          }
        }
      }
      break;

    case WRP_STREAM_ROLE:
      if (!siTcp_data.empty() && !soTOE_Data.full()  && !soTOE_len.full() )
      {
        currWordIn = siTcp_data.read();
        tcpTX_current_packet_length += extractByteCnt(currWordIn);
        //if (DEBUG_LEVEL & TRACE_WRP) {
        //     printAxiWord(myName, currWordIn);
        //}
        if(!streaming_mode)
        {
          currWordIn.tlast = 0; // we ignore users tlast if the length is known
        }
        printf("streaming from ROLE to TOE: tcpTX_packet_length: %d, tcpTX_current_packet_length: %d \n", (int) tcpTX_packet_length, (int) tcpTX_current_packet_length);
        if(!streaming_mode && tcpTX_current_packet_length >= tcpTX_packet_length) //&& tcpTX_packet_length > 0
        {
          currWordIn.tlast = 1;
        }
        if(currWordIn.tlast == 1) //either by the user, or by us
        {
          wrpFsmState = WRP_WAIT_META;
          if(streaming_mode)
          {
            soTOE_len.write(tcpTX_current_packet_length);
          }
        }
        else if (streaming_mode)
        {
          if(tcpTX_current_packet_length >= NAL_STREAMING_SPLIT_TCP)
          {
            soTOE_len.write(tcpTX_current_packet_length);
            tcpTX_current_packet_length = 0;
            currWordIn.tlast = 1; //to be sure? (actually, unecessary)
          }
        }
        soTOE_Data.write((TcpAppData) currWordIn);
      }
      //else {
      //  printf("ERROR: can't stream to TOE!\n");
      //}
      break;

    case WRP_DROP_PACKET:
      if( !siTcp_data.empty())
      {
        siTcp_data.read(currWordIn);
        tcpTX_current_packet_length += extractByteCnt(currWordIn);
        //until Tlast or length
        if( (tcpTX_packet_length > 0 && tcpTX_current_packet_length >= tcpTX_packet_length)
            || (currWordIn.tlast == 1)
          )
        {
          wrpFsmState = WRP_WAIT_META;
        }
      }
      break;
  } // switch case

  //-- ALWAYS -----------------------
  if(!internal_event_fifo.full()
      //&& wrpFsmState != WRP_WAIT_CONNECTION && wrpFsmState!= WRP_W8FORREQS_2
      //&& !skip_events_read
    )
  {
    //    uint8_t read_offset = 11;
    //    if(evs_arr_offset == 11)
    //    {
    //      read_offset = 0;
    //    }
    // for(uint8_t i = 0; i < 11; i++)
    //  {
    //#pragma HLS pipeline off

    //      if(evs2snd_valid[read_offset + evs_loop_i])
    //      {
    //        internal_event_fifo.write(events_to_send[read_offset + evs_loop_i]);
    //        evs2snd_valid[read_offset + evs_loop_i] = false;
    //        //break; //one at a time
    //      }
    //      evs_loop_i++;
    //      if(evs_loop_i >= 11)
    //      {
    //        evs_loop_i = 0;
    //        if(evs_arr_offset == 0)
    //        {
    //          evs_arr_offset = 11;
    //        } else {
    //          evs_arr_offset = 0;
    //        }
    ////        for(uint8_t i = 0; i < 11; i++)
    ////        {
    ////          evs2snd_valid[i] = false;
    ////        }
    //      }
    if(!evsStreams[evs_loop_i].empty())
    {
      internal_event_fifo.write(evsStreams[evs_loop_i].read());
    }
    evs_loop_i++;
    if(evs_loop_i >= 10)
    {
      evs_loop_i = 0;
    }
    // }
  }
  //  if(evs2snd_watermark >= 6)
  //  {//a huge backlog of events, should actually not happen
  //   //(there are 3 events per packet)
  //   //better to delete all
  //    evs2snd_watermark = 0;
  //  }

  //} // else

}


/*****************************************************************************
 * @brief Write Buffer (WBu) - From WRp to TOE.
 *  Process to synchronize with TOE's TX buffer (and it's available space).
 *  In case of streaming mode (i.e. ROLE's length was 0), WRp takes care of
 *  splitting the data and writing the right len.
 *
 * @param[in]  siWrp_Data,   Tx data from [Wrp].
 * @param[in]  siWrp_SessId, the session Id from [Wrp].
 * @param[in]  siWrp_len,    the length of the current chunk (is *never* 0) from [Wrp].
 * @param[out]
 * @param[in]
 *
 ******************************************************************************/
void pTcpWBu(
    ap_uint<1>        *layer_4_enabled,
    ap_uint<1>        *piNTS_ready,
    stream<TcpAppData>        &siWrp_Data,
    stream<TcpAppMeta>        &siWrp_SessId,
    stream<TcpDatLen>     &siWrp_len,
    stream<TcpAppData>      &soTOE_Data,
    stream<TcpAppSndReq>    &soTOE_SndReq,
    stream<TcpAppSndRep>    &siTOE_SndRep
    //const bool            *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  char *myName  = concat3(THIS_NAME, "/", "Tcp_WBu");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static WbuFsmStates wbuState = WBU_WAIT_META;
  static uint16_t dequeue_cnt = 0; //in BYTES!!
  static TcpDatLen original_requested_length = 0;

#pragma HLS RESET variable=wbuState
#pragma HLS RESET variable=dequeue_cnt
#pragma HLS RESET variable=original_requested_length

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static TcpAppMeta current_sessId;
  static TcpDatLen current_requested_length = 0;
  static TcpDatLen current_approved_length = 0;
  static bool need_to_request_again = false;


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppSndReq toe_sendReq;


  //  if(!*nts_ready_and_enabled)
  //  {
  //    dequeue_cnt = 0;
  //    wbuState = WBU_WAIT_META;
  //    //drain all streams
  //    if(!siWrp_Data.empty())
  //    {
  //      siWrp_Data.read();
  //    }
  //    if(!siWrp_SessId.empty())
  //    {
  //      siWrp_SessId.read();
  //    }
  //    if(!siWrp_len.empty())
  //    {
  //      siWrp_len.read();
  //    }
  //  } else
  //  {
  switch(wbuState) {
    default:
    case WBU_WAIT_META:
      //wait for meta
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wbuState = WBU_DRAIN;
      }
      else if(!siWrp_SessId.empty() && !siWrp_len.empty())
      {
        TcpDatLen new_len = siWrp_len.read();
        current_sessId = siWrp_SessId.read();
        original_requested_length = new_len;

        current_requested_length = new_len;
        dequeue_cnt = 0;
        wbuState = WBU_SND_REQ;
      }
      break;
    case WBU_SND_REQ:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wbuState = WBU_DRAIN;
      }
      else if(!soTOE_SndReq.full())
      {
        toe_sendReq.sessId = current_sessId;
        toe_sendReq.length = current_requested_length;
        soTOE_SndReq.write(toe_sendReq);
        if (DEBUG_LEVEL & TRACE_WRP) 
        {
          printInfo(myName, "Received a data forward request from [NAL/WRP] for sessId=%d and nrBytes=%d (repeating request %d).\n",
              toe_sendReq.sessId.to_uint(), toe_sendReq.length.to_uint(), (int) need_to_request_again);
        }
        wbuState = WBU_WAIT_REP;
      }
      break;
    case WBU_WAIT_REP:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wbuState = WBU_DRAIN;
      }
      else if(!siTOE_SndRep.empty())
      {
        //-- Read the request-to-send reply and continue accordingly
        TcpAppSndRep appSndRep = siTOE_SndRep.read();
        switch (appSndRep.error) {
          case NO_ERROR:
            //fifo_fillstand = enqueue_cnt;
            wbuState = WBU_STREAM;
            current_approved_length = appSndRep.spaceLeft;
            need_to_request_again = false;
            dequeue_cnt = 0;
            break;
          case NO_SPACE:
            printWarn(myName, "Not enough space for writing %d bytes in the Tx buffer of session #%d. Available space is %d bytes.\n",
                appSndRep.length.to_uint(), appSndRep.sessId.to_uint(), appSndRep.spaceLeft.to_uint());

            dequeue_cnt = 0;
            current_approved_length = appSndRep.spaceLeft - 7; //with "security margin"
            need_to_request_again = true;

            wbuState = WBU_STREAM;
            break;
          case NO_CONNECTION:
            printWarn(myName, "Attempt to write data for a session that is not established.\n");
            //since this is after the WRP, this should never happen
            wbuState = WBU_DROP;
            dequeue_cnt = current_requested_length;
            //TODO: write internal event?
            //TODO: or ignore it, because WRP ensures this should not happen?
            break;
          default:
            printWarn(myName, "Received unknown TCP request to send reply from [TOE].\n");
            wbuState = WBU_DROP;
            dequeue_cnt = current_requested_length;
            //TODO: write internal event?
            //TODO: or ignore it, because WRP ensures this should not happen?
            break;
        }
      }
      break;
    case WBU_STREAM:
      //dequeue data
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wbuState = WBU_DRAIN;
      }
      else if(!soTOE_Data.full() && !siWrp_Data.empty() )
      {
        TcpAppData tmp = siWrp_Data.read();
        dequeue_cnt += extractByteCnt((Axis<64>) tmp);
        if(dequeue_cnt >= current_approved_length)
        {
          tmp.setTLast(1); //to be sure
          if(need_to_request_again)
          {
            current_requested_length -= dequeue_cnt;
            wbuState = WBU_SND_REQ;
          } else {
            //done
            wbuState = WBU_WAIT_META;
            printInfo(myName, "Done with packet (#%d, %d)\n",
                current_sessId.to_uint(), original_requested_length.to_uint());
          }
        } else {
          tmp.setTLast(0); //to be sure
        }
        //TODO necessary?
        //else if (!need_to_request_again && tmp.getTLast() == 1)
        //{
        //    //done
        //    wbuState = WBU_WAIT_META;
        //}
        soTOE_Data.write(tmp);
      }
      break;
    case WBU_DROP:
      //TODO: delete, because WRP ensures this should not happen?
      if(!siWrp_Data.empty())
      {
        TcpAppData tmp = siWrp_Data.read();
        if(tmp.getTLast() == 1)
        {
          //dequeue_cnt = 0;
          wbuState = WBU_WAIT_META;
        } else {
          dequeue_cnt -= extractByteCnt((Axis<64>) tmp);
          if(dequeue_cnt == 0)
          {
            wbuState = WBU_WAIT_META;
          }
        }
      }
      break;
    case WBU_DRAIN:
      //drain all streams
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        if(!siWrp_Data.empty())
        {
          siWrp_Data.read();
        }
        if(!siWrp_SessId.empty())
        {
          siWrp_SessId.read();
        }
        if(!siWrp_len.empty())
        {
          siWrp_len.read();
        }
      } else {
        wbuState = WBU_WAIT_META;
      }
      break;
  } //switch
  //} //else
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
    stream<NalNewTableEntry>   &sAddNewTriple_TcpCon,
    stream<NalTriple>     &sNewTcpCon_Req,
    stream<NalNewTcpConRep>    &sNewTcpCon_Rep
    //const bool          *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


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
  static NalTriple         triple_for_new_connection = 0x0;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppOpnRep     newConn;


  //  if(!*nts_ready_and_enabled)
  //  {
  //    opnFsmState = OPN_IDLE;
  //    //return;
  //  } else
  //  {

  switch (opnFsmState)
  {

    case OPN_IDLE:
      if (startupDelay > 0) 
      {
        startupDelay--;
        //Drain any potential status data
        //TODO?
        //if (!siTOE_OpnRep.empty()) 
        //{
        //  siTOE_OpnRep.read(newConn);
        //  //printInfo(myName, "Requesting to close sessionId=%d.\n", newConn.sessId.to_uint());
        //  //soTOE_ClsReq.write(newConn.sessId);
        //}
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
      if (!sNewTcpCon_Req.empty() && !soTOE_OpnReq.full())
      {
        triple_for_new_connection = sNewTcpCon_Req.read();
        Ip4Addr remoteIp = getRemoteIpAddrFromTriple(triple_for_new_connection);
        TcpPort remotePort = getRemotePortFromTriple(triple_for_new_connection);

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
      if(!sAddNewTriple_TcpCon.full() && !sNewTcpCon_Rep.full())
      {
        watchDogTimer_pcon--;
        if (!siTOE_OpnRep.empty())
        {
          // Read the reply stream
          siTOE_OpnRep.read(newConn);
          if (newConn.tcpState == ESTABLISHED) {
            if (DEBUG_LEVEL & TRACE_CON) {
              printInfo(myName, "Client successfully connected to remote socket:\n");
              printSockAddr(myName, HostSockAddr);
            }
            //addnewTrippleToTable(newConn.sessId, *tripple_for_new_connection);
            NalNewTableEntry ne_struct =  NalNewTableEntry(triple_for_new_connection, newConn.sessId);
            sAddNewTriple_TcpCon.write(ne_struct);
            opnFsmState = OPN_DONE;
            //*tcp_need_new_connection_request = false;
            //*tcp_new_connection_failure = false;
            NalNewTcpConRep con_rep = NalNewTcpConRep(triple_for_new_connection, newConn.sessId, false);
            sNewTcpCon_Rep.write(con_rep);
          }
          else {
            printError(myName, "Client failed to connect to remote socket:\n");
            printSockAddr(myName, HostSockAddr);
            opnFsmState = OPN_DONE;
            //*tcp_need_new_connection_request = false;
            //*tcp_new_connection_failure = true;
            NalNewTcpConRep con_rep = NalNewTcpConRep(triple_for_new_connection, UNUSED_SESSION_ENTRY_VALUE, true);
            sNewTcpCon_Rep.write(con_rep);
          }
        }
        else {
          if (watchDogTimer_pcon == 0) {
            if (DEBUG_LEVEL & TRACE_CON) {
              printError(myName, "Timeout: Failed to connect to the following remote socket:\n");
              printSockAddr(myName, HostSockAddr);
            }
            //*tcp_need_new_connection_request = false;
            //*tcp_new_connection_failure = true;
            //the packet will be dropped, so we are done
            opnFsmState = OPN_DONE;
            NalNewTcpConRep con_rep = NalNewTcpConRep(triple_for_new_connection, UNUSED_SESSION_ENTRY_VALUE, true);
            sNewTcpCon_Rep.write(con_rep);
          }

        }
      }
      break;
    case OPN_DONE:
      //No need to wait...
      opnFsmState = OPN_REQ;
      break;
  } //switch
  //} //else 
}

/*****************************************************************************
 * @brief Closes unused sessions (Cls).
 *
 * @param[out] soTOE_ClsReq,  close connection request to TOE.
 *
 ******************************************************************************/
void pTcpCls(
    stream<TcpAppClsReq>      &soTOE_ClsReq,
    stream<bool>              &sGetNextDelRow_Req,
    stream<SessionId>         &sGetNextDelRow_Rep,
    //const bool            *start_tcp_cls_fsm,
    stream<bool>              &sStartTclCls
    //const bool            *nts_ready_and_enabled
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char *myName  = concat3(THIS_NAME, "/", "Tcp_Cls");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static ClsFsmStates clsFsmState_Tcp = CLS_IDLE;

#pragma HLS RESET variable=clsFsmState_Tcp
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------


  // if(!*nts_ready_and_enabled)
  // {
  //   clsFsmState_Tcp = CLS_IDLE;
  // } else {

  switch (clsFsmState_Tcp) {
    default:
    case CLS_IDLE:
      //we wait until we are activated;
      //if(*start_tcp_cls_fsm)
      if(!sStartTclCls.empty())
      {
        if(sStartTclCls.read())
        {
          clsFsmState_Tcp = CLS_NEXT;
        }
        //*start_tcp_cls_fsm = false;
      }
      break;
    case CLS_NEXT:
      if(!sGetNextDelRow_Req.full())
      {
        sGetNextDelRow_Req.write(true);
        clsFsmState_Tcp = CLS_WAIT4RESP;
      }
      break;
    case CLS_WAIT4RESP:
      if(!soTOE_ClsReq.full() && !sGetNextDelRow_Rep.empty())
      {
        //SessionId nextToDelete = getAndDeleteNextMarkedRow();
        SessionId nextToDelete = sGetNextDelRow_Rep.read();
        if(nextToDelete != (SessionId) UNUSED_SESSION_ENTRY_VALUE)
        {
          soTOE_ClsReq.write(nextToDelete);
          clsFsmState_Tcp = CLS_NEXT;
        } else {
          clsFsmState_Tcp = CLS_IDLE;
        }
      }
      break;
  }
  // }
}



/*! \} */

