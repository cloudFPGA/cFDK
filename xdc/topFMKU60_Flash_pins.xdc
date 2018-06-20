# ******************************************************************************
# *
# *                        Zurich cloudFPGA
# *            All rights reserved -- Property of IBM
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Physical pin constraints file for the FMKU2595 equipped with a
# * 	        XCKU060.
# *
# * File    : topFMKU60_Flash_pins.xdc
# *
# * Created : Sep. 2017
# * Authors : Francois Abel <fab@zurich.ibm.com>
# *
# * Devices : xcku060-ffva1156-2-i
# * Tools   : Vivado v2016.4, v2017.4 (64-bit)
# * Depends : None
# *
# * Description : This file contains all the physical pin constraints for
# *     operating a XCKU060 on a FMKU2595 module. The constraints are grouped
# *      by external device and connector entity names:
# *       - the synchronous dynamic random access memory (DDR4)
# *       - the programmable system on chip controller (PSOC)
# *       - the configuration Flash memory (FLASH)
# *       - the clock tree generator (CLKT)
# *       - the edge backplane connector (ECON)
# *       - the top extension connector (XCON)
# *
# *     The DDR4 has the following interfaces:
# *       - a memory channel 0 (DDR4_Mc0)
# *       - a memory channel 1 (DDR4_Mc1)
# *     The PSOC has the following interfaces:
# *       - an External memory interface (PSOC_Emif)
# *       - an Fpga configuration interface (PSOC_Fcfg)
# *     The FLASH has the following interfaces:
# *       - a byte peripheral interface (FLASH_Bpi)
# *     The CLKT has the following interfaces:
# *       - User clocks 0 and 1 (CLKT_Usr0 and CLKT_Usr1)
# *       - Dynamic random access memory clocks 0 and 1 (Mem0, Mem1)
# *       - Gigabit transceiver clocks (CLKT_10Ge and CLKT_Gtio)
# *     The ECON has the following interfaces:
# *       - 10GE[0] Rx and Tx transceives (ECON_10Ge0)
# *     The XCON has the following interfaces:
# *       - [TODO]
# *
# *-----------------------------------------------------------------------------
# * Comments:
# *
# ******************************************************************************


#---------------------------------------------------------------------
# Setting UNUSEDPIN
#---------------------------------------------------------------------
#set_property BITSTREAM.CONFIG.UNUSEDPIN PULLNONE [current_design]

#---------------------------------------------------------------------
# LED Interface (Heart beat)
#---------------------------------------------------------------------
set_property PACKAGE_PIN J24 [get_ports poTOP_Led_HeartBeat_n]
set_property IOSTANDARD LVCMOS18 [get_ports poTOP_Led_HeartBeat_n]

#---------------------------------------------------------------------
# Constraints related to the Clock Tree (CLKT)
#---------------------------------------------------------------------

# CLKT / Reference clock for GTH transceivers of the 10GE Interface
set_property PACKAGE_PIN V5 [get_ports piCLKT_10GeClk_n]
set_property PACKAGE_PIN V6 [get_ports piCLKT_10GeClk_p]

# CLKT / Reference clock for the DRAM block 0
set_property PACKAGE_PIN AJ24 [get_ports piCLKT_Mem0Clk_n]
set_property PACKAGE_PIN AJ23 [get_ports piCLKT_Mem0Clk_p]
# [INFO] The clock IOSTANDARDs are already constrained by the IP core.
#  set_property IOSTANDARD LVDS [get_ports piCLKT_Mem0Clk_n]
#  set_property IOSTANDARD LVDS [get_ports piCLKT_Mem0Clk_p]

# CLKT / Reference clock for the DRAM block 1
set_property PACKAGE_PIN D16 [get_ports piCLKT_Mem1Clk_n]
set_property PACKAGE_PIN E16 [get_ports piCLKT_Mem1Clk_p]
# [INFO] The clock IOSTANDARDs are already constrained by the IP core.
#  set_property IOSTANDARD LVDS [get_ports piCLKT_Mem1Clk_n]
#  set_property IOSTANDARD LVDS [get_ports piCLKT_Mem1Clk_p]

# CLKT / User Clock 0
set_property PACKAGE_PIN AK21 [get_ports piCLKT_Usr0Clk_n]
set_property PACKAGE_PIN AJ21 [get_ports piCLKT_Usr0Clk_p]
set_property IOSTANDARD LVDS [get_ports piCLKT_Usr0Clk_n]
set_property IOSTANDARD LVDS [get_ports piCLKT_Usr0Clk_p]

# CLKT / User Clock 1
set_property PACKAGE_PIN G16 [get_ports piCLKT_Usr1Clk_n]
set_property PACKAGE_PIN G17 [get_ports piCLKT_Usr1Clk_p]
set_property IOSTANDARD LVDS [get_ports piCLKT_Usr1Clk_n]
set_property IOSTANDARD LVDS [get_ports piCLKT_Usr1Clk_p]

#---------------------------------------------------------------------
# Constraints related to the Backplane Edge Connector (ECON)
#---------------------------------------------------------------------

# ECON / 10 Gigabit Ethernet Rx/Tx links #0
set_property PACKAGE_PIN Y1 [get_ports piECON_Top_10Ge0_n]
set_property PACKAGE_PIN Y2 [get_ports piECON_Top_10Ge0_p]
set_property PACKAGE_PIN AA3 [get_ports poTOP_Econ_10Ge0_n]
set_property PACKAGE_PIN AA4 [get_ports poTOP_Econ_10Ge0_p]

