# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel, Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch compilation, simulation,
# *   synthesis of the Tx Engine (TXe) of the TCP offload engine used by the
# *   shell of the cloudFPGA module.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "tx_engine"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "TCP/TXe"
set ipDescription  "Tx Engine"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

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
set hlsRtl       $::env(hlsRtl)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

# Add source files
#-------------------------------------------------
add_files     ${currDir}/src/${projectName}.cpp
add_files     ${currDir}/../../../toe/src/toe_utils.cpp
add_files     ${currDir}/../../../toe/test/test_toe_utils.cpp

# Add test bench files
#-------------------------------------------------
add_files -tb ${currDir}/../../../toe/src/toe.cpp
add_files -tb ${currDir}/../../../toe/src/state_table/state_table.cpp
add_files -tb ${currDir}/../../../toe/src/session_lookup_controller/session_lookup_controller.cpp
add_files -tb ${currDir}/../../../toe/src/rx_sar_table/rx_sar_table.cpp
add_files -tb ${currDir}/../../../toe/src/tx_sar_table/tx_sar_table.cpp
add_files -tb ${currDir}/../../../toe/src/retransmit_timer/retransmit_timer.cpp
add_files -tb ${currDir}/../../../toe/src/probe_timer/probe_timer.cpp
add_files -tb ${currDir}/../../../toe/src/close_timer/close_timer.cpp
add_files -tb ${currDir}/../../../toe/src/event_engine/event_engine.cpp
add_files -tb ${currDir}/../../../toe/src/ack_delay/ack_delay.cpp
add_files -tb ${currDir}/../../../toe/src/port_table/port_table.cpp
add_files -tb ${currDir}/../../../toe/src/rx_engine/src/rx_engine.cpp
add_files -tb ${currDir}/../../../toe/src/rx_app_if/rx_app_if.cpp
add_files -tb ${currDir}/../../../toe/src/rx_app_stream_if/rx_app_stream_if.cpp
add_files -tb ${currDir}/../../../toe/src/tx_app_interface/tx_app_interface.cpp
add_files -tb ${currDir}/../../../toe/src/tx_app_stream/tx_app_stream.cpp

add_files -tb ${currDir}/../../../toe/test/dummy_memory/dummy_memory.cpp

add_files -tb ${currDir}/test/test_${projectName}.cpp -cflags "-fstack-check"

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

#--------------------------------------------
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

#--------------------------------------------
# Specifying Compiler-FIFO Depth (see UG902)
#--------------------------------------------
# Start Propagation 
#  - disable: The compiler might automatically create a start FIFO to propagate a start token
#             to an internal process. Such FIFOs can sometimes be a bottleneck for performance,
#             in which case you can increase the default size (fixed to 2). However, if an
#             unbounded slack between producer and consumer is needed, and internal processes
#              can run forever, fully and safely driven by their inputs or outputs (FIFOs or
#             PIPOs), these start FIFOs can be removed, at user's risk, locally for a given 
#             dataflow region.
#------------------------------------------------------------------------------------------------
# [TODO - Check vivado_hls version and only enable this command if >= 2018]
# config_rtl -disable_start_propagation

#----------------------------------------------------
# Configuring the behavior of the front-end compiler
#----------------------------------------------------
#  -name_max_length: Specify the maximum length of the function names. If the length of one name
#                    is over the threshold, the last part of the name will be truncated.
#  -pipeline_loops : Specify the lower threshold used during pipelining loops automatically. The
#                    default is '0' for no automatic loop pipelining. 
#------------------------------------------------------------------------------------------------
config_compile -name_max_length 128 -pipeline_loops 0

#------------------------------------------
# Set Dataflow Checking Level (see UG9020)
#------------------------------------------
# - error  : Vivado HLS has a dataflow checker which, when enabled, checks the code to see if it
#            is in the recommended canonical form.
# - warning: By default this checker is set to warning.
# -off     : To disable the checker.
#------------------------------------------------------------------------------------------------
config_dataflow -strict_mode error

# Run C Simulation (refer to UG902)
#-------------------------------------------------
if { $hlsCSim} {
    csim_design -setup -clean -compiler gcc
    # [TODO] csim_design -argv "../../../../test/testVectors/ipRx_OneSynPkt.dat"
    # [TODO] csim_design -argv "../../../../test/testVectors/ipRx_OnePkt.dat"

    csim_design -argv "1 ../../../../test/testVectors/appRx_OneSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_TwoSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_ThreeSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_FourLongSeg.dat"
    csim_design -argv "1 ../../../../test/testVectors/appRx_EightSeg.dat"

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
    # [OPTION] set_directive_dataflow rx_engine 

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
    cosim_design -tool xsim -rtl verilog -disable_deadlock_detection -trace_level none -argv "1 ../../../../test/testVectors/appRx_OneSeg.dat"
    cosim_design -tool xsim -rtl verilog -disable_deadlock_detection -trace_level none -argv "1 ../../../../test/testVectors/appRx_TwoSeg.dat"
    cosim_design -tool xsim -rtl verilog -disable_deadlock_detection -trace_level none -argv "1 ../../../../test/testVectors/appRx_ThreeSeg.dat"
    cosim_design -tool xsim -rtl verilog -disable_deadlock_detection -trace_level none -argv "1 ../../../../test/testVectors/appRx_FourLongSeg.dat"
    cosim_design -tool xsim -rtl verilog -disable_deadlock_detection -trace_level none -argv "1 ../../../../test/testVectors/appRx_EightSeg.dat"
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
#    Obtains more accurate timing and utilization data for the specified HDL using RTL synthesis.
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
# -vivado_synth_design_args {args...}
#    Specifies the value to pass to 'synth_design' within the export_design -evaluate Vivado synthesis run.
# -vivado_report_level <value>
#    Specifies the utilization and timing report options.
#---------------------------------------------------------------------------------------------------
if { $hlsRtl } {
    switch $hlsRtl {
        1 {
            export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        2 {
            export_design -flow syn -rtl verilog -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        3 {
            export_design -flow impl -rtl verilog -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
        }
        default { 
            puts "####  INVALID VALUE ($hlsRtl) ####"
            exit 1
        }
    }
    puts "#############################################################"
    puts "####                                                     ####"
    puts "####          SUCCESSFUL EXPORT OF THE DESIGN            ####"
    puts "####                                                     ####"
    puts "#############################################################"

}

# Exit Vivado HLS
#--------------------------------------------------
exit

