// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : LOOPBACK TURN BETWEEN ETH0 LAYER-2 AND LAYER-3.
// *
// * File    : tenGigEth_Loop.v
// *
// * Created : Nov. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : The SHELL of the cloudFPGA can be configured to integrate a 
// *    loopback turn between the network layers L2 and L3 of the ETH0 interface.
// *    When this loopback is enabled, the data output by the AXI4-S interface
// *    of ETH0 are passed back to the AXI4-S input of the same ETH0. Otherwise,
// *    this module is neutral and data pass through it untouched.  
// * 
// * Parameters:
// *    gSecurityPriviledges: Sets the level of the security privileges.
// *      [ "user" (Default) | "super" ]
// *    gBitstreamUsage: defines the usage of the bitstream to generate.
// *      [ "user" (Default) | "flash" ]
// *
// *----------------------------------------------------------------------------
// * 
// *                     | Ly3
// *     +---------------|-------+
// *     | ELP           |       |
// *     |               |       |
// *     |  +-------+    |       |  
// *     |  |       |    |       |
// *     |  |    +--V----V--+    |
// *     |  |    |   MUX    |    |
// *     |  |    +----+-----+    |
// *     |  |         |          |
// *     |  |    +----V-----+    |
// *     |  |    |   SWAP   |    |
// *     |  |    +----+-----+    |
// *     |  |         |          |
// *     +--|---------|----------+  
// *        |         | 
// *        |   Ly2   V
// *
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  ELP - 10G ETHERNET LOOPBACK TURN
// *****************************************************************************

module TenGigEth_Loop (

  //-- Clocks and Resets inputs ------------------
  input         piETH0_CoreClk,
  input         piETH0_CoreResetDone,  

  // -- MMIO : Control Inputs and Status Ouputs --
  input         piMMIO_LoopbackEn,
  input         piMMIO_AddrSwapEn,
   
  //-- LY2 : Input AXI-Write Stream Interface ----
  input  [63:0] piLY2_Elp_Axis_tdata, 
  input  [ 7:0] piLY2_Elp_Axis_tkeep,
  input         piLY2_Elp_Axis_tlast,
  input         piLY2_Elp_Axis_tvalid,
  output        poELP_Ly2_Axis_tready,
        
   // LY2 : Output AXI-Write Stream Interface ------
  input         piLY2_Elp_Axis_tready,
  output [63:0] poELP_Ly2_Axis_tdata,
  output [ 7:0] poELP_Ly2_Axis_tkeep,
  output        poELP_Ly2_Axis_tlast,
  output        poELP_Ly2_Axis_tvalid,
       
  //-- LY3 : Input AXI-Write Stream Interface ----
  input  [63:0] piLY3_Elp_Axis_tdata,
  input  [ 7:0] piLY3_Elp_Axis_tkeep,
  input         piLY3_Elp_Axis_tlast,
  input         piLY3_Elp_Axis_tvalid,
  output        poELP_Ly3_Axis_tready,
        
  // LY3 : Input AXI-Write Stream Interface -------
  input         piLY3_Elp_Axis_tready,
  output [63:0] poELP_Ly3_Axis_tdata,
  output [ 7:0] poELP_Ly3_Axis_tkeep,
  output        poELP_Ly3_Axis_tlast,
  output        poELP_Ly3_Axis_tvalid

);  // End of PortList

  // Parameters
  parameter gSecurityPriviledges = "user";  // "user" or "super"
  parameter gBitstreamUsage      = "user";  // "user" or "flash"
  
