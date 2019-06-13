# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel, Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch compilation, simulation,
# *   synthesis of the Rx Engine of the TCP offload engine used by the shell
# *   of the cloudFPGA module.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "rx_engine"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set testDir      ${currDir}/test

# Retrieve the HLS target goals from ENV
#-------------------------------------------------
set hlsCSim      $::env(hlsCSim)
set hlsCSynth    $::env(hlsCSynth)
set hlsCoSim     $::env(hlsCoSim)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

# Add files
#-------------------------------------------------
add_files     ${currDir}/src/${projectName}.cpp
add_files     ${currDir}/../../../toe/src/toe_utils.cpp
add_files     ${currDir}/../../../toe/test/test_toe_utils.cpp

add_files -tb ${currDir}/../../../toe/test/dummy_memory/dummy_memory.cpp
add_files -tb ${currDir}/test/test_${projectName}.cpp -cflags "-fstack-check"

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Request any static or global variable to be reset to its initialized value
# config_rtl -reset state

# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    csim_design -argv "../../../../test/testVectors/ipRx_OneSynPkt.dat"
    csim_design -argv "../../../../test/testVectors/ipRx_OnePkt.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF C SIMULATION             ####"
    puts "####                                                     ####"
    puts "#############################################################"
}

# Run C Synthesis (refer to UG902)
#-------------------------------------------------
if { $hlsCSynth} {

    # If required, you may set the DATAFLOW directive here instead of placing a pragma in the source file
    # set_directive_dataflow rx_engine 

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
    cosim_design -tool xsim -rtl verilog -trace_level none -argv "../../../../test/testVectors/ipRx_OneSynPkt.dat"
    cosim_design -tool xsim -rtl verilog -trace_level all  -argv "../../../../test/testVectors/ipRx_OnePkt.dat"
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL END OF CO-SIMULATION            ####"
    puts "####                                                     ####"
    puts "#############################################################"
}

# Exit Vivado HLS
#--------------------------------------------------
exit

