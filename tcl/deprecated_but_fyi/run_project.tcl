# ******************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *-----------------------------------------------------------------------------
# * Created : Mar 02 2018
# * Authors : Francois Abel
# * 
# * Description : A Tcl script that creates the TOP level project in so-called
# *   "Project Mode" of the Vivado design flow. The design is then further 
# *   synthesized, placed and routed in batch mode.
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
set force    0
set full_src 0
set create   1
set synth    1
set impl     1

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
        { create   "Only run the project creation step." }
        { force    "Continue, even if an old project will be deleted."}
        { full_src "Import SHELL as source-code not as IP-core."} 
        { impl     "Only run the implemenation step."} 
        { synth    "Only run the synthesis step."} 
    }
    set usage "\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        if { ${key} eq "clean" && ${value} eq 1 } {
            set clean    1
            set force    1
            set full_src 0
            set create   1
            set synth    1
            set impl     1
            my_info_puts "The argument \'clean\' is set and takes precedence over \'force\', \'create\', \'synth\' and \'impl \'."
        } else {
            if { ${key} eq "create" && ${value} eq 1 } {
                set create 1
                my_info_puts "The argument \'create\' is set."
            }
            if { ${key} eq "force" && ${value} eq 1 } { 
                set force 1
                my_dbg_trace "Setting force to \'1\' " ${dbgLvl_1}
            }
            if { ${key} eq "full_src" && ${value} eq 1 } { 
                set full_src 1
                my_dbg_trace "Setting full_src to \'1\' " ${dbgLvl_1}
            }
            if { ${key} eq "synth" && ${value} eq 1 } {
                set synth  1
                my_info_puts "The argument \'synth\' is set."
            } 
            if { ${key} eq "impl" && ${value} eq 1 } {
                set impl   1
                 my_info_puts "The argument \'impl\' is set."
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

    # Set IP repository paths
    #-------------------------------------------------------------------------------
    set srcObj [ get_filesets sources_1 ]
    my_dbg_trace "Setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}
    set_property "ip_repo_paths" "${ipDir}" ${srcObj}
    my_dbg_trace "Done with setting ip_repo_paths to ${ipDir}" ${dbgLvl_1}

    # Rebuild user ip_repo's index before adding any source files
    #-------------------------------------------------------------------------------
    update_ip_catalog -rebuild

    # Add *ALL* the HDL Source Files from the HLD Directory (Recursively) 
    #-------------------------------------------------------------------------------
    #OBSOLETE-20180503 set srcObj [ get_filesets sources_1 ]
    #OBSOLETE-20180503 add_files -fileset ${srcObj`} ${hdlDir}
    add_files -fileset ${srcObj} ${hdlDir}
    my_dbg_trace "Finished adding the HDL files of the TOP." ${dbgLvl_1}

    if { ${full_src} } {

        # Add *ALL* the HDL Source Files for the SHELL
        #-------------------------------------------------------------------------------
        add_files     ${rootDir}/../../SHELL/Shell/hdl/
        my_dbg_trace "Done with add_files (HDL) for the SHELL." 1
        
        # IP Cores SHELL
        # Specify the IP Repository Path to make IPs available through the IP Catalog
        #  (Must do this because IPs are stored outside of the current project) 
        #-------------------------------------------------------------------------------
        set ipDirShell ${rootDir}/../../SHELL/Shell/ip/
        set_property ip_repo_paths "${ipDirShell} ${rootDir}/../../SHELL/Shell/hls" [ current_project ]
        update_ip_catalog
        my_dbg_trace "Done with update_ip_catalog for the SHELL" 1
        
        # Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) needed for the SHELL. 
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
        #OBSOLETE add_files -fileset constrs_1 -norecurse [ glob ${rootDir}/../../SHELL/Shell/xdc/*.xdc ]
        
        my_dbg_trace "Done with the import of the SHELL Source files" ${dbgLvl_1}

    } else {

        # Create and customize the SHELL module from the Shell IP
        #-------------------------------------------------------------------------------
        my_puts ""
        my_warn_puts "THE USAGE OF THE SHELL AS A PACKAGED IP IS NOT YET SUPPORTED !!!"
        my_warn_puts "  The script will be aborted here..."
        my_puts ""
        exit ${KO}
        # [TODO] create_ip -name Shell -vendor ZRL -library cloudFPGA -version 1.0 -module_name SuperShell
        # [TODO] update_compile_order -fileset sources_1
        # [TODO] my_dbg_trace "Done with the creation and customization of the SHELL-IP (.i.e SuperShell)." ${dbgLvl_1}
    }

    # Add HDL Source Files for the ROLE
    #-----------------------------------
    add_files -norecurse ${rootDir}/../../ROLE/RoleFlash/hdl/roleFlash.vhdl  
    update_compile_order -fileset sources_1
    my_dbg_trace "Finished adding the  HDL files of the ROLE." ${dbgLvl_1}


    # Update Compile Order
    #-------------------------------------------------------------------------------
    update_compile_order -fileset sources_1

    # Create 'constrs_1' fileset (if not found)
    #-------------------------------------------------------------------------------
    if { [ string equal [ get_filesets -quiet constrs_1 ] "" ] } {
        create_fileset -constrset constrs_1
    }

    # Add Constraints Files
    #  INFO: By default, the order of the XDC files (or Tcl scripts) displayed in
    #        the Vivado IDE defines the read sequence used by the tool when loading
    #        an elaborated or synthesized design into memory (UG903-Ch2). Therefore,
    #        ensure that the file "xdc_settings.tcl" is loaded first because it
    #        defines some of the XDC constraints as variables.
    #  INFO: UG903 recommends to organize the constraints in the following sequence:
    #         Timing Assertions -> Timing Exceptions -> Physical Constraints
    #-------------------------------------------------------------------------------
    set constrObj [ get_filesets constrs_1 ]
    set orderedList "xdc_settings.tcl topFMKU60_Flash_timg.xdc topFMKU60_Flash_pins.xdc  topFMKU60_Flash.xdc"
    #OBSOLETE-20180504 Temporary remove of: topFMKU60_Flash_pr.xdc
    foreach file ${orderedList} {
        if { [ add_files -fileset ${constrObj} -norecurse ${xdcDir}/${file} ] eq "" } {
            my_err_puts "Could not add file \'${file}\' to the fileset \'${constrObj}\' !!!"
            my_err_puts "  The script will be aborted here..."
            my_puts ""
            exit ${KO}        
        }
    }
    #OBSOLETE-20180503 add_files -fileset ${obj} [ glob ${xdcDir}/*.xdc ]
    #OBSOLETE-20180503 set_property PROCESSING_ORDER LATE [ get_files ${xdcDir}/${xprName}_pins.xdc ]

    my_dbg_trace "Done with adding XDC files." ${dbgLvl_1}

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

    # Specify the tcl.pre script to apply before the synthesis run
    set_property STEPS.SYNTH_DESIGN.TCL.PRE  ${xdcDir}/xdc_settings.tcl ${syntObj}

    current_run -synthesis ${syntObj}


    #-------------------------------------------------------------------------------
    # Create 'impl_1' run (if not found)
    #-------------------------------------------------------------------------------
    set year [ lindex [ split [ version -short ] "." ] 0 ]  

    if { [ string equal [ get_runs -quiet impl_1 ] "" ] } {
        create_run -name impl_1 -part ${xilPartName} -flow {Vivado Implementation ${year}} -strategy "Vivado Implementation Defaults" -constrset constrs_1 -parent_run synth_1
    } else {
        set_property strategy "Vivado Implementation Defaults" [ get_runs impl_1 ]
        set_property flow "Vivado Implementation ${year}" [ get_runs impl_1 ]
    }

    # Set the current impl run
    #-------------------------------------------------------------------------------
    set implObj [ get_runs impl_1 ]

    # Specify the tcl.pre script to apply before the implementation run
    #OBSOLETE-20180503 set_property STEPS.OPT_DESIGN.TCL.PRE                 ${xdcDir}/xdc_settings.tcl ${implObj}
    #OBSOLETE-20180503 set_property STEPS.POWER_OPT_DESIGN.TCL.PRE           ${xdcDir}/xdc_settings.tcl ${implObj}
    #OBSOLETE-20180503 set_property STEPS.PLACE_DESIGN.TCL.PRE               ${xdcDir}/xdc_settings.tcl ${implObj}
    #OBSOLETE-20180503 set_property STEPS.PHYS_OPT_DESIGN.TCL.PRE            ${xdcDir}/xdc_settings.tcl ${implObj}
    #OBSOLETE-20180503 set_property STEPS.ROUTE_DESIGN.TCL.PRE               ${xdcDir}/xdc_settings.tcl ${implObj}
    #OBSOLETE-20180503 set_property STEPS.POST_ROUTE_PHYS_OPT_DESIGN.TCL.PRE ${xdcDir}/xdc_settings.tcl ${implObj}

    # Set the current impl run
    #-------------------------------------------------------------------------------
    current_run -implementation ${implObj}


    #-------------------------------------------------------------------------------
    # Create 'sim_1' run (if not found)
    #------------------------------------------------------------------------------- 
    if { [ string equal [ get_runs -quiet sim_1 ] ""] } {
        set_property SOURCE_SET sources_1 [ get_filesets sim_1 ]
        add_files -fileset sim_1 -norecurse  ${rootDir}/sim/tb_topFlash_Shell_Mmio.vhd
        set_property source_mgmt_mode All [ current_project ]
        update_compile_order -fileset sim_1
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








if { ${impl} } {

    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN IMPLEMENTATION: ${xprName}  "
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    if { ! ${create} } {
        open_project ${xprDir}/${xprName}.xpr
    }
    
    # Select a Strategy
    #  Strategies are a defined set of Vivado implementation feature options that control
    #  the implementation results. Vivado Design Suite includes a set of pre-defined 
    #  strategies. You can list the Implementation Strategies using the list_property_value
    #  command (e.g. join [list_property_value strategy [get_runs impl_1] ]).
    #-------------------------------------------------------------------------------
    set_property strategy Performance_Explore ${implObj}

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

    set_property "steps.write_bitstream.args.readback_file" "0" ${implObj}
    set_property "steps.write_bitstream.args.verbose"       "0" ${implObj}

    launch_runs impl_1 -to_step write_bitstream -jobs 8
    wait_on_run impl_1

    my_puts "################################################################################"
    my_puts "##  DONE WITH BITSTREAM GENERATION RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}


# Close project
#-------------------------------------------------------------------------------
 close_project

# OBSOLETE 20180418
# Launch Vivado' GUI
#-------------------------------------------------------------------------------
#catch { cd ${xprDir} }
#start_gui




