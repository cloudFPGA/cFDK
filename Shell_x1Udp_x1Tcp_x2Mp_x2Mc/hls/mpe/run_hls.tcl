# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Aug 2017
# * Authors : Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the "Castor" SMC
# *   process used by the SHELL of the cloudFPGA project.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "mpe"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "MPE for cloudFPGA"
set ipDescription  "Message Passing Engine, aka MPI Layer"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set tbDir        ${currDir}/tb
#set implDir      ${currDir}/${projectName}_prj/${solutionName}/impl/ip 
#set repoDir      ${currDir}/../../ip


# Get targets out of env  
#-------------------------------------------------

set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
#set_top       ${projectName}
set_top       mpe_main


add_files     ${srcDir}/${projectName}.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/${projectName}.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/zrlmpi_int.hpp
add_files     ${srcDir}/zrlmpi_common.cpp 
add_files     ${srcDir}/zrlmpi_common.hpp 

add_files -tb tb/tb_${projectName}.cpp 

open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Simulation and Synthesis
#-------------------------------------------------

if { $hlsSim} { 
  csim_design -compiler gcc -clean
  #csim_design -compiler clang -clean
} else {

  csynth_design
  
  if { $hlsCoSim} {
    #cosim_design -compiler gcc -trace_level all -rtl vhdl
    #cosim_design -compiler clang -trace_level all 
    cosim_design -compiler gcc -trace_level all 
  } else {
  
  # Export RTL
  #-------------------------------------------------
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
  }
}

exit
