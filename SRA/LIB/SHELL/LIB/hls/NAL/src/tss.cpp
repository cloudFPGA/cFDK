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
 * @param[out]  soTOE_LsnReq,            listen port request to TOE.
 * @param[in]   siTOE_LsnRep,            listen port acknowledge from TOE.
 * @param[in]   sTcpPortsToOpen,         port open request from pPortLogic
 * @param[out]  sTcpPortsOpenFeedback,   feedback to pPortLogic
 *
 * @warning
 *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
 *   32,767) which are used for listening ports, and one for dynamically
 *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
 *   connections. Therefore, listening port numbers must be in the range 0
 *   to 32,767.
 *
 ******************************************************************************/
void pTcpLsn(
    stream<TcpAppLsnReq>      &soTOE_LsnReq,
    stream<TcpAppLsnRep>      &siTOE_LsnRep,
    stream<TcpPort>           &sTcpPortsToOpen,
    stream<bool>              &sTcpPortsOpenFeedback
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

  } //switch
}



/*****************************************************************************
 * @brief Enqueus the incoming notificiations from TOE into the internal buffer.
 *
 ******************************************************************************/
void pTcpRxNotifEnq(
    ap_uint<1>              *layer_4_enabled,
    ap_uint<1>              *piNTS_ready,
    stream<TcpAppNotif>     &siTOE_Notif,
    stream<TcpAppNotif>     &sTcpNotif_buffer
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RrhEnqFsmStates rrhEngFsm = RRH_ENQ_RESET;
#pragma HLS RESET variable=rrhEngFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  //This FSM is more or less senseless, but without Vivado HLS removes the 'sTcpNotif_buffer'

  switch(rrhEngFsm)
  {
    default:
    case RRH_ENQ_RESET:
      if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        rrhEngFsm = RRH_ENQ_STREAM;
      }
      else if(!siTOE_Notif.empty())
      {
        siTOE_Notif.read();
      }
      break;
    case RRH_ENQ_STREAM:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rrhEngFsm = RRH_ENQ_RESET;
      }
      else if(!siTOE_Notif.empty() && !sTcpNotif_buffer.full())
      {
        TcpAppNotif new_notif = siTOE_Notif.read();
        sTcpNotif_buffer.write(new_notif);
      }
      break;
  }

}



/*****************************************************************************
 * @brief ReadRequestHandler (RRh).
 *  Waits for a notification indicating the availability of new data for
 *  the ROLE or FMC.
 *
 * @param[in]   layer_4_enabled,          external signal if layer 4 is enabled
 * @param[in]   piNTS_ready,              external signal if NTS is up and running
 * @param[in]   piMMIO_CfrmIp4Addr,       the IP address of the CFRM (from MMIO)
 * @param[in]   piMMIO_FmcLsnPort,        the management listening port (from MMIO)
 * @param[in]   siTOE_Notif,              a new Rx data notification from TOE.
 * @param[out]  soTOE_DReq,               a Rx data request to TOE.
 * @param[out]  sAddNewTriple_TcpRrh,     Notification for the TCP Agency to add a new Triple/SessionId pair
 * @param[out]  sMarkAsPriv,              Notification for the TCP Agency to mark a connection as privileged
 * @param[out]  sDeleteEntryBySid,        Notifies the TCP Agency of the closing of a connection
 * @param[out]  sRDp_ReqNotif,            Notifies pTcpRDp about a n incoming TCP chunk
 * @param[in]   fmc_write_cnt_sig,        Signal from pFmcTcpRxDeq about how many bytes are written
 * @param[in]   role_write_cnt_sig,       Signal from pRoleTcpRxDeq about how many bytes are written
 *
 ******************************************************************************/
