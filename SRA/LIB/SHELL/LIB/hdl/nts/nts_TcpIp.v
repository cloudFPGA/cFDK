// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Toplevel of the TCP/IP subsystem stack instantiated by the SHELL.
// *
// * File    : nts_TcpIp.v
// *
// * Created : Nov. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : This is the toplevel design of the TCP/IP-based networking
// *    subsystem that is instantiated by the shell of the current target
// *    platform to transfer data sequences between the user application layer
// *    and the underlaying Ethernet media layer.
// *    From an Open Systems Interconnection (OSI) model point of view, this
// *    module implements the Network layer (L3), the Transport layer (L4) and
// *    the Session layer (L5) of the OSI model.
// * 
// *****************************************************************************

`timescale 1ns / 1ps

`define USE_DEPRECATED_DIRECTIVES

// *****************************************************************************
// **  MODULE - IP NETWORK + TCP/UDP TRANSPORT + DHCP SESSION SUBSYSTEM
// *****************************************************************************

module NetworkTransportSession_TcpIp (

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
   
  // OBSOLETE-20190826 //-- System Reset --------------------------------------
  // OBSOLETE-20190826 //--   (This is a delayed version of the global reset. We use it when we
  // OBSOLETE-20190826 //--    specifically want to control the re-initialization of a HLS variable.
  // OBSOLETE-20190826 //--    We recommended to leave the "config_rtl" configuration to its default
  // OBSOLETE-20190826 //--    "control" setting and to use this signal to provide finer grain reset
  // OBSOLETE-20190826 //--    functionnality. See "Controlling the Reset Behavior" in UG902).
  // OBSOLETE-20190826 input          piShlRstDly,
  
  //------------------------------------------------------
  //-- ETH / Ethernet Layer-2 Interfaces
  //------------------------------------------------------
  //-- Input AXIS Interface ---------------------- 
  input [ 63:0]  siETH_Data_tdata, 
  input [  7:0]  siETH_Data_tkeep,
  input          siETH_Data_tlast,
  input          siETH_Data_tvalid,
  output         siETH_Data_tready,
  //-- Output AXIS Interface --------------------- 
  output [ 63:0] soETH_Data_tdata,
  output [  7:0] soETH_Data_tkeep,
  output         soETH_Data_tlast,
  output         soETH_Data_tvalid,
  input          soETH_Data_tready,
 
  //------------------------------------------------------
  //-- MEM / TxP Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path / S2MM-AXIS --------------------
  //---- Stream Read Command -------------------
  output[ 79:0]  soMEM_TxP_RdCmd_tdata,
  output         soMEM_TxP_RdCmd_tvalid,
  input          soMEM_TxP_RdCmd_tready,
  //---- Stream Read Status ------------------
  input [  7:0]  siMEM_TxP_RdSts_tdata,
  input          siMEM_TxP_RdSts_tvalid,
  output         siMEM_TxP_RdSts_tready,
  //---- Stream Data Input Channel ----------
  input [ 63:0]  siMEM_TxP_Data_tdata,
  input [  7:0]  siMEM_TxP_Data_tkeep,
  input          siMEM_TxP_Data_tlast,
  input          siMEM_TxP_Data_tvalid,
  output         siMEM_TxP_Data_tready,
  //---- Stream Write Command ----------------
  output [ 79:0] soMEM_TxP_WrCmd_tdata,
  output         soMEM_TxP_WrCmd_tvalid,
  input          soMEM_TxP_WrCmd_tready,
  //---- Stream Write Status -----------------
  input [  7:0]  siMEM_TxP_WrSts_tdata,
  input          siMEM_TxP_WrSts_tvalid,
  output         siMEM_TxP_WrSts_tready,
  //---- Stream Data Output Channel ----------
  output [ 63:0] soMEM_TxP_Data_tdata,
  output [  7:0] soMEM_TxP_Data_tkeep,
  output         soMEM_TxP_Data_tlast,
  output         soMEM_TxP_Data_tvalid,
  input          soMEM_TxP_Data_tready,

  //------------------------------------------------------
  //-- MEM / RxP Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path / S2MM-AXIS -------------
  //---- Stream Read Command -----------------
  output [ 79:0] soMEM_RxP_RdCmd_tdata,
  output         soMEM_RxP_RdCmd_tvalid,
  input          soMEM_RxP_RdCmd_tready,
  //---- Stream Read Status ------------------
  input [   7:0] siMEM_RxP_RdSts_tdata,
  input          siMEM_RxP_RdSts_tvalid,
  output         siMEM_RxP_RdSts_tready,
  //---- Stream Data Input Channel ----------
  input [ 63:0]  siMEM_RxP_Data_tdata,
  input [  7:0]  siMEM_RxP_Data_tkeep,
  input          siMEM_RxP_Data_tlast,
  input          siMEM_RxP_Data_tvalid,
  output         siMEM_RxP_Data_tready,
  //---- Stream Write Command ----------------=
  output[ 79:0]  soMEM_RxP_WrCmd_tdata,
  output         soMEM_RxP_WrCmd_tvalid,
  input          soMEM_RxP_WrCmd_tready,
  //---- Stream Write Status -----------------
  input [  7:0]  siMEM_RxP_WrSts_tdata,
  input          siMEM_RxP_WrSts_tvalid,
  output         siMEM_RxP_WrSts_tready,
  //---- Stream Data Input Channel -----------
  output [ 63:0] soMEM_RxP_Data_tdata,
  output [  7:0] soMEM_RxP_Data_tkeep,
  output         soMEM_RxP_Data_tlast,
  output         soMEM_RxP_Data_tvalid,
  input          soMEM_RxP_Data_tready, 

  //------------------------------------------------------
  //-- NRC/Role / Nts0 / Udp Interfaces
  //------------------------------------------------------
  //-- UDMX ==> URIF / Open Port Acknowledge -----
  output  [ 7:0]  soROL_Udp_OpnAck_tdata,
  output          soROL_Udp_OpnAck_tvalid,
  input           soROL_Udp_OpnAck_tready,
  //-- UDMX ==> URIF / Data ----------------------
  output  [63:0]  soROL_Udp_Data_tdata,
  output  [ 7:0]  soROL_Udp_Data_tkeep,
  output          soROL_Udp_Data_tlast,
  output          soROL_Udp_Data_tvalid,
  input           soROL_Udp_Data_tready,
  //-- UDMX ==> URIF / Meta ----------------------
  output  [95:0]  soROL_Udp_Meta_tdata,
  output          soROL_Udp_Meta_tvalid,
  input           soROL_Udp_Meta_tready,
  //-- URIF ==> UDMX / OpenPortRequest / Axis ----
  input   [15:0]  siROL_Udp_OpnReq_tdata,
  input           siROL_Udp_OpnReq_tvalid,
  output          siROL_Udp_OpnReq_tready,
  //-- URIF ==> UDMX / Data / Axis ---------------
  input   [63:0]  siROL_Udp_Data_tdata,
  input   [ 7:0]  siROL_Udp_Data_tkeep,
  input           siROL_Udp_Data_tlast,
  input           siROL_Udp_Data_tvalid,
  output          siROL_Udp_Data_tready,
  //-- URIF ==> UDMX / Meta / Axis ---------------
  input   [95:0]  siROL_Udp_Meta_tdata,
  input           siROL_Udp_Meta_tvalid,
  output          siROL_Udp_Meta_tready,
  //-- URIF ==> UDMX / TxLen / Axis --------------
  input   [15:0]  siROL_Udp_PLen_tdata,
  input           siROL_Udp_PLen_tvalid,
  output          siROL_Udp_PLen_tready,

  //------------------------------------------------------
  //-- ROLE / Tcp / TxP Data Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path (ROLE-->SHELL) ---------
  //---- Stream TCP Data ---------------------
  input [ 63:0]  siROL_Tcp_Data_tdata,
  input [  7:0]  siROL_Tcp_Data_tkeep,
  input          siROL_Tcp_Data_tvalid,
  input          siROL_Tcp_Data_tlast,
  output         siROL_Tcp_Data_tready,
  //---- Stream TCP Metadata -----------------
  input [ 15:0]  siROL_Tcp_Meta_tdata,
  input          siROL_Tcp_Meta_tvalid,
  output         siROL_Tcp_Meta_tready,
  //---- Stream TCP Data Status --------------
  output [ 23:0] soROL_Tcp_DSts_tdata,
  output         soROL_Tcp_DSts_tvalid,
  input          soROL_Tcp_DSts_tready,

  //------------------------------------------------------
  //-- ROLE / Tcp / RxP Data Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path (SHELL-->ROLE) ----------
  //-- Stream TCP Data -----------------------
  output [ 63:0] soROL_Tcp_Data_tdata,
  output [  7:0] soROL_Tcp_Data_tkeep,
  output         soROL_Tcp_Data_tvalid,
  output         soROL_Tcp_Data_tlast,
  input          soROL_Tcp_Data_tready,
  //-- Stream TCP Metadata -------------------
  output [ 15:0] soROL_Tcp_Meta_tdata,
  output         soROL_Tcp_Meta_tvalid,
  input          soROL_Tcp_Meta_tready,
  //-- Stream TCP Data Notification ----------
  output [103:0] soROL_Tcp_Notif_tdata,
  output         soROL_Tcp_Notif_tvalid,
  input          soROL_Tcp_Notif_tready,
  //-- Stream TCP Data Request ---------------
  input  [ 31:0] siROL_Tcp_DReq_tdata,
  input          siROL_Tcp_DReq_tvalid,
  output         siROL_Tcp_DReq_tready,
  
  //------------------------------------------------------
  //-- ROLE / Tcp / TxP Ctlr Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path (ROLE-->SHELL) ---------
  //---- Stream TCP Open Session Request -----
  input [ 47:0]  siROL_Tcp_OpnReq_tdata,
  input          siROL_Tcp_OpnReq_tvalid,
  output         siROL_Tcp_OpnReq_tready,
  //---- Stream TCP Open Session Reply -------
  output [ 23:0] soROL_Tcp_OpnRep_tdata,
  output         soROL_Tcp_OpnRep_tvalid,
  input          soROL_Tcp_OpnRep_tready,
  //---- Stream TCP Close Request ------------
  input [ 15:0]  siROL_Tcp_ClsReq_tdata,
  input          siROL_Tcp_ClsReq_tvalid,
  output         siROL_Tcp_ClsReq_tready,
  
  //------------------------------------------------------
  //-- ROLE / Tcp / RxP Ctlr Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path (SHELL-->ROLE) ----------
  //---- Stream TCP Listen Request -----------
  input [ 15:0]  siROL_Tcp_LsnReq_tdata,
  input          siROL_Tcp_LsnReq_tvalid,
  output         siROL_Tcp_LsnReq_tready,
  //---- Stream TCP Listen Ackknowledge ------
  output [  7:0] soROL_Tcp_LsnAck_tdata,  // AckBit stream must be 8-bits boundary
  output         soROL_Tcp_LsnAck_tvalid,
  input          soROL_Tcp_LsnAck_tready,
 
  //------------------------------------------------------
  //-- MMIO / Interfaces
  //------------------------------------------------------
  input          piMMIO_Layer3Rst,
  input          piMMIO_Layer4Rst,
  input  [ 47:0] piMMIO_MacAddress,
  input  [ 31:0] piMMIO_IpAddress,
  input  [ 31:0] piMMIO_SubNetMask,
  input  [ 31:0] piMMIO_GatewayAddr,
  output         poMMIO_CamReady,
  output         poMMIO_ToeReady,
  
  output         poVoid
  
); // End of PortList


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire          sTODO_1b0  =  1'b0;

  //------------------------------------------------------
  //-- IPRX = IP-RX-HANDLER
  //------------------------------------------------------
  //-- IPRX ==>[ARS0]==> ARP ---------------------
  //---- IPRX ==>[ARS0]
  wire  [63:0]  ssIPRX_ARS0_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS0_Data_tkeep;
  wire          ssIPRX_ARS0_Data_tlast;
  wire          ssIPRX_ARS0_Data_tvalid;
  wire          ssIPRX_ARS0_Data_tready;
  //             [ARS0]==> ARP/Data ----
  wire  [63:0]  ssARS0_ARP_Data_tdata;
  wire  [ 7:0]  ssARS0_ARP_Data_tkeep;
  wire          ssARS0_ARP_Data_tlast;
  wire          ssARS0_ARP_Data_tvalid;
  wire          ssARS0_ARP_Data_tready;
  //-- IPRX ==>[ARS1]==> ICMP/Data ---------------
  //---- IPRX ==>[ARS1] 
  wire  [63:0]  ssIPRX_ARS1_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS1_Data_tkeep;
  wire          ssIPRX_ARS1_Data_tlast;
  wire          ssIPRX_ARS1_Data_tvalid;
  wire          ssIPRX_ARS1_Data_tready;
  //             [ARS1]==> ICMP/Data ---
  wire  [63:0]  ssARS1_ICMP_Data_tdata;
  wire  [ 7:0]  ssARS1_ICMP_Data_tkeep;
  wire          ssARS1_ICMP_Data_tlast;
  wire          ssARS1_ICMP_Data_tvalid;
  wire          ssARS1_ICMP_Data_tready;
  
  //-- IPRX ==> ICMP/Ttl -------------------------
  wire  [63:0]  ssIPRX_ICMP_Ttl_tdata;
  wire  [ 7:0]  ssIPRX_ICMP_Ttl_tkeep;
  wire          ssIPRX_ICMP_Ttl_tlast;
  wire          ssIPRX_ICMP_Ttl_tvalid;
  wire          ssIPRX_ICMP_Ttl_tready;
    
  //-- IPRX ==> UDP ------------------------------
  wire  [63:0]  ssIPRX_UDP_Data_tdata;
  wire  [ 7:0]  ssIPRX_UDP_Data_tkeep;
  wire          ssIPRX_UDP_Data_tlast;
  wire          ssIPRX_UDP_Data_tvalid;
  wire          ssIPRX_UDP_Data_tready;
    
  //-- IPRX ==>[ARS2]==> TOE ---------------------
  //---- IPRX ==>[ARS2]
  wire  [63:0]  ssIPRX_ARS2_Data_tdata;
  wire  [ 7:0]  ssIPRX_ARS2_Data_tkeep;
  wire          ssIPRX_ARS2_Data_tlast;
  wire          ssIPRX_ARS2_Data_tvalid;
  wire          ssIPRX_ARS2_Data_tready;
  //             [ARS2]==>TOE ----------
  wire  [63:0]  ssARS2_TOE_Data_tdata;
  wire  [ 7:0]  ssARS2_TOE_Data_tkeep;
  wire          ssARS2_TOE_Data_tlast;
  wire          ssARS2_TOE_Data_tvalid;
  wire          ssARS2_TOE_Data_tready;  
  
  //------------------------------------------------------------------
  //-- UDP = UDP-CORE-MODULE
  //------------------------------------------------------------------
  //-- UDP ==> ICMP ------------------------------
  wire  [63:0]  ssUDP_ICMP_Data_tdata;
  wire  [ 7:0]  ssUDP_ICMP_Data_tkeep;
  wire          ssUDP_ICMP_Data_tlast;
  wire          ssUDP_ICMP_Data_tvalid;
  wire          ssUDP_ICMP_Data_tready;

  //-- UDP ==> L3MUX -----------------------------
  wire  [63:0]  ssUDP_L3MUX_Data_tdata;
  wire  [ 7:0]  ssUDP_L3MUX_Data_tkeep;
  wire          ssUDP_L3MUX_Data_tlast;
  wire          ssUDP_L3MUX_Data_tvalid;
  wire          ssUDP_L3MUX_Data_tready;

  //-- UDP ==> UDMX / Open Port Status -----------
  wire  [ 7:0]  ssUDP_UDMX_OpnSts_tdata;
  wire          ssUDP_UDMX_OpnSts_tvalid;
  wire          ssUDP_UDMX_OpnSts_tready;
  //-- UDP ==> UDMX / Data -----------------------
  wire  [63:0]  ssUDP_UDMX_Data_tdata;
  wire  [ 7:0]  ssUDP_UDMX_Data_tkeep;
  wire          ssUDP_UDMX_Data_tlast;
  wire          ssUDP_UDMX_Data_tvalid;
  wire          ssUDP_UDMX_Data_tready;
  //-- UDP ==> UDMX / Meta -----------------------
  wire  [95:0]  ssUDP_UDMX_Meta_tdata;
  wire          ssUDP_UDMX_Meta_tvalid;
  wire          ssUDP_UDMX_Meta_tready;

  //------------------------------------------------------------------
  //-- UDMX = UDP-MUX
  //------------------------------------------------------------------
  //-- UDMX ==> UDP / OpenPortRequest ------------
  wire  [15:0]  ssUDMX_UDP_OpnReq_tdata;
  wire          ssUDMX_UDP_OpnReq_tvalid;
  wire          ssUDMX_UDP_OpnReq_tready;
  //-- UDMX ==> UDP / Data -----------------------
  wire  [63:0]  ssUDMX_UDP_Data_tdata;
  wire  [ 7:0]  ssUDMX_UDP_Data_tkeep;
  wire          ssUDMX_UDP_Data_tlast;
  wire          ssUDMX_UDP_Data_tvalid;
  wire          ssUDMX_UDP_Data_tready;
  //-- UDMX ==> UDP / TxLength -------------------
  wire  [15:0]  ssUDMX_UDP_PLen_tdata;
  wire          ssUDMX_UDP_PLen_tvalid;
  wire          ssUDMX_UDP_PLen_tready;
  //-- UDMX ==> UDP / Meta -----------------------
  wire  [95:0]  ssUDMX_UDP_Meta_tdata;
  wire          ssUDMX_UDP_Meta_tvalid;
  wire          ssUDMX_UDP_Meta_tready;

  //-- UDMX ==> DHCP / Open Port Acknowledge -----
  wire  [ 7:0]  ssUDMX_DHCP_OpnAck_tdata;
  wire          ssUDMX_DHCP_OpnAck_tvalid;
  wire          ssUDMX_DHCP_OpnAck_tready;
  //-- UDMX ==> DHCP -----------------------------
  wire  [63:0]  ssUDMX_DHCP_Data_tdata;
  wire  [ 7:0]  ssUDMX_DHCP_Data_tkeep;
  wire          ssUDMX_DHCP_Data_tlast;
  wire          ssUDMX_DHCP_Data_tvalid;
  wire          ssUDMX_DHCP_Data_tready;
  //-- UDMX ==> DHCP -----------------------------
  wire  [95:0]  ssUDMX_DHCP_Meta_tdata;
  wire          ssUDMX_DHCP_Meta_tvalid;
  wire          ssUDMX_DHCP_Meta_tready;
  
  //------------------------------------------------------------------
  //-- DHCP = DHCP-CLIENT
  //------------------------------------------------------------------      
  //-- DHCP ==> UDMX / Open Port Request --------
  wire  [15:0]  ssDHCP_UDMX_OpnReq_tdata;
  wire          ssDHCP_UDMX_OpnReq_tvalid;
  wire          ssDHCP_UDMX_OpnReq_tready;
  //-- DHCP ==> UDMX / Data /Axis ---------------
  wire  [63:0]  ssDHCP_UDMX_Data_tdata;
  wire  [7:0]   ssDHCP_UDMX_Data_tkeep;
  wire          ssDHCP_UDMX_Data_tlast;
  wire          ssDHCP_UDMX_Data_tvalid;
  wire          ssDHCP_UDMX_Data_tready;  
  //-- DHCP ==> UDMX / Tx Length ----------------
  wire  [15:0]  ssDHCP_UDMX_PLen_tdata;
  wire          ssDHCP_UDMX_PLen_tvalid;
  wire          ssDHCP_UDMX_PLen_tready;
  //-- DHCP ==> UDMX / Tx MetaData --------------
  wire  [95:0]  ssDHCP_UDMX_Meta_tdata;
  wire          ssDHCP_UDMX_Meta_tvalid;
  wire          ssDHCP_UDMX_Meta_tready;

  //------------------------------------------------------------------
  //-- TOE = TCP-OFFLOAD-ENGINE
  //------------------------------------------------------------------
  //-- TOE ==>[ARS4]==> ROLE / Tcp / Data
  wire  [63:0]  ssTOE_ARS4_Tcp_Data_tdata;
  wire  [ 7:0]  ssTOE_ARS4_Tcp_Data_tkeep;
  wire          ssTOE_ARS4_Tcp_Data_tlast;
  wire          ssTOE_ARS4_Tcp_Data_tvalid;
  wire          ssTOE_ARS4_Tcp_Data_tready; 
  //-- TOE ==>[ARS4]==> ROLE / Tcp / Meta
  wire  [15:0]  ssTOE_ARS4_Tcp_Meta_tdata;
  wire          ssTOE_ARS4_Tcp_Meta_tvalid;
  wire          ssTOE_ARS4_Tcp_Meta_tready; 
  //-- TOE ==>[ARS3]==> L3MUX / Data ---
  //---- TOE ==> [ARS3]
  wire  [63:0]  ssTOE_ARS3_Data_tdata;
  wire  [ 7:0]  ssTOE_ARS3_Data_tkeep;
  wire          ssTOE_ARS3_Data_tlast;
  wire          ssTOE_ARS3_Data_tvalid;
  wire          ssTOE_ARS3_Data_tready;
  //----         [ARS3] ==> L3MUX ----------------
  wire  [63:0]  ssARS3_L3MUX_Data_tdata;
  wire  [ 7:0]  ssARS3_L3MUX_Data_tkeep;
  wire          ssARS3_L3MUX_Data_tlast;
  wire          ssARS3_L3MUX_Data_tvalid;
  wire          ssARS3_L3MUX_Data_tready;
  //-- TOE ==> CAM / LookupRequest -----
  wire [103:0]  ssTOE_CAM_LkpReq_tdata;  //( 1 + 96) - 1 = 96  but HLS aligns to the next 8-bit boundary 
  wire          ssTOE_CAM_LkpReq_tvalid;
  wire          ssTOE_CAM_LkpReq_tready;
  //-- TOE ==> CAM / UpdateRequest -----
  wire  [111:0] ssTOE_CAM_UpdReq_tdata;  //( 1 + 1 + 14 + 96) - 1 = 111
  wire          ssTOE_CAM_UpdReq_tvalid;
  wire          ssTOE_CAM_UpdReq_tready;
 
  //------------------------------------------------------------------
  //-- CAM = CONTENT ADDRESSABLE MEMORY
  //------------------------------------------------------------------
  //-- CAM ==> TOE / LookupReply -------
  wire  [15:0]  ssCAM_TOE_LkpRep_tdata;
  wire          ssCAM_TOE_LkpRep_tvalid;
  wire          ssCAM_TOE_LkpRep_tready;
  //-- CAM ==> TOE / UpdateReply -------
  wire  [15:0]  ssCAM_TOE_UpdRpl_tdata;
  wire          ssCAM_TOE_UpdRpl_tvalid;
  wire          ssCAM_TOE_UpdRpl_tready;

  //------------------------------------------------------------------
  //-- ICMP = ICMP-SERVER
  //------------------------------------------------------------------
  //-- ICMP ==> L3MUX / Data -----------
  wire  [63:0]  ssICMP_L3MUX_Data_tdata;
  wire  [ 7:0]  ssICMP_L3MUX_Data_tkeep;
  wire          ssICMP_L3MUX_Data_tlast;
  wire          ssICMP_L3MUX_Data_tvalid;
  wire          ssICMP_L3MUX_Data_tready;

  //------------------------------------------------------------------
  //-- ARP = ARP-SERVER
  //------------------------------------------------------------------
  //-- ARP ==> L2MUX / Data ------------
  wire  [63:0]  ssARP_L2MUX_Data_tdata;
  wire  [ 7:0]  ssARP_L2MUX_Data_tkeep;
  wire          ssARP_L2MUX_Data_tlast;
  wire          ssARP_L2MUX_Data_tvalid;
  wire          ssARP_L2MUX_Data_tready;
  //-- ARP ==> IPTX / LkpRpl -----------
  wire [55:0]   ssARP_IPTX_LkpRpl_tdata;
  wire          ssARP_IPTX_LkpRpl_tvalid;
  wire          ssARP_IPTX_LkpRpl_tready;
  
  //------------------------------------------------------------------
  //-- IPTX = IP-TX-HANDLER
  //------------------------------------------------------------------ 
  //-- IPTX ==> ARP / LookupRequest ----
  wire  [31:0]  ssIPTX_ARP_LkpReq_tdata;
  wire          ssIPTX_ARP_LkpReq_tvalid;
  wire          ssIPTX_ARP_LkpReq_tready;
  //-- IPTX ==> L2MUX / Data -----------
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

  //------------------------------------------------------------------
  //-- ARS5 = AXI REGISTER SLICE #5
  //------------------------------------------------------------------
  //-- ROLE ==>[ARS5]==> TOE ---------------------
  //---- ROLE ==>[ARS5] (see siROL_Tcp_Data_t*)
  //----         [ARS5]==> TOE ---------
  wire  [63:0]  ssARS5_TOE_Data_tdata;
  wire  [ 7:0]  ssARS5_TOE_Data_tkeep;
  wire          ssARS5_TOE_Data_tlast;
  wire          ssARS5_TOE_Data_tvalid;
  wire          ssARS5_TOE_Data_tready;
  //-- ROLE ==>[ARS5]==> TOE ---------------------
  //---- ROLE ==>[ARS5] (see siROL_Tcp_Meta_t*)
  //----         [ARS5] ==> TOE -----------------
   wire  [15:0]  ssARS5_TOE_Meta_tdata;
   wire          ssARS5_TOE_Meta_tvalid;
   wire          ssARS5_TOE_Meta_tready;

  //-- End of signal declarations ----------------------------------------------

 
  //============================================================================
  //  COMB: CONTINIOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================ 
  assign poVoid = sTODO_1b0;


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
    .aresetn                  (~(piShlRst | piMMIO_Layer3Rst)),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_This_MacAddress_V (piMMIO_MacAddress),
    .piMMIO_This_Ip4Address_V (piMMIO_IpAddress),
                      
    //------------------------------------------------------
    //-- From ETH Interfaces
    //------------------------------------------------------
    .siETH_This_Data_TDATA    (siETH_Data_tdata),
    .siETH_This_Data_TKEEP    (siETH_Data_tkeep),
    .siETH_This_Data_TLAST    (siETH_Data_tlast),
    .siETH_This_Data_TVALID   (siETH_Data_tvalid),
    .siETH_This_Data_TREADY   (siETH_Data_tready),
    
    //------------------------------------------------------
    //-- ARP Interfaces (via [ARS0])
    //------------------------------------------------------
    //-- To  ARP / Data ----------------
    .soTHIS_Arp_Data_TDATA    (ssIPRX_ARS0_Data_tdata),       
    .soTHIS_Arp_Data_TKEEP    (ssIPRX_ARS0_Data_tkeep),      
    .soTHIS_Arp_Data_TLAST    (ssIPRX_ARS0_Data_tlast),   
    .soTHIS_Arp_Data_TVALID   (ssIPRX_ARS0_Data_tvalid),
    .soTHIS_Arp_Data_TREADY   (ssIPRX_ARS0_Data_tready),      
   
    //------------------------------------------------------
    //-- ICMP Interfaces (via ARS1)
    //------------------------------------------------------
    //-- To ICMP / Data ----------------
    .soTHIS_Icmp_Data_TDATA   (ssIPRX_ARS1_Data_tdata),
    .soTHIS_Icmp_Data_TKEEP   (ssIPRX_ARS1_Data_tkeep),
    .soTHIS_Icmp_Data_TLAST   (ssIPRX_ARS1_Data_tlast),
    .soTHIS_Icmp_Data_TVALID  (ssIPRX_ARS1_Data_tvalid),
    .soTHIS_Icmp_Data_TREADY  (ssIPRX_ARS1_Data_tready),
    //-- To ICMP / Ttl -----------------
    .soTHIS_Icmp_Derr_TDATA   (ssIPRX_ICMP_Ttl_tdata),
    .soTHIS_Icmp_Derr_TKEEP   (ssIPRX_ICMP_Ttl_tkeep),
    .soTHIS_Icmp_Derr_TLAST   (ssIPRX_ICMP_Ttl_tlast),
    .soTHIS_Icmp_Derr_TVALID  (ssIPRX_ICMP_Ttl_tvalid),
    .soTHIS_Icmp_Derr_TREADY  (ssIPRX_ICMP_Ttl_tready),

    //------------------------------------------------------
    //-- UDP Interfaces
    //------------------------------------------------------
    //-- To UDP / Data -----------------
    .soTHIS_Udp_Data_TDATA    (ssIPRX_UDP_Data_tdata),
    .soTHIS_Udp_Data_TKEEP    (ssIPRX_UDP_Data_tkeep),
    .soTHIS_Udp_Data_TLAST    (ssIPRX_UDP_Data_tlast),
    .soTHIS_Udp_Data_TVALID   (ssIPRX_UDP_Data_tvalid),
    .soTHIS_Udp_Data_TREADY   (ssIPRX_UDP_Data_tready),
 
    //------------------------------------------------------
    //-- TOE Interfaces (via ARS2)
    //------------------------------------------------------
    //-- To TOE / Data -----------------
    .soTHIS_Tcp_Data_TDATA    (ssIPRX_ARS2_Data_tdata),
    .soTHIS_Tcp_Data_TKEEP    (ssIPRX_ARS2_Data_tkeep),
    .soTHIS_Tcp_Data_TLAST    (ssIPRX_ARS2_Data_tlast),
    .soTHIS_Tcp_Data_TVALID   (ssIPRX_ARS2_Data_tvalid),
    .soTHIS_Tcp_Data_TREADY   (ssIPRX_ARS2_Data_tready)

  ); // End of IPRX

