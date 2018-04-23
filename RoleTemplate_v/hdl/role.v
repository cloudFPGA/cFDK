// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Template ROLE for the FMKU2595 module when equipped with a XCKU060.
// *
// * File    : role.v
// *
// * Created : Dec. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : In cloudFPGA, the user application is referred to as a 'ROLE'    
// *    and is integrated along with a 'SHELL' that abstracts the HW components
// *    of the FPGA module. 
// *    The current module contains a Verilog example of such a user 'ROLE'. It
// *    is provided as a template for new HDL-based applications.
// * 
// * Parameters:
// *
// * Comments:
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - TEMPLATE ROLE FOR FMKU60
// *****************************************************************************

module Role (

  //-- Global Clock used by the entire ROLE --------------
  //---- This is the same 156.25MHz clock as the SHELL ---
  input           piSHL_156_25Clk, 

  //-- TOP : topFMKU60 Interface -------------------------
  input           piTOP_Reset,
  input           piTOP_156_25Clk,    // Freerunning
  input           piTOP_250Clk,       // Freerunning
  
  //------------------------------------------------------
  //-- SHELL / Role / Nts0 / Udp Interface
  //------------------------------------------------------
  //-- Input AXI-Write Stream Interface ----------
  input   [63:0]  piSHL_Rol_Nts0_Udp_Axis_tdata,
  input   [ 7:0]  piSHL_Rol_Nts0_Udp_Axis_tkeep,
  input           piSHL_Rol_Nts0_Udp_Axis_tlast,
  input           piSHL_Rol_Nts0_Udp_Axis_tvalid,
  output          poROL_Shl_Nts0_Udp_Axis_tready,
  //-- Output AXI-Write Stream Interface ---------
  input           piSHL_Rol_Nts0_Udp_Axis_tready,
  output  [63:0]  poROL_Shl_Nts0_Udp_Axis_tdata,
  output  [ 7:0]  poROL_Shl_Nts0_Udp_Axis_tkeep,
  output          poROL_Shl_Nts0_Udp_Axis_tlast,
  output          poROL_Shl_Nts0_Udp_Axis_tvalid,
  
  //------------------------------------------------------
  //-- SHELL / Role / Nts0 / Tcp Interface
  //------------------------------------------------------
  //-- Input AXI-Write Stream Interface ----------
  input   [63:0]  piSHL_Rol_Nts0_Tcp_Axis_tdata,
  input   [ 7:0]  piSHL_Rol_Nts0_Tcp_Axis_tkeep,
  input           piSHL_Rol_Nts0_Tcp_Axis_tlast,
  input           piSHL_Rol_Nts0_Tcp_Axis_tvalid,
  output          poROL_Shl_Nts0_Tcp_Axis_tready,
  //-- Output AXI-Write Stream Interface ---------
  input           piSHL_Rol_Nts0_Tcp_Axis_tready,
  output  [63:0]  poROL_Shl_Nts0_Tcp_Axis_tdata,
  output  [ 7:0]  poROL_Shl_Nts0_Tcp_Axis_tkeep,
  output          poROL_Shl_Nts0_Tcp_Axis_tlast,
  output          poROL_Shl_Nts0_Tcp_Axis_tvalid,
  
  //----------------------------------------------
  //-- SHELL / Role / Mem / Up0 Interface
  //----------------------------------------------
  //-- User Port #0 / S2MM-AXIS ------------------   
  //---- Stream Read Command -----------------
  input           piSHL_Rol_Mem_Up0_Axis_RdCmd_tready,
  output [ 71:0]  poROL_Shl_Mem_Up0_Axis_RdCmd_tdata,
  output          poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid,
  //---- Stream Read Status ------------------
  input  [  7:0]  piSHL_Rol_Mem_Up0_Axis_RdSts_tdata,
  input           piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid,
  output          poROL_Shl_Mem_Up0_Axis_RdSts_tready,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piSHL_Rol_Mem_Up0_Axis_Read_tdata,
  input  [ 63:0]  piSHL_Rol_Mem_Up0_Axis_Read_tkeep,
  input           piSHL_Rol_Mem_Up0_Axis_Read_tlast,
  input           piSHL_Rol_Mem_Up0_Axis_Read_tvalid,
  output          poROL_Shl_Mem_Up0_Axis_Read_tready,
  //---- Stream Write Command ----------------
  input           piSHL_Rol_Mem_Up0_Axis_WrCmd_tready,
  output [ 71:0]  poROL_Shl_Mem_Up0_Axis_WrCmd_tdata,
  output          poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid,
  //---- Stream Write Status -----------------
  input           piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid,
  input [  7:0]   piSHL_Rol_Mem_Up0_Axis_WrSts_tdata,
  output          poROL_Mem_Up0_Axis_WrSts_tready,
  //---- Stream Data Output Channel ----------
  input           piSHL_Rol_Mem_Up0_Axis_Write_tready, 
  output [511:0]  poROL_Shl_Mem_Up0_Axis_Write_tdata,
  output [ 63:0]  poROL_Shl_Mem_Up0_Axis_Write_tkeep,
  output          poROL_Shl_Mem_Up0_Axis_Write_tlast,
  output          poROL_Shl_Mem_Up0_Axis_Write_tvalid,
  
  //----------------------------------------------
  //-- SHELL / Role / Mem / Up1 Interface
  //----------------------------------------------
  //-- User Port #1 / S2MM-AXIS ------------------   
  //---- Stream Read Command -----------------
  input           piSHL_Rol_Mem_Up1_Axis_RdCmd_tready,
  output [ 71:0]  poROL_Shl_Mem_Up1_Axis_RdCmd_tdata,
  output          poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid,
  //---- Stream Read Status ------------------
  input  [  7:0]  piSHL_Rol_Mem_Up1_Axis_RdSts_tdata,
  input           piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid,
  output          poROL_Shl_Mem_Up1_Axis_RdSts_tready,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piSHL_Rol_Mem_Up1_Axis_Read_tdata,
  input  [ 63:0]  piSHL_Rol_Mem_Up1_Axis_Read_tkeep,
  input           piSHL_Rol_Mem_Up1_Axis_Read_tlast,
  input           piSHL_Rol_Mem_Up1_Axis_Read_tvalid,
  output          poROL_Shl_Mem_Up1_Axis_Read_tready,
  //---- Stream Write Command ----------------
  input           piSHL_Rol_Mem_Up1_Axis_WrCmd_tready,
  output [ 71:0]  poROL_Shl_Mem_Up1_Axis_WrCmd_tdata,
  output          poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid,
  //---- Stream Write Status -----------------
  input           piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid,
  input [  7:0]   piSHL_Rol_Mem_Up1_Axis_WrSts_tdata,
  output          poROL_Shl_Mem_Up1_Axis_WrSts_tready,
  //---- Stream Data Output Channel ----------
  input           piSHL_Rol_Mem_Up1_Axis_Write_tready, 
  output [511:0]  poROL_Shl_Mem_Up1_Axis_Write_tdata,
  output [ 63:0]  poROL_Shl_Mem_Up1_Axis_Write_tkeep,
  output          poROL_Shl_Mem_Up1_Axis_Write_tlast,
  output          poROL_Shl_Mem_Up1_Axis_Write_tvalid,
  
  output          poVoid  

);  // End of PortList

  

// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================


    
  //============================================================================
  // TEMPORARY PROC: ROLE / Nts0 / Udp Interface to AVOID UNDEFINED CONTENT
  //============================================================================
  (* keep = "true" *) reg [63:0]  sSHL_Rol_Nts0_Udp_Axis_tdata;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Nts0_Udp_Axis_tkeep;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Udp_Axis_tlast;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Udp_Axis_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Udp_Axis_tready;

  //-- Input AXI-Write Stream Interface ----------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Nts0_Udp_Axis_tdata  <= piSHL_Rol_Nts0_Udp_Axis_tdata;
    sSHL_Rol_Nts0_Udp_Axis_tkeep  <= piSHL_Rol_Nts0_Udp_Axis_tkeep;  
    sSHL_Rol_Nts0_Udp_Axis_tlast  <= piSHL_Rol_Nts0_Udp_Axis_tlast;
    sSHL_Rol_Nts0_Udp_Axis_tvalid <= piSHL_Rol_Nts0_Udp_Axis_tvalid;
  end
  assign poROL_Shl_Nts0_Udp_Axis_tready = sSHL_Rol_Nts0_Udp_Axis_tready;
  
  //-- Output AXI-Write Stream Interface --------- 
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Nts0_Udp_Axis_tready <= piSHL_Rol_Nts0_Udp_Axis_tready;
  end
  assign poROL_Shl_Nts0_Udp_Axis_tdata  = sSHL_Rol_Nts0_Udp_Axis_tdata;
  assign poROL_Shl_Nts0_Udp_Axis_tkeep  = sSHL_Rol_Nts0_Udp_Axis_tkeep;
  assign poROL_Shl_Nts0_Udp_Axis_tlast  = sSHL_Rol_Nts0_Udp_Axis_tlast;
  assign poROL_Shl_Nts0_Udp_Axis_tvalid = sSHL_Rol_Nts0_Udp_Axis_tvalid;
    
  //============================================================================
  // TEMPORARY PROC: ROLE / Nts0 / Tcp Interface to AVOID UNDEFINED CONTENT
  //============================================================================
  (* keep = "true" *) reg [63:0]  sSHL_Rol_Nts0_Tcp_Axis_tdata;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Nts0_Tcp_Axis_tkeep;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Tcp_Axis_tlast;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Tcp_Axis_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Nts0_Tcp_Axis_tready;

  //-- Input AXI-Write Stream Interface ----------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Nts0_Tcp_Axis_tdata  <= piSHL_Rol_Nts0_Tcp_Axis_tdata;
    sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= piSHL_Rol_Nts0_Tcp_Axis_tkeep;  
    sSHL_Rol_Nts0_Tcp_Axis_tlast  <= piSHL_Rol_Nts0_Tcp_Axis_tlast;
    sSHL_Rol_Nts0_Tcp_Axis_tvalid <= piSHL_Rol_Nts0_Tcp_Axis_tvalid;
  end
  assign poROL_Shl_Nts0_Tcp_Axis_tready = sSHL_Rol_Nts0_Tcp_Axis_tready;
  
  //-- Output AXI-Write Stream Interface --------- 
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Nts0_Tcp_Axis_tready <= piSHL_Rol_Nts0_Tcp_Axis_tready;
  end
  assign poROL_Shl_Nts0_Tcp_Axis_tdata  = sSHL_Rol_Nts0_Tcp_Axis_tdata;
  assign poROL_Shl_Nts0_Tcp_Axis_tkeep  = sSHL_Rol_Nts0_Tcp_Axis_tkeep;
  assign poROL_Shl_Nts0_Tcp_Axis_tlast  = sSHL_Rol_Nts0_Tcp_Axis_tlast;
  assign poROL_Shl_Nts0_Tcp_Axis_tvalid = sSHL_Rol_Nts0_Tcp_Axis_tvalid;
  
  //============================================================================
  //  TEMPORARY PROC: ROLE / Mem / Up0 Interface to AVOID UNDEFINED CONTENT
  //============================================================================
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_RdCmd_tready;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up0_Axis_RdSts_tdata;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_RdSts_tvalid;
  (* keep = "true" *) reg [63:0]  sSHL_Rol_Mem_Up0_Axis_Read_tdata;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up0_Axis_Read_tkeep;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_Read_tlast;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_Read_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_WrCmd_tready;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up0_Axis_WrSts_tdata;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_WrSts_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up0_Axis_Write_tready;
  
  //---- Stream Read Command -------------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up0_Axis_RdCmd_tready  <= piSHL_Rol_Mem_Up0_Axis_RdCmd_tready;
  end
  assign poROL_Shl_Mem_Up0_Axis_RdCmd_tdata  = 72'b1;
  assign poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid =  1'b0;
  
  //---- Stream Read Status ------------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up0_Axis_RdSts_tdata   <= piSHL_Rol_Mem_Up0_Axis_RdSts_tdata;
    sSHL_Rol_Mem_Up0_Axis_RdSts_tvalid  <= piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid;
  end 
  assign poROL_Shl_Mem_Up0_Axis_RdSts_tready = 1'b1;
  
  //---- Stream Data Input Channel ----------
  always @(posedge piSHL_156_25Clk) begin 
    sSHL_Rol_Mem_Up0_Axis_Read_tdata   <= piSHL_Rol_Mem_Up0_Axis_Read_tdata;
    sSHL_Rol_Mem_Up0_Axis_Read_tkeep   <= piSHL_Rol_Mem_Up0_Axis_Read_tkeep;
    sSHL_Rol_Mem_Up0_Axis_Read_tlast   <= piSHL_Rol_Mem_Up0_Axis_Read_tlast;
    sSHL_Rol_Mem_Up0_Axis_Read_tvalid  <= piSHL_Rol_Mem_Up0_Axis_Read_tvalid;
  end
  assign poROL_Shl_Mem_Up0_Axis_Read_tready = 1'b1;
  
  //---- Stream Write Command ----------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up0_Axis_WrCmd_tready  <= piSHL_Rol_Mem_Up0_Axis_WrCmd_tready;
  end
  assign poROL_Shl_Mem_Up0_Axis_WrCmd_tdata  = 72'b0;
  assign poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid =  1'b0;

  //---- Stream Write Status -----------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up0_Axis_WrSts_tdata   <= piSHL_Rol_Mem_Up0_Axis_WrSts_tdata;
    sSHL_Rol_Mem_Up0_Axis_WrSts_tvalid  <= piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid;
  end
  assign poROL_Shl_Mem_Up0_Axis_WrSts_tready = 1'b1;
  
  //---- Stream Data Output Channel ----------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up0_Axis_Write_tready  <= piSHL_Rol_Mem_Up0_Axis_Write_tready;  
  end
  assign poROL_Shl_Mem_Up0_Axis_Write_tdata  = 64'b0;
  assign poROL_Shl_Mem_Up0_Axis_Write_tkeep  =  8'b0;
  assign poROL_Shl_Mem_Up0_Axis_Write_tlast  =  1'b0;
  assign poROL_Shl_Mem_Up0_Axis_Write_tvalid =  1'b0;

  //============================================================================
  //  TEMPORARY PROC: ROLE / Mem / Up1 Interface to AVOID UNDEFINED CONTENT
  //============================================================================
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_RdCmd_tready;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up1_Axis_RdSts_tdata;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_RdSts_tvalid;
  (* keep = "true" *) reg [63:0]  sSHL_Rol_Mem_Up1_Axis_Read_tdata;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up1_Axis_Read_tkeep;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_Read_tlast;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_Read_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_WrCmd_tready;
  (* keep = "true" *) reg [ 7:0]  sSHL_Rol_Mem_Up1_Axis_WrSts_tdata;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_WrSts_tvalid;
  (* keep = "true" *) reg         sSHL_Rol_Mem_Up1_Axis_Write_tready;
  
  //---- Stream Read Command -------------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up1_Axis_RdCmd_tready  <= piSHL_Rol_Mem_Up1_Axis_RdCmd_tready;
  end
  assign poROL_Shl_Mem_Up1_Axis_RdCmd_tdata  = 72'b1;
  assign poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid =  1'b0;
  
  //---- Stream Read Status ------------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up1_Axis_RdSts_tdata   <= piSHL_Rol_Mem_Up1_Axis_RdSts_tdata;
    sSHL_Rol_Mem_Up1_Axis_RdSts_tvalid  <= piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid;
  end 
  assign poROL_Shl_Mem_Up1_Axis_RdSts_tready = 1'b1;
  
  //---- Stream Data Input Channel ----------
  always @(posedge piSHL_156_25Clk) begin 
    sSHL_Rol_Mem_Up1_Axis_Read_tdata   <= piSHL_Rol_Mem_Up1_Axis_Read_tdata;
    sSHL_Rol_Mem_Up1_Axis_Read_tkeep   <= piSHL_Rol_Mem_Up1_Axis_Read_tkeep;
    sSHL_Rol_Mem_Up1_Axis_Read_tlast   <= piSHL_Rol_Mem_Up1_Axis_Read_tlast;
    sSHL_Rol_Mem_Up1_Axis_Read_tvalid  <= piSHL_Rol_Mem_Up1_Axis_Read_tvalid;
  end
  assign poROL_Shl_Mem_Up1_Axis_Read_tready = 1'b1;
  
  //---- Stream Write Command ----------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up1_Axis_WrCmd_tready  <= piSHL_Rol_Mem_Up1_Axis_WrCmd_tready;
  end
  assign poROL_Shl_Mem_Up1_Axis_WrCmd_tdata  = 72'b0;
  assign poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid =  1'b0;

  //---- Stream Write Status -----------------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up1_Axis_WrSts_tdata   <= piSHL_Rol_Mem_Up1_Axis_WrSts_tdata;
    sSHL_Rol_Mem_Up1_Axis_WrSts_tvalid  <= piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid;
  end
  assign poROL_Shl_Mem_Up1_Axis_WrSts_tready = 1'b1;
  
  //---- Stream Data Output Channel ----------
  always @(posedge piSHL_156_25Clk) begin
    sSHL_Rol_Mem_Up1_Axis_Write_tready  <= piSHL_Rol_Mem_Up1_Axis_Write_tready;  
  end
  assign poROL_Shl_Mem_Up1_Axis_Write_tdata  = 64'b0;
  assign poROL_Shl_Mem_Up1_Axis_Write_tkeep  =  8'b0;
  assign poROL_Shl_Mem_Up1_Axis_Write_tlast  =  1'b0;
  assign poROL_Shl_Mem_Up1_Axis_Write_tvalid =  1'b0;

   
endmodule
