# /*******************************************************************************
#  * Copyright 2016 -- 2020 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: May 2018
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        TCL file to execute the Vivado commands
#  *




package require cmdline

# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
source ../../cFDK/SRA/LIB/tcl/xpr_settings.tcl

# import environment Variables
set usedRole $env(roleName1)
set usedRole2 $env(roleName2)

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set dbgLvl_1         1
set dbgLvl_2         2
set dbgLvl_3         3

#TODO not used any longer?
#source extra_procs.tcl

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
set create   0
set synth    0
set impl1    0
set impl2    0
set bitGen1  0
set bitGen2  0
set pr             0
set pr_verify      0
set forceWithoutBB 0
set pr_grey_impl   0
set pr_grey_bitgen 0
set link 0
#set activeFlowPr_1 0
#set activeFlowPr_2 0
set impl_opt       0
set use_incr       0
set save_incr      0
set only_pr_bitgen 0
set insert_ila 0


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
        { clean    "Start with a clean new project directory WITHOUT reuse of results or partial reconfiguration." }
        { create   "Only run the project creation step." }
        { force    "Continue, even if an old project will be deleted."}
        { full_src "Import SHELL as source-code not as IP-core."} 
        { impl     "Only run the implemenation step."} 
        { impl2    "Only run the implemenation step for 2nd PR flow."} 
        { implGrey "Only run the implemenation step for GreyBox PR flow."} 
        { synth    "Only run the synthesis step."}
        { bitgen   "Only run the bitfile generation step."}
        { bitgen2   "Only run the bitfile generation step for 2nd PR flow."}
        { bitgenGrey "Only run the bitfile generation step for GreyBox PR flow."}
        { link     "Only run the link step (with or without pr)."}
        { pr       "Activates PARTIAL RECONFIGURATION flow (in all steps)." }
        { pr_verify "Run pr_verify." } 
        { forceWithoutBB "Disable any reuse of intermediate results or the use of Black Boxes."}
        { impl_opt "Optimize implementation for performance (increases runtime)"}
        { use_incr "Use incremental compile (if possible)"}
        { save_incr "Save current implementation for use in incremental compile for non-BlackBox flow."}
        { only_pr_bitgen "Generate only the partial bitfiles for PR-Designs."}
        { insert_ila "Insert the debug nets according to xdc/debug.xdc"}
    }
    set usage "\nIT IS STRONGLY RECOMMENDED TO CALL THIS SCRIPT ONLY THROUGH THE CORRESPONDING MAKEFILES\n\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        if { ${key} eq "clean" && ${value} eq 1 } {
            set clean     1
            set force     1
            set full_src  0
            set create    1
            set synth     1
            set impl      1
            set bitGen    1
            set pr        0
            set link      1
            set pr_verify 0

            my_info_puts "The argument \'clean\' is set and takes precedence over \'force\', \'create\', \'synth\', \'impl \' and \'bitgen\' and DISABLE any PR-Flow Steps."
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
                set impl1   1
                 my_info_puts "The argument \'impl\' is set."
            }
            if { ${key} eq "impl2" && ${value} eq 1 } {
                set impl2   1
                 my_info_puts "The argument \'impl2\' is set."
            }
            if { ${key} eq "implGrey" && ${value} eq 1 } {
                set pr_grey_impl   1
                 my_info_puts "The argument \'implGrey\' is set."
            }
            if { ${key} eq "pr" && ${value} eq 1 } {
              set pr 1
              my_info_puts "The argument \'pr\' is set."
            }
            if { ${key} eq "pr_verify" && ${value} eq 1 } {
              set pr_verify 1
              my_info_puts "The argument \'pr_verify\' is set."
            }
            if { ${key} eq "link" && ${value} eq 1 } {
              set link 1
              my_info_puts "The argument \'bitgen\' is set."
            }
            if { ${key} eq "bitgen" && ${value} eq 1 } {
              set bitGen1 1
              my_info_puts "The argument \'bitgen\' is set."
            }
            if { ${key} eq "bitgen2" && ${value} eq 1 } {
              set bitGen2 1
              my_info_puts "The argument \'bitgen2\' is set."
            }
            if { ${key} eq "bitgenGrey" && ${value} eq 1 } {
              set pr_grey_bitgen 1
              my_info_puts "The argument \'bitgenGrey\' is set."
            }
            if { ${key} eq "forceWithoutBB" && ${value} eq 1 } {
              set forceWithoutBB 1
              my_info_puts "The argument \'forceWithoutBB\' is set."
            }
            if { ${key} eq "impl_opt" && ${value} eq 1 } {
              set impl_opt 1
              my_info_puts "The argument \'impl_opt\' is set."
            }
            if { ${key} eq "use_incr" && ${value} eq 1 } {
              set use_incr 1
              my_info_puts "The argument \'use_incr\' is set."
            }
            if { ${key} eq "save_incr" && ${value} eq 1 } {
              set save_incr 1
              my_info_puts "The argument \'save_incr\' is set."
            }
            if { ${key} eq "only_pr_bitgen" && ${value} eq 1 } {
              set only_pr_bitgen 1
              my_info_puts "The argument \'only_pr_bitgen\' is set."
            }
            if { ${key} eq "insert_ila" && ${value} eq 1 } {
              set insert_ila 1
              my_info_puts "The argument \'insert_ila\' is set."
            }
        } 
    }
}

