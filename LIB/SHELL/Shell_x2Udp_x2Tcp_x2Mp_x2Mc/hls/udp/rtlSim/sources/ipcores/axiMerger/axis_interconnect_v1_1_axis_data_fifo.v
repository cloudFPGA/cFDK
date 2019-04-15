//  (c) Copyright 2012-2014 Xilinx, Inc. All rights reserved.
//
//  This file contains confidential and proprietary information
//  of Xilinx, Inc. and is protected under U.S. and
//  international copyright and other intellectual property
//  laws.
//
//  DISCLAIMER
//  This disclaimer is not a license and does not grant any
//  rights to the materials distributed herewith. Except as
//  otherwise provided in a valid license issued to you by
//  Xilinx, and to the maximum extent permitted by applicable
//  law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
//  WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
//  AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
//  BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
//  INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
//  (2) Xilinx shall not be liable (whether in contract or tort,
//  including negligence, or under any other theory of
//  liability) for any loss or damage of any kind or nature
//  related to, arising under or in connection with these
//  materials, including for any direct, or any indirect,
//  special, incidental, or consequential loss or damage
//  (including loss of data, profits, goodwill, or any type of
//  loss or damage suffered as a result of any action brought
//  by a third party) even if such damage or loss was
//  reasonably foreseeable or Xilinx had been advised of the
//  possibility of the same.
//
//  CRITICAL APPLICATIONS
//  Xilinx products are not designed or intended to be fail-
//  safe, or for use in any application requiring fail-safe
//  performance, such as life-support or safety devices or
//  systems, Class III medical devices, nuclear facilities,
//  applications related to the deployment of airbags, or any
//  other applications that could lead to death, personal
//  injury, or severe property or environmental damage
//  (individually and collectively, "Critical
//  Applications"). Customer assumes the sole risk and
//  liability of any use of Xilinx products in Critical
//  Applications, subject only to applicable laws and
//  regulations governing limitations on product liability.
//
//  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
//  PART OF THIS FILE AT ALL TIMES. 
//-----------------------------------------------------------------------------
//
// axis_data_fifo
//   Instantiates AXIS FIFO Generator Core
//
// Verilog-standard:  Verilog 2001
//--------------------------------------------------------------------------
//
// Structure:
//   axis_data_fifo
//     fifo-generator_v9_3
//--------------------------------------------------------------------------

`timescale 1ps/1ps

module axis_interconnect_v1_1_axis_data_fifo #
(
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
   parameter         C_FAMILY           = "virtex6",
   parameter integer C_AXIS_TDATA_WIDTH = 32,
   parameter integer C_AXIS_TID_WIDTH   = 1,
   parameter integer C_AXIS_TDEST_WIDTH = 1,
   parameter integer C_AXIS_TUSER_WIDTH = 1,
   parameter [31:0]  C_AXIS_SIGNAL_SET  = 32'hFF,
   // C_AXIS_SIGNAL_SET: each bit if enabled specifies which axis optional signals are present
   //   [0] => TREADY present
   //   [1] => TDATA present
   //   [2] => TSTRB present, TDATA must be present
   //   [3] => TKEEP present, TDATA must be present
   //   [4] => TLAST present
   //   [5] => TID present
   //   [6] => TDEST present
   //   [7] => TUSER present
   parameter integer C_FIFO_DEPTH       = 1024,
   //  Valid values 16,32,64,128,256,512,1024,2048,4096,...
   parameter integer C_FIFO_MODE  = 1,
   // Values: 
   //   0 == N0 FIFO
   //   1 == Regular FIFO
   //   2 == Store and Forward FIFO (Packet Mode). Requires TLAST. 
   parameter integer C_IS_ACLK_ASYNC    = 0,
   //  Enables async clock cross when 1.
   parameter integer C_ACLKEN_CONV_MODE  = 0,
   // C_ACLKEN_CONV_MODE: Determines how to handle the clock enable pins during
   // clock conversion
   // 0 -- Clock enables not converted
   // 1 -- S_AXIS_ACLKEN can toggle,  M_AXIS_ACLKEN always high.
   // 2 -- S_AXIS_ACLKEN always high, M_AXIS_ACLKEN can toggle.
   // 3 -- S_AXIS_ACLKEN can toggle,  M_AXIS_ACLKEN can toggle.
   parameter integer C_SYNCHRONIZER_STAGE = 2
   )
  (
///////////////////////////////////////////////////////////////////////////////
// Port Declarations
///////////////////////////////////////////////////////////////////////////////
   // System Signals
   input wire                             S_AXIS_ARESETN,
   input wire                             M_AXIS_ARESETN,
/*   input wire ACLKEN,*/

   // Slave side
   input  wire                            S_AXIS_ACLK,
   input  wire                            S_AXIS_ACLKEN,
   input  wire                            S_AXIS_TVALID,
   output wire                            S_AXIS_TREADY,
   input  wire [C_AXIS_TDATA_WIDTH-1:0]   S_AXIS_TDATA,
   input  wire [C_AXIS_TDATA_WIDTH/8-1:0] S_AXIS_TSTRB,
   input  wire [C_AXIS_TDATA_WIDTH/8-1:0] S_AXIS_TKEEP,
   input  wire                            S_AXIS_TLAST,
   input  wire [C_AXIS_TID_WIDTH-1:0]     S_AXIS_TID,
   input  wire [C_AXIS_TDEST_WIDTH-1:0]   S_AXIS_TDEST,
   input  wire [C_AXIS_TUSER_WIDTH-1:0]   S_AXIS_TUSER,

   // Master side
   input  wire                            M_AXIS_ACLK,
   input  wire                            M_AXIS_ACLKEN,
   output wire                            M_AXIS_TVALID,
   input  wire                            M_AXIS_TREADY,
   output wire [C_AXIS_TDATA_WIDTH-1:0]   M_AXIS_TDATA,
   output wire [C_AXIS_TDATA_WIDTH/8-1:0] M_AXIS_TSTRB,
   output wire [C_AXIS_TDATA_WIDTH/8-1:0] M_AXIS_TKEEP,
   output wire                            M_AXIS_TLAST,
   output wire [C_AXIS_TID_WIDTH-1:0]     M_AXIS_TID,
   output wire [C_AXIS_TDEST_WIDTH-1:0]   M_AXIS_TDEST,
   output wire [C_AXIS_TUSER_WIDTH-1:0]   M_AXIS_TUSER,

   // Status signals
   output wire [31:0]                     AXIS_DATA_COUNT,
   output wire [31:0]                     AXIS_WR_DATA_COUNT,
   output wire [31:0]                     AXIS_RD_DATA_COUNT
   );

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
`include "axis_interconnect_v1_1_axis_infrastructure.vh"

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam P_TREADY_EXISTS = C_AXIS_SIGNAL_SET[0]? 1: 0;
localparam P_TDATA_EXISTS  = C_AXIS_SIGNAL_SET[1]? 1: 0;
localparam P_TSTRB_EXISTS  = C_AXIS_SIGNAL_SET[2]? 1: 0;
localparam P_TKEEP_EXISTS  = C_AXIS_SIGNAL_SET[3]? 1: 0;
localparam P_TLAST_EXISTS  = C_AXIS_SIGNAL_SET[4]? 1: 0;
localparam P_TID_EXISTS    = C_AXIS_SIGNAL_SET[5]? 1: 0;
localparam P_TDEST_EXISTS  = C_AXIS_SIGNAL_SET[6]? 1: 0;
localparam P_TUSER_EXISTS  = C_AXIS_SIGNAL_SET[7]? 1: 0;
localparam P_AXIS_PAYLOAD_WIDTH = f_payload_width(C_AXIS_TDATA_WIDTH, C_AXIS_TID_WIDTH, C_AXIS_TDEST_WIDTH, 
                                             C_AXIS_TUSER_WIDTH, C_AXIS_SIGNAL_SET);
localparam P_WR_PNTR_WIDTH = f_clogb2(C_FIFO_DEPTH);
localparam P_FIFO_COUNT_WIDTH = P_WR_PNTR_WIDTH+1;
localparam P_FIFO_TYPE     = (C_FIFO_DEPTH > 32) ? 1 : 2; // 1 = bram, 2 = lutram.  Force 1 when > 32 deep.
localparam P_IMPLEMENTATION_TYPE_AXIS = C_IS_ACLK_ASYNC ? P_FIFO_TYPE + 10 : P_FIFO_TYPE;
localparam P_COMMON_CLOCK  = C_IS_ACLK_ASYNC ? 0 : 1;
localparam P_MSGON_VAL     = C_IS_ACLK_ASYNC ? 0 : 1;

// Packet mode only valid if tlast is enabled.  Force to 0 if no TLAST
// present.
localparam integer P_APPLICATION_TYPE_AXIS = P_TLAST_EXISTS ? (C_FIFO_MODE == 2) : 0;
localparam integer LP_S_ACLKEN_CAN_TOGGLE = (C_ACLKEN_CONV_MODE == 1) || (C_ACLKEN_CONV_MODE == 3);
localparam integer LP_M_ACLKEN_CAN_TOGGLE = (C_ACLKEN_CONV_MODE == 2) || (C_ACLKEN_CONV_MODE == 3);
                                           

