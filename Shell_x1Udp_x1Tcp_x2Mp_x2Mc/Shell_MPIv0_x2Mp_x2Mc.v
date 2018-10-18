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

//module Shell_MPIv0_x2Mp_x2Mc # (
module Shell_x1Udp_x1Tcp_x2Mp_x2Mc # (
  
  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user",  // "user" or "flash"
  parameter gTopDateYear         =  8'hFF,  // uint8
  parameter gTopDateMonth        =  8'hFF,  // uint8
  parameter gTopDateDay          =  8'hFF,  // uint8
  parameter gMmioAddrWidth       =      8,  // Default is 8-bits
  parameter gMmioDataWidth       =      8   // Default is 8-bits

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
  //-- ROLE / MPI Interface
  //------------------------------------------------------
  //output [ 7:0] poMPE_ROLE_MPIif_out_mpi_call_TDATA,
  //output        poMPE_ROLE_MPIif_out_mpi_call_TVALID,
  //input         piMPE_ROLE_MPIif_out_mpi_call_TREADY,
  input  [ 7:0] piROLE_MPE_MPIif_in_mpi_call_TDATA,
  input         piROLE_MPE_MPIif_in_mpi_call_TVALID,
  output        poROLE_MPE_MPIif_in_mpi_call_TREADY,
  input  [31:0] piROLE_MPE_MPIif_in_count_TDATA,
  input         piROLE_MPE_MPIif_in_count_TVALID,
  output        poROLE_MPE_MPIif_in_count_TREADY,
  //output [31:0] poMPE_ROLE_MPIif_out_count_TDATA,
  //output        poMPE_ROLE_MPIif_out_count_TVALID,
  //input         piMPE_ROLE_MPIif_out_count_TREADY,
  input  [31:0] piROLE_MPE_MPIif_in_rank_TDATA,
  input         piROLE_MPE_MPIif_in_rank_TVALID,
  output        poROLE_MPE_MPIif_in_rank_TREADY,
  //output [31:0] poMPE_ROLE_MPIif_out_rank_TDATA,
  //output        poMPE_ROLE_MPIif_out_rank_TVALID,
  //input         piMPE_ROLE_MPIif_out_rank_TREADY,
  input  [ 7:0] piROLE_MPE_MPI_data_TDATA,
  input         piROLE_MPE_MPI_data_TVALID,
  output        poROLE_MPE_MPI_data_TREADY,
  input         piROLE_MPE_MPI_data_TKEEP,
  input         piROLE_MPE_MPI_data_TLAST,
  output [ 7:0] poMPE_ROLE_MPI_data_TDATA,
  output        poMPE_ROLE_MPI_data_TVALID,
  input         piMPE_ROLE_MPI_data_TREADY,
  output        poMPE_ROLE_MPI_data_TKEEP,
  output        poMPE_ROLE_MPI_data_TLAST,

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
  output          poSHL_Rol_Mem_Mp1_Axis_Write_tready,
  
  //------------------------------------------------------
  //-- SHELL / Role / Mmio / Flash Debug Interface
  //------------------------------------------------------
  //-- MMIO / CTRL_2 Register ----------------
  output  [ 1:0]  poSHL_Rol_Mmio_UdpEchoCtrl,
  output          poSHL_Rol_Mmio_UdpPostPktEn,
  output          poSHL_Rol_Mmio_UdpCaptPktEn,
  output  [ 1:0]  poSHL_Rol_Mmio_TcpEchoCtrl,
  output          poSHL_Rol_Mmio_TcpPostPktEn,
  output          poSHL_Rol_Mmio_TcpCaptPktEn,

  //----------------------------------------------------
  // ROLE / Shl/ EMIF Registers 
  //----------------------------------------------------
  input   [15:0]  piROL_SHL_EMIF_2B_Reg,
  output  [15:0]  poSHL_ROL_EMIF_2B_Reg,

  //----------------------------------------------------
  // ROLE <--> SMC 
  //----------------------------------------------------
  output [31:0]  poSMC_ROLE_rank,
  output [31:0]  poSMC_ROLE_size
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
  wire  [31:0]  sMMIO_Nts0_SubNetMask;
  wire  [31:0]  sMMIO_Nts0_GatewayAddr;   
  //---- Pcie Registers Interface ----------------
  //---- Diagnostic Registers Interface ----------
  wire          sMMIO_Eth0_PcsLoopbackEn;
  wire          sMMIO_Eth0_MacLoopbackEn;
  wire          sMMIO_Eth0_MacAddrSwapEn;
  
  //------------------------------------------------------
  //-- SIGNAL DECLARATIONS: Decoupling
  //------------------------------------------------------
  wire            sDECOUP_MPE_ROLE_MPIif_out_mpi_call_TREADY;
  wire  [ 7:0]    sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TDATA;
  wire            sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TVALID;
  wire  [31:0]    sDECOUP_ROLE_MPE_MPIif_in_count_TDATA;
  wire            sDECOUP_ROLE_MPE_MPIif_in_count_TVALID;
  wire            sDECOUP_MPE_ROLE_MPIif_out_count_TREADY;
  wire  [31:0]    sDECOUP_ROLE_MPE_MPIif_in_rank_TDATA;
  wire            sDECOUP_ROLE_MPE_MPIif_in_rank_TVALID;
  wire            sDECOUP_MPE_ROLE_MPIif_out_rank_TREADY;
  wire  [ 7:0]    sDECOUP_ROLE_MPE_MPI_data_TDATA;
  wire            sDECOUP_ROLE_MPE_MPI_data_TVALID;
  wire            sDECOUP_ROLE_MPE_MPI_data_TKEEP;
  wire            sDECOUP_ROLE_MPE_MPI_data_TLAST;
  wire            sDECOUP_MPE_ROLE_MPI_data_TREADY;
  wire    [15:0]  sDECOUP_SHL_EMIF_2B_Reg;
  wire   [ 71:0]  sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tdata;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tvalid;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_RdSts_tready;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_Read_tready;
  wire   [ 71:0]  sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tdata;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tvalid;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_WrSts_tready;
  wire   [511:0]  sDECOUP_Shl_Mem_Mp0_Axis_Write_tdata;
  wire   [ 63:0]  sDECOUP_Shl_Mem_Mp0_Axis_Write_tkeep;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_Write_tlast;
  wire            sDECOUP_Shl_Mem_Mp0_Axis_Write_tvalid;
  wire   [ 71:0]  sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tdata;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tvalid;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_RdSts_tready;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_Read_tready;
  wire   [ 71:0]  sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tdata;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tvalid;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_WrSts_tready;
  wire   [511:0]  sDECOUP_Shl_Mem_Mp1_Axis_Write_tdata;
  wire   [ 63:0]  sDECOUP_Shl_Mem_Mp1_Axis_Write_tkeep;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_Write_tlast;
  wire            sDECOUP_Shl_Mem_Mp1_Axis_Write_tvalid;

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : HWICAPC 
  //--------------------------------------------------------
  //wire        sCASTOR_HWICAPC_eos_in;
  //wire        sCASTOR_HWICAPC_axi_aclk;
  //wire        sCASTOR_HWICAPC_axi_aresetn;
  wire [ 8:0] sCASTOR_HWICAPC_axi_awaddr;
  wire        sCASTOR_HWICAPC_axi_awvalid;
  wire        sCASTOR_HWICAPC_axi_awready;
  wire [31:0] sCASTOR_HWICAPC_axi_wdata;
  wire        sCASTOR_HWICAPC_axi_wstrb;
  wire        sCASTOR_HWICAPC_axi_wvalid;
  wire        sCASTOR_HWICAPC_axi_wready;
  wire [ 1:0] sCASTOR_HWICAPC_axi_bresp;
  wire        sCASTOR_HWICAPC_axi_bvalid;
  wire        sCASTOR_HWICAPC_axi_bready;
  wire [ 8:0] sCASTOR_HWICAPC_axi_araddr;
  wire        sCASTOR_HWICAPC_axi_arvalid; 
  wire        sCASTOR_HWICAPC_axi_arready;
  wire [31:0] sCASTOR_HWICAPC_axi_rdata;
  wire [ 1:0] sCASTOR_HWICAPC_axi_rresp;
  wire        sCASTOR_HWICAPC_axi_rvalid;
  wire        sCASTOR_HWICAPC_axi_rready;
  wire        sCASTOR_HWICAPC_ip2intc_irpt;
  
  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : CASTOR
  //--------------------------------------------------------
  wire [31:0] sCASTOR_MMIO_4B_Reg;
  wire [31:0] sMMIO_CASTOR_4B_Reg;
  wire        sDECOUP_CASTOR_status;
  wire        sCASTOR_DECOUP_activate;
  wire [8:0]  sCASTOR_MMIO_XMEM_Addr;
  wire [31:0] sCASTOR_MMIO_XMEM_RData;
  wire        sCASTOR_MMIO_XMEM_cen; //Chip-enable
  wire        sCASTOR_MMIO_XMEM_wren; //Write-enable
  wire [31:0] sCASTOR_MMIO_XMEM_WData;
  wire [31:0] sCASTOR_ROLE_rank; 
  wire [31:0] sCASTOR_ROLE_size; 

  //--------------------------------------------------------
  //-- SIGNAL DECLARATIONS : MPE
  //--------------------------------------------------------
  wire [63:0] sNTS_MPE_Tcp_TDATA;
  wire        sNTS_MPE_Tcp_TVALID;
  wire        sNTS_MPE_Tcp_TREADY;
  wire [ 7:0] sNTS_MPE_Tcp_TKEEP;
  wire        sNTS_MPE_Tcp_TLAST;
  wire [31:0] sNTS_MPE_IP_ipAddress_TDATA;
  wire        sNTS_MPE_IP_ipAddress_TVALID;
  wire        sNTS_MPE_IP_ipAddress_TREADY;
  wire [63:0] sMPE_NTS_Tcp_TDATA;
  wire        sMPE_NTS_Tcp_TVALID;
  wire        sMPE_NTS_Tcp_TREADY;
  wire [ 7:0] sMPE_NTS_Tcp_TKEEP;
  wire        sMPE_NTS_Tcp_TLAST;
  wire [31:0] sMPE_NTS_IP_ipAddress_TDATA;
  wire        sMPE_NTS_IP_ipAddress_TVALID;
  wire        sMPE_NTS_IP_ipAddress_TREADY;
  wire        sSMC_MPE_ctrlLink_AXI_AWVALID;
  wire        sSMC_MPE_ctrlLink_AXI_AWREADY;
  wire [13:0] sSMC_MPE_ctrlLink_AXI_AWADDR;
  wire        sSMC_MPE_ctrlLink_AXI_WVALID;
  wire        sSMC_MPE_ctrlLink_AXI_WREADY;
  wire [31:0] sSMC_MPE_ctrlLink_AXI_WDATA;
  wire [ 3:0] sSMC_MPE_ctrlLink_AXI_WSTRB;
  wire        sSMC_MPE_ctrlLink_AXI_ARVALID;
  wire        sSMC_MPE_ctrlLink_AXI_ARREADY;
  wire [13:0] sSMC_MPE_ctrlLink_AXI_ARADDR;
  wire        sSMC_MPE_ctrlLink_AXI_RVALID;
  wire        sSMC_MPE_ctrlLink_AXI_RREADY;
  wire [31:0] sSMC_MPE_ctrlLink_AXI_RDATA;
  wire [ 1:0] sSMC_MPE_ctrlLink_AXI_RRESP;
  wire        sSMC_MPE_ctrlLink_AXI_BVALID;
  wire        sSMC_MPE_ctrlLink_AXI_BREADY;
  wire [ 1:0] sSMC_MPE_ctrlLink_AXI_BRESP;
  
  wire [31:0] sMPE_Nts0_IPmeta_tdata;
  wire        sMPE_Nts0_IPmeta_tvalid;
  wire        sMPE_Nts0_IPmeta_tready;
  wire [31:0] sNts0_MPE_IPmeta_tdata;
  wire        sNts0_MPE_IPmeta_tvalid;
  wire        sNts0_MPE_IPmeta_tready;
 
  wire [ 7:0] sMPIif_V_mpi_call_V_TDATA ;
  wire        sMPIif_V_mpi_call_V_TVALID;
  wire        sMPIif_V_mpi_call_V_TREADY;
  wire [31:0] sMPIif_V_count_V_TDATA    ;
  wire        sMPIif_V_count_V_TVALID   ;
  wire        sMPIif_V_count_V_TREADY   ;
  wire [31:0] sMPIif_V_rank_V_TDATA     ;
  wire        sMPIif_V_rank_V_TVALID    ;
  wire        sMPIif_V_rank_V_TREADY    ;
  wire [ 7:0] sROLE_MPE_MPI_data_TDATA ;
  wire        sROLE_MPE_MPI_data_TVALID;
  wire        sROLE_MPE_MPI_data_TREADY;
  wire        sROLE_MPE_MPI_data_TKEEP ;
  wire        sROLE_MPE_MPI_data_TLAST ;
  wire [ 7:0] sMPE_ROLE_MPI_data_TDATA ;
  wire        sMPE_ROLE_MPI_data_TVALID;
  wire        sMPE_ROLE_MPI_data_TREADY;
  wire        sMPE_ROLE_MPI_data_TKEEP ;
  wire        sMPE_ROLE_MPI_data_TLAST ;


  //-- END OF SIGNAL DECLARATIONS ----------------------------------------------

    
  //============================================================================
  //  INST: MMIIO CLIENT
  //============================================================================
  MmioClient_A8_D8 #(

    .gTopDateYear         (gTopDateYear),
    .gTopDateMonth        (gTopDateMonth),
    .gTopDateDay          (gTopDateDay),
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
    .poMMIO_Nts0_SubNetMask         (sMMIO_Nts0_SubNetMask),
    .poMMIO_Nts0_GatewayAddr        (sMMIO_Nts0_GatewayAddr),
    
    //-- ROLE : Status inputs and Control Outputs --
    .poMMIO_Role_UdpEchoCtrl        (poMMIO_Role_UdpEchoCtrl),
    .poMMIO_Role_UdpPostPktEn       (poMMIO_Role_UdpPostPktEn),
    .poMMIO_Role_UdpCaptPktEn       (poMMIO_Role_UdpCaptPktEn),
    .poMMIO_Role_TcpEchoCtrl        (poMMIO_Role_TcpEchoCtrl),
    .poMMIO_Role_TcpPostPktEn       (poMMIO_Role_TcpPostPktEn),
    .poMMIO_Role_TcpCaptPktEn       (poMMIO_Role_TcpCaptPktEn),
    
    // ROLE EMIF Registers 
    .poMMIO_ROLE_2B_Reg             (poSHL_ROL_EMIF_2B_Reg),
    .piMMIO_ROLE_2B_Reg             (sDECOUP_SHL_EMIF_2B_Reg),
    // SMC Registers
    .piMMIO_SMC_4B_Reg              (sCASTOR_MMIO_4B_Reg),
    .poMMIO_SMC_4B_Reg              (sMMIO_CASTOR_4B_Reg),
    //XMem Port B
    .piSMC_MMIO_XMEM_en             (sCASTOR_MMIO_XMEM_cen),
    .piSMC_MMIO_XMEM_Wren           (sCASTOR_MMIO_XMEM_wren),
    .piSMC_MMIO_XMEM_WrData         (sCASTOR_MMIO_XMEM_WData),
    .poSMC_MMIO_XMEM_RData          (sCASTOR_MMIO_XMEM_RData),
    .piSMC_MMIO_XMEM_Addr           (sCASTOR_MMIO_XMEM_Addr),
 
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
    .piROL_Nts0_Udp_Axis_tdata        (sMPE_NTS_Tcp_TDATA),
    .piROL_Nts0_Udp_Axis_tkeep        (sMPE_NTS_Tcp_TKEEP),
    .piROL_Nts0_Udp_Axis_tlast        (sMPE_NTS_Tcp_TLAST),
    .piROL_Nts0_Udp_Axis_tvalid       (sMPE_NTS_Tcp_TVALID),
    .poNTS0_Rol_Udp_Axis_tready       (sMPE_NTS_Tcp_TREADY),
    //-- Output AXI-Write Stream Interface ---------
    .poNTS0_Rol_Udp_Axis_tdata        (sNTS_MPE_Tcp_TDATA),
    .poNTS0_Rol_Udp_Axis_tkeep        (sNTS_MPE_Tcp_TKEEP),
    .poNTS0_Rol_Udp_Axis_tlast        (sNTS_MPE_Tcp_TLAST),
    .poNTS0_Rol_Udp_Axis_tvalid       (sNTS_MPE_Tcp_TVALID),
    .piROL_Nts0_Udp_Axis_tready       (sNTS_MPE_Tcp_TREADY),
  
    //------------------------------------------------------
    //-- ROLE / Nts0 / IP Meta
    //------------------------------------------------------
    .piMPE_Nts0_IPmeta_tdata         (sMPE_Nts0_IPmeta_tdata),
    .piMPE_Nts0_IPmeta_tvalid        (sMPE_Nts0_IPmeta_tvalid),
    .poMPE_Nts0_IPmeta_tready        (sMPE_Nts0_IPmeta_tready),
    .poNts0_MPE_IPmeta_tdata         (sNts0_MPE_IPmeta_tdata),
    .poNts0_MPE_IPmeta_tvalid        (sNts0_MPE_IPmeta_tvalid),
    .piNts0_MPE_IPmeta_tready        (sNts0_MPE_IPmeta_tready),

    //------------------------------------------------------
    //-- ROLE / Nts0 / TCP Interfaces
    //------------------------------------------------------
   // //-- Input AXI-Write Stream Interface ----------
   // .piROL_Nts0_Tcp_Axis_tdata        (sDECOUP_Shl_Nts0_Tcp_Axis_tdata),
   // .piROL_Nts0_Tcp_Axis_tkeep        (sDECOUP_Shl_Nts0_Tcp_Axis_tkeep),
   // .piROL_Nts0_Tcp_Axis_tlast        (sDECOUP_Shl_Nts0_Tcp_Axis_tlast),
   // .piROL_Nts0_Tcp_Axis_tvalid       (sDECOUP_Shl_Nts0_Tcp_Axis_tvalid),
   // .poNTS0_Rol_Tcp_Axis_tready       (poSHL_Rol_Nts0_Tcp_Axis_tready),
   // //-- Output AXI-Write Stream Interface ---------
   // .piROL_Nts0_Tcp_Axis_tready       (sDECOUP_Shl_Nts0_Tcp_Axis_tready),
   // .poNTS0_Rol_Tcp_Axis_tdata        (poSHL_Rol_Nts0_Tcp_Axis_tdata),
   // .poNTS0_Rol_Tcp_Axis_tkeep        (poSHL_Rol_Nts0_Tcp_Axis_tkeep),
   // .poNTS0_Rol_Tcp_Axis_tlast        (poSHL_Rol_Nts0_Tcp_Axis_tlast),
   // .poNTS0_Rol_Tcp_Axis_tvalid       (poSHL_Rol_Nts0_Tcp_Axis_tvalid),
   
    //------------------------------------------------------
    //-- MMIO / Nts0 / Interfaces
    //------------------------------------------------------
    .piMMIO_Nts0_MacAddress           (sMMIO_Nts0_MacAddress),
    .piMMIO_Nts0_IpAddress            (sMMIO_Nts0_IpAddress),
    .piMMIO_Nts0_SubNetMask           (sMMIO_Nts0_SubNetMask),
    .piMMIO_Nts0_GatewayAddr          (sMMIO_Nts0_GatewayAddr),   
    
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
    .piROL_Mem_Mp0_Axis_RdCmd_tdata   (sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tdata),
    .piROL_Mem_Mp0_Axis_RdCmd_tvalid  (sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_RdCmd_tready  (poSHL_Rol_Mem_Mp0_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piROL_Mem_Mp0_Axis_RdSts_tready  (sDECOUP_Shl_Mem_Mp0_Axis_RdSts_tready),
    .poMEM_Rol_Mp0_Axis_RdSts_tdata   (poSHL_Rol_Mem_Mp0_Axis_RdSts_tdata),
    .poMEM_Rol_Mp0_Axis_RdSts_tvalid  (poSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piROL_Mem_Mp0_Axis_Read_tready   (sDECOUP_Shl_Mem_Mp0_Axis_Read_tready),
    .poMEM_Rol_Mp0_Axis_Read_tdata    (poSHL_Rol_Mem_Mp0_Axis_Read_tdata),
    .poMEM_Rol_Mp0_Axis_Read_tkeep    (poSHL_Rol_Mem_Mp0_Axis_Read_tkeep),
    .poMEM_Rol_Mp0_Axis_Read_tlast    (poSHL_Rol_Mem_Mp0_Axis_Read_tlast),
    .poMEM_Rol_Mp0_Axis_Read_tvalid   (poSHL_Rol_Mem_Mp0_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp0_Axis_WrCmd_tdata   (sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tdata),
    .piROL_Mem_Mp0_Axis_WrCmd_tvalid  (sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tvalid),
    .poMEM_Rol_Mp0_Axis_WrCmd_tready  (poSHL_Rol_Mem_Mp0_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piROL_Mem_Mp0_Axis_WrSts_tready  (sDECOUP_Shl_Mem_Mp0_Axis_WrSts_tready),
    .poMEM_Rol_Mp0_Axis_WrSts_tdata   (poSHL_Rol_Mem_Mp0_Axis_WrSts_tdata),
    .poMEM_Rol_Mp0_Axis_WrSts_tvalid  (poSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp0_Axis_Write_tdata   (sDECOUP_Shl_Mem_Mp0_Axis_Write_tdata),
    .piROL_Mem_Mp0_Axis_Write_tkeep   (sDECOUP_Shl_Mem_Mp0_Axis_Write_tkeep),
    .piROL_Mem_Mp0_Axis_Write_tlast   (sDECOUP_Shl_Mem_Mp0_Axis_Write_tlast),
    .piROL_Mem_Mp0_Axis_Write_tvalid  (sDECOUP_Shl_Mem_Mp0_Axis_Write_tvalid),
    .poMEM_Rol_Mp0_Axis_Write_tready  (poSHL_Rol_Mem_Mp0_Axis_Write_tready),
    
    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    //-- Memory Port #1 / S2MM-AXIS ------------------   
    //---- Stream Read Command ---------------
    .piROL_Mem_Mp1_Axis_RdCmd_tdata   (sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tdata),
    .piROL_Mem_Mp1_Axis_RdCmd_tvalid  (sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_RdCmd_tready  (poSHL_Rol_Mem_Mp1_Axis_RdCmd_tready),
    //---- Stream Read Status ----------------
    .piROL_Mem_Mp1_Axis_RdSts_tready  (sDECOUP_Shl_Mem_Mp1_Axis_RdSts_tready),
    .poMEM_Rol_Mp1_Axis_RdSts_tdata   (poSHL_Rol_Mem_Mp1_Axis_RdSts_tdata),
    .poMEM_Rol_Mp1_Axis_RdSts_tvalid  (poSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel --------
    .piROL_Mem_Mp1_Axis_Read_tready   (sDECOUP_Shl_Mem_Mp1_Axis_Read_tready),
    .poMEM_Rol_Mp1_Axis_Read_tdata    (poSHL_Rol_Mem_Mp1_Axis_Read_tdata),
    .poMEM_Rol_Mp1_Axis_Read_tkeep    (poSHL_Rol_Mem_Mp1_Axis_Read_tkeep),
    .poMEM_Rol_Mp1_Axis_Read_tlast    (poSHL_Rol_Mem_Mp1_Axis_Read_tlast),
    .poMEM_Rol_Mp1_Axis_Read_tvalid   (poSHL_Rol_Mem_Mp1_Axis_Read_tvalid),
    //---- Stream Write Command --------------
    .piROL_Mem_Mp1_Axis_WrCmd_tdata   (sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tdata),
    .piROL_Mem_Mp1_Axis_WrCmd_tvalid  (sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tvalid),
    .poMEM_Rol_Mp1_Axis_WrCmd_tready  (poSHL_Rol_Mem_Mp1_Axis_WrCmd_tready),
    //---- Stream Write Status ---------------
    .piROL_Mem_Mp1_Axis_WrSts_tready  (sDECOUP_Shl_Mem_Mp1_Axis_WrSts_tready),
    .poMEM_Rol_Mp1_Axis_WrSts_tdata   (poSHL_Rol_Mem_Mp1_Axis_WrSts_tdata),
    .poMEM_Rol_Mp1_Axis_WrSts_tvalid  (poSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel ---------
    .piROL_Mem_Mp1_Axis_Write_tdata   (sDECOUP_Shl_Mem_Mp1_Axis_Write_tdata),
    .piROL_Mem_Mp1_Axis_Write_tkeep   (sDECOUP_Shl_Mem_Mp1_Axis_Write_tkeep),
    .piROL_Mem_Mp1_Axis_Write_tlast   (sDECOUP_Shl_Mem_Mp1_Axis_Write_tlast),
    .piROL_Mem_Mp1_Axis_Write_tvalid  (sDECOUP_Shl_Mem_Mp1_Axis_Write_tvalid),
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
  // SMC related modules 
  //===========================================================================

  HWICAPC HWICAP (
    .icap_clk       (sETH0_ShlClk),
    .eos_in         (1),
    //.s_axi_aclk     (sCASTOR_HWICAPC_axi_aclk),
    .s_axi_aclk     (sETH0_ShlClk),
    //.s_axi_aresetn  (sCASTOR_HWICAPC_axi_aresetn),
    .s_axi_aresetn  (~ piTOP_156_25Rst),
    .s_axi_awaddr   (sCASTOR_HWICAPC_axi_awaddr),
    .s_axi_awvalid  (sCASTOR_HWICAPC_axi_awvalid),
    .s_axi_awready  (sCASTOR_HWICAPC_axi_awready),
    .s_axi_wdata    (sCASTOR_HWICAPC_axi_wdata),
    .s_axi_wstrb    (sCASTOR_HWICAPC_axi_wstrb),
    .s_axi_wvalid   (sCASTOR_HWICAPC_axi_wvalid),
    .s_axi_wready   (sCASTOR_HWICAPC_axi_wready),
    .s_axi_bresp    (sCASTOR_HWICAPC_axi_bresp),
    .s_axi_bvalid   (sCASTOR_HWICAPC_axi_bvalid),
    .s_axi_bready   (sCASTOR_HWICAPC_axi_bready),
    .s_axi_araddr   (sCASTOR_HWICAPC_axi_araddr),
    .s_axi_arvalid  (sCASTOR_HWICAPC_axi_arvalid), 
    .s_axi_arready  (sCASTOR_HWICAPC_axi_arready),
    .s_axi_rdata    (sCASTOR_HWICAPC_axi_rdata),
    .s_axi_rresp    (sCASTOR_HWICAPC_axi_rresp),
    .s_axi_rvalid   (sCASTOR_HWICAPC_axi_rvalid),
    .s_axi_rready   (sCASTOR_HWICAPC_axi_rready),
    .ip2intc_irpt   (sCASTOR_HWICAPC_ip2intc_irpt)
  );


  SMC CASTOR (
    //-- Global Clock used by the entire SHELL -------------
    .ap_clk                 (sETH0_ShlClk),
    //-- Global Reset used by the entire SHELL -------------
    .ap_rst_n               (~ piTOP_156_25Rst),
    //core should start immediately 
    .ap_start               (1),
    .piSysReset_V           (piTOP_156_25Rst),
    .piSysReset_V_ap_vld   (1),
    .poMMIO_V              (sCASTOR_MMIO_4B_Reg),
    //.poMMIO_V_ap_vld     ( ),
    .piMMIO_V              (sMMIO_CASTOR_4B_Reg),
    .piMMIO_V_ap_vld        (1),
    .m_axi_poSMC_to_HWICAP_AXIM_AWADDR   (sCASTOR_HWICAPC_axi_awaddr),
    .m_axi_poSMC_to_HWICAP_AXIM_AWVALID  (sCASTOR_HWICAPC_axi_awvalid),
    .m_axi_poSMC_to_HWICAP_AXIM_AWREADY  (sCASTOR_HWICAPC_axi_awready),
    .m_axi_poSMC_to_HWICAP_AXIM_WDATA    (sCASTOR_HWICAPC_axi_wdata),
    .m_axi_poSMC_to_HWICAP_AXIM_WSTRB    (sCASTOR_HWICAPC_axi_wstrb),
    .m_axi_poSMC_to_HWICAP_AXIM_WVALID   (sCASTOR_HWICAPC_axi_wvalid),
    .m_axi_poSMC_to_HWICAP_AXIM_WREADY   (sCASTOR_HWICAPC_axi_wready),
    .m_axi_poSMC_to_HWICAP_AXIM_BRESP    (sCASTOR_HWICAPC_axi_bresp),
    .m_axi_poSMC_to_HWICAP_AXIM_BVALID   (sCASTOR_HWICAPC_axi_bvalid),
    .m_axi_poSMC_to_HWICAP_AXIM_BREADY   (sCASTOR_HWICAPC_axi_bready),
    .m_axi_poSMC_to_HWICAP_AXIM_ARADDR   (sCASTOR_HWICAPC_axi_araddr),
    .m_axi_poSMC_to_HWICAP_AXIM_ARVALID  (sCASTOR_HWICAPC_axi_arvalid), 
    .m_axi_poSMC_to_HWICAP_AXIM_ARREADY  (sCASTOR_HWICAPC_axi_arready),
    .m_axi_poSMC_to_HWICAP_AXIM_RDATA    (sCASTOR_HWICAPC_axi_rdata),
    .m_axi_poSMC_to_HWICAP_AXIM_RRESP    (sCASTOR_HWICAPC_axi_rresp),
    .m_axi_poSMC_to_HWICAP_AXIM_RVALID   (sCASTOR_HWICAPC_axi_rvalid),
    .m_axi_poSMC_to_HWICAP_AXIM_RREADY   (sCASTOR_HWICAPC_axi_rready),
    .piDECOUP_SMC_status_V               (sDECOUP_CASTOR_status),
    .poSMC_DECOUP_activate_V             (sCASTOR_DECOUP_activate),
    .xmem_V_Address0                     (sCASTOR_MMIO_XMEM_Addr),
    .xmem_V_ce0                          (sCASTOR_MMIO_XMEM_cen), 
    .xmem_V_we0                          (sCASTOR_MMIO_XMEM_wren),
    .xmem_V_d0                           (sCASTOR_MMIO_XMEM_WData),
    .xmem_V_q0                           (sCASTOR_MMIO_XMEM_RData),
    .m_axi_poSMC_MPE_ctrlLink_AXI_AWVALID        (sSMC_MPE_ctrlLink_AXI_AWVALID),
    .m_axi_poSMC_MPE_ctrlLink_AXI_AWREADY        (sSMC_MPE_ctrlLink_AXI_AWREADY),
    .m_axi_poSMC_MPE_ctrlLink_AXI_AWADDR        (sSMC_MPE_ctrlLink_AXI_AWADDR),
    .m_axi_poSMC_MPE_ctrlLink_AXI_WVALID        (sSMC_MPE_ctrlLink_AXI_WVALID),
    .m_axi_poSMC_MPE_ctrlLink_AXI_WREADY        (sSMC_MPE_ctrlLink_AXI_WREADY),
    .m_axi_poSMC_MPE_ctrlLink_AXI_WDATA        (sSMC_MPE_ctrlLink_AXI_WDATA),
    .m_axi_poSMC_MPE_ctrlLink_AXI_WSTRB        (sSMC_MPE_ctrlLink_AXI_WSTRB),
    .m_axi_poSMC_MPE_ctrlLink_AXI_ARVALID        (sSMC_MPE_ctrlLink_AXI_ARVALID),
    .m_axi_poSMC_MPE_ctrlLink_AXI_ARREADY        (sSMC_MPE_ctrlLink_AXI_ARREADY),
    .m_axi_poSMC_MPE_ctrlLink_AXI_ARADDR        (sSMC_MPE_ctrlLink_AXI_ARADDR),
    .m_axi_poSMC_MPE_ctrlLink_AXI_RVALID        (sSMC_MPE_ctrlLink_AXI_RVALID),
    .m_axi_poSMC_MPE_ctrlLink_AXI_RREADY        (sSMC_MPE_ctrlLink_AXI_RREADY),
    .m_axi_poSMC_MPE_ctrlLink_AXI_RDATA        (sSMC_MPE_ctrlLink_AXI_RDATA),
    .m_axi_poSMC_MPE_ctrlLink_AXI_RRESP        (sSMC_MPE_ctrlLink_AXI_RRESP),
    .m_axi_poSMC_MPE_ctrlLink_AXI_BVALID        (sSMC_MPE_ctrlLink_AXI_BVALID),
    .m_axi_poSMC_MPE_ctrlLink_AXI_BREADY        (sSMC_MPE_ctrlLink_AXI_BREADY),
    .m_axi_poSMC_MPE_ctrlLink_AXI_BRESP        (sSMC_MPE_ctrlLink_AXI_BRESP),
    .poSMC_to_ROLE_rank_V                (sCASTOR_ROLE_rank),
    .poSMC_to_ROLE_size_V                (sCASTOR_ROLE_size)
  );

  assign poSMC_ROLE_rank = sCASTOR_ROLE_rank;
  assign poSMC_ROLE_size = sCASTOR_ROLE_size;


  Decoupler DECOUP (
    .rp_ROLE_MPE_ROLE_MPIif_out_mpi_call_TREADY   (piMPE_ROLE_MPIif_out_mpi_call_TREADY),
    .s_ROLE_MPE_ROLE_MPIif_out_mpi_call_TREADY    (sDECOUP_MPE_ROLE_MPIif_out_mpi_call_TREADY),
    .rp_ROLE_ROLE_MPE_MPIif_in_mpi_call_TDATA     (piROLE_MPE_MPIif_in_mpi_call_TDATA),
    .s_ROLE_ROLE_MPE_MPIif_in_mpi_call_TDATA      (sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TDATA),
    .rp_ROLE_ROLE_MPE_MPIif_in_mpi_call_TVALID    (piROLE_MPE_MPIif_in_mpi_call_TVALID),
    .s_ROLE_ROLE_MPE_MPIif_in_mpi_call_TVALID     (sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TVALID),
    .rp_ROLE_ROLE_MPE_MPIif_in_count_TDATA        (piROLE_MPE_MPIif_in_count_TDATA),
    .s_ROLE_ROLE_MPE_MPIif_in_count_TDATA         (sDECOUP_ROLE_MPE_MPIif_in_count_TDATA),
    .rp_ROLE_ROLE_MPE_MPIif_in_count_TVALID       (piROLE_MPE_MPIif_in_count_TVALID),
    .s_ROLE_ROLE_MPE_MPIif_in_count_TVALID        (sDECOUP_ROLE_MPE_MPIif_in_count_TVALID),
    .rp_ROLE_MPE_ROLE_MPIif_out_count_TREADY      (piMPE_ROLE_MPIif_out_count_TREADY),
    .s_ROLE_MPE_ROLE_MPIif_out_count_TREADY       (sDECOUP_MPE_ROLE_MPIif_out_count_TREADY),
    .rp_ROLE_ROLE_MPE_MPIif_in_rank_TDATA         (piROLE_MPE_MPIif_in_rank_TDATA),
    .s_ROLE_ROLE_MPE_MPIif_in_rank_TDATA          (sDECOUP_ROLE_MPE_MPIif_in_rank_TDATA),
    .rp_ROLE_ROLE_MPE_MPIif_in_rank_TVALID        (piROLE_MPE_MPIif_in_rank_TVALID),
    .s_ROLE_ROLE_MPE_MPIif_in_rank_TVALID         (sDECOUP_ROLE_MPE_MPIif_in_rank_TVALID),
    .rp_ROLE_MPE_ROLE_MPIif_out_rank_TREADY       (piMPE_ROLE_MPIif_out_rank_TREADY),
    .s_ROLE_MPE_ROLE_MPIif_out_rank_TREADY        (sDECOUP_MPE_ROLE_MPIif_out_rank_TREADY),
    .rp_ROLE_ROLE_MPE_MPI_data_TDATA              (piROLE_MPE_MPI_data_TDATA),
    .s_ROLE_ROLE_MPE_MPI_data_TDATA               (sDECOUP_ROLE_MPE_MPI_data_TDATA),
    .rp_ROLE_ROLE_MPE_MPI_data_TVALID             (piROLE_MPE_MPI_data_TVALID),
    .s_ROLE_ROLE_MPE_MPI_data_TVALID              (sDECOUP_ROLE_MPE_MPI_data_TVALID),
    .rp_ROLE_ROLE_MPE_MPI_data_TKEEP              (piROLE_MPE_MPI_data_TKEEP),
    .s_ROLE_ROLE_MPE_MPI_data_TKEEP               (sDECOUP_ROLE_MPE_MPI_data_TKEEP),
    .rp_ROLE_ROLE_MPE_MPI_data_TLAST              (piROLE_MPE_MPI_data_TLAST),
    .s_ROLE_ROLE_MPE_MPI_data_TLAST               (sDECOUP_ROLE_MPE_MPI_data_TLAST),
    .rp_ROLE_MPE_ROLE_MPI_data_TREADY             (piMPE_ROLE_MPI_data_TREADY),
    .s_ROLE_MPE_ROLE_MPI_data_TREADY              (sDECOUP_MPE_ROLE_MPI_data_TREADY),
    .rp_ROLE_EMIF_2B_Reg     (piROL_SHL_EMIF_2B_Reg),
    .s_ROLE_EMIF_2B_Reg    (sDECOUP_SHL_EMIF_2B_Reg),
    .rp_ROLE_Mem_Up0_Axis_RdCmd_tdata         (piROL_Shl_Mem_Mp0_Axis_RdCmd_tdata),
    .s_ROLE_Mem_Up0_Axis_RdCmd_tdata     (sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tdata),
    .rp_ROLE_Mem_Up0_Axis_RdCmd_tvalid        (piROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid),
    .s_ROLE_Mem_Up0_Axis_RdCmd_tvalid    (sDECOUP_Shl_Mem_Mp0_Axis_RdCmd_tvalid),
    .rp_ROLE_Mem_Up0_Axis_RdSts_tready        (piROL_Shl_Mem_Mp0_Axis_RdSts_tready),
    .s_ROLE_Mem_Up0_Axis_RdSts_tready    (sDECOUP_Shl_Mem_Mp0_Axis_RdSts_tready),
    .rp_ROLE_Mem_Up0_Axis_Read_tready         (piROL_Shl_Mem_Mp0_Axis_Read_tready),
    .s_ROLE_Mem_Up0_Axis_Read_tready     (sDECOUP_Shl_Mem_Mp0_Axis_Read_tready),
    .rp_ROLE_Mem_Up0_Axis_WrCmd_tdata         (piROL_Shl_Mem_Mp0_Axis_WrCmd_tdata),
    .s_ROLE_Mem_Up0_Axis_WrCmd_tdata     (sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tdata),
    .rp_ROLE_Mem_Up0_Axis_WrCmd_tvalid        (piROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid),
    .s_ROLE_Mem_Up0_Axis_WrCmd_tvalid    (sDECOUP_Shl_Mem_Mp0_Axis_WrCmd_tvalid),
    .rp_ROLE_Mem_Up0_Axis_WrSts_tready        (piROL_Shl_Mem_Mp0_Axis_WrSts_tready),
    .s_ROLE_Mem_Up0_Axis_WrSts_tready    (sDECOUP_Shl_Mem_Mp0_Axis_WrSts_tready),
    .rp_ROLE_Mem_Up0_Axis_Write_tdata         (piROL_Shl_Mem_Mp0_Axis_Write_tdata),
    .s_ROLE_Mem_Up0_Axis_Write_tdata     (sDECOUP_Shl_Mem_Mp0_Axis_Write_tdata),
    .rp_ROLE_Mem_Up0_Axis_Write_tkeep         (piROL_Shl_Mem_Mp0_Axis_Write_tkeep),
    .s_ROLE_Mem_Up0_Axis_Write_tkeep     (sDECOUP_Shl_Mem_Mp0_Axis_Write_tkeep),
    .rp_ROLE_Mem_Up0_Axis_Write_tlast         (piROL_Shl_Mem_Mp0_Axis_Write_tlast),
    .s_ROLE_Mem_Up0_Axis_Write_tlast     (sDECOUP_Shl_Mem_Mp0_Axis_Write_tlast),
    .rp_ROLE_Mem_Up0_Axis_Write_tvalid        (piROL_Shl_Mem_Mp0_Axis_Write_tvalid),
    .s_ROLE_Mem_Up0_Axis_Write_tvalid    (sDECOUP_Shl_Mem_Mp0_Axis_Write_tvalid),
    .rp_ROLE_Mem_Up1_Axis_RdCmd_tdata         (piROL_Shl_Mem_Mp1_Axis_RdCmd_tdata),
    .s_ROLE_Mem_Up1_Axis_RdCmd_tdata     (sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tdata),
    .rp_ROLE_Mem_Up1_Axis_RdCmd_tvalid        (piROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid),
    .s_ROLE_Mem_Up1_Axis_RdCmd_tvalid    (sDECOUP_Shl_Mem_Mp1_Axis_RdCmd_tvalid),
    .rp_ROLE_Mem_Up1_Axis_RdSts_tready        (piROL_Shl_Mem_Mp1_Axis_RdSts_tready),
    .s_ROLE_Mem_Up1_Axis_RdSts_tready    (sDECOUP_Shl_Mem_Mp1_Axis_RdSts_tready),
    .rp_ROLE_Mem_Up1_Axis_Read_tready         (piROL_Shl_Mem_Mp1_Axis_Read_tready),
    .s_ROLE_Mem_Up1_Axis_Read_tready     (sDECOUP_Shl_Mem_Mp1_Axis_Read_tready),
    .rp_ROLE_Mem_Up1_Axis_WrCmd_tdata         (piROL_Shl_Mem_Mp1_Axis_WrCmd_tdata),
    .s_ROLE_Mem_Up1_Axis_WrCmd_tdata     (sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tdata),
    .rp_ROLE_Mem_Up1_Axis_WrCmd_tvalid        (piROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid),
    .s_ROLE_Mem_Up1_Axis_WrCmd_tvalid    (sDECOUP_Shl_Mem_Mp1_Axis_WrCmd_tvalid),
    .rp_ROLE_Mem_Up1_Axis_WrSts_tready        (piROL_Shl_Mem_Mp1_Axis_WrSts_tready),
    .s_ROLE_Mem_Up1_Axis_WrSts_tready    (sDECOUP_Shl_Mem_Mp1_Axis_WrSts_tready),
    .rp_ROLE_Mem_Up1_Axis_Write_tdata         (piROL_Shl_Mem_Mp1_Axis_Write_tdata),
    .s_ROLE_Mem_Up1_Axis_Write_tdata     (sDECOUP_Shl_Mem_Mp1_Axis_Write_tdata),
    .rp_ROLE_Mem_Up1_Axis_Write_tkeep         (piROL_Shl_Mem_Mp1_Axis_Write_tkeep),
    .s_ROLE_Mem_Up1_Axis_Write_tkeep     (sDECOUP_Shl_Mem_Mp1_Axis_Write_tkeep),
    .rp_ROLE_Mem_Up1_Axis_Write_tlast         (piROL_Shl_Mem_Mp1_Axis_Write_tlast),
    .s_ROLE_Mem_Up1_Axis_Write_tlast     (sDECOUP_Shl_Mem_Mp1_Axis_Write_tlast),
    .rp_ROLE_Mem_Up1_Axis_Write_tvalid        (piROL_Shl_Mem_Mp1_Axis_Write_tvalid),
    .s_ROLE_Mem_Up1_Axis_Write_tvalid    (sDECOUP_Shl_Mem_Mp1_Axis_Write_tvalid),
    .decouple     (sCASTOR_DECOUP_activate),
    .decouple_status (sDECOUP_CASTOR_status)
  );
  
  //============================================================================
  //  INST: AXI4-STREAM REGISTER SLICE (ROLE <==> MPE)
  //============================================================================
  AxisRegisterSlice_64 ARS0 (
    .aclk           (piTOP_156_25Clk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (sDECOUP_ROLE_MPE_MPI_data_TDATA),
    .s_axis_tvalid  (sDECOUP_ROLE_MPE_MPI_data_TVALID),
    .s_axis_tready  (poROLE_MPE_MPI_data_TREADY    ),
    .s_axis_tkeep   (sDECOUP_ROLE_MPE_MPI_data_TKEEP),
    .s_axis_tlast   (sDECOUP_ROLE_MPE_MPI_data_TLAST),
    //-- To MPE 
    .m_axis_tdata   (sROLE_MPE_MPI_data_TDATA),
    .m_axis_tvalid  (sROLE_MPE_MPI_data_TVALID),
    .m_axis_tready  (sROLE_MPE_MPI_data_TREADY),
    .m_axis_tkeep   (sROLE_MPE_MPI_data_TKEEP),
    .m_axis_tlast   (sROLE_MPE_MPI_data_TLAST)
  );

  AxisRegisterSlice_64 ARS1 (
    .aclk           (piTOP_156_25Clk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From MPE
    .s_axis_tdata   (sMPE_ROLE_MPI_data_TDATA),
    .s_axis_tvalid  (sMPE_ROLE_MPI_data_TVALID),
    .s_axis_tready  (sMPE_ROLE_MPI_data_TREADY),
    .s_axis_tkeep   (sMPE_ROLE_MPI_data_TKEEP),
    .s_axis_tlast   (sMPE_ROLE_MPI_data_TLAST),
    //-- To ROLE
    .m_axis_tdata   (poMPE_ROLE_MPI_data_TDATA),
    .m_axis_tvalid  (poMPE_ROLE_MPI_data_TVALID),
    .m_axis_tready  (sDECOUP_MPE_ROLE_MPI_data_TREADY),
    .m_axis_tkeep   (poMPE_ROLE_MPI_data_TKEEP),
    .m_axis_tlast   (poMPE_ROLE_MPI_data_TLAST)
  );
  
  AxisRegisterSlice_64 ARS2 (
    .aclk           (piTOP_156_25Clk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TDATA),
    .s_axis_tvalid  (sDECOUP_ROLE_MPE_MPIif_in_mpi_call_TVALID),
    .s_axis_tready  (poROLE_MPE_MPIif_in_mpi_call_TREADY),
    //-- To MPE 
    .m_axis_tdata   (sMPIif_V_mpi_call_V_TDATA  ),
    .m_axis_tvalid  (sMPIif_V_mpi_call_V_TVALID ),
    .m_axis_tready  (sMPIif_V_mpi_call_V_TREADY )
  );
  
  AxisRegisterSlice_64 ARS3 (
    .aclk           (piTOP_156_25Clk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (sDECOUP_ROLE_MPE_MPIif_in_count_TDATA),
    .s_axis_tvalid  (sDECOUP_ROLE_MPE_MPIif_in_count_TVALID),
    .s_axis_tready  (poROLE_MPE_MPIif_in_count_TREADY),
    //-- To MPE 
    .m_axis_tdata   (sMPIif_V_count_V_TDATA  ),
    .m_axis_tvalid  (sMPIif_V_count_V_TVALID ),
    .m_axis_tready  (sMPIif_V_count_V_TREADY )
  );
  
  AxisRegisterSlice_64 ARS4 (
    .aclk           (piTOP_156_25Clk),
    .aresetn        (~piTOP_156_25Rst),
    //-- From ROLE 
    .s_axis_tdata   (sDECOUP_ROLE_MPE_MPIif_in_rank_TDATA),
    .s_axis_tvalid  (sDECOUP_ROLE_MPE_MPIif_in_rank_TVALID),
    .s_axis_tready  (poROLE_MPE_MPIif_in_rank_TREADY),
    //-- To MPE 
    .m_axis_tdata   (sMPIif_V_rank_V_TDATA  ),
    .m_axis_tvalid  (sMPIif_V_rank_V_TVALID ),
    .m_axis_tready  (sMPIif_V_rank_V_TREADY )
  );

  
  MPE MPI_LAYER (
    //-- Global Clock used by the entire SHELL -------------
    .ap_clk                 (sETH0_ShlClk),
    //-- Global Reset used by the entire SHELL -------------
    .ap_rst_n               (~ piTOP_156_25Rst),
    .piSysReset_V           (piTOP_156_25Rst),
    .piSysReset_V_ap_vld    (1),
    .siTcp_TDATA        (sNTS_MPE_Tcp_TDATA),
    .siTcp_TVALID        (sNTS_MPE_Tcp_TVALID),
    .siTcp_TREADY        (sNTS_MPE_Tcp_TREADY),
    .siTcp_TKEEP        (sNTS_MPE_Tcp_TKEEP),
    .siTcp_TLAST        (sNTS_MPE_Tcp_TLAST),
    .siIP_V_ipAddress_V_TDATA        (sNts0_MPE_IPmeta_tdata),
    .siIP_V_ipAddress_V_TVALID       (sNts0_MPE_IPmeta_tvalid),
    .siIP_V_ipAddress_V_TREADY       (sNts0_MPE_IPmeta_tready),
    .soTcp_TDATA        (sMPE_NTS_Tcp_TDATA),
    .soTcp_TVALID        (sMPE_NTS_Tcp_TVALID),
    .soTcp_TREADY        (sMPE_NTS_Tcp_TREADY),
    .soTcp_TKEEP        (sMPE_NTS_Tcp_TKEEP),
    .soTcp_TLAST        (sMPE_NTS_Tcp_TLAST),
    .soIP_V_ipAddress_V_TDATA        (sMPE_Nts0_IPmeta_tdata),
    .soIP_V_ipAddress_V_TVALID       (sMPE_Nts0_IPmeta_tvalid),
    .soIP_V_ipAddress_V_TREADY       (sMPE_Nts0_IPmeta_tready),
    .siMPIif_V_mpi_call_V_TDATA      (sMPIif_V_mpi_call_V_TDATA),
    .siMPIif_V_mpi_call_V_TVALID     (sMPIif_V_mpi_call_V_TVALID),
    .siMPIif_V_mpi_call_V_TREADY     (sMPIif_V_mpi_call_V_TREADY), 
    .siMPIif_V_count_V_TDATA         (sMPIif_V_count_V_TDATA),
    .siMPIif_V_count_V_TVALID        (sMPIif_V_count_V_TVALID),
    .siMPIif_V_count_V_TREADY        (sMPIif_V_count_V_TREADY), 
    .siMPIif_V_rank_V_TDATA          (sMPIif_V_rank_V_TDATA ),
    .siMPIif_V_rank_V_TVALID         (sMPIif_V_rank_V_TVALID),
    .siMPIif_V_rank_V_TREADY         (sMPIif_V_rank_V_TREADY),
    //.soMPIif_V_rank_V_TDATA         (poMPE_ROLE_MPIif_out_rank_TDATA),
    //.soMPIif_V_rank_V_TVALID        (poMPE_ROLE_MPIif_out_rank_TVALID),
    //.soMPIif_V_rank_V_TREADY        (sDECOUP_MPE_ROLE_MPIif_out_rank_TREADY),
    //.soMPIif_V_count_V_TDATA         (poMPE_ROLE_MPIif_out_count_TDATA),
    //.soMPIif_V_count_V_TVALID        (poMPE_ROLE_MPIif_out_count_TVALID),
    //.soMPIif_V_count_V_TREADY        (sDECOUP_MPE_ROLE_MPIif_out_count_TREADY),
    //.soMPIif_V_mpi_call_V_TDATA         (poMPE_ROLE_MPIif_out_mpi_call_TDATA),
    //.soMPIif_V_mpi_call_V_TVALID        (poMPE_ROLE_MPIif_out_mpi_call_TVALID),
    //.soMPIif_V_mpi_call_V_TREADY        (sDECOUP_MPE_ROLE_MPIif_out_mpi_call_TREADY),
    .siMPI_data_TDATA        (sROLE_MPE_MPI_data_TDATA),
    .siMPI_data_TVALID       (sROLE_MPE_MPI_data_TVALID),
    .siMPI_data_TREADY       (sROLE_MPE_MPI_data_TREADY),
    .siMPI_data_TKEEP        (sROLE_MPE_MPI_data_TKEEP),
    .siMPI_data_TLAST        (sROLE_MPE_MPI_data_TLAST),
    .soMPI_data_TDATA        (sMPE_ROLE_MPI_data_TDATA),
    .soMPI_data_TVALID       (sMPE_ROLE_MPI_data_TVALID),
    .soMPI_data_TREADY       (sMPE_ROLE_MPI_data_TREADY),
    .soMPI_data_TKEEP        (sMPE_ROLE_MPI_data_TKEEP),
    .soMPI_data_TLAST        (sMPE_ROLE_MPI_data_TLAST),
    .s_axi_piSMC_MPE_ctrlLink_AXI_AWVALID        (sSMC_MPE_ctrlLink_AXI_AWVALID),
    .s_axi_piSMC_MPE_ctrlLink_AXI_AWREADY        (sSMC_MPE_ctrlLink_AXI_AWREADY),
    .s_axi_piSMC_MPE_ctrlLink_AXI_AWADDR        (sSMC_MPE_ctrlLink_AXI_AWADDR),
    .s_axi_piSMC_MPE_ctrlLink_AXI_WVALID        (sSMC_MPE_ctrlLink_AXI_WVALID),
    .s_axi_piSMC_MPE_ctrlLink_AXI_WREADY        (sSMC_MPE_ctrlLink_AXI_WREADY),
    .s_axi_piSMC_MPE_ctrlLink_AXI_WDATA        (sSMC_MPE_ctrlLink_AXI_WDATA),
    .s_axi_piSMC_MPE_ctrlLink_AXI_WSTRB        (sSMC_MPE_ctrlLink_AXI_WSTRB),
    .s_axi_piSMC_MPE_ctrlLink_AXI_ARVALID        (sSMC_MPE_ctrlLink_AXI_ARVALID),
    .s_axi_piSMC_MPE_ctrlLink_AXI_ARREADY        (sSMC_MPE_ctrlLink_AXI_ARREADY),
    .s_axi_piSMC_MPE_ctrlLink_AXI_ARADDR        (sSMC_MPE_ctrlLink_AXI_ARADDR),
    .s_axi_piSMC_MPE_ctrlLink_AXI_RVALID        (sSMC_MPE_ctrlLink_AXI_RVALID),
    .s_axi_piSMC_MPE_ctrlLink_AXI_RREADY        (sSMC_MPE_ctrlLink_AXI_RREADY),
    .s_axi_piSMC_MPE_ctrlLink_AXI_RDATA        (sSMC_MPE_ctrlLink_AXI_RDATA),
    .s_axi_piSMC_MPE_ctrlLink_AXI_RRESP        (sSMC_MPE_ctrlLink_AXI_RRESP),
    .s_axi_piSMC_MPE_ctrlLink_AXI_BVALID        (sSMC_MPE_ctrlLink_AXI_BVALID),
    .s_axi_piSMC_MPE_ctrlLink_AXI_BREADY        (sSMC_MPE_ctrlLink_AXI_BREADY),
    .s_axi_piSMC_MPE_ctrlLink_AXI_BRESP        (sSMC_MPE_ctrlLink_AXI_BRESP)
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
