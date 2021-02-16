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
 * @file       : hss.cpp
 * @brief      : The Housekeeping Sub System (HSS) of the NAL core.
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

/**
 * This is the process that has to know which config value means what
 * And to where it should be send:
 *
 * Returns:
 *  0 : no propagation
 *  1 : to TCP tables
 *  2 : to Port logic
 *  3 : to own rank receivers
 */
uint8_t selectConfigUpdatePropagation(uint16_t config_addr)
{
#pragma HLS INLINE
  switch(config_addr) {
    default:
      return 0;
    case NAL_CONFIG_OWN_RANK:
      return 3;
    case NAL_CONFIG_SAVED_FMC_PORTS:
    case NAL_CONFIG_SAVED_TCP_PORTS:
    case NAL_CONFIG_SAVED_UDP_PORTS:
      return 2;
  }
}



/*****************************************************************************
 * @brief Contains the Axi4 Lite secondary endpoint and reads the MRT and 
 *        configuration values from it as well as writes the status values. It 
 *        notifies all other concerned processes on MRT or configuration updates 
 *        and is notified on status updates.
 *
 * @param[in/out]  ctrlLink,              the Axi4Lite bus
 * @param[out]     sToPortLogic,          notification of configuration changes
 * @param[out]     sToUdpRx,              notification of configuration changes
 * @param[out]     sToTcpRx,              notification of configuration changes
 * @param[out]     sToStatusProc,         notification of configuration changes
 * @param[out]     sMrtUpdate,            notification of MRT content changes
 * @param[out]     mrt_version_update_0,  notification of MRT version change
 * @param[out]     mrt_version_update_1,  notification of MRT version change
 * @param[in]      sStatusUpdate,         Satus update notification for Axi4Lite proc
 *
 ******************************************************************************/
