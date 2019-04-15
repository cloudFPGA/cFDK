# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Feb 7 2018
# * Authors : Francois Abel
# * 
# * Description : A Tcl script that synthesizes all the IP cores of the managed
# *   IP project.
# * 
# * Synopsis : vivado -mode batch -source <this_file> -notrace
# *                             [ -log    <log_file_name>        ]
# *                             [  -tclargs <myscript_arguments> ]
# *                             [ -help                          ]
# * 
# * Return: 0 if OK, otherwise the number of errors which corresponds to the 
# *   number of IPs that failed to be synthesized. 
# * 
# * Reference documents:
# *  - UG896 / Ch.3 / Using Manage IP Projects.
# *  - UG896 / Ch.6 / Tcl Commands for Common IP Operations. 
# *-----------------------------------------------------------------------------
# * Modification History:
# *  Fab: Feb-16-2018 Created.
# ******************************************************************************

package require cmdline

# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
if { ! [ info exists ipXprDir ] && ! [ info exists ipXprName ] } {
    # Expect to be in the TCL directory and source the TCL settings file 
    source xpr_settings.tcl
}

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
set nrVivadoIPs      0
set nrHlsBasedIPs    0
set nrErrors         0

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

# By default, create all the IP cores 
set gTargetIpCore "all"

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
        { ipModuleName.arg "all"  "The name of the IP module to synthesize." }
        { ipList                  "Display the names of the IP modules." }
    }
    set usage "\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        if { ${key} eq "ipModuleName" && ${value} ne "all" } {
            set gTargetIpCore ${value}
            my_dbg_trace "Setting gTargetIpCore to \'${gTargetIpCore}\' " ${dbgLvl_1}
            break;
        } 
        if { ${key} eq "ipList" } {
            my_puts "IP MODULE THAT CAN BE SYNTHESIZED BY THIS SCRIPT:"
            set thisScript [ open "create_ip_cores.tcl" ]
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
my_puts "##  SYNTHESIZING IP CORES OF THE MANAGED IP PROJECT "
my_puts "##"
my_puts "################################################################################"
my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"


# Always start from the root directory
#-------------------------------------------------------------------------------
catch { cd ${rootDir} }

# Create a Managed IP directory and project
#------------------------------------------------------------------------------
if { [ file exists ${ipDir} ] != 1 } {
    my_err_puts "Could not find the managed IP directory: \'${ipDir}\'"
    exit 999
}
if { [ file exists ${ipXprDir} ] != 1 } {
    my_err_puts "Could not find the managed IP project directory: \'${ipXprDir}\' "
    exit 999
}
#if { [ file exists ${ipXprDir}/ip_user_files/ip ] != 1 } {
#    my_err_puts "Could not find the IP user files directory: \'${ipXprDir}/ip_user_files/ip\' "
#    exit 999
#} 

# Open managed IP project
#------------------------------------------------------------------------------
if { [ catch { current_project } rc ] } {
    my_puts "Opening managed IP project: ${ipXprFile}"
    open_project ${ipXprFile}
} else {
    my_warn "Project \'${rc}\' is already opened!"
}

# Set the simulator language (Mixed, VHDL, Verilog)
#------------------------------------------------------------------------------
#set_property simulator_language Mixed [current_project]
# Set target simulator (not sure if really needed)
#set_property target_simulator XSim [current_project]

# Target language for instantiation template and wrapper (Verilog, VHDL)
#------------------------------------------------------------------------------
#set_property target_language Verilog [current_project]


my_puts "################################################################################"
my_puts "##"
my_puts "##  PHASE-1: Synthesizing Vivado-based IPs from the Managed IP Repository "
my_puts "##"
my_puts "################################################################################"
my_puts ""

#KEEP-FOR-A-WHILE set ipList [ glob -nocomplain ${ipDir}/ip_user_files/ip/* ]
set ipList [ get_ips -filter {IPDEF=~*xilinx.com:ip*} ]
my_dbg_trace "ipList = ${ipList} " ${dbgLvl_2}
if { $ipList ne "" } {
    foreach ipModuleName $ipList {

        if { ${::gTargetIpCore} ne ${ipModuleName} && ${::gTargetIpCore} ne "all" } {
            # Skip the synthesize of this IP module.
            continue 
        }
        
        set title "##  Launch synthesis run for the VIVADO-IP : ${ipModuleName} "
        while { [ string length $title ] < 80 } { append title "#" }
        my_puts "${title}"
        my_puts "##    Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "

        # if { [ file exists ${ipXprDir}/${ipModuleName}.runs/${ipModuleName}_synth_1/${ipModuleName}.dcp ] } {
        #     file delete -force ${ipXprDir}/${ipModuleName}.runs/${ipModuleName}_synth_1/${ipModuleName}.dcp
        #     my_dbg_trace "Removing previous DCP file: \'${ipXprDir}/${ipModuleName}/${ipModuleName}.dcp\'." ${dbgLvl_1}
        # }

        set synthRun "${ipModuleName}_synth_1"
        set rc [ reset_run ${synthRun} ]
        set rc [ launch_runs -jobs 3 ${synthRun} ]
        wait_on_run ${synthRun}

        # export_ip_user_files -of_objects [get_files /dataL/fab/ZMS/cF_SHELL/SHELL/FMKU60/Shell/ip/AxisRegisterSlice_64/AxisRegisterSlice_64.xci] -no_script -sync -force -quiet
        # create_ip_run [get_files -of_objects [get_fileset sources_1] /dataL/fab/ZMS/cF_SHELL/SHELL/FMKU60/Shell/ip/AxisRegisterSlice_64/AxisRegisterSlice_64.xci]
        # launch_runs -jobs 6 AxisRegisterSlice_64_synth_1
        # [Wed Feb 21 19:09:01 2018] Launched AxisRegisterSlice_64_synth_1...
        # Run output will be captured here: /dataL/fab/ZMS/cF_SHELL/SHELL/FMKU60/Shell/ip/managed_ip_project/managed_ip_project.runs/AxisRegisterSlice_64_synth_1/runme.log

        export_simulation -of_objects [get_files ${ipDir}/${ipModuleName}/${ipModuleName}.xci ] \
            -directory           ${ipDir}/ip_user_files/sim_scripts \
            -ip_user_files_dir   ${ipDir}/ip_user_files \
            -ipstatic_source_dir ${ipDir}/ip_user_files/ipstatic \
            -lib_map_path [ list {modelsim=${ipXprDir}/${ipXprName}.cache/compile_simlib/modelsim} \
                                {questa=${ipXprDir}/${ipXprName}.cache/compile_simlib/questa} \
                                {ies=${ipXprDir}/${ipXprName}.cache/compile_simlib/ies} \
                                {vcs=${ipXprDir}/${ipXprName}.cache/compile_simlib/vcs} \
                                {riviera=${ipXprDir}/${ipXprName}.cache/compile_simlib/riviera} ] -use_ip_compiled_libs -force -quiet

        if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }
        set nrVivadoIPs [ expr { ${nrVivadoIPs} + 1 } ]
        
        my_puts "##    End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
        my_puts "##  Done with the synthesis of: ${ipModuleName}"
    }
}
my_puts ""


