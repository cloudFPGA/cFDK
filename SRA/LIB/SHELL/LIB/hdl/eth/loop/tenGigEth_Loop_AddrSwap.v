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


//------------------------------------------------------------------------------
// Title      : Address Swap module for Ethernet packets
// Project    : 10G Gigabit Ethernet
//------------------------------------------------------------------------------
// File       : tenGigEth_Loop_AddrSwap.v
// Author     : FAB
//------------------------------------------------------------------------------
// Description: This is the address swapper block that swaps the source and 
//              destination MAC addresses of every Ethernet frame that passes 
//              through it when the 'MacAddrSwapEn' control signal is set.
//              When 'MacAddrSwapEn' is not enabled this module is transparent.
//------------------------------------------------------------------------------

`timescale 1ps / 1ps

// *****************************************************************************
// **  SWAP - ETHERNET MAC ADDRESS SWAPPER
// *****************************************************************************

module TenGigEth_Loop_AddrSwap (

  //-- Clocks and Resets inputs ------------------
  input              piEthCoreClk,
  input              piEthCoreResetDone,
  
  // -- MMIO: SWAP Enable ------------------------
  input              piSwapEn,
  
  //-- MUX : Input AXI-Write Stream Interface ----
  input       [63:0] siMUX_Data_tdata,
  input       [7:0]  siMUX_Data_tkeep,
  input              siMUX_Data_tlast,
  input              siMUX_Data_tvalid,
  output             siMUX_Data_tready,
  
  //-- LY2 : Output AXI-Write Stream Interface ---
  output      [63:0] soLY2_Data_tdata,
  output      [7:0]  soLY2_Data_tkeep,
  output             soLY2_Data_tlast,
  output             soLY2_Data_tvalid,
  input              soLY2_Data_tready
 
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
 
  wire         sAxisDataBeat;

  reg          sTx_AxisLastReg;
  reg          sAxisDataBeatReg;
  reg          sTx_AxisValidReg;


  //============================================================================
  //  COMB: CONTINUOUS INTERNAL ASSIGNMENTS
  //============================================================================
  assign sAxisDataBeat = siMUX_Data_tvalid & soLY2_Data_tready;


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
          if (siMUX_Data_tvalid & siMUX_Data_tkeep != 0 & soLY2_Data_tready) begin
            sRxSofReg_n <= 1;
            sStateReg   <= ADDR;
          end
        end
        
        ADDR : begin
          sRxSofReg_n <= 0;
          if (siMUX_Data_tvalid & siMUX_Data_tlast & soLY2_Data_tready) begin
            sStateReg <= TLAST_SEEN;
          end
        end
        
        TLAST_SEEN : begin
          if (siMUX_Data_tvalid & siMUX_Data_tkeep != 0 & soLY2_Data_tready) begin
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
       sRxSofRegReg_n     <= 1'b0;
       sRx_AxisLastReg    <= 1'b0;

       data_stored_n             <= 1'b0;
    end
    else begin
       sRx_AxisLastReg  <= 1'b0;
       if (sAxisDataBeat) begin
          data_stored_n      <= 1'b1;
          sRx_AxisDataReg    <= siMUX_Data_tdata;
          sRx_AxisDataRegReg <= sRx_AxisDataReg[47:16];
          sRx_AxisKeepReg    <= siMUX_Data_tkeep;
          sRxSofRegReg_n     <= sRxSofReg_n;
          sRx_AxisLastReg    <= siMUX_Data_tlast;
 
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
           siMUX_Data_tdata or sRx_AxisDataReg or sRx_AxisDataRegReg)
  begin
    if (sRxSofReg_n)
      sTx_AxisData <= {sRx_AxisDataReg[15:0],
                     siMUX_Data_tdata[31:0],
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
      if (soLY2_Data_tready) begin
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
  assign soLY2_Data_tdata  = (piSwapEn) ?  sTx_AxisDataReg  : siMUX_Data_tdata;
  assign soLY2_Data_tkeep  = (piSwapEn) ?  sTx_AxisKeepReg  : siMUX_Data_tkeep;
  assign soLY2_Data_tlast  = (piSwapEn) ? (sTx_AxisLastReg & soLY2_Data_tready & sTx_AxisValidReg) : siMUX_Data_tlast;
  assign soLY2_Data_tvalid = (piSwapEn) ?  sTx_AxisValidReg : siMUX_Data_tvalid;
  
  assign siMUX_Data_tready = soLY2_Data_tready;

endmodule
