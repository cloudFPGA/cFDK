#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        XPR/TCL board and project SPECIFIC settings
#  *

#-------------------------------------------------------------------------------
# User Defined Settings (Can be edited)
#-------------------------------------------------------------------------------
set xprName      "shellFMKU60" 
set xilPartName  "xcku060-ffva1156-2-i"
set brdPartName  "FMKU60"

# [INFO] SHELL Naming Convention:
#  Udp  stands for one UDP interface
#  Tcp  stands fro one TCP interface
#  McDp stands for one MemoryChannel with a DualPort interface
#-------------------------------------------------------------- 
#OBSOLETE: set topName      "Shell_x1Udp_x1Tcp_x2Mp_x2Mc"
#OBSOLETE: set topFile      "Shell.v"


#-------------------------------------------------------------------------------
# This Xilinx Project Settings (Do not edit)  
#-------------------------------------------------------------------------------
set currDir      [pwd]
set rootDir      [file dirname [pwd]] 
set hdlDir       ${rootDir}/hdl
set hlsDir       ${rootDir}/hls
set ipDir        ${rootDir}/ip
set tclDir       ${rootDir}/tcl
set xdcDir       ${rootDir}/xdc
set xprDir       ${rootDir}/xpr

#-- IPs Managed by this Xilinx (Xpr) Project
set ipXprName    "managed_ip_project"
set ipXprDir     ${ipDir}/${ipXprName}
set ipXprFile    [file join ${ipXprDir} ${ipXprName}.xpr ]

#-- IPs Managed by the Shell-Role-Architecture (Sra) Project  
#BOSOLETE??: set ipSraDir     ${rootDir}/../../IP
#BOSOLETE??: set ipSraXprName "managed_ip_project"
#BOSOLETE??: set ipSraXprDir  ${ipSraDir}/${ipSraXprName}
#BOSOLETE??: set ipSraXprFile [ file join ${ipSraXprDir} ${ipSraXprName}.xpr ]
#BOSOLETE??: 



