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

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

add_files     ${srcDir}/${projectName}.cpp
add_files -tb ${testDir}/test_${projectName}.cpp

open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Synthesis
#-------------------------------------------------
#csim_design -clean              [FIXME]
#csim_design -clean -setup       [FIXME]
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level all

# Export RTL
#-------------------------------------------------
export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}

#####################
# OBSOLETE-20180251 #  
# ###################
# #
# # # Import Implemented IP Into User IP Repository
# # #-------------------------------------------------
# # if { [file exists ${repoDir} ] == 1 } {
# #     if { ${ipPkgFormat} eq "ip_catalog" } {
# #         if { [file exists ${repoDir}/${ipVendor}_${ipLibrary}_${ipName}_${ipVersion} ] } {
# #             file delete -force  ${repoDir}/${ipVendor}_${ipLibrary}_${ipName}_${ipVersion}
# #         }
# #         file copy ${implDir} ${repoDir}/${ipVendor}_${ipLibrary}_${ipName}_${ipVersion}
# #   }
# # } else {
# #     puts "WARNING: The IP repository \"${repoDir}\" does not exist!"
# #     puts "         Cannot copy the implemented IP into the user IP repository."
# # }

# Exit Vivado HLS
#--------------------------------------------------
exit