my_puts "################################################################################"
my_puts "##"
my_puts "##  PHASE-2: Synthesizing HLS-baed cores from the Managed IP Repository "
my_puts "##"
my_puts "################################################################################"
my_puts ""

set hlsList [ get_ips -filter {IPDEF=~*IBM:hls:*} ]
my_dbg_trace "hlsList = ${hlsList} " ${dbgLvl_3}
if { $hlsList ne "" } {
    foreach ipModuleName $hlsList {

        if { ${::gTargetIpCore} ne ${ipModuleName} && ${::gTargetIpCore} ne "all" } {
            # Skip the synthesize of this IP module.
            continue 
        }

        set title "##  Launch synthesis run for the HLS-IP : ${ipModuleName} "
        while { [ string length $title ] < 80 } { append title "#" }
        my_puts "${title}"
        my_puts "##    Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "

        if { [ file exists ${ipXprDir}/${ipModuleName}/${ipModuleName}.dcp ] } {
            file delete -force ${ipXprDir}/${ipModuleName}/${ipModuleName}.dcp
            my_dbg_trace "Done with DCP file deletion. " ${dbgLvl_1}
        }
        
        set synthRun "${ipModuleName}_synth_1"
        set rc [ launch_runs -jobs 3 ${synthRun} ]
        wait_on_run ${synthRun}

        export_simulation -of_objects [get_files ${ipDir}/${ipModuleName}/${ipModuleName}.xci ] \
            -directory           ${ipDir}/ip_user_files/sim_scripts \
            -ip_user_files_dir   ${ipDir}/ip_user_files \
            -ipstatic_source_dir ${ipDir}/ip_user_files/ipstatic \
            -lib_map_path [ list {modelsim=${ipXprDir}/${ipXprName}.cache/compile_simlib/modelsim} \
                                {questa=${ipXprDir}/${ipXprName}.cache/compile_simlib/questa} \
                                {ies=${ipXprDir}/${ipXprName}.cache/compile_simlib/ies} \
                                {vcs=${ipXprDir}/${ipXprName}.cache/compile_simlib/vcs} \
                                {riviera=${ipXprDir}/${ipXprName}.cache/compile_simlib/riviera} ] -use_ip_compiled_libs -force -quiet

        if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }
        set nrHlsBasedIPs [ expr { ${nrHlsBasedIPs} + 1 } ]
        
        my_puts "##    End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
        my_puts "##  Done with the synthesis of: ${ipModuleName}"
    }
}


