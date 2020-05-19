# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jun 2017
# * Authors : Burkhard Ringlein (NGL@zurich.ibm.com)
# * 
# * Description : A Tcl script for the HLS batch syhthesis 
# * 
# * Synopsis : vivado_hls -f <this_file>
# * Version: 2.0
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set appName        "mem_test_flash"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"
set projectName    "${appName}"

set ipName         ${projectName}
set ipDisplayName  "MemTestFlash"
set ipDescription  "DDR4 Meomry Test (single channel)"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set tbDir        ${currDir}/tb
#set implDir      ${currDir}/${appName}_prj/${solutionName}/impl/ip 
#set repoDir      ${currDir}/../../ip


# Get targets out of env  
#-------------------------------------------------

set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${appName}_main
#set_top       mpi_wrapper



#for DEBUG flag 
#if { $hlsSim || $hlsCoSim } { 
#  add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM -DDEBUG"
#  add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM -DDEBUG"
#  
#  add_files -tb ${tbDir}/tb_${appName}.cpp -cflags "-DDEBUG"
#
#} else {
  add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM"
  add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM"

  add_files -tb ${tbDir}/tb_${appName}.cpp 
#}

#since DEBUG flag won't work....
add_files ${srcDir}/dynamic.hpp

open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Simulation and Synthesis
#-------------------------------------------------

if { $hlsSim } { 
  csim_design -compiler gcc -clean
} else {

  csynth_design
  
  if { $hlsCoSim } {
    cosim_design -compiler gcc -trace_level all 
  } else {
  
  # Export RTL
  #-------------------------------------------------
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
  }
}

exit