# ECON / 10 Gigabit Ethernet Rx/Tx links #1
####set_property package_pin M1   [get_ports piECON_Top_10Ge1_n]
####set_property package_pin M2   [get_ports piECON_Top_10Ge1_p]
####set_property package_pin N3   [get_ports poTOP_Econ_10Ge1_n]
####set_property package_pin N4   [get_ports poTOP_Econ_10Ge1_p]

#---------------------------------------------------------------------
# Constraints related to the Programmable SoC (PSOC)
#---------------------------------------------------------------------

# PSOC / FPGA Configuration Interface
#-------------------------------------
set_property PACKAGE_PIN H23 [get_ports piPSOC_Fcfg_Rst_n]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Fcfg_Rst_n]

# PSOC / External Memory Interface
#----------------------------------

# PSOC / Emif - Input clock
set_property PACKAGE_PIN AA32 [get_ports piPSOC_Emif_Clk]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Emif_Clk]

# PSOC / Emif - Chip select
set_property PACKAGE_PIN AC31 [get_ports piPSOC_Emif_Cs_n]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Emif_Cs_n]

# PSOC / Emif - Write enable
set_property PACKAGE_PIN AC32 [get_ports piPSOC_Emif_We_n]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Emif_We_n]

# PSoC / Emif - Output enable
set_property PACKAGE_PIN AD30 [get_ports piPSOC_Emif_Oe_n]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Emif_Oe_n]

# PSoC / Emif - Address Strobe
set_property PACKAGE_PIN AB32 [get_ports piPSOC_Emif_AdS_n]
set_property IOSTANDARD LVCMOS18 [get_ports piPSOC_Emif_AdS_n]

