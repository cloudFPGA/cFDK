#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        XPR/TCL PROJECT SPECIFIC settings
#  *


#source constants and functions 
source ../../../tcl/xpr_constants.tcl

# source board specific settings 
source ../../../../MOD/FMKU60/tcl/xpr_settings.tcl

#-------------------------------------------------------------------------------
# Xilinx Project Settings
#-------------------------------------------------------------------------------
set xprName      "topFMKU60"

set topName      "top"
set topFile      "top.vhdl"

set usedRoleType  "Role_x1Udp_x1Tcp_x2Mp"
set usedShellType "Shell_x1Udp_x1Tcp_x2Mp_x2Mc" 

#TODO !!! 
# usedRole = Dir name; usedRoleType = Entity name 
# BUT: usedShellType = Entity and DIR name 
# -> FIX 




