// ==============================================================
// RTL generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2015.1
// Copyright (C) 2015 Xilinx Inc. All rights reserved.
// 
// ===========================================================

`timescale 1 ns / 1 ps 

module udp_rxEngineUdpChecksumVerificatio (
        ap_clk,
        ap_rst,
        ap_start,
        ap_done,
        ap_continue,
        ap_idle,
        ap_ready,
        strip2rxChecksum_V_data_V_dout,
        strip2rxChecksum_V_data_V_empty_n,
        strip2rxChecksum_V_data_V_read,
        strip2rxChecksum_V_keep_V_dout,
        strip2rxChecksum_V_keep_V_empty_n,
        strip2rxChecksum_V_keep_V_read,
        strip2rxChecksum_V_last_V_dout,
        strip2rxChecksum_V_last_V_empty_n,
        strip2rxChecksum_V_last_V_read,
        rxChecksum2rxEngine_V_din,
        rxChecksum2rxEngine_V_full_n,
        rxChecksum2rxEngine_V_write
);

parameter    ap_const_logic_1 = 1'b1;
parameter    ap_const_logic_0 = 1'b0;
parameter    ap_ST_st1_fsm_0 = 1'b1;
parameter    ap_const_lv32_0 = 32'b00000000000000000000000000000000;
parameter    ap_const_lv1_1 = 1'b1;
parameter    ap_const_lv2_0 = 2'b00;
parameter    ap_const_lv16_0 = 16'b0000000000000000;
parameter    ap_const_lv2_2 = 2'b10;
parameter    ap_const_lv1_0 = 1'b0;
parameter    ap_const_lv2_1 = 2'b1;
parameter    ap_const_lv10_1 = 10'b1;
parameter    ap_const_lv32_30 = 32'b110000;
parameter    ap_const_lv32_3F = 32'b111111;
parameter    ap_const_lv32_20 = 32'b100000;
parameter    ap_const_lv32_2F = 32'b101111;
parameter    ap_const_lv32_10 = 32'b10000;
parameter    ap_const_lv32_1F = 32'b11111;
parameter    ap_const_lv32_FFFFFFFF = 32'b11111111111111111111111111111111;
parameter    ap_const_lv10_3 = 10'b11;
parameter    ap_const_lv32_18 = 32'b11000;
parameter    ap_const_lv32_17 = 32'b10111;
parameter    ap_true = 1'b1;

input   ap_clk;
input   ap_rst;
input   ap_start;
output   ap_done;
input   ap_continue;
output   ap_idle;
output   ap_ready;
input  [63:0] strip2rxChecksum_V_data_V_dout;
input   strip2rxChecksum_V_data_V_empty_n;
output   strip2rxChecksum_V_data_V_read;
input  [7:0] strip2rxChecksum_V_keep_V_dout;
input   strip2rxChecksum_V_keep_V_empty_n;
output   strip2rxChecksum_V_keep_V_read;
input  [0:0] strip2rxChecksum_V_last_V_dout;
input   strip2rxChecksum_V_last_V_empty_n;
output   strip2rxChecksum_V_last_V_read;
output  [0:0] rxChecksum2rxEngine_V_din;
input   rxChecksum2rxEngine_V_full_n;
output   rxChecksum2rxEngine_V_write;

reg ap_done;
reg ap_idle;
reg ap_ready;
reg[0:0] rxChecksum2rxEngine_V_din;
reg rxChecksum2rxEngine_V_write;
reg    ap_done_reg = 1'b0;
(* fsm_encoding = "none" *) reg   [0:0] ap_CS_fsm = 1'b1;
reg    ap_sig_cseq_ST_st1_fsm_0;
reg    ap_sig_bdd_20;
reg   [1:0] udpChecksumState = 2'b00;
reg   [15:0] receivedChecksum_V = 16'b0000000000000000;
reg   [9:0] wordCounter_V = 10'b0000000000;
reg   [31:0] udpChecksum_V = 32'b00000000000000000000000000000000;
wire   [0:0] tmp_32_nbwritereq_fu_96_p3;
wire   [0:0] or_cond_fu_278_p2;
wire    strip2rxChecksum_V_data_V0_status;
wire   [0:0] grp_nbreadreq_fu_113_p5;
reg    ap_sig_bdd_86;
reg    strip2rxChecksum_V_data_V0_update;
wire   [0:0] tmp_last_V_fu_302_p1;
wire   [15:0] tmp_47_fu_332_p3;
wire   [0:0] tmp_45_fu_306_p2;
wire   [9:0] tmp_44_fu_290_p2;
wire   [31:0] p_Val2_22_fu_250_p2;
wire   [31:0] tmp_52_fu_392_p2;
wire   [31:0] tmp_58_cast_fu_468_p1;
wire   [15:0] tmp_92_fu_186_p1;
wire   [15:0] r_V_fu_194_p4;
wire   [16:0] tmp_61_cast_fu_190_p1;
wire   [16:0] r_V_cast_fu_204_p1;
wire   [15:0] tmp_94_fu_214_p1;
wire   [15:0] tmp_95_fu_218_p2;
wire   [16:0] tmp_46_fu_208_p2;
wire   [0:0] tmp_96_fu_228_p3;
wire   [16:0] tmp_63_cast_fu_224_p1;
wire   [16:0] r_V_1_fu_236_p1;
wire   [16:0] tmp_48_fu_240_p2;
wire   [31:0] tmp_64_cast_fu_246_p1;
wire   [15:0] tempChecksum_V_fu_262_p1;
wire   [0:0] tmp_49_fu_266_p2;
wire   [0:0] tmp_50_fu_272_p2;
wire   [7:0] p_Result_11_fu_322_p4;
wire   [7:0] p_Result_10_fu_312_p4;
wire   [15:0] grp_fu_140_p4;
wire   [15:0] grp_fu_150_p4;
wire   [15:0] grp_fu_160_p4;
wire   [15:0] tmp_98_fu_358_p1;
wire   [31:0] tmp_51_fu_346_p1;
wire   [16:0] tmp_58_cast5_fu_362_p1;
wire   [16:0] tmp_57_cast_fu_354_p1;
wire   [16:0] tmp4_fu_372_p2;
wire   [17:0] tmp_56_cast6_fu_350_p1;
wire   [17:0] tmp4_cast_fu_378_p1;
wire   [17:0] tmp3_fu_382_p2;
wire   [31:0] tmp2_fu_366_p2;
wire   [31:0] tmp3_cast_fu_388_p1;
wire   [15:0] tmp_93_fu_434_p1;
wire   [16:0] tmp_53_cast_fu_426_p1;
wire   [16:0] tmp_cast_fu_422_p1;
wire   [16:0] tmp_s_fu_442_p2;
wire   [16:0] tmp_55_cast_cast_fu_438_p1;
wire   [16:0] tmp_54_cast_cast_fu_430_p1;
wire   [16:0] tmp1_fu_452_p2;
wire   [17:0] tmp_56_cast_fu_448_p1;
wire   [17:0] tmp1_cast_fu_458_p1;
wire   [17:0] tmp_43_fu_462_p2;
reg   [0:0] ap_NS_fsm;
reg    ap_sig_bdd_113;
reg    ap_sig_bdd_89;
reg    ap_sig_bdd_102;
reg    ap_sig_bdd_60;
reg    ap_sig_bdd_106;
reg    ap_sig_bdd_78;
reg    ap_sig_bdd_75;
reg    ap_sig_bdd_333;




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
        end else if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_86)) begin
            ap_done_reg <= ap_const_logic_1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_89) begin
        if ((ap_const_lv2_0 == udpChecksumState)) begin
            receivedChecksum_V <= ap_const_lv16_0;
        end else if (ap_sig_bdd_113) begin
            receivedChecksum_V <= tmp_47_fu_332_p3;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_89) begin
        if (ap_sig_bdd_78) begin
            udpChecksumState <= ap_const_lv2_1;
        end else if (ap_sig_bdd_106) begin
            udpChecksumState <= ap_const_lv2_2;
        end else if (ap_sig_bdd_60) begin
            udpChecksumState <= ap_const_lv2_0;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_89) begin
        if (ap_sig_bdd_78) begin
            udpChecksum_V <= tmp_58_cast_fu_468_p1;
        end else if (ap_sig_bdd_75) begin
            udpChecksum_V <= tmp_52_fu_392_p2;
        end else if (ap_sig_bdd_60) begin
            udpChecksum_V <= p_Val2_22_fu_250_p2;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_333) begin
        if ((ap_const_lv2_0 == udpChecksumState)) begin
            wordCounter_V <= ap_const_lv10_1;
        end else if ((udpChecksumState == ap_const_lv2_1)) begin
            wordCounter_V <= tmp_44_fu_290_p2;
        end
    end
end

/// ap_done assign process. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm_0 or ap_sig_bdd_86)
begin
    if (((ap_const_logic_1 == ap_done_reg) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_86))) begin
        ap_done = ap_const_logic_1;
    end else begin
        ap_done = ap_const_logic_0;
    end
end

/// ap_idle assign process. ///
always @ (ap_start or ap_sig_cseq_ST_st1_fsm_0)
begin
    if ((~(ap_const_logic_1 == ap_start) & (ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0))) begin
        ap_idle = ap_const_logic_1;
    end else begin
        ap_idle = ap_const_logic_0;
    end
end

/// ap_ready assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ap_sig_bdd_86)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_86)) begin
        ap_ready = ap_const_logic_1;
    end else begin
        ap_ready = ap_const_logic_0;
    end
end

/// ap_sig_cseq_ST_st1_fsm_0 assign process. ///
always @ (ap_sig_bdd_20)
begin
    if (ap_sig_bdd_20) begin
        ap_sig_cseq_ST_st1_fsm_0 = ap_const_logic_1;
    end else begin
        ap_sig_cseq_ST_st1_fsm_0 = ap_const_logic_0;
    end
end

/// rxChecksum2rxEngine_V_din assign process. ///
always @ (or_cond_fu_278_p2 or ap_sig_bdd_102)
begin
    if (ap_sig_bdd_102) begin
        if (~(ap_const_lv1_0 == or_cond_fu_278_p2)) begin
            rxChecksum2rxEngine_V_din = ap_const_lv1_1;
        end else if ((ap_const_lv1_0 == or_cond_fu_278_p2)) begin
            rxChecksum2rxEngine_V_din = ap_const_lv1_0;
        end else begin
            rxChecksum2rxEngine_V_din = 'bx;
        end
    end else begin
        rxChecksum2rxEngine_V_din = 'bx;
    end
end

/// rxChecksum2rxEngine_V_write assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or udpChecksumState or tmp_32_nbwritereq_fu_96_p3 or or_cond_fu_278_p2 or ap_sig_bdd_86)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0) & (ap_const_lv1_0 == or_cond_fu_278_p2) & ~ap_sig_bdd_86) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0) & ~(ap_const_lv1_0 == or_cond_fu_278_p2) & ~ap_sig_bdd_86))) begin
        rxChecksum2rxEngine_V_write = ap_const_logic_1;
    end else begin
        rxChecksum2rxEngine_V_write = ap_const_logic_0;
    end
end

/// strip2rxChecksum_V_data_V0_update assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or udpChecksumState or grp_nbreadreq_fu_113_p5 or ap_sig_bdd_86)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (udpChecksumState == ap_const_lv2_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & ~ap_sig_bdd_86) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & (ap_const_lv2_0 == udpChecksumState) & ~ap_sig_bdd_86))) begin
        strip2rxChecksum_V_data_V0_update = ap_const_logic_1;
    end else begin
        strip2rxChecksum_V_data_V0_update = ap_const_logic_0;
    end
end
/// the next state (ap_NS_fsm) of the state machine. ///
always @ (ap_CS_fsm or ap_sig_bdd_86)
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


/// ap_sig_bdd_102 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or udpChecksumState or tmp_32_nbwritereq_fu_96_p3 or ap_sig_bdd_86)
begin
    ap_sig_bdd_102 = ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0) & ~ap_sig_bdd_86);
end

/// ap_sig_bdd_106 assign process. ///
always @ (udpChecksumState or grp_nbreadreq_fu_113_p5 or tmp_last_V_fu_302_p1)
begin
    ap_sig_bdd_106 = ((udpChecksumState == ap_const_lv2_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & ~(ap_const_lv1_0 == tmp_last_V_fu_302_p1));
end

/// ap_sig_bdd_113 assign process. ///
always @ (udpChecksumState or grp_nbreadreq_fu_113_p5 or tmp_45_fu_306_p2)
begin
    ap_sig_bdd_113 = ((udpChecksumState == ap_const_lv2_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & ~(ap_const_lv1_0 == tmp_45_fu_306_p2));
end

/// ap_sig_bdd_20 assign process. ///
always @ (ap_CS_fsm)
begin
    ap_sig_bdd_20 = (ap_CS_fsm[ap_const_lv32_0] == ap_const_lv1_1);
end

/// ap_sig_bdd_333 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or grp_nbreadreq_fu_113_p5 or ap_sig_bdd_86)
begin
    ap_sig_bdd_333 = ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & ~ap_sig_bdd_86);
end

/// ap_sig_bdd_60 assign process. ///
always @ (udpChecksumState or tmp_32_nbwritereq_fu_96_p3)
begin
    ap_sig_bdd_60 = ((udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0));
end

/// ap_sig_bdd_75 assign process. ///
always @ (udpChecksumState or grp_nbreadreq_fu_113_p5)
begin
    ap_sig_bdd_75 = ((udpChecksumState == ap_const_lv2_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5));
end

/// ap_sig_bdd_78 assign process. ///
always @ (udpChecksumState or grp_nbreadreq_fu_113_p5)
begin
    ap_sig_bdd_78 = (~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & (ap_const_lv2_0 == udpChecksumState));
end

/// ap_sig_bdd_86 assign process. ///
always @ (ap_start or ap_done_reg or udpChecksumState or rxChecksum2rxEngine_V_full_n or tmp_32_nbwritereq_fu_96_p3 or or_cond_fu_278_p2 or strip2rxChecksum_V_data_V0_status or grp_nbreadreq_fu_113_p5)
begin
    ap_sig_bdd_86 = (((rxChecksum2rxEngine_V_full_n == ap_const_logic_0) & (udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0) & (ap_const_lv1_0 == or_cond_fu_278_p2)) | ((rxChecksum2rxEngine_V_full_n == ap_const_logic_0) & (udpChecksumState == ap_const_lv2_2) & ~(tmp_32_nbwritereq_fu_96_p3 == ap_const_lv1_0) & ~(ap_const_lv1_0 == or_cond_fu_278_p2)) | ((strip2rxChecksum_V_data_V0_status == ap_const_logic_0) & (udpChecksumState == ap_const_lv2_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5)) | ((strip2rxChecksum_V_data_V0_status == ap_const_logic_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_113_p5) & (ap_const_lv2_0 == udpChecksumState)) | (ap_start == ap_const_logic_0) | (ap_done_reg == ap_const_logic_1));
end

/// ap_sig_bdd_89 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ap_sig_bdd_86)
begin
    ap_sig_bdd_89 = ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_86);
end
assign grp_fu_140_p4 = {{strip2rxChecksum_V_data_V_dout[ap_const_lv32_3F : ap_const_lv32_30]}};
assign grp_fu_150_p4 = {{strip2rxChecksum_V_data_V_dout[ap_const_lv32_2F : ap_const_lv32_20]}};
assign grp_fu_160_p4 = {{strip2rxChecksum_V_data_V_dout[ap_const_lv32_1F : ap_const_lv32_10]}};
assign grp_nbreadreq_fu_113_p5 = (strip2rxChecksum_V_data_V_empty_n & strip2rxChecksum_V_keep_V_empty_n & strip2rxChecksum_V_last_V_empty_n);
assign or_cond_fu_278_p2 = (tmp_49_fu_266_p2 | tmp_50_fu_272_p2);
assign p_Result_10_fu_312_p4 = {{strip2rxChecksum_V_data_V_dout[ap_const_lv32_1F : ap_const_lv32_18]}};
assign p_Result_11_fu_322_p4 = {{strip2rxChecksum_V_data_V_dout[ap_const_lv32_17 : ap_const_lv32_10]}};
assign p_Val2_22_fu_250_p2 = (tmp_64_cast_fu_246_p1 ^ ap_const_lv32_FFFFFFFF);
assign r_V_1_fu_236_p1 = tmp_96_fu_228_p3;
assign r_V_cast_fu_204_p1 = r_V_fu_194_p4;
assign r_V_fu_194_p4 = {{udpChecksum_V[ap_const_lv32_1F : ap_const_lv32_10]}};
assign strip2rxChecksum_V_data_V0_status = (strip2rxChecksum_V_data_V_empty_n & strip2rxChecksum_V_keep_V_empty_n & strip2rxChecksum_V_last_V_empty_n);
assign strip2rxChecksum_V_data_V_read = strip2rxChecksum_V_data_V0_update;
assign strip2rxChecksum_V_keep_V_read = strip2rxChecksum_V_data_V0_update;
assign strip2rxChecksum_V_last_V_read = strip2rxChecksum_V_data_V0_update;
assign tempChecksum_V_fu_262_p1 = p_Val2_22_fu_250_p2[15:0];
assign tmp1_cast_fu_458_p1 = tmp1_fu_452_p2;
assign tmp1_fu_452_p2 = (tmp_55_cast_cast_fu_438_p1 + tmp_54_cast_cast_fu_430_p1);
assign tmp2_fu_366_p2 = (tmp_51_fu_346_p1 + udpChecksum_V);
assign tmp3_cast_fu_388_p1 = tmp3_fu_382_p2;
assign tmp3_fu_382_p2 = (tmp_56_cast6_fu_350_p1 + tmp4_cast_fu_378_p1);
assign tmp4_cast_fu_378_p1 = tmp4_fu_372_p2;
assign tmp4_fu_372_p2 = (tmp_58_cast5_fu_362_p1 + tmp_57_cast_fu_354_p1);
assign tmp_32_nbwritereq_fu_96_p3 = rxChecksum2rxEngine_V_full_n;
assign tmp_43_fu_462_p2 = (tmp_56_cast_fu_448_p1 + tmp1_cast_fu_458_p1);
assign tmp_44_fu_290_p2 = (wordCounter_V + ap_const_lv10_1);
assign tmp_45_fu_306_p2 = (tmp_44_fu_290_p2 == ap_const_lv10_3? 1'b1: 1'b0);
assign tmp_46_fu_208_p2 = (tmp_61_cast_fu_190_p1 + r_V_cast_fu_204_p1);
assign tmp_47_fu_332_p3 = {{p_Result_11_fu_322_p4}, {p_Result_10_fu_312_p4}};
assign tmp_48_fu_240_p2 = (tmp_63_cast_fu_224_p1 + r_V_1_fu_236_p1);
assign tmp_49_fu_266_p2 = (tempChecksum_V_fu_262_p1 == ap_const_lv16_0? 1'b1: 1'b0);
assign tmp_50_fu_272_p2 = (receivedChecksum_V == ap_const_lv16_0? 1'b1: 1'b0);
assign tmp_51_fu_346_p1 = grp_fu_140_p4;
assign tmp_52_fu_392_p2 = (tmp2_fu_366_p2 + tmp3_cast_fu_388_p1);
assign tmp_53_cast_fu_426_p1 = grp_fu_150_p4;
assign tmp_54_cast_cast_fu_430_p1 = grp_fu_160_p4;
assign tmp_55_cast_cast_fu_438_p1 = tmp_93_fu_434_p1;
assign tmp_56_cast6_fu_350_p1 = grp_fu_150_p4;
assign tmp_56_cast_fu_448_p1 = tmp_s_fu_442_p2;
assign tmp_57_cast_fu_354_p1 = grp_fu_160_p4;
assign tmp_58_cast5_fu_362_p1 = tmp_98_fu_358_p1;
assign tmp_58_cast_fu_468_p1 = tmp_43_fu_462_p2;
assign tmp_61_cast_fu_190_p1 = tmp_92_fu_186_p1;
assign tmp_63_cast_fu_224_p1 = tmp_95_fu_218_p2;
assign tmp_64_cast_fu_246_p1 = tmp_48_fu_240_p2;
assign tmp_92_fu_186_p1 = udpChecksum_V[15:0];
assign tmp_93_fu_434_p1 = strip2rxChecksum_V_data_V_dout[15:0];
assign tmp_94_fu_214_p1 = udpChecksum_V[15:0];
assign tmp_95_fu_218_p2 = (r_V_fu_194_p4 + tmp_94_fu_214_p1);
assign tmp_96_fu_228_p3 = tmp_46_fu_208_p2[ap_const_lv32_10];
assign tmp_98_fu_358_p1 = strip2rxChecksum_V_data_V_dout[15:0];
assign tmp_cast_fu_422_p1 = grp_fu_140_p4;
assign tmp_last_V_fu_302_p1 = strip2rxChecksum_V_last_V_dout;
assign tmp_s_fu_442_p2 = (tmp_53_cast_fu_426_p1 + tmp_cast_fu_422_p1);


endmodule //udp_rxEngineUdpChecksumVerificatio
