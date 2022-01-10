# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
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
#  *******************************************************************************/

# ******************************************************************************
# * 
# * Description : A Tcl script to create the IP cores of the current SHELL.
# * 
# * Synopsis : vivado -mode batch -source <this_file> -notrace -log <filename.log> 
# *
# ******************************************************************************

package require cmdline

## Set the Global Settings used by the SHELL Project
##-------------------------------------------------------------------------------
#if { ! [ info exists ipXprDir ] && ! [ info exists ipXprName ] } {
#    # Expect to be in the TCL directory and source the TCL settings file 
#    source xpr_settings.tcl
#}
set ipXprDir     ${ipDir}/managed_ip_project
set ipXprName    "managed_ip_project"
set ipXprFile    [file join ${ipXprDir} ${ipXprName}.xpr ]

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set nrGenIPs         0
set nrErrors         0

set dbgLvl_1         1
set dbgLvl_2         2
set dbgLvl_3         3

#-------------------------------------------------------------------------------
# my_clean_ip_dir ${ipModName}
#  A procedure that deletes the previous IP directory (if exists).
#  :param ipModName the name of the IP module to clean.
#  :return 0 if OK
#-------------------------------------------------------------------------------
proc my_clean_ip_dir { ipModName } {

     ## STEP-1: Open the project if it is not yet open.
    if { [ catch { current_project } rc ] } {
        my_warn "Project \'${rc}\' is not opened. Openning now!"
        open_project ${::ipXprFile}
    }

    ## STEP-2: Check if a FileSet is already in use in the project.
    set filesets [ get_filesets ]
    foreach fileset $filesets {
        if [ string match "${::ipDir}/${ipModName}/${ipModName}.xci" ${fileset} ] {
            my_dbg_trace "An IP with name \'${ipModName}\' already exists and will be removed from the project \'${::ipXprName} !" ${::dbgLvl_2}
            remove_files -fileset ${ipModName} ${fileset}
            break
        }        
    }

    ## STEP-3: Check for possible dangling files (may happen after IP flow got broken)
    set files [ get_files ]
    foreach file $files {
        if [ string match "${::ipDir}/${ipModName}/${ipModName}.xci" ${file} ] {
            my_dbg_trace "A dangling IP file \'${ipModName}\' already exists and will be removed from the project \'${::ipXprName} !" ${::dbgLvl_2}
            remove_files ${ipModName} ${file}
            break
        }        
    }

    ## STEP-4: Delete related previous files and directories from the disk
    file delete -force ${::ipDir}/ip_user_files/ip/${ipModName}
    file delete -force ${::ipDir}/ip_user_files/sim_scripts/${ipModName}

    file delete -force  ${::ipDir}/${ipModName}

    return ${::OK}
}


