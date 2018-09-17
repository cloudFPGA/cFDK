# ******************************************************************************
# *
# *                        Zurich cloudFPGA
# *            All rights reserved -- Property of IBM
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Default physical constraint file for the FMKU2595 equipped with a
# * 	      XCKU060.
# * File    : top_FMKU60_Flash.xdc
# *
# * Created : Oct. 2017
# * Authors : Francois Abel <fab@zurich.ibm.com>
# *
# * Devices : xcku060-ffva1156-2-i
# * Tools   : Vivado v2016.4, v2017.4 (64-bit)
# * Depends : None
# *
# * Description : This file contains all the default physical constraints for
# *     operating a XCKU060 on a FMKU2595 module. The constraints are grouped
# *      by external device and connector entity names:
# *       - the synchronous dynamic random access memory (SDRM)
# *       - the programmable system on chip controller (PSOC)
# *       - the configuration Flash memory (FLASH)
# *       - the clock tree generator (CLKT)
# *       - the edge backplane connector (ECON)
# *       - the top extension connector (XCON)
# *
# *     The SDRM has the following interfaces:
# *       - a memory channel 0 (SDRM_Mch0)
# *       - a memory channel 1 (SDRM_Mch1)
# *     The PSOC has the following interfaces:
# *       - an External memory interface (PSOC_Emif)
# *       - an Fpga configuration interface (PSOC_Fcfg)
# *     The FLASH has the following interfaces:
# *       - a byte peripheral interface (FLASH_Bpi)
# *     The CLKT has the following interfaces:
# *       -
# *     The ECON has the following interfaces:
# *     The XCON has the following interfaces:
# *
# *
# *-----------------------------------------------------------------------------
# * Comments:
# *
# ******************************************************************************

#---------------------------------------------------------------------
# Setting CONFIG_VOLTAGE and CFGBVS
#   To avoid some of the DRC warnings during Bitstream generation
#---------------------------------------------------------------------
set_property CONFIG_VOLTAGE 1.8 [current_design]
set_property CFGBVS GND [current_design]

#---------------------------------------------------------------------
# Enable bitstream compression
#---------------------------------------------------------------------
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

#---------------------------------------------------------------------
# Settings for the BPI Flash Programming
#
#  [INFO] Run the following command to generate a '.mcs' file for a
#         1Gbit (i.e. 128x8) BPI configuration flash.
#           write_cfgmem -format mcs -size 128 -interface BPIx16  /
#                        -loadbit {up 0x00000000 ${xprName}.bit } /
#                        -file ${xprName}.mcs
#---------------------------------------------------------------------
set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN DIV-1 [current_design]
set_property BITSTREAM.CONFIG.BPI_SYNC_MODE TYPE1 [current_design]
set_property CONFIG_MODE BPI16 [current_design]



#OBSOLETE-20180418 set_property CLKFBOUT_MULT_F 2 [get_cells SHELL/MEM/MC0/MCC/inst/u_ddr4_infrastructure/gen_mmcme3.u_mmcme_adv_inst]
#OBSOLETE-20180418 set_property CLKFBOUT_MULT_F 2 [get_cells SHELL/MEM/MC1/MCC/inst/u_ddr4_infrastructure/gen_mmcme3.u_mmcme_adv_inst]



#---------------------------------------------------------------------
# Define Specific Physical Blocks (PBLOCK) for the Memory Channels
#---------------------------------------------------------------------
#OBSOLETE-20180419 create_pblock pblock_MC0
#OBSOLETE-20180419 add_cells_to_pblock [get_pblocks pblock_MC0] [get_cells -quiet [list SHELL/inst/MEM/MC0]]
#OBSOLETE-20180419 resize_pblock [get_pblocks pblock_MC0] -add {CLOCKREGION_X1Y0:CLOCKREGION_X2Y2}

#OBSOLETE-20180419 create_pblock pblock_MC1
#OBSOLETE-20180419 add_cells_to_pblock [get_pblocks pblock_MC1] [get_cells -quiet [list SHELL/inst/MEM/MC1]]
#OBSOLETE-20180419 resize_pblock [get_pblocks pblock_MC1] -add {CLOCKREGION_X3Y2:CLOCKREGION_X4Y4}







