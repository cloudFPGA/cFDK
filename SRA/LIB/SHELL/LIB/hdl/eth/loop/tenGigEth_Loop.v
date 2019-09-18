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
// *    loopback turn between the network layers L2 and L3 of the ETH interface.
// *    When this loopback is enabled, the data output by the AXI4-S interface
// *    of ETH are passed back to the AXI4-S input of the same ETH. Otherwise,
// *    this module is neutral and data pass through it untouched.
// *
// *----------------------------------------------------------------------------
// *                     
// *             |    LY3   | 
// *     +-------|----------|-------+
// *     | ELP   |          |       |
// *     |       |          |       |
// *     |       +-----+    |       |  
// *     |       |     |    |       |
// *     |       |  +--V----V--+    |
// *     |       |  |   MUX    |    |
// *     |       |  +----+-----+    |
// *     |       |       |          |
// *     |       |  +----V-----+    |
// *     |       |  |   SWAP   |    |
// *     |       |  +----+-----+    |
// *     |       |       |          |
// *     +-------|-------|----------+  
// *             |       | 
// *             | LY2   V
// *
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  ELP - 10G ETHERNET LOOPBACK TURN
// *****************************************************************************

module TenGigEth_Loop (

  //-- Clocks and Resets inputs ------------------
  input         piETH_CoreClk,
  input         piETH_CoreResetDone,  

  // -- MMIO : Control Inputs and Status Ouputs --
  input         piMMIO_LoopbackEn,
  input         piMMIO_AddrSwapEn,
   
  //-- LY2 : Input AXI-Write Stream Interface ----
  input  [63:0] siLY2_Data_tdata, 
  input  [ 7:0] siLY2_Data_tkeep,
  input         siLY2_Data_tlast,
  input         siLY2_Data_tvalid,
  output        siLY2_Data_tready,
        
   // LY2 : Output AXI-Write Stream Interface ------
  output [63:0] soLY2_Data_tdata,
  output [ 7:0] soLY2_Data_tkeep,
  output        soLY2_Data_tlast,
  output        soLY2_Data_tvalid,
  input         soLY2_Data_tready,
       
  //-- LY3 : Input AXI-Write Stream Interface ----
  input  [63:0] siLY3_Data_tdata,
  input  [ 7:0] siLY3_Data_tkeep,
  input         siLY3_Data_tlast,
  input         siLY3_Data_tvalid,
  output        siLY3_Data_tready,
        
  // LY3 : Output AXI-Write Stream Interface ------
  output [63:0] soLY3_Data_tdata,
  output [ 7:0] soLY3_Data_tkeep,
  output        soLY3_Data_tlast,
  output        soLY3_Data_tvalid,
  input         soLY3_Data_tready

); // End of PortList

 
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //-- AXI4 Stream MUX -> SWP --------------------
  wire   [63:0] ssMUX_SWAP_Data_tdata;
  wire   [ 7:0] ssMUX_SWAP_Data_tkeep;
  wire          ssMUX_SWAP_Data_tlast;
  wire          ssMUX_SWAP_Data_tvalid;
  wire          ssMUX_SWAP_Data_tready;
  
  wire          sMUX_LY2_Data_tready;
 
  // Metastable signals
  wire          sMETA0_LoopbackEn;
  wire          sMETA1_AddrSwapEn;
  
  
  //============================================================================
  //  INST META0: ANTI-METASTABILITY BLOCKS
  //============================================================================
  TenGigEth_SyncBlock META0 (
    .clk        (piETH_CoreClk),
    .data_in    (piMMIO_LoopbackEn), 
    .data_out   (sMETA0_LoopbackEn)
  );
  
  
  //============================================================================
  //  INST META1: ANTI-METASTABILITY BLOCKS
  //============================================================================
  TenGigEth_SyncBlock META1 (
    .clk        (piETH_CoreClk),
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
    .tdata0           (siLY3_Data_tdata),
    .tkeep0           (siLY3_Data_tkeep),
    .tlast0           (siLY3_Data_tlast),
    .tvalid0          (siLY3_Data_tvalid),
    .tready0          (siLY3_Data_tready),
    
    //-- LY2 : Input AXI-Write Stream Interface ----
    .tdata1           (siLY2_Data_tdata),
    .tkeep1           (siLY2_Data_tkeep),
    .tlast1           (siLY2_Data_tlast),
    .tvalid1          (siLY2_Data_tvalid),
    .tready1          (sMUX_LY2_Data_tready),
    
    //-- SWAP : Output AXI-Write Stream Interface --
    .tdata            (ssMUX_SWAP_Data_tdata),
    .tkeep            (ssMUX_SWAP_Data_tkeep),
    .tlast            (ssMUX_SWAP_Data_tlast),
    .tvalid           (ssMUX_SWAP_Data_tvalid),
    .tready           (ssMUX_SWAP_Data_tready)
  );

  //============================================================================
  //  INST SWAP: ETHERNET MAC ADDRESS SWAPPER
  //============================================================================
  TenGigEth_Loop_AddrSwap SWAP (
  
    //-- Clocks and Resets inputs ------------------
    .piEthCoreClk           (piETH_CoreClk),
    .piEthCoreResetDone     (piETH_CoreResetDone),
    
    // -- SWAP Enable ------------------------------
    .piSwapEn               (sMETA1_AddrSwapEn),
    
    //-- MUX : Input AXI-Write Stream Interface ----
    .siMUX_Data_tdata  (ssMUX_SWAP_Data_tdata),
    .siMUX_Data_tkeep  (ssMUX_SWAP_Data_tkeep),
    .siMUX_Data_tlast  (ssMUX_SWAP_Data_tlast),
    .siMUX_Data_tvalid (ssMUX_SWAP_Data_tvalid),
    .siMUX_Data_tready (ssMUX_SWAP_Data_tready),
     
    //-- LY2 : Output AXI-Write Stream Interface ---
    .soLY2_Data_tdata  (soLY2_Data_tdata),
    .soLY2_Data_tkeep  (soLY2_Data_tkeep),
    .soLY2_Data_tlast  (soLY2_Data_tlast),
    .soLY2_Data_tvalid (soLY2_Data_tvalid),
    .soLY2_Data_tready (soLY2_Data_tready)
  );
  
  
  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign soLY3_Data_tdata  = siLY2_Data_tdata;
  assign soLY3_Data_tkeep  = siLY2_Data_tkeep;
  assign soLY3_Data_tlast  = siLY2_Data_tlast;
  assign soLY3_Data_tvalid = siLY2_Data_tvalid;
   
  assign siLY2_Data_tready = (sMETA0_LoopbackEn) ? sMUX_LY2_Data_tready : soLY3_Data_tready;  
 
endmodule
