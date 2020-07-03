// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2014.3
// Copyright (C) 2014 Xilinx Inc. All rights reserved.
// 
// ==============================================================

`timescale 1 ns / 1 ps
module icmp_server_top (
m_axis_TVALID,
m_axis_TREADY,
m_axis_TDATA,
m_axis_TKEEP,
m_axis_TLAST,
s_axis_TVALID,
s_axis_TREADY,
s_axis_TDATA,
s_axis_TKEEP,
s_axis_TLAST,
ttlIn_TVALID,
ttlIn_TREADY,
ttlIn_TDATA,
ttlIn_TKEEP,
ttlIn_TLAST,
udpIn_TVALID,
udpIn_TREADY,
udpIn_TDATA,
udpIn_TKEEP,
udpIn_TLAST,
aresetn,
aclk,
ap_start,
ap_ready,
ap_done,
ap_idle
);

parameter RESET_ACTIVE_LOW = 1;

output m_axis_TVALID ;
input m_axis_TREADY ;
output [64 - 1:0] m_axis_TDATA ;
output [8 - 1:0] m_axis_TKEEP ;
output [1 - 1:0] m_axis_TLAST ;


input s_axis_TVALID ;
output s_axis_TREADY ;
input [64 - 1:0] s_axis_TDATA ;
input [8 - 1:0] s_axis_TKEEP ;
input [1 - 1:0] s_axis_TLAST ;


input ttlIn_TVALID ;
output ttlIn_TREADY ;
input [64 - 1:0] ttlIn_TDATA ;
input [8 - 1:0] ttlIn_TKEEP ;
input [1 - 1:0] ttlIn_TLAST ;


input udpIn_TVALID ;
output udpIn_TREADY ;
input [64 - 1:0] udpIn_TDATA ;
input [8 - 1:0] udpIn_TKEEP ;
input [1 - 1:0] udpIn_TLAST ;

input aresetn ;

input aclk ;

input ap_start ;
output ap_ready ;
output ap_done ;
output ap_idle ;


wire m_axis_TVALID;
wire m_axis_TREADY;
wire [64 - 1:0] m_axis_TDATA;
wire [8 - 1:0] m_axis_TKEEP;
wire [1 - 1:0] m_axis_TLAST;


wire s_axis_TVALID;
wire s_axis_TREADY;
wire [64 - 1:0] s_axis_TDATA;
wire [8 - 1:0] s_axis_TKEEP;
wire [1 - 1:0] s_axis_TLAST;


wire ttlIn_TVALID;
wire ttlIn_TREADY;
wire [64 - 1:0] ttlIn_TDATA;
wire [8 - 1:0] ttlIn_TKEEP;
wire [1 - 1:0] ttlIn_TLAST;


wire udpIn_TVALID;
wire udpIn_TREADY;
wire [64 - 1:0] udpIn_TDATA;
wire [8 - 1:0] udpIn_TKEEP;
wire [1 - 1:0] udpIn_TLAST;

wire aresetn;


wire [64 - 1:0] sig_icmp_server_dataOut_V_data_V_din;
wire sig_icmp_server_dataOut_V_data_V_full_n;
wire sig_icmp_server_dataOut_V_data_V_write;
wire [8 - 1:0] sig_icmp_server_dataOut_V_keep_V_din;
wire sig_icmp_server_dataOut_V_keep_V_full_n;
wire sig_icmp_server_dataOut_V_keep_V_write;
wire [1 - 1:0] sig_icmp_server_dataOut_V_last_V_din;
wire sig_icmp_server_dataOut_V_last_V_full_n;
wire sig_icmp_server_dataOut_V_last_V_write;

wire [64 - 1:0] sig_icmp_server_dataIn_V_data_V_dout;
wire sig_icmp_server_dataIn_V_data_V_empty_n;
wire sig_icmp_server_dataIn_V_data_V_read;
wire [8 - 1:0] sig_icmp_server_dataIn_V_keep_V_dout;
wire sig_icmp_server_dataIn_V_keep_V_empty_n;
wire sig_icmp_server_dataIn_V_keep_V_read;
wire [1 - 1:0] sig_icmp_server_dataIn_V_last_V_dout;
wire sig_icmp_server_dataIn_V_last_V_empty_n;
wire sig_icmp_server_dataIn_V_last_V_read;

wire [64 - 1:0] sig_icmp_server_ttlIn_V_data_V_dout;
wire sig_icmp_server_ttlIn_V_data_V_empty_n;
wire sig_icmp_server_ttlIn_V_data_V_read;
wire [8 - 1:0] sig_icmp_server_ttlIn_V_keep_V_dout;
wire sig_icmp_server_ttlIn_V_keep_V_empty_n;
wire sig_icmp_server_ttlIn_V_keep_V_read;
wire [1 - 1:0] sig_icmp_server_ttlIn_V_last_V_dout;
wire sig_icmp_server_ttlIn_V_last_V_empty_n;
wire sig_icmp_server_ttlIn_V_last_V_read;

wire [64 - 1:0] sig_icmp_server_udpIn_V_data_V_dout;
wire sig_icmp_server_udpIn_V_data_V_empty_n;
wire sig_icmp_server_udpIn_V_data_V_read;
wire [8 - 1:0] sig_icmp_server_udpIn_V_keep_V_dout;
wire sig_icmp_server_udpIn_V_keep_V_empty_n;
wire sig_icmp_server_udpIn_V_keep_V_read;
wire [1 - 1:0] sig_icmp_server_udpIn_V_last_V_dout;
wire sig_icmp_server_udpIn_V_last_V_empty_n;
wire sig_icmp_server_udpIn_V_last_V_read;

wire sig_icmp_server_ap_rst;



icmp_server icmp_server_U(
    .dataOut_V_data_V_din(sig_icmp_server_dataOut_V_data_V_din),
    .dataOut_V_data_V_full_n(sig_icmp_server_dataOut_V_data_V_full_n),
    .dataOut_V_data_V_write(sig_icmp_server_dataOut_V_data_V_write),
    .dataOut_V_keep_V_din(sig_icmp_server_dataOut_V_keep_V_din),
    .dataOut_V_keep_V_full_n(sig_icmp_server_dataOut_V_keep_V_full_n),
    .dataOut_V_keep_V_write(sig_icmp_server_dataOut_V_keep_V_write),
    .dataOut_V_last_V_din(sig_icmp_server_dataOut_V_last_V_din),
    .dataOut_V_last_V_full_n(sig_icmp_server_dataOut_V_last_V_full_n),
    .dataOut_V_last_V_write(sig_icmp_server_dataOut_V_last_V_write),
    .dataIn_V_data_V_dout(sig_icmp_server_dataIn_V_data_V_dout),
    .dataIn_V_data_V_empty_n(sig_icmp_server_dataIn_V_data_V_empty_n),
    .dataIn_V_data_V_read(sig_icmp_server_dataIn_V_data_V_read),
    .dataIn_V_keep_V_dout(sig_icmp_server_dataIn_V_keep_V_dout),
    .dataIn_V_keep_V_empty_n(sig_icmp_server_dataIn_V_keep_V_empty_n),
    .dataIn_V_keep_V_read(sig_icmp_server_dataIn_V_keep_V_read),
    .dataIn_V_last_V_dout(sig_icmp_server_dataIn_V_last_V_dout),
    .dataIn_V_last_V_empty_n(sig_icmp_server_dataIn_V_last_V_empty_n),
    .dataIn_V_last_V_read(sig_icmp_server_dataIn_V_last_V_read),
    .ttlIn_V_data_V_dout(sig_icmp_server_ttlIn_V_data_V_dout),
    .ttlIn_V_data_V_empty_n(sig_icmp_server_ttlIn_V_data_V_empty_n),
    .ttlIn_V_data_V_read(sig_icmp_server_ttlIn_V_data_V_read),
    .ttlIn_V_keep_V_dout(sig_icmp_server_ttlIn_V_keep_V_dout),
    .ttlIn_V_keep_V_empty_n(sig_icmp_server_ttlIn_V_keep_V_empty_n),
    .ttlIn_V_keep_V_read(sig_icmp_server_ttlIn_V_keep_V_read),
    .ttlIn_V_last_V_dout(sig_icmp_server_ttlIn_V_last_V_dout),
    .ttlIn_V_last_V_empty_n(sig_icmp_server_ttlIn_V_last_V_empty_n),
    .ttlIn_V_last_V_read(sig_icmp_server_ttlIn_V_last_V_read),
    .udpIn_V_data_V_dout(sig_icmp_server_udpIn_V_data_V_dout),
    .udpIn_V_data_V_empty_n(sig_icmp_server_udpIn_V_data_V_empty_n),
    .udpIn_V_data_V_read(sig_icmp_server_udpIn_V_data_V_read),
    .udpIn_V_keep_V_dout(sig_icmp_server_udpIn_V_keep_V_dout),
    .udpIn_V_keep_V_empty_n(sig_icmp_server_udpIn_V_keep_V_empty_n),
    .udpIn_V_keep_V_read(sig_icmp_server_udpIn_V_keep_V_read),
    .udpIn_V_last_V_dout(sig_icmp_server_udpIn_V_last_V_dout),
    .udpIn_V_last_V_empty_n(sig_icmp_server_udpIn_V_last_V_empty_n),
    .udpIn_V_last_V_read(sig_icmp_server_udpIn_V_last_V_read),
    .ap_rst(sig_icmp_server_ap_rst),
    .ap_clk(aclk),
    .ap_start(ap_start),
    .ap_ready(ap_ready),
    .ap_done(ap_done),
    .ap_idle(ap_idle)
);

icmp_server_m_axis_if icmp_server_m_axis_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataOut_V_data_V_din(sig_icmp_server_dataOut_V_data_V_din),
    .dataOut_V_data_V_full_n(sig_icmp_server_dataOut_V_data_V_full_n),
    .dataOut_V_data_V_write(sig_icmp_server_dataOut_V_data_V_write),
    .dataOut_V_keep_V_din(sig_icmp_server_dataOut_V_keep_V_din),
    .dataOut_V_keep_V_full_n(sig_icmp_server_dataOut_V_keep_V_full_n),
    .dataOut_V_keep_V_write(sig_icmp_server_dataOut_V_keep_V_write),
    .dataOut_V_last_V_din(sig_icmp_server_dataOut_V_last_V_din),
    .dataOut_V_last_V_full_n(sig_icmp_server_dataOut_V_last_V_full_n),
    .dataOut_V_last_V_write(sig_icmp_server_dataOut_V_last_V_write),
    .TVALID(m_axis_TVALID),
    .TREADY(m_axis_TREADY),
    .TDATA(m_axis_TDATA),
    .TKEEP(m_axis_TKEEP),
    .TLAST(m_axis_TLAST));

icmp_server_s_axis_if icmp_server_s_axis_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataIn_V_data_V_dout(sig_icmp_server_dataIn_V_data_V_dout),
    .dataIn_V_data_V_empty_n(sig_icmp_server_dataIn_V_data_V_empty_n),
    .dataIn_V_data_V_read(sig_icmp_server_dataIn_V_data_V_read),
    .dataIn_V_keep_V_dout(sig_icmp_server_dataIn_V_keep_V_dout),
    .dataIn_V_keep_V_empty_n(sig_icmp_server_dataIn_V_keep_V_empty_n),
    .dataIn_V_keep_V_read(sig_icmp_server_dataIn_V_keep_V_read),
    .dataIn_V_last_V_dout(sig_icmp_server_dataIn_V_last_V_dout),
    .dataIn_V_last_V_empty_n(sig_icmp_server_dataIn_V_last_V_empty_n),
    .dataIn_V_last_V_read(sig_icmp_server_dataIn_V_last_V_read),
    .TVALID(s_axis_TVALID),
    .TREADY(s_axis_TREADY),
    .TDATA(s_axis_TDATA),
    .TKEEP(s_axis_TKEEP),
    .TLAST(s_axis_TLAST));

icmp_server_ttlIn_if icmp_server_ttlIn_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .ttlIn_V_data_V_dout(sig_icmp_server_ttlIn_V_data_V_dout),
    .ttlIn_V_data_V_empty_n(sig_icmp_server_ttlIn_V_data_V_empty_n),
    .ttlIn_V_data_V_read(sig_icmp_server_ttlIn_V_data_V_read),
    .ttlIn_V_keep_V_dout(sig_icmp_server_ttlIn_V_keep_V_dout),
    .ttlIn_V_keep_V_empty_n(sig_icmp_server_ttlIn_V_keep_V_empty_n),
    .ttlIn_V_keep_V_read(sig_icmp_server_ttlIn_V_keep_V_read),
    .ttlIn_V_last_V_dout(sig_icmp_server_ttlIn_V_last_V_dout),
    .ttlIn_V_last_V_empty_n(sig_icmp_server_ttlIn_V_last_V_empty_n),
    .ttlIn_V_last_V_read(sig_icmp_server_ttlIn_V_last_V_read),
    .TVALID(ttlIn_TVALID),
    .TREADY(ttlIn_TREADY),
    .TDATA(ttlIn_TDATA),
    .TKEEP(ttlIn_TKEEP),
    .TLAST(ttlIn_TLAST));

icmp_server_udpIn_if icmp_server_udpIn_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .udpIn_V_data_V_dout(sig_icmp_server_udpIn_V_data_V_dout),
    .udpIn_V_data_V_empty_n(sig_icmp_server_udpIn_V_data_V_empty_n),
    .udpIn_V_data_V_read(sig_icmp_server_udpIn_V_data_V_read),
    .udpIn_V_keep_V_dout(sig_icmp_server_udpIn_V_keep_V_dout),
    .udpIn_V_keep_V_empty_n(sig_icmp_server_udpIn_V_keep_V_empty_n),
    .udpIn_V_keep_V_read(sig_icmp_server_udpIn_V_keep_V_read),
    .udpIn_V_last_V_dout(sig_icmp_server_udpIn_V_last_V_dout),
    .udpIn_V_last_V_empty_n(sig_icmp_server_udpIn_V_last_V_empty_n),
    .udpIn_V_last_V_read(sig_icmp_server_udpIn_V_last_V_read),
    .TVALID(udpIn_TVALID),
    .TREADY(udpIn_TREADY),
    .TDATA(udpIn_TDATA),
    .TKEEP(udpIn_TKEEP),
    .TLAST(udpIn_TLAST));

icmp_server_ap_rst_if #(
    .RESET_ACTIVE_LOW(RESET_ACTIVE_LOW))
ap_rst_if_U(
    .dout(sig_icmp_server_ap_rst),
    .din(aresetn));

endmodule