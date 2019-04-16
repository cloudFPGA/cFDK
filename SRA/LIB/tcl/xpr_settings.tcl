#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        XPR/TCL PROJECT SPECIFIC settings; 
#  *        Generic file to extract ENVIRONMENT variables
#  *


#source constants and functions 
source ./xpr_constants.tcl

## IMPORT ENVIRONMENTs
set cFpIpDir $env(cFpIpDir)
set cFpMOD   $env(cFpMOD)
set usedRole $env(usedRole)
set usedRole2 $env(usedRole2)
set cFpSRAtype $env(cFpSRAtype)
set cFpRootDir $env(cFpRootDir)
set cFpXprDir $env(cFpXprDir)
set cFpDcpDir $env(cFpDcpDir)

# source board specific settings 
source ../../../MOD/${cFpMOD}/tcl/xpr_settings.tcl

#-------------------------------------------------------------------------------
# TOP  Project Settings  
#-------------------------------------------------------------------------------
#set currDir      [pwd]
set rootDir      ${cFpRootDir} 
#set hdlDir       ${rootDir}/hdl
#set hlsDir       ${rootDir}/hls
#set ipDir        ${rootDir}/../../IP/
set ipDir        ${cFpIpDir}
#set tclDir       ${rootDir}/tcl
#set xdcDir       ${rootDir}/xdc
#set xprDir       ${rootDir}/xpr 
set xprDir       ${cFpXprDir}
#set dcpDir       ${rootDir}/dcps
set dcpDir       ${cFpDcpDir}

# Not used: set ipXprDir     ${ipDir}/managed_ip_project
# Not used:set ipXprName    "managed_ip_project"
# Not used: set ipXprFile    [file join ${ipXprDir} ${ipXprName}.xpr ]



#-------------------------------------------------------------------------------
# Xilinx Project Settings
#-------------------------------------------------------------------------------
set xprName      "top${cFpMOD}"

set topName      "top${cFpMOD}"
set topFile      "top.vhdl"

set usedRoleType  "Role_${cFpSRAtype}"
set usedShellType "Shell_${cFpSRAtype}"


#TODO !!! 
# usedRole = Dir name; usedRoleType = Entity name 
# BUT: usedShellType = Entity and DIR name 
# -> FIX 




