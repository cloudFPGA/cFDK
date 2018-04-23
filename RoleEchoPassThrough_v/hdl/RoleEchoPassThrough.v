// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Toplevel of the echo application in pass-through mode.
// *
// * File    : RoleEchoPassThrough.v
// *
// * Created : Apr. 2018
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2017.4 (64-bit)
// * Depends : None
// *
// * Description : This version of the role implements an echo application made
// *    of a UDP loopback and a TCP loopback connections. The role is said to be
// *    operating in "pass-through" mode because every received packet is sent
// *    back without being stored by the role.          
// * 
// * Comments:
// *
// *****************************************************************************


`timescale 1 ns / 1 ps

// *****************************************************************************
// **  MODULE - ECHO PASS-TRHOUGH ROLE FOR FMKU60
// *****************************************************************************

module RoleEchoPassThrough
(
    input  wire         s_axis_ip_rx_data_TVALID,
    output wire         s_axis_ip_rx_data_TREADY,
    input  wire  [63:0] s_axis_ip_rx_data_TDATA,
    input  wire   [7:0] s_axis_ip_rx_data_TKEEP,
    input  wire   [0:0] s_axis_ip_rx_data_TLAST,

    output wire         s_axis_ip_tx_data_TVALID,
    input  wire         s_axis_ip_tx_data_TREADY,
    output wire  [63:0] s_axis_ip_tx_data_TDATA,
    output wire   [7:0] s_axis_ip_tx_data_TKEEP,
    output wire   [0:0] s_axis_ip_tx_data_TLAST,
    input  wire         aclk,
    input  wire         aresetn
);


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
 
  // ** Wire Declarations **
  reg [63:0]           data  = 64'd0;
  reg [ 7:0]           keep  = 8'd0;
  reg                  valid = 1'b0;
  reg                  last  = 1'b0;


  // ** Routing & Logic **

  // Accept if there is no valid data or the receiver is ready
  assign s_axis_ip_rx_data_TREADY = valid ? s_axis_ip_tx_data_TREADY : 1'b1;

  // Assign our registers to outputs
  assign s_axis_ip_tx_data_TVALID = valid;
  assign s_axis_ip_tx_data_TDATA  = data;
  assign s_axis_ip_tx_data_TKEEP  = keep;
  assign s_axis_ip_tx_data_TLAST  = last;

  // Local registers
  always@(posedge(aclk)) begin
    if (~aresetn) begin
      data  <= 64'd0;
      keep  <= 8'd0;
      valid <= 1'b0;
      last  <= 1'b0;
    end else begin
      if (s_axis_ip_rx_data_TREADY) begin
        valid <= s_axis_ip_rx_data_TVALID;
        data  <= s_axis_ip_rx_data_TDATA;
        keep  <= s_axis_ip_rx_data_TKEEP;
        last  <= s_axis_ip_rx_data_TLAST;
      end   
    end
  end


endmodule

