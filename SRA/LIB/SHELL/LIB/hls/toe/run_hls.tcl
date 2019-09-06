
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel, Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the TCP offload 
# *   engine used by the shell of the cloudFPGA module.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
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
add_files     ${currDir}/src/${projectName}_utils.cpp
add_files     ${currDir}/test/test_${projectName}_utils.cpp

add_files -tb ${currDir}/test/test_${projectName}.cpp -cflags "-fstack-check"
add_files -tb ${currDir}/test/test_${projectName}_utils.cpp
add_files -tb ${currDir}/test/dummy_memory/dummy_memory.cpp

add_files ${srcDir}/ack_delay/ack_delay.cpp
add_files ${srcDir}/close_timer/close_timer.cpp
add_files ${srcDir}/event_engine/event_engine.cpp
add_files ${srcDir}/port_table/port_table.cpp
add_files ${srcDir}/probe_timer/probe_timer.cpp
add_files ${srcDir}/retransmit_timer/retransmit_timer.cpp
add_files ${srcDir}/rx_app_if/rx_app_if.cpp
add_files ${srcDir}/rx_app_stream_if/rx_app_stream_if.cpp
add_files ${srcDir}/rx_engine/src/rx_engine.cpp
add_files ${srcDir}/rx_sar_table/rx_sar_table.cpp
add_files ${srcDir}/session_lookup_controller/session_lookup_controller.cpp
add_files ${srcDir}/state_table/state_table.cpp
add_files ${srcDir}/toe_utils.cpp
add_files ${srcDir}/tx_app_interface/tx_app_interface.cpp
add_files ${srcDir}/tx_app_stream/tx_app_stream.cpp
add_files ${srcDir}/tx_engine/tx_engine.cpp
add_files ${srcDir}/tx_sar_table/tx_sar_table.cpp

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

#-------------------------------------------
# Controlling the Reset Behavior (see UG902)
#--------------------------------------------
#  - control: This is the default and ensures all control registers are reset. Control registers 
#             are those used in state machines and to generate I/O protocol signals. This setting 
#             ensures the design can immediately start its operation state.
#  - state  : This option adds a reset to control registers (as in the control setting) plus any 
#             registers or memories derived from static and global variables in the C code. This 
#             setting ensures static and global variable initialized in the C code are reset to
#             their initialized value after the reset is applied.
#------------------------------------------------------------------------------------------------
config_rtl -reset control

#----------------------------------------------------
# Configuring the behavior of the front-end compiler
#----------------------------------------------------
#  -name_max_length: Specify the maximum length of the function names. If the length of one name
#                    is over the threshold, the last part of the name will be truncated.
#  -pipeline_loops : Specify the lower threshold used during pipelining loops automatically. The
#                    default is '0' for no automatic loop pipelining. 
#------------------------------------------------------------------------------------------------
config_compile -name_max_length 128 -pipeline_loops 0

# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    csim_design -argv "0 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_OneSynMssPkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_OnePkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_TwoPkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_ThreePkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_FourPkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_FivePkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_Ramp64.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_TwentyPkt.dat"
    csim_design -argv "0 ../../../../test/testVectors/ipRx_ThousandPkt.dat"

    csim_design -argv "1 ../../../../test/testVectors/appRx_OneSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_TwoSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_ThreeSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_FourLongSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_EightSeg.dat"

    csim_design -argv "3 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_OneSynMssPkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_OnePkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_TwoPkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_ThreePkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_FourPkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_FivePkt.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_Ramp64.dat"
    csim_design -argv "3 ../../../../test/testVectors/ipRx_TwentyPkt.dat"

    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF C SIMULATION             ####"
    puts "####                                                     ####"
    puts "#############################################################"    
}

# Run C Synthesis (refer to UG902)
#-------------------------------------------------
if { $hlsCSynth} { 
    csynth_design
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF SYNTHESIS                ####"
    puts "####                                                     ####"
    puts "#############################################################"
}

# Run C/RTL CoSimulation (refer to UG902)
#-------------------------------------------------
if { $hlsCoSim } {
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_OnePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_TwoPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_ThreePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_FourPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_FivePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_TwentyPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "0 ../../../../test/testVectors/ipRx_ThousandPkt.dat"

    cosim_design -tool xsim -rtl verilog -trace_level none -argv "1 ../../../../test/testVectors/appRx_OneSeg.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "1 ../../../../test/testVectors/appRx_TwoSeg.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "1 ../../../../test/testVectors/appRx_ThreeSeg.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "1 ../../../../test/testVectors/appRx_FourLongSeg.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "1 ../../../../test/testVectors/appRx_EightSeg.dat"

    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_OnePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_TwoPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_ThreePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_FourPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_FivePkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "3 ../../../../test/testVectors/ipRx_TwentyPkt.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF CO-SIMULATION            ####"
    puts "####                                                     ####"
    puts "#############################################################"

}

#-----------------------------
# Export RTL (refer to UG902)
#-----------------------------
#
# -description <string>
#    Provides a description for the generated IP Catalog IP.
# -display_name <string>
#    Provides a display name for the generated IP.
# -flow (syn|impl)
#    Obtains more accurate timing  and utilization data for the specified HDL using RTL synthesis.
# -format (ip_catalog|sysgen|syn_dcp)
#    Specifies the format to package the IP.
# -ip_name <string>
#    Provides an IP name for the generated IP.
# -library <string>
#    Specifies  the library name for the generated IP catalog IP.
# -rtl (verilog|vhdl)
#    Selects which HDL is used when the '-flow' option is executed. If not specified, verilog is
#    the default language.
# -vendor <string>
#    Specifies the vendor string for the generated IP catalog IP.
# -version <string>
#    Specifies the version string for the generated IP catalog.
#---------------------------------------------------------------------------------------------------
if { $hlsRtl } {
    export_design -flow syn -rtl verilog -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL EXPORT OF THE DESIGN            ####"
    puts "####                                                     ####"
    puts "#############################################################"

}

# Exit Vivado HLS
#--------------------------------------------------
exit

