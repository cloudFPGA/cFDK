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



## IMPORT ENVIRONMENTs
set cFpIpDir $env(cFpIpDir)
set cFpMOD   $env(cFpMOD)
set usedRoleDir $env(usedRoleDir)
set usedRole2Dir $env(usedRole2Dir)
set cFpSRAtype $env(cFpSRAtype)
set cFpRootDir $env(cFpRootDir)
set cFpXprDir $env(cFpXprDir)
set cFpDcpDir $env(cFpDcpDir)

#source constants and functions 
#source xpr_constants.tcl
source ${cFpRootDir}/cFDK/SRA/LIB/tcl/xpr_constants.tcl
# source board specific settings 
#source ../../../MOD/${cFpMOD}/tcl/xpr_settings.tcl
source ${cFpRootDir}/cFDK/MOD/${cFpMOD}/tcl/xpr_settings.tcl

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
set tclTopDir    ${cFpRootDir}/TOP/tcl/
#set xdcDir       ${rootDir}/xdc
#set xdcDir       ../../../MOD/${cFpMOD}/xdc/
set xdcDir       ${cFpRootDir}/cFDK/MOD/${cFpMOD}/xdc/
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

#-------------------------------------------------------------------------------
# Debug outpus
#-------------------------------------------------------------------------------

my_info_puts "RootDir: ${rootDir}"
my_info_puts "ipDir: ${ipDir}"
my_info_puts "xdcDir: ${xdcDir}"
my_info_puts "xprDir: ${xprDir}"
my_info_puts "dcpDir: ${dcpDir}"
my_info_puts "Role 1 Dir: ${usedRoleDir}"
my_info_puts "Role 2 Dir: ${usedRole2Dir}"