`endif //  `ifdef USE_DEPRECATED_DIRECTIVES
 
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==>[ARS0]==> ARP)
  //============================================================================
  AxisRegisterSlice_64 ARS0 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
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
    .aresetn        (~piShlRst),
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
  
    .aclk                           (piShlClk),
    .aresetn                        (~(piShlRst | piMMIO_Layer3Rst)),
  
    //------------------------------------------------------
    //-- IPRX Interfaces (via ARS0)
    //------------------------------------------------------
    //-- IPRX ==> [ARS0] / Data --------
    .axi_arp_slice_to_arp_tdata     (ssARS0_ARP_Data_tdata),
    .axi_arp_slice_to_arp_tkeep     (ssARS0_ARP_Data_tkeep),
    .axi_arp_slice_to_arp_tlast     (ssARS0_ARP_Data_tlast),
    .axi_arp_slice_to_arp_tvalid    (ssARS0_ARP_Data_tvalid),
    .axi_arp_slice_to_arp_tready    (ssARS0_ARP_Data_tready),
   
    //------------------------------------------------------
    //-- IPTX Interfaces
    //------------------------------------------------------
    //-- To   IPTX / LoopkupRequest ----
    .axis_arp_lookup_request_TDATA  (ssIPTX_ARP_LkpReq_tdata),
    .axis_arp_lookup_request_TVALID (ssIPTX_ARP_LkpReq_tvalid),
    .axis_arp_lookup_request_TREADY (ssIPTX_ARP_LkpReq_tready),
    //-- From IPTX / LoopkupReply ------
    .axis_arp_lookup_reply_TDATA    (ssARP_IPTX_LkpRpl_tdata),
    .axis_arp_lookup_reply_TVALID   (ssARP_IPTX_LkpRpl_tvalid),
    .axis_arp_lookup_reply_TREADY   (ssARP_IPTX_LkpRpl_tready),
    
    //------------------------------------------------------
    //-- L2MUX Interfaces
    //------------------------------------------------------
    //-- To   L2MUX / Data  
    .axi_arp_to_arp_slice_tdata     (ssARP_L2MUX_Data_tdata),
    .axi_arp_to_arp_slice_tkeep     (ssARP_L2MUX_Data_tkeep),
    .axi_arp_to_arp_slice_tlast     (ssARP_L2MUX_Data_tlast),
    .axi_arp_to_arp_slice_tvalid    (ssARP_L2MUX_Data_tvalid),
    .axi_arp_to_arp_slice_tready    (ssARP_L2MUX_Data_tready),
    
    .myMacAddress                   (piMMIO_MacAddress),
    .myIpAddress                    (piMMIO_IpAddress)

  ); // End of ARP
  
  //============================================================================
  //  INST: TCP-OFFLOAD-ENGINE
  //============================================================================
  TcpOffloadEngine TOE (
  
    .aclk                      (piShlClk),
    .aresetn                   (~(piShlRst | piMMIO_Layer4Rst)),

    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_IpAddr_V           (piMMIO_IpAddress),
    
    //------------------------------------------------------
    //-- NTS Interfaces
    //------------------------------------------------------    
    .poNTS_Ready_V             (poMMIO_ToeReady),
                        
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
    //-- ROLE / TRIF / TCP RxP Data Flow Interfaces
    //------------------------------------------------------
    //-- To   ROLE / TCP Data Notification
    .soTRIF_Notif_TDATA        (soROL_Tcp_Notif_tdata),
    .soTRIF_Notif_TVALID       (soROL_Tcp_Notif_tvalid),  
    .soTRIF_Notif_TREADY       (soROL_Tcp_Notif_tready),
    //-- From ROLE / TCP Data Read Request
    .siTRIF_DReq_TDATA         (siROL_Tcp_DReq_tdata),
    .siTRIF_DReq_TVALID        (siROL_Tcp_DReq_tvalid),
    .siTRIF_DReq_TREADY        (siROL_Tcp_DReq_tready),
    //-- To   ROLE (via ARS4) / TCP Data Stream
    .soTRIF_Data_TDATA         (ssTOE_ARS4_Tcp_Data_tdata),
    .soTRIF_Data_TKEEP         (ssTOE_ARS4_Tcp_Data_tkeep),
    .soTRIF_Data_TLAST         (ssTOE_ARS4_Tcp_Data_tlast),
    .soTRIF_Data_TVALID        (ssTOE_ARS4_Tcp_Data_tvalid),
    .soTRIF_Data_TREADY        (ssTOE_ARS4_Tcp_Data_tready),
    //-- To   ROLE (via ARS4) / TCP Metadata   _Rol_
    .soTRIF_Meta_TDATA         (ssTOE_ARS4_Tcp_Meta_tdata),
    .soTRIF_Meta_TVALID        (ssTOE_ARS4_Tcp_Meta_tvalid),
    .soTRIF_Meta_TREADY        (ssTOE_ARS4_Tcp_Meta_tready),
    
    //------------------------------------------------------
    //-- ROLE / TRIF / TCP RxP Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From ROLE / TCP Listen Port Request
    .siTRIF_LsnReq_TDATA       (siROL_Tcp_LsnReq_tdata),
    .siTRIF_LsnReq_TVALID      (siROL_Tcp_LsnReq_tvalid),
    .siTRIF_LsnReq_TREADY      (siROL_Tcp_LsnReq_tready),
    //-- To   ROLE / TCP Listen Port Ack
    .soTRIF_LsnAck_TDATA       (soROL_Tcp_LsnAck_tdata),
    .soTRIF_LsnAck_TVALID      (soROL_Tcp_LsnAck_tvalid),
    .soTRIF_LsnAck_TREADY      (soROL_Tcp_LsnAck_tready),    
    
    //------------------------------------------------------
    //-- ROLE / TCP / TCP TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- From ROLE (via ARS5) / TCP Data Stream
    .siTRIF_Data_TDATA         (ssARS5_TOE_Data_tdata),
    .siTRIF_Data_TKEEP         (ssARS5_TOE_Data_tkeep),
    .siTRIF_Data_TLAST         (ssARS5_TOE_Data_tlast),
    .siTRIF_Data_TVALID        (ssARS5_TOE_Data_tvalid),
    .siTRIF_Data_TREADY        (ssARS5_TOE_Data_tready),
    //-- From ROLE (via ARS5) / TCP Metadata
    .siTRIF_Meta_TDATA         (ssARS5_TOE_Meta_tdata),
    .siTRIF_Meta_TVALID        (ssARS5_TOE_Meta_tvalid),
    .siTRIF_Meta_TREADY        (ssARS5_TOE_Meta_tready),
    //-- To  ROLE / TCP Data Write Status
    .soTRIF_DSts_TDATA         (soROL_Tcp_DSts_tdata),
    .soTRIF_DSts_TVALID        (soROL_Tcp_DSts_tvalid),
    .soTRIF_DSts_TREADY        (soROL_Tcp_DSts_tready),
  
    //------------------------------------------------------
    //-- ROLE / TRIF / TCP TxP Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From ROLE / TCP Open Session Request
    .siTRIF_OpnReq_TDATA       (siROL_Tcp_OpnReq_tdata),
    .siTRIF_OpnReq_TVALID      (siROL_Tcp_OpnReq_tvalid),
    .siTRIF_OpnReq_TREADY      (siROL_Tcp_OpnReq_tready),
    //-- To   ROLE / TCP Open Session Reply
    .soTRIF_OpnRep_TREADY      (soROL_Tcp_OpnRep_tready),
    .soTRIF_OpnRep_TDATA       (soROL_Tcp_OpnRep_tdata),
    .soTRIF_OpnRep_TVALID      (soROL_Tcp_OpnRep_tvalid),
    //-- From ROLE / TCP Close Session Request
    .siTRIF_ClsReq_TDATA       (siROL_Tcp_ClsReq_tdata),
    .siTRIF_ClsReq_TVALID      (siROL_Tcp_ClsReq_tvalid),
    .siTRIF_ClsReq_TREADY      (siROL_Tcp_ClsReq_tready),
    //-- To   ROLE / TCP Close Session Status
    // [INFO] Not used    
   
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
    //-- To DEBUG / Session Statistics Interfaces
    //------------------------------------------------------
    .poDBG_SssRelCnt_V         (),
    .poDBG_SssRegCnt_V         (),
    
    //------------------------------------------------------
    //-- To DEBUG / Simulation Counter Interfaces
    //------------------------------------------------------
    .poSimCycCount_V           ()

  );  // End of TOE
  
  //============================================================================
  //  INST: CONTENT ADDRESSABLE MEMORY
  //============================================================================  
