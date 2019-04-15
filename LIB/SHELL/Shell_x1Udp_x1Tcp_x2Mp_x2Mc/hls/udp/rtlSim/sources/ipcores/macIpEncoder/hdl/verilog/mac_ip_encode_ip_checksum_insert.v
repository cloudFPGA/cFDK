// ==============================================================
// RTL generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2015.1
// Copyright (C) 2015 Xilinx Inc. All rights reserved.
// 
// ===========================================================

`timescale 1 ns / 1 ps 

module mac_ip_encode_ip_checksum_insert (
        ap_clk,
        ap_rst,
        ap_start,
        ap_done,
        ap_continue,
        ap_idle,
        ap_ready,
        dataStreamBuffer0_V_dout,
        dataStreamBuffer0_V_empty_n,
        dataStreamBuffer0_V_read,
        dataStreamBuffer1_V_din,
        dataStreamBuffer1_V_full_n,
        dataStreamBuffer1_V_write,
        checksumFifo_V_V_dout,
        checksumFifo_V_V_empty_n,
        checksumFifo_V_V_read
);

parameter    ap_const_logic_1 = 1'b1;
parameter    ap_const_logic_0 = 1'b0;
parameter    ap_ST_st1_fsm_0 = 1'b1;
parameter    ap_const_lv32_0 = 32'b00000000000000000000000000000000;
parameter    ap_const_lv1_1 = 1'b1;
parameter    ap_const_lv4_0 = 4'b0000;
parameter    ap_const_lv1_0 = 1'b0;
parameter    ap_const_lv4_1 = 4'b1;
parameter    ap_const_lv4_2 = 4'b10;
parameter    ap_const_lv32_48 = 32'b1001000;
parameter    ap_const_lv32_8 = 32'b1000;
parameter    ap_const_lv32_F = 32'b1111;
parameter    ap_const_lv32_10 = 32'b10000;
parameter    ap_const_lv32_1F = 32'b11111;
parameter    ap_const_lv32_40 = 32'b1000000;
parameter    ap_true = 1'b1;

input   ap_clk;
input   ap_rst;
input   ap_start;
output   ap_done;
input   ap_continue;
output   ap_idle;
output   ap_ready;
input  [72:0] dataStreamBuffer0_V_dout;
input   dataStreamBuffer0_V_empty_n;
output   dataStreamBuffer0_V_read;
output  [72:0] dataStreamBuffer1_V_din;
input   dataStreamBuffer1_V_full_n;
output   dataStreamBuffer1_V_write;
input  [15:0] checksumFifo_V_V_dout;
input   checksumFifo_V_V_empty_n;
output   checksumFifo_V_V_read;

reg ap_done;
reg ap_idle;
reg ap_ready;
reg dataStreamBuffer0_V_read;
reg[72:0] dataStreamBuffer1_V_din;
reg dataStreamBuffer1_V_write;
reg checksumFifo_V_V_read;
reg    ap_done_reg = 1'b0;
(* fsm_encoding = "none" *) reg   [0:0] ap_CS_fsm = 1'b1;
reg    ap_sig_cseq_ST_st1_fsm_0;
reg    ap_sig_bdd_20;
reg   [3:0] ici_wordCount_V = 4'b0000;
reg   [0:0] ici_wordCount_V_flag_phi_fu_127_p6;
wire   [0:0] grp_nbreadreq_fu_88_p3;
wire   [0:0] tmp_3_nbreadreq_fu_96_p3;
reg    ap_sig_bdd_81;
reg   [3:0] ici_wordCount_V_new_phi_fu_141_p6;
wire   [0:0] grp_fu_202_p3;
reg   [0:0] ici_wordCount_V_flag_1_phi_fu_166_p10;
wire   [0:0] p_ici_wordCount_V_flag_fu_274_p2;
wire   [3:0] p_ici_wordCount_V_new_fu_281_p3;
wire   [72:0] tmp_390_fu_265_p3;
wire   [7:0] tmp_110_fu_231_p1;
wire   [7:0] p_Result_s_fu_221_p4;
wire   [63:0] p_Val2_2_fu_217_p1;
wire   [15:0] tmp_s_fu_235_p3;
wire   [8:0] tmp_24_fu_255_p4;
wire   [63:0] p_Result_s_21_fu_243_p5;
reg   [0:0] ap_NS_fsm;
reg    ap_sig_bdd_199;
reg    ap_sig_bdd_198;




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
        end else if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_81)) begin
            ap_done_reg <= ap_const_logic_1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_81 & ~(ap_const_lv1_0 == ici_wordCount_V_flag_1_phi_fu_166_p10))) begin
        ici_wordCount_V <= p_ici_wordCount_V_new_fu_281_p3;
    end
end

/// ap_done assign process. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm_0 or ap_sig_bdd_81)
begin
    if (((ap_const_logic_1 == ap_done_reg) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_81))) begin
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
always @ (ap_sig_cseq_ST_st1_fsm_0 or ap_sig_bdd_81)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~ap_sig_bdd_81)) begin
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

/// checksumFifo_V_V_read assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3 or ap_sig_bdd_81)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & ~ap_sig_bdd_81)) begin
        checksumFifo_V_V_read = ap_const_logic_1;
    end else begin
        checksumFifo_V_V_read = ap_const_logic_0;
    end
end

/// dataStreamBuffer0_V_read assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3 or ap_sig_bdd_81)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & ~ap_sig_bdd_81) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81))) begin
        dataStreamBuffer0_V_read = ap_const_logic_1;
    end else begin
        dataStreamBuffer0_V_read = ap_const_logic_0;
    end
end

/// dataStreamBuffer1_V_din assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or dataStreamBuffer0_V_dout or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3 or ap_sig_bdd_81 or tmp_390_fu_265_p3)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81))) begin
        dataStreamBuffer1_V_din = dataStreamBuffer0_V_dout;
    end else if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & ~ap_sig_bdd_81)) begin
        dataStreamBuffer1_V_din = tmp_390_fu_265_p3;
    end else begin
        dataStreamBuffer1_V_din = 'bx;
    end
end

/// dataStreamBuffer1_V_write assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3 or ap_sig_bdd_81)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & ~ap_sig_bdd_81) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V) & ~ap_sig_bdd_81))) begin
        dataStreamBuffer1_V_write = ap_const_logic_1;
    end else begin
        dataStreamBuffer1_V_write = ap_const_logic_0;
    end
end

/// ici_wordCount_V_flag_1_phi_fu_166_p10 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3 or p_ici_wordCount_V_flag_fu_274_p2)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (((ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3)) | (~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V)) | (~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V))))) begin
        ici_wordCount_V_flag_1_phi_fu_166_p10 = p_ici_wordCount_V_flag_fu_274_p2;
    end else if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & (ap_const_lv1_0 == grp_nbreadreq_fu_88_p3)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V)))) begin
        ici_wordCount_V_flag_1_phi_fu_166_p10 = ap_const_lv1_0;
    end else begin
        ici_wordCount_V_flag_1_phi_fu_166_p10 = 'bx;
    end
end

/// ici_wordCount_V_flag_phi_fu_127_p6 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or ici_wordCount_V or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V)))) begin
        ici_wordCount_V_flag_phi_fu_127_p6 = ap_const_lv1_1;
    end else if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V))) begin
        ici_wordCount_V_flag_phi_fu_127_p6 = ap_const_lv1_0;
    end else begin
        ici_wordCount_V_flag_phi_fu_127_p6 = 'bx;
    end
end

/// ici_wordCount_V_new_phi_fu_141_p6 assign process. ///
always @ (ici_wordCount_V or ap_sig_bdd_199 or ap_sig_bdd_198)
begin
    if (ap_sig_bdd_198) begin
        if ((ap_const_lv4_0 == ici_wordCount_V)) begin
            ici_wordCount_V_new_phi_fu_141_p6 = ap_const_lv4_1;
        end else if (ap_sig_bdd_199) begin
            ici_wordCount_V_new_phi_fu_141_p6 = ap_const_lv4_2;
        end else begin
            ici_wordCount_V_new_phi_fu_141_p6 = 'bx;
        end
    end else begin
        ici_wordCount_V_new_phi_fu_141_p6 = 'bx;
    end
end
/// the next state (ap_NS_fsm) of the state machine. ///
always @ (ap_CS_fsm or ap_sig_bdd_81)
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


/// ap_sig_bdd_198 assign process. ///
always @ (ap_sig_cseq_ST_st1_fsm_0 or grp_nbreadreq_fu_88_p3)
begin
    ap_sig_bdd_198 = ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3));
end

/// ap_sig_bdd_199 assign process. ///
always @ (ici_wordCount_V or tmp_3_nbreadreq_fu_96_p3)
begin
    ap_sig_bdd_199 = ((ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3));
end

/// ap_sig_bdd_20 assign process. ///
always @ (ap_CS_fsm)
begin
    ap_sig_bdd_20 = (ap_CS_fsm[ap_const_lv32_0] == ap_const_lv1_1);
end

/// ap_sig_bdd_81 assign process. ///
always @ (ap_start or ap_done_reg or ici_wordCount_V or dataStreamBuffer0_V_empty_n or dataStreamBuffer1_V_full_n or checksumFifo_V_V_empty_n or grp_nbreadreq_fu_88_p3 or tmp_3_nbreadreq_fu_96_p3)
begin
    ap_sig_bdd_81 = (((dataStreamBuffer0_V_empty_n == ap_const_logic_0) & (ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3)) | ((ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & (checksumFifo_V_V_empty_n == ap_const_logic_0)) | ((ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ap_const_lv1_0 == tmp_3_nbreadreq_fu_96_p3) & (dataStreamBuffer1_V_full_n == ap_const_logic_0)) | ((dataStreamBuffer0_V_empty_n == ap_const_logic_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (ap_const_lv4_0 == ici_wordCount_V)) | (~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (dataStreamBuffer1_V_full_n == ap_const_logic_0) & (ap_const_lv4_0 == ici_wordCount_V)) | ((dataStreamBuffer0_V_empty_n == ap_const_logic_0) & ~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V)) | (~(ap_const_lv1_0 == grp_nbreadreq_fu_88_p3) & (dataStreamBuffer1_V_full_n == ap_const_logic_0) & ~(ici_wordCount_V == ap_const_lv4_1) & ~(ap_const_lv4_0 == ici_wordCount_V)) | (ap_start == ap_const_logic_0) | (ap_done_reg == ap_const_logic_1));
end
assign grp_fu_202_p3 = dataStreamBuffer0_V_dout[ap_const_lv32_48];
assign grp_nbreadreq_fu_88_p3 = dataStreamBuffer0_V_empty_n;
assign p_Result_s_21_fu_243_p5 = {{p_Val2_2_fu_217_p1[32'd63 : 32'd32]}, {tmp_s_fu_235_p3}, {p_Val2_2_fu_217_p1[32'd15 : 32'd0]}};
assign p_Result_s_fu_221_p4 = {{checksumFifo_V_V_dout[ap_const_lv32_F : ap_const_lv32_8]}};
assign p_Val2_2_fu_217_p1 = dataStreamBuffer0_V_dout[63:0];
assign p_ici_wordCount_V_flag_fu_274_p2 = (grp_fu_202_p3 | ici_wordCount_V_flag_phi_fu_127_p6);
assign p_ici_wordCount_V_new_fu_281_p3 = ((grp_fu_202_p3[0:0]===1'b1)? ap_const_lv4_0: ici_wordCount_V_new_phi_fu_141_p6);
assign tmp_110_fu_231_p1 = checksumFifo_V_V_dout[7:0];
assign tmp_24_fu_255_p4 = {{dataStreamBuffer0_V_dout[ap_const_lv32_48 : ap_const_lv32_40]}};
assign tmp_390_fu_265_p3 = {{tmp_24_fu_255_p4}, {p_Result_s_21_fu_243_p5}};
assign tmp_3_nbreadreq_fu_96_p3 = checksumFifo_V_V_empty_n;
assign tmp_s_fu_235_p3 = {{tmp_110_fu_231_p1}, {p_Result_s_fu_221_p4}};


endmodule //mac_ip_encode_ip_checksum_insert