#-------------------------------------------------------------------------------
# my_create_ip ${ipModName ipDir ipVendor ipLibrary ipName ipVersion ipCfgList}
#  A procedure that automates the creation and the customization of an IP.
#  :param ipModName the name of the IP module to create (as a string).
#  :param ipDir     the directory path for remote IP to be created and
#                     managed outside of the project (as a string).
#  :param ipVendor  the IP vendor name (as a string).
#  :param ipLibrary the IP library name (as a string).
#  :param ipName    the IP name (as a string").
#  :param ipVersion the IP version (as a string).
#  :param ipCfgList a list of name/value pairs of properties to set.
#  :return 0 if OK
#-------------------------------------------------------------------------------
proc my_customize_ip {ipModName ipDir ipVendor ipLibrary ipName ipVersion ipCfgList} {

    if { ${::gTargetIpCore} ne ${ipModName} && ${::gTargetIpCore} ne "all" } {
        # Skip the creation and customization this IP module
        my_dbg_trace "Skipping Module ${ipModName}" ${::dbgLvl_2}
        return ${::OK} 
    } else {
        set ::nrGenIPs [ expr { ${::nrGenIPs} + 1 } ]
    }
    
    set title "##  Creating IP Module: ${ipModName} "
    while { [ string length $title ] < 80 } {
        append title "#"
    }
    my_puts "${title}"
    my_puts " (in remote directory: ${ipDir})"
    my_puts "    ipName    = ${ipName}"
    my_puts "    ipVendor  = ${ipVendor}"
    my_puts "    ipLibrary = ${ipLibrary}"
    my_puts "    ipVersion = ${ipVersion}"
    my_puts "    ipCfgList = ${ipCfgList}"

    set rc [ my_clean_ip_dir "${ipModName}" ]

    my_dbg_trace "Done with \'my_clean_ip\' (RC=${rc})." ${::dbgLvl_2}

    # Note-1: A typical 'create_ip" command looks like the following:
    #   "create_ip -name axis_register_slice -vendor xilinx.com -library ip \
    #              -version 1.1 -module_name ${ipModName} -dir ${ipDir}"
    #
    # Note-2: Intercepting errors from 'create_ip' with 'catch' does not work for me.
    #   Therefore, I decided to use the '-quiet' option which execute the command quietly,
    #   returning no messages from the command and always returning 'TCL_OK' regardless of
    #   any errors encountered during execution.
    if { [ catch { create_ip -name ${ipName} -vendor ${ipVendor} -library ${ipLibrary} \
                       -version ${ipVersion} -module_name ${ipModName} -dir ${ipDir} } errMsg ] } {
        puts "ERROR_MSG = " ${errMsg}
        my_err_puts "## The TCL command \'create_ip\' failed (Error message = ${errMsg}"
        return  ${::KO}
    }

    my_dbg_trace "Done with \'create_ip\' (errMsg=${errMsg})." ${::dbgLvl_2}

    if { ! [ string match ${ipModName}  [ get_ips ${ipModName} ] ] } {
        # The returned list does not match expected value. IP was not created.
        my_err_puts "## Failed to create IP \'${ipModName}\'."
        my_puts ""
        return  ${::KO}
    }

    # Note: A typical 'set_property' command looks like the following:
    #   "set_property -dict [ list CONFIG.TDATA_NUM_BYTES {8} \
    #                              CONFIG.HAS_TKEEP {1}       \
    #                              CONFIG.HAS_TLAST {1} ] [ get_ips ${ipModName} ] 
    if { [llength ${ipCfgList} ] != 0 } {
        set_property -dict ${ipCfgList} [ get_ips ${ipModName} ]
        my_dbg_trace "Done with \'set_property\'." ${::dbgLvl_2}
    } else {
        my_dbg_trace "There is no \'set_property\' to be done." ${::dbgLvl_2}
    }

    generate_target {instantiation_template} \
        [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ] -force
    my_dbg_trace "Done with \'generate_target\'." ${::dbgLvl_2}

# TOTO upgrade_ip -srcset ContentAddressableMemory -vlnv IBM:hls:toecam_top:1.0 [get_ips  ContentAddressableMemory] -log ip_upgrade.lo

    update_compile_order -fileset sources_1

    generate_target all [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ]
    my_dbg_trace "Done with \'generate_target all\'." ${::dbgLvl_2}

    catch { config_ip_cache -export [ get_ips -all ${ipModName} ] }

    export_ip_user_files -of_objects \
        [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ] -no_script -sync -force -quiet
    my_dbg_trace "Done with \'export_ip_user_files\'." ${::dbgLvl_2}

    create_ip_run [ get_files -of_objects \
                        [ get_fileset sources_1 ] ${ipDir}/${ipModName}/${ipModName}.xci ]
    my_dbg_trace "Done with \'create_ip_run\'." ${::dbgLvl_2}

    puts "## Done with IP \'${ipModName}\' creation and customization. \n"
    return ${::OK}

}



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

# By default, create all the IP cores 
set gTargetIpCore "all"

