# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 20 2018
# * Authors : Francois Abel
# * 
# * Description : This file contains variables and constants used within the 
# *   XDC constraint files. This script will be sourced if it is specified in
# *   the tcl.pre option of the Synthesis and/or Implementation settings.
# *   The reason it is done this way is explained in AR#59158. In short, upon
# *   opening an implemented run, THE XDC in the DCP file does not contain any
# *   variables anymore because they all have been resolved. Therefore, in an
# *   implemented design, variables need to be defined in the Tcl console
# *   before they can be used in any command line or they must be predefined
# *   in a Tcl script file that can be specified in the tcl.pre option of the
# *   the Synthesis and/or Implementation settings.
# *
# * Synopsis : source <this_file>
# * 
# * Information : All frequencies are in MHz and all periods are in ns.
# *
# ******************************************************************************


#=====================================================================
# CONSTANTS FOR THE SHELL
#=====================================================================
set cShellClockFreq       156.25
set cShellClockPeriod     [ expr 1000.0 / ${cShellClockFreq} ]


#-------------------------------------------------------------------------------
# CONSTANTS FOR THE DDR4 /Memory Channel 0 & 1 
#-------------------------------------------------------------------------------
set cMem0ClkFreq          300
set cMem0ClkPeriod        [ expr 1000.0 / ${cMem0ClkFreq} ]
set cMem1ClkFreq          300
set cMem1ClkPeriod        [ expr 1000.0 / ${cMem1ClkFreq} ]


#-------------------------------------------------------------------------------
# CONSTANTS FOR THE PSOC / Internal Bus Clock (used here as a timing reference)
#-------------------------------------------------------------------------------
set cPsocBusClockFreq     24
set cPsocBusClockPeriod   [ expr 1000.0 / ${cPsocBusClockFreq} ]


#-------------------------------------------------------------------------------
# CONSTANTS FOR THE PSOC / Emif - Reference clock for the EMIF interface
#-------------------------------------------------------------------------------
# Note: There is a minimum of 4 bus cycles between two EMIF acces
set cPsocEmifClkPeriod    [ expr 4.0 * ${cPsocBusClockPeriod} ]
set cPsocEmifClkRise      [ expr 1.5 * ${cPsocBusClockPeriod} ]
set cPsocEmifClkFall      [ expr 2.0 * ${cPsocBusClockPeriod} ]
set cPsocEmifClkWaveform  "${cPsocEmifClkRise} ${cPsocEmifClkFall}"


#-------------------------------------------------------------------------------
# TIMING DIAGRAM: PSOC / External Memory Interface (cf PSoC Creator Comp. v1.30)
#-------------------------------------------------------------------------------
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


#-------------------------------------------------------------------------------
# CONSTANTS FOR THE PSoC / Emif - Address[7:0] - Write setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifAddrSetup    [ expr 1.0 * ${cPsocBusClockPeriod} / 2 ]
set cPsocEmifAddrHold     [ expr 2.5 * ${cPsocBusClockPeriod} / 2 ]

#-------------------------------------------------------------------------------
# PSOC / Emif - Chip select - Access setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifCsSetup      [ expr 1.0 * ${cPsocBusClockPeriod} / 2 ]
set cPsocEmifCsHold       [ expr 1.0 * ${cPsocBusClockPeriod} / 2 ]

#-------------------------------------------------------------------------------
# PSoC / Emif - Address Strobe - Access setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifAdsSetup     [ expr 1.0 * ${cPsocBusClockPeriod} / 2]
set cPsocEmifAdsHold      [ expr 1.0 * ${cPsocBusClockPeriod} / 2]

#-------------------------------------------------------------------------------
# PSOC / Emif - Write enable - Write setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifWeSetup      [ expr 1.0 * ${cPsocBusClockPeriod} / 2]
set cPsocEmifWeHold       [ expr 1.0 * ${cPsocBusClockPeriod} / 2]

#-------------------------------------------------------------------------------
# PSoC / Emif - Output enable - Read setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifOeSetup      [ expr 1.0 * ${cPsocBusClockPeriod} / 2 ]
set cPsocEmifOeHold       [ expr 1.5 * ${cPsocBusClockPeriod} / 2 ]

#-------------------------------------------------------------------------------
# PSoC / Emif - Data[7:0] - Write setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifDataWrSetup  [ expr 1.0 * ${cPsocBusClockPeriod} - 10 ]
set cPsocEmifDataWrHold   [ expr 2.5 * ${cPsocBusClockPeriod} /  2 ]

#-------------------------------------------------------------------------------
# PSoC / Emif - Data[7:0] - Read setup and hold times
#-------------------------------------------------------------------------------
set cPsocEmifDataRdSetup  [ expr 1.5 * ${cPsocBusClockPeriod} - 5 ]
set cPsocEmifDataRdHold   [ expr 1.5 * ${cPsocBusClockPeriod} + 5 ]




