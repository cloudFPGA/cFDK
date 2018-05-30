# ******************************************************************************
# *
# *                        Zurich cloudFPGA
# *            All rights reserved -- Property of IBM
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Out-Of-Context (OOC) constraint file for the SHELL of the FMKU60.
# * File    : Shell_ooc.xdc
# *
# * Created : Feb. 2018
# * Authors : Francois Abel <fab@zurich.ibm.com>
# *
# * Devices : xcku060-ffva1156-2-i
# * Tools   : Vivado v2016.4 (64-bit)
# * Depends : None
# *
# * Description : When the SHELL is packaged as an IP, the design inherits some
# *   of the needed constraints from the parent design. This XDC file separates
# *   the constraints that the SHELL requires when used out-of-context (OOC) in
# *   a so-called standalone XDC file. Vivado synthesis uses this standalone XDC
# *   file in the OOC synthesis run to constrain the IP to the recommended clock
# *   frequency, and the presence of this file is therefore mandatory. 
# *   In other words, this file contains all the clocks that are not internal, 
# *   or local to the SHELL, but are provided by the parent design. 
# *
# * Reference documents:
# *  - UG1119 / Lab1 / Packaging a Project
# *
# *-----------------------------------------------------------------------------
# * Comments:
# *  - According to UG1119-Lab1, "If the created clock is internal to the IP 
# *    (GT), or if the IP contains an input buffer (IBUF), the create_clock 
# *    constraint should stay in the IP XDC file because it is needed to define 
# *    local clocks."
# *
# ******************************************************************************


# This XDC is used only for OOC mode of synthesis, implementation
# This constraints file contains default clock frequencies to be used during
# out-of-context flows such as OOC Synthesis and Hierarchical Designs.
# This constraints file is not used in normal top-down synthesis (default flow
# of Vivado)



#=====================================================================
# Constraints related to the Clock Tree (CLKT)
#=====================================================================

# CLKT / Design reference clock (i.e. User Clock #0)
create_clock -period 6.400 [ get_ports piTOP_156_25Clk ]

# CLKT / Reference clock for GTH transceivers of the 10GE Interface
create_clock -period 6.400 [ get_ports piCLKT_Shl_10GeClk_p ]

# CLKT / Reference clock for the DRAM block 0
create_clock -period 3.333 [ get_ports piCLKT_Shl_Mem0Clk_p ]

# CLKT / Reference clock for the DRAM block 1
create_clock -period 3.333 [ get_ports piCLKT_Shl_Mem1Clk_p ]