puts    ""
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
my_puts "################################################################################"
my_puts "##  DONE WITH THE SYNTHESIS OF [expr {${nrVivadoIPs} + ${nrHlsBasedIPs}} ] IP CORES FROM THE MANAGED IP REPOSITORY "
my_puts "##    Created a total of ${nrVivadoIPs} Vivado-based IPs  "
my_puts "##    Created a total of ${nrHlsBasedIPs} HLS-based IPs   "
my_puts "##                                                        "
my_puts "##             (Return Code = ${nrErrors}) "
my_puts "################################################################################\n"


# Close project
#-------------------------------------------------
close_project

return ${nrErrors}


#////////////////////////////////////////////////////////////////////////////////////

#////////////////////////////////////////////////////////////////////////////////////

#////////////////////////////////////////////////////////////////////////////////////



#==============================================================================
#  REPOSITORY OF TCL COMMANDS
#==============================================================================


#-------------------------------------------------
# Add HLS Generated IP(s) to the User Repository
#-------------------------------------------------
#
# Set the IP_REPO_PATHS property of the current 
# source fileset to add an IP repository
#--------------------------------------------------
#set_property ip_repo_paths ${ipDir} [current_project]
#set ipList [glob -nocomplain *.zip] >> ${log_file}
#if { $ipList ne "" } {
#  foreach ip $ipList {
#      update_ip_catalog -add_ip ${ipDir}/${ip} -repo_path ${ipDir} >> ${log_file}
#  }
#}
#update_ip_catalog -rebuild -repo_path ${ipDir}  >> ${log_file}



# Clean previous project directory
#-------------------------------------------------
# catch { cd $xprDir }
# # Delete the cache
# file delete -force -- [concat ${projectName}.cache] 
# # Delete all *.log and *.jou files
# set fileList [glob -nocomplain *.jou]
# if { $fileList ne "" } {
#   foreach file $fileList {
#     file delete $file
#   }
# }
# set fileList [glob -nocomplain *.log] 
# if {$fileList ne ""} {
#   foreach file $fileList {
#     file delete $file
#   }
# }
# catch { cd $currDir }


# Clean previous IP directory
#-------------------------------------------------
# file delete -force ${ipDir}
# file mkdir  ${ipDir}



# Set project properties
#-------------------------------------------------
#set obj [get_projects ${projectName}]
#set_property part ${xilPartName} $obj                        -verbose
#set_property "target_language" "Verilog" $obj                -verbose
#set_property top ${topName} [current_fileset]                -verbose
#set_property top_file ${srcDir}/${topFile} [current_fileset] -verbose 

# Add sources
#-------------------------------------------------
#add_files ${srcDir}  >> ${log_file}


#==============================================================================
#  LIST OF VIVADO-BASED IPs
#==============================================================================