////////////////////////////////////////////////////////////////////////////////
// Wires/Reg declarations
////////////////////////////////////////////////////////////////////////////////
wire [P_FIFO_COUNT_WIDTH-1:0]   axis_data_count_i;
wire [P_FIFO_COUNT_WIDTH-1:0]   axis_wr_data_count_i;
wire [P_FIFO_COUNT_WIDTH-1:0]   axis_rd_data_count_i;
(* KEEP = "TRUE" *)
wire                            s_and_m_aresetn_i;
wire                            d1_tvalid;
wire                            d1_tready;
wire [C_AXIS_TDATA_WIDTH-1:0]   d1_tdata;
wire [C_AXIS_TDATA_WIDTH/8-1:0] d1_tstrb;
wire [C_AXIS_TDATA_WIDTH/8-1:0] d1_tkeep;
wire                            d1_tlast;
wire [C_AXIS_TID_WIDTH-1:0]     d1_tid  ;
wire [C_AXIS_TDEST_WIDTH-1:0]   d1_tdest;
wire [C_AXIS_TUSER_WIDTH-1:0]   d1_tuser;

wire                            d2_tvalid;
wire                            d2_tready;
wire [C_AXIS_TDATA_WIDTH-1:0]   d2_tdata;
wire [C_AXIS_TDATA_WIDTH/8-1:0] d2_tstrb;
wire [C_AXIS_TDATA_WIDTH/8-1:0] d2_tkeep;
wire                            d2_tlast;
wire [C_AXIS_TID_WIDTH-1:0]     d2_tid  ;
wire [C_AXIS_TDEST_WIDTH-1:0]   d2_tdest;
wire [C_AXIS_TUSER_WIDTH-1:0]   d2_tuser;

////////////////////////////////////////////////////////////////////////////////
// Tie offs to reduce warnings
////////////////////////////////////////////////////////////////////////////////
localparam C_DIN_WIDTH           = 18;
localparam C_RD_PNTR_WIDTH       = 10;
localparam C_WR_PNTR_WIDTH       = 10;
localparam C_DOUT_WIDTH          = 18;
localparam C_DATA_COUNT_WIDTH    = 10;
localparam C_RD_DATA_COUNT_WIDTH = 10;
localparam C_WR_DATA_COUNT_WIDTH = 10;
localparam C_AXI_ID_WIDTH        = 4;
localparam C_AXI_ADDR_WIDTH      = 32;
localparam C_AXI_DATA_WIDTH      = 64;
localparam C_AXI_AWUSER_WIDTH    = 1;
localparam C_AXI_ARUSER_WIDTH    = 1;
localparam C_AXI_RUSER_WIDTH     = 1;
localparam C_AXI_BUSER_WIDTH     = 1;
localparam C_AXI_WUSER_WIDTH     = 1;
localparam C_WR_PNTR_WIDTH_RACH  = 4;
localparam C_WR_PNTR_WIDTH_RDCH  = 10;
localparam C_WR_PNTR_WIDTH_WACH  = 4;
localparam C_WR_PNTR_WIDTH_WDCH  = 10;
localparam C_WR_PNTR_WIDTH_WRCH  = 4;
localparam C_RD_PNTR_WIDTH_RACH  = 4;
localparam C_RD_PNTR_WIDTH_RDCH  = 10;
localparam C_RD_PNTR_WIDTH_WACH  = 4;
localparam C_RD_PNTR_WIDTH_WDCH  = 10;
localparam C_RD_PNTR_WIDTH_WRCH  = 4;
localparam C_WR_PNTR_WIDTH_AXIS  = P_WR_PNTR_WIDTH;

wire                               BACKUP;
wire                               BACKUP_MARKER;
wire                               CLK;
wire                               RST;
wire                               SRST;
wire                               WR_CLK;
wire                               WR_RST_BUSY;
wire                               WR_RST;
wire                               RD_CLK;
wire                               RD_RST_BUSY;
wire                               RD_RST;
wire [C_DIN_WIDTH-1:0]             DIN;
wire                               WR_EN;
wire                               RD_EN;
// Optional wires
wire [C_RD_PNTR_WIDTH-1:0]         PROG_EMPTY_THRESH;
wire [C_RD_PNTR_WIDTH-1:0]         PROG_EMPTY_THRESH_ASSERT;
wire [C_RD_PNTR_WIDTH-1:0]         PROG_EMPTY_THRESH_NEGATE;
wire [C_WR_PNTR_WIDTH-1:0]         PROG_FULL_THRESH;
wire [C_WR_PNTR_WIDTH-1:0]         PROG_FULL_THRESH_ASSERT;
wire [C_WR_PNTR_WIDTH-1:0]         PROG_FULL_THRESH_NEGATE;
wire                               INT_CLK;
wire                               INJECTDBITERR;
wire                               INJECTSBITERR;

wire [C_DOUT_WIDTH-1:0]           DOUT;
wire                              FULL;
wire                              ALMOST_FULL;
wire                              WR_ACK;
wire                              OVERFLOW;
wire                              EMPTY;
wire                              ALMOST_EMPTY;
wire                              VALID;
wire                              UNDERFLOW;
wire [C_DATA_COUNT_WIDTH-1:0]     DATA_COUNT;
wire [C_RD_DATA_COUNT_WIDTH-1:0]  RD_DATA_COUNT;
wire [C_WR_DATA_COUNT_WIDTH-1:0]  WR_DATA_COUNT;
wire                              PROG_FULL;
wire                              PROG_EMPTY;
wire                              SBITERR;
wire                              SLEEP;
wire                              DBITERR;


// AXI Global Signal
wire                               M_ACLK;
wire                               S_ACLK;
wire                               S_ARESETN;
wire                               S_ACLK_EN;
wire                               M_ACLK_EN;

// AXI Full/Lite Slave Write Channel (write side)
wire [C_AXI_ID_WIDTH-1:0]          S_AXI_AWID;
wire [C_AXI_ADDR_WIDTH-1:0]        S_AXI_AWADDR;
wire [8-1:0]                       S_AXI_AWLEN;
wire [3-1:0]                       S_AXI_AWSIZE;
wire [2-1:0]                       S_AXI_AWBURST;
wire [2-1:0]                       S_AXI_AWLOCK;
wire [4-1:0]                       S_AXI_AWCACHE;
wire [3-1:0]                       S_AXI_AWPROT;
wire [4-1:0]                       S_AXI_AWQOS;
wire [4-1:0]                       S_AXI_AWREGION;
wire [C_AXI_AWUSER_WIDTH-1:0]      S_AXI_AWUSER;
wire                               S_AXI_AWVALID;
wire                              S_AXI_AWREADY;
wire [C_AXI_ID_WIDTH-1:0]          S_AXI_WID;
wire [C_AXI_DATA_WIDTH-1:0]        S_AXI_WDATA;
wire [C_AXI_DATA_WIDTH/8-1:0]      S_AXI_WSTRB;
wire                               S_AXI_WLAST;
wire [C_AXI_WUSER_WIDTH-1:0]       S_AXI_WUSER;
wire                               S_AXI_WVALID;
wire                              S_AXI_WREADY;
wire [C_AXI_ID_WIDTH-1:0]         S_AXI_BID;
wire [2-1:0]                      S_AXI_BRESP;
wire [C_AXI_BUSER_WIDTH-1:0]      S_AXI_BUSER;
wire                              S_AXI_BVALID;
wire                               S_AXI_BREADY;

// AXI Full/Lite Master Write Channel (Read side)
wire [C_AXI_ID_WIDTH-1:0]         M_AXI_AWID;
wire [C_AXI_ADDR_WIDTH-1:0]       M_AXI_AWADDR;
wire [8-1:0]                      M_AXI_AWLEN;
wire [3-1:0]                      M_AXI_AWSIZE;
wire [2-1:0]                      M_AXI_AWBURST;
wire [2-1:0]                      M_AXI_AWLOCK;
wire [4-1:0]                      M_AXI_AWCACHE;
wire [3-1:0]                      M_AXI_AWPROT;
wire [4-1:0]                      M_AXI_AWQOS;
wire [4-1:0]                      M_AXI_AWREGION;
wire [C_AXI_AWUSER_WIDTH-1:0]     M_AXI_AWUSER;
wire                              M_AXI_AWVALID;
wire                               M_AXI_AWREADY;
wire [C_AXI_ID_WIDTH-1:0]         M_AXI_WID;
wire [C_AXI_DATA_WIDTH-1:0]       M_AXI_WDATA;
wire [C_AXI_DATA_WIDTH/8-1:0]     M_AXI_WSTRB;
wire                              M_AXI_WLAST;
wire [C_AXI_WUSER_WIDTH-1:0]      M_AXI_WUSER;
wire                              M_AXI_WVALID;
wire                               M_AXI_WREADY;
wire [C_AXI_ID_WIDTH-1:0]          M_AXI_BID;
wire [2-1:0]                       M_AXI_BRESP;
wire [C_AXI_BUSER_WIDTH-1:0]       M_AXI_BUSER;
wire                               M_AXI_BVALID;
wire                              M_AXI_BREADY;


