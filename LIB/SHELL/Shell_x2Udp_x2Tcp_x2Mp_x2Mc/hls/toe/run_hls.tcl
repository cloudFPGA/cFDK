# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Jagath Weerasinghe, Francois Abel  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the TCP offload 
# *   engine used by the shell of the cloudFPGA module.
# *   project.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# *  Fab: Jan-16-2018 Adds header and environment variables.
# *  Fab: Feb-15-2018 Changed the export procedure.
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "toe"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "TCP Offload Engine for cloudFPGA"
set ipDescription  "Handles TCP packets."
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

# Add files
#-------------------------------------------------
add_files     ${srcDir}/${projectName}.cpp
add_files -tb ${testDir}/test_${projectName}.cpp

add_files ${srcDir}/ack_delay/ack_delay.cpp
add_files ${srcDir}/close_timer/close_timer.cpp
add_files ${srcDir}/dummy_memeory/dummy_memory.cpp
add_files ${srcDir}/event_engine/event_engine.cpp
add_files ${srcDir}/port_table/port_table.cpp
add_files ${srcDir}/probe_timer/probe_timer.cpp
add_files ${srcDir}/retransmit_timer/retransmit_timer.cpp
add_files ${srcDir}/rx_app_if/rx_app_if.cpp
add_files ${srcDir}/rx_app_stream_if/rx_app_stream_if.cpp
add_files ${srcDir}/rx_engine/rx_engine.cpp
add_files ${srcDir}/rx_sar_table/rx_sar_table.cpp
add_files ${srcDir}/session_lookup_controller/session_lookup_controller.cpp
add_files ${srcDir}/state_table/state_table.cpp
add_files ${srcDir}/tx_app_if/tx_app_if.cpp
add_files ${srcDir}/tx_app_interface/tx_app_interface.cpp
add_files ${srcDir}/tx_app_stream_if/tx_app_stream_if.cpp
add_files ${srcDir}/tx_engine/tx_engine.cpp
add_files ${srcDir}/tx_sar_table/tx_sar_table.cpp

open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Synthesis
#-------------------------------------------------
csynth_design

# Export RTL (refer to UG902)
#   -format ( sysgen | ip_catalog | syn_dcp )
#-------------------------------------------------
export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}

# Exit Vivado HLS
#--------------------------------------------------
exit