void pTcpRRh(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *piNTS_ready,
    ap_uint<32>               *piMMIO_CfrmIp4Addr,
    ap_uint<16>               *piMMIO_FmcLsnPort,
    stream<TcpAppNotif>       &siTOE_Notif,
    stream<TcpAppRdReq>       &soTOE_DReq,
    stream<NalNewTableEntry>  &sAddNewTriple_TcpRrh,
    stream<SessionId>         &sMarkAsPriv,
    stream<SessionId>         &sDeleteEntryBySid,
    stream<TcpAppRdReq>       &sRDp_ReqNotif,
    stream<PacketLen>         &fmc_write_cnt_sig,
    stream<PacketLen>         &role_write_cnt_sig
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  char* myName  = concat3(THIS_NAME, "/", "Tcp_RRh");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RrhFsmStates rrhFsmState = RRH_RESET;

#pragma HLS RESET variable=rrhFsmState

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static Cam8<SessionId,TcpDatLen> sessionLength = Cam8<SessionId,TcpDatLen>();
#ifndef __SYNTHESIS__
  if(MAX_NAL_SESSIONS != 8)
  {
    printf("\n\t\tERROR: pTcpRRh is currently configured to support only 8 parallel sessions! Abort.\n(Currently, the use of \'Cam8\' must be updated accordingly by hand.)\n");
    exit(-1);
  }
#endif

  static stream<NalWaitingData> waitingSessions ("sTcpRRh_WaitingSessions");
  static stream<NalWaitingData> session_reinsert ("sTcpRRh_sessions_to_reinsert");
#pragma HLS STREAM variable=waitingSessions  depth=8
#pragma HLS STREAM variable=session_reinsert  depth=8

  static TcpDatLen waiting_length = 0;
  static TcpAppNotif notif_pRrh = TcpAppNotif();
  static bool already_waiting = false;
  static TcpDatLen requested_length = 0;
  static TcpDatLen length_update_value = 0;
  static SessionId found_ID = 0;
  static bool found_fmc_sess = false;
  static bool need_cam_update = false;
  static bool go_back_to_ack_wait_role = false;
  static bool go_back_to_ack_wait_fmc = false;

  static PacketLen role_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;
  static PacketLen fmc_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  switch(rrhFsmState)
  {
    default:
    case RRH_RESET:
      sessionLength.reset();
      go_back_to_ack_wait_fmc = false;
      go_back_to_ack_wait_role = false;
      role_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;
      fmc_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;
      rrhFsmState = RRH_WAIT_NOTIF;
      break;
    case RRH_WAIT_NOTIF:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rrhFsmState = RRH_DRAIN;
      }
      else if(!siTOE_Notif.empty()  && !sDeleteEntryBySid.full()
          )
      {
        siTOE_Notif.read(notif_pRrh);
        if (notif_pRrh.tcpDatLen != 0)
        {

          //waiting_length = 0;
          already_waiting = sessionLength.lookup(notif_pRrh.sessionID, waiting_length);
          rrhFsmState = RRH_PROCESS_NOTIF;

        } else if(notif_pRrh.tcpState == FIN_WAIT_1 || notif_pRrh.tcpState == FIN_WAIT_2
            || notif_pRrh.tcpState == CLOSING || notif_pRrh.tcpState == TIME_WAIT
            || notif_pRrh.tcpState == LAST_ACK || notif_pRrh.tcpState == CLOSED)
        {
          // we were notified about a closing connection
          sDeleteEntryBySid.write(notif_pRrh.sessionID);
          if(go_back_to_ack_wait_fmc)
          {
            rrhFsmState = RRH_WAIT_FMC;
          }
          else if(go_back_to_ack_wait_role)
          {
            rrhFsmState = RRH_WAIT_ROLE;
          }
          //else, stay here...
        }
      }
      else if(!session_reinsert.empty() && !waitingSessions.full()
          && !go_back_to_ack_wait_fmc && !go_back_to_ack_wait_role
          )
      {
        waitingSessions.write(session_reinsert.read());
        rrhFsmState = RRH_START_REQUEST;
      }
      else if(!waitingSessions.empty()
          && !go_back_to_ack_wait_fmc && !go_back_to_ack_wait_role
          )
      {
        rrhFsmState = RRH_START_REQUEST;
      }
      break;
    case RRH_PROCESS_NOTIF:
      if(!waitingSessions.full() && !sAddNewTriple_TcpRrh.full()
          && !sMarkAsPriv.full()
        )
      {
        if(already_waiting)
        {
          sessionLength.update(notif_pRrh.sessionID, notif_pRrh.tcpDatLen + waiting_length);
          printf("[TCP-RRH] adding %d to waiting sessions for session %d.\n",(int) notif_pRrh.tcpDatLen, (int) notif_pRrh.sessionID);

        } else {
          sessionLength.insert(notif_pRrh.sessionID, notif_pRrh.tcpDatLen);
          //bool found_slot = sessionLength.insert(notif_pRrh.sessionID, notif_pRrh.tcpDatLen);
          //if(!found_slot)
          //{
          //  //we have a problem...
          //  //but shouldn't happen actually since we have the same size as the table in TOE...
          //  printf("[TCP-RRH:PANIC] We don't have space left in the waiting table...\n");
          //  break;
          //} else {
          NalNewTableEntry ne_struct = NalNewTableEntry(newTriple(notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort),
              notif_pRrh.sessionID);
          sAddNewTriple_TcpRrh.write(ne_struct);
          bool is_fmc = false;
          if(notif_pRrh.ip4SrcAddr == *piMMIO_CfrmIp4Addr && notif_pRrh.tcpDstPort == *piMMIO_FmcLsnPort)
          {
            is_fmc = true;
            sMarkAsPriv.write(notif_pRrh.sessionID);
          }
          NalWaitingData new_sess = NalWaitingData(notif_pRrh.sessionID, is_fmc);
          waitingSessions.write(new_sess);
          printf("[TCP-RRH] adding %d with %d bytes as new waiting session.\n", (int) notif_pRrh.sessionID, (int) notif_pRrh.tcpDatLen);
          //}
        }
        if(go_back_to_ack_wait_fmc)
        {
          rrhFsmState = RRH_WAIT_FMC;
        }
        else if(go_back_to_ack_wait_role)
        {
          rrhFsmState = RRH_WAIT_ROLE;
        } else {
          rrhFsmState = RRH_START_REQUEST;
        }
      }
      break;
    case RRH_START_REQUEST:
      if(!waitingSessions.empty() && !session_reinsert.full()
          //&& role_fifo_free_cnt > 0
          //&& fmc_fifo_free_cnt > 0
          //if we are here, we ensured there is space
        )
      {
        NalWaitingData new_data = waitingSessions.read();
        found_ID = new_data.sessId;
        TcpDatLen found_length = 0;
        found_fmc_sess = new_data.fmc_con;

        sessionLength.lookup(found_ID, found_length);
        //bool found_smth = sessionLength.lookup(found_ID, found_length);
        //if(!found_smth)
        //{
        //  //we have a problem...
        //  //but shouldn't happen actually since the stream is filled by ourselve
        //  printf("[TCP-RRH:PANIC] We received a sessionID that is not in the CAM...\n");
        //} else {
        //requested_length = 0;
        if(found_fmc_sess)
        {
          if(found_length >= fmc_fifo_free_cnt)
          {
            need_cam_update = true;
            requested_length = fmc_fifo_free_cnt;
            length_update_value = found_length - fmc_fifo_free_cnt;
          } else {
            need_cam_update = false;
            requested_length = found_length;
          }
        } else {
          if(found_length >= role_fifo_free_cnt)
          {
            need_cam_update = true;
            requested_length = role_fifo_free_cnt;
            length_update_value = found_length - role_fifo_free_cnt;
          } else {
            need_cam_update = false;
            requested_length = found_length;
          }
        }
        ////both cases
        rrhFsmState = RRH_PROCESS_REQUEST;
        //}
      }
      //else if(both_fifo_free_cnt == 0)
      //{
      //  rrhFsmState = RRH_WAIT_WRITE_ACK;
      //}
      break;
    case RRH_PROCESS_REQUEST:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rrhFsmState = RRH_DRAIN;
      }
      else if(!soTOE_DReq.full() && !sRDp_ReqNotif.full()
          && !session_reinsert.full()
          )
      {
        if(need_cam_update)
        {
          sessionLength.update(found_ID, length_update_value);
          session_reinsert.write(NalWaitingData(found_ID, found_fmc_sess));
          if(found_fmc_sess)
          {
            fmc_fifo_free_cnt = 0;
          } else {
            role_fifo_free_cnt = 0;
          }
        } else {
          sessionLength.deleteEntry(found_ID);
          if(found_fmc_sess)
          {
            fmc_fifo_free_cnt -= requested_length;
          } else {
            role_fifo_free_cnt -= requested_length;
          }
        }
        printf("[TCP-RRH] requesting data for #%d with length %d (FMC: %d)\n", (int) found_ID, (int) requested_length, (int) found_fmc_sess);
        TcpAppRdReq new_req = TcpAppRdReq(found_ID, requested_length);
        soTOE_DReq.write(new_req);
        sRDp_ReqNotif.write(new_req);
        go_back_to_ack_wait_fmc  = false;
        go_back_to_ack_wait_role = false;
        if(found_fmc_sess)
        {
          rrhFsmState = RRH_WAIT_FMC;
        } else {
          rrhFsmState = RRH_WAIT_ROLE;
        }
      }
      break;
    case RRH_WAIT_FMC:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rrhFsmState = RRH_DRAIN;
      }
      else if(!fmc_write_cnt_sig.empty())
      {
        PacketLen fmc_new_free = fmc_write_cnt_sig.read();
        fmc_fifo_free_cnt += fmc_new_free;
        if(fmc_fifo_free_cnt > NAL_MAX_FIFO_DEPTHS_BYTES)
        {
          //to be sure
          fmc_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;
        }
        printf("[TCP-RRH] FMC FIFO completed write of %d bytes; In total %d bytes are free in FMC FIFO.\n", (int) fmc_new_free, (int) fmc_fifo_free_cnt);
        go_back_to_ack_wait_fmc = false;
        rrhFsmState = RRH_WAIT_NOTIF;
      }
      else if(!siTOE_Notif.empty())
      {
        rrhFsmState = RRH_WAIT_NOTIF;
        go_back_to_ack_wait_fmc = true;
      }
      break;
    case RRH_WAIT_ROLE:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rrhFsmState = RRH_DRAIN;
      }
      else if(!role_write_cnt_sig.empty())
      {
        PacketLen role_new_free = role_write_cnt_sig.read();
        role_fifo_free_cnt += role_new_free;
        if(role_fifo_free_cnt > NAL_MAX_FIFO_DEPTHS_BYTES)
        {
          //to be sure
          role_fifo_free_cnt = NAL_MAX_FIFO_DEPTHS_BYTES;
        }
        printf("[TCP-RRH] ROLE FIFO completed write of %d bytes; In total %d bytes are free in ROLE FIFO.\n", (int) role_new_free, (int) role_fifo_free_cnt);
        go_back_to_ack_wait_role = false;
        rrhFsmState = RRH_WAIT_NOTIF;
      }
      else if(!siTOE_Notif.empty())
      {
        rrhFsmState = RRH_WAIT_NOTIF;
        go_back_to_ack_wait_role = true;
      }
      break;
    case RRH_DRAIN:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        if(!siTOE_Notif.empty())
        {
          siTOE_Notif.read();
        }
        if(!waitingSessions.empty())
        {
          waitingSessions.read();
        }
        if(!session_reinsert.empty())
        {
          session_reinsert.read();
        }
        if(!fmc_write_cnt_sig.empty())
        {
          fmc_write_cnt_sig.read();
        }
        if(!role_write_cnt_sig.empty())
        {
          role_write_cnt_sig.read();
        }
      } else {
        rrhFsmState = RRH_RESET;
      }
      break;
  }

}



