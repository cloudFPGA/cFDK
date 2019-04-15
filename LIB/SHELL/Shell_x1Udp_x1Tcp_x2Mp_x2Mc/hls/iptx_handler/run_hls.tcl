# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jan 2018
# * Authors : Jagath Weerasinghe, Francois Abel  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the IP Tx handler
# *   process used by the SHELL of the cloudFPGA module.
# *   project.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *-----------------------------------------------------------------------------
# * Modification History:
# *  Fab: Jan-26-2018 Adds header and variables.
# *  Fab: Feb-15-2018 Changed the export procedure.
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "iptx_handler"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "IP Tx Handler for cloudFPGA"
set ipDescription  "Computes the IP checksum and appends the Ethernet MAC destination address."
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set testDir      ${currDir}/test
set implDir      ${currDir}/${projectName}_prj/${solutionName}/impl/ip 
set repoDir      ${currDir}/../../ip

# Retrieve the HLS target goals from ENV
#-------------------------------------------------
set hlsCSim      $::env(hlsCSim)
set hlsCSynth    $::env(hlsCSynth)
set hlsCoSim     $::env(hlsCoSim)
set hlsRtl       $::env(hlsRtl)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

# Add files
#-------------------------------------------------
add_files     ${srcDir}/${projectName}.cpp
add_files     ${srcDir}/${projectName}.hpp

add_files -tb ${testDir}/test_${projectName}.cpp

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    csim_design
}  

# Run C Synthesis (refer to UG902)
#-------------------------------------------------
if { $hlsCSynth} { 
    csynth_design
}

# Run C/RTL CoSimulation (refer to UG902)
#-------------------------------------------------
if { $hlsCoSim } {
    cosim_design -tool xsim -rtl verilog -trace_level all
}

# Export RTL (refer to UG902)
#   -format ( sysgen | ip_catalog | syn_dcp )
#-------------------------------------------------
if { $hlsRtl } {
    export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
}

# Exit Vivado HLS
#--------------------------------------------------
exit



