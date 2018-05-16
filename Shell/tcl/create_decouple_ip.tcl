
create_ip -name pr_decoupler -vendor xilinx.com -library ip -version 1.0 -module_name pr_decoupler_ROLE

set_property -dict [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF {Role_Udp_Tcp_McDp_4BEmif {ID 0 SIGNALS {
 piSHL_156_25Clk {DIRECTION in}
 piTOP_Reset {DIRECTION in} 
 #DECOUPLED 0 ?
 piTOP_250_00Clk {DIRECTION in} 
 #-- SHELL / Role / Nts0 / Udp Interface
 #---- Input AXI-Write Stream Interface ----------
 piSHL_Rol_Nts0_Udp_Axis_tdata {DIRECTION in WIDTH 64}
 piSHL_Rol_Nts0_Udp_Axis_tkeep {DIRECTION in WIDTH 8}
 piSHL_Rol_Nts0_Udp_Axis_tvalid {DIRECTION in}
 piSHL_Rol_Nts0_Udp_Axis_tlast {DIRECTION in}
 poROL_Shl_Nts0_Udp_Axis_tready {DIRECTION out}
 #---- Output AXI-Write Stream Interface ---------
 piSHL_Rol_Nts0_Udp_Axis_tready {DIRECTION in}
 poROL_Shl_Nts0_Udp_Axis_tdata {DIRECTION out WIDTH 64}
 poROL_Shl_Nts0_Udp_Axis_tkeep {DIRECTION out WIDTH 8}
 poROL_Shl_Nts0_Udp_Axis_tvalid {DIRECTION out}
 poROL_Shl_Nts0_Udp_Axis_tlast {DIRECTION out}
 #---- Input AXI-Write Stream Interface ----------
 piSHL_Rol_Nts0_Tcp_Axis_tdata {DIRECTION in WIDTH 64}
 piSHL_Rol_Nts0_Tcp_Axis_tkeep {DIRECTION in WIDTH 8}
 piSHL_Rol_Nts0_Tcp_Axis_tvalid {DIRECTION in}
 piSHL_Rol_Nts0_Tcp_Axis_tlast {DIRECTION in}
 poROL_Shl_Nts0_Tcp_Axis_tready {DIRECTION out}
 #---- Output AXI-Write Stream Interface ---------
 piSHL_Rol_Nts0_Tcp_Axis_tready {DIRECTION in}
 poROL_Shl_Nts0_Tcp_Axis_tdata {DIRECTION out WIDTH 64}
 poROL_Shl_Nts0_Tcp_Axis_tkeep {DIRECTION out WIDTH 8}
 poROL_Shl_Nts0_Tcp_Axis_tvalid {DIRECTION out}
 poROL_Shl_Nts0_Tcp_Axis_tlast {DIRECTION out}
 #-- ROLE EMIF Registers
 poROL_SHL_EMIF_2B_Reg {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xadde}
 piSHL_ROL_EMIF_2B_Reg {DIRECTION in WIDTH 16}
 #---- User Port #0 / S2MM-AXIS ------------------ 
 #------ Stream Read Command -----------------
 piSHL_Rol_Mem_Up0_Axis_RdCmd_tready {DIRECTION in}
 poROL_Shl_Mem_Up0_Axis_RdCmd_tdata {DIRECTION out WIDTH 72}
 poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid {DIRECTION out}
 #------ Stream Read Status ------------------
 piSHL_Rol_Mem_Up0_Axis_RdSts_tdata {DIRECTION in WIDTH 8}
 piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid {DIRECTION in}
 poROL_Shl_Mem_Up0_Axis_RdSts_tready {DIRECTION out}
 #------ Stream Data Input Channel -----------
 piSHL_Rol_Mem_Up0_Axis_Read_tdata {DIRECTION in WIDTH 512}
 piSHL_Rol_Mem_Up0_Axis_Read_tkeep {DIRECTION in WIDTH 64}
 piSHL_Rol_Mem_Up0_Axis_Read_tlast {DIRECTION in}
 piSHL_Rol_Mem_Up0_Axis_Read_tvalid {DIRECTION in}
 poROL_Shl_Mem_Up0_Axis_Read_tready {DIRECTION out}
 #------ Stream Write Command ----------------
 piSHL_Rol_Mem_Up0_Axis_WrCmd_tready {DIRECTION in}
 poROL_Shl_Mem_Up0_Axis_WrCmd_tdata {DIRECTION out WIDTH 72}
 poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid {DIRECTION out}
 #------ Stream Write Status -----------------
 piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid {DIRECTION in}
 piSHL_Rol_Mem_Up0_Axis_WrSts_tdata {DIRECTION in WIDTH 8}
 poROL_Shl_Mem_Up0_Axis_WrSts_tready {DIRECTION out}
 #------ Stream Data Output Channel ----------
 piSHL_Rol_Mem_Up0_Axis_Write_tready {DIRECTION in} 
 poROL_Shl_Mem_Up0_Axis_Write_tdata {DIRECTION out WIDTH 512}
 poROL_Shl_Mem_Up0_Axis_Write_tkeep {DIRECTION out WIDTH 64}
 poROL_Shl_Mem_Up0_Axis_Write_tlast {DIRECTION out}
 poROL_Shl_Mem_Up0_Axis_Write_tvalid {DIRECTION out}
 #-- SHELL / Role / Mem / Up1 Interface
 #---- User Port #1 / S2MM-AXIS ------------------ 
 #------ Stream Read Command -----------------
 piSHL_Rol_Mem_Up1_Axis_RdCmd_tready {DIRECTION in}
 poROL_Shl_Mem_Up1_Axis_RdCmd_tdata {DIRECTION out WIDTH 72}
 poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid {DIRECTION out}
 #------ Stream Read Status ------------------
 piSHL_Rol_Mem_Up1_Axis_RdSts_tdata {DIRECTION in WIDTH 8}
 piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid {DIRECTION in}
 poROL_Shl_Mem_Up1_Axis_RdSts_tready {DIRECTION out}
 #------ Stream Data Input Channel -----------
 piSHL_Rol_Mem_Up1_Axis_Read_tdata {DIRECTION in WIDTH 512}
 piSHL_Rol_Mem_Up1_Axis_Read_tkeep {DIRECTION in WIDTH 64}
 piSHL_Rol_Mem_Up1_Axis_Read_tlast {DIRECTION in}
 piSHL_Rol_Mem_Up1_Axis_Read_tvalid {DIRECTION in}
 poROL_Shl_Mem_Up1_Axis_Read_tready {DIRECTION out}
 #------ Stream Write Command ----------------
 piSHL_Rol_Mem_Up1_Axis_WrCmd_tready {DIRECTION in}
 poROL_Shl_Mem_Up1_Axis_WrCmd_tdata {DIRECTION out WIDTH 72}
 poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid {DIRECTION out}
 #------ Stream Write Status -----------------
 piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid {DIRECTION in}
 piSHL_Rol_Mem_Up1_Axis_WrSts_tdata {DIRECTION in WIDTH 8}
 poROL_Shl_Mem_Up1_Axis_WrSts_tready {DIRECTION out}
 #------ Stream Data Output Channel ----------
 piSHL_Rol_Mem_Up1_Axis_Write_tready {DIRECTION in} 
 poROL_Shl_Mem_Up1_Axis_Write_tdata {DIRECTION out WIDTH 512}
 poROL_Shl_Mem_Up1_Axis_Write_tkeep {DIRECTION out WIDTH 64}
 poROL_Shl_Mem_Up1_Axis_Write_tlast {DIRECTION out}
 poROL_Shl_Mem_Up1_Axis_Write_tvalid {DIRECTION out}
 }}}}] [get_ips pr_decoupler_ROLE]


