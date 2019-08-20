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
  //------------------------------------------------------------------
  //-- URIF = USER-ROLE-INTERFACE
  //------------------------------------------------------------------
  //-- URIF ==> UDMX / OpenPortRequest -
  wire  [15:0]  ssURIF_UDMX_OpnReq_tdata;
  wire          ssURIF_UDMX_OpnReq_tvalid;
  wire          ssURIF_UDMX_OpnReq_tready;
  //-- UDMX ==> URIF / Open Port Acknowledge -----
  wire  [ 7:0]  ssUDMX_URIF_OpnAck_tdata;
  wire          ssUDMX_URIF_OpnAck_tvalid;
  wire          ssUDMX_URIF_OpnAck_tready;
  //-- URIF ==> UDMX / Data ------------
  wire  [63:0]  ssURIF_UDMX_Data_tdata;
  wire  [ 7:0]  ssURIF_UDMX_Data_tkeep;
  wire          ssURIF_UDMX_Data_tlast;
  wire          ssURIF_UDMX_Data_tvalid;
  wire          ssURIF_UDMX_Data_tready;
  //-- URIF ==> UDMX / Meta ------------
  wire  [95:0]  ssURIF_UDMX_Meta_tdata;
  wire          ssURIF_UDMX_Meta_tvalid;
  wire          ssURIF_UDMX_Meta_tready;
  //-- URIF ==> UDMX / TxLen -----------
  wire  [15:0]  ssURIF_UDMX_PLen_tdata;
  wire          ssURIF_UDMX_PLen_tvalid;
  wire          ssURIF_UDMX_PLen_tready;
  //-- URIF ==>[ARS6]==> ROLE / Data -------------
  //-- URIF ==>[ARS6]
  wire  [63:0]  ssURIF_ARS6_Data_tdata;
  wire  [ 7:0]  ssURIF_ARS6_Data_tkeep;
  wire          ssURIF_ARS6_Data_tlast;
  wire          ssURIF_ARS6_Data_tvalid;
  wire          ssURIF_ARS6_Data_tready;
  //-- UDMX ==> URIF / Data ----------------------
  wire  [63:0]  ssUDMX_URIF_Data_tdata;
  wire  [ 7:0]  ssUDMX_URIF_Data_tkeep;
  wire          ssUDMX_URIF_Data_tlast;
  wire          ssUDMX_URIF_Data_tvalid;
  wire          ssUDMX_URIF_Data_tready;
  //-- UDMX ==> URIF / Meta ----------------------
  wire  [95:0]  ssUDMX_URIF_Meta_tdata;
  wire          ssUDMX_URIF_Meta_tvalid;
  wire          ssUDMX_URIF_Meta_tready;
  //-- ROLE ==>[ARS7]=> URIF --------------------
  //---- ROLE ==> [ARS7] (see siROL_Udp_Data_t*)
  //----          [ARS7]==> URIF -------
  wire  [63:0]  ssARS7_URIF_Data_tdata;
  wire  [ 7:0]  ssARS7_URIF_Data_tkeep;
  wire          ssARS7_URIF_Data_tlast;
  wire          ssARS7_URIF_Data_tvalid;
  wire          ssARS7_URIF_Data_tready;


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
  wire          sNTS0_MMIO_ToeReady;
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
  
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : HWICAPC 
  //--------------------------------------------------------
  wire [ 8:0] ssFMC_HWICAP_Axi_awaddr;
  wire        ssFMC_HWICAP_Axi_awvalid;
  wire        ssFMC_HWICAP_Axi_awready;
  wire [31:0] ssFMC_HWICAP_Axi_wdata;
  wire        ssFMC_HWICAP_Axi_wstrb;
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
  wire        sDECOUP_FMC_status;
  wire        sFMC_DECOUP_activate;
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
  // FMC <==> CoreToDebug PYROLINK 
  wire [ 7:0] ssFMC_CoreToDebug_Pyrolink_TDATA;
  wire        ssFMC_CoreToDebug_Pyrolink_TVALID;
  wire        ssFMC_CoreToDebug_Pyrolink_TREADY;
  wire        ssFMC_CoreToDebug_Pyrolink_TKEEP;
  wire        ssFMC_CoreToDebug_Pyrolink_TLAST;
  wire [ 7:0] ssCoreToDebug_FMC_Pyrolink_TDATA;
  wire        ssCoreToDebug_FMC_Pyrolink_TVALID;
  wire        ssCoreToDebug_FMC_Pyrolink_TREADY;
  wire        ssCoreToDebug_FMC_Pyrolink_TKEEP;
  wire        ssCoreToDebug_FMC_Pyrolink_TLAST;


 
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
    .piNTS0_ToeReady                (sNTS0_MMIO_ToeReady),
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
    //-- NRC :  Control Registers
    //----------------------------------------------
    //---- MNGT_RMIP -------------------
    .poNRC_RmIpAddress              (),  // [TODO - Not yet used by this SHELL]
    //---- MNGT_TCPLSN -----------------
    .poNRC_TcpLsnPort               (),  // [TODO - Not yet used by this SHELL]

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
    //-- MEM / TxP Interfaces
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
    //-- MEM / RxP Interfaces
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
    //-- NRC/Role / Nts0 / Udp Interfaces
    //------------------------------------------------------
    //-- UDMX ==> URIF / Open Port Acknowledge -----
    .soROL_Udp_OpnAck_tdata         (ssUDMX_URIF_OpnAck_tdata),
    .soROL_Udp_OpnAck_tvalid        (ssUDMX_URIF_OpnAck_tvalid),
    .soROL_Udp_OpnAck_tready        (ssUDMX_URIF_OpnAck_tready),
    //-- UDMX ==> URIF / Data ----------------------
    .soROL_Udp_Data_tdata           (ssUDMX_URIF_Data_tdata),
    .soROL_Udp_Data_tkeep           (ssUDMX_URIF_Data_tkeep),
    .soROL_Udp_Data_tlast           (ssUDMX_URIF_Data_tlast),
    .soROL_Udp_Data_tvalid          (ssUDMX_URIF_Data_tvalid),
    .soROL_Udp_Data_tready          (ssUDMX_URIF_Data_tready),
    //-- UDMX ==> URIF / Meta ----------------------
    .soROL_Udp_Meta_tdata           (ssUDMX_URIF_Meta_tdata),
    .soROL_Udp_Meta_tvalid          (ssUDMX_URIF_Meta_tvalid),
    .soROL_Udp_Meta_tready          (ssUDMX_URIF_Meta_tready),
    //-- URIF ==> UDMX / OpenPortRequest / Axis ----
    .siROL_Udp_OpnReq_tdata         (ssURIF_UDMX_OpnReq_tdata),
    .siROL_Udp_OpnReq_tvalid        (ssURIF_UDMX_OpnReq_tvalid),
    .siROL_Udp_OpnReq_tready        (ssURIF_UDMX_OpnReq_tready),
    //-- URIF ==> UDMX / Data / Axis ---------------
    .siROL_Udp_Data_tdata           (ssURIF_UDMX_Data_tdata),
    .siROL_Udp_Data_tkeep           (ssURIF_UDMX_Data_tkeep),
    .siROL_Udp_Data_tlast           (ssURIF_UDMX_Data_tlast),
    .siROL_Udp_Data_tvalid          (ssURIF_UDMX_Data_tvalid),
    .siROL_Udp_Data_tready          (ssURIF_UDMX_Data_tready),
    //-- URIF ==> UDMX / Meta / Axis ---------------
    .siROL_Udp_Meta_tdata           (ssURIF_UDMX_Meta_tdata),
    .siROL_Udp_Meta_tvalid          (ssURIF_UDMX_Meta_tvalid),
    .siROL_Udp_Meta_tready          (ssURIF_UDMX_Meta_tready),
    //-- URIF ==> UDMX / TxLen / Axis --------------
    .siROL_Udp_PLen_tdata           (ssURIF_UDMX_PLen_tdata),
    .siROL_Udp_PLen_tvalid          (ssURIF_UDMX_PLen_tvalid),
    .siROL_Udp_PLen_tready          (ssURIF_UDMX_PLen_tready),

    //------------------------------------------------------
    //-- ROLE / Tcp / TxP Data Flow Interfaces
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
    //-- ROLE / Tcp / RxP Data Flow Interfaces    
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
    //-- ROLE / Tcp / TxP Ctlr Flow Interfaces
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
    //-- ROLE / Tcp / RxP Ctlr Flow Interfaces
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
    //-- MMIO / Interfaces
    //------------------------------------------------------
    .piMMIO_MacAddress                (sMMIO_NTS0_MacAddress),
    .piMMIO_IpAddress                 (sMMIO_NTS0_IpAddress),
    .piMMIO_SubNetMask                (sMMIO_NTS0_SubNetMask),
    .piMMIO_GatewayAddr               (sMMIO_NTS0_GatewayAddr),
    .poMMIO_CamReady                  (sNTS0_MMIO_CamReady),
    .poMMIO_ToeReady                  (sNTS0_MMIO_ToeReady),

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
    //------------------------------------------------------dst_ran
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

  //============================================================================
  //  INST: FPGA MANAGEMENT CORE
  //============================================================================
  FpgaManagementCore FMC (
    //-- Global Clock used by the entire SHELL -------------
    .ap_clk                 (sETH0_ShlClk),
    //-- Global Reset used by the entire SHELL -------------
    .ap_rst_n               (~ piTOP_156_25Rst),
    //core should start immediately 
    .ap_start               (1),
    //.piSysReset_V           (piSHL_156_25Rst_delayed),
    //.piSysReset_V_ap_vld   (1),
    //.poMMIO_V_ap_vld     ( ),
    .piMMIO_V              (sMMIO_FMC_WrFmcReg),
    .piMMIO_V_ap_vld        (1),
    .poMMIO_V              (sFMC_MMIO_RdFmcReg),
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
    .m_axi_boHWICAP_RRESP    (ssFMC_HWICAP_Axi_rresp),
    .m_axi_boHWICAP_RVALID   (ssFMC_HWICAP_Axi_rvalid),
    .m_axi_boHWICAP_RREADY   (ssFMC_HWICAP_Axi_rready),
    .piDECOUP_status_V                   (sDECOUP_FMC_status),
    .poDECOUP_activate_V                 (sFMC_DECOUP_activate),
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
    .m_axi_boNRC_ctrlLink_RRESP          (ssFMC_NRC_ctrlLink_Axi_RRESP),
    .m_axi_boNRC_ctrlLink_BVALID         (ssFMC_NRC_ctrlLink_Axi_BVALID),
    .m_axi_boNRC_ctrlLink_BREADY         (ssFMC_NRC_ctrlLink_Axi_BREADY),
    .m_axi_boNRC_ctrlLink_BRESP          (ssFMC_NRC_ctrlLink_Axi_BRESP),
    .piDisableCtrlLink_V                 (1),//TODO: SET to 0 IF NRC IS PRESENT
    //.piDisableCtrlLink_V_ap_vld          (1),
    .soPYROLINK_TDATA                    (ssFMC_CoreToDebug_Pyrolink_TDATA),
    .soPYROLINK_TVALID                   (ssFMC_CoreToDebug_Pyrolink_TVALID),
    .soPYROLINK_TREADY                   (ssFMC_CoreToDebug_Pyrolink_TREADY),
    .soPYROLINK_TKEEP                    (ssFMC_CoreToDebug_Pyrolink_TKEEP),
    .soPYROLINK_TLAST                    (ssFMC_CoreToDebug_Pyrolink_TLAST),
    .siPYROLINK_TDATA                    (ssCoreToDebug_FMC_Pyrolink_TDATA),
    .siPYROLINK_TVALID                   (ssCoreToDebug_FMC_Pyrolink_TVALID),
    .siPYROLINK_TREADY                   (ssCoreToDebug_FMC_Pyrolink_TREADY),
    .siPYROLINK_TKEEP                    (ssCoreToDebug_FMC_Pyrolink_TKEEP),
    .siPYROLINK_TLAST                    (ssCoreToDebug_FMC_Pyrolink_TLAST),
    .piDisablePyroLink_V                 (1),//TODO: SET to 0 IF PYROLINK SHOULD BE USED
    //.piDisablePyroLink_V_ap_vld          (1),
    .poROLE_rank_V                       (poROL_Fmc_Rank),
    .poROLE_size_V                       (poROL_Fmc_Size)
  );

  // == Temporary assignment (until NRC module is back) ==
  assign ssFMC_NRC_ctrlLink_Axi_AWREADY = 0;
  assign ssFMC_NRC_ctrlLink_Axi_WREADY  = 0;
  //assign ssFMC_NRC_ctrlLink_Axi_BID     = 0;
  assign ssFMC_NRC_ctrlLink_Axi_BRESP   = 0;
  //assign ssFMC_NRC_ctrlLink_Axi_BUSER   = 0;
  assign ssFMC_NRC_ctrlLink_Axi_BVALID  = 0;
  assign ssFMC_NRC_ctrlLink_Axi_ARREADY = 0;
  assign ssFMC_NRC_ctrlLink_Axi_BREADY  = 0;
  assign ssFMC_NRC_ctrlLink_Axi_RDATA   = 0;
  assign ssFMC_NRC_ctrlLink_Axi_RRESP   = 0;
  //assign ssFMC_NRC_ctrlLink_Axi_RLAST   = 0;
  //assign ssFMC_NRC_ctrlLink_Axi_RUSER   = 0;
  assign ssFMC_NRC_ctrlLink_Axi_RVALID  = 0;

  // == propagate constants as long as PYROLINK isn't in use
  assign ssFMC_CoreToDebug_Pyrolink_TREADY   = 0;
  assign ssCoreToDebug_FMC_Pyrolink_TDATA    = 0;
  assign ssCoreToDebug_FMC_Pyrolink_TVALID   = 0;
  assign ssCoreToDebug_FMC_Pyrolink_TKEEP    = 0;
  assign ssCoreToDebug_FMC_Pyrolink_TLAST    = 0;
  
  // == Temporary assignment (until Decoupeling module is back) == 
  assign sDECOUP_FMC_status = 0;


  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (URIF ==>[ARS6]==> ROLE/Udp/Data)
  //============================================================================
  AxisRegisterSlice_64 ARS6 (
    .aclk          (sETH0_ShlClk),
    .aresetn       (~piTOP_156_25Rst),
    // From URIF / Data ---------------
    .s_axis_tdata  (ssURIF_ARS6_Data_tdata),
    .s_axis_tkeep  (ssURIF_ARS6_Data_tkeep),
    .s_axis_tlast  (ssURIF_ARS6_Data_tlast),
    .s_axis_tvalid (ssURIF_ARS6_Data_tvalid),
    .s_axis_tready (ssURIF_ARS6_Data_tready),     
    //-- To ROLE / Udp / Data --------
    .m_axis_tdata  (soROL_Nts_Udp_Data_tdata),
    .m_axis_tkeep  (soROL_Nts_Udp_Data_tkeep),
    .m_axis_tlast  (soROL_Nts_Udp_Data_tlast),
    .m_axis_tvalid (soROL_Nts_Udp_Data_tvalid),
    .m_axis_tready (soROL_Nts_Udp_Data_tready)
  );
  
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (ROLE/Udp/Data ==>[ARS7]==> URIF)
  //============================================================================
  AxisRegisterSlice_64 ARS7 (
    .aclk          (sETH0_ShlClk),
    .aresetn       (~piTOP_156_25Rst),
    //-- From ROLE / Udp / Data -------
    .s_axis_tdata   (siROL_Nts_Udp_Data_tdata),
    .s_axis_tkeep   (siROL_Nts_Udp_Data_tkeep),
    .s_axis_tlast   (siROL_Nts_Udp_Data_tlast),
    .s_axis_tvalid  (siROL_Nts_Udp_Data_tvalid),
    .s_axis_tready  (soROL_Nts_Udp_Data_tready),
    //-- To ARS7 / Data ----------------
    .m_axis_tdata   (ssARS7_URIF_Data_tdata),
    .m_axis_tkeep   (ssARS7_URIF_Data_tkeep),
    .m_axis_tlast   (ssARS7_URIF_Data_tlast),
    .m_axis_tvalid  (ssARS7_URIF_Data_tvalid),
    .m_axis_tready  (ssARS7_URIF_Data_tready)
  );
  
  //============================================================================
  //  INST: UDP-ROLE-INTERFACE
  //============================================================================