/*****************************************************************************
 * @brief Read Path (RDp) - From TOE to ROLE or FMC.
 *  Process waits for a new data segment to read and forwards it to ROLE or FMC.
 *  Invalid packets are dropped.
 *
 * @param[in]   layer_4_enabled,          external signal if layer 4 is enabled
 * @param[in]   piNTS_ready,              external signal if NTS is up and running
 * @param[in]   sRDp_ReqNotif,            Notifies pTcpRDp about a n incoming TCP chunk
 * @param[in]   siTOE_Data,               Data from [TOE].
 * @param[in]   siTOE_SessId,             Session Id from [TOE].
 * @param[out]  soFmc_data,               Data for FMC
 * @param[out]  soFmc_meta,               Metadata for FMC
 * @param[out]  soTcp_data,               Data to [ROLE].
 * @param[out]  soTcp_meta,               Meta Data to [ROLE].
 * @param[in]   sConfigUpdate,            notification of configuration changes
 * @param[out]  sGetNidReq_TcpRx,         Request stream for the the MRT Agency
 * @param[in]   sGetNidRep_TcpRx,         Reply stream from the MRT Agency
 * @param[out]  &sGetTripleFromSid_Req,   Request stream for the the TCP Agency
 * @param[in]   &sGetTripleFromSid_Rep,   Reply stream from the TCO Agency
 * @param[in]   piMMIO_CfrmIp4Addr,       the IP address of the CFRM (from MMIO)
 * @param[in]   piMMIO_FmcLsnPort,        the management listening port (from MMIO)
 * @param[in]   layer_7_enabled,          external signal if layer 7 is enabled
 * @param[in]   role_decoupled,           external signal if the role is decoupled
 * @param[in]   cache_inval_sig,          Signal from the Cache Invalidation Logic
 * @param[out]  internal_event_fifo,      Fifo for event reporting
 *
 *****************************************************************************/
