# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jun 2017
# * Authors : Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the "Castor" SMC
# *   process used by the SHELL of a cloudFPGA module.
# *   project.
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
set appName        "jacobi2d"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"
set projectName    "${appName}v0"

set ipName         ${projectName}
set ipDisplayName  "Jacobi2D filter with MPI"
set ipDescription  "Application for cloudFPGA"
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
#set_top       ${appName}_main
set_top       mpi_wrapper


add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/MPI.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/MPI.cpp -cflags "-DCOSIM"

#for DEBUG flag 
#add_files -tb src/smc.cpp -cflags "-DDEBUG"
add_files -tb tb/tb_${appName}.cpp 


open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Simulation and Synthesis
#-------------------------------------------------

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

exit