//`ifdef USE_DEPRECATED_DIRECTIVES TODO: fix different pragma versions
//  
  UdpRoleInterface URIF (

  .aclk                           (sETH0_ShlClk),
  .aresetn                        (~piTOP_156_25Rst),
  
  //------------------------------------------------------
  //-- ROLE / Udp / TxP Data Flow Interfaces
  //------------------------------------------------------
  //-- From ROLE / Udp ==>[ARS7] / Data
  .siROL_This_Data_TDATA          (ssARS7_URIF_Data_tdata),
  .siROL_This_Data_TKEEP          (ssARS7_URIF_Data_tkeep),
  .siROL_This_Data_TLAST          (ssARS7_URIF_Data_tlast),
  .siROL_This_Data_TVALID         (ssARS7_URIF_Data_tvalid),
  .siROL_This_Data_TREADY         (ssARS7_URIF_Data_tready),
  //-- To   ROLE ==[ARS6] / Udp / Data
  .soTHIS_Rol_Data_TDATA          (ssURIF_ARS6_Data_tdata),
  .soTHIS_Rol_Data_TKEEP          (ssURIF_ARS6_Data_tkeep),
  .soTHIS_Rol_Data_TLAST          (ssURIF_ARS6_Data_tlast),
  .soTHIS_Rol_Data_TVALID         (ssURIF_ARS6_Data_tvalid),
  .soTHIS_Rol_Data_TREADY         (ssURIF_ARS6_Data_tready),

  //------------------------------------------------------
  //-- UDMX / Ctrl Flow Interfaces
  //------------------------------------------------------
  //-- To   UDM / Open Port Request ----
  .soTHIS_Udmx_OpnReq_TDATA       (ssURIF_UDMX_OpnReq_tdata),
  .soTHIS_Udmx_OpnReq_TVALID      (ssURIF_UDMX_OpnReq_tvalid),
  .soTHIS_Udmx_OpnReq_TREADY      (ssURIF_UDMX_OpnReq_tready),
  //-- From UDMX / Open Port Acknowledgment
  .siUDMX_This_OpnAck_TDATA       (ssUDMX_URIF_OpnAck_tdata),
  .siUDMX_This_OpnAck_TVALID      (ssUDMX_URIF_OpnAck_tvalid),
  .siUDMX_This_OpnAck_TREADY      (ssUDMX_URIF_OpnAck_tready),

  //------------------------------------------------------
  //-- UDMX / UDP Data & MetaData Interfaces
  //------------------------------------------------------
  //-- From UDMX / Data ----------------
  .siUDMX_This_Data_TDATA         (ssUDMX_URIF_Data_tdata),
  .siUDMX_This_Data_TKEEP         (ssUDMX_URIF_Data_tkeep),
  .siUDMX_This_Data_TLAST         (ssUDMX_URIF_Data_tlast),
  .siUDMX_This_Data_TVALID        (ssUDMX_URIF_Data_tvalid),
  .siUDMX_This_Data_TREADY        (ssUDMX_URIF_Data_tready),
   //-- From / MetaData ----------------
  .siUDMX_This_Meta_TDATA         (ssUDMX_URIF_Meta_tdata),
  .siUDMX_This_Meta_TVALID        (ssUDMX_URIF_Meta_tvalid),
  .siUDMX_This_Meta_TREADY        (ssUDMX_URIF_Meta_tready),
   //-- To   UDMX / Data ---------------  
  .soTHIS_Udmx_Data_TDATA         (ssURIF_UDMX_Data_tdata),
  .soTHIS_Udmx_Data_TKEEP         (ssURIF_UDMX_Data_tkeep),
  .soTHIS_Udmx_Data_TLAST         (ssURIF_UDMX_Data_tlast),
  .soTHIS_Udmx_Data_TVALID        (ssURIF_UDMX_Data_tvalid),
  .soTHIS_Udmx_Data_TREADY        (ssURIF_UDMX_Data_tready),
  //-- To   UDMX / MetaData ------------
  .soTHIS_Udmx_Meta_TDATA         (ssURIF_UDMX_Meta_tdata),
  .soTHIS_Udmx_Meta_TVALID        (ssURIF_UDMX_Meta_tvalid),
  .soTHIS_Udmx_Meta_TREADY        (ssURIF_UDMX_Meta_tready),
  //-- To   UDMX / Length --------------
  .soTHIS_Udmx_PLen_TDATA         (ssURIF_UDMX_PLen_tdata),
  .soTHIS_Udmx_PLen_TVALID        (ssURIF_UDMX_PLen_tvalid),
  .soTHIS_Udmx_PLen_TREADY        (ssURIF_UDMX_PLen_tready)

);