create_debug_core u_ila_0 ila
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_0]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_0]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_0]
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_0]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_0]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_0]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_0]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_0]
set_property port_width 1 [get_debug_ports u_ila_0/clk]
connect_debug_port u_ila_0/clk [get_nets [list SHELL/SuperCfg.ETH0/ETH/CORE/IP/U0/xpcs/U0/ten_gig_eth_pcs_pma_shared_clock_reset_block/CLK]]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe0]
set_property port_width 64 [get_debug_ports u_ila_0/probe0]
connect_debug_port u_ila_0/probe0 [get_nets [list {sROL_Shl_Nts0_Udp_Axis_tdata[0]} {sROL_Shl_Nts0_Udp_Axis_tdata[1]} {sROL_Shl_Nts0_Udp_Axis_tdata[2]} {sROL_Shl_Nts0_Udp_Axis_tdata[3]} {sROL_Shl_Nts0_Udp_Axis_tdata[4]} {sROL_Shl_Nts0_Udp_Axis_tdata[5]} {sROL_Shl_Nts0_Udp_Axis_tdata[6]} {sROL_Shl_Nts0_Udp_Axis_tdata[7]} {sROL_Shl_Nts0_Udp_Axis_tdata[8]} {sROL_Shl_Nts0_Udp_Axis_tdata[9]} {sROL_Shl_Nts0_Udp_Axis_tdata[10]} {sROL_Shl_Nts0_Udp_Axis_tdata[11]} {sROL_Shl_Nts0_Udp_Axis_tdata[12]} {sROL_Shl_Nts0_Udp_Axis_tdata[13]} {sROL_Shl_Nts0_Udp_Axis_tdata[14]} {sROL_Shl_Nts0_Udp_Axis_tdata[15]} {sROL_Shl_Nts0_Udp_Axis_tdata[16]} {sROL_Shl_Nts0_Udp_Axis_tdata[17]} {sROL_Shl_Nts0_Udp_Axis_tdata[18]} {sROL_Shl_Nts0_Udp_Axis_tdata[19]} {sROL_Shl_Nts0_Udp_Axis_tdata[20]} {sROL_Shl_Nts0_Udp_Axis_tdata[21]} {sROL_Shl_Nts0_Udp_Axis_tdata[22]} {sROL_Shl_Nts0_Udp_Axis_tdata[23]} {sROL_Shl_Nts0_Udp_Axis_tdata[24]} {sROL_Shl_Nts0_Udp_Axis_tdata[25]} {sROL_Shl_Nts0_Udp_Axis_tdata[26]} {sROL_Shl_Nts0_Udp_Axis_tdata[27]} {sROL_Shl_Nts0_Udp_Axis_tdata[28]} {sROL_Shl_Nts0_Udp_Axis_tdata[29]} {sROL_Shl_Nts0_Udp_Axis_tdata[30]} {sROL_Shl_Nts0_Udp_Axis_tdata[31]} {sROL_Shl_Nts0_Udp_Axis_tdata[32]} {sROL_Shl_Nts0_Udp_Axis_tdata[33]} {sROL_Shl_Nts0_Udp_Axis_tdata[34]} {sROL_Shl_Nts0_Udp_Axis_tdata[35]} {sROL_Shl_Nts0_Udp_Axis_tdata[36]} {sROL_Shl_Nts0_Udp_Axis_tdata[37]} {sROL_Shl_Nts0_Udp_Axis_tdata[38]} {sROL_Shl_Nts0_Udp_Axis_tdata[39]} {sROL_Shl_Nts0_Udp_Axis_tdata[40]} {sROL_Shl_Nts0_Udp_Axis_tdata[41]} {sROL_Shl_Nts0_Udp_Axis_tdata[42]} {sROL_Shl_Nts0_Udp_Axis_tdata[43]} {sROL_Shl_Nts0_Udp_Axis_tdata[44]} {sROL_Shl_Nts0_Udp_Axis_tdata[45]} {sROL_Shl_Nts0_Udp_Axis_tdata[46]} {sROL_Shl_Nts0_Udp_Axis_tdata[47]} {sROL_Shl_Nts0_Udp_Axis_tdata[48]} {sROL_Shl_Nts0_Udp_Axis_tdata[49]} {sROL_Shl_Nts0_Udp_Axis_tdata[50]} {sROL_Shl_Nts0_Udp_Axis_tdata[51]} {sROL_Shl_Nts0_Udp_Axis_tdata[52]} {sROL_Shl_Nts0_Udp_Axis_tdata[53]} {sROL_Shl_Nts0_Udp_Axis_tdata[54]} {sROL_Shl_Nts0_Udp_Axis_tdata[55]} {sROL_Shl_Nts0_Udp_Axis_tdata[56]} {sROL_Shl_Nts0_Udp_Axis_tdata[57]} {sROL_Shl_Nts0_Udp_Axis_tdata[58]} {sROL_Shl_Nts0_Udp_Axis_tdata[59]} {sROL_Shl_Nts0_Udp_Axis_tdata[60]} {sROL_Shl_Nts0_Udp_Axis_tdata[61]} {sROL_Shl_Nts0_Udp_Axis_tdata[62]} {sROL_Shl_Nts0_Udp_Axis_tdata[63]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe1]
set_property port_width 8 [get_debug_ports u_ila_0/probe1]
connect_debug_port u_ila_0/probe1 [get_nets [list {sROL_Shl_Nts0_Udp_Axis_tkeep[0]} {sROL_Shl_Nts0_Udp_Axis_tkeep[1]} {sROL_Shl_Nts0_Udp_Axis_tkeep[2]} {sROL_Shl_Nts0_Udp_Axis_tkeep[3]} {sROL_Shl_Nts0_Udp_Axis_tkeep[4]} {sROL_Shl_Nts0_Udp_Axis_tkeep[5]} {sROL_Shl_Nts0_Udp_Axis_tkeep[6]} {sROL_Shl_Nts0_Udp_Axis_tkeep[7]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe2]
set_property port_width 64 [get_debug_ports u_ila_0/probe2]
connect_debug_port u_ila_0/probe2 [get_nets [list {sSHL_Rol_Nts0_Udp_Axis_tdata[0]} {sSHL_Rol_Nts0_Udp_Axis_tdata[1]} {sSHL_Rol_Nts0_Udp_Axis_tdata[2]} {sSHL_Rol_Nts0_Udp_Axis_tdata[3]} {sSHL_Rol_Nts0_Udp_Axis_tdata[4]} {sSHL_Rol_Nts0_Udp_Axis_tdata[5]} {sSHL_Rol_Nts0_Udp_Axis_tdata[6]} {sSHL_Rol_Nts0_Udp_Axis_tdata[7]} {sSHL_Rol_Nts0_Udp_Axis_tdata[8]} {sSHL_Rol_Nts0_Udp_Axis_tdata[9]} {sSHL_Rol_Nts0_Udp_Axis_tdata[10]} {sSHL_Rol_Nts0_Udp_Axis_tdata[11]} {sSHL_Rol_Nts0_Udp_Axis_tdata[12]} {sSHL_Rol_Nts0_Udp_Axis_tdata[13]} {sSHL_Rol_Nts0_Udp_Axis_tdata[14]} {sSHL_Rol_Nts0_Udp_Axis_tdata[15]} {sSHL_Rol_Nts0_Udp_Axis_tdata[16]} {sSHL_Rol_Nts0_Udp_Axis_tdata[17]} {sSHL_Rol_Nts0_Udp_Axis_tdata[18]} {sSHL_Rol_Nts0_Udp_Axis_tdata[19]} {sSHL_Rol_Nts0_Udp_Axis_tdata[20]} {sSHL_Rol_Nts0_Udp_Axis_tdata[21]} {sSHL_Rol_Nts0_Udp_Axis_tdata[22]} {sSHL_Rol_Nts0_Udp_Axis_tdata[23]} {sSHL_Rol_Nts0_Udp_Axis_tdata[24]} {sSHL_Rol_Nts0_Udp_Axis_tdata[25]} {sSHL_Rol_Nts0_Udp_Axis_tdata[26]} {sSHL_Rol_Nts0_Udp_Axis_tdata[27]} {sSHL_Rol_Nts0_Udp_Axis_tdata[28]} {sSHL_Rol_Nts0_Udp_Axis_tdata[29]} {sSHL_Rol_Nts0_Udp_Axis_tdata[30]} {sSHL_Rol_Nts0_Udp_Axis_tdata[31]} {sSHL_Rol_Nts0_Udp_Axis_tdata[32]} {sSHL_Rol_Nts0_Udp_Axis_tdata[33]} {sSHL_Rol_Nts0_Udp_Axis_tdata[34]} {sSHL_Rol_Nts0_Udp_Axis_tdata[35]} {sSHL_Rol_Nts0_Udp_Axis_tdata[36]} {sSHL_Rol_Nts0_Udp_Axis_tdata[37]} {sSHL_Rol_Nts0_Udp_Axis_tdata[38]} {sSHL_Rol_Nts0_Udp_Axis_tdata[39]} {sSHL_Rol_Nts0_Udp_Axis_tdata[40]} {sSHL_Rol_Nts0_Udp_Axis_tdata[41]} {sSHL_Rol_Nts0_Udp_Axis_tdata[42]} {sSHL_Rol_Nts0_Udp_Axis_tdata[43]} {sSHL_Rol_Nts0_Udp_Axis_tdata[44]} {sSHL_Rol_Nts0_Udp_Axis_tdata[45]} {sSHL_Rol_Nts0_Udp_Axis_tdata[46]} {sSHL_Rol_Nts0_Udp_Axis_tdata[47]} {sSHL_Rol_Nts0_Udp_Axis_tdata[48]} {sSHL_Rol_Nts0_Udp_Axis_tdata[49]} {sSHL_Rol_Nts0_Udp_Axis_tdata[50]} {sSHL_Rol_Nts0_Udp_Axis_tdata[51]} {sSHL_Rol_Nts0_Udp_Axis_tdata[52]} {sSHL_Rol_Nts0_Udp_Axis_tdata[53]} {sSHL_Rol_Nts0_Udp_Axis_tdata[54]} {sSHL_Rol_Nts0_Udp_Axis_tdata[55]} {sSHL_Rol_Nts0_Udp_Axis_tdata[56]} {sSHL_Rol_Nts0_Udp_Axis_tdata[57]} {sSHL_Rol_Nts0_Udp_Axis_tdata[58]} {sSHL_Rol_Nts0_Udp_Axis_tdata[59]} {sSHL_Rol_Nts0_Udp_Axis_tdata[60]} {sSHL_Rol_Nts0_Udp_Axis_tdata[61]} {sSHL_Rol_Nts0_Udp_Axis_tdata[62]} {sSHL_Rol_Nts0_Udp_Axis_tdata[63]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe3]
set_property port_width 8 [get_debug_ports u_ila_0/probe3]
connect_debug_port u_ila_0/probe3 [get_nets [list {sSHL_Rol_Nts0_Udp_Axis_tkeep[0]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[1]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[2]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[3]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[4]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[5]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[6]} {sSHL_Rol_Nts0_Udp_Axis_tkeep[7]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe4]
set_property port_width 1 [get_debug_ports u_ila_0/probe4]
connect_debug_port u_ila_0/probe4 [get_nets [list sROL_Shl_Nts0_Udp_Axis_tlast]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe5]
set_property port_width 1 [get_debug_ports u_ila_0/probe5]
connect_debug_port u_ila_0/probe5 [get_nets [list sROL_Shl_Nts0_Udp_Axis_tready]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe6]
set_property port_width 1 [get_debug_ports u_ila_0/probe6]
connect_debug_port u_ila_0/probe6 [get_nets [list sROL_Shl_Nts0_Udp_Axis_tvalid]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe7]
set_property port_width 1 [get_debug_ports u_ila_0/probe7]
connect_debug_port u_ila_0/probe7 [get_nets [list sSHL_Rol_Nts0_Udp_Axis_tlast]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe8]
set_property port_width 1 [get_debug_ports u_ila_0/probe8]
connect_debug_port u_ila_0/probe8 [get_nets [list sSHL_Rol_Nts0_Udp_Axis_tready]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe9]
set_property port_width 1 [get_debug_ports u_ila_0/probe9]
connect_debug_port u_ila_0/probe9 [get_nets [list sSHL_Rol_Nts0_Udp_Axis_tvalid]]
set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk]
