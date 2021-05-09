/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 */

// *****************************************************************************
// *
// * Title : Toplevel of the TCP/IP subsystem stack instantiated by the SHELL.
// *
// * File    : nts_TcpIp.v
// *
// * Tools   : Vivado v2016.4, v2017.4 (64-bit)
// *
// * Description : This is the toplevel design of the TCP/IP-based networking
// *    subsystem that is instantiated by the shell of the current target
// *    platform. It is used to transfer data sequences between the user
// *    application layer and the underlaying Ethernet media layer.
// *    From an Open Systems Interconnection (OSI) model point of view, this
// *    module implements the Network layer (L3) and the Transport layer (L4).
// * 
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - IP NETWORK + TCP/UDP TRANSPORT
// *****************************************************************************

module NetworkTransportStack_TcpIp (

  //------------------------------------------------------
  //-- Global Clock used by the entire SHELL
  //--   (This is typically 'sETH0_ShlClk' and we use it all over the place)
  //------------------------------------------------------ 
  input          piShlClk,
  
  //------------------------------------------------------
  //-- Global Reset used by the entire SHELL
  //--  This is typically 'sETH0_ShlRst'. If the module is created by HLS,
  //--   we use it as the default startup reset of the module.)
  //------------------------------------------------------ 
  input          piShlRst,
   
  //------------------------------------------------------
  //-- ETH / Ethernet Layer-2 Interfaces
  //------------------------------------------------------
  //--  Axi4-Stream Ethernet Rx Data --------
  input [ 63:0]  siETH_Data_tdata, 
  input [  7:0]  siETH_Data_tkeep,
  input          siETH_Data_tlast,
  input          siETH_Data_tvalid,
  output         siETH_Data_tready,
  //-- Axi4-Stream Ethernet Tx Data --------
  output [ 63:0] soETH_Data_tdata,
  output [  7:0] soETH_Data_tkeep,
  output         soETH_Data_tlast,
  output         soETH_Data_tvalid,
  input          soETH_Data_tready,
 
  //------------------------------------------------------
  //-- MEM / TxP Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path / S2MM-AXIS --------------------
  //---- Axi4-Stream Read Command -----------
  output[ 79:0]  soMEM_TxP_RdCmd_tdata,
  output         soMEM_TxP_RdCmd_tvalid,
  input          soMEM_TxP_RdCmd_tready,
  //---- Axi4-Stream Read Status ------------
  input [  7:0]  siMEM_TxP_RdSts_tdata,
  input          siMEM_TxP_RdSts_tvalid,
  output         siMEM_TxP_RdSts_tready,
  //---- Axi4-Stream Data Input Channel ----
  input [ 63:0]  siMEM_TxP_Data_tdata,
  input [  7:0]  siMEM_TxP_Data_tkeep,
  input          siMEM_TxP_Data_tlast,
  input          siMEM_TxP_Data_tvalid,
  output         siMEM_TxP_Data_tready,
  //---- Axi4-Stream Write Command ----------
  output [ 79:0] soMEM_TxP_WrCmd_tdata,
  output         soMEM_TxP_WrCmd_tvalid,
  input          soMEM_TxP_WrCmd_tready,
  //---- Axi4-Stream Write Status -----------
  input [  7:0]  siMEM_TxP_WrSts_tdata,
  input          siMEM_TxP_WrSts_tvalid,
  output         siMEM_TxP_WrSts_tready,
  //---- Axi4-Stream Data Output Channel ----
  output [ 63:0] soMEM_TxP_Data_tdata,
  output [  7:0] soMEM_TxP_Data_tkeep,
  output         soMEM_TxP_Data_tlast,
  output         soMEM_TxP_Data_tvalid,
  input          soMEM_TxP_Data_tready,

  //------------------------------------------------------
  //-- MEM / RxP Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path / S2MM-AXIS -------------
  //---- Axi4-Stream Read Command -----------
  output [ 79:0] soMEM_RxP_RdCmd_tdata,
  output         soMEM_RxP_RdCmd_tvalid,
  input          soMEM_RxP_RdCmd_tready,
  //---- Axi4-Stream Read Status ------------
  input [   7:0] siMEM_RxP_RdSts_tdata,
  input          siMEM_RxP_RdSts_tvalid,
  output         siMEM_RxP_RdSts_tready,
  //---- Axi4-Stream Data Input Channel -----
  input [ 63:0]  siMEM_RxP_Data_tdata,
  input [  7:0]  siMEM_RxP_Data_tkeep,
  input          siMEM_RxP_Data_tlast,
  input          siMEM_RxP_Data_tvalid,
  output         siMEM_RxP_Data_tready,
  //---- Axi4-Stream Write Command ----------
  output[ 79:0]  soMEM_RxP_WrCmd_tdata,
  output         soMEM_RxP_WrCmd_tvalid,
  input          soMEM_RxP_WrCmd_tready,
  //---- Axi4-Stream Write Status -----------
  input [  7:0]  siMEM_RxP_WrSts_tdata,
  input          siMEM_RxP_WrSts_tvalid,
  output         siMEM_RxP_WrSts_tready,
  //---- Axi4-Stream Data Input Channel -----
  output [ 63:0] soMEM_RxP_Data_tdata,
  output [  7:0] soMEM_RxP_Data_tkeep,
  output         soMEM_RxP_Data_tlast,
  output         soMEM_RxP_Data_tvalid,
  input          soMEM_RxP_Data_tready, 

  //------------------------------------------------------
  //-- UAIF / UDP Tx Data Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ---------------
  input   [63:0] siAPP_Udp_Data_tdata,
  input   [ 7:0] siAPP_Udp_Data_tkeep,
  input          siAPP_Udp_Data_tlast,
  input          siAPP_Udp_Data_tvalid,
  output         siAPP_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata -----------
  input   [95:0] siAPP_Udp_Meta_tdata,
  input          siAPP_Udp_Meta_tvalid,
  output         siAPP_Udp_Meta_tready,
  //---- Axi4-Stream UDP Data Length ---------
  input   [15:0] siAPP_Udp_DLen_tdata,
  input          siAPP_Udp_DLen_tvalid,
  output         siAPP_Udp_DLen_tready,
  
  //------------------------------------------------------
  //-- UAIF / Rx Data Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ---------------
  output  [63:0] soAPP_Udp_Data_tdata,
  output  [ 7:0] soAPP_Udp_Data_tkeep,
  output         soAPP_Udp_Data_tlast,
  output         soAPP_Udp_Data_tvalid,
  input          soAPP_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata -----------
  output  [95:0] soAPP_Udp_Meta_tdata,
  output         soAPP_Udp_Meta_tvalid,
  input          soAPP_Udp_Meta_tready,
    
  //------------------------------------------------------
  //-- UAIF / UDP Rx Ctrl Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Listen Request -----
  input   [15:0] siAPP_Udp_LsnReq_tdata,
  input          siAPP_Udp_LsnReq_tvalid,
  output         siAPP_Udp_LsnReq_tready,
  //---- Axi4-Stream UDP Listen Reply -------
  output  [ 7:0] soAPP_Udp_LsnRep_tdata,
  output         soAPP_Udp_LsnRep_tvalid,
  input          soAPP_Udp_LsnRep_tready,
  //---- Axi4-Stream UDP Close Request ------
  input   [15:0] siAPP_Udp_ClsReq_tdata,
  input          siAPP_Udp_ClsReq_tvalid,
  output         siAPP_Udp_ClsReq_tready,
  //---- Axi4-Stream UDP Close Reply --------
  output  [ 7:0] soAPP_Udp_ClsRep_tdata,
  output         soAPP_Udp_ClsRep_tvalid,
  input          soAPP_Udp_ClsRep_tready,
  
  //------------------------------------------------------
  //-- TAIF / Tx Data Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream APP Data ---------------
  input [ 63:0]  siAPP_Tcp_Data_tdata,
  input [  7:0]  siAPP_Tcp_Data_tkeep,
  input          siAPP_Tcp_Data_tvalid,
  input          siAPP_Tcp_Data_tlast,
  output         siAPP_Tcp_Data_tready,
  //---- Axi4-Stream APP Send Request -------
  input [ 31:0]  siAPP_Tcp_SndReq_tdata,
  input          siAPP_Tcp_SndReq_tvalid,
  output         siAPP_Tcp_SndReq_tready,
  //---- Axi4-Stream APP Send Reply ---------
  output [ 55:0] soAPP_Tcp_SndRep_tdata,
  output         soAPP_Tcp_SndRep_tvalid,
  input          soAPP_Tcp_SndRep_tready,

  //------------------------------------------------------
  //-- TAIF / Rx Data Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //-- Axi4-Stream TCP Data -----------------
  output [ 63:0] soAPP_Tcp_Data_tdata,
  output [  7:0] soAPP_Tcp_Data_tkeep,
  output         soAPP_Tcp_Data_tvalid,
  output         soAPP_Tcp_Data_tlast,
  input          soAPP_Tcp_Data_tready,
  //--  Axi4-Stream TCP Metadata ------------
  output [ 15:0] soAPP_Tcp_Meta_tdata,
  output         soAPP_Tcp_Meta_tvalid,
  input          soAPP_Tcp_Meta_tready,
  //--  Axi4-Stream TCP Data Notification ---
  output [103:0] soAPP_Tcp_Notif_tdata,
  output         soAPP_Tcp_Notif_tvalid,
  input          soAPP_Tcp_Notif_tready,
  //--  Axi4-Stream TCP Data Request --------
  input  [ 31:0] siAPP_Tcp_DReq_tdata,
  input          siAPP_Tcp_DReq_tvalid,
  output         siAPP_Tcp_DReq_tready,
  
  //------------------------------------------------------
  //-- TAIF / Tx Ctlr Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream TCP Open Session Request
  input [ 47:0]  siAPP_Tcp_OpnReq_tdata,
  input          siAPP_Tcp_OpnReq_tvalid,
  output         siAPP_Tcp_OpnReq_tready,
  //---- Axi4-Stream TCP Open Session Reply
  output [ 23:0] soAPP_Tcp_OpnRep_tdata,
  output         soAPP_Tcp_OpnRep_tvalid,
  input          soAPP_Tcp_OpnRep_tready,
  //---- Axi4-Stream TCP Close Request ------
  input [ 15:0]  siAPP_Tcp_ClsReq_tdata,
  input          siAPP_Tcp_ClsReq_tvalid,
  output         siAPP_Tcp_ClsReq_tready,
  
  //------------------------------------------------------
  //-- TAIF / Rx Ctlr Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //----  Axi4-Stream TCP Listen Request ----
  input [ 15:0]  siAPP_Tcp_LsnReq_tdata,
  input          siAPP_Tcp_LsnReq_tvalid,
  output         siAPP_Tcp_LsnReq_tready,
  //----  Axi4-Stream TCP Listen Ack --------
  output [  7:0] soAPP_Tcp_LsnRep_tdata,  // RepBit stream must be 8-bits boundary
  output         soAPP_Tcp_LsnRep_tvalid,
  input          soAPP_Tcp_LsnRep_tready,
 
  //------------------------------------------------------
  //-- MMIO / Interfaces
  //------------------------------------------------------
  input          piMMIO_Layer2Rst,
  input          piMMIO_Layer3Rst,
  input          piMMIO_Layer4Rst,
  input          piMMIO_Layer4En,
  input  [ 47:0] piMMIO_MacAddress,
  input  [ 31:0] piMMIO_Ip4Address,
  input  [ 31:0] piMMIO_SubNetMask,
  input  [ 31:0] piMMIO_GatewayAddr,
  output         poMMIO_CamReady,
  output         poMMIO_NtsReady
  
); // End of PortList


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire          sLOW_1b0   =  1'b0;
  wire          sHIGH_1b1  =  1'b1;
  wire  [ 7:0]  sHIGH_8b1  =  8'b11111111;

  //------------------------------------------------------
  //-- IPRX = IP-RX-HANDLER
  //------------------------------------------------------
  //-- IPRX ==>[ARS0]==> ARP / Data
  //---- IPRX ==>[ARS0]
  wire  [63:0]  ssIPRX_ARS0_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS0_Data_tkeep;
  wire          ssIPRX_ARS0_Data_tlast;
  wire          ssIPRX_ARS0_Data_tvalid;
  wire          ssIPRX_ARS0_Data_tready;
  //             [ARS0]==> ARP/Data
  wire  [63:0]  ssARS0_ARP_Data_tdata;
  wire  [ 7:0]  ssARS0_ARP_Data_tkeep;
  wire          ssARS0_ARP_Data_tlast;
  wire          ssARS0_ARP_Data_tvalid;
  wire          ssARS0_ARP_Data_tready;
  //-- IPRX ==>[ARS1]==> ICMP/Data
  //---- IPRX ==>[ARS1] 
  wire  [63:0]  ssIPRX_ARS1_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS1_Data_tkeep;
  wire          ssIPRX_ARS1_Data_tlast;
  wire          ssIPRX_ARS1_Data_tvalid;
  wire          ssIPRX_ARS1_Data_tready;
  //             [ARS1]==> ICMP/Data
  wire  [63:0]  ssARS1_ICMP_Data_tdata;
  wire  [ 7:0]  ssARS1_ICMP_Data_tkeep;
  wire          ssARS1_ICMP_Data_tlast;
  wire          ssARS1_ICMP_Data_tvalid;
  wire          ssARS1_ICMP_Data_tready;
  //OBSOLETE //-- IPRX ==> ICMP/Derr
  //OBSOLETE wire  [63:0]  ssIPRX_ICMP_Derr_tdata;
  //OBSOLETE wire  [ 7:0]  ssIPRX_ICMP_Derr_tkeep;
  //OBSOLETE wire          ssIPRX_ICMP_Derr_tlast;
  //OBSOLETE wire          ssIPRX_ICMP_Derr_tvalid;
  //OBSOLETE wire          ssIPRX_ICMP_Derr_tready;
  //-- IPRX ==>[ARS5]==> ICMP/Derr
  //---- IPRX ==>[ARS5] 
  wire  [63:0]  ssIPRX_ARS5_Derr_tdata;
  wire  [ 7:0]  ssIPRX_ARS5_Derr_tkeep;
  wire          ssIPRX_ARS5_Derr_tlast;
  wire          ssIPRX_ARS5_Derr_tvalid;
  wire          ssIPRX_ARS5_Derr_tready;
  //             [ARS5]==> ICMP/Derr
  wire  [63:0]  ssARS5_ICMP_Derr_tdata;
  wire  [ 7:0]  ssARS5_ICMP_Derr_tkeep;
  wire          ssARS5_ICMP_Derr_tlast;
  wire          ssARS5_ICMP_Derr_tvalid;
  wire          ssARS5_ICMP_Derr_tready;  
  //-- IPRX ==>[ARS2]==> TOE
  //---- IPRX ==>[ARS2]
  wire  [63:0]  ssIPRX_ARS2_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS2_Data_tkeep;
  wire          ssIPRX_ARS2_Data_tlast;
  wire          ssIPRX_ARS2_Data_tvalid;
  wire          ssIPRX_ARS2_Data_tready;
  //             [ARS2]==> TOE
  wire  [63:0]  ssARS2_TOE_Data_tdata;
  wire  [ 7:0]  ssARS2_TOE_Data_tkeep;
  wire          ssARS2_TOE_Data_tlast;
  wire          ssARS2_TOE_Data_tvalid;
  wire          ssARS2_TOE_Data_tready;  
  //-- IPRX ==>[ARS3]==> UOE
  //---- IPRX ==>[ARS3]
  wire  [63:0]  ssIPRX_ARS3_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS3_Data_tkeep;
  wire          ssIPRX_ARS3_Data_tlast;
  wire          ssIPRX_ARS3_Data_tvalid;
  wire          ssIPRX_ARS3_Data_tready;
  //             [ARS3]==> UOE
  wire  [63:0]  ssARS3_UOE_Data_tdata;
  wire  [ 7:0]  ssARS3_UOE_Data_tkeep;
  wire          ssARS3_UOE_Data_tlast;
  wire          ssARS3_UOE_Data_tvalid;
  wire          ssARS3_UOE_Data_tready;
 
  //------------------------------------------------------------------
  //-- UOE = UDP-OFFLOAD-ENGINE
  //------------------------------------------------------------------
  //-- UOE ==>[ARS6]==>
  //---- UOE ==>[ARS6]
  wire  [ 7:0]  ssUOE_ARS6_Ready_tdata;
  wire          ssUOE_ARS6_Ready_tvalid;
  wire          ssUOE_ARS6_Ready_tready;  
  //-- ARS6 ==> RLB / ReadyLogicBarrier
  wire  [ 7:0]  ssARS6_RLB_Ready_tdata;
  wire          ssARS6_RLB_Ready_tvalid;
  wire          ssARS6_RLB_Ready_tready;
  //-- UOE ==> L3MUX / Data
  wire  [63:0]  ssUOE_L3MUX_Data_tdata;
  wire  [ 7:0]  ssUOE_L3MUX_Data_tkeep;
  wire          ssUOE_L3MUX_Data_tlast;
  wire          ssUOE_L3MUX_Data_tvalid;
  wire          ssUOE_L3MUX_Data_tready;
  //-- UOE ==> ICMP / Data
  wire  [63:0]  ssUOE_ICMP_Data_tdata;
  wire  [ 7:0]  ssUOE_ICMP_Data_tkeep;
  wire          ssUOE_ICMP_Data_tlast;
  wire          ssUOE_ICMP_Data_tvalid;
  wire          ssUOE_ICMP_Data_tready;
  
  //------------------------------------------------------------------
  //-- TOE = TCP-OFFLOAD-ENGINE
  //------------------------------------------------------------------
  //-- TOE ==> RLB / ReadyLogicBarrier
  wire  [ 7:0]  ssTOE_RLB_Ready_tdata;
  wire          ssTOE_RLB_Ready_tvalid;
  wire          ssTOE_RLB_Ready_tready;
  //-- TOE ==>[ARS4]==> L3MUX / Data
  //---- TOE ==> [ARS4]
  wire  [63:0]  ssTOE_ARS4_Data_tdata;
  wire  [ 7:0]  ssTOE_ARS4_Data_tkeep;
  wire          ssTOE_ARS4_Data_tlast;
  wire          ssTOE_ARS4_Data_tvalid;
  wire          ssTOE_ARS4_Data_tready;
  //----         [ARS4] ==> L3MUX
  wire  [63:0]  ssARS4_L3MUX_Data_tdata;
  wire  [ 7:0]  ssARS4_L3MUX_Data_tkeep;
  wire          ssARS4_L3MUX_Data_tlast;
  wire          ssARS4_L3MUX_Data_tvalid;
  wire          ssARS4_L3MUX_Data_tready;
  //-- TOE ==> CAM / LookupRequest
  wire [103:0]  ssTOE_CAM_LkpReq_tdata;  //( 1 + 96) - 1 = 96  but HLS aligns to the next 8-bit boundary 
  wire          ssTOE_CAM_LkpReq_tvalid;
  wire          ssTOE_CAM_LkpReq_tready;
  //-- TOE ==> CAM / UpdateRequest
  wire  [111:0] ssTOE_CAM_UpdReq_tdata;  //( 1 + 1 + 14 + 96) - 1 = 111
  wire          ssTOE_CAM_UpdReq_tvalid;
  wire          ssTOE_CAM_UpdReq_tready;
 
  //------------------------------------------------------------------
  //-- CAM = CONTENT ADDRESSABLE MEMORY
  //------------------------------------------------------------------
  //-- CAM ==> TOE / LookupReply
  wire  [15:0]  ssCAM_TOE_LkpRep_tdata;
  wire          ssCAM_TOE_LkpRep_tvalid;
  wire          ssCAM_TOE_LkpRep_tready;
  //-- CAM ==> TOE / UpdateReply
  wire  [15:0]  ssCAM_TOE_UpdRpl_tdata;
  wire          ssCAM_TOE_UpdRpl_tvalid;
  wire          ssCAM_TOE_UpdRpl_tready;

  //------------------------------------------------------------------
  //-- ICMP = ICMP-SERVER
  //------------------------------------------------------------------
  //-- ICMP ==> L3MUX / Data
  wire  [63:0]  ssICMP_L3MUX_Data_tdata;
  wire  [ 7:0]  ssICMP_L3MUX_Data_tkeep;
  wire          ssICMP_L3MUX_Data_tlast;
  wire          ssICMP_L3MUX_Data_tvalid;
  wire          ssICMP_L3MUX_Data_tready;

  //------------------------------------------------------------------
  //-- ARP = ARP-SERVER
  //------------------------------------------------------------------
  //-- ARP ==> L2MUX / Data
  wire  [63:0]  ssARP_L2MUX_Data_tdata;
  wire  [ 7:0]  ssARP_L2MUX_Data_tkeep;
  wire          ssARP_L2MUX_Data_tlast;
  wire          ssARP_L2MUX_Data_tvalid;
  wire          ssARP_L2MUX_Data_tready;
  //-- ARP ==>[ARS7]==> ARP/MacLookupReply
  //---- ARP ==>[ARS7]
  wire  [55:0]  ssARP_ARS7_MacLkpRep_tdata;
  wire          ssARP_ARS7_MacLkpRep_tvalid;
  wire          ssARP_ARS7_MacLkpRep_tready;
  //            [ARS7]==> IPTX/MacLookupReply
  wire  [55:0]  ssARS7_IPTX_MacLkpRep_tdata;
  wire          ssARS7_IPTX_MacLkpRep_tvalid;
  wire          ssARS7_IPTX_MacLkpRep_tready;
  
  //------------------------------------------------------------------
  //-- IPTX = IP-TX-HANDLER
  //------------------------------------------------------------------
  //-- IPTX ==>[ARS8]==> ARP/MacLookupRequest
  //---- IPTX ==>[ARS8]
  wire  [31:0]  ssIPTX_ARS8_MacLkpReq_tdata;
  wire          ssIPTX_ARS8_MacLkpReq_tvalid;
  wire          ssIPTX_ARS8_MacLkpReq_tready;
  //             [ARS8]==> ARP/MacLookupRequest  
  wire  [31:0]  ssARS8_ARP_MacLkpReq_tdata;
  wire          ssARS8_ARP_MacLkpReq_tvalid;
  wire          ssARS8_ARP_MacLkpReq_tready;
  //-- IPTX ==> L2MUX / Data
  wire  [63:0]  ssIPTX_L2MUX_Data_tdata;
  wire  [ 7:0]  ssIPTX_L2MUX_Data_tkeep;
  wire          ssIPTX_L2MUX_Data_tlast;
  wire          ssIPTX_L2MUX_Data_tvalid;
  wire          ssIPTX_L2MUX_Data_tready;
  
  //------------------------------------------------------------------
  //-- L2MUX = LAYER-2-MULTIPLEXER
  //------------------------------------------------------------------ 

  //------------------------------------------------------------------
  //-- L3MUX = LAYER-3-MULTIPLEXER
  //------------------------------------------------------------------ 
  //-- L3MUX ==> IPTX / Data -----------
  wire  [63:0]  ssL3MUX_IPTX_Data_tdata;
  wire  [ 7:0]  ssL3MUX_IPTX_Data_tkeep;
  wire          ssL3MUX_IPTX_Data_tlast;
  wire          ssL3MUX_IPTX_Data_tvalid;
  wire          ssL3MUX_IPTX_Data_tready;
  
  //-- End of signal declarations ----------------------------------------------
 
  //============================================================================
  //  COMB: CONTINIOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================ 
  assign siMEM_TxP_RdSts_tready = sHIGH_1b1; // [FIXME - Add TxP_RdSts to TOE]
  assign siMEM_RxP_RdSts_tready = sHIGH_1b1; // [FIXME - Add RxP_RdSts to TOE]
  
  //============================================================================
  //  INST: IP-RX-HANDLER
  //============================================================================
  IpRxHandler IPRX (
    //------------------------------------------------------
    //-- From SHELL Interfaces
    //------------------------------------------------------
    //-- Global Clock & Reset
   `ifdef USE_DEPRECATED_DIRECTIVES
    .aclk                     (piShlClk),
    .aresetn                  (~piMMIO_Layer3Rst),
   `else
    .ap_clk                   (piShlClk),
    .ap_rst_n                 (~piMMIO_Layer3Rst),
   `endif 
    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_MacAddress_V      (piMMIO_MacAddress),
    .piMMIO_Ip4Address_V      (piMMIO_Ip4Address),
    //------------------------------------------------------
    //-- From ETH Interface
    //------------------------------------------------------
    .siETH_Data_TDATA         (siETH_Data_tdata),
    .siETH_Data_TKEEP         (siETH_Data_tkeep),
    .siETH_Data_TLAST         (siETH_Data_tlast),
    .siETH_Data_TVALID        (siETH_Data_tvalid),
    .siETH_Data_TREADY        (siETH_Data_tready),
    //------------------------------------------------------
    //-- ARP Interface (via [ARS0])
    //------------------------------------------------------
    //-- To  ARP / Data ----------------
    .soARP_Data_TDATA         (ssIPRX_ARS0_Data_tdata),
    .soARP_Data_TKEEP         (ssIPRX_ARS0_Data_tkeep),
    .soARP_Data_TLAST         (ssIPRX_ARS0_Data_tlast),
    .soARP_Data_TVALID        (ssIPRX_ARS0_Data_tvalid),
    .soARP_Data_TREADY        (ssIPRX_ARS0_Data_tready),
    //------------------------------------------------------
    //-- ICMP Interface (via ARS1)
    //------------------------------------------------------
    //-- To ICMP / Data ----------------
    .soICMP_Data_TDATA        (ssIPRX_ARS1_Data_tdata),
    .soICMP_Data_TKEEP        (ssIPRX_ARS1_Data_tkeep),
    .soICMP_Data_TLAST        (ssIPRX_ARS1_Data_tlast),
    .soICMP_Data_TVALID       (ssIPRX_ARS1_Data_tvalid),
    .soICMP_Data_TREADY       (ssIPRX_ARS1_Data_tready),
    //-- To ICMP / Ttl -----------------
    .soICMP_Derr_TDATA        (ssIPRX_ARS5_Derr_tdata),
    .soICMP_Derr_TKEEP        (ssIPRX_ARS5_Derr_tkeep),
    .soICMP_Derr_TLAST        (ssIPRX_ARS5_Derr_tlast),
    .soICMP_Derr_TVALID       (ssIPRX_ARS5_Derr_tvalid),
    .soICMP_Derr_TREADY       (ssIPRX_ARS5_Derr_tready),
    //------------------------------------------------------
    //-- UOE Interface (via ARS3)
    //------------------------------------------------------
    //-- To UOE / Data -----------------
    .soUOE_Data_TDATA         (ssIPRX_ARS3_Data_tdata),
    .soUOE_Data_TKEEP         (ssIPRX_ARS3_Data_tkeep),
    .soUOE_Data_TLAST         (ssIPRX_ARS3_Data_tlast),
    .soUOE_Data_TVALID        (ssIPRX_ARS3_Data_tvalid),
    .soUOE_Data_TREADY        (ssIPRX_ARS3_Data_tready),
    //------------------------------------------------------
    //-- TOE Interface (via ARS2)
    //------------------------------------------------------
    //-- To TOE / Data -----------------
    .soTOE_Data_TDATA         (ssIPRX_ARS2_Data_tdata),
    .soTOE_Data_TKEEP         (ssIPRX_ARS2_Data_tkeep),
    .soTOE_Data_TLAST         (ssIPRX_ARS2_Data_tlast),
    .soTOE_Data_TVALID        (ssIPRX_ARS2_Data_tvalid),
    .soTOE_Data_TREADY        (ssIPRX_ARS2_Data_tready)
  ); // End of IPRX

  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPRX ==>[ARS0]==> ARP)
  //============================================================================
  AxisRegisterSlice_64 ARS0 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPRX / Data --------------
    .s_axis_tdata   (ssIPRX_ARS0_Data_tdata),
    .s_axis_tkeep   (ssIPRX_ARS0_Data_tkeep),
    .s_axis_tlast   (ssIPRX_ARS0_Data_tlast),
    .s_axis_tvalid  (ssIPRX_ARS0_Data_tvalid),
    .s_axis_tready  (ssIPRX_ARS0_Data_tready),
    //-- To ARP / Data -----------------
    .m_axis_tdata   (ssARS0_ARP_Data_tdata),
    .m_axis_tkeep   (ssARS0_ARP_Data_tkeep),
    .m_axis_tlast   (ssARS0_ARP_Data_tlast),
    .m_axis_tvalid  (ssARS0_ARP_Data_tvalid),
    .m_axis_tready  (ssARS0_ARP_Data_tready)
  );

  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPRX ==>[ARS1]==> ICMP)
  //============================================================================
  AxisRegisterSlice_64 ARS1 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPRX / Data --------------
    .s_axis_tdata   (ssIPRX_ARS1_Data_tdata),
    .s_axis_tkeep   (ssIPRX_ARS1_Data_tkeep),
    .s_axis_tlast   (ssIPRX_ARS1_Data_tlast),
    .s_axis_tvalid  (ssIPRX_ARS1_Data_tvalid),
    .s_axis_tready  (ssIPRX_ARS1_Data_tready),
    //-- To ICMP / Data ----------------
    .m_axis_tdata   (ssARS1_ICMP_Data_tdata),
    .m_axis_tkeep   (ssARS1_ICMP_Data_tkeep),
    .m_axis_tlast   (ssARS1_ICMP_Data_tlast),
    .m_axis_tvalid  (ssARS1_ICMP_Data_tvalid),
    .m_axis_tready  (ssARS1_ICMP_Data_tready)
  );
  
  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPRX ==>[ARS2]==> TOE)
  //============================================================================
  AxisRegisterSlice_64 ARS2 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPRX / Data --------------
    .s_axis_tdata   (ssIPRX_ARS2_Data_tdata),
    .s_axis_tkeep   (ssIPRX_ARS2_Data_tkeep),
    .s_axis_tlast   (ssIPRX_ARS2_Data_tlast),
    .s_axis_tvalid  (ssIPRX_ARS2_Data_tvalid),
    .s_axis_tready  (ssIPRX_ARS2_Data_tready),
    //-- To   TOE / Data ---------------
    .m_axis_tdata   (ssARS2_TOE_Data_tdata),
    .m_axis_tkeep   (ssARS2_TOE_Data_tkeep),
    .m_axis_tlast   (ssARS2_TOE_Data_tlast),
    .m_axis_tvalid  (ssARS2_TOE_Data_tvalid),
    .m_axis_tready  (ssARS2_TOE_Data_tready)
  ); 
  
  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPRX ==>[ARS3]==> UOE)
  //============================================================================
  AxisRegisterSlice_64 ARS3 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPRX / Data --------------
    .s_axis_tdata   (ssIPRX_ARS3_Data_tdata),
    .s_axis_tkeep   (ssIPRX_ARS3_Data_tkeep),
    .s_axis_tlast   (ssIPRX_ARS3_Data_tlast),
    .s_axis_tvalid  (ssIPRX_ARS3_Data_tvalid),
    .s_axis_tready  (ssIPRX_ARS3_Data_tready),
    //-- To   TOE / Data ---------------
    .m_axis_tdata   (ssARS3_UOE_Data_tdata),
    .m_axis_tkeep   (ssARS3_UOE_Data_tkeep),
    .m_axis_tlast   (ssARS3_UOE_Data_tlast),
    .m_axis_tvalid  (ssARS3_UOE_Data_tvalid),
    .m_axis_tready  (ssARS3_UOE_Data_tready)
  ); 
  
  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPRX ==>[ARS5]==> ICMP)
  //============================================================================
  AxisRegisterSlice_64 ARS5 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPRX / Data --------------
    .s_axis_tdata   (ssIPRX_ARS5_Derr_tdata),
    .s_axis_tkeep   (ssIPRX_ARS5_Derr_tkeep),
    .s_axis_tlast   (ssIPRX_ARS5_Derr_tlast),
    .s_axis_tvalid  (ssIPRX_ARS5_Derr_tvalid),
    .s_axis_tready  (ssIPRX_ARS5_Derr_tready),
    //-- To   TOE / Data ---------------
    .m_axis_tdata   (ssARS5_ICMP_Derr_tdata),
    .m_axis_tkeep   (ssARS5_ICMP_Derr_tkeep),
    .m_axis_tlast   (ssARS5_ICMP_Derr_tlast),
    .m_axis_tvalid  (ssARS5_ICMP_Derr_tvalid),
    .m_axis_tready  (ssARS5_ICMP_Derr_tready)
  ); 
  
  //============================================================================
  //  INST: ADDRESS-RESOLUTION-PROCESS 
  //============================================================================
  `ifdef USE_DEPRECATED_DIRECTIVES
  AddressResolutionProcess #(32, 48, 1) ARP (
  `else
  AddressResolutionProcess #(32, 48, 0) ARP (
  `endif
    .piShlClk                       (piShlClk),
    .piMMIO_Rst                     (piMMIO_Layer3Rst), // Warning: This reset is active HIGH !!
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_MacAddress              (piMMIO_MacAddress),
    .piMMIO_Ip4Address              (piMMIO_Ip4Address),
    //------------------------------------------------------
    //-- IPRX Interfaces (via ARS0)
    //------------------------------------------------------
    //-- IPRX ==> [ARS0] ==> ARP / Data
    .siIPRX_Data_tdata              (ssARS0_ARP_Data_tdata),
    .siIPRX_Data_tkeep              (ssARS0_ARP_Data_tkeep),
    .siIPRX_Data_tlast              (ssARS0_ARP_Data_tlast),
    .siIPRX_Data_tvalid             (ssARS0_ARP_Data_tvalid),
    .siIPRX_Data_tready             (ssARS0_ARP_Data_tready),
    //------------------------------------------------------
    //-- ETH Interface (via L2MUX)
    //------------------------------------------------------
    //-- ARP ==> [L2MUX] ==> ETH / Data  
    .soETH_Data_tdata               (ssARP_L2MUX_Data_tdata),
    .soETH_Data_tkeep               (ssARP_L2MUX_Data_tkeep),
    .soETH_Data_tlast               (ssARP_L2MUX_Data_tlast),
    .soETH_Data_tvalid              (ssARP_L2MUX_Data_tvalid),
    .soETH_Data_tready              (ssARP_L2MUX_Data_tready),
    //------------------------------------------------------
    //-- IPTX Interfaces
    //------------------------------------------------------
    .siIPTX_MacLkpReq_TDATA         (ssARS8_ARP_MacLkpReq_tdata),
    .siIPTX_MacLkpReq_TVALID        (ssARS8_ARP_MacLkpReq_tvalid),
    .siIPTX_MacLkpReq_TREADY        (ssARS8_ARP_MacLkpReq_tready),
    //--
    .soIPTX_MacLkpRep_TDATA         (ssARP_ARS7_MacLkpRep_tdata),
    .soIPTX_MacLkpRep_TVALID        (ssARP_ARS7_MacLkpRep_tvalid),
    .soIPTX_MacLkpRep_TREADY        (ssARP_ARS7_MacLkpRep_tready)
  ); // End of ARP
  
  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (ARP ==>[ARS7]==> IPTX)
  //============================================================================
  AxisRegisterSlice_56 ARS7 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From ARP / MacLkpRep ----------
    .s_axis_tdata   (ssARP_ARS7_MacLkpRep_tdata),
    .s_axis_tvalid  (ssARP_ARS7_MacLkpRep_tvalid),
    .s_axis_tready  (ssARP_ARS7_MacLkpRep_tready),
    //-- To   ARP / MacLkpReq ----------
    .m_axis_tdata   (ssARS7_IPTX_MacLkpRep_tdata),
    .m_axis_tvalid  (ssARS7_IPTX_MacLkpRep_tvalid),
    .m_axis_tready  (ssARS7_IPTX_MacLkpRep_tready)
  );
  
  //============================================================================
  //  INST: TCP-OFFLOAD-ENGINE
  //============================================================================
  `ifdef USE_DEPRECATED_DIRECTIVES
  TcpOffloadEngine TOE (  
    .aclk                      (piShlClk),
    .aresetn                   (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_IpAddr_V           (piMMIO_Ip4Address),
    .poNTS_Ready_V             (),     // [FIXME-ssTOE_RLB_Ready_tdata]
    //------------------------------------------------------
    //-- IPRX / IP Rx Data Interface
    //------------------------------------------------------
    .siIPRX_Data_TDATA         (ssARS2_TOE_Data_tdata),
    .siIPRX_Data_TKEEP         (ssARS2_TOE_Data_tkeep),
    .siIPRX_Data_TLAST         (ssARS2_TOE_Data_tlast),
    .siIPRX_Data_TVALID        (ssARS2_TOE_Data_tvalid),
    .siIPRX_Data_TREADY        (ssARS2_TOE_Data_tready),
    //------------------------------------------------------
    //-- IPTX / IP Tx Data Interface (via ARS4/L3MUX)
    //------------------------------------------------------
    .soIPTX_Data_TDATA         (ssTOE_ARS4_Data_tdata),
    .soIPTX_Data_TKEEP         (ssTOE_ARS4_Data_tkeep),
    .soIPTX_Data_TLAST         (ssTOE_ARS4_Data_tlast),
    .soIPTX_Data_TVALID        (ssTOE_ARS4_Data_tvalid),
    .soIPTX_Data_TREADY        (ssTOE_ARS4_Data_tready),
    //------------------------------------------------------
    //-- TAIF / APP Rx Data Interfaces
    //------------------------------------------------------
    //-- To   APP / Data Notification
    .soTAIF_Notif_TDATA        (soAPP_Tcp_Notif_tdata),
    .soTAIF_Notif_TVALID       (soAPP_Tcp_Notif_tvalid),  
    .soTAIF_Notif_TREADY       (soAPP_Tcp_Notif_tready),
    //-- From APP / Data Read Request
    .siTAIF_DReq_TDATA         (siAPP_Tcp_DReq_tdata),
    .siTAIF_DReq_TVALID        (siAPP_Tcp_DReq_tvalid),
    .siTAIF_DReq_TREADY        (siAPP_Tcp_DReq_tready),
    //-- To   APP / Data Stream
    .soTAIF_Data_TDATA         (soAPP_Tcp_Data_tdata),
    .soTAIF_Data_TKEEP         (soAPP_Tcp_Data_tkeep),
    .soTAIF_Data_TLAST         (soAPP_Tcp_Data_tlast),
    .soTAIF_Data_TVALID        (soAPP_Tcp_Data_tvalid),
    .soTAIF_Data_TREADY        (soAPP_Tcp_Data_tready),
    //-- To   APP / Metadata
    .soTAIF_Meta_TDATA         (soAPP_Tcp_Meta_tdata),
    .soTAIF_Meta_TVALID        (soAPP_Tcp_Meta_tvalid),
    .soTAIF_Meta_TREADY        (soAPP_Tcp_Meta_tready),
    //------------------------------------------------------
    //-- TAIF / APP Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- From APP / Listen Port Request
    .siTAIF_LsnReq_TDATA       (siAPP_Tcp_LsnReq_tdata),
    .siTAIF_LsnReq_TVALID      (siAPP_Tcp_LsnReq_tvalid),
    .siTAIF_LsnReq_TREADY      (siAPP_Tcp_LsnReq_tready),
    //-- To   APP / Listen Port Reply
    .soTAIF_LsnRep_TDATA       (soAPP_Tcp_LsnRep_tdata),
    .soTAIF_LsnRep_TVALID      (soAPP_Tcp_LsnRep_tvalid),
    .soTAIF_LsnRep_TREADY      (soAPP_Tcp_LsnRep_tready),
    //------------------------------------------------------
    //-- TAIF / APP Tx Data Flow Interfaces
    //------------------------------------------------------
    //-- From APP / Data Stream
    .siTAIF_Data_TDATA         (siAPP_Tcp_Data_tdata),
    .siTAIF_Data_TKEEP         (siAPP_Tcp_Data_tkeep),
    .siTAIF_Data_TLAST         (siAPP_Tcp_Data_tlast),
    .siTAIF_Data_TVALID        (siAPP_Tcp_Data_tvalid),
    .siTAIF_Data_TREADY        (siAPP_Tcp_Data_tready),
    //-- From APP / Send Request
    .siTAIF_SndReq_TDATA       (siAPP_Tcp_SndReq_tdata),
    .siTAIF_SndReq_TVALID      (siAPP_Tcp_SndReq_tvalid),
    .siTAIF_SndReq_TREADY      (siAPP_Tcp_SndReq_tready),
    //-- To  APP / Send Reply
    .soTAIF_SndRep_TDATA       (soAPP_Tcp_SndRep_tdata),
    .soTAIF_SndRep_TVALID      (soAPP_Tcp_SndRep_tvalid),
    .soTAIF_SndRep_TREADY      (soAPP_Tcp_SndRep_tready),
    //------------------------------------------------------
    //-- TAIF / APP Tx Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From APP / Open Session Request
    .siTAIF_OpnReq_TDATA       (siAPP_Tcp_OpnReq_tdata),
    .siTAIF_OpnReq_TVALID      (siAPP_Tcp_OpnReq_tvalid),
    .siTAIF_OpnReq_TREADY      (siAPP_Tcp_OpnReq_tready),
    //-- To   APP / Open Session Reply
    .soTAIF_OpnRep_TDATA       (soAPP_Tcp_OpnRep_tdata),
    .soTAIF_OpnRep_TVALID      (soAPP_Tcp_OpnRep_tvalid),
    .soTAIF_OpnRep_TREADY      (soAPP_Tcp_OpnRep_tready),
    //-- From APP / Close Session Request
    .siTAIF_ClsReq_TDATA       (siAPP_Tcp_ClsReq_tdata),
    .siTAIF_ClsReq_TVALID      (siAPP_Tcp_ClsReq_tvalid),
    .siTAIF_ClsReq_TREADY      (siAPP_Tcp_ClsReq_tready),
    //-- To   APP / Close Session Status
    // [FIXME-TODO]
    //------------------------------------------------------
    //-- MEM / RxP Interface
    //------------------------------------------------------
    //-- Receive Path / S2MM-AXIS --------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used
    //---- Stream Read Command -----------------
    .soMEM_RxP_RdCmd_TDATA     (soMEM_RxP_RdCmd_tdata),
    .soMEM_RxP_RdCmd_TVALID    (soMEM_RxP_RdCmd_tvalid),
    .soMEM_RxP_RdCmd_TREADY    (soMEM_RxP_RdCmd_tready),
    //---- Stream Data Input Channel -----------
    .siMEM_RxP_Data_TDATA      (siMEM_RxP_Data_tdata),
    .siMEM_RxP_Data_TKEEP      (siMEM_RxP_Data_tkeep),
    .siMEM_RxP_Data_TLAST      (siMEM_RxP_Data_tlast),
    .siMEM_RxP_Data_TVALID     (siMEM_RxP_Data_tvalid),  
    .siMEM_RxP_Data_TREADY     (siMEM_RxP_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_RxP_WrSts_TDATA     (siMEM_RxP_WrSts_tdata),
    .siMEM_RxP_WrSts_TVALID    (siMEM_RxP_WrSts_tvalid), 
    .siMEM_RxP_WrSts_TREADY    (siMEM_RxP_WrSts_tready),
    //---- Stream Write Command ----------------
    .soMEM_RxP_WrCmd_TDATA     (soMEM_RxP_WrCmd_tdata),
    .soMEM_RxP_WrCmd_TVALID    (soMEM_RxP_WrCmd_tvalid),
    .soMEM_RxP_WrCmd_TREADY    (soMEM_RxP_WrCmd_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_RxP_Data_TDATA      (soMEM_RxP_Data_tdata),
    .soMEM_RxP_Data_TKEEP      (soMEM_RxP_Data_tkeep),
    .soMEM_RxP_Data_TLAST      (soMEM_RxP_Data_tlast),
    .soMEM_RxP_Data_TVALID     (soMEM_RxP_Data_tvalid),
    .soMEM_RxP_Data_TREADY     (soMEM_RxP_Data_tready),
    //------------------------------------------------------
    //-- MEM / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS -------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used
    //---- Stream Read Command -----------------
    .soMEM_TxP_RdCmd_TDATA     (soMEM_TxP_RdCmd_tdata),
    .soMEM_TxP_RdCmd_TVALID    (soMEM_TxP_RdCmd_tvalid),
    .soMEM_TxP_RdCmd_TREADY    (soMEM_TxP_RdCmd_tready),
    //---- Stream Data Input Channel ----------- 
    .siMEM_TxP_Data_TDATA      (siMEM_TxP_Data_tdata),
    .siMEM_TxP_Data_TKEEP      (siMEM_TxP_Data_tkeep),
    .siMEM_TxP_Data_TLAST      (siMEM_TxP_Data_tlast),
    .siMEM_TxP_Data_TVALID     (siMEM_TxP_Data_tvalid),
    .siMEM_TxP_Data_TREADY     (siMEM_TxP_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_TxP_WrSts_TDATA     (siMEM_TxP_WrSts_tdata),
    .siMEM_TxP_WrSts_TVALID    (siMEM_TxP_WrSts_tvalid),
    .siMEM_TxP_WrSts_TREADY    (siMEM_TxP_WrSts_tready),
    //---- Stream Write Command ----------------
    .soMEM_TxP_WrCmd_TDATA     (soMEM_TxP_WrCmd_tdata),
    .soMEM_TxP_WrCmd_TVALID    (soMEM_TxP_WrCmd_tvalid),
    .soMEM_TxP_WrCmd_TREADY    (soMEM_TxP_WrCmd_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_TxP_Data_TDATA      (soMEM_TxP_Data_tdata),
    .soMEM_TxP_Data_TKEEP      (soMEM_TxP_Data_tkeep),
    .soMEM_TxP_Data_TLAST      (soMEM_TxP_Data_tlast),
    .soMEM_TxP_Data_TVALID     (soMEM_TxP_Data_tvalid),
    .soMEM_TxP_Data_TREADY     (soMEM_TxP_Data_tready),
    //------------------------------------------------------
    //-- CAM / Session Lookup Interfaces
    //------------------------------------------------------
    //-- To   CAM / TCP Session Lookup Request
    .soCAM_SssLkpReq_TDATA     (ssTOE_CAM_LkpReq_tdata),
    .soCAM_SssLkpReq_TVALID    (ssTOE_CAM_LkpReq_tvalid),
    .soCAM_SssLkpReq_TREADY    (ssTOE_CAM_LkpReq_tready),
    //-- From CAM / TCP Session Lookup Reply
    .siCAM_SssLkpRep_TDATA     (ssCAM_TOE_LkpRep_tdata),
    .siCAM_SssLkpRep_TVALID    (ssCAM_TOE_LkpRep_tvalid),
    .siCAM_SssLkpRep_TREADY    (ssCAM_TOE_LkpRep_tready),
    //------------------------------------------------------
    //-- CAM / Session Update Interfaces
    //------------------------------------------------------
    //-- To    CAM / TCP Session Update Request
    .soCAM_SssUpdReq_TDATA     (ssTOE_CAM_UpdReq_tdata),
    .soCAM_SssUpdReq_TVALID    (ssTOE_CAM_UpdReq_tvalid),
    .soCAM_SssUpdReq_TREADY    (ssTOE_CAM_UpdReq_tready),
    //-- From CAM / TCP Session Update Reply
    .siCAM_SssUpdRep_TDATA     (ssCAM_TOE_UpdRpl_tdata),
    .siCAM_SssUpdRep_TVALID    (ssCAM_TOE_UpdRpl_tvalid),
    .siCAM_SssUpdRep_TREADY    (ssCAM_TOE_UpdRpl_tready),
    //------------------------------------------------------
    //-- DEBUG / Not Used
    //------------------------------------------------------
    .poDBG_SssRelCnt_V         (),
    .poDBG_SssRegCnt_V         ()
    // .poSimCycCount_V        ()
  );  // End of TOE
  `else
    TcpOffloadEngine TOE (
    .ap_clk                    (piShlClk),
    .ap_rst_n                  (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_IpAddr_V           (piMMIO_Ip4Address),
    //------------------------------------------------------
    //-- Ready Logic Interface
    //------------------------------------------------------    
    .poNTS_Ready_V             (),     // [FIXME-ssTOE_RLB_Ready_tdata]
    //------------------------------------------------------
    //-- IPRX / IP Rx Data Interface (via ARS2)
    //------------------------------------------------------
    .siIPRX_Data_TDATA         (ssARS2_TOE_Data_tdata),
    .siIPRX_Data_TKEEP         (ssARS2_TOE_Data_tkeep),
    .siIPRX_Data_TLAST         (ssARS2_TOE_Data_tlast),
    .siIPRX_Data_TVALID        (ssARS2_TOE_Data_tvalid),
    .siIPRX_Data_TREADY        (ssARS2_TOE_Data_tready),
    //------------------------------------------------------
    //-- IPTX / IP Tx Data Interface (via ARS4/L3MUX)
    //------------------------------------------------------
    .soIPTX_Data_TDATA         (ssTOE_ARS4_Data_tdata),
    .soIPTX_Data_TKEEP         (ssTOE_ARS4_Data_tkeep),
    .soIPTX_Data_TLAST         (ssTOE_ARS4_Data_tlast),
    .soIPTX_Data_TVALID        (ssTOE_ARS4_Data_tvalid),
    .soIPTX_Data_TREADY        (ssTOE_ARS4_Data_tready),
    //------------------------------------------------------
    //-- TAIF / APP Rx Data Interfaces
    //------------------------------------------------------
    //-- To   APP / Data Notification
    .soTAIF_Notif_V_TDATA      (soAPP_Tcp_Notif_tdata),
    .soTAIF_Notif_V_TVALID     (soAPP_Tcp_Notif_tvalid),  
    .soTAIF_Notif_V_TREADY     (soAPP_Tcp_Notif_tready),
    //-- From APP / Data Read Request
    .siTAIF_DReq_V_TDATA       (siAPP_Tcp_DReq_tdata),
    .siTAIF_DReq_V_TVALID      (siAPP_Tcp_DReq_tvalid),
    .siTAIF_DReq_V_TREADY      (siAPP_Tcp_DReq_tready),
    //-- To   APP / Data Stream
    .soTAIF_Data_TDATA         (soAPP_Tcp_Data_tdata),
    .soTAIF_Data_TKEEP         (soAPP_Tcp_Data_tkeep),
    .soTAIF_Data_TLAST         (soAPP_Tcp_Data_tlast),
    .soTAIF_Data_TVALID        (soAPP_Tcp_Data_tvalid),
    .soTAIF_Data_TREADY        (soAPP_Tcp_Data_tready),
    //-- To   APP / Metadata
    .soTAIF_Meta_V_V_TDATA     (soAPP_Tcp_Meta_tdata),
    .soTAIF_Meta_V_V_TVALID    (soAPP_Tcp_Meta_tvalid),
    .soTAIF_Meta_V_V_TREADY    (soAPP_Tcp_Meta_tready),
    //------------------------------------------------------
    //-- TAIF / APP Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- From APP / Listen Port Request
    .siTAIF_LsnReq_V_V_TDATA   (siAPP_Tcp_LsnReq_tdata),
    .siTAIF_LsnReq_V_V_TVALID  (siAPP_Tcp_LsnReq_tvalid),
    .siTAIF_LsnReq_V_V_TREADY  (siAPP_Tcp_LsnReq_tready),
    //-- To   APP / Listen Port Reply
    .soTAIF_LsnRep_V_TDATA     (soAPP_Tcp_LsnRep_tdata),
    .soTAIF_LsnRep_V_TVALID    (soAPP_Tcp_LsnRep_tvalid),
    .soTAIF_LsnRep_V_TREADY    (soAPP_Tcp_LsnRep_tready),
    //------------------------------------------------------
    //-- TAIF / APP Tx Data Flow Interfaces
    //------------------------------------------------------
    //-- From APP / Data Stream
    .siTAIF_Data_TDATA         (siAPP_Tcp_Data_tdata),
    .siTAIF_Data_TKEEP         (siAPP_Tcp_Data_tkeep),
    .siTAIF_Data_TLAST         (siAPP_Tcp_Data_tlast),
    .siTAIF_Data_TVALID        (siAPP_Tcp_Data_tvalid),
    .siTAIF_Data_TREADY        (siAPP_Tcp_Data_tready),
    //-- From APP / Send Request
    .siTAIF_SndReq_V_TDATA     (siAPP_Tcp_SndReq_tdata),
    .siTAIF_SndReq_V_TVALID    (siAPP_Tcp_SndReq_tvalid),
    .siTAIF_SndReq_V_TREADY    (siAPP_Tcp_SndReq_tready),
    //-- To  APP / Send Reply
    .soTAIF_SndRep_V_TDATA     (soAPP_Tcp_SndRep_tdata),
    .soTAIF_SndRep_V_TVALID    (soAPP_Tcp_SndRep_tvalid),
    .soTAIF_SndRep_V_TREADY    (soAPP_Tcp_SndRep_tready),
    //------------------------------------------------------
    //-- TAIF / APP Tx Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From APP / Open Session Request
    .siTAIF_OpnReq_V_TDATA     (siAPP_Tcp_OpnReq_tdata),
    .siTAIF_OpnReq_V_TVALID    (siAPP_Tcp_OpnReq_tvalid),
    .siTAIF_OpnReq_V_TREADY    (siAPP_Tcp_OpnReq_tready),
    //-- To   APP / Open Session Reply
    .soTAIF_OpnRep_V_TDATA     (soAPP_Tcp_OpnRep_tdata),
    .soTAIF_OpnRep_V_TVALID    (soAPP_Tcp_OpnRep_tvalid),
    .soTAIF_OpnRep_V_TREADY    (soAPP_Tcp_OpnRep_tready),
    //-- From APP / Close Session Request
    .siTAIF_ClsReq_V_V_TDATA   (siAPP_Tcp_ClsReq_tdata),
    .siTAIF_ClsReq_V_V_TVALID  (siAPP_Tcp_ClsReq_tvalid),
    .siTAIF_ClsReq_V_V_TREADY  (siAPP_Tcp_ClsReq_tready),
    //-- To   APP / Close Session Status
    // [FIXME-TODO]
    //------------------------------------------------------
    //-- MEM / RxP Interface
    //------------------------------------------------------
    //-- Receive Path / S2MM-AXIS --------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used
    //---- Stream Read Command -----------------
    .soMEM_RxP_RdCmd_V_TDATA   (soMEM_RxP_RdCmd_tdata),
    .soMEM_RxP_RdCmd_V_TVALID  (soMEM_RxP_RdCmd_tvalid),
    .soMEM_RxP_RdCmd_V_TREADY  (soMEM_RxP_RdCmd_tready),
    //---- Stream Data Input Channel -----------
    .siMEM_RxP_Data_TDATA      (siMEM_RxP_Data_tdata),
    .siMEM_RxP_Data_TKEEP      (siMEM_RxP_Data_tkeep),
    .siMEM_RxP_Data_TLAST      (siMEM_RxP_Data_tlast),
    .siMEM_RxP_Data_TVALID     (siMEM_RxP_Data_tvalid),  
    .siMEM_RxP_Data_TREADY     (siMEM_RxP_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_RxP_WrSts_V_TDATA   (siMEM_RxP_WrSts_tdata),
    .siMEM_RxP_WrSts_V_TVALID  (siMEM_RxP_WrSts_tvalid), 
    .siMEM_RxP_WrSts_V_TREADY  (siMEM_RxP_WrSts_tready),
    //---- Stream Write Command ----------------
    .soMEM_RxP_WrCmd_V_TDATA   (soMEM_RxP_WrCmd_tdata),
    .soMEM_RxP_WrCmd_V_TVALID  (soMEM_RxP_WrCmd_tvalid),
    .soMEM_RxP_WrCmd_V_TREADY  (soMEM_RxP_WrCmd_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_RxP_Data_TDATA      (soMEM_RxP_Data_tdata),
    .soMEM_RxP_Data_TKEEP      (soMEM_RxP_Data_tkeep),
    .soMEM_RxP_Data_TLAST      (soMEM_RxP_Data_tlast),
    .soMEM_RxP_Data_TVALID     (soMEM_RxP_Data_tvalid),
    .soMEM_RxP_Data_TREADY     (soMEM_RxP_Data_tready),
    //------------------------------------------------------
    //-- MEM / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS -------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used
    //---- Stream Read Command -----------------
    .soMEM_TxP_RdCmd_V_TDATA   (soMEM_TxP_RdCmd_tdata),
    .soMEM_TxP_RdCmd_V_TVALID  (soMEM_TxP_RdCmd_tvalid),
    .soMEM_TxP_RdCmd_V_TREADY  (soMEM_TxP_RdCmd_tready),
    //---- Stream Data Input Channel ----------- 
    .siMEM_TxP_Data_TDATA      (siMEM_TxP_Data_tdata),
    .siMEM_TxP_Data_TKEEP      (siMEM_TxP_Data_tkeep),
    .siMEM_TxP_Data_TLAST      (siMEM_TxP_Data_tlast),
    .siMEM_TxP_Data_TVALID     (siMEM_TxP_Data_tvalid),
    .siMEM_TxP_Data_TREADY     (siMEM_TxP_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_TxP_WrSts_V_TDATA   (siMEM_TxP_WrSts_tdata),
    .siMEM_TxP_WrSts_V_TVALID  (siMEM_TxP_WrSts_tvalid),
    .siMEM_TxP_WrSts_V_TREADY  (siMEM_TxP_WrSts_tready),
    //---- Stream Write Command ----------------
    .soMEM_TxP_WrCmd_V_TDATA   (soMEM_TxP_WrCmd_tdata),
    .soMEM_TxP_WrCmd_V_TVALID  (soMEM_TxP_WrCmd_tvalid),
    .soMEM_TxP_WrCmd_V_TREADY  (soMEM_TxP_WrCmd_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_TxP_Data_TDATA      (soMEM_TxP_Data_tdata),
    .soMEM_TxP_Data_TKEEP      (soMEM_TxP_Data_tkeep),
    .soMEM_TxP_Data_TLAST      (soMEM_TxP_Data_tlast),
    .soMEM_TxP_Data_TVALID     (soMEM_TxP_Data_tvalid),
    .soMEM_TxP_Data_TREADY     (soMEM_TxP_Data_tready),
    //------------------------------------------------------
    //-- CAM / Session Lookup Interfaces
    //------------------------------------------------------
    //-- To   CAM / TCP Session Lookup Request
    .soCAM_SssLkpReq_V_TDATA   (ssTOE_CAM_LkpReq_tdata),
    .soCAM_SssLkpReq_V_TVALID  (ssTOE_CAM_LkpReq_tvalid),
    .soCAM_SssLkpReq_V_TREADY  (ssTOE_CAM_LkpReq_tready),
    //-- From CAM / TCP Session Lookup Reply
    .siCAM_SssLkpRep_V_TDATA   (ssCAM_TOE_LkpRep_tdata),
    .siCAM_SssLkpRep_V_TVALID  (ssCAM_TOE_LkpRep_tvalid),
    .siCAM_SssLkpRep_V_TREADY  (ssCAM_TOE_LkpRep_tready),
    //------------------------------------------------------
    //-- CAM / Session Update Interfaces
    //------------------------------------------------------
    //-- To    CAM / TCP Session Update Request
    .soCAM_SssUpdReq_V_TDATA   (ssTOE_CAM_UpdReq_tdata),
    .soCAM_SssUpdReq_V_TVALID  (ssTOE_CAM_UpdReq_tvalid),
    .soCAM_SssUpdReq_V_TREADY  (ssTOE_CAM_UpdReq_tready),
    //-- From CAM / TCP Session Update Reply
    .siCAM_SssUpdRep_V_TDATA   (ssCAM_TOE_UpdRpl_tdata),
    .siCAM_SssUpdRep_V_TVALID  (ssCAM_TOE_UpdRpl_tvalid),
    .siCAM_SssUpdRep_V_TREADY  (ssCAM_TOE_UpdRpl_tready),
    //------------------------------------------------------
    //-- DEBUG / Not Used
    //------------------------------------------------------
    .poDBG_SssRelCnt_V         (),
    .poDBG_SssRegCnt_V         ()
    // NOT-USED .poSimCycCount_V ()
  );  // End of TOE 
  `endif  // End of HLS_VERSION
  
  //============================================================================
  //  INST: CONTENT-ADDRESSABLE-MEMORY
  //============================================================================  
`define USE_FAKE_CAM

`ifndef USE_FAKE_CAM
  ToeCam RTLCAM (
   .piClk                        (piShlClk),
   .piRst_n                      (~piMMIO_Layer4Rst),
   //--
   .poCamReady                   (poMMIO_CamReady),
   //------------------------------------------------------
   //-- TOE Interfaces
   //------------------------------------------------------
   //-- From TOE - Lookup Request ------
   .piTOE_LkpReq_tdata           (ssTOE_CAM_LkpReq_tdata),
   .piTOE_LkpReq_tvalid          (ssTOE_CAM_LkpReq_tvalid),
   .poTOE_LkpReq_tready          (ssTOE_CAM_LkpReq_tready), 
   //-- To   TOE - Lookup Reply --------
   .poTOE_LkpRep_tdata           (ssCAM_TOE_LkpRep_tdata),
   .poTOE_LkpRep_tvalid          (ssCAM_TOE_LkpRep_tvalid),
   .piTOE_LkpRep_tready          (ssCAM_TOE_LkpRep_tready),
   //-- From TOE - Update Request ------
   .piTOE_UpdReq_tdata           (ssTOE_CAM_UpdReq_tdata),
   .piTOE_UpdReq_tvalid          (ssTOE_CAM_UpdReq_tvalid),
   .poTOE_UpdReq_tready          (ssTOE_CAM_UpdReq_tready),
   //-- To   TOE - Update Reply --------
   .poTOE_UpdRep_tdata           (sCAM_TOE_UpdRpl_tdata),
   .poTOE_UpdRep_tvalid          (sCAM_TOE_UpdRpl_tvalid),
   .piTOE_UpdRep_tready          (sCAM_TOE_UpdRpl_tready),
   //------------------------------------------------------
   //-- LED & Debug Interfaces
   //------------------------------------------------------
   .poLed0                       (),
   .poLed1                       (),
   .poDebug                      ()
  );
`else
  `ifdef USE_DEPRECATED_DIRECTIVES
  ContentAddressableMemory HLSCAM (
    .aclk                         (piShlClk),
    .aresetn                      (~piMMIO_Layer4Rst),
    //-- 
    .poMMIO_CamReady_V            (poMMIO_CamReady),
    //------------------------------------------------------
    //-- TOE Interfaces                                        
    //------------------------------------------------------
    //-- From TOE - Lookup Request -----
    .siTOE_SssLkpReq_TDATA        (ssTOE_CAM_LkpReq_tdata),
    .siTOE_SssLkpReq_TVALID       (ssTOE_CAM_LkpReq_tvalid),
    .siTOE_SssLkpReq_TREADY       (ssTOE_CAM_LkpReq_tready),
    //-- To   TOE - Lookup Reply -------
    .soTOE_SssLkpRep_TDATA        (ssCAM_TOE_LkpRep_tdata),
    .soTOE_SssLkpRep_TVALID       (ssCAM_TOE_LkpRep_tvalid),
    .soTOE_SssLkpRep_TREADY       (ssCAM_TOE_LkpRep_tready),
    //-- From TOE - Update Request -----
    .siTOE_SssUpdReq_TDATA        (ssTOE_CAM_UpdReq_tdata),
    .siTOE_SssUpdReq_TVALID       (ssTOE_CAM_UpdReq_tvalid),
    .siTOE_SssUpdReq_TREADY       (ssTOE_CAM_UpdReq_tready),
    //-- To   TOE - Update Reply -------
    .soTOE_SssUpdRep_TDATA        (ssCAM_TOE_UpdRpl_tdata),
    .soTOE_SssUpdRep_TVALID       (ssCAM_TOE_UpdRpl_tvalid),
    .soTOE_SssUpdRep_TREADY       (ssCAM_TOE_UpdRpl_tready)
  );
  `else
  ContentAddressableMemory HLSCAM (
    .ap_clk                       (piShlClk),
    .ap_rst_n                     (~piMMIO_Layer4Rst),
    //-- 
    .poMMIO_CamReady_V            (poMMIO_CamReady),
    //------------------------------------------------------
    //-- TOE Interfaces                                        
    //------------------------------------------------------
    //-- From TOE - Lookup Request -----
    .siTOE_SssLkpReq_V_TDATA      (ssTOE_CAM_LkpReq_tdata),
    .siTOE_SssLkpReq_V_TVALID     (ssTOE_CAM_LkpReq_tvalid),
    .siTOE_SssLkpReq_V_TREADY     (ssTOE_CAM_LkpReq_tready),
    //-- To   TOE - Lookup Reply -------
    .soTOE_SssLkpRep_V_TDATA      (ssCAM_TOE_LkpRep_tdata),
    .soTOE_SssLkpRep_V_TVALID     (ssCAM_TOE_LkpRep_tvalid),
    .soTOE_SssLkpRep_V_TREADY     (ssCAM_TOE_LkpRep_tready),
    //-- From TOE - Update Request -----
    .siTOE_SssUpdReq_V_TDATA      (ssTOE_CAM_UpdReq_tdata),
    .siTOE_SssUpdReq_V_TVALID     (ssTOE_CAM_UpdReq_tvalid),
    .siTOE_SssUpdReq_V_TREADY     (ssTOE_CAM_UpdReq_tready),
    //-- To   TOE - Update Reply -------
    .soTOE_SssUpdRep_V_TDATA      (ssCAM_TOE_UpdRpl_tdata),
    .soTOE_SssUpdRep_V_TVALID     (ssCAM_TOE_UpdRpl_tvalid),
    .soTOE_SssUpdRep_V_TREADY     (ssCAM_TOE_UpdRpl_tready)
  );       
  `endif  // USE_DEPRECATED_DIRECTIVES
`endif  // USE_FAKE_CAM
    
  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (TOE ==>[ARS4]==> IPTX)
  //============================================================================
  AxisRegisterSlice_64 ARS4 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer4Rst),
    //-- From TOE / Data ---------------
    .s_axis_tdata   (ssTOE_ARS4_Data_tdata),
    .s_axis_tkeep   (ssTOE_ARS4_Data_tkeep),
    .s_axis_tlast   (ssTOE_ARS4_Data_tlast),
    .s_axis_tvalid  (ssTOE_ARS4_Data_tvalid),
    .s_axis_tready  (ssTOE_ARS4_Data_tready),
    //-- To   L3MUX / Data -------------
    .m_axis_tdata   (ssARS4_L3MUX_Data_tdata),
    .m_axis_tkeep   (ssARS4_L3MUX_Data_tkeep),
    .m_axis_tlast   (ssARS4_L3MUX_Data_tlast),
    .m_axis_tvalid  (ssARS4_L3MUX_Data_tvalid),
    .m_axis_tready  (ssARS4_L3MUX_Data_tready)
  ); 

  //============================================================================
  //  INST: UDP-OFFLOAD-ENGINE
  //============================================================================
  `ifdef USE_DEPRECATED_DIRECTIVES
   UdpOffloadEngine UOE ( 
    .aclk                       (piShlClk),
    .aresetn                    (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    .piMMIO_En_V                (piMMIO_Layer4En),
    //--
    .soMMIO_Ready_TDATA         (ssUOE_ARS6_Ready_tdata),
    .soMMIO_Ready_TVALID        (ssUOE_ARS6_Ready_tvalid),
    .soMMIO_Ready_TREADY        (ssUOE_ARS6_Ready_tready),
    //------------------------------------------------------
    //-- IPRX Data Interface (via ARS3)
    //------------------------------------------------------
    .siIPRX_Data_TDATA          (ssARS3_UOE_Data_tdata),
    .siIPRX_Data_TKEEP          (ssARS3_UOE_Data_tkeep),
    .siIPRX_Data_TLAST          (ssARS3_UOE_Data_tlast),
    .siIPRX_Data_TVALID         (ssARS3_UOE_Data_tvalid),
    .siIPRX_Data_TREADY         (ssARS3_UOE_Data_tready),
    //------------------------------------------------------
    //-- IPTX Data Interface (via L3MUX)
    //------------------------------------------------------
    .soIPTX_Data_TDATA          (ssUOE_L3MUX_Data_tdata),
    .soIPTX_Data_TKEEP          (ssUOE_L3MUX_Data_tkeep),
    .soIPTX_Data_TLAST          (ssUOE_L3MUX_Data_tlast),
    .soIPTX_Data_TVALID         (ssUOE_L3MUX_Data_tvalid),
    .soIPTX_Data_TREADY         (ssUOE_L3MUX_Data_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Ctrl Port Interfaces
    //------------------------------------------------------
    //---- Listen Request
    .siUAIF_LsnReq_TDATA        (siAPP_Udp_LsnReq_tdata) ,
    .siUAIF_LsnReq_TVALID       (siAPP_Udp_LsnReq_tvalid),
    .siUAIF_LsnReq_TREADY       (siAPP_Udp_LsnReq_tready),
    //---- Listen Reply
    .soUAIF_LsnRep_TDATA        (soAPP_Udp_LsnRep_tdata) ,
    .soUAIF_LsnRep_TVALID       (soAPP_Udp_LsnRep_tvalid),
    .soUAIF_LsnRep_TREADY       (soAPP_Udp_LsnRep_tready),
    //---- Close Request
    .siUAIF_ClsReq_tdata        (siAPP_Udp_ClsReq_tdata) ,
    .siUAIF_ClsReq_tvalid       (siAPP_Udp_ClsReq_tvalid),
    .siUAIF_ClsReq_tready       (siAPP_Udp_ClsReq_tready),
    //---- Close Reply
    .soUAIF_ClsRep_TDATA        (soAPP_Udp_ClsRep_tdata) ,
    .soUAIF_ClsRep_TVALID       (soAPP_Udp_ClsRep_tvalid),
    .soUAIF_ClsRep_TREADY       (soAPP_Udp_ClsRep_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Rx Data Interfaces (.i.e UOE->APP)
    //------------------------------------------------------
    //---- UDP Data
    .soUAIF_Data_TDATA          (soAPP_Udp_Data_tdata), 
    .soUAIF_Data_TKEEP          (soAPP_Udp_Data_tkeep),
    .soUAIF_Data_TLAST          (soAPP_Udp_Data_tlast),
    .soUAIF_Data_TVALID         (soAPP_Udp_Data_tvalid),
    .soUAIF_Data_TREADY         (soAPP_Udp_Data_tready),
    //---- UDP Metadata
    .soUAIF_Meta_TDATA          (soAPP_Udp_Meta_tdata),
    .soUAIF_Meta_TVALID         (soAPP_Udp_Meta_tvalid),
    .soUAIF_Meta_TREADY         (soAPP_Udp_Meta_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Tx Data Interfaces (.i.e APP->UOE)
    //------------------------------------------------------ 
    //---- UDP Data
    .siUAIF_Data_TDATA          (siAPP_Udp_Data_tdata),
    .siUAIF_Data_TKEEP          (siAPP_Udp_Data_tkeep),
    .siUAIF_Data_TLAST          (siAPP_Udp_Data_tlast),
    .siUAIF_Data_TVALID         (siAPP_Udp_Data_tvalid),
    .siUAIF_Data_TREADY         (siAPP_Udp_Data_tready),
    //---- UDP Metadata
    .siUAIF_Meta_TDATA          (siAPP_Udp_Meta_tdata),
    .siUAIF_Meta_TVALID         (siAPP_Udp_Meta_tvalid),
    .siUAIF_Meta_TREADY         (siAPP_Udp_Meta_tready),
    //---- UDP Data length
    .siUAIF_DLen_TDATA          (siAPP_Udp_DLen_tdata),
    .siUAIF_DLen_TVALID         (siAPP_Udp_DLen_tvalid),
    .siUAIF_DLen_TREADY         (siAPP_Udp_DLen_tready),
    //------------------------------------------------------
    //-- ICMP / Message Data Interface (Port Unreachable)
    //------------------------------------------------------
    .soICMP_Data_TDATA           (ssUOE_ICMP_Data_tdata),
    .soICMP_Data_TKEEP           (ssUOE_ICMP_Data_tkeep),
    .soICMP_Data_TLAST           (ssUOE_ICMP_Data_tlast),
    .soICMP_Data_TVALID          (ssUOE_ICMP_Data_tvalid),
    .soICMP_Data_TREADY          (ssUOE_ICMP_Data_tready)
   );
  `else
   UdpOffloadEngine UOE ( 
    .ap_clk                     (piShlClk),
    .ap_rst_n                   (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    .piMMIO_En_V                (piMMIO_Layer4En),
    //--
    .soMMIO_Ready_V_TDATA       (ssUOE_ARS6_Ready_tdata),
    .soMMIO_Ready_V_TVALID      (ssUOE_ARS6_Ready_tvalid),
    .soMMIO_Ready_V_TREADY      (ssUOE_ARS6_Ready_tready),
    //------------------------------------------------------
    //-- IPRX Data Interface (via ARS3)
    //------------------------------------------------------
    .siIPRX_Data_TDATA          (ssARS3_UOE_Data_tdata),
    .siIPRX_Data_TKEEP          (ssARS3_UOE_Data_tkeep),
    .siIPRX_Data_TLAST          (ssARS3_UOE_Data_tlast),
    .siIPRX_Data_TVALID         (ssARS3_UOE_Data_tvalid),
    .siIPRX_Data_TREADY         (ssARS3_UOE_Data_tready),
    //------------------------------------------------------
    //-- IPTX Data Interface (via L3MUX)
    //------------------------------------------------------
    .soIPTX_Data_TDATA          (ssUOE_L3MUX_Data_tdata),
    .soIPTX_Data_TKEEP          (ssUOE_L3MUX_Data_tkeep),
    .soIPTX_Data_TLAST          (ssUOE_L3MUX_Data_tlast),
    .soIPTX_Data_TVALID         (ssUOE_L3MUX_Data_tvalid),
    .soIPTX_Data_TREADY         (ssUOE_L3MUX_Data_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Ctrl Port Interfaces
    //------------------------------------------------------
    //---- Listen Request
    .siUAIF_LsnReq_V_V_TDATA    (siAPP_Udp_LsnReq_tdata) ,
    .siUAIF_LsnReq_V_V_TVALID   (siAPP_Udp_LsnReq_tvalid),
    .siUAIF_LsnReq_V_V_TREADY   (siAPP_Udp_LsnReq_tready),
    //---- Listen Reply
    .soUAIF_LsnRep_V_TDATA      (soAPP_Udp_LsnRep_tdata) ,
    .soUAIF_LsnRep_V_TVALID     (soAPP_Udp_LsnRep_tvalid),
    .soUAIF_LsnRep_V_TREADY     (soAPP_Udp_LsnRep_tready),
    //---- Close Request
    .siUAIF_ClsReq_V_V_tdata    (siAPP_Udp_ClsReq_tdata) ,
    .siUAIF_ClsReq_V_V_tvalid   (siAPP_Udp_ClsReq_tvalid),
    .siUAIF_ClsReq_V_V_tready   (siAPP_Udp_ClsReq_tready),
    //---- Close Reply
    .soUAIF_ClsRep_V_TDATA      (soAPP_Udp_ClsRep_tdata) ,
    .soUAIF_ClsRep_V_TVALID     (soAPP_Udp_ClsRep_tvalid),
    .soUAIF_ClsRep_V_TREADY     (soAPP_Udp_ClsRep_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Rx Data Interfaces (.i.e UOE->APP)
    //------------------------------------------------------
    //---- UDP Data
    .soUAIF_Data_TDATA          (soAPP_Udp_Data_tdata), 
    .soUAIF_Data_TKEEP          (soAPP_Udp_Data_tkeep),
    .soUAIF_Data_TLAST          (soAPP_Udp_Data_tlast),
    .soUAIF_Data_TVALID         (soAPP_Udp_Data_tvalid),
    .soUAIF_Data_TREADY         (soAPP_Udp_Data_tready),
    //---- UDP Metadata
    .soUAIF_Meta_TDATA          (soAPP_Udp_Meta_tdata),
    .soUAIF_Meta_TVALID         (soAPP_Udp_Meta_tvalid),
    .soUAIF_Meta_TREADY         (soAPP_Udp_Meta_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Tx Data Interfaces (.i.e APP->UOE)
    //------------------------------------------------------ 
    //---- UDP Data
    .siUAIF_Data_TDATA          (siAPP_Udp_Data_tdata),
    .siUAIF_Data_TKEEP          (siAPP_Udp_Data_tkeep),
    .siUAIF_Data_TLAST          (siAPP_Udp_Data_tlast),
    .siUAIF_Data_TVALID         (siAPP_Udp_Data_tvalid),
    .siUAIF_Data_TREADY         (siAPP_Udp_Data_tready),
    //---- UDP Metadata
    .siUAIF_Meta_TDATA          (siAPP_Udp_Meta_tdata),
    .siUAIF_Meta_TVALID         (siAPP_Udp_Meta_tvalid),
    .siUAIF_Meta_TREADY         (siAPP_Udp_Meta_tready),
    //---- UDP Data length
    .siUAIF_DLen_V_V_TDATA      (siAPP_Udp_DLen_tdata),
    .siUAIF_DLen_V_V_TVALID     (siAPP_Udp_DLen_tvalid),
    .siUAIF_DLen_V_V_TREADY     (siAPP_Udp_DLen_tready),
    //------------------------------------------------------
    //-- ICMP / Message Data Interface (Port Unreachable)
    //------------------------------------------------------
    .soICMP_Data_TDATA           (ssUOE_ICMP_Data_tdata),
    .soICMP_Data_TKEEP           (ssUOE_ICMP_Data_tkeep),
    .soICMP_Data_TLAST           (ssUOE_ICMP_Data_tlast),
    .soICMP_Data_TVALID          (ssUOE_ICMP_Data_tvalid),
    .soICMP_Data_TREADY          (ssUOE_ICMP_Data_tready)
   ); // End-of: UdpOffloadEngine
  `endif

  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (UOE ==>[ARS6]==> RLB)
  //============================================================================
  AxisRegisterSlice_8 ARS6 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From UOE / Data ---------------
    .s_axis_tdata   (ssUOE_ARS6_Ready_tdata),
    .s_axis_tvalid  (ssUOE_ARS6_Ready_tvalid),
    .s_axis_tready  (ssUOE_ARS6_Ready_tready),
    //-- To   UOE / Data ---------------
    .m_axis_tdata   (ssARS6_RLB_Ready_tdata),
    .m_axis_tvalid  (ssARS6_RLB_Ready_tvalid),
    .m_axis_tready  (ssARS6_RLB_Ready_tready)
  ); 

  //============================================================================
  //  INST: ICMP-SERVER
  //============================================================================
  InternetControlMessageProcess ICMP (                   
    //------------------------------------------------------
    //-- From SHELL Interfaces
    //------------------------------------------------------
    //-- Global Clock & Reset
   `ifdef USE_DEPRECATED_DIRECTIVES
    .aclk                 (piShlClk),
    .aresetn              (~piMMIO_Layer3Rst),
   `else
    .ap_clk               (piShlClk),
    .ap_rst_n             (~piMMIO_Layer3Rst),
   `endif
    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_Ip4Address_V  (piMMIO_Ip4Address),
    //------------------------------------------------------
    //-- IPRX Interfaces
    //------------------------------------------------------
    //-- From IPRX==>[ARS1] / Data -----
    .siIPRX_Data_TDATA    (ssARS1_ICMP_Data_tdata),
    .siIPRX_Data_TKEEP    (ssARS1_ICMP_Data_tkeep),
    .siIPRX_Data_TLAST    (ssARS1_ICMP_Data_tlast),
    .siIPRX_Data_TVALID   (ssARS1_ICMP_Data_tvalid),
    .siIPRX_Data_TREADY   (ssARS1_ICMP_Data_tready),
    //-- To   IPRX / Ttl --------------------
    .siIPRX_Derr_TDATA    (ssARS5_ICMP_Derr_tdata),
    .siIPRX_Derr_TKEEP    (ssARS5_ICMP_Derr_tkeep),
    .siIPRX_Derr_TLAST    (ssARS5_ICMP_Derr_tlast),
    .siIPRX_Derr_TVALID   (ssARS5_ICMP_Derr_tvalid),
    .siIPRX_Derr_TREADY   (ssARS5_ICMP_Derr_tready),
    //------------------------------------------------------
    //-- UOE Interfaces
    //------------------------------------------------------
    //-- From UOE / Data   
    .siUOE_Data_TDATA     (ssUOE_ICMP_Data_tdata),
    .siUOE_Data_TKEEP     (ssUOE_ICMP_Data_tkeep),
    .siUOE_Data_TLAST     (ssUOE_ICMP_Data_tlast),
    .siUOE_Data_TVALID    (ssUOE_ICMP_Data_tvalid),
    .siUOE_Data_TREADY    (ssUOE_ICMP_Data_tready),    
    //------------------------------------------------------
    //-- L3MUX Interfaces
    //------------------------------------------------------
    //-- To   L3MUX / Data -------------
    .soIPTX_Data_TDATA    (ssICMP_L3MUX_Data_tdata),
    .soIPTX_Data_TKEEP    (ssICMP_L3MUX_Data_tkeep),
    .soIPTX_Data_TLAST    (ssICMP_L3MUX_Data_tlast),
    .soIPTX_Data_TVALID   (ssICMP_L3MUX_Data_tvalid),
    .soIPTX_Data_TREADY   (ssICMP_L3MUX_Data_tready)
  ); // End of: ICMP

  //============================================================================
  //  INST: L3MUX-AXI4-STREAM-INTERCONNECT-RTL (Muxes ICMP, TOE, and UOE)
  //============================================================================
  AxisInterconnectRtl_3S1M_D8 L3MUX (   
    .ACLK               (piShlClk),                         
    .ARESETN            (~piMMIO_Layer3Rst),
    //-- 
    .S00_AXIS_ACLK      (piShlClk),
    .S01_AXIS_ACLK      (piShlClk),            
    .S02_AXIS_ACLK      (piShlClk),        
     //-- 
    .S00_AXIS_ARESETN   (~piMMIO_Layer3Rst),
    .S01_AXIS_ARESETN   (~piMMIO_Layer3Rst),
    .S02_AXIS_ARESETN   (~piMMIO_Layer3Rst),
    //------------------------------------------------------
    //-- From ICMP Interfaces
    //------------------------------------------------------
    .S00_AXIS_TDATA     (ssICMP_L3MUX_Data_tdata),
    .S00_AXIS_TKEEP     (ssICMP_L3MUX_Data_tkeep),
    .S00_AXIS_TLAST     (ssICMP_L3MUX_Data_tlast),
    .S00_AXIS_TVALID    (ssICMP_L3MUX_Data_tvalid),
    .S00_AXIS_TREADY    (ssICMP_L3MUX_Data_tready),
    //------------------------------------------------------
    //-- From UDP Interfaces
    //------------------------------------------------------
    .S01_AXIS_TDATA     (ssUOE_L3MUX_Data_tdata), 
    .S01_AXIS_TKEEP     (ssUOE_L3MUX_Data_tkeep),
    .S01_AXIS_TLAST     (ssUOE_L3MUX_Data_tlast),
    .S01_AXIS_TVALID    (ssUOE_L3MUX_Data_tvalid),
    .S01_AXIS_TREADY    (ssUOE_L3MUX_Data_tready),
    //------------------------------------------------------
    //-- From TOE Interfaces (via [ARS4])
    //------------------------------------------------------
    .S02_AXIS_TDATA     (ssARS4_L3MUX_Data_tdata),
    .S02_AXIS_TKEEP     (ssARS4_L3MUX_Data_tkeep),
    .S02_AXIS_TLAST     (ssARS4_L3MUX_Data_tlast),
    .S02_AXIS_TVALID    (ssARS4_L3MUX_Data_tvalid),
    .S02_AXIS_TREADY    (ssARS4_L3MUX_Data_tready),
    //--     
    .M00_AXIS_ACLK      (piShlClk),        
    .M00_AXIS_ARESETN   (~piMMIO_Layer3Rst),    
    //------------------------------------------------------
    //-- To IPTX Interfaces
    //------------------------------------------------------
    .M00_AXIS_TDATA     (ssL3MUX_IPTX_Data_tdata),
    .M00_AXIS_TKEEP     (ssL3MUX_IPTX_Data_tkeep),
    .M00_AXIS_TLAST     (ssL3MUX_IPTX_Data_tlast),
    .M00_AXIS_TVALID    (ssL3MUX_IPTX_Data_tvalid),
    .M00_AXIS_TREADY    (ssL3MUX_IPTX_Data_tready),
    //-- 
    .S00_ARB_REQ_SUPPRESS(1'b0),
    .S01_ARB_REQ_SUPPRESS(1'b0),
    .S02_ARB_REQ_SUPPRESS(1'b0)
  );
  
  //============================================================================
  //  INST: IP-TX-HANDLER
  //============================================================================
  `ifdef USE_DEPRECATED_DIRECTIVES
   IpTxHandler IPTX (
    .aclk                     (piShlClk),
    .aresetn                  (~piMMIO_Layer3Rst),
    //------------------------------------------------------
    //-- L3MUX Interfaces
    //------------------------------------------------------
    //-- From L3MUX / Data -------------
    .siL3MUX_Data_TDATA       (ssL3MUX_IPTX_Data_tdata),
    .siL3MUX_Data_TKEEP       (ssL3MUX_IPTX_Data_tkeep),
    .siL3MUX_Data_TLAST       (ssL3MUX_IPTX_Data_tlast),
    .siL3MUX_Data_TVALID      (ssL3MUX_IPTX_Data_tvalid),
    .siL3MUX_Data_TREADY      (ssL3MUX_IPTX_Data_tready),
    //------------------------------------------------------
    //-- ARP Interfaces
    //------------------------------------------------------
    //-- To   ARP / LookupRequest ------                 
    .soARP_LookupReq_TDATA     (ssIPTX_ARS8_MacLkpReq_tdata),
    .soARP_LookupReq_TVALID    (ssIPTX_ARS8_MacLkpReq_tvalid),
    .soARP_LookupReq_TREADY    (ssIPTX_ARS8_MacLkpReq_tready),
    //-- From ARP / LookupReply --------
    .siARP_LookupRep_TDATA    (ssARS7_IPTX_MacLkpRep_tdata),
    .siARP_LookupRep_TVALID   (ssARS7_IPTX_MacLkpRep_tvalid),
    .siARP_LookupRep_TREADY   (ssARS7_IPTX_MacLkpRep_tready),
    //------------------------------------------------------
    //-- L2MUX Interfaces
    //------------------------------------------------------
    //-- To L2MUX / Data
    .soL2MUX_Data_TDATA       (ssIPTX_L2MUX_Data_tdata),
    .soL2MUX_Data_TKEEP       (ssIPTX_L2MUX_Data_tkeep),
    .soL2MUX_Data_TLAST       (ssIPTX_L2MUX_Data_tlast),
    .soL2MUX_Data_TVALID      (ssIPTX_L2MUX_Data_tvalid),
    .soL2MUX_Data_TREADY      (ssIPTX_L2MUX_Data_tready),
    //-- 
    .piMMIO_SubNetMask_V      (piMMIO_SubNetMask), 
    .piMMIO_GatewayAddr_V     (piMMIO_GatewayAddr),
    .piMMIO_MacAddress_V      (piMMIO_MacAddress)
  ); // End of IPTX
  `else
   IpTxHandler IPTX (
    .ap_clk                   (piShlClk),
    .ap_rst_n                 (~piMMIO_Layer3Rst),
    //------------------------------------------------------
    //-- L3MUX Interfaces
    //------------------------------------------------------
    //-- From L3MUX / Data -------------
    .siL3MUX_Data_TDATA       (ssL3MUX_IPTX_Data_tdata),
    .siL3MUX_Data_TKEEP       (ssL3MUX_IPTX_Data_tkeep),
    .siL3MUX_Data_TLAST       (ssL3MUX_IPTX_Data_tlast),
    .siL3MUX_Data_TVALID      (ssL3MUX_IPTX_Data_tvalid),
    .siL3MUX_Data_TREADY      (ssL3MUX_IPTX_Data_tready),
    //------------------------------------------------------
    //-- ARP Interfaces
    //------------------------------------------------------
    //-- To   ARP / LookupRequest ------                 
    .soARP_LookupReq_V_V_TDATA  (ssIPTX_ARS8_MacLkpReq_tdata),
    .soARP_LookupReq_V_V_TVALID (ssIPTX_ARS8_MacLkpReq_tvalid),
    .soARP_LookupReq_V_V_TREADY (ssIPTX_ARS8_MacLkpReq_tready),
    //-- From ARP / LookupReply --------
    .siARP_LookupRep_V_TDATA  (ssARP_IPTX_MacLkpRep_tdata),
    .siARP_LookupRep_V_TVALID (ssARP_IPTX_MacLkpRep_tvalid),
    .siARP_LookupRep_V_TREADY (ssARP_IPTX_MacLkpRep_tready),
    //------------------------------------------------------
    //-- L2MUX Interfaces
    //------------------------------------------------------
    //-- To L2MUX / Data
    .soL2MUX_Data_TDATA       (ssIPTX_L2MUX_Data_tdata),
    .soL2MUX_Data_TKEEP       (ssIPTX_L2MUX_Data_tkeep),
    .soL2MUX_Data_TLAST       (ssIPTX_L2MUX_Data_tlast),
    .soL2MUX_Data_TVALID      (ssIPTX_L2MUX_Data_tvalid),
    .soL2MUX_Data_TREADY      (ssIPTX_L2MUX_Data_tready),
    //-- 
    .piMMIO_SubNetMask_V      (piMMIO_SubNetMask), 
    .piMMIO_GatewayAddr_V     (piMMIO_GatewayAddr),
    .piMMIO_MacAddress_V      (piMMIO_MacAddress)
  );  // End of IPTX
  `endif  // End of USE_DEPRECATED_DIRECTIVES

  //============================================================================
  //  INST: AXI4-STREAM-REGISTER-SLICE (IPTX ==>[ARS8]==> ARP)
  //============================================================================
  AxisRegisterSlice_32 ARS8 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer3Rst),
    //-- From IPTX / MacLkpReq ---------
    .s_axis_tdata   (ssIPTX_ARS8_MacLkpReq_tdata),
    .s_axis_tvalid  (ssIPTX_ARS8_MacLkpReq_tvalid),
    .s_axis_tready  (ssIPTX_ARS8_MacLkpReq_tready),
    //-- To   ARP / MacLkpReq ----------
    .m_axis_tdata   (ssARS8_ARP_MacLkpReq_tdata),
    .m_axis_tvalid  (ssARS8_ARP_MacLkpReq_tvalid),
    .m_axis_tready  (ssARS8_ARP_MacLkpReq_tready)
  ); 

  //============================================================================
  //  INST: L2MUX-AXI4-STREAM-INTERCONNECT-RTL (Muxes IP and ARP)
  //============================================================================
  AxisInterconnectRtl_2S1M_D8 L2MUX (
    .ACLK                 (piShlClk), 
    .ARESETN              (~piMMIO_Layer3Rst),
     //-- 
    .S00_AXIS_ACLK        (piShlClk), 
    .S01_AXIS_ACLK        (piShlClk), 
    .S00_AXIS_ARESETN     (~piMMIO_Layer3Rst),
    .S01_AXIS_ARESETN     (~piMMIO_Layer3Rst),
    //------------------------------------------------------
    //-- ARP Interfaces
    //------------------------------------------------------   
    //-- From ARP / Data ---------------
    .S00_AXIS_TDATA       (ssARP_L2MUX_Data_tdata),
    .S00_AXIS_TKEEP       (ssARP_L2MUX_Data_tkeep),
    .S00_AXIS_TLAST       (ssARP_L2MUX_Data_tlast),
    .S00_AXIS_TVALID      (ssARP_L2MUX_Data_tvalid),
    .S00_AXIS_TREADY      (ssARP_L2MUX_Data_tready),
    //------------------------------------------------------
    //-- IPTX Interfaces
    //------------------------------------------------------   
    //-- From IPTX / Data --------------
    .S01_AXIS_TDATA       (ssIPTX_L2MUX_Data_tdata),
    .S01_AXIS_TKEEP       (ssIPTX_L2MUX_Data_tkeep),
    .S01_AXIS_TLAST       (ssIPTX_L2MUX_Data_tlast),
    .S01_AXIS_TVALID      (ssIPTX_L2MUX_Data_tvalid),
    .S01_AXIS_TREADY      (ssIPTX_L2MUX_Data_tready),
     //--
    .M00_AXIS_ACLK        (piShlClk), 
    .M00_AXIS_ARESETN     (~piMMIO_Layer3Rst),
    //------------------------------------------------------
    //-- ETH / Ethernet Layer-2 Interface
    //------------------------------------------------------   
    //-- To   ETH / Data ---------------
    .M00_AXIS_TDATA       (soETH_Data_tdata),
    .M00_AXIS_TKEEP       (soETH_Data_tkeep),
    .M00_AXIS_TLAST       (soETH_Data_tlast),
    .M00_AXIS_TVALID      (soETH_Data_tvalid),
    .M00_AXIS_TREADY      (soETH_Data_tready),
     //--
    .S00_ARB_REQ_SUPPRESS (1'b0), 
    .S01_ARB_REQ_SUPPRESS (1'b0)
  );

  //============================================================================
  //  INST: READY-LOGIC-BARRIER
  //============================================================================
  `ifdef USE_DEPRECATED_DIRECTIVES
   ReadyLogicBarrier RLB (
     .aclk                    (piShlClk),
     .aresetn                 (~piMMIO_Layer4Rst),
     //------------------------------------------------------
     //-- MMIO Interface
     //------------------------------------------------------
     .poMMIO_Ready_V          (poMMIO_NtsReady),
      //------------------------------------------------------
      //-- UOE / Data Stream Interface
      //------------------------------------------------------
      .siUOE_Ready_TDATA      (ssARS6_RLB_Ready_tdata),
      .siUOE_Ready_TVALID     (ssARS6_RLB_Ready_tvalid),
      .siUOE_Ready_TREADY     (ssARS6_RLB_Ready_tready),
      //------------------------------------------------------
      //-- TOE / Data Stream Interface
      //------------------------------------------------------
      .siTOE_Ready_TDATA      (sHIGH_8b1),  // [FIXME] (ssTOE_RML_Ready_tdata),
      .siTOE_Ready_TVALID     (sHIGH_1b1),  // [FIXME] (ssTOE_RML_Ready_tvalid),
      .siTOE_Ready_TREADY     ()            // [FIXME] (ssTOE_RML_Ready_tready)
   ); // End of RLB    
  `else
   ReadyLogicBarrier RLB (
    .ap_clk                   (piShlClk),
    .ap_rst_n                 (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    .poMMIO_Ready_V           (poMMIO_NtsReady),
     //------------------------------------------------------
     //-- UOE / Data Stream Interface
     //------------------------------------------------------
     .siUOE_Ready_V_TDATA     (ssARS6_RLB_Ready_tdata),
     .siUOE_Ready_V_TVALID    (ssARS6_RLB_Ready_tvalid),
     .siUOE_Ready_V_TREADY    (ssARS6_RLB_Ready_tready),
     //------------------------------------------------------
     //-- TOE / Data Stream Interface
     //------------------------------------------------------
     .siTOE_Ready_V_TDATA     (sHIGH_8b1),  // [FIXME] (ssTOE_RML_Ready_tdata),
     .siTOE_Ready_V_TVALID    (sHIGH_1b1),  // [FIXME] (ssTOE_RML_Ready_tvalid),
     .siTOE_Ready_V_TREADY    ()            // [FIXME] (ssTOE_RML_Ready_tready)
  ); // End of RLB
 `endif  // End of USE_DEPRECATED_DIRECTIVES

endmodule