`define USE_FAKE_CAM

`ifndef USE_FAKE_CAM
 
  ToeCam CAM (
  
   .piClk                        (piShlClk),
   .piRst_n                      (~(piShlRst | piMMIO_Layer4Rst)),
   
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
 
  ContentAddressableMemory CAM (
    
    .aclk                         (piShlClk),
    .aresetn                      (~(piShlRst | piMMIO_Layer4Rst)),
     
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
    .aresetn        (~piShlRst),
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
    .aresetn        (~piShlRst),
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
  //  INST: AXI4-STREAM REGISTER SLICE (TOE ==>[ARS4]==> ROLE/Tcp/Data)
  //============================================================================
  AxisRegisterSlice_64 ARS4_TcpData ( 
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From TOE / Tcp / Data --------
    .s_axis_tdata   (ssTOE_ARS4_Tcp_Data_tdata),
    .s_axis_tvalid  (ssTOE_ARS4_Tcp_Data_tvalid),
    .s_axis_tkeep   (ssTOE_ARS4_Tcp_Data_tkeep),
    .s_axis_tlast   (ssTOE_ARS4_Tcp_Data_tlast),
    .s_axis_tready  (ssTOE_ARS4_Tcp_Data_tready),
    //-- To   ROLE / Tcp / Data -------
    .m_axis_tdata   (soROL_Tcp_Data_tdata),
    .m_axis_tkeep   (soROL_Tcp_Data_tkeep),
    .m_axis_tlast   (soROL_Tcp_Data_tlast),
    .m_axis_tvalid  (soROL_Tcp_Data_tvalid),
    .m_axis_tready  (soROL_Tcp_Data_tready)
  );
  
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (TOE ==>[ARS4]==> ROLE/Tcp/Meta)
  //============================================================================
  AxisRegisterSlice_16 ARS4_TcpMeta ( 
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From TOE / Tcp / Meta --------
    .s_axis_tdata   (ssTOE_ARS4_Tcp_Meta_tdata),
    .s_axis_tvalid  (ssTOE_ARS4_Tcp_Meta_tvalid),
    .s_axis_tready  (ssTOE_ARS4_Tcp_Meta_tready),
    //-- To   ROLE / Tcp / Meta -------
    .m_axis_tdata   (soROL_Tcp_Meta_tdata),
    .m_axis_tvalid  (soROL_Tcp_Meta_tvalid),
    .m_axis_tready  (soROL_Tcp_Meta_tready)
  );

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (ROLE/Tcp/Data ==>[ARS5]==> TOE)
  //============================================================================
  AxisRegisterSlice_64 ARS5_TcpData (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From ROLE / Tcp / Data -------
    .s_axis_tdata   (siROL_Tcp_Data_tdata),  
    .s_axis_tkeep   (siROL_Tcp_Data_tkeep),
    .s_axis_tlast   (siROL_Tcp_Data_tlast),
    .s_axis_tvalid  (siROL_Tcp_Data_tvalid),  
    .s_axis_tready  (siROL_Tcp_Data_tready),
    //-- To  TOE / Tcp / Data
    .m_axis_tdata   (ssARS5_TOE_Data_tdata),
    .m_axis_tkeep   (ssARS5_TOE_Data_tkeep),
    .m_axis_tlast   (ssARS5_TOE_Data_tlast),
    .m_axis_tvalid  (ssARS5_TOE_Data_tvalid),
    .m_axis_tready  (ssARS5_TOE_Data_tready)
  );
    
 //============================================================================
 //  INST: AXI4-STREAM REGISTER SLICE (ROLE/Tcp/Meta ==>[ARS5]==> TOE)
 //============================================================================
 AxisRegisterSlice_16 ARS5_TcpMeta (
   .aclk           (piShlClk),
   .aresetn        (~piShlRst),
   //-- From ROLE / Tcp / Meta --------
   .s_axis_tdata   (siROL_Tcp_Meta_tdata),
   .s_axis_tvalid  (siROL_Tcp_Meta_tvalid),  
   .s_axis_tready  (siROL_Tcp_Meta_tready),
   //-- To   TOE / Tcp / Meta ---------
   .m_axis_tdata   (ssARS5_TOE_Meta_tdata),
   .m_axis_tvalid  (ssARS5_TOE_Meta_tvalid),
   .m_axis_tready  (ssARS5_TOE_Meta_tready)
 );

  //============================================================================
  //  INST: UDP-CORE-MODULE
  //============================================================================
  UdpCore UDP (
  
    .aclk                             (piShlClk),
    .aresetn                          (~(piShlRst | piMMIO_Layer4Rst)),

    //------------------------------------------------------
    //-- UDMX / UDP TxP Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From UDMX / UDP Open Port Request
    .openPort_TDATA                   (ssUDMX_UDP_OpnReq_tdata),
    .openPort_TVALID                  (ssUDMX_UDP_OpnReq_tvalid),
    .openPort_TREADY                  (ssUDMX_UDP_OpnReq_tready),
    //-- To  UDMX / UDP Open Port Status
    .confirmPortStatus_TDATA          (ssUDP_UDMX_OpnSts_tdata),
    .confirmPortStatus_TVALID         (ssUDP_UDMX_OpnSts_tvalid),
    .confirmPortStatus_TREADY         (ssUDP_UDMX_OpnSts_tready),
 
    //------------------------------------------------------
    //-- IPRX / UDP TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- From IPRX / UDP Data Stream
    .inputPathInData_TDATA            (ssIPRX_UDP_Data_tdata),
    .inputPathInData_TKEEP            (ssIPRX_UDP_Data_tkeep),
    .inputPathInData_TLAST            (ssIPRX_UDP_Data_tlast),
    .inputPathInData_TVALID           (ssIPRX_UDP_Data_tvalid),
    .inputPathInData_TREADY           (ssIPRX_UDP_Data_tready),
     
     //------------------------------------------------------
     //-- UDMX / UDP TxP Data Flow Interfaces
     //------------------------------------------------------ 
    //-- From UDMX / UDP Data Stream
    .outputPathInData_TDATA           (ssUDMX_UDP_Data_tdata),
    .outputPathInData_TKEEP           (ssUDMX_UDP_Data_tkeep),
    .outputPathInData_TLAST           (ssUDMX_UDP_Data_tlast),
    .outputPathInData_TVALID          (ssUDMX_UDP_Data_tvalid),
    .outputPathInData_TREADY          (ssUDMX_UDP_Data_tready),
    //-- From UDMX / UDP Metadata
    .outputPathInMetadata_TDATA       (ssUDMX_UDP_Meta_tdata),
    .outputPathInMetadata_TVALID      (ssUDMX_UDP_Meta_tvalid),
    .outputPathInMetadata_TREADY      (ssUDMX_UDP_Meta_tready),
    //-- From UDMX / UDP Length
    .outputpathInLength_TDATA         (ssUDMX_UDP_PLen_tdata),
    .outputpathInLength_TVALID        (ssUDMX_UDP_PLen_tvalid),
    .outputpathInLength_TREADY        (ssUDMX_UDP_PLen_tready),
   
    //------------------------------------------------------
    //-- UDMX / UDP RxP Data Flow Interfaces
    //------------------------------------------------------
    //-- To   UDMX / UDP Data Stream ---
    .inputpathOutData_TDATA           (ssUDP_UDMX_Data_tdata), 
    .inputpathOutData_TKEEP           (ssUDP_UDMX_Data_tkeep),
    .inputpathOutData_TLAST           (ssUDP_UDMX_Data_tlast),
    .inputpathOutData_TVALID          (ssUDP_UDMX_Data_tvalid),
    .inputpathOutData_TREADY          (ssUDP_UDMX_Data_tready),
    //-- To   UDMX / MetaData ----------
    .inputPathOutputMetadata_TDATA    (ssUDP_UDMX_Meta_tdata),
    .inputPathOutputMetadata_TVALID   (ssUDP_UDMX_Meta_tvalid),
    .inputPathOutputMetadata_TREADY   (ssUDP_UDMX_Meta_tready),

    //------------------------------------------------------
    //-- L3MUX / UDP TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- To   L3MUX / UDP Data Stream
    .outputPathOutData_TDATA          (ssUDP_L3MUX_Data_tdata),
    .outputPathOutData_TKEEP          (ssUDP_L3MUX_Data_tkeep),
    .outputPathOutData_TLAST          (ssUDP_L3MUX_Data_tlast),
    .outputPathOutData_TVALID         (ssUDP_L3MUX_Data_tvalid),
    .outputPathOutData_TREADY         (ssUDP_L3MUX_Data_tready),
    
    //------------------------------------------------------
    //-- ICMP / UDP Ctrl Interfaces
    //------------------------------------------------------
    //-- To   UDP / Input Not Reachable
    .inputPathPortUnreachable_TDATA   (ssUDP_ICMP_Data_tdata),
    .inputPathPortUnreachable_TKEEP   (ssUDP_ICMP_Data_tkeep),
    .inputPathPortUnreachable_TLAST   (ssUDP_ICMP_Data_tlast),
    .inputPathPortUnreachable_TVALID  (ssUDP_ICMP_Data_tvalid),
    .inputPathPortUnreachable_TREADY  (ssUDP_ICMP_Data_tready),
    
    //-- Unused Interface ----------------------------------
    .portRelease_TDATA                (15'b0),          
    .portRelease_TVALID               (1'b0),         
    .portRelease_TREADY               ()     
                          
  );
  
  //============================================================================
  //  INST: UDP MULTIPLEXER
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES
  
  UdpMultiplexer UDMX (  // Deprecated version
    
    .aclk                         (piShlClk),                                                  
    .aresetn                      (~(piShlRst | piMMIO_Layer4Rst)),

    //------------------------------------------------------
    //-- DHCP / UDP Ctrl Interfaces
    //------------------------------------------------------
    //-- From DHCP / Open Port Request
    .siDHCP_This_OpnReq_TDATA     (ssDHCP_UDMX_OpnReq_tdata),
    .siDHCP_This_OpnReq_TVALID    (ssDHCP_UDMX_OpnReq_tvalid),
    .siDHCP_This_OpnReq_TREADY    (ssDHCP_UDMX_OpnReq_tready),
    //-- To   DHCP / Open Port Acknowledgment
    .soTHIS_Dhcp_OpnAck_TDATA     (ssUDMX_DHCP_OpnAck_tdata),
    .soTHIS_Dhcp_OpnAck_TVALID    (ssUDMX_DHCP_OpnAck_tvalid),
    .soTHIS_Dhcp_OpnAck_TREADY    (ssUDMX_DHCP_OpnAck_tready),

    //------------------------------------------------------
    //-- DHCP / Data & MetaData Interfaces
    //------------------------------------------------------               
    //-- From DHCP / Data
    .siDHCP_This_Data_TDATA       (ssDHCP_UDMX_Data_tdata),          
    .siDHCP_This_Data_TKEEP       (ssDHCP_UDMX_Data_tkeep),      
    .siDHCP_This_Data_TLAST       (ssDHCP_UDMX_Data_tlast),            
    .siDHCP_This_Data_TVALID      (ssDHCP_UDMX_Data_tvalid),
    .siDHCP_This_Data_TREADY      (ssDHCP_UDMX_Data_tready),
    //-- From DHCP / TMetaData
    .siDHCP_This_Meta_TDATA       (ssDHCP_UDMX_Meta_tdata),
    .siDHCP_This_Meta_TVALID      (ssDHCP_UDMX_Meta_tvalid),
    .siDHCP_This_Meta_TREADY      (ssDHCP_UDMX_Meta_tready),
    //-- From DHCP / TxLen
    .siDHCP_This_PLen_TDATA       (ssDHCP_UDMX_PLen_tdata),
    .siDHCP_This_PLen_TVALID      (ssDHCP_UDMX_PLen_tvalid),
    .siDHCP_This_PLen_TREADY      (ssDHCP_UDMX_PLen_tready),
    //-- To   DHCP / Data
    .soTHIS_Dhcp_Data_TDATA       (ssUDMX_DHCP_Data_tdata),
    .soTHIS_Dhcp_Data_TKEEP       (ssUDMX_DHCP_Data_tkeep),
    .soTHIS_Dhcp_Data_TLAST       (ssUDMX_DHCP_Data_tlast),
    .soTHIS_Dhcp_Data_TVALID      (ssUDMX_DHCP_Data_tvalid),
    .soTHIS_Dhcp_Data_TREADY      (ssUDMX_DHCP_Data_tready),     
    //-- To   DHCP MetaData
    .soTHIS_Dhcp_Meta_TDATA       (ssUDMX_DHCP_Meta_tdata),
    .soTHIS_Dhcp_Meta_TVALID      (ssUDMX_DHCP_Meta_tvalid),
    .soTHIS_Dhcp_Meta_TREADY      (ssUDMX_DHCP_Meta_tready),

    //------------------------------------------------------
    //-- UDP / Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From UDP / Open Port Acknowledgment
    .siUDP_This_OpnAck_TDATA      (ssUDP_UDMX_OpnSts_tdata),
    .siUDP_This_OpnAck_TVALID     (ssUDP_UDMX_OpnSts_tvalid),
    .siUDP_This_OpnAck_TREADY     (ssUDP_UDMX_OpnSts_tready),        
    //-- To   UDP / Open Port Request
    .soTHIS_Udp_OpnReq_TDATA      (ssUDMX_UDP_OpnReq_tdata),
    .soTHIS_Udp_OpnReq_TVALID     (ssUDMX_UDP_OpnReq_tvalid),
    .soTHIS_Udp_OpnReq_TREADY     (ssUDMX_UDP_OpnReq_tready),

    //------------------------------------------------------
    //-- UDP / RxP Data Flow Interfaces
    //------------------------------------------------------                      
    //-- From UDP / Data
    .siUDP_This_Data_TDATA        (ssUDP_UDMX_Data_tdata),
    .siUDP_This_Data_TKEEP        (ssUDP_UDMX_Data_tkeep),
    .siUDP_This_Data_TLAST        (ssUDP_UDMX_Data_tlast),
    .siUDP_This_Data_TVALID       (ssUDP_UDMX_Data_tvalid),
    .siUDP_This_Data_TREADY       (ssUDP_UDMX_Data_tready),
    //-- From UDP / MetaData
    .siUDP_This_Meta_TDATA        (ssUDP_UDMX_Meta_tdata),
    .siUDP_This_Meta_TVALID       (ssUDP_UDMX_Meta_tvalid),
    .siUDP_This_Meta_TREADY       (ssUDP_UDMX_Meta_tready),
    //-- To   UDP / Data
    .soTHIS_Udp_Data_TDATA        (ssUDMX_UDP_Data_tdata),
    .soTHIS_Udp_Data_TKEEP        (ssUDMX_UDP_Data_tkeep),
    .soTHIS_Udp_Data_TLAST        (ssUDMX_UDP_Data_tlast),
    .soTHIS_Udp_Data_TVALID       (ssUDMX_UDP_Data_tvalid),
    .soTHIS_Udp_Data_TREADY       (ssUDMX_UDP_Data_tready),
    //-- To   UDP / MetaData
    .soTHIS_Udp_Meta_TDATA        (ssUDMX_UDP_Meta_tdata),
    .soTHIS_Udp_Meta_TVALID       (ssUDMX_UDP_Meta_tvalid),
    .soTHIS_Udp_Meta_TREADY       (ssUDMX_UDP_Meta_tready),
    //-- To   UDP / Tx Length
    .soTHIS_Udp_PLen_TDATA        (ssUDMX_UDP_PLen_tdata),
    .soTHIS_Udp_PLen_TVALID       (ssUDMX_UDP_PLen_tvalid),
    .soTHIS_Udp_PLen_TREADY       (ssUDMX_UDP_PLen_tready),

    //------------------------------------------------------
    //-- URIF / Ctrl Flow Interfaces
    //------------------------------------------------------
    //-- From URIF / Open Port Request
    .siURIF_This_OpnReq_TDATA     (siROL_Udp_OpnReq_tdata),
    .siURIF_This_OpnReq_TVALID    (siROL_Udp_OpnReq_tvalid),
    .siURIF_This_OpnReq_TREADY    (siROL_Udp_OpnReq_tready),
    //-- To   URIF / Open Port Acknowledgment
    .soTHIS_Urif_OpnAck_TDATA     (soROL_Udp_OpnAck_tdata),
    .soTHIS_Urif_OpnAck_TVALID    (soROL_Udp_OpnAck_tvalid),
    .soTHIS_Urif_OpnAck_TREADY    (soROL_Udp_OpnAck_tready),

    //------------------------------------------------------
    //-- URIF / TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- From URIF / Data
    .siURIF_This_Data_TDATA       (siROL_Udp_Data_tdata),
    .siURIF_This_Data_TKEEP       (siROL_Udp_Data_tkeep),
    .siURIF_This_Data_TLAST       (siROL_Udp_Data_tlast),
    .siURIF_This_Data_TVALID      (siROL_Udp_Data_tvalid),
    .siURIF_This_Data_TREADY      (siROL_Udp_Data_tready),
    //-- From URIF / MetaData
    .siURIF_This_Meta_TDATA       (siROL_Udp_Meta_tdata),
    .siURIF_This_Meta_TVALID      (siROL_Udp_Meta_tvalid),
    .siURIF_This_Meta_TREADY      (siROL_Udp_Meta_tready),
    //-- From URIF / Length
    .siURIF_This_PLen_TDATA       (siROL_Udp_PLen_tdata),
    .siURIF_This_PLen_TVALID      (siROL_Udp_PLen_tvalid),
    .siURIF_This_PLen_TREADY      (siROL_Udp_PLen_tready),
    //-- To   URIF / Data
    .soTHIS_Urif_Data_TDATA       (soROL_Udp_Data_tdata),
    .soTHIS_Urif_Data_TKEEP       (soROL_Udp_Data_tkeep),
    .soTHIS_Urif_Data_TLAST       (soROL_Udp_Data_tlast),
    .soTHIS_Urif_Data_TVALID      (soROL_Udp_Data_tvalid),
    .soTHIS_Urif_Data_TREADY      (soROL_Udp_Data_tready),
    //-- To   URIF / Meta
    .soTHIS_Urif_Meta_TDATA       (soROL_Udp_Meta_tdata),
    .soTHIS_Urif_Meta_TVALID      (soROL_Udp_Meta_tvalid),
    .soTHIS_Urif_Meta_TREADY      (soROL_Udp_Meta_tready)
  );

`else
  
    UdpMultiplexer UDMX (
    
    .ap_clk                       (piShlClk),
    .ap_rst_n                     (~(piShlRst | piMMIO_Layer3Rst)),

    //------------------------------------------------------
    //-- From DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- DHCP / OpenPortRequest
    .siDHCP_This_OpnReq_V_V_TDATA (sDHCP_Udmx_OpnReq_Axis_tdata),
    .siDHCP_This_OpnReq_V_V_TVALID(sDHCP_Udmx_OpnReq_Axis_tvalid),
    .siDHCP_This_OpnReq_V_V_TREADY(sDHCP_Udmx_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / OpenPortAck
    .soTHIS_Dhcp_OpnAck_V_TDATA   (sUDMX_Dhcp_OpnAck_Axis_tdata),
    .soTHIS_Dhcp_OpnAck_V_TVALID  (sUDMX_Dhcp_OpnAck_Axis_tvalid),
    .soTHIS_Dhcp_OpnAck_V_TREADY  (sUDMX_Dhcp_OpnAck_Axis_tready),

    //------------------------------------------------------
    //-- From DHCP / Data & MetaData Interfaces
    //------------------------------------------------------               
    //-- DHCP / This / Data
    .siDHCP_This_Data_TDATA       (sDHCP_Udmx_Data_Axis_tdata),          
    .siDHCP_This_Data_TKEEP       (sDHCP_Udmx_Data_Axis_tkeep),      
    .siDHCP_This_Data_TLAST       (sDHCP_Udmx_Data_Axis_tlast),            
    .siDHCP_This_Data_TVALID      (sDHCP_Udmx_Data_Axis_tvalid),
    .siDHCP_This_Data_TREADY      (sDHCP_Udmx_Data_Axis_tready),
    //-- DHCP / This / MetaData
    .siDHCP_This_Meta_TDATA       (sDHCP_Udmx_Meta_Axis_tdata),
    .siDHCP_This_Meta_TVALID      (sDHCP_Udmx_Meta_Axis_tvalid),
    .siDHCP_This_Meta_TREADY      (ssDHCP_UdmxMeta_Axis_tready),
    //-- DHCP / This / TxLen
    .siDHCP_This_PLen_V_V_TDATA   (sDHCP_Udmx_PLen_Axis_tdata),
    .siDHCP_This_PLen_V_V_TVALID  (sDHCP_Udmx_PLen_Axis_tvalid),
    .siDHCP_This_PLen_V_V_TREADY  (sDHCP_Udmx_PLen_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP Interfaces / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / Data
    .soTHIS_Dhcp_Data_TDATA       (sUDMX_Dhcp_Data_Axis_tdata),
    .soTHIS_Dhcp_Data_TKEEP       (sUDMX_Dhcp_Data_Axis_tkeep),
    .soTHIS_Dhcp_Data_TLAST       (sUDMX_Dhcp_Data_Axis_tlast),
    .soTHIS_Dhcp_Data_TVALID      (sUDMX_Dhcp_Data_Axis_tvalid),
    .soTHIS_Dhcp_Data_TREADY      (sUDMX_Dhcp_Data_Axis_tready),                
    //-- THIS / Dhcp / MetaData
    .soTHIS_Dhcp_Meta_TDATA       (sUDMX_Dhcp_Meta_Axis_tdata),
    .soTHIS_Dhcp_Meta_TVALID      (sUDMX_Dhcp_Meta_Axis_tvalid),
    .soTHIS_Dhcp_Meta_TREADY      (sUDMX_Dhcp_Meta_Axis_tready),

    //------------------------------------------------------
    //-- From UDP / Open-Port Interfaces
    //------------------------------------------------------
    //-- UDP / This / OpenPortAck
    .siUDP_This_OpnAck_V_TDATA    (sUDP_Udmx_OpnSts_Axis_tdata),
    .siUDP_This_OpnAck_V_TVALID   (sUDP_Udmx_OpnSts_Axis_tvalid),
    .siUDP_This_OpnAck_V_TREADY   (sUDP_Udmx_OpnSts_Axis_tready),

    //------------------------------------------------------
    //-- To UDP   / Open-Port Interfaces
    //------------------------------------------------------                
    //-- THIS / Udp / OpenPortRequest
    .soTHIS_Udp_OpnReq_V_V_TDATA  (sUDMX_Udp_OpnReq_Axis_tdata),
    .soTHIS_Udp_OpnReq_V_V_TVALID (sUDMX_Udp_OpnReq_Axis_tvalid),
    .soTHIS_Udp_OpnReq_V_V_TREADY (sUDMX_Udp_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- From UDP / Data & MetaData Interfaces
    //------------------------------------------------------                      
    //-- UDP / This / Data
    .siUDP_This_Data_TDATA        (sUDP_Udmx_Data_Axis_tdata),
    .siUDP_This_Data_TKEEP        (sUDP_Udmx_Data_Axis_tkeep),
    .siUDP_This_Data_TLAST        (sUDP_Udmx_Data_Axis_tlast),
    .siUDP_This_Data_TVALID       (sUDP_Udmx_Data_Axis_tvalid),
    .siUDP_This_Data_TREADY       (sUDP_Udmx_Data_Axis_tready),
    //-- UDP / This / MetaData
    .siUDP_This_Meta_TDATA        (sUDP_Udmx_Meta_Axis_tdata),
    .siUDP_This_Meta_TVALID       (sUDP_Udmx_Meta_Axis_tvalid),
    .siUDP_This_Meta_TREADY       (sUDP_Udmx Meta_Axis_tready),
    
    //------------------------------------------------------
    //-- To UDP   /  Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Udp / Data
    .soTHIS_Udp_Data_TDATA        (sUDMX_Udp_Data_Axis_tdata),
    .soTHIS_Udp_Data_TKEEP        (sUDMX_Udp_Data_Axis_tkeep),
    .soTHIS_Udp_Data_TLAST        (sUDMX_Udp_Data_Axis_tlast),
    .soTHIS_Udp_Data_TVALID       (sUDMX_Udp_Data_Axis_tvalid),
    .soTHIS_Udp_Data_TREADY       (sUDMX_Udp_Data_Axis_tready),
    //-- THIS / Udp / MetaData 
    .soTHIS_Udp_Meta_TDATA        (sUDMX_Udp_Meta_Axis_tdata),
    .soTHIS_Udp_Meta_TVALID       (sUDMX_Udp_Meta_Axis_tvalid),
    .soTHIS_Udp_Meta_TREADY       (sUDMX_Udp_Meta_Axis_tready),
    //-- THIS / Udp / TxLength
    .soTHIS_Udp_PLen_V_V_TDATA    (sUDMX_Udp_TxLen_Axis_tdata),
    .soTHIS_Udp_PLen_V_V_TVALID   (sUDMX_Udp_TxLen_Axis_tvalid),
    .soTHIS_Udp_PLen_V_V_TREADY   (sUDMX_Udp_TxLen_Axis_tready),

    //------------------------------------------------------
    //-- From URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- URIF / This / OpenPortRequest
    .siURIF_This_OpnReq_V_V_TDATA (siROL_Udp_OpnReq_tdata),
    .siURIF_This_OpnReq_V_V_TVALID(siROL_Udp_OpnReq_tvalid),
    .siURIF_This_OpnReq_V_V_TREADY(siROL_Udp_OpnReq_tready),

    //------------------------------------------------------
    //-- To   URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / OpenPortStatus
    .soTHIS_Urif_OpnAck_V_TREADY  (soROL_Udp_OpnAck_tready),
    .soTHIS_Urif_OpnAck_V_TDATA   (soROL_Udp_OpnAck_tdata),
    .soTHIS_Urif_OpnAck_V_TVALID  (soROL_Udp_OpnAck_tvalid),

    //------------------------------------------------------
    //-- From URIF / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- URIF / This / Data 
    .siURIF_This_Data_TDATA       (siROL_Udp_Data_tdata),           
    .siURIF_This_Data_TKEEP       (siROL_Udp_Data_tkeep),      
    .siURIF_This_Data_TLAST       (siROL_Udp_Data_tlast),
    .siURIF_This_Data_TVALID      (siROL_Udp_Data_tvalid),
    .siURIF_This_Data_TREADY      (siROL_Udp_Data_tready),
    //-- URIF / This / MetaData 
    .siURIF_This_Meta_TDATA       (siROL_Udp_Meta_tdata),
    .siURIF_This_Meta_TVALID      (siROL_Udp_Meta_tvalid),     
    .siURIF_This_Meta_TREADY      (siROL_Udp_Meta_tready),
    //-- URIF /This / TxLn
    .siURIF_This_PLen_V_V_TDATA   (siROL_Udp_PLen_tdata),
    .siURIF_This_PLen_V_V_TVALID  (siROL_Udp_PLen_tvalid),
    .siURIF_This_PLen_V_V_TREADY  (siROL_Udp_PLen_tready),
                       
    //------------------------------------------------------
    //-- To URIF / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / Data / Output AXI-Write Stream Interface
    .soTHIS_Urif_Data_TREADY      (soROL_Udp_Data_tready),
    .soTHIS_Urif_Data_TDATA       (soROL_Udp_Data_tdata),
    .soTHIS_Urif_Data_TKEEP       (soROL_Udp_Data_tkeep),
    .soTHIS_Urif_Data_TLAST       (soROL_Udp_Data_tlast),
    .soTHIS_Urif_Data_TVALID      (soROL_Udp_Data_tvalid),
    //-- THIS / Urif / Meta / Output AXI-Write Stream Interface
    .soTHIS_Urif_Meta_TREADY      (soROL_Udp_Meta_tready),
    .soTHIS_Urif_Meta_TDATA       (soROL_Udp_Meta_tdata),
    .soTHIS_Urif_Meta_TVALID      (soROL_Udp_Meta_tvalid)
  );

`endif // !`ifdef USE_DEPRECATED_DIRECTIVES
   
  //============================================================================
  //  INST: DHCP-CLIENT -- [TOOD - Remove this useless DHCP-client module]
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES

  DynamicHostConfigurationProcess DHCP (
  
    .aclk                           (piShlClk),                      
    .aresetn                        (~piShlRst),

    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_This_Enable_V           (1'b0),
    .piMMIO_This_MacAddress_V       (piMMIO_MacAddress),
    
    //------------------------------------------------------
    //-- NTS IPv4 Interfaces
    //------------------------------------------------------
    .poTHIS_Nts_IpAddress_V         (),  // [INFO - Do not connect because we don't use DHCP]
    
    //------------------------------------------------------
    //-- UDMX / Open-Port Interfaces
    //------------------------------------------------------
    //-- From UDMX / OpnAck ------------
    .siUDMX_This_OpnAck_TDATA       (ssUDMX_DHCP_OpnAck_tdata),
    .siUDMX_This_OpnAck_TVALID      (ssUDMX_DHCP_OpnAck_tvalid), 
    .siUDMX_This_OpnAck_TREADY      (ssUDMX_DHCP_OpnAck_tready),    
    //-- To   UDMX / OpnReq ------------
    .soTHIS_Udmx_OpnReq_TDATA       (ssDHCP_UDMX_OpnReq_tdata),
    .soTHIS_Udmx_OpnReq_TVALID      (ssDHCP_UDMX_OpnReq_tvalid),
    .soTHIS_Udmx_OpnReq_TREADY      (ssDHCP_UDMX_OpnReq_tready),
    //-- From UDMX / Data --------------            
    .siUDMX_This_Data_TDATA         (ssUDMX_DHCP_Data_tdata),
    .siUDMX_This_Data_TKEEP         (ssUDMX_DHCP_Data_tkeep),
    .siUDMX_This_Data_TLAST         (ssUDMX_DHCP_Data_tlast),
    .siUDMX_This_Data_TVALID        (ssUDMX_DHCP_Data_tvalid),
    .siUDMX_This_Data_TREADY        (ssUDMX_DHCP_Data_tready),
    //-- From UDMX / MetaData ----------
    .siUDMX_This_Meta_TDATA         (ssUDMX_DHCP_Meta_tdata),
    .siUDMX_This_Meta_TVALID        (ssUDMX_DHCP_Meta_tvalid),
    .siUDMX_This_Meta_TREADY        (ssUDMX_DHCP_Meta_tready),
     //-- To   UDMX / Data -------------
    .soTHIS_Udmx_Data_TDATA         (ssDHCP_UDMX_Data_tdata),
    .soTHIS_Udmx_Data_TKEEP         (ssDHCP_UDMX_Data_tkeep),
    .soTHIS_Udmx_Data_TLAST         (ssDHCP_UDMX_Data_tlast),
    .soTHIS_Udmx_Data_TVALID        (ssDHCP_UDMX_Data_tvalid),
    .soTHIS_Udmx_Data_TREADY        (ssDHCP_UDMX_Data_tready),
    //-- To   UDMX / MetaData ----------
    .soTHIS_Udmx_Meta_TDATA         (ssDHCP_UDMX_Meta_tdata),
    .soTHIS_Udmx_Meta_TVALID        (ssDHCP_UDMX_Meta_tvalid),
    .soTHIS_Udmx_Meta_TREADY        (ssDHCP_UDMX_Meta_tready),
    //-- To   UDMX / TxLength ----------
    .soTHIS_Udmx_PLen_TDATA         (ssDHCP_UDMX_PLen_tdata),
    .soTHIS_Udmx_PLen_TVALID        (ssDHCP_UDMX_PLen_tvalid),
    .soTHIS_Udmx_PLen_TREADY        (ssDHCP_UDMX_PLen_tready)
   
  ); // End of DHCP

`else // !`ifdef USE_DEPRECATED_DIRECTIVES
   
  DynamicHostConfigurationProcess DHCP (
  
    .ap_clk                         (piShlClk),                      
    .ap_rst_n                       (~piShlRst),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_This_Enable_V           (1'b0),
    .piMMIO_This_MacAddress_V       (piMMIO_Nts0_MacAddress),
    
    //------------------------------------------------------
    //-- To NTS IPv4 Interfaces
    //------------------------------------------------------
    .poTHIS_Nts_IpAddress_V         (),  // [INFO - Do not connect because we don't use DHCP]
    
    //------------------------------------------------------
    //-- From UDMX / Open-Port Interfaces
    //------------------------------------------------------
    //-- UDMX / This / OpenPortStatus / Axis
    .siUDMX_This_OpnAck_V_TDATA     (sUDMX_Dhcp_OpnAck_Axis_tdata),
    .siUDMX_This_OpnAck_V_TVALID    (sUDMX_Dhcp_OpnAck_Axis_tvalid), 
    .siUDMX_This_OpnAck_V_TREADY    (sDHCP_Udmx_OpnAck_Axis_tready),
    
    //------------------------------------------------------
    //-- To UDMX / Open-Port Interfaces
    //------------------------------------------------------     
    //-- THIS / Udmx / OpenPortRequest / Axis
    .soTHIS_Udmx_OpnReq_V_V_TREADY  (sUDMX_Dhcp_OpnReq_Axis_tready),
    .soTHIS_Udmx_OpnReq_V_V_TDATA   (sDHCP_Udmx_OpnReq_Axis_tdata),
    .soTHIS_Udmx_OpnReq_V_V_TVALID  (sDHCP_Udmx_OpnReq_Axis_tvalid),
     
    //------------------------------------------------------
    //-- From UDMX / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- UDMX / This / Data / Axis            
    .siUDMX_This_Data_TDATA         (sUDMX_Dhcp_Data_Axis_tdata),
    .siUDMX_This_Data_TKEEP         (sUDMX_Dhcp_Data_Axis_tkeep),
    .siUDMX_This_Data_TLAST         (sUDMX_Dhcp_Data_Axis_tlast),
    .siUDMX_This_Data_TVALID        (sUDMX_Dhcp_Data_Axis_tvalid),
    .siUDMX_This_Data_TREADY        (sDHCP_Udmx_Data_Axis_tready),
    //-- UDMX / This / MetaData / Axis
    .siUDMX_This_Meta_TDATA         (sUDMX_Dhcp_Meta_Axis_tdata),
    .siUDMX_This_Meta_TVALID        (sUDMX_Dhcp_Meta_Axis_tvalid),
    .siUDMX_This_Meta_TREADY        (sDHCP_Udmx_Meta_Axis_tready),
       
    //------------------------------------------------------
    //-- To UDMX / Data & MetaData Interfaces
    //------------------------------------------------------     
     //-- THIS / Udmx / Data / Axis
    .soTHIS_Udmx_Data_TREADY        (sUDMX_Dhcp_Data_Axis_tready),
    .soTHIS_Udmx_Data_TDATA         (sDHCP_Udmx_Data_Axis_tdata),                             
    .soTHIS_Udmx_Data_TKEEP         (sDHCP_Udmx_Data_Axis_tkeep),                             
    .soTHIS_Udmx_Data_TLAST         (sDHCP_Udmx_Data_Axis_tlast),  
    .soTHIS_Udmx_Data_TVALID        (sDHCP_Udmx_Data_Axis_tvalid),
    //-- THIS / Udmx / MetaData / Axis
    .soTHIS_Udmx_Meta_TREADY        (sUDMX_Dhcp_Meta_Axis_tready),
    .soTHIS_Udmx_Meta_TDATA         (sDHCP_Udmx_Meta_Axis_tdata),
    .soTHIS_Udmx_Meta_TVALID        (sDHCP_Udmx_Meta_Axis_tvalid),
    //-- THIS / Udmx / TxLength / Axis
    .soTHIS_Udmx_PLen_V_V_TREADY    (sUDMX_Dhcp_PLen_Axis_tready),
    .soTHIS_Udmx_PLen_V_V_TDATA     (sDHCP_Udmx_PLen_Axis_tdata),
    .soTHIS_Udmx_PLen_V_V_TVALID    (sDHCP_Udmx_PLen_Axis_tvalid)
   
  ); // End of DHCP

`endif // `ifdef USE_DEPRECATED_DIRECTIVES
   
  //============================================================================
  //  INST: ICMP-SERVER
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES

  InternetControlMessageProcess ICMP (
                    
    //------------------------------------------------------
    //-- From SHELL Interfaces
    //------------------------------------------------------
    //-- Global Clock & Reset
    .aclk                     (piShlClk),
    .aresetn                  (~(piShlRst | piMMIO_Layer3Rst)),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_This_IpAddr_V (piMMIO_IpAddress),
  
    //------------------------------------------------------
    //-- IPRX Interfaces
    //------------------------------------------------------
    //-- From IPRX==>[ARS1] / Data -----
    .s_dataIn_TDATA     (ssARS1_ICMP_Data_tdata),
    .s_dataIn_TKEEP     (ssARS1_ICMP_Data_tkeep),
    .s_dataIn_TLAST     (ssARS1_ICMP_Data_tlast),
    .s_dataIn_TVALID    (ssARS1_ICMP_Data_tvalid),
    .s_dataIn_TREADY    (ssARS1_ICMP_Data_tready),
    //-- To   IPRX / Ttl --------------------
    .s_ttlIn_TDATA      (ssIPRX_ICMP_Ttl_tdata),
    .s_ttlIn_TKEEP      (ssIPRX_ICMP_Ttl_tkeep),
    .s_ttlIn_TLAST      (ssIPRX_ICMP_Ttl_tlast),
    .s_ttlIn_TVALID     (ssIPRX_ICMP_Ttl_tvalid),
    .s_ttlIn_TREADY     (ssIPRX_ICMP_Ttl_tready),
    
    //------------------------------------------------------
    //-- UDP Interfaces
    //------------------------------------------------------
    //-- From UDP / Data   
    .s_udpIn_TDATA      (ssUDP_ICMP_Data_tdata),
    .s_udpIn_TKEEP      (ssUDP_ICMP_Data_tkeep),
    .s_udpIn_TLAST      (ssUDP_ICMP_Data_tlast),
    .s_udpIn_TVALID     (ssUDP_ICMP_Data_tvalid),
    .s_udpIn_TREADY     (ssUDP_ICMP_Data_tready),
    
    //------------------------------------------------------
    //-- L3MUX Interfaces
    //------------------------------------------------------
    //-- To   L3MUX / Data -------------
    .m_dataOut_TDATA    (ssICMP_L3MUX_Data_tdata),
    .m_dataOut_TKEEP    (ssICMP_L3MUX_Data_tkeep),
    .m_dataOut_TLAST    (ssICMP_L3MUX_Data_tlast),
    .m_dataOut_TVALID   (ssICMP_L3MUX_Data_tvalid),
    .m_dataOut_TREADY   (ssICMP_L3MUX_Data_tready)

  ); // End of ICMP

`endif // `ifdef USE_DEPRECATED_DIRECTIVES
   
   
  //============================================================================
  //  INST: L3MUX AXI4-STREAM INTERCONNECT RTL (Muxes ICMP, TCP, and UDP)
  //============================================================================
  AxisInterconnectRtl_3S1M_D8 L3MUX (
   
    .ACLK               (piShlClk),                         
    .ARESETN            (~piShlRst),           
 
    .S00_AXIS_ACLK      (piShlClk),
    .S01_AXIS_ACLK      (piShlClk),            
    .S02_AXIS_ACLK      (piShlClk),        
 
    .S00_AXIS_ARESETN   (~piShlRst),       
    .S01_AXIS_ARESETN   (~piShlRst),       
    .S02_AXIS_ARESETN   (~piShlRst),     
 
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
    .S01_AXIS_TDATA     (ssUDP_L3MUX_Data_tdata), 
    .S01_AXIS_TKEEP     (ssUDP_L3MUX_Data_tkeep),
    .S01_AXIS_TLAST     (ssUDP_L3MUX_Data_tlast),
    .S01_AXIS_TVALID    (ssUDP_L3MUX_Data_tvalid),
    .S01_AXIS_TREADY    (ssUDP_L3MUX_Data_tready),
    
    //------------------------------------------------------
    //-- From TOE Interfaces (via [ARS3])
    //------------------------------------------------------
    .S02_AXIS_TDATA     (ssARS3_L3MUX_Data_tdata),
    .S02_AXIS_TKEEP     (ssARS3_L3MUX_Data_tkeep),
    .S02_AXIS_TLAST     (ssARS3_L3MUX_Data_tlast),
    .S02_AXIS_TVALID    (ssARS3_L3MUX_Data_tvalid),
    .S02_AXIS_TREADY    (ssARS3_L3MUX_Data_tready),
         
             
    .M00_AXIS_ACLK      (piShlClk),        
    .M00_AXIS_ARESETN   (~piShlRst),    
 
    //------------------------------------------------------
    //-- To IPTX Interfaces
    //------------------------------------------------------
    .M00_AXIS_TDATA     (ssL3MUX_IPTX_Data_tdata),
    .M00_AXIS_TKEEP     (ssL3MUX_IPTX_Data_tkeep),
    .M00_AXIS_TLAST     (ssL3MUX_IPTX_Data_tlast),
    .M00_AXIS_TVALID    (ssL3MUX_IPTX_Data_tvalid),
    .M00_AXIS_TREADY    (ssL3MUX_IPTX_Data_tready),

    .S00_ARB_REQ_SUPPRESS(1'b0),  
    .S01_ARB_REQ_SUPPRESS(1'b0),
    .S02_ARB_REQ_SUPPRESS(1'b0)  
  );
  
  //============================================================================
  //  INST: IP TX HANDLER
  //============================================================================
  IpTxHandler IPTX (
  
    .aclk                     (piShlClk),         
    .aresetn                  (~piShlRst),  
  
    //------------------------------------------------------
    //-- L3MUX Interfaces
    //------------------------------------------------------
    //-- From L3MUX / Data -------------
    .s_dataIn_TDATA           (ssL3MUX_IPTX_Data_tdata),
    .s_dataIn_TKEEP           (ssL3MUX_IPTX_Data_tkeep),
    .s_dataIn_TLAST           (ssL3MUX_IPTX_Data_tlast),
    .s_dataIn_TVALID          (ssL3MUX_IPTX_Data_tvalid),
    .s_dataIn_TREADY          (ssL3MUX_IPTX_Data_tready),
  
    //------------------------------------------------------
    //-- ARP Interfaces
    //------------------------------------------------------
    //-- To   ARP / LookupRequest ------                 
   .m_arpTableOut_TDATA       (ssIPTX_ARP_LkpReq_tdata), 
   .m_arpTableOut_TVALID      (ssIPTX_ARP_LkpReq_tvalid),
   .m_arpTableOut_TREADY      (ssIPTX_ARP_LkpReq_tready),
    //-- From ARP / LookupReply --------
    .s_arpTableIn_TDATA       (ssARP_IPTX_LkpRpl_tdata),
    .s_arpTableIn_TVALID      (ssARP_IPTX_LkpRpl_tvalid),
    .s_arpTableIn_TREADY      (ssARP_IPTX_LkpRpl_tready),
  
    //------------------------------------------------------
    //-- L2MUX Interfaces
    //------------------------------------------------------
    //-- To L2MUX / Data
    .m_dataOut_TDATA          (ssIPTX_L2MUX_Data_tdata),
    .m_dataOut_TKEEP          (ssIPTX_L2MUX_Data_tkeep),
    .m_dataOut_TLAST          (ssIPTX_L2MUX_Data_tlast),
    .m_dataOut_TVALID         (ssIPTX_L2MUX_Data_tvalid),
    .m_dataOut_TREADY         (ssIPTX_L2MUX_Data_tready),
  
    .regSubNetMask_V          (piMMIO_SubNetMask), 
    .regDefaultGateway_V      (piMMIO_GatewayAddr),
    .myMacAddress_V           (piMMIO_MacAddress) 
    
  ); // End of IPTX
    

  //============================================================================
  //  INST: L2MUX AXI4-STREAM INTERCONNECT RTL (Muxes IP and ARP)
  //============================================================================
  AxisInterconnectRtl_2S1M_D8 L2MUX (
    
    .ACLK                 (piShlClk), 
    .ARESETN              (~piShlRst), 
 
    .S00_AXIS_ACLK        (piShlClk), 
    .S01_AXIS_ACLK        (piShlClk), 
    .S00_AXIS_ARESETN     (~piShlRst), 
    .S01_AXIS_ARESETN     (~piShlRst),
 
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
 
    .M00_AXIS_ACLK        (piShlClk), 
    .M00_AXIS_ARESETN     (~piShlRst), 
 
    //------------------------------------------------------
    //-- ETH / Ethernet Layer-2 Interface
    //------------------------------------------------------   
    //-- To   ETH / Data ---------------
    .M00_AXIS_TDATA       (soETH_Data_tdata),
    .M00_AXIS_TKEEP       (soETH_Data_tkeep),
    .M00_AXIS_TLAST       (soETH_Data_tlast),
    .M00_AXIS_TVALID      (soETH_Data_tvalid),
    .M00_AXIS_TREADY      (soETH_Data_tready),
 
    .S00_ARB_REQ_SUPPRESS (1'b0), 
    .S01_ARB_REQ_SUPPRESS (1'b0)
  );


endmodule
