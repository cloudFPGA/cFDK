
create_ip -name pr_decoupler -vendor xilinx.com -library ip -version 1.0 -module_name pr_decoupler_ROLE

set_property -dict [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF {Role_Udp_Tcp_McDp_4BEmif {ID 0 SIGNALS { \
        piSHL_156_25Clk {DIRECTION in} \
        piTOP_Reset {DIRECTION in} \
        piTOP_250_00Clk {DIRECTION in} \
        piSHL_Rol_Nts0_Udp_Axis_tdata {DIRECTION in WIDTH 64} \
        piSHL_Rol_Nts0_Udp_Axis_tkeep {DIRECTION in WIDTH 8} \
        piSHL_Rol_Nts0_Udp_Axis_tvalid {DIRECTION in} \
        piSHL_Rol_Nts0_Udp_Axis_tlast {DIRECTION in} \
        poROL_Shl_Nts0_Udp_Axis_tready {DIRECTION out} \
        piSHL_Rol_Nts0_Udp_Axis_tready {DIRECTION in} \
        poROL_Shl_Nts0_Udp_Axis_tdata {DIRECTION out WIDTH 64} \
        poROL_Shl_Nts0_Udp_Axis_tkeep {DIRECTION out WIDTH 8} \
        poROL_Shl_Nts0_Udp_Axis_tvalid {DIRECTION out} \
        poROL_Shl_Nts0_Udp_Axis_tlast {DIRECTION out} \
        piSHL_Rol_Nts0_Tcp_Axis_tdata {DIRECTION in WIDTH 64} \
        piSHL_Rol_Nts0_Tcp_Axis_tkeep {DIRECTION in WIDTH 8} \
        piSHL_Rol_Nts0_Tcp_Axis_tvalid {DIRECTION in} \
        piSHL_Rol_Nts0_Tcp_Axis_tlast {DIRECTION in} \
        poROL_Shl_Nts0_Tcp_Axis_tready {DIRECTION out} \
        piSHL_Rol_Nts0_Tcp_Axis_tready {DIRECTION in} \
        poROL_Shl_Nts0_Tcp_Axis_tdata {DIRECTION out WIDTH 64} \
        poROL_Shl_Nts0_Tcp_Axis_tkeep {DIRECTION out WIDTH 8} \
        poROL_Shl_Nts0_Tcp_Axis_tvalid {DIRECTION out} \
        poROL_Shl_Nts0_Tcp_Axis_tlast {DIRECTION out} \
        poROL_SHL_EMIF_2B_Reg {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xadde} \
        piSHL_ROL_EMIF_2B_Reg {DIRECTION in WIDTH 16} \
        piSHL_Rol_Mem_Up0_Axis_RdCmd_tready {DIRECTION in} \
        poROL_Shl_Mem_Up0_Axis_RdCmd_tdata {DIRECTION out WIDTH 72} \
        poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid {DIRECTION out} \
        piSHL_Rol_Mem_Up0_Axis_RdSts_tdata {DIRECTION in WIDTH 8} \
        piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid {DIRECTION in} \
        poROL_Shl_Mem_Up0_Axis_RdSts_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up0_Axis_Read_tdata {DIRECTION in WIDTH 512} \
        piSHL_Rol_Mem_Up0_Axis_Read_tkeep {DIRECTION in WIDTH 64} \
        piSHL_Rol_Mem_Up0_Axis_Read_tlast {DIRECTION in} \
        piSHL_Rol_Mem_Up0_Axis_Read_tvalid {DIRECTION in} \
        poROL_Shl_Mem_Up0_Axis_Read_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up0_Axis_WrCmd_tready {DIRECTION in} \
        poROL_Shl_Mem_Up0_Axis_WrCmd_tdata {DIRECTION out WIDTH 72} \
        poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid {DIRECTION out} \
        piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid {DIRECTION in} \
        piSHL_Rol_Mem_Up0_Axis_WrSts_tdata {DIRECTION in WIDTH 8} \
        poROL_Shl_Mem_Up0_Axis_WrSts_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up0_Axis_Write_tready {DIRECTION in} \
        poROL_Shl_Mem_Up0_Axis_Write_tdata {DIRECTION out WIDTH 512} \
        poROL_Shl_Mem_Up0_Axis_Write_tkeep {DIRECTION out WIDTH 64} \
        poROL_Shl_Mem_Up0_Axis_Write_tlast {DIRECTION out} \
        poROL_Shl_Mem_Up0_Axis_Write_tvalid {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_RdCmd_tready {DIRECTION in} \
        poROL_Shl_Mem_Up1_Axis_RdCmd_tdata {DIRECTION out WIDTH 72} \
        poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_RdSts_tdata {DIRECTION in WIDTH 8} \
        piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid {DIRECTION in} \
        poROL_Shl_Mem_Up1_Axis_RdSts_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_Read_tdata {DIRECTION in WIDTH 512} \
        piSHL_Rol_Mem_Up1_Axis_Read_tkeep {DIRECTION in WIDTH 64} \
        piSHL_Rol_Mem_Up1_Axis_Read_tlast {DIRECTION in} \
        piSHL_Rol_Mem_Up1_Axis_Read_tvalid {DIRECTION in} \
        poROL_Shl_Mem_Up1_Axis_Read_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_WrCmd_tready {DIRECTION in} \
        poROL_Shl_Mem_Up1_Axis_WrCmd_tdata {DIRECTION out WIDTH 72} \
        poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid {DIRECTION in} \
        piSHL_Rol_Mem_Up1_Axis_WrSts_tdata {DIRECTION in WIDTH 8} \
        poROL_Shl_Mem_Up1_Axis_WrSts_tready {DIRECTION out} \
        piSHL_Rol_Mem_Up1_Axis_Write_tready {DIRECTION in} \
        poROL_Shl_Mem_Up1_Axis_Write_tdata {DIRECTION out WIDTH 512} \
        poROL_Shl_Mem_Up1_Axis_Write_tkeep {DIRECTION out WIDTH 64} \
        poROL_Shl_Mem_Up1_Axis_Write_tlast {DIRECTION out} \
        poROL_Shl_Mem_Up1_Axis_Write_tvalid {DIRECTION out} \
    }}}}] [get_ips pr_decoupler_ROLE] 


generate_target all [get_ips pr_decoupler_ROLE]


create_ip_run [get_ips pr_decoupler_ROLE]

#update_compile_order -fileset sources_1

