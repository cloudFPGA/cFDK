
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel, Burkhard Ringlein  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the TCP interface 
# *   between the cloudFPGA SHELL and the user application ROLE.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "tcp_role_if"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "TCP Role Interface for cloudFPGA"
set ipDescription  "An interface between user's role and the TCP core of the shell."
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
add_files     ${currDir}/src/${projectName}.cpp
add_files     ${currDir}/src/${projectName}.hpp
add_files     ${currDir}/../toe/test/test_toe_utils.cpp
add_files     ${currDir}/../toe/test/test_toe_utils.hpp

add_files -tb ${currDir}/test/test_${projectName}.cpp

add_files     ${currDir}/../toe/src/toe_utils.cpp
add_files     ${currDir}/../toe/src/toe_utils.hpp



# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# config_rtl -reset state

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



