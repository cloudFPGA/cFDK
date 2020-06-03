//////////////////////////////////////////////////////////////////////////////////
//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *       ROLE template for Themisto SRA
//  *
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - FMKU60 ROLE
// *****************************************************************************



module Role_Themisto (
    //------------------------------------------------------
    //-- TOP / Global Input Clock and Reset Interface
    //------------------------------------------------------
    input            piSHL_156_25Clk,
    input            piSHL_156_25Rst,
    //-- LY7 Enable and Reset
    input            piMMIO_Ly7_Rst,
    input            piMMIO_Ly7_En,
    
    //------------------------------------------------------
    //-- SHELL / Role / Nts0 / Udp Interface
    //------------------------------------------------------
    //---- Input AXI-Write Stream Interface ----------
    input  [ 63: 0]   siNRC_Udp_Data_tdata,
    input  [  7: 0]   siNRC_Udp_Data_tkeep,
    input             siNRC_Udp_Data_tvalid,
    input             siNRC_Udp_Data_tlast,
    output            siNRC_Udp_Data_tready,
    //---- Output AXI-Write Stream Interface ---------
    output [ 63: 0]   soNRC_Udp_Data_tdata,
    output [  7: 0]   soNRC_Udp_Data_tkeep,
    output            soNRC_Udp_Data_tvalid,
    output            soNRC_Udp_Data_tlast,
    input             soNRC_Udp_Data_tready,
    //-- Open Port vector
    output [ 31: 0]   poROL_Nrc_Udp_Rx_ports,
    //-- ROLE <-> NRC Meta Interface
    output [ 79: 0]   soROLE_Nrc_Udp_Meta_TDATA,
    output            soROLE_Nrc_Udp_Meta_TVALID,
    input             soROLE_Nrc_Udp_Meta_TREADY,
    output [  9: 0]   soROLE_Nrc_Udp_Meta_TKEEP,
    output            soROLE_Nrc_Udp_Meta_TLAST,
    input  [ 79: 0]   siNRC_Role_Udp_Meta_TDATA,
    input             siNRC_Role_Udp_Meta_TVALID,
    output            siNRC_Role_Udp_Meta_TREADY,
    input  [  9: 0]   siNRC_Role_Udp_Meta_TKEEP,
    input             siNRC_Role_Udp_Meta_TLAST,
    
    //------------------------------------------------------
    //-- SHELL / Role / Nts0 / Tcp Interface
    //------------------------------------------------------
    //---- Input AXI-Write Stream Interface ----------
    input  [ 63: 0]   siNRC_Tcp_Data_tdata,
    input  [  7: 0]   siNRC_Tcp_Data_tkeep,
    input             siNRC_Tcp_Data_tvalid,
    input             siNRC_Tcp_Data_tlast,
    output            siNRC_Tcp_Data_tready,
    //---- Output AXI-Write Stream Interface ---------
    output [ 63: 0]  soNRC_Tcp_Data_tdata,
    output [  7: 0]  soNRC_Tcp_Data_tkeep,
    output           soNRC_Tcp_Data_tvalid,
    output           soNRC_Tcp_Data_tlast,
    input            soNRC_Tcp_Data_tready,
    //-- Open Port vector
    output [ 31: 0]  poROL_Nrc_Tcp_Rx_ports,
    //-- ROLE <-> NRC Meta Interface
    output [ 79: 0]  soROLE_Nrc_Tcp_Meta_TDATA,
    output           soROLE_Nrc_Tcp_Meta_TVALID,
    input            soROLE_Nrc_Tcp_Meta_TREADY,
    output [  9: 0]  soROLE_Nrc_Tcp_Meta_TKEEP,
    output           soROLE_Nrc_Tcp_Meta_TLAST,
    input  [ 79: 0]  siNRC_Role_Tcp_Meta_TDATA,
    input            siNRC_Role_Tcp_Meta_TVALID,
    output           siNRC_Role_Tcp_Meta_TREADY,
    input  [  9: 0]  siNRC_Role_Tcp_Meta_TKEEP,
    input            siNRC_Role_Tcp_Meta_TLAST,
    
    //------------------------------------------------------
    //-- SHELL / Mem / Mp0 Interface
    //------------------------------------------------------
    //---- Memory Port #0 / S2MM-AXIS -------------
    //------ Stream Read Command ---------
    output [ 79: 0]  soMEM_Mp0_RdCmd_tdata,
    output           soMEM_Mp0_RdCmd_tvalid,
    input            soMEM_Mp0_RdCmd_tready,
    //------ Stream Read Status ----------
    input  [  7: 0]  siMEM_Mp0_RdSts_tdata,
    input            siMEM_Mp0_RdSts_tvalid,
    output           siMEM_Mp0_RdSts_tready,
    //------ Stream Data Input Channel ---
    input  [511: 0]  siMEM_Mp0_Read_tdata,
    input  [ 63: 0]  siMEM_Mp0_Read_tkeep,
    input            siMEM_Mp0_Read_tlast,
    input            siMEM_Mp0_Read_tvalid,
    output           siMEM_Mp0_Read_tready,
    //------ Stream Write Command --------
    output [ 79: 0]  soMEM_Mp0_WrCmd_tdata,
    output           soMEM_Mp0_WrCmd_tvalid,
    input            soMEM_Mp0_WrCmd_tready,
    //------ Stream Write Status ---------
    input            siMEM_Mp0_WrSts_tvalid,
    input  [  7: 0]  siMEM_Mp0_WrSts_tdata,
    output           siMEM_Mp0_WrSts_tready,
    //------ Stream Data Output Channel --
    output [511: 0]  soMEM_Mp0_Write_tdata,
    output [ 63: 0]  soMEM_Mp0_Write_tkeep,
    output           soMEM_Mp0_Write_tlast,
    output           soMEM_Mp0_Write_tvalid,
    input            soMEM_Mp0_Write_tready,
    
    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    output [  3: 0]  moMEM_Mp1_AWID,
    output [ 32: 0]  moMEM_Mp1_AWADDR,
    output [  7: 0]  moMEM_Mp1_AWLEN,
    output [  3: 0]  moMEM_Mp1_AWSIZE,
    output [  1: 0]  moMEM_Mp1_AWBURST,
    output           moMEM_Mp1_AWVALID,
    input            moMEM_Mp1_AWREADY,
    output [511: 0]  moMEM_Mp1_WDATA,
    output [ 63: 0]  moMEM_Mp1_WSTRB,
    output           moMEM_Mp1_WLAST,
    output           moMEM_Mp1_WVALID,
    input            moMEM_Mp1_WREADY,
    input  [  3: 0]  moMEM_Mp1_BID,
    input  [  1: 0]  moMEM_Mp1_BRESP,
    input            moMEM_Mp1_BVALID,
    output           moMEM_Mp1_BREADY,
    output [  3: 0]  moMEM_Mp1_ARID,
    output [ 32: 0]  moMEM_Mp1_ARADDR,
    output [  7: 0]  moMEM_Mp1_ARLEN,
    output [  3: 0]  moMEM_Mp1_ARSIZE,
    output [  1: 0]  moMEM_Mp1_ARBURST,
    output           moMEM_Mp1_ARVALID,
    input            moMEM_Mp1_ARREADY,
    input  [  3: 0]  moMEM_Mp1_RID,
    input  [511: 0]  moMEM_Mp1_RDATA,
    input  [  1: 0]  moMEM_Mp1_RRESP,
    input            moMEM_Mp1_RLAST,
    input            moMEM_Mp1_RVALID,
    output           moMEM_Mp1_RREADY,
    
    //--------------------------------------------------------
    //-- SHELL / Mmio / AppFlash Interface
    //--------------------------------------------------------
    //---- [DIAG_CTRL_1] -----------------
    input  [  1: 0]  piSHL_Mmio_Mc1_MemTestCtrl,
    //---- [DIAG_STAT_1] -----------------
    output [  1: 0]  poSHL_Mmio_Mc1_MemTestStat,
    //---- [DIAG_CTRL_2] -----------------
    input  [  1: 0]  piSHL_Mmio_UdpEchoCtrl,
    input            piSHL_Mmio_UdpPostDgmEn,
    input            piSHL_Mmio_UdpCaptDgmEn,
    input  [  1: 0]  piSHL_Mmio_TcpEchoCtrl,
    input            piSHL_Mmio_TcpPostSegEn,
    input            piSHL_Mmio_TcpCaptSegEn,
    //---- [APP_RDROL] -------------------
    output [ 15: 0]  poSHL_Mmio_RdReg,
    //--- [APP_WRROL] --------------------
    input  [ 15: 0]  piSHL_Mmio_WrReg,
    
    //--------------------------------------------------------
    //-- TOP : Secondary Clock (Asynchronous)
    //--------------------------------------------------------
    input            piTOP_250_00Clk,
    
    //------------------------------------------------
    //-- FMC Interface
    //------------------------------------------------
    input  [ 31: 0]  piFMC_ROLE_rank,
    input  [ 31: 0]  piFMC_ROLE_size,
    
    output poVoid
); // End of PortList


  // *****************************************************************************
  // **  STRUCTURE
  // *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================



  //============================================================================
  //  INSTANTIATIONS
  //============================================================================



endmodule