void pTcpRDp(
    ap_uint<1>                  *layer_4_enabled,
    ap_uint<1>                  *piNTS_ready,
    stream<TcpAppRdReq>         &sRDp_ReqNotif,
    stream<TcpAppData>          &siTOE_Data,
    stream<TcpAppMeta>          &siTOE_SessId,
    stream<TcpAppData>          &soFMC_data,
    stream<TcpAppMeta>          &soFMC_SessId,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    stream<NalConfigUpdate>     &sConfigUpdate,
    stream<Ip4Addr>             &sGetNidReq_TcpRx,
    stream<NodeId>              &sGetNidRep_TcpRx,
    stream<SessionId>           &sGetTripleFromSid_Req,
    stream<NalTriple>           &sGetTripleFromSid_Rep,
    //stream<SessionId>         &sMarkAsPriv,
    ap_uint<32>                 *piMMIO_CfrmIp4Addr,
    ap_uint<16>                 *piMMIO_FmcLsnPort,
    ap_uint<1>                  *layer_7_enabled,
    ap_uint<1>                  *role_decoupled,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char* myName  = concat3(THIS_NAME, "/", "Tcp_RDp");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static RdpFsmStates rdpFsmState = RDP_RESET;
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
  static ap_uint<32> fmc_tcp_bytes_cnt = 0;

  static stream<NalEventNotif> evsStreams[7];

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppData currWord;
  NalEventNotif new_ev_not;
  NetworkMeta tmp_meta;


  switch (rdpFsmState)
  {

    default:
    case RDP_RESET:
      if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        cache_init = false;
        fmc_tcp_bytes_cnt = 0;
        current_length = 0;
        rdpFsmState = RDP_WAIT_META;
      } else {
        if(!sRDp_ReqNotif.empty() )
        {
          sRDp_ReqNotif.read();
        }
        if(!siTOE_SessId.empty())
        {
          siTOE_SessId.read();
        }
        if(!siTOE_Data.empty())
        {
          siTOE_Data.read();
        }
      }
      break;
    case RDP_WAIT_META:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rdpFsmState = RDP_RESET;
      }
      else if(!cache_inval_sig.empty())
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
        }
      }
      break;

    case RDP_W8FORREQS_1:
      if(!sGetTripleFromSid_Rep.empty() && !sGetNidReq_TcpRx.full())
      {
        triple_in = sGetTripleFromSid_Rep.read();
        remoteAddr = getRemoteIpAddrFromTriple(triple_in);
        dstPort = getLocalPortFromTriple(triple_in);
        if(dstPort != *piMMIO_FmcLsnPort)
        {
          sGetNidReq_TcpRx.write(remoteAddr);
          printf("[TCP-RX:INFO] need to ask for Node ID.\n");
          rdpFsmState = RDP_W8FORREQS_2;
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
      break;

    case RDP_W8FORREQS_2:
      if(!sGetNidRep_TcpRx.empty())
      {

        src_id = sGetNidRep_TcpRx.read();
        cache_init = true;
        cached_src_id = src_id;
        cached_tcp_rx_session_id = sessId;
        cached_tcp_rx_triple = triple_in;
        rdpFsmState = RDP_FILTER_META;
      }
      break;

    case RDP_FILTER_META:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rdpFsmState = RDP_RESET;
      }
      else if(//!sMarkAsPriv.full() && 
          !siTOE_SessId.empty()
          && !soTcp_meta.full() && !soFMC_SessId.full()
          )
      {
        TcpAppMeta controll_id = siTOE_SessId.read();
        if(controll_id != sessId)
        {//actually, should not happen
          printf("[TCP-RDp:PANIC] We received data that we didn't expect...\n");
          //SINK packet
          //new_ev_not = NalEventNotif(NID_MISS_RX, 1);
          //evsStreams[9].write_nb(new_ev_not);
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
            session_toFMC = sessId;
            soFMC_SessId.write(session_toFMC);
            fmc_tcp_bytes_cnt = 0;
            rdpFsmState = RDP_STREAM_FMC;
            new_ev_not = NalEventNotif(AUTH_ACCESS, 1);
            evsStreams[0].write_nb(new_ev_not);
            //done by RRh
            //if(!found_in_cache)
            //{
            //  //markSessionAsPrivileged(sessId);
            //  sMarkAsPriv.write(sessId);
            //}
            break;
          } else {
            new_ev_not = NalEventNotif(UNAUTH_ACCESS, 1);
            evsStreams[1].write_nb(new_ev_not);
            //We won't miss this one
            //update: we aren't since every event has it's own "slot"
            //evsStreams[1].write(new_ev_not);
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
          new_ev_not = NalEventNotif(NID_MISS_RX, 1);
          evsStreams[2].write_nb(new_ev_not);
          rdpFsmState = RDP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          cache_init = false;
          break;
        }
        new_ev_not = NalEventNotif(LAST_RX_NID, src_id);
        evsStreams[3].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(LAST_RX_PORT, dstPort);
        evsStreams[4].write_nb(new_ev_not);
        tmp_meta = NetworkMeta(own_rank, dstPort, src_id, srcPort, current_length);
        in_meta_tcp = NetworkMetaStream(tmp_meta);

        //write meta
        soTcp_meta.write(in_meta_tcp);
        new_ev_not = NalEventNotif(PACKET_RX, 1);
        evsStreams[5].write_nb(new_ev_not);
        rdpFsmState  = RDP_STREAM_ROLE;
      }
      break;
    case RDP_STREAM_ROLE:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rdpFsmState = RDP_RESET;
      }
      else if (!siTOE_Data.empty() && !soTcp_data.full())
      {
        siTOE_Data.read(currWord);
        NetworkWord tcpWord = currWord;
        soTcp_data.write(tcpWord);
        if (currWord.getTLast() == 1)
        {
          rdpFsmState  = RDP_WAIT_META;
        }
      }
      break;

    case RDP_STREAM_FMC:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rdpFsmState = RDP_RESET;
      }
      else if (!siTOE_Data.empty()
          && !soFMC_data.full()
          )
      {
        siTOE_Data.read(currWord);
        soFMC_data.write(currWord);
        fmc_tcp_bytes_cnt += extractByteCnt((Axis<64>) currWord);
        if (currWord.getTLast() == 1)
        {
          new_ev_not = NalEventNotif(FMC_TCP_BYTES, fmc_tcp_bytes_cnt);
          evsStreams[6].write_nb(new_ev_not);
          rdpFsmState  = RDP_WAIT_META;
        }
      }
      break;

    case RDP_DROP_PACKET:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        rdpFsmState = RDP_RESET;
      }
      else if( !siTOE_Data.empty() )
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

}