// AXI Full/Lite Slave Read Channel (Write side)
wire [C_AXI_ID_WIDTH-1:0]          S_AXI_ARID;
wire [C_AXI_ADDR_WIDTH-1:0]        S_AXI_ARADDR; 
wire [8-1:0]                       S_AXI_ARLEN;
wire [3-1:0]                       S_AXI_ARSIZE;
wire [2-1:0]                       S_AXI_ARBURST;
wire [2-1:0]                       S_AXI_ARLOCK;
wire [4-1:0]                       S_AXI_ARCACHE;
wire [3-1:0]                       S_AXI_ARPROT;
wire [4-1:0]                       S_AXI_ARQOS;
wire [4-1:0]                       S_AXI_ARREGION;
wire [C_AXI_ARUSER_WIDTH-1:0]      S_AXI_ARUSER;
wire                               S_AXI_ARVALID;
wire                              S_AXI_ARREADY;
wire [C_AXI_ID_WIDTH-1:0]         S_AXI_RID;       
wire [C_AXI_DATA_WIDTH-1:0]       S_AXI_RDATA; 
wire [2-1:0]                      S_AXI_RRESP;
wire                              S_AXI_RLAST;
wire [C_AXI_RUSER_WIDTH-1:0]      S_AXI_RUSER;
wire                              S_AXI_RVALID;
wire                               S_AXI_RREADY;



// AXI Full/Lite Master Read Channel (Read side)
wire [C_AXI_ID_WIDTH-1:0]         M_AXI_ARID;        
wire [C_AXI_ADDR_WIDTH-1:0]       M_AXI_ARADDR;  
wire [8-1:0]                      M_AXI_ARLEN;
wire [3-1:0]                      M_AXI_ARSIZE;
wire [2-1:0]                      M_AXI_ARBURST;
wire [2-1:0]                      M_AXI_ARLOCK;
wire [4-1:0]                      M_AXI_ARCACHE;
wire [3-1:0]                      M_AXI_ARPROT;
wire [4-1:0]                      M_AXI_ARQOS;
wire [4-1:0]                      M_AXI_ARREGION;
wire [C_AXI_ARUSER_WIDTH-1:0]     M_AXI_ARUSER;
wire                              M_AXI_ARVALID;
wire                               M_AXI_ARREADY;
wire [C_AXI_ID_WIDTH-1:0]          M_AXI_RID;        
wire [C_AXI_DATA_WIDTH-1:0]        M_AXI_RDATA;  
wire [2-1:0]                       M_AXI_RRESP;
wire                               M_AXI_RLAST;
wire [C_AXI_RUSER_WIDTH-1:0]       M_AXI_RUSER;
wire                               M_AXI_RVALID;
wire                              M_AXI_RREADY;

// AXI Full/Lite Write Address Channel Signals
wire                               AXI_AW_INJECTSBITERR;
wire                               AXI_AW_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_WACH-1:0]   AXI_AW_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_WACH-1:0]   AXI_AW_PROG_EMPTY_THRESH;
wire [C_WR_PNTR_WIDTH_WACH:0]     AXI_AW_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WACH:0]     AXI_AW_WR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WACH:0]     AXI_AW_RD_DATA_COUNT;
wire                              AXI_AW_SBITERR;
wire                              AXI_AW_DBITERR;
wire                              AXI_AW_OVERFLOW;
wire                              AXI_AW_UNDERFLOW;
wire                              AXI_AW_PROG_FULL;
wire                              AXI_AW_PROG_EMPTY;


// AXI Full/Lite Write Data Channel Signals
wire                               AXI_W_INJECTSBITERR;
wire                               AXI_W_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_WDCH-1:0]   AXI_W_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_WDCH-1:0]   AXI_W_PROG_EMPTY_THRESH;
wire [C_WR_PNTR_WIDTH_WDCH:0]     AXI_W_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WDCH:0]     AXI_W_WR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WDCH:0]     AXI_W_RD_DATA_COUNT;
wire                              AXI_W_SBITERR;
wire                              AXI_W_DBITERR;
wire                              AXI_W_OVERFLOW;
wire                              AXI_W_UNDERFLOW;
wire                              AXI_W_PROG_FULL;
wire                              AXI_W_PROG_EMPTY;


// AXI Full/Lite Write Response Channel Signals
wire                               AXI_B_INJECTSBITERR;
wire                               AXI_B_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_WRCH-1:0]   AXI_B_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_WRCH-1:0]   AXI_B_PROG_EMPTY_THRESH;
wire [C_WR_PNTR_WIDTH_WRCH:0]     AXI_B_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WRCH:0]     AXI_B_WR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_WRCH:0]     AXI_B_RD_DATA_COUNT;
wire                              AXI_B_SBITERR;
wire                              AXI_B_DBITERR;
wire                              AXI_B_OVERFLOW;
wire                              AXI_B_UNDERFLOW;
wire                              AXI_B_PROG_FULL;
wire                              AXI_B_PROG_EMPTY;



// AXI Full/Lite Read Address Channel Signals
wire                               AXI_AR_INJECTSBITERR;
wire                               AXI_AR_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_RACH-1:0]   AXI_AR_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_RACH-1:0]   AXI_AR_PROG_EMPTY_THRESH;
wire [C_WR_PNTR_WIDTH_RACH:0]     AXI_AR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_RACH:0]     AXI_AR_WR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_RACH:0]     AXI_AR_RD_DATA_COUNT;
wire                              AXI_AR_SBITERR;
wire                              AXI_AR_DBITERR;
wire                              AXI_AR_OVERFLOW;
wire                              AXI_AR_UNDERFLOW;
wire                              AXI_AR_PROG_FULL;
wire                              AXI_AR_PROG_EMPTY;


// AXI Full/Lite Read Data Channel Signals
wire                               AXI_R_INJECTSBITERR;
wire                               AXI_R_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_RDCH-1:0]   AXI_R_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_RDCH-1:0]   AXI_R_PROG_EMPTY_THRESH;
wire [C_WR_PNTR_WIDTH_RDCH:0]     AXI_R_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_RDCH:0]     AXI_R_WR_DATA_COUNT;
wire [C_WR_PNTR_WIDTH_RDCH:0]     AXI_R_RD_DATA_COUNT;
wire                              AXI_R_SBITERR;
wire                              AXI_R_DBITERR;
wire                              AXI_R_OVERFLOW;
wire                              AXI_R_UNDERFLOW;
wire                              AXI_R_PROG_FULL;
wire                              AXI_R_PROG_EMPTY;


// AXI Streaming FIFO Related Signals
wire                               AXIS_INJECTSBITERR;
wire                               AXIS_INJECTDBITERR;
wire  [C_WR_PNTR_WIDTH_AXIS-1:0]   AXIS_PROG_FULL_THRESH;
wire  [C_WR_PNTR_WIDTH_AXIS-1:0]   AXIS_PROG_EMPTY_THRESH;
wire                              AXIS_SBITERR;
wire                              AXIS_DBITERR;
wire                              AXIS_OVERFLOW;
wire                              AXIS_UNDERFLOW;
wire                              AXIS_PROG_FULL;
wire                              AXIS_PROG_EMPTY;