void axi4liteProcessing(
    ap_uint<32>   ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    //stream<NalConfigUpdate> &sToTcpAgency, //(currently not used)
    stream<NalConfigUpdate>   &sToPortLogic,
    stream<NalConfigUpdate>   &sToUdpRx,
    stream<NalConfigUpdate>   &sToTcpRx,
    stream<NalConfigUpdate>   &sToStatusProc,
    stream<NalMrtUpdate>      &sMrtUpdate,
    //ap_uint<32>               localMRT[MAX_MRT_SIZE],
    stream<uint32_t>          &mrt_version_update_0,
    stream<uint32_t>          &mrt_version_update_1,
    stream<NalStatusUpdate>   &sStatusUpdate
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
  //no pipeline, isn't compatible to AXI4Lite bus

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static uint16_t tableCopyVariable = 0;
  static bool tables_initialized = false;
  static AxiLiteFsmStates a4lFsm = A4L_COPY_CONFIG; //start with config after reset
  static ConfigBcastStates cbFsm = CB_WAIT;
  static uint32_t processed_mrt_version = 0;
  static ConfigBcastStates mbFsm = CB_WAIT;

#pragma HLS reset variable=tableCopyVariable
#pragma HLS reset variable=tables_initialized
#pragma HLS reset variable=a4lFsm
#pragma HLS reset variable=cbFsm
#pragma HLS reset variable=processed_mrt_version
#pragma HLS reset variable=mbFsm

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<32> config[NUMBER_CONFIG_WORDS];
  static ap_uint<32> status[NUMBER_STATUS_WORDS];

  static ap_uint<16> configProp = 0x0;
  static NalConfigUpdate cu_toCB = NalConfigUpdate();
  static ap_uint<32> new_word;

  static ap_uint<32> localMRT[MAX_MRT_SIZE];
  //#pragma HLS RESOURCE variable=localMRT core=RAM_2P_BRAM
  //#pragma HLS ARRAY_PARTITION variable=localMRT complete dim=1
  //FIXME: maybe optimize and remove localMRT here (send updates when indicated by version?)


  //#pragma HLS ARRAY_PARTITION variable=status cyclic factor=4 dim=1
  //#pragma HLS ARRAY_PARTITION variable=config cyclic factor=4 dim=1

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  NalConfigUpdate cu = NalConfigUpdate();
  uint32_t new_mrt_version;
  ap_uint<32> new_ip4node;


  if(!tables_initialized)
  {
    // ----- tables init -----
    for(int i = 0; i < MAX_MRT_SIZE; i++)
    {
      //localMRT[i] = 0x0;
      localMRT[i] = 0x1; //to force init of MRT agency
    }
    for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    {
      config[i] = 0x0;
    }
    for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
    {
      status[i] = 0x0;
    }
    tables_initialized = true;
  } else {

    // ----- AXI4Lite Processing -----

    switch(a4lFsm)
    {
      default:
        //case A4L_RESET:
        //  // ----- tables init -----
        //  for(int i = 0; i < MAX_MRT_SIZE; i++)
        //  {
        //    localMRT[i] = 0x0;
        //  }
        //  for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
        //  {
        //    config[i] = 0x0;
        //  }
        //  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
        //  {
        //    status[i] = 0x0;
        //  }
        //  tableCopyVariable = 0;
        //  a4lFsm = A4L_STATUS_UPDATE;
        //  break;
      case A4L_STATUS_UPDATE:
        if(!sStatusUpdate.empty())
        {
          // ----- apply updates -----
          NalStatusUpdate su = sStatusUpdate.read();
          status[su.status_addr] = su.new_value;
          printf("[A4l] got status update for address %d with value %d\n", (int) su.status_addr, (int) su.new_value);
        } else {
          a4lFsm = A4L_COPY_CONFIG;
        }
        break;
      case A4L_COPY_CONFIG:
        //TODO: necessary? Or does this AXI4Lite anyways "in the background"?
        //or do we need to copy it explicetly, but could do this also every ~2 seconds?

        //printf("[A4l] copy config %d\n", tableCopyVariable);
        new_word = ctrlLink[tableCopyVariable];
        if(new_word != config[tableCopyVariable])
        {
          configProp = selectConfigUpdatePropagation(tableCopyVariable);
          cu = NalConfigUpdate(tableCopyVariable, new_word);
          cbFsm = CB_START;
          //config[tableCopyVariable] = new_word;
          a4lFsm = A4L_COPY_CONFIG_2;
        } else {
          tableCopyVariable++;
          if(tableCopyVariable >= NUMBER_CONFIG_WORDS)
          {
            tableCopyVariable = 0;
            a4lFsm = A4L_COPY_MRT;
          }
        }
        break;
      case A4L_COPY_CONFIG_2:
        printf("[A4l] waiting CB broadcast\n");
        if(cbFsm == CB_WAIT)
        {
          config[tableCopyVariable] = new_word;
          tableCopyVariable++;
          if(tableCopyVariable >= NUMBER_CONFIG_WORDS)
          {
            tableCopyVariable = 0;
            a4lFsm = A4L_COPY_MRT;
          } else {
            a4lFsm = A4L_COPY_CONFIG;
          }
        }
        break;
      case A4L_COPY_MRT:
        //printf("[A4l] copy MRT %d\n", tableCopyVariable);
        new_ip4node = ctrlLink[tableCopyVariable + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
        if (new_ip4node != localMRT[tableCopyVariable])
        {
          NalMrtUpdate mu = NalMrtUpdate(tableCopyVariable, new_ip4node);
          sMrtUpdate.write(mu);
          localMRT[tableCopyVariable] = new_ip4node;
          printf("[A4l] update MRT at %d with %d\n", tableCopyVariable, (int) new_ip4node);
          NalMrtUpdate mrt_update = NalMrtUpdate(tableCopyVariable, new_ip4node);
          sMrtUpdate.write(mrt_update);
        }
        tableCopyVariable++;
        if(tableCopyVariable >= MAX_MRT_SIZE)
        {
          tableCopyVariable = 0;
          a4lFsm = A4L_COPY_STATUS;
        }
        break;
      case A4L_COPY_STATUS:
        ctrlLink[NUMBER_CONFIG_WORDS + tableCopyVariable] = status[tableCopyVariable];
        tableCopyVariable++;
        if(tableCopyVariable >= NUMBER_STATUS_WORDS)
        {
          //tableCopyVariable = 0;
          a4lFsm = A4L_COPY_FINISH;
        }
        break;
      case A4L_COPY_FINISH:
        tableCopyVariable = 0;
        //acknowledge the processed version
        new_mrt_version = (uint32_t) config[NAL_CONFIG_MRT_VERSION];
        if(new_mrt_version != processed_mrt_version)
        {
          printf("\t\t\t[A4L:CtrlLink:Info] Acknowledged MRT version %d.\n", (int) new_mrt_version);
          mbFsm = CB_START;
          processed_mrt_version = new_mrt_version;
        }
        a4lFsm = A4L_WAIT_FOR_SUB_FSMS;
        break;
      case A4L_WAIT_FOR_SUB_FSMS:
        printf("[A4l] SubFSMs state mb: %d; cb: %d\n", (int) mbFsm, (int) cbFsm);
        if(mbFsm == CB_WAIT && cbFsm == CB_WAIT)
        {
          a4lFsm = A4L_STATUS_UPDATE;
          printf("[A4l] SubFSMs done...continue\n");
        }
        break;
    }

    // ----- Config Broadcast -----

    switch(cbFsm)
    {
      default:
      case CB_WAIT:
        //NOP
        break;
      case CB_START:
        cu_toCB = cu;
        switch (configProp) {
          default:
          case 0:
            cbFsm = CB_WAIT; //currently not used
            break;
          case 1:
            cbFsm = CB_1;
            break;
          case 2:
            cbFsm = CB_2;
            break;
          case 3:
            cbFsm = CB_3_0;
            printf("[A4l] Issued rank update: %d\n", (int) cu.update_value);
            break;
        }
        break;
      case CB_1:
        //(currently not used)
        //if(!sToTcpAgency.full())
        //{
        //      sToTcpAgency.write(cu_toCB);
        cbFsm = CB_WAIT;
        //}
        break;
      case CB_2:
        if(!sToPortLogic.full())
        {
          sToPortLogic.write(cu_toCB);
          cbFsm = CB_WAIT;
        }
        break;
      case CB_3_0:
        if(!sToUdpRx.full())
        {
          sToUdpRx.write(cu_toCB);
          cbFsm = CB_3_1;
        }
        break;
      case CB_3_1:
        if(!sToTcpRx.full())
        {
          sToTcpRx.write(cu_toCB);
          cbFsm = CB_3_2;
        }
        break;
      case CB_3_2:
        if(!sToStatusProc.full())
        {
          sToStatusProc.write(cu_toCB);
          cbFsm = CB_WAIT;
        }
        break;
    }

    // ----- MRT version broadcast ----

    switch(mbFsm)
    {
      default:
      case CB_WAIT:
        //NOP
        break;
      case CB_START:
        mbFsm = CB_1;
        printf("[A4l] Issued Mrt update: %d\n", (int) processed_mrt_version);
        break;
      case CB_1:
        if(!mrt_version_update_0.full())
        {
          mrt_version_update_0.write(processed_mrt_version);
          mbFsm = CB_2;
        }
        break;
      case CB_2:
        if(!mrt_version_update_1.full())
        {
          mrt_version_update_1.write(processed_mrt_version);
          mbFsm = CB_WAIT;
        }
        break;
    }

  } // else

}


/*****************************************************************************
 * @brief Can access the BRAM that contains the MRT and replies to lookup requests
 *
 * @param[in]    sMrtUpdate,            Notification of MRT changes
 * @param[in]    sGetIpReq_UdpTx,       Request stream to get the IPv4 to a NodeId (from UdpTx)
 * @param[out]   sGetIpRep_UdpTx,       Reply stream containing the IP address (to UdpTx)
 * @param[in]    sGetIpReq_TcpTx,       Request stream to get the IPv4 to a NodeId (from TcpTx)
 * @param[out]   sGetIpRep_TcpTx,       Reply stream containing the IP address (to TcpTx)
 * @param[in]    sGetNidReq_UdpRx,      Request stream to get the NodeId to an IPv4 (from UdRx)
 * @param[out]   sGetNidRep_UdpRx,      Reply stream containing the NodeId (to UdpRx)
 * @param[in]    sGetNidReq_TcpRx,      Request stream to get the NodeId to an IPv4 (from TcpRx)
 * @param[out]   sGetNidRep_TcpRx,      Reply stream containing the NodeId (to TcpRX)
 *
 ******************************************************************************/
void pMrtAgency(
    stream<NalMrtUpdate>  &sMrtUpdate,
    //const ap_uint<32>     localMRT[MAX_MRT_SIZE],
    stream<NodeId>        &sGetIpReq_UdpTx,
    stream<Ip4Addr>       &sGetIpRep_UdpTx,
    stream<NodeId>        &sGetIpReq_TcpTx,
    stream<Ip4Addr>       &sGetIpRep_TcpTx,
    stream<Ip4Addr>       &sGetNidReq_UdpRx,
    stream<NodeId>        &sGetNidRep_UdpRx,
    stream<Ip4Addr>       &sGetNidReq_TcpRx,
    stream<NodeId>        &sGetNidRep_TcpRx//,
    //stream<Ip4Addr>       &sGetNidReq_TcpTx,
    //stream<NodeId>        &sGetNidRep_TcpTx
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  //static bool tables_initialized = false;

  //#pragma HLS reset variable=tables_initialized

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //static ap_uint<32> localMRT[MAX_MRT_SIZE];

  //#define CAM_SIZE 8
  //#define CAM_NUM 16
  //  static Cam8<NodeId,Ip4Addr> mrt_cam_arr[CAM_NUM];
  //#ifndef __SYNTHESIS__
  //  if(MAX_MRT_SIZE != 128)
  //  {
  //    printf("\n\t\tERROR: pMrtAgency is currently configured to support only a MRT size up to 128! Abort.\n(Currently, the use of \'mrt_cam_arr\' must be updated accordingly by hand.)\n");
  //    exit(-1);
  //  }
  //#endif

  static ap_uint<32> localMRT[MAX_MRT_SIZE];
  //#pragma HLS ARRAY_PARTITION variable=localMRT cyclic factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=localMRT complete dim=1


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  //  if(!tables_initialized)
  //  {
  //    //for(int i = 0; i < CAM_NUM; i++)
  //    //{
  //    //  mrt_cam_arr[i].reset();
  //    //}
  //    tables_initialized = true;
  //  } else

  if( !sMrtUpdate.empty() )
  {
    NalMrtUpdate mu = sMrtUpdate.read();
    if(mu.nid < MAX_MRT_SIZE)
    {
      localMRT[mu.nid] = mu.ip4a;
    }

  } else if( !sGetIpReq_UdpTx.empty() && !sGetIpRep_UdpTx.full())
  {
    NodeId rank;
    rank = sGetIpReq_UdpTx.read();
    //uint8_t cam_select = rank / CAM_SIZE;
    Ip4Addr rep = 0;  //return zero on failure
    //if(rank < MAX_MRT_SIZE && cam_select < CAM_NUM)
    if(rank < MAX_MRT_SIZE)
    {
      //mrt_cam_arr[cam_select].lookup(rank, rep);
      rep = localMRT[rank];
    }
    sGetIpRep_UdpTx.write(rep);
  }
  else if( !sGetIpReq_TcpTx.empty() && !sGetIpRep_TcpTx.full())
  {
    NodeId rank;
    rank = sGetIpReq_TcpTx.read();
    //uint8_t cam_select = rank / CAM_SIZE;
    Ip4Addr rep = 0;  //return zero on failure
    //if(rank < MAX_MRT_SIZE && cam_select < CAM_NUM)
    if(rank < MAX_MRT_SIZE)
    {
      //mrt_cam_arr[cam_select].lookup(rank, rep);
      rep = localMRT[rank];
    }
    sGetIpRep_TcpTx.write(rep);
  }
  else if( !sGetNidReq_UdpRx.empty() && !sGetNidRep_UdpRx.full())
  {
    ap_uint<32> ipAddr =  sGetNidReq_UdpRx.read();
    printf("[HSS-INFO] Searching for Node ID of IP %d.\n", (int) ipAddr);
    NodeId rep = INVALID_MRT_VALUE;
    for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
    {
#pragma HLS unroll factor=8
      if(localMRT[i] == ipAddr)
      {
        rep = (NodeId) i;
        break;
      }
    }
    printf("[HSS-INFO] found Node Id %d.\n", (int) rep);
    sGetNidRep_UdpRx.write(rep);
  }
  else if( !sGetNidReq_TcpRx.empty() && !sGetNidRep_TcpRx.full())
  {
    ap_uint<32> ipAddr =  sGetNidReq_TcpRx.read();
    printf("[HSS-INFO] Searching for Node ID of IP %d.\n", (int) ipAddr);
    NodeId rep = INVALID_MRT_VALUE;
    for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
    {
#pragma HLS unroll factor=8
      if(localMRT[i] == ipAddr)
      {
        rep = (NodeId) i;
        break;
      }
    }
    printf("[HSS-INFO] found Node Id %d.\n", (int) rep);
    sGetNidRep_TcpRx.write(rep);
  }
  //for(int i = 0; i < CAM_NUM; i++)
  //{
  //  if(mrt_cam_arr[i].reverse_lookup(ipAddr, rep))
  //  {
  //    break;
  //  }
  //}
}


/*****************************************************************************
 * @brief Translates the one-hot encoded open-port vectors from the Role
 *        (i.e. `piUdpRxPorts` and `piTcpRxPorts`) to absolute port numbers
 *        If the input vector changes, or during a reset of the Role, the
 *        necessary open or close requests are send to `pUdpLsn`, `pUdpCls`,
 *        `pTcpLsn`, and `pTcpCls`.
 *
 * @param[in]   layer_4_enabled,        external signal if layer 4 is enabled
 * @param[in]   layer_7_enabled,        external signal if layer 7 is enabled
 * @param[in]   role_decoupled,         external signal if the role is decoupled
 * @param[in]   piNTS_ready,            external signal if NTS is up and running
 * @param[in]   piMMIO_FmcLsnPort,      the management listening port (from MMIO)
 * @param[in]   pi_udp_rx_ports,        one-hot encoded UDP Role listening ports
 * @param[in]   pi_tcp_rx_ports,        one-hot encoded Tcp Role listening ports
 * @param[in]   sConfigUpdate,          notification of configuration changes
 * @param[out]  sUdpPortsToOpen,        stream containing the next UdpPort to open (to pUdpLsn)
 * @param[out]  sUdpPortsToClose,       stream containing the next UdpPort to close (to pUdpCls)
 * @param[out]  sTcpPortsToOpen,        stream containing the next TcpPort to open (to pUdpLsn)
 * @param[in]   sUdpPortsOpenFeedback,  signal of Udp Port opening results (success/failure)
 * @param[in]   sTcpPortsOpenFeedback,  signal of Tcp Port opening results (success/failure)
 * @param[out]  sMarkToDel_unpriv,      signal to mark unpivileged Tcp session as to be closed (to TCP agency)
 * @param[out]  sPortUpdate,            stream containing updates of currently opened ports (to status processing)
 * @param[out]  sStartTclCls            signal to start TCP Connection closing (to pTcpCls)
 *
 ******************************************************************************/
void pPortLogic(
    ap_uint<1>                *layer_4_enabled,
    ap_uint<1>                *layer_7_enabled,
    ap_uint<1>                *role_decoupled,
    ap_uint<1>                *piNTS_ready,
    ap_uint<16>               *piMMIO_FmcLsnPort,
    ap_uint<32>               *pi_udp_rx_ports,
    ap_uint<32>               *pi_tcp_rx_ports,
    stream<NalConfigUpdate>   &sConfigUpdate,
    stream<UdpPort>           &sUdpPortsToOpen,
    stream<UdpPort>           &sUdpPortsToClose,
    stream<TcpPort>           &sTcpPortsToOpen,
    stream<bool>              &sUdpPortsOpenFeedback,
    stream<bool>              &sTcpPortsOpenFeedback,
    stream<bool>              &sMarkToDel_unpriv,
    stream<NalPortUpdate>     &sPortUpdate,
    stream<bool>              &sStartTclCls
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static ap_uint<16> processed_FMC_listen_port = 0;
  static ap_uint<32> tcp_rx_ports_processed = 0;
  static ap_uint<32> udp_rx_ports_processed = 0;
  static PortFsmStates port_fsm = PORT_RESET;

#ifndef __SYNTHESIS__
  static ap_uint<16>  mmio_stabilize_counter = 1;
#else
  static ap_uint<16>  mmio_stabilize_counter = NAL_MMIO_STABILIZE_TIME;
#endif


#pragma HLS reset variable=mmio_stabilize_counter
#pragma HLS reset variable=processed_FMC_listen_port
#pragma HLS reset variable=udp_rx_ports_processed
#pragma HLS reset variable=tcp_rx_ports_processed
#pragma HLS reset variable=port_fsm

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<16> new_relative_port_to_req_udp;
  static ap_uint<16> new_relative_port_to_req_tcp;
  static ap_uint<16> current_requested_port;
  static NalPortUpdate current_port_update;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  switch(port_fsm)
  {
    default:
    case PORT_RESET:
      if(mmio_stabilize_counter > 0)
      {
        mmio_stabilize_counter--;
      } else {
        port_fsm = PORT_IDLE;
      }
      break;
    case PORT_IDLE:
      if(!sConfigUpdate.empty())
      {
        // restore saved states (after NAL got reset)
        // TODO: apparently, this isn't working in time...for this we have to have a bigger "stabilizing count"
        // at the other side: to request ports twice is apparently not harmful
        NalConfigUpdate ca = sConfigUpdate.read();
        switch(ca.config_addr)
        {
          case NAL_CONFIG_SAVED_FMC_PORTS:
            if(processed_FMC_listen_port == 0)
            {
              processed_FMC_listen_port = (ap_uint<16>) ca.update_value;
            }
            break;
          case NAL_CONFIG_SAVED_UDP_PORTS:
            if(udp_rx_ports_processed == 0)
            {
              udp_rx_ports_processed = ca.update_value;
            }
            break;
          case NAL_CONFIG_SAVED_TCP_PORTS:
            if(tcp_rx_ports_processed == 0)
            {
              tcp_rx_ports_processed = ca.update_value;
            }
            break;
          default:
            printf("[ERROR] invalid config update received!\n");
            break;
        }

      } else if(*layer_4_enabled == 0 || *piNTS_ready == 0)
      {
        port_fsm = PORT_L4_RESET;
      } else if (*layer_7_enabled == 0 || *role_decoupled == 1 )
      {
        port_fsm = PORT_L7_RESET;
      } else {
        //  port requests
        if(processed_FMC_listen_port != *piMMIO_FmcLsnPort)
        {
          port_fsm = PORT_NEW_FMC_REQ;
        }
        else if(udp_rx_ports_processed != *pi_udp_rx_ports)
        {
          port_fsm = PORT_NEW_UDP_REQ;
        }
        else if(tcp_rx_ports_processed != *pi_tcp_rx_ports)
        {
          port_fsm = PORT_NEW_TCP_REQ;
        }
      }
      break;
    case PORT_L4_RESET:
      if(!sPortUpdate.full()
          && (processed_FMC_listen_port != 0 || udp_rx_ports_processed != 0 || tcp_rx_ports_processed !=0 )
        )
      {//first, notify
        if(processed_FMC_listen_port != 0)
        {
          sPortUpdate.write(NalPortUpdate(FMC, 0));
          processed_FMC_listen_port = 0;
        }
        else if(udp_rx_ports_processed != 0)
        {
          sPortUpdate.write(NalPortUpdate(UDP, 0));
          udp_rx_ports_processed = 0;
        }
        else if(tcp_rx_ports_processed != 0)
        {
          sPortUpdate.write(NalPortUpdate(TCP, 0));
          tcp_rx_ports_processed = 0;
        }
      }
      else if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        //if layer 4 is reset, ports will be closed
        //in all cases
        processed_FMC_listen_port = 0x0;
        udp_rx_ports_processed = 0x0;
        tcp_rx_ports_processed = 0x0;
        port_fsm = PORT_IDLE;
        //no need for PORT_RESET (everything should still be "stable")
      }
      break;
    case PORT_L7_RESET:
      if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        if(udp_rx_ports_processed > 0)
        {
          port_fsm = PORT_START_UDP_CLS;
        } else if(tcp_rx_ports_processed > 0)
        {
          //port_fsm = PORT_START_TCP_CLS_0;
          port_fsm = PORT_START_TCP_CLS_1;
        } else {
          port_fsm = PORT_IDLE;
        }
      } else {
        port_fsm = PORT_L4_RESET;
      }
      break;
    case PORT_START_UDP_CLS:
      if(!sUdpPortsToClose.full())
      {

        //mark all UDP ports as to be deleted
        ap_uint<16> newRelativePortToClose = 0;
        ap_uint<16> newAbsolutePortToClose = 0;
        if(udp_rx_ports_processed != 0)
        {
          newRelativePortToClose = getRightmostBitPos(udp_rx_ports_processed);
          newAbsolutePortToClose = NAL_RX_MIN_PORT + newRelativePortToClose;
          sUdpPortsToClose.write(newAbsolutePortToClose);
          ap_uint<32> one_cold_closed_port = ~(((ap_uint<32>) 1) << (newRelativePortToClose));
          udp_rx_ports_processed &= one_cold_closed_port;
          printf("new UDP port ports to close: %#04x\n",(unsigned int) udp_rx_ports_processed);
        }
        if(udp_rx_ports_processed == 0)
        {
          current_port_update = NalPortUpdate(UDP, 0);
          port_fsm = PORT_SEND_UPDATE;
        }
      }
      break;
      //case PORT_START_TCP_CLS_0:
      //to close the open Role ports (not connections)
      //TODO, add if TOE supports it
      //  break;
    case PORT_START_TCP_CLS_1:
      if(!sMarkToDel_unpriv.full())
      {
        sMarkToDel_unpriv.write(true);
        if(*role_decoupled == 1)
        {
          port_fsm = PORT_WAIT_PR;
        } else {
          port_fsm = PORT_START_TCP_CLS_2;
        }
      }
      break;
    case PORT_WAIT_PR:
      if(*role_decoupled == 0)
      {
        port_fsm = PORT_START_TCP_CLS_2;
      }
      break;
    case PORT_START_TCP_CLS_2:
      if(!sStartTclCls.full())
      {
        sStartTclCls.write(true);
        tcp_rx_ports_processed = 0x0;
        current_port_update = NalPortUpdate(TCP, 0);
        port_fsm = PORT_SEND_UPDATE;
      }
      break;
    case PORT_NEW_FMC_REQ:
      //TODO: if processed_FMC_listen_port != 0, we should actually close it?
      if(!sTcpPortsToOpen.full())
      {
        printf("Need FMC port request: %#02x\n",(unsigned int) *piMMIO_FmcLsnPort);
        sTcpPortsToOpen.write(*piMMIO_FmcLsnPort);
        current_requested_port = *piMMIO_FmcLsnPort;
        port_fsm = PORT_NEW_FMC_REP;
      }
      break;
    case PORT_NEW_FMC_REP:
      if(!sTcpPortsOpenFeedback.empty())
      {
        bool fed = sTcpPortsOpenFeedback.read();
        if(fed)
        {
          processed_FMC_listen_port = current_requested_port;
          printf("FMC Port opened: %#03x\n",(int) processed_FMC_listen_port);
        } else {
          printf("[ERROR] FMC TCP port opening failed.\n");
          //TODO: add block list for ports? otherwise we will try it again and again
        }
        current_port_update = NalPortUpdate(FMC, processed_FMC_listen_port);
        port_fsm = PORT_SEND_UPDATE;
      }
      break;
    case PORT_NEW_UDP_REQ:
      if(!sUdpPortsToOpen.full())
      {
        ap_uint<32> tmp = udp_rx_ports_processed | *pi_udp_rx_ports;
        ap_uint<32> diff = udp_rx_ports_processed ^ tmp;
        //printf("rx_ports IN: %#04x\n",(int) *pi_udp_rx_ports);
        //printf("udp_rx_ports_processed: %#04x\n",(int) udp_rx_ports_processed);
        printf("UDP port diff: %#04x\n",(unsigned int) diff);
        if(diff != 0)
        {//we have to open new ports, one after another
          new_relative_port_to_req_udp = getRightmostBitPos(diff);
          UdpPort new_port_to_open = NAL_RX_MIN_PORT + new_relative_port_to_req_udp;
          sUdpPortsToOpen.write(new_port_to_open);
          port_fsm = PORT_NEW_UDP_REP;
        } else {
          udp_rx_ports_processed = *pi_udp_rx_ports;
          port_fsm = PORT_IDLE;
        }
      }
      break;
    case PORT_NEW_UDP_REP:
      if(!sUdpPortsOpenFeedback.empty())
      {
        bool fed = sUdpPortsOpenFeedback.read();
        if(fed)
        {
          udp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_udp);
          printf("new udp_rx_ports_processed: %#03x\n",(int) udp_rx_ports_processed);
        } else {
          printf("[ERROR] UDP port opening failed.\n");
          //TODO: add block list for ports? otherwise we will try it again and again
        }
        //in all cases
        current_port_update = NalPortUpdate(UDP, udp_rx_ports_processed);
        port_fsm = PORT_SEND_UPDATE;
      }
      break;
    case PORT_NEW_TCP_REQ:
      if( !sTcpPortsToOpen.full() )
      {
        ap_uint<32> tmp = tcp_rx_ports_processed | *pi_tcp_rx_ports;
        ap_uint<32> diff = tcp_rx_ports_processed ^ tmp;
        //printf("rx_ports IN: %#04x\n",(int) *pi_tcp_rx_ports);
        //printf("tcp_rx_ports_processed: %#04x\n",(int) tcp_rx_ports_processed);
        printf("TCP port diff: %#04x\n",(unsigned int) diff);
        if(diff != 0)
        {//we have to open new ports, one after another
          new_relative_port_to_req_tcp = getRightmostBitPos(diff);
          TcpPort new_port = NAL_RX_MIN_PORT + new_relative_port_to_req_tcp;
          sTcpPortsToOpen.write(new_port);
          port_fsm = PORT_NEW_TCP_REP;
        } else {
          tcp_rx_ports_processed = *pi_tcp_rx_ports;
          port_fsm = PORT_IDLE;
        }
      }
      break;
    case PORT_NEW_TCP_REP:
      if(!sTcpPortsOpenFeedback.empty())
      {
        bool fed = sTcpPortsOpenFeedback.read();
        if(fed)
        {
          tcp_rx_ports_processed |= ((ap_uint<32>) 1) << (new_relative_port_to_req_tcp);
          printf("new tcp_rx_ports_processed: %#03x\n",(int) tcp_rx_ports_processed);
        } else {
          printf("[ERROR] TCP port opening failed.\n");
          //TODO: add block list for ports? otherwise we will try it again and again
        }
        //in all cases
        current_port_update = NalPortUpdate(TCP, tcp_rx_ports_processed);
        port_fsm = PORT_SEND_UPDATE;
      }
      break;
    case PORT_SEND_UPDATE:
      if(!sPortUpdate.full())
      {
        sPortUpdate.write(current_port_update);
        port_fsm = PORT_IDLE;
      }
      break;
  }
}


