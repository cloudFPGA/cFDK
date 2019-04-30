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
// * Comments:
// *
// *
// *****************************************************************************

`timescale 1ns / 1ps

`define USE_DEPRECATED_DIRECTIVES

// *****************************************************************************
// **  MODULE - IP NETWORK + TCP/UDP TRANSPORT + DHCP SESSION SUBSYSTEM
// *****************************************************************************

module NetworkTransportSession_TcpIp (

  //-- Global Clock used by the entire SHELL -------------
  //--   (This is typically 'sETH0_ShlClk' and we use it all over the place) 
  input          piShlClk,
  
  //-- Global Reset used by the entire SHELL -------------
  //--   (This is typically 'sETH0_ShlRst'. If the module is created by HLS,
  //--    we use it as the default startup reset of the module.) 
  input          piShlRst,
  
  //-- System Reset --------------------------------------
  //--   (This is a delayed version of the global reset. We use it when we
  //--    specifically want to control the re-initialization of a HLS variable.
  //--    We recommended to leave the "config_rtl" configuration to its default
  //--    "control" setting and to use this signal to provide finer grain reset
  //--    functionnality. See "Controlling the Reset Behavior" in UG902).
  input          piShlRstDly,
  
  //-- MMIO : Control inputs and Status outputs ----------
   
  //------------------------------------------------------
  //-- ETHERNET / Nts0 / AXI-Write Stream Interfaces
  //------------------------------------------------------
  //-- Input AXIS Interface ---------------------- 
  input [ 63:0]  piETH0_Nts0_Axis_tdata,
  input [  7:0]  piETH0_Nts0_Axis_tkeep,
  input          piETH0_Nts0_Axis_tlast,
  input          piETH0_Nts0_Axis_tvalid,
  output         poNTS0_Eth0_Axis_tready,
  //-- Output AXIS Interface --------------------- 
  output [ 63:0] poNTS0_Eth0_Axis_tdata,
  output [  7:0] poNTS0_Eth0_Axis_tkeep,
  output         poNTS0_Eth0_Axis_tlast,
  output         poNTS0_Eth0_Axis_tvalid,
  input          piETH0_Nts0_Axis_tready,
 
  //------------------------------------------------------
  //-- MEM / Nts0 / TxP Interfaces
  //------------------------------------------------------
  //-- Transmit Path / S2MM-AXIS -------------------------
  //---- Stream Read Command -------------------
  input          piMEM_Nts0_TxP_Axis_RdCmd_tready,
  output[ 79:0]  poNTS0_Mem_TxP_Axis_RdCmd_tdata,
  output         poNTS0_Mem_TxP_Axis_RdCmd_tvalid,
  //---- Stream Read Status ------------------
  input [  7:0]  piMEM_Nts0_TxP_Axis_RdSts_tdata,
  input          piMEM_Nts0_TxP_Axis_RdSts_tvalid,
  output         poNTS0_Mem_TxP_Axis_RdSts_tready,
  //---- Stream Data Input Channel ----------
  input [ 63:0]  piMEM_Nts0_TxP_Axis_Data_tdata,
  input [  7:0]  piMEM_Nts0_TxP_Axis_Data_tkeep,
  input          piMEM_Nts0_TxP_Axis_Data_tlast,
  input          piMEM_Nts0_TxP_Axis_Data_tvalid,
  output         poNTS0_Mem_TxP_Axis_Data_tready,
  //---- Stream Write Command ----------------
  input          piMEM_Nts0_TxP_Axis_WrCmd_tready,
  output [ 79:0] poNTS0_Mem_TxP_Axis_WrCmd_tdata,
  output         poNTS0_Mem_TxP_Axis_WrCmd_tvalid,
  //---- Stream Write Status -----------------
  input [  7:0]  piMEM_Nts0_TxP_Axis_WrSts_tdata,
  input          piMEM_Nts0_TxP_Axis_WrSts_tvalid,
  output         poNTS0_Mem_TxP_Axis_WrSts_tready,
  //---- Stream Data Output Channel ----------
  input          piMEM_Nts0_TxP_Axis_Data_tready,
  output [ 63:0] poNTS0_Mem_TxP_Axis_Data_tdata,
  output [  7:0] poNTS0_Mem_TxP_Axis_Data_tkeep,
  output         poNTS0_Mem_TxP_Axis_Data_tlast,
  output         poNTS0_Mem_TxP_Axis_Data_tvalid,

  //------------------------------------------------------
  //-- MEM / Nts0 / RxP Interfaces
  //------------------------------------------------------
  //-- Receive Path / S2MM-AXIS ------------------
  //---- Stream Read Command -----------------
  input          piMEM_Nts0_RxP_Axis_RdCmd_tready,
  output [ 79:0] poNTS0_Mem_RxP_Axis_RdCmd_tdata,
  output         poNTS0_Mem_RxP_Axis_RdCmd_tvalid,
  //---- Stream Read Status ------------------
  input [   7:0] piMEM_Nts0_RxP_Axis_RdSts_tdata,
  input          piMEM_Nts0_RxP_Axis_RdSts_tvalid,
  output         poNTS0_Mem_RxP_Axis_RdSts_tready,
  //---- Stream Data Input Channel ----------
  input [ 63:0]  piMEM_Nts0_RxP_Axis_Data_tdata,
  input [  7:0]  piMEM_Nts0_RxP_Axis_Data_tkeep,
  input          piMEM_Nts0_RxP_Axis_Data_tlast,
  input          piMEM_Nts0_RxP_Axis_Data_tvalid,
  output         poNTS0_Mem_RxP_Axis_Data_tready,
  //---- Stream Write Command ----------------
  input          piMEM_Nts0_RxP_Axis_WrCmd_tready,
  output[ 79:0]  poNTS0_Mem_RxP_Axis_WrCmd_tdata,
  output         poNTS0_Mem_RxP_Axis_WrCmd_tvalid,
  //---- Stream Write Status -----------------
  input [  7:0]  piMEM_Nts0_RxP_Axis_WrSts_tdata,
  input          piMEM_Nts0_RxP_Axis_WrSts_tvalid,
  output         poNTS0_Mem_RxP_Axis_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input          piMEM_Nts0_RxP_Axis_Data_tready, 
  output [ 63:0] poNTS0_Mem_RxP_Axis_Data_tdata,
  output [  7:0] poNTS0_Mem_RxP_Axis_Data_tkeep,
  output         poNTS0_Mem_RxP_Axis_Data_tlast,
  output         poNTS0_Mem_RxP_Axis_Data_tvalid,
  
  //------------------------------------------------------
  //-- NRC / Nts0 / Udp Interfaces
  //------------------------------------------------------
  //-- UDMX ==> URIF / Open Port Acknowledge -----
  output  [ 7:0]  soUDMX_Urif_OpnAck_Axis_tdata,
  output          soUDMX_Urif_OpnAck_Axis_tvalid,
  input           soURIF_Udmx_OpnAck_Axis_tready,
  //-- UDMX ==> URIF / Data ----------------------
  output  [63:0]  soUDMX_Urif_Data_Axis_tdata,
  output  [ 7:0]  soUDMX_Urif_Data_Axis_tkeep,
  output          soUDMX_Urif_Data_Axis_tlast,
  output          soUDMX_Urif_Data_Axis_tvalid,
  input           soURIF_Udmx_Data_Axis_tready,
  //-- UDMX ==> URIF / Meta ----------------------
  output  [95:0]  soUDMX_Urif_Meta_Axis_tdata,
  output          soUDMX_Urif_Meta_Axis_tvalid,
  input           soURIF_Udmx_Meta_Axis_tready,
  //-- URIF ==> UDMX / OpenPortRequest / Axis ----
  input   [15:0]  siURIF_Udmx_OpnReq_Axis_tdata,
  input           siURIF_Udmx_OpnReq_Axis_tvalid,
  output          siUDMX_Urif_OpnReq_Axis_tready,
  //-- URIF ==> UDMX / Data / Axis ---------------              
  input   [63:0]  siURIF_Udmx_Data_Axis_tdata,
  input   [ 7:0]  siURIF_Udmx_Data_Axis_tkeep,
  input           siURIF_Udmx_Data_Axis_tlast,
  input           siURIF_Udmx_Data_Axis_tvalid,   
  output          siUDMX_Urif_Data_Axis_tready,
  //-- URIF ==> UDMX / Meta / Axis ---------------
  input   [95:0]  siURIF_Udmx_Meta_Axis_tdata,
  input           siURIF_Udmx_Meta_Axis_tvalid,
  output          siUDMX_Urif_Meta_Axis_tready,
  //-- URIF ==> UDMX / TxLen / Axis --------------
  input   [15:0]  siURIF_Udmx_PLen_Axis_tdata,
  input           siURIF_Udmx_PLen_Axis_tvalid,
  output          siUDMX_Urif_PLen_Axis_tready,
  
  //------------------------------------------------------
  //-- ROLE / Nts0 / Tcp Interfaces
  //------------------------------------------------------
  //-- Input AXI-Write Stream Interface ----------
  input [ 63:0]  piROL_Nts0_Tcp_Axis_tdata,
  input [  7:0]  piROL_Nts0_Tcp_Axis_tkeep,
  input          piROL_Nts0_Tcp_Axis_tvalid,
  input          piROL_Nts0_Tcp_Axis_tlast,
  output         poNTS0_Rol_Tcp_Axis_tready,
  //-- Output AXI-Write Stream Interface ---------
  input          piROL_Nts0_Tcp_Axis_tready,
  output [ 63:0] poNTS0_Rol_Tcp_Axis_tdata,
  output [  7:0] poNTS0_Rol_Tcp_Axis_tkeep,
  output         poNTS0_Rol_Tcp_Axis_tvalid,
  output         poNTS0_Rol_Tcp_Axis_tlast,
  
  //------------------------------------------------------
  //-- MMIO / Nts0 / Interfaces
  //------------------------------------------------------
  input  [ 47:0] piMMIO_Nts0_MacAddress,
  input  [ 31:0] piMMIO_Nts0_IpAddress,
  input  [ 31:0] piMMIO_Nts0_SubNetMask,
  input  [ 31:0] piMMIO_Nts0_GatewayAddr,   
  
  output         poVoid
  
); // End of PortList
   

   
// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire          sTODO_1b0  =  1'b0;

  //------------------------------------------------------------------
  //-- IPRX = IP-RX-HANDLER
  //------------------------------------------------------------------
  //-- IPRX ==> [ARS0] ==> ARP -------------------
  //---- IPRX ==> [ARS0]
  wire  [63:0]  sIPRX_Arp_Axis_tdata;
  wire  [ 7:0]  sIPRX_Arp_Axis_tkeep;
  wire          sIPRX_Arp_Axis_tlast;
  wire          sIPRX_Arp_Axis_tvalid;
  wire          sARP_Iprx_Axis_treadyReg;
  //------------- [ARS0] ==> ARP
  wire  [63:0]  sIPRX_Arp_Axis_tdataReg;
  wire  [ 7:0]  sIPRX_Arp_Axis_tkeepReg;
  wire          sIPRX_Arp_Axis_tlastReg;
  wire          sIPRX_Arp_Axis_tvalidReg;
  wire          sARP_Iprx_Axis_tready;
  //-- IPRX ==> [ARS1] ==> ICMP/Dat --------------
  //---- IPRX ==> [ARS1]
  wire  [63:0]  sIPRX_Icmp_Data_Axis_tdata;
  wire  [ 7:0]  sIPRX_Icmp_Data_Axis_tkeep;
  wire          sIPRX_Icmp_Data_Axis_tlast;
  wire          sIPRX_Icmp_Data_Axis_tvalid;
  wire          sICMP_Iprx_Data_Axis_treadyReg;
  //------------- [ARS1] ==> ICMP_Dat
  wire  [63:0]  sIPRX_Icmp_Data_Axis_tdataReg;
  wire  [ 7:0]  sIPRX_Icmp_Data_Axis_tkeepReg;
  wire          sIPRX_Icmp_Data_Axis_tlastReg;
  wire          sIPRX_Icmp_Data_Axis_tvalidReg;
  wire          sICMP_Iprx_Data_Axis_tready;

  //-- IPRX ==> ICMP/Ttl -------------------------
  wire  [63:0]  sIPRX_Icmp_Ttl_Axis_tdata;
  wire  [ 7:0]  sIPRX_Icmp_Ttl_Axis_tkeep;
  wire          sIPRX_Icmp_Ttl_Axis_tlast;
  wire          sIPRX_Icmp_Ttl_Axis_tvalid;
  wire          sICMP_Iprx_Ttl_Axis_tready;
    
  //-- IPRX ==> UDP ------------------------------
  wire  [63:0]  sIPRX_Udp_Axis_tdata;
  wire  [ 7:0]  sIPRX_Udp_Axis_tkeep;
  wire          sIPRX_Udp_Axis_tlast;
  wire          sIPRX_Udp_Axis_tvalid;
  wire          sUDP_Iprx_Axis_tready;
    
  //-- IPRX ==> [ARS2] ==> TOE -------------------
  //---- IPRX ==> [ARS2]
  wire  [63:0]  sIPRX_Toe_Axis_tdata;
  wire  [ 7:0]  sIPRX_Toe_Axis_tkeep;
  wire          sIPRX_Toe_Axis_tlast;
  wire          sIPRX_Toe_Axis_tvalid;
  wire          sTOE_Iprx_Axis_treadyReg;
  //------------ [ARS2] ==> TOE
  wire  [63:0]  sIPRX_Toe_Axis_tdataReg;
  wire  [ 7:0]  sIPRX_Toe_Axis_tkeepReg;
  wire          sIPRX_Toe_Axis_tlastReg;
  wire          sIPRX_Toe_Axis_tvalidReg;
  wire          sTOE_Iprx_Axis_tready;  
  
  //------------------------------------------------------------------
  //-- UDP = UDP-CORE-MODULE
  //------------------------------------------------------------------
  //-- UDP ==> ICMP ------------------------------
  wire  [63:0]  sUDP_Icmp_Axis_tdata;
  wire  [ 7:0]  sUDP_Icmp_Axis_tkeep;
  wire          sUDP_Icmp_Axis_tlast;
  wire          sUDP_Icmp_Axis_tvalid;
  wire          sICMP_Udp_Axis_tready;

  //-- UDP ==> L3MUX -----------------------------
  wire  [63:0]  sUDP_L3mux_Axis_tdata;
  wire  [ 7:0]  sUDP_L3mux_Axis_tkeep;
  wire          sUDP_L3mux_Axis_tlast;
  wire          sUDP_L3mux_Axis_tvalid;
  wire          sL3MUX_Udp_Axis_tready;

  //-- UDP ==> UDMX / OpenPortStatus -------------
  wire  [ 7:0]  sUDP_Udmx_OpnSts_Axis_tdata;
  wire          sUDP_Udmx_OpnSts_Axis_tvalid;
  wire          sUDMX_Udp_OpnSts_Axis_tready;
  //-- UDP ==> UDMX / Data -----------------------
  wire  [63:0]  sUDP_Udmx_Data_Axis_tdata;
  wire  [ 7:0]  sUDP_Udmx_Data_Axis_tkeep;
  wire          sUDP_Udmx_Data_Axis_tlast;
  wire          sUDP_Udmx_Data_Axis_tvalid;
  wire          sUDMX_Udp_Data_Axis_tready;
  //-- UDP ==> UDMX / Meta -----------------------
  wire  [95:0]  sUDP_Udmx_Meta_Axis_tdata;
  wire          sUDP_Udmx_Meta_Axis_tvalid;
  wire          sUDMX_Udp_Meta_Axis_tready;

  //------------------------------------------------------------------
  //-- UDMX = UDP-MUX
  //------------------------------------------------------------------
  //-- UDMX ==> UDP / OpenPortRequest ------------
  wire  [15:0]  sUDMX_Udp_OpnReq_Axis_tdata;
  wire          sUDMX_Udp_OpnReq_Axis_tvalid;
  wire          sUDP_Udmx_OpnReq_Axis_tready;
  //-- UDMX ==> UDP / Data -----------------------
  wire  [63:0]  sUDMX_Udp_Data_Axis_tdata;
  wire  [ 7:0]  sUDMX_Udp_Data_Axis_tkeep;
  wire          sUDMX_Udp_Data_Axis_tlast;
  wire          sUDMX_Udp_Data_Axis_tvalid;
  wire          sUDP_Udmx_Data_Axis_tready;
  //-- UDMX ==> UDP / TxLength -------------------
  wire  [15:0]  sUDMX_Udp_TxLn_Axis_tdata;
  wire          sUDMX_Udp_TxLn_Axis_tvalid;
  wire          sUDP_Udmx_PLen_Axis_tready;
  //-- UDMX ==> UDP / Meta -----------------------
  wire  [95:0]  sUDMX_Udp_Meta_Axis_tdata;
  wire          sUDMX_Udp_Meta_Axis_tvalid;
  wire          sUDP_Udmx_Meta_Axis_tready;

  //-- UDMX ==> URIF / Open Port Acknowledge -----
  //OBSOLETE wire  [ 7:0]  sUDMX_Urif_OpnAck_Axis_tdata;
  //OBSOLETE wire          sUDMX_Urif_OpnAck_Axis_tvalid;
  //OBSOLETE wire          sURIF_Udmx_OpnAck_Axis_tready;
  //OBSOLETE //-- UDMX ==> URIF / Data ----------------------
  //OBSOLETE wire  [63:0]  sUDMX_Urif_Data_Axis_tdata;
  //OBSOLETE wire  [ 7:0]  sUDMX_Urif_Data_Axis_tkeep;
  //OBSOLETE wire          sUDMX_Urif_Data_Axis_tlast;
  //OBSOLETE wire          sUDMX_Urif_Data_Axis_tvalid;
  //OBSOLETE wire          sURIF_Udmx_Data_Axis_tready;
  //OBSOLETE //-- UDMX ==> URIF / Meta ----------------------
  //OBSOLETE wire  [95:0]  sUDMX_Urif_Meta_Axis_tdata;
  //OBSOLETE wire          sUDMX_Urif_Meta_Axis_tvalid;
  //OBSOLETE wire          sURIF_Udmx_Meta_Axis_tready;
    
  //-- UDMX ==> DHCP / Open Port Acknowledge -----
  wire  [ 7:0]  sUDMX_Dhcp_OpnAck_Axis_tdata;
  wire          sUDMX_Dhcp_OpnAck_Axis_tvalid;
  wire          sDHCP_Udmx_OpnAck_Axis_tready;
  //-- UDMX ==> DHCP -----------------------------
  wire  [63:0]  sUDMX_Dhcp_Data_Axis_tdata;
  wire  [ 7:0]  sUDMX_Dhcp_Data_Axis_tkeep;
  wire          sUDMX_Dhcp_Data_Axis_tlast;
  wire          sUDMX_Dhcp_Data_Axis_tvalid;
  wire          sDHCP_Udmx_Data_Axis_tready;
  //-- UDMX ==> DHCP -----------------------------
  wire  [95:0]  sUDMX_Dhcp_Meta_Axis_tdata;
  wire          sUDMX_Dhcp_Meta_Axis_tvalid;
  wire          sDHCP_Udmx_Meta_Axis_tready;
  
  //------------------------------------------------------------------
  //-- DHCP = DHCP-CLIENT
  //------------------------------------------------------------------      
  //-- DHCP ==> UDMX / Open Port Request --------
  wire  [15:0]  sDHCP_Udmx_OpnReq_Axis_tdata;
  wire          sDHCP_Udmx_OpnReq_Axis_tvalid;
  wire          sUDMX_Dhcp_OpnReq_Axis_tready;
  //-- DHCP ==> UDMX / Data /Axis ---------------
  wire  [63:0]  sDHCP_Udmx_Data_Axis_tdata;
  wire  [7:0]   sDHCP_Udmx_Data_Axis_tkeep;
  wire          sDHCP_Udmx_Data_Axis_tlast;
  wire          sDHCP_Udmx_Data_Axis_tvalid;
  wire          sUDMX_Dhcp_Data_Axis_tready;  
  //-- DHCP ==> UDMX / Tx Length ----------------
  wire  [15:0]  sDHCP_Udmx_PLen_Axis_tdata;
  wire          sDHCP_Udmx_PLen_Axis_tvalid;
  wire          sUDMX_Dhcp_PLen_Axis_tready;
  //-- DHCP ==> UDMX / Tx MetaData --------------
  wire  [95:0]  sDHCP_Udmx_Meta_Axis_tdata;
  wire          sDHCP_Udmx_Meta_Axis_tvalid;
  wire          sUDMX_Dhcp_Meta_Axis_tready;

  //------------------------------------------------------------------
  //-- URIF = USER-ROLE-INTERFACE
  //------------------------------------------------------------------
  //-- URIF ==> UDMX / OpenPortRequest / Axis ----
  //OBSOLETE wire  [15:0]  sURIF_Udmx_OpnReq_Axis_tdata;
  //OBSOLETE wire          sURIF_Udmx_OpnReq_Axis_tvalid;
  //OBSOLETE wire          sUDMX_Urif_OpnReq_Axis_tready;
  //OBSOLETE //-- URIF ==> UDMX / Data / Axis ---------------              
  //OBSOLETE wire  [63:0]  sURIF_Udmx_Data_Axis_tdata;    
  //OBSOLETE wire  [ 7:0]  sURIF_Udmx_Data_Axis_tkeep;
  //OBSOLETE wire          sURIF_Udmx_Data_Axis_tlast;
  //OBSOLETE wire          sURIF_Udmx_Data_Axis_tvalid;   
  //OBSOLETE wire          sUDMX_Urif_Data_Axis_tready;
  //OBSOLETE //-- URIF ==> UDMX / Meta / Axis ---------------
  //OBSOLETE wire  [95:0]  sURIF_Udmx_Meta_Axis_tdata;
  //OBSOLETE wire          sURIF_Udmx_Meta_Axis_tvalid;
  //OBSOLETE wire          sUDMX_Urif_Meta_Axis_tready;
  //OBSOLETE //-- URIF ==> UDMX / TxLen / Axis --------------
  //OBSOLETE wire  [15:0]  sURIF_Udmx_PLen_Axis_tdata;
  //OBSOLETE wire          sURIF_Udmx_PLen_Axis_tvalid;
  //OBSOLETE wire          sUDMX_Urif_PLen_Axis_tready;
  //-- URIF ==> ROLE / Axis ----------------------
  //OBSOLETE wire  [63:0]  sURIF_Rol_Axis_tdata;
  //OBSOLETE wire  [ 7:0]  sURIF_Rol_Axis_tkeep;
  //OBSOLETE wire          sURIF_Rol_Axis_tlast;
  //OBSOLETE wire          sURIF_Rol_Axis_tvalid;
  //OBSOLETE wire          sROL_Urif_Axis_treadyReg;
  
  //------------------------------------------------------------------
  //-- TRIF = USER-ROLE-INTERFACE
  //------------------------------------------------------------------
  //-- TRIF ==> TOE / SendDataRequest / Axis ---------------------
  wire  [63:0]  sTRIF_Toe_Data_Axis_tdata;
  wire  [7:0]   sTRIF_Toe_Data_Axis_tkeep;
  wire          sTRIF_Toe_Data_Axis_tlast;
  wire          sTRIF_Toe_Data_Axis_tvalid;
  wire          sTOE_Trif_Data_Axis_tready;
  //-- TRIF ==> TOE / SendMetaDataRequest / Axis
  wire  [15:0]  sTRIF_Toe_Meta_Axis_tdata;
  wire          sTRIF_Toe_Meta_Axis_tvalid;
  wire          sTOE_Trif_Meta_Axis_tready;
  //-- TRIF ==> TOE / ReceiveDataRequest / Axis
  wire  [31:0]  sTRIF_Toe_DReq_Axis_tdata;
  wire          sTRIF_Toe_DReq_Axis_tvalid;
  wire          sTOE_Trif_DReq_Axis_tready;
  //-- TRIF ==> TOE / OpenConnectionRequest / Axis ----
  wire  [47:0]  sTRIF_Toe_OpnReq_Axis_tdata;
  wire          sTRIF_Toe_OpnReq_Axis_tvalid;
  wire          sTOE_Trif_OpnReq_Axis_tready;
  //-- TRIF ==> TOE / ListenPortRequest / Axis
  wire  [15:0]  sTRIF_Toe_LsnReq_Axis_tdata;
  wire          sTRIF_Toe_LsnReq_Axis_tvalid;
  wire          sTOE_Trif_LsnReq_Axis_tready;
  //-- TRIF ==> TOE / CloseConnectionRequest
  wire  [15:0]  sTRIF_Toe_ClsReq_Axis_tdata;
  wire          sTRIF_Toe_ClsReq_Axis_tvalid;
  wire          sTOE_Trif_ClsReq_Axis_tready;
  //-- TRIF ==> ROLE / Axis ----------------------
  wire  [63:0]  sTRIF_Rol_Axis_tdata;
  wire  [ 7:0]  sTRIF_Rol_Axis_tkeep;
  wire          sTRIF_Rol_Axis_tlast;
  wire          sTRIF_Rol_Axis_tvalid;
  wire          sROL_Trif_Axis_treadyReg;

  //------------------------------------------------------------------
  //-- TOE = TCP-OFFLOAD-ENGINE
  //------------------------------------------------------------------
  //-- TOE ==> TRIF / ReceiveDataReply / Axis ---------
  wire  [63:0]  sTOE_Trif_Data_Axis_tdata;
  wire  [ 7:0]  sTOE_Trif_Data_Axis_tkeep;
  wire          sTOE_Trif_Data_Axis_tlast;
  wire          sTOE_Trif_Data_Axis_tvalid;
  wire          sTRIF_Toe_Data_Axis_tready;
  //-- TOE ==> TRIF / REceiveMetaDataReply / Axis -----
  wire  [15:0]  sTOE_Trif_Meta_Axis_tdata;
  wire          sTOE_Trif_Meta_Axis_tvalid;
  wire          sTRIF_Toe_Meta_Axis_tready;
  //-- TOE ==> TRIF / SendDataSeply / Axis ------------
  wire  [23:0]  sTOE_Trif_DSts_Axis_tdata;
  wire          sTOE_Trif_DSts_Axis_tvalid;
  wire          sTRIF_Toe_DSts_Axis_tready;
  //-- TOE ==> TRIF / OpenConnectionResponse / Axis
  wire  [23:0]  sTOE_Trif_OpnSts_Axis_tdata;
  wire          sTOE_Trif_OpnSts_Axis_tvalid;
  wire          sTRIF_Toe_OpnSts_Axis_tready;
  //-- TOE ==> TRIF / ListenPortResponse / Axis
  wire  [7:0]   sTOE_Trif_LsnAck_Axis_tdata;
  wire          sTOE_Trif_LsnAck_Axis_tvalid;
  wire          sTRIF_Toe_LsnAck_Axis_tready;
  //-- TOE ==> TRIF / Notifications / Axis
  wire  [87:0]  sTOE_Trif_Notif_Axis_tdata;
  wire          sTOE_Trif_Notif_Axis_tvalid;
  wire          sTRIF_Toe_Notif_Axis_tready;
  //-- TOE ==> [ARS3] ==> L3MUX ------------------
  //---- TOE ==> [ARS3]
  wire  [63:0]  sTOE_L3mux_Axis_tdata;
  wire  [ 7:0]  sTOE_L3mux_Axis_tkeep;
  wire          sTOE_L3mux_Axis_tlast;
  wire          sTOE_L3mux_Axis_tvalid;
  wire          sL3MUX_Toe_Axis_treadyReg;
  //----         [ARS3] ==> L3MUX ----------------
  wire  [63:0]  sTOE_L3mux_Axix_tdataReg;
  wire  [ 7:0]  sTOE_L3mux_Axix_tkeepReg;
  wire          sTOE_L3mux_Axix_tlastReg;
  wire          sTOE_L3mux_Axix_tvalidReg;
  wire          sL3MUX_Toe_Axix_tready;
  //-- TOE ==> CAM / LookupRequest / Axis
  wire  [97:0]  sTOE_Cam_LkpReq_Axis_tdata;  // [FIXME: should be 96, also wrong in SmartCam]
  wire          sTOE_Cam_LkpReq_Axis_tvalid;
  wire          sCAM_Toe_LkpReq_Axis_tready;
  //-- TOE ==> CAM / UpdateRequest / Axis
  wire  [111:0] sTOE_Cam_UpdReq_Axis_tdata;  //( 1 + 1 + 14 + 96) - 1 = 111
  wire          sTOE_Cam_UpdReq_Axis_tvalid;
  wire          sCAM_Toe_UpdReq_Axis_tready;
 
  //------------------------------------------------------------------
  //-- CAM = TOE-CAM
  //------------------------------------------------------------------
  //-- CAM ==> TOE / LookupReply / Axis
  wire  [15:0]  sCAM_Toe_LkpRep_Axis_tdata;
  wire          sCAM_Toe_LkpRep_Axis_tvalid;
  wire          sTOE_Cam_LkpRep_Axis_tready;
  //-- CAM ==> TOE / UpdateReply / Axis
  wire  [15:0]  sCAM_Toe_UpdRpl_Axis_tdata;
  wire          sCAM_Toe_UpdRpl_Axis_tvalid;
  wire          sTOE_Cam_UpdRpl_Axis_tready;

  //------------------------------------------------------------------
  //-- ICMP = ICMP-SERVER
  //------------------------------------------------------------------
  //-- ICMP ==> L3MUX / Axis
  wire  [63:0]  sICMP_L3mux_Axis_tdata;
  wire  [ 7:0]  sICMP_L3mux_Axis_tkeep;
  wire          sICMP_L3mux_Axis_tlast;
  wire          sICMP_L3mux_Axis_tvalid;
  wire          sL3MUX_Icmp_Axis_tready;

  //------------------------------------------------------------------
  //-- ARP = ARP-SERVER
  //------------------------------------------------------------------
  //-- ARP ==> L2MUX / Axis
  wire  [63:0]  sARP_L2mux_Axis_tdata;
  wire  [ 7:0]  sARP_L2mux_Axis_tkeep;
  wire          sARP_L2mux_Axis_tlast;
  wire          sARP_L2mux_Axis_tvalid;
  wire          sL2MUX_Arp_Axis_tready;
  //-- ARP ==> IPTX / LkpRpl / Axis
  wire [55:0]   sARP_Iptx_LkpRpl_Axis_tdata;
  wire          sARP_Iptx_LkpRpl_Axis_tvalid;
  wire          sIPTX_Arp_LkpRpl_Axis_tready;
  
  //------------------------------------------------------------------
  //-- IPTX = IP-TX-HANDLER
  //------------------------------------------------------------------ 
  //-- IPTX ==> ARP / LookupRequest / Axis
  wire  [31:0]  sIPTX_Arp_LkpReq_Axis_tdata;
  wire          sIPTX_Arp_LkpReq_Axis_tvalid;
  wire          sARP_Iptx_LkpReq_Axis_tready;
  //-- IPTX ==> L2MUX /Axis
  wire  [63:0]  sIPTX_L2mux_Axis_tdata;
  wire  [ 7:0]  sIPTX_L2mux_Axis_tkeep;
  wire          sIPTX_L2mux_Axis_tlast;
  wire          sIPTX_L2mux_Axis_tvalid;
  wire          sL2MUX_Iptx_Axis_tready;
  
  //------------------------------------------------------------------
  //-- L2MUX = LAYER-2-MULTIPLEXER
  //------------------------------------------------------------------ 
  
  //------------------------------------------------------------------
  //-- L3MUX = LAYER-3-MULTIPLEXER
  //------------------------------------------------------------------ 
  //-- L3MUX ==> IPTX / Axis
  wire  [63:0]  sL3MUX_Iptx_Axis_tdata;
  wire  [ 7:0]  sL3MUX_Iptx_Axis_tkeep;
  wire          sL3MUX_Iptx_Axis_tlast;
  wire          sL3MUX_Iptx_Axis_tvalid;
  wire          sIPTX_L3mux_Axis_tready;

  //------------------------------------------------------------------
  //-- ROLE = USER-LOGIC
  //------------------------------------------------------------------
  //-- ROLE ==> [ARS5] ==> TRIF ------------------
  //---- ROLE ==> [ARS5] (see piROL_Nts0_Tcp_Axis_t*)
  //----          [ARS5] ==> TRIF ----------------
  wire  [63:0]  sROL_Nts0_Tcp_Axis_tdataReg;
  wire  [ 7:0]  sROL_Nts0_Tcp_Axis_tkeepReg;
  wire          sROL_Nts0_Tcp_Axis_tlastReg;
  wire          sROL_Nts0_Tcp_Axis_tvalidReg;
  wire          sTRIF_Rol_Axis_tready;
  
  //-- ROLE ==> [ARS7] ==> URIF ------------------
  //---- ROLE ==> [ARS7] (see piROL_Nts0_Udp_Axis_t*)
  //----          [ARS7] ==> URIF ----------------
  wire  [63:0]  sROL_Nts0_Udp_Axis_tdataReg;
  wire  [ 7:0]  sROL_Nts0_Udp_Axis_tkeepReg;
  wire          sROL_Nts0_Udp_Axis_tlastReg;
  wire          sROL_Nts0_Udp_Axis_tvalidReg;
  wire          sURIF_Rol_Axis_tready;

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
    .aresetn                  (~piShlRst),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_This_MacAddress_V (piMMIO_Nts0_MacAddress),
    .piMMIO_This_Ip4Address_V (piMMIO_Nts0_IpAddress),
                      
    //------------------------------------------------------
    //-- From ETH0 Interfaces
    //------------------------------------------------------
    //-- ETH[0] / Nts[0] / Data/ Axis
    .siETH_This_Data_TDATA    (piETH0_Nts0_Axis_tdata),
    .siETH_This_Data_TKEEP    (piETH0_Nts0_Axis_tkeep),
    .siETH_This_Data_TLAST    (piETH0_Nts0_Axis_tlast),
    .siETH_This_Data_TVALID   (piETH0_Nts0_Axis_tvalid),
    .siETH_This_Data_TREADY   (poNTS0_Eth0_Axis_tready),
    
    //------------------------------------------------------
    //-- To ARP Interfaces
    //------------------------------------------------------
    //-- THIS / Arp / Data / Axis
    .soTHIS_Arp_Data_TREADY   (sARP_Iprx_Axis_treadyReg),     
    .soTHIS_Arp_Data_TDATA    (sIPRX_Arp_Axis_tdata),       
    .soTHIS_Arp_Data_TKEEP    (sIPRX_Arp_Axis_tkeep),      
    .soTHIS_Arp_Data_TLAST    (sIPRX_Arp_Axis_tlast),   
    .soTHIS_Arp_Data_TVALID   (sIPRX_Arp_Axis_tvalid), 
   
    //------------------------------------------------------
    //-- To ICMP Interfaces
    //------------------------------------------------------
    //-- THIS / Icmp / Data / Axis
    .soTHIS_Icmp_Data_TREADY  (sICMP_Iprx_Data_Axis_treadyReg),
    .soTHIS_Icmp_Data_TDATA   (sIPRX_Icmp_Data_Axis_tdata),
    .soTHIS_Icmp_Data_TKEEP   (sIPRX_Icmp_Data_Axis_tkeep),
    .soTHIS_Icmp_Data_TLAST   (sIPRX_Icmp_Data_Axis_tlast),
    .soTHIS_Icmp_Data_TVALID  (sIPRX_Icmp_Data_Axis_tvalid),
    //-- THIS / Icmp / Derr / Axis
    .soTHIS_Icmp_Derr_TREADY  (sICMP_Iprx_Ttl_Axis_tready),
    .soTHIS_Icmp_Derr_TDATA   (sIPRX_Icmp_Ttl_Axis_tdata),
    .soTHIS_Icmp_Derr_TKEEP   (sIPRX_Icmp_Ttl_Axis_tkeep),
    .soTHIS_Icmp_Derr_TLAST   (sIPRX_Icmp_Ttl_Axis_tlast),
    .soTHIS_Icmp_Derr_TVALID  (sIPRX_Icmp_Ttl_Axis_tvalid),

    //------------------------------------------------------
    //-- To UDP Interfaces
    //------------------------------------------------------
    //-- THIS / Udp / Axis
    .soTHIS_Udp_Data_TREADY   (sUDP_Iprx_Axis_tready),
    .soTHIS_Udp_Data_TDATA    (sIPRX_Udp_Axis_tdata),
    .soTHIS_Udp_Data_TKEEP    (sIPRX_Udp_Axis_tkeep),
    .soTHIS_Udp_Data_TLAST    (sIPRX_Udp_Axis_tlast),
    .soTHIS_Udp_Data_TVALID   (sIPRX_Udp_Axis_tvalid),
 
    //------------------------------------------------------
    //-- To TOE Interfaces
    //------------------------------------------------------
    //-- THIS / Toe / Axis
    .soTHIS_Tcp_Data_TREADY   (sTOE_Iprx_Axis_treadyReg),
    .soTHIS_Tcp_Data_TDATA    (sIPRX_Toe_Axis_tdata),
    .soTHIS_Tcp_Data_TKEEP    (sIPRX_Toe_Axis_tkeep),
    .soTHIS_Tcp_Data_TLAST    (sIPRX_Toe_Axis_tlast),
    .soTHIS_Tcp_Data_TVALID   (sIPRX_Toe_Axis_tvalid)

  ); // End of IPRX

