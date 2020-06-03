# ******************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *-----------------------------------------------------------------------------
# * Created : Mar 02 2018
# * Authors : Francois Abel, Burkhard Ringlein
# * 
# * Description : A Tcl script that creates a toplevel project in so-called
# *   "Project Mode" of the Vivado design flow. The design can then be further 
# *   synthesized with the Vivado GUI or in Vivado batch mode.
# * 
# * Synopsis : vivado -mode batch -source <this_file> [-notrace]
# *                               [-log     <log_file_name>]
# *                               [-tclargs [script_arguments]]
# *
# * Reference documents:
# *  - UG939 / Lab3 / Scripting the Project Mode.
# *  - UG835 / All  / Vivado Design Suite Tcl Guide.
# *  - UG903 / Ch2  / Constraints Methodology.
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

# Global variables
set clean    0
set create   0
set force    0
set gui      0
set synth    0


#-------------------------------------------------------------------------------
# Parsing of the Command Line
#  Note: All the strings after the '-tclargs' option are considered as TCL
#        arguments to this script and are passed on to TCL 'argc' and 'argv'.
#-------------------------------------------------------------------------------
if { $argc > 0 } {
    my_dbg_trace "There are [ llength $argv ] TCL arguments to this script." ${dbgLvl_1}
    set i 0
    foreach arg $argv {
        my_dbg_trace "  argv\[$i\] = $arg " ${dbgLvl_1}
        set i [ expr { ${i} + 1 } ]
    }
    # Step-1: Processing of '$argv' using the 'cmdline' package
    set options {
        { clean    "Start with a clean new project directory." }
        { create   "Only run the project creation step." }
        { force    "Continue, even if an old project will be deleted."}
        { gui      "Launch Vivado's GUI at the end of this script." }
        { synth    "Only run the synthesis step."}
    }
    set usage "\nIT IS STRONGLY RECOMMENDED TO CALL THIS SCRIPT ONLY THROUGH THE CORRESPONDING MAKEFILES\n\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the TCL arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_3}
 
        if { ${key} eq "clean" && ${value} eq 1 } {
            set clean    1
            set force    1
            set create   1
            set synth    1
            my_dbg_trace "The argument \'clean\' is set and takes precedence over \'force\', \'create\' and \'synth\'." ${dbgLvl_3}
        } else {
            if { ${key} eq "create" && ${value} eq 1 } {
                set create 1
                my_dbg_trace "The argument \'create\' is set." ${dbgLvl_3}
            }
            if { ${key} eq "force" && ${value} eq 1 } { 
                set force 1
                my_dbg_trace "The argument \'force\' is set." ${dbgLvl_3}
            }
            if { ${key} eq "gui" && ${value} eq 1 } { 
                set gui 1
                my_dbg_trace "The argument \'gui\' is set." ${dbgLvl_3}
            }
            if { ${key} eq "synth" && ${value} eq 1 } {
                set synth  1
                my_dbg_trace "The argument \'synth\' is set." ${dbgLvl_3}
            } 
        } 
    }
}

