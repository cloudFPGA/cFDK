# ******************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *-----------------------------------------------------------------------------
# * Created : Nov 10 2017
# * Authors : Francois Abel
# * 
# * Description : A Tcl script that creates the SHELL in so-called "Project Mode"
# *   of the Vivado design flow.
# * 
# * Synopsis : vivado -mode batch -source <this_file> [-notrace]
# *                               [-log     <log_file_name>]
# *                               [-tclargs [script_arguments]]
# *
# * Reference documents:
# *  - UG939 / Lab3 / Scripting the Project Mode.
# *  - UG835 / All  / Vivado Design Suite Tcl Guide. 
# ******************************************************************************


# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
source xpr_settings.tcl


# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set dbgLvl      1

#set log_file   ${argv0}.log


################################################################################
#                                                                              #
#                        *     *                                               #
#                        **   **    **       *    *    *                       #
#                        * * * *   *  *      *    **   *                       #
#                        *  *  *  *    *     *    * *  *                       #
#                        *     *  ******     *    *  * *                       #
#                        *     *  *    *     *    *   **                       #
#                        *     *  *    *     *    *    *                       #
#                                                                              #
################################################################################

if {0} {
    my_dbg_trace "There are $argc arguments to this script" 3
    my_dbg_trace "The name of this script is $argv0"        3
    if { ${argc} > 0} {
        my_dbg_trace "The other arguments are: $argv"       3
    }
}

my_puts "################################################################################"
my_puts "##"
my_puts "##  CREATING PROJECT: ${xprName}  "
my_puts "##"
my_puts "################################################################################"
my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

# Always start from the root directory
#-------------------------------------------------------------------------------
catch { cd ${rootDir} }

# Check if the Xilinx Project Already Exists
#-------------------------------------------------------------------------------
if { [ file exists ${xprDir}/${xprName}.xpr ] == 1 } {
    my_warn_puts "The project \'${xprName}.xpr\' already exists!"
    my_warn_puts "You are about to delete the project directory: '${xprDir}\' "
    my_warn_puts "\t Are you sure (Y/N) ? "
    flush stdout
    set kbdIn [ gets stdin ]
    scan ${kbdIn} "%s" keyPressed
    if { [ string toupper ${keyPressed} ] ne "Y" } {
        my_puts "OK, go it. This script (\'${argv0}\') will be aborted now."
        my_puts "Bye.\n" 
        exit 0
    }
}

# Clean Previous Xilinx Project Directory
#-------------------------------------------------------------------------------
file delete -force ${xprDir}
file mkdir ${xprDir}

# Check if the Managed IP Project Already Exists
#-------------------------------------------------------------------------------
if { ! [ file exists ${ipXprDir}/${ipXprName}.xpr ] } {
    my_info_puts "The managed IP project \'${ipXprName}.xpr\' does not exist yet!"
    my_info_puts "Do you want to generate the IP cores used by the SHELL now (Y/N) ? "
    flush stdout
    set kbdIn [ gets stdin ]
    scan ${kbdIn} "%s" keyPressed
    if { [ string toupper ${keyPressed} ] eq "Y" } {
        set rc [ source ${tclDir}/create_ip_cores.tcl ]
        if { ${rc} != ${OK} } {
            my_puts("")
            my_warn_puts "Failed to generate the IPs cores used by the SHELL!"
            my_warn_puts "  (Number IPs that failed to be generated = ${rc})"
            my_puts("")
        } else {
            my_puts "##  Done with IP cores generation."
        }
    } else {
        my_puts "OK, go it. The generation of the IP cores will be skipped."
        my_puts("")
   }
}


#===============================================================================
# Create Xilinx Project
#===============================================================================
create_project ${xprName} ${xprDir}
my_dbg_trace "Done with create_project." 1

# Set Project Properties
#-------------------------------------------------------------------------------
set          obj               [ get_projects ${xprName} ]
set_property part              ${xilPartName}       $obj                -verbose
set_property "target_language" "Verilog"            $obj                -verbose

set_property top               ${topName}           [ current_fileset ] -verbose
set_property top_file          ${hdlDir}/${topFile} [ current_fileset ] -verbose 

# Add *ALL* the HDL Source Files from the HLD Directory (Recursively) 
#-------------------------------------------------------------------------------
add_files    ${hdlDir}
my_dbg_trace "Done with add_files (HDL)." 1

# Specify the IP Repository Path to make IPs available through the IP Catalog
#  (Must do this because IPs are stored outside of the current project) 
#-------------------------------------------------------------------------------
set_property      ip_repo_paths "${ipDir} ${hlsDir}" [ current_project ]
update_ip_catalog
my_dbg_trace "Done with update_ip_catalog." 1

# Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) to the Project. 
#-------------------------------------------------------------------------------
set ipList [ glob -nocomplain ${ipDir}/ip_user_files/ip/* ]
if { $ipList ne "" } {
    foreach ip $ipList {
        set ipName [file tail ${ip} ]
        add_files ${ipDir}/${ipName}/${ipName}.xci
        my_dbg_trace "Done with add_files: ${ipDir}/${ipName}/${ipName}.xci" 2
    }
}


# Update Compile Order
#-------------------------------------------------------------------------------
update_compile_order -fileset sources_1


#================================================================================
# 
# Add and Manage Constraints
#
#   [INFO] By default, a cloudFPGA SHELL is expected to be provided as a custom
#     IP. As such, is must be synthesized out-of-context (OOC) of the top-level
#     design.
#
#================================================================================

# Add Constraints Files and Disable their usage during implementation
#---------------------------------------------------------------------
add_files -fileset constrs_1 -norecurse [ glob ${xdcDir}/*.xdc ]
set_property used_in_implementation false [ get_files  ${xdcDir}/*.xdc ]

# Manage the Out-of-Context Constraints
#-------------------------------------------------
set_property USED_IN {synthesis} [ get_files  ${xdcDir}/*.xdc ]
set_property USED_IN {synthesis out_of_context} [ get_files ${xdcDir}/*_ooc.xdc ]

my_dbg_trace "Done with adding and managing XDC files." ${dbgLvl}


#================================================================================
# 
# Specify the Synthesis Settings
#
#================================================================================

# Force the Out-Of-Context Mode for this module
#  [INFO] This ensures that no IOBUF get inferred for this module.
#------------------------------------------------------------------
set_property -name {STEPS.SYNTH_DESIGN.ARGS.MORE OPTIONS} -value {-mode out_of_context} -objects [get_runs synth_1]

my_dbg_trace "Done with the synthesis settings." ${dbgLvl}



my_puts "################################################################################"
my_puts "##  DONE WITH PROJECT CREATION "
my_puts "################################################################################"
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"


if {0} {

    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN SYNTHESIS: ${xprName}  "
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    launch_runs synth_1
    wait_on_run synth_1

    my_puts "################################################################################"
    my_puts "##  DONE WITH SYNTHESIS RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"


    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN IMPLEMENTATION: ${xprName}  "
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    set_property startegy HighEffort [ get_runs impl_1 ]
    launch_runs impl_1
    wait_on_run impl_1

    my_puts "################################################################################"
    my_puts "##  DONE WITH IMPLEMENATATION RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN BITTSETREAM GENERATION: ${xprName}  "
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    launch_runs impl_1 -to_step write_bitstream
    wait_on_run impl_1

    my_puts "################################################################################"
    my_puts "##  DONE WITH BITSTREAM GENERATION RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
}



# Close project
#-------------------------------------------------------------------------------
 close_project

# Launch Vivado' GUI
#-------------------------------------------------------------------------------
#catch { cd ${xprDir} }
#start_gui