`endif //  `ifdef USE_DEPRECATED_DIRECTIVES
   
      
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_ip_module_rx_path_1 IPRX (
//    .s_dataIn_TVALID(rx_data_TVALID),                  
//    .s_dataIn_TREADY(rx_data_TREADY),                 
//    .s_dataIn_TDATA(rx_data_TDATA),                   
//    .s_dataIn_TKEEP(rx_data_TKEEP),                  
//    .s_dataIn_TLAST(rx_data_TLAST),                  
 
//    .m_ARPdataOut_TVALID(axi_iph_to_arp_slice_tvalid),    
//    .m_ARPdataOut_TREADY(axi_iph_to_arp_slice_tready),     
//    .m_ARPdataOut_TDATA(axi_iph_to_arp_slice_tdata),       
//    .m_ARPdataOut_TKEEP(axi_iph_to_arp_slice_tkeep),      
//    .m_ARPdataOut_TLAST(axi_iph_to_arp_slice_tlast),      
 
//    .m_ICMPdataOut_TVALID(axi_iph_to_icmp_slice_tvalid),   
//    .m_ICMPdataOut_TREADY(axi_iph_to_icmp_slice_tready),   
//    .m_ICMPdataOut_TDATA(axi_iph_to_icmp_slice_tdata),     
//    .m_ICMPdataOut_TKEEP(axi_iph_to_icmp_slice_tkeep),     
//    .m_ICMPdataOut_TLAST(axi_iph_to_icmp_slice_tlast),    
 
//    .m_ICMPexpDataOut_TVALID(axis_ttl_to_icmp_tvalid),     
//    .m_ICMPexpDataOut_TREADY(axis_ttl_to_icmp_tready),     
//    .m_ICMPexpDataOut_TDATA(axis_ttl_to_icmp_tdata),      
//    .m_ICMPexpDataOut_TKEEP(axis_ttl_to_icmp_tkeep),      
//    .m_ICMPexpDataOut_TLAST(axis_ttl_to_icmp_tlast),    
 
//    .m_UDPdataOut_TVALID(axi_iph_to_udp_tvalid),        
//    .m_UDPdataOut_TREADY(axi_iph_to_udp_tready),          
//    .m_UDPdataOut_TDATA(axi_iph_to_udp_tdata),            
//    .m_UDPdataOut_TKEEP(axi_iph_to_udp_tkeep),           
//    .m_UDPdataOut_TLAST(axi_iph_to_udp_tlast),            
 
//    .m_TCPdataOut_TVALID(axi_iph_to_toe_tvalid),        
//    .m_TCPdataOut_TREADY(axi_iph_to_toe_tready),         
//    .m_TCPdataOut_TDATA(axi_iph_to_toe_tdata),        
//    .m_TCPdataOut_TKEEP(axi_iph_to_toe_tkeep),         
//    .m_TCPdataOut_TLAST(axi_iph_to_toe_tlast),        
 
//    //.regIpAddress_V(32'h01010101),                     
//    //.regIpAddress_V(inputIpAddress),
//    .regIpAddress_V(cloud_fpga_ip),
//    .myMacAddress_V(cloud_fpga_mac),                  
//    .aclk(cf_axi_clk),                               
//    .aresetn(cf_aresetn)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==> ARP)
  //============================================================================
  AxisRegisterSlice_64 ARS0 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- Form IPRX / Axis
    .s_axis_tdata   (sIPRX_Arp_Axis_tdata),
    .s_axis_tkeep   (sIPRX_Arp_Axis_tkeep),
    .s_axis_tlast   (sIPRX_Arp_Axis_tlast),
    .s_axis_tvalid  (sIPRX_Arp_Axis_tvalid),
    .s_axis_tready  (sARP_Iprx_Axis_treadyReg),
    //-- To ARP / Axis
    .m_axis_tready  (sARP_Iprx_Axis_tready),
    .m_axis_tdata   (sIPRX_Arp_Axis_tdataReg),
    .m_axis_tkeep   (sIPRX_Arp_Axis_tkeepReg),
    .m_axis_tlast   (sIPRX_Arp_Axis_tlastReg),
    .m_axis_tvalid  (sIPRX_Arp_Axis_tvalidReg)
  );
      
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS0 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
    
//    .s_axis_tvalid(axi_iph_to_arp_slice_tvalid),
//    .s_axis_tready(axi_iph_to_arp_slice_tready),
//    .s_axis_tdata(axi_iph_to_arp_slice_tdata),
//    .s_axis_tkeep(axi_iph_to_arp_slice_tkeep),
//    .s_axis_tlast(axi_iph_to_arp_slice_tlast),
 
//    .m_axis_tvalid(axi_arp_slice_to_arp_tvalid),
//    .m_axis_tready(axi_arp_slice_to_arp_tready),
//    .m_axis_tdata(axi_arp_slice_to_arp_tdata),
//    .m_axis_tkeep(axi_arp_slice_to_arp_tkeep),
//    .m_axis_tlast(axi_arp_slice_to_arp_tlast)
//  );  
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==> ICMP)
  //============================================================================
  AxisRegisterSlice_64 ARS1 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From IPRX / Axis
    .s_axis_tdata   (sIPRX_Icmp_Data_Axis_tdata),
    .s_axis_tkeep   (sIPRX_Icmp_Data_Axis_tkeep),
    .s_axis_tlast   (sIPRX_Icmp_Data_Axis_tlast),
    .s_axis_tvalid  (sIPRX_Icmp_Data_Axis_tvalid),
    .s_axis_tready  (sICMP_Iprx_Data_Axis_treadyReg),
    //-- To ICMP / Axis  
    .m_axis_tready  (sICMP_Iprx_Data_Axis_tready),
    .m_axis_tdata   (sIPRX_Icmp_Data_Axis_tdataReg),
    .m_axis_tkeep   (sIPRX_Icmp_Data_Axis_tkeepReg),
    .m_axis_tlast   (sIPRX_Icmp_Data_Axis_tlastReg),
    .m_axis_tvalid  (sIPRX_Icmp_Data_Axis_tvalidReg)
  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS1 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
  
//    .s_axis_tvalid(axi_iph_to_icmp_slice_tvalid),
//    .s_axis_tready(axi_iph_to_icmp_slice_tready),
//    .s_axis_tdata(axi_iph_to_icmp_slice_tdata),
//    .s_axis_tkeep(axi_iph_to_icmp_slice_tkeep),
//    .s_axis_tlast(axi_iph_to_icmp_slice_tlast),
  