// *****************************************************************************


  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //-- AXI4 Stream MUX -> SWP --------------------
  wire   [63:0] sMUX_Swap_Axis_tdata;
  wire   [ 7:0] sMUX_Swap_Axis_tkeep;
  wire          sMUX_Swap_Axis_tlast;
  wire          sMUX_Swap_Axis_tvalid;
  
  wire          sMUX_Swap_Axis_tready;
  wire          sSWAP_Mux_Axis_tready;
 
  // Metastable signals
  wire          sMETA0_LoopbackEn;
  wire          sMETA1_AddrSwapEn;
  
  
  //============================================================================
  //  INST META0: ANTI-METASTABILITY BLOCKS
  //============================================================================
  TenGigEth_SyncBlock META0 (
    .clk        (piETH0_CoreClk),
    .data_in    (piMMIO_LoopbackEn), 
    .data_out   (sMETA0_LoopbackEn)
  );
  
  
  //============================================================================
  //  INST META1: ANTI-METASTABILITY BLOCKS
  //============================================================================
  TenGigEth_SyncBlock META1 (
    .clk        (piETH0_CoreClk),
    .data_in    (piMMIO_AddrSwapEn), 
    .data_out   (sMETA1_AddrSwapEn)
  );
  
    
  //============================================================================
  //  INST MUX: AXI-STREAMING MULTIPLEXER
  //============================================================================
  TenGigEth_Loop_AxisMux MUX (
    
    // -- MUX Selector ---------------------------
    .mux_select       (sMETA0_LoopbackEn),

    //-- LY3 : Input AXI-Write Stream Interface ----
    .tdata0           (piLY3_Elp_Axis_tdata),
    .tkeep0           (piLY3_Elp_Axis_tkeep),
    .tlast0           (piLY3_Elp_Axis_tlast),
    .tvalid0          (piLY3_Elp_Axis_tvalid),
    .tready0          (poELP_Ly3_Axis_tready),
    
    //-- LY2 : Input AXI-Write Stream Interface ----
    .tdata1           (piLY2_Elp_Axis_tdata),
    .tkeep1           (piLY2_Elp_Axis_tkeep),
    .tlast1           (piLY2_Elp_Axis_tlast),
    .tvalid1          (piLY2_Elp_Axis_tvalid),
    .tready1          (sMUX_Swap_Axis_tready),
    
    //-- SWAP : Output AXI-Write Stream Interface --
    .tdata            (sMUX_Swap_Axis_tdata),
    .tkeep            (sMUX_Swap_Axis_tkeep),
    .tlast            (sMUX_Swap_Axis_tlast),
    .tvalid           (sMUX_Swap_Axis_tvalid),
    .tready           (sSWAP_Mux_Axis_tready)
  );


  //============================================================================
  //  INST SWAP: ETHERNET MAC ADDRESS SWAPPER
  //============================================================================
  TenGigEth_Loop_AddrSwap SWAP (
  
    //-- Clocks and Resets inputs ------------------
    .piEthCoreClk           (piETH0_CoreClk),
    .piEthCoreResetDone     (piETH0_CoreResetDone),
    
    // -- SWAP Enable ------------------------------
    .piSwapEn               (sMETA1_AddrSwapEn),
    
    //-- MUX : Input AXI-Write Stream Interface ----
    .piMUX_Swap_Axis_tdata  (sMUX_Swap_Axis_tdata),
    .piMUX_Swap_Axis_tkeep  (sMUX_Swap_Axis_tkeep),
    .piMUX_Swap_Axis_tlast  (sMUX_Swap_Axis_tlast),
    .piMUX_Swap_Axis_tvalid (sMUX_Swap_Axis_tvalid),
    .poSWAP_Mux_Axis_tready (sSWAP_Mux_Axis_tready),
     
    //-- LY2 : Output AXI-Write Stream Interface ---
    .piLY2_Swap_Axis_tready (piLY2_Elp_Axis_tready),  
    .poSWAP_Ly2_Axis_tdata  (poELP_Ly2_Axis_tdata),
    .poSWAP_Ly2_Axis_tkeep  (poELP_Ly2_Axis_tkeep),
    .poSWAP_Ly2_Axis_tlast  (poELP_Ly2_Axis_tlast),
    .poSWAP_Ly2_Axis_tvalid (poELP_Ly2_Axis_tvalid)
  );
  
  
  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poELP_Ly3_Axis_tdata  = piLY2_Elp_Axis_tdata;
  assign poELP_Ly3_Axis_tkeep  = piLY2_Elp_Axis_tkeep;
  assign poELP_Ly3_Axis_tlast  = piLY2_Elp_Axis_tlast;
  assign poELP_Ly3_Axis_tvalid = piLY2_Elp_Axis_tvalid;
  
  assign poELP_Ly2_Axis_tready = (sMETA0_LoopbackEn) ? sMUX_Swap_Axis_tready : piLY3_Elp_Axis_tready;  
 
endmodule
