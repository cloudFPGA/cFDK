# ******************************************************************************
# *
# *                        Zurich cloudFPGA
# *            All rights reserved -- Property of IBM
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Default timing constraint file for the SHELL of the FMKU60.
# *
# * File    : Shell.xdc
# *
# * Created : Feb. 2018
# * Authors : Francois Abel <fab@zurich.ibm.com>
# *
# * Devices : xcku060-ffva1156-2-i
# * Tools   : Vivado v2016.4, 2017.4 (64-bit)
# * Depends : None
# *
# * Description : This file contains all the timing constraints for synthesizing
# *   the SHELL of the FMKU60. These constraints are defined for the SHELL as if
# *   is was a standalone design. These constraints are grouped by external 
# *   devices and interfaces:
# *       - the synchronous dynamic random access memory (SDRM)
# *       - the programmable system on chip controller (PSOC)
# *       - the configuration Flash memory (FLASH)
# *       - the clock tree generator (CLKT)
# *       - the edge backplane connector (ECON)
# *       - the top extension connector (XCON)
# *
# *     The SDRM has the following interfaces:
# *       - a memory channel 0 (SDRM_Mc0)
# *       - a memory channel 1 (SDRM_Mc1)
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
# *  - According to UG1119-Lab1, "If the created clock is internal to the IP 
# *    (GT), or if the IP contains an input buffer (IBUF), the create_clock 
# *    constraint should stay in the IP XDC file because it is needed to define 
# *    local clocks."
# *
# ******************************************************************************



#=====================================================================
# Constraints related to the Clock Tree (CLKT)
#=====================================================================

# CLKT / Reference clock for GTH transceivers of the 10GE Interface
create_clock -period 6.400 [ get_ports piCLKT_Shl_10GeClk_p ]

# CLKT / Reference clock for the DRAM block 0
create_clock -period 3.333 [ get_ports piCLKT_Shl_Mem0Clk_p ]

# CLKT / Reference clock for the DRAM block 1
create_clock -period 3.333 [ get_ports piCLKT_Shl_Mem1Clk_p ]


#=====================================================================
# Constraints related to the Programmable SoC (PSOC)
#=====================================================================

# PSOC / FPGA Configuration Interface
#-------------------------------------
#OBSOLETE-20180414 set_false_path -from [ get_ports piTOP_156_25Rst ]

#---------------------------------------------------------------------
# PSOC / External Memory Interface (see PSoC Creator Component v1.30)
#---------------------------------------------------------------------
#
#             +---+   +---+   +---+   +---+   +---+   +---+   +---+
# Bus Clock   |   |   |   |   |   |   |   |   |   |   |   |   |   |
#          ---+   +---+   +---+   +---+   +---+   +---+   +---+   +---
#
#                         +---+                           +---+
# EMIF Clk                |   |                           |   |
#          ---------------+   +---------------------------+   +-------
#
#              +-----------------------------+ +----------------------
# EMIF Addr --X                               X
#              +-----------------------------+ +----------------------
#
#          ----------+        +-----------------------+        +------
# EMIF CTtl          |        |                       |        |
#                    +--------+                       +--------+
#
#----------------------------------------------------------------------

# PSOC / Internal Bus Clock (used here as a reference)

# PSOC / Emif - Input clock
create_clock -period 166.667 -name piPSOC_Shl_Emif_Clk -waveform {62.490 83.330} [ get_ports piPSOC_Shl_Emif_Clk ]

# PSoC / Emif - Address[7:0] - Write setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -20.833 [ get_ports {piPSOC_Shl_Emif_Addr[*]} ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 52.083  [ get_ports {piPSOC_Shl_Emif_Addr[*]} ]

# PSOC / Emif - Chip select - Access setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -20.833 [ get_ports piPSOC_Shl_Emif_Cs_n ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 20.833 [ get_ports piPSOC_Shl_Emif_Cs_n ]

# PSoC / Emif - Address Strobe - Access setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -20.833 [ get_ports piPSOC_Shl_Emif_AdS_n ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 20.833 [ get_ports piPSOC_Shl_Emif_AdS_n ]

# PSOC / Emif - Write enable - Write setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -20.833 [ get_ports piPSOC_Shl_Emif_We_n ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 20.833 [ get_ports piPSOC_Shl_Emif_We_n ]

# PSoC / Emif - Output enable - Read setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -20.833 [ get_ports piPSOC_Shl_Emif_Oe_n ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 31.250 [ get_ports piPSOC_Shl_Emif_Oe_n ]

# PSoC / Emif - Data[7:0] - Write setup and hold times
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max -31.667 [ get_ports {pioPSOC_Shl_Emif_Data[*]} ]
set_input_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 52.083 [ get_ports {pioPSOC_Shl_Emif_Data[*]} ]

# PSoC / Emif - Data[7:0] - Read setup and hold times
set_output_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -max 57.500 [ get_ports {pioPSOC_Shl_Emif_Data[*]} ]
set_output_delay -clock [ get_clocks piPSOC_Shl_Emif_Clk ] -min 67.500 [ get_ports {pioPSOC_Shl_Emif_Data[*]} ]