# Retreive the Vivado version 
#-------------------------------------------------------------------------------
set VIVADO_VERSION [file tail $::env(XILINX_VIVADO)]
set HLS_VERSION    [expr entier(${VIVADO_VERSION})]

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
        { ipModuleName.arg "all"  "The module name of the IP to create." }
        { ipList                  "Display the list of IP modules names." }
    }
    set usage "\nUSAGE: vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        if { ${key} eq "ipModuleName" && ${value} ne "all" } {
            set gTargetIpCore ${value}
            my_dbg_trace "Setting gTargetIpCore to \'${gTargetIpCore}\' " ${dbgLvl_1}
            break;
        } 
        if { ${key} eq "ipList" && ${value} eq 1} {
            my_puts "IP MODULE NAMES THAT CAN GENERATED BY THIS SCRIPT:"
            set thisScript [ open ${argv0} ]
            set lines [ split [ read $thisScript ] \n ]
            foreach line ${lines} {
                if { [ string match "set*ipModName*" ${line} ] } {
                    if {  [ llength [ split ${line} ] ] == 3  } {
                        if { [ lindex [ split ${line}  ] 0 ] eq "set" && \
                                 [ lindex [ split ${line}  ] 1 ] eq "ipModName" } {
                            set modName [ lindex [ split ${line}  ] 2 ]
                            puts "\t${modName}"
                        }
                    }
                } 
            }
            close ${thisScript}
            return ${::OK}
        } 
    }
}



my_puts "################################################################################"
my_puts "##"
my_puts "##  CREATING IP CORES FOR THE SHELL (ENCRYPTED ONLY)"
my_puts "##"
my_puts "################################################################################"
my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"


# Always start from the root directory
#-------------------------------------------------------------------------------
catch { cd ${rootDir} }

# Create a Managed IP directory and project
#------------------------------------------------------------------------------
if { [ file exists ${ipDir} ] != 1 } {
    my_puts "Creating the managed IP directory: \'${ipDir}\' "
    file mkdir ${ipDir}
} else {
    my_puts "The managed IP directory already exists. "
    if { ${gTargetIpCore} eq "all" } { 
        if { [ file exists ${ipDir}/ip_user_files ] } {
            my_dbg_trace "Removing directory: \'${ipDir}/ip_user_files\' " ${dbgLvl_1}
            file delete -force ${ipDir}/ip_user_files
            my_dbg_trace "Removing directory: \'${ipXprDir}\' " ${dbgLvl_1}            
            file delete -force ${ipXprDir}
            file mkdir ${ipDir}/ip_user_files 
            my_dbg_trace "Done with the cleaning of: \'${ipDir}/ip_user_files\' " ${dbgLvl_1}
        }   
    } else {
        if { [ file exists ${ipDir}/ip_user_files/${gTargetIpCore} ] } {
            file delete -force ${ipDir}/ip_user_files/${gTargetIpCore}
        }
    }
}

if { [ file exists ${ipXprDir} ] != 1 } {
    my_puts "Creating the managed IP project directory: \'${ipXprDir}\' "
    file mkdir ${ipXprDir}
} 

if { [ file exists ${ipXprDir}/${ipXprName}.xpr ] != 1 } {
    my_puts "Creating the managed IP project: \'${ipXprName}.xpr\' "
    #my_warn ": ${ipXprName} ${ipXprDir} ${xilPartName}"
    create_project ${ipXprName} ${ipXprDir} -part ${xilPartName} -ip -force
} else {
    my_puts "The managed IP project \'${ipXprName}.xpr\' already exists."
    my_puts "Opening managed IP project: ${ipXprFile}"
    open_project ${ipXprFile}
}

# Open managed IP project
#------------------------------------------------------------------------------
# TODO 
#if { [ catch { current_project } rc ] } {
#    my_puts "Opening managed IP project: ${ipXprFile}"
#    open_project ${ipXprFile}
#} else {
#    my_warn "Project \'${rc}\' is already opened!"
#}

# Set the simulator language (Mixed, VHDL, Verilog)
#------------------------------------------------------------------------------
set_property simulator_language Mixed [current_project]
# Set target simulator (not sure if really needed)
set_property target_simulator XSim [current_project]

# Target language for instantiation template and wrapper (Verilog, VHDL)
#------------------------------------------------------------------------------
set_property target_language Verilog [current_project]


###############################################################################
##
##  PHASE-3: Creating Vivado-based ENCRYPTED IPs
##
###############################################################################
my_puts ""

source ${rootDir}/cFDK/SRA/LIB/SHELL/LIB/tcl/enc_ips_list.tcl