if { ${create} } {

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
    if { ${clean} } {
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
                exit ${OK}
            }
        }  
    }
    create_project ${xprName} ${xprDir} -force
    my_dbg_trace "Done with project creation." ${dbgLvl_1}


    # Set Project Properties
    #-------------------------------------------------------------------------------
    set          xprObj            [ get_projects ${xprName} ]

    set_property part              ${xilPartName}       ${xprObj}           -verbose
    set_property "target_language" "VHDL"               ${xprObj}           -verbose


    set_property top               ${topName}           [ current_fileset ] -verbose
    set_property top_file          ${hdlDir}/${topFile} [ current_fileset ] -verbose 

    set_property "default_lib"                "xil_defaultlib" ${xprObj}
    set_property "ip_cache_permissions"       "read write"     ${xprObj}
    set_property "sim.ip.auto_export_scripts" "1"              ${xprObj}
    set_property "simulator_language"         "Mixed"          ${xprObj}
    set_property "xsim.array_display_limit"   "64"             ${xprObj}
    set_property "xsim.trace_limit"           "65536"          ${xprObj}

    set_property "ip_output_repo" "${xprDir}/${xprName}/${xprName}.cache/ip" ${xprObj}

    my_dbg_trace "Done with set project properties." ${dbgLvl_1}

    # Create 'sources_1' fileset (if not found)
    #-------------------------------------------------------------------------------
    if { [ string equal [ get_filesets -quiet sources_1 ] "" ] } {
        create_fileset -srcset sources_1
    }

    # IP Cores ROLE & IP Repository Paths
    # Specify the IP Repository Path to make IPs available through the IP Catalog
    #  (Must do this because IPs are stored outside of the current project) 
    #-------------------------------------------------------------------------------
    set srcObj [ get_filesets sources_1 ]
    my_dbg_trace "Setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}
    set_property "ip_repo_paths" "${ipDir}" ${srcObj}
    set_property ip_repo_paths [ concat [ get_property ip_repo_paths [current_project] ] \
                                                         ${hlsDir} ] [current_project]
    my_dbg_trace "Done with setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}

    # Rebuild user ip_repo's index before adding any source files
    #-------------------------------------------------------------------------------
    update_ip_catalog -rebuild

    # Add *ALL* the HDL Source Files from the HLD Directory 
    #-------------------------------------------------------------------------------
    add_files -fileset ${srcObj} ${hdlDir}
    my_dbg_trace "Finished adding the HDL files of the TOP." ${dbgLvl_1}

    # Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) used by the ROLE 
    #-------------------------------------------------------------------------------
    set ipList [ glob -nocomplain ${ipDir}/ip_user_files/ip/* ]
    if { $ipList ne "" } {
        foreach ip $ipList {
            set ipName [file tail ${ip} ]
            add_files ${ipDir}/${ipName}/${ipName}.xci
            my_dbg_trace "Done with add_files for ROLE: ${ipDir}/${ipName}/${ipName}.xci" 2
        }
    }

    # Turn VHDL-2008 mode on 
    #-------------------------------------------------------------------------------
    set_property file_type {VHDL 2008} [ get_files *.vhd* ]

    # Create 'constrs_1' fileset (if not found)
    #-------------------------------------------------------------------------------
    if { [ string equal [ get_filesets -quiet constrs_1 ] "" ] } {
        create_fileset -constrset constrs_1
    }

    # Add Constraints Files
    #
    #  INFO: By default, the order of the XDC files (or Tcl scripts) displayed in
    #        the Vivado IDE defines the read sequence used by the tool when loading
    #        an elaborated or synthesized design into memory (UG903-Ch2). Therefore,
    #        if you are using a TCL file that contains variables and constants that
    #        are used within the XDC constraint files, ensure that this TCL file is
    #        is loaded first (e.g. by adding it into the "orderedList" variable).
    #
    #  INFO: UG903 recommends to organize the constraints in the following sequence:
    #         Timing Assertions -> Timing Exceptions -> Physical Constraints
    #-------------------------------------------------------------------------------
    if { [ file exists ${xdcDir} ] } {
        set constrObj [ get_filesets constrs_1 ]
        # If needed, you may want to populate the following list
        set orderedList ""

        if { [ llength orderedList ] != 0 } {
            # The user is willing to specify an ordered list of files.
            foreach file ${orderedList} {
                if { [ file exists ${xdcDir}/${file} ] == 0 } {
                    my_err_puts "Could not find file \'${xdcDir}/${file}\' for addition to the fileset \'${constrObj}\' !!!"
                    my_err_puts "  The script will be aborted here..."
                    my_puts ""
                    exit ${KO}   
                }
                if { [ add_files -fileset ${constrObj} -norecurse ${xdcDir}/${file} ] eq "" } {
                    my_err_puts "Could not add file \'${file}\' to the fileset \'${constrObj}\' !!!"
                    my_err_puts "  The script will be aborted here..."
                    my_puts ""
                    exit ${KO}   
                }
                my_dbg_trace "Adding constraint file ${xdcDir}/${file}." ${dbgLvl_2}
            }
        } else {
            # Automatically add any XDC and TCL files that are present in the XDC directory.
            add_files -fileset ${constrObj} [ glob ${xdcDir}/*.tcl ]
            my_dbg_trace "Adding constraint files ${xdcDir}/*.tcl" ${dbgLvl_2}
            add_files -fileset ${constrObj} [ glob ${xdcDir}/*.xdc ]
            my_dbg_trace "Adding constraint files ${xdcDir}/*.xdc" ${dbgLvl_2}
        }
        my_dbg_trace "Done with adding XDC files." ${dbgLvl_1}
    }
   
  
    #-------------------------------------------------------------------------------
    # Create 'synth_1' run (if not found)
    #------------------------------------------------------------------------------- 
    set year [ lindex [ split [ version -short ] "." ] 0 ]

    if { [ string equal [ get_runs -quiet synth_1 ] ""] } {
        create_run -name synth_1 -part ${xilPartName} -flow {Vivado Synthesis ${year}} -strategy "Vivado Synthesis Defaults" -constrset constrs_1
    } else {
        set_property strategy "Vivado Synthesis Defaults" [ get_runs synth_1 ]
        set_property flow "Vivado Synthesis ${year}" [ get_runs synth_1 ]
    }

    # Set the current synth run
    set syntObj [ get_runs synth_1 ]
    current_run -synthesis ${syntObj}

    #-------------------------------------------------------------------------------
    # Create 'sim_1' run (if not found)
    #------------------------------------------------------------------------------- 
    if { [ file exists ${simDir} ] } {
        if { [ string equal [ get_runs -quiet sim_1 ] ""] } {
            set simObj [ get_filesets sim_1 ]
            set_property SOURCE_SET sources_1 ${simObj}
            add_files -fileset ${simObj} [ glob ${simDir}/*.vhd* ]
            set_property source_mgmt_mode All [ current_project ]
            update_compile_order -fileset ${simObj}
        }
    }
    my_puts "################################################################################"
    my_puts "##  DONE WITH PROJECT CREATION "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}






if { ${synth} } {

    my_puts "################################################################################"
    my_puts "##" 
    my_puts "##  RUN SYNTHESIS: ${xprName}  " 
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    if { ! ${create} } {
        open_project ${xprDir}/${xprName}.xpr
        # Reset the previous run 'synth_1' before launching a new one
        reset_run synth_1
    }

    launch_runs synth_1 -jobs 8
    wait_on_run synth_1 

    my_puts "################################################################################"
    my_puts "##  DONE WITH SYNTHESIS RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n" 

}


# Close project
#-------------------------------------------------------------------------------
catch {close_project}


# Launch Vivado' GUI
#-------------------------------------------------------------------------------
if { ${gui} } {
    catch { cd ${xprDir} }
    start_gui
}





