//                              -*- Mode: Verilog -*-
// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Shell for the BRING-UP TEST of the FMKU2595 module (a.k.a "Kale").
// *
// * File    : Kale/Shell.v
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
// *    This shell is referred to as "Kale". It is a limited version of a typical cF shell
// *    because it is solely used for bring-up purposes. Kale implements the following interfaces: 
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
// *      - one MM2S/S2MM AXI4-Stream interfaces for the 1st Memory Port
// *        (refer to PG022-AXI- DataMover for a description of the MM2S and S2MM).
// *      - one AXI4 (full) interface for the 2nd Memory Port.
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

module Shell_Kale # (
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
  //-- ROLE / Nts / Udp / Tx Data Interfaces (.i.e ROLE-->SHELL)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ----------------
  input  [ 63:0]  siROL_Nts_Udp_Data_tdata,
  input  [  7:0]  siROL_Nts_Udp_Data_tkeep,
  input           siROL_Nts_Udp_Data_tlast,
  input           siROL_Nts_Udp_Data_tvalid,
  output          siROL_Nts_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata ------------
  input   [95:0] siROL_Nts_Udp_Meta_tdata,
  input          siROL_Nts_Udp_Meta_tvalid,
  output         siROL_Nts_Udp_Meta_tready,
  //---- Axi4-Stream UDP Data Length ---------
  input   [15:0] siROL_Nts_Udp_DLen_tdata,
  input          siROL_Nts_Udp_DLen_tvalid,
  output         siROL_Nts_Udp_DLen_tready,
  //------------------------------------------------------
  //-- ROLE / Nts / Udp / Rx Data Interfaces (.i.e SHELL-->ROLE)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ---------------
  output  [63:0] soROL_Nts_Udp_Data_tdata,
  output  [ 7:0] soROL_Nts_Udp_Data_tkeep,
  output         soROL_Nts_Udp_Data_tlast,
  output         soROL_Nts_Udp_Data_tvalid,
  input          soROL_Nts_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata -----------
  output  [95:0] soROL_Nts_Udp_Meta_tdata ,
  output         soROL_Nts_Udp_Meta_tvalid,
  input          soROL_Nts_Udp_Meta_tready,
  //---- Axi4-Stream UDP Metadata -----------
  output  [15:0] soROL_Nts_Udp_DLen_tdata ,
  output         soROL_Nts_Udp_DLen_tvalid,
  input          soROL_Nts_Udp_DLen_tready,
  //------------------------------------------------------
  //-- ROLE / Nts/ Udp / Rx Ctrl Interfaces (.i.e SHELL<-->ROLE)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Listen Request -----
  input   [15:0] siROL_Nts_Udp_LsnReq_tdata ,
  input          siROL_Nts_Udp_LsnReq_tvalid,
  output         siROL_Nts_Udp_LsnReq_tready,
  //---- Axi4-Stream UDP Listen Reply --------
  output  [ 7:0] soROL_Nts_Udp_LsnRep_tdata ,
  output         soROL_Nts_Udp_LsnRep_tvalid,
  input          soROL_Nts_Udp_LsnRep_tready,
  //---- Axi4-Stream UDP Close Request ------
  input   [15:0] siROL_Nts_Udp_ClsReq_tdata ,
  input          siROL_Nts_Udp_ClsReq_tvalid,
  output         siROL_Nts_Udp_ClsReq_tready,
  //---- Axi4-Stream UDP Close Reply ---------
  output  [ 7:0] soROL_Nts_Udp_ClsRep_tdata ,
  output         soROL_Nts_Udp_ClsRep_tvalid,
  input          soROL_Nts_Udp_ClsRep_tready,
  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / Tx Data Interfaces (.i.e ROLE-->SHELL)
  //------------------------------------------------------
  //---- Axi4-Stream TCP Data ---------------
  input  [ 63:0]  siROL_Nts_Tcp_Data_tdata,
  input  [  7:0]  siROL_Nts_Tcp_Data_tkeep,
  input           siROL_Nts_Tcp_Data_tlast,
  input           siROL_Nts_Tcp_Data_tvalid,
  output          siROL_Nts_Tcp_Data_tready,
  //---- Axi4-Stream TCP Send Request -------
  input  [ 31:0]  siROL_Nts_Tcp_SndReq_tdata,
  input           siROL_Nts_Tcp_SndReq_tvalid,
  output          siROL_Nts_Tcp_SndReq_tready,
  //---- Axi4-Stream TCP Send Reply ---------
  output [ 55:0]  soROL_Nts_Tcp_SndRep_tdata,
  output          soROL_Nts_Tcp_SndRep_tvalid,
  input           soROL_Nts_Tcp_SndRep_tready,
  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / Rx Data Interfaces  (.i.e SHELL-->ROLE)
  //------------------------------------------------------
  //---- Axi4-Stream TCP Data -----------------
  output [ 63:0]  soROL_Nts_Tcp_Data_tdata,
  output [  7:0]  soROL_Nts_Tcp_Data_tkeep,
  output          soROL_Nts_Tcp_Data_tlast,
  output          soROL_Nts_Tcp_Data_tvalid,
  input           soROL_Nts_Tcp_Data_tready,
  //----  Axi4-Stream TCP Metadata ------------
  output [ 15:0]  soROL_Nts_Tcp_Meta_tdata,
  output          soROL_Nts_Tcp_Meta_tvalid,
  input           soROL_Nts_Tcp_Meta_tready,
  //----  Axi4-Stream TCP Data Notification ---
  output [103:0]  soROL_Nts_Tcp_Notif_tdata,  // 7+96
  output          soROL_Nts_Tcp_Notif_tvalid,
  input           soROL_Nts_Tcp_Notif_tready,
  //----  Axi4-Stream TCP Data Request --------
  input  [ 31:0]  siROL_Nts_Tcp_DReq_tdata,
  input           siROL_Nts_Tcp_DReq_tvalid,
  output          siROL_Nts_Tcp_DReq_tready,
  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / TxP Ctlr Interfaces (.i.e ROLE-->SHELL)
  //------------------------------------------------------
  //---- Axi4-Stream TCP Open Session Request
  input [ 47:0]  siROL_Nts_Tcp_OpnReq_tdata,
  input          siROL_Nts_Tcp_OpnReq_tvalid,
  output         siROL_Nts_Tcp_OpnReq_tready,
  //---- Axi4-Stream TCP Open Session Reply
  output [ 23:0] soROL_Nts_Tcp_OpnRep_tdata,
  output         soROL_Nts_Tcp_OpnRep_tvalid,
  input          soROL_Nts_Tcp_OpnRep_tready,
  //---- Axi4-Stream TCP Close Request ------
  input [ 15:0]  siROL_Nts_Tcp_ClsReq_tdata,
  input          siROL_Nts_Tcp_ClsReq_tvalid,
  output         siROL_Nts_Tcp_ClsReq_tready,
  //------------------------------------------------------
  //-- ROLE / Nts / Tcp / Rx Ctlr Interfaces (.i.e SHELL-->ROLE)
  //------------------------------------------------------
  //----  Axi4-Stream TCP Listen Request ----
  input [ 15:0]  siROL_Nts_Tcp_LsnReq_tdata,   
  input          siROL_Nts_Tcp_LsnReq_tvalid,
  output         siROL_Nts_Tcp_LsnReq_tready,
  //----  Axi4-Stream TCP Listen Rep --------
  output [  7:0] soROL_Nts_Tcp_LsnRep_tdata,
  output         soROL_Nts_Tcp_LsnRep_tvalid,
  input          soROL_Nts_Tcp_LsnRep_tready,
  //------------------------------------------------------  
  //-- ROLE / Mem / Mp0 Interface
  //------------------------------------------------------
  //-- Memory Port #0 / S2MM-AXIS ------------------
  //---- Axi4-Stream Read Command -----
  input  [ 79:0]  siROL_Mem_Mp0_RdCmd_tdata,
  input           siROL_Mem_Mp0_RdCmd_tvalid,
  output          siROL_Mem_Mp0_RdCmd_tready,
  //---- Axi4-Stream Read Status ------
  output [  7:0]  soROL_Mem_Mp0_RdSts_tdata,
  output          soROL_Mem_Mp0_RdSts_tvalid,
  input           soROL_Mem_Mp0_RdSts_tready,
  //---- Axi4-Stream Data Output Channel
  output [511:0]  soROL_Mem_Mp0_Read_tdata,
  output [ 63:0]  soROL_Mem_Mp0_Read_tkeep,
  output          soROL_Mem_Mp0_Read_tlast,
  output          soROL_Mem_Mp0_Read_tvalid,
  input           soROL_Mem_Mp0_Read_tready,
  //---- Axi4-Stream Write Command ----
  input  [ 79:0]  siROL_Mem_Mp0_WrCmd_tdata,
  input           siROL_Mem_Mp0_WrCmd_tvalid,
  output          siROL_Mem_Mp0_WrCmd_tready,
  //---- Axi4-Stream Write Status -----
  output [  7:0]  soROL_Mem_Mp0_WrSts_tdata,
  output          soROL_Mem_Mp0_WrSts_tvalid,
  input           soROL_Mem_Mp0_WrSts_tready,
  //---- Axi4-Stream Data Input Channel
  input  [511:0]  siROL_Mem_Mp0_Write_tdata,
  input  [ 63:0]  siROL_Mem_Mp0_Write_tkeep,
  input           siROL_Mem_Mp0_Write_tlast,
  input           siROL_Mem_Mp0_Write_tvalid,
  output          siROL_Mem_Mp0_Write_tready, 
  //------------------------------------------------------
  //-- ROLE / Mem / Mp1 Interface
  //------------------------------------------------------
  //---- Write Address Channel ---------
  input  [  7: 0]  miROL_Mem_Mp1_AWID,
  input  [ 32: 0]  miROL_Mem_Mp1_AWADDR,
  input  [  7: 0]  miROL_Mem_Mp1_AWLEN,
  input  [  2: 0]  miROL_Mem_Mp1_AWSIZE,
  input  [  1: 0]  miROL_Mem_Mp1_AWBURST,
  input            miROL_Mem_Mp1_AWVALID,
  output           miROL_Mem_Mp1_AWREADY,
  //---- Write Data Channel ------------
  input  [511: 0]  miROL_Mem_Mp1_WDATA,
  input  [ 63: 0]  miROL_Mem_Mp1_WSTRB,
  input            miROL_Mem_Mp1_WLAST,
  input            miROL_Mem_Mp1_WVALID,
  output           miROL_Mem_Mp1_WREADY,
  //---- Write Response Channel --------
  output [  7: 0]  miROL_Mem_Mp1_BID,
  output [  1: 0]  miROL_Mem_Mp1_BRESP,
  output           miROL_Mem_Mp1_BVALID,
  input            miROL_Mem_Mp1_BREADY,
  //---- Read Address Channel ----------
  input  [  7: 0]  miROL_Mem_Mp1_ARID,
  input  [ 32: 0]  miROL_Mem_Mp1_ARADDR,
  input  [  7: 0]  miROL_Mem_Mp1_ARLEN,
  input  [  2: 0]  miROL_Mem_Mp1_ARSIZE,
  input  [  1: 0]  miROL_Mem_Mp1_ARBURST,
  input            miROL_Mem_Mp1_ARVALID,
  output           miROL_Mem_Mp1_ARREADY,
  //---- Read Data Channel -------------
  output [  7: 0]  miROL_Mem_Mp1_RID,
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
  output  [15:0]  poROL_Mmio_WrReg  
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
  (* keep = "true" *) 
  wire          sETH0_ShlClk;
  (* keep = "true" *) 
  wire          sETH0_ShlRst;
  wire          sETH0_CoreResetDone;
  
   //-- Async SoftReset & SoftEnable Signals --------------
   wire  [ 7:0]  sMMIO_META_Rst;
   wire  [ 7:0]  sMMIO_META_En;
   //-- Sync  SoftReset & SoftEnable Signals --------------
   (* keep = "true" *) 
   wire  [ 7:0]  sMETA_LayerRst;
   (* keep = "true" *) 
   wire  [ 7:0]  sMETA_LayerEn;

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
  //-- NTS / Udp / Tx Data Interfaces (.i.e NTS<-->UARS)
  //--------------------------------------------------------
  wire  [ 63:0] ssNTS0_UARS_Udp_Data_tdata ;
  wire  [  7:0] ssNTS0_UARS_Udp_Data_tkeep ;
  wire          ssNTS0_UARS_Udp_Data_tlast ;
  wire          ssNTS0_UARS_Udp_Data_tvalid;
  wire          ssNTS0_UARS_Udp_Data_tready;
  //--
  wire  [ 95:0] ssNTS0_UARS_Udp_Meta_tdata ;
  wire          ssNTS0_UARS_Udp_Meta_tvalid;
  wire          ssNTS0_UARS_Udp_Meta_tready;
  //--
  wire  [ 15:0] ssNTS0_UARS_Udp_DLen_tdata ;
  wire          ssNTS0_UARS_Udp_DLen_tvalid;
  wire          ssNTS0_UARS_Udp_DLen_tready;
  
  //--------------------------------------------------------
  //-- NTS / Udp / Rx Data Interfaces (.i.e NTS<-->UARS)
  //--------------------------------------------------------
  wire  [ 63:0] ssUARS_NTS0_Udp_Data_tdata ;
  wire  [  7:0] ssUARS_NTS0_Udp_Data_tkeep ;
  wire          ssUARS_NTS0_Udp_Data_tlast ;
  wire          ssUARS_NTS0_Udp_Data_tvalid;
  wire          ssUARS_NTS0_Udp_Data_tready;
  //--
  wire  [ 95:0] ssUARS_NTS0_Udp_Meta_tdata ; 
  wire          ssUARS_NTS0_Udp_Meta_tvalid;
  wire          ssUARS_NTS0_Udp_Meta_tready;
  //--
  wire  [ 15:0] ssUARS_NTS0_Udp_DLen_tdata ; 
  wire          ssUARS_NTS0_Udp_DLen_tvalid;
  wire          ssUARS_NTS0_Udp_DLen_tready;
  
  //------------------------------------------------------
  //-- NTS / Udp / Rx Ctrl Interfaces (.i.e NTS<-->UARS)
  //------------------------------------------------------
  wire  [ 15:0] ssUARS_NTS0_Udp_LsnReq_tdata ;
  wire          ssUARS_NTS0_Udp_LsnReq_tvalid;
  wire          ssUARS_NTS0_Udp_LsnReq_tready;
  //--
  wire  [  7:0] ssNTS0_UARS_Udp_LsnRep_tdata ;
  wire          ssNTS0_UARS_Udp_LsnRep_tvalid;
  wire          ssNTS0_UARS_Udp_LsnRep_tready;
  //--
  wire  [ 15:0] ssUARS_NTS0_Udp_ClsReq_tdata ;
  wire          ssUARS_NTS0_Udp_ClsReq_tvalid;
  wire          ssUARS_NTS0_Udp_ClsReq_tready;
  //--
  wire  [  7:0] ssNTS0_UARS_Udp_ClsRep_tdata ;
  wire          ssNTS0_UARS_Udp_ClsRep_tvalid;
  wire          ssNTS0_UARS_Udp_ClsRep_tready;
  
  //------------------------------------------------------
  //-- NTS / Tcp / Tx Data Interfaces (.i.e NTS<-->TARS)
  //------------------------------------------------------
  wire  [ 63:0] ssTARS_NTS0_Tcp_Data_tdata ;
  wire  [  7:0] ssTARS_NTS0_Tcp_Data_tkeep ;
  wire          ssTARS_NTS0_Tcp_Data_tlast ;
  wire          ssTARS_NTS0_Tcp_Data_tvalid;
  wire          ssTARS_NTS0_Tcp_Data_tready;
  //--
  wire  [ 31:0] ssTARS_NTS0_Tcp_SndReq_tdata ;
  wire          ssTARS_NTS0_Tcp_SndReq_tvalid;
  wire          ssTARS_NTS0_Tcp_SndReq_tready;
  //--
  wire  [ 55:0] ssNTS0_TARS_Tcp_SndRep_tdata ;
  wire          ssNTS0_TARS_Tcp_SndRep_tvalid;
  wire          ssNTS0_TARS_Tcp_SndRep_tready;
      
  //------------------------------------------------------
  //-- NTS / Tcp / Rx Data Interfaces (.i.e NTS<-->TARS)
  //------------------------------------------------------
  wire  [ 63:0] ssNTS0_TARS_Tcp_Data_tdata  ;
  wire  [  7:0] ssNTS0_TARS_Tcp_Data_tkeep  ;
  wire          ssNTS0_TARS_Tcp_Data_tlast  ;
  wire          ssNTS0_TARS_Tcp_Data_tvalid ;
  wire          ssNTS0_TARS_Tcp_Data_tready ;
  //--
  wire  [ 15:0] ssNTS0_TARS_Tcp_Meta_tdata  ;
  wire          ssNTS0_TARS_Tcp_Meta_tvalid ;
  wire          ssNTS0_TARS_Tcp_Meta_tready ;
  //--
  wire  [103:0] ssNTS0_TARS_Tcp_Notif_tdata ;  // 7+96
  wire          ssNTS0_TARS_Tcp_Notif_tvalid;
  wire          ssNTS0_TARS_Tcp_Notif_tready;
  //--
  wire  [ 31:0] ssTARS_NTS0_Tcp_DReq_tdata  ;
  wire          ssTARS_NTS0_Tcp_DReq_tvalid ;
  wire          ssTARS_NTS0_Tcp_DReq_tready ;
    
  //------------------------------------------------------
  //-- NTS / Tcp / Tx Ctlr Interfaces (.i.e NTS<-->TARS)
  //------------------------------------------------------
  wire  [ 47:0] ssTARS_NTS0_Tcp_OpnReq_tdata ;
  wire          ssTARS_NTS0_Tcp_OpnReq_tvalid;
  wire          ssTARS_NTS0_Tcp_OpnReq_tready;
  //--
  wire  [ 23:0] ssNTS0_TARS_Tcp_OpnRep_tdata ;
  wire          ssNTS0_TARS_Tcp_OpnRep_tvalid;
  wire          ssNTS0_TARS_Tcp_OpnRep_tready;
  //--
  wire  [ 15:0] ssTARS_NTS0_Tcp_ClsReq_tdata ;
  wire          ssTARS_NTS0_Tcp_ClsReq_tvalid;
  wire          ssTARS_NTS0_Tcp_ClsReq_tready;
    
  //------------------------------------------------------
  //-- NTS / Tcp / Rx Ctlr Interfaces (.i.e NTS<-->TARS)
  //------------------------------------------------------
  wire  [ 15:0] ssTARS_NTS0_Tcp_LsnReq_tdata ;   
  wire          ssTARS_NTS0_Tcp_LsnReq_tvalid;
  wire          ssTARS_NTS0_Tcp_LsnReq_tready;
  //--
  wire  [  7:0] ssNTS0_TARS_Tcp_LsnRep_tdata ;
  wire          ssNTS0_TARS_Tcp_LsnRep_tvalid;
  wire          ssNTS0_TARS_Tcp_LsnRep_tready;  
 
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
  //------ [DIAG_STAT_1] ---------------
  wire          sNTS0_MMIO_Mc0RxWrErr;
  //------ [DIAG_CTRL_2] ---------------
  //------ [DIAG_URDDC] ----------------
  wire  [15:0]  sNTS0_MMIO_UdpRxDataDropCnt;
  //------ [DIAG_TRNDC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxNotifDropCnt;
  //------ [DIAG_TRMDC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxMetaDropCnt;
  //------ [DIAG_TRDDC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxDataDropCnt;
  //------ [DIAG_TRCDC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxCrcDropCnt;
    //------ [DIAG_TRSDC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxSessDropCnt;
    //------ [DIAG_TRODC] ----------------
  wire  [ 7:0]  sNTS0_MMIO_TcpRxOooDropCnt;
  
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
    .piTOP_Rst                      (sETH0_ShlRst),  // OBSOLETE_20210330 (piTOP_156_25Rst),
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
    //.piNTS0_Mc0RxRdErr              (sNTS0_MMIO_Mc0RxRdErr),  // [FIXME]
    .piNTS0_Mc0RxWrErr              (sNTS0_MMIO_Mc0RxWrErr),
    //.piNTS0_Mc0TxRdErr              (sNTS0_MMIO_Mc0TxRdErr),  // [FIXME]
    //.piNTS0_Mc0TxWrErr              (sNTS0_MMIO_Mc0TxWrErr),  // [FIXME]
    .piNTS0_UdpRxDataDropCnt        (sNTS0_MMIO_UdpRxDataDropCnt),
    .piNTS0_TcpRxNotifDropCnt       (sNTS0_MMIO_TcpRxNotifDropCnt),
    .piNTS0_TcpRxMetaDropCnt        (sNTS0_MMIO_TcpRxMetaDropCnt),
    .piNTS0_TcpRxDataDropCnt        (sNTS0_MMIO_TcpRxDataDropCnt),
    .piNTS0_TcpRxCrcDropCnt         (sNTS0_MMIO_TcpRxCrcDropCnt),
    .piNTS0_TcpRxSessDropCnt        (sNTS0_MMIO_TcpRxSessDropCnt),
    .piNTS0_TcpRxOooDropCnt         (sNTS0_MMIO_TcpRxOooDropCnt),
    //--
    .poNTS0_MacAddress              (sMMIO_NTS0_MacAddress),
    .poNTS0_Ip4Address              (sMMIO_NTS0_Ip4Address),
    .poNTS0_SubNetMask              (sMMIO_NTS0_SubNetMask),
    .poNTS0_GatewayAddr             (sMMIO_NTS0_GatewayAddr),
    //----------------------------------------------
    //-- ROLE : Status input and Control Outputs
    //----------------------------------------------
    //---- [PHY_RESET] -------------
    .poSHL_ResetLayer               (sMMIO_META_Rst),
    //---- [PHY_ENABLE] ------------
    .poSHL_EnableLayer              (sMMIO_META_En),
    //---- DIAG_CTRL_1 -------------
    .poROLE_Mc1_MemTestCtrl         (poROL_Mmio_Mc1_MemTestCtrl),
    //---- DIAG_STAT_1 -------------
    .piROLE_Mc1_MemTestStat         (piROL_Mmio_Mc1_MemTestStat),
    //---- DIAG_CTRL_2 -------------  
    .poROLE_UdpEchoCtrl             (poROL_Mmio_UdpEchoCtrl),
    .poROLE_UdpPostDgmEn            (poROL_Mmio_UdpPostDgmEn),
    .poROLE_UdpCaptDgmEn            (poROL_Mmio_UdpCaptDgmEn),
    .poROLE_TcpEchoCtrl             (poROL_Mmio_TcpEchoCtrl),
    .poROLE_TcpPostSegEn            (poROL_Mmio_TcpPostSegEn),
    .poROLE_TcpCaptSegEn            (poROL_Mmio_TcpCaptSegEn),
     //---- APP_RDROL --------------
    .piROLE_RdReg                   (piROL_Mmio_RdReg),
     //---- APP_WRROL --------------
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
    .piFMC_RdReg                    (),  // [TODO - Not yet used by this SHELL]
    //---- APP_WRFMC ----------------
    .poFMC_WrReg                    (),  // [TODO - Not yet used by this SHELL]
    //----------------------------------------------
    //-- EMIF Extended Memory Port B
    //----------------------------------------------
    .piXXX_XMem_en                 (),    // [TODO - Not yet used by this SHELL]
    .piXXX_XMem_Wren               (),    // [TODO - Not yet used by this SHELL]
    .piXXX_XMem_WrData             (),    // [TODO - Not yet used by this SHELL]
    .poXXX_XMem_RData              (),    // [TODO - Not yet used by this SHELL]
    .piXXX_XMemAddr                ()     // [TODO - Not yet used by this SHELL]
  );  // End of MMIO

   //============================================================================
   // HARD_SYNC: A Set of Metastability Hardened Registers for Reset and Enable
   //============================================================================
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_RESET_LY2 (
      .DIN  (sMMIO_META_Rst[2]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerRst[2])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_RESET_LY3 (
      .DIN  (sMMIO_META_Rst[3]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerRst[3])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_RESET_LY4 (
      .DIN  (sMMIO_META_Rst[4]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerRst[4])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_RESET_LY7 (
      .DIN  (sMMIO_META_Rst[7]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerRst[7])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_ENABLE_LY2 (
      .DIN  (sMMIO_META_En[2]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerEn[2])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_ENABLE_LY3 (
      .DIN  (sMMIO_META_En[3]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerEn[3])
   );
   
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_ENABLE_LY4 (
      .DIN  (sMMIO_META_En[4]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerEn[4])
   );
    
   HARD_SYNC #(
      .INIT(1'b0),            // Initial values, 1'b0, 1'b1
      .IS_CLK_INVERTED(1'b0), // Programmable inversion on CLK input
      .LATENCY(2)             // 2-3
   )
   SW_ENABLE_LY7 (
      .DIN  (sMMIO_META_En[7]),
      .CLK  (sETH0_ShlClk),
      .DOUT (sMETA_LayerEn[7])
   );

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
    //  INST: 10G ETHERNET SUBSYSTEM W/ LOOPBACK SUPPORT
    //========================================================================
    TenGigEth_Flash ETH0 (
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
    .siAPP_Udp_Data_tdata             (ssUARS_NTS0_Udp_Data_tdata),
    .siAPP_Udp_Data_tkeep             (ssUARS_NTS0_Udp_Data_tkeep),
    .siAPP_Udp_Data_tlast             (ssUARS_NTS0_Udp_Data_tlast),
    .siAPP_Udp_Data_tvalid            (ssUARS_NTS0_Udp_Data_tvalid),
    .siAPP_Udp_Data_tready            (ssUARS_NTS0_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .siAPP_Udp_Meta_tdata             (ssUARS_NTS0_Udp_Meta_tdata),
    .siAPP_Udp_Meta_tvalid            (ssUARS_NTS0_Udp_Meta_tvalid),
    .siAPP_Udp_Meta_tready            (ssUARS_NTS0_Udp_Meta_tready),
    //---- Axis4Stream UDP Data Length ---------
    .siAPP_Udp_DLen_tdata             (ssUARS_NTS0_Udp_DLen_tdata),
    .siAPP_Udp_DLen_tvalid            (ssUARS_NTS0_Udp_DLen_tvalid),
    .siAPP_Udp_DLen_tready            (ssUARS_NTS0_Udp_DLen_tready),
    //------------------------------------------------------
    //-- UAIF / Rx Data Interfaces (.i.e NTS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .soAPP_Udp_Data_tdata             (ssNTS0_UARS_Udp_Data_tdata),
    .soAPP_Udp_Data_tkeep             (ssNTS0_UARS_Udp_Data_tkeep),
    .soAPP_Udp_Data_tlast             (ssNTS0_UARS_Udp_Data_tlast),
    .soAPP_Udp_Data_tvalid            (ssNTS0_UARS_Udp_Data_tvalid),
    .soAPP_Udp_Data_tready            (ssNTS0_UARS_Udp_Data_tready),
     //---- Axi4-Stream UDP Metadata -----------
    .soAPP_Udp_Meta_tdata             (ssNTS0_UARS_Udp_Meta_tdata),
    .soAPP_Udp_Meta_tvalid            (ssNTS0_UARS_Udp_Meta_tvalid),
    .soAPP_Udp_Meta_tready            (ssNTS0_UARS_Udp_Meta_tready),
    //---- Axi4-Stream UDP Data Len -----------
    .soAPP_Udp_DLen_tdata             (ssNTS0_UARS_Udp_DLen_tdata),
    .soAPP_Udp_DLen_tvalid            (ssNTS0_UARS_Udp_DLen_tvalid),
    .soAPP_Udp_DLen_tready            (ssNTS0_UARS_Udp_DLen_tready),
    //------------------------------------------------------
    //-- UAIF / UDP Rx Ctrl Interfaces (.i.e NTS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Listen Request -----
    .siAPP_Udp_LsnReq_tdata           (ssUARS_NTS0_Udp_LsnReq_tdata),
    .siAPP_Udp_LsnReq_tvalid          (ssUARS_NTS0_Udp_LsnReq_tvalid),
    .siAPP_Udp_LsnReq_tready          (ssUARS_NTS0_Udp_LsnReq_tready),
    //---- Axi4-Stream UDP Listen Reply --------
    .soAPP_Udp_LsnRep_tdata           (ssNTS0_UARS_Udp_LsnRep_tdata),
    .soAPP_Udp_LsnRep_tvalid          (ssNTS0_UARS_Udp_LsnRep_tvalid),
    .soAPP_Udp_LsnRep_tready          (ssNTS0_UARS_Udp_LsnRep_tready),
    //---- Axi4-Stream UDP Close Request -------
    .siAPP_Udp_ClsReq_tdata           (ssUARS_NTS0_Udp_ClsReq_tdata),
    .siAPP_Udp_ClsReq_tvalid          (ssUARS_NTS0_Udp_ClsReq_tvalid),
    .siAPP_Udp_ClsReq_tready          (ssUARS_NTS0_Udp_ClsReq_tready),
    //---- Axi4-Stream UDP Close Reply ---------
    .soAPP_Udp_ClsRep_tdata           (ssNTS0_UARS_Udp_ClsRep_tdata),
    .soAPP_Udp_ClsRep_tvalid          (ssNTS0_UARS_Udp_ClsRep_tvalid),
    .soAPP_Udp_ClsRep_tready          (ssNTS0_UARS_Udp_ClsRep_tready),
    //------------------------------------------------------
    //-- TAIF / Tx Data Interfaces (.i.e APP-->NTS)
    //------------------------------------------------------
    //---- Axi4-Stream APP Data ---------------
    .siAPP_Tcp_Data_tdata             (ssTARS_NTS0_Tcp_Data_tdata),
    .siAPP_Tcp_Data_tkeep             (ssTARS_NTS0_Tcp_Data_tkeep),
    .siAPP_Tcp_Data_tlast             (ssTARS_NTS0_Tcp_Data_tlast),
    .siAPP_Tcp_Data_tvalid            (ssTARS_NTS0_Tcp_Data_tvalid),
    .siAPP_Tcp_Data_tready            (ssTARS_NTS0_Tcp_Data_tready),
    //---- Axi4- Stream TCP SndReq ------------
    .siAPP_Tcp_SndReq_tdata           (ssTARS_NTS0_Tcp_SndReq_tdata),
    .siAPP_Tcp_SndReq_tvalid          (ssTARS_NTS0_Tcp_SndReq_tvalid),
    .siAPP_Tcp_SndReq_tready          (ssTARS_NTS0_Tcp_SndReq_tready),
    //---- Axi4-Stream TCP SndRep -------------
    .soAPP_Tcp_SndRep_tdata           (ssNTS0_TARS_Tcp_SndRep_tdata),
    .soAPP_Tcp_SndRep_tvalid          (ssNTS0_TARS_Tcp_SndRep_tvalid),
    .soAPP_Tcp_SndRep_tready          (ssNTS0_TARS_Tcp_SndRep_tready),      
    //---------------------------------------------------
    //-- TAIF / Rx Data Interfaces (.i.e NTS-->APP)
    //---------------------------------------------------
    //---- Axi4-Stream APP Data ---------------         
    .soAPP_Tcp_Data_tdata             (ssNTS0_TARS_Tcp_Data_tdata),
    .soAPP_Tcp_Data_tkeep             (ssNTS0_TARS_Tcp_Data_tkeep),
    .soAPP_Tcp_Data_tlast             (ssNTS0_TARS_Tcp_Data_tlast),
    .soAPP_Tcp_Data_tvalid            (ssNTS0_TARS_Tcp_Data_tvalid),
    .soAPP_Tcp_Data_tready            (ssNTS0_TARS_Tcp_Data_tready),
    //---- Axi4-Stream APP Metadata -----------  
    .soAPP_Tcp_Meta_tdata             (ssNTS0_TARS_Tcp_Meta_tdata),
    .soAPP_Tcp_Meta_tvalid            (ssNTS0_TARS_Tcp_Meta_tvalid),
    .soAPP_Tcp_Meta_tready            (ssNTS0_TARS_Tcp_Meta_tready),
     //---- Axi4-Stream APP Data Notification -
    .soAPP_Tcp_Notif_tdata            (ssNTS0_TARS_Tcp_Notif_tdata),
    .soAPP_Tcp_Notif_tvalid           (ssNTS0_TARS_Tcp_Notif_tvalid),
    .soAPP_Tcp_Notif_tready           (ssNTS0_TARS_Tcp_Notif_tready),
    //---- Axi4-Stream APP Data Request -------
    .siAPP_Tcp_DReq_tdata             (ssTARS_NTS0_Tcp_DReq_tdata),    
    .siAPP_Tcp_DReq_tvalid            (ssTARS_NTS0_Tcp_DReq_tvalid),
    .siAPP_Tcp_DReq_tready            (ssTARS_NTS0_Tcp_DReq_tready),
    //------------------------------------------------------
    //-- TAIF / Tx Ctlr Interfaces (.i.e APP-->NTS)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Open Session Request
    .siAPP_Tcp_OpnReq_tdata           (ssTARS_NTS0_Tcp_OpnReq_tdata),
    .siAPP_Tcp_OpnReq_tvalid          (ssTARS_NTS0_Tcp_OpnReq_tvalid),
    .siAPP_Tcp_OpnReq_tready          (ssTARS_NTS0_Tcp_OpnReq_tready),
    //---- Axi4-Stream TCP Open Session Status
    .soAPP_Tcp_OpnRep_tdata           (ssNTS0_TARS_Tcp_OpnRep_tdata),
    .soAPP_Tcp_OpnRep_tvalid          (ssNTS0_TARS_Tcp_OpnRep_tvalid),
    .soAPP_Tcp_OpnRep_tready          (ssNTS0_TARS_Tcp_OpnRep_tready),
    //---- Axi4-Stream TCP Close Request -----
    .siAPP_Tcp_ClsReq_tdata           (ssTARS_NTS0_Tcp_ClsReq_tdata),
    .siAPP_Tcp_ClsReq_tvalid          (ssTARS_NTS0_Tcp_ClsReq_tvalid),
    .siAPP_Tcp_ClsReq_tready          (ssTARS_NTS0_Tcp_ClsReq_tready),
    //------------------------------------------------------
    //-- TAIF / Rx Ctlr Interfaces (.i.e NTS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Listen Request -----
    .siAPP_Tcp_LsnReq_tdata           (ssTARS_NTS0_Tcp_LsnReq_tdata),
    .siAPP_Tcp_LsnReq_tvalid          (ssTARS_NTS0_Tcp_LsnReq_tvalid),
    .siAPP_Tcp_LsnReq_tready          (ssTARS_NTS0_Tcp_LsnReq_tready),
    //---- Axi4-Stream TCP Listen Status ------
    .soAPP_Tcp_LsnRep_tdata           (ssNTS0_TARS_Tcp_LsnRep_tdata),
    .soAPP_Tcp_LsnRep_tvalid          (ssNTS0_TARS_Tcp_LsnRep_tvalid),
    .soAPP_Tcp_LsnRep_tready          (ssNTS0_TARS_Tcp_LsnRep_tready),
    //------------------------------------------------------
    //-- MMIO / Interfaces
    //------------------------------------------------------
    .piMMIO_Layer2Rst                 (sMETA_LayerRst[2]),
    .piMMIO_Layer3Rst                 (sMETA_LayerRst[3]),
    .piMMIO_Layer4Rst                 (sMETA_LayerRst[4]),
    .piMMIO_Layer4En                  (sMETA_LayerEn[4]),
    .piMMIO_MacAddress                (sMMIO_NTS0_MacAddress),
    .piMMIO_Ip4Address                (sMMIO_NTS0_Ip4Address),
    .piMMIO_SubNetMask                (sMMIO_NTS0_SubNetMask),
    .piMMIO_GatewayAddr               (sMMIO_NTS0_GatewayAddr),
    //--
    .poMMIO_CamReady                  (sNTS0_MMIO_CamReady),      // [TODO-Merge this signal with NtsReady]
    .poMMIO_NtsReady                  (sNTS0_MMIO_NtsReady),
    .poMMIO_Mc0RxWrErr                (sNTS0_MMIO_Mc0RxWrErr),
    .poMMIO_TcpRxNotifDropCnt         (sNTS0_MMIO_TcpRxNotifDropCnt),
    .poMMIO_TcpRxMetaDropCnt          (sNTS0_MMIO_TcpRxMetaDropCnt),
    .poMMIO_TcpRxDataDropCnt          (sNTS0_MMIO_TcpRxDataDropCnt),
    .poMMIO_TcpRxCrcDropCnt           (sNTS0_MMIO_TcpRxCrcDropCnt),
    .poMMIO_TcpRxSessDropCnt          (sNTS0_MMIO_TcpRxSessDropCnt),
    .poMMIO_TcpRxOooDropCnt           (sNTS0_MMIO_TcpRxOooDropCnt),
    .poMMIO_UdpRxDataDropCnt          (sNTS0_MMIO_UdpRxDataDropCnt)
  );  // End of NTS0


  //============================================================================
  //  INST: TCP APPLICATION REGISTER SLICE (NTS0<-->[TARS]<-->APP)
  //============================================================================
  TcpApplicationRegisterSlice TARS (
    .piClk                      (sETH0_ShlClk),
    .piRst                      (sETH0_ShlRst),    // OBSOLETE_20210329  (piTOP_156_25Rst),
    //------------------------------------------------------
    //-- APP / Tcp / Tx Data Interfaces (.i.e THIS<-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Data ---------------
    .siAPP_Tcp_Data_tdata       (siROL_Nts_Tcp_Data_tdata ),
    .siAPP_Tcp_Data_tkeep       (siROL_Nts_Tcp_Data_tkeep ),
    .siAPP_Tcp_Data_tlast       (siROL_Nts_Tcp_Data_tlast ),
    .siAPP_Tcp_Data_tvalid      (siROL_Nts_Tcp_Data_tvalid),
    .siAPP_Tcp_Data_tready      (siROL_Nts_Tcp_Data_tready),
    //---- Axi4-Stream APP Send Request -------
    .siAPP_Tcp_SndReq_tdata     (siROL_Nts_Tcp_SndReq_tdata ),
    .siAPP_Tcp_SndReq_tvalid    (siROL_Nts_Tcp_SndReq_tvalid),
    .siAPP_Tcp_SndReq_tready    (siROL_Nts_Tcp_SndReq_tready),
    //---- Axi4-Stream APP Send Reply ---------
    .soAPP_Tcp_SndRep_tdata     (soROL_Nts_Tcp_SndRep_tdata ),
    .soAPP_Tcp_SndRep_tvalid    (soROL_Nts_Tcp_SndRep_tvalid),
    .soAPP_Tcp_SndRep_tready    (soROL_Nts_Tcp_SndRep_tready),
    //------------------------------------------------------
    //-- APP / Tcp / Rx Data Interfaces (.i.e THIS<-->APP)
    //------------------------------------------------------
    //-- Axi4-Stream TCP Data -----------------
    .soAPP_Tcp_Data_tdata       (soROL_Nts_Tcp_Data_tdata ),
    .soAPP_Tcp_Data_tkeep       (soROL_Nts_Tcp_Data_tkeep ),
    .soAPP_Tcp_Data_tlast       (soROL_Nts_Tcp_Data_tlast ),
    .soAPP_Tcp_Data_tvalid      (soROL_Nts_Tcp_Data_tvalid),
    .soAPP_Tcp_Data_tready      (soROL_Nts_Tcp_Data_tready),
    //--  Axi4-Stream TCP Metadata ------------
    .soAPP_Tcp_Meta_tdata       (soROL_Nts_Tcp_Meta_tdata ),
    .soAPP_Tcp_Meta_tvalid      (soROL_Nts_Tcp_Meta_tvalid),
    .soAPP_Tcp_Meta_tready      (soROL_Nts_Tcp_Meta_tready),
    //--  Axi4-Stream TCP Data Notification ---
    .soAPP_Tcp_Notif_tdata      (soROL_Nts_Tcp_Notif_tdata ), // 7+96
    .soAPP_Tcp_Notif_tvalid     (soROL_Nts_Tcp_Notif_tvalid),
    .soAPP_Tcp_Notif_tready     (soROL_Nts_Tcp_Notif_tready),
     //--  Axi4-Stream TCP Data Request --------
    .siAPP_Tcp_DReq_tdata       (siROL_Nts_Tcp_DReq_tdata ),
    .siAPP_Tcp_DReq_tvalid      (siROL_Nts_Tcp_DReq_tvalid),
    .siAPP_Tcp_DReq_tready      (siROL_Nts_Tcp_DReq_tready),
    //------------------------------------------------------
    //-- APP / Tcp / Tx Ctlr Interfaces (.i.e THIS<-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Open Session Request
    .siAPP_Tcp_OpnReq_tdata     (siROL_Nts_Tcp_OpnReq_tdata ),
    .siAPP_Tcp_OpnReq_tvalid    (siROL_Nts_Tcp_OpnReq_tvalid),
    .siAPP_Tcp_OpnReq_tready    (siROL_Nts_Tcp_OpnReq_tready),
    //---- Axi4-Stream TCP Open Session Reply
    .soAPP_Tcp_OpnRep_tdata     (soROL_Nts_Tcp_OpnRep_tdata ),
    .soAPP_Tcp_OpnRep_tvalid    (soROL_Nts_Tcp_OpnRep_tvalid),
    .soAPP_Tcp_OpnRep_tready    (soROL_Nts_Tcp_OpnRep_tready),
    //---- Axi4-Stream TCP Close Request ------
    .siAPP_Tcp_ClsReq_tdata     (siROL_Nts_Tcp_ClsReq_tdata ), 
    .siAPP_Tcp_ClsReq_tvalid    (siROL_Nts_Tcp_ClsReq_tvalid),
    .siAPP_Tcp_ClsReq_tready    (siROL_Nts_Tcp_ClsReq_tready),
    //------------------------------------------------------
    //-- APP / Tcp / Rx Ctlr Interfaces (.i.e THIS<-->APP)
    //------------------------------------------------------
    //----  Axi4-Stream TCP Listen Request ----
    .siAPP_Tcp_LsnReq_tdata     (siROL_Nts_Tcp_LsnReq_tdata ),   
    .siAPP_Tcp_LsnReq_tvalid    (siROL_Nts_Tcp_LsnReq_tvalid),
    .siAPP_Tcp_LsnReq_tready    (siROL_Nts_Tcp_LsnReq_tready),
    //----  Axi4-Stream TCP Listen Rep --------
    .soAPP_Tcp_LsnRep_tdata     (soROL_Nts_Tcp_LsnRep_tdata ),
    .soAPP_Tcp_LsnRep_tvalid    (soROL_Nts_Tcp_LsnRep_tvalid),
    .soAPP_Tcp_LsnRep_tready    (soROL_Nts_Tcp_LsnRep_tready),
    //------------------------------------------------------
    //-- NTS / Tcp / Tx Data Interfaces (.i.e NTS<-->THIS)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Data ---------------
    .soNTS_Tcp_Data_tdata       (ssTARS_NTS0_Tcp_Data_tdata ),
    .soNTS_Tcp_Data_tkeep       (ssTARS_NTS0_Tcp_Data_tkeep ),
    .soNTS_Tcp_Data_tlast       (ssTARS_NTS0_Tcp_Data_tlast ),
    .soNTS_Tcp_Data_tvalid      (ssTARS_NTS0_Tcp_Data_tvalid),
    .soNTS_Tcp_Data_tready      (ssTARS_NTS0_Tcp_Data_tready),
    //---- Axi4-Stream TCP Send Request --------
    .soNTS_Tcp_SndReq_tdata     (ssTARS_NTS0_Tcp_SndReq_tdata ),
    .soNTS_Tcp_SndReq_tvalid    (ssTARS_NTS0_Tcp_SndReq_tvalid),
    .soNTS_Tcp_SndReq_tready    (ssTARS_NTS0_Tcp_SndReq_tready),
    //---- Axi4-Stream TCP Send Reply ----------
    .siNTS_Tcp_SndRep_tdata     (ssNTS0_TARS_Tcp_SndRep_tdata  ),
    .siNTS_Tcp_SndRep_tvalid    (ssNTS0_TARS_Tcp_SndRep_tvalid ),
    .siNTS_Tcp_SndRep_tready    (ssNTS0_TARS_Tcp_SndRep_tready ),
    //------------------------------------------------------
    //-- NTS / Tcp / Rx Data Interfaces (.i.e NTS<-->THIS)
    //------------------------------------------------------
    //-- Axi4-Stream TCP Data -----------------
    .siNTS_Tcp_Data_tdata       (ssNTS0_TARS_Tcp_Data_tdata ),
    .siNTS_Tcp_Data_tkeep       (ssNTS0_TARS_Tcp_Data_tkeep ),
    .siNTS_Tcp_Data_tlast       (ssNTS0_TARS_Tcp_Data_tlast ),
    .siNTS_Tcp_Data_tvalid      (ssNTS0_TARS_Tcp_Data_tvalid),
    .siNTS_Tcp_Data_tready      (ssNTS0_TARS_Tcp_Data_tready),
    //--  Axi4-Stream TCP Metadata ------------
    .siNTS_Tcp_Meta_tdata       (ssNTS0_TARS_Tcp_Meta_tdata ),
    .siNTS_Tcp_Meta_tvalid      (ssNTS0_TARS_Tcp_Meta_tvalid),
    .siNTS_Tcp_Meta_tready      (ssNTS0_TARS_Tcp_Meta_tready),
    //--  Axi4-Stream TCP Data Notification ---
    .siNTS_Tcp_Notif_tdata      (ssNTS0_TARS_Tcp_Notif_tdata ),  // 7+96
    .siNTS_Tcp_Notif_tvalid     (ssNTS0_TARS_Tcp_Notif_tvalid),
    .siNTS_Tcp_Notif_tready     (ssNTS0_TARS_Tcp_Notif_tready),
    //--  Axi4-Stream TCP Data Request --------
    .soNTS_Tcp_DReq_tdata       (ssTARS_NTS0_Tcp_DReq_tdata ),
    .soNTS_Tcp_DReq_tvalid      (ssTARS_NTS0_Tcp_DReq_tvalid),
    .soNTS_Tcp_DReq_tready      (ssTARS_NTS0_Tcp_DReq_tready),
    //------------------------------------------------------
    //-- NTS / Tcp / Tx Ctlr Interfaces (.i.e NTS<-->THIS)
    //------------------------------------------------------
    //---- Axi4-Stream TCP Open Session Request
    .soNTS_Tcp_OpnReq_tdata     (ssTARS_NTS0_Tcp_OpnReq_tdata ),
    .soNTS_Tcp_OpnReq_tvalid    (ssTARS_NTS0_Tcp_OpnReq_tvalid),
    .soNTS_Tcp_OpnReq_tready    (ssTARS_NTS0_Tcp_OpnReq_tready),
    //---- Axi4-Stream TCP Open Session Reply
    .siNTS_Tcp_OpnRep_tdata     (ssNTS0_TARS_Tcp_OpnRep_tdata ),
    .siNTS_Tcp_OpnRep_tvalid    (ssNTS0_TARS_Tcp_OpnRep_tvalid),
    .siNTS_Tcp_OpnRep_tready    (ssNTS0_TARS_Tcp_OpnRep_tready),
    //---- Axi4-Stream TCP Close Request ------
    .soNTS_Tcp_ClsReq_tdata     (ssTARS_NTS0_Tcp_ClsReq_tdata ),
    .soNTS_Tcp_ClsReq_tvalid    (ssTARS_NTS0_Tcp_ClsReq_tvalid),
    .soNTS_Tcp_ClsReq_tready    (ssTARS_NTS0_Tcp_ClsReq_tready),
    //------------------------------------------------------
    //-- NTS / Tcp / Rx Ctlr Interfaces (.i.e NTS<-->THIS)
    //------------------------------------------------------
    //----  Axi4-Stream TCP Listen Request ----
    .soNTS_Tcp_LsnReq_tdata     (ssTARS_NTS0_Tcp_LsnReq_tdata ),   
    .soNTS_Tcp_LsnReq_tvalid    (ssTARS_NTS0_Tcp_LsnReq_tvalid),
    .soNTS_Tcp_LsnReq_tready    (ssTARS_NTS0_Tcp_LsnReq_tready),
    //----  Axi4-Stream TCP Listen Reply ------
    .siNTS_Tcp_LsnRep_tdata     (ssNTS0_TARS_Tcp_LsnRep_tdata ),
    .siNTS_Tcp_LsnRep_tvalid    (ssNTS0_TARS_Tcp_LsnRep_tvalid),
    .siNTS_Tcp_LsnRep_tready    (ssNTS0_TARS_Tcp_LsnRep_tready)
  );

  //============================================================================
  //  INST: UDP APPLICATION REGISTER SLICE (NTS0<-->[UARS]<-->APP)
  //============================================================================
  UdpApplicationRegisterSlice UARS (
    .piClk                    (sETH0_ShlClk),
    .piRst                    (sETH0_ShlRst),   // OBSOLETE_20210329  (piTOP_156_25Rst),
    //------------------------------------------------------
    //-- APP / Udp / Tx Data Interfaces (.i.e APP->UARS)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .siAPP_Udp_Data_tdata     (siROL_Nts_Udp_Data_tdata ),
    .siAPP_Udp_Data_tkeep     (siROL_Nts_Udp_Data_tkeep ),
    .siAPP_Udp_Data_tlast     (siROL_Nts_Udp_Data_tlast ),
    .siAPP_Udp_Data_tvalid    (siROL_Nts_Udp_Data_tvalid),
    .siAPP_Udp_Data_tready    (siROL_Nts_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .siAPP_Udp_Meta_tdata     (siROL_Nts_Udp_Meta_tdata ),
    .siAPP_Udp_Meta_tvalid    (siROL_Nts_Udp_Meta_tvalid),
    .siAPP_Udp_Meta_tready    (siROL_Nts_Udp_Meta_tready),
    //---- Axis4Stream UDP Data Length ---------
    .siAPP_Udp_DLen_tdata     (siROL_Nts_Udp_DLen_tdata ),
    .siAPP_Udp_DLen_tvalid    (siROL_Nts_Udp_DLen_tvalid),
    .siAPP_Udp_DLen_tready    (siROL_Nts_Udp_DLen_tready),
    //------------------------------------------------------
    //-- APP / Udp / Rx Data Interfaces (.i.e UARS-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .soAPP_Udp_Data_tdata     (soROL_Nts_Udp_Data_tdata ),
    .soAPP_Udp_Data_tkeep     (soROL_Nts_Udp_Data_tkeep ),
    .soAPP_Udp_Data_tlast     (soROL_Nts_Udp_Data_tlast ),
    .soAPP_Udp_Data_tvalid    (soROL_Nts_Udp_Data_tvalid),
    .soAPP_Udp_Data_tready    (soROL_Nts_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .soAPP_Udp_Meta_tdata     (soROL_Nts_Udp_Meta_tdata ),
    .soAPP_Udp_Meta_tvalid    (soROL_Nts_Udp_Meta_tvalid),
    .soAPP_Udp_Meta_tready    (soROL_Nts_Udp_Meta_tready),
    //---- Axi4-Stream UDP Data Len -----------
    .soAPP_Udp_DLen_tdata     (soROL_Nts_Udp_DLen_tdata ),
    .soAPP_Udp_DLen_tvalid    (soROL_Nts_Udp_DLen_tvalid),
    .soAPP_Udp_DLen_tready    (soROL_Nts_Udp_DLen_tready),
    //------------------------------------------------------
    //-- APP / Udp / Rx Ctrl Interfaces (.i.e UARS<-->APP)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Listen Request -----
    .siAPP_Udp_LsnReq_tdata   (siROL_Nts_Udp_LsnReq_tdata ),
    .siAPP_Udp_LsnReq_tvalid  (siROL_Nts_Udp_LsnReq_tvalid),
    .siAPP_Udp_LsnReq_tready  (siROL_Nts_Udp_LsnReq_tready),
    //---- Axi4-Stream UDP Listen Reply --------
    .soAPP_Udp_LsnRep_tdata   (soROL_Nts_Udp_LsnRep_tdata ),
    .soAPP_Udp_LsnRep_tvalid  (soROL_Nts_Udp_LsnRep_tvalid),
    .soAPP_Udp_LsnRep_tready  (soROL_Nts_Udp_LsnRep_tready),
    //---- Axi4-Stream UDP Close Request ------
    .siAPP_Udp_ClsReq_tdata   (siROL_Nts_Udp_ClsReq_tdata ),
    .siAPP_Udp_ClsReq_tvalid  (siROL_Nts_Udp_ClsReq_tvalid),
    .siAPP_Udp_ClsReq_tready  (siROL_Nts_Udp_ClsReq_tready),
    //---- Axis4-Stream UDP Close Reply ---------
    .soAPP_Udp_ClsRep_tdata   (soROL_Nts_Udp_ClsRep_tdata ),
    .soAPP_Udp_ClsRep_tvalid  (soROL_Nts_Udp_ClsRep_tvalid),
    .soAPP_Udp_ClsRep_tready  (soROL_Nts_Udp_ClsRep_tready),
    //------------------------------------------------------
    //-- NTS / Udp / Tx Data Interfaces (.i.e UARS-->NTS)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .soNTS_Udp_Data_tdata     (ssUARS_NTS0_Udp_Data_tdata ),
    .soNTS_Udp_Data_tkeep     (ssUARS_NTS0_Udp_Data_tkeep ),
    .soNTS_Udp_Data_tlast     (ssUARS_NTS0_Udp_Data_tlast ),
    .soNTS_Udp_Data_tvalid    (ssUARS_NTS0_Udp_Data_tvalid),
    .soNTS_Udp_Data_tready    (ssUARS_NTS0_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .soNTS_Udp_Meta_tdata     (ssUARS_NTS0_Udp_Meta_tdata ),
    .soNTS_Udp_Meta_tvalid    (ssUARS_NTS0_Udp_Meta_tvalid),
    .soNTS_Udp_Meta_tready    (ssUARS_NTS0_Udp_Meta_tready),
    //---- Axis4Stream UDP Data Length ---------
    .soNTS_Udp_DLen_tdata     (ssUARS_NTS0_Udp_DLen_tdata ),
    .soNTS_Udp_DLen_tvalid    (ssUARS_NTS0_Udp_DLen_tvalid),
    .soNTS_Udp_DLen_tready    (ssUARS_NTS0_Udp_DLen_tready),  
    //------------------------------------------------------
    //-- NTS / Udp / Rx Data Interfaces (.i.e NTS<-->UARS)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Data ---------------
    .siNTS_Udp_Data_tdata     (ssNTS0_UARS_Udp_Data_tdata ),
    .siNTS_Udp_Data_tkeep     (ssNTS0_UARS_Udp_Data_tkeep ),
    .siNTS_Udp_Data_tlast     (ssNTS0_UARS_Udp_Data_tlast ),
    .siNTS_Udp_Data_tvalid    (ssNTS0_UARS_Udp_Data_tvalid),
    .siNTS_Udp_Data_tready    (ssNTS0_UARS_Udp_Data_tready),
    //---- Axi4-Stream UDP Metadata -----------
    .siNTS_Udp_Meta_tdata     (ssNTS0_UARS_Udp_Meta_tdata ),
    .siNTS_Udp_Meta_tvalid    (ssNTS0_UARS_Udp_Meta_tvalid),
    .siNTS_Udp_Meta_tready    (ssNTS0_UARS_Udp_Meta_tready),
    //---- Axi4-Stream UDP Data Len -----------
    .siNTS_Udp_DLen_tdata     (ssNTS0_UARS_Udp_DLen_tdata ),
    .siNTS_Udp_DLen_tvalid    (ssNTS0_UARS_Udp_DLen_tvalid),
    .siNTS_Udp_DLen_tready    (ssNTS0_UARS_Udp_DLen_tready),
    //------------------------------------------------------
    //-- NTS / Udp / Rx Ctrl Interfaces (.i.e NTS<-->UARS)
    //------------------------------------------------------
    //---- Axi4-Stream UDP Listen Request -----
    .soNTS_Udp_LsnReq_tdata   (ssUARS_NTS0_Udp_LsnReq_tdata ),
    .soNTS_Udp_LsnReq_tvalid  (ssUARS_NTS0_Udp_LsnReq_tvalid),
    .soNTS_Udp_LsnReq_tready  (ssUARS_NTS0_Udp_LsnReq_tready),
    //---- Axi4-Stream UDP Listen Reply --------
    .siNTS_Udp_LsnRep_tdata   (ssNTS0_UARS_Udp_LsnRep_tdata ),
    .siNTS_Udp_LsnRep_tvalid  (ssNTS0_UARS_Udp_LsnRep_tvalid),
    .siNTS_Udp_LsnRep_tready  (ssNTS0_UARS_Udp_LsnRep_tready),
    //---- Axi4-Stream UDP Close Request ------
    .soNTS_Udp_ClsReq_tdata   (ssUARS_NTS0_Udp_ClsReq_tdata ),
    .soNTS_Udp_ClsReq_tvalid  (ssUARS_NTS0_Udp_ClsReq_tvalid),
    .soNTS_Udp_ClsReq_tready  (ssUARS_NTS0_Udp_ClsReq_tready),
    //---- Axi4-Stream UDP Listen Reply --------
    .siNTS_Udp_ClsRep_tdata   (ssNTS0_UARS_Udp_ClsRep_tdata ),
    .siNTS_Udp_ClsRep_tvalid  (ssNTS0_UARS_Udp_ClsRep_tvalid),
    .siNTS_Udp_ClsRep_tready  (ssNTS0_UARS_Udp_ClsRep_tready)
  ); // End-of: UARS

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
    .piMMIO_Rst                       (1'b0),  // [NOT-USED]
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
    .siROL_Mem_Mp0_RdCmd_tdata        (siROL_Mem_Mp0_RdCmd_tdata),
    .siROL_Mem_Mp0_RdCmd_tvalid       (siROL_Mem_Mp0_RdCmd_tvalid),
    .siROL_Mem_Mp0_RdCmd_tready       (siROL_Mem_Mp0_RdCmd_tready),
    //---- Stream Read Status ----------------
    .soROL_Mem_Mp0_RdSts_tdata        (soROL_Mem_Mp0_RdSts_tdata),
    .soROL_Mem_Mp0_RdSts_tvalid       (soROL_Mem_Mp0_RdSts_tvalid),
    .soROL_Mem_Mp0_RdSts_tready       (soROL_Mem_Mp0_RdSts_tready),
    //---- Stream Data Output Channel --------
    .soROL_Mem_Mp0_Read_tdata         (soROL_Mem_Mp0_Read_tdata),
    .soROL_Mem_Mp0_Read_tkeep         (soROL_Mem_Mp0_Read_tkeep),
    .soROL_Mem_Mp0_Read_tlast         (soROL_Mem_Mp0_Read_tlast),
    .soROL_Mem_Mp0_Read_tvalid        (soROL_Mem_Mp0_Read_tvalid),
    .soROL_Mem_Mp0_Read_tready        (soROL_Mem_Mp0_Read_tready),
    //---- Stream Write Command --------------
    .siROL_Mem_Mp0_WrCmd_tdata        (siROL_Mem_Mp0_WrCmd_tdata),
    .siROL_Mem_Mp0_WrCmd_tvalid       (siROL_Mem_Mp0_WrCmd_tvalid),
    .siROL_Mem_Mp0_WrCmd_tready       (siROL_Mem_Mp0_WrCmd_tready),
    //---- Stream Write Status ---------------
    .soROL_Mem_Mp0_WrSts_tdata        (soROL_Mem_Mp0_WrSts_tdata),
    .soROL_Mem_Mp0_WrSts_tvalid       (soROL_Mem_Mp0_WrSts_tvalid),
    .soROL_Mem_Mp0_WrSts_tready       (soROL_Mem_Mp0_WrSts_tready),
    //---- Stream Data Input Channel ---------
    .siROL_Mem_Mp0_Write_tdata        (siROL_Mem_Mp0_Write_tdata),
    .siROL_Mem_Mp0_Write_tkeep        (siROL_Mem_Mp0_Write_tkeep),
    .siROL_Mem_Mp0_Write_tlast        (siROL_Mem_Mp0_Write_tlast),
    .siROL_Mem_Mp0_Write_tvalid       (siROL_Mem_Mp0_Write_tvalid),
    .siROL_Mem_Mp0_Write_tready       (siROL_Mem_Mp0_Write_tready),
    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    //---- Write Address Channel -------------
    .miROL_Mem_Mp1_AWID               (miROL_Mem_Mp1_AWID   ),
    .miROL_Mem_Mp1_AWADDR             (miROL_Mem_Mp1_AWADDR ),
    .miROL_Mem_Mp1_AWLEN              (miROL_Mem_Mp1_AWLEN  ),
    .miROL_Mem_Mp1_AWSIZE             (miROL_Mem_Mp1_AWSIZE ),
    .miROL_Mem_Mp1_AWBURST            (miROL_Mem_Mp1_AWBURST),
    .miROL_Mem_Mp1_AWVALID            (miROL_Mem_Mp1_AWVALID),
    .miROL_Mem_Mp1_AWREADY            (miROL_Mem_Mp1_AWREADY),
    //---- Write Data Channel ------------------  
    .miROL_Mem_Mp1_WDATA              (miROL_Mem_Mp1_WDATA  ),
    .miROL_Mem_Mp1_WSTRB              (miROL_Mem_Mp1_WSTRB  ),
    .miROL_Mem_Mp1_WLAST              (miROL_Mem_Mp1_WLAST  ),
    .miROL_Mem_Mp1_WVALID             (miROL_Mem_Mp1_WVALID ),
    .miROL_Mem_Mp1_WREADY             (miROL_Mem_Mp1_WREADY ),
    //---- Write Response Channel --------------
    .miROL_Mem_Mp1_BID                (miROL_Mem_Mp1_BID    ),
    .miROL_Mem_Mp1_BRESP              (miROL_Mem_Mp1_BRESP  ),
    .miROL_Mem_Mp1_BVALID             (miROL_Mem_Mp1_BVALID ),
    .miROL_Mem_Mp1_BREADY             (miROL_Mem_Mp1_BREADY ),
    //---- Read Address Channel ----------------
    .miROL_Mem_Mp1_ARID               (miROL_Mem_Mp1_ARID   ),
    .miROL_Mem_Mp1_ARADDR             (miROL_Mem_Mp1_ARADDR ),
    .miROL_Mem_Mp1_ARLEN              (miROL_Mem_Mp1_ARLEN  ),
    .miROL_Mem_Mp1_ARSIZE             (miROL_Mem_Mp1_ARSIZE ),
    .miROL_Mem_Mp1_ARBURST            (miROL_Mem_Mp1_ARBURST),
    .miROL_Mem_Mp1_ARVALID            (miROL_Mem_Mp1_ARVALID),
    .miROL_Mem_Mp1_ARREADY            (miROL_Mem_Mp1_ARREADY),
    //---- Read Data Channel -------------------
    .miROL_Mem_Mp1_RID                (miROL_Mem_Mp1_RID    ),
    .miROL_Mem_Mp1_RDATA              (miROL_Mem_Mp1_RDATA  ),
    .miROL_Mem_Mp1_RRESP              (miROL_Mem_Mp1_RRESP  ),
    .miROL_Mem_Mp1_RLAST              (miROL_Mem_Mp1_RLAST  ),
    .miROL_Mem_Mp1_RVALID             (miROL_Mem_Mp1_RVALID ),
    .miROL_Mem_Mp1_RREADY             (miROL_Mem_Mp1_RREADY ),
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
  ) HW_RESET (
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
  assign poROL_Mmio_Ly7Rst = sMETA_LayerRst[7];
  assign poROL_Mmio_Ly7En  = sMETA_LayerEn[7];

endmodule
