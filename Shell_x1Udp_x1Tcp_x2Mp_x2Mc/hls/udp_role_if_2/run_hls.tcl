# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the UDP interface 
# *   between the cloudFPGA SHELL and the user application ROLE.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# *  Fab: Jan-18-2018 Adds header and environment variables.
# *  Fab: Feb-15-2018 Changed the export procedure.
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "udp_role_if_2"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "UDP Role Interface for cloudFPGA"
set ipDescription  "An interface between user's role and the UDP core of the shell."
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

# Get targets out of env  
#-------------------------------------------------
set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)


# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

add_files     ${srcDir}/${projectName}.cpp
add_files     ${srcDir}/${projectName}.hpp

add_files -tb ${testDir}/tb_${projectName}.cpp

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

if { $hlsSim} { 
  csim_design -compiler gcc -clean
} else {

  csynth_design
  
  if { $hlsCoSim} {
    cosim_design -compiler gcc -trace_level all 
  } else {
  
  # Export RTL
  #-------------------------------------------------
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
  }
}

# Exit Vivado HLS
#--------------------------------------------------
exit

