//                              -*- Mode: Verilog -*-
// Filename        : Shell_x1Udp_x1Tcp_x2Mc.v
// Description     : 
// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Shell for the FMKU2595 when equipped with a XCKU060.
// *
// * File    : Shell_x1Udp_x1Tcp_x2Mp_x2Mc.v
// *
// * Created : Nov. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : cloudFPGA uses a 'SHELL' to abstract the HW components of an
// *    FPGA module and to expose a unified interface for the user to integrate 
// *    its application, referred to as 'ROLE'. 
// * 
// *    As the name indicates, this shell implements the following interfaces: 
// *      - one UDP port interface (based on the AXI4-Stream interface), 
// *      - one TCP port interface (based on the AXI4-Stream interface),
// *      - two Memory Port interfaces (based on the MM2S and S2MM AXI4-Stream interfaces)
// *      - two Memory Channel interfaces towards two DDR4 banks. 
// *
// *    This shell implements the following IP cores and physical interfaces:
// *      - one 10G Ethernet subsystem (ETH0) as described in PG157,
// *      - two 8GB DDR4 Memory Channels (MC0, MC1) as described in PG150,
// *      - one network, tansport and session (NTS0) core based on TCP/IP,
// *      - one register file with memory mapped IOs (MMIO).
// *     
// *    The interfaces exposed to the user's ROLE are:
// *      - one AXI4-Stream interface for the UDP interface, 
// *      - one AXI4-Stream interface for the TCP interface,
// *      - two MM2S and two S2MM AXI4-Stream interfaces for the Memory Ports.
// *        (refer to PG022-AXI- DataMover for a description of the MM2S and S2MM).     
// * 
// * Parameters:
// *    gSecurityPriviledges: Sets the level of the security privileges.
// *      [ "user" (Default) | "super" ]
// *    gBitstreamUsage: defines the usage of the bitstream to generate.
// *      [ "user" (Default) | "flash" ]
// *
// * Comments:
// *
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - SHELL FOR FMKU60
// *****************************************************************************

