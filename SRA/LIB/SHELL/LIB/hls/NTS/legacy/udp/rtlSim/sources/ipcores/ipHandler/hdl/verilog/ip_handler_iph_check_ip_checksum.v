// ==============================================================
// RTL generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2014.4
// Copyright (C) 2014 Xilinx Inc. All rights reserved.
// 
// ===========================================================

`timescale 1 ns / 1 ps 

module ip_handler_iph_check_ip_checksum (
        ap_clk,
        ap_rst,
        ap_start,
        ap_done,
        ap_continue,
        ap_idle,
        ap_ready,
        iph_subSumsFifoOut_V_dout,
        iph_subSumsFifoOut_V_empty_n,
        iph_subSumsFifoOut_V_read,
        ipValidFifo_V_V_din,
        ipValidFifo_V_V_full_n,
        ipValidFifo_V_V_write
);

parameter    ap_const_logic_1 = 1'b1;
parameter    ap_const_logic_0 = 1'b0;
parameter    ap_ST_st1_fsm0_0 = 1'b1;
parameter    ap_ST_st2_fsm1_1 = 2'b10;
parameter    ap_ST_st0_fsm1_0 = 2'b1;
parameter    ap_const_lv32_0 = 32'b00000000000000000000000000000000;
parameter    ap_const_lv1_1 = 1'b1;
parameter    ap_const_lv3_0 = 3'b000;
parameter    ap_const_lv1_0 = 1'b0;
parameter    ap_const_lv3_3 = 3'b11;
parameter    ap_const_lv32_1 = 32'b1;
parameter    ap_const_lv3_2 = 3'b10;
parameter    ap_const_lv3_1 = 3'b1;
parameter    ap_const_lv2_1 = 2'b1;
parameter    ap_const_lv17_1FFFF = 17'b11111111111111111;
parameter    ap_const_lv16_0 = 16'b0000000000000000;
parameter    ap_const_lv32_10 = 32'b10000;
parameter    ap_const_lv32_11 = 32'b10001;
parameter    ap_const_lv32_21 = 32'b100001;
parameter    ap_const_lv32_22 = 32'b100010;
parameter    ap_const_lv32_32 = 32'b110010;
parameter    ap_const_lv32_33 = 32'b110011;
parameter    ap_const_lv32_43 = 32'b1000011;
parameter    ap_const_lv32_44 = 32'b1000100;
parameter    ap_true = 1'b1;

input   ap_clk;
input   ap_rst;
input   ap_start;
output   ap_done;
input   ap_continue;
output   ap_idle;
output   ap_ready;
input  [68:0] iph_subSumsFifoOut_V_dout;
input   iph_subSumsFifoOut_V_empty_n;
output   iph_subSumsFifoOut_V_read;
output  [0:0] ipValidFifo_V_V_din;
input   ipValidFifo_V_V_full_n;
output   ipValidFifo_V_V_write;

reg ap_done;
reg ap_idle;
reg ap_ready;
reg iph_subSumsFifoOut_V_read;
reg ipValidFifo_V_V_write;
reg    ap_done_reg = 1'b0;
(* fsm_encoding = "none" *) reg   [0:0] ap_CS_fsm0 = 1'b1;
reg    ap_sig_cseq_ST_st1_fsm0_0;
reg    ap_sig_bdd_23;
(* fsm_encoding = "none" *) reg   [1:0] ap_CS_fsm1 = 2'b1;
reg    ap_sig_cseq_ST_st0_fsm1_0;
reg    ap_sig_bdd_34;
wire   [2:0] tmp_cast_fu_132_p1;
wire   [0:0] tmp_nbreadreq_fu_94_p3;
reg    ap_sig_bdd_56;
reg   [2:0] tmp_cast_reg_370;
reg    ap_sig_bdd_67;
reg    ap_sig_cseq_ST_st2_fsm1_1;
reg    ap_sig_bdd_73;
reg   [1:0] cics_state_V = 2'b00;
reg   [16:0] icic_ip_sums_sum0_V = 17'b00000000000000000;
reg   [16:0] icic_ip_sums_sum1_V = 17'b00000000000000000;
reg   [16:0] icic_ip_sums_sum2_V = 17'b00000000000000000;
reg   [16:0] icic_ip_sums_sum3_V = 17'b00000000000000000;
reg   [0:0] icic_ip_sums_ipMatch = 1'b0;
wire   [0:0] tmp_34_fu_168_p2;
reg   [0:0] tmp_34_reg_374;
wire   [1:0] grp_fu_116_p2;
wire   [16:0] p_Val2_s_fu_152_p2;
wire   [16:0] p_8_cast_fu_202_p1;
wire   [16:0] p_cast_fu_246_p1;
wire   [16:0] tmp_124_fu_288_p1;
wire   [16:0] p_7_cast_fu_278_p1;
wire   [15:0] tmp_123_fu_164_p1;
wire   [16:0] tmp_32_fu_174_p2;
wire   [0:0] tmp_119_fu_180_p3;
wire   [15:0] tmp_120_fu_188_p1;
wire   [15:0] tmp_121_fu_192_p1;
wire   [15:0] tmp_122_fu_196_p2;
wire   [16:0] tmp_s_fu_212_p2;
wire   [0:0] tmp_111_fu_224_p3;
wire   [15:0] tmp_112_fu_232_p1;
wire   [15:0] tmp_113_fu_236_p1;
wire   [15:0] tmp_114_fu_240_p2;
wire   [16:0] tmp_30_fu_218_p2;
wire   [0:0] tmp_115_fu_256_p3;
wire   [15:0] tmp_116_fu_264_p1;
wire   [15:0] tmp_117_fu_268_p1;
wire   [15:0] tmp_118_fu_272_p2;
reg   [0:0] ap_NS_fsm0;
reg   [1:0] ap_NS_fsm1;
reg    ap_sig_bdd_51;
reg    ap_sig_bdd_80;




/// the current state (ap_CS_fsm0) of the state machine. ///
always @ (posedge ap_clk)
begin : ap_ret_ap_CS_fsm0
    if (ap_rst == 1'b1) begin
        ap_CS_fsm0 <= ap_ST_st1_fsm0_0;
    end else begin
        ap_CS_fsm0 <= ap_NS_fsm0;
    end
end

/// the current state (ap_CS_fsm1) of the state machine. ///
always @ (posedge ap_clk)
begin : ap_ret_ap_CS_fsm1
    if (ap_rst == 1'b1) begin
        ap_CS_fsm1 <= ap_ST_st0_fsm1_0;
    end else begin
        ap_CS_fsm1 <= ap_NS_fsm1;
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
        end else if (((ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_67))) begin
            ap_done_reg <= ap_const_logic_1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_80) begin
        if (ap_sig_bdd_51) begin
            icic_ip_sums_sum0_V <= tmp_124_fu_288_p1;
        end else if ((tmp_cast_fu_132_p1 == ap_const_lv3_1)) begin
            icic_ip_sums_sum0_V <= p_cast_fu_246_p1;
        end else if ((tmp_cast_fu_132_p1 == ap_const_lv3_2)) begin
            icic_ip_sums_sum0_V <= p_8_cast_fu_202_p1;
        end else if ((tmp_cast_fu_132_p1 == ap_const_lv3_3)) begin
            icic_ip_sums_sum0_V <= p_Val2_s_fu_152_p2;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (ap_sig_bdd_80) begin
        if (ap_sig_bdd_51) begin
            icic_ip_sums_sum1_V <= {{iph_subSumsFifoOut_V_dout[ap_const_lv32_21 : ap_const_lv32_11]}};
        end else if ((tmp_cast_fu_132_p1 == ap_const_lv3_1)) begin
            icic_ip_sums_sum1_V <= p_7_cast_fu_278_p1;
        end
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if ((((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))) & (tmp_cast_fu_132_p1 == ap_const_lv3_3)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & (tmp_cast_fu_132_p1 == ap_const_lv3_0) & ~(tmp_nbreadreq_fu_94_p3 == ap_const_lv1_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1)))) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))) & (tmp_cast_fu_132_p1 == ap_const_lv3_2)) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))) & (tmp_cast_fu_132_p1 == ap_const_lv3_1)))) begin
        cics_state_V <= grp_fu_116_p2;
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & (tmp_cast_fu_132_p1 == ap_const_lv3_0) & ~(tmp_nbreadreq_fu_94_p3 == ap_const_lv1_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))))) begin
        icic_ip_sums_ipMatch <= iph_subSumsFifoOut_V_dout[ap_const_lv32_44];
        icic_ip_sums_sum2_V <= {{iph_subSumsFifoOut_V_dout[ap_const_lv32_32 : ap_const_lv32_22]}};
        icic_ip_sums_sum3_V <= {{iph_subSumsFifoOut_V_dout[ap_const_lv32_43 : ap_const_lv32_33]}};
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))) & (tmp_cast_fu_132_p1 == ap_const_lv3_3))) begin
        tmp_34_reg_374 <= tmp_34_fu_168_p2;
    end
end

/// assign process. ///
always @(posedge ap_clk)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))))) begin
        tmp_cast_reg_370[0] <= tmp_cast_fu_132_p1[0];
tmp_cast_reg_370[1] <= tmp_cast_fu_132_p1[1];
    end
end

/// ap_done assign process. ///
always @ (ap_done_reg or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    if (((ap_const_logic_1 == ap_done_reg) | ((ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_67)))) begin
        ap_done = ap_const_logic_1;
    end else begin
        ap_done = ap_const_logic_0;
    end
end

/// ap_idle assign process. ///
always @ (ap_start or ap_sig_cseq_ST_st1_fsm0_0 or ap_sig_cseq_ST_st0_fsm1_0)
begin
    if ((~(ap_const_logic_1 == ap_start) & (ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & (ap_const_logic_1 == ap_sig_cseq_ST_st0_fsm1_0))) begin
        ap_idle = ap_const_logic_1;
    end else begin
        ap_idle = ap_const_logic_0;
    end
end

/// ap_ready assign process. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm0_0 or ap_sig_bdd_56 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))))) begin
        ap_ready = ap_const_logic_1;
    end else begin
        ap_ready = ap_const_logic_0;
    end
end

/// ap_sig_cseq_ST_st0_fsm1_0 assign process. ///
always @ (ap_sig_bdd_34)
begin
    if (ap_sig_bdd_34) begin
        ap_sig_cseq_ST_st0_fsm1_0 = ap_const_logic_1;
    end else begin
        ap_sig_cseq_ST_st0_fsm1_0 = ap_const_logic_0;
    end
end

/// ap_sig_cseq_ST_st1_fsm0_0 assign process. ///
always @ (ap_sig_bdd_23)
begin
    if (ap_sig_bdd_23) begin
        ap_sig_cseq_ST_st1_fsm0_0 = ap_const_logic_1;
    end else begin
        ap_sig_cseq_ST_st1_fsm0_0 = ap_const_logic_0;
    end
end

/// ap_sig_cseq_ST_st2_fsm1_1 assign process. ///
always @ (ap_sig_bdd_73)
begin
    if (ap_sig_bdd_73) begin
        ap_sig_cseq_ST_st2_fsm1_1 = ap_const_logic_1;
    end else begin
        ap_sig_cseq_ST_st2_fsm1_1 = ap_const_logic_0;
    end
end

/// ipValidFifo_V_V_write assign process. ///
always @ (ap_done_reg or tmp_cast_reg_370 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    if (((tmp_cast_reg_370 == ap_const_lv3_3) & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_67))) begin
        ipValidFifo_V_V_write = ap_const_logic_1;
    end else begin
        ipValidFifo_V_V_write = ap_const_logic_0;
    end
end

/// iph_subSumsFifoOut_V_read assign process. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm0_0 or tmp_cast_fu_132_p1 or tmp_nbreadreq_fu_94_p3 or ap_sig_bdd_56 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & (tmp_cast_fu_132_p1 == ap_const_lv3_0) & ~(tmp_nbreadreq_fu_94_p3 == ap_const_lv1_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))))) begin
        iph_subSumsFifoOut_V_read = ap_const_logic_1;
    end else begin
        iph_subSumsFifoOut_V_read = ap_const_logic_0;
    end
end
/// the next state (ap_NS_fsm1) of the state machine. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm0_0 or ap_CS_fsm1 or ap_sig_bdd_56 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    case (ap_CS_fsm1)
        ap_ST_st2_fsm1_1 : 
        begin
            if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_67) & ~ap_sig_bdd_56)) begin
                ap_NS_fsm1 = ap_ST_st2_fsm1_1;
            end else if ((~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_67) & (~(ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) | ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ap_sig_bdd_56)))) begin
                ap_NS_fsm1 = ap_ST_st0_fsm1_0;
            end else begin
                ap_NS_fsm1 = ap_ST_st2_fsm1_1;
            end
        end
        ap_ST_st0_fsm1_0 : 
        begin
            if (((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))))) begin
                ap_NS_fsm1 = ap_ST_st2_fsm1_1;
            end else begin
                ap_NS_fsm1 = ap_ST_st0_fsm1_0;
            end
        end
        default : 
        begin
            ap_NS_fsm1 = 'bx;
        end
    endcase
end

/// the next state (ap_NS_fsm0) of the state machine. ///
always @ (ap_done_reg or ap_CS_fsm0 or ap_sig_bdd_56 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    case (ap_CS_fsm0)
        ap_ST_st1_fsm0_0 : 
        begin
            ap_NS_fsm0 = ap_ST_st1_fsm0_0;
        end
        default : 
        begin
            ap_NS_fsm0 = 'bx;
        end
    endcase
end


/// ap_sig_bdd_23 assign process. ///
always @ (ap_CS_fsm0)
begin
    ap_sig_bdd_23 = (ap_CS_fsm0[ap_const_lv32_0] == ap_const_lv1_1);
end

/// ap_sig_bdd_34 assign process. ///
always @ (ap_CS_fsm1)
begin
    ap_sig_bdd_34 = (ap_const_lv1_1 == ap_CS_fsm1[ap_const_lv32_0]);
end

/// ap_sig_bdd_51 assign process. ///
always @ (tmp_cast_fu_132_p1 or tmp_nbreadreq_fu_94_p3)
begin
    ap_sig_bdd_51 = ((tmp_cast_fu_132_p1 == ap_const_lv3_0) & ~(tmp_nbreadreq_fu_94_p3 == ap_const_lv1_0));
end

/// ap_sig_bdd_56 assign process. ///
always @ (ap_start or ap_done_reg or iph_subSumsFifoOut_V_empty_n or tmp_cast_fu_132_p1 or tmp_nbreadreq_fu_94_p3)
begin
    ap_sig_bdd_56 = (((iph_subSumsFifoOut_V_empty_n == ap_const_logic_0) & (tmp_cast_fu_132_p1 == ap_const_lv3_0) & ~(tmp_nbreadreq_fu_94_p3 == ap_const_lv1_0)) | (ap_start == ap_const_logic_0) | (ap_done_reg == ap_const_logic_1));
end

/// ap_sig_bdd_67 assign process. ///
always @ (ipValidFifo_V_V_full_n or tmp_cast_reg_370)
begin
    ap_sig_bdd_67 = ((ipValidFifo_V_V_full_n == ap_const_logic_0) & (tmp_cast_reg_370 == ap_const_lv3_3));
end

/// ap_sig_bdd_73 assign process. ///
always @ (ap_CS_fsm1)
begin
    ap_sig_bdd_73 = (ap_const_lv1_1 == ap_CS_fsm1[ap_const_lv32_1]);
end

/// ap_sig_bdd_80 assign process. ///
always @ (ap_done_reg or ap_sig_cseq_ST_st1_fsm0_0 or ap_sig_bdd_56 or ap_sig_bdd_67 or ap_sig_cseq_ST_st2_fsm1_1)
begin
    ap_sig_bdd_80 = ((ap_const_logic_1 == ap_sig_cseq_ST_st1_fsm0_0) & ~((ap_done_reg == ap_const_logic_1) | ap_sig_bdd_56 | (ap_sig_bdd_67 & (ap_const_logic_1 == ap_sig_cseq_ST_st2_fsm1_1))));
end
assign grp_fu_116_p2 = (cics_state_V + ap_const_lv2_1);
assign ipValidFifo_V_V_din = (tmp_34_reg_374 & icic_ip_sums_ipMatch);
assign p_7_cast_fu_278_p1 = tmp_118_fu_272_p2;
assign p_8_cast_fu_202_p1 = tmp_122_fu_196_p2;
assign p_Val2_s_fu_152_p2 = (icic_ip_sums_sum0_V ^ ap_const_lv17_1FFFF);
assign p_cast_fu_246_p1 = tmp_114_fu_240_p2;
assign tmp_111_fu_224_p3 = tmp_s_fu_212_p2[ap_const_lv32_10];
assign tmp_112_fu_232_p1 = tmp_111_fu_224_p3;
assign tmp_113_fu_236_p1 = tmp_s_fu_212_p2[15:0];
assign tmp_114_fu_240_p2 = (tmp_112_fu_232_p1 + tmp_113_fu_236_p1);
assign tmp_115_fu_256_p3 = tmp_30_fu_218_p2[ap_const_lv32_10];
assign tmp_116_fu_264_p1 = tmp_115_fu_256_p3;
assign tmp_117_fu_268_p1 = tmp_30_fu_218_p2[15:0];
assign tmp_118_fu_272_p2 = (tmp_116_fu_264_p1 + tmp_117_fu_268_p1);
assign tmp_119_fu_180_p3 = tmp_32_fu_174_p2[ap_const_lv32_10];
assign tmp_120_fu_188_p1 = tmp_119_fu_180_p3;
assign tmp_121_fu_192_p1 = tmp_32_fu_174_p2[15:0];
assign tmp_122_fu_196_p2 = (tmp_120_fu_188_p1 + tmp_121_fu_192_p1);
assign tmp_123_fu_164_p1 = p_Val2_s_fu_152_p2[15:0];
assign tmp_124_fu_288_p1 = iph_subSumsFifoOut_V_dout[16:0];
assign tmp_30_fu_218_p2 = (icic_ip_sums_sum1_V + icic_ip_sums_sum3_V);
assign tmp_32_fu_174_p2 = (icic_ip_sums_sum0_V + icic_ip_sums_sum1_V);
assign tmp_34_fu_168_p2 = (tmp_123_fu_164_p1 == ap_const_lv16_0? 1'b1: 1'b0);
assign tmp_cast_fu_132_p1 = cics_state_V;
assign tmp_nbreadreq_fu_94_p3 = iph_subSumsFifoOut_V_empty_n;
assign tmp_s_fu_212_p2 = (icic_ip_sums_sum0_V + icic_ip_sums_sum2_V);
always @ (posedge ap_clk)
begin
    tmp_cast_reg_370[2] <= 1'b0;
end



endmodule //ip_handler_iph_check_ip_checksum