my_info_puts "usedRole  is set to $usedRole"
my_info_puts "usedRole2 is set to $usedRole2"

# -----------------------------------------------------
# Assert valid combination of arguments 
if {$pr || $link} {
  set forceWithoutBB 0
}

if {$pr && $impl1 && $synth} {
  set link 1
}
if {$only_pr_bitgen} {
  # to deal with https://www.xilinx.com/support/answers/70708.html
  set pr_verify 0
}
# -----------------------------------------------------

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

    #===============================================================================
    #
    # STEP-1 : Create and Specify PROJECT Settings
    #
    #===============================================================================

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
    set_property top_file          ${rootDir}/TOP/hdl/${topFile} [ current_fileset ] -verbose 

    set_property "default_lib"                "xil_defaultlib" ${xprObj}
    set_property "ip_cache_permissions"       "read write"     ${xprObj}
    set_property "sim.ip.auto_export_scripts" "1"              ${xprObj}
    set_property "simulator_language"         "Mixed"          ${xprObj}
    set_property "xsim.array_display_limit"   "64"             ${xprObj}
    set_property "xsim.trace_limit"           "65536"          ${xprObj}

    set_property "ip_output_repo" "${xprDir}/${xprName}/${xprName}.cache/ip" ${xprObj}

    my_dbg_trace "Done with set project properties." ${dbgLvl_1}

    #===============================================================================
    #
    # STEP-2 : Create and Specify SOURCE Settings
    #
    #===============================================================================

    # Create 'sources_1' fileset (if not found)
    #-------------------------------------------------------------------------------
    if { [ string equal [ get_filesets -quiet sources_1 ] "" ] } {
        create_fileset -srcset sources_1
    }

    # Set IP Repository Paths (both SHELL and CORE)
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
    
    #add_files -fileset ${srcObj} ../hdl/
    add_files -fileset ${srcObj} ${rootDir}/TOP/hdl/
    #add_files -fileset ${srcObj} ${hdlDir}/topFlash_pkg.vhdl
    
    # add TOP library files
    add_files -fileset ${srcObj} ${rootDir}/cFDK/SRA/LIB/TOP/hdl/

    # Turn the VHDL-2008 mode on 
    #-------------------------------------------------------------------------------
    set_property file_type {VHDL 2008} [ get_files [ glob -nocomplain ${rootDir}/TOP/hdl/* ] ]
    set_property file_type {VHDL 2008} [ get_files [ glob -nocomplain ${rootDir}/cFDK/SRA/LIB/TOP/hdl/* ] ]

    my_dbg_trace "Finished adding the HDL files of the TOP." ${dbgLvl_1}

    if { ${full_src} } {

        # Add *ALL* the HDL Source Files for the SHELL
        #---------------------------------------------------------------------------
        add_files     ${rootDir}/cFDK/SRA/LIB/SHELL/${cFpSRAtype}/Shell.v
        add_files     ${rootDir}/cFDK/SRA/LIB/SHELL/${cFpSRAtype}/hdl/
        add_files     ${rootDir}/cFDK/SRA/LIB/SHELL/LIB/hdl/

        my_dbg_trace "Done with add_files (HDL) for the SHELL." ${dbgLvl_1}

        # IP Cores SHELL
        # Specify the IP Repository Path to make IPs available through the IP Catalog
        #  (Must do this because IPs are stored outside of the current project) 
        #---------------------------------------------------------------------------
        #set ipDirShell  ${rootDir}/cFDK/SRA/LIB/SHELL/${usedShellType}/ip/
        set ipDirShell  ${ipDir}
        set hlsDirShell ${rootDir}/cFDK/SRA/LIB/SHELL/LIB/hls/
        #OBSOLETE-20180917 set_property ip_repo_paths "${ipDirShell} ${rootDir}/../../SHELL/${usedShellType}/hls" [ current_project ]
        set_property ip_repo_paths [ concat [ get_property ip_repo_paths [current_project] ] \
                                          ${ipDirShell} ${hlsDirShell} ] [current_project]
        update_ip_catalog
        my_dbg_trace "Done with update_ip_catalog for the SHELL" ${dbgLvl_1}
        
        # Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) needed for the SHELL. 
        #---------------------------------------------------------------------------
        set ipList [ glob -nocomplain ${ipDirShell}/ip_user_files/ip/* ]
        if { $ipList ne "" } {
            foreach ip $ipList {
                set ipName [file tail ${ip} ]
                add_files ${ipDirShell}/${ipName}/${ipName}.xci
                my_dbg_trace "Done with add_files for SHELL: ${ipDir}/${ipName}/${ipName}.xci" 2
            }
        }
   
        # Update Compile Order
        #---------------------------------------------------------------------------
        update_compile_order -fileset sources_1
        
        # Add Constraints Files SHELL
        #---------------------------------------------------------------------------
        #OBSOLETE add_files -fileset constrs_1 -norecurse [ glob ${rootDir}/../../SHELL/${usedShellType}/xdc/*.xdc ]
        
        my_dbg_trace "Done with the import of the SHELL Source files" ${dbgLvl_1}

    } else {

        # Create and customize the SHELL module from the Shell IP
        #-------------------------------------------------------------------------------
        my_puts ""
        my_warn_puts "THE USAGE OF THE SHELL AS A STATIC NETLIST IS NOT YET SUPPORTED !!!"
        my_warn_puts "  The script will be aborted here..."
        my_puts ""
        exit ${KO}
        #TODO: maybe import static netlist here?
  }

  if { $forceWithoutBB } {

    # Add HDL Source Files for the ROLE and turn VHDL-2008 mode on
    #---------------------------------------------------------------------------
    add_files  ${usedRoleDir}/hdl/
    set_property file_type {VHDL 2008} [ get_files [ file normalize ${usedRoleDir}/hdl/*.vhd* ] ]
    update_compile_order -fileset sources_1
    my_dbg_trace "Finished adding the  HDL files of the ROLE." ${dbgLvl_1}

    # IP Cores ROLE
    # Specify the IP Repository Path to make IPs available through the IP Catalog
    #  (Must do this because IPs are stored outside of the current project) 
    #----------------------------------------------------------------------------
    set ipDirRole  ${usedRoleDir}/ip
    set hlsDirRole ${usedRoleDir}/hls
    set_property ip_repo_paths [ concat [ get_property ip_repo_paths [current_project] ] \
    ${ipDirRole} ${hlsDirRole} ] [current_project]

    # Add *ALL* the User-based IPs (i.e. VIVADO- as well HLS-based) needed for the ROLE. 
    #---------------------------------------------------------------------------
    set ipList [ glob -nocomplain ${ipDirRole}/ip_user_files/ip/* ]
    if { $ipList ne "" } {
      foreach ip $ipList {
        set ipName [file tail ${ip} ]
        add_files ${ipDirRole}/${ipName}/${ipName}.xci
        my_dbg_trace "Done with add_files for ROLE: ${ipDir}/${ipName}/${ipName}.xci" 2
          }
      }

      update_ip_catalog
      my_dbg_trace "Done with update_ip_catalog for the ROLE" ${dbgLvl_1}

      # Update Compile Order
      #---------------------------------------------------------------------------
      update_compile_order -fileset sources_1
  }



  #===============================================================================
    #
    # STEP-3 : Create and Specify CONSTRAINTS Settings
    #
    #=============================================================================== 

    # Create 'constrs_1' fileset (if not found)
    #-------------------------------------------------------------------------------
    if { [ string equal [ get_filesets -quiet constrs_1 ] "" ] } {
        create_fileset -constrset constrs_1
    }

    # Add Constraints Files
    # cFDK: adding here only the MOD/${cFpMOD}/ files. 
    #  INFO: By default, the order of the XDC files (or Tcl scripts) displayed in
    #        the Vivado IDE defines the read sequence used by the tool when loading
    #        an elaborated or synthesized design into memory (UG903-Ch2). Therefore,
    #        ensure that the file "xdc_settings.tcl" is loaded first because it
    #        defines some of the XDC constraints as variables.
    #  INFO: UG903 recommends to organize the constraints in the following sequence:
    #         Timing Assertions -> Timing Exceptions -> Physical Constraints
    #-------------------------------------------------------------------------------
    set constrObj [ get_filesets constrs_1 ]
    #set orderedList "xdc_settings.tcl topFMKU60_timg.xdc topFMKU60_pins.xdc  topFMKU60.xdc"
    # import orderedList
    source ${xdcDir}/order.tcl 
    foreach file ${orderedList} {
        if { [ add_files -fileset ${constrObj} -norecurse ${xdcDir}/${file} ] eq "" } {
            my_err_puts "Could not add file \'${file}\' to the fileset \'${constrObj}\' !!!"
            my_err_puts "  The script will be aborted here..."
            my_puts ""
            exit ${KO}
        }
    }

    my_dbg_trace "Done with adding XDC files." ${dbgLvl_1}


    #===============================================================================
    #
    # STEP-4: Create and Specify SYNTHESIS Settings
    #
    #=============================================================================== 
    set year [ lindex [ split [ version -short ] "." ] 0 ]

    if { [ string equal [ get_runs -quiet synth_1 ] ""] } {
        # 'synth_1' run was not found --> create run
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


    #===============================================================================
    #
    # STEP-5: Create and Specify IMPLEMENTATION Settings
    #
    #===============================================================================

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

    # Set the current impl run
    #-------------------------------------------------------------------------------
    current_run -implementation ${implObj}


    #===============================================================================
    #
    # STEP-6: Create and Specify SIMULATION Settings
    #
    #===============================================================================

    #-------------------------------------------------------------------------------
    # Create 'sim_1' run (if not found)
    #-------------------------------------------------------------------------------
    # TODO: add SIM 
    #if { [ string equal [ get_runs -quiet sim_1 ] ""] } {
    #    set_property SOURCE_SET sources_1 [ get_filesets sim_1 ]
    #    #OBSOLETE-20180705 add_files -fileset sim_1 -norecurse  ${rootDir}/sim/tb_topFlash_Shell_Mmio.vhd
    #    add_files -fileset sim_1 -norecurse  ${rootDir}/sim

    #    # Turn the VHDL-2008 mode on 
    #    #---------------------------------------------------------------------------
    #    set_property file_type {VHDL 2008} [ get_files  ${rootDir}/sim/*.vhd* ]
    #    #OBSOLETE-20180705 set_property file_type {VHDL 2008} [ get_files  ${hdlDir}/*.vhd* ]

    #    set_property source_mgmt_mode All [ current_project ]
    #    update_compile_order -fileset sim_1
    #}

    my_puts "################################################################################"
    my_puts "##  DONE WITH PROJECT CREATION "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}



if { ${synth} } {

    my_puts "################################################################################"
    my_puts "##" 
    if { $forceWithoutBB } { 
    my_puts "##  RUN SYNTHESIS: ${xprName}  " 
  } else { 
    my_puts "##  RUN SYNTHESIS: ${xprName}  WITHOUT ROLE"
  }
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    if { ! ${create} } {
        open_project ${xprDir}/${xprName}.xpr
        # Reset the previous run 'synth_1' before launching a new one
    }
    
    reset_run synth_1

    launch_runs synth_1 -jobs 8
    wait_on_run synth_1 

    #secureSynth
    #guidedSynth
    
    #ensure synth finished successfull
    if {[get_property STATUS [get_runs synth_1]] != {synth_design Complete!}} {
      my_err_puts " SYNTHESIS FAILED"
      exit ${KO}
    }

    if { ! $forceWithoutBB } { 
      open_run synth_1 -name synth_1
      # otherwise write checkpoint will fail...
      write_checkpoint -force ${dcpDir}/0_${topName}_static_without_role.dcp 
      close_design
    } else { 
      my_dbg_trace "forceWithoutBB is active, so no dcp is saved at this stage." ${dbgLvl_1}
      
      #link must not be enabeld in this mode 
      set link 0
    }

    my_puts "################################################################################"
    if { $forceWithoutBB } { 
    my_puts "##  DONE WITH SYNTHESIS RUN "
    } else {
    my_puts "##  DONE WITH SYNTHESIS RUN (ROLE as BLACK BOX); .dcp SAVED "
    }
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n" 

}


if { ${link} } { 
  

  #if { ! ${create} } {
     catch {open_project ${xprDir}/${xprName}.xpr} 
  #}
  #set roleDcpFile ${rootDir}/../../ROLE/${usedRole}/${usedRoleType}_OOC.dcp
  set roleDcpFile ${usedRoleDir}/Role_${cFpSRAtype}_OOC.dcp
  add_files ${roleDcpFile}
  update_compile_order -fileset sources_1
  my_dbg_trace "Added dcp of ROLE ${roleDcpFile}." ${dbgLvl_1}

  set_property SCOPED_TO_CELLS {ROLE} [get_files ${roleDcpFile} ]

  open_run synth_1 -name synth_1
  # Link the two dcps together
  #link_design -mode default -reconfig_partitions {ROLE}  -top ${topName} -part ${xilPartName} 
  ### NOTE: link_design is done by open_design in project mode!!
  # to prevent the "out-of-date" message; we just added an alreday synthesized dcp -> not necessary
  set_property needs_refresh false [get_runs synth_1]
  


  if { $pr } { 
    set constrObj [ get_filesets constrs_1 ]
    if { $insert_ila } { 
      # we have to add debug constrains (of the Shell) befor we add PR partitions
      add_files -fileset ${constrObj} ${rootDir}/TOP/xdc/debug.xdc 
      my_info_puts "DEBUG XDC ADDED."
      set_property needs_refresh false [get_runs synth_1]
    }

    set prConstrFile "${xdcDir}/topFMKU60_pr.xdc"
    add_files -fileset ${constrObj} ${prConstrFile} 
    #if { [ add_files -fileset ${constrObj} ${prConstrFile} ] eq "" } {
    #    my_err_puts "Could not add file ${prConstrFile} to the fileset \'${constrObj}\' !!!"
    #    my_err_puts "  The script will be aborted here..."
    #    my_puts ""
    #    exit ${KO}
    #}
    my_puts "################################################################################"
    my_puts "## ADDED Partial Reconfiguration Constraint File: ${prConstrFile}; PBLOCK CREATED;"
    my_puts "################################################################################"

    write_checkpoint -force ${dcpDir}/1_${topName}_linked_pr.dcp
  } else {
    write_checkpoint -force ${dcpDir}/1_${topName}_linked.dcp
  }

  my_puts "################################################################################"
  my_puts "## DONE WITH DESIGN LINKING; .dcp SAVED" 
  my_puts "################################################################################"
  my_puts "At: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n" 


}



if { ${impl1} || ( $forceWithoutBB && $impl1 ) } {

    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN IMPLEMENTATION: ${xprName}  with ${usedRole}"
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

    #if { ! ${create} } {
        catch {open_project ${xprDir}/${xprName}.xpr}
    #}
    
    if { $insert_ila && $forceWithoutBB } { 
     # only for non-pr flow
     set constrObj [ get_filesets constrs_1 ]
     #add_files -fileset ${constrObj} ${xdcDir}/debug.xdc 
     add_files -fileset ${constrObj} ${rootDir}/TOP/xdc/debug.xdc 
     my_info_puts "DEBUG XDC ADDED."
    }
  
    set_property needs_refresh false [get_runs synth_1]
    
    # Select a Strategy
    #  Strategies are a defined set of Vivado implementation feature options that control
    #  the implementation results. Vivado Design Suite includes a set of pre-defined 
    #  strategies. You can list the Implementation Strategies using the list_property_value
    #  command (e.g. join [list_property_value strategy [get_runs impl_1] ]).
    #-------------------------------------------------------------------------------
    set implObj [ get_runs impl_1 ]
    
    if { ! $impl_opt } {
      set_property strategy Flow_RuntimeOptimized ${implObj}
      my_puts "Flow_RuntimeOptimized is set"
    } else {
      set_property strategy Performance_Explore ${implObj}
      my_puts "Performance_Explore is set"
    }

    if { $use_incr } {
      my_puts "Try to use incremental Checkpoint in ${dcpDir}"
      if { $forceWithoutBB } { 
        catch { set_property incremental_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole}_monolithic_reference.dcp ${implObj} }
      } else { 
        if { $pr } {
          #catch { set_property incremental_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole}_complete_pr.dcp ${implObj} }
          #catch { set_property incremental_checkpoint ${dcpDir}/3_${topName}_STATIC.dcp ${implObj} }
          my_info_puts "Incremental compile seems not to work for PARTIAL RECONFIGURATION FLOW, so this option will be ignored."
        } else {
          catch { set_property incremental_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole}_complete.dcp ${implObj} }
        }
      }
    }


    reset_run impl_1
    launch_runs impl_1 -jobs 8
    wait_on_run impl_1

    #secureImpl
    #guidedImpl
   

    if { ! $forceWithoutBB } {

    #  if { $pr } { 
    #    # it seems, that incremental compile does not work with pr after this step, so deactivate it in order to be safe
    #    set_property incremental_checkpoint {} ${implObj}
    #  }

      open_run impl_1
      if { $pr } { 
      write_checkpoint -force ${dcpDir}/2_${topName}_impl_${usedRole}_complete_pr.dcp
      } else { 
        write_checkpoint -force ${dcpDir}/2_${topName}_impl_${usedRole}_complete.dcp
      }
      #write_checkpoint -force -cell ROLE ${dcpDir}/2P_${topName}_impl_${usedRole}.dcp
      
      my_puts "################################################################################"
      my_puts "##  DONE WITH 1. IMPLEMENATATION RUN; .dcp SAVED "
    } else { 
      my_puts "################################################################################"
      my_puts "##  DONE WITH IMPLEMENATATION RUN "
      my_dbg_trace "forceWithoutBB is active, so no dcp is saved at this stage." ${dbgLvl_1}
    }

    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n" 

    if { $pr } { 
      # now, black box 
      update_design -cell ROLE -black_box

      lock_design -level routing
      
      write_checkpoint -force ${dcpDir}/3_${topName}_STATIC.dcp
      
      my_puts "################################################################################"
      my_puts "## STATIC DESIGN LOCKED; .dcp SAVED" 
      my_puts "################################################################################"
      my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
      
      close_project

    }

}

if { $save_incr } { 
  my_puts "################################################################################"
  my_puts "trying to save current Design as reference Checkpoint"

  if { $forceWithoutBB } { 
    # nothing must be called before
    open_project ${xprDir}/${xprName}.xpr

    open_run impl_1 

    write_checkpoint -force ${dcpDir}/2_${topName}_impl_${usedRole}_monolithic_reference.dcp
  } else {

    my_err_puts "This line should never be reached! Saving the incremental checkpoint for other than the Non-BB-flow is done automatically!"
    exit ${KO}

  }

  my_puts "################################################################################"
  my_puts "## CURRENT PROJECT IMPLEMENTATION RUN SAVED for use in INCREMENTAL BUILD" 
  my_puts "################################################################################"
  my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}


if { $impl2 } { 
  
  catch {close_project}
  create_project -in_memory -part ${xilPartName}
  add_files ${dcpDir}/3_${topName}_STATIC.dcp
  my_dbg_trace "Started in-memory project; added locked static part." ${dbgLvl_1}
  
  #set roleDcpFile_conf2 ${rootDir}/../../ROLE/${usedRole2}/${usedRoleType}_OOC.dcp
  set roleDcpFile_conf2  ${usedRole2Dir}/Role_${cFpSRAtype}_OOC.dcp
  add_files ${roleDcpFile_conf2}
  set_property SCOPED_TO_CELLS {ROLE} [get_files ${roleDcpFile_conf2} ]
  my_dbg_trace "Added dcp of ROLE ${roleDcpFile_conf2}." ${dbgLvl_1}
  
  link_design -mode default -reconfig_partitions {ROLE} -part ${xilPartName} -top ${topName} 
  
  my_dbg_trace "Linked 2. Configuration together" ${dbgLvl_1}
  
    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN 2. IMPLEMENTATION: ${xprName}  with ${usedRole2}"
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

  opt_design 

  if { $use_incr } { 
    #my_puts "Trying to use incremental checkpoint"
    #catch { read_checkpoint -incremental ${dcpDir}/2_${topName}_impl_${usedRole2}_complete_pr.dcp }
    #catch { set_property incremental_checkpoint ${dcpDir}/3_${topName}_STATIC.dcp ${implObj} } 
    #my_info_puts "Incremental compile seems not to work for the 2. Implementation run, so this option will be ignored."
    #my_info_puts "And it is not necessary, because STATIC.dcp will be reused anyway."
    my_info_puts "Incremental compile seems not to work for PARTIAL RECONFIGURATION FLOW, so this option will be ignored."
  }

  place_design
  route_design
  
  
  write_checkpoint -force ${dcpDir}/2_${topName}_impl_${usedRole2}_complete_pr.dcp
  #write_checkpoint -force -cell ROLE ${dcpDir}/2P_${topName}_impl_${usedRole2}.dcp
  
  my_puts "################################################################################"
  my_puts "##  DONE WITH 2. IMPLEMENATATION RUN; .dcp SAVED "
  my_puts "################################################################################"
  my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
  
  close_project
} 


if { $pr_grey_impl } { 

  catch {close_project}
  open_checkpoint ${dcpDir}/3_${topName}_STATIC.dcp
  
  update_design -cell ROLE -buffer_ports
  
  source ${tclTopDir}/fix_things.tcl 
  
  my_puts "################################################################################"
  my_puts "##"
  my_puts "## RUN GREYBOX IMPLEMENTATION "
  my_puts "##"
  my_puts "################################################################################"
  my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
  
  place_design
  route_design 
  
  write_checkpoint -force ${dcpDir}/3_${topName}_impl_grey_box.dcp
    
  my_puts "################################################################################"
  my_puts "##  DONE WITH GREYBOX IMPLEMENATATION RUN; .dcp SAVED "
  my_puts "################################################################################"
  my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
  
  close_project

}


if { $pr_verify } { 
  catch {close_project}
  my_dbg_trace "Starting pr_verify" ${dbgLvl_1}
  
  set toVerifyList [ glob -nocomplain ${dcpDir}/2_* ]
  set ll [llength $toVerifyList]
  if { $ll < 2 } { 
    my_puts "################################################################################"
    my_err_puts "Only one .dcp to verify --> not possible --> SKIP pr_verify."
  } else {
    #pr_verify ${toVerifyList}
    pr_verify -initial [lindex $toVerifyList 0] -additional [lrange $toVerifyList 1 $ll] -file ${dcpDir}/pr_verify.rpt
    # yes, $ll is here 'out of bounce' but tcl dosen't care
  
    my_puts "################################################################################"
    my_puts "##  DONE WITH pr_verify "
  }
  my_puts "################################################################################"
  my_puts "At: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
  
  catch {close_project}

}


if { $bitGen1 || $bitGen2 || $pr_grey_bitgen } {

  if { ! $forceWithoutBB } {  
    catch {close_project}
  }

    my_puts "################################################################################"
    my_puts "##"
    my_puts "##  RUN BITTSETREAM GENERATION: ${xprName}  "
    my_puts "##  SETTING: PR: $pr PR FLOW 1/NORMAL FLOW: $bitGen1 PR FLOW 2: $bitGen2 PR_GREY: $pr_grey_bitgen BB: $forceWithoutBB "
    my_puts "##"
    my_puts "################################################################################"
    my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
    
    set curImpl ${usedRole}

    if { ${forceWithoutBB} } {
      #---------------------------
      # We are in monolythic flow
      #---------------------------
      catch {open_project ${xprDir}/${xprName}.xpr} 
      set implObj [ get_runs impl_1 ]
      #set_property "steps.write_bitstream.args.readback_file" "0" ${implObj}
      #set_property "steps.write_bitstream.args.verbose"       "0" ${implObj}

      #catch {open_run impl_1}
      
      open_run impl_1
      # Request to embed a timestamp into the bitstream
      set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [current_design]

      write_bitstream -force ${dcpDir}/4_${topName}_impl_${curImpl}_monolithic.bit
      #launch_runs impl_1 -to_step write_bitstream -jobs 8
      #wait_on_run impl_1
    
      # DEBUG probes
      if { $insert_ila } { 
        write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}_monolithic.ltx
      }

    } else {
      #---------------------------
      # We are in PR flow
      #---------------------------
      # Partial bitstream can be instrumented with CRC values every frame, so any failures are detected *before* the bad frame can be loaded in configuration memory, then corrective/fallback measures can be taken.
      # for every open Checkpoint: 
      #set_property bitstream.general.perFrameCRC yes [current_design]
      # --> moved to fix_things.tcl

      if { $pr } {
        if { $bitGen1 } { 
          open_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole}_complete_pr.dcp 
          
          source ${tclTopDir}/fix_things.tcl 
          #source ./fix_things.tcl 
          if { $only_pr_bitgen } {
            write_bitstream -bin_file -cell ROLE -force ${dcpDir}/4_${topName}_impl_${curImpl}_pblock_ROLE_partial 
            # no file extenstions .bit/.bin here!
          } else {
            write_bitstream -bin_file -force ${dcpDir}/4_${topName}_impl_${curImpl}.bit
          }
          #close_project
          # DEBUG probes
          if { $insert_ila } { 
            write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}.ltx
          }
        } 
        # else: do nothing: only impl2 or grey_box will be generated (to save time)
        
      } else {
        open_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole}_complete.dcp 
        source ${tclTopDir}/fix_things.tcl 
        #source ./fix_things.tcl 
        write_bitstream -force ${dcpDir}/4_${topName}_impl_${curImpl}.bit
        #close_project
        # DEBUG probes
        if { $insert_ila } { 
          write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}.ltx
        }
      }

      if { $bitGen2 } { 
        catch {close_project}
        open_checkpoint ${dcpDir}/2_${topName}_impl_${usedRole2}_complete_pr.dcp 
        set curImpl ${usedRole2}
        
        #source ./fix_things.tcl 
        source ${tclTopDir}/fix_things.tcl 
        if { $only_pr_bitgen } {
          write_bitstream -bin_file -cell ROLE -force ${dcpDir}/4_${topName}_impl_${curImpl}_pblock_ROLE_partial 
          # no file extenstions .bit/.bin here!
        } else {
          write_bitstream -bin_file -force ${dcpDir}/4_${topName}_impl_${curImpl}.bit
        }
        #close_project
        # DEBUG probes
        if { $insert_ila } { 
          write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}.ltx
        }
      } 
      if { $pr_grey_bitgen } { 
        catch {close_project}
        open_checkpoint ${dcpDir}/3_${topName}_impl_grey_box.dcp 
        set curImpl "grey_box"
        
        source ${tclTopDir}/fix_things.tcl 
        # source ./fix_things.tcl 
        write_bitstream -force ${dcpDir}/4_${topName}_impl_${curImpl}.bit
        #close_project
      } 

    }

    # DEBUG probes
    # if { $insert_ila } { 
    #   if { ${forceWithoutBB} } {
    #     write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}_monolithic.ltx
    #   } else {
    #     write_debug_probes -force ${dcpDir}/5_${topName}_impl_${curImpl}.ltx
    #   }
    # }


    my_puts "################################################################################"
    my_puts "##  DONE WITH BITSTREAM GENERATION RUN "
    my_puts "################################################################################"
    my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}


# Close project
#-------------------------------------------------------------------------------
catch {close_project}
exit ${VERIFY_RETVAL}