/*****************************************************************************
 * @brief Terminates the internal TCP RX FIFOs and forwards packets to the Role.
 *
 ******************************************************************************/
void pRoleTcpRxDeq(
    ap_uint<1>                  *layer_7_enabled,
    ap_uint<1>                  *role_decoupled,
    stream<NetworkWord>         &sRoleTcpDataRx_buffer,
    stream<NetworkMetaStream>   &sRoleTcpMetaRx_buffer,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,
    stream<PacketLen>           &role_write_cnt_sig
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static DeqFsmStates deqFsmState = DEQ_WAIT_META;
#pragma HLS RESET variable=deqFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static PacketLen current_bytes_written = 0;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord cur_word = NetworkWord();
  NetworkMetaStream cur_meta = NetworkMetaStream();
  bool role_disabled = (*layer_7_enabled == 0 && *role_decoupled == 1);

  switch(deqFsmState)
  {
    default:
    case DEQ_SEND_NOTIF:
      if(!role_write_cnt_sig.full())
      {
        role_write_cnt_sig.write(current_bytes_written);
        current_bytes_written = 0;
        deqFsmState = DEQ_WAIT_META;
      }
      break;
    case DEQ_WAIT_META:
      if(!sRoleTcpDataRx_buffer.empty() && !sRoleTcpMetaRx_buffer.empty()
          && ( (!soTcp_data.full() && !soTcp_meta.full()) ||  //user can read
            (role_disabled) //role is disabled -> drain FIFOs
            )
        )
      {
        cur_word = sRoleTcpDataRx_buffer.read();
        cur_meta = sRoleTcpMetaRx_buffer.read();
        current_bytes_written = extractByteCntNW(cur_word);
        if(!role_disabled)
        {
          soTcp_data.write(cur_word);
          soTcp_meta.write(cur_meta);
        }
        if(cur_word.tlast == 0)
        {
          deqFsmState = DEQ_STREAM_DATA;
        } else {
          deqFsmState = DEQ_SEND_NOTIF;
        }
      }
      break;
    case DEQ_STREAM_DATA:
      if(!sRoleTcpDataRx_buffer.empty()
          && (!soTcp_data.full() || role_disabled)
        )
      {
        cur_word = sRoleTcpDataRx_buffer.read();
        current_bytes_written += extractByteCntNW(cur_word);
        if(!role_disabled)
        {
          soTcp_data.write(cur_word);
        }
        if(cur_word.tlast == 1)
        {
          deqFsmState = DEQ_SEND_NOTIF;
        }
      }
      break;
  }

}


/*****************************************************************************
 * @brief Terminates the internal TCP RX FIFOs and forwards packets to the FMC.
 *
 ******************************************************************************/