//    .m_axis_tvalid(axi_icmp_slice_to_icmp_tvalid),
//    .m_axis_tready(axi_icmp_slice_to_icmp_tready),
//    .m_axis_tdata(axi_icmp_slice_to_icmp_tdata),
//    .m_axis_tkeep(axi_icmp_slice_to_icmp_tkeep),
//    .m_axis_tlast(axi_icmp_slice_to_icmp_tlast)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: ARP 
  //============================================================================
  AddressResolutionProcess ARP (
  
    .aclk                           (piShlClk),
    .aresetn                        (~piShlRst),
  
    //------------------------------------------------------
    //-- From IPRX Interfaces
    //------------------------------------------------------
    //-- IPRX / This / Axis
    .axi_arp_slice_to_arp_tdata     (sIPRX_Arp_Axis_tdataReg),   // [TODO - Bad HLS port names]
    .axi_arp_slice_to_arp_tkeep     (sIPRX_Arp_Axis_tkeepReg),
    .axi_arp_slice_to_arp_tlast     (sIPRX_Arp_Axis_tlastReg),
    .axi_arp_slice_to_arp_tvalid    (sIPRX_Arp_Axis_tvalidReg),
    .axi_arp_slice_to_arp_tready    (sARP_Iprx_Axis_tready),     // [TODO - Bad HLS port names]
   
    //------------------------------------------------------
    //-- From IPTX Interfaces
    //------------------------------------------------------
    //-- IPTX / LoopkupRequest / Axis
    .axis_arp_lookup_request_TDATA  (sIPTX_Arp_LkpReq_Axis_tdata),
    .axis_arp_lookup_request_TVALID (sIPTX_Arp_LkpReq_Axis_tvalid),
    .axis_arp_lookup_request_TREADY (sARP_Iptx_LkpReq_Axis_tready),
    
    //------------------------------------------------------
    //-- To IPTX nterfaces
    //------------------------------------------------------
     //-- THIS / Iptx / LoopkupReply / Axis
    .axis_arp_lookup_reply_TREADY   (sIPTX_Arp_LkpRpl_Axis_tready),
    .axis_arp_lookup_reply_TDATA    (sARP_Iptx_LkpRpl_Axis_tdata),
    .axis_arp_lookup_reply_TVALID   (sARP_Iptx_LkpRpl_Axis_tvalid),
    
    //------------------------------------------------------
    //-- To L2MUX Interfaces
    //------------------------------------------------------
    //-- THIS / L2mux / Axis  
    .axi_arp_to_arp_slice_tready    (sL2MUX_Arp_Axis_tready),
    .axi_arp_to_arp_slice_tdata     (sARP_L2mux_Axis_tdata),
    .axi_arp_to_arp_slice_tkeep     (sARP_L2mux_Axis_tkeep),
    .axi_arp_to_arp_slice_tlast     (sARP_L2mux_Axis_tlast),
    .axi_arp_to_arp_slice_tvalid    (sARP_L2mux_Axis_tvalid),
    
    .myMacAddress                   (piMMIO_Nts0_MacAddress),
    .myIpAddress                    (piMMIO_Nts0_IpAddress)

  ); // End of ARP
  