/*****************************************************************************
 * @brief Detects if the caches of the USS and TSS have to be invalidated and 
 *        signals this to the concerned processes.
 *
 * @param[in]   layer_4_enabled,        external signal if layer 4 is enabled
 * @param[in]   layer_7_enabled,        external signal if layer 7 is enabled
 * @param[in]   role_decoupled,         external signal if the role is decoupled
 * @param[in]   piNTS_ready,            external signal if NTS is up and running
 * @param[in]   mrt_version_update,     notification of MRT version change
 * @param[in]   inval_del_sig,          notification of connection closing
 * @param[out]  cache_inval_0,          signals that caches must be invalidated
 * @param[out]  cache_inval_1,          signals that caches must be invalidated
 * @param[out]  cache_inval_2,          signals that caches must be invalidated
 * @param[out]  cache_inval_3,          signals that caches must be invalidated
 *
 ******************************************************************************/
void pCacheInvalDetection(
    ap_uint<1>        *layer_4_enabled,
    ap_uint<1>        *layer_7_enabled,
    ap_uint<1>        *role_decoupled,
    ap_uint<1>        *piNTS_ready,
    stream<uint32_t>  &mrt_version_update,
    stream<bool>      &inval_del_sig,
    stream<bool>      &cache_inval_0,
    stream<bool>      &cache_inval_1,
    stream<bool>      &cache_inval_2, //MUST be connected to TCP
    stream<bool>      &cache_inval_3  //MUST be connected to TCP
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static CacheInvalFsmStates cache_fsm = CACHE_WAIT_FOR_VALID;
  static uint32_t mrt_version_current = 0;

#pragma HLS RESET variable=cache_fsm
#pragma HLS RESET variable=mrt_version_current

  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static ap_uint<1> role_state;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  //===========================================================

  switch (cache_fsm)
  {
    default:
    case CACHE_WAIT_FOR_VALID:
      if(*layer_4_enabled == 1 && *piNTS_ready == 1)
      {
        role_state = *role_decoupled;
        cache_fsm = CACHE_VALID;
      }
      break;
    case CACHE_VALID:
      if(*layer_4_enabled == 0 || *piNTS_ready == 0 || *role_decoupled != role_state || *layer_7_enabled == 0)
      {
        if(*role_decoupled == 0 || *layer_4_enabled == 0 || *piNTS_ready == 0)
        { //not layer_7_enabled == 0, since this would also be valid during PR
          //role_decoupled == 0, if layer_7_enabled == 0 but no PR, so we invalidate the cache
          //but wait until a PR is done

          //i.e. after a PR
          cache_fsm = CACHE_INV_SEND_0;
          printf("[pCacheInvalDetection] Detected cache invalidation condition!\n");
        } else {
          //not yet invalid
          role_state = *role_decoupled;
        }
      } else if(!mrt_version_update.empty())
      {
        uint32_t tmp = mrt_version_update.read();
        if(tmp != mrt_version_current)
        {
          mrt_version_current = tmp;
          cache_fsm = CACHE_INV_SEND_0;
        }
      } else if(!inval_del_sig.empty())
      {
        bool sig = inval_del_sig.read();
        if(sig)
        {
          cache_fsm = CACHE_INV_SEND_2;
          //we only need to invalidate the Cache of the TCP processes
          //(2 and 3)
        }
      }
      break;
    case CACHE_INV_SEND_0:
      //UDP RX
      if(!cache_inval_0.full())
      {
        cache_inval_0.write(true);
        cache_fsm = CACHE_INV_SEND_1;
      }
      break;
    case CACHE_INV_SEND_1:
      //UDP TX
      if(!cache_inval_1.full())
      {
        cache_inval_1.write(true);
        cache_fsm = CACHE_INV_SEND_2;
      }
      break;
    case CACHE_INV_SEND_2:
      //TCP RDp
      if(!cache_inval_2.full())
      {
        cache_inval_2.write(true);
        cache_fsm = CACHE_INV_SEND_3;
      }
      break;
    case CACHE_INV_SEND_3:
      //TCP WRp
      if(!cache_inval_3.full())
      {
        cache_inval_3.write(true);
        cache_fsm = CACHE_WAIT_FOR_VALID;
      }
      break;
  }
}


