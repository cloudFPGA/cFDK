# ******************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *-----------------------------------------------------------------------------
# * Created : Mar 02 2018
# * Authors : Francois Abel
# * 
# * Description : A Tcl script that creates the TOP level project in so-called
# *   "Project Mode" of the Vivado design flow.
# * 
# * Synopsis : vivado -mode batch -source <this_file> [-notrace]
# *                               [-log     <log_file_name>]
# *                               [-tclargs [script_arguments]]
# *
# * Reference documents:
# *  - UG939 / Lab3 / Scripting the Project Mode.
# *  - UG835 / All  / Vivado Design Suite Tcl Guide. 
# ******************************************************************************

package require cmdline

# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
source xpr_settings.tcl

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set dbgLvl_1         1
set dbgLvl_2         2
set dbgLvl_3         3


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

# By default, do not clean the entire project directory 
set gCleanXprDir 0
set force 0
set full_src 0

#-------------------------------------------------------------------------------
# Parsing of the Command Line
#  Note: All the strings after the '-tclargs' option are considered as TCL
#        arguments to this script and are passed on to TCL 'argc' and 'argv'.
#-------------------------------------------------------------------------------
if { $argc > 0 } {
    my_dbg_trace "There are [ llength $argv ] TCL arguments to this script." ${dbgLvl_1}
    set i 0
    foreach arg $argv {
        my_dbg_trace "  argv\[$i\] = $arg " ${dbgLvl_2}
        set i [ expr { ${i} + 1 } ]
    }
    # Step-1: Processing of '$argv' using the 'cmdline' package
    set options {
        { clean    "Start with a clean new project directory." }
        { force    "Continue, even if an old project will be deleted."}
        { full_src "Import SHELL as source-code not as IP-core."} 
    }
    set usage "\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        #if { ${key} eq "help" } {
        #    help;
        #    return ${::OK}
        #}
        if { ${key} eq "clean" && ${value} eq 1 } {
            set gCleanXprDir 1
            my_dbg_trace "Setting gCleanXprDir to \'1\' " ${dbgLvl_1}
        } 
        if { ${key} eq "force" && ${value} eq 1 } { 
          set force 1
          my_dbg_trace "Setting force to \'1\' " ${dbgLvl_1}
        }
        if { ${key} eq "full_src" && ${value} eq 1 } { 
          set full_src 1
          my_dbg_trace "Setting full_src to \'1\' " ${dbgLvl_1}
        }
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

# If requested , clean the files of the previous run
#-------------------------------------------------------------------------------
if { ${gCleanXprDir} } {
    # Start with a clean new project
    file delete -force ${xprDir}
    my_dbg_trace "Finished cleaning the project directory." ${dbgLvl_1}
} 

# Create the Xilinx project
#-------------------------------------------------------------------------------
if { [ file exists ${xprDir} ] != 1 } {
    file mkdir ${xprDir}
} else {
    # Check if the Xilinx Project Already Exists
    if { [ file exists ${xprDir}/${xprName}.xpr ] == 1 && ! ${force} } {
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
}
create_project ${xprName} ${xprDir} -force
my_dbg_trace "Done with project creation." ${dbgLvl_1}


# Set Project Properties
#-------------------------------------------------------------------------------
set          obj               [ get_projects ${xprName} ]

set_property part              ${xilPartName}       ${obj}              -verbose
set_property "target_language" "VHDL"               ${obj}              -verbose


set_property top               ${topName}           [ current_fileset ] -verbose
set_property top_file          ${hdlDir}/${topFile} [ current_fileset ] -verbose 

set_property "default_lib"                "xil_defaultlib" ${obj}
set_property "ip_cache_permissions"       "read write"     ${obj}
set_property "sim.ip.auto_export_scripts" "1"              ${obj}
set_property "simulator_language"         "Mixed"          ${obj}
set_property "xsim.array_display_limit"   "64"             ${obj}
set_property "xsim.trace_limit"           "65536"          ${obj}

set_property "ip_output_repo" "${xprDir}/${xprName}/${xprName}.cache/ip" ${obj}

my_dbg_trace "Done with set project properties." ${dbgLvl_1}


# Create 'sources_1' fileset (if not found)
#-------------------------------------------------------------------------------
if { [ string equal [ get_filesets -quiet sources_1 ] "" ] } {
  create_fileset -srcset sources_1
}

# Set IP repository paths
#-------------------------------------------------------------------------------
set obj [ get_filesets sources_1 ]
my_dbg_trace "Setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}
set_property "ip_repo_paths" "${ipDir}" ${obj}
my_dbg_trace "Done with setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}

# Rebuild user ip_repo's index before adding any source files
#-------------------------------------------------------------------------------
update_ip_catalog -rebuild

# Add *ALL* the HDL Source Files from the HLD Directory (Recursively) 
#-------------------------------------------------------------------------------
set obj [ get_filesets sources_1 ]
add_files -fileset ${obj} ${hdlDir}
my_dbg_trace "Finished adding the HDL files of the TOP." ${dbgLvl_1}

if { ${full_src} } {
    # Add *ALL* the HDL Source Files for the SHELL
    #-------------------------------------------------------------------------------
    #add_files    ${hdlDir}
    add_files     ${rootDir}/../../../SHELL/FMKU60/Shell/hdl/
    my_dbg_trace "Done with add_files (HDL) for the SHELL." 1
    
    # IP Cores SHELL
    # Specify the IP Repository Path to make IPs available through the IP Catalog
    #  (Must do this because IPs are stored outside of the current project) 
    #-------------------------------------------------------------------------------
    #set_property      ip_repo_paths "${ipDir} ${hlsDir}" [ current_project ]
    set ipDirShell ${rootDir}/../../../SHELL/FMKU60/Shell/ip/
    set_property      ip_repo_paths "${ipDirShell} ${rootDir}/../../../SHELL/FMKU60/Shell/hls" [ current_project ]
    update_ip_catalog
    my_dbg_trace "Done with update_ip_catalog for the SHELL" 1
    
    # Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) to the SHELL. 
    #-------------------------------------------------------------------------------
    set ipList [ glob -nocomplain ${ipDirShell}/ip_user_files/ip/* ]
    if { $ipList ne "" } {
        foreach ip $ipList {
            set ipName [file tail ${ip} ]
            add_files ${ipDirShell}/${ipName}/${ipName}.xci
            my_dbg_trace "Done with add_files for SHELL: ${ipDir}/${ipName}/${ipName}.xci" 2
        }
    }
    # Update Compile Order
    #-------------------------------------------------------------------------------
    update_compile_order -fileset sources_1
    
    # Add Constraints Files SHELL
    #---------------------------------------------------------------------
    #add_files -fileset constrs_1 -norecurse [ glob ${rootDir}/../../../SHELL/FMKU60/Shell/xdc/*.xdc ]
    
    my_dbg_trace "Done with the import of the SHELL Source files" ${dbgLvl_1}
} else {
    # Create and customize the SHELL module from the Shell IP
    #-------------------------------------------------------------------------------
    create_ip -name Shell -vendor ZRL -library cloudFPGA -version 1.0 -module_name SuperShell
    update_compile_order -fileset sources_1
    my_dbg_trace "Done with the creation and customization of the SHELL-IP (.i.e SuperShell)." ${dbgLvl_1}
}

# Add HDL Source Files for the ROLE
#-----------------------------------
add_files -norecurse ${rootDir}/../../../ROLE/FMKU60/RoleFlash_vhd/hdl/roleFlash.vhdl  
update_compile_order -fileset sources_1
my_dbg_trace "Finished adding the  HDL files of the ROLE." ${dbgLvl_1}

# Update Compile Order
#-------------------------------------------------------------------------------
update_compile_order -fileset sources_1

# Create 'constrs_1' fileset (if not found)
#-------------------------------------------------------------------------------
if { [ string equal [ get_filesets -quiet constrs_1 ] ""]} {
  create_fileset -constrset constrs_1
}

# Add Constraints Files
#-------------------------------------------------------------------------------
set obj [ get_filesets constrs_1 ]
add_files -fileset ${obj} ${xdcDir}
set_property PROCESSING_ORDER LATE [ get_files ${xdcDir}/${xprName}_pins.xdc ]
#OBSOLETE-20180414 set_property used_in_synthesis false [ get_files ${xdcDir}/${xprName}_timg.xdc ]
#OBSOLETE-20180414 set_property used_in_synthesis false [ get_files ${xdcDir}/${xprName}_pins.xdc ]

#OBSOLETE-20180414 set_property used_in_implementation false [get_files Shell.xdc]
my_dbg_trace "Done with adding XDC files." ${dbgLvl_1}


# Create 'synth_1' run (if not found)
#-------------------------------------------------------------------------------
set year [ lindex [ split [ version -short ] "." ] 0 ]  
if { [ string equal [ get_runs -quiet synth_1 ] ""] } {
    create_run -name synth_1 -part ${xilPartName} -flow {Vivado Synthesis ${year}} -strategy "Vivado Synthesis Defaults" -constrset constrs_1
} else {
  set_property strategy "Vivado Synthesis Defaults" [ get_runs synth_1 ]
    set_property flow "Vivado Synthesis ${year}" [ get_runs synth_1 ]
}
set obj [ get_runs synth_1 ]
#OBSOLETE set_property "part" "xcku060-ffva1156-2-i" $obj

# set the current synth run
current_run -synthesis [ get_runs synth_1 ]

# Create 'impl_1' run (if not found)
#-------------------------------------------------------------------------------
set year [ lindex [ split [ version -short ] "." ] 0 ]  
if { [ string equal [ get_runs -quiet impl_1 ] "" ] } {
    create_run -name impl_1 -part ${xilPartName} -flow {Vivado Implementation ${year}} -strategy "Vivado Implementation Defaults" -constrset constrs_1 -parent_run synth_1
} else {
  set_property strategy "Vivado Implementation Defaults" [ get_runs impl_1 ]
    set_property flow "Vivado Implementation ${year}" [ get_runs impl_1 ]
}
set obj [ get_runs impl_1 ]
#OBSOLET set_property "part" "xcku060-ffva1156-2-i" $obj
set_property "steps.write_bitstream.args.readback_file" "0" ${obj}
set_property "steps.write_bitstream.args.verbose"       "0" ${obj}

# Set the current impl run
#-------------------------------------------------------------------------------
current_run -implementation [ get_runs impl_1 ]


my_puts "################################################################################"
my_puts "##  DONE WITH PROJECT CREATION "
my_puts "################################################################################"
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"


my_puts "################################################################################"
my_puts "##"
my_puts "##  RUN SYNTHESIS: ${xprName}  "
my_puts "##"
my_puts "################################################################################"
my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

launch_runs synth_1 -jobs 8
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

#TODO 
#set_property strategy HighEffort [ get_runs impl_1 ]
launch_runs impl_1 -jobs 8
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

launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

my_puts "################################################################################"
my_puts "##  DONE WITH BITSTREAM GENERATION RUN "
my_puts "################################################################################"
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"



# Close project
#-------------------------------------------------------------------------------
 close_project

# OBSOLETE 20180418
# Launch Vivado' GUI
#-------------------------------------------------------------------------------
#catch { cd ${xprDir} }
#start_gui