/* -----\/----- [OBSOLETE] -----\/-----
//  arpServerWrapper ARP (
//    .axi_arp_to_arp_slice_tvalid(axi_arp_to_arp_slice_tvalid),
//    .axi_arp_to_arp_slice_tready(axi_arp_to_arp_slice_tready),
//    .axi_arp_to_arp_slice_tdata(axi_arp_to_arp_slice_tdata),
//    .axi_arp_to_arp_slice_tkeep(axi_arp_to_arp_slice_tkeep),
//    .axi_arp_to_arp_slice_tlast(axi_arp_to_arp_slice_tlast),
    
//    .axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
//    .axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
//    .axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),
    
//    .axi_arp_slice_to_arp_tvalid(axi_arp_slice_to_arp_tvalid),
//    .axi_arp_slice_to_arp_tready(axi_arp_slice_to_arp_tready),
//    .axi_arp_slice_to_arp_tdata(axi_arp_slice_to_arp_tdata),
//    .axi_arp_slice_to_arp_tkeep(axi_arp_slice_to_arp_tkeep),
//    .axi_arp_slice_to_arp_tlast(axi_arp_slice_to_arp_tlast),
    
//    .axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
//    .axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
//    .axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),
    
//    .myMacAddress(cloud_fpga_mac),
//    .myIpAddress(cloud_fpga_ip),
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: TCP-OFFLOAD-MODULE
  //============================================================================
  TcpOffloadEngine TOE (
  
    .aclk                      (piShlClk),
    .aresetn                   (~piShlRst),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------    
    .piMMIO_IpAddr_V           (piMMIO_Nts0_IpAddress),
                        
    //------------------------------------------------------
    //-- From IPRX / IP Rx Data Interface
    //------------------------------------------------------
    //-- IPRX / This / Data / Axis
    .siIPRX_Data_TDATA         (sIPRX_Toe_Axis_tdataReg),
    .siIPRX_Data_TKEEP         (sIPRX_Toe_Axis_tkeepReg),
    .siIPRX_Data_TLAST         (sIPRX_Toe_Axis_tlastReg),
    .siIPRX_Data_TVALID        (sIPRX_Toe_Axis_tvalidReg),
    .siIPRX_Data_TREADY        (sTOE_Iprx_Axis_tready),

    //------------------------------------------------------
    //-- To L3MUX / IP Tx Data Interface
    //------------------------------------------------------
    //-- THIS / L3mux / Data / Axis
    .soL3MUX_Data_TREADY       (sL3MUX_Toe_Axis_treadyReg),
    .soL3MUX_Data_TDATA        (sTOE_L3mux_Axis_tdata),
    .soL3MUX_Data_TKEEP        (sTOE_L3mux_Axis_tkeep),    
    .soL3MUX_Data_TLAST        (sTOE_L3mux_Axis_tlast),
    .soL3MUX_Data_TVALID       (sTOE_L3mux_Axis_tvalid),

    //------------------------------------------------------
    //-- From TRIF / ROLE Rx Data Interfaces
    //------------------------------------------------------
    //-- TRIF / This / Rx Data Request / Axis
    .siTRIF_DReq_TDATA         (sTRIF_Toe_DReq_Axis_tdata),
    .siTRIF_DReq_TVALID        (sTRIF_Toe_DReq_Axis_tvalid),
    .siTRIF_DReq_TREADY        (sTOE_Trif_DReq_Axis_tready),

    //------------------------------------------------------
    //-- To TRIF / ROLE Rx Data Interfaces
    //------------------------------------------------------
    //-- THIS / Trif / Rx Notification / Axis
    .soTRIF_Notif_TREADY       (sTRIF_Toe_Notif_Axis_tready),
    .soTRIF_Notif_TDATA        (sTOE_Trif_Notif_Axis_tdata),
    .soTRIF_Notif_TVALID       (sTOE_Trif_Notif_Axis_tvalid),  
    //-- THIS / Trif / Rx Data Reply / Axis
    .soTRIF_Data_TREADY        (sTRIF_Toe_Data_Axis_tready),
    .soTRIF_Data_TDATA         (sTOE_Trif_Data_Axis_tdata),
    .soTRIF_Data_TKEEP         (sTOE_Trif_Data_Axis_tkeep),
    .soTRIF_Data_TLAST         (sTOE_Trif_Data_Axis_tlast),
    .soTRIF_Data_TVALID        (sTOE_Trif_Data_Axis_tvalid),
    //-- THIS / Trif / Rx MetaData Reply / Axis
    .soTRIF_Meta_TREADY        (sTRIF_Toe_Meta_Axis_tready),
    .soTRIF_Meta_TDATA         (sTOE_Trif_Meta_Axis_tdata),
    .soTRIF_Meta_TVALID        (sTOE_Trif_Meta_Axis_tvalid),
                        
    //------------------------------------------------------
    //-- From TRIF / ROLE Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- TRIF / This / Rx Listen Port Request / Axis
    .siTRIF_LsnReq_TDATA       (sTRIF_Toe_LsnReq_Axis_tdata),
    .siTRIF_LsnReq_TVALID      (sTRIF_Toe_LsnReq_Axis_tvalid),
    .siTRIF_LsnReq_TREADY      (sTOE_Trif_LsnReq_Axis_tready),

    //------------------------------------------------------
    //-- To TRIF / ROLE Rx Listen Interface
    //------------------------------------------------------
    // THIS / Trif / Rx Listen Port Acknowledge / Axis
    .soTRIF_LsnAck_TREADY      (sTRIF_Toe_LsnAck_Axis_tready),
    .soTRIF_LsnAck_TDATA       (sTOE_Trif_LsnAck_Axis_tdata),
    .soTRIF_LsnAck_TVALID      (sTOE_Trif_LsnAck_Axis_tvalid),
                                               
    //------------------------------------------------------
    //-- From TRIF / ROLE Tx Data Interfaces
    //------------------------------------------------------
    //-- TRIF / This / Tx Data / Axis
    .siTRIF_Data_TDATA         (sTRIF_Toe_Data_Axis_tdata),
    .siTRIF_Data_TKEEP         (sTRIF_Toe_Data_Axis_tkeep),
    .siTRIF_Data_TLAST         (sTRIF_Toe_Data_Axis_tlast),
    .siTRIF_Data_TVALID        (sTRIF_Toe_Data_Axis_tvalid),
    .siTRIF_Data_TREADY        (sTOE_Trif_Data_Axis_tready),
    //-- TRIF / This / Tx MetaData / Axis
    .siTRIF_Meta_TDATA         (sTRIF_Toe_Meta_Axis_tdata),
    .siTRIF_Meta_TVALID        (sTRIF_Toe_Meta_Axis_tvalid),
    .siTRIF_Meta_TREADY        (sTOE_Trif_Meta_Axis_tready),
                        
    //------------------------------------------------------
    //-- To TRIF / ROLE Tx Data Interfaces
    //------------------------------------------------------                 
    //-- THIS / Trif / Tx Data Status / Axis
    .soTRIF_DSts_TREADY        (sTRIF_Toe_DSts_Axis_tready),
    .soTRIF_DSts_TDATA         (sTOE_Trif_DSts_Axis_tdata),
    .soTRIF_DSts_TVALID        (sTOE_Trif_DSts_Axis_tvalid),    
                        
    //------------------------------------------------------
    //-- From TRIF / ROLE Tx Ctrl Interfaces
    //------------------------------------------------------
    //-- TRIF / This / Tx Open Connection Request / Axis
    .siTRIF_OpnReq_TDATA       (sTRIF_Toe_OpnReq_Axis_tdata),
    .siTRIF_OpnReq_TVALID      (sTRIF_Toe_OpnReq_Axis_tvalid),
    .siTRIF_OpnReq_TREADY      (sTOE_Trif_OpnReq_Axis_tready),
    //-- THIS / Trif / Tx Open Connection Status / Axis
    .soTRIF_OpnSts_TREADY      (sTRIF_Toe_OpnSts_Axis_tready),
    .soTRIF_OpnSts_TDATA       (sTOE_Trif_OpnSts_Axis_tdata),
    .soTRIF_OpnSts_TVALID      (sTOE_Trif_OpnSts_Axis_tvalid),

    //------------------------------------------------------
    //-- To TRIF / ROLE Tx Ctrl Interfaces
    //------------------------------------------------------
    //-- TRIF / This / Tx Close Connection Request / Axis
    .siTRIF_ClsReq_TDATA       (sTRIF_Toe_ClsReq_Axis_tdata),
    .siTRIF_ClsReq_TVALID      (sTRIF_Toe_ClsReq_Axis_tvalid),
    .siTRIF_ClsReq_TREADY      (sTOE_Trif_ClsReq_Axis_tready),
    //--  THIS / Trif / Tx Close Connection Request / Axis
    // [INFO] Not used                                 
   
    //------------------------------------------------------
    //-- MEM / Nts0 / RxP Interface
    //------------------------------------------------------
    //-- Receive Path / S2MM-AXIS --------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used                                 
    //---- Stream Read Command -----------------
    .soMEM_RxP_RdCmd_TREADY    (piMEM_Nts0_RxP_Axis_RdCmd_tready),
    .soMEM_RxP_RdCmd_TDATA     (poNTS0_Mem_RxP_Axis_RdCmd_tdata),
    .soMEM_RxP_RdCmd_TVALID    (poNTS0_Mem_RxP_Axis_RdCmd_tvalid),
    //---- Stream Data Input Channel -----------
    .siMEM_RxP_Data_TDATA      (piMEM_Nts0_RxP_Axis_Data_tdata),
    .siMEM_RxP_Data_TKEEP      (piMEM_Nts0_RxP_Axis_Data_tkeep),
    .siMEM_RxP_Data_TLAST      (piMEM_Nts0_RxP_Axis_Data_tlast),
    .siMEM_RxP_Data_TVALID     (piMEM_Nts0_RxP_Axis_Data_tvalid),  
    .siMEM_RxP_Data_TREADY     (poNTS0_Mem_RxP_Axis_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_RxP_WrSts_TDATA     (piMEM_Nts0_RxP_Axis_WrSts_tdata),
    .siMEM_RxP_WrSts_TVALID    (piMEM_Nts0_RxP_Axis_WrSts_tvalid), 
    .siMEM_RxP_WrSts_TREADY    (poNTS0_Mem_RxP_Axis_WrSts_tready),                        
    //---- Stream Write Command ----------------
    .soMEM_RxP_WrCmd_TREADY    (piMEM_Nts0_RxP_Axis_WrCmd_tready),
    .soMEM_RxP_WrCmd_TDATA     (poNTS0_Mem_RxP_Axis_WrCmd_tdata),
    .soMEM_RxP_WrCmd_TVALID    (poNTS0_Mem_RxP_Axis_WrCmd_tvalid),
    //---- Stream Data Output Channel ----------
    .soMEM_RxP_Data_TREADY     (piMEM_Nts0_RxP_Axis_Data_tready),
    .soMEM_RxP_Data_TDATA      (poNTS0_Mem_RxP_Axis_Data_tdata),
    .soMEM_RxP_Data_TKEEP      (poNTS0_Mem_RxP_Axis_Data_tkeep),
    .soMEM_RxP_Data_TLAST      (poNTS0_Mem_RxP_Axis_Data_tlast),
    .soMEM_RxP_Data_TVALID     (poNTS0_Mem_RxP_Axis_Data_tvalid),

    //------------------------------------------------------
    //-- MEM / Nts0 / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS -------------------------
    //---- Stream Read Status ------------------
    // [INFO] Not used
    //---- Stream Read Command -----------------
    .soMEM_TxP_RdCmd_TREADY    (piMEM_Nts0_TxP_Axis_RdCmd_tready),
    .soMEM_TxP_RdCmd_TDATA     (poNTS0_Mem_TxP_Axis_RdCmd_tdata),
    .soMEM_TxP_RdCmd_TVALID    (poNTS0_Mem_TxP_Axis_RdCmd_tvalid),
    //---- Stream Data Input Channel ----------- 
    .siMEM_TxP_Data_TDATA      (piMEM_Nts0_TxP_Axis_Data_tdata),
    .siMEM_TxP_Data_TKEEP      (piMEM_Nts0_TxP_Axis_Data_tkeep),
    .siMEM_TxP_Data_TLAST      (piMEM_Nts0_TxP_Axis_Data_tlast),
    .siMEM_TxP_Data_TVALID     (piMEM_Nts0_TxP_Axis_Data_tvalid),
    .siMEM_TxP_Data_TREADY     (poNTS0_Mem_TxP_Axis_Data_tready),
    //---- Stream Write Status -----------------
    .siMEM_TxP_WrSts_TDATA     (piMEM_Nts0_TxP_Axis_WrSts_tdata),
    .siMEM_TxP_WrSts_TVALID    (piMEM_Nts0_TxP_Axis_WrSts_tvalid),
    .siMEM_TxP_WrSts_TREADY    (poNTS0_Mem_TxP_Axis_WrSts_tready),
    //---- Stream Write Command ----------------
    .soMEM_TxP_WrCmd_TREADY    (piMEM_Nts0_TxP_Axis_WrCmd_tready),
    .soMEM_TxP_WrCmd_TDATA     (poNTS0_Mem_TxP_Axis_WrCmd_tdata),
    .soMEM_TxP_WrCmd_TVALID    (poNTS0_Mem_TxP_Axis_WrCmd_tvalid),
    //---- Stream Data Output Channel ----------
    .soMEM_TxP_Data_TREADY     (piMEM_Nts0_TxP_Axis_Data_tready),
    .soMEM_TxP_Data_TDATA      (poNTS0_Mem_TxP_Axis_Data_tdata),
    .soMEM_TxP_Data_TKEEP      (poNTS0_Mem_TxP_Axis_Data_tkeep),
    .soMEM_TxP_Data_TLAST      (poNTS0_Mem_TxP_Axis_Data_tlast),
    .soMEM_TxP_Data_TVALID     (poNTS0_Mem_TxP_Axis_Data_tvalid),

    //------------------------------------------------------
    //-- From CAM / Session Lookup & Update Interfaces
    //------------------------------------------------------
    //-- CAM / This / LookupReply / Axis
    .siCAM_SssLkpRep_TDATA     (sCAM_Toe_LkpRep_Axis_tdata),
    .siCAM_SssLkpRep_TVALID    (sCAM_Toe_LkpRep_Axis_tvalid),
    .siCAM_SssLkpRep_TREADY    (sTOE_Cam_LkpRep_Axis_tready),
    //-- CAM / This / UpdateReply /Axis
    .siCAM_SssUpdRep_TDATA     (sCAM_Toe_UpdRpl_Axis_tdata),
    .siCAM_SssUpdRep_TVALID    (sCAM_Toe_UpdRpl_Axis_tvalid),
    .siCAM_SssUpdRep_TREADY    (sTOE_Cam_UpdRpl_Axis_tready),

    //------------------------------------------------------
    //-- To CAM Interfaces
    //------------------------------------------------------
    //-- THIS / Cam / LookupRequest / Axis
    .soCAM_SssLkpReq_TREADY    (sCAM_Toe_LkpReq_Axis_tready),
    .soCAM_SssLkpReq_TDATA     (sTOE_Cam_LkpReq_Axis_tdata),
    .soCAM_SssLkpReq_TVALID    (sTOE_Cam_LkpReq_Axis_tvalid),
    //-- THIS / Cam / UpdateRequest / Axis
    .soCAM_SssUpdReq_TREADY    (sCAM_Toe_UpdReq_Axis_tready),
    .soCAM_SssUpdReq_TDATA     (sTOE_Cam_UpdReq_Axis_tdata),
    .soCAM_SssUpdReq_TVALID    (sTOE_Cam_UpdReq_Axis_tvalid),

    //------------------------------------------------------
    //-- To DEBUG / Session Statistics Interfaces
    //------------------------------------------------------
    .poDBG_SssRelCnt_V         (),
    .poDBG_SssRegCnt_V         ()

  );  // End of TOE
  
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_tcp_module_1_01 TCP ( 
//    // Data output
//    .m_axis_tcp_data_TVALID(toe_to_iph_slice_tvalid), // output AXI_M_Stream_TVALID
//    .m_axis_tcp_data_TREADY(toe_to_iph_slice_tready), // input AXI_M_Stream_TREADY
//    .m_axis_tcp_data_TDATA(toe_to_iph_slice_tdata),   // output [63 : 0] AXI_M_Stream_TDATA
//    .m_axis_tcp_data_TKEEP(toe_to_iph_slice_tkeep),   // output [7 : 0] AXI_M_Stream_TSTRB    
//    .m_axis_tcp_data_TLAST(toe_to_iph_slice_tlast),   // output [0 : 0] AXI_M_Stream_TLAST

//    // Data input
//    .s_axis_tcp_data_TVALID(iph_to_toe_slice_tvalid), // input AXI_S_Stream_TVALID
//    .s_axis_tcp_data_TREADY(iph_to_toe_slice_tready), // output AXI_S_Stream_TREADY
//    .s_axis_tcp_data_TDATA(iph_to_toe_slice_tdata), // input [63 : 0] AXI_S_Stream_TDATA
//    .s_axis_tcp_data_TKEEP(iph_to_toe_slice_tkeep), // input [7 : 0] AXI_S_Stream_TKEEP
//    .s_axis_tcp_data_TLAST(iph_to_toe_slice_tlast), // input [0 : 0] AXI_S_Stream_TLAST

//    // rx read commands
//    .m_axis_rxread_cmd_TVALID(TOE_RX_c0_ddr3_s_axis_read_cmd_tvalid), 
//    .m_axis_rxread_cmd_TREADY(TOE_RX_c0_ddr3_s_axis_read_cmd_tready),
//    .m_axis_rxread_cmd_TDATA(TOE_RX_c0_ddr3_s_axis_read_cmd_tdata),
//    // rx write commands
//    .m_axis_rxwrite_cmd_TVALID(TOE_RX_c0_ddr3_s_axis_write_cmd_tvalid),
//    .m_axis_rxwrite_cmd_TREADY(TOE_RX_c0_ddr3_s_axis_write_cmd_tready),
//    .m_axis_rxwrite_cmd_TDATA(TOE_RX_c0_ddr3_s_axis_write_cmd_tdata),
//    // rx write status
//    .s_axis_rxwrite_sts_TVALID(TOE_RX_c0_ddr3_m_axis_write_sts_tvalid), 
//    .s_axis_rxwrite_sts_TREADY(TOE_RX_c0_ddr3_m_axis_write_sts_tready),
//    .s_axis_rxwrite_sts_TDATA(TOE_RX_c0_ddr3_m_axis_write_sts_tdata),
//    // rx buffer read path
//    .s_axis_rxread_data_TVALID(TOE_RX_c0_ddr3_m_axis_read_tvalid),  
//    .s_axis_rxread_data_TREADY(TOE_RX_c0_ddr3_m_axis_read_tready),
//    .s_axis_rxread_data_TDATA(TOE_RX_c0_ddr3_m_axis_read_tdata),
//    .s_axis_rxread_data_TKEEP(TOE_RX_c0_ddr3_m_axis_read_tkeep),
//    .s_axis_rxread_data_TLAST(TOE_RX_c0_ddr3_m_axis_read_tlast),

//    // rx buffer write path
//    .m_axis_rxwrite_data_TVALID(TOE_RX_c0_ddr3_s_axis_write_tvalid),
//    .m_axis_rxwrite_data_TREADY(TOE_RX_c0_ddr3_s_axis_write_tready),
//    .m_axis_rxwrite_data_TDATA(TOE_RX_c0_ddr3_s_axis_write_tdata),
//    .m_axis_rxwrite_data_TKEEP(TOE_RX_c0_ddr3_s_axis_write_tkeep),
//    .m_axis_rxwrite_data_TLAST(TOE_RX_c0_ddr3_s_axis_write_tlast),

//    // tx read commands
//    .m_axis_txread_cmd_TVALID(TOE_TX_c0_ddr3_s_axis_read_cmd_tvalid),
//    .m_axis_txread_cmd_TREADY(TOE_TX_c0_ddr3_s_axis_read_cmd_tready),
//    .m_axis_txread_cmd_TDATA(TOE_TX_c0_ddr3_s_axis_read_cmd_tdata),
//    //tx write commands
//    .m_axis_txwrite_cmd_TVALID(TOE_TX_c0_ddr3_s_axis_write_cmd_tvalid),
//    .m_axis_txwrite_cmd_TREADY(TOE_TX_c0_ddr3_s_axis_write_cmd_tready),
//    .m_axis_txwrite_cmd_TDATA(TOE_TX_c0_ddr3_s_axis_write_cmd_tdata),
//    // tx write status
//    .s_axis_txwrite_sts_TVALID(TOE_TX_c0_ddr3_m_axis_write_sts_tvalid),
//    .s_axis_txwrite_sts_TREADY(TOE_TX_c0_ddr3_m_axis_write_sts_tready),
//    .s_axis_txwrite_sts_TDATA(TOE_TX_c0_ddr3_m_axis_write_sts_tdata),
//    // tx read path
//    .s_axis_txread_data_TVALID(TOE_TX_c0_ddr3_m_axis_read_tvalid),
//    .s_axis_txread_data_TREADY(TOE_TX_c0_ddr3_m_axis_read_tready),
//    .s_axis_txread_data_TDATA(TOE_TX_c0_ddr3_m_axis_read_tdata),
//    .s_axis_txread_data_TKEEP(TOE_TX_c0_ddr3_m_axis_read_tkeep),
//    .s_axis_txread_data_TLAST(TOE_TX_c0_ddr3_m_axis_read_tlast),
//    // tx write path
//    .m_axis_txwrite_data_TVALID(TOE_TX_c0_ddr3_s_axis_write_tvalid),
//    .m_axis_txwrite_data_TREADY(TOE_TX_c0_ddr3_s_axis_write_tready),
//    .m_axis_txwrite_data_TDATA(TOE_TX_c0_ddr3_s_axis_write_tdata),
//    .m_axis_txwrite_data_TKEEP(TOE_TX_c0_ddr3_s_axis_write_tkeep),
//    .m_axis_txwrite_data_TLAST(TOE_TX_c0_ddr3_s_axis_write_tlast),

//    /// SmartCAM I/F update
//    .m_axis_session_upd_req_TVALID(upd_req_TVALID),
//    .m_axis_session_upd_req_TREADY(upd_req_TREADY),
//    .m_axis_session_upd_req_TDATA(upd_req_TDATA),

//    .s_axis_session_upd_rsp_TVALID(upd_rsp_TVALID),
//    .s_axis_session_upd_rsp_TREADY(upd_rsp_TREADY),
//    .s_axis_session_upd_rsp_TDATA(upd_rsp_TDATA),

//    /// SmartCAM I/F lookup
//    .m_axis_session_lup_req_TVALID(lup_req_TVALID),
//    .m_axis_session_lup_req_TREADY(lup_req_TREADY),
//    .m_axis_session_lup_req_TDATA(lup_req_TDATA),
    
//    .s_axis_session_lup_rsp_TVALID(lup_rsp_TVALID),
//    .s_axis_session_lup_rsp_TREADY(lup_rsp_TREADY),
//    .s_axis_session_lup_rsp_TDATA(lup_rsp_TDATA),

//    // Application Interface 
//    // listen&close port
//    .s_axis_listen_port_req_TVALID(axis_listen_port_TVALID),
//    .s_axis_listen_port_req_TREADY(axis_listen_port_TREADY),
//    .s_axis_listen_port_req_TDATA(axis_listen_port_TDATA),

//    .m_axis_listen_port_rsp_TVALID(axis_listen_port_status_TVALID),
//    .m_axis_listen_port_rsp_TREADY(axis_listen_port_status_TREADY),
//    .m_axis_listen_port_rsp_TDATA(axis_listen_port_status_TDATA),

//    // notification & read request
//    .m_axis_notification_TVALID(axis_notifications_TVALID),
//    .m_axis_notification_TREADY(axis_notifications_TREADY),
//    .m_axis_notification_TDATA(axis_notifications_TDATA),

//    .s_axis_rx_data_req_TVALID(axis_read_package_TVALID),
//    .s_axis_rx_data_req_TREADY(axis_read_package_TREADY),
//    .s_axis_rx_data_req_TDATA(axis_read_package_TDATA),

//    // open&close connection
//    .s_axis_open_conn_req_TVALID(axis_open_connection_TVALID),
//    .s_axis_open_conn_req_TREADY(axis_open_connection_TREADY),
//    .s_axis_open_conn_req_TDATA(axis_open_connection_TDATA),

//    .m_axis_open_conn_rsp_TVALID(axis_open_status_TVALID),
//    .m_axis_open_conn_rsp_TREADY(axis_open_status_TREADY),
//    .m_axis_open_conn_rsp_TDATA(axis_open_status_TDATA),

//    .s_axis_close_conn_req_TVALID(axis_close_connection_TVALID),//axis_close_connection_TVALID
//    .s_axis_close_conn_req_TREADY(axis_close_connection_TREADY),
//    .s_axis_close_conn_req_TDATA(axis_close_connection_TDATA),

//    // rx data
//    .m_axis_rx_data_rsp_metadata_TVALID(axis_rx_metadata_TVALID),
//    .m_axis_rx_data_rsp_metadata_TREADY(axis_rx_metadata_TREADY),
//    .m_axis_rx_data_rsp_metadata_TDATA(axis_rx_metadata_TDATA),
    
//    .m_axis_rx_data_rsp_TVALID(axis_rx_data_TVALID),
//    .m_axis_rx_data_rsp_TREADY(axis_rx_data_TREADY),
//    .m_axis_rx_data_rsp_TDATA(axis_rx_data_TDATA),
//    .m_axis_rx_data_rsp_TKEEP(axis_rx_data_TKEEP),
//    .m_axis_rx_data_rsp_TLAST(axis_rx_data_TLAST),

//    // tx data
//    .s_axis_tx_data_req_metadata_TVALID(axis_tx_metadata_TVALID),
//    .s_axis_tx_data_req_metadata_TREADY(axis_tx_metadata_TREADY),
//    .s_axis_tx_data_req_metadata_TDATA(axis_tx_metadata_TDATA),

//    .s_axis_tx_data_req_TVALID(axis_tx_data_TVALID),
//    .s_axis_tx_data_req_TREADY(axis_tx_data_TREADY),
//    .s_axis_tx_data_req_TDATA(axis_tx_data_TDATA),
//    .s_axis_tx_data_req_TKEEP(axis_tx_data_TKEEP),
//    .s_axis_tx_data_req_TLAST(axis_tx_data_TLAST),

//    .m_axis_tx_data_rsp_TVALID(axis_tx_status_TVALID),
//    .m_axis_tx_data_rsp_TREADY(axis_tx_status_TREADY),
//    .m_axis_tx_data_rsp_TDATA(axis_tx_status_TDATA),

//    // Debug signals //
//    ////////////////////
//    .regIpAddress_V(cloud_fpga_ip),             // input wire [31 : 0] regIpAddress_V
//    .relSessionCount_V(relSessionCount),        // output wire [15 : 0] relSessionCount_V
//    .regSessionCount_V(regSessionCount),        // output wire [15 : 0] regSessionCount_V
//    .aclk(cf_axi_clk),                          // input aclk
//    .aresetn(cf_aresetn)                        // input aresetn
 
//  );  // End of TCP
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: TOE-CAM-MODULE
  //============================================================================
  ToeCam CAM (
  
    .clk                          (piShlClk),
    .rst                          (~piShlRst),

    //------------------------------------------------------
    //-- From TOE Interfaces
    //------------------------------------------------------
    //-- TOE / This / LookupRequest / Axis
    .lup_req_din                  (sTOE_Cam_LkpReq_Axis_tdata),
    .lup_req_valid                (sTOE_Cam_LkpReq_Axis_tvalid),
    .lup_req_ready                (sCAM_Toe_LkpReq_Axis_tready),
    //-- TOE / This / UpdateRequest / Axis
    .upd_req_din                  (sTOE_Cam_UpdReq_Axis_tdata),
    .upd_req_valid                (sTOE_Cam_UpdReq_Axis_tvalid),
    .upd_req_ready                (sCAM_Toe_UpdReq_Axis_tready),
    
    //------------------------------------------------------
    //-- To TOE Interfaces
    //------------------------------------------------------
    //-- THIS / Toe / LookupReply / Axis
    .lup_rsp_ready                (sTOE_Cam_LkpRep_Axis_tready),
    .lup_rsp_dout                 (sCAM_Toe_LkpRep_Axis_tdata),
    .lup_rsp_valid                (sCAM_Toe_LkpRep_Axis_tvalid),
    //-- THIS / Toe / UpdateReply / Axis
    .upd_rsp_ready                (sTOE_Cam_UpdRpl_Axis_tready),
    .upd_rsp_dout                 (sCAM_Toe_UpdRpl_Axis_tdata),
    .upd_rsp_valid                (sCAM_Toe_UpdRpl_Axis_tvalid),

    .led0                         (),
    .led1                         (),
    .cam_ready                    (),

    .debug()

  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  SmartCamCtl CAM (
//    .clk(cf_axi_clk),
//    .rst(~cf_aresetn),
//    .led0(sc_led0),
//    .led1(sc_led1),
//    .cam_ready(cam_ready),

//    .lup_req_valid(lup_req_TVALID),
//    .lup_req_ready(lup_req_TREADY),
//    .lup_req_din(lup_req_TDATA),

//    .lup_rsp_valid(lup_rsp_TVALID),
//    .lup_rsp_ready(lup_rsp_TREADY),
//    .lup_rsp_dout(lup_rsp_TDATA),

//    .upd_req_valid(upd_req_TVALID),
//    .upd_req_ready(upd_req_TREADY),
//    .upd_req_din(upd_req_TDATA),

//    .upd_rsp_valid(upd_rsp_TVALID),
//    .upd_rsp_ready(upd_rsp_TREADY),
//    .upd_rsp_dout(upd_rsp_TDATA),

//    .debug()
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (IPRX ==> TOE)
  //============================================================================
  AxisRegisterSlice_64 ARS2 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From IPRX / Toe / Axis 
    .s_axis_tdata   (sIPRX_Toe_Axis_tdata),
    .s_axis_tkeep   (sIPRX_Toe_Axis_tkeep),
    .s_axis_tlast   (sIPRX_Toe_Axis_tlast),
    .s_axis_tvalid  (sIPRX_Toe_Axis_tvalid),
    .s_axis_tready  (sTOE_Iprx_Axis_treadyReg),
    //-- To TOE / Iprx / Axis         
    .m_axis_tready  (sTOE_Iprx_Axis_tready),
    .m_axis_tdata   (sIPRX_Toe_Axis_tdataReg),
    .m_axis_tkeep   (sIPRX_Toe_Axis_tkeepReg),
    .m_axis_tlast   (sIPRX_Toe_Axis_tlastReg),
    .m_axis_tvalid  (sIPRX_Toe_Axis_tvalidReg)
  ); 
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS2 (
//   .aclk(cf_axi_clk),
//   .aresetn(cf_aresetn),
     
//   .s_axis_tvalid(axi_iph_to_toe_tvalid),
//   .s_axis_tready(axi_iph_to_toe_tready),
//   .s_axis_tdata(axi_iph_to_toe_tdata),
//   .s_axis_tkeep(axi_iph_to_toe_tkeep),
//   .s_axis_tlast(axi_iph_to_toe_tlast),
     
//   .m_axis_tvalid(iph_to_toe_slice_tvalid),
//   .m_axis_tready(iph_to_toe_slice_tready),
//   .m_axis_tdata(iph_to_toe_slice_tdata),
//   .m_axis_tkeep(iph_to_toe_slice_tkeep),
//   .m_axis_tlast(iph_to_toe_slice_tlast)
//  ); 
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (TCP ==> IPTX)
  //============================================================================
  AxisRegisterSlice_64 ARS3 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From TOE / L3mux / Axis
    .s_axis_tdata   (sTOE_L3mux_Axis_tdata),
    .s_axis_tkeep   (sTOE_L3mux_Axis_tkeep),
    .s_axis_tlast   (sTOE_L3mux_Axis_tlast),
    .s_axis_tvalid  (sTOE_L3mux_Axis_tvalid),
    .s_axis_tready  (sL3MUX_Toe_Axis_treadyReg),
    //-- To L3MUX / Toe / Axis        
    .m_axis_tready  (sL3MUX_Toe_Axix_tready),
    .m_axis_tdata   (sTOE_L3mux_Axix_tdataReg),
    .m_axis_tkeep   (sTOE_L3mux_Axix_tkeepReg),
    .m_axis_tlast   (sTOE_L3mux_Axix_tlastReg),
    .m_axis_tvalid  (sTOE_L3mux_Axix_tvalidReg)
  ); 
   
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS3 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
        
//    .s_axis_tvalid(toe_to_iph_slice_tvalid),
//    .s_axis_tready(toe_to_iph_slice_tready),
//    .s_axis_tdata(toe_to_iph_slice_tdata),
//    .s_axis_tkeep(toe_to_iph_slice_tkeep),
//    .s_axis_tlast(toe_to_iph_slice_tlast),
        
//    .m_axis_tvalid(axi_toe_to_toe_slice_tvalid),
//    .m_axis_tready(axi_toe_to_toe_slice_tready),
//    .m_axis_tdata(axi_toe_to_toe_slice_tdata),
//    .m_axis_tkeep(axi_toe_to_toe_slice_tkeep),
//    .m_axis_tlast(axi_toe_to_toe_slice_tlast)
//  ); 
 -----/\----- [OBSOLETE] -----/\----- */
   
  //============================================================================
  //  INST: TCP-ROLE-INTERFACE
  //============================================================================
  TcpRoleInterface TRIF (
  
    .aclk                        (piShlClk),
    .aresetn                     (~piShlRst),
  
    //------------------------------------------------------
    //-- SHELL / System Reset
    //------------------------------------------------------
    .piSHL_SysRst_n_V            (~piShlRst),
  
    //------------------------------------------------------
    //-- From ROLE / Rx Data Interface
    //------------------------------------------------------
    //-- ROLE / This / Tcp / Axis
    .siROL_Data_TDATA            (sROL_Nts0_Tcp_Axis_tdataReg),
    .siROL_Data_TKEEP            (sROL_Nts0_Tcp_Axis_tkeepReg),
    .siROL_Data_TLAST            (sROL_Nts0_Tcp_Axis_tlastReg),
    .siROL_Data_TVALID           (sROL_Nts0_Tcp_Axis_tvalidReg),
    .siROL_Data_TREADY           (sTRIF_Rol_Axis_tready),

    //------------------------------------------------------
    //-- To ROLE / Tx Data Interface
    //------------------------------------------------------
    //-- THIS / Role / Tcp / Axis
    .soROL_Data_TREADY           (sROL_Trif_Axis_treadyReg),
    .soROL_Data_TDATA            (sTRIF_Rol_Axis_tdata),
    .soROL_Data_TKEEP            (sTRIF_Rol_Axis_tkeep),
    .soROL_Data_TLAST            (sTRIF_Rol_Axis_tlast),
    .soROL_Data_TVALID           (sTRIF_Rol_Axis_tvalid),
  
    //------------------------------------------------------
    //-- From TOE / Rx Data Interfaces
    //------------------------------------------------------
    //-- TOE / This / Rx Data Notification / Axis
    .siTOE_Notif_TDATA           (sTOE_Trif_Notif_Axis_tdata),
    .siTOE_Notif_TVALID          (sTOE_Trif_Notif_Axis_tvalid),
    .siTOE_Notif_TREADY          (sTRIF_Toe_Notif_Axis_tready),
    //-- TOE / This / Rx Data / Axis
    .siTOE_Data_TDATA            (sTOE_Trif_Data_Axis_tdata),
    .siTOE_Data_TKEEP            (sTOE_Trif_Data_Axis_tkeep),
    .siTOE_Data_TLAST            (sTOE_Trif_Data_Axis_tlast),
    .siTOE_Data_TVALID           (sTOE_Trif_Data_Axis_tvalid),
    .siTOE_Data_TREADY           (sTRIF_Toe_Data_Axis_tready),
    //-- TOE / This / Rx MetaData / Axis
    .siTOE_Meta_TDATA            (sTOE_Trif_Meta_Axis_tdata),
    .siTOE_Meta_TVALID           (sTOE_Trif_Meta_Axis_tvalid),
    .siTOE_Meta_TREADY           (sTRIF_Toe_Meta_Axis_tready),

    //------------------------------------------------------
    //-- To TOE / Rx Data Interfaces
    //------------------------------------------------------                         
    //-- THIS / Toe / ReadRequest / Axis
    .soTOE_DReq_TREADY          (sTOE_Trif_DReq_Axis_tready),
    .soTOE_DReq_TDATA           (sTRIF_Toe_DReq_Axis_tdata),
    .soTOE_DReq_TVALID          (sTRIF_Toe_DReq_Axis_tvalid),

    //------------------------------------------------------
    //-- From TOE / Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- TOE / This / ListenAcknowledge / Axis
    .siTOE_LsnAck_TDATA          (sTOE_Trif_LsnAck_Axis_tdata),
    .siTOE_LsnAck_TVALID         (sTOE_Trif_LsnAck_Axis_tvalid),
    .siTOE_LsnAck_TREADY         (sTRIF_Toe_LsnAck_Axis_tready),
 
    //------------------------------------------------------
    //-- To TOE / Rx Ctrl Interfaces
    //------------------------------------------------------
    //-- THIS / Toe / ListenRequest / Axis
    .soTOE_LsnReq_TREADY         (sTOE_Trif_LsnReq_Axis_tready),
    .soTOE_LsnReq_TDATA          (sTRIF_Toe_LsnReq_Axis_tdata),
    .soTOE_LsnReq_TVALID         (sTRIF_Toe_LsnReq_Axis_tvalid),
                            
    //------------------------------------------------------
    //-- From TOE / Tx Data Interfaces
    //------------------------------------------------------
    //-- TOE / This / Tx Data Status / Axis
    .siTOE_DSts_TDATA            (sTOE_Trif_DSts_Axis_tdata),
    .siTOE_DSts_TVALID           (sTOE_Trif_DSts_Axis_tvalid),
    .siTOE_DSts_TREADY           (sTRIF_Toe_DSts_Axis_tready),

    //------------------------------------------------------
    //-- To TOE / Tx Data Interfaces
    //------------------------------------------------------
    //-- THIS / Toe / Tx Data / Axis 
    .soTOE_Data_TREADY           (sTOE_Trif_Data_Axis_tready),
    .soTOE_Data_TDATA            (sTRIF_Toe_Data_Axis_tdata),
    .soTOE_Data_TKEEP            (sTRIF_Toe_Data_Axis_tkeep),
    .soTOE_Data_TLAST            (sTRIF_Toe_Data_Axis_tlast),
    .soTOE_Data_TVALID           (sTRIF_Toe_Data_Axis_tvalid),
    //-- THIS / Toe / MetaData / Axis
    .soTOE_Meta_TREADY           (sTOE_Trif_Meta_Axis_tready),
    .soTOE_Meta_TDATA            (sTRIF_Toe_Meta_Axis_tdata),
    .soTOE_Meta_TVALID           (sTRIF_Toe_Meta_Axis_tvalid),

    //------------------------------------------------------
    //-- From TOE / Tx Ctrl Interfaces
    //------------------------------------------------------
    //-- TOE / This / Tx Open Status / Axis  
    .siTOE_OpnSts_TDATA          (sTOE_Trif_OpnSts_Axis_tdata),
    .siTOE_OpnSts_TVALID         (sTOE_Trif_OpnSts_Axis_tvalid),
    .siTOE_OpnSts_TREADY         (sTRIF_Toe_OpnSts_Axis_tready),

    //------------------------------------------------------
    //-- To TOE / This / Tx Ctrl Interfaces
    //------------------------------------------------------
    //-- THIS / Toe / Tx Open Request / Axis
    .soTOE_OpnReq_TREADY         (sTOE_Trif_OpnReq_Axis_tready),
    .soTOE_OpnReq_TDATA          (sTRIF_Toe_OpnReq_Axis_tdata),
    .soTOE_OpnReq_TVALID         (sTRIF_Toe_OpnReq_Axis_tvalid),
                         
    //------------------------------------------------------
    //-- To TOE / Close-Connection
    //------------------------------------------------------                        
    //-- THIS / Toe / CloseRequest / Axis
    .soTOE_ClsReq_TREADY         (sTOE_Trif_ClsReq_Axis_tready),
    .soTOE_ClsReq_TDATA          (sTRIF_Toe_ClsReq_Axis_tdata),
    .soTOE_ClsReq_TVALID         (sTRIF_Toe_ClsReq_Axis_tvalid)
     
  );
      
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_tcp_app_interface_18 TRIF (
//    .m_axis_close_connection_TVALID(axis_close_connection_TVALID),
//    .m_axis_close_connection_TREADY(axis_close_connection_TREADY),
//    .m_axis_close_connection_TDATA(axis_close_connection_TDATA),
    
//    .m_axis_listen_port_TVALID(axis_listen_port_TVALID),
//    .m_axis_listen_port_TREADY(axis_listen_port_TREADY),
//    .m_axis_listen_port_TDATA(axis_listen_port_TDATA),
    
//    .m_axis_open_connection_TVALID(axis_open_connection_TVALID),
//    .m_axis_open_connection_TREADY(axis_open_connection_TREADY),
//    .m_axis_open_connection_TDATA(axis_open_connection_TDATA),
    
//    .m_axis_read_request_TVALID(axis_read_package_TVALID),
//    .m_axis_read_request_TREADY(axis_read_package_TREADY),
//    .m_axis_read_request_TDATA(axis_read_package_TDATA),
    
//    .m_axis_tx_data_TVALID(axis_tx_data_TVALID),
//    .m_axis_tx_data_TREADY(axis_tx_data_TREADY),
//    .m_axis_tx_data_TDATA(axis_tx_data_TDATA),
//    .m_axis_tx_data_TKEEP(axis_tx_data_TKEEP),
//    .m_axis_tx_data_TLAST(axis_tx_data_TLAST),
    
//    .m_axis_tx_meta_data_TVALID(axis_tx_metadata_TVALID),
//    .m_axis_tx_meta_data_TREADY(axis_tx_metadata_TREADY),
//    .m_axis_tx_meta_data_TDATA(axis_tx_metadata_TDATA),
    
//    .m_axis_vfpga_rx_data_TVALID(net_to_rs_vFPGA_rx_data_TVALID),
//    .m_axis_vfpga_rx_data_TREADY(net_to_rs_vFPGA_rx_data_TREADY),
//    .m_axis_vfpga_rx_data_TDATA(net_to_rs_vFPGA_rx_data_TDATA),
//    .m_axis_vfpga_rx_data_TKEEP(net_to_rs_vFPGA_rx_data_TKEEP),
//    .m_axis_vfpga_rx_data_TLAST(net_to_rs_vFPGA_rx_data_TLAST),
    
//    .s_axis_listen_port_status_TVALID(axis_listen_port_status_TVALID),
//    .s_axis_listen_port_status_TREADY(axis_listen_port_status_TREADY),
//    .s_axis_listen_port_status_TDATA(axis_listen_port_status_TDATA),
    
//    .s_axis_notifications_TVALID(axis_notifications_TVALID),
//    .s_axis_notifications_TREADY(axis_notifications_TREADY),
//    .s_axis_notifications_TDATA(axis_notifications_TDATA),
    
//    .s_axis_open_connection_status_TVALID(axis_open_status_TVALID),
//    .s_axis_open_connection_status_TREADY(axis_open_status_TREADY),
//    .s_axis_open_connection_status_TDATA(axis_open_status_TDATA),
    
//    .s_axis_rx_data_TVALID(axis_rx_data_TVALID),
//    .s_axis_rx_data_TREADY(axis_rx_data_TREADY),
//    .s_axis_rx_data_TDATA(axis_rx_data_TDATA),
//    .s_axis_rx_data_TKEEP(axis_rx_data_TKEEP),
//    .s_axis_rx_data_TLAST(axis_rx_data_TLAST),
    
//    .s_axis_rx_meta_data_TVALID(axis_rx_metadata_TVALID),
//    .s_axis_rx_meta_data_TREADY(axis_rx_metadata_TREADY),
//    .s_axis_rx_meta_data_TDATA(axis_rx_metadata_TDATA),
    
//    .s_axis_tx_status_TVALID(axis_tx_status_TVALID),
//    .s_axis_tx_status_TREADY(axis_tx_status_TREADY),
//    .s_axis_tx_status_TDATA(axis_tx_status_TDATA),
    
//    .s_axis_vfpga_tx_data_TVALID(rs_to_net_vFPGA_tx_data_TVALID),
//    .s_axis_vfpga_tx_data_TREADY(rs_to_net_vFPGA_tx_data_TREADY),
//    .s_axis_vfpga_tx_data_TDATA(rs_to_net_vFPGA_tx_data_TDATA),
//    .s_axis_vfpga_tx_data_TKEEP(rs_to_net_vFPGA_tx_data_TKEEP),
//    .s_axis_vfpga_tx_data_TLAST(rs_to_net_vFPGA_tx_data_TLAST),
    
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn)
//  );
 -----/\----- [OBSOLETE] -----/\----- */
  
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (TRIF ==> NTS0/Role/Tcp)
  //============================================================================
  AxisRegisterSlice_64 ARS4 ( 
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From TRIF / Role / Tcp / Axis 
    .s_axis_tvalid  (sTRIF_Rol_Axis_tvalid),
    .s_axis_tdata   (sTRIF_Rol_Axis_tdata),
    .s_axis_tkeep   (sTRIF_Rol_Axis_tkeep),
    .s_axis_tlast   (sTRIF_Rol_Axis_tlast),
    .s_axis_tready  (sROL_Trif_Axis_treadyReg),

    //-- To NTS0 / Role / Tcp / Axis
    .m_axis_tready  (piROL_Nts0_Tcp_Axis_tready),
    .m_axis_tdata   (poNTS0_Rol_Tcp_Axis_tdata),
    .m_axis_tkeep   (poNTS0_Rol_Tcp_Axis_tkeep),
    .m_axis_tlast   (poNTS0_Rol_Tcp_Axis_tlast),
    .m_axis_tvalid  (poNTS0_Rol_Tcp_Axis_tvalid)
  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS4 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
       
//    .s_axis_tvalid(net_to_rs_vFPGA_rx_data_TVALID),
//    .s_axis_tready(net_to_rs_vFPGA_rx_data_TREADY),
//    .s_axis_tdata(net_to_rs_vFPGA_rx_data_TDATA),
//    .s_axis_tkeep(net_to_rs_vFPGA_rx_data_TKEEP),
//    .s_axis_tlast(net_to_rs_vFPGA_rx_data_TLAST),
       
//    .m_axis_tvalid(vFPGA_TCP_rx_data_TVALID),
//    .m_axis_tready(vFPGA_TCP_rx_data_TREADY),
//    .m_axis_tdata(vFPGA_TCP_rx_data_TDATA),
//    .m_axis_tkeep(vFPGA_TCP_rx_data_TKEEP),
//    .m_axis_tlast(vFPGA_TCP_rx_data_TLAST)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (ROLE/Nts0/Tcp ==> TRIF)
  //============================================================================
  AxisRegisterSlice_64 ARS5 (
    .aclk           (piShlClk),
    .aresetn        (~piShlRst),
    //-- From ROLE / Nts0 / Tcp / Axis -----------
    .s_axis_tdata   (piROL_Nts0_Tcp_Axis_tdata),
    .s_axis_tkeep   (piROL_Nts0_Tcp_Axis_tkeep),
    .s_axis_tlast   (piROL_Nts0_Tcp_Axis_tlast),
    .s_axis_tvalid  (piROL_Nts0_Tcp_Axis_tvalid),    
    .s_axis_tready  (poNTS0_Rol_Tcp_Axis_tready),
    //-- To TRFI / Role / Axis ------------------- 
    .m_axis_tready  (sTRIF_Rol_Axis_tready),   
    .m_axis_tdata   (sROL_Nts0_Tcp_Axis_tdataReg),
    .m_axis_tkeep   (sROL_Nts0_Tcp_Axis_tkeepReg),
    .m_axis_tlast   (sROL_Nts0_Tcp_Axis_tlastReg),
    .m_axis_tvalid  (sROL_Nts0_Tcp_Axis_tvalidReg)
  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS5 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
       
//    .s_axis_tvalid(vFPGA_TCP_tx_data_TVALID),
//    .s_axis_tready(vFPGA_TCP_tx_data_TREADY),
//    .s_axis_tdata(vFPGA_TCP_tx_data_TDATA),
//    .s_axis_tkeep(vFPGA_TCP_tx_data_TKEEP),
//    .s_axis_tlast(vFPGA_TCP_tx_data_TLAST),
       
//    .m_axis_tvalid(rs_to_net_vFPGA_tx_data_TVALID),
//    .m_axis_tready(rs_to_net_vFPGA_tx_data_TREADY),
//    .m_axis_tdata(rs_to_net_vFPGA_tx_data_TDATA),
//    .m_axis_tkeep(rs_to_net_vFPGA_tx_data_TKEEP),
//    .m_axis_tlast(rs_to_net_vFPGA_tx_data_TLAST)
//  );
 -----/\----- [OBSOLETE] -----/\----- */
   
  //============================================================================
  //  INST: UDP-CORE-MODULE
  //============================================================================
  UdpCore UDP (
  
    .aclk                             (piShlClk),
    .aresetn                          (~piShlRst),

    //------------------------------------------------------
    //-- From UDMX / Open-Port Interfaces
    //------------------------------------------------------
    //-- UDMX / This / OpenPortRequest / Axis       
    .openPort_TDATA                   (sUDMX_Udp_OpnReq_Axis_tdata),
    .openPort_TVALID                  (sUDMX_Udp_OpnReq_Axis_tvalid),            
    .openPort_TREADY                  (sUDP_Udmx_OpnReq_Axis_tready),    

    //------------------------------------------------------
    //-- To UDMX  / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Udmx / OpenPortStatus / Axis
    .confirmPortStatus_TREADY         (sUDMX_Udp_OpnSts_Axis_tready),
    .confirmPortStatus_TDATA          (sUDP_Udmx_OpnSts_Axis_tdata),
    .confirmPortStatus_TVALID         (sUDP_Udmx_OpnSts_Axis_tvalid),
 
    //------------------------------------------------------
    //-- From IPRX Interfaces
    //------------------------------------------------------
    //-- IPRX / This / Data / Axis -------------------------
    .inputPathInData_TDATA            (sIPRX_Udp_Axis_tdata),
    .inputPathInData_TKEEP            (sIPRX_Udp_Axis_tkeep),
    .inputPathInData_TLAST            (sIPRX_Udp_Axis_tlast),
    .inputPathInData_TVALID           (sIPRX_Udp_Axis_tvalid),
    .inputPathInData_TREADY           (sUDP_Iprx_Axis_tready),
     
     //------------------------------------------------------
     //-- From UDMX Interfaces
     //------------------------------------------------------ 
    //-- UDMX / This / Data / Axis
    .outputPathInData_TDATA           (sUDMX_Udp_Data_Axis_tdata),
    .outputPathInData_TKEEP           (sUDMX_Udp_Data_Axis_tkeep),
    .outputPathInData_TLAST           (sUDMX_Udp_Data_Axis_tlast),
    .outputPathInData_TVALID          (sUDMX_Udp_Data_Axis_tvalid),
    .outputPathInData_TREADY          (sUDP_Udmx_Data_Axis_tready),
    //-- UDMX / This / MetaData / Axis
    .outputPathInMetadata_TDATA       (sUDMX_Udp_Meta_Axis_tdata),
    .outputPathInMetadata_TVALID      (sUDMX_Udp_Meta_Axis_tvalid),
    .outputPathInMetadata_TREADY      (sUDP_Udmx_Meta_Axis_tready),
    //-- UDMX / This / TxLength / Axis
    .outputpathInLength_TDATA         (sUDMX_Udp_TxLn_Axis_tdata),
    .outputpathInLength_TVALID        (sUDMX_Udp_TxLn_Axis_tvalid),
    .outputpathInLength_TREADY        (sUDP_Udmx_PLen_Axis_tready),
   
    //------------------------------------------------------
    //-- To UDMX Interfaces
    //------------------------------------------------------
    //-- THIS / Udmx / Data / Axis
    .inputpathOutData_TREADY          (sUDMX_Udp_Data_Axis_tready),
    .inputpathOutData_TDATA           (sUDP_Udmx_Data_Axis_tdata), 
    .inputpathOutData_TKEEP           (sUDP_Udmx_Data_Axis_tkeep),
    .inputpathOutData_TLAST           (sUDP_Udmx_Data_Axis_tlast),
    .inputpathOutData_TVALID          (sUDP_Udmx_Data_Axis_tvalid),
    //-- THIS / Udmx / MetaData / Axis
    .inputPathOutputMetadata_TREADY   (sUDMX_Udp_Meta_Axis_tready),
    .inputPathOutputMetadata_TDATA    (sUDP_Udmx_Meta_Axis_tdata),
    .inputPathOutputMetadata_TVALID   (sUDP_Udmx_Meta_Axis_tvalid),

    //------------------------------------------------------
    //-- To L3MUX Interfaces
    //------------------------------------------------------
    //-- THIS / L3mux / Axis
    .outputPathOutData_TREADY         (sL3MUX_Udp_Axis_tready),   
    .outputPathOutData_TDATA          (sUDP_L3mux_Axis_tdata),
    .outputPathOutData_TKEEP          (sUDP_L3mux_Axis_tkeep),
    .outputPathOutData_TLAST          (sUDP_L3mux_Axis_tlast),
    .outputPathOutData_TVALID         (sUDP_L3mux_Axis_tvalid),
    
    //------------------------------------------------------
    //-- To ICMP Interfaces
    //------------------------------------------------------
    //-- THIS / Icmp / Axis
    .inputPathPortUnreachable_TREADY  (sICMP_Udp_Axis_tready),
    .inputPathPortUnreachable_TDATA   (sUDP_Icmp_Axis_tdata),
    .inputPathPortUnreachable_TKEEP   (sUDP_Icmp_Axis_tkeep),
    .inputPathPortUnreachable_TLAST   (sUDP_Icmp_Axis_tlast),
    .inputPathPortUnreachable_TVALID  (sUDP_Icmp_Axis_tvalid),
    
    //-- Unused Interface ----------------------------------
    .portRelease_TDATA                (15'b0),          
    .portRelease_TVALID               (1'b0),         
    .portRelease_TREADY               ()     
                          
  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_udp_module UDP (
//    .inputPathInData_TVALID(axi_iph_to_udp_tvalid),                  
//    .inputPathInData_TREADY(axi_iph_to_udp_tready),                  
//    .inputPathInData_TDATA(axi_iph_to_udp_tdata),                    
//    .inputPathInData_TKEEP(axi_iph_to_udp_tkeep),                   
//    .inputPathInData_TLAST(axi_iph_to_udp_tlast),                    
    
//    .inputpathOutData_TVALID(udp2muxRxDataIn_TVALID),                
//    .inputpathOutData_TREADY(udp2muxRxDataIn_TREADY),                
//    .inputpathOutData_TDATA(udp2muxRxDataIn_TDATA),                  
//    .inputpathOutData_TKEEP(udp2muxRxDataIn_TKEEP),                  
//    .inputpathOutData_TLAST(udp2muxRxDataIn_TLAST),                  
    
//    .openPort_TVALID(mux2udp_requestPortOpenOut_V_TVALID),            
//    .openPort_TREADY(mux2udp_requestPortOpenOut_V_TREADY),           
//    .openPort_TDATA(mux2udp_requestPortOpenOut_V_TDATA),            
    
//    .confirmPortStatus_TVALID(udp2mux_portOpenReplyIn_V_V_TVALID),   
//    .confirmPortStatus_TREADY(udp2mux_portOpenReplyIn_V_V_TREADY),   
//    .confirmPortStatus_TDATA(udp2mux_portOpenReplyIn_V_V_TDATA),   
    
//    .inputPathOutputMetadata_TVALID(udp2muxRxMetadataIn_V_TVALID),   
//    .inputPathOutputMetadata_TREADY(udp2muxRxMetadataIn_V_TREADY),   
//    .inputPathOutputMetadata_TDATA(udp2muxRxMetadataIn_V_TDATA),   
    
//    .portRelease_TVALID(1'b0),                                        
//    .portRelease_TREADY(),                                          
//    .portRelease_TDATA(15'b0),                                      
    
//    .outputPathInData_TVALID(mux2udp_TVALID),                      
//    .outputPathInData_TREADY(mux2udp_TREADY),                        
//    .outputPathInData_TDATA(mux2udp_TDATA),                        
//    .outputPathInData_TKEEP(mux2udp_TKEEP),                         
//    .outputPathInData_TLAST(mux2udp_TLAST),                         
    
//    .outputPathOutData_TVALID(axi_udp_to_merge_tvalid),            
//    .outputPathOutData_TREADY(axi_udp_to_merge_tready),              
//    .outputPathOutData_TDATA(axi_udp_to_merge_tdata),              
//    .outputPathOutData_TKEEP(axi_udp_to_merge_tkeep),             
//    .outputPathOutData_TLAST(axi_udp_to_merge_tlast),        
    
//    .outputPathInMetadata_TVALID(mux2udpTxMetadataOut_V_TVALID),    
//    .outputPathInMetadata_TREADY(mux2udpTxMetadataOut_V_TREADY),    
//    .outputPathInMetadata_TDATA(mux2udpTxMetadataOut_V_TDATA),     
    
//    .outputpathInLength_TVALID(mux2udpTxLengthOut_V_V_TVALID),      
//    .outputpathInLength_TREADY(mux2udpTxLengthOut_V_V_TREADY),    
//    .outputpathInLength_TDATA(mux2udpTxLengthOut_V_V_TDATA),        
    
//    .inputPathPortUnreachable_TVALID(axis_udp_to_icmp_tvalid),    
//    .inputPathPortUnreachable_TREADY(axis_udp_to_icmp_tready),      
//    .inputPathPortUnreachable_TDATA(axis_udp_to_icmp_tdata),        
//    .inputPathPortUnreachable_TKEEP(axis_udp_to_icmp_tkeep),        
//    .inputPathPortUnreachable_TLAST(axis_udp_to_icmp_tlast),    
//      //.ap_start(1'b1),                                            
//      //.ap_ready(),                                                 
//      //.ap_done(),                                                
//      //.ap_idle(),                                                 
//      .aclk(cf_axi_clk),                                                  
//      .aresetn(cf_aresetn)                                   
//  );
 -----/\----- [OBSOLETE] -----/\----- */
   
  //============================================================================
  //  INST: UDP-MUX
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES
  
  UdpMultiplexer UDMX (  // Deprecated version
    
    .aclk                         (piShlClk),                                                  
    .aresetn                      (~piShlRst),

    //------------------------------------------------------
    //-- From DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- DHCP / This / OpenPortRequest / Axis
    .siDHCP_This_OpnReq_TDATA     (sDHCP_Udmx_OpnReq_Axis_tdata),
    .siDHCP_This_OpnReq_TVALID    (sDHCP_Udmx_OpnReq_Axis_tvalid),
    .siDHCP_This_OpnReq_TREADY    (sUDMX_Dhcp_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / OpenPortAck / Axis
    .soTHIS_Dhcp_OpnAck_TREADY    (sDHCP_Udmx_OpnAck_Axis_tready),
    .soTHIS_Dhcp_OpnAck_TDATA     (sUDMX_Dhcp_OpnAck_Axis_tdata),
    .soTHIS_Dhcp_OpnAck_TVALID    (sUDMX_Dhcp_OpnAck_Axis_tvalid),

    //------------------------------------------------------
    //-- From DHCP / Data & MetaData Interfaces
    //------------------------------------------------------               
    //-- DHCP / This / Data / Axis 
    .siDHCP_This_Data_TDATA       (sDHCP_Udmx_Data_Axis_tdata),          
    .siDHCP_This_Data_TKEEP       (sDHCP_Udmx_Data_Axis_tkeep),      
    .siDHCP_This_Data_TLAST       (sDHCP_Udmx_Data_Axis_tlast),            
    .siDHCP_This_Data_TVALID      (sDHCP_Udmx_Data_Axis_tvalid),
    .siDHCP_This_Data_TREADY      (sUDMX_Dhcp_Data_Axis_tready),
    //-- DHCP / This / MetaData / Axis
    .siDHCP_This_Meta_TDATA       (sDHCP_Udmx_Meta_Axis_tdata),
    .siDHCP_This_Meta_TVALID      (sDHCP_Udmx_Meta_Axis_tvalid),
    .siDHCP_This_Meta_TREADY      (sUDMX_Dhcp_Meta_Axis_tready),
    //-- DHCP / This / TxLen / Axis
    .siDHCP_This_PLen_TDATA       (sDHCP_Udmx_PLen_Axis_tdata),
    .siDHCP_This_PLen_TVALID      (sDHCP_Udmx_PLen_Axis_tvalid),
    .siDHCP_This_PLen_TREADY      (sUDMX_Dhcp_PLen_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / Data / Axis
    .soTHIS_Dhcp_Data_TREADY      (sDHCP_Udmx_Data_Axis_tready),                
    .soTHIS_Dhcp_Data_TDATA       (sUDMX_Dhcp_Data_Axis_tdata),
    .soTHIS_Dhcp_Data_TKEEP       (sUDMX_Dhcp_Data_Axis_tkeep),
    .soTHIS_Dhcp_Data_TLAST       (sUDMX_Dhcp_Data_Axis_tlast),
    .soTHIS_Dhcp_Data_TVALID      (sUDMX_Dhcp_Data_Axis_tvalid),
    //-- THIS / Dhcp / MetaData / Axis
    .soTHIS_Dhcp_Meta_TREADY      (sDHCP_Udmx_Meta_Axis_tready),
    .soTHIS_Dhcp_Meta_TDATA       (sUDMX_Dhcp_Meta_Axis_tdata),
    .soTHIS_Dhcp_Meta_TVALID      (sUDMX_Dhcp_Meta_Axis_tvalid),

    //------------------------------------------------------
    //-- From UDP / Open-Port Interfaces
    //------------------------------------------------------
    //-- UDP / This / OpenPortAck / Axis
    .siUDP_This_OpnAck_TDATA      (sUDP_Udmx_OpnSts_Axis_tdata),
    .siUDP_This_OpnAck_TVALID     (sUDP_Udmx_OpnSts_Axis_tvalid),
    .siUDP_This_OpnAck_TREADY     (sUDMX_Udp_OpnSts_Axis_tready),

    //------------------------------------------------------
    //-- To UDP   / Open-Port Interfaces
    //------------------------------------------------------                
    //-- THIS / Udp / OpenPortRequest / Axis
    .soTHIS_Udp_OpnReq_TREADY     (sUDP_Udmx_OpnReq_Axis_tready),
    .soTHIS_Udp_OpnReq_TDATA      (sUDMX_Udp_OpnReq_Axis_tdata),
    .soTHIS_Udp_OpnReq_TVALID     (sUDMX_Udp_OpnReq_Axis_tvalid),

    //------------------------------------------------------
    //-- From UDP / Data & MetaData Interfaces
    //------------------------------------------------------                      
    //-- UDP / This / Data / Axis
    .siUDP_This_Data_TDATA        (sUDP_Udmx_Data_Axis_tdata),
    .siUDP_This_Data_TKEEP        (sUDP_Udmx_Data_Axis_tkeep),
    .siUDP_This_Data_TLAST        (sUDP_Udmx_Data_Axis_tlast),
    .siUDP_This_Data_TVALID       (sUDP_Udmx_Data_Axis_tvalid),
    .siUDP_This_Data_TREADY       (sUDMX_Udp_Data_Axis_tready),
    //-- UDP / This / MetaData / Axis
    .siUDP_This_Meta_TDATA        (sUDP_Udmx_Meta_Axis_tdata),
    .siUDP_This_Meta_TVALID       (sUDP_Udmx_Meta_Axis_tvalid),
    .siUDP_This_Meta_TREADY       (sUDMX_Udp_Meta_Axis_tready),
    
    //------------------------------------------------------
    //-- To UDP   /  Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Udp / Data / Axis
    .soTHIS_Udp_Data_TREADY       (sUDP_Udmx_Data_Axis_tready),
    .soTHIS_Udp_Data_TDATA        (sUDMX_Udp_Data_Axis_tdata),
    .soTHIS_Udp_Data_TKEEP        (sUDMX_Udp_Data_Axis_tkeep),
    .soTHIS_Udp_Data_TLAST        (sUDMX_Udp_Data_Axis_tlast),
    .soTHIS_Udp_Data_TVALID       (sUDMX_Udp_Data_Axis_tvalid),
    //-- THIS / Udp / MetaData / Axis
    .soTHIS_Udp_Meta_TREADY       (sUDP_Udmx_Meta_Axis_tready),
    .soTHIS_Udp_Meta_TDATA        (sUDMX_Udp_Meta_Axis_tdata),
    .soTHIS_Udp_Meta_TVALID       (sUDMX_Udp_Meta_Axis_tvalid),
    //-- THIS / Udp / TxLength / Axis
    .soTHIS_Udp_PLen_TREADY       (sUDP_Udmx_PLen_Axis_tready),
    .soTHIS_Udp_PLen_TDATA        (sUDMX_Udp_TxLn_Axis_tdata),
    .soTHIS_Udp_PLen_TVALID       (sUDMX_Udp_TxLn_Axis_tvalid),

    //------------------------------------------------------
    //-- From URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- URIF / This / OpenPortRequest / Axis
    .siURIF_This_OpnReq_TDATA     (siURIF_Udmx_OpnReq_Axis_tdata),
    .siURIF_This_OpnReq_TVALID    (siURIF_Udmx_OpnReq_Axis_tvalid),
    .siURIF_This_OpnReq_TREADY    (siUDMX_Urif_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- To   URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / OpenPortStatus / Axis
    .soTHIS_Urif_OpnAck_TREADY    (soURIF_Udmx_OpnAck_Axis_tready),
    .soTHIS_Urif_OpnAck_TDATA     (soUDMX_Urif_OpnAck_Axis_tdata),
    .soTHIS_Urif_OpnAck_TVALID    (soUDMX_Urif_OpnAck_Axis_tvalid),

    //------------------------------------------------------
    //-- From URIF / Data & MetaData Interfaces
    //------------------------------------------------------                                     
    //-- URIF / This / Data / Axis
    .siURIF_This_Data_TDATA       (siURIF_Udmx_Data_Axis_tdata),           
    .siURIF_This_Data_TKEEP       (siURIF_Udmx_Data_Axis_tkeep),      
    .siURIF_This_Data_TLAST       (siURIF_Udmx_Data_Axis_tlast),
    .siURIF_This_Data_TVALID      (siURIF_Udmx_Data_Axis_tvalid),
    .siURIF_This_Data_TREADY      (siUDMX_Urif_Data_Axis_tready),
    //-- URIF / This / MetaData / Axis
    .siURIF_This_Meta_TDATA       (siURIF_Udmx_Meta_Axis_tdata),
    .siURIF_This_Meta_TVALID      (siURIF_Udmx_Meta_Axis_tvalid),     
    .siURIF_This_Meta_TREADY      (siUDMX_Urif_Meta_Axis_tready),
    //-- URIF /This / TxLn / Axis
    .siURIF_This_PLen_TDATA       (siURIF_Udmx_PLen_Axis_tdata),
    .siURIF_This_PLen_TVALID      (siURIF_Udmx_PLen_Axis_tvalid),
    .siURIF_This_PLen_TREADY      (siUDMX_Urif_PLen_Axis_tready),
                       
    //------------------------------------------------------
    //-- To URIF / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / Data / Output AXI-Write Stream Interface
    .soTHIS_Urif_Data_TREADY      (soURIF_Udmx_Data_Axis_tready),
    .soTHIS_Urif_Data_TDATA       (soUDMX_Urif_Data_Axis_tdata),
    .soTHIS_Urif_Data_TKEEP       (soUDMX_Urif_Data_Axis_tkeep),
    .soTHIS_Urif_Data_TLAST       (soUDMX_Urif_Data_Axis_tlast),
    .soTHIS_Urif_Data_TVALID      (soUDMX_Urif_Data_Axis_tvalid),
    //-- THIS / Urif / Meta / Output AXI-Write Stream Interface
    .soTHIS_Urif_Meta_TREADY      (soURIF_Udmx_Meta_Axis_tready),
    .soTHIS_Urif_Meta_TDATA       (soUDMX_Urif_Meta_Axis_tdata),
    .soTHIS_Urif_Meta_TVALID      (soUDMX_Urif_Meta_Axis_tvalid)
                                                     
  );

`else
  
    UdpMultiplexer UDMX (
    
    .ap_clk                       (piShlClk),                                                  
    .ap_rst_n                     (~piShlRst),

    //------------------------------------------------------
    //-- From DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- DHCP / This / OpenPortRequest / Axis
    .siDHCP_This_OpnReq_V_V_TDATA (sDHCP_Udmx_OpnReq_Axis_tdata),
    .siDHCP_This_OpnReq_V_V_TVALID(sDHCP_Udmx_OpnReq_Axis_tvalid),
    .siDHCP_This_OpnReq_V_V_TREADY(sUDMX_Dhcp_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / OpenPortAck / Axis
    .soTHIS_Dhcp_OpnAck_V_TREADY  (sDHCP_Udmx_OpnAck_Axis_tready),
    .soTHIS_Dhcp_OpnAck_V_TDATA   (sUDMX_Dhcp_OpnAck_Axis_tdata),
    .soTHIS_Dhcp_OpnAck_V_TVALID  (sUDMX_Dhcp_OpnAck_Axis_tvalid),

    //------------------------------------------------------
    //-- From DHCP / Data & MetaData Interfaces
    //------------------------------------------------------               
    //-- DHCP / This / Data / Axis 
    .siDHCP_This_Data_TDATA       (sDHCP_Udmx_Data_Axis_tdata),          
    .siDHCP_This_Data_TKEEP       (sDHCP_Udmx_Data_Axis_tkeep),      
    .siDHCP_This_Data_TLAST       (sDHCP_Udmx_Data_Axis_tlast),            
    .siDHCP_This_Data_TVALID      (sDHCP_Udmx_Data_Axis_tvalid),
    .siDHCP_This_Data_TREADY      (sUDMX_Dhcp_Data_Axis_tready),
    //-- DHCP / This / MetaData / Axis
    .siDHCP_This_Meta_TDATA       (sDHCP_Udmx_Meta_Axis_tdata),
    .siDHCP_This_Meta_TVALID      (sDHCP_Udmx_Meta_Axis_tvalid),
    .siDHCP_This_Meta_TREADY      (sUDMX_Dhcp_Meta_Axis_tready),
    //-- DHCP / This / TxLen / Axis
    .siDHCP_This_PLen_V_V_TDATA   (sDHCP_Udmx_PLen_Axis_tdata),
    .siDHCP_This_PLen_V_V_TVALID  (sDHCP_Udmx_PLen_Axis_tvalid),
    .siDHCP_This_PLen_V_V_TREADY  (sUDMX_Dhcp_PLen_Axis_tready),

    //------------------------------------------------------
    //-- To DHCP Interfaces / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Dhcp / Data / Axis
    .soTHIS_Dhcp_Data_TREADY      (sDHCP_Udmx_Data_Axis_tready),                
    .soTHIS_Dhcp_Data_TDATA       (sUDMX_Dhcp_Data_Axis_tdata),
    .soTHIS_Dhcp_Data_TKEEP       (sUDMX_Dhcp_Data_Axis_tkeep),
    .soTHIS_Dhcp_Data_TLAST       (sUDMX_Dhcp_Data_Axis_tlast),
    .soTHIS_Dhcp_Data_TVALID      (sUDMX_Dhcp_Data_Axis_tvalid),
    //-- THIS / Dhcp / MetaData / Axis
    .soTHIS_Dhcp_Meta_TREADY      (sDHCP_Udmx_Meta_Axis_tready),
    .soTHIS_Dhcp_Meta_TDATA       (sUDMX_Dhcp_Meta_Axis_tdata),
    .soTHIS_Dhcp_Meta_TVALID      (sUDMX_Dhcp_Meta_Axis_tvalid),

    //------------------------------------------------------
    //-- From UDP / Open-Port Interfaces
    //------------------------------------------------------
    //-- UDP / This / OpenPortAck / Axis
    .siUDP_This_OpnAck_V_TDATA    (sUDP_Udmx_OpnSts_Axis_tdata),
    .siUDP_This_OpnAck_V_TVALID   (sUDP_Udmx_OpnSts_Axis_tvalid),
    .siUDP_This_OpnAck_V_TREADY   (sUDMX_Udp_OpnSts_Axis_tready),

    //------------------------------------------------------
    //-- To UDP   / Open-Port Interfaces
    //------------------------------------------------------                
    //-- THIS / Udp / OpenPortRequest / Axis
    .soTHIS_Udp_OpnReq_V_V_TREADY (sUDP_Udmx_OpnReq_Axis_tready),
    .soTHIS_Udp_OpnReq_V_V_TDATA  (sUDMX_Udp_OpnReq_Axis_tdata),
    .soTHIS_Udp_OpnReq_V_V_TVALID (sUDMX_Udp_OpnReq_Axis_tvalid),

    //------------------------------------------------------
    //-- From UDP / Data & MetaData Interfaces
    //------------------------------------------------------                      
    //-- UDP / This / Data / Axis
    .siUDP_This_Data_TDATA        (sUDP_Udmx_Data_Axis_tdata),
    .siUDP_This_Data_TKEEP        (sUDP_Udmx_Data_Axis_tkeep),
    .siUDP_This_Data_TLAST        (sUDP_Udmx_Data_Axis_tlast),
    .siUDP_This_Data_TVALID       (sUDP_Udmx_Data_Axis_tvalid),
    .siUDP_This_Data_TREADY       (sUDMX_Udp_Data_Axis_tready),
    //-- UDP / This / MetaData / Axis
    .siUDP_This_Meta_TDATA        (sUDP_Udmx_Meta_Axis_tdata),
    .siUDP_This_Meta_TVALID       (sUDP_Udmx_Meta_Axis_tvalid),
    .siUDP_This_Meta_TREADY       (sUDMX_Udp_Meta_Axis_tready),
    
    //------------------------------------------------------
    //-- To UDP   /  Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Udp / Data / Axis
    .soTHIS_Udp_Data_TREADY       (sUDP_Udmx_Data_Axis_tready),
    .soTHIS_Udp_Data_TDATA        (sUDMX_Udp_Data_Axis_tdata),
    .soTHIS_Udp_Data_TKEEP        (sUDMX_Udp_Data_Axis_tkeep),
    .soTHIS_Udp_Data_TLAST        (sUDMX_Udp_Data_Axis_tlast),
    .soTHIS_Udp_Data_TVALID       (sUDMX_Udp_Data_Axis_tvalid),
    //-- THIS / Udp / MetaData / Axis
    .soTHIS_Udp_Meta_TREADY       (sUDP_Udmx_Meta_Axis_tready),
    .soTHIS_Udp_Meta_TDATA        (sUDMX_Udp_Meta_Axis_tdata),
    .soTHIS_Udp_Meta_TVALID       (sUDMX_Udp_Meta_Axis_tvalid),
    //-- THIS / Udp / TxLength / Axis
    .soTHIS_Udp_PLen_V_V_TREADY   (sUDP_Udmx_PLen_Axis_tready),
    .soTHIS_Udp_PLen_V_V_TDATA    (sUDMX_Udp_TxLn_Axis_tdata),
    .soTHIS_Udp_PLen_V_V_TVALID   (sUDMX_Udp_TxLn_Axis_tvalid),

    //------------------------------------------------------
    //-- From URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- URIF / This / OpenPortRequest / Axis
    .siURIF_This_OpnReq_V_V_TDATA (sURIF_Udmx_OpnReq_Axis_tdata),
    .siURIF_This_OpnReq_V_V_TVALID(sURIF_Udmx_OpnReq_Axis_tvalid),
    .siURIF_This_OpnReq_V_V_TREADY(sUDMX_Urif_OpnReq_Axis_tready),

    //------------------------------------------------------
    //-- To   URIF / Open-Port Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / OpenPortStatus / Axis
    .soTHIS_Urif_OpnAck_V_TREADY  (sURIF_Udmx_OpnAck_Axis_tready),
    .soTHIS_Urif_OpnAck_V_TDATA   (sUDMX_Urif_OpnAck_Axis_tdata),
    .soTHIS_Urif_OpnAck_V_TVALID  (sUDMX_Urif_OpnAck_Axis_tvalid),

    //------------------------------------------------------
    //-- From URIF / Data & MetaData Interfaces
    //------------------------------------------------------                                     
    //-- URIF / This / Data / Axis
    .siURIF_This_Data_TDATA       (sURIF_Udmx_Data_Axis_tdata),           
    .siURIF_This_Data_TKEEP       (sURIF_Udmx_Data_Axis_tkeep),      
    .siURIF_This_Data_TLAST       (sURIF_Udmx_Data_Axis_tlast),
    .siURIF_This_Data_TVALID      (sURIF_Udmx_Data_Axis_tvalid),
    .siURIF_This_Data_TREADY      (sUDMX_Urif_Data_Axis_tready),
    //-- URIF / This / MetaData / Axis
    .siURIF_This_Meta_TDATA       (sURIF_Udmx_Meta_Axis_tdata),
    .siURIF_This_Meta_TVALID      (sURIF_Udmx_Meta_Axis_tvalid),     
    .siURIF_This_Meta_TREADY      (sUDMX_Urif_Meta_Axis_tready),
    //-- URIF /This / TxLn / Axis
    .siURIF_This_PLen_V_V_TDATA   (sURIF_Udmx_PLen_Axis_tdata),
    .siURIF_This_PLen_V_V_TVALID  (sURIF_Udmx_PLen_Axis_tvalid),
    .siURIF_This_PLen_V_V_TREADY  (sUDMX_Urif_PLen_Axis_tready),
                       
    //------------------------------------------------------
    //-- To URIF / Data & MetaData Interfaces
    //------------------------------------------------------
    //-- THIS / Urif / Data / Output AXI-Write Stream Interface
    .soTHIS_Urif_Data_TREADY      (sURIF_Udmx_Data_Axis_tready),
    .soTHIS_Urif_Data_TDATA       (sUDMX_Urif_Data_Axis_tdata),
    .soTHIS_Urif_Data_TKEEP       (sUDMX_Urif_Data_Axis_tkeep),
    .soTHIS_Urif_Data_TLAST       (sUDMX_Urif_Data_Axis_tlast),
    .soTHIS_Urif_Data_TVALID      (sUDMX_Urif_Data_Axis_tvalid),
    //-- THIS / Urif / Meta / Output AXI-Write Stream Interface
    .soTHIS_Urif_Meta_TREADY      (sURIF_Udmx_Meta_Axis_tready),
    .soTHIS_Urif_Meta_TDATA       (sUDMX_Urif_Meta_Axis_tdata),
    .soTHIS_Urif_Meta_TVALID      (sUDMX_Urif_Meta_Axis_tvalid)
                                                     
  );

`endif // !`ifdef USE_DEPRECATED_DIRECTIVES
   
  
      
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_udp_app_if UMUX (
//    .portOpenReplyIn_TVALID(udp2mux_portOpenReplyIn_V_V_TVALID),         
//    .portOpenReplyIn_TREADY(udp2mux_portOpenReplyIn_V_V_TREADY),          
//    .portOpenReplyIn_TDATA(udp2mux_portOpenReplyIn_V_V_TDATA),                    
    
//    .requestPortOpenOut_TVALID(mux2udp_requestPortOpenOut_V_TVALID),     
//    .requestPortOpenOut_TREADY(mux2udp_requestPortOpenOut_V_TREADY),      
//    .requestPortOpenOut_TDATA(mux2udp_requestPortOpenOut_V_TDATA),       
      
//    .portOpenReplyOutApp_TVALID(lbPortOpenReplyIn_TVALID),                
//    .portOpenReplyOutApp_TREADY(lbPortOpenReplyIn_TREADY),               
//    .portOpenReplyOutApp_TDATA(lbPortOpenReplyIn_TDATA),                   
    
//    .requestPortOpenInApp_TVALID(lbRequestPortOpenOut_TVALID),           
//    .requestPortOpenInApp_TREADY(lbRequestPortOpenOut_TREADY),            
//    .requestPortOpenInApp_TDATA(lbRequestPortOpenOut_TDATA),                   
        
//    .portOpenReplyOutDhcp_TVALID(mux2dhcp_portOpenReplyIn_V_V_TVALID),    
//    .portOpenReplyOutDhcp_TREADY(mux2dhcp_portOpenReplyIn_V_V_TREADY),    
//    .portOpenReplyOutDhcp_TDATA(mux2dhcp_portOpenReplyIn_V_V_TDATA),     
    
//    .requestPortOpenInDhcp_TVALID(dhcp2mux_requestPortOpenOut_V_TVALID),  
//    .requestPortOpenInDhcp_TREADY(dhcp2mux_requestPortOpenOut_V_TREADY),  
//    .requestPortOpenInDhcp_TDATA(dhcp2mux_requestPortOpenOut_V_TDATA),   
      
//    .rxDataIn_TVALID(udp2muxRxDataIn_TVALID),                            
//    .rxDataIn_TREADY(udp2muxRxDataIn_TREADY),                         
//    .rxDataIn_TDATA(udp2muxRxDataIn_TDATA),                              
//    .rxDataIn_TKEEP(udp2muxRxDataIn_TKEEP),                             
//    .rxDataIn_TLAST(udp2muxRxDataIn_TLAST),                              
    
//    .rxDataOutApp_TVALID(lbRxDataIn_TVALID),                            
//    .rxDataOutApp_TREADY(lbRxDataIn_TREADY),                             
//    .rxDataOutApp_TDATA(lbRxDataIn_TDATA),                               
//    .rxDataOutApp_TKEEP(lbRxDataIn_TKEEP),                               
//    .rxDataOutApp_TLAST(lbRxDataIn_TLAST),                               
    
//    .rxDataOutDhcp_TVALID(mux2dhcpRxDataIn_TVALID),                      
//    .rxDataOutDhcp_TREADY(mux2dhcpRxDataIn_TREADY),                     
//    .rxDataOutDhcp_TDATA(mux2dhcpRxDataIn_TDATA),                       
//    .rxDataOutDhcp_TKEEP(mux2dhcpRxDataIn_TKEEP),                       
//    .rxDataOutDhcp_TLAST(mux2dhcpRxDataIn_TLAST),                      
    
//    .rxMetadataIn_TVALID(udp2muxRxMetadataIn_V_TVALID),                  
//    .rxMetadataIn_TREADY(udp2muxRxMetadataIn_V_TREADY),               
//    .rxMetadataIn_TDATA(udp2muxRxMetadataIn_V_TDATA),                   
    
//    .rxMetadataOutApp_TVALID(lbRxMetadataIn_TVALID),                    
//    .rxMetadataOutApp_TREADY(lbRxMetadataIn_TREADY),                   
//    .rxMetadataOutApp_TDATA(lbRxMetadataIn_TDATA),                       
    
//    .rxMetadataOutDhcp_TVALID(mux2dhcpRxMetadataIn_V_TVALID),            
//    .rxMetadataOutDhcp_TREADY(mux2dhcpRxMetadataIn_V_TREADY),          
//    .rxMetadataOutDhcp_TDATA(mux2dhcpRxMetadataIn_V_TDATA),              
    
//    .txDataInApp_TVALID(lbTxDataOut_TVALID),                              
//    .txDataInApp_TREADY(lbTxDataOut_TREADY),                             
//    .txDataInApp_TDATA(lbTxDataOut_TDATA),                              
//    .txDataInApp_TKEEP(lbTxDataOut_TKEEP),                               
//    .txDataInApp_TLAST(lbTxDataOut_TLAST),                                
    
//    .txDataInDhcp_TVALID(dhcp2mux_TVALID),                             
//    .txDataInDhcp_TREADY(dhcp2mux_TREADY),                            
//    .txDataInDhcp_TDATA(dhcp2mux_TDATA),                                 
//    .txDataInDhcp_TKEEP(dhcp2mux_TKEEP),                                 
//    .txDataInDhcp_TLAST(dhcp2mux_TLAST),                                 
    
//    .txDataOut_TVALID(mux2udp_TVALID),                                   
//    .txDataOut_TREADY(mux2udp_TREADY),                                    
//    .txDataOut_TDATA(mux2udp_TDATA),                                      
//    .txDataOut_TKEEP(mux2udp_TKEEP),                                     
//    .txDataOut_TLAST(mux2udp_TLAST),                                   
    
//    .txLengthInApp_TVALID(lbTxLengthOut_TVALID),                          
//    .txLengthInApp_TREADY(lbTxLengthOut_TREADY),                    
//    .txLengthInApp_TDATA(lbTxLengthOut_TDATA),                            
    
//    .txLengthInDhcp_TVALID(dhcp2muxTxLengthOut_V_V_TVALID),             
//    .txLengthInDhcp_TREADY(dhcp2muxTxLengthOut_V_V_TREADY),            
//    .txLengthInDhcp_TDATA(dhcp2muxTxLengthOut_V_V_TDATA),                
    
//    .txLengthOut_TVALID(mux2udpTxLengthOut_V_V_TVALID),                  
//    .txLengthOut_TREADY(mux2udpTxLengthOut_V_V_TREADY),                   
//    .txLengthOut_TDATA(mux2udpTxLengthOut_V_V_TDATA),                   
    
//    .txMetadataInApp_TVALID(lbTxMetadataOut_TVALID),                   
//    .txMetadataInApp_TREADY(lbTxMetadataOut_TREADY),                     
//    .txMetadataInApp_TDATA(lbTxMetadataOut_TDATA),                        
    
//    .txMetadataInDhcp_TVALID(dhcp2muxTxMetadataOut_V_TVALID),             
//    .txMetadataInDhcp_TREADY(dhcp2muxTxMetadataOut_V_TREADY),            
//    .txMetadataInDhcp_TDATA(dhcp2muxTxMetadataOut_V_TDATA),              
    
//    .txMetadataOut_TVALID(mux2udpTxMetadataOut_V_TVALID),                 
//    .txMetadataOut_TREADY(mux2udpTxMetadataOut_V_TREADY),              
//    .txMetadataOut_TDATA(mux2udpTxMetadataOut_V_TDATA),                 
    
//    .aclk(cf_axi_clk),                                                  
//    .aresetn(cf_aresetn)                                                   
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (URIF ==> NTS0/Role/Udp)
  //============================================================================
  //OBSOLETE
  //AxisRegisterSlice_64 ARS6 (
  //   .aclk          (piShlClk),
  //   .aresetn       (~piShlRst),
  //   // From URIF / Role / Udp / Axis
  //   .s_axis_tdata  (sURIF_Rol_Axis_tdata),
  //   .s_axis_tkeep  (sURIF_Rol_Axis_tkeep),
  //   .s_axis_tlast  (sURIF_Rol_Axis_tlast),
  //   .s_axis_tvalid (sURIF_Rol_Axis_tvalid),
  //   .s_axis_tready (sROL_Urif_Axis_treadyReg),     
  //   //-- To NTS0 / Role / Udp / Axis
  //   .m_axis_tready (piROL_Nts0_Udp_Axis_tready),
  //   .m_axis_tdata  (poNTS0_Rol_Udp_Axis_tdata),
  //   .m_axis_tkeep  (poNTS0_Rol_Udp_Axis_tkeep),
  //   .m_axis_tlast  (poNTS0_Rol_Udp_Axis_tlast),
  //   .m_axis_tvalid (poNTS0_Rol_Udp_Axis_tvalid)
  //);
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS6 (
//     .aclk(cf_axi_clk),
//     .aresetn(cf_aresetn),
     
//     .s_axis_tvalid(uai_to_app_slice_tvalid),
//     .s_axis_tready(uai_to_app_slice_tready),
//     .s_axis_tdata(uai_to_app_slice_tdata),
//     .s_axis_tkeep(uai_to_app_slice_tkeep),
//     .s_axis_tlast(uai_to_app_slice_tlast),
     
//     .m_axis_tvalid(vFPGA_UDP_rx_data_TVALID),
//     .m_axis_tready(vFPGA_UDP_rx_data_TREADY),
//     .m_axis_tdata(vFPGA_UDP_rx_data_TDATA),
//     .m_axis_tkeep(vFPGA_UDP_rx_data_TKEEP),
//     .m_axis_tlast(vFPGA_UDP_rx_data_TLAST)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (ROLE/Nts0/Udp ==> URIF)
  //============================================================================
  //TODO: necessary?
//  AxisRegisterSlice_64 ARS7 (
//    .aclk           (piShlClk),
//    .aresetn        (~piShlRst),
//    //-- From ROLE / Nts0 / Udp / Axis
//    .s_axis_tdata   (piROL_Nts0_Udp_Axis_tdata),
//    .s_axis_tkeep   (piROL_Nts0_Udp_Axis_tkeep),
//    .s_axis_tlast   (piROL_Nts0_Udp_Axis_tlast),
//    .s_axis_tvalid  (piROL_Nts0_Udp_Axis_tvalid),
//    .s_axis_tready  (poNTS0_Rol_Udp_Axis_tready),
//    //-- To URIF / Role / Axis
//    .m_axis_tready  (sURIF_Rol_Axis_tready),
//    .m_axis_tdata   (sROL_Nts0_Udp_Axis_tdataReg),
//    .m_axis_tkeep   (sROL_Nts0_Udp_Axis_tkeepReg),
//    .m_axis_tlast   (sROL_Nts0_Udp_Axis_tlastReg),
//    .m_axis_tvalid  (sROL_Nts0_Udp_Axis_tvalidReg)
//  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_register_slice_64 ARS7 (
//    .aclk(cf_axi_clk),
//    .aresetn(cf_aresetn),
        
//    .s_axis_tvalid(vFPGA_UDP_tx_data_TVALID),
//    .s_axis_tready(vFPGA_UDP_tx_data_TREADY),
//    .s_axis_tdata(vFPGA_UDP_tx_data_TDATA),
//    .s_axis_tkeep(vFPGA_UDP_tx_data_TKEEP),
//    .s_axis_tlast(vFPGA_UDP_tx_data_TLAST),
        
//    .m_axis_tvalid(app_to_uai_slice_tvalid),
//    .m_axis_tready(app_to_uai_slice_tready),
//    .m_axis_tdata(app_to_uai_slice_tdata),
//    .m_axis_tkeep(app_to_uai_slice_tkeep),
//    .m_axis_tlast(app_to_uai_slice_tlast)
//  ); 
 -----/\----- [OBSOLETE] -----/\----- */

//OBSOLETE  //============================================================================
//OBSOLETE  //  INST: UDP-ROLE-INTERFACE
//OBSOLETE  //============================================================================
//OBSOLETE`ifdef USE_DEPRECATED_DIRECTIVES
//OBSOLETE  
//OBSOLETE  UdpRoleInterface URIF (
//OBSOLETE
//OBSOLETE  .aclk                           (piShlClk),                      
//OBSOLETE  .aresetn                        (~piShlRst),
//OBSOLETE  
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- From ROLE Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- ROLE / This / Udp / Axis
//OBSOLETE  .siROL_This_Data_TDATA          (sROL_Nts0_Udp_Axis_tdataReg),
//OBSOLETE  .siROL_This_Data_TKEEP          (sROL_Nts0_Udp_Axis_tkeepReg),
//OBSOLETE  .siROL_This_Data_TLAST          (sROL_Nts0_Udp_Axis_tlastReg),
//OBSOLETE  .siROL_This_Data_TVALID         (sROL_Nts0_Udp_Axis_tvalidReg),
//OBSOLETE  .siROL_This_Data_TREADY         (sURIF_Rol_Axis_tready),
//OBSOLETE  
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- To ROLE Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- THIS / Role / Udp / Axis Output Interface
//OBSOLETE  .soTHIS_Rol_Data_TREADY         (sROL_Urif_Axis_treadyReg),
//OBSOLETE  .soTHIS_Rol_Data_TDATA          (sURIF_Rol_Axis_tdata),
//OBSOLETE  .soTHIS_Rol_Data_TKEEP          (sURIF_Rol_Axis_tkeep),
//OBSOLETE  .soTHIS_Rol_Data_TLAST          (sURIF_Rol_Axis_tlast),
//OBSOLETE  .soTHIS_Rol_Data_TVALID         (sURIF_Rol_Axis_tvalid),
//OBSOLETE
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- From UDMX / Open-Port Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- UDMX / This / OpenPortAcknowledge / Axis
//OBSOLETE  .siUDMX_This_OpnAck_TDATA       (sUDMX_Urif_OpnAck_Axis_tdata),
//OBSOLETE  .siUDMX_This_OpnAck_TVALID      (sUDMX_Urif_OpnAck_Axis_tvalid),
//OBSOLETE  .siUDMX_This_OpnAck_TREADY      (sURIF_Udmx_OpnAck_Axis_tready),
//OBSOLETE
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- To UDMX / Open-Port Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- THIS / Udmx / OpenPortRequest / Axis
//OBSOLETE  .soTHIS_Udmx_OpnReq_TREADY      (sUDMX_Urif_OpnReq_Axis_tready),
//OBSOLETE  .soTHIS_Udmx_OpnReq_TDATA       (sURIF_Udmx_OpnReq_Axis_tdata),
//OBSOLETE  .soTHIS_Udmx_OpnReq_TVALID      (sURIF_Udmx_OpnReq_Axis_tvalid),
//OBSOLETE
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- From UDMX / Data & MetaData Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- UDMX / This / Data / Axis
//OBSOLETE  .siUDMX_This_Data_TDATA         (sUDMX_Urif_Data_Axis_tdata),
//OBSOLETE  .siUDMX_This_Data_TKEEP         (sUDMX_Urif_Data_Axis_tkeep),
//OBSOLETE  .siUDMX_This_Data_TLAST         (sUDMX_Urif_Data_Axis_tlast),
//OBSOLETE  .siUDMX_This_Data_TVALID        (sUDMX_Urif_Data_Axis_tvalid),
//OBSOLETE  .siUDMX_This_Data_TREADY        (sURIF_Udmx_Data_Axis_tready),
//OBSOLETE   //-- UDMX / This / MetaData / Axis
//OBSOLETE  .siUDMX_This_Meta_TDATA         (sUDMX_Urif_Meta_Axis_tdata),
//OBSOLETE  .siUDMX_This_Meta_TVALID        (sUDMX_Urif_Meta_Axis_tvalid),
//OBSOLETE  .siUDMX_This_Meta_TREADY        (sURIF_Udmx_Meta_Axis_tready),
//OBSOLETE  
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- To UDMX / Data & MetaData Interfaces
//OBSOLETE  //------------------------------------------------------
//OBSOLETE  //-- THIS / Udmx / Data / Axis  
//OBSOLETE  .soTHIS_Udmx_Data_TREADY        (sUDMX_Urif_Data_Axis_tready),    
//OBSOLETE  .soTHIS_Udmx_Data_TDATA         (sURIF_Udmx_Data_Axis_tdata),   
//OBSOLETE  .soTHIS_Udmx_Data_TKEEP         (sURIF_Udmx_Data_Axis_tkeep),
//OBSOLETE  .soTHIS_Udmx_Data_TLAST         (sURIF_Udmx_Data_Axis_tlast),
//OBSOLETE  .soTHIS_Udmx_Data_TVALID        (sURIF_Udmx_Data_Axis_tvalid),
//OBSOLETE  //-- THIS / Udmx / MetaData / Axis
//OBSOLETE  .soTHIS_Udmx_Meta_TREADY        (sUDMX_Urif_Meta_Axis_tready),
//OBSOLETE  .soTHIS_Udmx_Meta_TDATA         (sURIF_Udmx_Meta_Axis_tdata),
//OBSOLETE  .soTHIS_Udmx_Meta_TVALID        (sURIF_Udmx_Meta_Axis_tvalid),
//OBSOLETE  //-- THIS / Udmx / Tx Length / Axis
//OBSOLETE  .soTHIS_Udmx_PLen_TREADY        (sUDMX_Urif_PLen_Axis_tready),
//OBSOLETE  .soTHIS_Udmx_PLen_TDATA         (sURIF_Udmx_PLen_Axis_tdata),
//OBSOLETE  .soTHIS_Udmx_PLen_TVALID        (sURIF_Udmx_PLen_Axis_tvalid)
//OBSOLETE
//OBSOLETE);
//OBSOLETE
//OBSOLETE`else // !`ifdef USE_DEPRECATED_DIRECTIVES
//OBSOLETE 
//OBSOLETE  UdpRoleInterface URIF (
//OBSOLETE  
//OBSOLETE    .ap_clk                         (piShlClk),                      
//OBSOLETE    .ap_rst_n                       (~piShlRst),
//OBSOLETE    
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- From ROLE Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- ROLE / This / Udp / Axis
//OBSOLETE    .siROL_This_Data_TDATA          (sROL_Nts0_Udp_Axis_tdataReg),
//OBSOLETE    .siROL_This_Data_TKEEP          (sROL_Nts0_Udp_Axis_tkeepReg),
//OBSOLETE    .siROL_This_Data_TLAST          (sROL_Nts0_Udp_Axis_tlastReg),
//OBSOLETE    .siROL_This_Data_TVALID         (sROL_Nts0_Udp_Axis_tvalidReg),
//OBSOLETE    .siROL_This_Data_TREADY         (sURIF_Rol_Axis_tready),
//OBSOLETE    
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- To ROLE Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- THIS / Role / Udp / Axis Output Interface
//OBSOLETE    .soTHIS_Rol_Data_TREADY         (sROL_Urif_Axis_treadyReg),
//OBSOLETE    .soTHIS_Rol_Data_TDATA          (sURIF_Rol_Axis_tdata),
//OBSOLETE    .soTHIS_Rol_Data_TKEEP          (sURIF_Rol_Axis_tkeep),
//OBSOLETE    .soTHIS_Rol_Data_TLAST          (sURIF_Rol_Axis_tlast),
//OBSOLETE    .soTHIS_Rol_Data_TVALID         (sURIF_Rol_Axis_tvalid),
//OBSOLETE
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- From UDMX / Open-Port Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- UDMX / This / OpenPortAcknowledge / Axis
//OBSOLETE    .siUDMX_This_OpnAck_V_TDATA     (sUDMX_Urif_OpnAck_Axis_tdata),
//OBSOLETE    .siUDMX_This_OpnAck_V_TVALID    (sUDMX_Urif_OpnAck_Axis_tvalid),
//OBSOLETE    .siUDMX_This_OpnAck_V_TREADY    (sURIF_Udmx_OpnAck_Axis_tready),
//OBSOLETE
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- To UDMX / Open-Port Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- THIS / Udmx / OpenPortRequest / Axis
//OBSOLETE    .soTHIS_Udmx_OpnReq_V_V_TREADY  (sUDMX_Urif_OpnReq_Axis_tready),
//OBSOLETE    .soTHIS_Udmx_OpnReq_V_V_TDATA   (sURIF_Udmx_OpnReq_Axis_tdata),
//OBSOLETE    .soTHIS_Udmx_OpnReq_V_V_TVALID  (sURIF_Udmx_OpnReq_Axis_tvalid),
//OBSOLETE
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- From UDMX / Data & MetaData Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- UDMX / This / Data / Axis
//OBSOLETE    .siUDMX_This_Data_TDATA         (sUDMX_Urif_Data_Axis_tdata),
//OBSOLETE    .siUDMX_This_Data_TKEEP         (sUDMX_Urif_Data_Axis_tkeep),
//OBSOLETE    .siUDMX_This_Data_TLAST         (sUDMX_Urif_Data_Axis_tlast),
//OBSOLETE    .siUDMX_This_Data_TVALID        (sUDMX_Urif_Data_Axis_tvalid),
//OBSOLETE    .siUDMX_This_Data_TREADY        (sURIF_Udmx_Data_Axis_tready),
//OBSOLETE     //-- UDMX / This / MetaData / Axis
//OBSOLETE    .siUDMX_This_Meta_TDATA         (sUDMX_Urif_Meta_Axis_tdata),
//OBSOLETE    .siUDMX_This_Meta_TVALID        (sUDMX_Urif_Meta_Axis_tvalid),
//OBSOLETE    .siUDMX_This_Meta_TREADY        (sURIF_Udmx_Meta_Axis_tready),
//OBSOLETE    
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- To UDMX / Data & MetaData Interfaces
//OBSOLETE    //------------------------------------------------------
//OBSOLETE    //-- THIS / Udmx / Data / Axis  
//OBSOLETE    .soTHIS_Udmx_Data_TREADY        (sUDMX_Urif_Data_Axis_tready),    
//OBSOLETE    .soTHIS_Udmx_Data_TDATA         (sURIF_Udmx_Data_Axis_tdata),   
//OBSOLETE    .soTHIS_Udmx_Data_TKEEP         (sURIF_Udmx_Data_Axis_tkeep),
//OBSOLETE    .soTHIS_Udmx_Data_TLAST         (sURIF_Udmx_Data_Axis_tlast),
//OBSOLETE    .soTHIS_Udmx_Data_TVALID        (sURIF_Udmx_Data_Axis_tvalid),
//OBSOLETE    //-- THIS / Udmx / MetaData / Axis
//OBSOLETE    .soTHIS_Udmx_Meta_TREADY        (sUDMX_Urif_Meta_Axis_tready),
//OBSOLETE    .soTHIS_Udmx_Meta_TDATA         (sURIF_Udmx_Meta_Axis_tdata),
//OBSOLETE    .soTHIS_Udmx_Meta_TVALID        (sURIF_Udmx_Meta_Axis_tvalid),
//OBSOLETE    //-- THIS / Udmx / Tx Length / Axis
//OBSOLETE    .soTHIS_Udmx_PLen_V_V_TREADY    (sUDMX_Urif_PLen_Axis_tready),
//OBSOLETE    .soTHIS_Udmx_PLen_V_V_TDATA     (sURIF_Udmx_PLen_Axis_tdata),
//OBSOLETE    .soTHIS_Udmx_PLen_V_V_TVALID    (sURIF_Udmx_PLen_Axis_tvalid)
//OBSOLETE
//OBSOLETE  );
//OBSOLETE   
//OBSOLETE`endif // !`ifdef USE_DEPRECATED_DIRECTIVES
   
 
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_udp_application_interface_1 URIF (
//    .lbPortOpenReplyIn_TVALID(lbPortOpenReplyIn_TVALID),      
//    .lbPortOpenReplyIn_TREADY(lbPortOpenReplyIn_TREADY),      
//    .lbPortOpenReplyIn_TDATA(lbPortOpenReplyIn_TDATA),       
     
//    .lbRequestPortOpenOut_TVALID(lbRequestPortOpenOut_TVALID),   
//    .lbRequestPortOpenOut_TREADY(lbRequestPortOpenOut_TREADY),   
//    .lbRequestPortOpenOut_TDATA(lbRequestPortOpenOut_TDATA),     
    
//    .lbRxDataIn_TVALID(lbRxDataIn_TVALID),                        
//    .lbRxDataIn_TREADY(lbRxDataIn_TREADY),                          
//    .lbRxDataIn_TDATA(lbRxDataIn_TDATA),                       
//    .lbRxDataIn_TKEEP(lbRxDataIn_TKEEP),                         
//    .lbRxDataIn_TLAST(lbRxDataIn_TLAST),                          
     
//    .lbRxMetadataIn_TVALID(lbRxMetadataIn_TVALID),            
//    .lbRxMetadataIn_TREADY(lbRxMetadataIn_TREADY),        
//    .lbRxMetadataIn_TDATA(lbRxMetadataIn_TDATA),              
    
//    .lbTxDataOut_TVALID(lbTxDataOut_TVALID),                  
//    .lbTxDataOut_TREADY(lbTxDataOut_TREADY),                       
//    .lbTxDataOut_TDATA(lbTxDataOut_TDATA),                                
//    .lbTxDataOut_TKEEP(lbTxDataOut_TKEEP),                     
//    .lbTxDataOut_TLAST(lbTxDataOut_TLAST),                       
    
//    .lbTxLengthOut_TVALID(lbTxLengthOut_TVALID),            
//    .lbTxLengthOut_TREADY(lbTxLengthOut_TREADY),         
//    .lbTxLengthOut_TDATA(lbTxLengthOut_TDATA),           
    
//    .lbTxMetadataOut_TVALID(lbTxMetadataOut_TVALID),          
//    .lbTxMetadataOut_TREADY(lbTxMetadataOut_TREADY),         
//    .lbTxMetadataOut_TDATA(lbTxMetadataOut_TDATA),       
                  
//    .vFPGA_UDP_Rx_Data_Out_TVALID(uai_to_app_slice_tvalid),
//    .vFPGA_UDP_Rx_Data_Out_TREADY(uai_to_app_slice_tready),
//    .vFPGA_UDP_Rx_Data_Out_TDATA(uai_to_app_slice_tdata),
//    .vFPGA_UDP_Rx_Data_Out_TKEEP(uai_to_app_slice_tkeep),
//    .vFPGA_UDP_Rx_Data_Out_TLAST(uai_to_app_slice_tlast),
  
//    .vFPGA_UDP_Tx_Data_in_TVALID(app_to_uai_slice_tvalid),
//    .vFPGA_UDP_Tx_Data_in_TREADY(app_to_uai_slice_tready),
//    .vFPGA_UDP_Tx_Data_in_TDATA(app_to_uai_slice_tdata),
//    .vFPGA_UDP_Tx_Data_in_TKEEP(app_to_uai_slice_tkeep),
//    .vFPGA_UDP_Tx_Data_in_TLAST(app_to_uai_slice_tlast),
    
//    .aclk(cf_axi_clk),                                       
//    .aresetn(cf_aresetn)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

  //============================================================================
  //  INST: DHCP-CLIENT -- [TOOD - Remove this useless DHCP-client module]
  //============================================================================
`ifdef USE_DEPRECATED_DIRECTIVES

  DynamicHostConfigurationProcess DHCP (
  
    .aclk                           (piShlClk),                      
    .aresetn                        (~piShlRst),

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
    .siUDMX_This_OpnAck_TDATA       (sUDMX_Dhcp_OpnAck_Axis_tdata),
    .siUDMX_This_OpnAck_TVALID      (sUDMX_Dhcp_OpnAck_Axis_tvalid), 
    .siUDMX_This_OpnAck_TREADY      (sDHCP_Udmx_OpnAck_Axis_tready),
    
    //------------------------------------------------------
    //-- To UDMX / Open-Port Interfaces
    //------------------------------------------------------     
    //-- THIS / Udmx / OpenPortRequest / Axis
    .soTHIS_Udmx_OpnReq_TREADY      (sUDMX_Dhcp_OpnReq_Axis_tready),
    .soTHIS_Udmx_OpnReq_TDATA       (sDHCP_Udmx_OpnReq_Axis_tdata),
    .soTHIS_Udmx_OpnReq_TVALID      (sDHCP_Udmx_OpnReq_Axis_tvalid),
     
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
    .soTHIS_Udmx_PLen_TREADY        (sUDMX_Dhcp_PLen_Axis_tready),
    .soTHIS_Udmx_PLen_TDATA         (sDHCP_Udmx_PLen_Axis_tdata),
    .soTHIS_Udmx_PLen_TVALID        (sDHCP_Udmx_PLen_Axis_tvalid)
   
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
   
   
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_dhcp_client DHCP (
//    .dhcpEnable_V(1'b1),                                     
//    //.inputIpAddress_V(inputIpAddress),                           
//    .inputIpAddress_V(cloud_fpga_ip),
//    .dhcpIpAddressOut_V(cloud_fpga_ip),                          
//    .myMacAddress_V(cloud_fpga_mac),                                 
        
//    .m_axis_open_port_TVALID(dhcp2mux_requestPortOpenOut_V_TVALID),   
//    .m_axis_open_port_TREADY(dhcp2mux_requestPortOpenOut_V_TREADY),   
//    .m_axis_open_port_TDATA(dhcp2mux_requestPortOpenOut_V_TDATA),      
        
//    .m_axis_tx_data_TVALID(dhcp2mux_TVALID),                           
//    .m_axis_tx_data_TREADY(dhcp2mux_TREADY),                           
//    .m_axis_tx_data_TDATA(dhcp2mux_TDATA),                             
//    .m_axis_tx_data_TKEEP(dhcp2mux_TKEEP),                             
//    .m_axis_tx_data_TLAST(dhcp2mux_TLAST),                             
       
//    .m_axis_tx_length_TVALID(dhcp2muxTxLengthOut_V_V_TVALID),          
//    .m_axis_tx_length_TREADY(dhcp2muxTxLengthOut_V_V_TREADY),          
//    .m_axis_tx_length_TDATA(dhcp2muxTxLengthOut_V_V_TDATA),            
        
//    .m_axis_tx_metadata_TVALID(dhcp2muxTxMetadataOut_V_TVALID),        
//    .m_axis_tx_metadata_TREADY(dhcp2muxTxMetadataOut_V_TREADY),        
//    .m_axis_tx_metadata_TDATA(dhcp2muxTxMetadataOut_V_TDATA),          
        
//    .s_axis_open_port_status_TVALID(mux2dhcp_portOpenReplyIn_V_V_TVALID), 
//    .s_axis_open_port_status_TREADY(mux2dhcp_portOpenReplyIn_V_V_TREADY), 
//    .s_axis_open_port_status_TDATA(mux2dhcp_portOpenReplyIn_V_V_TDATA),   
        
//    .s_axis_rx_data_TVALID(mux2dhcpRxDataIn_TVALID),                      
//    .s_axis_rx_data_TREADY(mux2dhcpRxDataIn_TREADY),                      
//    .s_axis_rx_data_TDATA(mux2dhcpRxDataIn_TDATA),                        
//    .s_axis_rx_data_TKEEP(mux2dhcpRxDataIn_TKEEP),                        
//    .s_axis_rx_data_TLAST(mux2dhcpRxDataIn_TLAST),                        
        
//    .s_axis_rx_metadata_TVALID(mux2dhcpRxMetadataIn_V_TVALID),            
//    .s_axis_rx_metadata_TREADY(mux2dhcpRxMetadataIn_V_TREADY),            
//    .s_axis_rx_metadata_TDATA(mux2dhcpRxMetadataIn_V_TDATA),              
       
//    .aclk(cf_axi_clk),                                                    
//    .aresetn(cf_aresetn)                                                  
//  );    
 -----/\----- [OBSOLETE] -----/\----- */

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
    .aresetn                  (~piShlRst),

    //------------------------------------------------------
    //-- From MMIO Interfaces
    //------------------------------------------------------                     
    .piMMIO_This_IpAddr_V (piMMIO_Nts0_IpAddress),
  
    //------------------------------------------------------
    //-- From IPRX Interfaces
    //------------------------------------------------------
    //-- IPRX / This / Data / Axis
    .s_dataIn_TDATA     (sIPRX_Icmp_Data_Axis_tdataReg),
    .s_dataIn_TKEEP     (sIPRX_Icmp_Data_Axis_tkeepReg),
    .s_dataIn_TLAST     (sIPRX_Icmp_Data_Axis_tlastReg),
    .s_dataIn_TVALID    (sIPRX_Icmp_Data_Axis_tvalidReg),
    .s_dataIn_TREADY    (sICMP_Iprx_Data_Axis_tready),
    //-- IPRX / This / Ttl / Axis
    .s_ttlIn_TDATA      (sIPRX_Icmp_Ttl_Axis_tdata),       
    .s_ttlIn_TKEEP      (sIPRX_Icmp_Ttl_Axis_tkeep),
    .s_ttlIn_TLAST      (sIPRX_Icmp_Ttl_Axis_tlast),
    .s_ttlIn_TVALID     (sIPRX_Icmp_Ttl_Axis_tvalid),
    .s_ttlIn_TREADY     (sICMP_Iprx_Ttl_Axis_tready),
    
    //------------------------------------------------------
    //-- From UDP Interfaces
    //------------------------------------------------------
    //-- UDP / This / Axis   
    .s_udpIn_TDATA      (sUDP_Icmp_Axis_tdata),
    .s_udpIn_TKEEP      (sUDP_Icmp_Axis_tkeep),
    .s_udpIn_TLAST      (sUDP_Icmp_Axis_tlast),
    .s_udpIn_TVALID     (sUDP_Icmp_Axis_tvalid),
    .s_udpIn_TREADY     (sICMP_Udp_Axis_tready),
    
    //------------------------------------------------------
    //-- From L3MUX Interfaces
    //------------------------------------------------------
    //-- THIS / L3mux / Axis
    .m_dataOut_TREADY   (sL3MUX_Icmp_Axis_tready),  
    .m_dataOut_TDATA    (sICMP_L3mux_Axis_tdata),
    .m_dataOut_TKEEP    (sICMP_L3mux_Axis_tkeep),
    .m_dataOut_TLAST    (sICMP_L3mux_Axis_tlast),
    .m_dataOut_TVALID   (sICMP_L3mux_Axis_tvalid)

  ); // End of ICMP

`endif // `ifdef USE_DEPRECATED_DIRECTIVES
   
   
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_icmp_server_1 ICMP (
//    .s_dataIn_TVALID(axi_icmp_slice_to_icmp_tvalid),   
//    .s_dataIn_TREADY(axi_icmp_slice_to_icmp_tready),   
//    .s_dataIn_TDATA(axi_icmp_slice_to_icmp_tdata),     
//    .s_dataIn_TKEEP(axi_icmp_slice_to_icmp_tkeep),     
//    .s_dataIn_TLAST(axi_icmp_slice_to_icmp_tlast),     
    
//    .s_udpIn_TVALID(axis_udp_to_icmp_tvalid),          
//    .s_udpIn_TREADY(axis_udp_to_icmp_tready),          
//    .s_udpIn_TDATA(axis_udp_to_icmp_tdata),            
//    .s_udpIn_TKEEP(axis_udp_to_icmp_tkeep),           
//    .s_udpIn_TLAST(axis_udp_to_icmp_tlast),           
    
//    .s_ttlIn_TVALID(axis_ttl_to_icmp_tvalid),          
//    .s_ttlIn_TREADY(axis_ttl_to_icmp_tready),          
//    .s_ttlIn_TDATA(axis_ttl_to_icmp_tdata),            
//    .s_ttlIn_TKEEP(axis_ttl_to_icmp_tkeep),            
//    .s_ttlIn_TLAST(axis_ttl_to_icmp_tlast),            
    
//    .m_dataOut_TVALID(sICMP_L3mux_Axis_tvalid),  
//    .m_dataOut_TREADY(axi_icmp_to_icmp_slice_tready),  
//    .m_dataOut_TDATA(axi_icmp_to_icmp_slice_tdata),    
//    .m_dataOut_TKEEP(axi_icmp_to_icmp_slice_tkeep),    
//    .m_dataOut_TLAST(axi_icmp_to_icmp_slice_tlast),    
    
//    .aclk(cf_axi_clk),                             
//    .aresetn(cf_aresetn)                            
//  );
 -----/\----- [OBSOLETE] -----/\----- */
   
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
    .S00_AXIS_TDATA     (sICMP_L3mux_Axis_tdata),
    .S00_AXIS_TKEEP     (sICMP_L3mux_Axis_tkeep),
    .S00_AXIS_TLAST     (sICMP_L3mux_Axis_tlast),
    .S00_AXIS_TVALID    (sICMP_L3mux_Axis_tvalid),
    .S00_AXIS_TREADY    (sL3MUX_Icmp_Axis_tready),
    
    //------------------------------------------------------
    //-- From UDP Interfaces
    //------------------------------------------------------
    .S01_AXIS_TDATA     (sUDP_L3mux_Axis_tdata), 
    .S01_AXIS_TKEEP     (sUDP_L3mux_Axis_tkeep),
    .S01_AXIS_TLAST     (sUDP_L3mux_Axis_tlast),
    .S01_AXIS_TVALID    (sUDP_L3mux_Axis_tvalid),
    .S01_AXIS_TREADY    (sL3MUX_Udp_Axis_tready),
    
    //------------------------------------------------------
    //-- From TOE Interfaces
    //------------------------------------------------------
    .S02_AXIS_TDATA     (sTOE_L3mux_Axix_tdataReg),
    .S02_AXIS_TKEEP     (sTOE_L3mux_Axix_tkeepReg),
    .S02_AXIS_TLAST     (sTOE_L3mux_Axix_tlastReg),
    .S02_AXIS_TVALID    (sTOE_L3mux_Axix_tvalidReg),
     .S02_AXIS_TREADY    (sL3MUX_Toe_Axix_tready),
         
             
    .M00_AXIS_ACLK      (piShlClk),        
    .M00_AXIS_ARESETN   (~piShlRst),    
 
    //------------------------------------------------------
    //-- To IPTX Interfaces
    //------------------------------------------------------
    .M00_AXIS_TREADY    (sIPTX_L3mux_Axis_tready),
    .M00_AXIS_TDATA     (sL3MUX_Iptx_Axis_tdata),     
    .M00_AXIS_TKEEP     (sL3MUX_Iptx_Axis_tkeep),     
    .M00_AXIS_TLAST     (sL3MUX_Iptx_Axis_tlast),
    .M00_AXIS_TVALID    (sL3MUX_Iptx_Axis_tvalid),     

    .S00_ARB_REQ_SUPPRESS(1'b0),  
    .S01_ARB_REQ_SUPPRESS(1'b0),
    .S02_ARB_REQ_SUPPRESS(1'b0)  
  );
  
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_interconnect_3_to_1 L3MRG (
//    .ACLK(cf_axi_clk),                         
//    .ARESETN(cf_aresetn),                 
 
//    .S00_AXIS_ACLK(cf_axi_clk),           
//    .S01_AXIS_ACLK(cf_axi_clk),            
//    .S02_AXIS_ACLK(cf_axi_clk),        
 
//    .S00_AXIS_ARESETN(cf_aresetn),       
//    .S01_AXIS_ARESETN(cf_aresetn),       
//    .S02_AXIS_ARESETN(cf_aresetn),     
 
//    .S00_AXIS_TVALID(axi_icmp_to_icmp_slice_tvalid),      
//    .S01_AXIS_TVALID(axi_udp_to_merge_tvalid),          
//    .S02_AXIS_TVALID(axi_toe_to_toe_slice_tvalid),         
 
//    .S00_AXIS_TREADY(axi_icmp_to_icmp_slice_tready),      
//    .S01_AXIS_TREADY(axi_udp_to_merge_tready),           
//    .S02_AXIS_TREADY(axi_toe_to_toe_slice_tready),       
 
//    .S00_AXIS_TDATA(axi_icmp_to_icmp_slice_tdata),       
//    .S01_AXIS_TDATA(axi_udp_to_merge_tdata),            
//    .S02_AXIS_TDATA(axi_toe_to_toe_slice_tdata),       
 
//    .S00_AXIS_TKEEP(axi_icmp_to_icmp_slice_tkeep),        
//    .S01_AXIS_TKEEP(axi_udp_to_merge_tkeep),       
//    .S02_AXIS_TKEEP(axi_toe_to_toe_slice_tkeep),     
 
//    .S00_AXIS_TLAST(axi_icmp_to_icmp_slice_tlast),     
//    .S01_AXIS_TLAST(axi_udp_to_merge_tlast),         
//    .S02_AXIS_TLAST(axi_toe_to_toe_slice_tlast),  
 
//    .M00_AXIS_ACLK(cf_axi_clk),        
//    .M00_AXIS_ARESETN(cf_aresetn),    
 
//    .M00_AXIS_TVALID(axi_intercon_to_mie_tvalid),     
//    .M00_AXIS_TREADY(axi_intercon_to_mie_tready),      
//    .M00_AXIS_TDATA(axi_intercon_to_mie_tdata),     
//    .M00_AXIS_TKEEP(axi_intercon_to_mie_tkeep),     
//    .M00_AXIS_TLAST(axi_intercon_to_mie_tlast),     
 
//    .S00_ARB_REQ_SUPPRESS(1'b0),  
//    .S01_ARB_REQ_SUPPRESS(1'b0),
//    .S02_ARB_REQ_SUPPRESS(1'b0)  
//  );
 -----/\----- [OBSOLETE] -----/\----- */
   
  //============================================================================
  //  INST: IP TX HANDLER
  //============================================================================
  IpTxHandler IPTX (
  
    .aclk                     (piShlClk),         
    .aresetn                  (~piShlRst),  
  
    //------------------------------------------------------
    //-- From L3MUX Interfaces
    //------------------------------------------------------
    //-- L3MUX / This / Axis
    .s_dataIn_TDATA           (sL3MUX_Iptx_Axis_tdata),
    .s_dataIn_TKEEP           (sL3MUX_Iptx_Axis_tkeep),
    .s_dataIn_TLAST           (sL3MUX_Iptx_Axis_tlast),
    .s_dataIn_TVALID          (sL3MUX_Iptx_Axis_tvalid),
    .s_dataIn_TREADY          (sIPTX_L3mux_Axis_tready),
  
    //------------------------------------------------------
    //-- From ARP Interfaces
    //------------------------------------------------------
    //-- ARP / This / LookupReply / Axis
    .s_arpTableIn_TDATA       (sARP_Iptx_LkpRpl_Axis_tdata),
    .s_arpTableIn_TVALID      (sARP_Iptx_LkpRpl_Axis_tvalid),
    .s_arpTableIn_TREADY      (sIPTX_Arp_LkpRpl_Axis_tready), 
  
    //------------------------------------------------------
    //-- To :L2MUX Interfaces
    //------------------------------------------------------
    //-- THIS / L2mux / Axis
    .m_dataOut_TREADY         (sL2MUX_Iptx_Axis_tready),
    .m_dataOut_TDATA          (sIPTX_L2mux_Axis_tdata),
    .m_dataOut_TKEEP          (sIPTX_L2mux_Axis_tkeep),
    .m_dataOut_TLAST          (sIPTX_L2mux_Axis_tlast),
    .m_dataOut_TVALID         (sIPTX_L2mux_Axis_tvalid),              
  
     //------------------------------------------------------
     //-- To ARP Interfaces
     //------------------------------------------------------
     //-- THIS / Arp / LookupRequest / Axis
    .m_arpTableOut_TREADY     (sARP_Iptx_LkpReq_Axis_tready),
    .m_arpTableOut_TDATA      (sIPTX_Arp_LkpReq_Axis_tdata),
    .m_arpTableOut_TVALID     (sIPTX_Arp_LkpReq_Axis_tvalid),
  
    .regSubNetMask_V          (piMMIO_Nts0_SubNetMask),   //OBSOLETE-20181005 (32'h00FFFFFF), 
    .regDefaultGateway_V      (piMMIO_Nts0_GatewayAddr),   //OBSOLETE-20181005 (32'h01010101),   
    .myMacAddress_V           (piMMIO_Nts0_MacAddress) 
    
  ); // End of IPTX
    
/* -----\/----- [OBSOLETE] -----\/-----
//  cloudFPGA_ip_module_tx_path_2 IPTX (
//    .s_dataIn_TVALID(axi_intercon_to_mie_tvalid),                  
//    .s_dataIn_TREADY(axi_intercon_to_mie_tready),                   
//    .s_dataIn_TDATA(axi_intercon_to_mie_tdata),                  
//    .s_dataIn_TKEEP(axi_intercon_to_mie_tkeep),                   
//    .s_dataIn_TLAST(axi_intercon_to_mie_tlast),         
  
//    .s_arpTableIn_TVALID(axis_arp_lookup_reply_TVALID),      
//    .s_arpTableIn_TREADY(axis_arp_lookup_reply_TREADY),      
//    .s_arpTableIn_TDATA(axis_arp_lookup_reply_TDATA),     
  
//    .m_dataOut_TVALID(axi_mie_to_intercon_tvalid),            
//    .m_dataOut_TREADY(axi_mie_to_intercon_tready),           
//    .m_dataOut_TDATA(axi_mie_to_intercon_tdata),             
//    .m_dataOut_TKEEP(axi_mie_to_intercon_tkeep),            
//    .m_dataOut_TLAST(axi_mie_to_intercon_tlast),            
  
//    .m_arpTableOut_TVALID(axis_arp_lookup_request_TVALID), 
//    .m_arpTableOut_TREADY(axis_arp_lookup_request_TREADY), 
//    .m_arpTableOut_TDATA(axis_arp_lookup_request_TDATA),  
  
//    .regSubNetMask_V(32'h00FFFFFF),          
//    .regDefaultGateway_V(32'h01010101),   
//    .myMacAddress_V(cloud_fpga_mac),      
    
//    .aclk(cf_axi_clk),         
//    .aresetn(cf_aresetn)    
//  );
 -----/\----- [OBSOLETE] -----/\----- */

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
    //-- From ARP Interfaces
    //------------------------------------------------------   
    //-- ARP / This / Axis
    .S00_AXIS_TDATA       (sARP_L2mux_Axis_tdata),
    .S00_AXIS_TKEEP       (sARP_L2mux_Axis_tkeep),
    .S00_AXIS_TLAST       (sARP_L2mux_Axis_tlast),
    .S00_AXIS_TVALID      (sARP_L2mux_Axis_tvalid),
    .S00_AXIS_TREADY      (sL2MUX_Arp_Axis_tready), 
 
    //------------------------------------------------------
    //-- From IPTX Interfaces
    //------------------------------------------------------   
    //-- IPTX / This / Axis
    .S01_AXIS_TDATA       (sIPTX_L2mux_Axis_tdata),
    .S01_AXIS_TKEEP       (sIPTX_L2mux_Axis_tkeep),
    .S01_AXIS_TLAST       (sIPTX_L2mux_Axis_tlast),
    .S01_AXIS_TVALID      (sIPTX_L2mux_Axis_tvalid),
    .S01_AXIS_TREADY      (sL2MUX_Iptx_Axis_tready),
 
 
    .M00_AXIS_ACLK        (piShlClk), 
    .M00_AXIS_ARESETN     (~piShlRst), 
 
    //------------------------------------------------------
    //-- To NTS0 / Eth0 / Axis Interfaces
    //------------------------------------------------------   
    //-- NTS0 / Eth0 / Axis
    .M00_AXIS_TREADY      (piETH0_Nts0_Axis_tready),
    .M00_AXIS_TDATA       (poNTS0_Eth0_Axis_tdata), 
    .M00_AXIS_TKEEP       (poNTS0_Eth0_Axis_tkeep), 
    .M00_AXIS_TLAST       (poNTS0_Eth0_Axis_tlast),
    .M00_AXIS_TVALID      (poNTS0_Eth0_Axis_tvalid), 
 
    .S00_ARB_REQ_SUPPRESS (1'b0), 
    .S01_ARB_REQ_SUPPRESS (1'b0)
  );
      
/* -----\/----- [OBSOLETE] -----\/-----
//  axis_interconnect_2to1 L2MRG (
//    .ACLK(cf_axi_clk), 
//    .ARESETN(cf_aresetn), 
 
//    .S00_AXIS_ACLK(cf_axi_clk), 
//    .S01_AXIS_ACLK(cf_axi_clk), 
//    .S00_AXIS_ARESETN(cf_aresetn), 
//    .S01_AXIS_ARESETN(cf_aresetn),
 
//    .S00_AXIS_TVALID(axi_arp_to_arp_slice_tvalid), 
//    .S01_AXIS_TVALID(axi_mie_to_intercon_tvalid), 
 
//    .S00_AXIS_TREADY(axi_arp_to_arp_slice_tready), 
//    .S01_AXIS_TREADY(axi_mie_to_intercon_tready), 
 
//    .S00_AXIS_TDATA(axi_arp_to_arp_slice_tdata), 
//    .S01_AXIS_TDATA(axi_mie_to_intercon_tdata), 
 
//    .S00_AXIS_TKEEP(axi_arp_to_arp_slice_tkeep), 
//    .S01_AXIS_TKEEP(axi_mie_to_intercon_tkeep),
 
//    .S00_AXIS_TLAST(axi_arp_to_arp_slice_tlast), 
//    .S01_AXIS_TLAST(axi_mie_to_intercon_tlast), 
 
//    .M00_AXIS_ACLK(cf_axi_clk), 
//    .M00_AXIS_ARESETN(cf_aresetn), 
 
//    .M00_AXIS_TVALID(tx_data_TVALID),
//    .M00_AXIS_TREADY(tx_data_TREADY), 
//    .M00_AXIS_TDATA(tx_data_TDATA), 
//    .M00_AXIS_TKEEP(tx_data_TKEEP), 
//    .M00_AXIS_TLAST(tx_data_TLAST), 
 
//    .S00_ARB_REQ_SUPPRESS(1'b0), 
//    .S01_ARB_REQ_SUPPRESS(1'b0)
//  );
 -----/\----- [OBSOLETE] -----/\----- */

endmodule