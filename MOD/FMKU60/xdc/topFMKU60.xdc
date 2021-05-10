# ******************************************************************************
# * Copyright 2016 -- 2020 IBM Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# ******************************************************************************

# ******************************************************************************
# *
# *                                 cloudFPGA
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Default physical constraint file for the  the module FMKU60
# *            (i.e. FMKU2595 equipped with a XCKU060).
# *
# * File    : top_FMKU60_Flash.xdc
# *
# * Devices : xcku060-ffva1156-2-i
# *
# * Tools   : Vivado v2016.4, v2017.4, v2019.2 (64-bit)
# *
# * Description : This file contains all the default physical constraints for
# *     operating a XCKU060 on a FMKU2595 module. The constraints are grouped
# *      by external device and connector names:
# *       - the synchronous dynamic random access memory (SDRM or DDR4)
# *       - the programmable system on chip controller (PSOC)
# *       - the configuration Flash memory (FLASH)
# *       - the clock tree generator (CLKT)
# *       - the edge backplane connector (ECON)
# *       - the top extension connector (XCON)
# *
# *     The SDRM has the following interfaces:
# *       - a memory channel 0 (DDR4-Mc0)
# *       - a memory channel 1 (DDR4-Mc1)
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
# *     The XCON is not used.
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



