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
// Title      : AXI-Streaming MUX
// Project    : 10G Gigabit Ethernet
//------------------------------------------------------------------------------
// File       : tenGigEth_Loop_AxisMux.v
// Author     : FAB
// -----------------------------------------------------------------------------
// Description:  A simple AXI-Streaming MUX
//------------------------------------------------------------------------------

`timescale 1 ps/1 ps

// *****************************************************************************
// **  MODULE - AXI STREAMING MULTIPLEXER
// *****************************************************************************

module TenGigEth_Loop_AxisMux (

  //-- MUX Selector ------------------------------
  input                   mux_select,

  //-- MUX Inputs left ---------------------------
  input       [63:0]      tdata0,
  input       [7:0]       tkeep0,
  input                   tvalid0,
  input                   tlast0,
  output reg              tready0,

  //-- MUX Inputs Right --------------------------
  input       [63:0]      tdata1,
  input       [7:0]       tkeep1,
  input                   tvalid1,
  input                   tlast1,
  output reg              tready1,

  // -- Mux Outputs ------------------------------
  output reg  [63:0]      tdata,
  output reg  [7:0]       tkeep,
  output reg              tvalid,
  output reg              tlast,
  input                   tready
  
  );
  
// *****************************************************************************  
  
  
  //============================================================================
  //  PROC: DATA PATH MULTIPLEXER
  //============================================================================
  always @(mux_select or tdata0 or tvalid0 or tlast0 or tdata1 or tkeep0 or tkeep1 or
           tvalid1 or tlast1)
  begin
    if (mux_select) begin
      tdata    = tdata1;
      tkeep    = tkeep1;
      tvalid   = tvalid1;
      tlast    = tlast1;
    end
    else begin
      tdata    = tdata0;
      tkeep    = tkeep0;
      tvalid   = tvalid0;
      tlast    = tlast0;
    end
  end
  
  //============================================================================
  //  PROC: CTRL PATH MULTIPLEXER
  //============================================================================
  always @(mux_select or tready)
  begin
    if (mux_select) begin
      tready0  = 1'b1 ;
      tready1  = tready;
    end
    else begin
      tready0  = tready;
      tready1  = 1'b1 ;
    end
  end

endmodule