/*****************************************************************************
 * @brief Contains the SessionId-Triple CAM for TCP sessions. It replies to 
 *        stram requests.
 *
 * @param[in]   sGetTripleFromSid_Req,       Request stream to get the Tcp Triple to a SessionId
 * @param[out]  sGetTripleFromSid_Rep,       Reply stream containing Tcp Triple
 * @param[in]   sGetSidFromTriple_Req,       Request stream to get the SessionId to a Tcp Triple
 * @param[out]  sGetSidFromTriple_Rep,       Reply stream containing the SessionId
 * @param[in]   sAddNewTriple_TcpRrh,        Stream containing new SessionIds with Triples (from TcpRRh)
 * @param[out]  sAddNewTriple_TcpCon,        Stream containing new SessionIds with Triples (from TcpCOn)
 * @param[in]   sDeleteEntryBySid,           Notification to mark a table entry as closed
 * @param[out]  inval_del_sig,               Notification of connection closing to Cache Invalidation Logic
 * @param[in]   sMarkAsPriv,                 Notification to mark a session as prvileged
 * @param[in]   sMarkToDel_unpriv,           Signal to mark all un-privileged sessions as to-be-deleted
 * @param[in]   sGetNextDelRow_Req,          Request to get the next sesseion that is marked as to-be-deleted
 * @param[out]  sGetNextDelRow_Rep,          Reply containin the SessionId of the next to-be-deleted session
 *
 ******************************************************************************/
