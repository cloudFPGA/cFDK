# *
# * Copyright 2016 -- 2021 IBM Corporation
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
# *

# ******************************************************************************
# * 
# * Description : A Tcl script to environment variables and to set specific
# *                project settings.
# *
# ******************************************************************************


# Retreive the Vivado version 
#-------------------------------------------------
set VIVADO_VERSION [file tail $::env(XILINX_VIVADO)]

#-------------------------------------------------------------------------------
# ENV Import
#-------------------------------------------------------------------------------
set cFpIpDir $env(cFpIpDir)
set cFpMOD   $env(cFpMOD)
set usedRoleDir $env(usedRoleDir)
set usedRole2Dir $env(usedRole2Dir)
set cFpSRAtype $env(cFpSRAtype)
set cFpRootDir $env(cFpRootDir)
set cFpXprDir $env(cFpXprDir)
set cFpDcpDir $env(cFpDcpDir)

# Source constants and functions 
source ${cFpRootDir}/cFDK/SRA/LIB/tcl/xpr_constants.tcl
# Source board specific settings 
source ${cFpRootDir}/cFDK/MOD/${cFpMOD}/tcl/xpr_settings.tcl

#-------------------------------------------------------------------------------
# TOP Project Settings
#-------------------------------------------------------------------------------
set rootDir      ${cFpRootDir} 
set ipDir        ${cFpIpDir}
set tclTopDir    ${cFpRootDir}/TOP/tcl/
set xdcDir       ${cFpRootDir}/cFDK/MOD/${cFpMOD}/xdc/
set xprDir       ${cFpXprDir}
set dcpDir       ${cFpDcpDir}

# Not used: set ipXprDir     ${ipDir}/managed_ip_project
# Not used: set ipXprName    "managed_ip_project"
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


