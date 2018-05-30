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
// * Comments:
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - 10G ETHERNET LOOPBACK TURN
// *****************************************************************************

module TenGigEth_Loop (

  //-- Clocks and Resets inputs ------------------
  input         piEthCoreClk,
  input         piEthCoreResetDone,  

  // -- EMIF : Control Inputs and Status Ouputs --
  input         piLoopbackEn,
   
  //-- Input AXI-Write Stream from L2 ------------
  input  [63:0] piLY2_Axis_tdata, 
  input  [ 7:0] piLY2_Axis_tkeep,
  input         piLY2_Axis_tlast,
  input         piLY2_Axis_tvalid,
  output        poLy2_Axis_tready,
        
  //-- Output AXI-Write Stream to L2 -------------
  input         piLY2_Axis_tready,
  output [63:0] poLy2_Axis_tdata,
  output [ 7:0] poLy2_Axis_tkeep,
  output        poLy2_Axis_tlast,
  output        poLy2_Axis_tvalid,
       
  //-- Input AXI-Write Stream from L3 ------------
  input  [63:0] piLY3_Axis_tdata,
  input  [ 7:0] piLY3_Axis_tkeep,
  input         piLY3_Axis_tlast,
  input         piLY3_Axis_tvalid,
  output        poLy3_Axis_tready,
        
  //-- Output AXI-Write Stream to L3 -------------
  input         piLY3_Axis_tready,
  output [63:0] poLy3_Axis_tdata,
  output [ 7:0] poLy3_Axis_tkeep,
  output        poLy3_Axis_tlast,
  output        poLy3_Axis_tvalid

);  // End of PortList

  // Parameters
  parameter gSecurityPriviledges = "user";  // "user" or "super"
  parameter gBitstreamUsage      = "user";  // "user" or "flash"
  
// *****************************************************************************


  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //-- AXI4 Stream MUX -> SWP --------------------
  wire   [63:0] sMUX_Axis_tdata;
  wire   [ 7:0] sMUX_Axis_tkeep;
  wire          sMUX_Axis_tlast;
  wire          sMUX_Axis_tvalid;
  
  wire          sMUX_Axis_tready;
  wire          sSWAP_Axis_tready;
 
    // Metastable signals
  wire          sMETA0_LoopbackEn;
  
  
  //============================================================================
  //  INST: ANTI-METASTABILITY BLOCKS
  //============================================================================
  TenGigEth_SyncBlock META0 (
    .data_in    (piLoopbackEn),
    .clk        (piEthCoreClk),
    .data_out   (sMETA0_LoopbackEn)
  );
  
  
  //============================================================================
  //  INST: AXI-STREAMING MULTIPLEXER
  //============================================================================
  TenGigEth_Loop_AxisMux MUX (
    
    // -- MUX Selector --------------------------
    .mux_select       (sMETA0_LoopbackEn),

    //-- MUX Inputs left -------------------------
    .tdata0           (piLY2_Axis_tdata),
    .tkeep0           (piLY2_Axis_tkeep),
    .tlast0           (piLY2_Axis_tlast),
    .tvalid0          (piLY2_Axis_tvalid),
    .tready0          (sMUX_Axis_tready),
  
    //-- MUX Inputs Right ------------------------
    .tdata1           (piLY3_Axis_tdata),
    .tkeep1           (piLY3_Axis_tkeep),
    .tlast1           (piLY3_Axis_tlast),
    .tvalid1          (piLY3_Axis_tvalid),
    .tready1          (poLy3_Axis_tready),
  
    // -- Mux Outputs ----------------------------
    .tdata            (sMUX_Axis_tdata),
    .tkeep            (sMUX_Axis_tkeep),
    .tlast            (sMUX_Axis_tlast),
    .tvalid           (sMUX_Axis_tvalid),
    .tready           (sSWAP_Axis_tready)
  );


  //============================================================================
  //  INST: ETHERNET MAC ADDRESS SWAPPER
  //============================================================================
  TenGigEth_Loop_AddrSwap SWAP (
  
    //-- Clocks and Resets inputs ------------------
    .piEthCoreClk       (piEthCoreClk),
    .piEthCoreResetDone (piEthCoreResetDone),
    
    // -- SWAP Enable ------------------------------
    .piSwapEn           (sMETA0_LoopbackEn),
    
    //-- SWAP Receive Interface --------------------
    .pi_Axis_tdata      (sMUX_Axis_tdata),
    .pi_Axis_tkeep      (sMUX_Axis_tkeep),
    .pi_Axis_tlast      (sMUX_Axis_tlast),
    .pi_Axis_tvalid     (sMUX_Axis_tvalid),
    .po_Axis_tready     (sSWAP_Axis_tready),
     
    //-- SWAP Transmit Interface -------------------
    .pi_Axis_tready     (piLY2_Axis_tready),  
    .po_Axis_tdata      (poLy2_Axis_tdata),
    .po_Axis_tkeep      (poLy2_Axis_tkeep),
    .po_Axis_tlast      (poLy2_Axis_tlast),
    .po_Axis_tvalid     (poLy2_Axis_tvalid)

  );
  
  
  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poLy3_Axis_tdata  = piLY2_Axis_tdata;
  assign poLy3_Axis_tkeep  = piLY2_Axis_tkeep;
  assign poLy3_Axis_tlast  = piLY2_Axis_tlast;
  assign poLy3_Axis_tvalid = piLY2_Axis_tvalid;
  assign poLy2_Axis_tready = (sMETA0_LoopbackEn) ? sMUX_Axis_tready : piLY3_Axis_tready;  
 
endmodule
