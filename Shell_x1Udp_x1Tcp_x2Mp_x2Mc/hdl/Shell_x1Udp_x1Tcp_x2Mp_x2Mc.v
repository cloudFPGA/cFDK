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
  parameter gMmioAddrWidth       = 8,        // Default is 8-bits
  parameter gMmioDataWidth       = 8         // Default is 8-bits

) (

  //------------------------------------------------------
  //-- TOP / Input Clocks and Resets from topFMKU60
  //------------------------------------------------------
  input           piTOP_156_25Rst,
  input           piTOP_156_25Clk,
  
  //------------------------------------------------------
  //-- CLKT / Shl / Clock Tree Interface 
  //------------------------------------------------------
  input           piCLKT_Shl_Mem0Clk_n,
  input           piCLKT_Shl_Mem0Clk_p,
  input           piCLKT_Shl_Mem1Clk_n,
  input           piCLKT_Shl_Mem1Clk_p,
  input           piCLKT_Shl_10GeClk_n,
  input           piCLKT_Shl_10GeClk_p,
  
  //------------------------------------------------------
  //-- PSOC / Shl / External Memory Interface (Emif)
  //------------------------------------------------------
  input           piPSOC_Shl_Emif_Clk,
  input           piPSOC_Shl_Emif_Cs_n,
  input           piPSOC_Shl_Emif_We_n,
  input           piPSOC_Shl_Emif_Oe_n,
  input           piPSOC_Shl_Emif_AdS_n,
  input [gMmioAddrWidth-1: 0]  
                  piPSOC_Shl_Emif_Addr,
  inout [gMmioDataWidth-1: 0]  
                  pioPSOC_Shl_Emif_Data,
                  
  //------------------------------------------------------
  //-- LED / Shl / Heart Beat Interface (Yellow LED)
  //------------------------------------------------------
  output          poSHL_Led_HeartBeat_n,
                                    
  //------------------------------------------------------
  // -- DDR4 / Shl / Memory Channel 0 Interface (Mc0)
  //------------------------------------------------------
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc0_DmDbi_n,
  inout  [71:0]   pioDDR_Shl_Mem_Mc0_Dq,
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc0_Dqs_n,
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc0_Dqs_p,
  output          poSHL_Ddr4_Mem_Mc0_Act_n,
  output [16:0]   poSHL_Ddr4_Mem_Mc0_Adr,
  output [ 1:0]   poSHL_Ddr4_Mem_Mc0_Ba,
  output [ 1:0]   poSHL_Ddr4_Mem_Mc0_Bg,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc0_Cke,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc0_Odt,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc0_Cs_n,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc0_Ck_n,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc0_Ck_p,
  output          poSHL_Ddr4_Mem_Mc0_Reset_n,

  //------------------------------------------------------
  //-- DDR4 / Shl / Memory Channel 1 Interface (Mc1)
  //------------------------------------------------------  
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc1_DmDbi_n,
  inout  [71:0]   pioDDR_Shl_Mem_Mc1_Dq,
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc1_Dqs_n,
  inout  [ 8:0]   pioDDR_Shl_Mem_Mc1_Dqs_p,
  output          poSHL_Ddr4_Mem_Mc1_Act_n,
  output [16:0]   poSHL_Ddr4_Mem_Mc1_Adr,
  output [ 1:0]   poSHL_Ddr4_Mem_Mc1_Ba,
  output [ 1:0]   poSHL_Ddr4_Mem_Mc1_Bg,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc1_Cke,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc1_Odt,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc1_Cs_n,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc1_Ck_n,
  output [ 0:0]   poSHL_Ddr4_Mem_Mc1_Ck_p,
  output          poSHL_Ddr4_Mem_Mc1_Reset_n,
  
  //------------------------------------------------------
  //-- ECON / Shl / Edge Connector Interface (SPD08-200)
  //------------------------------------------------------
  input           piECON_Shl_Eth0_10Ge0_n, 
  input           piECON_Shl_Eth0_10Ge0_p,
  output          poSHL_Econ_Eth0_10Ge0_n,
  output          poSHL_Econ_Eth0_10Ge0_p,

  //------------------------------------------------------
  //-- ROLE / Output Clock and Reset Interfaces
  //------------------------------------------------------
  output          poSHL_156_25Clk,
  output          poSHL_156_25Rst,

  //------------------------------------------------------
  //-- ROLE / Shl/ Nts0 / Udp Interface
  //------------------------------------------------------
  //-- Input AXI-Write Stream Interface ----------
  input  [ 63:0]  piROL_Shl_Nts0_Udp_Axis_tdata,
  input  [  7:0]  piROL_Shl_Nts0_Udp_Axis_tkeep,
  input           piROL_Shl_Nts0_Udp_Axis_tlast,
  input           piROL_Shl_Nts0_Udp_Axis_tvalid,
  output          poSHL_Rol_Nts0_Udp_Axis_tready,
  //-- Output AXI-Write Stream Interface ---------
  input           piROL_Shl_Nts0_Udp_Axis_tready,
  output [ 63:0]  poSHL_Rol_Nts0_Udp_Axis_tdata,
  output [  7:0]  poSHL_Rol_Nts0_Udp_Axis_tkeep,
  output          poSHL_Rol_Nts0_Udp_Axis_tlast,
  output          poSHL_Rol_Nts0_Udp_Axis_tvalid,
  
  //------------------------------------------------------
  //-- ROLE / Shl / Nts0 / Tcp Interfaces
  //------------------------------------------------------
  //-- Input AXI-Write Stream Interface ----------
  input  [ 63:0]  piROL_Shl_Nts0_Tcp_Axis_tdata,
  input  [  7:0]  piROL_Shl_Nts0_Tcp_Axis_tkeep,
  input           piROL_Shl_Nts0_Tcp_Axis_tlast,
  input           piROL_Shl_Nts0_Tcp_Axis_tvalid,
  output          poSHL_Rol_Nts0_Tcp_Axis_tready,
  //-- Output AXI-Write Stream Interface ---------
  input           piROL_Shl_Nts0_Tcp_Axis_tready,
  output [ 63:0]  poSHL_Rol_Nts0_Tcp_Axis_tdata,
  output [  7:0]  poSHL_Rol_Nts0_Tcp_Axis_tkeep,
  output          poSHL_Rol_Nts0_Tcp_Axis_tlast,
  output          poSHL_Rol_Nts0_Tcp_Axis_tvalid, 

  //----------------------------------------------------
  // ROLE / Shl/ EMIF Registers 
  //----------------------------------------------------
  input   [15:0]  piROL_SHL_EMIF_2B_Reg,
  output  [15:0]  poSHL_ROL_EMIF_2B_Reg,


  //------------------------------------------------------  
  //-- ROLE / Shl / Mem / Mp0 Interface
  //------------------------------------------------------
  //-- Memory Port #0 / S2MM-AXIS ------------------   
  //---- Stream Read Command -----------------
  input  [ 71:0]  piROL_Shl_Mem_Mp0_Axis_RdCmd_tdata,
  input           piROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid,
  output          poSHL_Rol_Mem_Mp0_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piROL_Shl_Mem_Mp0_Axis_RdSts_tready,
  output [  7:0]  poSHL_Rol_Mem_Mp0_Axis_RdSts_tdata,
  output          poSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piROL_Shl_Mem_Mp0_Axis_Read_tready,
  output [511:0]  poSHL_Rol_Mem_Mp0_Axis_Read_tdata,
  output [ 63:0]  poSHL_Rol_Mem_Mp0_Axis_Read_tkeep,
  output          poSHL_Rol_Mem_Mp0_Axis_Read_tlast,
  output          poSHL_Rol_Mem_Mp0_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [ 71:0]  piROL_Shl_Mem_Mp0_Axis_WrCmd_tdata,
  input           piROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid,
  output          poSHL_Rol_Mem_Mp0_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piROL_Shl_Mem_Mp0_Axis_WrSts_tready,
  output          poSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid,
  output [  7:0]  poSHL_Rol_Mem_Mp0_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piROL_Shl_Mem_Mp0_Axis_Write_tdata,
  input  [ 63:0]  piROL_Shl_Mem_Mp0_Axis_Write_tkeep,
  input           piROL_Shl_Mem_Mp0_Axis_Write_tlast,
  input           piROL_Shl_Mem_Mp0_Axis_Write_tvalid,
  output          poSHL_Rol_Mem_Mp0_Axis_Write_tready, 
  
  //------------------------------------------------------
  //-- ROLE / Shl / Mem / Mp1 Interface
  //------------------------------------------------------
  //-- Memory Port #1 / S2MM-AXIS ------------------
  //---- Stream Read Command -----------------
  input  [ 71:0]  piROL_Shl_Mem_Mp1_Axis_RdCmd_tdata,
  input           piROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid,
  output          poSHL_Rol_Mem_Mp1_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piROL_Shl_Mem_Mp1_Axis_RdSts_tready,
  output [  7:0]  poSHL_Rol_Mem_Mp1_Axis_RdSts_tdata,
  output          poSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piROL_Shl_Mem_Mp1_Axis_Read_tready,
  output [511:0]  poSHL_Rol_Mem_Mp1_Axis_Read_tdata,
  output [ 63:0]  poSHL_Rol_Mem_Mp1_Axis_Read_tkeep,
  output          poSHL_Rol_Mem_Mp1_Axis_Read_tlast,
  output          poSHL_Rol_Mem_Mp1_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [ 71:0]  piROL_Shl_Mem_Mp1_Axis_WrCmd_tdata,
  input           piROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid,
  output          poSHL_Rol_Mem_Mp1_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piROL_Shl_Mem_Mp1_Axis_WrSts_tready,
  output          poSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid,
  output [  7:0]  poSHL_Rol_Mem_Mp1_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piROL_Shl_Mem_Mp1_Axis_Write_tdata,
  input  [ 63:0]  piROL_Shl_Mem_Mp1_Axis_Write_tkeep,
  input           piROL_Shl_Mem_Mp1_Axis_Write_tlast,
  input           piROL_Shl_Mem_Mp1_Axis_Write_tvalid,
  output          poSHL_Rol_Mem_Mp1_Axis_Write_tready

);  // End of PortList


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  
  //-- Global Clock and Reset used by the entire SHELL -------------------------
  //---- This clock is generated by the ETH core and runs at 156.25MHz --------- 
  wire          sETH0_ShlClk;
  wire          sETH0_ShlRst;
  wire          sETH0_CoreResetDone;  
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : ETH0 <--> NTS0 
  //--------------------------------------------------------
  //---- AXI-Write Stream Interface : ETH0 --> NTS0 --------
  wire [ 63:0]  sETH0_Nts0_Axis_tdata;
  wire [  7:0]  sETH0_Nts0_Axis_tkeep;
  wire          sETH0_Nts0_Axis_tvalid;
  wire          sETH0_Nts0_Axis_tlast;
  wire          sNTS0_Eth0_Axis_tready;
  //---- AXI-Write Stream Interface : NTS0 --> ETH0 --------
  wire [ 63:0]  sNTS0_Eth0_Axis_tdata;
  wire [  7:0]  sNTS0_Eth0_Axis_tkeep;
  wire          sNTS0_Eth0_Axis_tvalid;
  wire          sNTS0_Eth0_Axis_tlast;
  wire          sETH0_Nts0_Axis_tready;
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : NTS0 <--> MEM
  //--------------------------------------------------------
  //----  Transmit Path --------------------------
  //------  Stream Read Command --------------
  wire [ 71:0]  sNTS0_Mem_TxP_Axis_RdCmd_tdata;
  wire          sNTS0_Mem_TxP_Axis_RdCmd_tvalid;
  wire          sMEM_Nts0_TxP_Axis_RdCmd_tready;
  //------ Stream Read Status ----------------
  wire          sNTS0_Mem_TxP_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Nts0_TxP_Axis_RdSts_tdata;
  wire          sMEM_Nts0_TxP_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --------
  wire          sNTS0_Mem_TxP_Axis_Read_tready;
  wire [ 63:0]  sMEM_Nts0_TxP_Axis_Read_tdata;
  wire [  7:0]  sMEM_Nts0_TxP_Axis_Read_tkeep;
  wire          sMEM_Nts0_TxP_Axis_Read_tlast;
  wire          sMEM_Nts0_TxP_Axis_Read_tvalid;
  //------ Stream Write Command --------------
  wire [ 71:0]  sNTS0_Mem_TxP_Axis_WrCmd_tdata;
  wire          sNTS0_Mem_TxP_Axis_WrCmd_tvalid;
  wire          sMEM_Nts0_TxP_Axis_WrCmd_tready;
  //------ Stream Write Status ---------------
  wire          sNTS0_Mem_TxP_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Nts0_TxP_Axis_WrSts_tdata;
  wire          sMEM_Nts0_TxP_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---------
  wire [ 63:0]  sNTS0_Mem_TxP_Axis_Write_tdata;
  wire [  7:0]  sNTS0_Mem_TxP_Axis_Write_tkeep;
  wire          sNTS0_Mem_TxP_Axis_Write_tlast;
  wire          sNTS0_Mem_TxP_Axis_Write_tvalid;
  wire          sMEM_Nts0_TxP_Axis_Write_tready;
  //---- Receive Path ----------------------------
  //------ Stream Read Command ---------------
  wire [ 71:0]  sNTS0_Mem_RxP_Axis_RdCmd_tdata;
  wire          sNTS0_Mem_RxP_Axis_RdCmd_tvalid;
  wire          sMEM_Nts0_RxP_Axis_RdCmd_tready;
  //------ Stream Read Status ----------------
  wire          sNTS0_Mem_RxP_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Nts0_RxP_Axis_RdSts_tdata;
  wire          sMEM_Nts0_RxP_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --------
  wire          sNTS0_Mem_RxP_Axis_Read_tready;
  wire [ 63:0]  sMEM_Nts0_RxP_Axis_Read_tdata;
  wire [  7:0]  sMEM_Nts0_RxP_Axis_Read_tkeep;
  wire          sMEM_Nts0_RxP_Axis_Read_tlast;
  wire          sMEM_Nts0_RxP_Axis_Read_tvalid;
  //------ Stream Write Command --------------
  wire [ 71:0]  sNTS0_Mem_RxP_Axis_WrCmd_tdata;
  wire          sNTS0_Mem_RxP_Axis_WrCmd_tvalid;
  wire          sMEM_Nts0_RxP_Axis_WrCmd_tready;
  //------ Stream Write Status ---------------
  wire          sNTS0_Mem_RxP_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Nts0_RxP_Axis_WrSts_tdata;
  wire          sMEM_Nts0_RxP_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---------
  wire [ 63:0]  sNTS0_Mem_RxP_Axis_Write_tdata;
  wire [  7:0]  sNTS0_Mem_RxP_Axis_Write_tkeep;
  wire          sNTS0_Mem_RxP_Axis_Write_tlast;
  wire          sNTS0_Mem_RxP_Axis_Write_tvalid;
  wire          sMEM_Nts0_RxP_Axis_Write_tready;
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : ROLE <--> MEM
  //--------------------------------------------------------
  //-- Memory Port #0 ------------------------------
  //------  Stream Read Command --------------
  wire [ 71:0]  sROL_Mem_Mp0_Axis_RdCmd_tdata;
  wire          sROL_Mem_Mp0_Axis_RdCmd_tvalid;
  wire          sMEM_Rol_Mp0_Axis_RdCmd_tready;
  //------ Stream Read Status ----------------
  wire          sROL_Mem_Mp0_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Rol_Mp0_Axis_RdSts_tdata;
  wire          sMEM_Rol_Mp0_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --------
  wire          sROL_Mem_Mp0_Axis_Read_tready;
  wire [511:0]  sMEM_Rol_Mp0_Axis_Read_tdata;
  wire [ 63:0]  sMEM_Rol_Mp0_Axis_Read_tkeep;
  wire          sMEM_Rol_Mp0_Axis_Read_tlast;
  wire          sMEM_Rol_Mp0_Axis_Read_tvalid;
  //------ Stream Write Command --------------
  wire [ 71:0]  sROL_Mem_Mp0_Axis_WrCmd_tdata;
  wire          sROL_Mem_Mp0_Axis_WrCmd_tvalid;
  wire          sMEM_Rol_Mp0_Axis_WrCmd_tready;
  //------ Stream Write Status ---------------
  wire          sROL_Mem_Mp0_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Rol_Mp0_Axis_WrSts_tdata;
  wire          sMEM_Rol_Mp0_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---------
  wire [511:0]  sROL_Mem_Mp0_Axis_Write_tdata;
  wire [ 63:0]  sROL_Mem_Mp0_Axis_Write_tkeep;
  wire          sROL_Mem_Mp0_Axis_Write_tlast;
  wire          sROL_Mem_Mp0_Axis_Write_tvalid;
  wire          sMEM_Rol_Mp0_Axis_Write_tready;
  //---- Receive Path ----------------------------
  //------ Stream Read Command ---------------
  wire [ 71:0]  sROL_Mem_Mp1_Axis_RdCmd_tdata;
  wire          sROL_Mem_Mp1_Axis_RdCmd_tvalid;
  wire          sMEM_Rol_Mp1_Axis_RdCmd_tready;
  //------ Stream Read Status ----------------
  wire          sROL_Mem_Mp1_Axis_RdSts_tready;
  wire [  7:0]  sMEM_Rol_Mp1_Axis_RdSts_tdata;
  wire          sMEM_Rol_Mp1_Axis_RdSts_tvalid;
  //------ Stream Data Output Channel --------
  wire          sROL_Mem_Mp1_Axis_Read_tready;
  wire [511:0]  sMEM_Rol_Mp1_Axis_Read_tdata;
  wire [ 63:0]  sMEM_Rol_Mp1_Axis_Read_tkeep;
  wire          sMEM_Rol_Mp1_Axis_Read_tlast;
  wire          sMEM_Rol_Mp1_Axis_Read_tvalid;
  //------ Stream Write Command --------------
  wire [ 71:0]  sROL_Mem_Mp1_Axis_WrCmd_tdata;
  wire          sROL_Mem_Mp1_Axis_WrCmd_tvalid;
  wire          sMEM_Rol_Mp1_Axis_WrCmd_tready;
  //------ Stream Write Status ---------------
  wire          sROL_Mem_Mp1_Axis_WrSts_tready;
  wire [  7:0]  sMEM_Rol_Mp1_Axis_WrSts_tdata;
  wire          sMEM_Rol_Mp1_Axis_WrSts_tvalid;
  //------ Stream Data Input Channel ---------
  wire [511:0]  sROL_Mem_Mp1_Axis_Write_tdata;
  wire [ 63:0]  sROL_Mem_Mp1_Axis_Write_tkeep;
  wire          sROL_Mem_Mp1_Axis_Write_tlast;
  wire          sROL_Mem_Mp1_Axis_Write_tvalid;
  wire          sMEM_Rol_Mp1_Axis_Write_tready;
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : ROLE <--> NTS0
  //--------------------------------------------------------
  //---- Udp Interface ---------------------------  
  //------ Input AXI-Write Stream Interface ------
  wire [ 63:0]  sROL_Nts0_Udp_Axis_tdata;
  wire [  7:0]  sROL_Nts0_Udp_Axis_tkeep;
  wire          sROL_Nts0_Udp_Axis_tlast;
  wire          sROL_Nts0_Udp_Axis_tvalid;
  wire          sNTS0_Rol_Udp_Axis_tready;
  //------ Output AXI-Write Stream Interface -----
  wire          sROL_Nts0_Udp_Axis_tready;
  wire [ 63:0]  sNTS0_Rol_Udp_Axis_tdata;
  wire [  7:0]  sNTS0_Rol_Udp_Axis_tkeep;
  wire          sNTS0_Rol_Udp_Axis_tlast;
  wire          sNTS0_Rol_Udp_Axis_tvalid;  
  //---- Tcp Interface ---------------------------
  //------ Input AXI-Write Stream Interface ------
  wire [ 63:0]  sROL_Nts0_Tcp_Axis_tdata;
  wire [  7:0]  sROL_Nts0_Tcp_Axis_tkeep;
  wire          sROL_Nts0_Tcp_Axis_tlast;
  wire          sROL_Nts0_Tcp_Axis_tvalid;
  wire          sNTS0_Rol_Tcp_Axis_tready;
  //------ Output AXI-Write Stream Interface -----
  wire          sROL_Nts0_Tcp_Axis_tready;
  wire [ 63:0]  sNTS0_Rol_Tcp_Axis_tdata;
  wire [  7:0]  sNTS0_Rol_Tcp_Axis_tkeep;
  wire          sNTS0_Rol_Tcp_Axis_tlast;
  wire          sNTS0_Rol_Tcp_Axis_tvalid;
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : MMIO <--> ETH0 
  //--------------------------------------------------------
  //---- Configuration Registers Interface -------  
  //---- Physiscal Registers Interface -----------
  //------ [PHY_STATUS] --------------------------
  wire          sMEM_Mmio_Mc0InitCalComplete;
  wire          sMEM_Mmio_Mc1InitCalComplete;
  wire          sETH0_Mmio_CoreReady;
  wire          sETH0_Mmio_QpllLock;
  //------ [PHY_ETH0] ----------------------------
  wire          sMMIO_Eth0_RxEqualizerMode;
  wire  [ 3:0]  sMMIO_Eth0_TxDriverSwing;
  wire  [ 4:0]  sMMIO_Eth0_TxPreCursor;
  wire  [ 4:0]  sMMIO_Eth0_TxPostCursor;
  //---- Layer-2 Registers Interface -------------
  wire  [47:0]  sMMIO_Nts0_MacAddress;
  //---- Layer-3 Registers Interface -------------
  wire  [31:0]  sMMIO_Nts0_IpAddress;
  //---- Pcie Registers Interface ----------------
  //---- Diagnostic Registers Interface ----------
  wire          sMMIO_Eth0_PcsLoopbackEn;
  wire          sMMIO_Eth0_MacLoopbackEn;
  wire          sMMIO_Eth0_MacAddrSwapEn;

  //-- END OF SIGNAL DECLARATIONS ----------------------------------------------

    
  //============================================================================
  //  INST: MMIIO CLIENT
  //============================================================================
  MmioClient_A8_D8 #(

    .gSecurityPriviledges (gSecurityPriviledges),
    .gBitstreamUsage      (gBitstreamUsage)
    
  ) MMIO (
   
    //-- Global Clock used by the entire SHELL --------
    .piShlClk                       (sETH0_ShlClk),
 
    //-- Global Reset used by the entire TOP ----------
    .piTopRst                       (piTOP_156_25Rst),   
     
    //-- PSOC : Mmio Bus Interface --------------------
    .piPSOC_Mmio_Clk                (piPSOC_Shl_Emif_Clk),
    .piPSOC_Mmio_Cs_n               (piPSOC_Shl_Emif_Cs_n),
    .piPSOC_Mmio_We_n               (piPSOC_Shl_Emif_We_n),
    .piPSOC_Mmio_AdS_n              (piPSOC_Shl_Emif_AdS_n),
    .piPSOC_Mmio_Oe_n               (piPSOC_Shl_Emif_Oe_n),
    .piPSOC_Mmio_Addr               (piPSOC_Shl_Emif_Addr),
    .pioPSOC_Mmio_Data              (pioPSOC_Shl_Emif_Data),
 
    //-- MEM : Status inputs and Control outputs ------
    .piMEM_Mmio_Mc0InitCalComplete  (sMEM_Mmio_Mc0InitCalComplete),
    .piMEM_Mmio_Mc1InitCalComplete  (sMEM_Mmio_Mc1InitCalComplete),
    
    //-- ETH0: Status inputs and Control outputs ------
    .piETH0_Mmio_CoreReady          (sETH0_Mmio_CoreReady),
    .piETH0_Mmio_QpllLock           (sETH0_Mmio_QpllLock),
    .poMMIO_Eth0_RxEqualizerMode    (sMMIO_Eth0_RxEqualizerMode),
    .poMMIO_Eth0_TxDriverSwing      (sMMIO_Eth0_TxDriverSwing),
    .poMMIO_Eth0_TxPreCursor        (sMMIO_Eth0_TxPreCursor),
    .poMMIO_Eth0_TxPostCursor       (sMMIO_Eth0_TxPostCursor),
    .poMMIO_Eth0_PcsLoopbackEn      (sMMIO_Eth0_PcsLoopbackEn),
    .poMMIO_Eth0_MacLoopbackEn      (sMMIO_Eth0_MacLoopbackEn),
    .poMMIO_Eth0_MacAddrSwapEn      (sMMIO_Eth0_MacAddrSwapEn),
    
    //-- NTS0: Status inputs and Control outputs ------
    .poMMIO_Nts0_MacAddress         (sMMIO_Nts0_MacAddress),
    .poMMIO_Nts0_IpAddress          (sMMIO_Nts0_IpAddress),
    
    // ROLE EMIF Registers 
    .poMMIO_ROLE_2B_Reg             (poSHL_ROL_EMIF_2B_Reg),
    .piMMIO_ROLE_2B_Reg             (piROL_SHL_EMIF_2B_Reg),
 
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
        .piCLKT_Gt_RefClk_n           (piCLKT_Shl_10GeClk_n),
        .piCLKT_Gt_RefClk_p           (piCLKT_Shl_10GeClk_p),
        .piTOP_Reset                  (piTOP_156_25Rst),
          
        //-- Clocks and Resets outputs ---------------
        .poETH0_CoreClk               (sETH0_ShlClk),
        .poETH0_CoreResetDone         (sETH0_CoreResetDone),
           
        //-- MMIO : Control inputs and Status outputs
        .piMMIO_Eth0_RxEqualizerMode  (sMMIO_Eth0_RxEqualizerMode),
        .piMMIO_Eth0_TxDriverSwing    (sMMIO_Eth0_TxDriverSwing),
        .piMMIO_Eth0_TxPreCursor      (sMMIO_Eth0_TxPreCursor),
        .piMMIO_Eth0_TxPostCursor     (sMMIO_Eth0_TxPostCursor),
        .piMMIO_Eth0_PcsLoopbackEn    (sMMIO_Eth0_PcsLoopbackEn),
        .piMMIO_Eth0_MacLoopbackEn    (sMMIO_Eth0_MacLoopbackEn),
        .piMMIO_MacAddrSwapEn         (sMMIO_Eth0_MacAddrSwapEn),
        .poETH0_Mmio_CoreReady        (sETH0_Mmio_CoreReady),
        .poETH0_Mmio_QpllLock         (sETH0_Mmio_QpllLock),
        
        //OBSOLETE-20180517 .poETH0_Mmio_ResetDone        (sETH0_Mmio_ResetDone),
    
        //-- ECON : Gigabit Transceivers -------------
        .piECON_Eth0_Gt_n             (piECON_Shl_Eth0_10Ge0_n),
        .piECON_Eth0_Gt_p             (piECON_Shl_Eth0_10Ge0_p),
        .poETH0_Econ_Gt_n             (poSHL_Econ_Eth0_10Ge0_n),
        .poETH0_Econ_Gt_p             (poSHL_Econ_Eth0_10Ge0_p),

        //-- NTS0: Network-Transport-Session ---------
        //---- Input AXI-Write Stream Interface ------
        .piLY3_Eth0_Axis_tdata        (sNTS0_Eth0_Axis_tdata),
        .piLY3_Eth0_Axis_tkeep        (sNTS0_Eth0_Axis_tkeep),
        .piLY3_Eth0_Axis_tvalid       (sNTS0_Eth0_Axis_tvalid),
        .piLY3_Eth0_Axis_tlast        (sNTS0_Eth0_Axis_tlast),
        .poETH0_Ly3_Axis_tready       (sETH0_Nts0_Axis_tready),
        //---- Output AXI-Write Stream Interface -----
        .piLY3_Eth0_Axis_tready       (sNTS0_Eth0_Axis_tready),
        .poETH0_Ly3_Axis_tdata        (sETH0_Nts0_Axis_tdata),
        .poETH0_Ly3_Axis_tkeep        (sETH0_Nts0_Axis_tkeep),
        .poETH0_Ly3_Axis_tvalid       (sETH0_Nts0_Axis_tvalid),
        .poETH0_Ly3_Axis_tlast        (sETH0_Nts0_Axis_tlast)

      );  // End of UserCfg.ETH0
    
    end // if ((gBitstreamUsage == "user") && (gSecurityPriviledges == "user"))
    
    else if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) begin: SuperCfg
    
      //========================================================================
      //  INST: 10G ETHERNET SUBSYSTEM W/ LOOPBACK SUPPORT
      //========================================================================
      TenGigEth_Flash ETH0 (
       
        //-- Clocks and Resets inputs ----------------
        .piTOP_156_25Clk              (piTOP_156_25Clk),    // Freerunning
        .piCLKT_Gt_RefClk_n           (piCLKT_Shl_10GeClk_n),
        .piCLKT_Gt_RefClk_p           (piCLKT_Shl_10GeClk_p),
        .piTOP_Reset                  (piTOP_156_25Rst),
        
        //-- Clocks and Resets outputs ---------------
        .poETH0_CoreClk               (sETH0_ShlClk),
        .poETH0_CoreResetDone         (sETH0_CoreResetDone),
        
        //-- MMIO : Control inputs and Status outputs 
        .piMMIO_Eth0_RxEqualizerMode  (sMMIO_Eth0_RxEqualizerMode),
        .piMMIO_Eth0_PcsLoopbackEn    (sMMIO_Eth0_PcsLoopbackEn),
        .piMMIO_Eth0_MacLoopbackEn    (sMMIO_Eth0_MacLoopbackEn),
        .piMMIO_Eth0_MacAddrSwapEn    (sMMIO_Eth0_MacAddrSwapEn),
        .poETH0_Mmio_CoreReady        (sETH0_Mmio_CoreReady),
        .poETH0_Mmio_QpllLock         (sETH0_Mmio_QpllLock),
        
        //-- ECON : Gigabit Transceivers -------------
        .piECON_Eth0_Gt_n             (piECON_Shl_Eth0_10Ge0_n),
        .piECON_Eth0_Gt_p             (piECON_Shl_Eth0_10Ge0_p),
        .poETH0_Econ_Gt_n             (poSHL_Econ_Eth0_10Ge0_n),
        .poETH0_Econ_Gt_p             (poSHL_Econ_Eth0_10Ge0_p),
        
        //-- NTS0: Network-Transport-Session ---------
        //---- Input AXI-Write Stream Interface ------
        .piLY3_Eth0_Axis_tdata        (sNTS0_Eth0_Axis_tdata),
        .piLY3_Eth0_Axis_tkeep        (sNTS0_Eth0_Axis_tkeep),
        .piLY3_Eth0_Axis_tvalid       (sNTS0_Eth0_Axis_tvalid),
        .piLY3_Eth0_Axis_tlast        (sNTS0_Eth0_Axis_tlast),
        .poETH0_Ly3_Axis_tready       (sETH0_Nts0_Axis_tready),
        //---- Output AXI-Write Stream Interface -----
        .piLY3_Eth0_Axis_tready       (sNTS0_Eth0_Axis_tready),
        .poETH0_Ly3_Axis_tdata        (sETH0_Nts0_Axis_tdata),
        .poETH0_Ly3_Axis_tkeep        (sETH0_Nts0_Axis_tkeep),
        .poETH0_Ly3_Axis_tvalid       (sETH0_Nts0_Axis_tvalid),
        .poETH0_Ly3_Axis_tlast        (sETH0_Nts0_Axis_tlast)
       
       );  // End of SuperCfg.ETH0 
       
    end // if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super"))
    
  endgenerate
  

  //============================================================================
  //  INST: NETWORK+TRANSPORT+SESSION SUBSYSTEM (OSI Network Layers 3+4+5)
  //============================================================================
  NetworkTransportSession_TcpIp NTS0 (
  
    //-- Global Clock used by the entire SHELL --------------
    .piShlClk                         (sETH0_ShlClk),
  
    //-- Global Reset used by the entire SHELL --------------
    .piShlRst                         (sETH0_ShlRst),
   
    //------------------------------------------------------
    //-- ETH0 / Nts0 / AXI-Write Stream Interfaces
    //------------------------------------------------------
    //-- Input AXIS Interface --------------------
    .piETH0_Nts0_Axis_tdata           (sETH0_Nts0_Axis_tdata),
    .piETH0_Nts0_Axis_tkeep           (sETH0_Nts0_Axis_tkeep),
    .piETH0_Nts0_Axis_tlast           (sETH0_Nts0_Axis_tlast),
    .piETH0_Nts0_Axis_tvalid          (sETH0_Nts0_Axis_tvalid),
    .poNTS0_Eth0_Axis_tready          (sNTS0_Eth0_Axis_tready),
    //-- Output AXIS Interface ------------------- 
    .piETH0_Nts0_Axis_tready          (sETH0_Nts0_Axis_tready),
    .poNTS0_Eth0_Axis_tdata           (sNTS0_Eth0_Axis_tdata),
    .poNTS0_Eth0_Axis_tkeep           (sNTS0_Eth0_Axis_tkeep),
    .poNTS0_Eth0_Axis_tlast           (sNTS0_Eth0_Axis_tlast),
    .poNTS0_Eth0_Axis_tvalid          (sNTS0_Eth0_Axis_tvalid),
 
    //------------------------------------------------------
    //-- MEM / Nts0 / TxP Interfaces
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS -------------------------
    //---- Stream Read Command -------------------
    .piMEM_Nts0_TxP_Axis_RdCmd_tready (sMEM_Nts0_TxP_Axis_RdCmd_tready),
    .poNTS0_Mem_TxP_Axis_RdCmd_tdata  (sNTS0_Mem_TxP_Axis_RdCmd_tdata),
    .poNTS0_Mem_TxP_Axis_RdCmd_tvalid (sNTS0_Mem_TxP_Axis_RdCmd_tvalid),
    //---- Stream Read Status ------------------
    .piMEM_Nts0_TxP_Axis_RdSts_tdata  (sMEM_Nts0_TxP_Axis_RdSts_tdata),
    .piMEM_Nts0_TxP_Axis_RdSts_tvalid (sMEM_Nts0_TxP_Axis_RdSts_tvalid),
    .poNTS0_Mem_TxP_Axis_RdSts_tready (sNTS0_Mem_TxP_Axis_RdSts_tready),
    //---- Stream Data Input Channel -----------
    .piMEM_Nts0_TxP_Axis_Read_tdata   (sMEM_Nts0_TxP_Axis_Read_tdata),
    .piMEM_Nts0_TxP_Axis_Read_tkeep   (sMEM_Nts0_TxP_Axis_Read_tkeep),
    .piMEM_Nts0_TxP_Axis_Read_tlast   (sMEM_Nts0_TxP_Axis_Read_tlast),
    .piMEM_Nts0_TxP_Axis_Read_tvalid  (sMEM_Nts0_TxP_Axis_Read_tvalid),
    .poNTS0_Mem_TxP_Axis_Read_tready  (sNTS0_Mem_TxP_Axis_Read_tready),
    //---- Stream Write Command ----------------
    .piMEM_Nts0_TxP_Axis_WrCmd_tready (sMEM_Nts0_TxP_Axis_WrCmd_tready),
    .poNTS0_Mem_TxP_Axis_WrCmd_tdata  (sNTS0_Mem_TxP_Axis_WrCmd_tdata),
    .poNTS0_Mem_TxP_Axis_WrCmd_tvalid (sNTS0_Mem_TxP_Axis_WrCmd_tvalid),
    //---- Stream Write Status -----------------
    .piMEM_Nts0_TxP_Axis_WrSts_tdata  (sMEM_Nts0_TxP_Axis_WrSts_tdata),
    .piMEM_Nts0_TxP_Axis_WrSts_tvalid (sMEM_Nts0_TxP_Axis_WrSts_tvalid),
    .poNTS0_Mem_TxP_Axis_WrSts_tready (sNTS0_Mem_TxP_Axis_WrSts_tready),
    //---- Stream Data Output Channel ----------
    .piMEM_Nts0_TxP_Axis_Write_tready (sMEM_Nts0_TxP_Axis_Write_tready),
    .poNTS0_Mem_TxP_Axis_Write_tdata  (sNTS0_Mem_TxP_Axis_Write_tdata),
    .poNTS0_Mem_TxP_Axis_Write_tkeep  (sNTS0_Mem_TxP_Axis_Write_tkeep),
    .poNTS0_Mem_TxP_Axis_Write_tlast  (sNTS0_Mem_TxP_Axis_Write_tlast),
    .poNTS0_Mem_TxP_Axis_Write_tvalid (sNTS0_Mem_TxP_Axis_Write_tvalid),

    //------------------------------------------------------
    //-- MEM / Nts0 / RxP Interfaces
    //------------------------------------------------------
    //-- Receive Path / S2MM-AXIS ------------------
    //---- Stream Read Command -----------------
    .piMEM_Nts0_RxP_Axis_RdCmd_tready (sMEM_Nts0_RxP_Axis_RdCmd_tready),
    .poNTS0_Mem_RxP_Axis_RdCmd_tdata  (sNTS0_Mem_RxP_Axis_RdCmd_tdata),
    .poNTS0_Mem_RxP_Axis_RdCmd_tvalid (sNTS0_Mem_RxP_Axis_RdCmd_tvalid),
    //---- Stream Read Status ------------------
    .piMEM_Nts0_RxP_Axis_RdSts_tdata  (sMEM_Nts0_RxP_Axis_RdSts_tdata),
    .piMEM_Nts0_RxP_Axis_RdSts_tvalid (sMEM_Nts0_RxP_Axis_RdSts_tvalid),
    .poNTS0_Mem_RxP_Axis_RdSts_tready (sNTS0_Mem_RxP_Axis_RdSts_tready),
    //---- Stream Data Input Channel ----------
    .piMEM_Nts0_RxP_Axis_Read_tdata   (sMEM_Nts0_RxP_Axis_Read_tdata),
    .piMEM_Nts0_RxP_Axis_Read_tkeep   (sMEM_Nts0_RxP_Axis_Read_tkeep),
    .piMEM_Nts0_RxP_Axis_Read_tlast   (sMEM_Nts0_RxP_Axis_Read_tlast),
    .piMEM_Nts0_RxP_Axis_Read_tvalid  (sMEM_Nts0_RxP_Axis_Read_tvalid),
    .poNTS0_Mem_RxP_Axis_Read_tready  (sNTS0_Mem_RxP_Axis_Read_tready),
    //---- Stream Write Command ----------------
    .piMEM_Nts0_RxP_Axis_WrCmd_tready (sMEM_Nts0_RxP_Axis_WrCmd_tready),
    .poNTS0_Mem_RxP_Axis_WrCmd_tdata  (sNTS0_Mem_RxP_Axis_WrCmd_tdata),
    .poNTS0_Mem_RxP_Axis_WrCmd_tvalid (sNTS0_Mem_RxP_Axis_WrCmd_tvalid),
    //---- Stream Write Status -----------------
    .piMEM_Nts0_RxP_Axis_WrSts_tdata  (sMEM_Nts0_RxP_Axis_WrSts_tdata),
    .piMEM_Nts0_RxP_Axis_WrSts_tvalid (sMEM_Nts0_RxP_Axis_WrSts_tvalid),
    .poNTS0_Mem_RxP_Axis_WrSts_tready (sNTS0_Mem_RxP_Axis_WrSts_tready),
    //---- Stream Data Output Channel ----------
    .piMEM_Nts0_RxP_Axis_Write_tready (sMEM_Nts0_RxP_Axis_Write_tready),  
    .poNTS0_Mem_RxP_Axis_Write_tdata  (sNTS0_Mem_RxP_Axis_Write_tdata),
    .poNTS0_Mem_RxP_Axis_Write_tkeep  (sNTS0_Mem_RxP_Axis_Write_tkeep),
    .poNTS0_Mem_RxP_Axis_Write_tlast  (sNTS0_Mem_RxP_Axis_Write_tlast),
    .poNTS0_Mem_RxP_Axis_Write_tvalid (sNTS0_Mem_RxP_Axis_Write_tvalid),
    
    //------------------------------------------------------
    //-- ROLE / Nts0 / Udp Interfaces
    //------------------------------------------------------
    //-- Input AXI-Write Stream Interface ----------
    .piROL_Nts0_Udp_Axis_tdata        (piROL_Shl_Nts0_Udp_Axis_tdata),
    .piROL_Nts0_Udp_Axis_tkeep        (piROL_Shl_Nts0_Udp_Axis_tkeep),
    .piROL_Nts0_Udp_Axis_tlast        (piROL_Shl_Nts0_Udp_Axis_tlast),
    .piROL_Nts0_Udp_Axis_tvalid       (piROL_Shl_Nts0_Udp_Axis_tvalid),
    .poNTS0_Rol_Udp_Axis_tready       (poSHL_Rol_Nts0_Udp_Axis_tready),
    //-- Output AXI-Write Stream Interface ---------
    .piROL_Nts0_Udp_Axis_tready       (piROL_Shl_Nts0_Udp_Axis_tready),
    .poNTS0_Rol_Udp_Axis_tdata        (poSHL_Rol_Nts0_Udp_Axis_tdata),
    .poNTS0_Rol_Udp_Axis_tkeep        (poSHL_Rol_Nts0_Udp_Axis_tkeep),
    .poNTS0_Rol_Udp_Axis_tlast        (poSHL_Rol_Nts0_Udp_Axis_tlast),
    .poNTS0_Rol_Udp_Axis_tvalid       (poSHL_Rol_Nts0_Udp_Axis_tvalid),

    //------------------------------------------------------
    //-- ROLE / Nts0 / TCP Interfaces
    //------------------------------------------------------
    //-- Input AXI-Write Stream Interface ----------
    .piROL_Nts0_Tcp_Axis_tdata        (piROL_Shl_Nts0_Tcp_Axis_tdata),
    .piROL_Nts0_Tcp_Axis_tkeep        (piROL_Shl_Nts0_Tcp_Axis_tkeep),
    .piROL_Nts0_Tcp_Axis_tlast        (piROL_Shl_Nts0_Tcp_Axis_tlast),
    .piROL_Nts0_Tcp_Axis_tvalid       (piROL_Shl_Nts0_Tcp_Axis_tvalid),
    .poNTS0_Rol_Tcp_Axis_tready       (poSHL_Rol_Nts0_Tcp_Axis_tready),
    //-- Output AXI-Write Stream Interface ---------
    .piROL_Nts0_Tcp_Axis_tready       (piROL_Shl_Nts0_Tcp_Axis_tready),
    .poNTS0_Rol_Tcp_Axis_tdata        (poSHL_Rol_Nts0_Tcp_Axis_tdata),
    .poNTS0_Rol_Tcp_Axis_tkeep        (poSHL_Rol_Nts0_Tcp_Axis_tkeep),
    .poNTS0_Rol_Tcp_Axis_tlast        (poSHL_Rol_Nts0_Tcp_Axis_tlast),
    .poNTS0_Rol_Tcp_Axis_tvalid       (poSHL_Rol_Nts0_Tcp_Axis_tvalid),
    
    //------------------------------------------------------
    //-- MMIO / Nts0 / Interfaces
    //------------------------------------------------------
    .piMMIO_Nts0_MacAddress           (sMMIO_Nts0_MacAddress),
    .piMMIO_Nts0_IpAddress            (sMMIO_Nts0_IpAddress),
    
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
    .piCLKT_Mem0Clk_n                 (piCLKT_Shl_Mem0Clk_n),
    .piCLKT_Mem0Clk_p                 (piCLKT_Shl_Mem0Clk_p),
    .piCLKT_Mem1Clk_n                 (piCLKT_Shl_Mem1Clk_n),
    .piCLKT_Mem1Clk_p                 (piCLKT_Shl_Mem1Clk_p),
  
    //-- Control Inputs and Status Ouputs ------------------
    .poMmio_Mc0_InitCalComplete       (sMEM_Mmio_Mc0InitCalComplete),
    .poMmio_Mc1_InitCalComplete       (sMEM_Mmio_Mc1InitCalComplete),

    //------------------------------------------------------
    //-- NTS0 / Mem / TxP Interface
    //------------------------------------------------------
    //-- Transmit Path / S2MM-AXIS ---------------
    //---- Stream Read Command ---------------
    .piNTS0_Mem_TxP_Axis_RdCmd_tdata  (sNTS0_Mem_TxP_Axis_RdCmd_tdata),
    .piNTS0_Mem_TxP_Axis_RdCmd_tvalid (sNTS0_Mem_TxP_Axis_RdCmd_tvalid),
    .poMEM_Nts0_TxP_Axis_RdCmd_tready (sMEM_Nts0_TxP_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piNTS0_Mem_TxP_Axis_RdSts_tready (sNTS0_Mem_TxP_Axis_RdSts_tready),
    .poMEM_Nts0_TxP_Axis_RdSts_tdata  (sMEM_Nts0_TxP_Axis_RdSts_tdata),
    .poMEM_Nts0_TxP_Axis_RdSts_tvalid (sMEM_Nts0_TxP_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piNTS0_Mem_TxP_Axis_Read_tready  (sNTS0_Mem_TxP_Axis_Read_tready),
    .poMEM_Nts0_TxP_Axis_Read_tdata   (sMEM_Nts0_TxP_Axis_Read_tdata),
    .poMEM_Nts0_TxP_Axis_Read_tkeep   (sMEM_Nts0_TxP_Axis_Read_tkeep),
    .poMEM_Nts0_TxP_Axis_Read_tlast   (sMEM_Nts0_TxP_Axis_Read_tlast),
    .poMEM_Nts0_TxP_Axis_Read_tvalid  (sMEM_Nts0_TxP_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piNTS0_Mem_TxP_Axis_WrCmd_tdata  (sNTS0_Mem_TxP_Axis_WrCmd_tdata),
    .piNTS0_Mem_TxP_Axis_WrCmd_tvalid (sNTS0_Mem_TxP_Axis_WrCmd_tvalid),
    .poMEM_Nts0_TxP_Axis_WrCmd_tready (sMEM_Nts0_TxP_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piNTS0_Mem_TxP_Axis_WrSts_tready (sNTS0_Mem_TxP_Axis_WrSts_tready),
    .poMEM_Nts0_TxP_Axis_WrSts_tdata  (sMEM_Nts0_TxP_Axis_WrSts_tdata),
    .poMEM_Nts0_TxP_Axis_WrSts_tvalid (sMEM_Nts0_TxP_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piNTS0_Mem_TxP_Axis_Write_tdata  (sNTS0_Mem_TxP_Axis_Write_tdata),
    .piNTS0_Mem_TxP_Axis_Write_tkeep  (sNTS0_Mem_TxP_Axis_Write_tkeep),
    .piNTS0_Mem_TxP_Axis_Write_tlast  (sNTS0_Mem_TxP_Axis_Write_tlast),
    .piNTS0_Mem_TxP_Axis_Write_tvalid (sNTS0_Mem_TxP_Axis_Write_tvalid),
    .poMEM_Nts0_TxP_Axis_Write_tready (sMEM_Nts0_TxP_Axis_Write_tready),
    
    //------------------------------------------------------
    //-- NTS0 / Mem / Rx Interface
    //------------------------------------------------------
    //-- Receive Path  / S2MM-AXIS -----------------
    //---- Stream Read Command ---------------
    .piNTS0_Mem_RxP_Axis_RdCmd_tdata  (sNTS0_Mem_RxP_Axis_RdCmd_tdata),
    .piNTS0_Mem_RxP_Axis_RdCmd_tvalid (sNTS0_Mem_RxP_Axis_RdCmd_tvalid),
    .poMEM_Nts0_RxP_Axis_RdCmd_tready (sMEM_Nts0_RxP_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piNTS0_Mem_RxP_Axis_RdSts_tready (sNTS0_Mem_RxP_Axis_RdSts_tready),
    .poMEM_Nts0_RxP_Axis_RdSts_tdata  (sMEM_Nts0_RxP_Axis_RdSts_tdata),
    .poMEM_Nts0_RxP_Axis_RdSts_tvalid (sMEM_Nts0_RxP_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piNTS0_Mem_RxP_Axis_Read_tready  (sNTS0_Mem_RxP_Axis_Read_tready),
    .poMEM_Nts0_RxP_Axis_Read_tdata   (sMEM_Nts0_RxP_Axis_Read_tdata),
    .poMEM_Nts0_RxP_Axis_Read_tkeep   (sMEM_Nts0_RxP_Axis_Read_tkeep),
    .poMEM_Nts0_RxP_Axis_Read_tlast   (sMEM_Nts0_RxP_Axis_Read_tlast),
    .poMEM_Nts0_RxP_Axis_Read_tvalid  (sMEM_Nts0_RxP_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piNTS0_Mem_RxP_Axis_WrCmd_tdata  (sNTS0_Mem_RxP_Axis_WrCmd_tdata),
    .piNTS0_Mem_RxP_Axis_WrCmd_tvalid (sNTS0_Mem_RxP_Axis_WrCmd_tvalid),
    .poMEM_Nts0_RxP_Axis_WrCmd_tready (sMEM_Nts0_RxP_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piNTS0_Mem_RxP_Axis_WrSts_tready (sNTS0_Mem_RxP_Axis_WrSts_tready),
    .poMEM_Nts0_RxP_Axis_WrSts_tdata  (sMEM_Nts0_RxP_Axis_WrSts_tdata),
    .poMEM_Nts0_RxP_Axis_WrSts_tvalid (sMEM_Nts0_RxP_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piNTS0_Mem_RxP_Axis_Write_tdata  (sNTS0_Mem_RxP_Axis_Write_tdata),
    .piNTS0_Mem_RxP_Axis_Write_tkeep  (sNTS0_Mem_RxP_Axis_Write_tkeep),
    .piNTS0_Mem_RxP_Axis_Write_tlast  (sNTS0_Mem_RxP_Axis_Write_tlast),
    .piNTS0_Mem_RxP_Axis_Write_tvalid (sNTS0_Mem_RxP_Axis_Write_tvalid),
    .poMEM_Nts0_RxP_Axis_Write_tready (sMEM_Nts0_RxP_Axis_Write_tready),  

    //------------------------------------------------------
    // -- Physical DDR4 Interface #0
    //------------------------------------------------------
    .pioDDR_Mem_Mc0_DmDbi_n           (pioDDR_Shl_Mem_Mc0_DmDbi_n),
    .pioDDR_Mem_Mc0_Dq                (pioDDR_Shl_Mem_Mc0_Dq),
    .pioDDR_Mem_Mc0_Dqs_n             (pioDDR_Shl_Mem_Mc0_Dqs_n),
    .pioDDR_Mem_Mc0_Dqs_p             (pioDDR_Shl_Mem_Mc0_Dqs_p),    
    .poMEM_Ddr4_Mc0_Act_n             (poSHL_Ddr4_Mem_Mc0_Act_n),
    .poMEM_Ddr4_Mc0_Adr               (poSHL_Ddr4_Mem_Mc0_Adr),
    .poMEM_Ddr4_Mc0_Ba                (poSHL_Ddr4_Mem_Mc0_Ba),
    .poMEM_Ddr4_Mc0_Bg                (poSHL_Ddr4_Mem_Mc0_Bg),
    .poMEM_Ddr4_Mc0_Cke               (poSHL_Ddr4_Mem_Mc0_Cke),
    .poMEM_Ddr4_Mc0_Odt               (poSHL_Ddr4_Mem_Mc0_Odt),
    .poMEM_Ddr4_Mc0_Cs_n              (poSHL_Ddr4_Mem_Mc0_Cs_n),
    .poMEM_Ddr4_Mc0_Ck_n              (poSHL_Ddr4_Mem_Mc0_Ck_n),
    .poMEM_Ddr4_Mc0_Ck_p              (poSHL_Ddr4_Mem_Mc0_Ck_p),
    .poMEM_Ddr4_Mc0_Reset_n           (poSHL_Ddr4_Mem_Mc0_Reset_n),

    //------------------------------------------------------
    //-- ROLE / Mem / Mp0 Interface
    //------------------------------------------------------
    //-- Memory Port #0 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .piROL_Mem_Mp0_Axis_RdCmd_tdata   (piROL_Shl_Mem_Mp0_Axis_RdCmd_tdata),
    .piROL_Mem_Mp0_Axis_RdCmd_tvalid  (piROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_RdCmd_tready  (poSHL_Rol_Mem_Mp0_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piROL_Mem_Mp0_Axis_RdSts_tready  (piROL_Shl_Mem_Mp0_Axis_RdSts_tready),
    .poMEM_Rol_Mp0_Axis_RdSts_tdata   (poSHL_Rol_Mem_Mp0_Axis_RdSts_tdata),
    .poMEM_Rol_Mp0_Axis_RdSts_tvalid  (poSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piROL_Mem_Mp0_Axis_Read_tready   (piROL_Shl_Mem_Mp0_Axis_Read_tready),
    .poMEM_Rol_Mp0_Axis_Read_tdata    (poSHL_Rol_Mem_Mp0_Axis_Read_tdata),
    .poMEM_Rol_Mp0_Axis_Read_tkeep    (poSHL_Rol_Mem_Mp0_Axis_Read_tkeep),
    .poMEM_Rol_Mp0_Axis_Read_tlast    (poSHL_Rol_Mem_Mp0_Axis_Read_tlast),
    .poMEM_Rol_Mp0_Axis_Read_tvalid   (poSHL_Rol_Mem_Mp0_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp0_Axis_WrCmd_tdata   (piROL_Shl_Mem_Mp0_Axis_WrCmd_tdata),
    .piROL_Mem_Mp0_Axis_WrCmd_tvalid  (piROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_WrCmd_tready  (poSHL_Rol_Mem_Mp0_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piROL_Mem_Mp0_Axis_WrSts_tready  (piROL_Shl_Mem_Mp0_Axis_WrSts_tready),
    .poMEM_Rol_Mp0_Axis_WrSts_tdata   (poSHL_Rol_Mem_Mp0_Axis_WrSts_tdata),
    .poMEM_Rol_Mp0_Axis_WrSts_tvalid  (poSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp0_Axis_Write_tdata   (piROL_Shl_Mem_Mp0_Axis_Write_tdata),
    .piROL_Mem_Mp0_Axis_Write_tkeep   (piROL_Shl_Mem_Mp0_Axis_Write_tkeep),
    .piROL_Mem_Mp0_Axis_Write_tlast   (piROL_Shl_Mem_Mp0_Axis_Write_tlast),
    .piROL_Mem_Mp0_Axis_Write_tvalid  (piROL_Shl_Mem_Mp0_Axis_Write_tvalid),
    .poMEM_Rol_Mp0_Axis_Write_tready  (poSHL_Rol_Mem_Mp0_Axis_Write_tready),
    
    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    //-- Memory Port #1 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .piROL_Mem_Mp1_Axis_RdCmd_tdata   (piROL_Shl_Mem_Mp1_Axis_RdCmd_tdata),
    .piROL_Mem_Mp1_Axis_RdCmd_tvalid  (piROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_RdCmd_tready  (poSHL_Rol_Mem_Mp1_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piROL_Mem_Mp1_Axis_RdSts_tready  (piROL_Shl_Mem_Mp1_Axis_RdSts_tready),
    .poMEM_Rol_Mp1_Axis_RdSts_tdata   (poSHL_Rol_Mem_Mp1_Axis_RdSts_tdata),
    .poMEM_Rol_Mp1_Axis_RdSts_tvalid  (poSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piROL_Mem_Mp1_Axis_Read_tready   (piROL_Shl_Mem_Mp1_Axis_Read_tready),
    .poMEM_Rol_Mp1_Axis_Read_tdata    (poSHL_Rol_Mem_Mp1_Axis_Read_tdata),
    .poMEM_Rol_Mp1_Axis_Read_tkeep    (poSHL_Rol_Mem_Mp1_Axis_Read_tkeep),
    .poMEM_Rol_Mp1_Axis_Read_tlast    (poSHL_Rol_Mem_Mp1_Axis_Read_tlast),
    .poMEM_Rol_Mp1_Axis_Read_tvalid   (poSHL_Rol_Mem_Mp1_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp1_Axis_WrCmd_tdata   (piROL_Shl_Mem_Mp1_Axis_WrCmd_tdata),
    .piROL_Mem_Mp1_Axis_WrCmd_tvalid  (piROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_WrCmd_tready  (poSHL_Rol_Mem_Mp1_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piROL_Mem_Mp1_Axis_WrSts_tready  (piROL_Shl_Mem_Mp1_Axis_WrSts_tready),
    .poMEM_Rol_Mp1_Axis_WrSts_tdata   (poSHL_Rol_Mem_Mp1_Axis_WrSts_tdata),
    .poMEM_Rol_Mp1_Axis_WrSts_tvalid  (poSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp1_Axis_Write_tdata   (piROL_Shl_Mem_Mp1_Axis_Write_tdata),
    .piROL_Mem_Mp1_Axis_Write_tkeep   (piROL_Shl_Mem_Mp1_Axis_Write_tkeep),
    .piROL_Mem_Mp1_Axis_Write_tlast   (piROL_Shl_Mem_Mp1_Axis_Write_tlast),
    .piROL_Mem_Mp1_Axis_Write_tvalid  (piROL_Shl_Mem_Mp1_Axis_Write_tvalid),
    .poMEM_Rol_Mp1_Axis_Write_tready  (poSHL_Rol_Mem_Mp1_Axis_Write_tready),
  
    //------------------------------------------------------
    // -- Physical DDR4 Interface #1
    //------------------------------------------------------
    .pioDDR_Mem_Mc1_DmDbi_n           (pioDDR_Shl_Mem_Mc1_DmDbi_n),
    .pioDDR_Mem_Mc1_Dq                (pioDDR_Shl_Mem_Mc1_Dq),
    .pioDDR_Mem_Mc1_Dqs_n             (pioDDR_Shl_Mem_Mc1_Dqs_n),
    .pioDDR_Mem_Mc1_Dqs_p             (pioDDR_Shl_Mem_Mc1_Dqs_p),
    .poMEM_Ddr4_Mc1_Act_n             (poSHL_Ddr4_Mem_Mc1_Act_n),
    .poMEM_Ddr4_Mc1_Adr               (poSHL_Ddr4_Mem_Mc1_Adr),
    .poMEM_Ddr4_Mc1_Ba                (poSHL_Ddr4_Mem_Mc1_Ba),
    .poMEM_Ddr4_Mc1_Bg                (poSHL_Ddr4_Mem_Mc1_Bg),
    .poMEM_Ddr4_Mc1_Cke               (poSHL_Ddr4_Mem_Mc1_Cke),
    .poMEM_Ddr4_Mc1_Odt               (poSHL_Ddr4_Mem_Mc1_Odt),
    .poMEM_Ddr4_Mc1_Cs_n              (poSHL_Ddr4_Mem_Mc1_Cs_n),
    .poMEM_Ddr4_Mc1_Ck_n              (poSHL_Ddr4_Mem_Mc1_Ck_n),
    .poMEM_Ddr4_Mc1_Ck_p              (poSHL_Ddr4_Mem_Mc1_Ck_p),
    .poMEM_Ddr4_Mc1_Reset_n           (poSHL_Ddr4_Mem_Mc1_Reset_n),

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
  assign sETH0_Ready = sETH0_Mmio_CoreReady;
  
  wire sMc0_Ready;
  wire sMc1_Ready;
  assign sMc0_Ready = sMEM_Mmio_Mc0InitCalComplete;
  assign sMc1_Ready = sMEM_Mmio_Mc1InitCalComplete;
  
  always @(posedge sETH0_ShlClk)
    sLed_HeartBeat <= (!sBinCnt[29] && !sBinCnt[28])                                              ||  // Start bit
                      (!sBinCnt[29] &&  sBinCnt[28] && !sBinCnt[27] && sBinCnt[26] & sMc0_Ready)  ||  // Memory channel 0
                      (!sBinCnt[29] &&  sBinCnt[28] &&  sBinCnt[27] && sBinCnt[26] & sMc1_Ready)  ||  // Memory channel 1
                      ( sBinCnt[29] && !sBinCnt[28] && !sBinCnt[27] && sBinCnt[26] & sETH0_Ready);    // Ethernet MAC 0
  
  assign poSHL_Led_HeartBeat_n = ~sLed_HeartBeat; // LED is active low  
  

  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poSHL_156_25Clk = sETH0_ShlClk;
  assign poSHL_156_25Rst = sETH0_ShlRst;

    
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
