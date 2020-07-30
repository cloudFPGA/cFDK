/*
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
 */

// *****************************************************************************
// *
// * Title : Toplevel of the TCP/IP subsystem stack instantiated by the SHELL.
// *
// * File    : nts_TcpIp.v
// *
// * Created : Nov. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Tools   : Vivado v2016.4, v2017.4 (64-bit)
// * Depends : None
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

`define USE_DEPRECATED_DIRECTIVES

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
  //---- Axi4-Stream TCP Data ---------------
  input [ 63:0]  siAPP_Tcp_Data_tdata,
  input [  7:0]  siAPP_Tcp_Data_tkeep,
  input          siAPP_Tcp_Data_tvalid,
  input          siAPP_Tcp_Data_tlast,
  output         siAPP_Tcp_Data_tready,
  //---- Axi4-Stream TCP Metadata -----------
  input [ 15:0]  siAPP_Tcp_Meta_tdata,
  input          siAPP_Tcp_Meta_tvalid,
  output         siAPP_Tcp_Meta_tready,
  //---- Axi4-Stream TCP Data Status --------
  output [ 23:0] soAPP_Tcp_DSts_tdata,
  output         soAPP_Tcp_DSts_tvalid,
  input          soAPP_Tcp_DSts_tready,

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
  output         poMMIO_NtsReady,
  
  output         poVoid
  
); // End of PortList


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire          sTODO_1b0  =  1'b0;
  wire          sTODO_1b1  =  1'b1;
  wire  [ 7:0]  sTODO_8b1  =  8'b11111111;

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
  //-- IPRX ==> ICMP/Ttl
  wire  [63:0]  ssIPRX_ICMP_Derr_tdata;
  wire  [ 7:0]  ssIPRX_ICMP_Derr_tkeep;
  wire          ssIPRX_ICMP_Derr_tlast;
  wire          ssIPRX_ICMP_Derr_tvalid;
  wire          ssIPRX_ICMP_Derr_tready;
  //-- IPRX ==> UOE
  wire  [63:0]  ssIPRX_UOE_Data_tdata;
  wire  [ 7:0]  ssIPRX_UOE_Data_tkeep;
  wire          ssIPRX_UOE_Data_tlast;
  wire          ssIPRX_UOE_Data_tvalid;
  wire          ssIPRX_UOE_Data_tready;
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
  
  //------------------------------------------------------------------
  //-- UOE = UDP-OFFLOAD-ENGINE
  //------------------------------------------------------------------
  //-- UOE ==> RML / ReadyMergeLogic
  wire  [ 7:0]  ssUOE_RML_Ready_tdata;
  wire          ssUOE_RML_Ready_tvalid;
  wire          ssUOE_RML_Ready_tready;
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
  //-- TOE ==> RML / ReadyMergeLogic
  wire  [ 7:0]  ssTOE_RML_Ready_tdata;
  wire          ssTOE_RML_Ready_tvalid;
  wire          ssTOE_RML_Ready_tready;
  //-- TOE ==>[ARS3]==> L3MUX / Data
  //---- TOE ==> [ARS3]
  wire  [63:0]  ssTOE_ARS3_Data_tdata;
  wire  [ 7:0]  ssTOE_ARS3_Data_tkeep;
  wire          ssTOE_ARS3_Data_tlast;
  wire          ssTOE_ARS3_Data_tvalid;
  wire          ssTOE_ARS3_Data_tready;
  //----         [ARS3] ==> L3MUX
  wire  [63:0]  ssARS3_L3MUX_Data_tdata;
  wire  [ 7:0]  ssARS3_L3MUX_Data_tkeep;
  wire          ssARS3_L3MUX_Data_tlast;
  wire          ssARS3_L3MUX_Data_tvalid;
  wire          ssARS3_L3MUX_Data_tready;
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
  //-- ARP ==> IPTX / LkpRpl
  wire [55:0]   ssARP_IPTX_MacLkpRep_tdata;
  wire          ssARP_IPTX_MacLkpRep_tvalid;
  wire          ssARP_IPTX_MacLkpRep_tready;
  
  //------------------------------------------------------------------
  //-- IPTX = IP-TX-HANDLER
  //------------------------------------------------------------------ 
  //-- IPTX ==> ARP / LookupRequest
  wire  [31:0]  ssIPTX_ARP_MacLkpReq_tdata;
  wire          ssIPTX_ARP_MacLkpReq_tvalid;
  wire          ssIPTX_ARP_MacLkpReq_tready;
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
  assign poVoid = sTODO_1b0;
  
  assign siMEM_TxP_RdSts_tready = sTODO_1b1; // [FIXME - Add TxP_RdSts to TOE]
  assign siMEM_RxP_RdSts_tready = sTODO_1b1; // [FIXME - Add RxP_RdSts to TOE]
  
  //============================================================================
  //  INST: IP-RX-HANDLER
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES

  IpRxHandler IPRX (
    //------------------------------------------------------
    //-- From SHELL Interfaces
    //------------------------------------------------------
    //-- Global Clock & Reset
    .aclk                     (piShlClk),
    .aresetn                  (~piMMIO_Layer3Rst),
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
    .soICMP_Derr_TDATA        (ssIPRX_ICMP_Derr_tdata),
    .soICMP_Derr_TKEEP        (ssIPRX_ICMP_Derr_tkeep),
    .soICMP_Derr_TLAST        (ssIPRX_ICMP_Derr_tlast),
    .soICMP_Derr_TVALID       (ssIPRX_ICMP_Derr_tvalid),
    .soICMP_Derr_TREADY       (ssIPRX_ICMP_Derr_tready),
    //------------------------------------------------------
    //-- UOE Interface
    //------------------------------------------------------
    //-- To UOE / Data -----------------
    .soUOE_Data_TDATA         (ssIPRX_UOE_Data_tdata),
    .soUOE_Data_TKEEP         (ssIPRX_UOE_Data_tkeep),
    .soUOE_Data_TLAST         (ssIPRX_UOE_Data_tlast),
    .soUOE_Data_TVALID        (ssIPRX_UOE_Data_tvalid),
    .soUOE_Data_TREADY        (ssIPRX_UOE_Data_tready),
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

`endif //  `ifdef USE_DEPRECATED_DIRECTIVES
 
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==>[ARS0]==> ARP)
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
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==>[ARS1]==> ICMP)
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
  //  INST: ARP 
  //============================================================================
  AddressResolutionProcess ARP (
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
    .siIPTX_MacLkpReq_TDATA         (ssIPTX_ARP_MacLkpReq_tdata),
    .siIPTX_MacLkpReq_TVALID        (ssIPTX_ARP_MacLkpReq_tvalid),
    .siIPTX_MacLkpReq_TREADY        (ssIPTX_ARP_MacLkpReq_tready),
    //--
    .soIPTX_MacLkpRep_TDATA         (ssARP_IPTX_MacLkpRep_tdata),
    .soIPTX_MacLkpRep_TVALID        (ssARP_IPTX_MacLkpRep_tvalid),
    .soIPTX_MacLkpRep_TREADY        (ssARP_IPTX_MacLkpRep_tready)
  ); // End of ARP
  
  //============================================================================
  //  INST: TCP-OFFLOAD-ENGINE
  //============================================================================
  TcpOffloadEngine TOE ( 
    .aclk                      (piShlClk),
    .aresetn                   (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_IpAddr_V           (piMMIO_Ip4Address),
    .poNTS_Ready_V             (),     // [FIXME-ssTOE_RML_Ready_tdata]
    //------------------------------------------------------
    //-- IPRX / IP Rx Data Interface
    //------------------------------------------------------
    .siIPRX_Data_TDATA         (ssARS2_TOE_Data_tdata),
    .siIPRX_Data_TKEEP         (ssARS2_TOE_Data_tkeep),
    .siIPRX_Data_TLAST         (ssARS2_TOE_Data_tlast),
    .siIPRX_Data_TVALID        (ssARS2_TOE_Data_tvalid),
    .siIPRX_Data_TREADY        (ssARS2_TOE_Data_tready),
    //------------------------------------------------------
    //-- L3MUX / IP Tx Data Interface (via ARS3)
    //------------------------------------------------------
    .soL3MUX_Data_TDATA        (ssTOE_ARS3_Data_tdata),
    .soL3MUX_Data_TKEEP        (ssTOE_ARS3_Data_tkeep),
    .soL3MUX_Data_TLAST        (ssTOE_ARS3_Data_tlast),
    .soL3MUX_Data_TVALID       (ssTOE_ARS3_Data_tvalid),
    .soL3MUX_Data_TREADY       (ssTOE_ARS3_Data_tready),
    //------------------------------------------------------
    //-- TAIF / TCP Rx Data Interfaces
    //------------------------------------------------------
    //-- To   APP / TCP Data Notification
    .soTAIF_Notif_TDATA        (soAPP_Tcp_Notif_tdata),
    .soTAIF_Notif_TVALID       (soAPP_Tcp_Notif_tvalid),  
    .soTAIF_Notif_TREADY       (soAPP_Tcp_Notif_tready),
    //-- From APP / TCP Data Read Request
    .siTAIF_DReq_TDATA         (siAPP_Tcp_DReq_tdata),
    .siTAIF_DReq_TVALID        (siAPP_Tcp_DReq_tvalid),
    .siTAIF_DReq_TREADY        (siAPP_Tcp_DReq_tready),
    //-- To   APP (via ARS4) / TCP Data Stream
    .soTAIF_Data_TDATA         (soAPP_Tcp_Data_tdata),
    .soTAIF_Data_TKEEP         (soAPP_Tcp_Data_tkeep),
    .soTAIF_Data_TLAST         (soAPP_Tcp_Data_tlast),
    .soTAIF_Data_TVALID        (soAPP_Tcp_Data_tvalid),
    .soTAIF_Data_TREADY        (soAPP_Tcp_Data_tready),
    //-- To   APP (via ARS4) / TCP Metadata   _Rol_
    .soTAIF_Meta_TDATA         (soAPP_Tcp_Meta_tdata),
    .soTAIF_Meta_TVALID        (soAPP_Tcp_Meta_tvalid),
    .soTAIF_Meta_TREADY        (soAPP_Tcp_Meta_tready),
    //------------------------------------------------------
    //-- TAIF / TCP Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- From APP / TCP Listen Port Request
    .siTAIF_LsnReq_TDATA       (siAPP_Tcp_LsnReq_tdata),
    .siTAIF_LsnReq_TVALID      (siAPP_Tcp_LsnReq_tvalid),
    .siTAIF_LsnReq_TREADY      (siAPP_Tcp_LsnReq_tready),
    //-- To   APP / TCP Listen Port Ack
    .soTAIF_LsnRep_TDATA       (soAPP_Tcp_LsnRep_tdata),
    .soTAIF_LsnRep_TVALID      (soAPP_Tcp_LsnRep_tvalid),
    .soTAIF_LsnRep_TREADY      (soAPP_Tcp_LsnRep_tready),
    //------------------------------------------------------
    //-- TAIF / TCP Tx Data Flow Interfaces
    //------------------------------------------------------
    //-- From APP (via ARS5) / TCP Data Stream
    .siTAIF_Data_TDATA         (siAPP_Tcp_Data_tdata),
    .siTAIF_Data_TKEEP         (siAPP_Tcp_Data_tkeep),
    .siTAIF_Data_TLAST         (siAPP_Tcp_Data_tlast),
    .siTAIF_Data_TVALID        (siAPP_Tcp_Data_tvalid),
    .siTAIF_Data_TREADY        (siAPP_Tcp_Data_tready),
    //-- From APP (via ARS5) / TCP Metadata
    .siTAIF_Meta_TDATA         (siAPP_Tcp_Meta_tdata),
    .siTAIF_Meta_TVALID        (siAPP_Tcp_Meta_tvalid),
    .siTAIF_Meta_TREADY        (siAPP_Tcp_Meta_tready),
    //-- To  APP / TCP Data Write Status
    .soTAIF_DSts_TDATA         (soAPP_Tcp_DSts_tdata),
    .soTAIF_DSts_TVALID        (soAPP_Tcp_DSts_tvalid),
    .soTAIF_DSts_TREADY        (soAPP_Tcp_DSts_tready),
    //------------------------------------------------------
    //-- APP / TRIF / TCP Tx Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From ROLE / TCP Open Session Request
    .siTAIF_OpnReq_TDATA       (siAPP_Tcp_OpnReq_tdata),
    .siTAIF_OpnReq_TVALID      (siAPP_Tcp_OpnReq_tvalid),
    .siTAIF_OpnReq_TREADY      (siAPP_Tcp_OpnReq_tready),
    //-- To   ROLE / TCP Open Session Reply
    .soTAIF_OpnRep_TDATA       (soAPP_Tcp_OpnRep_tdata),
    .soTAIF_OpnRep_TVALID      (soAPP_Tcp_OpnRep_tvalid),
    .soTAIF_OpnRep_TREADY      (soAPP_Tcp_OpnRep_tready),
    //-- From ROLE / TCP Close Session Request
    .siTAIF_ClsReq_TDATA       (siAPP_Tcp_ClsReq_tdata),
    .siTAIF_ClsReq_TVALID      (siAPP_Tcp_ClsReq_tvalid),
    .siTAIF_ClsReq_TREADY      (siAPP_Tcp_ClsReq_tready),
    //-- To   ROLE / TCP Close Session Status
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
  
  //============================================================================
  //  INST: CONTENT ADDRESSABLE MEMORY
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

`endif
  
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==>[ARS2]==> TOE)
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
  //  INST: AXI4-STREAM REGISTER SLICE (TOE ==>[ARS3]==> IPTX)
  //============================================================================
  AxisRegisterSlice_64 ARS3 (
    .aclk           (piShlClk),
    .aresetn        (~piMMIO_Layer4Rst),
    //-- From TOE / Data ---------------
    .s_axis_tdata   (ssTOE_ARS3_Data_tdata),
    .s_axis_tkeep   (ssTOE_ARS3_Data_tkeep),
    .s_axis_tlast   (ssTOE_ARS3_Data_tlast),
    .s_axis_tvalid  (ssTOE_ARS3_Data_tvalid),
    .s_axis_tready  (ssTOE_ARS3_Data_tready),
    //-- To   L3MUX / Data -------------
    .m_axis_tdata   (ssARS3_L3MUX_Data_tdata),
    .m_axis_tkeep   (ssARS3_L3MUX_Data_tkeep),
    .m_axis_tlast   (ssARS3_L3MUX_Data_tlast),
    .m_axis_tvalid  (ssARS3_L3MUX_Data_tvalid),
    .m_axis_tready  (ssARS3_L3MUX_Data_tready)
  ); 

  //============================================================================
  //  INST: UDP-OFFLOAD-ENGINE
  //============================================================================
  UdpOffloadEngine UOE ( 
    .aclk                       (piShlClk),
    .aresetn                    (~piMMIO_Layer4Rst),
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    .piMMIO_En_V                (piMMIO_Layer4En),
    //--
    .soMMIO_Ready_TDATA         (ssUOE_RML_Ready_tdata),
    .soMMIO_Ready_TVALID        (ssUOE_RML_Ready_tvalid),
    .soMMIO_Ready_TREADY        (ssUOE_RML_Ready_tready),
    //------------------------------------------------------
    //-- IPRX Data Interface
    //------------------------------------------------------
    .siIPRX_Data_TDATA          (ssIPRX_UOE_Data_tdata),
    .siIPRX_Data_TKEEP          (ssIPRX_UOE_Data_tkeep),
    .siIPRX_Data_TLAST          (ssIPRX_UOE_Data_tlast),
    .siIPRX_Data_TVALID         (ssIPRX_UOE_Data_tvalid),
    .siIPRX_Data_TREADY         (ssIPRX_UOE_Data_tready),
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
  ); // End-of: UdpOffloadEngine
     
  //============================================================================
  //  INST: ICMP-SERVER
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES

  InternetControlMessageProcess ICMP (                   
    //------------------------------------------------------
    //-- From SHELL Interfaces
    //------------------------------------------------------
    //-- Global Clock & Reset
    .aclk                 (piShlClk),
    .aresetn              (~piMMIO_Layer3Rst),
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
    .siIPRX_Derr_TDATA    (ssIPRX_ICMP_Derr_tdata),
    .siIPRX_Derr_TKEEP    (ssIPRX_ICMP_Derr_tkeep),
    .siIPRX_Derr_TLAST    (ssIPRX_ICMP_Derr_tlast),
    .siIPRX_Derr_TVALID   (ssIPRX_ICMP_Derr_tvalid),
    .siIPRX_Derr_TREADY   (ssIPRX_ICMP_Derr_tready),
    //------------------------------------------------------
    //-- UOE Interfaces
    //------------------------------------------------------
    //-- From UOE / Data   
    .siUOE_Data_TDATA     (ssUOE_ICMP_Data_tdata),  // [TODO-Rename siUDP_Data_TDATA into siUOE_Data_TDATA]
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

`endif // `ifdef USE_DEPRECATED_DIRECTIVES
   
   
  //============================================================================
  //  INST: L3MUX AXI4-STREAM INTERCONNECT RTL (Muxes ICMP, TOE, and UOE)
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
    //-- From TOE Interfaces (via [ARS3])
    //------------------------------------------------------
    .S02_AXIS_TDATA     (ssARS3_L3MUX_Data_tdata),
    .S02_AXIS_TKEEP     (ssARS3_L3MUX_Data_tkeep),
    .S02_AXIS_TLAST     (ssARS3_L3MUX_Data_tlast),
    .S02_AXIS_TVALID    (ssARS3_L3MUX_Data_tvalid),
    .S02_AXIS_TREADY    (ssARS3_L3MUX_Data_tready),
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
  //  INST: IP TX HANDLER
  //============================================================================
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
   .soARP_LookupReq_TDATA     (ssIPTX_ARP_MacLkpReq_tdata),
   .soARP_LookupReq_TVALID    (ssIPTX_ARP_MacLkpReq_tvalid),
   .soARP_LookupReq_TREADY    (ssIPTX_ARP_MacLkpReq_tready),
    //-- From ARP / LookupReply --------
    .siARP_LookupRep_TDATA    (ssARP_IPTX_MacLkpRep_tdata),
    .siARP_LookupRep_TVALID   (ssARP_IPTX_MacLkpRep_tvalid),
    .siARP_LookupRep_TREADY   (ssARP_IPTX_MacLkpRep_tready),
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
    

  //============================================================================
  //  INST: L2MUX AXI4-STREAM INTERCONNECT RTL (Muxes IP and ARP)
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
  //  INST: READY LOGIC BARRIER
  //============================================================================
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
     .siUOE_Ready_V_TDATA     (ssUOE_RML_Ready_tdata),
     .siUOE_Ready_V_TVALID    (ssUOE_RML_Ready_tvalid),
     .siUOE_Ready_V_TREADY    (ssUOE_RML_Ready_tready),
     //------------------------------------------------------
     //-- TOE / Data Stream Interface
     //------------------------------------------------------
     .siTOE_Ready_V_TDATA     (sTODO_8b1),  // [FIXME] (ssTOE_RML_Ready_tdata),
     .siTOE_Ready_V_TVALID    (sTODO_1b1),  // [FIXME] (ssTOE_RML_Ready_tvalid),
     .siTOE_Ready_V_TREADY    ()            // [FIXME] (ssTOE_RML_Ready_tready)
  ); // End of RLB

endmodule