void pFmcTcpRxDeq(
    stream<TcpAppData>    &sFmcTcpDataRx_buffer,
    stream<TcpAppMeta>    &sFmcTcpMetaRx_buffer,
    stream<TcpAppData>    &soFmc_data,
    stream<TcpAppMeta>    &soFmc_meta,
    stream<PacketLen>     &fmc_write_cnt_sig
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static DeqFsmStates deqFsmState = DEQ_WAIT_META;
#pragma HLS RESET variable=deqFsmState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static PacketLen current_bytes_written = 0;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppData cur_word = TcpAppData();
  TcpAppMeta cur_meta = TcpAppMeta();

  switch(deqFsmState)
  {
    default:
    case DEQ_SEND_NOTIF:
      if(!fmc_write_cnt_sig.full())
      {
        fmc_write_cnt_sig.write(current_bytes_written);
        current_bytes_written = 0;
        deqFsmState = DEQ_WAIT_META;
      }
      break;
    case DEQ_WAIT_META:
      if(!sFmcTcpDataRx_buffer.empty() && !sFmcTcpMetaRx_buffer.empty()
          && !soFmc_data.full() && !soFmc_meta.full()
        )
      {
        printf("[pFmcTcpRxDeq] Start processing FMC packet\n");
        cur_word = sFmcTcpDataRx_buffer.read();
        cur_meta = sFmcTcpMetaRx_buffer.read();
        current_bytes_written = extractByteCnt((Axis<64>) cur_word);
        soFmc_data.write(cur_word);
        soFmc_meta.write(cur_meta);
        if(cur_word.getTLast() == 0)
        {
          deqFsmState = DEQ_STREAM_DATA;
        } else {
          deqFsmState = DEQ_SEND_NOTIF;
        }
      }
      break;
    case DEQ_STREAM_DATA:
      if(!sFmcTcpDataRx_buffer.empty()
          && !soFmc_data.full()
        )
      {
        cur_word = sFmcTcpDataRx_buffer.read();
        soFmc_data.write(cur_word);
        current_bytes_written += extractByteCnt((Axis<64>) cur_word);
        if(cur_word.getTLast() == 1)
        {
          deqFsmState = DEQ_SEND_NOTIF;
        }
      }
      break;
  }
}



/*****************************************************************************
 * @brief Write Path (WRp) - From ROLE or FMC to TOE.
 *  Process waits for a new data segment to write and forwards it to TOE.
 *
 * @param[in]   layer_4_enabled,          external signal if layer 4 is enabled
 * @param[in]   piNTS_ready,              external signal if NTS is up and running
 * @param[in]   siFmc_data,               Data from FMC
 * @param[in]   siFmc_meta,               Metadata from FMC
 * @param[in]   siTcp_data,               Data from [ROLE].
 * @param[in]   siTcp_meta,               Meta Data from [ROLE].
 * @param[out]  soTOE_Data,               Data for [TOE].
 * @param[out]  soTOE_SessId,             Session Id for [TOE].
 * @param[out]  soTOE_len,                Data length for [TOE].
 * @param[out]  sGetIpReq_TcpTx,          Request stream for the the MRT Agency
 * @param[in]   sGetIpRep_TcpTx,          Reply stream from the MRT Agency
 * @param[out]  &sGetSidFromTriple_Req,   Request stream for the the TCP Agency
 * @param[in]   &sGetSidFromTriple_Rep,   Reply stream from the TCO Agency
 * @param[out]  sNewTcpCon_Req,           Request stream for pTcpCOn to open a new connection
 * @param[in]   sNewTcpCon_Rep,           Reply stream from pTcpCOn
 * @param[in]   cache_inval_sig,          Signal from the Cache Invalidation Logic
 * @param[out]  internal_event_fifo,      Fifo for event reporting
 *
 ******************************************************************************/
