/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

// ----------------------------------------------------------------------------
// (c) Copyright 2014 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Title      : ETHERNET CORE
// Project    : 10G/25G Gigabit Ethernet
//-----------------------------------------------------------------------------
// File       : tenGigEth_Core.v
// Author     : Xilinx Inc.
//-----------------------------------------------------------------------------
// Description: This is the Ethernet layer 2 core of the 10GE subsystem. It 
//              implements the data link layer and the physical layer with two
//              AXI FIFOs connected to the AXI-S transmit and receive interfaces
//              of an Ethernet IP core.
//-----------------------------------------------------------------------------

`timescale 1ps / 1ps

module TenGigEth_Core #(
  parameter   gEthId    =   0,  // The interface to instanciate (default=0 for ETH0)
  parameter   gAutoNeg  =   0,  // The PCS/PMA option (default=0 for 10GBASE-R)
  parameter   FIFO_SIZE = 1024
) (
   
   //-- Clocks and Resets inputs ----------------
   input                               refclk_p,
   input                               refclk_n,
   input                               dclk,
   input                               reset,
   //-- Clocks and Resets outputs ---------------
   output                              resetdone_out,
   output                              coreclk_out,
   output                              rxrecclk_out,
   output                              qplllock_out,
   //-- AXI4 Input Stream Interface --------------
   input                               tx_axis_mac_aresetn,
   input                               tx_axis_fifo_aresetn,
   input        [63 : 0]               tx_axis_fifo_tdata,
   input        [ 7 : 0]               tx_axis_fifo_tkeep,
   input                               tx_axis_fifo_tvalid,
   input                               tx_axis_fifo_tlast,
   output                              tx_axis_fifo_tready,
   //-- AXI4 Output Stream Interface ------------
   input                               rx_axis_mac_aresetn,
   input                               rx_axis_fifo_aresetn,
   output       [63 : 0]               rx_axis_fifo_tdata,
   output       [ 7 : 0]               rx_axis_fifo_tkeep,
   output                              rx_axis_fifo_tvalid,
   output                              rx_axis_fifo_tlast,
   input                               rx_axis_fifo_tready,
   //-- Gigabit Transceivers ---------------------
   output                              txp,
   output                              txn,
   input                               rxp,
   input                               rxn,
   //-- GT Configuration and Status Signals ------
   input                               transceiver_debug_gt_rxlpmen,
   input        [ 3:  0]               transceiver_debug_gt_txdiffctrl,
   input        [ 4:  0]               transceiver_debug_gt_txprecursor,
   input        [ 4:  0]               transceiver_debug_gt_txpostcursor,
   //-- PCS/PMA Configuration and Status Signals -
   input       [535 : 0]               pcs_pma_configuration_vector,
   output      [447 : 0]               pcs_pma_status_vector,
   //-- PCS/PMA Miscellaneous Ports -------------- 
   output       [ 7 : 0]               pcspma_status,  
   //-- MAC Configuration and Status Signals ----
   input        [79 : 0]               mac_tx_configuration_vector,
   input        [79 : 0]               mac_rx_configuration_vector,
   output       [ 2 : 0]               mac_status_vector,
   //-- Pause Control Interface ------------------
   input        [15 : 0]               pause_val,
   input                               pause_req,  
   //-- Optical Module Interface -----------------
   input                               signal_detect,
   input                               tx_fault,
   output                              tx_disable,
   //-- Statistics Vectors Outputs ---------------
   output       [25 : 0]               tx_statistics_vector,
   output       [29 : 0]               rx_statistics_vector,
   output                              tx_statistics_valid,
   output                              rx_statistics_valid,
   //-- Others
   input                               sim_speedup_control,
   input        [ 7 : 0]               tx_ifg_delay
    
  );  // End of PortList
  
  // Local Parameters

   
  //****************************************************************************


  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  wire rx_axis_mac_aresetn_i  = ~reset | rx_axis_mac_aresetn;
  wire rx_axis_fifo_aresetn_i = ~reset | rx_axis_fifo_aresetn;
  wire tx_axis_mac_aresetn_i  = ~reset | tx_axis_mac_aresetn;
  wire tx_axis_fifo_aresetn_i = ~reset | tx_axis_fifo_aresetn;

  wire         [63:0]                  tx_axis_mac_tdata;
  wire         [7:0]                   tx_axis_mac_tkeep;
  wire                                 tx_axis_mac_tvalid;
  wire                                 tx_axis_mac_tlast;
  wire                                 tx_axis_mac_tready;
  
  wire         [63:0]                  rx_axis_mac_tdata;
  wire         [7:0]                   rx_axis_mac_tkeep;
  wire                                 rx_axis_mac_tvalid;
  wire                                 rx_axis_mac_tuser;
  wire                                 rx_axis_mac_tlast;
  
  wire                                 coreclk;

  assign coreclk_out = coreclk;


  //============================================================================
  //  CONDITIONAL INSTANTIATION OF THE ETHERNET CORE IP
  //    Depending on the values of gEthId and gAutoNeg.
  //============================================================================
  generate 

    if (gEthId == 0) begin
      
      //------------------------------------------------------------------------
      // INST: CORE IP for ETH0 (i.e. X1Y8)
      //------------------------------------------------------------------------

      if (gAutoNeg == 0) begin
      
        //----------------------------------------------------------------------
        // INST: CORE IP w/o Autonegociation (i.e. 10GBASE-R)
        //----------------------------------------------------------------------
           
        TenGigEthSubSys_X1Y8_R IP (
          //-- Clocks and Resets inputs ----------------
          .refclk_p                        (refclk_p),
          .refclk_n                        (refclk_n),
          .dclk                            (dclk),
          .reset                           (reset),
          //-- Clocks and Resets outputs ---------------
          .resetdone_out                   (resetdone_out),
          .coreclk_out                     (),
          .rxrecclk_out                    (rxrecclk_out),
          .qpll0lock_out                   (qplllock_out),                          
          .areset_coreclk_out              (),  // Synced reset w/ coreclk_out
          .areset_datapathclk_out          (),
          .reset_counter_done_out          (),
          .qpll0outclk_out                 (),
          .qpll0outrefclk_out              (),
          .txusrclk_out                    (),
          .txusrclk2_out                   (coreclk),
          .gttxreset_out                   (),
          .gtrxreset_out                   (),
          .txuserrdy_out                   (),
 
          .tx_ifg_delay                    (tx_ifg_delay),
          .tx_statistics_vector            (tx_statistics_vector),
          .tx_statistics_valid             (tx_statistics_valid),
          .rx_statistics_vector            (rx_statistics_vector),
          .rx_statistics_valid             (rx_statistics_valid),
          .s_axis_pause_tdata              (pause_val),
          .s_axis_pause_tvalid             (pause_req),
      
          .tx_axis_aresetn                 (tx_axis_mac_aresetn),
          .s_axis_tx_tdata                 (tx_axis_mac_tdata),
          .s_axis_tx_tvalid                (tx_axis_mac_tvalid),
          .s_axis_tx_tlast                 (tx_axis_mac_tlast),
          .s_axis_tx_tuser                 (1'b0),
          .s_axis_tx_tkeep                 (tx_axis_mac_tkeep),
          .s_axis_tx_tready                (tx_axis_mac_tready),
      
          .rx_axis_aresetn                 (rx_axis_mac_aresetn),
          .m_axis_rx_tdata                 (rx_axis_mac_tdata),
          .m_axis_rx_tkeep                 (rx_axis_mac_tkeep),
          .m_axis_rx_tvalid                (rx_axis_mac_tvalid),
          .m_axis_rx_tuser                 (rx_axis_mac_tuser),
          .m_axis_rx_tlast                 (rx_axis_mac_tlast),
          .mac_tx_configuration_vector     (mac_tx_configuration_vector),
          .mac_rx_configuration_vector     (mac_rx_configuration_vector),
          .mac_status_vector               (mac_status_vector),
          .pcs_pma_configuration_vector    (pcs_pma_configuration_vector),
          .pcs_pma_status_vector           (pcs_pma_status_vector),
          // Serial links
          .txp                             (txp),
          .txn                             (txn),
          .rxp                             (rxp),
          .rxn                             (rxn),
          //-- PCS/PMA Miscellaneous Ports --------------
          .pcspma_status                   (pcspma_status),
          //-- Optical Module Interface -----------------
          .signal_detect                   (1'b1),
          .tx_fault                        (1'b0),
          .tx_disable                      (tx_disable),
          //-- Transceiver debug Signals ---------------
          .transceiver_debug_gt_cpllpd           (1'b1),
          .transceiver_debug_gt_dmonitorout      (),
          .transceiver_debug_gt_eyescandataerror (),
          .transceiver_debug_gt_eyescanreset     (1'b0),
          .transceiver_debug_gt_eyescantrigger   (1'b0),
          .transceiver_debug_gt_loopback         (3'b000),
          .transceiver_debug_gt_pcsrsvdin        (16'b0),
          .transceiver_debug_gt_qpll0pd          (1'b0),
          .transceiver_debug_gt_qpll1pd          (1'b1), 
          .transceiver_debug_gt_rxbufstatus      (), 
          .transceiver_debug_gt_rxcdrhold        (1'b0),
          .transceiver_debug_gt_rxdfelpmreset    (1'b0),
          .transceiver_debug_gt_rxlpmen          (transceiver_debug_gt_rxlpmen),  // 0:DFE or 1:LPM
          .transceiver_debug_gt_rxpd             (2'b0),
          .transceiver_debug_gt_rxpmareset       (1'b0),
          .transceiver_debug_gt_rxpmaresetdone   (),
          .transceiver_debug_gt_rxpolarity       (1'b0),
          .transceiver_debug_gt_rxprbserr        (),
          .transceiver_debug_gt_rxprbslocked     (),
          .transceiver_debug_gt_rxrate           (3'b0),
          .transceiver_debug_gt_rxresetdone      (),
          //--
          .transceiver_debug_gt_txbufstatus      (),
          .transceiver_debug_gt_txdiffctrl       (transceiver_debug_gt_txdiffctrl), // TxDriverSwing
          .transceiver_debug_gt_txelecidle       (1'b0),
          .transceiver_debug_gt_txoutclksel      (3'b101),
          .transceiver_debug_gt_txpcsreset       (1'b0),
          .transceiver_debug_gt_txpd             (2'b0),
          .transceiver_debug_gt_txpdelecidlemode (1'b0),
          .transceiver_debug_gt_txpmareset       (1'b0),
          .transceiver_debug_gt_txpolarity       (1'b0),
          .transceiver_debug_gt_txpostcursor     (transceiver_debug_gt_txpostcursor), // TxPostCursor
          .transceiver_debug_gt_txprbsforceerr   (1'b0),
          .transceiver_debug_gt_txprecursor      (transceiver_debug_gt_txprecursor),  // TxPrecursor
          .transceiver_debug_gt_txresetdone      (),
          //-- Others ----------------------------
          .sim_speedup_control                   (sim_speedup_control)
        );

      end   // if (gAutoNeg == 0)

    end   // if (gEthId == 0)
    
  endgenerate
  

  //============================================================================
  //  INST: ETHERNET MAC FIFO
  //============================================================================
  TenGigEth_MacFifo #(
    .TX_FIFO_SIZE                    (FIFO_SIZE),
    .RX_FIFO_SIZE                    (FIFO_SIZE)
  ) FIFO (
    .tx_axis_fifo_aresetn            (tx_axis_fifo_aresetn_i),
    .tx_axis_fifo_aclk               (coreclk),
    .tx_axis_fifo_tdata              (tx_axis_fifo_tdata),
    .tx_axis_fifo_tkeep              (tx_axis_fifo_tkeep),
    .tx_axis_fifo_tvalid             (tx_axis_fifo_tvalid),
    .tx_axis_fifo_tlast              (tx_axis_fifo_tlast),
    .tx_axis_fifo_tready             (tx_axis_fifo_tready),
    .tx_fifo_full                    (),
    .tx_fifo_status                  (),
    .rx_axis_fifo_aresetn            (rx_axis_fifo_aresetn_i),
    .rx_axis_fifo_aclk               (coreclk),
    .rx_axis_fifo_tdata              (rx_axis_fifo_tdata),
    .rx_axis_fifo_tkeep              (rx_axis_fifo_tkeep),
    .rx_axis_fifo_tvalid             (rx_axis_fifo_tvalid),
    .rx_axis_fifo_tlast              (rx_axis_fifo_tlast),
    .rx_axis_fifo_tready             (rx_axis_fifo_tready),
    .rx_fifo_status                  (),
    .tx_axis_mac_aresetn             (tx_axis_mac_aresetn_i),
    .tx_axis_mac_aclk                (coreclk),
    .tx_axis_mac_tdata               (tx_axis_mac_tdata),
    .tx_axis_mac_tkeep               (tx_axis_mac_tkeep),
    .tx_axis_mac_tvalid              (tx_axis_mac_tvalid),
    .tx_axis_mac_tlast               (tx_axis_mac_tlast),
    .tx_axis_mac_tready              (tx_axis_mac_tready),
    .rx_axis_mac_aresetn             (rx_axis_mac_aresetn_i),
    .rx_axis_mac_aclk                (coreclk),
    .rx_axis_mac_tdata               (rx_axis_mac_tdata),
    .rx_axis_mac_tkeep               (rx_axis_mac_tkeep),
    .rx_axis_mac_tvalid              (rx_axis_mac_tvalid),
    .rx_axis_mac_tlast               (rx_axis_mac_tlast),
    .rx_axis_mac_tuser               (rx_axis_mac_tuser),
    .rx_fifo_full                    ()
  );

endmodule
