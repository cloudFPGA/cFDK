//------------------------------------------------------------------------------
// Title      : Address Swap module for Ethernet packets
// Project    : 10G Gigabit Ethernet
//------------------------------------------------------------------------------
// File       : tenGigEth_Loop_AddrSwap.v
// Author     : Xilinx Inc.
//------------------------------------------------------------------------------
// Description: This is the address swapper block that swaps the source and 
//              destination MAC addresses of every Ethernet frame that passes 
//              through it when the loopback turn is enabled.
//              When loopback turn is not enabled this module is transparent.
//------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
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


`timescale 1ps / 1ps

// *****************************************************************************
// **  MODULE - ETHERNET MAC ADDRESS SWAPPER
// *****************************************************************************

module TenGigEth_Loop_AddrSwap (

  //-- Clocks and Resets inputs ------------------
  input              piEthCoreClk,
  input              piEthCoreResetDone,
  
  // -- SWAP Enable ------------------------------
  input              piSwapEn,
  //OBSOLETE-20171127 input  enable_custom_preamble,
  
  //-- SWAP Receive Interface --------------------
  input       [63:0] pi_Axis_tdata,
  input       [7:0]  pi_Axis_tkeep,
  input              pi_Axis_tlast,
  input              pi_Axis_tvalid,
  output             po_Axis_tready,
  
  //-- SWAP Transmit Interface -------------------
  input              pi_Axis_tready,
  output      [63:0] po_Axis_tdata,
  output      [7:0]  po_Axis_tkeep,
  output             po_Axis_tlast,
  output             po_Axis_tvalid
 
 );
 
// ***************************************************************************** 


  localparam  IDLE        = 0,
              PREAMBLE    = 1,
              ADDR        = 2,
              TLAST_SEEN  = 3;

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  reg  [1:0]   sStateReg;
  
  // two state fifo state machine
  reg          data_stored_n;

  // single register in Local Link data path
  reg  [63:0]  sRx_AxisDataReg;
  reg  [63:0]  sTx_AxisData;
  reg  [63:0]  sTx_AxisDataReg;
  reg  [31:0]  sRx_AxisDataRegReg;
  reg  [7:0]   sRx_AxisKeepReg;
  reg  [7:0]   sTx_AxisKeepReg;
  
  reg          sRxSofReg_n;
  reg          sRxSofRegReg_n;
  
  reg          sRx_AxisLastReg;
  //OBSOLETE-20171127 reg          rx_axis_tvalid_reg;

  wire         sAxisDataBeat;

  reg          sTx_AxisLastReg;
  reg          sAxisDataBeatReg;
  reg          sTx_AxisValidReg;


  //============================================================================
  //  COMB: CONTINUOUS INTERNAL ASSIGNMENTS
  //============================================================================
  assign sAxisDataBeat = pi_Axis_tvalid & pi_Axis_tready;


  //============================================================================
  //  FSM: START OF FRAME
  //============================================================================
  always @(posedge piEthCoreClk)
  begin : pFsm
    if (piEthCoreResetDone) begin
      sStateReg   <= IDLE;
      sRxSofReg_n <= 0;
    end
    else begin
      case (sStateReg)
        IDLE : begin
          //OBSOLETE-20171127 if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & enable_custom_preamble & pi_Axis_tready) begin
          //OBSOLETE-20171127   sStateReg <= PREAMBLE;
          //OBSOLETE-20171127 end
          //OBSOLETE-20171127 else if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & !enable_custom_preamble & pi_Axis_tready) begin
          //OBSOLETE-20171127   sRxSofReg_n <= 1;
          //OBSOLETE-20171127   sStateReg <= ADDR;
          //OBSOLETE-20171127 end
          if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & pi_Axis_tready) begin
            sRxSofReg_n <= 1;
            sStateReg   <= ADDR;
          end
        end
        
        //OBSOLETE-20171127 PREAMBLE : begin
        //OBSOLETE-20171127   if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & pi_Axis_tready) begin
        //OBSOLETE-20171127     sRxSofReg_n <= 1;
        //OBSOLETE-20171127   end
        //OBSOLETE-20171127   sStateReg <= ADDR;
        //OBSOLETE-20171127 end
        
        ADDR : begin
          sRxSofReg_n <= 0;
          if (pi_Axis_tvalid & pi_Axis_tlast & pi_Axis_tready) begin
            sStateReg <= TLAST_SEEN;
          end
        end
        
        TLAST_SEEN : begin
          //OBSOLETE-20171127 if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & enable_custom_preamble & pi_Axis_tready) begin
          //OBSOLETE-20171127   sStateReg <= PREAMBLE;
          //OBSOLETE-20171127 end
          //OBSOLETE-20171127 else if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & !enable_custom_preamble & pi_Axis_tready) begin
          //OBSOLETE-20171127   sRxSofReg_n <= 1;
          //OBSOLETE-20171127   sStateReg <= ADDR;
          //OBSOLETE-20171127 end
          if (pi_Axis_tvalid & pi_Axis_tkeep != 0 & pi_Axis_tready) begin
            sRxSofReg_n <= 1;
            sStateReg   <= ADDR;
          end
        end
        
      endcase
    end
  end
 

  //============================================================================
  //  PROC: INPUT DATA & CTRL REGISTER
  //============================================================================
  always @(posedge piEthCoreClk)
  begin
    if (piEthCoreResetDone) begin
       sRx_AxisDataReg    <= 64'b0;
       sRx_AxisDataRegReg <= 32'b0;
       sRx_AxisKeepReg    <= 8'b0;
       sRxSofRegReg_n    <= 1'b0;
       sRx_AxisLastReg    <= 1'b0;
       
       data_stored_n             <= 1'b0;
       //OBSOLETE-20171127 rx_axis_tvalid_reg        <= 1'b0;
    end
    else begin
       //OBSOLETE-20171127 rx_axis_tvalid_reg <= piRx_AxiValid;
       sRx_AxisLastReg  <= 1'b0;
       if (sAxisDataBeat) begin
          data_stored_n     <= 1'b1;
          sRx_AxisDataReg    <= pi_Axis_tdata;
          sRx_AxisDataRegReg <= sRx_AxisDataReg[47:16];
          sRx_AxisKeepReg    <= pi_Axis_tkeep;
          sRxSofRegReg_n    <= sRxSofReg_n;
          sRx_AxisLastReg    <= pi_Axis_tlast;
 
       end
       else if (!sAxisDataBeat && sRx_AxisLastReg) begin
          sRx_AxisLastReg <= sRx_AxisLastReg;
          data_stored_n  <= 1'b0;
       end
    end
  end


  //============================================================================
  //  COMB: SWAP MAC ADDRESSES (Only when SofReg or SofRegReg)
  //============================================================================
  always @(sRxSofReg_n or sRxSofRegReg_n or
           pi_Axis_tdata or sRx_AxisDataReg or sRx_AxisDataRegReg)
  begin
    if (sRxSofReg_n)
      sTx_AxisData <= {sRx_AxisDataReg[15:0],
                     pi_Axis_tdata[31:0],
                     sRx_AxisDataReg[63:48]};
    else if (sRxSofRegReg_n)
      sTx_AxisData <= {sRx_AxisDataReg[63:32],
                      sRx_AxisDataRegReg};
    else
      sTx_AxisData <= sRx_AxisDataReg;
  end


  //============================================================================
  //  PROC: OUTPUT DATA & CTRL REGISTER
  //============================================================================
  always @(posedge piEthCoreClk) begin
    if (piEthCoreResetDone) begin
      sTx_AxisDataReg  <= 64'b0;
      sTx_AxisKeepReg  <= 8'b0;
      sTx_AxisValidReg <= 1'b0;
      sAxisDataBeatReg <= 1'b0;
      sTx_AxisLastReg  <= 1'b0;
    end
    else begin
      if (pi_Axis_tready) begin
        sTx_AxisDataReg  <= sTx_AxisData;
        sTx_AxisKeepReg  <= sRx_AxisKeepReg;
        sAxisDataBeatReg <= sAxisDataBeat;
        sTx_AxisValidReg <= sAxisDataBeatReg;
        sTx_AxisLastReg  <= sRx_AxisLastReg;
      end
    end
  end


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign po_Axis_tvalid = (piSwapEn) ? sTx_AxisValidReg : pi_Axis_tvalid;
  assign po_Axis_tdata  = (piSwapEn) ? sTx_AxisDataReg  : pi_Axis_tdata;
  assign po_Axis_tkeep  = (piSwapEn) ? sTx_AxisKeepReg  : pi_Axis_tkeep;
  assign po_Axis_tlast  = (piSwapEn) ? (sTx_AxisLastReg & pi_Axis_tready & sTx_AxisValidReg) : pi_Axis_tlast;
  assign po_Axis_tready = pi_Axis_tready;

endmodule