#-------------------------------------------------
# Add HLS Generated IP(s) to the User Repository
#-------------------------------------------------
#set_property  ip_repo_paths ${ipDir} [current_project] >> ${log_file}
#set ipList [glob -nocomplain *.zip] >> ${log_file}
#if { $ipList ne "" } {
#  foreach ip $ipList {
#      update_ip_catalog -add_ip ${ipDir}/${ip} -repo_path ${ipDir} >> ${log_file}
#  }
#}
#update_ip_catalog -rebuild -repo_path ${ipDir}  >> ${log_file}


#------------------------------------------------------------------------------  
# USER-IP : Addres Resolution Server  
#------------------------------------------------------------------------------
#set ipModName "AddressResolutionServer"

#create_ip -name  arp_server -vendor IBM -library hls -version 0.1 \
#    -module_name ${{ipModName}}  -dir ${ipDir}
 
#generate_target {instantiation_template} \
#    [get_files  $ipDir/${ipModName}/${ipModName}.xci ] >> ${log_file} 

#------------------------------------------------------------------------------  
# USER-IP : IP Rx Handler
#------------------------------------------------------------------------------
# set ipModName "IpRxHandler"

# create_ip -name  iprx_handler -vendor IBM -library hls -version 0.1 \
#     -module_name ${{ipModName}}-dir ${ipDir}
 
# generate_target {instantiation_template} \
#     [get_files  $ipDir/${ipModName}/${ipModName}.xci ] >> ${log_file} 


# Add constraints files
#-------------------------------------------------
# add_files -fileset constrs_1 $xdcDir >> ${log_file}
# set_property used_in_synthesis false [get_files  ${xdcDir}/${projectName}_timg.xdc] >> ${log_file}
# set_property used_in_synthesis false [get_files  ${xdcDir}/${projectName}_pins.xdc] >> ${log_file}


# Close log file
#------------------------------------------------
# puts "## \[CREATE PROJECT\] ## Done   at: [clock format [clock seconds] -format {%T %a %b %d %Y}]"
# close_project >> ${log_file}


#OBSOLETE-20180216 Specify the IP Repository Path to add the HLS-based IP implementation paths.
#OBSOLETE-20180216    (Must do this because IPs are stored outside of the current project) 
#OBSOLETE-20180216 -------------------------------------------------------------------------------
#OBSOLETE-20180216 set_property      ip_repo_paths ${ipHls} [ current_project ]
#OBSOLETE-20180216 update_ip_catalog -rebuild

#OBSOLETE-20180216  Set the IP_REPO_PATHS to Add *ALL* the HLS-based IP Implementation Paths
#OBSOLETE-20180216 -------------------------------------------------------------------------------
#OBSOLETE-20180216 set hlsList [ get_ips -filter {IPDEF=~*IBM:hls:*} ]
#OBSOLETE-20180216 my_dbg_trace "hlsList      = $hlsList" ${dbgLvl}
#OBSOLETE-20180216 foreach ip ${hlsList} {
#OBSOLETE-20180216     my_dbg_trace "ip           = $ip" ${dbgLvl}
#OBSOLETE-20180216      set venNamLibVer [ get_property IPDEF [ get_ips -filter "CONFIG.Component_Name =~ ${ip}" ] ]
#OBSOLETE-20180216      my_dbg_trace "venNamLibVer = $venNamLibVer" ${dbgLvl}
#OBSOLETE-20180216      # At this point we have retrieved the "Vendor:Library:Name:Version" property 
#OBSOLETE-20180216      # which looks like: "IBM:hls:arp_server:1.0".
#OBSOLETE-20180216      set vendor  [ lindex [ split ${venNamLibVer} : ] 0 ]
#OBSOLETE-20180216      set library [ lindex [ split ${venNamLibVer} : ] 1 ]
#OBSOLETE-20180216      set ipname  [ lindex [ split ${venNamLibVer} : ] 2 ]
#OBSOLETE-20180216      set version [ lindex [ split ${venNamLibVer} : ] 3 ]
#OBSOLETE-20180216      my_dbg_trace "ipname       = $ipname" ${dbgLvl}
#OBSOLETE-20180216      # Set ip_repo_paths
#OBSOLETE-20180216      set_property ip_repo_paths ${hlsDir}/${ipname}/${ipname}_prj/solution1/impl/ip [current_fileset]
#OBSOLETE-20180216  }