//`else // !`ifdef USE_DEPRECATED_DIRECTIVES
// 
//  UdpRoleInterface URIF (
//  
//    .ap_clk                         (piShlClk),                      
//    .ap_rst_n                       (~piShlRst),
//    
//    //------------------------------------------------------
//    //-- From ROLE Interfaces
//    //------------------------------------------------------
//    //-- ROLE / This / Udp / Axis
//    .siROL_This_Data_TDATA          (sROL_Nts0_Udp_Axis_tdataReg),
//    .siROL_This_Data_TKEEP          (sROL_Nts0_Udp_Axis_tkeepReg),
//    .siROL_This_Data_TLAST          (sROL_Nts0_Udp_Axis_tlastReg),
//    .siROL_This_Data_TVALID         (sROL_Nts0_Udp_Axis_tvalidReg),
//    .siROL_This_Data_TREADY         (sURIF_Rol_Axis_tready),
//    
//    //------------------------------------------------------
//    //-- To ROLE Interfaces
//    //------------------------------------------------------
//    //-- THIS / Role / Udp / Axis Output Interface
//    .soTHIS_Rol_Data_TREADY         (sROL_Urif_Axis_treadyReg),
//    .soTHIS_Rol_Data_TDATA          (sURIF_Rol_Axis_tdata),
//    .soTHIS_Rol_Data_TKEEP          (sURIF_Rol_Axis_tkeep),
//    .soTHIS_Rol_Data_TLAST          (sURIF_Rol_Axis_tlast),
//    .soTHIS_Rol_Data_TVALID         (sURIF_Rol_Axis_tvalid),
//
//    //------------------------------------------------------
//    //-- From UDMX / Open-Port Interfaces
//    //------------------------------------------------------
//    //-- UDMX / This / OpenPortAcknowledge / Axis
//    .siUDMX_This_OpnAck_V_TDATA     (sUDMX_Urif_OpnAck_Axis_tdata),
//    .siUDMX_This_OpnAck_V_TVALID    (sUDMX_Urif_OpnAck_Axis_tvalid),
//    .siUDMX_This_OpnAck_V_TREADY    (sURIF_Udmx_OpnAck_Axis_tready),
//
//    //------------------------------------------------------
//    //-- To UDMX / Open-Port Interfaces
//    //------------------------------------------------------
//    //-- THIS / Udmx / OpenPortRequest / Axis
//    .soTHIS_Udmx_OpnReq_V_V_TREADY  (sUDMX_Urif_OpnReq_Axis_tready),
//    .soTHIS_Udmx_OpnReq_V_V_TDATA   (sURIF_Udmx_OpnReq_Axis_tdata),
//    .soTHIS_Udmx_OpnReq_V_V_TVALID  (sURIF_Udmx_OpnReq_Axis_tvalid),
//
//    //------------------------------------------------------
//    //-- From UDMX / Data & MetaData Interfaces
//    //------------------------------------------------------
//    //-- UDMX / This / Data / Axis
//    .siUDMX_This_Data_TDATA         (sUDMX_Urif_Data_Axis_tdata),
//    .siUDMX_This_Data_TKEEP         (sUDMX_Urif_Data_Axis_tkeep),
//    .siUDMX_This_Data_TLAST         (sUDMX_Urif_Data_Axis_tlast),
//    .siUDMX_This_Data_TVALID        (sUDMX_Urif_Data_Axis_tvalid),
//    .siUDMX_This_Data_TREADY        (sURIF_Udmx_Data_Axis_tready),
//     //-- UDMX / This / MetaData / Axis
//    .siUDMX_This_Meta_TDATA         (sUDMX_Urif_Meta_Axis_tdata),
//    .siUDMX_This_Meta_TVALID        (sUDMX_Urif_Meta_Axis_tvalid),
//    .siUDMX_This_Meta_TREADY        (sURIF_Udmx_Meta_Axis_tready),
//    
//    //------------------------------------------------------
//    //-- To UDMX / Data & MetaData Interfaces
//    //------------------------------------------------------
//    //-- THIS / Udmx / Data / Axis  
//    .soTHIS_Udmx_Data_TREADY        (sUDMX_Urif_Data_Axis_tready),    
//    .soTHIS_Udmx_Data_TDATA         (sURIF_Udmx_Data_Axis_tdata),   
//    .soTHIS_Udmx_Data_TKEEP         (sURIF_Udmx_Data_Axis_tkeep),
//    .soTHIS_Udmx_Data_TLAST         (sURIF_Udmx_Data_Axis_tlast),
//    .soTHIS_Udmx_Data_TVALID        (sURIF_Udmx_Data_Axis_tvalid),
//    //-- THIS / Udmx / MetaData / Axis
//    .soTHIS_Udmx_Meta_TREADY        (sUDMX_Urif_Meta_Axis_tready),
//    .soTHIS_Udmx_Meta_TDATA         (sURIF_Udmx_Meta_Axis_tdata),
//    .soTHIS_Udmx_Meta_TVALID        (sURIF_Udmx_Meta_Axis_tvalid),
//    //-- THIS / Udmx / Tx Length / Axis
//    .soTHIS_Udmx_PLen_V_V_TREADY    (sUDMX_Urif_PLen_Axis_tready),
//    .soTHIS_Udmx_PLen_V_V_TDATA     (sURIF_Udmx_PLen_Axis_tdata),
//    .soTHIS_Udmx_PLen_V_V_TVALID    (sURIF_Udmx_PLen_Axis_tvalid)
//
//  );
//   
//`endif // !`ifdef USE_DEPRECATED_DIRECTIVES
   


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
