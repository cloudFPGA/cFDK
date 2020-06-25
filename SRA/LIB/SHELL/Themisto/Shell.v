//                              -*- Mode: Verilog -*-
//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        Shell with node2node communication for UDP & TCP
//  *


`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - SHELL FOR FMKU60
// *****************************************************************************

module Shell_Themisto # (

  parameter gSecurityPriviledges = "super", // "user" or "super"
  parameter gBitstreamUsage      = "flash", // "user" or "flash"
  parameter gMmioAddrWidth       =      8,  // Default is 8-bits
  parameter gMmioDataWidth       =      8   // Default is 8-bits

) (

  //------------------------------------------------------
  //-- TOP / Input Clocks and Resets from topFMKU60
  //------------------------------------------------------
  input           piTOP_156_25Rst,
  input           piTOP_156_25Clk,

  //------------------------------------------------------
  //-- TOP / Bitstream Identification
  //------------------------------------------------------
  input  [31: 0]  piTOP_Timestamp,
  
  //------------------------------------------------------
  //-- CLKT / Clock Tree Interface 
  //------------------------------------------------------
  input           piCLKT_Mem0Clk_n,
  input           piCLKT_Mem0Clk_p,
  input           piCLKT_Mem1Clk_n,
  input           piCLKT_Mem1Clk_p,
  input           piCLKT_10GeClk_n,
  input           piCLKT_10GeClk_p,

  //------------------------------------------------------
  //-- PSOC / External Memory Interface (Emif)
  //------------------------------------------------------
  input           piPSOC_Emif_Clk,
  input           piPSOC_Emif_Cs_n,
  input           piPSOC_Emif_We_n,
  input           piPSOC_Emif_Oe_n,
  input           piPSOC_Emif_AdS_n,
  input [gMmioAddrWidth-1: 0]  
                  piPSOC_Emif_Addr,
  inout [gMmioDataWidth-1: 0]  
                  pioPSOC_Emif_Data,

  //------------------------------------------------------
  //-- LED / Heart Beat Interface (Yellow LED)
  //------------------------------------------------------
  output          poLED_HeartBeat_n,

  //------------------------------------------------------
  // -- DDR4 / Memory Channel 0 Interface (Mc0)
  //------------------------------------------------------
  inout  [ 8:0]   pioDDR4_Mem_Mc0_DmDbi_n,
  inout  [71:0]   pioDDR4_Mem_Mc0_Dq,
  inout  [ 8:0]   pioDDR4_Mem_Mc0_Dqs_n,
  inout  [ 8:0]   pioDDR4_Mem_Mc0_Dqs_p,
  output          poDDR4_Mem_Mc0_Act_n,
  output [16:0]   poDDR4_Mem_Mc0_Adr,
  output [ 1:0]   poDDR4_Mem_Mc0_Ba,
  output [ 1:0]   poDDR4_Mem_Mc0_Bg,
  output [ 0:0]   poDDR4_Mem_Mc0_Cke,
  output [ 0:0]   poDDR4_Mem_Mc0_Odt,
  output [ 0:0]   poDDR4_Mem_Mc0_Cs_n,
  output [ 0:0]   poDDR4_Mem_Mc0_Ck_n,
  output [ 0:0]   poDDR4_Mem_Mc0_Ck_p,
  output          poDDR4_Mem_Mc0_Reset_n,

  //------------------------------------------------------
  //-- DDR4 / Memory Channel 1 Interface (Mc1)
  //------------------------------------------------------  
  inout  [ 8:0]   pioDDR4_Mem_Mc1_DmDbi_n,
  inout  [71:0]   pioDDR4_Mem_Mc1_Dq,
  inout  [ 8:0]   pioDDR4_Mem_Mc1_Dqs_n,
  inout  [ 8:0]   pioDDR4_Mem_Mc1_Dqs_p,
  output          poDDR4_Mem_Mc1_Act_n,
  output [16:0]   poDDR4_Mem_Mc1_Adr,
  output [ 1:0]   poDDR4_Mem_Mc1_Ba,
  output [ 1:0]   poDDR4_Mem_Mc1_Bg,
  output [ 0:0]   poDDR4_Mem_Mc1_Cke,
  output [ 0:0]   poDDR4_Mem_Mc1_Odt,
  output [ 0:0]   poDDR4_Mem_Mc1_Cs_n,
  output [ 0:0]   poDDR4_Mem_Mc1_Ck_n,
  output [ 0:0]   poDDR4_Mem_Mc1_Ck_p,
  output          poDDR4_Mem_Mc1_Reset_n,

  //------------------------------------------------------
  //-- ECON / Edge Connector Interface (SPD08-200)
  //------------------------------------------------------
  input           piECON_Eth_10Ge0_n, 
  input           piECON_Eth_10Ge0_p,
  output          poECON_Eth_10Ge0_n,
  output          poECON_Eth_10Ge0_p,

  //------------------------------------------------------
  //-- ROLE / Reset and Clock Interfaces
  //------------------------------------------------------
  output          poROL_156_25Clk,
  output          poROL_156_25Rst,

  //------------------------------------------------------
  //-- ROLE / Nts / UDP Interfaces
  //------------------------------------------------------
  //---- Input UDP Data (AXI4S) --------
  input  [ 63:0]  siROL_Nts_Udp_Data_tdata,
  input  [  7:0]  siROL_Nts_Udp_Data_tkeep,
  input           siROL_Nts_Udp_Data_tlast,
  input           siROL_Nts_Udp_Data_tvalid,
  output          siROL_Nts_Udp_Data_tready,
  //---- Output UDP Data (AXI4S) -------
  output [ 63:0]  soROL_Nts_Udp_Data_tdata,
  output [  7:0]  soROL_Nts_Udp_Data_tkeep,
  output          soROL_Nts_Udp_Data_tlast,
  output          soROL_Nts_Udp_Data_tvalid,
  input           soROL_Nts_Udp_Data_tready,

  //Open Port vector
  input [ 31:0]  piROL_Nrc_Udp_Rx_ports,
  //-- ROLE <-> NRC Meta Interface
  input   [79:0] siROLE_Nrc_Udp_Meta_TDATA,
  input          siROLE_Nrc_Udp_Meta_TVALID,
  output         siROLE_Nrc_Udp_Meta_TREADY,
  input   [ 9:0] siROLE_Nrc_Udp_Meta_TKEEP,
  input          siROLE_Nrc_Udp_Meta_TLAST,
  output  [79:0] soNRC_Role_Udp_Meta_TDATA,
  output         soNRC_Role_Udp_Meta_TVALID,
  input          soNRC_Role_Udp_Meta_TREADY,
  output  [ 9:0] soNRC_Role_Udp_Meta_TKEEP,
  output         soNRC_Role_Udp_Meta_TLAST,
  
  //------------------------------------------------------
  //-- ROLE / Nts / TCP Interfaces
  //------------------------------------------------------
  //---- Input TCP Data (AXI4S) --------
  input  [ 63:0]  siROL_Nts_Tcp_Data_tdata,
  input  [  7:0]  siROL_Nts_Tcp_Data_tkeep,
  input           siROL_Nts_Tcp_Data_tlast,
  input           siROL_Nts_Tcp_Data_tvalid,
  output          siROL_Nts_Tcp_Data_tready,
  //---- Output TCP Data (AXI4S) -------
  output [ 63:0]  soROL_Nts_Tcp_Data_tdata,
  output [  7:0]  soROL_Nts_Tcp_Data_tkeep,
  output          soROL_Nts_Tcp_Data_tlast,
  output          soROL_Nts_Tcp_Data_tvalid,
  input           soROL_Nts_Tcp_Data_tready,

  //Open Port vector
  input [ 31:0]  piROL_Nrc_Tcp_Rx_ports,
  //-- ROLE <-> NRC Meta Interface
  input   [79:0] siROLE_Nrc_Tcp_Meta_TDATA,
  input          siROLE_Nrc_Tcp_Meta_TVALID,
  output         siROLE_Nrc_Tcp_Meta_TREADY,
  input   [ 9:0] siROLE_Nrc_Tcp_Meta_TKEEP,
  input          siROLE_Nrc_Tcp_Meta_TLAST,
  output  [79:0] soNRC_Role_Tcp_Meta_TDATA,
  output         soNRC_Role_Tcp_Meta_TVALID,
  input          soNRC_Role_Tcp_Meta_TREADY,
  output  [ 9:0] soNRC_Role_Tcp_Meta_TKEEP,
  output         soNRC_Role_Tcp_Meta_TLAST,


 //------------------------------------------------------  
  //-- ROLE / Mem / Mp0 Interface
  //------------------------------------------------------
  //-- Memory Port #0 / S2MM-AXIS ------------------
  //---- Stream Read Command -----------
  input  [ 79:0]  siROL_Mem_Mp0_RdCmd_tdata,
  input           siROL_Mem_Mp0_RdCmd_tvalid,
  output          siROL_Mem_Mp0_RdCmd_tready,
  //---- Stream Read Status ------------
  output [  7:0]  soROL_Mem_Mp0_RdSts_tdata,
  output          soROL_Mem_Mp0_RdSts_tvalid,
  input           soROL_Mem_Mp0_RdSts_tready,
  //---- Stream Data Output Channel ----
  output [511:0]  soROL_Mem_Mp0_Read_tdata,
  output [ 63:0]  soROL_Mem_Mp0_Read_tkeep,
  output          soROL_Mem_Mp0_Read_tlast,
  output          soROL_Mem_Mp0_Read_tvalid,
  input           soROL_Mem_Mp0_Read_tready,
  //---- Stream Write Command ----------
  input  [ 79:0]  siROL_Mem_Mp0_WrCmd_tdata,
  input           siROL_Mem_Mp0_WrCmd_tvalid,
  output          siROL_Mem_Mp0_WrCmd_tready,
  //---- Stream Write Status -----------
  output [  7:0]  soROL_Mem_Mp0_WrSts_tdata,
  output          soROL_Mem_Mp0_WrSts_tvalid,
  input           soROL_Mem_Mp0_WrSts_tready,
  //---- Stream Data Input Channel -----
  input  [511:0]  siROL_Mem_Mp0_Write_tdata,
  input  [ 63:0]  siROL_Mem_Mp0_Write_tkeep,
  input           siROL_Mem_Mp0_Write_tlast,
  input           siROL_Mem_Mp0_Write_tvalid,
  output          siROL_Mem_Mp0_Write_tready, 

  //------------------------------------------------------
  //-- ROLE / Mem / Mp1 Interface
  //------------------------------------------------------
  input  [  3: 0]  miROL_Mem_Mp1_AWID,
  input  [ 32: 0]  miROL_Mem_Mp1_AWADDR,
  input  [  7: 0]  miROL_Mem_Mp1_AWLEN,
  input  [  2: 0]  miROL_Mem_Mp1_AWSIZE,
  input  [  1: 0]  miROL_Mem_Mp1_AWBURST,
  input            miROL_Mem_Mp1_AWVALID,
  output           miROL_Mem_Mp1_AWREADY,
  input  [511: 0]  miROL_Mem_Mp1_WDATA,
  input  [ 63: 0]  miROL_Mem_Mp1_WSTRB,
  input            miROL_Mem_Mp1_WLAST,
  input            miROL_Mem_Mp1_WVALID,
  output           miROL_Mem_Mp1_WREADY,
  output [  3: 0]  miROL_Mem_Mp1_BID,
  output [  1: 0]  miROL_Mem_Mp1_BRESP,
  output           miROL_Mem_Mp1_BVALID,
  input            miROL_Mem_Mp1_BREADY,
  input  [  3: 0]  miROL_Mem_Mp1_ARID,
  input  [ 32: 0]  miROL_Mem_Mp1_ARADDR,
  input  [  7: 0]  miROL_Mem_Mp1_ARLEN,
  input  [  2: 0]  miROL_Mem_Mp1_ARSIZE,
  input  [  1: 0]  miROL_Mem_Mp1_ARBURST,
  input            miROL_Mem_Mp1_ARVALID,
  output           miROL_Mem_Mp1_ARREADY,
  output [  3: 0]  miROL_Mem_Mp1_RID,
  output [511: 0]  miROL_Mem_Mp1_RDATA,
  output [  1: 0]  miROL_Mem_Mp1_RRESP,
  output           miROL_Mem_Mp1_RLAST,
  output           miROL_Mem_Mp1_RVALID,
  input            miROL_Mem_Mp1_RREADY,

  //--------------------------------------------------------
  //-- ROLE / Mmio / AppFlash Interface
  //--------------------------------------------------------
  //---- [PHY_RESET] -------------------
  output          poROL_Mmio_Ly7Rst,
  //---- [PHY_ENABLE] ------------------
  output          poROL_Mmio_Ly7En,
  //---- [DIAG_CTRL_1] -----------------
  output [ 1: 0]  poROL_Mmio_Mc1_MemTestCtrl,
  //---- [DIAG_STAT_1] -----------------
  input  [ 1: 0]  piROL_Mmio_Mc1_MemTestStat,
  //---- [DIAG_CTRL_2] -----------------
  output [  1:0]  poROL_Mmio_UdpEchoCtrl,
  output          poROL_Mmio_UdpPostDgmEn,
  output          poROL_Mmio_UdpCaptDgmEn,
  output [  1:0]  poROL_Mmio_TcpEchoCtrl,
  output          poROL_Mmio_TcpPostSegEn,
  output          poROL_Mmio_TcpCaptSegEn,
  //---- [APP_RDROL] -------------------
  input   [15:0]  piROL_Mmio_RdReg,
  //---- [APP_WRROL] -------------------
  output  [15:0]  poROL_Mmio_WrReg,


  //----------------------------------------------------
  // -- ROLE / Fmc / Management Interface 
  //----------------------------------------------------
  output [ 31:0]  poROL_Fmc_Rank,
  output [ 31:0]  poROL_Fmc_Size,
 
  output          poVoid
  
);  // End of PortList


  // *****************************************************************************
  // **  STRUCTURE
  // *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //--------------------------------------------------------
  //-- Global Clock and Reset used by the entire SHELL
  //--  This clock is generated by the ETH core and runs at 156.25MHz
  //--------------------------------------------------------
  (* keep="true" *)
  wire          sETH0_ShlClk;
  (* keep="true" *)
  wire          sETH0_ShlRst;
  wire          sETH0_CoreResetDone;  
   
  //-- SoftReset & SoftEnable Signals ---------------------
  wire  [ 7:0]  sMMIO_LayerRst;
  wire  [ 7:0]  sMMIO_LayerEn;  

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : ETH[0] <--> NTS[0] 
  //--------------------------------------------------------
  //---- AXI-Write Stream Interface : ETH0 --> NTS0 --------
  wire [ 63:0]  ssETH0_NTS0_Data_tdata;
  wire [  7:0]  ssETH0_NTS0_Data_tkeep;
  wire          ssETH0_NTS0_Data_tvalid;
  wire          ssETH0_NTS0_Data_tlast;
  wire          ssETH0_NTS0_Data_tready;
  //---- AXI-Write Stream Interface : NTS0 --> ETH0 --------
  wire [ 63:0]  ssNTS0_ETH0_Data_tdata;
  wire [  7:0]  ssNTS0_ETH0_Data_tkeep;
  wire          ssNTS0_ETH0_Data_tvalid;
  wire          ssNTS0_ETH0_Data_tlast;
  wire          ssNTS0_ETH0_Data_tready;

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : NTS[0] <--> MEM
  //--------------------------------------------------------
  //----  Transmit Path --------------------------
  //------  Stream Read Command --------
  wire [ 79:0]  ssNTS0_MEM_TxP_RdCmd_tdata;
  wire          ssNTS0_MEM_TxP_RdCmd_tvalid;
  wire          ssNTS0_MEM_TxP_RdCmd_tready;
  //------ Stream Read Status ----------
  wire [  7:0]  ssMEM_NTS0_TxP_RdSts_tdata;
  wire          ssMEM_NTS0_TxP_RdSts_tvalid;
  wire          ssMEM_NTS0_TxP_RdSts_tready;
  //------ Stream Data Output Channel --
  wire [ 63:0]  ssMEM_NTS0_TxP_Read_tdata;
  wire [  7:0]  ssMEM_NTS0_TxP_Read_tkeep;
  wire          ssMEM_NTS0_TxP_Read_tlast;
  wire          ssMEM_NTS0_TxP_Read_tvalid;
  wire          ssMEM_NTS0_TxP_Read_tready;
  //------ Stream Write Command --------
  wire [ 79:0]  ssNTS0_MEM_TxP_WrCmd_tdata;
  wire          ssNTS0_MEM_TxP_WrCmd_tvalid;
  wire          ssNTS0_MEM_TxP_WrCmd_tready;
  //------ Stream Write Status ---------
  wire [  7:0]  ssMEM_NTS0_TxP_WrSts_tdata;
  wire          ssMEM_NTS0_TxP_WrSts_tvalid;
  wire          ssMEM_NTS0_TxP_WrSts_tready;
  //------ Stream Data Input Channel ---
  wire [ 63:0]  ssNTS0_MEM_TxP_Write_tdata;
  wire [  7:0]  ssNTS0_MEM_TxP_Write_tkeep;
  wire          ssNTS0_MEM_TxP_Write_tlast;
  wire          ssNTS0_MEM_TxP_Write_tvalid;
  wire          ssNTS0_MEM_TxP_Write_tready;
  //---- Receive Path ----------------------------
  //------ Stream Read Command ---------
  wire [ 79:0]  ssNTS0_MEM_RxP_RdCmd_tdata;
  wire          ssNTS0_MEM_RxP_RdCmd_tvalid;
  wire          ssNTS0_MEM_RxP_RdCmd_tready;
  //------ Stream Read Status ----------
  wire [  7:0]  ssMEM_NTS0_RxP_RdSts_tdata;
  wire          ssMEM_NTS0_RxP_RdSts_tvalid;
  wire          ssMEM_NTS0_RxP_RdSts_tready;
  //------ Stream Data Output Channel --
  wire [ 63:0]  ssMEM_NTS0_RxP_Read_tdata;
  wire [  7:0]  ssMEM_NTS0_RxP_Read_tkeep;
  wire          ssMEM_NTS0_RxP_Read_tlast;
  wire          ssMEM_NTS0_RxP_Read_tvalid;
  wire          ssMEM_NTS0_RxP_Read_tready;
  //------ Stream Write Command --------
  wire [ 79:0]  ssNTS0_MEM_RxP_WrCmd_tdata;
  wire          ssNTS0_MEM_RxP_WrCmd_tvalid;
  wire          ssNTS0_MEM_RxP_WrCmd_tready;
  //------ Stream Write Status ---------
  wire [  7:0]  ssMEM_NTS0_RxP_WrSts_tdata;
  wire          ssMEM_NTS0_RxP_WrSts_tvalid;
  wire          ssMEM_NTS0_RxP_WrSts_tready;
  //------ Stream Data Input Channel ---
  wire [ 63:0]  ssNTS0_MEM_RxP_Write_tdata;
  wire [  7:0]  ssNTS0_MEM_RxP_Write_tkeep;
  wire          ssNTS0_MEM_RxP_Write_tlast;
  wire          ssNTS0_MEM_RxP_Write_tvalid;
  wire          ssNTS0_MEM_RxP_Write_tready;

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : ROLE <--> NTS0
  //--------------------------------------------------------
  //---- Udp Interface -------------------------------------  
  //------ UDP Data (AXI4S) ------------
  //wire [ 63:0]  sROL_Nts0_Udp_Axis_tdata;
  //wire [  7:0]  sROL_Nts0_Udp_Axis_tkeep;
  //wire          sROL_Nts0_Udp_Axis_tlast;
  //wire          sROL_Nts0_Udp_Axis_tvalid;
  //wire          sNTS0_Rol_Udp_Axis_tready;
  ////------ UDP Data (AXI4S) ----------
  //wire          sROL_Nts0_Udp_Axis_tready;
  //wire [ 63:0]  sNTS0_Rol_Udp_Axis_tdata;
  //wire [  7:0]  sNTS0_Rol_Udp_Axis_tkeep;
  //wire          sNTS0_Rol_Udp_Axis_tlast;
  //wire          sNTS0_Rol_Udp_Axis_tvalid;  

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : MMIO <--> ETH|NTS|MEM|ROL|FMC 
  //--------------------------------------------------------
  //---- Configuration Registers Interface -------  
  //---- Physiscal Registers Interface -----------
  //------ [PHY_STATUS] ----------------
  wire          sMEM_MMIO_Mc0InitCalComplete;
  wire          sMEM_MMIO_Mc1InitCalComplete;
  wire          sETH0_MMIO_CoreReady;
  wire          sETH0_MMIO_QpllLock;
  wire          sNTS0_MMIO_CamReady;
  wire          sNTS0_MMIO_NtsReady;
  //------ [PHY_ETH0] ------------------
  wire          sMMIO_ETH0_RxEqualizerMode;
  wire  [ 3:0]  sMMIO_ETH0_TxDriverSwing;
  wire  [ 4:0]  sMMIO_ETH0_TxPreCursor;
  wire  [ 4:0]  sMMIO_ETH0_TxPostCursor;
  //---- Layer-2 Registers Interface -------------
  //------ [LY2_MAC] -------------------
  wire  [47:0]  sMMIO_NTS0_MacAddress;
  //---- Layer-3 Registers Interface -------------
  //------ [LY3_IPv4] ------------------
  wire  [31:0]  sMMIO_NTS0_Ip4Address;
  //------ [LY3_SUBNET] ----------------
  wire  [31:0]  sMMIO_NTS0_SubNetMask;
  //------ [LY3_GATEWAY] ---------------
  wire  [31:0]  sMMIO_NTS0_GatewayAddr;
  // ------- [MNGT REGISTERS] ------------
  wire  [31:0]  sMMIO_NRC_CfrmIp4Addr;
  wire  [15:0]  sMMIO_NRC_FmcLsnPort;
  //---- Role Registers --------------------------
  //------ [APP_RDROLE] ----------------
  wire  [15:0]  sROL_MMIO_RdRoleReg;
  //------ [APPE_WRROLE] ---------------
  wire  [15:0]  sMMIO_ROL_WrRoleReg;
  //---- Role Registers --------------------------
  //---- APP_RDFMC ---------------------
  wire  [31:0]  sFMC_MMIO_RdFmcReg;
  //---- APP_WRFMC -------------------------------
  wire  [31:0]  sMMIO_FMC_WrFmcReg;  
  //---- Diagnostic Registers Interface ----------
  //------ [DIAG_CTRL_1] ---------------
  wire          sMMIO_ETH0_PcsLoopbackEn;
  wire          sMMIO_ETH0_MacLoopbackEn;
  wire          sMMIO_ETH0_MacAddrSwapEn; 
  //---- DIAG_CTRL_1 ---------------
  wire  [ 1:0]  sMMIO_ROL_Mc1_MemTestCtrl;
  //---- DIAG_STAT_1 ---------------
  wire  [ 1:0]  sROL_MMIO_Mc1_MemTestStat;
  //---- Diagnostic Registers Interface ----------
  //------ [DIAG_CTRL_2] ---------------
  //OBSOLETE-20190718 wire  [ 1:0]  sMMIO_ROL_UdpEchoCtrl;
  //OBSOLETE-20190718 wire          sMMIO_ROL_UdpPostDgmEn;
  //OBSOLETE-20190718 wire          sMMIO_ROL_UdpCaptDgmEn;
  //OBSOLETE-20190718 wire  [ 1:0]  sMMIO_ROL_TcpEchoCtrl;
  //OBSOLETE-20190718 wire          sMMIO_ROL_TcpPostSegEn;
  //OBSOLETE-20190718 wire          sMMIO_ROL_TcpCaptSegEn; 
  
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : HWICAPC 
  //--------------------------------------------------------
  wire [ 8:0] ssFMC_HWICAP_Axi_awaddr;
  wire        ssFMC_HWICAP_Axi_awvalid;
  wire        ssFMC_HWICAP_Axi_awready;
  wire [31:0] ssFMC_HWICAP_Axi_wdata;
  wire [ 3:0] ssFMC_HWICAP_Axi_wstrb;
  wire        ssFMC_HWICAP_Axi_wvalid;
  wire        ssFMC_HWICAP_Axi_wready;
  wire [ 1:0] ssFMC_HWICAP_Axi_bresp;
  wire        ssFMC_HWICAP_Axi_bvalid;
  wire        ssFMC_HWICAP_Axi_bready;
  wire [ 8:0] ssFMC_HWICAP_Axi_araddr;
  wire        ssFMC_HWICAP_Axi_arvalid; 
  wire        ssFMC_HWICAP_Axi_arready;
  wire [31:0] ssFMC_HWICAP_Axi_rdata;
  wire [ 1:0] ssFMC_HWICAP_Axi_rresp;
  wire        ssFMC_HWICAP_Axi_rvalid;
  wire        ssFMC_HWICAP_Axi_rready;
  wire        ssFMC_HWICAP_ip2intc_irpt;

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : FPGA Management Core
  //--------------------------------------------------------
  //wire [31:0] sFMC_MMIO_4B_Reg;
  //wire [31:0] sMMIO_FMC_4B_Reg;
  wire        sDCP_FMC_status;
  wire        sFMC_DCP_activate;
  wire        sFMC_ROLE_soft_reset;
  wire [8:0]  sbFMC_MMIO_Xmem_Addr;
  wire [31:0] sbFMC_MMIO_Xmem_RData;
  wire        sbFMC_MMIO_Xmem_cen; //Chip-enable
  wire        sbFMC_MMIO_Xmem_wren; //Write-enable
  wire [31:0] sbFMC_MMIO_Xmem_WData;
  // FMC <==> NRC ctrlLink
  wire        ssFMC_NRC_ctrlLink_Axi_AWVALID;
  wire        ssFMC_NRC_ctrlLink_Axi_AWREADY;
  wire [13:0] ssFMC_NRC_ctrlLink_Axi_AWADDR;
  wire        ssFMC_NRC_ctrlLink_Axi_WVALID;
  wire        ssFMC_NRC_ctrlLink_Axi_WREADY;
  wire [31:0] ssFMC_NRC_ctrlLink_Axi_WDATA;
  wire [ 3:0] ssFMC_NRC_ctrlLink_Axi_WSTRB;
  wire        ssFMC_NRC_ctrlLink_Axi_ARVALID;
  wire        ssFMC_NRC_ctrlLink_Axi_ARREADY;
  wire [13:0] ssFMC_NRC_ctrlLink_Axi_ARADDR;
  wire        ssFMC_NRC_ctrlLink_Axi_RVALID;
  wire        ssFMC_NRC_ctrlLink_Axi_RREADY;
  wire [31:0] ssFMC_NRC_ctrlLink_Axi_RDATA;
  wire [ 1:0] ssFMC_NRC_ctrlLink_Axi_RRESP;
  wire        ssFMC_NRC_ctrlLink_Axi_BVALID;
  wire        ssFMC_NRC_ctrlLink_Axi_BREADY;
  wire [ 1:0] ssFMC_NRC_ctrlLink_Axi_BRESP;

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : NRC
  //--------------------------------------------------------
  
  //-- NTS / Udp / Tx Data Interfaces (.i.e NTS<-->NRC)
  wire  [ 63:0] ssNTS0_NRC_Udp_Data_tdata ;
  wire  [  7:0] ssNTS0_NRC_Udp_Data_tkeep ;
  wire          ssNTS0_NRC_Udp_Data_tlast ;
  wire          ssNTS0_NRC_Udp_Data_tvalid;
  wire          ssNTS0_NRC_Udp_Data_tready;
  wire  [ 95:0] ssNTS0_NRC_Udp_Meta_tdata ;
  wire          ssNTS0_NRC_Udp_Meta_tvalid;
  wire          ssNTS0_NRC_Udp_Meta_tready;
  wire  [ 15:0] ssNTS0_NRC_Udp_DLen_tdata ;
  wire          ssNTS0_NRC_Udp_DLen_tvalid;
  wire          ssNTS0_NRC_Udp_DLen_tready;
  
  //-- NTS / Udp / Rx Data Interfaces (.i.e NTS<-->NRC)
  wire  [ 63:0] ssNRC_NTS0_Udp_Data_tdata ;
  wire  [  7:0] ssNRC_NTS0_Udp_Data_tkeep ;
  wire          ssNRC_NTS0_Udp_Data_tlast ;
  wire          ssNRC_NTS0_Udp_Data_tvalid;
  wire          ssNRC_NTS0_Udp_Data_tready;
  wire  [ 95:0] ssNRC_NTS0_Udp_Meta_tdata ; 
  wire          ssNRC_NTS0_Udp_Meta_tvalid;
  wire          ssNRC_NTS0_Udp_Meta_tready;
  wire  [ 15:0] ssNRC_NTS0_Udp_DLen_tdata ; 
  wire          ssNRC_NTS0_Udp_DLen_tvalid;
  wire          ssNRC_NTS0_Udp_DLen_tready;
  
  //-- NTS / Udp / Rx Ctrl Interfaces (.i.e NTS<-->NRC)
  wire  [ 15:0] ssNRC_NTS0_Udp_LsnReq_tdata ;
  wire          ssNRC_NTS0_Udp_LsnReq_tvalid;
  wire          ssNRC_NTS0_Udp_LsnReq_tready;
  wire  [  7:0] ssNTS0_NRC_Udp_LsnRep_tdata ;
  wire          ssNTS0_NRC_Udp_LsnRep_tvalid;
  wire          ssNTS0_NRC_Udp_LsnRep_tready;
  wire  [ 15:0] ssNRC_NTS0_Udp_ClsReq_tdata ;
  wire          ssNRC_NTS0_Udp_ClsReq_tvalid;
  wire          ssNRC_NTS0_Udp_ClsReq_tready;
  wire  [  7:0] ssNTS0_NRC_Udp_ClsRep_tdata ;
  wire          ssNTS0_NRC_Udp_ClsRep_tvalid;
  wire          ssNTS0_NRC_Udp_ClsRep_tready;
  
  //-- FPGA Transmit Path (ROLE-->SHELL) ---------
  //---- Stream TCP Data ---------------------
  wire [ 63:0]  ssNRC_TOE_Tcp_Data_tdata;
  wire [  7:0]  ssNRC_TOE_Tcp_Data_tkeep;
  wire          ssNRC_TOE_Tcp_Data_tvalid;
  wire          ssNRC_TOE_Tcp_Data_tlast;
  wire          ssNRC_TOE_Tcp_Data_tready;
  //---- Stream TCP Metadata -----------------
  wire [ 15:0]  ssNRC_TOE_Tcp_Meta_tdata;
  wire          ssNRC_TOE_Tcp_Meta_tvalid;
  wire          ssNRC_TOE_Tcp_Meta_tready;
  //-- Stream TCP Data Request ---------------
  wire  [ 31:0] ssNRC_TOE_Tcp_DReq_tdata;
  wire          ssNRC_TOE_Tcp_DReq_tvalid;
  wire          ssNRC_TOE_Tcp_DReq_tready;
  //---- Stream TCP Open Session Request -----
  wire [ 47:0]  ssNRC_TOE_Tcp_OpnReq_tdata;
  wire          ssNRC_TOE_Tcp_OpnReq_tvalid;
  wire          ssNRC_TOE_Tcp_OpnReq_tready;
  //---- Stream TCP Close Request ------------
  wire [ 15:0]  ssNRC_TOE_Tcp_ClsReq_tdata;
  wire          ssNRC_TOE_Tcp_ClsReq_tvalid;
  wire          ssNRC_TOE_Tcp_ClsReq_tready;
  //---- Stream TCP Listen Request -----------
  wire [ 15:0]  ssNRC_TOE_Tcp_LsnReq_tdata;
  wire          ssNRC_TOE_Tcp_LsnReq_tvalid;
  wire          ssNRC_TOE_Tcp_LsnReq_tready;
  //---- Stream TCP Data Status --------------
  wire  [ 23:0] ssTOE_NRC_Tcp_DSts_tdata;
  wire          ssTOE_NRC_Tcp_DSts_tvalid;
  wire          ssTOE_NRC_Tcp_DSts_tready;
  //-- Stream TCP Data -----------------------
  wire  [ 63:0] ssTOE_NRC_Tcp_Data_tdata;
  wire  [  7:0] ssTOE_NRC_Tcp_Data_tkeep;
  wire          ssTOE_NRC_Tcp_Data_tvalid;
  wire          ssTOE_NRC_Tcp_Data_tlast;
  wire          ssTOE_NRC_Tcp_Data_tready;
  //-- Stream TCP Metadata -------------------
  wire  [ 15:0] ssTOE_NRC_Tcp_Meta_tdata;
  wire          ssTOE_NRC_Tcp_Meta_tvalid;
  wire          ssTOE_NRC_Tcp_Meta_tready;
  //-- Stream TCP Data Notification ----------
  wire  [103:0] ssTOE_NRC_Tcp_Notif_tdata; // 7+96
  wire          ssTOE_NRC_Tcp_Notif_tvalid;
  wire          ssTOE_NRC_Tcp_Notif_tready;
  //---- Stream TCP Open Session Status ------
  wire  [ 23:0] ssTOE_NRC_Tcp_OpnRep_tdata;
  wire          ssTOE_NRC_Tcp_OpnRep_tvalid;
  wire          ssTOE_NRC_Tcp_OpnRep_tready;
  //---- Stream TCP Listen Status ------------
  wire  [  7:0] ssTOE_NRC_Tcp_LsnAck_tdata;
  wire          ssTOE_NRC_Tcp_LsnAck_tvalid;
  wire          ssTOE_NRC_Tcp_LsnAck_tready;
  
  // ===== Timer Broadcast =====
  wire [31:0]  sTIME_Broadcast_seconds;
  wire [31:0]  sTIME_Broadcast_minutes;
  wire [31:0]  sTIME_Broadcast_hours;
  
  
  // ===== NRC <-> FMC TCP connection =====
  wire [ 63:0]  ssNRC_Fifo_Tcp_Data_tdata_V_din;
  wire          ssNRC_Fifo_Tcp_Data_tdata_V_full;
  wire          ssNRC_Fifo_Tcp_Data_tdata_V_write;
  wire [  7:0]  ssNRC_Fifo_Tcp_Data_tkeep_V_din;
  wire          ssNRC_Fifo_Tcp_Data_tkeep_V_full;
  wire          ssNRC_Fifo_Tcp_Data_tkeep_V_write;
  wire [  0:0]  ssNRC_Fifo_Tcp_Data_tlast_V_din;
  wire          ssNRC_Fifo_Tcp_Data_tlast_V_full;
  wire          ssNRC_Fifo_Tcp_Data_tlast_V_write;
  wire [ 15:0]  ssNRC_Fifo_Tcp_SessId_tdata_V_din;
  wire          ssNRC_Fifo_Tcp_SessId_tdata_V_full;
  wire          ssNRC_Fifo_Tcp_SessId_tdata_V_write;
  //wire [  1:0]  ssNRC_Fifo_Tcp_SessId_tkeep_V_din;
  //wire          ssNRC_Fifo_Tcp_SessId_tkeep_V_full;
  //wire          ssNRC_Fifo_Tcp_SessId_tkeep_V_write;
  //wire [  0:0]  ssNRC_Fifo_Tcp_SessId_tlast_V_din;
  //wire          ssNRC_Fifo_Tcp_SessId_tlast_V_full;
  //wire          ssNRC_Fifo_Tcp_SessId_tlast_V_write;
  
  wire [ 63:0]  ssFifo_FMC_Tcp_Data_tdata_V_dout;
  wire          ssFifo_FMC_Tcp_Data_tdata_V_empty;
  wire          ssFifo_FMC_Tcp_Data_tdata_V_read;
  wire [  7:0]  ssFifo_FMC_Tcp_Data_tkeep_V_dout;
  wire          ssFifo_FMC_Tcp_Data_tkeep_V_empty;
  wire          ssFifo_FMC_Tcp_Data_tkeep_V_read;
  wire [  0:0]  ssFifo_FMC_Tcp_Data_tlast_V_dout;
  wire          ssFifo_FMC_Tcp_Data_tlast_V_empty;
  wire          ssFifo_FMC_Tcp_Data_tlast_V_read;
  wire [ 15:0]  ssFifo_FMC_Tcp_SessId_tdata_V_dout;
  wire          ssFifo_FMC_Tcp_SessId_tdata_V_empty;
  wire          ssFifo_FMC_Tcp_SessId_tdata_V_read;
  //wire [  1:0]  ssFifo_FMC_Tcp_SessId_tkeep_V_dout;
  //wire          ssFifo_FMC_Tcp_SessId_tkeep_V_empty;
  //wire          ssFifo_FMC_Tcp_SessId_tkeep_V_read;
  //wire [  0:0]  ssFifo_FMC_Tcp_SessId_tlast_V_dout;
  //wire          ssFifo_FMC_Tcp_SessId_tlast_V_empty;
  //wire          ssFifo_FMC_Tcp_SessId_tlast_V_read;
  
  wire [ 63:0]  ssFMC_Fifo_Tcp_Data_tdata_V_din;
  wire          ssFMC_Fifo_Tcp_Data_tdata_V_full;
  wire          ssFMC_Fifo_Tcp_Data_tdata_V_write;
  wire [  7:0]  ssFMC_Fifo_Tcp_Data_tkeep_V_din;
  wire          ssFMC_Fifo_Tcp_Data_tkeep_V_full;
  wire          ssFMC_Fifo_Tcp_Data_tkeep_V_write;
  wire [  0:0]  ssFMC_Fifo_Tcp_Data_tlast_V_din;
  wire          ssFMC_Fifo_Tcp_Data_tlast_V_full;
  wire          ssFMC_Fifo_Tcp_Data_tlast_V_write;
  wire [ 15:0]  ssFMC_Fifo_Tcp_SessId_tdata_V_din;
  wire          ssFMC_Fifo_Tcp_SessId_tdata_V_full;
  wire          ssFMC_Fifo_Tcp_SessId_tdata_V_write;
  //wire [  1:0]  ssFMC_Fifo_Tcp_SessId_tkeep_V_din;
  //wire          ssFMC_Fifo_Tcp_SessId_tkeep_V_full;
  //wire          ssFMC_Fifo_Tcp_SessId_tkeep_V_write;
  //wire [  0:0]  ssFMC_Fifo_Tcp_SessId_tlast_V_din;
  //wire          ssFMC_Fifo_Tcp_SessId_tlast_V_full;
  //wire          ssFMC_Fifo_Tcp_SessId_tlast_V_write;
  
  wire [ 63:0]  ssFifo_NRC_Tcp_Data_tdata_V_dout;
  wire          ssFifo_NRC_Tcp_Data_tdata_V_empty;
  wire          ssFifo_NRC_Tcp_Data_tdata_V_read;
  wire [  7:0]  ssFifo_NRC_Tcp_Data_tkeep_V_dout;
  wire          ssFifo_NRC_Tcp_Data_tkeep_V_empty;
  wire          ssFifo_NRC_Tcp_Data_tkeep_V_read;
  wire [  0:0]  ssFifo_NRC_Tcp_Data_tlast_V_dout;
  wire          ssFifo_NRC_Tcp_Data_tlast_V_empty;
  wire          ssFifo_NRC_Tcp_Data_tlast_V_read;
  wire [ 15:0]  ssFifo_NRC_Tcp_SessId_tdata_V_dout;
  wire          ssFifo_NRC_Tcp_SessId_tdata_V_empty;
  wire          ssFifo_NRC_Tcp_SessId_tdata_V_read;
  //wire [  1:0]  ssFifo_NRC_Tcp_SessId_tkeep_V_dout;
  //wire          ssFifo_NRC_Tcp_SessId_tkeep_V_empty;
  //wire          ssFifo_NRC_Tcp_SessId_tkeep_V_read;
  //wire [  0:0]  ssFifo_NRC_Tcp_SessId_tlast_V_dout;
  //wire          ssFifo_NRC_Tcp_SessId_tlast_V_empty;
  //wire          ssFifo_NRC_Tcp_SessId_tlast_V_read;


  //Conncetions from NRC to AXIS Slices
  wire [63:0] slcInUdp_data_TDATA  ;
  wire        slcInUdp_data_TVALID ;
  wire        slcInUdp_data_TREADY ;
  wire [ 7:0] slcInUdp_data_TKEEP  ;
  wire        slcInUdp_data_TLAST  ;
  wire [63:0] slcOutUdp_data_TDATA  ;
  wire        slcOutUdp_data_TVALID ;
  wire        slcOutUdp_data_TREADY ;
  wire [ 7:0] slcOutUdp_data_TKEEP  ;
  wire        slcOutUdp_data_TLAST  ;
  wire [79:0] slcInNrc_Udp_meta_TDATA  ;
  wire        slcInNrc_Udp_meta_TVALID ;
  wire        slcInNrc_Udp_meta_TREADY ;
  wire [ 9:0] slcInNrc_Udp_meta_TKEEP  ;
  wire        slcInNrc_Udp_meta_TLAST  ;
  wire [79:0] slcOutNrc_Udp_meta_TDATA  ;
  wire        slcOutNrc_Udp_meta_TVALID ;
  wire        slcOutNrc_Udp_meta_TREADY ;
  wire [ 9:0] slcOutNrc_Udp_meta_TKEEP  ;
  wire        slcOutNrc_Udp_meta_TLAST  ;
  //TCP
  wire [63:0] slcInTcp_data_TDATA  ;
  wire        slcInTcp_data_TVALID ;
  wire        slcInTcp_data_TREADY ;
  wire [ 7:0] slcInTcp_data_TKEEP  ;
  wire        slcInTcp_data_TLAST  ;
  wire [63:0] slcOutTcp_data_TDATA  ;
  wire        slcOutTcp_data_TVALID ;
  wire        slcOutTcp_data_TREADY ;
  wire [ 7:0] slcOutTcp_data_TKEEP  ;
  wire        slcOutTcp_data_TLAST  ;
  wire [79:0] slcInNrc_Tcp_meta_TDATA  ;
  wire        slcInNrc_Tcp_meta_TVALID ;
  wire        slcInNrc_Tcp_meta_TREADY ;
  wire [ 9:0] slcInNrc_Tcp_meta_TKEEP  ;
  wire        slcInNrc_Tcp_meta_TLAST  ;
  wire [79:0] slcOutNrc_Tcp_meta_TDATA  ;
  wire        slcOutNrc_Tcp_meta_TVALID ;
  wire        slcOutNrc_Tcp_meta_TREADY ;
  wire [ 9:0] slcOutNrc_Tcp_meta_TKEEP  ;
  wire        slcOutNrc_Tcp_meta_TLAST  ;

  //------------------------------------------------------
  //-- Decoupling DCP to X
  //------------------------------------------------------
  wire   [ 63:0]   siDCP_ROL_Nts_Udp_Data_tdata;
  wire   [  7:0]   siDCP_ROL_Nts_Udp_Data_tkeep;
  wire             siDCP_ROL_Nts_Udp_Data_tlast;
  wire             siDCP_ROL_Nts_Udp_Data_tvalid;
  wire             siDCP_ROL_Nts_Udp_Data_tready;
  wire   [ 63:0]   soDCP_ROL_Nts_Udp_Data_tdata;
  wire   [  7:0]   soDCP_ROL_Nts_Udp_Data_tkeep;
  wire             soDCP_ROL_Nts_Udp_Data_tlast;
  wire             soDCP_ROL_Nts_Udp_Data_tvalid;
  wire             soDCP_ROL_Nts_Udp_Data_tready;
  wire   [ 31:0]   piDCP_ROL_Nrc_Udp_Rx_ports;
  wire    [79:0]   siDCP_ROLE_Nrc_Udp_Meta_TDATA;
  wire             siDCP_ROLE_Nrc_Udp_Meta_TVALID;
  wire             siDCP_ROLE_Nrc_Udp_Meta_TREADY;
  wire    [ 9:0]   siDCP_ROLE_Nrc_Udp_Meta_TKEEP;
  wire             siDCP_ROLE_Nrc_Udp_Meta_TLAST;
  wire    [79:0]   soDCP_NRC_Role_Udp_Meta_TDATA;
  wire             soDCP_NRC_Role_Udp_Meta_TVALID;
  wire             soDCP_NRC_Role_Udp_Meta_TREADY;
  wire    [ 9:0]   soDCP_NRC_Role_Udp_Meta_TKEEP;
  wire             soDCP_NRC_Role_Udp_Meta_TLAST;
  wire   [ 63:0]   siDCP_ROL_Nts_Tcp_Data_tdata;
  wire   [  7:0]   siDCP_ROL_Nts_Tcp_Data_tkeep;
  wire             siDCP_ROL_Nts_Tcp_Data_tlast;
  wire             siDCP_ROL_Nts_Tcp_Data_tvalid;
  wire             siDCP_ROL_Nts_Tcp_Data_tready;
  wire   [ 63:0]   soDCP_ROL_Nts_Tcp_Data_tdata;
  wire   [  7:0]   soDCP_ROL_Nts_Tcp_Data_tkeep;
  wire             soDCP_ROL_Nts_Tcp_Data_tlast;
  wire             soDCP_ROL_Nts_Tcp_Data_tvalid;
  wire             soDCP_ROL_Nts_Tcp_Data_tready;
  wire   [ 31:0]   piDCP_ROL_Nrc_Tcp_Rx_ports;
  wire    [79:0]   siDCP_ROLE_Nrc_Tcp_Meta_TDATA;
  wire             siDCP_ROLE_Nrc_Tcp_Meta_TVALID;
  wire             siDCP_ROLE_Nrc_Tcp_Meta_TREADY;
  wire    [ 9:0]   siDCP_ROLE_Nrc_Tcp_Meta_TKEEP;
  wire             siDCP_ROLE_Nrc_Tcp_Meta_TLAST;
  wire    [79:0]   soDCP_NRC_Role_Tcp_Meta_TDATA;
  wire             soDCP_NRC_Role_Tcp_Meta_TVALID;
  wire             soDCP_NRC_Role_Tcp_Meta_TREADY;
  wire    [ 9:0]   soDCP_NRC_Role_Tcp_Meta_TKEEP;
  wire             soDCP_NRC_Role_Tcp_Meta_TLAST;
  wire   [ 79:0]   siDCP_ROL_Mem_Mp0_RdCmd_tdata;
  wire             siDCP_ROL_Mem_Mp0_RdCmd_tvalid;
  wire             siDCP_ROL_Mem_Mp0_RdCmd_tready;
  wire   [  7:0]   soDCP_ROL_Mem_Mp0_RdSts_tdata;
  wire             soDCP_ROL_Mem_Mp0_RdSts_tvalid;
  wire             soDCP_ROL_Mem_Mp0_RdSts_tready;
  wire   [511:0]   soDCP_ROL_Mem_Mp0_Read_tdata;
  wire   [ 63:0]   soDCP_ROL_Mem_Mp0_Read_tkeep;
  wire             soDCP_ROL_Mem_Mp0_Read_tlast;
  wire             soDCP_ROL_Mem_Mp0_Read_tvalid;
  wire             soDCP_ROL_Mem_Mp0_Read_tready;
  wire   [ 79:0]   siDCP_ROL_Mem_Mp0_WrCmd_tdata;
  wire             siDCP_ROL_Mem_Mp0_WrCmd_tvalid;
  wire             siDCP_ROL_Mem_Mp0_WrCmd_tready;
  wire   [  7:0]   soDCP_ROL_Mem_Mp0_WrSts_tdata;
  wire             soDCP_ROL_Mem_Mp0_WrSts_tvalid;
  wire             soDCP_ROL_Mem_Mp0_WrSts_tready;
  wire   [511:0]   siDCP_ROL_Mem_Mp0_Write_tdata;
  wire   [ 63:0]   siDCP_ROL_Mem_Mp0_Write_tkeep;
  wire             siDCP_ROL_Mem_Mp0_Write_tlast;
  wire             siDCP_ROL_Mem_Mp0_Write_tvalid;
  wire             siDCP_ROL_Mem_Mp0_Write_tready; 
  wire   [  3: 0]  miDCP_ROL_Mem_Mp1_AWID;
  wire   [ 32: 0]  miDCP_ROL_Mem_Mp1_AWADDR;
  wire   [  7: 0]  miDCP_ROL_Mem_Mp1_AWLEN;
  wire   [  2: 0]  miDCP_ROL_Mem_Mp1_AWSIZE;
  wire   [  1: 0]  miDCP_ROL_Mem_Mp1_AWBURST;
  wire             miDCP_ROL_Mem_Mp1_AWVALID;
  wire             miDCP_ROL_Mem_Mp1_AWREADY;
  wire   [511: 0]  miDCP_ROL_Mem_Mp1_WDATA;
  wire   [ 63: 0]  miDCP_ROL_Mem_Mp1_WSTRB;
  wire             miDCP_ROL_Mem_Mp1_WLAST;
  wire             miDCP_ROL_Mem_Mp1_WVALID;
  wire             miDCP_ROL_Mem_Mp1_WREADY;
  wire   [  3: 0]  miDCP_ROL_Mem_Mp1_BID;
  wire   [  1: 0]  miDCP_ROL_Mem_Mp1_BRESP;
  wire             miDCP_ROL_Mem_Mp1_BVALID;
  wire             miDCP_ROL_Mem_Mp1_BREADY;
  wire   [  3: 0]  miDCP_ROL_Mem_Mp1_ARID;
  wire   [ 32: 0]  miDCP_ROL_Mem_Mp1_ARADDR;
  wire   [  7: 0]  miDCP_ROL_Mem_Mp1_ARLEN;
  wire   [  2: 0]  miDCP_ROL_Mem_Mp1_ARSIZE;
  wire   [  1: 0]  miDCP_ROL_Mem_Mp1_ARBURST;
  wire             miDCP_ROL_Mem_Mp1_ARVALID;
  wire             miDCP_ROL_Mem_Mp1_ARREADY;
  wire   [  3: 0]  miDCP_ROL_Mem_Mp1_RID;
  wire   [511: 0]  miDCP_ROL_Mem_Mp1_RDATA;
  wire   [  1: 0]  miDCP_ROL_Mem_Mp1_RRESP;
  wire             miDCP_ROL_Mem_Mp1_RLAST;
  wire             miDCP_ROL_Mem_Mp1_RVALID;
  wire             miDCP_ROL_Mem_Mp1_RREADY;
  wire   [  1: 0]  piDCP_ROL_Mmio_Mc1_MemTestStat;
  wire   [ 15: 0]  piDCP_ROL_Mmio_RdReg;
  wire   [ 31: 0]  poDCP_ROL_Fmc_Rank;
  wire   [ 31: 0]  poDCP_ROL_Fmc_Size;
 

  //-- END OF SIGNAL DECLARATIONS ----------------------------------------------

  //============================================================================
  //  INST: MMIIO CLIENT
  //============================================================================
  MmioClient_A8_D8 #(
    .gSecurityPriviledges (gSecurityPriviledges),
    .gBitstreamUsage      (gBitstreamUsage)
  ) MMIO (
    //----------------------------------------------
    //-- Global Clock & Reset Inputs
    //----------------------------------------------
    .piSHL_Clk                      (sETH0_ShlClk),
    .piTOP_Rst                      (piTOP_156_25Rst),

    //----------------------------------------------
    //-- Bitstream Identification
    //----------------------------------------------
    .piTOP_Timestamp                (piTOP_Timestamp),

    //----------------------------------------------
    //-- PSOC : Mmio Bus Interface
    //----------------------------------------------
    .piPSOC_Emif_Clk                (piPSOC_Emif_Clk),
    .piPSOC_Emif_Cs_n               (piPSOC_Emif_Cs_n),
    .piPSOC_Emif_We_n               (piPSOC_Emif_We_n),
    .piPSOC_Emif_AdS_n              (piPSOC_Emif_AdS_n),
    .piPSOC_Emif_Oe_n               (piPSOC_Emif_Oe_n),
    .piPSOC_Emif_Addr               (piPSOC_Emif_Addr),
    .pioPSOC_Emif_Data              (pioPSOC_Emif_Data),

    //----------------------------------------------
    //-- MEM : Status inputs and Control outputs
    //----------------------------------------------
    .piMEM_Mc0InitCalComplete       (sMEM_MMIO_Mc0InitCalComplete),
    .piMEM_Mc1InitCalComplete       (sMEM_MMIO_Mc1InitCalComplete),

    //----------------------------------------------
    //-- ETH[0]: Status inputs and Control outputs
    //----------------------------------------------
    .piETH0_CoreReady               (sETH0_MMIO_CoreReady),
    .piETH0_QpllLock                (sETH0_MMIO_QpllLock),
    .poETH0_RxEqualizerMode         (sMMIO_ETH0_RxEqualizerMode),
    .poETH0_TxDriverSwing           (sMMIO_ETH0_TxDriverSwing),
    .poETH0_TxPreCursor             (sMMIO_ETH0_TxPreCursor),
    .poETH0_TxPostCursor            (sMMIO_ETH0_TxPostCursor),
    .poETH0_PcsLoopbackEn           (sMMIO_ETH0_PcsLoopbackEn),
    .poETH0_MacLoopbackEn           (sMMIO_ETH0_MacLoopbackEn),
    .poETH0_MacAddrSwapEn           (sMMIO_ETH0_MacAddrSwapEn),

    //----------------------------------------------
    //-- NTS[0]: Status inputs and Control outputs
    //----------------------------------------------
    .piNTS0_CamReady                (sNTS0_MMIO_CamReady),
    .piNTS0_NtsReady                (sNTS0_MMIO_NtsReady),
    .poNTS0_MacAddress              (sMMIO_NTS0_MacAddress),
    .poNTS0_Ip4Address              (sMMIO_NTS0_Ip4Address),
    .poNTS0_SubNetMask              (sMMIO_NTS0_SubNetMask),
    .poNTS0_GatewayAddr             (sMMIO_NTS0_GatewayAddr),

    //----------------------------------------------
    //-- ROLE : Status input and Control Outputs
    //----------------------------------------------
    //---- [PHY_RESET] -------------
    .poSHL_ResetLayer               (sMMIO_LayerRst),
    //---- [PHY_ENABLE] ------------
    .poSHL_EnableLayer              (sMMIO_LayerEn),
    //---- DIAG_CTRL_1 -------------
    .poROLE_Mc1_MemTestCtrl         (poROL_Mmio_Mc1_MemTestCtrl),  //TODO: remove from Themisto SRA?
    //---- DIAG_STAT_1 -------------
    .piROLE_Mc1_MemTestStat         (piDCP_ROL_Mmio_Mc1_MemTestStat),
    //---- DIAG_CTRL_2 -------------  
    .poROLE_UdpEchoCtrl             (poROL_Mmio_UdpEchoCtrl),  //TODO: remove from Themisto SRA?
    .poROLE_UdpPostDgmEn            (poROL_Mmio_UdpPostDgmEn),
    .poROLE_UdpCaptDgmEn            (poROL_Mmio_UdpCaptDgmEn),
    .poROLE_TcpEchoCtrl             (poROL_Mmio_TcpEchoCtrl),
    .poROLE_TcpPostSegEn            (poROL_Mmio_TcpPostSegEn),
    .poROLE_TcpCaptSegEn            (poROL_Mmio_TcpCaptSegEn),
     //---- APP_RDROL ----------------
    .piROLE_RdReg                   (piDCP_ROL_Mmio_RdReg),
     //---- APP_WRROL -----------------
    .poROLE_WrReg                   (poROL_Mmio_WrReg),
    
    //----------------------------------------------
    //-- NRC :  Control Registers
    //----------------------------------------------
    //---- MNGT_RMIP -------------------
    .poNRC_RmIpAddress              (sMMIO_NRC_CfrmIp4Addr),
    //---- MNGT_TCPLSN -----------------
    .poNRC_TcpLsnPort               (sMMIO_NRC_FmcLsnPort),

    //----------------------------------------------
    //-- FMC : Registers and Extended Memory
    //----------------------------------------------
    //---- APP_RDFMC ----------------
    .piFMC_RdReg                    (sFMC_MMIO_RdFmcReg),
    //---- APP_WRFMC ----------------
    .poFMC_WrReg                    (sMMIO_FMC_WrFmcReg),
 
    //----------------------------------------------
    //-- EMIF Extended Memory Port B
    //----------------------------------------------
    .piXXX_XMem_en                 (sbFMC_MMIO_Xmem_cen),
    .piXXX_XMem_Wren               (sbFMC_MMIO_Xmem_wren),
    .piXXX_XMem_WrData             (sbFMC_MMIO_Xmem_WData),
    .poXXX_XMem_RData              (sbFMC_MMIO_Xmem_RData),
    .piXXX_XMemAddr                (sbFMC_MMIO_Xmem_Addr),
    
    .poVoid                         ()

  );  // End of MMIO


  //============================================================================
  //  CONDITIONAL INSTANTIATION OF A LOOPBACK TURN BETWEEN ETH0 Ly2 and Ly3.  
  //    Depending on the values of gBitstreamUsage and gSecurityPriviledges.
  //============================================================================
  generate

  if ((gBitstreamUsage == "user") && (gSecurityPriviledges == "user")) begin: UserCfg

    //========================================================================
    //  INST: 10G ETHERNET SUBSYSTEM (OSI Network Layers 1+2)
    //========================================================================
    TenGigEth ETH0 (

      //-- Clocks and Resets inputs ----------------
      .piTOP_156_25Clk              (piTOP_156_25Clk),    // Freerunning
      .piCLKT_Gt_RefClk_n           (piCLKT_10GeClk_n),
      .piCLKT_Gt_RefClk_p           (piCLKT_10GeClk_p),
      .piTOP_Reset                  (piTOP_156_25Rst),
      //-- Clocks and Resets outputs ---------------
      .poSHL_CoreClk                (sETH0_ShlClk),
      .poSHL_CoreResetDone          (sETH0_CoreResetDone),

      //-- MMIO : Control inputs and Status outputs
      .piMMIO_RxEqualizerMode       (sMMIO_ETH0_RxEqualizerMode),
      .piMMIO_TxDriverSwing         (sMMIO_ETH0_TxDriverSwing),
      .piMMIO_TxPreCursor           (sMMIO_ETH0_TxPreCursor),
      .piMMIO_TxPostCursor          (sMMIO_ETH0_TxPostCursor),
      .piMMIO_PcsLoopbackEn         (sMMIO_ETH0_PcsLoopbackEn),
      .poMMIO_CoreReady             (sETH0_MMIO_CoreReady),
      .poMMIO_QpllLock              (sETH0_MMIO_QpllLock),

      //-- ECON : Gigabit Transceivers -------------
      .piECON_Gt_n                  (piECON_Eth_10Ge0_n),
      .piECON_Gt_p                  (piECON_Eth_10Ge0_p),
      .poECON_Gt_n                  (poECON_Eth_10Ge0_n),
      .poECON_Gt_p                  (poECON_Eth_10Ge0_p),

      //-- NTS0 : Network-Transport-Session ---------
      //---- Input AXI-Write Stream Interface ------
      .siLY3_Data_tdata             (ssNTS0_ETH0_Data_tdata),
      .siLY3_Data_tkeep             (ssNTS0_ETH0_Data_tkeep),
      .siLY3_Data_tvalid            (ssNTS0_ETH0_Data_tvalid),
      .siLY3_Data_tlast             (ssNTS0_ETH0_Data_tlast),
      .siLY3_Data_tready            (ssNTS0_ETH0_Data_tready),
      //---- Output AXI-Write Stream Interface -----
      .soLY3_Data_tdata             (ssETH0_NTS0_Data_tdata),
      .soLY3_Data_tkeep             (ssETH0_NTS0_Data_tkeep),
      .soLY3_Data_tvalid            (ssETH0_NTS0_Data_tvalid),
      .soLY3_Data_tlast             (ssETH0_NTS0_Data_tlast),
      .soLY3_Data_tready            (ssETH0_NTS0_Data_tready)

    );  // End of UserCfg.ETH0

  end // if ((gBitstreamUsage == "user") && (gSecurityPriviledges == "user"))

  else if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) begin: SuperCfg

    //========================================================================
    //  INST: 10G ETHERNET SUBSYSTEM (OSI Network Layers 1+2) (W/ LOOPBACK SUPPORT)
    //========================================================================
    TenGigEth_Flash ETH0 (

      //-- Clocks and Resets inputs ----------------
      .piTOP_156_25Clk              (piTOP_156_25Clk),    // Freerunning
      .piCLKT_Gt_RefClk_n           (piCLKT_10GeClk_n),
      .piCLKT_Gt_RefClk_p           (piCLKT_10GeClk_p),
      .piTOP_Reset                  (piTOP_156_25Rst),    // [TODO-Add piMMIO_Layer2Rst]
      //-- Clocks and Resets outputs ---------------
      .poSHL_CoreClk                (sETH0_ShlClk),
      .poSHL_CoreResetDone          (sETH0_CoreResetDone),

      //-- MMIO : Control inputs and Status outputs
      .piMMIO_RxEqualizerMode       (sMMIO_ETH0_RxEqualizerMode),
      .piMMIO_TxDriverSwing         (sMMIO_ETH0_TxDriverSwing),
      .piMMIO_TxPreCursor           (sMMIO_ETH0_TxPreCursor),
      .piMMIO_TxPostCursor          (sMMIO_ETH0_TxPostCursor),
      .piMMIO_PcsLoopbackEn         (sMMIO_ETH0_PcsLoopbackEn),
      .piMMIO_MacLoopbackEn         (sMMIO_ETH0_MacLoopbackEn),
      .piMMIO_MacAddrSwapEn         (sMMIO_ETH0_MacAddrSwapEn),
      .poMMIO_CoreReady             (sETH0_MMIO_CoreReady),
      .poMMIO_QpllLock              (sETH0_MMIO_QpllLock),

      //-- ECON : Gigabit Transceivers -------------
      .piECON_Gt_n                  (piECON_Eth_10Ge0_n),
      .piECON_Gt_p                  (piECON_Eth_10Ge0_p),
      .poECON_Gt_n                  (poECON_Eth_10Ge0_n),
      .poECON_Gt_p                  (poECON_Eth_10Ge0_p),

      //-- NTS : Network-Transport-Session ---------
      //---- Input AXI-Write Stream Interface ------
      .siLY3_Data_tdata             (ssNTS0_ETH0_Data_tdata),
      .siLY3_Data_tkeep             (ssNTS0_ETH0_Data_tkeep),
      .siLY3_Data_tvalid            (ssNTS0_ETH0_Data_tvalid),
      .siLY3_Data_tlast             (ssNTS0_ETH0_Data_tlast),
      .siLY3_Data_tready            (ssNTS0_ETH0_Data_tready),
      //---- Output AXI-Write Stream Interface -----
      .soLY3_Data_tdata             (ssETH0_NTS0_Data_tdata),
      .soLY3_Data_tkeep             (ssETH0_NTS0_Data_tkeep),
      .soLY3_Data_tvalid            (ssETH0_NTS0_Data_tvalid),
      .soLY3_Data_tlast             (ssETH0_NTS0_Data_tlast),
      .soLY3_Data_tready            (ssETH0_NTS0_Data_tready)

    );  // End of SuperCfg.ETH0 

  end // if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super"))

  endgenerate


  //============================================================================
  //  INST: NETWORK+TRANSPORT STACK SUBSYSTEM (OSI Network Layers 3+4)
  //============================================================================
  NetworkTransportStack_TcpIp NTS0 (
    //------------------------------------------------------
    //-- Global Clock used by the entire SHELL
    //--   (This is typically 'sETH0_ShlClk' and we use it all over the place)
    //------------------------------------------------------
    .piShlClk                         (sETH0_ShlClk),
    //------------------------------------------------------
    //-- Global Reset used by the entire SHELL
    //--  This is typically 'sETH0_ShlRst'. If the module is created by HLS,
    //--    we use it as the default startup reset of the module.
    //------------------------------------------------------
    .piShlRst                         (sETH0_ShlRst),
    
    //------------------------------------------------------
    //-- ETH / Ethernet Layer-2 Interfaces
    //------------------------------------------------------
    //--  Axi4-Stream Ethernet Rx Data --------
    .siETH_Data_tdata                 (ssETH0_NTS0_Data_tdata),
    .siETH_Data_tkeep                 (ssETH0_NTS0_Data_tkeep),
    .siETH_Data_tlast                 (ssETH0_NTS0_Data_tlast),
    .siETH_Data_tvalid                (ssETH0_NTS0_Data_tvalid),
    .siETH_Data_tready                (ssETH0_NTS0_Data_tready),
    //-- Axi4-Stream Ethernet Tx Data --------
    .soETH_Data_tdata                 (ssNTS0_ETH0_Data_tdata),
    .soETH_Data_tkeep                 (ssNTS0_ETH0_Data_tkeep),
    .soETH_Data_tlast                 (ssNTS0_ETH0_Data_tlast),
    .soETH_Data_tvalid                (ssNTS0_ETH0_Data_tvalid),
    .soETH_Data_tready                (ssNTS0_ETH0_Data_tready),  

    //------------------------------------------------------
    //-- MEM / TxP Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path / S2MM-AXIS --------------------
    //---- Axi4-Stream Read Command -----------
    .soMEM_TxP_RdCmd_tdata            (ssNTS0_MEM_TxP_RdCmd_tdata),
    .soMEM_TxP_RdCmd_tvalid           (ssNTS0_MEM_TxP_RdCmd_tvalid),
    .soMEM_TxP_RdCmd_tready           (ssNTS0_MEM_TxP_RdCmd_tready),
    //---- Axi4-Stream Read Status ------------
    .siMEM_TxP_RdSts_tdata            (ssMEM_NTS0_TxP_RdSts_tdata),
    .siMEM_TxP_RdSts_tvalid           (ssMEM_NTS0_TxP_RdSts_tvalid),
    .siMEM_TxP_RdSts_tready           (ssMEM_NTS0_TxP_RdSts_tready),
    //---- Axi4-Stream Data Input Channel -----
    .siMEM_TxP_Data_tdata             (ssMEM_NTS0_TxP_Read_tdata),
    .siMEM_TxP_Data_tkeep             (ssMEM_NTS0_TxP_Read_tkeep),
    .siMEM_TxP_Data_tlast             (ssMEM_NTS0_TxP_Read_tlast),
    .siMEM_TxP_Data_tvalid            (ssMEM_NTS0_TxP_Read_tvalid),
    .siMEM_TxP_Data_tready            (ssMEM_NTS0_TxP_Read_tready),
    //---- Axi4-Stream Write Command ----------
    .soMEM_TxP_WrCmd_tdata            (ssNTS0_MEM_TxP_WrCmd_tdata),
    .soMEM_TxP_WrCmd_tvalid           (ssNTS0_MEM_TxP_WrCmd_tvalid),
    .soMEM_TxP_WrCmd_tready           (ssNTS0_MEM_TxP_WrCmd_tready),
    //---- Axi4-Stream Write Status -----------
    .siMEM_TxP_WrSts_tdata            (ssMEM_NTS0_TxP_WrSts_tdata),
    .siMEM_TxP_WrSts_tvalid           (ssMEM_NTS0_TxP_WrSts_tvalid),
    .siMEM_TxP_WrSts_tready           (ssMEM_NTS0_TxP_WrSts_tready),
    //---- Axi4-Stream Data Output Channel ----
    .soMEM_TxP_Data_tdata             (ssNTS0_MEM_TxP_Write_tdata),
    .soMEM_TxP_Data_tkeep             (ssNTS0_MEM_TxP_Write_tkeep),
    .soMEM_TxP_Data_tlast             (ssNTS0_MEM_TxP_Write_tlast),
    .soMEM_TxP_Data_tvalid            (ssNTS0_MEM_TxP_Write_tvalid),
    .soMEM_TxP_Data_tready            (ssNTS0_MEM_TxP_Write_tready),

    //------------------------------------------------------
    //-- MEM / RxP Interfaces
    //------------------------------------------------------
    //-- FPGA Receive Path / S2MM-AXIS -------------
    //---- Axi4-Stream Read Command -----------
    .soMEM_RxP_RdCmd_tdata            (ssNTS0_MEM_RxP_RdCmd_tdata),
    .soMEM_RxP_RdCmd_tvalid           (ssNTS0_MEM_RxP_RdCmd_tvalid),
    .soMEM_RxP_RdCmd_tready           (ssNTS0_MEM_RxP_RdCmd_tready),
    //---- Axi4-Stream Read Status ------------
    .siMEM_RxP_RdSts_tdata            (ssMEM_NTS0_RxP_RdSts_tdata),
    .siMEM_RxP_RdSts_tvalid           (ssMEM_NTS0_RxP_RdSts_tvalid),
    .siMEM_RxP_RdSts_tready           (ssMEM_NTS0_RxP_RdSts_tready),
    //---- Axi4-Stream Data Input Channel ------
    .siMEM_RxP_Data_tdata             (ssMEM_NTS0_RxP_Read_tdata),
    .siMEM_RxP_Data_tkeep             (ssMEM_NTS0_RxP_Read_tkeep),
    .siMEM_RxP_Data_tlast             (ssMEM_NTS0_RxP_Read_tlast),
    .siMEM_RxP_Data_tvalid            (ssMEM_NTS0_RxP_Read_tvalid),
    .siMEM_RxP_Data_tready            (ssMEM_NTS0_RxP_Read_tready),
    //---- Axi4-Stream Write Command ----------
    .soMEM_RxP_WrCmd_tdata            (ssNTS0_MEM_RxP_WrCmd_tdata),
    .soMEM_RxP_WrCmd_tvalid           (ssNTS0_MEM_RxP_WrCmd_tvalid),
    .soMEM_RxP_WrCmd_tready           (ssNTS0_MEM_RxP_WrCmd_tready),
    //---- Axi4-Stream Write Status -----------
    .siMEM_RxP_WrSts_tdata            (ssMEM_NTS0_RxP_WrSts_tdata),
    .siMEM_RxP_WrSts_tvalid           (ssMEM_NTS0_RxP_WrSts_tvalid),
    .siMEM_RxP_WrSts_tready           (ssMEM_NTS0_RxP_WrSts_tready),
    //---- Axi4-Stream Data Output Channel ----
    .soMEM_RxP_Data_tdata             (ssNTS0_MEM_RxP_Write_tdata),
    .soMEM_RxP_Data_tkeep             (ssNTS0_MEM_RxP_Write_tkeep),
    .soMEM_RxP_Data_tlast             (ssNTS0_MEM_RxP_Write_tlast),
    .soMEM_RxP_Data_tvalid            (ssNTS0_MEM_RxP_Write_tvalid),
    .soMEM_RxP_Data_tready            (ssNTS0_MEM_RxP_Write_tready),

    //------------------------------------------------------
    //-- UAIF / UDP Tx Data Interfaces (.i.e APP-->NTS)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .siAPP_Udp_Data_tdata             (ssNRC_NTS0_Udp_Data_tdata),
    .siAPP_Udp_Data_tkeep             (ssNRC_NTS0_Udp_Data_tkeep),
    .siAPP_Udp_Data_tlast             (ssNRC_NTS0_Udp_Data_tlast),
    .siAPP_Udp_Data_tvalid            (ssNRC_NTS0_Udp_Data_tvalid),
    .siAPP_Udp_Data_tready            (ssNRC_NTS0_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .siAPP_Udp_Meta_tdata             (ssNRC_NTS0_Udp_Meta_tdata),
    .siAPP_Udp_Meta_tvalid            (ssNRC_NTS0_Udp_Meta_tvalid),
    .siAPP_Udp_Meta_tready            (ssNRC_NTS0_Udp_Meta_tready),
    //---- Axis4Stream UDP Data Length ---------
    .siAPP_Udp_DLen_tdata             (ssNRC_NTS0_Udp_DLen_tdata),
    .siAPP_Udp_DLen_tvalid            (ssNRC_NTS0_Udp_DLen_tvalid),
    .siAPP_Udp_DLen_tready            (ssNRC_NTS0_Udp_DLen_tready),
    //------------------------------------------------------
    //-- UAIF / Rx Data Interfaces (.i.e NTS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .soAPP_Udp_Data_tdata             (ssNTS0_NRC_Udp_Data_tdata),
    .soAPP_Udp_Data_tkeep             (ssNTS0_NRC_Udp_Data_tkeep),
    .soAPP_Udp_Data_tlast             (ssNTS0_NRC_Udp_Data_tlast),
    .soAPP_Udp_Data_tvalid            (ssNTS0_NRC_Udp_Data_tvalid),
    .soAPP_Udp_Data_tready            (ssNTS0_NRC_Udp_Data_tready),
     //---- Axi4-Stream UDP Metadata -----------
    .soAPP_Udp_Meta_tdata             (ssNTS0_NRC_Udp_Meta_tdata),
    .soAPP_Udp_Meta_tvalid            (ssNTS0_NRC_Udp_Meta_tvalid),
    .soAPP_Udp_Meta_tready            (ssNTS0_NRC_Udp_Meta_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Rx Ctrl Interfaces (.i.e NTS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Listen Request -----
    .siAPP_Udp_LsnReq_tdata           (ssNRC_NTS0_Udp_LsnReq_tdata),
    .siAPP_Udp_LsnReq_tvalid          (ssNRC_NTS0_Udp_LsnReq_tvalid),
    .siAPP_Udp_LsnReq_tready          (ssNRC_NTS0_Udp_LsnReq_tready),
    //---- Axi4-Stream UDP Listen Reply --------
    .soAPP_Udp_LsnRep_tdata           (ssNTS0_NRC_Udp_LsnRep_tdata),
    .soAPP_Udp_LsnRep_tvalid          (ssNTS0_NRC_Udp_LsnRep_tvalid),
    .soAPP_Udp_LsnRep_tready          (ssNTS0_NRC_Udp_LsnRep_tready),
    //---- Axi4-Stream UDP Close Request -------
    .siAPP_Udp_ClsReq_tdata           (ssNRC_NTS0_Udp_ClsReq_tdata),
    .siAPP_Udp_ClsReq_tvalid          (ssNRC_NTS0_Udp_ClsReq_tvalid),
    .siAPP_Udp_ClsReq_tready          (ssNRC_NTS0_Udp_ClsReq_tready),
    //---- Axi4-Stream UDP Close Reply ---------
    .soAPP_Udp_ClsRep_tdata           (ssNTS0_NRC_Udp_ClsRep_tdata),
    .soAPP_Udp_ClsRep_tvalid          (ssNTS0_NRC_Udp_ClsRep_tvalid),
    .soAPP_Udp_ClsRep_tready          (ssNTS0_NRC_Udp_ClsRep_tready),

    //------------------------------------------------------
    //-- ROLE / Tcp / TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path (ROLE-->NTS) -----------
    //---- Stream TCP Data ---------------------
    .siAPP_Tcp_Data_tdata             (ssNRC_TOE_Tcp_Data_tdata),
    .siAPP_Tcp_Data_tkeep             (ssNRC_TOE_Tcp_Data_tkeep),
    .siAPP_Tcp_Data_tlast             (ssNRC_TOE_Tcp_Data_tlast),
    .siAPP_Tcp_Data_tvalid            (ssNRC_TOE_Tcp_Data_tvalid),
    .siAPP_Tcp_Data_tready            (ssNRC_TOE_Tcp_Data_tready),
    //---- Stream TCP Metadata -------------
    .siAPP_Tcp_Meta_tdata             (ssNRC_TOE_Tcp_Meta_tdata),
    .siAPP_Tcp_Meta_tvalid            (ssNRC_TOE_Tcp_Meta_tvalid),
    .siAPP_Tcp_Meta_tready            (ssNRC_TOE_Tcp_Meta_tready),
    //---- Stream TCP Data Status -----------
    .soAPP_Tcp_DSts_tdata             (ssTOE_NRC_Tcp_DSts_tdata),
    .soAPP_Tcp_DSts_tvalid            (ssTOE_NRC_Tcp_DSts_tvalid),
    .soAPP_Tcp_DSts_tready            (ssTOE_NRC_Tcp_DSts_tready),

    //---------------------------------------------------
    //-- ROLE / Tcp / RxP Data Flow Interfaces    
    //---------------------------------------------------
    //-- FPGA Receive Path (NTS-->ROLE) -------------    
    //-- Stream TCP Data -----------------------         
    .soAPP_Tcp_Data_tdata             (ssTOE_NRC_Tcp_Data_tdata),
    .soAPP_Tcp_Data_tkeep             (ssTOE_NRC_Tcp_Data_tkeep),
    .soAPP_Tcp_Data_tlast             (ssTOE_NRC_Tcp_Data_tlast),
    .soAPP_Tcp_Data_tvalid            (ssTOE_NRC_Tcp_Data_tvalid),
    .soAPP_Tcp_Data_tready            (ssTOE_NRC_Tcp_Data_tready),
    //-- Stream TCP Metadata ---------------
    .soAPP_Tcp_Meta_tdata             (ssTOE_NRC_Tcp_Meta_tdata),
    .soAPP_Tcp_Meta_tvalid            (ssTOE_NRC_Tcp_Meta_tvalid),
    .soAPP_Tcp_Meta_tready            (ssTOE_NRC_Tcp_Meta_tready),
     //-- Stream TCP Data Notification ------
    .soAPP_Tcp_Notif_tdata            (ssTOE_NRC_Tcp_Notif_tdata),
    .soAPP_Tcp_Notif_tvalid           (ssTOE_NRC_Tcp_Notif_tvalid),
    .soAPP_Tcp_Notif_tready           (ssTOE_NRC_Tcp_Notif_tready),
    //-- Stream TCP Data Request ------------
    .siAPP_Tcp_DReq_tdata             (ssNRC_TOE_Tcp_DReq_tdata),    
    .siAPP_Tcp_DReq_tvalid            (ssNRC_TOE_Tcp_DReq_tvalid),
    .siAPP_Tcp_DReq_tready            (ssNRC_TOE_Tcp_DReq_tready),
    
    //------------------------------------------------------
    //-- ROLE / Tcp / TxP Ctlr Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path (ROLE-->ETH) -----------
    //---- Stream TCP Open Session Request -----
    .siAPP_Tcp_OpnReq_tdata           (ssNRC_TOE_Tcp_OpnReq_tdata),
    .siAPP_Tcp_OpnReq_tvalid          (ssNRC_TOE_Tcp_OpnReq_tvalid),
    .siAPP_Tcp_OpnReq_tready          (ssNRC_TOE_Tcp_OpnReq_tready),
    //---- Stream TCP Open Session Status ------
    .soAPP_Tcp_OpnRep_tdata           (ssTOE_NRC_Tcp_OpnRep_tdata),
    .soAPP_Tcp_OpnRep_tvalid          (ssTOE_NRC_Tcp_OpnRep_tvalid),
    .soAPP_Tcp_OpnRep_tready          (ssTOE_NRC_Tcp_OpnRep_tready),
    //---- Stream TCP Close Request ------------
    .siAPP_Tcp_ClsReq_tdata           (ssNRC_TOE_Tcp_ClsReq_tdata),
    .siAPP_Tcp_ClsReq_tvalid          (ssNRC_TOE_Tcp_ClsReq_tvalid),
    .siAPP_Tcp_ClsReq_tready          (ssNRC_TOE_Tcp_ClsReq_tready),
    
    //------------------------------------------------------
    //-- ROLE / Tcp / RxP Ctlr Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Receive Path (ETH-->ROLE) ------------
    //---- Stream TCP Listen Request -----------
    .siAPP_Tcp_LsnReq_tdata           (ssNRC_TOE_Tcp_LsnReq_tdata),
    .siAPP_Tcp_LsnReq_tvalid          (ssNRC_TOE_Tcp_LsnReq_tvalid),
    .siAPP_Tcp_LsnReq_tready          (ssNRC_TOE_Tcp_LsnReq_tready),
    //---- Stream TCP Listen Status ------------
    .soAPP_Tcp_LsnAck_tdata           (ssTOE_NRC_Tcp_LsnAck_tdata),
    .soAPP_Tcp_LsnAck_tvalid          (ssTOE_NRC_Tcp_LsnAck_tvalid),
    .soAPP_Tcp_LsnAck_tready          (ssTOE_NRC_Tcp_LsnAck_tready),
    
    //------------------------------------------------------
    //-- MMIO / Interfaces
    //------------------------------------------------------
    .piMMIO_Layer2Rst                 (sMMIO_LayerRst[2]),
    .piMMIO_Layer3Rst                 (sMMIO_LayerRst[3]),
    .piMMIO_Layer4Rst                 (sMMIO_LayerRst[4]),
    .piMMIO_Layer4En                  (sMMIO_LayerEn[4]),
    .piMMIO_MacAddress                (sMMIO_NTS0_MacAddress),
    .piMMIO_Ip4Address                 (sMMIO_NTS0_Ip4Address),
    .piMMIO_SubNetMask                (sMMIO_NTS0_SubNetMask),
    .piMMIO_GatewayAddr               (sMMIO_NTS0_GatewayAddr),
    .poMMIO_CamReady                  (sNTS0_MMIO_CamReady),      // [TODO-Merge this signal with NtsReady]
    .poMMIO_NtsReady                  (sNTS0_MMIO_NtsReady),

    .poVoid                           ()

  );  // End of NTS0


  //============================================================================
  //  INST: SYNCHRONOUS DYNAMIC RANDOM ACCESS MEMORY SUBSYSTEM
  //============================================================================
  MemorySubSystem #(
    "user",     // gSecurityPriviledges
    "user"      // gBitstreamUsage
  ) MEM (
    //------------------------------------------------------
    //-- Global Clock used by the entire SHELL
    //------------------------------------------------------
    .piSHL_Clk                        (sETH0_ShlClk),

    //------------------------------------------------------
    //-- Global Reset used by the entire SHELL
    //------------------------------------------------------
    .piSHL_Rst                        (sETH0_ShlRst),

    //------------------------------------------------------
    //-- Alternate System Reset
    //------------------------------------------------------
    .piMMIO_Rst                       (sMMIO_LayerRst[1]),   // [FIXME]

    //------------------------------------------------------
    //-- DDR4 Reference Memory Clocks
    //------------------------------------------------------
    .piCLKT_Mem0Clk_n                 (piCLKT_Mem0Clk_n),
    .piCLKT_Mem0Clk_p                 (piCLKT_Mem0Clk_p),
    .piCLKT_Mem1Clk_n                 (piCLKT_Mem1Clk_n),
    .piCLKT_Mem1Clk_p                 (piCLKT_Mem1Clk_p),

    //------------------------------------------------------ 
    //-- MMIO / Status Interface
    //------------------------------------------------------
    .poMMIO_Mc0_InitCalComplete       (sMEM_MMIO_Mc0InitCalComplete),
    .poMMIO_Mc1_InitCalComplete       (sMEM_MMIO_Mc1InitCalComplete),

    //------------------------------------------------------
    //-- NTS / Mem / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS ---------------
    //---- Stream Read Command ---------------
    .siNTS_Mem_TxP_RdCmd_tdata        (ssNTS0_MEM_TxP_RdCmd_tdata),
    .siNTS_Mem_TxP_RdCmd_tvalid       (ssNTS0_MEM_TxP_RdCmd_tvalid),
    .siNTS_Mem_TxP_RdCmd_tready       (ssNTS0_MEM_TxP_RdCmd_tready),
    //---- Stream Read Status ----------------
    .soNTS_Mem_TxP_RdSts_tdata        (ssMEM_NTS0_TxP_RdSts_tdata),
    .soNTS_Mem_TxP_RdSts_tvalid       (ssMEM_NTS0_TxP_RdSts_tvalid),
    .soNTS_Mem_TxP_RdSts_tready       (ssMEM_NTS0_TxP_RdSts_tready),
    //---- Stream Data Output Channel --------
    .soNTS_Mem_TxP_Read_tdata         (ssMEM_NTS0_TxP_Read_tdata),
    .soNTS_Mem_TxP_Read_tkeep         (ssMEM_NTS0_TxP_Read_tkeep),
    .soNTS_Mem_TxP_Read_tlast         (ssMEM_NTS0_TxP_Read_tlast),
    .soNTS_Mem_TxP_Read_tvalid        (ssMEM_NTS0_TxP_Read_tvalid),
    .soNTS_Mem_TxP_Read_tready        (ssMEM_NTS0_TxP_Read_tready),
    //---- Stream Write Command --------------
    .siNTS_Mem_TxP_WrCmd_tdata        (ssNTS0_MEM_TxP_WrCmd_tdata),
    .siNTS_Mem_TxP_WrCmd_tvalid       (ssNTS0_MEM_TxP_WrCmd_tvalid),
    .siNTS_Mem_TxP_WrCmd_tready       (ssNTS0_MEM_TxP_WrCmd_tready),
    //---- Stream Write Status --------------
    .soNTS_Mem_TxP_WrSts_tdata        (ssMEM_NTS0_TxP_WrSts_tdata),
    .soNTS_Mem_TxP_WrSts_tvalid       (ssMEM_NTS0_TxP_WrSts_tvalid),
    .soNTS_Mem_TxP_WrSts_tready       (ssMEM_NTS0_TxP_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .siNTS_Mem_TxP_Write_tdata        (ssNTS0_MEM_TxP_Write_tdata),
    .siNTS_Mem_TxP_Write_tkeep        (ssNTS0_MEM_TxP_Write_tkeep),
    .siNTS_Mem_TxP_Write_tlast        (ssNTS0_MEM_TxP_Write_tlast),
    .siNTS_Mem_TxP_Write_tvalid       (ssNTS0_MEM_TxP_Write_tvalid),
    .siNTS_Mem_TxP_Write_tready       (ssNTS0_MEM_TxP_Write_tready),

    //------------------------------------------------------
    //-- NTS / Mem / Rx Interface
    //------------------------------------------------------
    //-- Receive Path  / S2MM-AXIS -----------------
    //---- Stream Read Command ---------------
    .siNTS_Mem_RxP_RdCmd_tdata        (ssNTS0_MEM_RxP_RdCmd_tdata),
    .siNTS_Mem_RxP_RdCmd_tvalid       (ssNTS0_MEM_RxP_RdCmd_tvalid),
    .siNTS_Mem_RxP_RdCmd_tready       (ssNTS0_MEM_RxP_RdCmd_tready),
    //---- Stream Read Status ----------------
    .soNTS_Mem_RxP_RdSts_tdata        (ssMEM_NTS0_RxP_RdSts_tdata),
    .soNTS_Mem_RxP_RdSts_tvalid       (ssMEM_NTS0_RxP_RdSts_tvalid),
    .soNTS_Mem_RxP_RdSts_tready       (ssMEM_NTS0_RxP_RdSts_tready),
    //---- Stream Data Output Channel --------
    .soNTS_Mem_RxP_Read_tdata         (ssMEM_NTS0_RxP_Read_tdata),
    .soNTS_Mem_RxP_Read_tkeep         (ssMEM_NTS0_RxP_Read_tkeep),
    .soNTS_Mem_RxP_Read_tlast         (ssMEM_NTS0_RxP_Read_tlast),
    .soNTS_Mem_RxP_Read_tvalid        (ssMEM_NTS0_RxP_Read_tvalid),
    .soNTS_Mem_RxP_Read_tready        (ssMEM_NTS0_RxP_Read_tready),
    //---- Stream Write Command --------------
    .siNTS_Mem_RxP_WrCmd_tdata        (ssNTS0_MEM_RxP_WrCmd_tdata),
    .siNTS_Mem_RxP_WrCmd_tvalid       (ssNTS0_MEM_RxP_WrCmd_tvalid),
    .siNTS_Mem_RxP_WrCmd_tready       (ssNTS0_MEM_RxP_WrCmd_tready),
    //---- Stream Write Status ---------------
    .soNTS_Mem_RxP_WrSts_tdata        (ssMEM_NTS0_RxP_WrSts_tdata),
    .soNTS_Mem_RxP_WrSts_tvalid       (ssMEM_NTS0_RxP_WrSts_tvalid),
    .soNTS_Mem_RxP_WrSts_tready       (ssMEM_NTS0_RxP_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .siNTS_Mem_RxP_Write_tdata        (ssNTS0_MEM_RxP_Write_tdata),
    .siNTS_Mem_RxP_Write_tkeep        (ssNTS0_MEM_RxP_Write_tkeep),
    .siNTS_Mem_RxP_Write_tlast        (ssNTS0_MEM_RxP_Write_tlast),
    .siNTS_Mem_RxP_Write_tvalid       (ssNTS0_MEM_RxP_Write_tvalid),
    .siNTS_Mem_RxP_Write_tready       (ssNTS0_MEM_RxP_Write_tready),  

    //------------------------------------------------------
    // -- Physical DDR4 Interface #0
    //------------------------------------------------------
    .pioDDR_Mem_Mc0_DmDbi_n           (pioDDR4_Mem_Mc0_DmDbi_n),
    .pioDDR_Mem_Mc0_Dq                (pioDDR4_Mem_Mc0_Dq),
    .pioDDR_Mem_Mc0_Dqs_n             (pioDDR4_Mem_Mc0_Dqs_n),
    .pioDDR_Mem_Mc0_Dqs_p             (pioDDR4_Mem_Mc0_Dqs_p),    
    .poDDR4_Mem_Mc0_Act_n             (poDDR4_Mem_Mc0_Act_n),
    .poDDR4_Mem_Mc0_Adr               (poDDR4_Mem_Mc0_Adr),
    .poDDR4_Mem_Mc0_Ba                (poDDR4_Mem_Mc0_Ba),
    .poDDR4_Mem_Mc0_Bg                (poDDR4_Mem_Mc0_Bg),
    .poDDR4_Mem_Mc0_Cke               (poDDR4_Mem_Mc0_Cke),
    .poDDR4_Mem_Mc0_Odt               (poDDR4_Mem_Mc0_Odt),
    .poDDR4_Mem_Mc0_Cs_n              (poDDR4_Mem_Mc0_Cs_n),
    .poDDR4_Mem_Mc0_Ck_n              (poDDR4_Mem_Mc0_Ck_n),
    .poDDR4_Mem_Mc0_Ck_p              (poDDR4_Mem_Mc0_Ck_p),
    .poDDR4_Mem_Mc0_Reset_n           (poDDR4_Mem_Mc0_Reset_n),

    //------------------------------------------------------
    //-- ROLE / Mem / Mp0 Interface
    //------------------------------------------------------
    //-- Memory Port #0 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .siROL_Mem_Mp0_RdCmd_tdata        (siDCP_ROL_Mem_Mp0_RdCmd_tdata),
    .siROL_Mem_Mp0_RdCmd_tvalid       (siDCP_ROL_Mem_Mp0_RdCmd_tvalid),
    .siROL_Mem_Mp0_RdCmd_tready       (siDCP_ROL_Mem_Mp0_RdCmd_tready),
    //---- Stream Read Status ----------------
    .soROL_Mem_Mp0_RdSts_tdata        (soDCP_ROL_Mem_Mp0_RdSts_tdata),
    .soROL_Mem_Mp0_RdSts_tvalid       (soDCP_ROL_Mem_Mp0_RdSts_tvalid),
    .soROL_Mem_Mp0_RdSts_tready       (soDCP_ROL_Mem_Mp0_RdSts_tready),
    //---- Stream Data Output Channel --------
    .soROL_Mem_Mp0_Read_tdata         (soDCP_ROL_Mem_Mp0_Read_tdata),
    .soROL_Mem_Mp0_Read_tkeep         (soDCP_ROL_Mem_Mp0_Read_tkeep),
    .soROL_Mem_Mp0_Read_tlast         (soDCP_ROL_Mem_Mp0_Read_tlast),
    .soROL_Mem_Mp0_Read_tvalid        (soDCP_ROL_Mem_Mp0_Read_tvalid),
    .soROL_Mem_Mp0_Read_tready        (soDCP_ROL_Mem_Mp0_Read_tready),
    //---- Stream Write Command --------------
    .siROL_Mem_Mp0_WrCmd_tdata        (siDCP_ROL_Mem_Mp0_WrCmd_tdata),
    .siROL_Mem_Mp0_WrCmd_tvalid       (siDCP_ROL_Mem_Mp0_WrCmd_tvalid),
    .siROL_Mem_Mp0_WrCmd_tready       (siDCP_ROL_Mem_Mp0_WrCmd_tready),
    //---- Stream Write Status ---------------
    .soROL_Mem_Mp0_WrSts_tdata        (soDCP_ROL_Mem_Mp0_WrSts_tdata),
    .soROL_Mem_Mp0_WrSts_tvalid       (soDCP_ROL_Mem_Mp0_WrSts_tvalid),
    .soROL_Mem_Mp0_WrSts_tready       (soDCP_ROL_Mem_Mp0_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .siROL_Mem_Mp0_Write_tdata        (siDCP_ROL_Mem_Mp0_Write_tdata),
    .siROL_Mem_Mp0_Write_tkeep        (siDCP_ROL_Mem_Mp0_Write_tkeep),
    .siROL_Mem_Mp0_Write_tlast        (siDCP_ROL_Mem_Mp0_Write_tlast),
    .siROL_Mem_Mp0_Write_tvalid       (siDCP_ROL_Mem_Mp0_Write_tvalid),
    .siROL_Mem_Mp0_Write_tready       (siDCP_ROL_Mem_Mp0_Write_tready),

    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    .miROL_Mem_Mp1_AWID               (miDCP_ROL_Mem_Mp1_AWID   ),
    .miROL_Mem_Mp1_AWADDR             (miDCP_ROL_Mem_Mp1_AWADDR ),
    .miROL_Mem_Mp1_AWLEN              (miDCP_ROL_Mem_Mp1_AWLEN  ),
    .miROL_Mem_Mp1_AWSIZE             (miDCP_ROL_Mem_Mp1_AWSIZE ),
    .miROL_Mem_Mp1_AWBURST            (miDCP_ROL_Mem_Mp1_AWBURST),
    .miROL_Mem_Mp1_AWVALID            (miDCP_ROL_Mem_Mp1_AWVALID),
    .miROL_Mem_Mp1_AWREADY            (miDCP_ROL_Mem_Mp1_AWREADY),
    .miROL_Mem_Mp1_WDATA              (miDCP_ROL_Mem_Mp1_WDATA  ),
    .miROL_Mem_Mp1_WSTRB              (miDCP_ROL_Mem_Mp1_WSTRB  ),
    .miROL_Mem_Mp1_WLAST              (miDCP_ROL_Mem_Mp1_WLAST  ),
    .miROL_Mem_Mp1_WVALID             (miDCP_ROL_Mem_Mp1_WVALID ),
    .miROL_Mem_Mp1_WREADY             (miDCP_ROL_Mem_Mp1_WREADY ),
    .miROL_Mem_Mp1_BID                (miDCP_ROL_Mem_Mp1_BID    ),
    .miROL_Mem_Mp1_BRESP              (miDCP_ROL_Mem_Mp1_BRESP  ),
    .miROL_Mem_Mp1_BVALID             (miDCP_ROL_Mem_Mp1_BVALID ),
    .miROL_Mem_Mp1_BREADY             (miDCP_ROL_Mem_Mp1_BREADY ),
    .miROL_Mem_Mp1_ARID               (miDCP_ROL_Mem_Mp1_ARID   ),
    .miROL_Mem_Mp1_ARADDR             (miDCP_ROL_Mem_Mp1_ARADDR ),
    .miROL_Mem_Mp1_ARLEN              (miDCP_ROL_Mem_Mp1_ARLEN  ),
    .miROL_Mem_Mp1_ARSIZE             (miDCP_ROL_Mem_Mp1_ARSIZE ),
    .miROL_Mem_Mp1_ARBURST            (miDCP_ROL_Mem_Mp1_ARBURST),
    .miROL_Mem_Mp1_ARVALID            (miDCP_ROL_Mem_Mp1_ARVALID),
    .miROL_Mem_Mp1_ARREADY            (miDCP_ROL_Mem_Mp1_ARREADY),
    .miROL_Mem_Mp1_RID                (miDCP_ROL_Mem_Mp1_RID    ),
    .miROL_Mem_Mp1_RDATA              (miDCP_ROL_Mem_Mp1_RDATA  ),
    .miROL_Mem_Mp1_RRESP              (miDCP_ROL_Mem_Mp1_RRESP  ),
    .miROL_Mem_Mp1_RLAST              (miDCP_ROL_Mem_Mp1_RLAST  ),
    .miROL_Mem_Mp1_RVALID             (miDCP_ROL_Mem_Mp1_RVALID ),
    .miROL_Mem_Mp1_RREADY             (miDCP_ROL_Mem_Mp1_RREADY ),

    //------------------------------------------------------
    // -- Physical DDR4 Interface #1
    //------------------------------------------------------
    .pioDDR_Mem_Mc1_DmDbi_n           (pioDDR4_Mem_Mc1_DmDbi_n),
    .pioDDR_Mem_Mc1_Dq                (pioDDR4_Mem_Mc1_Dq),
    .pioDDR_Mem_Mc1_Dqs_n             (pioDDR4_Mem_Mc1_Dqs_n),
    .pioDDR_Mem_Mc1_Dqs_p             (pioDDR4_Mem_Mc1_Dqs_p),
    .poDDR4_Mem_Mc1_Act_n             (poDDR4_Mem_Mc1_Act_n),
    .poDDR4_Mem_Mc1_Adr               (poDDR4_Mem_Mc1_Adr),
    .poDDR4_Mem_Mc1_Ba                (poDDR4_Mem_Mc1_Ba),
    .poDDR4_Mem_Mc1_Bg                (poDDR4_Mem_Mc1_Bg),
    .poDDR4_Mem_Mc1_Cke               (poDDR4_Mem_Mc1_Cke),
    .poDDR4_Mem_Mc1_Odt               (poDDR4_Mem_Mc1_Odt),
    .poDDR4_Mem_Mc1_Cs_n              (poDDR4_Mem_Mc1_Cs_n),
    .poDDR4_Mem_Mc1_Ck_n              (poDDR4_Mem_Mc1_Ck_n),
    .poDDR4_Mem_Mc1_Ck_p              (poDDR4_Mem_Mc1_Ck_p),
    .poDDR4_Mem_Mc1_Reset_n           (poDDR4_Mem_Mc1_Reset_n)
  );  // End of MEM

  //============================================================================
  //  INST: AXI HARDWARE INTERNAL CONFIGURATION ACCESS PORT
  //============================================================================
  HWICAPC HWICAP (
    .icap_clk       (sETH0_ShlClk),
    .eos_in         (1),
    //.s_axi_aclk     (sCASTOR_HWICAPC_axi_aclk),
    .s_axi_aclk     (sETH0_ShlClk),
    //.s_axi_aresetn  (sCASTOR_HWICAPC_axi_aresetn),dst_ran
    .s_axi_aresetn  (~ piTOP_156_25Rst),
    .s_axi_awaddr   (ssFMC_HWICAP_Axi_awaddr),
    .s_axi_awvalid  (ssFMC_HWICAP_Axi_awvalid),
    .s_axi_awready  (ssFMC_HWICAP_Axi_awready),
    .s_axi_wdata    (ssFMC_HWICAP_Axi_wdata),
    .s_axi_wstrb    (ssFMC_HWICAP_Axi_wstrb),
    .s_axi_wvalid   (ssFMC_HWICAP_Axi_wvalid),
    .s_axi_wready   (ssFMC_HWICAP_Axi_wready),
    .s_axi_bresp    (ssFMC_HWICAP_Axi_bresp),
    .s_axi_bvalid   (ssFMC_HWICAP_Axi_bvalid),
    .s_axi_bready   (ssFMC_HWICAP_Axi_bready),
    .s_axi_araddr   (ssFMC_HWICAP_Axi_araddr),
    .s_axi_arvalid  (ssFMC_HWICAP_Axi_arvalid), 
    .s_axi_arready  (ssFMC_HWICAP_Axi_arready),
    .s_axi_rdata    (ssFMC_HWICAP_Axi_rdata),
    .s_axi_rresp    (ssFMC_HWICAP_Axi_rresp),
    .s_axi_rvalid   (ssFMC_HWICAP_Axi_rvalid),
    .s_axi_rready   (ssFMC_HWICAP_Axi_rready),
    .ip2intc_irpt   (ssFMC_HWICAP_ip2intc_irpt)
  );


  smallTimer #(
    .clockFrequencyHz(156250000)
  ) TIME (
    .piClk      (sETH0_ShlClk),
    .piSyncRst  (sETH0_ShlRst),
    .poSeconds  (sTIME_Broadcast_seconds),
    .poMinutes  (sTIME_Broadcast_minutes),
    .poHours    (sTIME_Broadcast_hours)
  );



  //============================================================================
  //  INST: FPGA MANAGEMENT CORE
  //============================================================================
  FpgaManagementCore FMC (
    //-- Global Clock used by the entire SHELL -------------
    .ap_clk                 (sETH0_ShlClk),
    //-- Global Reset used by the entire SHELL -------------
    //.ap_rst_n               (~ piTOP_156_25Rst),
    .ap_rst_n               (~ sMMIO_LayerRst[5]),
    //core should start immediately 
    //.ap_start               (1),
    .ap_start                 (sMMIO_LayerEn[5] & (~ sMMIO_LayerRst[5])), 
    //.piSysReset_V           (piSHL_156_25Rst_delayed),
    //.piSysReset_V_ap_vld   (1),
    //.poMMIO_V_ap_vld     ( ),
    .piMMIO_V                 (sMMIO_FMC_WrFmcReg),
    .piMMIO_V_ap_vld          (1),
    .poMMIO_V                 (sFMC_MMIO_RdFmcReg),
    .piLayer4enabled_V        (sMMIO_LayerEn[4] & (~ sMMIO_LayerRst[4])),
    .piLayer4enabled_V_ap_vld (1),
    .piLayer6enabled_V        (sMMIO_LayerEn[6] & (~ sMMIO_LayerRst[6])),
    .piLayer6enabled_V_ap_vld (1),
    .piLayer7enabled_V        (sMMIO_LayerEn[7] & (~ sMMIO_LayerRst[7])),
    .piLayer7enabled_V_ap_vld (1),
    .piTime_seconds_V         (sTIME_Broadcast_seconds),
    .piTime_seconds_V_ap_vld  (1),
    .piTime_minutes_V         (sTIME_Broadcast_minutes),
    .piTime_minutes_V_ap_vld  (1),
    .piTime_hours_V           (sTIME_Broadcast_hours),
    .piTime_hours_V_ap_vld    (1),
    .piRole_mmio_V            (piDCP_ROL_Mmio_RdReg),
    .piRole_mmio_V_ap_vld     (1),
    .m_axi_boHWICAP_AWADDR   (ssFMC_HWICAP_Axi_awaddr),
    .m_axi_boHWICAP_AWVALID  (ssFMC_HWICAP_Axi_awvalid),
    .m_axi_boHWICAP_AWREADY  (ssFMC_HWICAP_Axi_awready),
    .m_axi_boHWICAP_WDATA    (ssFMC_HWICAP_Axi_wdata),
    .m_axi_boHWICAP_WSTRB    (ssFMC_HWICAP_Axi_wstrb),
    .m_axi_boHWICAP_WVALID   (ssFMC_HWICAP_Axi_wvalid),
    .m_axi_boHWICAP_WREADY   (ssFMC_HWICAP_Axi_wready),
    .m_axi_boHWICAP_BRESP    (ssFMC_HWICAP_Axi_bresp),
    .m_axi_boHWICAP_BVALID   (ssFMC_HWICAP_Axi_bvalid),
    .m_axi_boHWICAP_BREADY   (ssFMC_HWICAP_Axi_bready),
    .m_axi_boHWICAP_ARADDR   (ssFMC_HWICAP_Axi_araddr),
    .m_axi_boHWICAP_ARVALID  (ssFMC_HWICAP_Axi_arvalid), 
    .m_axi_boHWICAP_ARREADY  (ssFMC_HWICAP_Axi_arready),
    .m_axi_boHWICAP_RDATA    (ssFMC_HWICAP_Axi_rdata),
    .m_axi_boHWICAP_RLAST    (1), //TODO: valid?
    .m_axi_boHWICAP_RRESP    (ssFMC_HWICAP_Axi_rresp),
    .m_axi_boHWICAP_RVALID   (ssFMC_HWICAP_Axi_rvalid),
    .m_axi_boHWICAP_RREADY   (ssFMC_HWICAP_Axi_rready),
    .piDECOUP_status_V                   (sDCP_FMC_status),
    .poDECOUP_activate_V                 (sFMC_DCP_activate),
    .poSoftReset_V                       (sFMC_ROLE_soft_reset),
    .xmem_V_Address0                     (sbFMC_MMIO_Xmem_Addr),
    .xmem_V_ce0                          (sbFMC_MMIO_Xmem_cen), 
    .xmem_V_we0                          (sbFMC_MMIO_Xmem_wren),
    .xmem_V_d0                           (sbFMC_MMIO_Xmem_WData),
    .xmem_V_q0                           (sbFMC_MMIO_Xmem_RData),
    .m_axi_boNRC_ctrlLink_AWVALID        (ssFMC_NRC_ctrlLink_Axi_AWVALID),
    .m_axi_boNRC_ctrlLink_AWREADY        (ssFMC_NRC_ctrlLink_Axi_AWREADY),
    .m_axi_boNRC_ctrlLink_AWADDR         (ssFMC_NRC_ctrlLink_Axi_AWADDR),
    .m_axi_boNRC_ctrlLink_WVALID         (ssFMC_NRC_ctrlLink_Axi_WVALID),
    .m_axi_boNRC_ctrlLink_WREADY         (ssFMC_NRC_ctrlLink_Axi_WREADY),
    .m_axi_boNRC_ctrlLink_WDATA          (ssFMC_NRC_ctrlLink_Axi_WDATA),
    .m_axi_boNRC_ctrlLink_WSTRB          (ssFMC_NRC_ctrlLink_Axi_WSTRB),
    .m_axi_boNRC_ctrlLink_ARVALID        (ssFMC_NRC_ctrlLink_Axi_ARVALID),
    .m_axi_boNRC_ctrlLink_ARREADY        (ssFMC_NRC_ctrlLink_Axi_ARREADY),
    .m_axi_boNRC_ctrlLink_ARADDR         (ssFMC_NRC_ctrlLink_Axi_ARADDR),
    .m_axi_boNRC_ctrlLink_RVALID         (ssFMC_NRC_ctrlLink_Axi_RVALID),
    .m_axi_boNRC_ctrlLink_RREADY         (ssFMC_NRC_ctrlLink_Axi_RREADY),
    .m_axi_boNRC_ctrlLink_RDATA          (ssFMC_NRC_ctrlLink_Axi_RDATA),
    .m_axi_boNRC_ctrlLink_RLAST          (1), //TODO: valid?
    .m_axi_boNRC_ctrlLink_RRESP          (ssFMC_NRC_ctrlLink_Axi_RRESP),
    .m_axi_boNRC_ctrlLink_BVALID         (ssFMC_NRC_ctrlLink_Axi_BVALID),
    .m_axi_boNRC_ctrlLink_BREADY         (ssFMC_NRC_ctrlLink_Axi_BREADY),
    .m_axi_boNRC_ctrlLink_BRESP          (ssFMC_NRC_ctrlLink_Axi_BRESP),
    .piDisableCtrlLink_V                 (0),
    .siNRC_Tcp_data_V_tdata_V_dout       ( ssFifo_FMC_Tcp_Data_tdata_V_dout)     ,
    .siNRC_Tcp_data_V_tdata_V_empty_n    (~ssFifo_FMC_Tcp_Data_tdata_V_empty)  ,
    .siNRC_Tcp_data_V_tdata_V_read       ( ssFifo_FMC_Tcp_Data_tdata_V_read)   ,
    .siNRC_Tcp_data_V_tkeep_V_dout       ( ssFifo_FMC_Tcp_Data_tkeep_V_dout)     ,
    .siNRC_Tcp_data_V_tkeep_V_empty_n    (~ssFifo_FMC_Tcp_Data_tkeep_V_empty)  ,
    .siNRC_Tcp_data_V_tkeep_V_read       ( ssFifo_FMC_Tcp_Data_tkeep_V_read)   ,
    .siNRC_Tcp_data_V_tlast_V_dout       ( ssFifo_FMC_Tcp_Data_tlast_V_dout)     ,
    .siNRC_Tcp_data_V_tlast_V_empty_n    (~ssFifo_FMC_Tcp_Data_tlast_V_empty)  ,
    .siNRC_Tcp_data_V_tlast_V_read       ( ssFifo_FMC_Tcp_Data_tlast_V_read)   ,
    .siNRC_Tcp_SessId_V_V_dout     ( ssFifo_FMC_Tcp_SessId_tdata_V_dout)   ,
    .siNRC_Tcp_SessId_V_V_empty_n  (~ssFifo_FMC_Tcp_SessId_tdata_V_empty),
    .siNRC_Tcp_SessId_V_V_read     ( ssFifo_FMC_Tcp_SessId_tdata_V_read) ,
    //.siNRC_Tcp_SessId_V_tdata_V_dout     ( ssFifo_FMC_Tcp_SessId_tdata_V_dout)   ,
    //.siNRC_Tcp_SessId_V_tdata_V_empty_n  (~ssFifo_FMC_Tcp_SessId_tdata_V_empty),
    //.siNRC_Tcp_SessId_V_tdata_V_read     ( ssFifo_FMC_Tcp_SessId_tdata_V_read) ,
    //.siNRC_Tcp_SessId_V_tkeep_V_dout     ( ssFifo_FMC_Tcp_SessId_tkeep_V_dout)   ,
    //.siNRC_Tcp_SessId_V_tkeep_V_empty_n  (~ssFifo_FMC_Tcp_SessId_tkeep_V_empty),
    //.siNRC_Tcp_SessId_V_tkeep_V_read     ( ssFifo_FMC_Tcp_SessId_tkeep_V_read) ,
    //.siNRC_Tcp_SessId_V_tlast_V_dout     ( ssFifo_FMC_Tcp_SessId_tlast_V_dout)   ,
    //.siNRC_Tcp_SessId_V_tlast_V_empty_n  (~ssFifo_FMC_Tcp_SessId_tlast_V_empty),
    //.siNRC_Tcp_SessId_V_tlast_V_read     ( ssFifo_FMC_Tcp_SessId_tlast_V_read) ,
    .soNRC_Tcp_data_V_tdata_V_din       ( ssFMC_Fifo_Tcp_Data_tdata_V_din)     ,
    .soNRC_Tcp_data_V_tdata_V_full_n    (~ssFMC_Fifo_Tcp_Data_tdata_V_full)  ,
    .soNRC_Tcp_data_V_tdata_V_write     ( ssFMC_Fifo_Tcp_Data_tdata_V_write)   ,
    .soNRC_Tcp_data_V_tkeep_V_din       ( ssFMC_Fifo_Tcp_Data_tkeep_V_din)     ,
    .soNRC_Tcp_data_V_tkeep_V_full_n    (~ssFMC_Fifo_Tcp_Data_tkeep_V_full)  ,
    .soNRC_Tcp_data_V_tkeep_V_write     ( ssFMC_Fifo_Tcp_Data_tkeep_V_write)   ,
    .soNRC_Tcp_data_V_tlast_V_din       ( ssFMC_Fifo_Tcp_Data_tlast_V_din)     ,
    .soNRC_Tcp_data_V_tlast_V_full_n    (~ssFMC_Fifo_Tcp_Data_tlast_V_full)  ,
    .soNRC_Tcp_data_V_tlast_V_write     ( ssFMC_Fifo_Tcp_Data_tlast_V_write)   ,
    .soNRC_Tcp_SessId_V_V_din     ( ssFMC_Fifo_Tcp_SessId_tdata_V_din)   ,
    .soNRC_Tcp_SessId_V_V_full_n  (~ssFMC_Fifo_Tcp_SessId_tdata_V_full),
    .soNRC_Tcp_SessId_V_V_write   ( ssFMC_Fifo_Tcp_SessId_tdata_V_write) ,
    //.soNRC_Tcp_SessId_V_tdata_V_din     ( ssFMC_Fifo_Tcp_SessId_tdata_V_din)   ,
    //.soNRC_Tcp_SessId_V_tdata_V_full_n  (~ssFMC_Fifo_Tcp_SessId_tdata_V_full),
    //.soNRC_Tcp_SessId_V_tdata_V_write   ( ssFMC_Fifo_Tcp_SessId_tdata_V_write) ,
    //.soNRC_Tcp_SessId_V_tkeep_V_din     ( ssFMC_Fifo_Tcp_SessId_tkeep_V_din)   ,
    //.soNRC_Tcp_SessId_V_tkeep_V_full_n  (~ssFMC_Fifo_Tcp_SessId_tkeep_V_full),
    //.soNRC_Tcp_SessId_V_tkeep_V_write   ( ssFMC_Fifo_Tcp_SessId_tkeep_V_write) ,
    //.soNRC_Tcp_SessId_V_tlast_V_din     ( ssFMC_Fifo_Tcp_SessId_tlast_V_din)   ,
    //.soNRC_Tcp_SessId_V_tlast_V_full_n  (~ssFMC_Fifo_Tcp_SessId_tlast_V_full),
    //.soNRC_Tcp_SessId_V_tlast_V_write   ( ssFMC_Fifo_Tcp_SessId_tlast_V_write) ,
    .poROLE_rank_V                       (poDCP_ROL_Fmc_Rank),
    .poROLE_size_V                       (poDCP_ROL_Fmc_Size)
  );

  FifoNetwork_Data FIFO_DD_0 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssFMC_Fifo_Tcp_Data_tdata_V_din    ),
    .full   (ssFMC_Fifo_Tcp_Data_tdata_V_full   ),
    .wr_en  (ssFMC_Fifo_Tcp_Data_tdata_V_write  ),
    .dout   (ssFifo_NRC_Tcp_Data_tdata_V_dout   ),
    .empty  (ssFifo_NRC_Tcp_Data_tdata_V_empty  ),
    .rd_en  (ssFifo_NRC_Tcp_Data_tdata_V_read   )
  );
  
  FifoNetwork_Keep FIFO_DK_0 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssFMC_Fifo_Tcp_Data_tkeep_V_din    ),
    .full   (ssFMC_Fifo_Tcp_Data_tkeep_V_full   ),
    .wr_en  (ssFMC_Fifo_Tcp_Data_tkeep_V_write  ),
    .dout   (ssFifo_NRC_Tcp_Data_tkeep_V_dout   ),
    .empty  (ssFifo_NRC_Tcp_Data_tkeep_V_empty  ),
    .rd_en  (ssFifo_NRC_Tcp_Data_tkeep_V_read   )
  );

  FifoNetwork_Last FIFO_DL_0 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssFMC_Fifo_Tcp_Data_tlast_V_din    ),
    .full   (ssFMC_Fifo_Tcp_Data_tlast_V_full   ),
    .wr_en  (ssFMC_Fifo_Tcp_Data_tlast_V_write  ),
    .dout   (ssFifo_NRC_Tcp_Data_tlast_V_dout   ),
    .empty  (ssFifo_NRC_Tcp_Data_tlast_V_empty  ),
    .rd_en  (ssFifo_NRC_Tcp_Data_tlast_V_read   )
  );

  FifoSession_Data FIFO_SD_0 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssFMC_Fifo_Tcp_SessId_tdata_V_din    ),
    .full   (ssFMC_Fifo_Tcp_SessId_tdata_V_full   ),
    .wr_en  (ssFMC_Fifo_Tcp_SessId_tdata_V_write  ),
    .dout   (ssFifo_NRC_Tcp_SessId_tdata_V_dout   ),
    .empty  (ssFifo_NRC_Tcp_SessId_tdata_V_empty  ),
    .rd_en  (ssFifo_NRC_Tcp_SessId_tdata_V_read   )
  );
  
  //FifoSession_Keep FIFO_SK_0 (
  //  .clk    (sETH0_ShlClk),
  //  .srst   (sMMIO_LayerRst[6]),
  //  .din    (ssFMC_Fifo_Tcp_SessId_tkeep_V_din    ),
  //  .full   (ssFMC_Fifo_Tcp_SessId_tkeep_V_full   ),
  //  .wr_en  (ssFMC_Fifo_Tcp_SessId_tkeep_V_write  ),
  //  .dout   (ssFifo_NRC_Tcp_SessId_tkeep_V_dout   ),
  //  .empty  (ssFifo_NRC_Tcp_SessId_tkeep_V_empty  ),
  //  .rd_en  (ssFifo_NRC_Tcp_SessId_tkeep_V_read   )
  //);

  //FifoSession_Last FIFO_SL_0 (
  //  .clk    (sETH0_ShlClk),
  //  .srst   (sMMIO_LayerRst[6]),
  //  .din    (ssFMC_Fifo_Tcp_SessId_tlast_V_din    ),
  //  .full   (ssFMC_Fifo_Tcp_SessId_tlast_V_full   ),
  //  .wr_en  (ssFMC_Fifo_Tcp_SessId_tlast_V_write  ),
  //  .dout   (ssFifo_NRC_Tcp_SessId_tlast_V_dout   ),
  //  .empty  (ssFifo_NRC_Tcp_SessId_tlast_V_empty  ),
  //  .rd_en  (ssFifo_NRC_Tcp_SessId_tlast_V_read   )
  //);


  FifoNetwork_Data FIFO_DD_1 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssNRC_Fifo_Tcp_Data_tdata_V_din    ),
    .full   (ssNRC_Fifo_Tcp_Data_tdata_V_full   ),
    .wr_en  (ssNRC_Fifo_Tcp_Data_tdata_V_write  ),
    .dout   (ssFifo_FMC_Tcp_Data_tdata_V_dout   ),
    .empty  (ssFifo_FMC_Tcp_Data_tdata_V_empty  ),
    .rd_en  (ssFifo_FMC_Tcp_Data_tdata_V_read   )
  );
  
  FifoNetwork_Keep FIFO_DK_1 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssNRC_Fifo_Tcp_Data_tkeep_V_din    ),
    .full   (ssNRC_Fifo_Tcp_Data_tkeep_V_full   ),
    .wr_en  (ssNRC_Fifo_Tcp_Data_tkeep_V_write  ),
    .dout   (ssFifo_FMC_Tcp_Data_tkeep_V_dout   ),
    .empty  (ssFifo_FMC_Tcp_Data_tkeep_V_empty  ),
    .rd_en  (ssFifo_FMC_Tcp_Data_tkeep_V_read   )
  );

  FifoNetwork_Last FIFO_DL_1 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssNRC_Fifo_Tcp_Data_tlast_V_din    ),
    .full   (ssNRC_Fifo_Tcp_Data_tlast_V_full   ),
    .wr_en  (ssNRC_Fifo_Tcp_Data_tlast_V_write  ),
    .dout   (ssFifo_FMC_Tcp_Data_tlast_V_dout   ),
    .empty  (ssFifo_FMC_Tcp_Data_tlast_V_empty  ),
    .rd_en  (ssFifo_FMC_Tcp_Data_tlast_V_read   )
  );


  FifoSession_Data FIFO_SD_1 (
    .clk    (sETH0_ShlClk),
    .srst   (sMMIO_LayerRst[6]),
    .din    (ssNRC_Fifo_Tcp_SessId_tdata_V_din    ),
    .full   (ssNRC_Fifo_Tcp_SessId_tdata_V_full   ),
    .wr_en  (ssNRC_Fifo_Tcp_SessId_tdata_V_write  ),
    .dout   (ssFifo_FMC_Tcp_SessId_tdata_V_dout   ),
    .empty  (ssFifo_FMC_Tcp_SessId_tdata_V_empty  ),
    .rd_en  (ssFifo_FMC_Tcp_SessId_tdata_V_read   )
  );
  
  //FifoSession_Keep FIFO_SK_1 (
  //  .clk    (sETH0_ShlClk),
  //  .srst   (sMMIO_LayerRst[6]),
  //  .din    (ssNRC_Fifo_Tcp_SessId_tkeep_V_din    ),
  //  .full   (ssNRC_Fifo_Tcp_SessId_tkeep_V_full   ),
  //  .wr_en  (ssNRC_Fifo_Tcp_SessId_tkeep_V_write  ),
  //  .dout   (ssFifo_FMC_Tcp_SessId_tkeep_V_dout   ),
  //  .empty  (ssFifo_FMC_Tcp_SessId_tkeep_V_empty  ),
  //  .rd_en  (ssFifo_FMC_Tcp_SessId_tkeep_V_read   )
  //);

  //FifoSession_Last FIFO_SL_1 (
  //  .clk    (sETH0_ShlClk),
  //  .srst   (sMMIO_LayerRst[6]),
  //  .din    (ssNRC_Fifo_Tcp_SessId_tlast_V_din    ),
  //  .full   (ssNRC_Fifo_Tcp_SessId_tlast_V_full   ),
  //  .wr_en  (ssNRC_Fifo_Tcp_SessId_tlast_V_write  ),
  //  .dout   (ssFifo_FMC_Tcp_SessId_tlast_V_dout   ),
  //  .empty  (ssFifo_FMC_Tcp_SessId_tlast_V_empty  ),
  //  .rd_en  (ssFifo_FMC_Tcp_SessId_tlast_V_read   )
  //);


  NetworkRoutingCore NRC (
    //-- Global Clock used by the entire SHELL -------------
    .ap_clk                 (sETH0_ShlClk),
    //-- Global Reset used by the entire SHELL -------------
    //.ap_rst_n               (~ piTOP_156_25Rst),
    .ap_rst_n               (~ sMMIO_LayerRst[6]),
    .piLayer4enabled_V        (sMMIO_LayerEn[4] & (~ sMMIO_LayerRst[4])),
    .piLayer4enabled_V_ap_vld (1),
    .piLayer7enabled_V        (sMMIO_LayerEn[7] & (~ sMMIO_LayerRst[7])),
    .piLayer7enabled_V_ap_vld (1),
    .piRoleDecoup_active_V         (sFMC_DCP_activate),
    .piRoleDecoup_active_V_ap_vld  (1),
    .piNTS_ready_V          (sNTS0_MMIO_NtsReady),
    .piNTS_ready_V_ap_vld   (1),
    .piMMIO_FmcLsnPort_V    (sMMIO_NRC_FmcLsnPort),
    .piMMIO_FmcLsnPort_V_ap_vld (1),
    .piMMIO_CfrmIp4Addr_V   (sMMIO_NRC_CfrmIp4Addr),
    .piMMIO_CfrmIp4Addr_V_ap_vld (1),
    .piMyIpAddress_V          (sMMIO_NTS0_Ip4Address),
    .piMyIpAddress_V_ap_vld   (1),
    //.piROL_NRC_Udp_Rx_ports_V (sDECOUP_Nrc_Udp_Rx_ports),
    .piROL_Udp_Rx_ports_V (piDCP_ROL_Nrc_Udp_Rx_ports),
    .piROL_Udp_Rx_ports_V_ap_vld (1),
    .siUdp_data_TDATA         (slcInUdp_data_TDATA ) ,
    .siUdp_data_TVALID        (slcInUdp_data_TVALID) ,
    .siUdp_data_TREADY        (slcInUdp_data_TREADY) ,
    .siUdp_data_TKEEP         (slcInUdp_data_TKEEP ) ,
    .siUdp_data_TLAST         (slcInUdp_data_TLAST ) ,
    .soUdp_data_TDATA         (slcOutUdp_data_TDATA ) ,
    .soUdp_data_TVALID        (slcOutUdp_data_TVALID) ,
    .soUdp_data_TREADY        (slcOutUdp_data_TREADY) ,
    .soUdp_data_TKEEP         (slcOutUdp_data_TKEEP ) ,
    .soUdp_data_TLAST         (slcOutUdp_data_TLAST ) ,
    .siUdp_meta_TDATA         (slcInNrc_Udp_meta_TDATA ) ,
    .siUdp_meta_TVALID        (slcInNrc_Udp_meta_TVALID) ,
    .siUdp_meta_TREADY        (slcInNrc_Udp_meta_TREADY) ,
    .siUdp_meta_TKEEP         (slcInNrc_Udp_meta_TKEEP ) ,
    .siUdp_meta_TLAST         (slcInNrc_Udp_meta_TLAST ) ,
    .soUdp_meta_TDATA         (slcOutNrc_Udp_meta_TDATA ) ,
    .soUdp_meta_TVALID        (slcOutNrc_Udp_meta_TVALID) ,
    .soUdp_meta_TREADY        (slcOutNrc_Udp_meta_TREADY) ,
    .soUdp_meta_TKEEP         (slcOutNrc_Udp_meta_TKEEP ) ,
    .soUdp_meta_TLAST         (slcOutNrc_Udp_meta_TLAST ) ,
    .piROL_Tcp_Rx_ports_V (piDCP_ROL_Nrc_Tcp_Rx_ports),
    .piROL_Tcp_Rx_ports_V_ap_vld (1),
    .siTcp_data_TDATA         (slcInTcp_data_TDATA ) ,
    .siTcp_data_TVALID        (slcInTcp_data_TVALID) ,
    .siTcp_data_TREADY        (slcInTcp_data_TREADY) ,
    .siTcp_data_TKEEP         (slcInTcp_data_TKEEP ) ,
    .siTcp_data_TLAST         (slcInTcp_data_TLAST ) ,
    .soTcp_data_TDATA         (slcOutTcp_data_TDATA ) ,
    .soTcp_data_TVALID        (slcOutTcp_data_TVALID) ,
    .soTcp_data_TREADY        (slcOutTcp_data_TREADY) ,
    .soTcp_data_TKEEP         (slcOutTcp_data_TKEEP ) ,
    .soTcp_data_TLAST         (slcOutTcp_data_TLAST ) ,
    .siTcp_meta_TDATA         (slcInNrc_Tcp_meta_TDATA ) ,
    .siTcp_meta_TVALID        (slcInNrc_Tcp_meta_TVALID) ,
    .siTcp_meta_TREADY        (slcInNrc_Tcp_meta_TREADY) ,
    .siTcp_meta_TKEEP         (slcInNrc_Tcp_meta_TKEEP ) ,
    .siTcp_meta_TLAST         (slcInNrc_Tcp_meta_TLAST ) ,
    .soTcp_meta_TDATA         (slcOutNrc_Tcp_meta_TDATA ) ,
    .soTcp_meta_TVALID        (slcOutNrc_Tcp_meta_TVALID) ,
    .soTcp_meta_TREADY        (slcOutNrc_Tcp_meta_TREADY) ,
    .soTcp_meta_TKEEP         (slcOutNrc_Tcp_meta_TKEEP ) ,
    .soTcp_meta_TLAST         (slcOutNrc_Tcp_meta_TLAST ) ,
    .siFMC_Tcp_data_V_tdata_V_dout       ( ssFifo_NRC_Tcp_Data_tdata_V_dout)     ,
    .siFMC_Tcp_data_V_tdata_V_empty_n    (~ssFifo_NRC_Tcp_Data_tdata_V_empty)  ,
    .siFMC_Tcp_data_V_tdata_V_read       ( ssFifo_NRC_Tcp_Data_tdata_V_read)   ,
    .siFMC_Tcp_data_V_tkeep_V_dout       ( ssFifo_NRC_Tcp_Data_tkeep_V_dout)     ,
    .siFMC_Tcp_data_V_tkeep_V_empty_n    (~ssFifo_NRC_Tcp_Data_tkeep_V_empty)  ,
    .siFMC_Tcp_data_V_tkeep_V_read       ( ssFifo_NRC_Tcp_Data_tkeep_V_read)   ,
    .siFMC_Tcp_data_V_tlast_V_dout       ( ssFifo_NRC_Tcp_Data_tlast_V_dout)     ,
    .siFMC_Tcp_data_V_tlast_V_empty_n    (~ssFifo_NRC_Tcp_Data_tlast_V_empty)  ,
    .siFMC_Tcp_data_V_tlast_V_read       ( ssFifo_NRC_Tcp_Data_tlast_V_read)   ,
    .siFMC_Tcp_SessId_V_V_dout     ( ssFifo_NRC_Tcp_SessId_tdata_V_dout)   ,
    .siFMC_Tcp_SessId_V_V_empty_n  (~ssFifo_NRC_Tcp_SessId_tdata_V_empty),
    .siFMC_Tcp_SessId_V_V_read     ( ssFifo_NRC_Tcp_SessId_tdata_V_read) ,
    //.siFMC_Tcp_SessId_V_tdata_V_dout     ( ssFifo_NRC_Tcp_SessId_tdata_V_dout)   ,
    //.siFMC_Tcp_SessId_V_tdata_V_empty_n  (~ssFifo_NRC_Tcp_SessId_tdata_V_empty),
    //.siFMC_Tcp_SessId_V_tdata_V_read     ( ssFifo_NRC_Tcp_SessId_tdata_V_read) ,
    //.siFMC_Tcp_SessId_V_tkeep_V_dout     ( ssFifo_NRC_Tcp_SessId_tkeep_V_dout)   ,
    //.siFMC_Tcp_SessId_V_tkeep_V_empty_n  (~ssFifo_NRC_Tcp_SessId_tkeep_V_empty),
    //.siFMC_Tcp_SessId_V_tkeep_V_read     ( ssFifo_NRC_Tcp_SessId_tkeep_V_read) ,
    //.siFMC_Tcp_SessId_V_tlast_V_dout     ( ssFifo_NRC_Tcp_SessId_tlast_V_dout)   ,
    //.siFMC_Tcp_SessId_V_tlast_V_empty_n  (~ssFifo_NRC_Tcp_SessId_tlast_V_empty),
    //.siFMC_Tcp_SessId_V_tlast_V_read     ( ssFifo_NRC_Tcp_SessId_tlast_V_read) ,
    .soFMC_Tcp_data_V_tdata_V_din       ( ssNRC_Fifo_Tcp_Data_tdata_V_din)     ,
    .soFMC_Tcp_data_V_tdata_V_full_n    (~ssNRC_Fifo_Tcp_Data_tdata_V_full)  ,
    .soFMC_Tcp_data_V_tdata_V_write     ( ssNRC_Fifo_Tcp_Data_tdata_V_write)   ,
    .soFMC_Tcp_data_V_tkeep_V_din       ( ssNRC_Fifo_Tcp_Data_tkeep_V_din)     ,
    .soFMC_Tcp_data_V_tkeep_V_full_n    (~ssNRC_Fifo_Tcp_Data_tkeep_V_full)  ,
    .soFMC_Tcp_data_V_tkeep_V_write     ( ssNRC_Fifo_Tcp_Data_tkeep_V_write)   ,
    .soFMC_Tcp_data_V_tlast_V_din       ( ssNRC_Fifo_Tcp_Data_tlast_V_din)     ,
    .soFMC_Tcp_data_V_tlast_V_full_n    (~ssNRC_Fifo_Tcp_Data_tlast_V_full)  ,
    .soFMC_Tcp_data_V_tlast_V_write     ( ssNRC_Fifo_Tcp_Data_tlast_V_write)   ,
    .soFMC_Tcp_SessId_V_V_din     ( ssNRC_Fifo_Tcp_SessId_tdata_V_din)   ,
    .soFMC_Tcp_SessId_V_V_full_n  (~ssNRC_Fifo_Tcp_SessId_tdata_V_full),
    .soFMC_Tcp_SessId_V_V_write   ( ssNRC_Fifo_Tcp_SessId_tdata_V_write) ,
    //.soFMC_Tcp_SessId_V_tdata_V_din     ( ssNRC_Fifo_Tcp_SessId_tdata_V_din)   ,
    //.soFMC_Tcp_SessId_V_tdata_V_full_n  (~ssNRC_Fifo_Tcp_SessId_tdata_V_full),
    //.soFMC_Tcp_SessId_V_tdata_V_write   ( ssNRC_Fifo_Tcp_SessId_tdata_V_write) ,
    //.soFMC_Tcp_SessId_V_tkeep_V_din     ( ssNRC_Fifo_Tcp_SessId_tkeep_V_din)   ,
    //.soFMC_Tcp_SessId_V_tkeep_V_full_n  (~ssNRC_Fifo_Tcp_SessId_tkeep_V_full),
    //.soFMC_Tcp_SessId_V_tkeep_V_write   ( ssNRC_Fifo_Tcp_SessId_tkeep_V_write) ,
    //.soFMC_Tcp_SessId_V_tlast_V_din     ( ssNRC_Fifo_Tcp_SessId_tlast_V_din)   ,
    //.soFMC_Tcp_SessId_V_tlast_V_full_n  (~ssNRC_Fifo_Tcp_SessId_tlast_V_full),
    //.soFMC_Tcp_SessId_V_tlast_V_write   ( ssNRC_Fifo_Tcp_SessId_tlast_V_write) ,
    .soUOE_Udp_Data_TDATA             (ssNRC_NTS0_Udp_Data_tdata),
    .soUOE_Udp_Data_TKEEP             (ssNRC_NTS0_Udp_Data_tkeep),
    .soUOE_Udp_Data_TLAST             (ssNRC_NTS0_Udp_Data_tlast),
    .soUOE_Udp_Data_TVALID            (ssNRC_NTS0_Udp_Data_tvalid),
    .soUOE_Udp_Data_TREADY            (ssNRC_NTS0_Udp_Data_tready),
    .soUOE_Udp_Meta_V_TDATA           (ssNRC_NTS0_Udp_Meta_tdata),
    .soUOE_Udp_Meta_V_TVALID          (ssNRC_NTS0_Udp_Meta_tvalid),
    .soUOE_Udp_Meta_V_TREADY          (ssNRC_NTS0_Udp_Meta_tready),
    .soUOE_Udp_DLen_V_V_TDATA         (ssNRC_NTS0_Udp_DLen_tdata),
    .soUOE_Udp_DLen_V_V_TVALID        (ssNRC_NTS0_Udp_DLen_tvalid),
    .soUOE_Udp_DLen_V_V_TREADY        (ssNRC_NTS0_Udp_DLen_tready),
    .siUOE_Udp_Data_TDATA             (ssNTS0_NRC_Udp_Data_tdata),
    .siUOE_Udp_Data_TKEEP             (ssNTS0_NRC_Udp_Data_tkeep),
    .siUOE_Udp_Data_TLAST             (ssNTS0_NRC_Udp_Data_tlast),
    .siUOE_Udp_Data_TVALID            (ssNTS0_NRC_Udp_Data_tvalid),
    .siUOE_Udp_Data_TREADY            (ssNTS0_NRC_Udp_Data_tready),
    .siUOE_Udp_Meta_V_TDATA           (ssNTS0_NRC_Udp_Meta_tdata),
    .siUOE_Udp_Meta_V_TVALID          (ssNTS0_NRC_Udp_Meta_tvalid),
    .siUOE_Udp_Meta_V_TREADY          (ssNTS0_NRC_Udp_Meta_tready),
    .soUOE_Udp_LsnReq_V_V_TDATA       (ssNRC_NTS0_Udp_LsnReq_tdata),
    .soUOE_Udp_LsnReq_V_V_TVALID      (ssNRC_NTS0_Udp_LsnReq_tvalid),
    .soUOE_Udp_LsnReq_V_V_TREADY      (ssNRC_NTS0_Udp_LsnReq_tready),
    .siUOE_Udp_LsnRep_V_TDATA         (ssNTS0_NRC_Udp_LsnRep_tdata),
    .siUOE_Udp_LsnRep_V_TVALID        (ssNTS0_NRC_Udp_LsnRep_tvalid),
    .siUOE_Udp_LsnRep_V_TREADY        (ssNTS0_NRC_Udp_LsnRep_tready),
    .soUOE_Udp_ClsReq_V_V_TDATA       (ssNRC_NTS0_Udp_ClsReq_tdata),
    .soUOE_Udp_ClsReq_V_V_TVALID      (ssNRC_NTS0_Udp_ClsReq_tvalid),
    .soUOE_Udp_ClsReq_V_V_TREADY      (ssNRC_NTS0_Udp_ClsReq_tready),
    .siUOE_Udp_ClsRep_V_TDATA         (ssNTS0_NRC_Udp_ClsRep_tdata),
    .siUOE_Udp_ClsRep_V_TVALID        (ssNTS0_NRC_Udp_ClsRep_tvalid),
    .siUOE_Udp_ClsRep_V_TREADY        (ssNTS0_NRC_Udp_ClsRep_tready),
    .siTOE_Notif_V_TDATA            (ssTOE_NRC_Tcp_Notif_tdata  ),
    .siTOE_Notif_V_TVALID           (ssTOE_NRC_Tcp_Notif_tvalid ),
    .siTOE_Notif_V_TREADY           (ssTOE_NRC_Tcp_Notif_tready ),
    .soTOE_DReq_V_TDATA             (ssNRC_TOE_Tcp_DReq_tdata),
    .soTOE_DReq_V_TVALID            (ssNRC_TOE_Tcp_DReq_tvalid),
    .soTOE_DReq_V_TREADY            (ssNRC_TOE_Tcp_DReq_tready),
    .siTOE_Data_TDATA               (ssTOE_NRC_Tcp_Data_tdata),
    .siTOE_Data_TVALID              (ssTOE_NRC_Tcp_Data_tvalid),
    .siTOE_Data_TREADY              (ssTOE_NRC_Tcp_Data_tready),
    .siTOE_Data_TKEEP               (ssTOE_NRC_Tcp_Data_tkeep),
    .siTOE_Data_TLAST               (ssTOE_NRC_Tcp_Data_tlast),
    .siTOE_SessId_V_V_TDATA         (ssTOE_NRC_Tcp_Meta_tdata),
    .siTOE_SessId_V_V_TVALID        (ssTOE_NRC_Tcp_Meta_tvalid),
    .siTOE_SessId_V_V_TREADY        (ssTOE_NRC_Tcp_Meta_tready),
    .soTOE_LsnReq_V_V_TDATA         (ssNRC_TOE_Tcp_LsnReq_tdata),
    .soTOE_LsnReq_V_V_TVALID        (ssNRC_TOE_Tcp_LsnReq_tvalid),
    .soTOE_LsnReq_V_V_TREADY        (ssNRC_TOE_Tcp_LsnReq_tready),
    .siTOE_LsnAck_V_TDATA           (ssTOE_NRC_Tcp_LsnAck_tdata),
    .siTOE_LsnAck_V_TVALID          (ssTOE_NRC_Tcp_LsnAck_tvalid),
    .siTOE_LsnAck_V_TREADY          (ssTOE_NRC_Tcp_LsnAck_tready),
    .soTOE_Data_TDATA               (ssNRC_TOE_Tcp_Data_tdata),
    .soTOE_Data_TVALID              (ssNRC_TOE_Tcp_Data_tvalid),
    .soTOE_Data_TREADY              (ssNRC_TOE_Tcp_Data_tready),
    .soTOE_Data_TKEEP               (ssNRC_TOE_Tcp_Data_tkeep),
    .soTOE_Data_TLAST               (ssNRC_TOE_Tcp_Data_tlast),
    .soTOE_SessId_V_V_TDATA         (ssNRC_TOE_Tcp_Meta_tdata),
    .soTOE_SessId_V_V_TVALID        (ssNRC_TOE_Tcp_Meta_tvalid),
    .soTOE_SessId_V_V_TREADY        (ssNRC_TOE_Tcp_Meta_tready),
    .siTOE_DSts_V_V_TDATA           (ssTOE_NRC_Tcp_DSts_tdata),
    .siTOE_DSts_V_V_TVALID          (ssTOE_NRC_Tcp_DSts_tvalid),
    .siTOE_DSts_V_V_TREADY          (ssTOE_NRC_Tcp_DSts_tready),
    .soTOE_OpnReq_V_TDATA           (ssNRC_TOE_Tcp_OpnReq_tdata),
    .soTOE_OpnReq_V_TVALID          (ssNRC_TOE_Tcp_OpnReq_tvalid),
    .soTOE_OpnReq_V_TREADY          (ssNRC_TOE_Tcp_OpnReq_tready),
    .siTOE_OpnRep_V_TDATA           (ssTOE_NRC_Tcp_OpnRep_tdata),
    .siTOE_OpnRep_V_TVALID          (ssTOE_NRC_Tcp_OpnRep_tvalid),
    .siTOE_OpnRep_V_TREADY          (ssTOE_NRC_Tcp_OpnRep_tready),
    .soTOE_ClsReq_V_V_TDATA         (ssNRC_TOE_Tcp_ClsReq_tdata),
    .soTOE_ClsReq_V_V_TVALID        (ssNRC_TOE_Tcp_ClsReq_tvalid),
    .soTOE_ClsReq_V_V_TREADY        (ssNRC_TOE_Tcp_ClsReq_tready),
    .s_axi_piFMC_NRC_ctrlLink_AXI_AWVALID   (ssFMC_NRC_ctrlLink_Axi_AWVALID),
    .s_axi_piFMC_NRC_ctrlLink_AXI_AWREADY   (ssFMC_NRC_ctrlLink_Axi_AWREADY),
    .s_axi_piFMC_NRC_ctrlLink_AXI_AWADDR    (ssFMC_NRC_ctrlLink_Axi_AWADDR),
    .s_axi_piFMC_NRC_ctrlLink_AXI_WVALID    (ssFMC_NRC_ctrlLink_Axi_WVALID),
    .s_axi_piFMC_NRC_ctrlLink_AXI_WREADY    (ssFMC_NRC_ctrlLink_Axi_WREADY),
    .s_axi_piFMC_NRC_ctrlLink_AXI_WDATA     (ssFMC_NRC_ctrlLink_Axi_WDATA),
    .s_axi_piFMC_NRC_ctrlLink_AXI_WSTRB     (ssFMC_NRC_ctrlLink_Axi_WSTRB),
    .s_axi_piFMC_NRC_ctrlLink_AXI_ARVALID   (ssFMC_NRC_ctrlLink_Axi_ARVALID),
    .s_axi_piFMC_NRC_ctrlLink_AXI_ARREADY   (ssFMC_NRC_ctrlLink_Axi_ARREADY),
    .s_axi_piFMC_NRC_ctrlLink_AXI_ARADDR    (ssFMC_NRC_ctrlLink_Axi_ARADDR),
    .s_axi_piFMC_NRC_ctrlLink_AXI_RVALID    (ssFMC_NRC_ctrlLink_Axi_RVALID),
    .s_axi_piFMC_NRC_ctrlLink_AXI_RREADY    (ssFMC_NRC_ctrlLink_Axi_RREADY),
    .s_axi_piFMC_NRC_ctrlLink_AXI_RDATA     (ssFMC_NRC_ctrlLink_Axi_RDATA),
    .s_axi_piFMC_NRC_ctrlLink_AXI_RRESP     (ssFMC_NRC_ctrlLink_Axi_RRESP),
    .s_axi_piFMC_NRC_ctrlLink_AXI_BVALID    (ssFMC_NRC_ctrlLink_Axi_BVALID),
    .s_axi_piFMC_NRC_ctrlLink_AXI_BREADY    (ssFMC_NRC_ctrlLink_Axi_BREADY),
    .s_axi_piFMC_NRC_ctrlLink_AXI_BRESP     (ssFMC_NRC_ctrlLink_Axi_BRESP)
);


  // == propagate constans as long as Decouling is not updated
  //assign sDECOUP_FMC_status = 0;

  // -- UDP AXIS Slices ---
  AxisRegisterSlice_64 SARS0 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (siDCP_ROL_Nts_Udp_Data_tdata),
    .s_axis_tvalid  (siDCP_ROL_Nts_Udp_Data_tvalid),
    .s_axis_tready  (siDCP_ROL_Nts_Udp_Data_tready),
    .s_axis_tkeep   (siDCP_ROL_Nts_Udp_Data_tkeep),
    .s_axis_tlast   (siDCP_ROL_Nts_Udp_Data_tlast),
    //-- To NRC
    .m_axis_tdata   (slcInUdp_data_TDATA ),
    .m_axis_tvalid  (slcInUdp_data_TVALID),
    .m_axis_tready  (slcInUdp_data_TREADY),
    .m_axis_tkeep   (slcInUdp_data_TKEEP ),
    .m_axis_tlast   (slcInUdp_data_TLAST ) 
  );

  AxisRegisterSlice_64 SARS1 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From NRC
    .s_axis_tdata   (slcOutUdp_data_TDATA ),
    .s_axis_tvalid  (slcOutUdp_data_TVALID),
    .s_axis_tready  (slcOutUdp_data_TREADY),
    .s_axis_tkeep   (slcOutUdp_data_TKEEP ),
    .s_axis_tlast   (slcOutUdp_data_TLAST ),
    //-- To ROLE
    .m_axis_tdata   (soDCP_ROL_Nts_Udp_Data_tdata),
    .m_axis_tvalid  (soDCP_ROL_Nts_Udp_Data_tvalid),
    .m_axis_tready  (soDCP_ROL_Nts_Udp_Data_tready),
    .m_axis_tkeep   (soDCP_ROL_Nts_Udp_Data_tkeep),
    .m_axis_tlast   (soDCP_ROL_Nts_Udp_Data_tlast)
  );
  
  AxisRegisterSlice_80 SARS2 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (siDCP_ROLE_Nrc_Udp_Meta_TDATA),
    .s_axis_tvalid  (siDCP_ROLE_Nrc_Udp_Meta_TVALID),
    .s_axis_tready  (siDCP_ROLE_Nrc_Udp_Meta_TREADY),
    .s_axis_tkeep   (siDCP_ROLE_Nrc_Udp_Meta_TKEEP),
    .s_axis_tlast   (siDCP_ROLE_Nrc_Udp_Meta_TLAST),
    //-- To NRC
    .m_axis_tdata   (slcInNrc_Udp_meta_TDATA ),
    .m_axis_tvalid  (slcInNrc_Udp_meta_TVALID),
    .m_axis_tready  (slcInNrc_Udp_meta_TREADY),
    .m_axis_tkeep   (slcInNrc_Udp_meta_TKEEP ),
    .m_axis_tlast   (slcInNrc_Udp_meta_TLAST )
  );
  
  AxisRegisterSlice_80 SARS3 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From NRC
    .s_axis_tdata   (slcOutNrc_Udp_meta_TDATA ) ,
    .s_axis_tvalid  (slcOutNrc_Udp_meta_TVALID) ,
    .s_axis_tready  (slcOutNrc_Udp_meta_TREADY) ,
    .s_axis_tkeep   (slcOutNrc_Udp_meta_TKEEP ) ,
    .s_axis_tlast   (slcOutNrc_Udp_meta_TLAST ) ,
    //-- To Role
    .m_axis_tdata   (soDCP_NRC_Role_Udp_Meta_TDATA),
    .m_axis_tvalid  (soDCP_NRC_Role_Udp_Meta_TVALID),
    .m_axis_tready  (soDCP_NRC_Role_Udp_Meta_TREADY),
    .m_axis_tkeep   (soDCP_NRC_Role_Udp_Meta_TKEEP),
    .m_axis_tlast   (soDCP_NRC_Role_Udp_Meta_TLAST)
  );
  
  // -- TCP AXIS Slices ---
  AxisRegisterSlice_64 SARS4 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (siDCP_ROL_Nts_Tcp_Data_tdata),
    .s_axis_tvalid  (siDCP_ROL_Nts_Tcp_Data_tvalid),
    .s_axis_tready  (siDCP_ROL_Nts_Tcp_Data_tready),
    .s_axis_tkeep   (siDCP_ROL_Nts_Tcp_Data_tkeep),
    .s_axis_tlast   (siDCP_ROL_Nts_Tcp_Data_tlast),
    //-- To NRC
    .m_axis_tdata   (slcInTcp_data_TDATA ),
    .m_axis_tvalid  (slcInTcp_data_TVALID),
    .m_axis_tready  (slcInTcp_data_TREADY),
    .m_axis_tkeep   (slcInTcp_data_TKEEP ),
    .m_axis_tlast   (slcInTcp_data_TLAST ) 
  );

  AxisRegisterSlice_64 SARS5 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From NRC
    .s_axis_tdata   (slcOutTcp_data_TDATA ),
    .s_axis_tvalid  (slcOutTcp_data_TVALID),
    .s_axis_tready  (slcOutTcp_data_TREADY),
    .s_axis_tkeep   (slcOutTcp_data_TKEEP ),
    .s_axis_tlast   (slcOutTcp_data_TLAST ),
    //-- To ROLE
    .m_axis_tdata   (soDCP_ROL_Nts_Tcp_Data_tdata),
    .m_axis_tvalid  (soDCP_ROL_Nts_Tcp_Data_tvalid),
    .m_axis_tready  (soDCP_ROL_Nts_Tcp_Data_tready),
    .m_axis_tkeep   (soDCP_ROL_Nts_Tcp_Data_tkeep),
    .m_axis_tlast   (soDCP_ROL_Nts_Tcp_Data_tlast)
  );
  
  AxisRegisterSlice_80 SARS6 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (siDCP_ROLE_Nrc_Tcp_Meta_TDATA),
    .s_axis_tvalid  (siDCP_ROLE_Nrc_Tcp_Meta_TVALID),
    .s_axis_tready  (siDCP_ROLE_Nrc_Tcp_Meta_TREADY),
    .s_axis_tkeep   (siDCP_ROLE_Nrc_Tcp_Meta_TKEEP),
    .s_axis_tlast   (siDCP_ROLE_Nrc_Tcp_Meta_TLAST),
    //-- To NRC
    .m_axis_tdata   (slcInNrc_Tcp_meta_TDATA ),
    .m_axis_tvalid  (slcInNrc_Tcp_meta_TVALID),
    .m_axis_tready  (slcInNrc_Tcp_meta_TREADY),
    .m_axis_tkeep   (slcInNrc_Tcp_meta_TKEEP ),
    .m_axis_tlast   (slcInNrc_Tcp_meta_TLAST )
  );
  
  AxisRegisterSlice_80 SARS7 (
    .aclk           (sETH0_ShlClk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From NRC
    .s_axis_tdata   (slcOutNrc_Tcp_meta_TDATA ) ,
    .s_axis_tvalid  (slcOutNrc_Tcp_meta_TVALID) ,
    .s_axis_tready  (slcOutNrc_Tcp_meta_TREADY) ,
    .s_axis_tkeep   (slcOutNrc_Tcp_meta_TKEEP ) ,
    .s_axis_tlast   (slcOutNrc_Tcp_meta_TLAST ) ,
    //-- To Role
    .m_axis_tdata   (soDCP_NRC_Role_Tcp_Meta_TDATA),
    .m_axis_tvalid  (soDCP_NRC_Role_Tcp_Meta_TVALID),
    .m_axis_tready  (soDCP_NRC_Role_Tcp_Meta_TREADY),
    .m_axis_tkeep   (soDCP_NRC_Role_Tcp_Meta_TKEEP),
    .m_axis_tlast   (soDCP_NRC_Role_Tcp_Meta_TLAST)
  );
 

 Decoupler DCP (
    .decouple                          (sFMC_DCP_activate),
    .decouple_status                   (sDCP_FMC_status),
    //reconfig partition side pins
    .rp_ROLE_siROL_Nts_Udp_Data_tdata       (siROL_Nts_Udp_Data_tdata   ),
    .rp_ROLE_siROL_Nts_Udp_Data_tkeep       (siROL_Nts_Udp_Data_tkeep   ),
    .rp_ROLE_siROL_Nts_Udp_Data_tlast       (siROL_Nts_Udp_Data_tlast   ),
    .rp_ROLE_siROL_Nts_Udp_Data_tvalid      (siROL_Nts_Udp_Data_tvalid  ),
    .rp_ROLE_siROL_Nts_Udp_Data_tready      (siROL_Nts_Udp_Data_tready  ),
    .rp_ROLE_soROL_Nts_Udp_Data_tdata       (soROL_Nts_Udp_Data_tdata   ),
    .rp_ROLE_soROL_Nts_Udp_Data_tkeep       (soROL_Nts_Udp_Data_tkeep   ),
    .rp_ROLE_soROL_Nts_Udp_Data_tlast       (soROL_Nts_Udp_Data_tlast   ),
    .rp_ROLE_soROL_Nts_Udp_Data_tvalid      (soROL_Nts_Udp_Data_tvalid  ),
    .rp_ROLE_soROL_Nts_Udp_Data_tready      (soROL_Nts_Udp_Data_tready  ),
    .rp_ROLE_piROL_Nrc_Udp_Rx_ports         (piROL_Nrc_Udp_Rx_ports     ),
    .rp_ROLE_siROLE_Nrc_Udp_Meta_TDATA      (siROLE_Nrc_Udp_Meta_TDATA  ),
    .rp_ROLE_siROLE_Nrc_Udp_Meta_TVALID     (siROLE_Nrc_Udp_Meta_TVALID ),
    .rp_ROLE_siROLE_Nrc_Udp_Meta_TREADY     (siROLE_Nrc_Udp_Meta_TREADY ),
    .rp_ROLE_siROLE_Nrc_Udp_Meta_TKEEP      (siROLE_Nrc_Udp_Meta_TKEEP  ),
    .rp_ROLE_siROLE_Nrc_Udp_Meta_TLAST      (siROLE_Nrc_Udp_Meta_TLAST  ),
    .rp_ROLE_soNRC_Role_Udp_Meta_TDATA      (soNRC_Role_Udp_Meta_TDATA  ),
    .rp_ROLE_soNRC_Role_Udp_Meta_TVALID     (soNRC_Role_Udp_Meta_TVALID ),
    .rp_ROLE_soNRC_Role_Udp_Meta_TREADY     (soNRC_Role_Udp_Meta_TREADY ),
    .rp_ROLE_soNRC_Role_Udp_Meta_TKEEP      (soNRC_Role_Udp_Meta_TKEEP  ),
    .rp_ROLE_soNRC_Role_Udp_Meta_TLAST      (soNRC_Role_Udp_Meta_TLAST  ),
    .rp_ROLE_siROL_Nts_Tcp_Data_tdata       (siROL_Nts_Tcp_Data_tdata   ),
    .rp_ROLE_siROL_Nts_Tcp_Data_tkeep       (siROL_Nts_Tcp_Data_tkeep   ),
    .rp_ROLE_siROL_Nts_Tcp_Data_tlast       (siROL_Nts_Tcp_Data_tlast   ),
    .rp_ROLE_siROL_Nts_Tcp_Data_tvalid      (siROL_Nts_Tcp_Data_tvalid  ),
    .rp_ROLE_siROL_Nts_Tcp_Data_tready      (siROL_Nts_Tcp_Data_tready  ),
    .rp_ROLE_soROL_Nts_Tcp_Data_tdata       (soROL_Nts_Tcp_Data_tdata   ),
    .rp_ROLE_soROL_Nts_Tcp_Data_tkeep       (soROL_Nts_Tcp_Data_tkeep   ),
    .rp_ROLE_soROL_Nts_Tcp_Data_tlast       (soROL_Nts_Tcp_Data_tlast   ),
    .rp_ROLE_soROL_Nts_Tcp_Data_tvalid      (soROL_Nts_Tcp_Data_tvalid  ),
    .rp_ROLE_soROL_Nts_Tcp_Data_tready      (soROL_Nts_Tcp_Data_tready  ),
    .rp_ROLE_piROL_Nrc_Tcp_Rx_ports         (piROL_Nrc_Tcp_Rx_ports     ),
    .rp_ROLE_siROLE_Nrc_Tcp_Meta_TDATA      (siROLE_Nrc_Tcp_Meta_TDATA  ),
    .rp_ROLE_siROLE_Nrc_Tcp_Meta_TVALID     (siROLE_Nrc_Tcp_Meta_TVALID ),
    .rp_ROLE_siROLE_Nrc_Tcp_Meta_TREADY     (siROLE_Nrc_Tcp_Meta_TREADY ),
    .rp_ROLE_siROLE_Nrc_Tcp_Meta_TKEEP      (siROLE_Nrc_Tcp_Meta_TKEEP  ),
    .rp_ROLE_siROLE_Nrc_Tcp_Meta_TLAST      (siROLE_Nrc_Tcp_Meta_TLAST  ),
    .rp_ROLE_soNRC_Role_Tcp_Meta_TDATA      (soNRC_Role_Tcp_Meta_TDATA  ),
    .rp_ROLE_soNRC_Role_Tcp_Meta_TVALID     (soNRC_Role_Tcp_Meta_TVALID ),
    .rp_ROLE_soNRC_Role_Tcp_Meta_TREADY     (soNRC_Role_Tcp_Meta_TREADY ),
    .rp_ROLE_soNRC_Role_Tcp_Meta_TKEEP      (soNRC_Role_Tcp_Meta_TKEEP  ),
    .rp_ROLE_soNRC_Role_Tcp_Meta_TLAST      (soNRC_Role_Tcp_Meta_TLAST  ),
    .rp_ROLE_siROL_Mem_Mp0_RdCmd_tdata      (siROL_Mem_Mp0_RdCmd_tdata  ),
    .rp_ROLE_siROL_Mem_Mp0_RdCmd_tvalid     (siROL_Mem_Mp0_RdCmd_tvalid ),
    .rp_ROLE_siROL_Mem_Mp0_RdCmd_tready     (siROL_Mem_Mp0_RdCmd_tready ),
    .rp_ROLE_soROL_Mem_Mp0_RdSts_tdata      (soROL_Mem_Mp0_RdSts_tdata  ),
    .rp_ROLE_soROL_Mem_Mp0_RdSts_tvalid     (soROL_Mem_Mp0_RdSts_tvalid ),
    .rp_ROLE_soROL_Mem_Mp0_RdSts_tready     (soROL_Mem_Mp0_RdSts_tready ),
    .rp_ROLE_soROL_Mem_Mp0_Read_tdata       (soROL_Mem_Mp0_Read_tdata   ),
    .rp_ROLE_soROL_Mem_Mp0_Read_tkeep       (soROL_Mem_Mp0_Read_tkeep   ),
    .rp_ROLE_soROL_Mem_Mp0_Read_tlast       (soROL_Mem_Mp0_Read_tlast   ),
    .rp_ROLE_soROL_Mem_Mp0_Read_tvalid      (soROL_Mem_Mp0_Read_tvalid  ),
    .rp_ROLE_soROL_Mem_Mp0_Read_tready      (soROL_Mem_Mp0_Read_tready  ),
    .rp_ROLE_siROL_Mem_Mp0_WrCmd_tdata      (siROL_Mem_Mp0_WrCmd_tdata  ),
    .rp_ROLE_siROL_Mem_Mp0_WrCmd_tvalid     (siROL_Mem_Mp0_WrCmd_tvalid ),
    .rp_ROLE_siROL_Mem_Mp0_WrCmd_tready     (siROL_Mem_Mp0_WrCmd_tready ),
    .rp_ROLE_soROL_Mem_Mp0_WrSts_tdata      (soROL_Mem_Mp0_WrSts_tdata  ),
    .rp_ROLE_soROL_Mem_Mp0_WrSts_tvalid     (soROL_Mem_Mp0_WrSts_tvalid ),
    .rp_ROLE_soROL_Mem_Mp0_WrSts_tready     (soROL_Mem_Mp0_WrSts_tready ),
    .rp_ROLE_siROL_Mem_Mp0_Write_tdata      (siROL_Mem_Mp0_Write_tdata  ),
    .rp_ROLE_siROL_Mem_Mp0_Write_tkeep      (siROL_Mem_Mp0_Write_tkeep  ),
    .rp_ROLE_siROL_Mem_Mp0_Write_tlast      (siROL_Mem_Mp0_Write_tlast  ),
    .rp_ROLE_siROL_Mem_Mp0_Write_tvalid     (siROL_Mem_Mp0_Write_tvalid ),
    .rp_ROLE_siROL_Mem_Mp0_Write_tready     (siROL_Mem_Mp0_Write_tready ),
    .rp_ROLE_miROL_Mem_Mp1_AWID             (miROL_Mem_Mp1_AWID         ),
    .rp_ROLE_miROL_Mem_Mp1_AWADDR           (miROL_Mem_Mp1_AWADDR       ),
    .rp_ROLE_miROL_Mem_Mp1_AWLEN            (miROL_Mem_Mp1_AWLEN        ),
    .rp_ROLE_miROL_Mem_Mp1_AWSIZE           (miROL_Mem_Mp1_AWSIZE       ),
    .rp_ROLE_miROL_Mem_Mp1_AWBURST          (miROL_Mem_Mp1_AWBURST      ),
    .rp_ROLE_miROL_Mem_Mp1_AWVALID          (miROL_Mem_Mp1_AWVALID      ),
    .rp_ROLE_miROL_Mem_Mp1_AWREADY          (miROL_Mem_Mp1_AWREADY      ),
    .rp_ROLE_miROL_Mem_Mp1_WDATA            (miROL_Mem_Mp1_WDATA        ),
    .rp_ROLE_miROL_Mem_Mp1_WSTRB            (miROL_Mem_Mp1_WSTRB        ),
    .rp_ROLE_miROL_Mem_Mp1_WLAST            (miROL_Mem_Mp1_WLAST        ),
    .rp_ROLE_miROL_Mem_Mp1_WVALID           (miROL_Mem_Mp1_WVALID       ),
    .rp_ROLE_miROL_Mem_Mp1_WREADY           (miROL_Mem_Mp1_WREADY       ),
    .rp_ROLE_miROL_Mem_Mp1_BID              (miROL_Mem_Mp1_BID          ),
    .rp_ROLE_miROL_Mem_Mp1_BRESP            (miROL_Mem_Mp1_BRESP        ),
    .rp_ROLE_miROL_Mem_Mp1_BVALID           (miROL_Mem_Mp1_BVALID       ),
    .rp_ROLE_miROL_Mem_Mp1_BREADY           (miROL_Mem_Mp1_BREADY       ),
    .rp_ROLE_miROL_Mem_Mp1_ARID             (miROL_Mem_Mp1_ARID         ),
    .rp_ROLE_miROL_Mem_Mp1_ARADDR           (miROL_Mem_Mp1_ARADDR       ),
    .rp_ROLE_miROL_Mem_Mp1_ARLEN            (miROL_Mem_Mp1_ARLEN        ),
    .rp_ROLE_miROL_Mem_Mp1_ARSIZE           (miROL_Mem_Mp1_ARSIZE       ),
    .rp_ROLE_miROL_Mem_Mp1_ARBURST          (miROL_Mem_Mp1_ARBURST      ),
    .rp_ROLE_miROL_Mem_Mp1_ARVALID          (miROL_Mem_Mp1_ARVALID      ),
    .rp_ROLE_miROL_Mem_Mp1_ARREADY          (miROL_Mem_Mp1_ARREADY      ),
    .rp_ROLE_miROL_Mem_Mp1_RID              (miROL_Mem_Mp1_RID          ),
    .rp_ROLE_miROL_Mem_Mp1_RDATA            (miROL_Mem_Mp1_RDATA        ),
    .rp_ROLE_miROL_Mem_Mp1_RRESP            (miROL_Mem_Mp1_RRESP        ),
    .rp_ROLE_miROL_Mem_Mp1_RLAST            (miROL_Mem_Mp1_RLAST        ),
    .rp_ROLE_miROL_Mem_Mp1_RVALID           (miROL_Mem_Mp1_RVALID       ),
    .rp_ROLE_miROL_Mem_Mp1_RREADY           (miROL_Mem_Mp1_RREADY       ),
    .rp_ROLE_piROL_Mmio_Mc1_MemTestStat     (piROL_Mmio_Mc1_MemTestStat ),
    .rp_ROLE_piROL_Mmio_RdReg               (piROL_Mmio_RdReg           ),
    .rp_ROLE_poROL_Fmc_Rank                 (poROL_Fmc_Rank             ),
    .rp_ROLE_poROL_Fmc_Size                 (poROL_Fmc_Size             ),
    //Shell side pins
    .s_ROLE_siROL_Nts_Udp_Data_tdata       (siDCP_ROL_Nts_Udp_Data_tdata   ),
    .s_ROLE_siROL_Nts_Udp_Data_tkeep       (siDCP_ROL_Nts_Udp_Data_tkeep   ),
    .s_ROLE_siROL_Nts_Udp_Data_tlast       (siDCP_ROL_Nts_Udp_Data_tlast   ),
    .s_ROLE_siROL_Nts_Udp_Data_tvalid      (siDCP_ROL_Nts_Udp_Data_tvalid  ),
    .s_ROLE_siROL_Nts_Udp_Data_tready      (siDCP_ROL_Nts_Udp_Data_tready  ),
    .s_ROLE_soROL_Nts_Udp_Data_tdata       (soDCP_ROL_Nts_Udp_Data_tdata   ),
    .s_ROLE_soROL_Nts_Udp_Data_tkeep       (soDCP_ROL_Nts_Udp_Data_tkeep   ),
    .s_ROLE_soROL_Nts_Udp_Data_tlast       (soDCP_ROL_Nts_Udp_Data_tlast   ),
    .s_ROLE_soROL_Nts_Udp_Data_tvalid      (soDCP_ROL_Nts_Udp_Data_tvalid  ),
    .s_ROLE_soROL_Nts_Udp_Data_tready      (soDCP_ROL_Nts_Udp_Data_tready  ),
    .s_ROLE_piROL_Nrc_Udp_Rx_ports         (piDCP_ROL_Nrc_Udp_Rx_ports     ),
    .s_ROLE_siROLE_Nrc_Udp_Meta_TDATA      (siDCP_ROLE_Nrc_Udp_Meta_TDATA  ),
    .s_ROLE_siROLE_Nrc_Udp_Meta_TVALID     (siDCP_ROLE_Nrc_Udp_Meta_TVALID ),
    .s_ROLE_siROLE_Nrc_Udp_Meta_TREADY     (siDCP_ROLE_Nrc_Udp_Meta_TREADY ),
    .s_ROLE_siROLE_Nrc_Udp_Meta_TKEEP      (siDCP_ROLE_Nrc_Udp_Meta_TKEEP  ),
    .s_ROLE_siROLE_Nrc_Udp_Meta_TLAST      (siDCP_ROLE_Nrc_Udp_Meta_TLAST  ),
    .s_ROLE_soNRC_Role_Udp_Meta_TDATA      (soDCP_NRC_Role_Udp_Meta_TDATA  ),
    .s_ROLE_soNRC_Role_Udp_Meta_TVALID     (soDCP_NRC_Role_Udp_Meta_TVALID ),
    .s_ROLE_soNRC_Role_Udp_Meta_TREADY     (soDCP_NRC_Role_Udp_Meta_TREADY ),
    .s_ROLE_soNRC_Role_Udp_Meta_TKEEP      (soDCP_NRC_Role_Udp_Meta_TKEEP  ),
    .s_ROLE_soNRC_Role_Udp_Meta_TLAST      (soDCP_NRC_Role_Udp_Meta_TLAST  ),
    .s_ROLE_siROL_Nts_Tcp_Data_tdata       (siDCP_ROL_Nts_Tcp_Data_tdata   ),
    .s_ROLE_siROL_Nts_Tcp_Data_tkeep       (siDCP_ROL_Nts_Tcp_Data_tkeep   ),
    .s_ROLE_siROL_Nts_Tcp_Data_tlast       (siDCP_ROL_Nts_Tcp_Data_tlast   ),
    .s_ROLE_siROL_Nts_Tcp_Data_tvalid      (siDCP_ROL_Nts_Tcp_Data_tvalid  ),
    .s_ROLE_siROL_Nts_Tcp_Data_tready      (siDCP_ROL_Nts_Tcp_Data_tready  ),
    .s_ROLE_soROL_Nts_Tcp_Data_tdata       (soDCP_ROL_Nts_Tcp_Data_tdata   ),
    .s_ROLE_soROL_Nts_Tcp_Data_tkeep       (soDCP_ROL_Nts_Tcp_Data_tkeep   ),
    .s_ROLE_soROL_Nts_Tcp_Data_tlast       (soDCP_ROL_Nts_Tcp_Data_tlast   ),
    .s_ROLE_soROL_Nts_Tcp_Data_tvalid      (soDCP_ROL_Nts_Tcp_Data_tvalid  ),
    .s_ROLE_soROL_Nts_Tcp_Data_tready      (soDCP_ROL_Nts_Tcp_Data_tready  ),
    .s_ROLE_piROL_Nrc_Tcp_Rx_ports         (piDCP_ROL_Nrc_Tcp_Rx_ports     ),
    .s_ROLE_siROLE_Nrc_Tcp_Meta_TDATA      (siDCP_ROLE_Nrc_Tcp_Meta_TDATA  ),
    .s_ROLE_siROLE_Nrc_Tcp_Meta_TVALID     (siDCP_ROLE_Nrc_Tcp_Meta_TVALID ),
    .s_ROLE_siROLE_Nrc_Tcp_Meta_TREADY     (siDCP_ROLE_Nrc_Tcp_Meta_TREADY ),
    .s_ROLE_siROLE_Nrc_Tcp_Meta_TKEEP      (siDCP_ROLE_Nrc_Tcp_Meta_TKEEP  ),
    .s_ROLE_siROLE_Nrc_Tcp_Meta_TLAST      (siDCP_ROLE_Nrc_Tcp_Meta_TLAST  ),
    .s_ROLE_soNRC_Role_Tcp_Meta_TDATA      (soDCP_NRC_Role_Tcp_Meta_TDATA  ),
    .s_ROLE_soNRC_Role_Tcp_Meta_TVALID     (soDCP_NRC_Role_Tcp_Meta_TVALID ),
    .s_ROLE_soNRC_Role_Tcp_Meta_TREADY     (soDCP_NRC_Role_Tcp_Meta_TREADY ),
    .s_ROLE_soNRC_Role_Tcp_Meta_TKEEP      (soDCP_NRC_Role_Tcp_Meta_TKEEP  ),
    .s_ROLE_soNRC_Role_Tcp_Meta_TLAST      (soDCP_NRC_Role_Tcp_Meta_TLAST  ),
    .s_ROLE_siROL_Mem_Mp0_RdCmd_tdata      (siDCP_ROL_Mem_Mp0_RdCmd_tdata  ),
    .s_ROLE_siROL_Mem_Mp0_RdCmd_tvalid     (siDCP_ROL_Mem_Mp0_RdCmd_tvalid ),
    .s_ROLE_siROL_Mem_Mp0_RdCmd_tready     (siDCP_ROL_Mem_Mp0_RdCmd_tready ),
    .s_ROLE_soROL_Mem_Mp0_RdSts_tdata      (soDCP_ROL_Mem_Mp0_RdSts_tdata  ),
    .s_ROLE_soROL_Mem_Mp0_RdSts_tvalid     (soDCP_ROL_Mem_Mp0_RdSts_tvalid ),
    .s_ROLE_soROL_Mem_Mp0_RdSts_tready     (soDCP_ROL_Mem_Mp0_RdSts_tready ),
    .s_ROLE_soROL_Mem_Mp0_Read_tdata       (soDCP_ROL_Mem_Mp0_Read_tdata   ),
    .s_ROLE_soROL_Mem_Mp0_Read_tkeep       (soDCP_ROL_Mem_Mp0_Read_tkeep   ),
    .s_ROLE_soROL_Mem_Mp0_Read_tlast       (soDCP_ROL_Mem_Mp0_Read_tlast   ),
    .s_ROLE_soROL_Mem_Mp0_Read_tvalid      (soDCP_ROL_Mem_Mp0_Read_tvalid  ),
    .s_ROLE_soROL_Mem_Mp0_Read_tready      (soDCP_ROL_Mem_Mp0_Read_tready  ),
    .s_ROLE_siROL_Mem_Mp0_WrCmd_tdata      (siDCP_ROL_Mem_Mp0_WrCmd_tdata  ),
    .s_ROLE_siROL_Mem_Mp0_WrCmd_tvalid     (siDCP_ROL_Mem_Mp0_WrCmd_tvalid ),
    .s_ROLE_siROL_Mem_Mp0_WrCmd_tready     (siDCP_ROL_Mem_Mp0_WrCmd_tready ),
    .s_ROLE_soROL_Mem_Mp0_WrSts_tdata      (soDCP_ROL_Mem_Mp0_WrSts_tdata  ),
    .s_ROLE_soROL_Mem_Mp0_WrSts_tvalid     (soDCP_ROL_Mem_Mp0_WrSts_tvalid ),
    .s_ROLE_soROL_Mem_Mp0_WrSts_tready     (soDCP_ROL_Mem_Mp0_WrSts_tready ),
    .s_ROLE_siROL_Mem_Mp0_Write_tdata      (siDCP_ROL_Mem_Mp0_Write_tdata  ),
    .s_ROLE_siROL_Mem_Mp0_Write_tkeep      (siDCP_ROL_Mem_Mp0_Write_tkeep  ),
    .s_ROLE_siROL_Mem_Mp0_Write_tlast      (siDCP_ROL_Mem_Mp0_Write_tlast  ),
    .s_ROLE_siROL_Mem_Mp0_Write_tvalid     (siDCP_ROL_Mem_Mp0_Write_tvalid ),
    .s_ROLE_siROL_Mem_Mp0_Write_tready     (siDCP_ROL_Mem_Mp0_Write_tready ),
    .s_ROLE_miROL_Mem_Mp1_AWID             (miDCP_ROL_Mem_Mp1_AWID         ),
    .s_ROLE_miROL_Mem_Mp1_AWADDR           (miDCP_ROL_Mem_Mp1_AWADDR       ),
    .s_ROLE_miROL_Mem_Mp1_AWLEN            (miDCP_ROL_Mem_Mp1_AWLEN        ),
    .s_ROLE_miROL_Mem_Mp1_AWSIZE           (miDCP_ROL_Mem_Mp1_AWSIZE       ),
    .s_ROLE_miROL_Mem_Mp1_AWBURST          (miDCP_ROL_Mem_Mp1_AWBURST      ),
    .s_ROLE_miROL_Mem_Mp1_AWVALID          (miDCP_ROL_Mem_Mp1_AWVALID      ),
    .s_ROLE_miROL_Mem_Mp1_AWREADY          (miDCP_ROL_Mem_Mp1_AWREADY      ),
    .s_ROLE_miROL_Mem_Mp1_WDATA            (miDCP_ROL_Mem_Mp1_WDATA        ),
    .s_ROLE_miROL_Mem_Mp1_WSTRB            (miDCP_ROL_Mem_Mp1_WSTRB        ),
    .s_ROLE_miROL_Mem_Mp1_WLAST            (miDCP_ROL_Mem_Mp1_WLAST        ),
    .s_ROLE_miROL_Mem_Mp1_WVALID           (miDCP_ROL_Mem_Mp1_WVALID       ),
    .s_ROLE_miROL_Mem_Mp1_WREADY           (miDCP_ROL_Mem_Mp1_WREADY       ),
    .s_ROLE_miROL_Mem_Mp1_BID              (miDCP_ROL_Mem_Mp1_BID          ),
    .s_ROLE_miROL_Mem_Mp1_BRESP            (miDCP_ROL_Mem_Mp1_BRESP        ),
    .s_ROLE_miROL_Mem_Mp1_BVALID           (miDCP_ROL_Mem_Mp1_BVALID       ),
    .s_ROLE_miROL_Mem_Mp1_BREADY           (miDCP_ROL_Mem_Mp1_BREADY       ),
    .s_ROLE_miROL_Mem_Mp1_ARID             (miDCP_ROL_Mem_Mp1_ARID         ),
    .s_ROLE_miROL_Mem_Mp1_ARADDR           (miDCP_ROL_Mem_Mp1_ARADDR       ),
    .s_ROLE_miROL_Mem_Mp1_ARLEN            (miDCP_ROL_Mem_Mp1_ARLEN        ),
    .s_ROLE_miROL_Mem_Mp1_ARSIZE           (miDCP_ROL_Mem_Mp1_ARSIZE       ),
    .s_ROLE_miROL_Mem_Mp1_ARBURST          (miDCP_ROL_Mem_Mp1_ARBURST      ),
    .s_ROLE_miROL_Mem_Mp1_ARVALID          (miDCP_ROL_Mem_Mp1_ARVALID      ),
    .s_ROLE_miROL_Mem_Mp1_ARREADY          (miDCP_ROL_Mem_Mp1_ARREADY      ),
    .s_ROLE_miROL_Mem_Mp1_RID              (miDCP_ROL_Mem_Mp1_RID          ),
    .s_ROLE_miROL_Mem_Mp1_RDATA            (miDCP_ROL_Mem_Mp1_RDATA        ),
    .s_ROLE_miROL_Mem_Mp1_RRESP            (miDCP_ROL_Mem_Mp1_RRESP        ),
    .s_ROLE_miROL_Mem_Mp1_RLAST            (miDCP_ROL_Mem_Mp1_RLAST        ),
    .s_ROLE_miROL_Mem_Mp1_RVALID           (miDCP_ROL_Mem_Mp1_RVALID       ),
    .s_ROLE_miROL_Mem_Mp1_RREADY           (miDCP_ROL_Mem_Mp1_RREADY       ),
    .s_ROLE_piROL_Mmio_Mc1_MemTestStat     (piDCP_ROL_Mmio_Mc1_MemTestStat ),
    .s_ROLE_piROL_Mmio_RdReg               (piDCP_ROL_Mmio_RdReg           ),
    .s_ROLE_poROL_Fmc_Rank                 (poDCP_ROL_Fmc_Rank             ),
    .s_ROLE_poROL_Fmc_Size                 (poDCP_ROL_Fmc_Size             )
 );

  //===========================================================================
  //==  INST: METASTABILITY HARDENED BLOCK FOR THE SHELL RESET (Active high)
  //==    [INFO] Note that we instantiate 2 or 3 library primitives rather than
  //==      a Verilog process because it makes it easier to apply the 
  //==      "ASYNC_REG" property to those instances.
  //===========================================================================
  HARD_SYNC #(
    .INIT             (1'b0), // Initial values, 1'b0, 1'b1
    .IS_CLK_INVERTED  (1'b0), // Programmable inversion on CLK input
    .LATENCY          (3)     // 2-3
  ) META_RST (
    .CLK  (sETH0_ShlClk),                             // 1-bit input:  Clock
    .DIN  (piTOP_156_25Rst | ~sETH0_CoreResetDone),   // 1-bit input:  Data
    .DOUT (sETH0_ShlRst)                              // 1-bit output: Data
  );


  //============================================================================
  //  PROC: BINARY COUNTER
  //============================================================================
  localparam cCntWidth = 30;
  reg [cCntWidth-1:0]   sBinCnt = {cCntWidth{1'b0}};

  always @(posedge sETH0_ShlClk)
    sBinCnt <= sBinCnt + 1'b1;  


  //============================================================================
  //  PROC: HEART_BEAT
  //----------------------------------------------------------------------------
  //    Generates a heart beat that encodes the status of the major IP cores in
  //    the following blinking sequence (see yellow LED near top edge coonector):
  //    
  //      sBinCnt[26] | 0 1 0 1|0 1|0 1|0 1|0 1|0 1|0 1|  --> ~1 Hz   
  //      sBinCnt[27] | 0 0 1 1|0 0|1 1|0 0|1 1|0 0|1 1|  
  //      sBinCnt[28] | 0 0 0 0|1 1|1 1|0 0|0 0|1 1|1 1|
  //      sBinCnt[29] | 0 0 0 0|0 0|0 0|1 1|1 1|1 1|1 1|
  //      sMc0_Ready  | X X X X|X 1|X X|X X|X X|X X|X X|
  //      sMc1_Ready  | X X X X|X X|X 1|X X|X X|X X|X X|
  //      sETH0_Ready | X X X X|X X|X X|X 1|X X|X X|X X|
  //      ------------+--------+---+---+---+---+---|---+
  //   sLed_HeartBeat | 0 0 1 1|0 1|0 1|0 1|0 0|0 0|0 0|
  //
  //============================================================================
  reg   sLed_HeartBeat;

  wire  sETH0_Ready;
  assign sETH0_Ready = sETH0_MMIO_CoreReady;

  wire sMc0_Ready;
  wire sMc1_Ready;
  assign sMc0_Ready = sMEM_MMIO_Mc0InitCalComplete;
  assign sMc1_Ready = sMEM_MMIO_Mc1InitCalComplete;

  always @(posedge sETH0_ShlClk)
    sLed_HeartBeat <= (!sBinCnt[29] && !sBinCnt[28])                                              ||  // Start bit
                      (!sBinCnt[29] &&  sBinCnt[28] && !sBinCnt[27] && sBinCnt[26] & sMc0_Ready)  ||  // Memory channel 0
                      (!sBinCnt[29] &&  sBinCnt[28] &&  sBinCnt[27] && sBinCnt[26] & sMc1_Ready)  ||  // Memory channel 1
                      ( sBinCnt[29] && !sBinCnt[28] && !sBinCnt[27] && sBinCnt[26] & sETH0_Ready);    // Ethernet MAC 0

  assign poLED_HeartBeat_n = ~sLed_HeartBeat; // LED is active low  


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poROL_156_25Clk   = sETH0_ShlClk;
  assign poROL_156_25Rst   = sETH0_ShlRst;
  assign poROL_Mmio_Ly7Rst = sMMIO_LayerRst[7];
  assign poROL_Mmio_Ly7En  = sMMIO_LayerEn[7];

endmodule