assign S_ACLK_EN                = 1'b1;
assign M_ACLK_EN                = 1'b1;
assign BACKUP                   = 1'b0;
assign BACKUP_MARKER            = 1'b0;
assign CLK                      = 1'b0;
assign RST                      = 1'b0;
assign SRST                     = 1'b0;
assign WR_CLK                   = 1'b0;
assign WR_RST                   = 1'b0;
assign RD_CLK                   = 1'b0;
assign RD_RST                   = 1'b0;
assign DIN                      = {C_DIN_WIDTH{1'b0}};
assign WR_EN                    = 1'b0;
assign RD_EN                    = 1'b0;
assign PROG_EMPTY_THRESH        = {C_RD_PNTR_WIDTH{1'b0}};
assign PROG_EMPTY_THRESH_ASSERT = {C_RD_PNTR_WIDTH{1'b0}};
assign PROG_EMPTY_THRESH_NEGATE = {C_RD_PNTR_WIDTH{1'b0}};
assign PROG_FULL_THRESH         = {C_WR_PNTR_WIDTH{1'b0}};
assign PROG_FULL_THRESH_ASSERT  = {C_WR_PNTR_WIDTH{1'b0}};
assign PROG_FULL_THRESH_NEGATE  = {C_WR_PNTR_WIDTH{1'b0}};
assign INT_CLK                  = 1'b0;
assign INJECTDBITERR            = 1'b0;
assign INJECTSBITERR            = 1'b0;
assign S_AXI_AWID               = {C_AXI_ID_WIDTH{1'b0}};
assign S_AXI_AWADDR             = {C_AXI_ADDR_WIDTH{1'b0}};
assign S_AXI_AWLEN              = {8{1'b0}};
assign S_AXI_AWSIZE             = {3{1'b0}};
assign S_AXI_AWBURST            = {2{1'b0}};
assign S_AXI_AWLOCK             = {2{1'b0}};
assign S_AXI_AWCACHE            = {4{1'b0}};
assign S_AXI_AWPROT             = {3{1'b0}};
assign S_AXI_AWQOS              = {4{1'b0}};
assign S_AXI_AWREGION           = {4{1'b0}};
assign S_AXI_AWUSER             = {C_AXI_AWUSER_WIDTH{1'b0}};
assign S_AXI_AWVALID            = 1'b0;
assign S_AXI_WID                = {C_AXI_ID_WIDTH{1'b0}};
assign S_AXI_WDATA              = {C_AXI_DATA_WIDTH{1'b0}};
assign S_AXI_WSTRB              = {C_AXI_DATA_WIDTH/8{1'b0}};
assign S_AXI_WLAST              = 1'b0;
assign S_AXI_WUSER              = {C_AXI_WUSER_WIDTH{1'b0}};
assign S_AXI_WVALID             = 1'b0;
assign S_AXI_BREADY             = 1'b0;
assign M_AXI_AWREADY            = 1'b0;
assign M_AXI_WREADY             = 1'b0;
assign M_AXI_BID                = {C_AXI_ID_WIDTH{1'b0}};
assign M_AXI_BRESP              = {2{1'b0}};
assign M_AXI_BUSER              = {C_AXI_BUSER_WIDTH{1'b0}};
assign M_AXI_BVALID             = 1'b0;
assign S_AXI_ARID               = {C_AXI_ID_WIDTH{1'b0}};
assign S_AXI_ARADDR             = {C_AXI_ADDR_WIDTH{1'b0}};
assign S_AXI_ARLEN              = {8{1'b0}};
assign S_AXI_ARSIZE             = {3{1'b0}};
assign S_AXI_ARBURST            = {2{1'b0}};
assign S_AXI_ARLOCK             = {2{1'b0}};
assign S_AXI_ARCACHE            = {4{1'b0}};
assign S_AXI_ARPROT             = {3{1'b0}};
assign S_AXI_ARQOS              = {4{1'b0}};
assign S_AXI_ARREGION           = {4{1'b0}};
assign S_AXI_ARUSER             = {C_AXI_ARUSER_WIDTH{1'b0}};
assign S_AXI_ARVALID            = 1'b0;
assign S_AXI_RREADY             = 1'b0;
assign M_AXI_ARREADY            = 1'b0;
assign M_AXI_RID                = {C_AXI_ID_WIDTH{1'b0}};
assign M_AXI_RDATA              = {C_AXI_DATA_WIDTH{1'b0}};
assign M_AXI_RRESP              = {2{1'b0}};
assign M_AXI_RLAST              = 1'b0;
assign M_AXI_RUSER              = {C_AXI_RUSER_WIDTH{1'b0}};
assign M_AXI_RVALID             = 1'b0;
assign AXI_AW_INJECTSBITERR     = 1'b0;
assign AXI_AW_INJECTDBITERR     = 1'b0;
assign AXI_AW_PROG_FULL_THRESH  = {C_WR_PNTR_WIDTH_WACH{1'b0}};
assign AXI_AW_PROG_EMPTY_THRESH = {C_WR_PNTR_WIDTH_WACH{1'b0}};
assign AXI_W_INJECTSBITERR      = 1'b0;
assign AXI_W_INJECTDBITERR      = 1'b0;
assign AXI_W_PROG_FULL_THRESH   = {C_WR_PNTR_WIDTH_WDCH{1'b0}};
assign AXI_W_PROG_EMPTY_THRESH  = {C_WR_PNTR_WIDTH_WDCH{1'b0}};
assign AXI_B_INJECTSBITERR      = 1'b0;
assign AXI_B_INJECTDBITERR      = 1'b0;
assign AXI_B_PROG_FULL_THRESH   = {C_WR_PNTR_WIDTH_WRCH{1'b0}};
assign AXI_B_PROG_EMPTY_THRESH  = {C_WR_PNTR_WIDTH_WRCH{1'b0}};
assign AXI_AR_INJECTSBITERR     = 1'b0;
assign AXI_AR_INJECTDBITERR     = 1'b0;
assign AXI_AR_PROG_FULL_THRESH  = {C_WR_PNTR_WIDTH_RACH{1'b0}};
assign AXI_AR_PROG_EMPTY_THRESH = {C_WR_PNTR_WIDTH_RACH{1'b0}};
assign AXI_R_INJECTSBITERR      = 1'b0;
assign AXI_R_INJECTDBITERR      = 1'b0;
assign AXI_R_PROG_FULL_THRESH   = {C_WR_PNTR_WIDTH_RDCH{1'b0}};
assign AXI_R_PROG_EMPTY_THRESH  = {C_WR_PNTR_WIDTH_RDCH{1'b0}};
assign AXIS_INJECTSBITERR       = 1'b0;
assign AXIS_INJECTDBITERR       = 1'b0;
assign AXIS_PROG_FULL_THRESH    = {C_WR_PNTR_WIDTH_AXIS{1'b0}};
assign AXIS_PROG_EMPTY_THRESH   = {C_WR_PNTR_WIDTH_AXIS{1'b0}};
assign SLEEP                    = 1'b0;

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////

// Both S and M have to be out of reset before fifo will be let out of reset.
assign s_and_m_aresetn_i = S_AXIS_ARESETN & M_AXIS_ARESETN;

generate 
  if (C_FIFO_MODE > 0) begin : gen_fifo_generator
    assign AXIS_DATA_COUNT    = (P_COMMON_CLOCK == 1) ? {{(32-P_FIFO_COUNT_WIDTH){1'b0}}, axis_data_count_i} : AXIS_WR_DATA_COUNT;
    assign AXIS_WR_DATA_COUNT = (P_COMMON_CLOCK == 0) ? {{(32-P_FIFO_COUNT_WIDTH){1'b0}}, axis_wr_data_count_i} : AXIS_DATA_COUNT;
    assign AXIS_RD_DATA_COUNT = (P_COMMON_CLOCK == 0) ? {{(32-P_FIFO_COUNT_WIDTH){1'b0}}, axis_rd_data_count_i} : AXIS_DATA_COUNT;

    axis_interconnect_v1_1_util_aclken_converter_wrapper #( 
      .C_TDATA_WIDTH         ( C_AXIS_TDATA_WIDTH     ) ,
      .C_TID_WIDTH           ( C_AXIS_TID_WIDTH       ) ,
      .C_TDEST_WIDTH         ( C_AXIS_TDEST_WIDTH     ) ,
      .C_TUSER_WIDTH         ( C_AXIS_TUSER_WIDTH     ) ,
      .C_S_ACLKEN_CAN_TOGGLE ( LP_S_ACLKEN_CAN_TOGGLE ) ,
      .C_M_ACLKEN_CAN_TOGGLE ( 0                      ) 
    )
    s_util_aclken_converter_wrapper_0 ( 
      .ACLK     ( S_AXIS_ACLK    ) ,
      .ARESETN  ( S_AXIS_ARESETN ) ,
      .S_ACLKEN ( S_AXIS_ACLKEN  ) ,
      .S_VALID  ( S_AXIS_TVALID  ) ,
      .S_READY  ( S_AXIS_TREADY  ) ,
      .S_TDATA  ( S_AXIS_TDATA   ) ,
      .S_TSTRB  ( S_AXIS_TSTRB   ) ,
      .S_TKEEP  ( S_AXIS_TKEEP   ) ,
      .S_TLAST  ( S_AXIS_TLAST   ) ,
      .S_TID    ( S_AXIS_TID     ) ,
      .S_TDEST  ( S_AXIS_TDEST   ) ,
      .S_TUSER  ( S_AXIS_TUSER   ) ,
      .M_ACLKEN ( M_AXIS_ACLKEN  ) ,
      .M_VALID  ( d1_tvalid      ) ,
      .M_READY  ( d1_tready      ) ,
      .M_TDATA  ( d1_tdata       ) ,
      .M_TSTRB  ( d1_tstrb       ) ,
      .M_TKEEP  ( d1_tkeep       ) ,
      .M_TLAST  ( d1_tlast       ) ,
      .M_TID    ( d1_tid         ) ,
      .M_TDEST  ( d1_tdest       ) ,
      .M_TUSER  ( d1_tuser       ) 
    );
    // synthesis xilinx_generatecore
    fifo_generator_v12_0 #(
      .C_ADD_NGC_CONSTRAINT                ( 0                          ) ,
      .C_APPLICATION_TYPE_AXIS             ( P_APPLICATION_TYPE_AXIS    ) ,
      .C_APPLICATION_TYPE_RACH             ( 0                          ) ,
      .C_APPLICATION_TYPE_RDCH             ( 0                          ) ,
      .C_APPLICATION_TYPE_WACH             ( 0                          ) ,
      .C_APPLICATION_TYPE_WDCH             ( 0                          ) ,
      .C_APPLICATION_TYPE_WRCH             ( 0                          ) ,
      .C_AXI_ADDR_WIDTH                    ( 32                         ) ,
      .C_AXI_ARUSER_WIDTH                  ( 1                          ) ,
      .C_AXI_AWUSER_WIDTH                  ( 1                          ) ,
      .C_AXI_BUSER_WIDTH                   ( 1                          ) ,
      .C_AXI_DATA_WIDTH                    ( 64                         ) ,
      .C_AXI_ID_WIDTH                      ( 4                          ) ,
      .C_AXI_LEN_WIDTH                     ( 8         ) ,
      .C_AXI_LOCK_WIDTH                    ( 2         ) ,
      .C_AXI_RUSER_WIDTH                   ( 1                          ) ,
      .C_AXIS_TDATA_WIDTH                  ( C_AXIS_TDATA_WIDTH         ) ,
      .C_AXIS_TDEST_WIDTH                  ( C_AXIS_TDEST_WIDTH         ) ,
      .C_AXIS_TID_WIDTH                    ( C_AXIS_TID_WIDTH           ) ,
      .C_AXIS_TKEEP_WIDTH                  ( C_AXIS_TDATA_WIDTH/8       ) ,
      .C_AXIS_TSTRB_WIDTH                  ( C_AXIS_TDATA_WIDTH/8       ) ,
      .C_AXIS_TUSER_WIDTH                  ( C_AXIS_TUSER_WIDTH         ) ,
      .C_AXIS_TYPE                         ( 0                          ) ,
      .C_AXI_TYPE                          ( 0                          ) ,
      .C_AXI_WUSER_WIDTH                   ( 1                          ) ,
      .C_COMMON_CLOCK                      ( P_COMMON_CLOCK             ) ,
      .C_COUNT_TYPE                        ( 0                          ) ,
      .C_DATA_COUNT_WIDTH                  ( 10                         ) ,
      .C_DEFAULT_VALUE                     ( "BlankString"              ) ,
      .C_DIN_WIDTH                         ( 18                         ) ,
      .C_DIN_WIDTH_AXIS                    ( P_AXIS_PAYLOAD_WIDTH       ) ,
      .C_DIN_WIDTH_RACH                    ( 32                         ) ,
      .C_DIN_WIDTH_RDCH                    ( 64                         ) ,
      .C_DIN_WIDTH_WACH                    ( 32                         ) ,
      .C_DIN_WIDTH_WDCH                    ( 64                         ) ,
      .C_DIN_WIDTH_WRCH                    ( 2                          ) ,
      .C_DOUT_RST_VAL                      ( "0"                        ) ,
      .C_DOUT_WIDTH                        ( 18                         ) ,
      .C_ENABLE_RLOCS                      ( 0                          ) ,
      .C_ENABLE_RST_SYNC                   ( 1                          ) ,
      .C_ERROR_INJECTION_TYPE              ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_AXIS         ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_RACH         ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_RDCH         ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_WACH         ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_WDCH         ( 0                          ) ,
      .C_ERROR_INJECTION_TYPE_WRCH         ( 0                          ) ,
      .C_FAMILY                            ( C_FAMILY                   ) ,
      .C_FULL_FLAGS_RST_VAL                ( 1                          ) ,
      .C_HAS_ALMOST_EMPTY                  ( 0                          ) ,
      .C_HAS_ALMOST_FULL                   ( 0                          ) ,
      .C_HAS_AXI_ARUSER                    ( 0                          ) ,
      .C_HAS_AXI_AWUSER                    ( 0                          ) ,
      .C_HAS_AXI_BUSER                     ( 0                          ) ,
      .C_HAS_AXI_ID                        ( 0         ) ,
      .C_HAS_AXI_RD_CHANNEL                ( 0                          ) ,
      .C_HAS_AXI_RUSER                     ( 0                          ) ,
      .C_HAS_AXIS_TDATA                    ( P_TDATA_EXISTS             ) ,
      .C_HAS_AXIS_TDEST                    ( P_TDEST_EXISTS             ) ,
      .C_HAS_AXIS_TID                      ( P_TID_EXISTS               ) ,
      .C_HAS_AXIS_TKEEP                    ( P_TKEEP_EXISTS             ) ,
      .C_HAS_AXIS_TLAST                    ( P_TLAST_EXISTS             ) ,
      .C_HAS_AXIS_TREADY                   ( P_TREADY_EXISTS            ) ,
      .C_HAS_AXIS_TSTRB                    ( P_TSTRB_EXISTS             ) ,
      .C_HAS_AXIS_TUSER                    ( P_TUSER_EXISTS             ) ,
      .C_HAS_AXI_WR_CHANNEL                ( 0                          ) ,
      .C_HAS_AXI_WUSER                     ( 0                          ) ,
      .C_HAS_BACKUP                        ( 0                          ) ,
      .C_HAS_DATA_COUNT                    ( 0                          ) ,
      .C_HAS_DATA_COUNTS_AXIS              ( 1                          ) ,
      .C_HAS_DATA_COUNTS_RACH              ( 0                          ) ,
      .C_HAS_DATA_COUNTS_RDCH              ( 0                          ) ,
      .C_HAS_DATA_COUNTS_WACH              ( 0                          ) ,
      .C_HAS_DATA_COUNTS_WDCH              ( 0                          ) ,
      .C_HAS_DATA_COUNTS_WRCH              ( 0                          ) ,
      .C_HAS_INT_CLK                       ( 0                          ) ,
      .C_HAS_MASTER_CE                     ( 0                          ) ,
      .C_HAS_MEMINIT_FILE                  ( 0                          ) ,
      .C_HAS_OVERFLOW                      ( 0                          ) ,
      .C_HAS_PROG_FLAGS_AXIS               ( 0                          ) ,
      .C_HAS_PROG_FLAGS_RACH               ( 0                          ) ,
      .C_HAS_PROG_FLAGS_RDCH               ( 0                          ) ,
      .C_HAS_PROG_FLAGS_WACH               ( 0                          ) ,
      .C_HAS_PROG_FLAGS_WDCH               ( 0                          ) ,
      .C_HAS_PROG_FLAGS_WRCH               ( 0                          ) ,
      .C_HAS_RD_DATA_COUNT                 ( 0                          ) ,
      .C_HAS_RD_RST                        ( 0                          ) ,
      .C_HAS_RST                           ( 1                          ) ,
      .C_HAS_SLAVE_CE                      ( 0                          ) ,
      .C_HAS_SRST                          ( 0                          ) ,
      .C_HAS_UNDERFLOW                     ( 0                          ) ,
      .C_HAS_VALID                         ( 0                          ) ,
      .C_HAS_WR_ACK                        ( 0                          ) ,
      .C_HAS_WR_DATA_COUNT                 ( 0                          ) ,
      .C_HAS_WR_RST                        ( 0                          ) ,
      .C_IMPLEMENTATION_TYPE               ( 0                          ) ,
      .C_IMPLEMENTATION_TYPE_AXIS          ( P_IMPLEMENTATION_TYPE_AXIS ) ,
      .C_IMPLEMENTATION_TYPE_RACH          ( 2                          ) ,
      .C_IMPLEMENTATION_TYPE_RDCH          ( 1                          ) ,
      .C_IMPLEMENTATION_TYPE_WACH          ( 2                          ) ,
      .C_IMPLEMENTATION_TYPE_WDCH          ( 1                          ) ,
      .C_IMPLEMENTATION_TYPE_WRCH          ( 2                          ) ,
      .C_INIT_WR_PNTR_VAL                  ( 0                          ) ,
      .C_INTERFACE_TYPE                    ( 1                          ) ,
      .C_MEMORY_TYPE                       ( 1                          ) ,
      .C_MIF_FILE_NAME                     ( "BlankString"              ) ,
      .C_MSGON_VAL                         ( P_MSGON_VAL                ) ,
      .C_OPTIMIZATION_MODE                 ( 0                          ) ,
      .C_OVERFLOW_LOW                      ( 0                          ) ,
      .C_POWER_SAVING_MODE                 ( 0         ) ,
      .C_PRELOAD_LATENCY                   ( 1                          ) ,
      .C_PRELOAD_REGS                      ( 0                          ) ,
      .C_PRIM_FIFO_TYPE                    ( "4kx4"                     ) ,
      .C_PRIM_FIFO_TYPE_AXIS               ( "512x36"  ) ,
      .C_PRIM_FIFO_TYPE_RACH               ( "512x36"  ) ,
      .C_PRIM_FIFO_TYPE_RDCH               ( "512x36"  ) ,
      .C_PRIM_FIFO_TYPE_WACH               ( "512x36"  ) ,
      .C_PRIM_FIFO_TYPE_WDCH               ( "512x36"  ) ,
      .C_PRIM_FIFO_TYPE_WRCH               ( "512x36"  ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL      ( 2                          ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_AXIS ( C_FIFO_DEPTH - 2           ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_RACH ( 14                         ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_RDCH ( 1022                       ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_WACH ( 14                         ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_WDCH ( 1022                       ) ,
      .C_PROG_EMPTY_THRESH_ASSERT_VAL_WRCH ( 14                         ) ,
      .C_PROG_EMPTY_THRESH_NEGATE_VAL      ( 3                          ) ,
      .C_PROG_EMPTY_TYPE                   ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_AXIS              ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_RACH              ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_RDCH              ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_WACH              ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_WDCH              ( 0                          ) ,
      .C_PROG_EMPTY_TYPE_WRCH              ( 0                          ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL       ( 1022                       ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_AXIS  ( C_FIFO_DEPTH - 1           ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_RACH  ( 15                         ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_RDCH  ( 1023                       ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_WACH  ( 15                         ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_WDCH  ( 1023                       ) ,
      .C_PROG_FULL_THRESH_ASSERT_VAL_WRCH  ( 15                         ) ,
      .C_PROG_FULL_THRESH_NEGATE_VAL       ( 1021                       ) ,
      .C_PROG_FULL_TYPE                    ( 0                          ) ,
      .C_PROG_FULL_TYPE_AXIS               ( 0                          ) ,
      .C_PROG_FULL_TYPE_RACH               ( 0                          ) ,
      .C_PROG_FULL_TYPE_RDCH               ( 0                          ) ,
      .C_PROG_FULL_TYPE_WACH               ( 0                          ) ,
      .C_PROG_FULL_TYPE_WDCH               ( 0                          ) ,
      .C_PROG_FULL_TYPE_WRCH               ( 0                          ) ,
      .C_RACH_TYPE                         ( 0                          ) ,
      .C_RDCH_TYPE                         ( 0                          ) ,
      .C_RD_DATA_COUNT_WIDTH               ( 10                         ) ,
      .C_RD_DEPTH                          ( 1024                       ) ,
      .C_RD_FREQ                           ( 1                          ) ,
      .C_RD_PNTR_WIDTH                     ( 10                         ) ,
      .C_REG_SLICE_MODE_AXIS               ( 0                          ) ,
      .C_REG_SLICE_MODE_RACH               ( 0                          ) ,
      .C_REG_SLICE_MODE_RDCH               ( 0                          ) ,
      .C_REG_SLICE_MODE_WACH               ( 0                          ) ,
      .C_REG_SLICE_MODE_WDCH               ( 0                          ) ,
      .C_REG_SLICE_MODE_WRCH               ( 0                          ) ,
      .C_SYNCHRONIZER_STAGE                ( C_SYNCHRONIZER_STAGE       ) ,
      .C_UNDERFLOW_LOW                     ( 0                          ) ,
      .C_USE_COMMON_OVERFLOW               ( 0                          ) ,
      .C_USE_COMMON_UNDERFLOW              ( 0                          ) ,
      .C_USE_DEFAULT_SETTINGS              ( 0                          ) ,
      .C_USE_DOUT_RST                      ( 1                          ) ,
      .C_USE_ECC                           ( 0                          ) ,
      .C_USE_ECC_AXIS                      ( 0                          ) ,
      .C_USE_ECC_RACH                      ( 0                          ) ,
      .C_USE_ECC_RDCH                      ( 0                          ) ,
      .C_USE_ECC_WACH                      ( 0                          ) ,
      .C_USE_ECC_WDCH                      ( 0                          ) ,
      .C_USE_ECC_WRCH                      ( 0                          ) ,
      .C_USE_EMBEDDED_REG                  ( 0                          ) ,
      .C_USE_FIFO16_FLAGS                  ( 0                          ) ,
      .C_USE_FWFT_DATA_COUNT               ( 0                          ) ,
      .C_USE_PIPELINE_REG                  ( 0         ) ,
      .C_VALID_LOW                         ( 0                          ) ,
      .C_WACH_TYPE                         ( 0                          ) ,
      .C_WDCH_TYPE                         ( 0                          ) ,
      .C_WR_ACK_LOW                        ( 0                          ) ,
      .C_WRCH_TYPE                         ( 0                          ) ,
      .C_WR_DATA_COUNT_WIDTH               ( 10                         ) ,
      .C_WR_DEPTH                          ( 1024                       ) ,
      .C_WR_DEPTH_AXIS                     ( C_FIFO_DEPTH               ) ,
      .C_WR_DEPTH_RACH                     ( 16                         ) ,
      .C_WR_DEPTH_RDCH                     ( 1024                       ) ,
      .C_WR_DEPTH_WACH                     ( 16                         ) ,
      .C_WR_DEPTH_WDCH                     ( 1024                       ) ,
      .C_WR_DEPTH_WRCH                     ( 16                         ) ,
      .C_WR_FREQ                           ( 1                          ) ,
      .C_WR_PNTR_WIDTH                     ( 10                         ) ,
      .C_WR_PNTR_WIDTH_AXIS                ( P_WR_PNTR_WIDTH            ) ,
      .C_WR_PNTR_WIDTH_RACH                ( 4                          ) ,
      .C_WR_PNTR_WIDTH_RDCH                ( 10                         ) ,
      .C_WR_PNTR_WIDTH_WACH                ( 4                          ) ,
      .C_WR_PNTR_WIDTH_WDCH                ( 10                         ) ,
      .C_WR_PNTR_WIDTH_WRCH                ( 4                          ) ,
      .C_WR_RESPONSE_LATENCY               ( 1                          )
    )
    fifo_generator_inst (
      .almost_empty             ( ALMOST_EMPTY             ) ,
      .almost_full              ( ALMOST_FULL              ) ,
      .axi_ar_data_count        ( AXI_AR_DATA_COUNT        ) ,
      .axi_ar_dbiterr           ( AXI_AR_DBITERR           ) ,
      .axi_ar_injectdbiterr     ( AXI_AR_INJECTDBITERR     ) ,
      .axi_ar_injectsbiterr     ( AXI_AR_INJECTSBITERR     ) ,
      .axi_ar_overflow          ( AXI_AR_OVERFLOW          ) ,
      .axi_ar_prog_empty        ( AXI_AR_PROG_EMPTY        ) ,
      .axi_ar_prog_empty_thresh ( AXI_AR_PROG_EMPTY_THRESH ) ,
      .axi_ar_prog_full         ( AXI_AR_PROG_FULL         ) ,
      .axi_ar_prog_full_thresh  ( AXI_AR_PROG_FULL_THRESH  ) ,
      .axi_ar_rd_data_count     ( AXI_AR_RD_DATA_COUNT     ) ,
      .axi_ar_sbiterr           ( AXI_AR_SBITERR           ) ,
      .axi_ar_underflow         ( AXI_AR_UNDERFLOW         ) ,
      .axi_ar_wr_data_count     ( AXI_AR_WR_DATA_COUNT     ) ,
      .axi_aw_data_count        ( AXI_AW_DATA_COUNT        ) ,
      .axi_aw_dbiterr           ( AXI_AW_DBITERR           ) ,
      .axi_aw_injectdbiterr     ( AXI_AW_INJECTDBITERR     ) ,
      .axi_aw_injectsbiterr     ( AXI_AW_INJECTSBITERR     ) ,
      .axi_aw_overflow          ( AXI_AW_OVERFLOW          ) ,
      .axi_aw_prog_empty        ( AXI_AW_PROG_EMPTY        ) ,
      .axi_aw_prog_empty_thresh ( AXI_AW_PROG_EMPTY_THRESH ) ,
      .axi_aw_prog_full         ( AXI_AW_PROG_FULL         ) ,
      .axi_aw_prog_full_thresh  ( AXI_AW_PROG_FULL_THRESH  ) ,
      .axi_aw_rd_data_count     ( AXI_AW_RD_DATA_COUNT     ) ,
      .axi_aw_sbiterr           ( AXI_AW_SBITERR           ) ,
      .axi_aw_underflow         ( AXI_AW_UNDERFLOW         ) ,
      .axi_aw_wr_data_count     ( AXI_AW_WR_DATA_COUNT     ) ,
      .axi_b_data_count         ( AXI_B_DATA_COUNT         ) ,
      .axi_b_dbiterr            ( AXI_B_DBITERR            ) ,
      .axi_b_injectdbiterr      ( AXI_B_INJECTDBITERR      ) ,
      .axi_b_injectsbiterr      ( AXI_B_INJECTSBITERR      ) ,
      .axi_b_overflow           ( AXI_B_OVERFLOW           ) ,
      .axi_b_prog_empty         ( AXI_B_PROG_EMPTY         ) ,
      .axi_b_prog_empty_thresh  ( AXI_B_PROG_EMPTY_THRESH  ) ,
      .axi_b_prog_full          ( AXI_B_PROG_FULL          ) ,
      .axi_b_prog_full_thresh   ( AXI_B_PROG_FULL_THRESH   ) ,
      .axi_b_rd_data_count      ( AXI_B_RD_DATA_COUNT      ) ,
      .axi_b_sbiterr            ( AXI_B_SBITERR            ) ,
      .axi_b_underflow          ( AXI_B_UNDERFLOW          ) ,
      .axi_b_wr_data_count      ( AXI_B_WR_DATA_COUNT      ) ,
      .axi_r_data_count         ( AXI_R_DATA_COUNT         ) ,
      .axi_r_dbiterr            ( AXI_R_DBITERR            ) ,
      .axi_r_injectdbiterr      ( AXI_R_INJECTDBITERR      ) ,
      .axi_r_injectsbiterr      ( AXI_R_INJECTSBITERR      ) ,
      .axi_r_overflow           ( AXI_R_OVERFLOW           ) ,
      .axi_r_prog_empty         ( AXI_R_PROG_EMPTY         ) ,
      .axi_r_prog_empty_thresh  ( AXI_R_PROG_EMPTY_THRESH  ) ,
      .axi_r_prog_full          ( AXI_R_PROG_FULL          ) ,
      .axi_r_prog_full_thresh   ( AXI_R_PROG_FULL_THRESH   ) ,
      .axi_r_rd_data_count      ( AXI_R_RD_DATA_COUNT      ) ,
      .axi_r_sbiterr            ( AXI_R_SBITERR            ) ,
      .axi_r_underflow          ( AXI_R_UNDERFLOW          ) ,
      .axi_r_wr_data_count      ( AXI_R_WR_DATA_COUNT      ) ,
      .axis_data_count          ( axis_data_count_i        ) ,
      .axis_dbiterr             ( AXIS_DBITERR             ) ,
      .axis_injectdbiterr       ( AXIS_INJECTDBITERR       ) ,
      .axis_injectsbiterr       ( AXIS_INJECTSBITERR       ) ,
      .axis_overflow            ( AXIS_OVERFLOW            ) ,
      .axis_prog_empty          ( AXIS_PROG_EMPTY          ) ,
      .axis_prog_empty_thresh   ( AXIS_PROG_EMPTY_THRESH   ) ,
      .axis_prog_full           ( AXIS_PROG_FULL           ) ,
      .axis_prog_full_thresh    ( AXIS_PROG_FULL_THRESH    ) ,
      .axis_rd_data_count       ( axis_rd_data_count_i     ) ,
      .axis_sbiterr             ( AXIS_SBITERR             ) ,
      .axis_underflow           ( AXIS_UNDERFLOW           ) ,
      .axis_wr_data_count       ( axis_wr_data_count_i     ) ,
      .axi_w_data_count         ( AXI_W_DATA_COUNT         ) ,
      .axi_w_dbiterr            ( AXI_W_DBITERR            ) ,
      .axi_w_injectdbiterr      ( AXI_W_INJECTDBITERR      ) ,
      .axi_w_injectsbiterr      ( AXI_W_INJECTSBITERR      ) ,
      .axi_w_overflow           ( AXI_W_OVERFLOW           ) ,
      .axi_w_prog_empty         ( AXI_W_PROG_EMPTY         ) ,
      .axi_w_prog_empty_thresh  ( AXI_W_PROG_EMPTY_THRESH  ) ,
      .axi_w_prog_full          ( AXI_W_PROG_FULL          ) ,
      .axi_w_prog_full_thresh   ( AXI_W_PROG_FULL_THRESH   ) ,
      .axi_w_rd_data_count      ( AXI_W_RD_DATA_COUNT      ) ,
      .axi_w_sbiterr            ( AXI_W_SBITERR            ) ,
      .axi_w_underflow          ( AXI_W_UNDERFLOW          ) ,
      .axi_w_wr_data_count      ( AXI_W_WR_DATA_COUNT      ) ,
      .backup                   ( BACKUP                   ) ,
      .backup_marker            ( BACKUP_MARKER            ) ,
      .clk                      ( CLK                      ) ,
      .data_count               ( DATA_COUNT               ) ,
      .dbiterr                  ( DBITERR                  ) ,
      .din                      ( DIN                      ) ,
      .dout                     ( DOUT                     ) ,
      .empty                    ( EMPTY                    ) ,
      .full                     ( FULL                     ) ,
      .injectdbiterr            ( INJECTDBITERR            ) ,
      .injectsbiterr            ( INJECTSBITERR            ) ,
      .int_clk                  ( INT_CLK                  ) ,
      .m_aclk_en                ( M_ACLK_EN                ) ,
      .m_aclk                   ( M_AXIS_ACLK              ) ,
      .m_axi_araddr             ( M_AXI_ARADDR             ) ,
      .m_axi_arburst            ( M_AXI_ARBURST            ) ,
      .m_axi_arcache            ( M_AXI_ARCACHE            ) ,
      .m_axi_arid               ( M_AXI_ARID               ) ,
      .m_axi_arlen              ( M_AXI_ARLEN              ) ,
      .m_axi_arlock             ( M_AXI_ARLOCK             ) ,
      .m_axi_arprot             ( M_AXI_ARPROT             ) ,
      .m_axi_arqos              ( M_AXI_ARQOS              ) ,
      .m_axi_arready            ( M_AXI_ARREADY            ) ,
      .m_axi_arregion           ( M_AXI_ARREGION           ) ,
      .m_axi_arsize             ( M_AXI_ARSIZE             ) ,
      .m_axi_aruser             ( M_AXI_ARUSER             ) ,
      .m_axi_arvalid            ( M_AXI_ARVALID            ) ,
      .m_axi_awaddr             ( M_AXI_AWADDR             ) ,
      .m_axi_awburst            ( M_AXI_AWBURST            ) ,
      .m_axi_awcache            ( M_AXI_AWCACHE            ) ,
      .m_axi_awid               ( M_AXI_AWID               ) ,
      .m_axi_awlen              ( M_AXI_AWLEN              ) ,
      .m_axi_awlock             ( M_AXI_AWLOCK             ) ,
      .m_axi_awprot             ( M_AXI_AWPROT             ) ,
      .m_axi_awqos              ( M_AXI_AWQOS              ) ,
      .m_axi_awready            ( M_AXI_AWREADY            ) ,
      .m_axi_awregion           ( M_AXI_AWREGION           ) ,
      .m_axi_awsize             ( M_AXI_AWSIZE             ) ,
      .m_axi_awuser             ( M_AXI_AWUSER             ) ,
      .m_axi_awvalid            ( M_AXI_AWVALID            ) ,
      .m_axi_bid                ( M_AXI_BID                ) ,
      .m_axi_bready             ( M_AXI_BREADY             ) ,
      .m_axi_bresp              ( M_AXI_BRESP              ) ,
      .m_axi_buser              ( M_AXI_BUSER              ) ,
      .m_axi_bvalid             ( M_AXI_BVALID             ) ,
      .m_axi_rdata              ( M_AXI_RDATA              ) ,
      .m_axi_rid                ( M_AXI_RID                ) ,
      .m_axi_rlast              ( M_AXI_RLAST              ) ,
      .m_axi_rready             ( M_AXI_RREADY             ) ,
      .m_axi_rresp              ( M_AXI_RRESP              ) ,
      .m_axi_ruser              ( M_AXI_RUSER              ) ,
      .m_axi_rvalid             ( M_AXI_RVALID             ) ,
      .m_axis_tdata             ( d2_tdata                 ) ,
      .m_axis_tdest             ( d2_tdest                 ) ,
      .m_axis_tid               ( d2_tid                   ) ,
      .m_axis_tkeep             ( d2_tkeep                 ) ,
      .m_axis_tlast             ( d2_tlast                 ) ,
      .m_axis_tready            ( d2_tready                ) ,
      .m_axis_tstrb             ( d2_tstrb                 ) ,
      .m_axis_tuser             ( d2_tuser                 ) ,
      .m_axis_tvalid            ( d2_tvalid                ) ,
      .m_axi_wdata              ( M_AXI_WDATA              ) ,
      .m_axi_wid                ( M_AXI_WID                ) ,
      .m_axi_wlast              ( M_AXI_WLAST              ) ,
      .m_axi_wready             ( M_AXI_WREADY             ) ,
      .m_axi_wstrb              ( M_AXI_WSTRB              ) ,
      .m_axi_wuser              ( M_AXI_WUSER              ) ,
      .m_axi_wvalid             ( M_AXI_WVALID             ) ,
      .overflow                 ( OVERFLOW                 ) ,
      .prog_empty               ( PROG_EMPTY               ) ,
      .prog_empty_thresh_assert ( PROG_EMPTY_THRESH_ASSERT ) ,
      .prog_empty_thresh_negate ( PROG_EMPTY_THRESH_NEGATE ) ,
      .prog_empty_thresh        ( PROG_EMPTY_THRESH        ) ,
      .prog_full                ( PROG_FULL                ) ,
      .prog_full_thresh_assert  ( PROG_FULL_THRESH_ASSERT  ) ,
      .prog_full_thresh_negate  ( PROG_FULL_THRESH_NEGATE  ) ,
      .prog_full_thresh         ( PROG_FULL_THRESH         ) ,
      .rd_clk                   ( RD_CLK                   ) ,
      .rd_data_count            ( RD_DATA_COUNT            ) ,
      .rd_en                    ( RD_EN                    ) ,
      .rd_rst_busy              ( RD_RST_BUSY              ) ,
      .rd_rst                   ( RD_RST                   ) ,
      .rst                      ( RST                      ) ,
      .s_aclk_en                ( S_ACLK_EN                ) ,
      .s_aclk                   ( S_AXIS_ACLK              ) ,
      .s_aresetn                ( s_and_m_aresetn_i        ) ,
      .s_axi_araddr             ( S_AXI_ARADDR             ) ,
      .s_axi_arburst            ( S_AXI_ARBURST            ) ,
      .s_axi_arcache            ( S_AXI_ARCACHE            ) ,
      .s_axi_arid               ( S_AXI_ARID               ) ,
      .s_axi_arlen              ( S_AXI_ARLEN              ) ,
      .s_axi_arlock             ( S_AXI_ARLOCK             ) ,
      .s_axi_arprot             ( S_AXI_ARPROT             ) ,
      .s_axi_arqos              ( S_AXI_ARQOS              ) ,
      .s_axi_arready            ( S_AXI_ARREADY            ) ,
      .s_axi_arregion           ( S_AXI_ARREGION           ) ,
      .s_axi_arsize             ( S_AXI_ARSIZE             ) ,
      .s_axi_aruser             ( S_AXI_ARUSER             ) ,
      .s_axi_arvalid            ( S_AXI_ARVALID            ) ,
      .s_axi_awaddr             ( S_AXI_AWADDR             ) ,
      .s_axi_awburst            ( S_AXI_AWBURST            ) ,
      .s_axi_awcache            ( S_AXI_AWCACHE            ) ,
      .s_axi_awid               ( S_AXI_AWID               ) ,
      .s_axi_awlen              ( S_AXI_AWLEN              ) ,
      .s_axi_awlock             ( S_AXI_AWLOCK             ) ,
      .s_axi_awprot             ( S_AXI_AWPROT             ) ,
      .s_axi_awqos              ( S_AXI_AWQOS              ) ,
      .s_axi_awready            ( S_AXI_AWREADY            ) ,
      .s_axi_awregion           ( S_AXI_AWREGION           ) ,
      .s_axi_awsize             ( S_AXI_AWSIZE             ) ,
      .s_axi_awuser             ( S_AXI_AWUSER             ) ,
      .s_axi_awvalid            ( S_AXI_AWVALID            ) ,
      .s_axi_bid                ( S_AXI_BID                ) ,
      .s_axi_bready             ( S_AXI_BREADY             ) ,
      .s_axi_bresp              ( S_AXI_BRESP              ) ,
      .s_axi_buser              ( S_AXI_BUSER              ) ,
      .s_axi_bvalid             ( S_AXI_BVALID             ) ,
      .s_axi_rdata              ( S_AXI_RDATA              ) ,
      .s_axi_rid                ( S_AXI_RID                ) ,
      .s_axi_rlast              ( S_AXI_RLAST              ) ,
      .s_axi_rready             ( S_AXI_RREADY             ) ,
      .s_axi_rresp              ( S_AXI_RRESP              ) ,
      .s_axi_ruser              ( S_AXI_RUSER              ) ,
      .s_axi_rvalid             ( S_AXI_RVALID             ) ,
      .s_axis_tdata             ( d1_tdata                 ) ,
      .s_axis_tdest             ( d1_tdest                 ) ,
      .s_axis_tid               ( d1_tid                   ) ,
      .s_axis_tkeep             ( d1_tkeep                 ) ,
      .s_axis_tlast             ( d1_tlast                 ) ,
      .s_axis_tready            ( d1_tready                ) ,
      .s_axis_tstrb             ( d1_tstrb                 ) ,
      .s_axis_tuser             ( d1_tuser                 ) ,
      .s_axis_tvalid            ( d1_tvalid                ) ,
      .s_axi_wdata              ( S_AXI_WDATA              ) ,
      .s_axi_wid                ( S_AXI_WID                ) ,
      .s_axi_wlast              ( S_AXI_WLAST              ) ,
      .s_axi_wready             ( S_AXI_WREADY             ) ,
      .s_axi_wstrb              ( S_AXI_WSTRB              ) ,
      .s_axi_wuser              ( S_AXI_WUSER              ) ,
      .s_axi_wvalid             ( S_AXI_WVALID             ) ,
      .sbiterr                  ( SBITERR                  ) ,
      .sleep                    ( SLEEP                    ) ,
      .srst                     ( SRST                     ) ,
      .underflow                ( UNDERFLOW                ) ,
      .valid                    ( VALID                    ) ,
      .wr_ack                   ( WR_ACK                   ) ,
      .wr_clk                   ( WR_CLK                   ) ,
      .wr_data_count            ( WR_DATA_COUNT            ) ,
      .wr_en                    ( WR_EN                    ) ,
      .wr_rst_busy              ( WR_RST_BUSY              ) ,
      .wr_rst                   ( WR_RST                   ) 
    );

    axis_interconnect_v1_1_util_aclken_converter_wrapper #( 
      .C_TDATA_WIDTH         ( C_AXIS_TDATA_WIDTH     ) ,
      .C_TID_WIDTH           ( C_AXIS_TID_WIDTH       ) ,
      .C_TDEST_WIDTH         ( C_AXIS_TDEST_WIDTH     ) ,
      .C_TUSER_WIDTH         ( C_AXIS_TUSER_WIDTH     ) ,
      .C_S_ACLKEN_CAN_TOGGLE (                      0 ) ,
      .C_M_ACLKEN_CAN_TOGGLE ( LP_M_ACLKEN_CAN_TOGGLE )
    )
    m_util_aclken_converter_wrapper_0 ( 
      .ACLK     ( M_AXIS_ACLK    ) ,
      .ARESETN  ( M_AXIS_ARESETN ) ,
      .S_ACLKEN ( S_AXIS_ACLKEN  ) ,
      .S_VALID  ( d2_tvalid      ) ,
      .S_READY  ( d2_tready      ) ,
      .S_TDATA  ( d2_tdata       ) ,
      .S_TSTRB  ( d2_tstrb       ) ,
      .S_TKEEP  ( d2_tkeep       ) ,
      .S_TLAST  ( d2_tlast       ) ,
      .S_TID    ( d2_tid         ) ,
      .S_TDEST  ( d2_tdest       ) ,
      .S_TUSER  ( d2_tuser       ) ,
      .M_ACLKEN ( M_AXIS_ACLKEN  ) ,
      .M_VALID  ( M_AXIS_TVALID  ) ,
      .M_READY  ( (C_AXIS_SIGNAL_SET[0] == 0) ? 1'b1 : M_AXIS_TREADY  ) ,
      .M_TDATA  ( M_AXIS_TDATA   ) ,
      .M_TSTRB  ( M_AXIS_TSTRB   ) ,
      .M_TKEEP  ( M_AXIS_TKEEP   ) ,
      .M_TLAST  ( M_AXIS_TLAST   ) ,
      .M_TID    ( M_AXIS_TID     ) ,
      .M_TDEST  ( M_AXIS_TDEST   ) ,
      .M_TUSER  ( M_AXIS_TUSER   )  
    );
  end
  else begin : gen_fifo_passthru
    assign S_AXIS_TREADY   = (C_AXIS_SIGNAL_SET[0] == 0) ? 1'b1 : M_AXIS_TREADY ;
    assign M_AXIS_TVALID   = S_AXIS_TVALID ;
    assign M_AXIS_TDATA    = S_AXIS_TDATA  ;
    assign M_AXIS_TSTRB    = S_AXIS_TSTRB  ;
    assign M_AXIS_TKEEP    = S_AXIS_TKEEP  ;
    assign M_AXIS_TLAST    = S_AXIS_TLAST  ;
    assign M_AXIS_TID      = S_AXIS_TID    ;
    assign M_AXIS_TDEST    = S_AXIS_TDEST  ;
    assign M_AXIS_TUSER    = S_AXIS_TUSER  ;
    assign AXIS_DATA_COUNT = 32'b0;
    assign AXIS_WR_DATA_COUNT = 32'b0;
    assign AXIS_RD_DATA_COUNT = 32'b0;
  end
endgenerate

endmodule // axis_data_fifo

