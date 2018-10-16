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