module Shell_x1Udp_x1Tcp_x2Mp_x2Mc # (

  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user",  // "user" or "flash"
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
  output          poTOP_Led_HeartBeat_n,

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
  input           piTOP_156_25Rst_delayed,  // Soft Reset
  output          poROL_156_25Clk,
  output          poROL_156_25Rst,

  //------------------------------------------------------
  //-- ROLE / Nts / Udp Interfaces
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

  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / TxP Data Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path (ROLE-->NTS) -----------
  //---- Stream TCP Data ---------------
  input  [ 63:0]  siROL_Nts_Tcp_Data_tdata,
  input  [  7:0]  siROL_Nts_Tcp_Data_tkeep,
  input           siROL_Nts_Tcp_Data_tlast,
  input           siROL_Nts_Tcp_Data_tvalid,
  output          siROL_Nts_Tcp_Data_tready,
  //---- Stream TCP Metadata -----------
  input  [ 15:0]  siROL_Nts_Tcp_Meta_tdata,
  input           siROL_Nts_Tcp_Meta_tvalid,
  output          siROL_Nts_Tcp_Meta_tready,
  //---- Stream TCP Data Status --------
  output [ 23:0]  soROL_Nts_Tcp_DSts_tdata,
  output          soROL_Nts_Tcp_DSts_tvalid,
  input           soROL_Nts_Tcp_DSts_tready,
  
  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / RxP Data Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path (NTS-->ROLE) -------------
  //---- Stream TCP Data ---------------
  output [ 63:0]  soROL_Nts_Tcp_Data_tdata,
  output [  7:0]  soROL_Nts_Tcp_Data_tkeep,
  output          soROL_Nts_Tcp_Data_tlast,
  output          soROL_Nts_Tcp_Data_tvalid,
  input           soROL_Nts_Tcp_Data_tready,
  //---- Stream TCP Metadata -----------
  output [ 15:0]  soROL_Nts_Tcp_Meta_tdata,
  output          soROL_Nts_Tcp_Meta_tvalid,
  input           soROL_Nts_Tcp_Meta_tready,
  //---- Stream TCP Data Notification --
  output [ 87:0]  soROL_Nts_Tcp_Notif_tdata,
  output          soROL_Nts_Tcp_Notif_tvalid,
  input           soROL_Nts_Tcp_Notif_tready,
  //---- Stream TCP Data Request -------
  input  [ 31:0]  siROL_Nts_Tcp_DReq_tdata,
  input           siROL_Nts_Tcp_DReq_tvalid,
  output          siROL_Nts_Tcp_DReq_tready,

  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / TxP Ctlr Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path (ROLE-->NTS) -----------
  //---- Stream TCP Open Session Request
  input [ 47:0]  siROL_Nts_Tcp_OpnReq_tdata,
  input          siROL_Nts_Tcp_OpnReq_tvalid,
  output         siROL_Nts_Tcp_OpnReq_tready,
  //---- Stream TCP Open Session Status 
  output [ 23:0] soROL_Nts_Tcp_OpnRep_tdata,
  output         soROL_Nts_Tcp_OpnRep_tvalid,
  input          soROL_Nts_Tcp_OpnRep_tready,
  //---- Stream TCP Close Request ------
  input [ 15:0]  siROL_Nts_Tcp_ClsReq_tdata,
  input          siROL_Nts_Tcp_ClsReq_tvalid,
  output         siROL_Nts_Tcp_ClsReq_tready,

  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / RxP Ctlr Flow Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path (NTS-->ROLE) ------------
  //---- Stream TCP Listen Request -----
  input [ 15:0]  siROL_Nts_Tcp_LsnReq_tdata,   
  input          siROL_Nts_Tcp_LsnReq_tvalid,
  output         siROL_Nts_Tcp_LsnReq_tready,
  //---- Stream TCP Listen Status ------
  output [  7:0] soROL_Nts_Tcp_LsnAck_tdata,
  output         soROL_Nts_Tcp_LsnAck_tvalid,
  input          soROL_Nts_Tcp_LsnAck_tready,

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
  output          soROL_Mem_Mp0_WrSts_tvalid,
  output [  7:0]  soROL_Mem_Mp0_WrSts_tdata,
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
  //-- Memory Port #1 / S2MM-AXIS ------------------
  //---- Stream Read Command -----------
  input  [ 79:0]  siROL_Mem_Mp1_RdCmd_tdata,
  input           siROL_Mem_Mp1_RdCmd_tvalid,
  output          siROL_Mem_Mp1_RdCmd_tready,
  //---- Stream Read Status ------------
  output [  7:0]  soROL_Mem_Mp1_RdSts_tdata,
  output          soROL_Mem_Mp1_RdSts_tvalid,
  input           soROL_Mem_Mp1_RdSts_tready,
  //---- Stream Data Output Channel ----
  output [511:0]  soROL_Mem_Mp1_Read_tdata,
  output [ 63:0]  soROL_Mem_Mp1_Read_tkeep,
  output          soROL_Mem_Mp1_Read_tlast,
  output          soROL_Mem_Mp1_Read_tvalid,
  input           soROL_Mem_Mp1_Read_tready,
  //---- Stream Write Command ----------
  input  [ 79:0]  siROL_Mem_Mp1_WrCmd_tdata,
  input           siROL_Mem_Mp1_WrCmd_tvalid,
  output          siROL_Mem_Mp1_WrCmd_tready,
  //---- Stream Write Status -----------
  output          soROL_Mem_Mp1_WrSts_tvalid,
  output [  7:0]  soROL_Mem_Mp1_WrSts_tdata,
  input           soROL_Mem_Mp1_WrSts_tready,
  //---- Stream Data Input Channel -----
  input  [511:0]  siROL_Mem_Mp1_Write_tdata,
  input  [ 63:0]  siROL_Mem_Mp1_Write_tkeep,
  input           siROL_Mem_Mp1_Write_tlast,
  input           siROL_Mem_Mp1_Write_tvalid,
  output          siROL_Mem_Mp1_Write_tready,
    
  //--------------------------------------------------------
  //-- ROLE / Mmio / AppFlash Interface
  //--------------------------------------------------------
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
  // NOT_USED_BY_THIS_SHELL output [ 31:0]  poROL_Fmc_Rank,
  // NOT_USED_BY_THIS_SHELL output [ 31:0]  poROL_Fmc_Size,
  
  output          poVoid
  
);  // End of PortList


  // *****************************************************************************
  // **  STRUCTURE
  // *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //-- Global Clock and Reset used by the entire SHELL -------------------------
  //---- This clock is generated by the ETH core and runs at 156.25MHz ---------
  (* keep="true" *)
  wire          sETH0_ShlClk;
  (* keep="true" *)
  wire          sETH0_ShlRst;
  wire          sETH0_CoreResetDone;  

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
  //-- SIGNAL DECLARATIONS : ROLE <--> MEM
  //--------------------------------------------------------
  //-- Memory Port #0 ------------------------------
  //------  Stream Read Command --------
  wire [ 79:0]  sROL_Mem_Mp0_Axis_RdCmd_tdata;
  wire          sROL_Mem_Mp0_Axis_RdCmd_tvalid;
  wire          sMEM_Rol_Mp0_Axis_RdCmd_tready;
  //------ Stream Read Status ----------
  wire          sROL_Mem_Mp0_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Rol_Mp0_Axis_RdSts_tdata;
  wire          sMEM_Rol_Mp0_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --
  wire          sROL_Mem_Mp0_Axis_Read_tready;
  wire [511:0]  sMEM_Rol_Mp0_Axis_Read_tdata;
  wire [ 63:0]  sMEM_Rol_Mp0_Axis_Read_tkeep;
  wire          sMEM_Rol_Mp0_Axis_Read_tlast;
  wire          sMEM_Rol_Mp0_Axis_Read_tvalid;
  //------ Stream Write Command --------
  wire [ 79:0]  sROL_Mem_Mp0_Axis_WrCmd_tdata;
  wire          sROL_Mem_Mp0_Axis_WrCmd_tvalid;
  wire          sMEM_Rol_Mp0_Axis_WrCmd_tready;
  //------ Stream Write Status ---------
  wire          sROL_Mem_Mp0_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Rol_Mp0_Axis_WrSts_tdata;
  wire          sMEM_Rol_Mp0_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---
  wire [511:0]  sROL_Mem_Mp0_Axis_Write_tdata;
  wire [ 63:0]  sROL_Mem_Mp0_Axis_Write_tkeep;
  wire          sROL_Mem_Mp0_Axis_Write_tlast;
  wire          sROL_Mem_Mp0_Axis_Write_tvalid;
  wire          sMEM_Rol_Mp0_Axis_Write_tready;
  //---- Receive Path ----------------------------
  //------ Stream Read Command ---------
  wire [ 79:0]  sROL_Mem_Mp1_Axis_RdCmd_tdata;
  wire          sROL_Mem_Mp1_Axis_RdCmd_tvalid;
  wire          sMEM_Rol_Mp1_Axis_RdCmd_tready;
  //------ Stream Read Status ----------
  wire          sROL_Mem_Mp1_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Rol_Mp1_Axis_RdSts_tdata;
  wire          sMEM_Rol_Mp1_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --
  wire          sROL_Mem_Mp1_Axis_Read_tready;
  wire [511:0]  sMEM_Rol_Mp1_Axis_Read_tdata;
  wire [ 63:0]  sMEM_Rol_Mp1_Axis_Read_tkeep;
  wire          sMEM_Rol_Mp1_Axis_Read_tlast;
  wire          sMEM_Rol_Mp1_Axis_Read_tvalid;
  //------ Stream Write Command --------
  wire [ 79:0]  sROL_Mem_Mp1_Axis_WrCmd_tdata;
  wire          sROL_Mem_Mp1_Axis_WrCmd_tvalid;
  wire          sMEM_Rol_Mp1_Axis_WrCmd_tready;
  //------ Stream Write Status ---------
  wire          sROL_Mem_Mp1_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Rol_Mp1_Axis_WrSts_tdata;
  wire          sMEM_Rol_Mp1_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---
  wire [511:0]  sROL_Mem_Mp1_Axis_Write_tdata;
  wire [ 63:0]  sROL_Mem_Mp1_Axis_Write_tkeep;
  wire          sROL_Mem_Mp1_Axis_Write_tlast;
  wire          sROL_Mem_Mp1_Axis_Write_tvalid;
  wire          sMEM_Rol_Mp1_Axis_Write_tready;

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
  wire  [31:0]  sMMIO_NTS0_IpAddress;
  //------ [LY3_SUBNET] ----------------
  wire  [31:0]  sMMIO_NTS0_SubNetMask;
  //------ [LY3_GATEWAY] ---------------
  wire  [31:0]  sMMIO_NTS0_GatewayAddr; 
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
 
  //-- END OF SIGNAL DECLARATIONS ----------------------------------------------

  //============================================================================
  //  INST: MMIIO CLIENT
  //============================================================================
  MmioClient_A8_D8 #(
    .gSecurityPriviledges (gSecurityPriviledges),
    .gBitstreamUsage      (gBitstreamUsage)

  ) MMIO (

    //----------------------------------------------
    //-- Global Clock & Reset
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
    .poNTS0_MacAddress              (sMMIO_NTS0_MacAddress),
    .poNTS0_IpAddress               (sMMIO_NTS0_IpAddress),
    .poNTS0_SubNetMask              (sMMIO_NTS0_SubNetMask),
    .poNTS0_GatewayAddr             (sMMIO_NTS0_GatewayAddr),

    //----------------------------------------------
    //-- ROLE : Status input and Control Outputs
    //----------------------------------------------
    //---- DIAG_CTRL_1 ---------------
    .poROLE_Mc1_MemTestCtrl         (sMMIO_ROL_Mc1_MemTestCtrl),
    //---- DIAG_STAT_1 ---------------
    .piROLE_Mc1_MemTestStat         (sROL_MMIO_Mc1_MemTestStat),
    //---- DIAG_CTRL_2 ---------------  
    .poROLE_UdpEchoCtrl             (poROL_Mmio_UdpEchoCtrl),
    .poROLE_UdpPostDgmEn            (poROL_Mmio_UdpPostDgmEn),
    .poROLE_UdpCaptDgmEn            (poROL_Mmio_UdpCaptDgmEn),
    .poROLE_TcpEchoCtrl             (poROL_Mmio_TcpEchoCtrl),
    .poROLE_TcpPostSegEn            (poROL_Mmio_TcpPostSegEn),
    .poROLE_TcpCaptSegEn            (poROL_Mmio_TcpCaptSegEn),
     //---- APP_RDROL ----------------
    .piROLE_RdReg                   (piROL_Mmio_RdReg),
     //---- APP_WRROL -----------------
    .poROLE_WrReg                   (poROL_Mmio_WrReg),

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
    .piXXX_XMem_en                  ( 1'b0), // (OBSOLETE-20190710 sFMC_MMIO_XMem_cen),
    .piXXX_XMem_Wren                ( 1'b0), // (OBSOLETE-20190710 sFMC_MMIO_XMem_wren),
    .piXXX_XMem_WrData              (32'd0), // (OBSOLETE-20190710 sFMC_MMIO_XMem_WData),
    .poXXX_XMem_RData               (),      // (OBSOLETE-20190710 sMMIO_FMC_XMem_RData),
    .piXXX_XMemAddr                 ( 8'd0), // (OBSOLETE-20190710 sFMC_MMIO_XMem_Addr),
    
    .poVoid                         ()

  );  // End of MMMIO


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
      .poETH0_CoreClk               (sETH0_ShlClk),
      .poETH0_CoreResetDone         (sETH0_CoreResetDone),

      //-- MMIO : Control inputs and Status outputs
      .piMMIO_Eth0_RxEqualizerMode  (sMMIO_ETH0_RxEqualizerMode),
      .piMMIO_Eth0_TxDriverSwing    (sMMIO_ETH0_TxDriverSwing),
      .piMMIO_Eth0_TxPreCursor      (sMMIO_ETH0_TxPreCursor),
      .piMMIO_Eth0_TxPostCursor     (sMMIO_ETH0_TxPostCursor),
      .piMMIO_Eth0_PcsLoopbackEn    (sMMIO_ETH0_PcsLoopbackEn),
      .piMMIO_Eth0_MacLoopbackEn    (sMMIO_ETH0_MacLoopbackEn),
      .piMMIO_MacAddrSwapEn         (sMMIO_ETH0_MacAddrSwapEn),
      .poETH0_Mmio_CoreReady        (sETH0_MMIO_CoreReady),
      .poETH0_Mmio_QpllLock         (sETH0_MMIO_QpllLock),

      //-- ECON : Gigabit Transceivers -------------
      .piECON_Eth0_Gt_n             (piECON_Eth_10Ge0_n),
      .piECON_Eth0_Gt_p             (piECON_Eth_10Ge0_p),
      .poETH0_Econ_Gt_n             (poECON_Eth_10Ge0_n),
      .poETH0_Econ_Gt_p             (poECON_Eth_10Ge0_p),

      //-- NTS0: Network-Transport-Session ---------
      //---- Input AXI-Write Stream Interface ------
      .piLY3_Eth0_Axis_tdata        (ssNTS0_ETH0_Data_tdata),
      .piLY3_Eth0_Axis_tkeep        (ssNTS0_ETH0_Data_tkeep),
      .piLY3_Eth0_Axis_tvalid       (ssNTS0_ETH0_Data_tvalid),
      .piLY3_Eth0_Axis_tlast        (ssNTS0_ETH0_Data_tlast),
      .poETH0_Ly3_Axis_tready       (ssNTS0_ETH0_Data_tready),
      //---- Output AXI-Write Stream Interface -----
      .poETH0_Ly3_Axis_tdata        (ssETH0_NTS0_Data_tdata),
      .poETH0_Ly3_Axis_tkeep        (ssETH0_NTS0_Data_tkeep),
      .poETH0_Ly3_Axis_tvalid       (ssETH0_NTS0_Data_tvalid),
      .poETH0_Ly3_Axis_tlast        (ssETH0_NTS0_Data_tlast),
      .piLY3_Eth0_Axis_tready       (ssETH0_NTS0_Data_tready)

    );  // End of UserCfg.ETH0

  end // if ((gBitstreamUsage == "user") && (gSecurityPriviledges == "user"))

  else if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) begin: SuperCfg

    //========================================================================
    //  INST: 10G ETHERNET SUBSYSTEM W/ LOOPBACK SUPPORT
    //========================================================================
    TenGigEth_Flash ETH0 (

      //-- Clocks and Resets inputs ----------------
      .piTOP_156_25Clk              (piTOP_156_25Clk),    // Freerunning
      .piCLKT_Gt_RefClk_n           (piCLKT_10GeClk_n),
      .piCLKT_Gt_RefClk_p           (piCLKT_10GeClk_p),
      .piTOP_Reset                  (piTOP_156_25Rst),

      //-- Clocks and Resets outputs ---------------
      .poETH0_CoreClk               (sETH0_ShlClk),
      .poETH0_CoreResetDone         (sETH0_CoreResetDone),

      //-- MMIO : Control inputs and Status outputs 
      .piMMIO_Eth0_RxEqualizerMode  (sMMIO_ETH0_RxEqualizerMode),
      .piMMIO_Eth0_PcsLoopbackEn    (sMMIO_ETH0_PcsLoopbackEn),
      .piMMIO_Eth0_MacLoopbackEn    (sMMIO_ETH0_MacLoopbackEn),
      .piMMIO_Eth0_MacAddrSwapEn    (sMMIO_ETH0_MacAddrSwapEn),
      .poETH0_Mmio_CoreReady        (sETH0_MMIO_CoreReady),
      .poETH0_Mmio_QpllLock         (sETH0_MMIO_QpllLock),

      //-- ECON : Gigabit Transceivers -------------
      .piECON_Eth0_Gt_n             (piECON_Eth_10Ge0_n),
      .piECON_Eth0_Gt_p             (piECON_Eth_10Ge0_p),
      .poETH0_Econ_Gt_n             (poECON_Eth_10Ge0_n),
      .poETH0_Econ_Gt_p             (poECON_Eth_10Ge0_p),

      //-- NTS0 : Network-Transport-Session ---------
      //---- Input AXI-Write Stream Interface ------
      .piLY3_Eth0_Axis_tdata        (ssNTS0_ETH0_Data_tdata),
      .piLY3_Eth0_Axis_tkeep        (ssNTS0_ETH0_Data_tkeep),
      .piLY3_Eth0_Axis_tvalid       (ssNTS0_ETH0_Data_tvalid),
      .piLY3_Eth0_Axis_tlast        (ssNTS0_ETH0_Data_tlast),
      .poETH0_Ly3_Axis_tready       (ssNTS0_ETH0_Data_tready),
      //---- Output AXI-Write Stream Interface -----
      .poETH0_Ly3_Axis_tdata        (ssETH0_NTS0_Data_tdata),
      .poETH0_Ly3_Axis_tkeep        (ssETH0_NTS0_Data_tkeep),
      .poETH0_Ly3_Axis_tvalid       (ssETH0_NTS0_Data_tvalid),
      .poETH0_Ly3_Axis_tlast        (ssETH0_NTS0_Data_tlast),
      .piLY3_Eth0_Axis_tready       (ssETH0_NTS0_Data_tready)

    );  // End of SuperCfg.ETH0 

  end // if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super"))

  endgenerate


  //============================================================================
  //  INST: NETWORK+TRANSPORT+SESSION SUBSYSTEM (OSI Network Layers 3+4+5)
  //============================================================================
  NetworkTransportSession_TcpIp NTS0 (

    //-- Global Clock used by the entire SHELL --------------
    //--   (This is typically 'sETH0_ShlClk' and we use it all over the place) 
    .piShlClk                         (sETH0_ShlClk),

    //-- Global Reset used by the entire SHELL -------------
    //--   (This is typically 'sETH0_ShlRst'. If the module is created by HLS,
    //--    we use it as the default startup reset of the module.) 
    .piShlRst                         (sETH0_ShlRst),
    
    //-- System Reset --------------------------------------
    //--   (This is a delayed version of the global reset. We use it when we
    //--    specifically want to control the re-initialization of a HLS variable.
    //--    We recommended to leave the "config_rtl" configuration to its default
    //--    "control" setting and to use this signal to provide finer grain reset
    //--    functionnality. See "Controlling the Reset Behavior" in UG902).
    .piShlRstDly                      (piTOP_156_25Rst_delayed),
    
    //------------------------------------------------------
    //-- ETH / Ethernet Layer-2 Interfaces
    //------------------------------------------------------
    //-- Input AXIS Interface --------------------
    .siETH_Data_tdata                 (ssETH0_NTS0_Data_tdata),
    .siETH_Data_tkeep                 (ssETH0_NTS0_Data_tkeep),
    .siETH_Data_tlast                 (ssETH0_NTS0_Data_tlast),
    .siETH_Data_tvalid                (ssETH0_NTS0_Data_tvalid),
    .siETH_Data_tready                (ssETH0_NTS0_Data_tready),
    //-- Output AXIS Interface ------------------- 
    .soETH_Data_tdata                 (ssNTS0_ETH0_Data_tdata),
    .soETH_Data_tkeep                 (ssNTS0_ETH0_Data_tkeep),
    .soETH_Data_tlast                 (ssNTS0_ETH0_Data_tlast),
    .soETH_Data_tvalid                (ssNTS0_ETH0_Data_tvalid),
    .soETH_Data_tready                (ssNTS0_ETH0_Data_tready),  

    //------------------------------------------------------
    //-- MEM / Nts / TxP Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path / S2MM-AXIS --------------------
    //---- Stream Read Command -------------------
    //---- Stream Read Command -------------------
    .soMEM_TxP_RdCmd_tdata            (ssNTS0_MEM_TxP_RdCmd_tdata),
    .soMEM_TxP_RdCmd_tvalid           (ssNTS0_MEM_TxP_RdCmd_tvalid),
    .soMEM_TxP_RdCmd_tready           (ssNTS0_MEM_TxP_RdCmd_tready),
    //---- Stream Read Status ------------------
    .siMEM_TxP_RdSts_tdata            (ssMEM_NTS0_TxP_RdSts_tdata),
    .siMEM_TxP_RdSts_tvalid           (ssMEM_NTS0_TxP_RdSts_tvalid),
    .siMEM_TxP_RdSts_tready           (ssMEM_NTS0_TxP_RdSts_tready),
    //---- Stream Data Input Channel -----------
    .siMEM_TxP_Data_tdata             (ssMEM_NTS0_TxP_Read_tdata),
    .siMEM_TxP_Data_tkeep             (ssMEM_NTS0_TxP_Read_tkeep),
    .siMEM_TxP_Data_tlast             (ssMEM_NTS0_TxP_Read_tlast),
    .siMEM_TxP_Data_tvalid            (ssMEM_NTS0_TxP_Read_tvalid),
    .siMEM_TxP_Data_tready            (ssMEM_NTS0_TxP_Read_tready),
    //---- Stream Write Command ----------------
    .soMEM_TxP_WrCmd_tdata            (ssNTS0_MEM_TxP_WrCmd_tdata),
    .soMEM_TxP_WrCmd_tvalid           (ssNTS0_MEM_TxP_WrCmd_tvalid),
    .soMEM_TxP_WrCmd_tready           (ssNTS0_MEM_TxP_WrCmd_tready),
    //---- Stream Write Status -----------------
    .siMEM_TxP_WrSts_tdata            (ssMEM_NTS0_TxP_WrSts_tdata),
    .siMEM_TxP_WrSts_tvalid           (ssMEM_NTS0_TxP_WrSts_tvalid),
    .siMEM_TxP_WrSts_tready           (ssMEM_NTS0_TxP_WrSts_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_TxP_Data_tdata             (ssNTS0_MEM_TxP_Write_tdata),
    .soMEM_TxP_Data_tkeep             (ssNTS0_MEM_TxP_Write_tkeep),
    .soMEM_TxP_Data_tlast             (ssNTS0_MEM_TxP_Write_tlast),
    .soMEM_TxP_Data_tvalid            (ssNTS0_MEM_TxP_Write_tvalid),
    .soMEM_TxP_Data_tready            (ssNTS0_MEM_TxP_Write_tready),

 
    //------------------------------------------------------
    //-- MEM / Nts / RxP Interfaces
    //------------------------------------------------------
    //-- FPGA Receive Path / S2MM-AXIS -------------
    //---- Stream Read Command -----------------
    .soMEM_RxP_RdCmd_tdata            (ssNTS0_MEM_RxP_RdCmd_tdata),
    .soMEM_RxP_RdCmd_tvalid           (ssNTS0_MEM_RxP_RdCmd_tvalid),
    .soMEM_RxP_RdCmd_tready           (ssNTS0_MEM_RxP_RdCmd_tready),
    //---- Stream Read Status ------------------
    .siMEM_RxP_RdSts_tdata            (ssMEM_NTS0_RxP_RdSts_tdata),
    .siMEM_RxP_RdSts_tvalid           (ssMEM_NTS0_RxP_RdSts_tvalid),
    .siMEM_RxP_RdSts_tready           (ssMEM_NTS0_RxP_RdSts_tready),
    //---- Stream Data Input Channel ----------
    .siMEM_RxP_Data_tdata             (ssMEM_NTS0_RxP_Read_tdata),
    .siMEM_RxP_Data_tkeep             (ssMEM_NTS0_RxP_Read_tkeep),
    .siMEM_RxP_Data_tlast             (ssMEM_NTS0_RxP_Read_tlast),
    .siMEM_RxP_Data_tvalid            (ssMEM_NTS0_RxP_Read_tvalid),
    .siMEM_RxP_Data_tready            (ssMEM_NTS0_RxP_Read_tready),
    //---- Stream Write Command ----------------
    .soMEM_RxP_WrCmd_tdata            (ssNTS0_MEM_RxP_WrCmd_tdata),
    .soMEM_RxP_WrCmd_tvalid           (ssNTS0_MEM_RxP_WrCmd_tvalid),
    .soMEM_RxP_WrCmd_tready           (ssNTS0_MEM_RxP_WrCmd_tready),
    //---- Stream Write Status -----------------
    .siMEM_RxP_WrSts_tdata            (ssMEM_NTS0_RxP_WrSts_tdata),
    .siMEM_RxP_WrSts_tvalid           (ssMEM_NTS0_RxP_WrSts_tvalid),
    .siMEM_RxP_WrSts_tready           (ssMEM_NTS0_RxP_WrSts_tready),
    //---- Stream Data Output Channel ----------
    .soMEM_RxP_Data_tdata             (ssNTS0_MEM_RxP_Write_tdata),
    .soMEM_RxP_Data_tkeep             (ssNTS0_MEM_RxP_Write_tkeep),
    .soMEM_RxP_Data_tlast             (ssNTS0_MEM_RxP_Write_tlast),
    .soMEM_RxP_Data_tvalid            (ssNTS0_MEM_RxP_Write_tvalid),
    .soMEM_RxP_Data_tready            (ssNTS0_MEM_RxP_Write_tready),

    //------------------------------------------------------
    //-- ROLE / Nts / Tcp / TxP Data Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path (ROLE-->NTS) -----------
    //---- Stream TCP Data ---------------------
    .siROL_Tcp_Data_tdata             (siROL_Nts_Tcp_Data_tdata),
    .siROL_Tcp_Data_tkeep             (siROL_Nts_Tcp_Data_tkeep),
    .siROL_Tcp_Data_tlast             (siROL_Nts_Tcp_Data_tlast),
    .siROL_Tcp_Data_tvalid            (siROL_Nts_Tcp_Data_tvalid),
    .siROL_Tcp_Data_tready            (siROL_Nts_Tcp_Data_tready),
    //---- Stream TCP Metadata -----------------
    .siROL_Tcp_Meta_tdata             (siROL_Nts_Tcp_Meta_tdata),
    .siROL_Tcp_Meta_tvalid            (siROL_Nts_Tcp_Meta_tvalid),
    .siROL_Tcp_Meta_tready            (siROL_Nts_Tcp_Meta_tready),
    //---- Stream TCP Data Status --------------
    .soROL_Tcp_DSts_tdata             (soROL_Nts_Tcp_DSts_tdata),
    .soROL_Tcp_DSts_tvalid            (soROL_Nts_Tcp_DSts_tvalid),
    .soROL_Tcp_DSts_tready            (soROL_Nts_Tcp_DSts_tready),

    //---------------------------------------------------
    //-- ROLE / Nts / Tcp / RxP Data Flow Interfaces    
    //---------------------------------------------------
    //-- FPGA Receive Path (NTS-->ROLE) -------------    
    //-- Stream TCP Data -----------------------         
    .soROL_Tcp_Data_tdata             (soROL_Nts_Tcp_Data_tdata),
    .soROL_Tcp_Data_tkeep             (soROL_Nts_Tcp_Data_tkeep),
    .soROL_Tcp_Data_tlast             (soROL_Nts_Tcp_Data_tlast),
    .soROL_Tcp_Data_tvalid            (soROL_Nts_Tcp_Data_tvalid),
    .soROL_Tcp_Data_tready            (soROL_Nts_Tcp_Data_tready),
    //-- Stream TCP Metadata ---------------------  
    .soROL_Tcp_Meta_tdata             (soROL_Nts_Tcp_Meta_tdata),
    .soROL_Tcp_Meta_tvalid            (soROL_Nts_Tcp_Meta_tvalid),
    .soROL_Tcp_Meta_tready            (soROL_Nts_Tcp_Meta_tready),
     //-- Stream TCP Data Notification ----------
    .soROL_Tcp_Notif_tdata            (soROL_Nts_Tcp_Notif_tdata),
    .soROL_Tcp_Notif_tvalid           (soROL_Nts_Tcp_Notif_tvalid),
    .soROL_Tcp_Notif_tready           (soROL_Nts_Tcp_Notif_tready),
    //-- Stream TCP Data Request ------------
    .siROL_Tcp_DReq_tdata             (siROL_Nts_Tcp_DReq_tdata),    
    .siROL_Tcp_DReq_tvalid            (siROL_Nts_Tcp_DReq_tvalid),
    .siROL_Tcp_DReq_tready            (siROL_Nts_Tcp_DReq_tready),
    
    //------------------------------------------------------
    //-- ROLE / Nts / Tcp / TxP Ctlr Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Transmit Path (ROLE-->ETH) -----------
    //---- Stream TCP Open Session Request -----
    .siROL_Tcp_OpnReq_tdata           (siROL_Nts_Tcp_OpnReq_tdata),
    .siROL_Tcp_OpnReq_tvalid          (siROL_Nts_Tcp_OpnReq_tvalid),
    .siROL_Tcp_OpnReq_tready          (siROL_Nts_Tcp_OpnReq_tready),
    //---- Stream TCP Open Session Status ------
    .soROL_Tcp_OpnRep_tdata           (soROL_Nts_Tcp_OpnRep_tdata),
    .soROL_Tcp_OpnRep_tvalid          (soROL_Nts_Tcp_OpnRep_tvalid),
    .soROL_Tcp_OpnRep_tready          (soROL_Nts_Tcp_OpnRep_tready),
    //---- Stream TCP Close Request ------------
    .siROL_Tcp_ClsReq_tdata           (siROL_Nts_Tcp_ClsReq_tdata),
    .siROL_Tcp_ClsReq_tvalid          (siROL_Nts_Tcp_ClsReq_tvalid),
    .siROL_Tcp_ClsReq_tready          (siROL_Nts_Tcp_ClsReq_tready),
    
    //------------------------------------------------------
    //-- ROLE / Nts / Tcp / RxP Ctlr Flow Interfaces
    //------------------------------------------------------
    //-- FPGA Receive Path (ETH-->ROLE) ------------
    //---- Stream TCP Listen Request -----------
    .siROL_Tcp_LsnReq_tdata           (siROL_Nts_Tcp_LsnReq_tdata),
    .siROL_Tcp_LsnReq_tvalid          (siROL_Nts_Tcp_LsnReq_tvalid),
    .siROL_Tcp_LsnReq_tready          (siROL_Nts_Tcp_LsnReq_tready),
    //---- Stream TCP Listen Status ------------
    .soROL_Tcp_LsnAck_tdata           (soROL_Nts_Tcp_LsnAck_tdata),
    .soROL_Tcp_LsnAck_tvalid          (soROL_Nts_Tcp_LsnAck_tvalid),
    .soROL_Tcp_LsnAck_tready          (soROL_Nts_Tcp_LsnAck_tready),
    
    //------------------------------------------------------
    //-- ROLE / Nts / Udp Interfaces
    //------------------------------------------------------
    //-- FPGA Receive Path (NTS-->ROLE) -------------
    //-- Stream UDP Data -----------------------
    .siROL_Udp_Data_tdata             (siROL_Nts_Udp_Data_tdata),
    .siROL_Udp_Data_tkeep             (siROL_Nts_Udp_Data_tkeep),
    .siROL_Udp_Data_tlast             (siROL_Nts_Udp_Data_tlast),
    .siROL_Udp_Data_tvalid            (siROL_Nts_Udp_Data_tvalid),
    .siROL_Udp_Data_tready            (siROL_Nts_Udp_Data_tready),
    //-- Output AXI-Write Stream Interface ---------
    .soROL_Udp_Data_tdata             (soROL_Nts_Udp_Data_tdata),
    .soROL_Udp_Data_tkeep             (soROL_Nts_Udp_Data_tkeep),
    .soROL_Udp_Data_tlast             (soROL_Nts_Udp_Data_tlast),
    .soROL_Udp_Data_tvalid            (soROL_Nts_Udp_Data_tvalid),
    .soROL_Udp_Data_tready            (soROL_Nts_Udp_Data_tready),
        
    //------------------------------------------------------
    //-- MMIO / Nts0 / Interfaces
    //------------------------------------------------------
    .piMMIO_MacAddress                (sMMIO_NTS0_MacAddress),
    .piMMIO_IpAddress                 (sMMIO_NTS0_IpAddress),
    .piMMIO_SubNetMask                (sMMIO_NTS0_SubNetMask),
    .piMMIO_GatewayAddr               (sMMIO_NTS0_GatewayAddr),
    .poMMIO_CamReady                  (sNTS0_MMIO_CamReady),

    .poVoid                           ()

  );  // End of NTS0


  //============================================================================
  //  INST: SYNCHRONOUS DYNAMIC RANDOM ACCESS MEMORY SUBSYSTEM
  //============================================================================
  MemorySubSystem #(

    "user",     // gSecurityPriviledges
    "user"      // gBitstreamUsage

  ) MEM (

    //-- Global Clock used by the entire SHELL -------------
    .piShlClk                         (sETH0_ShlClk),

    //-- Global Reset used by the entire SHELL -------------
    .piTOP_156_25Rst                  (piTOP_156_25Rst),

    //-- DDR4 Reference Memory Clocks ----------------------
    .piCLKT_Mem0Clk_n                 (piCLKT_Mem0Clk_n),
    .piCLKT_Mem0Clk_p                 (piCLKT_Mem0Clk_p),
    .piCLKT_Mem1Clk_n                 (piCLKT_Mem1Clk_n),
    .piCLKT_Mem1Clk_p                 (piCLKT_Mem1Clk_p),

    //-- Control Inputs and Status Ouputs ------------------
    .poMmio_Mc0_InitCalComplete       (sMEM_MMIO_Mc0InitCalComplete),
    .poMmio_Mc1_InitCalComplete       (sMEM_MMIO_Mc1InitCalComplete),

    //------------------------------------------------------
    //-- NTS0 / Mem / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS ---------------
    //---- Stream Read Command ---------------
    .piNTS0_Mem_TxP_Axis_RdCmd_tdata  (ssNTS0_MEM_TxP_RdCmd_tdata),
    .piNTS0_Mem_TxP_Axis_RdCmd_tvalid (ssNTS0_MEM_TxP_RdCmd_tvalid),
    .poMEM_Nts0_TxP_Axis_RdCmd_tready (ssNTS0_MEM_TxP_RdCmd_tready),
    //---- Stream Read Status ----------------
    .poMEM_Nts0_TxP_Axis_RdSts_tdata  (ssMEM_NTS0_TxP_RdSts_tdata),
    .poMEM_Nts0_TxP_Axis_RdSts_tvalid (ssMEM_NTS0_TxP_RdSts_tvalid),
    .piNTS0_Mem_TxP_Axis_RdSts_tready (ssMEM_NTS0_TxP_RdSts_tready),
    //---- Stream Data Output Channel --------
    .poMEM_Nts0_TxP_Axis_Read_tdata   (ssMEM_NTS0_TxP_Read_tdata),
    .poMEM_Nts0_TxP_Axis_Read_tkeep   (ssMEM_NTS0_TxP_Read_tkeep),
    .poMEM_Nts0_TxP_Axis_Read_tlast   (ssMEM_NTS0_TxP_Read_tlast),
    .poMEM_Nts0_TxP_Axis_Read_tvalid  (ssMEM_NTS0_TxP_Read_tvalid),
    .piNTS0_Mem_TxP_Axis_Read_tready  (ssMEM_NTS0_TxP_Read_tready),
    //---- Stream Write Command --------------
    .piNTS0_Mem_TxP_Axis_WrCmd_tdata  (ssNTS0_MEM_TxP_WrCmd_tdata),
    .piNTS0_Mem_TxP_Axis_WrCmd_tvalid (ssNTS0_MEM_TxP_WrCmd_tvalid),
    .poMEM_Nts0_TxP_Axis_WrCmd_tready (ssNTS0_MEM_TxP_WrCmd_tready),
    //---- Stream Write Status --------------
    .poMEM_Nts0_TxP_Axis_WrSts_tdata  (ssMEM_NTS0_TxP_WrSts_tdata),
    .poMEM_Nts0_TxP_Axis_WrSts_tvalid (ssMEM_NTS0_TxP_WrSts_tvalid),
    .piNTS0_Mem_TxP_Axis_WrSts_tready (ssMEM_NTS0_TxP_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .piNTS0_Mem_TxP_Axis_Write_tdata  (ssNTS0_MEM_TxP_Write_tdata),
    .piNTS0_Mem_TxP_Axis_Write_tkeep  (ssNTS0_MEM_TxP_Write_tkeep),
    .piNTS0_Mem_TxP_Axis_Write_tlast  (ssNTS0_MEM_TxP_Write_tlast),
    .piNTS0_Mem_TxP_Axis_Write_tvalid (ssNTS0_MEM_TxP_Write_tvalid),
    .poMEM_Nts0_TxP_Axis_Write_tready (ssNTS0_MEM_TxP_Write_tready),

    //------------------------------------------------------
    //-- NTS0 / Mem / Rx Interface
    //------------------------------------------------------
    //-- Receive Path  / S2MM-AXIS -----------------
    //---- Stream Read Command ---------------
    .piNTS0_Mem_RxP_Axis_RdCmd_tdata  (ssNTS0_MEM_RxP_RdCmd_tdata),
    .piNTS0_Mem_RxP_Axis_RdCmd_tvalid (ssNTS0_MEM_RxP_RdCmd_tvalid),
    .poMEM_Nts0_RxP_Axis_RdCmd_tready (ssNTS0_MEM_RxP_RdCmd_tready),
    //---- Stream Read Status ----------------
    .poMEM_Nts0_RxP_Axis_RdSts_tdata  (ssMEM_NTS0_RxP_RdSts_tdata),
    .poMEM_Nts0_RxP_Axis_RdSts_tvalid (ssMEM_NTS0_RxP_RdSts_tvalid),
    .piNTS0_Mem_RxP_Axis_RdSts_tready (ssMEM_NTS0_RxP_RdSts_tready),
    //---- Stream Data Output Channel --------
    .poMEM_Nts0_RxP_Axis_Read_tdata   (ssMEM_NTS0_RxP_Read_tdata),
    .poMEM_Nts0_RxP_Axis_Read_tkeep   (ssMEM_NTS0_RxP_Read_tkeep),
    .poMEM_Nts0_RxP_Axis_Read_tlast   (ssMEM_NTS0_RxP_Read_tlast),
    .poMEM_Nts0_RxP_Axis_Read_tvalid  (ssMEM_NTS0_RxP_Read_tvalid),
    .piNTS0_Mem_RxP_Axis_Read_tready  (ssMEM_NTS0_RxP_Read_tready),
    //---- Stream Write Command --------------
    .piNTS0_Mem_RxP_Axis_WrCmd_tdata  (ssNTS0_MEM_RxP_WrCmd_tdata),
    .piNTS0_Mem_RxP_Axis_WrCmd_tvalid (ssNTS0_MEM_RxP_WrCmd_tvalid),
    .poMEM_Nts0_RxP_Axis_WrCmd_tready (ssNTS0_MEM_RxP_WrCmd_tready),
    //---- Stream Write Status ---------------
    .poMEM_Nts0_RxP_Axis_WrSts_tdata  (ssMEM_NTS0_RxP_WrSts_tdata),
    .poMEM_Nts0_RxP_Axis_WrSts_tvalid (ssMEM_NTS0_RxP_WrSts_tvalid),
    .piNTS0_Mem_RxP_Axis_WrSts_tready (ssMEM_NTS0_RxP_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .piNTS0_Mem_RxP_Axis_Write_tdata  (ssNTS0_MEM_RxP_Write_tdata),
    .piNTS0_Mem_RxP_Axis_Write_tkeep  (ssNTS0_MEM_RxP_Write_tkeep),
    .piNTS0_Mem_RxP_Axis_Write_tlast  (ssNTS0_MEM_RxP_Write_tlast),
    .piNTS0_Mem_RxP_Axis_Write_tvalid (ssNTS0_MEM_RxP_Write_tvalid),
    .poMEM_Nts0_RxP_Axis_Write_tready (ssNTS0_MEM_RxP_Write_tready),  

    //------------------------------------------------------
    // -- Physical DDR4 Interface #0
    //------------------------------------------------------
    .pioDDR_Mem_Mc0_DmDbi_n           (pioDDR4_Mem_Mc0_DmDbi_n),
    .pioDDR_Mem_Mc0_Dq                (pioDDR4_Mem_Mc0_Dq),
    .pioDDR_Mem_Mc0_Dqs_n             (pioDDR4_Mem_Mc0_Dqs_n),
    .pioDDR_Mem_Mc0_Dqs_p             (pioDDR4_Mem_Mc0_Dqs_p),    
    .poMEM_Ddr4_Mc0_Act_n             (poDDR4_Mem_Mc0_Act_n),
    .poMEM_Ddr4_Mc0_Adr               (poDDR4_Mem_Mc0_Adr),
    .poMEM_Ddr4_Mc0_Ba                (poDDR4_Mem_Mc0_Ba),
    .poMEM_Ddr4_Mc0_Bg                (poDDR4_Mem_Mc0_Bg),
    .poMEM_Ddr4_Mc0_Cke               (poDDR4_Mem_Mc0_Cke),
    .poMEM_Ddr4_Mc0_Odt               (poDDR4_Mem_Mc0_Odt),
    .poMEM_Ddr4_Mc0_Cs_n              (poDDR4_Mem_Mc0_Cs_n),
    .poMEM_Ddr4_Mc0_Ck_n              (poDDR4_Mem_Mc0_Ck_n),
    .poMEM_Ddr4_Mc0_Ck_p              (poDDR4_Mem_Mc0_Ck_p),
    .poMEM_Ddr4_Mc0_Reset_n           (poDDR4_Mem_Mc0_Reset_n),

    //------------------------------------------------------
    //-- ROLE / Mem / Mp0 Interface
    //------------------------------------------------------
    //-- Memory Port #0 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .piROL_Mem_Mp0_Axis_RdCmd_tdata   (siROL_Mem_Mp0_RdCmd_tdata),
    .piROL_Mem_Mp0_Axis_RdCmd_tvalid  (siROL_Mem_Mp0_RdCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_RdCmd_tready  (siROL_Mem_Mp0_RdCmd_tready),
    //---- Stream Read Status ----------------
    .poMEM_Rol_Mp0_Axis_RdSts_tdata   (soROL_Mem_Mp0_RdSts_tdata),
    .poMEM_Rol_Mp0_Axis_RdSts_tvalid  (soROL_Mem_Mp0_RdSts_tvalid),
    .piROL_Mem_Mp0_Axis_RdSts_tready  (soROL_Mem_Mp0_RdSts_tready),
    //---- Stream Data Output Channel --------
    .poMEM_Rol_Mp0_Axis_Read_tdata    (soROL_Mem_Mp0_Read_tdata),
    .poMEM_Rol_Mp0_Axis_Read_tkeep    (soROL_Mem_Mp0_Read_tkeep),
    .poMEM_Rol_Mp0_Axis_Read_tlast    (soROL_Mem_Mp0_Read_tlast),
    .poMEM_Rol_Mp0_Axis_Read_tvalid   (soROL_Mem_Mp0_Read_tvalid),
    .piROL_Mem_Mp0_Axis_Read_tready   (soROL_Mem_Mp0_Read_tready),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp0_Axis_WrCmd_tdata   (siROL_Mem_Mp0_WrCmd_tdata),
    .piROL_Mem_Mp0_Axis_WrCmd_tvalid  (siROL_Mem_Mp0_WrCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_WrCmd_tready  (siROL_Mem_Mp0_WrCmd_tready),
    //---- Stream Write Status ---------------
    .poMEM_Rol_Mp0_Axis_WrSts_tdata   (soROL_Mem_Mp0_WrSts_tdata),
    .poMEM_Rol_Mp0_Axis_WrSts_tvalid  (soROL_Mem_Mp0_WrSts_tvalid),
    .piROL_Mem_Mp0_Axis_WrSts_tready  (soROL_Mem_Mp0_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp0_Axis_Write_tdata   (siROL_Mem_Mp0_Write_tdata),
    .piROL_Mem_Mp0_Axis_Write_tkeep   (siROL_Mem_Mp0_Write_tkeep),
    .piROL_Mem_Mp0_Axis_Write_tlast   (siROL_Mem_Mp0_Write_tlast),
    .piROL_Mem_Mp0_Axis_Write_tvalid  (siROL_Mem_Mp0_Write_tvalid),
    .poMEM_Rol_Mp0_Axis_Write_tready  (siROL_Mem_Mp0_Write_tready),

    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    //-- Memory Port #1 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .piROL_Mem_Mp1_Axis_RdCmd_tdata   (siROL_Mem_Mp1_RdCmd_tdata),
    .piROL_Mem_Mp1_Axis_RdCmd_tvalid  (siROL_Mem_Mp1_RdCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_RdCmd_tready  (siROL_Mem_Mp1_RdCmd_tready),
    //---- Stream Read Status ----------------
    .poMEM_Rol_Mp1_Axis_RdSts_tdata   (soROL_Mem_Mp1_RdSts_tdata),
    .poMEM_Rol_Mp1_Axis_RdSts_tvalid  (soROL_Mem_Mp1_RdSts_tvalid),
    .piROL_Mem_Mp1_Axis_RdSts_tready  (soROL_Mem_Mp1_RdSts_tready),
    //---- Stream Data Output Channel --------
    .poMEM_Rol_Mp1_Axis_Read_tdata    (soROL_Mem_Mp1_Read_tdata),
    .poMEM_Rol_Mp1_Axis_Read_tkeep    (soROL_Mem_Mp1_Read_tkeep),
    .poMEM_Rol_Mp1_Axis_Read_tlast    (soROL_Mem_Mp1_Read_tlast),
    .poMEM_Rol_Mp1_Axis_Read_tvalid   (soROL_Mem_Mp1_Read_tvalid),
    .piROL_Mem_Mp1_Axis_Read_tready   (soROL_Mem_Mp1_Read_tready),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp1_Axis_WrCmd_tdata   (siROL_Mem_Mp1_WrCmd_tdata),
    .piROL_Mem_Mp1_Axis_WrCmd_tvalid  (siROL_Mem_Mp1_WrCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_WrCmd_tready  (siROL_Mem_Mp1_WrCmd_tready),
    //---- Stream Write Status ---------------
    .poMEM_Rol_Mp1_Axis_WrSts_tdata   (soROL_Mem_Mp1_WrSts_tdata),
    .poMEM_Rol_Mp1_Axis_WrSts_tvalid  (soROL_Mem_Mp1_WrSts_tvalid),
    .piROL_Mem_Mp1_Axis_WrSts_tready  (soROL_Mem_Mp1_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp1_Axis_Write_tdata   (siROL_Mem_Mp1_Write_tdata),
    .piROL_Mem_Mp1_Axis_Write_tkeep   (siROL_Mem_Mp1_Write_tkeep),
    .piROL_Mem_Mp1_Axis_Write_tlast   (siROL_Mem_Mp1_Write_tlast),
    .piROL_Mem_Mp1_Axis_Write_tvalid  (siROL_Mem_Mp1_Write_tvalid),
    .poMEM_Rol_Mp1_Axis_Write_tready  (siROL_Mem_Mp1_Write_tready),

    //------------------------------------------------------
    // -- Physical DDR4 Interface #1
    //------------------------------------------------------
    .pioDDR_Mem_Mc1_DmDbi_n           (pioDDR4_Mem_Mc1_DmDbi_n),
    .pioDDR_Mem_Mc1_Dq                (pioDDR4_Mem_Mc1_Dq),
    .pioDDR_Mem_Mc1_Dqs_n             (pioDDR4_Mem_Mc1_Dqs_n),
    .pioDDR_Mem_Mc1_Dqs_p             (pioDDR4_Mem_Mc1_Dqs_p),
    .poMEM_Ddr4_Mc1_Act_n             (poDDR4_Mem_Mc1_Act_n),
    .poMEM_Ddr4_Mc1_Adr               (poDDR4_Mem_Mc1_Adr),
    .poMEM_Ddr4_Mc1_Ba                (poDDR4_Mem_Mc1_Ba),
    .poMEM_Ddr4_Mc1_Bg                (poDDR4_Mem_Mc1_Bg),
    .poMEM_Ddr4_Mc1_Cke               (poDDR4_Mem_Mc1_Cke),
    .poMEM_Ddr4_Mc1_Odt               (poDDR4_Mem_Mc1_Odt),
    .poMEM_Ddr4_Mc1_Cs_n              (poDDR4_Mem_Mc1_Cs_n),
    .poMEM_Ddr4_Mc1_Ck_n              (poDDR4_Mem_Mc1_Ck_n),
    .poMEM_Ddr4_Mc1_Ck_p              (poDDR4_Mem_Mc1_Ck_p),
    .poMEM_Ddr4_Mc1_Reset_n           (poDDR4_Mem_Mc1_Reset_n),

    .poVoid                           ()

  );  // End of MEM


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

  assign poTOP_Led_HeartBeat_n = ~sLed_HeartBeat; // LED is active low  


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poROL_156_25Clk = sETH0_ShlClk;
  assign poROL_156_25Rst = sETH0_ShlRst;

  //============================================================================
  //  LIST OF HDL PORTS TO BE MARKED FOR DEBUGING
  //============================================================================

  //-- ETH0 ==> NTS0 / AXIS Interface ---------------------------- 
  //(* mark_debug = "true" *)  wire  [ 63:0]  sETH0_Nts0_Axis_tdata;
  //(* mark_debug = "true" *)  wire  [ 7:0]   sETH0_Nts0_Axis_tkeep;
  //(* mark_debug = "true" *)  wire           sETH0_Nts0_Axis_tlast;
  //(* mark_debug = "true" *)  wire           sETH0_Nts0_Axis_tvalid;
  //(* mark_debug = "true" *)  wire           sNTS0_Eth0_Axis_tready;
  //-- ETHERNET / Nts0 / Output AXIS Interface ---------------------- 
  //(* mark_debug = "true" *)  wire  [ 63:0]  sNTS0_Eth0_Axis_tdata;
  //(* mark_debug = "true" *)  wire  [  7:0]  sNTS0_Eth0_Axis_tkeep;
  //(* mark_debug = "true" *)  wire           sNTS0_Eth0_Axis_tlast;
  //(* mark_debug = "true" *)  wire           sNTS0_Eth0_Axis_tvalid;
  //(* mark_debug = "true" *)  wire           sETH0_Nts0_Axis_tready;

  //============================================================================
  //  VIO FOR HARDWARE BRING-UP AND DEBUG
  //============================================================================
  //  VirtualInputOutput_IP_0 VIO0 (
  //    .clk        (sSD4MI_Ui_Clk),
  //    .probe_in0  (piPSOC_Fcfg_Rst_n),                
  //    .probe_in1  (sMC0_InitCalibComplete),
  //    .probe_in2  (sDataCompareError),
  //    .probe_in3  (poSHL_Led_HeartBeat_n)
  //  );

endmodule
