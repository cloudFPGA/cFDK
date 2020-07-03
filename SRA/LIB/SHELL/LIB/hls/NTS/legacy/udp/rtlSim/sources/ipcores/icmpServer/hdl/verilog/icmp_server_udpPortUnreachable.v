// ==============================================================
// RTL generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2014.3
// Copyright (C) 2014 Xilinx Inc. All rights reserved.
// 
// ===========================================================

`timescale 1 ns / 1 ps 

module icmp_server_udpPortUnreachable (
        ap_clk,
        ap_rst,
        ap_start,
        ap_done,
        ap_continue,
        ap_idle,
        ap_ready,
        udpIn_V_data_V_dout,
        udpIn_V_data_V_empty_n,
        udpIn_V_data_V_read,
        udpIn_V_keep_V_dout,
        udpIn_V_keep_V_empty_n,
        udpIn_V_keep_V_read,
        udpIn_V_last_V_dout,
        udpIn_V_last_V_empty_n,
        udpIn_V_last_V_read,
        ttlIn_V_data_V_dout,
        ttlIn_V_data_V_empty_n,
        ttlIn_V_data_V_read,
        ttlIn_V_keep_V_dout,
        ttlIn_V_keep_V_empty_n,
        ttlIn_V_keep_V_read,
        ttlIn_V_last_V_dout,
        ttlIn_V_last_V_empty_n,
        ttlIn_V_last_V_read,
        udpPort2addIpHeader_data_V_dat_din,
        udpPort2addIpHeader_data_V_dat_full_n,
        udpPort2addIpHeader_data_V_dat_write,
        udpPort2addIpHeader_data_V_kee_din,
        udpPort2addIpHeader_data_V_kee_full_n,
        udpPort2addIpHeader_data_V_kee_write,
        udpPort2addIpHeader_data_V_las_din,
        udpPort2addIpHeader_data_V_las_full_n,
        udpPort2addIpHeader_data_V_las_write,
        udpPort2addIpHeader_header_V_V_din,
        udpPort2addIpHeader_header_V_V_full_n,
        udpPort2addIpHeader_header_V_V_write,
        checksumStreams_V_V_1_din,
        checksumStreams_V_V_1_full_n,
        checksumStreams_V_V_1_write
);

parameter    ap_const_logic_1 = 1'b1;
parameter    ap_const_logic_0 = 1'b0;
parameter    ap_ST_st1_fsm_0 = 1'b0;
parameter    ap_const_lv2_0 = 2'b00;
parameter    ap_const_lv3_0 = 3'b000;
parameter    ap_const_lv1_0 = 1'b0;
parameter    ap_const_lv3_3 = 3'b11;
parameter    ap_const_lv3_2 = 3'b10;
parameter    ap_const_lv3_1 = 3'b1;
parameter    ap_const_lv9_103 = 9'b100000011;
parameter    ap_const_lv9_B = 9'b1011;
parameter    ap_const_lv9_0 = 9'b000000000;
parameter    ap_const_lv8_FF = 8'b11111111;
parameter    ap_const_lv2_3 = 2'b11;
parameter    ap_const_lv2_2 = 2'b10;
parameter    ap_const_lv2_1 = 2'b1;
parameter    ap_const_lv1_1 = 1'b1;
parameter    ap_const_lv32_10 = 32'b10000;
parameter    ap_const_lv32_13 = 32'b10011;
parameter    ap_const_lv20_FFFFF = 20'b11111111111111111111;
parameter    ap_const_lv32_30 = 32'b110000;
parameter    ap_const_lv32_3F = 32'b111111;
parameter    ap_const_lv32_20 = 32'b100000;
parameter    ap_const_lv32_2F = 32'b101111;
parameter    ap_const_lv32_1F = 32'b11111;
parameter    ap_true = 1'b1;

input   ap_clk;
input   ap_rst;
input   ap_start;
output   ap_done;
input   ap_continue;
output   ap_idle;
output   ap_ready;
input  [63:0] udpIn_V_data_V_dout;
input   udpIn_V_data_V_empty_n;
output   udpIn_V_data_V_read;
input  [7:0] udpIn_V_keep_V_dout;
input   udpIn_V_keep_V_empty_n;
output   udpIn_V_keep_V_read;
input  [0:0] udpIn_V_last_V_dout;
input   udpIn_V_last_V_empty_n;
output   udpIn_V_last_V_read;
input  [63:0] ttlIn_V_data_V_dout;
input   ttlIn_V_data_V_empty_n;
output   ttlIn_V_data_V_read;
input  [7:0] ttlIn_V_keep_V_dout;
input   ttlIn_V_keep_V_empty_n;
output   ttlIn_V_keep_V_read;
input  [0:0] ttlIn_V_last_V_dout;
input   ttlIn_V_last_V_empty_n;
output   ttlIn_V_last_V_read;
output  [63:0] udpPort2addIpHeader_data_V_dat_din;
input   udpPort2addIpHeader_data_V_dat_full_n;
output   udpPort2addIpHeader_data_V_dat_write;
output  [7:0] udpPort2addIpHeader_data_V_kee_din;
input   udpPort2addIpHeader_data_V_kee_full_n;
output   udpPort2addIpHeader_data_V_kee_write;
output  [0:0] udpPort2addIpHeader_data_V_las_din;
input   udpPort2addIpHeader_data_V_las_full_n;
output   udpPort2addIpHeader_data_V_las_write;
output  [63:0] udpPort2addIpHeader_header_V_V_din;
input   udpPort2addIpHeader_header_V_V_full_n;
output   udpPort2addIpHeader_header_V_V_write;
output  [15:0] checksumStreams_V_V_1_din;
input   checksumStreams_V_V_1_full_n;
output   checksumStreams_V_V_1_write;

reg ap_done;
reg ap_idle;
reg ap_ready;
reg[63:0] udpPort2addIpHeader_data_V_dat_din;
reg[7:0] udpPort2addIpHeader_data_V_kee_din;
reg[0:0] udpPort2addIpHeader_data_V_las_din;
reg udpPort2addIpHeader_header_V_V_write;
reg checksumStreams_V_V_1_write;
reg    ap_done_reg = 1'b0;
(* fsm_encoding = "auto" *) reg   [0:0] ap_CS_fsm = 1'b0;
reg   [1:0] udpState = 2'b00;
reg   [2:0] ipWordCounter_V = 3'b000;
reg   [0:0] streamSource_V = 1'b0;
reg   [19:0] udpChecksum_V = 20'b00000000000000000000;
reg   [0:0] tmp_last_V_9_phi_fu_264_p4;
wire   [2:0] tmp_cast_fu_399_p1;
wire   [0:0] tmp_23_nbwritereq_fu_184_p3;
wire    udpIn_V_data_V0_status;
wire   [0:0] grp_fu_353_p2;
wire   [0:0] grp_nbwritereq_fu_199_p5;
wire    ttlIn_V_data_V0_status;
wire    udpPort2addIpHeader_data_V_dat1_status;
wire   [0:0] tmp_22_nbwritereq_fu_244_p3;
wire   [0:0] brmerge_fu_714_p2;
reg    ap_sig_bdd_148;
reg   [7:0] tmp_keep_V_9_phi_fu_274_p4;
reg   [63:0] p_Val2_19_phi_fu_284_p4;
reg   [0:0] tmp_last_V_10_phi_fu_294_p4;
reg   [7:0] tmp_keep_V_10_phi_fu_304_p4;
reg   [63:0] p_Val2_18_phi_fu_314_p4;
reg   [8:0] p_Val2_17_phi_fu_325_p6;
wire   [0:0] tmp_nbreadreq_fu_160_p5;
wire   [0:0] tmp_17_nbreadreq_fu_172_p5;
reg    udpIn_V_data_V0_update;
reg    ttlIn_V_data_V0_update;
reg    udpPort2addIpHeader_data_V_dat1_update;
wire   [63:0] tmp_data_V_6_fu_742_p1;
wire   [0:0] tmp_30_fu_690_p2;
wire   [2:0] tmp_31_fu_696_p2;
wire   [19:0] p_Val2_s_fu_485_p2;
wire   [19:0] tmp_26_fu_584_p2;
wire   [19:0] tmp_29_fu_678_p2;
wire   [19:0] tmp_35_cast_fu_747_p1;
wire   [0:0] grp_fu_337_p2;
wire   [0:0] grp_fu_342_p2;
wire   [0:0] grp_fu_348_p2;
wire   [15:0] tmp_95_fu_417_p1;
wire   [3:0] r_V_fu_425_p4;
wire   [16:0] r_V_cast_fu_435_p1;
wire   [16:0] tmp_27_cast_fu_421_p1;
wire   [15:0] tmp_96_fu_445_p1;
wire   [15:0] tmp_97_fu_449_p1;
wire   [15:0] tmp_98_fu_453_p2;
wire   [16:0] p_s_fu_439_p2;
wire   [0:0] tmp_99_fu_463_p3;
wire   [16:0] r_V_1_fu_471_p1;
wire   [16:0] tmp_28_cast_fu_459_p1;
wire   [16:0] p_8_fu_475_p2;
wire   [19:0] p_8_cast_fu_481_p1;
wire   [15:0] p_Result_s_fu_508_p4;
wire   [15:0] p_Result_8_fu_518_p4;
wire   [15:0] p_Result_9_fu_536_p4;
wire   [15:0] tmp_101_fu_550_p1;
wire   [16:0] tmp_39_cast_fu_532_p1;
wire   [16:0] tmp_38_cast_fu_528_p1;
wire   [16:0] tmp_s_fu_558_p2;
wire   [17:0] tmp_42_cast_fu_564_p1;
wire   [17:0] tmp_40_cast_fu_546_p1;
wire   [17:0] tmp_25_fu_568_p2;
wire   [17:0] tmp_cast3_fu_554_p1;
wire   [17:0] tmp2_fu_574_p2;
wire   [19:0] tmp2_cast_fu_580_p1;
wire   [15:0] p_Result_10_fu_602_p4;
wire   [15:0] p_Result_11_fu_612_p4;
wire   [15:0] p_Result_12_fu_630_p4;
wire   [15:0] tmp_102_fu_644_p1;
wire   [16:0] tmp_49_cast_fu_626_p1;
wire   [16:0] tmp_48_cast_fu_622_p1;
wire   [16:0] tmp_27_fu_652_p2;
wire   [17:0] tmp_52_cast_fu_658_p1;
wire   [17:0] tmp_50_cast_fu_640_p1;
wire   [17:0] tmp_28_fu_662_p2;
wire   [17:0] tmp_45_cast_fu_648_p1;
wire   [17:0] tmp1_fu_668_p2;
wire   [19:0] tmp1_cast_fu_674_p1;
wire   [9:0] tmp_data_V_6_fu_742_p0;
wire   [9:0] p_Val2_22_cast_fu_738_p1;
wire   [9:0] tmp_35_cast_fu_747_p0;
reg   [0:0] ap_NS_fsm;
reg    ap_sig_bdd_487;
reg    ap_sig_bdd_134;
reg    ap_sig_bdd_485;
reg    ap_sig_bdd_493;
reg    ap_sig_bdd_495;
reg    ap_sig_bdd_492;
reg    ap_sig_bdd_501;
reg    ap_sig_bdd_504;
reg    ap_sig_bdd_221;
reg    ap_sig_bdd_83;
reg    ap_sig_bdd_96;
reg    ap_sig_bdd_126;
reg    ap_sig_bdd_135;
reg    ap_sig_bdd_151;
reg    ap_sig_bdd_92;
reg    ap_sig_bdd_486;
reg    ap_sig_bdd_229;
reg    ap_sig_bdd_235;




/// the current state (ap_CS_fsm) of the state machine. ///
always @ (posedge ap_clk)
begin : ap_ret_ap_CS_fsm
    if (ap_rst == 1'b1) begin
        ap_CS_fsm <= ap_ST_st1_fsm_0;
    end else begin
        ap_CS_fsm <= ap_NS_fsm;
    end
end

/// ap_done_reg assign process. ///
always @ (posedge ap_clk)
begin : ap_ret_ap_done_reg
    if (ap_rst == 1'b1) begin
        ap_done_reg <= ap_const_logic_0;
    end else begin
        if ((ap_const_logic_1 == ap_continue)) begin
            ap_done_reg <= ap_const_logic_0;
        end else if (((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~ap_sig_bdd_148)) begin
            ap_done_reg <= ap_const_logic_1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_485) begin
        if (ap_sig_bdd_134) begin
            ipWordCounter_V <= ap_const_lv3_0;
        end else if (ap_sig_bdd_487) begin
            ipWordCounter_V <= tmp_31_fu_696_p2;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_221) begin
        if (~(ap_const_lv1_0 == tmp_nbreadreq_fu_160_p5)) begin
            streamSource_V <= ap_const_lv1_0;
        end else if (ap_sig_bdd_493) begin
            streamSource_V <= ap_const_lv1_1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_151) begin
        if (ap_sig_bdd_135) begin
            udpChecksum_V <= tmp_35_cast_fu_747_p1;
        end else if (ap_sig_bdd_126) begin
            udpChecksum_V <= tmp_29_fu_678_p2;
        end else if (ap_sig_bdd_96) begin
            udpChecksum_V <= tmp_26_fu_584_p2;
        end else if (ap_sig_bdd_83) begin
            udpChecksum_V <= p_Val2_s_fu_485_p2;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_151) begin
        if (ap_sig_bdd_135) begin
            udpState <= ap_const_lv2_1;
        end else if (ap_sig_bdd_235) begin
            udpState <= ap_const_lv2_2;
        end else if (ap_sig_bdd_229) begin
            udpState <= ap_const_lv2_3;
        end else if (ap_sig_bdd_83) begin
            udpState <= ap_const_lv2_0;
        end
    end
end

/// ap_done assign process. ///
always @ (ap_done_reg or ap_CS_fsm or ap_sig_bdd_148)
begin
    if (((ap_const_logic_1 == ap_done_reg) | ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~ap_sig_bdd_148))) begin
        ap_done = ap_const_logic_1;
    end else begin
        ap_done = ap_const_logic_0;
    end
end

/// ap_idle assign process. ///
always @ (ap_start or ap_CS_fsm)
begin
    if ((~(ap_const_logic_1 == ap_start) & (ap_ST_st1_fsm_0 == ap_CS_fsm))) begin
        ap_idle = ap_const_logic_1;
    end else begin
        ap_idle = ap_const_logic_0;
    end
end

/// ap_ready assign process. ///
always @ (ap_CS_fsm or ap_sig_bdd_148)
begin
    if (((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~ap_sig_bdd_148)) begin
        ap_ready = ap_const_logic_1;
    end else begin
        ap_ready = ap_const_logic_0;
    end
end

/// checksumStreams_V_V_1_write assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or tmp_23_nbwritereq_fu_184_p3 or ap_sig_bdd_148)
begin
    if (((ap_ST_st1_fsm_0 == ap_CS_fsm) & (tmp_cast_fu_399_p1 == ap_const_lv3_3) & ~(ap_const_lv1_0 == tmp_23_nbwritereq_fu_184_p3) & ~ap_sig_bdd_148)) begin
        checksumStreams_V_V_1_write = ap_const_logic_1;
    end else begin
        checksumStreams_V_V_1_write = ap_const_logic_0;
    end
end

/// p_Val2_17_phi_fu_325_p6 assign process. ///
always @ (tmp_nbreadreq_fu_160_p5 or ap_sig_bdd_493 or ap_sig_bdd_495 or ap_sig_bdd_492)
begin
    if (ap_sig_bdd_492) begin
        if (ap_sig_bdd_495) begin
            p_Val2_17_phi_fu_325_p6 = ap_const_lv9_0;
        end else if (ap_sig_bdd_493) begin
            p_Val2_17_phi_fu_325_p6 = ap_const_lv9_B;
        end else if (~(ap_const_lv1_0 == tmp_nbreadreq_fu_160_p5)) begin
            p_Val2_17_phi_fu_325_p6 = ap_const_lv9_103;
        end else begin
            p_Val2_17_phi_fu_325_p6 = 'bx;
        end
    end else begin
        p_Val2_17_phi_fu_325_p6 = 'bx;
    end
end

/// p_Val2_18_phi_fu_314_p4 assign process. ///
always @ (udpIn_V_data_V_dout or ttlIn_V_data_V_dout or streamSource_V or ap_sig_bdd_501)
begin
    if (ap_sig_bdd_501) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            p_Val2_18_phi_fu_314_p4 = udpIn_V_data_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            p_Val2_18_phi_fu_314_p4 = ttlIn_V_data_V_dout;
        end else begin
            p_Val2_18_phi_fu_314_p4 = 'bx;
        end
    end else begin
        p_Val2_18_phi_fu_314_p4 = 'bx;
    end
end

/// p_Val2_19_phi_fu_284_p4 assign process. ///
always @ (udpIn_V_data_V_dout or ttlIn_V_data_V_dout or streamSource_V or ap_sig_bdd_504)
begin
    if (ap_sig_bdd_504) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            p_Val2_19_phi_fu_284_p4 = udpIn_V_data_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            p_Val2_19_phi_fu_284_p4 = ttlIn_V_data_V_dout;
        end else begin
            p_Val2_19_phi_fu_284_p4 = 'bx;
        end
    end else begin
        p_Val2_19_phi_fu_284_p4 = 'bx;
    end
end

/// tmp_keep_V_10_phi_fu_304_p4 assign process. ///
always @ (udpIn_V_keep_V_dout or ttlIn_V_keep_V_dout or streamSource_V or ap_sig_bdd_501)
begin
    if (ap_sig_bdd_501) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            tmp_keep_V_10_phi_fu_304_p4 = udpIn_V_keep_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            tmp_keep_V_10_phi_fu_304_p4 = ttlIn_V_keep_V_dout;
        end else begin
            tmp_keep_V_10_phi_fu_304_p4 = 'bx;
        end
    end else begin
        tmp_keep_V_10_phi_fu_304_p4 = 'bx;
    end
end

/// tmp_keep_V_9_phi_fu_274_p4 assign process. ///
always @ (udpIn_V_keep_V_dout or ttlIn_V_keep_V_dout or streamSource_V or ap_sig_bdd_504)
begin
    if (ap_sig_bdd_504) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            tmp_keep_V_9_phi_fu_274_p4 = udpIn_V_keep_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            tmp_keep_V_9_phi_fu_274_p4 = ttlIn_V_keep_V_dout;
        end else begin
            tmp_keep_V_9_phi_fu_274_p4 = 'bx;
        end
    end else begin
        tmp_keep_V_9_phi_fu_274_p4 = 'bx;
    end
end

/// tmp_last_V_10_phi_fu_294_p4 assign process. ///
always @ (udpIn_V_last_V_dout or ttlIn_V_last_V_dout or streamSource_V or ap_sig_bdd_501)
begin
    if (ap_sig_bdd_501) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            tmp_last_V_10_phi_fu_294_p4 = udpIn_V_last_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            tmp_last_V_10_phi_fu_294_p4 = ttlIn_V_last_V_dout;
        end else begin
            tmp_last_V_10_phi_fu_294_p4 = 'bx;
        end
    end else begin
        tmp_last_V_10_phi_fu_294_p4 = 'bx;
    end
end

/// tmp_last_V_9_phi_fu_264_p4 assign process. ///
always @ (udpIn_V_last_V_dout or ttlIn_V_last_V_dout or streamSource_V or ap_sig_bdd_504)
begin
    if (ap_sig_bdd_504) begin
        if ((ap_const_lv1_0 == streamSource_V)) begin
            tmp_last_V_9_phi_fu_264_p4 = udpIn_V_last_V_dout;
        end else if (~(ap_const_lv1_0 == streamSource_V)) begin
            tmp_last_V_9_phi_fu_264_p4 = ttlIn_V_last_V_dout;
        end else begin
            tmp_last_V_9_phi_fu_264_p4 = 'bx;
        end
    end else begin
        tmp_last_V_9_phi_fu_264_p4 = 'bx;
    end
end

/// ttlIn_V_data_V0_update assign process. ///
always @ (ap_CS_fsm or streamSource_V or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3 or ap_sig_bdd_148)
begin
    if ((((ap_ST_st1_fsm_0 == ap_CS_fsm) & (tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & ~(ap_const_lv1_0 == streamSource_V) & ~ap_sig_bdd_148) | ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & ~(ap_const_lv1_0 == streamSource_V) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & ~ap_sig_bdd_148))) begin
        ttlIn_V_data_V0_update = ap_const_logic_1;
    end else begin
        ttlIn_V_data_V0_update = ap_const_logic_0;
    end
end

/// udpIn_V_data_V0_update assign process. ///
always @ (ap_CS_fsm or streamSource_V or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3 or ap_sig_bdd_148)
begin
    if ((((ap_ST_st1_fsm_0 == ap_CS_fsm) & (tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv1_0 == streamSource_V) & ~ap_sig_bdd_148) | ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv1_0 == streamSource_V) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & ~ap_sig_bdd_148))) begin
        udpIn_V_data_V0_update = ap_const_logic_1;
    end else begin
        udpIn_V_data_V0_update = ap_const_logic_0;
    end
end

/// udpPort2addIpHeader_data_V_dat1_update assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3 or brmerge_fu_714_p2 or ap_sig_bdd_148)
begin
    if ((((ap_ST_st1_fsm_0 == ap_CS_fsm) & (tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & ~ap_sig_bdd_148) | ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & ~ap_sig_bdd_148) | ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2) & ~ap_sig_bdd_148))) begin
        udpPort2addIpHeader_data_V_dat1_update = ap_const_logic_1;
    end else begin
        udpPort2addIpHeader_data_V_dat1_update = ap_const_logic_0;
    end
end

/// udpPort2addIpHeader_data_V_dat_din assign process. ///
always @ (p_Val2_19_phi_fu_284_p4 or p_Val2_18_phi_fu_314_p4 or tmp_data_V_6_fu_742_p1 or ap_sig_bdd_134 or ap_sig_bdd_485 or ap_sig_bdd_92 or ap_sig_bdd_486)
begin
    if (ap_sig_bdd_485) begin
        if (ap_sig_bdd_134) begin
            udpPort2addIpHeader_data_V_dat_din = tmp_data_V_6_fu_742_p1;
        end else if (ap_sig_bdd_486) begin
            udpPort2addIpHeader_data_V_dat_din = p_Val2_18_phi_fu_314_p4;
        end else if (ap_sig_bdd_92) begin
            udpPort2addIpHeader_data_V_dat_din = p_Val2_19_phi_fu_284_p4;
        end else begin
            udpPort2addIpHeader_data_V_dat_din = 'bx;
        end
    end else begin
        udpPort2addIpHeader_data_V_dat_din = 'bx;
    end
end

/// udpPort2addIpHeader_data_V_kee_din assign process. ///
always @ (tmp_keep_V_9_phi_fu_274_p4 or tmp_keep_V_10_phi_fu_304_p4 or ap_sig_bdd_134 or ap_sig_bdd_485 or ap_sig_bdd_92 or ap_sig_bdd_486)
begin
    if (ap_sig_bdd_485) begin
        if (ap_sig_bdd_134) begin
            udpPort2addIpHeader_data_V_kee_din = ap_const_lv8_FF;
        end else if (ap_sig_bdd_486) begin
            udpPort2addIpHeader_data_V_kee_din = tmp_keep_V_10_phi_fu_304_p4;
        end else if (ap_sig_bdd_92) begin
            udpPort2addIpHeader_data_V_kee_din = tmp_keep_V_9_phi_fu_274_p4;
        end else begin
            udpPort2addIpHeader_data_V_kee_din = 'bx;
        end
    end else begin
        udpPort2addIpHeader_data_V_kee_din = 'bx;
    end
end

/// udpPort2addIpHeader_data_V_las_din assign process. ///
always @ (tmp_last_V_9_phi_fu_264_p4 or tmp_last_V_10_phi_fu_294_p4 or ap_sig_bdd_134 or ap_sig_bdd_485 or ap_sig_bdd_92 or ap_sig_bdd_486)
begin
    if (ap_sig_bdd_485) begin
        if (ap_sig_bdd_134) begin
            udpPort2addIpHeader_data_V_las_din = ap_const_lv1_0;
        end else if (ap_sig_bdd_486) begin
            udpPort2addIpHeader_data_V_las_din = tmp_last_V_10_phi_fu_294_p4;
        end else if (ap_sig_bdd_92) begin
            udpPort2addIpHeader_data_V_las_din = tmp_last_V_9_phi_fu_264_p4;
        end else begin
            udpPort2addIpHeader_data_V_las_din = 'bx;
        end
    end else begin
        udpPort2addIpHeader_data_V_las_din = 'bx;
    end
end

/// udpPort2addIpHeader_header_V_V_write assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3 or ap_sig_bdd_148)
begin
    if (((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & ~ap_sig_bdd_148)) begin
        udpPort2addIpHeader_header_V_V_write = ap_const_logic_1;
    end else begin
        udpPort2addIpHeader_header_V_V_write = ap_const_logic_0;
    end
end
/// the next state (ap_NS_fsm) of the state machine. ///
always @ (ap_CS_fsm or ap_sig_bdd_148)
begin
    case (ap_CS_fsm)
        ap_ST_st1_fsm_0 : 
        begin
            ap_NS_fsm = ap_ST_st1_fsm_0;
        end
        default : 
        begin
            ap_NS_fsm = 'bx;
        end
    endcase
end


/// ap_sig_bdd_126 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3)
begin
    ap_sig_bdd_126 = (~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3));
end

/// ap_sig_bdd_134 assign process. ///
always @ (tmp_cast_fu_399_p1 or brmerge_fu_714_p2)
begin
    ap_sig_bdd_134 = ((ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2));
end

/// ap_sig_bdd_135 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_nbwritereq_fu_199_p5 or brmerge_fu_714_p2)
begin
    ap_sig_bdd_135 = (~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2));
end

/// ap_sig_bdd_148 assign process. ///
always @ (ap_start or ap_done_reg or streamSource_V or udpPort2addIpHeader_header_V_V_full_n or checksumStreams_V_V_1_full_n or tmp_cast_fu_399_p1 or tmp_23_nbwritereq_fu_184_p3 or udpIn_V_data_V0_status or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or ttlIn_V_data_V0_status or udpPort2addIpHeader_data_V_dat1_status or tmp_22_nbwritereq_fu_244_p3 or brmerge_fu_714_p2)
begin
    ap_sig_bdd_148 = (((checksumStreams_V_V_1_full_n == ap_const_logic_0) & (tmp_cast_fu_399_p1 == ap_const_lv3_3) & ~(ap_const_lv1_0 == tmp_23_nbwritereq_fu_184_p3)) | ((udpIn_V_data_V0_status == ap_const_logic_0) & (tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv1_0 == streamSource_V)) | ((tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ttlIn_V_data_V0_status == ap_const_logic_0) & ~(ap_const_lv1_0 == streamSource_V)) | ((tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (udpPort2addIpHeader_data_V_dat1_status == ap_const_logic_0)) | ((udpIn_V_data_V0_status == ap_const_logic_0) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv1_0 == streamSource_V) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3)) | (~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ttlIn_V_data_V0_status == ap_const_logic_0) & ~(ap_const_lv1_0 == streamSource_V) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3)) | (~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (udpPort2addIpHeader_data_V_dat1_status == ap_const_logic_0) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3)) | (~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & (udpPort2addIpHeader_header_V_V_full_n == ap_const_logic_0)) | (~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (udpPort2addIpHeader_data_V_dat1_status == ap_const_logic_0) & (ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2)) | (ap_start == ap_const_logic_0) | (ap_done_reg == ap_const_logic_1));
end

/// ap_sig_bdd_151 assign process. ///
always @ (ap_CS_fsm or ap_sig_bdd_148)
begin
    ap_sig_bdd_151 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~ap_sig_bdd_148);
end

/// ap_sig_bdd_221 assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_nbwritereq_fu_199_p5 or brmerge_fu_714_p2 or ap_sig_bdd_148)
begin
    ap_sig_bdd_221 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2) & ~ap_sig_bdd_148);
end

/// ap_sig_bdd_229 assign process. ///
always @ (tmp_last_V_9_phi_fu_264_p4 or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5)
begin
    ap_sig_bdd_229 = ((tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & ~(ap_const_lv1_0 == tmp_last_V_9_phi_fu_264_p4));
end

/// ap_sig_bdd_235 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3 or tmp_30_fu_690_p2)
begin
    ap_sig_bdd_235 = (~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & ~(ap_const_lv1_0 == tmp_30_fu_690_p2));
end

/// ap_sig_bdd_485 assign process. ///
always @ (ap_CS_fsm or grp_nbwritereq_fu_199_p5 or ap_sig_bdd_148)
begin
    ap_sig_bdd_485 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & ~ap_sig_bdd_148);
end

/// ap_sig_bdd_486 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2 or tmp_22_nbwritereq_fu_244_p3)
begin
    ap_sig_bdd_486 = (~(ap_const_lv1_0 == grp_fu_353_p2) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3));
end

/// ap_sig_bdd_487 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2 or tmp_22_nbwritereq_fu_244_p3 or tmp_30_fu_690_p2)
begin
    ap_sig_bdd_487 = (~(ap_const_lv1_0 == grp_fu_353_p2) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3) & (ap_const_lv1_0 == tmp_30_fu_690_p2));
end

/// ap_sig_bdd_492 assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_nbwritereq_fu_199_p5 or brmerge_fu_714_p2)
begin
    ap_sig_bdd_492 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (ap_const_lv3_0 == tmp_cast_fu_399_p1) & ~(ap_const_lv1_0 == brmerge_fu_714_p2));
end

/// ap_sig_bdd_493 assign process. ///
always @ (tmp_nbreadreq_fu_160_p5 or tmp_17_nbreadreq_fu_172_p5)
begin
    ap_sig_bdd_493 = ((ap_const_lv1_0 == tmp_nbreadreq_fu_160_p5) & ~(ap_const_lv1_0 == tmp_17_nbreadreq_fu_172_p5));
end

/// ap_sig_bdd_495 assign process. ///
always @ (tmp_nbreadreq_fu_160_p5 or tmp_17_nbreadreq_fu_172_p5)
begin
    ap_sig_bdd_495 = ((ap_const_lv1_0 == tmp_nbreadreq_fu_160_p5) & (ap_const_lv1_0 == tmp_17_nbreadreq_fu_172_p5));
end

/// ap_sig_bdd_501 assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5 or tmp_22_nbwritereq_fu_244_p3)
begin
    ap_sig_bdd_501 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5) & (tmp_cast_fu_399_p1 == ap_const_lv3_1) & ~(ap_const_lv1_0 == tmp_22_nbwritereq_fu_244_p3));
end

/// ap_sig_bdd_504 assign process. ///
always @ (ap_CS_fsm or tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5)
begin
    ap_sig_bdd_504 = ((ap_ST_st1_fsm_0 == ap_CS_fsm) & (tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5));
end

/// ap_sig_bdd_83 assign process. ///
always @ (tmp_cast_fu_399_p1 or tmp_23_nbwritereq_fu_184_p3)
begin
    ap_sig_bdd_83 = ((tmp_cast_fu_399_p1 == ap_const_lv3_3) & ~(ap_const_lv1_0 == tmp_23_nbwritereq_fu_184_p3));
end

/// ap_sig_bdd_92 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2)
begin
    ap_sig_bdd_92 = ((tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2));
end

/// ap_sig_bdd_96 assign process. ///
always @ (tmp_cast_fu_399_p1 or grp_fu_353_p2 or grp_nbwritereq_fu_199_p5)
begin
    ap_sig_bdd_96 = ((tmp_cast_fu_399_p1 == ap_const_lv3_2) & ~(ap_const_lv1_0 == grp_fu_353_p2) & ~(ap_const_lv1_0 == grp_nbwritereq_fu_199_p5));
end
assign brmerge_fu_714_p2 = (tmp_nbreadreq_fu_160_p5 | tmp_17_nbreadreq_fu_172_p5);
assign checksumStreams_V_V_1_din = p_Val2_s_fu_485_p2[15:0];
assign grp_fu_337_p2 = (streamSource_V ^ ap_const_lv1_1);
assign grp_fu_342_p2 = (tmp_nbreadreq_fu_160_p5 & grp_fu_337_p2);
assign grp_fu_348_p2 = (streamSource_V & tmp_17_nbreadreq_fu_172_p5);
assign grp_fu_353_p2 = (grp_fu_342_p2 | grp_fu_348_p2);
assign grp_nbwritereq_fu_199_p5 = (udpPort2addIpHeader_data_V_dat_full_n & udpPort2addIpHeader_data_V_kee_full_n & udpPort2addIpHeader_data_V_las_full_n);
assign p_8_cast_fu_481_p1 = $unsigned(p_8_fu_475_p2);
assign p_8_fu_475_p2 = (r_V_1_fu_471_p1 + tmp_28_cast_fu_459_p1);
assign p_Result_10_fu_602_p4 = {{p_Val2_18_phi_fu_314_p4[ap_const_lv32_3F : ap_const_lv32_30]}};
assign p_Result_11_fu_612_p4 = {{p_Val2_18_phi_fu_314_p4[ap_const_lv32_2F : ap_const_lv32_20]}};
assign p_Result_12_fu_630_p4 = {{p_Val2_18_phi_fu_314_p4[ap_const_lv32_1F : ap_const_lv32_10]}};
assign p_Result_8_fu_518_p4 = {{p_Val2_19_phi_fu_284_p4[ap_const_lv32_2F : ap_const_lv32_20]}};
assign p_Result_9_fu_536_p4 = {{p_Val2_19_phi_fu_284_p4[ap_const_lv32_1F : ap_const_lv32_10]}};
assign p_Result_s_fu_508_p4 = {{p_Val2_19_phi_fu_284_p4[ap_const_lv32_3F : ap_const_lv32_30]}};
assign p_Val2_22_cast_fu_738_p1 = $signed(p_Val2_17_phi_fu_325_p6);
assign p_Val2_s_fu_485_p2 = (p_8_cast_fu_481_p1 ^ ap_const_lv20_FFFFF);
assign p_s_fu_439_p2 = (r_V_cast_fu_435_p1 + tmp_27_cast_fu_421_p1);
assign r_V_1_fu_471_p1 = $unsigned(tmp_99_fu_463_p3);
assign r_V_cast_fu_435_p1 = $unsigned(r_V_fu_425_p4);
assign r_V_fu_425_p4 = {{udpChecksum_V[ap_const_lv32_13 : ap_const_lv32_10]}};
assign tmp1_cast_fu_674_p1 = $unsigned(tmp1_fu_668_p2);
assign tmp1_fu_668_p2 = (tmp_28_fu_662_p2 + tmp_45_cast_fu_648_p1);
assign tmp2_cast_fu_580_p1 = $unsigned(tmp2_fu_574_p2);
assign tmp2_fu_574_p2 = (tmp_25_fu_568_p2 + tmp_cast3_fu_554_p1);
assign tmp_101_fu_550_p1 = p_Val2_19_phi_fu_284_p4[15:0];
assign tmp_102_fu_644_p1 = p_Val2_18_phi_fu_314_p4[15:0];
assign tmp_17_nbreadreq_fu_172_p5 = (ttlIn_V_data_V_empty_n & ttlIn_V_keep_V_empty_n & ttlIn_V_last_V_empty_n);
assign tmp_22_nbwritereq_fu_244_p3 = udpPort2addIpHeader_header_V_V_full_n;
assign tmp_23_nbwritereq_fu_184_p3 = checksumStreams_V_V_1_full_n;
assign tmp_25_fu_568_p2 = (tmp_42_cast_fu_564_p1 + tmp_40_cast_fu_546_p1);
assign tmp_26_fu_584_p2 = (tmp2_cast_fu_580_p1 + udpChecksum_V);
assign tmp_27_cast_fu_421_p1 = $unsigned(tmp_95_fu_417_p1);
assign tmp_27_fu_652_p2 = (tmp_49_cast_fu_626_p1 + tmp_48_cast_fu_622_p1);
assign tmp_28_cast_fu_459_p1 = $unsigned(tmp_98_fu_453_p2);
assign tmp_28_fu_662_p2 = (tmp_52_cast_fu_658_p1 + tmp_50_cast_fu_640_p1);
assign tmp_29_fu_678_p2 = (tmp1_cast_fu_674_p1 + udpChecksum_V);
assign tmp_30_fu_690_p2 = (ipWordCounter_V == ap_const_lv3_2? 1'b1: 1'b0);
assign tmp_31_fu_696_p2 = (ipWordCounter_V + ap_const_lv3_1);
assign tmp_35_cast_fu_747_p0 = p_Val2_22_cast_fu_738_p1;
assign tmp_35_cast_fu_747_p1 = $unsigned(tmp_35_cast_fu_747_p0);
assign tmp_38_cast_fu_528_p1 = $unsigned(p_Result_s_fu_508_p4);
assign tmp_39_cast_fu_532_p1 = $unsigned(p_Result_8_fu_518_p4);
assign tmp_40_cast_fu_546_p1 = $unsigned(p_Result_9_fu_536_p4);
assign tmp_42_cast_fu_564_p1 = $unsigned(tmp_s_fu_558_p2);
assign tmp_45_cast_fu_648_p1 = $unsigned(tmp_102_fu_644_p1);
assign tmp_48_cast_fu_622_p1 = $unsigned(p_Result_10_fu_602_p4);
assign tmp_49_cast_fu_626_p1 = $unsigned(p_Result_11_fu_612_p4);
assign tmp_50_cast_fu_640_p1 = $unsigned(p_Result_12_fu_630_p4);
assign tmp_52_cast_fu_658_p1 = $unsigned(tmp_27_fu_652_p2);
assign tmp_95_fu_417_p1 = udpChecksum_V[15:0];
assign tmp_96_fu_445_p1 = $unsigned(r_V_fu_425_p4);
assign tmp_97_fu_449_p1 = udpChecksum_V[15:0];
assign tmp_98_fu_453_p2 = (tmp_96_fu_445_p1 + tmp_97_fu_449_p1);
assign tmp_99_fu_463_p3 = p_s_fu_439_p2[ap_const_lv32_10];
assign tmp_cast3_fu_554_p1 = $unsigned(tmp_101_fu_550_p1);
assign tmp_cast_fu_399_p1 = $unsigned(udpState);
assign tmp_data_V_6_fu_742_p0 = p_Val2_22_cast_fu_738_p1;
assign tmp_data_V_6_fu_742_p1 = $unsigned(tmp_data_V_6_fu_742_p0);
assign tmp_nbreadreq_fu_160_p5 = (udpIn_V_data_V_empty_n & udpIn_V_keep_V_empty_n & udpIn_V_last_V_empty_n);
assign tmp_s_fu_558_p2 = (tmp_39_cast_fu_532_p1 + tmp_38_cast_fu_528_p1);
assign ttlIn_V_data_V0_status = (ttlIn_V_data_V_empty_n & ttlIn_V_keep_V_empty_n & ttlIn_V_last_V_empty_n);
assign ttlIn_V_data_V_read = ttlIn_V_data_V0_update;
assign ttlIn_V_keep_V_read = ttlIn_V_data_V0_update;
assign ttlIn_V_last_V_read = ttlIn_V_data_V0_update;
assign udpIn_V_data_V0_status = (udpIn_V_data_V_empty_n & udpIn_V_keep_V_empty_n & udpIn_V_last_V_empty_n);
assign udpIn_V_data_V_read = udpIn_V_data_V0_update;
assign udpIn_V_keep_V_read = udpIn_V_data_V0_update;
assign udpIn_V_last_V_read = udpIn_V_data_V0_update;
assign udpPort2addIpHeader_data_V_dat1_status = (udpPort2addIpHeader_data_V_dat_full_n & udpPort2addIpHeader_data_V_kee_full_n & udpPort2addIpHeader_data_V_las_full_n);
assign udpPort2addIpHeader_data_V_dat_write = udpPort2addIpHeader_data_V_dat1_update;
assign udpPort2addIpHeader_data_V_kee_write = udpPort2addIpHeader_data_V_dat1_update;
assign udpPort2addIpHeader_data_V_las_write = udpPort2addIpHeader_data_V_dat1_update;
assign udpPort2addIpHeader_header_V_V_din = p_Val2_18_phi_fu_314_p4;


endmodule //icmp_server_udpPortUnreachable
