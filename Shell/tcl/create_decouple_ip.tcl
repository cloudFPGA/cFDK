
create_ip -name pr_decoupler -vendor xilinx.com -library ip -version 1.0 -module_name pr_decoupler_ROLE

set_property -dict [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF {ROLE {ID 0 SIGNALS {poROL_Shl_Nts0_Udp_Axis_tready {DIRECTION out} poROL_Shl_Nts0_Udp_Axis_tdata {WIDTH 64 DIRECTION out} poROL_Shl_Nts0_Udp_Axis_tkeep {DIRECTION out WIDTH 8} poROL_Shl_Nts0_Udp_Axis_tlast {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tready {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tdata {DIRECTION out WIDTH 64} poROL_Shl_Nts0_Tcp_Axis_tkeep {WIDTH 8 DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tvalid {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tlast {DIRECTION out} poROL_SHL_EMIF_2B_Reg {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xdecb} poROL_Shl_Mem_Up0_Axis_RdCmd_tdata {}}}}} CONFIG.Component_Name {pr_decoupler_ROLE} CONFIG.GUI_HAS_AXI_LITE {0} CONFIG.GUI_HAS_AXIS_CONTROL {0} CONFIG.GUI_SELECT_INTERFACE {0} CONFIG.GUI_INTERFACE_NAME {ROLE} CONFIG.GUI_SELECT_VLNV {undef} CONFIG.GUI_SIGNAL_SELECT_0 {poROL_Shl_Nts0_Udp_Axis_tready} CONFIG.GUI_SIGNAL_SELECT_1 {poROL_Shl_Nts0_Udp_Axis_tdata} CONFIG.GUI_SIGNAL_SELECT_2 {poROL_Shl_Nts0_Udp_Axis_tkeep} CONFIG.GUI_SIGNAL_SELECT_3 {poROL_Shl_Nts0_Udp_Axis_tlast} CONFIG.GUI_SIGNAL_SELECT_4 {poROL_Shl_Nts0_Tcp_Axis_tready} CONFIG.GUI_SIGNAL_SELECT_5 {poROL_Shl_Nts0_Tcp_Axis_tdata} CONFIG.GUI_SIGNAL_SELECT_6 {poROL_Shl_Nts0_Tcp_Axis_tkeep} CONFIG.GUI_SIGNAL_SELECT_7 {poROL_Shl_Nts0_Tcp_Axis_tvalid} CONFIG.GUI_SIGNAL_SELECT_8 {poROL_Shl_Nts0_Tcp_Axis_tlast} CONFIG.GUI_SIGNAL_SELECT_9 {poROL_SHL_EMIF_2B_Reg} CONFIG.GUI_NEW_SIGNAL_NAME {poROL_Shl_Mem_Up0_Axis_RdCmd_tdata} CONFIG.GUI_SIGNAL_DIRECTION_0 {out} CONFIG.GUI_SIGNAL_DIRECTION_1 {out} CONFIG.GUI_SIGNAL_DIRECTION_2 {out} CONFIG.GUI_SIGNAL_DIRECTION_3 {out} CONFIG.GUI_SIGNAL_DIRECTION_4 {out} CONFIG.GUI_SIGNAL_DIRECTION_5 {out} CONFIG.GUI_SIGNAL_DIRECTION_6 {out} CONFIG.GUI_SIGNAL_DIRECTION_7 {out} CONFIG.GUI_SIGNAL_DIRECTION_8 {out} CONFIG.GUI_SIGNAL_DIRECTION_9 {out} CONFIG.GUI_SIGNAL_DECOUPLED_0 {true} CONFIG.GUI_SIGNAL_DECOUPLED_1 {true} CONFIG.GUI_SIGNAL_DECOUPLED_2 {true} CONFIG.GUI_SIGNAL_DECOUPLED_3 {true} CONFIG.GUI_SIGNAL_DECOUPLED_4 {true} CONFIG.GUI_SIGNAL_DECOUPLED_5 {true} CONFIG.GUI_SIGNAL_DECOUPLED_6 {true} CONFIG.GUI_SIGNAL_DECOUPLED_7 {true} CONFIG.GUI_SIGNAL_DECOUPLED_8 {true} CONFIG.GUI_SIGNAL_DECOUPLED_9 {true} CONFIG.GUI_SIGNAL_DELETE_0 {false} CONFIG.GUI_SIGNAL_PRESENT_0 {true} CONFIG.GUI_SIGNAL_PRESENT_1 {true} CONFIG.GUI_SIGNAL_PRESENT_2 {true} CONFIG.GUI_SIGNAL_PRESENT_3 {true} CONFIG.GUI_SIGNAL_PRESENT_4 {true} CONFIG.GUI_SIGNAL_PRESENT_5 {true} CONFIG.GUI_SIGNAL_PRESENT_6 {true} CONFIG.GUI_SIGNAL_PRESENT_7 {true} CONFIG.GUI_SIGNAL_PRESENT_8 {true} CONFIG.GUI_SIGNAL_PRESENT_9 {true} CONFIG.GUI_SIGNAL_WIDTH_1 {64} CONFIG.GUI_SIGNAL_WIDTH_2 {8} CONFIG.GUI_SIGNAL_WIDTH_5 {64} CONFIG.GUI_SIGNAL_WIDTH_6 {8} CONFIG.GUI_SIGNAL_WIDTH_7 {1} CONFIG.GUI_SIGNAL_WIDTH_8 {1} CONFIG.GUI_SIGNAL_WIDTH_9 {16} CONFIG.GUI_SIGNAL_DECOUPLED_VALUE_9 {0xdecb}] [get_ips pr_decoupler_ROLE]


generate_target {instantiation_template} [get_files /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

update_compile_order -fileset sources_1

generate_target all [get_files  /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

catch { config_ip_cache -export [get_ips -all pr_decoupler_ROLE] }

export_ip_user_files -of_objects [get_files /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci] -no_script -sync -force -quiet

create_ip_run [get_files -of_objects [get_fileset sources_1] /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

launch_runs -jobs 8 pr_decoupler_ROLE_synth_1

set_property -dict [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF {ROLE {ID 0 SIGNALS {poROL_Shl_Nts0_Udp_Axis_tready {DIRECTION out} poROL_Shl_Nts0_Udp_Axis_tdata {WIDTH 64 DIRECTION out} poROL_Shl_Nts0_Udp_Axis_tkeep {DIRECTION out WIDTH 8} poROL_Shl_Nts0_Udp_Axis_tlast {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tready {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tdata {DIRECTION out WIDTH 64} poROL_Shl_Nts0_Tcp_Axis_tkeep {WIDTH 8 DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tvalid {DIRECTION out} poROL_Shl_Nts0_Tcp_Axis_tlast {DIRECTION out} poROL_SHL_EMIF_2B_Reg {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xdecb} poROL_Shl_Mem_Up0_Axis_RdCmd_tdata {DIRECTION out WIDTH 72} piSHL_Rol_Mem_Up0_Axis_RdCmd_tready {DIRECTION in} piTOP_Reset {DIRECTION in DECOUPLED 0}}}}} CONFIG.GUI_SIGNAL_SELECT_0 {poROL_Shl_Mem_Up0_Axis_RdCmd_tdata} CONFIG.GUI_SIGNAL_SELECT_1 {piSHL_Rol_Mem_Up0_Axis_RdCmd_tready} CONFIG.GUI_SIGNAL_SELECT_2 {piTOP_Reset} CONFIG.GUI_NEW_SIGNAL_NAME { } CONFIG.GUI_SIGNAL_DIRECTION_0 {out} CONFIG.GUI_SIGNAL_DIRECTION_1 {in} CONFIG.GUI_SIGNAL_DIRECTION_2 {in} CONFIG.GUI_SIGNAL_DECOUPLED_2 {false} CONFIG.GUI_SIGNAL_WIDTH_0 {72} CONFIG.GUI_SIGNAL_WIDTH_1 {1} CONFIG.GUI_SIGNAL_WIDTH_2 {1}] [get_ips pr_decoupler_ROLE]

#TODO 
# 1. adapt set property for the actual ROLE 
# -> should cover all signals, also Inputs 
# Some maybe should not be decoubled -> EDIT it seems not 
# 2. add commands for synthesis (see ./create_ip_cores.tcl? ) 


