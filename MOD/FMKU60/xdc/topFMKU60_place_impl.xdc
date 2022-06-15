# ******************************************************************************
# * Copyright 2016 -- 2022 IBM Corporation
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
# *                                cloudFPGA
# *
# *-----------------------------------------------------------------------------
# *
# * Title   : Default timing constraints for the module FMKU60.
# *
# * File    : topFMKU60_timg_impl.xdc
# *
# * Tools   : Vivado v2016.4, v2017.4 v2019.2 (64-bit)
# *
# ******************************************************************************


################################################################################
#                                                                              #
#  WARNING: This file makes use of constants which are defined in a TCL file.  #
#           Please see the local file: 'xdc_settings.tcl'.                     #
#                                                                              #
################################################################################


#===============================================================================
# Placement of MIG cores
#===============================================================================
# TODO collides with PR ROLE regions
#create_pblock pblock_MC0
#resize_pblock [get_pblocks pblock_MC0] -add {CLOCKREGION_X1Y0:CLOCKREGION_X2Y2}
#add_cells_to_pblock [get_pblocks pblock_MC0] [get_cells SHELL/MEM/MC0/MCC]
#
#create_pblock pblock_MC1
#resize_pblock [get_pblocks pblock_MC1] -add {CLOCKREGION_X3Y2:CLOCKREGION_X4Y4}
#add_cells_to_pblock [get_pblocks pblock_MC1] [get_cells SHELL/MEM/MC1/MCC]




