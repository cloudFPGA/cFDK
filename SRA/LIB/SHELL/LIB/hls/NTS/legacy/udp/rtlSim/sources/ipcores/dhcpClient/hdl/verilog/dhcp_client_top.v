// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2014.4
// Copyright (C) 2014 Xilinx Inc. All rights reserved.
// 
// ==============================================================

`timescale 1 ns / 1 ps
module dhcp_client_top (
m_axis_open_port_TVALID,
m_axis_open_port_TREADY,
m_axis_open_port_TDATA,
m_axis_tx_data_TVALID,
m_axis_tx_data_TREADY,
m_axis_tx_data_TDATA,
m_axis_tx_data_TKEEP,
m_axis_tx_data_TLAST,
m_axis_tx_length_TVALID,
m_axis_tx_length_TREADY,
m_axis_tx_length_TDATA,
m_axis_tx_metadata_TVALID,
m_axis_tx_metadata_TREADY,
m_axis_tx_metadata_TDATA,
s_axis_open_port_status_TVALID,
s_axis_open_port_status_TREADY,
s_axis_open_port_status_TDATA,
s_axis_rx_data_TVALID,
s_axis_rx_data_TREADY,
s_axis_rx_data_TDATA,
s_axis_rx_data_TKEEP,
s_axis_rx_data_TLAST,
s_axis_rx_metadata_TVALID,
s_axis_rx_metadata_TREADY,
s_axis_rx_metadata_TDATA,
aresetn,
aclk,
dhcpIpAddressOut_V,
dhcpIpAddressOut_V_ap_vld,
myMacAddress_V
);

parameter RESET_ACTIVE_LOW = 1;

output m_axis_open_port_TVALID ;
input m_axis_open_port_TREADY ;
output [16 - 1:0] m_axis_open_port_TDATA ;


output m_axis_tx_data_TVALID ;
input m_axis_tx_data_TREADY ;
output [64 - 1:0] m_axis_tx_data_TDATA ;
output [8 - 1:0] m_axis_tx_data_TKEEP ;
output [1 - 1:0] m_axis_tx_data_TLAST ;


output m_axis_tx_length_TVALID ;
input m_axis_tx_length_TREADY ;
output [16 - 1:0] m_axis_tx_length_TDATA ;


output m_axis_tx_metadata_TVALID ;
input m_axis_tx_metadata_TREADY ;
output [96 - 1:0] m_axis_tx_metadata_TDATA ;


input s_axis_open_port_status_TVALID ;
output s_axis_open_port_status_TREADY ;
input [8 - 1:0] s_axis_open_port_status_TDATA ;


input s_axis_rx_data_TVALID ;
output s_axis_rx_data_TREADY ;
input [64 - 1:0] s_axis_rx_data_TDATA ;
input [8 - 1:0] s_axis_rx_data_TKEEP ;
input [1 - 1:0] s_axis_rx_data_TLAST ;


input s_axis_rx_metadata_TVALID ;
output s_axis_rx_metadata_TREADY ;
input [96 - 1:0] s_axis_rx_metadata_TDATA ;

input aresetn ;

input aclk ;

output [32 - 1:0] dhcpIpAddressOut_V ;
output dhcpIpAddressOut_V_ap_vld ;
input [48 - 1:0] myMacAddress_V ;


wire m_axis_open_port_TVALID;
wire m_axis_open_port_TREADY;
wire [16 - 1:0] m_axis_open_port_TDATA;


wire m_axis_tx_data_TVALID;
wire m_axis_tx_data_TREADY;
wire [64 - 1:0] m_axis_tx_data_TDATA;
wire [8 - 1:0] m_axis_tx_data_TKEEP;
wire [1 - 1:0] m_axis_tx_data_TLAST;


wire m_axis_tx_length_TVALID;
wire m_axis_tx_length_TREADY;
wire [16 - 1:0] m_axis_tx_length_TDATA;


wire m_axis_tx_metadata_TVALID;
wire m_axis_tx_metadata_TREADY;
wire [96 - 1:0] m_axis_tx_metadata_TDATA;


wire s_axis_open_port_status_TVALID;
wire s_axis_open_port_status_TREADY;
wire [8 - 1:0] s_axis_open_port_status_TDATA;


wire s_axis_rx_data_TVALID;
wire s_axis_rx_data_TREADY;
wire [64 - 1:0] s_axis_rx_data_TDATA;
wire [8 - 1:0] s_axis_rx_data_TKEEP;
wire [1 - 1:0] s_axis_rx_data_TLAST;


wire s_axis_rx_metadata_TVALID;
wire s_axis_rx_metadata_TREADY;
wire [96 - 1:0] s_axis_rx_metadata_TDATA;

wire aresetn;


wire [16 - 1:0] sig_dhcp_client_openPort_V_V_din;
wire sig_dhcp_client_openPort_V_V_full_n;
wire sig_dhcp_client_openPort_V_V_write;

wire [64 - 1:0] sig_dhcp_client_dataOut_V_data_V_din;
wire sig_dhcp_client_dataOut_V_data_V_full_n;
wire sig_dhcp_client_dataOut_V_data_V_write;
wire [8 - 1:0] sig_dhcp_client_dataOut_V_keep_V_din;
wire sig_dhcp_client_dataOut_V_keep_V_full_n;
wire sig_dhcp_client_dataOut_V_keep_V_write;
wire [1 - 1:0] sig_dhcp_client_dataOut_V_last_V_din;
wire sig_dhcp_client_dataOut_V_last_V_full_n;
wire sig_dhcp_client_dataOut_V_last_V_write;

wire [16 - 1:0] sig_dhcp_client_dataOutLength_V_V_din;
wire sig_dhcp_client_dataOutLength_V_V_full_n;
wire sig_dhcp_client_dataOutLength_V_V_write;

wire [96 - 1:0] sig_dhcp_client_dataOutMeta_V_din;
wire sig_dhcp_client_dataOutMeta_V_full_n;
wire sig_dhcp_client_dataOutMeta_V_write;

wire [1 - 1:0] sig_dhcp_client_confirmPortStatus_V_dout;
wire sig_dhcp_client_confirmPortStatus_V_empty_n;
wire sig_dhcp_client_confirmPortStatus_V_read;

wire [64 - 1:0] sig_dhcp_client_dataIn_V_data_V_dout;
wire sig_dhcp_client_dataIn_V_data_V_empty_n;
wire sig_dhcp_client_dataIn_V_data_V_read;
wire [8 - 1:0] sig_dhcp_client_dataIn_V_keep_V_dout;
wire sig_dhcp_client_dataIn_V_keep_V_empty_n;
wire sig_dhcp_client_dataIn_V_keep_V_read;
wire [1 - 1:0] sig_dhcp_client_dataIn_V_last_V_dout;
wire sig_dhcp_client_dataIn_V_last_V_empty_n;
wire sig_dhcp_client_dataIn_V_last_V_read;

wire [96 - 1:0] sig_dhcp_client_dataInMeta_V_dout;
wire sig_dhcp_client_dataInMeta_V_empty_n;
wire sig_dhcp_client_dataInMeta_V_read;

wire sig_dhcp_client_ap_rst;



dhcp_client dhcp_client_U(
    .openPort_V_V_din(sig_dhcp_client_openPort_V_V_din),
    .openPort_V_V_full_n(sig_dhcp_client_openPort_V_V_full_n),
    .openPort_V_V_write(sig_dhcp_client_openPort_V_V_write),
    .dataOut_V_data_V_din(sig_dhcp_client_dataOut_V_data_V_din),
    .dataOut_V_data_V_full_n(sig_dhcp_client_dataOut_V_data_V_full_n),
    .dataOut_V_data_V_write(sig_dhcp_client_dataOut_V_data_V_write),
    .dataOut_V_keep_V_din(sig_dhcp_client_dataOut_V_keep_V_din),
    .dataOut_V_keep_V_full_n(sig_dhcp_client_dataOut_V_keep_V_full_n),
    .dataOut_V_keep_V_write(sig_dhcp_client_dataOut_V_keep_V_write),
    .dataOut_V_last_V_din(sig_dhcp_client_dataOut_V_last_V_din),
    .dataOut_V_last_V_full_n(sig_dhcp_client_dataOut_V_last_V_full_n),
    .dataOut_V_last_V_write(sig_dhcp_client_dataOut_V_last_V_write),
    .dataOutLength_V_V_din(sig_dhcp_client_dataOutLength_V_V_din),
    .dataOutLength_V_V_full_n(sig_dhcp_client_dataOutLength_V_V_full_n),
    .dataOutLength_V_V_write(sig_dhcp_client_dataOutLength_V_V_write),
    .dataOutMeta_V_din(sig_dhcp_client_dataOutMeta_V_din),
    .dataOutMeta_V_full_n(sig_dhcp_client_dataOutMeta_V_full_n),
    .dataOutMeta_V_write(sig_dhcp_client_dataOutMeta_V_write),
    .confirmPortStatus_V_dout(sig_dhcp_client_confirmPortStatus_V_dout),
    .confirmPortStatus_V_empty_n(sig_dhcp_client_confirmPortStatus_V_empty_n),
    .confirmPortStatus_V_read(sig_dhcp_client_confirmPortStatus_V_read),
    .dataIn_V_data_V_dout(sig_dhcp_client_dataIn_V_data_V_dout),
    .dataIn_V_data_V_empty_n(sig_dhcp_client_dataIn_V_data_V_empty_n),
    .dataIn_V_data_V_read(sig_dhcp_client_dataIn_V_data_V_read),
    .dataIn_V_keep_V_dout(sig_dhcp_client_dataIn_V_keep_V_dout),
    .dataIn_V_keep_V_empty_n(sig_dhcp_client_dataIn_V_keep_V_empty_n),
    .dataIn_V_keep_V_read(sig_dhcp_client_dataIn_V_keep_V_read),
    .dataIn_V_last_V_dout(sig_dhcp_client_dataIn_V_last_V_dout),
    .dataIn_V_last_V_empty_n(sig_dhcp_client_dataIn_V_last_V_empty_n),
    .dataIn_V_last_V_read(sig_dhcp_client_dataIn_V_last_V_read),
    .dataInMeta_V_dout(sig_dhcp_client_dataInMeta_V_dout),
    .dataInMeta_V_empty_n(sig_dhcp_client_dataInMeta_V_empty_n),
    .dataInMeta_V_read(sig_dhcp_client_dataInMeta_V_read),
    .ap_rst(sig_dhcp_client_ap_rst),
    .ap_clk(aclk),
    .dhcpIpAddressOut_V(dhcpIpAddressOut_V),
    .dhcpIpAddressOut_V_ap_vld(dhcpIpAddressOut_V_ap_vld),
    .myMacAddress_V(myMacAddress_V)
);

dhcp_client_m_axis_open_port_if dhcp_client_m_axis_open_port_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .openPort_V_V_din(sig_dhcp_client_openPort_V_V_din),
    .openPort_V_V_full_n(sig_dhcp_client_openPort_V_V_full_n),
    .openPort_V_V_write(sig_dhcp_client_openPort_V_V_write),
    .TVALID(m_axis_open_port_TVALID),
    .TREADY(m_axis_open_port_TREADY),
    .TDATA(m_axis_open_port_TDATA));

dhcp_client_m_axis_tx_data_if dhcp_client_m_axis_tx_data_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataOut_V_data_V_din(sig_dhcp_client_dataOut_V_data_V_din),
    .dataOut_V_data_V_full_n(sig_dhcp_client_dataOut_V_data_V_full_n),
    .dataOut_V_data_V_write(sig_dhcp_client_dataOut_V_data_V_write),
    .dataOut_V_keep_V_din(sig_dhcp_client_dataOut_V_keep_V_din),
    .dataOut_V_keep_V_full_n(sig_dhcp_client_dataOut_V_keep_V_full_n),
    .dataOut_V_keep_V_write(sig_dhcp_client_dataOut_V_keep_V_write),
    .dataOut_V_last_V_din(sig_dhcp_client_dataOut_V_last_V_din),
    .dataOut_V_last_V_full_n(sig_dhcp_client_dataOut_V_last_V_full_n),
    .dataOut_V_last_V_write(sig_dhcp_client_dataOut_V_last_V_write),
    .TVALID(m_axis_tx_data_TVALID),
    .TREADY(m_axis_tx_data_TREADY),
    .TDATA(m_axis_tx_data_TDATA),
    .TKEEP(m_axis_tx_data_TKEEP),
    .TLAST(m_axis_tx_data_TLAST));

dhcp_client_m_axis_tx_length_if dhcp_client_m_axis_tx_length_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataOutLength_V_V_din(sig_dhcp_client_dataOutLength_V_V_din),
    .dataOutLength_V_V_full_n(sig_dhcp_client_dataOutLength_V_V_full_n),
    .dataOutLength_V_V_write(sig_dhcp_client_dataOutLength_V_V_write),
    .TVALID(m_axis_tx_length_TVALID),
    .TREADY(m_axis_tx_length_TREADY),
    .TDATA(m_axis_tx_length_TDATA));

dhcp_client_m_axis_tx_metadata_if dhcp_client_m_axis_tx_metadata_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataOutMeta_V_din(sig_dhcp_client_dataOutMeta_V_din),
    .dataOutMeta_V_full_n(sig_dhcp_client_dataOutMeta_V_full_n),
    .dataOutMeta_V_write(sig_dhcp_client_dataOutMeta_V_write),
    .TVALID(m_axis_tx_metadata_TVALID),
    .TREADY(m_axis_tx_metadata_TREADY),
    .TDATA(m_axis_tx_metadata_TDATA));

dhcp_client_s_axis_open_port_status_if dhcp_client_s_axis_open_port_status_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .confirmPortStatus_V_dout(sig_dhcp_client_confirmPortStatus_V_dout),
    .confirmPortStatus_V_empty_n(sig_dhcp_client_confirmPortStatus_V_empty_n),
    .confirmPortStatus_V_read(sig_dhcp_client_confirmPortStatus_V_read),
    .TVALID(s_axis_open_port_status_TVALID),
    .TREADY(s_axis_open_port_status_TREADY),
    .TDATA(s_axis_open_port_status_TDATA));

dhcp_client_s_axis_rx_data_if dhcp_client_s_axis_rx_data_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataIn_V_data_V_dout(sig_dhcp_client_dataIn_V_data_V_dout),
    .dataIn_V_data_V_empty_n(sig_dhcp_client_dataIn_V_data_V_empty_n),
    .dataIn_V_data_V_read(sig_dhcp_client_dataIn_V_data_V_read),
    .dataIn_V_keep_V_dout(sig_dhcp_client_dataIn_V_keep_V_dout),
    .dataIn_V_keep_V_empty_n(sig_dhcp_client_dataIn_V_keep_V_empty_n),
    .dataIn_V_keep_V_read(sig_dhcp_client_dataIn_V_keep_V_read),
    .dataIn_V_last_V_dout(sig_dhcp_client_dataIn_V_last_V_dout),
    .dataIn_V_last_V_empty_n(sig_dhcp_client_dataIn_V_last_V_empty_n),
    .dataIn_V_last_V_read(sig_dhcp_client_dataIn_V_last_V_read),
    .TVALID(s_axis_rx_data_TVALID),
    .TREADY(s_axis_rx_data_TREADY),
    .TDATA(s_axis_rx_data_TDATA),
    .TKEEP(s_axis_rx_data_TKEEP),
    .TLAST(s_axis_rx_data_TLAST));

dhcp_client_s_axis_rx_metadata_if dhcp_client_s_axis_rx_metadata_if_U(
    .ACLK(aclk),
    .ARESETN(aresetn),
    .dataInMeta_V_dout(sig_dhcp_client_dataInMeta_V_dout),
    .dataInMeta_V_empty_n(sig_dhcp_client_dataInMeta_V_empty_n),
    .dataInMeta_V_read(sig_dhcp_client_dataInMeta_V_read),
    .TVALID(s_axis_rx_metadata_TVALID),
    .TREADY(s_axis_rx_metadata_TREADY),
    .TDATA(s_axis_rx_metadata_TDATA));

dhcp_client_ap_rst_if #(
    .RESET_ACTIVE_LOW(RESET_ACTIVE_LOW))
ap_rst_if_U(
    .dout(sig_dhcp_client_ap_rst),
    .din(aresetn));

endmodule