void pTcpAgency(
    stream<SessionId>         &sGetTripleFromSid_Req,
    stream<NalTriple>         &sGetTripleFromSid_Rep,
    stream<NalTriple>         &sGetSidFromTriple_Req,
    stream<SessionId>         &sGetSidFromTriple_Rep,
    stream<NalNewTableEntry>  &sAddNewTriple_TcpRrh,
    stream<NalNewTableEntry>  &sAddNewTriple_TcpCon,
    stream<SessionId>         &sDeleteEntryBySid,
    stream<bool>              &inval_del_sig,
    stream<SessionId>         &sMarkAsPriv,
    stream<bool>              &sMarkToDel_unpriv,
    stream<bool>              &sGetNextDelRow_Req,
    stream<SessionId>         &sGetNextDelRow_Rep
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  //static  TableFsmStates agencyFsm = TAB_FSM_READ;
  static  bool tables_initialized = false;

  //#pragma HLS RESET variable=agencyFsm
#pragma HLS RESET variable=tables_initialized
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static NalTriple  tripleList[MAX_NAL_SESSIONS];
  static SessionId   sessionIdList[MAX_NAL_SESSIONS];
  static ap_uint<1>  usedRows[MAX_NAL_SESSIONS];
  static ap_uint<1>  rowsToDelete[MAX_NAL_SESSIONS];
  static ap_uint<1>  privilegedRows[MAX_NAL_SESSIONS];

#pragma HLS ARRAY_PARTITION variable=tripleList complete dim=1
#pragma HLS ARRAY_PARTITION variable=sessionIdList complete dim=1
#pragma HLS ARRAY_PARTITION variable=usedRows complete dim=1
#pragma HLS ARRAY_PARTITION variable=rowsToDelete complete dim=1
#pragma HLS ARRAY_PARTITION variable=privilegedRows complete dim=1

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------


  if (!tables_initialized)
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
    tables_initialized = true;
  } else {

    //switch(agencyFsm)
    //{
    //case TAB_FSM_READ:
    if(!sGetTripleFromSid_Req.empty() && !sGetTripleFromSid_Rep.full())
    {
      SessionId sessionID = sGetTripleFromSid_Req.read();
      printf("searching for session: %d\n", (int) sessionID);
      uint32_t i = 0;
      NalTriple ret = UNUSED_TABLE_ENTRY_VALUE;
      bool found_smth = false;
      for(i = 0; i < MAX_NAL_SESSIONS; i++)
      {
        //#pragma HLS unroll factor=8
        if(sessionIdList[i] == sessionID && usedRows[i] == 1 && rowsToDelete[i] == 0)
        {
          ret = tripleList[i];
          printf("found triple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) ret);
          found_smth = true;
          break;
        }
      }
      if(!found_smth)
      {
        //unkown session TODO
        printf("[TcpAgency:INFO] Unknown session requested\n");
      }
      sGetTripleFromSid_Rep.write(ret);
    } else if(!sGetSidFromTriple_Req.empty() && !sGetSidFromTriple_Rep.full())
    {
      NalTriple triple = sGetSidFromTriple_Req.read();
      printf("Searching for triple: %llu\n", (unsigned long long) triple);
      uint32_t i = 0;
      SessionId ret = UNUSED_SESSION_ENTRY_VALUE;
      bool found_smth = false;
      for(i = 0; i < MAX_NAL_SESSIONS; i++)
      {
        //#pragma HLS unroll factor=8
        if(tripleList[i] == triple && usedRows[i] == 1 && rowsToDelete[i] == 0)
        {
          ret = sessionIdList[i];
          found_smth = true;
          break;
        }
      }
      if(!found_smth)
      {
        //there is (not yet) a connection TODO
        printf("[TcpAgency:INFO] Unknown triple requested\n");
      }
      sGetSidFromTriple_Rep.write(ret);
    } else
      // agencyFsm = TAB_FSM_WRITE;
      // break;
      //case TAB_FSM_WRITE:
      if(!sAddNewTriple_TcpRrh.empty() || !sAddNewTriple_TcpCon.empty())
      {
        NalNewTableEntry ne_struct;
        if(!sAddNewTriple_TcpRrh.empty())
        {
          ne_struct = sAddNewTriple_TcpRrh.read();
        } else {
          ne_struct = sAddNewTriple_TcpCon.read();
        }
        SessionId sessionID = ne_struct.sessId;
        NalTriple new_entry = ne_struct.new_triple;
        printf("new tripple entry: %d |  %llu\n",(int) sessionID, (unsigned long long) new_entry);
        //first check for duplicates!
        //ap_uint<64> test_tripple = getTrippleFromSessionId(sessionID);
        uint32_t i = 0;
        SessionId ret = UNUSED_SESSION_ENTRY_VALUE;
        bool found_smth = false;
        for(i = 0; i < MAX_NAL_SESSIONS; i++)
        {
          //#pragma HLS unroll factor=8
          if(sessionIdList[i] == sessionID && usedRows[i] == 1 && rowsToDelete[i] == 0)
          {
            ret = tripleList[i];
            printf("found triple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) ret);
            found_smth = true;
            break;
          }
        }
        if(found_smth)
        {
          printf("session/triple already known, skipping. \n");
          //break; no break, because other may want to run too
        } else {
          bool stored = false;
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
              printf("stored triple entry: %d | %d |  %llu\n",(int) i, (int) sessionID, (unsigned long long) new_entry);
              stored = true;
              break;
            }
          }
          if(!stored)
          {
            //we run out of sessions... TODO
            //actually, should not happen, since we have same table size as TOE
            printf("[TcpAgency:ERROR] no free space left in table!\n");
          }
        }
      } else if(!sDeleteEntryBySid.empty() && !inval_del_sig.full())
      {
        SessionId sessionID = sDeleteEntryBySid.read();
        printf("try to delete session: %d\n", (int) sessionID);
        for(uint32_t i = 0; i < MAX_NAL_SESSIONS; i++)
        {
          //#pragma HLS unroll factor=8
          if(sessionIdList[i] == sessionID && usedRows[i] == 1)
          {
            usedRows[i] = 0;
            privilegedRows[i] = 0;
            printf("found and deleting session: %d\n", (int) sessionID);
            //notify cache invalidation
            inval_del_sig.write(true);
            break;
          }
        }
        //nothing to delete, nothing to do...
      } else if(!sMarkAsPriv.empty())
      {
        SessionId sessionID = sMarkAsPriv.read();
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
      } else if(!sMarkToDel_unpriv.empty())
      {
        if(sMarkToDel_unpriv.read())
        {
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
      } else if(!sGetNextDelRow_Req.empty() && !sGetNextDelRow_Rep.full())
      {
        if(sGetNextDelRow_Req.read())
        {
          SessionId ret = UNUSED_SESSION_ENTRY_VALUE;
          bool found_smth = false;
          for(uint32_t i = 0; i< MAX_NAL_SESSIONS; i++)
          {
            //#pragma HLS unroll factor=8
            if(rowsToDelete[i] == 1)
            {
              ret = sessionIdList[i];
              //sessionIdList[i] = 0x0; //not necessary
              //tripleList[i] = 0x0;
              usedRows[i] = 0;
              rowsToDelete[i] = 0;
              //privilegedRows[i] = 0; //not necessary
              printf("Closing session %d at table row %d.\n",(int) ret, (int) i);
              found_smth = true;
              break;
            }
          }
          if(!found_smth)
          {
            //Tables are empty
            printf("TCP tables are empty\n");
          }
          sGetNextDelRow_Rep.write(ret);
        }
      }
    //agencyFsm = TAB_FSM_READ;
    //break;
    //} //switch
  } // else
}


/*! \} */