void pTcpWRp(
    ap_uint<1>                  *layer_4_enabled,
    ap_uint<1>                  *piNTS_ready,
    stream<TcpAppData>          &siFMC_data,
    stream<TcpAppMeta>          &siFMC_SessId,
    stream<NetworkWord>         &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<TcpAppData>          &soTOE_Data,
    stream<TcpAppMeta>          &soTOE_SessId,
    stream<TcpDatLen>           &soTOE_len,
    stream<NodeId>              &sGetIpReq_TcpTx,
    stream<Ip4Addr>             &sGetIpRep_TcpTx,
    stream<NalTriple>           &sGetSidFromTriple_Req,
    stream<SessionId>           &sGetSidFromTriple_Rep,
    stream<NalTriple>           &sNewTcpCon_Req,
    stream<NalNewTcpConRep>     &sNewTcpCon_Rep,
    stream<bool>                &cache_inval_sig,
    stream<NalEventNotif>       &internal_event_fifo
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1


  char* myName  = concat3(THIS_NAME, "/", "Tcp_WRp");

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static WrpFsmStates wrpFsmState = WRP_RESET;

  static SessionId cached_tcp_tx_session_id = UNUSED_SESSION_ENTRY_VALUE;
  static ap_uint<64> cached_tcp_tx_triple = UNUSED_TABLE_ENTRY_VALUE;
  static NodeId cached_dst_rank = INVALID_MRT_VALUE;
  static Ip4Addr cached_dst_ip_addr = 0x0;
  static bool cache_init = false;

  static uint8_t evs_loop_i = 0;

#pragma HLS RESET variable=wrpFsmState
#pragma HLS RESET variable=cached_tcp_tx_session_id
#pragma HLS RESET variable=cached_tcp_tx_triple
#pragma HLS RESET variable=cached_dst_rank
#pragma HLS RESET variable=cache_init
#pragma HLS RESET variable=cached_dst_ip_addr
#pragma HLS RESET variable=evs_loop_i

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static NetworkMetaStream out_meta_tcp = NetworkMetaStream();
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


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NetworkWord  currWordIn;
  //TcpAppData   currWordOut;
  NalEventNotif new_ev_not;

  switch (wrpFsmState)
  {
    default:
    case WRP_RESET:
      if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        cache_init = false;
        wrpFsmState = WRP_WAIT_META;
      } else {
        if(!siFMC_data.empty())
        {
          siFMC_data.read();
        }
        if(!siFMC_SessId.empty())
        {
          siFMC_SessId.read();
        }
        if(!siTcp_data.empty())
        {
          siTcp_data.read();
        }
        if(!siTcp_meta.empty())
        {
          siTcp_meta.read();
        }
      }
      break;
    case WRP_WAIT_META:
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if(!cache_inval_sig.empty())
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
      else if (!siFMC_SessId.empty()
          && !soTOE_SessId.full() //&& !soTOE_len.full() 
          )
      {
        //FMC must come first
        tcpSessId = (AppMeta) siFMC_SessId.read();
        soTOE_SessId.write(tcpSessId);
        //soTOE_len.write(0); //always streaming mode
        streaming_mode = true;
        tcpTX_current_packet_length = 0;
        //delete the session id, we don't need it any longer
        // (this expects an HTTP mode of request-response)
        //UPDATE: is done via the TOE_Notif if the client 
        //closes the connection

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
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          evsStreams[0].write_nb(new_ev_not);
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
          new_ev_not = NalEventNotif(PCOR_TX, 1);
          evsStreams[2].write_nb(new_ev_not);
        }

        if(cache_init && dst_rank == cached_dst_rank)
        {
          dst_ip_addr = cached_dst_ip_addr;
          wrpFsmState = WRP_W8FORREQS_11;
        } else {
          //need request both...
          sGetIpReq_TcpTx.write(dst_rank);
          cache_init = false;
          wrpFsmState = WRP_W8FORREQS_1;
          //break;
        }
      }
      break;

    case WRP_W8FORREQS_1:
      if(!sGetIpRep_TcpTx.empty())
      {
        dst_ip_addr = sGetIpRep_TcpTx.read();
        wrpFsmState = WRP_W8FORREQS_11;
      }
      break;

    case WRP_W8FORREQS_11:
      //both cases
      if(//cache_init ||
          !sGetSidFromTriple_Req.full())
      {
        //not else, in both cases
        wrpFsmState = WRP_W8FORREQS_2;
        if(dst_ip_addr == 0)
        {
          new_ev_not = NalEventNotif(NID_MISS_TX, 1);
          evsStreams[1].write_nb(new_ev_not);
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
      }
      break;

    case WRP_W8FORREQS_2:
      if(!sGetSidFromTriple_Rep.empty())
      {
        sessId = sGetSidFromTriple_Rep.read();
        cached_tcp_tx_triple = new_triple;
        cached_tcp_tx_session_id = sessId;
        cached_dst_ip_addr = dst_ip_addr;
        cached_dst_rank = dst_rank;
        cache_init = true;
        wrpFsmState = WRP_W8FORREQS_22;
      }
      break;

    case WRP_W8FORREQS_22:
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if(!soTOE_SessId.full() && !sNewTcpCon_Req.full() && !soTOE_len.full() )
      {
        //both cases
        //"final" preprocessing
        printf("session id found: %d\n", (int) sessId);
        if(sessId == (SessionId) UNUSED_SESSION_ENTRY_VALUE)
        {//we need to create one first
          sNewTcpCon_Req.write(new_triple);
          wrpFsmState = WRP_WAIT_CONNECTION;
          cache_init = false;
          printf("requesting new connection.\n");
          break;
        }
        new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        evsStreams[3].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        evsStreams[4].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        evsStreams[5].write_nb(new_ev_not);

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
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if( !soTOE_SessId.full() && !sNewTcpCon_Rep.empty() && !soTOE_len.full() )
      {
        NalNewTcpConRep con_rep = sNewTcpCon_Rep.read();
        if(con_rep.failure == true)
        {
          new_ev_not = NalEventNotif(TCP_CON_FAIL, 1);
          evsStreams[6].write_nb(new_ev_not);
          // we sink the packet, because otherwise the design will hang
          // and the user is notified with the flight recorder status
          wrpFsmState = WRP_DROP_PACKET;
          printf("NRC drops the packet...\n");
          break;
        }
        new_triple = con_rep.new_triple;
        sessId = con_rep.newSessionId;

        new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
        evsStreams[7].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
        evsStreams[8].write_nb(new_ev_not);
        new_ev_not = NalEventNotif(PACKET_TX, 1);
        evsStreams[9].write_nb(new_ev_not);

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
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if (!siFMC_data.empty() && !soTOE_Data.full() && !soTOE_len.full() )
      {
        TcpAppData tcpWord = siFMC_data.read();
        tcpTX_current_packet_length += extractByteCnt((Axis<64>) tcpWord);
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
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if (!siTcp_data.empty() && !soTOE_Data.full()  && !soTOE_len.full() )
      {
        currWordIn = siTcp_data.read();
        tcpTX_current_packet_length += extractByteCnt(currWordIn);
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
      break;

    case WRP_DROP_PACKET:
      if( *layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        wrpFsmState = WRP_RESET;
      }
      else if( !siTcp_data.empty())
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
    )
  {
    if(!evsStreams[evs_loop_i].empty())
    {
      internal_event_fifo.write(evsStreams[evs_loop_i].read());
    }
    evs_loop_i++;
    if(evs_loop_i >= 10)
    {
      evs_loop_i = 0;
    }
  }

}


/*****************************************************************************
 * @brief Write Buffer (WBu) - From WRp to TOE.
 *  Process to synchronize with TOE's TX buffer (and it's available space).
 *  In case of streaming mode (i.e. ROLE's length was 0), WRp takes care of
 *  splitting the data and writing the right len.
 *
 * @param[in]   layer_4_enabled,          external signal if layer 4 is enabled
 * @param[in]   piNTS_ready,              external signal if NTS is up and running
 * @param[in]   siWrp_Data,               Tx data from [Wrp].
 * @param[in]   siWrp_SessId,             the session Id from [Wrp].
 * @param[in]   siWrp_len,                the length of the current chunk (is *never* 0) from [Wrp].
 * @param[out]  soTOE_Data,               Data for the TOE
 * @param[out]  soTOE_SndReq,             Send request (containing the planned length) for the TOE
 * @param[in]   siTOE_SndRep,             Send reply from the TOE (containing the allowed length)
 *
 ******************************************************************************/
void pTcpWBu(
    ap_uint<1>              *layer_4_enabled,
    ap_uint<1>              *piNTS_ready,
    stream<TcpAppData>      &siWrp_Data,
    stream<TcpAppMeta>      &siWrp_SessId,
    stream<TcpDatLen>       &siWrp_len,
    stream<TcpAppData>      &soTOE_Data,
    stream<TcpAppSndReq>    &soTOE_SndReq,
    stream<TcpAppSndRep>    &siTOE_SndRep
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  char *myName  = concat3(THIS_NAME, "/", "Tcp_WBu");


  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static WbuFsmStates wbuState = WBU_WAIT_META;
  static uint16_t dequeue_cnt = 0; //in BYTES!!
  //static TcpDatLen original_requested_length = 0;

#pragma HLS RESET variable=wbuState
#pragma HLS RESET variable=dequeue_cnt
  //#pragma HLS RESET variable=original_requested_length

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static TcpAppMeta current_sessId;
  static TcpDatLen current_requested_length = 0;
  static TcpDatLen current_approved_length = 0;
  static bool need_to_request_again = false;


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  TcpAppSndReq toe_sendReq;


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
        //original_requested_length = new_len;

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
            wbuState = WBU_STREAM;
            current_approved_length = current_requested_length; //not spaceLeft(!), because this could be bigger!
            need_to_request_again = false;
            dequeue_cnt = 0;
            break;
          case NO_SPACE:
            printWarn(myName, "Not enough space for writing %d bytes in the Tx buffer of session #%d. Available space is %d bytes.\n",
                appSndRep.length.to_uint(), appSndRep.sessId.to_uint(), appSndRep.spaceLeft.to_uint());

            dequeue_cnt = 0;
            current_approved_length = appSndRep.spaceLeft - 7; //with "security margin" (so that the length is correct if tkeep is not 0xff for the last word)
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
                current_sessId.to_uint(), current_requested_length.to_uint());
          }
        } else {
          tmp.setTLast(0); //to be sure
        }
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
      //drain all streams as long as NTS disabled
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
}