# PSoC / Emif - Address[7:0]
set_property PACKAGE_PIN AC28 [get_ports {piPSOC_Emif_Addr[0]}]
set_property PACKAGE_PIN AD28 [get_ports {piPSOC_Emif_Addr[1]}]
set_property PACKAGE_PIN AF29 [get_ports {piPSOC_Emif_Addr[2]}]
set_property PACKAGE_PIN AG29 [get_ports {piPSOC_Emif_Addr[3]}]
set_property PACKAGE_PIN AE30 [get_ports {piPSOC_Emif_Addr[4]}]
set_property PACKAGE_PIN AG30 [get_ports {piPSOC_Emif_Addr[5]}]
set_property PACKAGE_PIN AF30 [get_ports {piPSOC_Emif_Addr[6]}]
set_property PACKAGE_PIN AD29 [get_ports {piPSOC_Emif_Addr[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {piPSOC_Emif_Addr[7]}]

# PSoC / Emif - Data[7:0]
set_property PACKAGE_PIN AG32 [get_ports {pioPSOC_Emif_Data[0]}]
set_property PACKAGE_PIN AG31 [get_ports {pioPSOC_Emif_Data[1]}]
set_property PACKAGE_PIN AE32 [get_ports {pioPSOC_Emif_Data[2]}]
set_property PACKAGE_PIN AF33 [get_ports {pioPSOC_Emif_Data[3]}]
set_property PACKAGE_PIN AF32 [get_ports {pioPSOC_Emif_Data[4]}]
set_property PACKAGE_PIN AG34 [get_ports {pioPSOC_Emif_Data[5]}]
set_property PACKAGE_PIN AF34 [get_ports {pioPSOC_Emif_Data[6]}]
set_property PACKAGE_PIN AE33 [get_ports {pioPSOC_Emif_Data[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {pioPSOC_Emif_Data[7]}]

#---------------------------------------------------------------------
# Constraints related to the Synchronous Dynamic RAM (DDR4)
#   DDR4 / Memory Channel #0
#---------------------------------------------------------------------
set_property PACKAGE_PIN AE22 [get_ports poTOP_Ddr4_Mc0_Reset_n]
set_property IOSTANDARD LVCMOS12 [get_ports poTOP_Ddr4_Mc0_Reset_n]
set_property DRIVE 8 [get_ports poTOP_Ddr4_Mc0_Reset_n]

set_property PACKAGE_PIN AL32 [get_ports {pioDDR_Top_Mc0_DmDbi_n[0]}]
set_property PACKAGE_PIN AJ29 [get_ports {pioDDR_Top_Mc0_DmDbi_n[1]}]
set_property PACKAGE_PIN AN26 [get_ports {pioDDR_Top_Mc0_DmDbi_n[2]}]
set_property PACKAGE_PIN AH26 [get_ports {pioDDR_Top_Mc0_DmDbi_n[3]}]
set_property PACKAGE_PIN AD19 [get_ports {pioDDR_Top_Mc0_DmDbi_n[4]}]
set_property PACKAGE_PIN AH18 [get_ports {pioDDR_Top_Mc0_DmDbi_n[5]}]
set_property PACKAGE_PIN AL14 [get_ports {pioDDR_Top_Mc0_DmDbi_n[6]}]
set_property PACKAGE_PIN AN14 [get_ports {pioDDR_Top_Mc0_DmDbi_n[7]}]
set_property PACKAGE_PIN AM21 [get_ports {pioDDR_Top_Mc0_DmDbi_n[8]}]
set_property IOSTANDARD POD12_DCI [get_ports {pioDDR_Top_Mc0_DmDbi_n[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc0_DmDbi_n[*]}]

set_property PACKAGE_PIN AN31 [get_ports {pioDDR_Top_Mc0_Dq[0]}]
set_property PACKAGE_PIN AL34 [get_ports {pioDDR_Top_Mc0_Dq[1]}]
set_property PACKAGE_PIN AN32 [get_ports {pioDDR_Top_Mc0_Dq[2]}]
set_property PACKAGE_PIN AP33 [get_ports {pioDDR_Top_Mc0_Dq[3]}]
set_property PACKAGE_PIN AM32 [get_ports {pioDDR_Top_Mc0_Dq[4]}]
set_property PACKAGE_PIN AM34 [get_ports {pioDDR_Top_Mc0_Dq[5]}]
set_property PACKAGE_PIN AP31 [get_ports {pioDDR_Top_Mc0_Dq[6]}]
set_property PACKAGE_PIN AN33 [get_ports {pioDDR_Top_Mc0_Dq[7]}]
set_property PACKAGE_PIN AH32 [get_ports {pioDDR_Top_Mc0_Dq[8]}]
set_property PACKAGE_PIN AJ30 [get_ports {pioDDR_Top_Mc0_Dq[9]}]
set_property PACKAGE_PIN AJ34 [get_ports {pioDDR_Top_Mc0_Dq[10]}]
set_property PACKAGE_PIN AK32 [get_ports {pioDDR_Top_Mc0_Dq[11]}]
set_property PACKAGE_PIN AH34 [get_ports {pioDDR_Top_Mc0_Dq[12]}]
set_property PACKAGE_PIN AK31 [get_ports {pioDDR_Top_Mc0_Dq[13]}]
set_property PACKAGE_PIN AH31 [get_ports {pioDDR_Top_Mc0_Dq[14]}]
set_property PACKAGE_PIN AJ31 [get_ports {pioDDR_Top_Mc0_Dq[15]}]
set_property PACKAGE_PIN AL30 [get_ports {pioDDR_Top_Mc0_Dq[16]}]
set_property PACKAGE_PIN AN27 [get_ports {pioDDR_Top_Mc0_Dq[17]}]
set_property PACKAGE_PIN AP29 [get_ports {pioDDR_Top_Mc0_Dq[18]}]
set_property PACKAGE_PIN AP28 [get_ports {pioDDR_Top_Mc0_Dq[19]}]
set_property PACKAGE_PIN AL29 [get_ports {pioDDR_Top_Mc0_Dq[20]}]
set_property PACKAGE_PIN AM29 [get_ports {pioDDR_Top_Mc0_Dq[21]}]
set_property PACKAGE_PIN AM30 [get_ports {pioDDR_Top_Mc0_Dq[22]}]
set_property PACKAGE_PIN AN28 [get_ports {pioDDR_Top_Mc0_Dq[23]}]
set_property PACKAGE_PIN AK27 [get_ports {pioDDR_Top_Mc0_Dq[24]}]
set_property PACKAGE_PIN AH27 [get_ports {pioDDR_Top_Mc0_Dq[25]}]
set_property PACKAGE_PIN AM26 [get_ports {pioDDR_Top_Mc0_Dq[26]}]
set_property PACKAGE_PIN AJ28 [get_ports {pioDDR_Top_Mc0_Dq[27]}]
set_property PACKAGE_PIN AK26 [get_ports {pioDDR_Top_Mc0_Dq[28]}]
set_property PACKAGE_PIN AH28 [get_ports {pioDDR_Top_Mc0_Dq[29]}]
set_property PACKAGE_PIN AM27 [get_ports {pioDDR_Top_Mc0_Dq[30]}]
set_property PACKAGE_PIN AK28 [get_ports {pioDDR_Top_Mc0_Dq[31]}]
set_property PACKAGE_PIN AD16 [get_ports {pioDDR_Top_Mc0_Dq[32]}]
set_property PACKAGE_PIN AF15 [get_ports {pioDDR_Top_Mc0_Dq[33]}]
set_property PACKAGE_PIN AF14 [get_ports {pioDDR_Top_Mc0_Dq[34]}]
set_property PACKAGE_PIN AF17 [get_ports {pioDDR_Top_Mc0_Dq[35]}]
set_property PACKAGE_PIN AE17 [get_ports {pioDDR_Top_Mc0_Dq[36]}]
set_property PACKAGE_PIN AE18 [get_ports {pioDDR_Top_Mc0_Dq[37]}]
set_property PACKAGE_PIN AD15 [get_ports {pioDDR_Top_Mc0_Dq[38]}]
set_property PACKAGE_PIN AF18 [get_ports {pioDDR_Top_Mc0_Dq[39]}]
set_property PACKAGE_PIN AH19 [get_ports {pioDDR_Top_Mc0_Dq[40]}]
set_property PACKAGE_PIN AG16 [get_ports {pioDDR_Top_Mc0_Dq[41]}]
set_property PACKAGE_PIN AH16 [get_ports {pioDDR_Top_Mc0_Dq[42]}]
set_property PACKAGE_PIN AG17 [get_ports {pioDDR_Top_Mc0_Dq[43]}]
set_property PACKAGE_PIN AG19 [get_ports {pioDDR_Top_Mc0_Dq[44]}]
set_property PACKAGE_PIN AG14 [get_ports {pioDDR_Top_Mc0_Dq[45]}]
set_property PACKAGE_PIN AJ16 [get_ports {pioDDR_Top_Mc0_Dq[46]}]
set_property PACKAGE_PIN AG15 [get_ports {pioDDR_Top_Mc0_Dq[47]}]
set_property PACKAGE_PIN AL15 [get_ports {pioDDR_Top_Mc0_Dq[48]}]
set_property PACKAGE_PIN AK18 [get_ports {pioDDR_Top_Mc0_Dq[49]}]
set_property PACKAGE_PIN AK17 [get_ports {pioDDR_Top_Mc0_Dq[50]}]
set_property PACKAGE_PIN AJ18 [get_ports {pioDDR_Top_Mc0_Dq[51]}]
set_property PACKAGE_PIN AK15 [get_ports {pioDDR_Top_Mc0_Dq[52]}]
set_property PACKAGE_PIN AL19 [get_ports {pioDDR_Top_Mc0_Dq[53]}]
set_property PACKAGE_PIN AK16 [get_ports {pioDDR_Top_Mc0_Dq[54]}]
set_property PACKAGE_PIN AM19 [get_ports {pioDDR_Top_Mc0_Dq[55]}]
set_property PACKAGE_PIN AP18 [get_ports {pioDDR_Top_Mc0_Dq[56]}]
set_property PACKAGE_PIN AP16 [get_ports {pioDDR_Top_Mc0_Dq[57]}]
set_property PACKAGE_PIN AM16 [get_ports {pioDDR_Top_Mc0_Dq[58]}]
set_property PACKAGE_PIN AN16 [get_ports {pioDDR_Top_Mc0_Dq[59]}]
set_property PACKAGE_PIN AN19 [get_ports {pioDDR_Top_Mc0_Dq[60]}]
set_property PACKAGE_PIN AM15 [get_ports {pioDDR_Top_Mc0_Dq[61]}]
set_property PACKAGE_PIN AM17 [get_ports {pioDDR_Top_Mc0_Dq[62]}]
set_property PACKAGE_PIN AP15 [get_ports {pioDDR_Top_Mc0_Dq[63]}]
set_property PACKAGE_PIN AP25 [get_ports {pioDDR_Top_Mc0_Dq[64]}]
set_property PACKAGE_PIN AP23 [get_ports {pioDDR_Top_Mc0_Dq[65]}]
set_property PACKAGE_PIN AP24 [get_ports {pioDDR_Top_Mc0_Dq[66]}]
set_property PACKAGE_PIN AN23 [get_ports {pioDDR_Top_Mc0_Dq[67]}]
set_property PACKAGE_PIN AN24 [get_ports {pioDDR_Top_Mc0_Dq[68]}]
set_property PACKAGE_PIN AN22 [get_ports {pioDDR_Top_Mc0_Dq[69]}]
set_property PACKAGE_PIN AM24 [get_ports {pioDDR_Top_Mc0_Dq[70]}]
set_property PACKAGE_PIN AM22 [get_ports {pioDDR_Top_Mc0_Dq[71]}]
set_property IOSTANDARD POD12_DCI [get_ports {pioDDR_Top_Mc0_Dq[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc0_Dq[*]}]

set_property PACKAGE_PIN AP34 [get_ports {pioDDR_Top_Mc0_Dqs_n[0]}]
set_property PACKAGE_PIN AN34 [get_ports {pioDDR_Top_Mc0_Dqs_p[0]}]
set_property PACKAGE_PIN AJ33 [get_ports {pioDDR_Top_Mc0_Dqs_n[1]}]
set_property PACKAGE_PIN AH33 [get_ports {pioDDR_Top_Mc0_Dqs_p[1]}]
set_property PACKAGE_PIN AP30 [get_ports {pioDDR_Top_Mc0_Dqs_n[2]}]
set_property PACKAGE_PIN AN29 [get_ports {pioDDR_Top_Mc0_Dqs_p[2]}]
set_property PACKAGE_PIN AL28 [get_ports {pioDDR_Top_Mc0_Dqs_n[3]}]
set_property PACKAGE_PIN AL27 [get_ports {pioDDR_Top_Mc0_Dqs_p[3]}]
set_property PACKAGE_PIN AE15 [get_ports {pioDDR_Top_Mc0_Dqs_n[4]}]
set_property PACKAGE_PIN AE16 [get_ports {pioDDR_Top_Mc0_Dqs_p[4]}]
set_property PACKAGE_PIN AJ14 [get_ports {pioDDR_Top_Mc0_Dqs_n[5]}]
set_property PACKAGE_PIN AJ15 [get_ports {pioDDR_Top_Mc0_Dqs_p[5]}]
set_property PACKAGE_PIN AL17 [get_ports {pioDDR_Top_Mc0_Dqs_n[6]}]
set_property PACKAGE_PIN AL18 [get_ports {pioDDR_Top_Mc0_Dqs_p[6]}]
set_property PACKAGE_PIN AN17 [get_ports {pioDDR_Top_Mc0_Dqs_n[7]}]
set_property PACKAGE_PIN AN18 [get_ports {pioDDR_Top_Mc0_Dqs_p[7]}]
set_property PACKAGE_PIN AP21 [get_ports {pioDDR_Top_Mc0_Dqs_n[8]}]
set_property PACKAGE_PIN AP20 [get_ports {pioDDR_Top_Mc0_Dqs_p[8]}]
set_property IOSTANDARD DIFF_POD12_DCI [get_ports {pioDDR_Top_Mc0_Dqs_n[*]}]
set_property IOSTANDARD DIFF_POD12_DCI [get_ports {pioDDR_Top_Mc0_Dqs_p[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc0_Dqs_n[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc0_Dqs_p[*]}]

set_property PACKAGE_PIN AH23 [get_ports poTOP_Ddr4_Mc0_Act_n]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Act_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Act_n]

set_property PACKAGE_PIN AF22 [get_ports {poTOP_Ddr4_Mc0_Adr[0]}]
set_property PACKAGE_PIN AG21 [get_ports {poTOP_Ddr4_Mc0_Adr[1]}]
set_property PACKAGE_PIN AG20 [get_ports {poTOP_Ddr4_Mc0_Adr[2]}]
set_property PACKAGE_PIN AK22 [get_ports {poTOP_Ddr4_Mc0_Adr[3]}]
set_property PACKAGE_PIN AL22 [get_ports {poTOP_Ddr4_Mc0_Adr[4]}]
set_property PACKAGE_PIN AN21 [get_ports {poTOP_Ddr4_Mc0_Adr[5]}]
set_property PACKAGE_PIN AD20 [get_ports {poTOP_Ddr4_Mc0_Adr[6]}]
set_property PACKAGE_PIN AL20 [get_ports {poTOP_Ddr4_Mc0_Adr[7]}]
set_property PACKAGE_PIN AE20 [get_ports {poTOP_Ddr4_Mc0_Adr[8]}]
set_property PACKAGE_PIN AK20 [get_ports {poTOP_Ddr4_Mc0_Adr[9]}]
set_property PACKAGE_PIN AG22 [get_ports {poTOP_Ddr4_Mc0_Adr[10]}]
set_property PACKAGE_PIN AF20 [get_ports {poTOP_Ddr4_Mc0_Adr[11]}]
set_property PACKAGE_PIN AF24 [get_ports {poTOP_Ddr4_Mc0_Adr[12]}]
set_property PACKAGE_PIN AJ20 [get_ports {poTOP_Ddr4_Mc0_Adr[13]}]
set_property PACKAGE_PIN AE23 [get_ports {poTOP_Ddr4_Mc0_Adr[14]}]
set_property PACKAGE_PIN AK25 [get_ports {poTOP_Ddr4_Mc0_Adr[15]}]
set_property PACKAGE_PIN AF23 [get_ports {poTOP_Ddr4_Mc0_Adr[16]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Adr[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Adr[*]}]

set_property PACKAGE_PIN AK23 [get_ports {poTOP_Ddr4_Mc0_Ba[0]}]
set_property PACKAGE_PIN AJ25 [get_ports {poTOP_Ddr4_Mc0_Ba[1]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Ba[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Ba[*]}]

set_property PACKAGE_PIN AL23 [get_ports {poTOP_Ddr4_Mc0_Bg[0]}]
set_property PACKAGE_PIN AL24 [get_ports {poTOP_Ddr4_Mc0_Bg[1]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Bg[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Bg[*]}]

set_property PACKAGE_PIN AG25 [get_ports poTOP_Ddr4_Mc0_Ck_n]
set_property PACKAGE_PIN AG24 [get_ports poTOP_Ddr4_Mc0_Ck_p]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Ck_n]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Ck_p]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Ck_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Ck_p]

set_property PACKAGE_PIN AD21 [get_ports poTOP_Ddr4_Mc0_Cke]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Cke]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Cke]
# [IS-NOT-USED] set_property PACKAGE_PIN      AG25        [get_ports {poTOP_Ddr4_Mc0_Cke[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Cke[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Cke[*]}]

set_property PACKAGE_PIN AL25 [get_ports poTOP_Ddr4_Mc0_Cs_n]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Cs_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Cs_n]
# [IS-NOT-USED] set_property PACKAGE_PIN AH24 [get_ports {poTOP_Ddr4_Mc0_Cs_n[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Cs_n[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Cs_n[*]}]

set_property PACKAGE_PIN AF25 [get_ports poTOP_Ddr4_Mc0_Odt]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc0_Odt]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc0_Odt]
# [IS-NOT-USED] set_property PACKAGE_PIN      AE26        [get_ports {poTOP_Ddr4_Mc0_Odt[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc0_Odt[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc0_Odt[*]}]


#---------------------------------------------------------------------
# Constraints related to the Synchronous Dynamic RAM (SDDR4)
#   DDR4 / Memory Channel #1
#---------------------------------------------------------------------
set_property PACKAGE_PIN H18 [get_ports poTOP_Ddr4_Mc1_Reset_n]
set_property IOSTANDARD LVCMOS12 [get_ports poTOP_Ddr4_Mc1_Reset_n]
set_property DRIVE 8 [get_ports poTOP_Ddr4_Mc1_Reset_n]

set_property PACKAGE_PIN F8 [get_ports {pioDDR_Top_Mc1_DmDbi_n[0]}]
set_property PACKAGE_PIN L8 [get_ports {pioDDR_Top_Mc1_DmDbi_n[1]}]
set_property PACKAGE_PIN H11 [get_ports {pioDDR_Top_Mc1_DmDbi_n[2]}]
set_property PACKAGE_PIN E11 [get_ports {pioDDR_Top_Mc1_DmDbi_n[3]}]
set_property PACKAGE_PIN F27 [get_ports {pioDDR_Top_Mc1_DmDbi_n[4]}]
set_property PACKAGE_PIN E26 [get_ports {pioDDR_Top_Mc1_DmDbi_n[5]}]
set_property PACKAGE_PIN D23 [get_ports {pioDDR_Top_Mc1_DmDbi_n[6]}]
set_property PACKAGE_PIN G24 [get_ports {pioDDR_Top_Mc1_DmDbi_n[7]}]
set_property PACKAGE_PIN B14 [get_ports {pioDDR_Top_Mc1_DmDbi_n[8]}]
set_property IOSTANDARD POD12_DCI [get_ports {pioDDR_Top_Mc1_DmDbi_n[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc1_DmDbi_n[*]}]

set_property PACKAGE_PIN C8 [get_ports {pioDDR_Top_Mc1_Dq[0]}]
set_property PACKAGE_PIN E10 [get_ports {pioDDR_Top_Mc1_Dq[1]}]
set_property PACKAGE_PIN C9 [get_ports {pioDDR_Top_Mc1_Dq[2]}]
set_property PACKAGE_PIN D10 [get_ports {pioDDR_Top_Mc1_Dq[3]}]
set_property PACKAGE_PIN D9 [get_ports {pioDDR_Top_Mc1_Dq[4]}]
set_property PACKAGE_PIN A9 [get_ports {pioDDR_Top_Mc1_Dq[5]}]
set_property PACKAGE_PIN D8 [get_ports {pioDDR_Top_Mc1_Dq[6]}]
set_property PACKAGE_PIN B9 [get_ports {pioDDR_Top_Mc1_Dq[7]}]
set_property PACKAGE_PIN G10 [get_ports {pioDDR_Top_Mc1_Dq[8]}]
set_property PACKAGE_PIN J8 [get_ports {pioDDR_Top_Mc1_Dq[9]}]
set_property PACKAGE_PIN G9 [get_ports {pioDDR_Top_Mc1_Dq[10]}]
set_property PACKAGE_PIN F9 [get_ports {pioDDR_Top_Mc1_Dq[11]}]
set_property PACKAGE_PIN H9 [get_ports {pioDDR_Top_Mc1_Dq[12]}]
set_property PACKAGE_PIN H8 [get_ports {pioDDR_Top_Mc1_Dq[13]}]
set_property PACKAGE_PIN J9 [get_ports {pioDDR_Top_Mc1_Dq[14]}]
set_property PACKAGE_PIN F10 [get_ports {pioDDR_Top_Mc1_Dq[15]}]
set_property PACKAGE_PIN J13 [get_ports {pioDDR_Top_Mc1_Dq[16]}]
set_property PACKAGE_PIN K11 [get_ports {pioDDR_Top_Mc1_Dq[17]}]
set_property PACKAGE_PIN G12 [get_ports {pioDDR_Top_Mc1_Dq[18]}]
set_property PACKAGE_PIN K12 [get_ports {pioDDR_Top_Mc1_Dq[19]}]
set_property PACKAGE_PIN H13 [get_ports {pioDDR_Top_Mc1_Dq[20]}]
set_property PACKAGE_PIN L12 [get_ports {pioDDR_Top_Mc1_Dq[21]}]
set_property PACKAGE_PIN H12 [get_ports {pioDDR_Top_Mc1_Dq[22]}]
set_property PACKAGE_PIN J11 [get_ports {pioDDR_Top_Mc1_Dq[23]}]
set_property PACKAGE_PIN B11 [get_ports {pioDDR_Top_Mc1_Dq[24]}]
set_property PACKAGE_PIN C12 [get_ports {pioDDR_Top_Mc1_Dq[25]}]
set_property PACKAGE_PIN B12 [get_ports {pioDDR_Top_Mc1_Dq[26]}]
set_property PACKAGE_PIN A13 [get_ports {pioDDR_Top_Mc1_Dq[27]}]
set_property PACKAGE_PIN A12 [get_ports {pioDDR_Top_Mc1_Dq[28]}]
set_property PACKAGE_PIN D13 [get_ports {pioDDR_Top_Mc1_Dq[29]}]
set_property PACKAGE_PIN C11 [get_ports {pioDDR_Top_Mc1_Dq[30]}]
set_property PACKAGE_PIN C13 [get_ports {pioDDR_Top_Mc1_Dq[31]}]
set_property PACKAGE_PIN C27 [get_ports {pioDDR_Top_Mc1_Dq[32]}]
set_property PACKAGE_PIN C28 [get_ports {pioDDR_Top_Mc1_Dq[33]}]
set_property PACKAGE_PIN B27 [get_ports {pioDDR_Top_Mc1_Dq[34]}]
set_property PACKAGE_PIN D28 [get_ports {pioDDR_Top_Mc1_Dq[35]}]
set_property PACKAGE_PIN A27 [get_ports {pioDDR_Top_Mc1_Dq[36]}]
set_property PACKAGE_PIN E28 [get_ports {pioDDR_Top_Mc1_Dq[37]}]
set_property PACKAGE_PIN A28 [get_ports {pioDDR_Top_Mc1_Dq[38]}]
set_property PACKAGE_PIN D29 [get_ports {pioDDR_Top_Mc1_Dq[39]}]
set_property PACKAGE_PIN D24 [get_ports {pioDDR_Top_Mc1_Dq[40]}]
set_property PACKAGE_PIN C24 [get_ports {pioDDR_Top_Mc1_Dq[41]}]
set_property PACKAGE_PIN D25 [get_ports {pioDDR_Top_Mc1_Dq[42]}]
set_property PACKAGE_PIN B25 [get_ports {pioDDR_Top_Mc1_Dq[43]}]
set_property PACKAGE_PIN E25 [get_ports {pioDDR_Top_Mc1_Dq[44]}]
set_property PACKAGE_PIN A25 [get_ports {pioDDR_Top_Mc1_Dq[45]}]
set_property PACKAGE_PIN B26 [get_ports {pioDDR_Top_Mc1_Dq[46]}]
set_property PACKAGE_PIN C26 [get_ports {pioDDR_Top_Mc1_Dq[47]}]
set_property PACKAGE_PIN B20 [get_ports {pioDDR_Top_Mc1_Dq[48]}]
set_property PACKAGE_PIN D21 [get_ports {pioDDR_Top_Mc1_Dq[49]}]
set_property PACKAGE_PIN D20 [get_ports {pioDDR_Top_Mc1_Dq[50]}]
set_property PACKAGE_PIN A20 [get_ports {pioDDR_Top_Mc1_Dq[51]}]
set_property PACKAGE_PIN B21 [get_ports {pioDDR_Top_Mc1_Dq[52]}]
set_property PACKAGE_PIN E22 [get_ports {pioDDR_Top_Mc1_Dq[53]}]
set_property PACKAGE_PIN B22 [get_ports {pioDDR_Top_Mc1_Dq[54]}]
set_property PACKAGE_PIN E23 [get_ports {pioDDR_Top_Mc1_Dq[55]}]
set_property PACKAGE_PIN F24 [get_ports {pioDDR_Top_Mc1_Dq[56]}]
set_property PACKAGE_PIN E20 [get_ports {pioDDR_Top_Mc1_Dq[57]}]
set_property PACKAGE_PIN G22 [get_ports {pioDDR_Top_Mc1_Dq[58]}]
set_property PACKAGE_PIN G21 [get_ports {pioDDR_Top_Mc1_Dq[59]}]
set_property PACKAGE_PIN F22 [get_ports {pioDDR_Top_Mc1_Dq[60]}]
set_property PACKAGE_PIN H21 [get_ports {pioDDR_Top_Mc1_Dq[61]}]
set_property PACKAGE_PIN F23 [get_ports {pioDDR_Top_Mc1_Dq[62]}]
set_property PACKAGE_PIN E21 [get_ports {pioDDR_Top_Mc1_Dq[63]}]
set_property PACKAGE_PIN A15 [get_ports {pioDDR_Top_Mc1_Dq[64]}]
set_property PACKAGE_PIN B17 [get_ports {pioDDR_Top_Mc1_Dq[65]}]
set_property PACKAGE_PIN C17 [get_ports {pioDDR_Top_Mc1_Dq[66]}]
set_property PACKAGE_PIN A19 [get_ports {pioDDR_Top_Mc1_Dq[67]}]
set_property PACKAGE_PIN B15 [get_ports {pioDDR_Top_Mc1_Dq[68]}]
set_property PACKAGE_PIN C18 [get_ports {pioDDR_Top_Mc1_Dq[69]}]
set_property PACKAGE_PIN B16 [get_ports {pioDDR_Top_Mc1_Dq[70]}]
set_property PACKAGE_PIN A18 [get_ports {pioDDR_Top_Mc1_Dq[71]}]
set_property IOSTANDARD POD12_DCI [get_ports {pioDDR_Top_Mc1_Dq[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc1_Dq[*]}]

set_property PACKAGE_PIN A10 [get_ports {pioDDR_Top_Mc1_Dqs_n[0]}]
set_property PACKAGE_PIN B10 [get_ports {pioDDR_Top_Mc1_Dqs_p[0]}]
set_property PACKAGE_PIN J10 [get_ports {pioDDR_Top_Mc1_Dqs_n[1]}]
set_property PACKAGE_PIN K10 [get_ports {pioDDR_Top_Mc1_Dqs_p[1]}]
set_property PACKAGE_PIN K13 [get_ports {pioDDR_Top_Mc1_Dqs_n[2]}]
set_property PACKAGE_PIN L13 [get_ports {pioDDR_Top_Mc1_Dqs_p[2]}]
set_property PACKAGE_PIN E13 [get_ports {pioDDR_Top_Mc1_Dqs_n[3]}]
set_property PACKAGE_PIN F13 [get_ports {pioDDR_Top_Mc1_Dqs_p[3]}]
set_property PACKAGE_PIN A29 [get_ports {pioDDR_Top_Mc1_Dqs_n[4]}]
set_property PACKAGE_PIN B29 [get_ports {pioDDR_Top_Mc1_Dqs_p[4]}]
set_property PACKAGE_PIN A24 [get_ports {pioDDR_Top_Mc1_Dqs_n[5]}]
set_property PACKAGE_PIN B24 [get_ports {pioDDR_Top_Mc1_Dqs_p[5]}]
set_property PACKAGE_PIN C22 [get_ports {pioDDR_Top_Mc1_Dqs_n[6]}]
set_property PACKAGE_PIN C21 [get_ports {pioDDR_Top_Mc1_Dqs_p[6]}]
set_property PACKAGE_PIN F20 [get_ports {pioDDR_Top_Mc1_Dqs_n[7]}]
set_property PACKAGE_PIN G20 [get_ports {pioDDR_Top_Mc1_Dqs_p[7]}]
set_property PACKAGE_PIN B19 [get_ports {pioDDR_Top_Mc1_Dqs_n[8]}]
set_property PACKAGE_PIN C19 [get_ports {pioDDR_Top_Mc1_Dqs_p[8]}]
set_property IOSTANDARD DIFF_POD12_DCI [get_ports {pioDDR_Top_Mc1_Dqs_n[*]}]
set_property IOSTANDARD DIFF_POD12_DCI [get_ports {pioDDR_Top_Mc1_Dqs_p[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc1_Dqs_n[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {pioDDR_Top_Mc1_Dqs_p[*]}]

set_property PACKAGE_PIN L15 [get_ports poTOP_Ddr4_Mc1_Act_n]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Act_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Act_n]

set_property PACKAGE_PIN F18 [get_ports {poTOP_Ddr4_Mc1_Adr[0]}]
set_property PACKAGE_PIN D19 [get_ports {poTOP_Ddr4_Mc1_Adr[1]}]
set_property PACKAGE_PIN G19 [get_ports {poTOP_Ddr4_Mc1_Adr[2]}]
set_property PACKAGE_PIN K18 [get_ports {poTOP_Ddr4_Mc1_Adr[3]}]
set_property PACKAGE_PIN J19 [get_ports {poTOP_Ddr4_Mc1_Adr[4]}]
set_property PACKAGE_PIN D18 [get_ports {poTOP_Ddr4_Mc1_Adr[5]}]
set_property PACKAGE_PIN E15 [get_ports {poTOP_Ddr4_Mc1_Adr[6]}]
set_property PACKAGE_PIN F19 [get_ports {poTOP_Ddr4_Mc1_Adr[7]}]
set_property PACKAGE_PIN G14 [get_ports {poTOP_Ddr4_Mc1_Adr[8]}]
set_property PACKAGE_PIN L19 [get_ports {poTOP_Ddr4_Mc1_Adr[9]}]
set_property PACKAGE_PIN K17 [get_ports {poTOP_Ddr4_Mc1_Adr[10]}]
set_property PACKAGE_PIN H19 [get_ports {poTOP_Ddr4_Mc1_Adr[11]}]
set_property PACKAGE_PIN J18 [get_ports {poTOP_Ddr4_Mc1_Adr[12]}]
set_property PACKAGE_PIN K16 [get_ports {poTOP_Ddr4_Mc1_Adr[13]}]
set_property PACKAGE_PIN J16 [get_ports {poTOP_Ddr4_Mc1_Adr[14]}]
set_property PACKAGE_PIN H16 [get_ports {poTOP_Ddr4_Mc1_Adr[15]}]
set_property PACKAGE_PIN C16 [get_ports {poTOP_Ddr4_Mc1_Adr[16]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Adr[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Adr[*]}]

set_property PACKAGE_PIN F14 [get_ports {poTOP_Ddr4_Mc1_Ba[0]}]
set_property PACKAGE_PIN F17 [get_ports {poTOP_Ddr4_Mc1_Ba[1]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Ba[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Ba[*]}]

set_property PACKAGE_PIN D14 [get_ports {poTOP_Ddr4_Mc1_Bg[0]}]
set_property PACKAGE_PIN E17 [get_ports {poTOP_Ddr4_Mc1_Bg[1]}]
set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Bg[*]}]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Bg[*]}]

set_property PACKAGE_PIN J14 [get_ports poTOP_Ddr4_Mc1_Ck_n]
set_property PACKAGE_PIN J15 [get_ports poTOP_Ddr4_Mc1_Ck_p]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Ck_n]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Ck_p]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Ck_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Ck_p]

set_property PACKAGE_PIN A14 [get_ports poTOP_Ddr4_Mc1_Cke]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Cke]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Cke]
# [IS-NOT-USED] set_property PACKAGE_PIN C14 [get_ports {poTOP_Ddr4_Mc1_Cke[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Cke[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Cke[*]}]

set_property PACKAGE_PIN G15 [get_ports poTOP_Ddr4_Mc1_Cs_n]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Cs_n]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Cs_n]
# [IS-NOT-USED] set_property PACKAGE_PIN K15 [get_ports {poTOP_Ddr4_Mc1_Cs_n[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Cs_n[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Cs_n[*]}]

set_property PACKAGE_PIN D15 [get_ports poTOP_Ddr4_Mc1_Odt]
set_property IOSTANDARD SSTL12_DCI [get_ports poTOP_Ddr4_Mc1_Odt]
set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports poTOP_Ddr4_Mc1_Odt]
# [IS-NOT-USED] set_property PACKAGE_PIN H14 [get_ports {poTOP_Ddr4_Mc1_Odt[1]}]
#OBSOLETE-20180413 set_property IOSTANDARD SSTL12_DCI [get_ports {poTOP_Ddr4_Mc1_Odt[*]}]
#OBSOLETE-20180413 set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {poTOP_Ddr4_Mc1_Odt[*]}]