generate_target {instantiation_template} [get_files /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

update_compile_order -fileset sources_1

generate_target all [get_files  /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

catch { config_ip_cache -export [get_ips -all pr_decoupler_ROLE] }

export_ip_user_files -of_objects [get_files /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci] -no_script -sync -force -quiet

create_ip_run [get_files -of_objects [get_fileset sources_1] /home/ngl/gitrepo/cloudFPGA/SRA3/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/pr_decoupler_ROLE/pr_decoupler_ROLE.xci]

launch_runs -jobs 8 pr_decoupler_ROLE_synth_1


#TODO 
# 1. adapt set property for the actual ROLE 
# -> should cover all signals, also Inputs 
# Some maybe should not be decoubled -> EDIT it seems not 
# 2. add commands for synthesis (see ./create_ip_cores.tcl? ) 

#        poROL_Shl_Nts0_Udp_Axis_tready {DIRECTION out} 
#        poROL_Shl_Nts0_Udp_Axis_tdata {WIDTH 64 DIRECTION out} 
#        poROL_Shl_Nts0_Udp_Axis_tkeep {DIRECTION out WIDTH 8} 
#        poROL_Shl_Nts0_Udp_Axis_tlast {DIRECTION out} 
#        poROL_Shl_Nts0_Tcp_Axis_tready {DIRECTION out} 
#        poROL_Shl_Nts0_Tcp_Axis_tdata {DIRECTION out WIDTH 64} 
#        poROL_Shl_Nts0_Tcp_Axis_tkeep {WIDTH 8 DIRECTION out} 
#        poROL_Shl_Nts0_Tcp_Axis_tvalid {DIRECTION out} 
#        poROL_Shl_Nts0_Tcp_Axis_tlast {DIRECTION out} 
#        poROL_SHL_EMIF_2B_Reg {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xdecb} 
#        poROL_Shl_Mem_Up0_Axis_RdCmd_tdata {DIRECTION out WIDTH 72}
#        piSHL_Rol_Mem_Up0_Axis_RdCmd_tready {DIRECTION in}
#        piTOP_Reset {DIRECTION in DECOUPLED 0}
#
      #}}}} CONFIG.Component_Name {pr_decoupler_ROLE} CONFIG.GUI_HAS_AXI_LITE {0} CONFIG.GUI_HAS_AXIS_CONTROL {0} CONFIG.GUI_SELECT_INTERFACE {0} CONFIG.GUI_INTERFACE_NAME {Role_Udp_Tcp_McDp_4BEmif} CONFIG.GUI_SELECT_VLNV {undef} CONFIG.GUI_SIGNAL_SELECT_0 {poROL_Shl_Nts0_Udp_Axis_tready} CONFIG.GUI_SIGNAL_SELECT_1 {poROL_Shl_Nts0_Udp_Axis_tdata} CONFIG.GUI_SIGNAL_SELECT_2 {poROL_Shl_Nts0_Udp_Axis_tkeep} CONFIG.GUI_SIGNAL_SELECT_3 {poROL_Shl_Nts0_Udp_Axis_tlast} CONFIG.GUI_SIGNAL_SELECT_4 {poROL_Shl_Nts0_Tcp_Axis_tready} CONFIG.GUI_SIGNAL_SELECT_5 {poROL_Shl_Nts0_Tcp_Axis_tdata} CONFIG.GUI_SIGNAL_SELECT_6 {poROL_Shl_Nts0_Tcp_Axis_tkeep} CONFIG.GUI_SIGNAL_SELECT_7 {poROL_Shl_Nts0_Tcp_Axis_tvalid} CONFIG.GUI_SIGNAL_SELECT_8 {poROL_Shl_Nts0_Tcp_Axis_tlast} CONFIG.GUI_SIGNAL_SELECT_9 {poROL_SHL_EMIF_2B_Reg} CONFIG.GUI_NEW_SIGNAL_NAME {poROL_Shl_Mem_Up0_Axis_RdCmd_tdata} CONFIG.GUI_SIGNAL_DIRECTION_0 {out} CONFIG.GUI_SIGNAL_DIRECTION_1 {out} CONFIG.GUI_SIGNAL_DIRECTION_2 {out} CONFIG.GUI_SIGNAL_DIRECTION_3 {out} CONFIG.GUI_SIGNAL_DIRECTION_4 {out} CONFIG.GUI_SIGNAL_DIRECTION_5 {out} CONFIG.GUI_SIGNAL_DIRECTION_6 {out} CONFIG.GUI_SIGNAL_DIRECTION_7 {out} CONFIG.GUI_SIGNAL_DIRECTION_8 {out} CONFIG.GUI_SIGNAL_DIRECTION_9 {out} CONFIG.GUI_SIGNAL_DECOUPLED_0 {true} CONFIG.GUI_SIGNAL_DECOUPLED_1 {true} CONFIG.GUI_SIGNAL_DECOUPLED_2 {true} CONFIG.GUI_SIGNAL_DECOUPLED_3 {true} CONFIG.GUI_SIGNAL_DECOUPLED_4 {true} CONFIG.GUI_SIGNAL_DECOUPLED_5 {true} CONFIG.GUI_SIGNAL_DECOUPLED_6 {true} CONFIG.GUI_SIGNAL_DECOUPLED_7 {true} CONFIG.GUI_SIGNAL_DECOUPLED_8 {true} CONFIG.GUI_SIGNAL_DECOUPLED_9 {true} CONFIG.GUI_SIGNAL_DELETE_0 {false} CONFIG.GUI_SIGNAL_PRESENT_0 {true} CONFIG.GUI_SIGNAL_PRESENT_1 {true} CONFIG.GUI_SIGNAL_PRESENT_2 {true} CONFIG.GUI_SIGNAL_PRESENT_3 {true} CONFIG.GUI_SIGNAL_PRESENT_4 {true} CONFIG.GUI_SIGNAL_PRESENT_5 {true} CONFIG.GUI_SIGNAL_PRESENT_6 {true} CONFIG.GUI_SIGNAL_PRESENT_7 {true} CONFIG.GUI_SIGNAL_PRESENT_8 {true} CONFIG.GUI_SIGNAL_PRESENT_9 {true} CONFIG.GUI_SIGNAL_WIDTH_1 {64} CONFIG.GUI_SIGNAL_WIDTH_2 {8} CONFIG.GUI_SIGNAL_WIDTH_5 {64} CONFIG.GUI_SIGNAL_WIDTH_6 {8} CONFIG.GUI_SIGNAL_WIDTH_7 {1} CONFIG.GUI_SIGNAL_WIDTH_8 {1} CONFIG.GUI_SIGNAL_WIDTH_9 {16} CONFIG.GUI_SIGNAL_DECOUPLED_VALUE_9 {0xdecb}] [get_ips pr_decoupler_ROLE]
#
#