/*****************************************************************************
 * @brief Client connection to remote HOST or FPGA socket (COn).
 *
 * @param[out]  soTOE_OpnReq,            open connection request to TOE.
 * @param[in]   siTOE_OpnRep,            open connection reply from TOE.
 * @param[out]  sAddNewTriple_TcpCon,    Notification for the TCP Agency to add a new Triple/SessionId pair
 * @param[in]   sNewTcpCon_Req,          Request stream from pTcpWRp to open a new connection
 * @param[out]  sNewTcpCon_Rep,          Reply stream to pTcpWRp
 *
 ******************************************************************************/
void pTcpCOn(
    stream<TcpAppOpnReq>        &soTOE_OpnReq,
    stream<TcpAppOpnRep>        &siTOE_OpnRep,
    stream<NalNewTableEntry>    &sAddNewTriple_TcpCon,
    stream<NalTriple>           &sNewTcpCon_Req,
    stream<NalNewTcpConRep>     &sNewTcpCon_Rep
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
        opnFsmState = OPN_REQ;
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
            NalNewTableEntry ne_struct =  NalNewTableEntry(triple_for_new_connection, newConn.sessId);
            sAddNewTriple_TcpCon.write(ne_struct);
            opnFsmState = OPN_DONE;
            NalNewTcpConRep con_rep = NalNewTcpConRep(triple_for_new_connection, newConn.sessId, false);
            sNewTcpCon_Rep.write(con_rep);
          }
          else {
            printError(myName, "Client failed to connect to remote socket:\n");
            printSockAddr(myName, HostSockAddr);
            opnFsmState = OPN_DONE;
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
}

/*****************************************************************************
 * @brief Asks the TOE to close Tcp *connections*, based on the request from 
 *        pPortLogic.
 *
 * @param[out]  soTOE_ClsReq,            close connection request to TOE.
 * @param[in]   sGetNextDelRow_Req,      request stream to TCP Agency
 * @param[in]   sGetNextDelRow_Rep,      reply stream rom TCP Agency
 * @param[in]   sStartTclCls,            start signal from pPortLogic
 *
 ******************************************************************************/
void pTcpCls(
    stream<TcpAppClsReq>      &soTOE_ClsReq,
    stream<bool>              &sGetNextDelRow_Req,
    stream<SessionId>         &sGetNextDelRow_Rep,
    stream<bool>              &sStartTclCls
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


  switch (clsFsmState_Tcp) {
    default:
    case CLS_IDLE:
      //we wait until we are activated;
      if(!sStartTclCls.empty())
      {
        if(sStartTclCls.read())
        {
          clsFsmState_Tcp = CLS_NEXT;
        }
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
}



/*! \} */

