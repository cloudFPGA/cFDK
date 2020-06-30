# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Feb 7 2018
# * Authors : Francois Abel
# * 
# * Description : A Tcl script to generate the HLS-based IP cores instanciated 
# *   by the SHELL of the FMKU60 as well as the cores created from the Vivado  
# *   repository of the Xilinx-IP catalog.
# * 
# * Synopsis : vivado -mode batch -source <this_file> -notrace
# *                             [ -log    <log_file_name>        ]
# *                             [ -help                          ]
# *                             [ -tclargs <myscript_arguments>  ]
# * 
# * Warning : All arguments before the '-tclargs' are arugments to the Vivado 
# *   invocation (e.g. -mode, -source, -log and -tclargs itself). All options
# *   after the '-tclargs' are TCL arguments to this script. Enter the following
# *   command to get the list of supported TCL arguments:
# *     vivado -mode batch -source <this_file> -notrace -tclargs -help
# * 
# * Return: 0 if OK, otherwise the number of errors which corresponds to the 
# *   number of IPs that failed to be generated. 
# * 
# * Reference documents:
# *  - UG896 / Ch.3 / Using Manage IP Projects.
# *  - UG896 / Ch.2 / IP Basics.
# *  - UG896 / Ch.6 / Tcl Commands for Common IP Operations.
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
            my_dbg_trace "An IP with name \'${ipModName}\' already exists and will be removed from the project \'${::ipXprName} !" ${::dbgLvl_1}
            remove_files -fileset ${ipModName} ${fileset}
            break
        }        
    }

    ## STEP-3: Check for possible dangling files (may happen after IP flow got broken)
    set files [get_files]
    foreach file $files {
        if [ string match "${::ipDir}/${ipModName}/${ipModName}.xci" ${file} ] {
            my_dbg_trace "A dangling IP file \'${ipModName}\' already exists and will be removed from the project \'${::ipXprName} !" ${::dbgLvl_1}
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
#  A procedure that automates the creation and the customizationof of an IP.
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
        my_dbg_trace "Skipping Module ${ipModName}" ${::dbgLvl_1}
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

    my_dbg_trace "Done with \'my_clean_ip\' (RC=${rc})." ${::dbgLvl_1}

    # Note-1: A typical 'create_ip" command looks like the following:
    #   "create_ip -name axis_register_slice -vendor xilinx.com -library ip \
    #              -version 1.1 -module_name ${ipModName} -dir ${ipDir}"
    #
    # Note-2: Intercepting errors from 'create_ip' with 'catch' does not work for me.
    #   Therefore, I decided to use the '-quiet' option which execute the command quietly,
    #   returning no messages from the command and always returning 'TCL_OK' regardless of
    #   any errors encountered during execution.
    if { [ catch { create_ip -name ${ipName} -vendor ${ipVendor} -library ${ipLibrary} \
                       -version ${ipVersion} -module_name ${ipModName} -dir ${ipDir} -quiet } errMsg ] } {
        puts "ERROR_MSG = " ${errMsg}
        my_err_puts "## The TCL command \'create_ip\' failed (Error message = ${errMsg}"
        return  ${::KO}
    }

    my_dbg_trace "Done with \'create_ip\' (errMsg=${errMsg})." ${::dbgLvl_1}

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
        my_dbg_trace "Done with \'set_property\'." ${::dbgLvl_1}
    } else {
        my_dbg_trace "There is no \'set_property\' to be done." ${::dbgLvl_2}
    }

    generate_target {instantiation_template} \
        [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ]
    my_dbg_trace "Done with \'generate_target\'." ${::dbgLvl_1}

    update_compile_order -fileset sources_1

    generate_target all [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ]
    my_dbg_trace "Done with \'generate_target all\'." ${::dbgLvl_1}

    catch { config_ip_cache -export [ get_ips -all ${ipModName} ] }

    export_ip_user_files -of_objects \
        [ get_files ${ipDir}/${ipModName}/${ipModName}.xci ] -no_script -sync -force -quiet
    my_dbg_trace "Done with \'export_ip_user_files\'." ${::dbgLvl_1}

    create_ip_run [ get_files -of_objects \
                        [ get_fileset sources_1 ] ${ipDir}/${ipModName}/${ipModName}.xci ]
    my_dbg_trace "Done with \'create_ip_run\'." ${::dbgLvl_1}

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
my_puts "##  CREATING IP CORES FOR THE SHELL "
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
            file delete -force ${ipDir}/ip_user_files
            #TODO:
            file delete -force ${ipXprDir}/${ipXprName}.xpr
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
##  PHASE-1: Creating Vivado-based IPs
##
###############################################################################
my_puts ""

#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [8]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_8"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {1} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [16]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_16"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {2} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [24]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_24"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {3} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [32]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_32"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {4} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [48]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_48"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {6} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

 
#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [64]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [8]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [Yes] : Enable TKEEP
#    [Yes] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_64"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {8} \
                      CONFIG.HAS_TKEEP {1} \
                      CONFIG.HAS_TLAST {1} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [96]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_96"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {12} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Register Slice [104]
#------------------------------------------------------------------------------
#  Signal Properties
#    [Yes] : Enable TREADY
#    [2]   : TDATA Width (bytes)
#    [No]  : Enable TSTRB
#    [No] : Enable TKEEP
#    [No] : Enable TLAST
#    [0]   : TID Width (bits)
#    [0]   : TDEST Width (bits)
#    [0]   : TUSER Width (bits)
#    [No]  : Enable ACLKEN
#------------------------------------------------------------------------------
set ipModName "AxisRegisterSlice_104"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {13} \
                      CONFIG.HAS_TKEEP {0} \
                      CONFIG.HAS_TLAST {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : 10G Ethernet Subsystem [ETH0, 10GBASE-R] 
#------------------------------------------------------------------------------
#  Ethernet Standard
#    [BASE-R]    : PCS/PMA Standard
#    [64 bit]    : AXI4-Stream data path width
#  MAC Options
#    Management  Options
#      [NO]      : AXI4-Lite for Configuration and Status
#      [200.00]  : AXI4-Lite Frequency (MHz)
#      [NO]}     : Statistic Gathering
#  Flow Control Options
#      [NO]      : IEEE802.1QBB Priority-based Flow Control
#  PCS/PMA Options
#    PCS/PMA Options
#      [NO]      : Auto-Negociation
#      [NO]      : Forward Error Correction (FEC)
#      [NO]      : Exclude Rx Elastic Buffer
#    DRP Clocking
#      [156.25]  : Frequency (MHz)
#    Transceiver : Clocking and Location
#      [X1Y8]    : Transceiver Location
#      [refclk0] : Transceiver RefClk Location
#      [156.25]  : Reference Clock Frequency (MHz)
#    Transceiver Debug   
#      [NO]      : Additionnal transceiver DRP ports
#      [YES]     : Additionnal transceiver control and status ports
#  IEEE1588 Options
#    [None]      : IEEE1588 hardware timestamping support
#  Shared Logic
#    [InCore]    : Include Shared Logic in core
#------------------------------------------------------------------------------
set ipModName "TenGigEthSubSys_X1Y8_R"
set ipName    "axi_10g_ethernet"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "3.1"
set ipCfgList  [ list CONFIG.Management_Interface {false} \
                      CONFIG.base_kr {BASE-R} \
                      CONFIG.DClkRate {156.25} \
                      CONFIG.SupportLevel {1} \
                      CONFIG.Locations {X1Y8} \
                      CONFIG.autonegotiation {0} \
                      CONFIG.TransceiverControl {true} \
                      CONFIG.fec {0} \
                      CONFIG.Statistics_Gathering {0} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

 
#------------------------------------------------------------------------------  
# VIVADO-IP : AXI DataMover [M512, S64, B16] 
#------------------------------------------------------------------------------
#  Basic Options
#    MM2S Interface
#      [ENABLE]
#        [Full]    : Channel Type
#        [512]     : Memory Map Data Width
#        [64]      : Stream Data Width
#        [16]      : Maximum Burst Size
#                     (BusrtsLength = (DataWidth / 8) * BurstSize) 
#        [16]      : Width of BTT field (bits)
#    S2MM Interface
#      [ENABLE]
#        [Full]    : Channel Type
#        [512]     : Memory Map Data Width
#        [64]      : Stream Data Width
#        [16]      : Maximum Burst Size
#                     (BusrtsLength = (DataWidth / 8) * BurstSize) 
#        [16]      : Width of BTT field (bits)
#
#    [DISABLE]     : XCACHE xUSER
#    [DISABLE]     : XCACHE xUSER
#    [DISABLE]     : MM2S Control Signals
#    [DISABLE]     : S2MM Control Signals
#    [33]          : Address Width
#
#  Advanced Options
#    MM2S Interface
#      [DISABLE]   : Asynchronous Clocks
#      [ENABLE]    : Allow Unaligned Transfer
#      [DISABLE]   : Store Forward
#      [4]         : ID Width
#      [0]         : ID Value        
#    S2MM Interface
#      [DISABLE]   : Asynchronous Clocks
#      [ENABLE]    : Allow Unaligned Transfer
#      [DISABLE]   : Indeterinate BTT Mode      
#      [DISABLE]   : Store Forward
#      [4]         : ID Width
#      [0]         : ID Value         
#------------------------------------------------------------------------------
set ipModName "AxiDataMover_M512_S64_B16"
set ipName    "axi_datamover"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "5.1"
set ipCfgList  [ list CONFIG.c_m_axi_mm2s_data_width {512} \
                      CONFIG.c_m_axis_mm2s_tdata_width {64} \
                      CONFIG.c_mm2s_burst_size {16} \
                      CONFIG.c_m_axi_s2mm_data_width {512} \
                      CONFIG.c_s_axis_s2mm_tdata_width {64} \
                      CONFIG.c_s2mm_burst_size {16} \
                      CONFIG.c_addr_width {33} \
                      CONFIG.c_include_mm2s_dre {true} \
                      CONFIG.c_include_s2mm_dre {true} \
                      CONFIG.c_mm2s_include_sf {false} \
                      CONFIG.c_s2mm_include_sf {false} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI DataMover [M512, S512, B64]
#------------------------------------------------------------------------------
#  Basic Options
#    MM2S Interface
#      [ENABLE]
#        [Full]    : Channel Type
#        [512]     : Memory Map Data Width
#        [512]     : Stream Data Width
#        [64]      : Maximum Burst Size 
#                     (BusrtsLength = (DataWidth / 8) * BurstSize) 
#        [16]      : Width of BTT field (bits)
#    S2MM Interface
#      [ENABLE]
#        [Full]    : Channel Type
#        [512]     : Memory Map Data Width
#        [512]     : Stream Data Width
#        [64]      : Maximum Burst Size
#                     (BusrtsLength = (DataWidth / 8) * BurstSize) 
#        [16]      : Width of BTT field (bits)
#
#    [DISABLE]     : XCACHE xUSER
#    [DISABLE]     : XCACHE xUSER
#    [DISABLE]     : MM2S Control Signals
#    [DISABLE]     : S2MM Control Signals
#    [33]          : Address Width
#
#  Advanced Options
#    MM2S Interface
#      [DISABLE]   : Asynchronous Clocks
#      [DISABLE]   : Allow Unaligned Transfer
#      [DISABLE]   : Store Forward
#      [4]         : ID Width
#      [0]         : ID Value        
#    S2MM Interface
#      [DISABLE]   : Asynchronous Clocks
#      [DISABLE]   : Allow Unaligned Transfer
#      [DISABLE]   : Indeterinate BTT Mode      
#      [DISABLE]   : Store Forward
#      [4]         : ID Width
#      [0]         : ID Value
#------------------------------------------------------------------------------
set ipModName "AxiDataMover_M512_S512_B64"
set ipName    "axi_datamover"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "5.1"
set ipCfgList  [ list CONFIG.c_m_axi_mm2s_data_width {512} \
                      CONFIG.c_m_axis_mm2s_tdata_width {512} \
                      CONFIG.c_mm2s_burst_size {64} \
                      CONFIG.c_m_axi_s2mm_data_width {512} \
                      CONFIG.c_s_axis_s2mm_tdata_width {512} \
                      CONFIG.c_s2mm_burst_size {64} \
                      CONFIG.c_addr_width {33} \
                      CONFIG.c_mm2s_include_sf {false} \
                      CONFIG.c_s2mm_include_sf {false} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI Interconnect [1M, 2S, A33, D512] 
#------------------------------------------------------------------------------
#  Global
#    [2]       : Number of Slave Interfaces
#  Global Settings
#    [4]       : Slave Interface ThreadID Width
#    [8]       : Master Interface ID Width
#    [33]      : Address Width
#    [512]     : Interconnect Internal Dat Width
#    [3]       : Synchronization Stages across Asynchronous Clock Crossing
#  Interfaces 
#    |Interface  |DataWidth|AsyncAclk|AclkRatio|ArbPriority|RegSlice |  
#    |-----------+---------+---------+---------+-----------+---------+
#    |Master I/F |   512   |   EN    |   1:1   |           |   EN    |
#    |Slave I/F 0|   512   |   EN    |   1:1   |   0(RR)   |   EN    |
#    |Slave I/F 1|   512   |   EN    |   1:1   |   0(RR)   |   EN    |
#  Read Write Channels
#    |Interface  |AXI Chan |Acceptanc|FiFoDepth|PacketFiFo |Acceptanc|FiFoDepth|PacketFiFo |   
#    |-----------+---------+---------+---------+-----------+---------+---------+-----------+
#    |Master I/F |  RD/WR  |   16    | 0(none) |   OFF     |   16    | 0(none) |   OFF     |
#    |Slave I/F 0|  RD/WR  |   16    | 0(none) |   OFF     |   16    | 0(none) |   OFF     |
#    |Slave I/F 1|  RD/WR  |   16    | 0(none) |   OFF     |   16    | 0(none) |   OFF     |
#     
#------------------------------------------------------------------------------
set ipModName "AxiInterconnect_1M2S_A33_D512"
set ipName    "axi_interconnect"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.7"
set ipCfgList  [ list CONFIG.INTERCONNECT_DATA_WIDTH {512} \
                      CONFIG.S00_AXI_DATA_WIDTH {512} \
                      CONFIG.S01_AXI_DATA_WIDTH {512} \
                      CONFIG.M00_AXI_DATA_WIDTH {512} \
                      CONFIG.S00_AXI_IS_ACLK_ASYNC {1} \
                      CONFIG.S01_AXI_IS_ACLK_ASYNC {1} \
                      CONFIG.M00_AXI_IS_ACLK_ASYNC {1} \
                      CONFIG.S00_AXI_WRITE_ACCEPTANCE {16} \
                      CONFIG.S01_AXI_WRITE_ACCEPTANCE {16} \
                      CONFIG.S00_AXI_READ_ACCEPTANCE {16} \
                      CONFIG.S01_AXI_READ_ACCEPTANCE {16} \
                      CONFIG.M00_AXI_WRITE_ISSUING {16} \
                      CONFIG.M00_AXI_READ_ISSUING {16} \
                      CONFIG.AXI_ADDR_WIDTH {33} \
                      CONFIG.S00_AXI_REGISTER {1} \
                      CONFIG.S01_AXI_REGISTER {1} \
                      CONFIG.M00_AXI_REGISTER {1} \
                      CONFIG.THREAD_ID_WIDTH {4}  ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



#------------------------------------------------------------------------------  
# VIVADO-IP : DDR4 Memory Channel Controller
#------------------------------------------------------------------------------
#  Basic
#    Controller Options
#      [MT40A1G8WE-075E] : Memory Part
#      [72]              : Data Width
#      [NO DM NO DBI]    : Data Mask and DBI
#      [True]            : ECC
#  Axi Options
#    [512]               : Data Width
#    [33]                : Address Width
#------------------------
# [TODO-Error related to 20017.4] 
#   ERROR: [Coretcl 2-1133] Requested IP 'xilinx.com:ip:ddr4:2.1' cannot be 
#   created. The latest available version in the catalog is 'xilinx.com:ip:ddr4:2.2'. 
#   If you do not wish to select a specific version please omit the version field
#   from the command arguments, or use a wildcard in the VLNV.
#------------------------------------------------------------------------------
set ipModName "MemoryChannelController"
set ipName    "ddr4"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
switch [version -short] {
    "2016.4" { set ipVersion "2.1" }
    "2017.4" { set ipVersion "2.2" }
    default  { 
        my_warn "Setting IP version to 2.2 for \'${ipModName}. n\\t Please double check version."
        set ipVersion "2.2"
    }
}
set ipCfgList  [ list CONFIG.C0.DDR4_MemoryPart {MT40A1G8WE-075E} \
                      CONFIG.C0.DDR4_DataWidth {72} \
                      CONFIG.C0.DDR4_AxiSelection {true} \
                      CONFIG.C0.DDR4_DataMask {NO_DM_NO_DBI} \
                      CONFIG.C0.DDR4_Ecc {true} \
                      CONFIG.C0.DDR4_AxiDataWidth {512} \
                      CONFIG.C0.DDR4_AxiAddressWidth {33} \
                      CONFIG.C0.DDR4_TimePeriod {833} \
                      CONFIG.C0.DDR4_InputClockPeriod {3332} \
                      CONFIG.C0.DDR4_Specify_MandD {true} \
                      CONFIG.C0.DDR4_AxiNarrowBurst {true} \
                      CONFIG.C0.DDR4_AxiIDWidth {8} ]


set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



#------------------------------------------------------------------------------  
# VIVADO-IP : AXI4-Stream Interconnect RTL [3S1M, D8] 
#------------------------------------------------------------------------------
#  Switch
#   Interconnect Properties 
#    [3]   : Number of slave intefaces (input channels)
#    [1]   : Number of master intefaces (output channels)
#   Global Signal Properties for all Master/Slave Interfaces
#    [Yes] : Use TDATA signal
#    [8]   : Interconnect switch TDATA witdth (bytes)
#    [No]  : Use TSTRB signal
#    [Yes] : Use TKEEP signal
#    [Yes] : USe TLAST signal
#    [No]  : Use TID signal
#    [1]   : Interconnect switch TID witdth (bits)
#    [No]  : Use TDEST signal
#    [1]   : Interconnect switch TDEST witdth
#    [No]  : Use TUSER signal
#    [1]   : Number of TUSER bits per TDATA byte
#    [No]  : Use ACLKEN signal
#   Arbitratio Settings
#    Arbiter Type
#     [On] : Round-Robin
#    [No]  : Arbitrate on TLAST transfer
#    [1]   : Arbitrate on maximum number of transfers
#    [0]   : Arbitrate on number of LOW TVALID cycles
#   Pipeline Settings
#    [Yes] : Enable decoder pipeline register
#    [No]  : Enable output pipeline register  
#   Synchronisation Stages 
#    [2]   : Across Cross Clock Domain Logic
#
#------------------------------------------------------------------------------  
set ipModName "AxisInterconnectRtl_3S1M_D8"
set ipName    "axis_interconnect"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.C_NUM_SI_SLOTS {3} \
                      CONFIG.SWITCH_TDATA_NUM_BYTES {8} \
                      CONFIG.HAS_TSTRB {false} \
                      CONFIG.HAS_TID {false} \
                      CONFIG.HAS_TDEST {false} \
                      CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {1} \
                      CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} \
                      CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} \
                      CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} \
                      CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} \
                      CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} \
                      CONFIG.M00_S01_CONNECTIVITY {true} \
                      CONFIG.M00_S02_CONNECTIVITY {true} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI4-Stream Interconnect RTL [2S1M, D8] 
#------------------------------------------------------------------------------
#  Switch
#   Interconnect Properties
#    [2]   : Number of slave intefaces (input channels)
#    [1]   : Number of master intefaces (output channels)
#   Global Signal Properties for all Master/Slave Interfaces
#    [Yes] : Use TDATA signal
#    [8]   : Interconnect switch TDATA witdth (bytes)
#    [No]  : Use TSTRB signal
#    [Yes] : Use TKEEP signal
#    [Yes] : USe TLAST signal
#    [No]  : Use TID signal
#    [1]   : Interconnect switch TID witdth (bits)
#    [No]  : Use TDEST signal
#    [1]   : Interconnect switch TDEST witdth
#    [No]  : Use TUSER signal
#    [1]   : Number of TUSER bits per TDATA byte
#    [No]  : Use ACLKEN signal
#   Arbitratio Settings
#    Arbiter Type
#     [On] : Round-Robin
#    [No]  : Arbitrate on TLAST transfer
#    [1]   : Arbitrate on maximum number of transfers
#    [0]   : Arbitrate on number of LOW TVALID cycles
#   Pipeline Settings
#    [Yes] : Enable decoder pipeline register
#    [No]  : Enable output pipeline register  
#   Synchronisation Stages 
#    [2]   : Across Cross Clock Domain Logic
#
#------------------------------------------------------------------------------  
set ipModName "AxisInterconnectRtl_2S1M_D8"
set ipName    "axis_interconnect"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList [list CONFIG.C_NUM_SI_SLOTS {2} \
                    CONFIG.SWITCH_TDATA_NUM_BYTES {8} \
                    CONFIG.HAS_TSTRB {false} \
                    CONFIG.HAS_TID {false} \
                    CONFIG.HAS_TDEST {false} \
                    CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {1} \
                    CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} \
                    CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} \
                    CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} \
                    CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} \
                    CONFIG.M00_S01_CONNECTIVITY {true} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#OBSOLETE: moved to Shell-tcl
##------------------------------------------------------------------------------  
## VIVADO-IP : Decouple IP 
##------------------------------------------------------------------------------ 
##get current port Descriptions 
#source ${tclDir}/decouple_ip_type.tcl 
#
#set ipModName "Decoupler"
#set ipName    "pr_decoupler"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "1.0"
#set ipCfgList ${DecouplerType}
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : AXI HWICAP IP
#------------------------------------------------------------------------------ 

set ipModName "HWICAPC"
set ipName    "axi_hwicap"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "3.0"
set ipCfgList [list CONFIG.C_WRITE_FIFO_DEPTH {1024} \
                    CONFIG.Component_Name {HWICAP} \
                    CONFIG.C_ICAP_EXTERNAL {0} ] 

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



################################################################################
##
##  PHASE-2: Creating HLS-based cores
##
################################################################################
my_puts ""

# Specify the IP Repository Path to add the HLS-based IP implementation paths.
#   (Must do this because IPs are stored outside of the current project) 
#-------------------------------------------------------------------------------
#set_property      ip_repo_paths ${hlsDir} [ current_fileset ]
#set_property      ip_repo_paths [list ${ip_repo_paths} ${cFpRootDir}/cFDK/SRA/LIB/SHELL/LIB/hls ] [ current_fileset ]
# --> only HLS cores in SHELL/LIB; should be an absolut path
set_property      ip_repo_paths ${cFpRootDir}/cFDK/SRA/LIB/SHELL/LIB/hls [ current_fileset ]
update_ip_catalog

#------------------------------------------------------------------------------  
# IBM-HSL-IP : Address Resolution Server 
#------------------------------------------------------------------------------
set ipModName "AddressResolutionServer"
set ipName    "arp"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

#------------------------------------------------------------------------------  
# IBM-HSL-IP : Content-Addressable Memory 
#------------------------------------------------------------------------------
set ipModName "ContentAddressableMemory"
set ipName    "cam"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : Internet Control Message Process 
#------------------------------------------------------------------------------
set ipModName "InternetControlMessageProcess"
set ipName    "icmp"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : IP RX Handler 
#------------------------------------------------------------------------------
set ipModName "IpRxHandler"
set ipName    "iprx"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : IP TX Handler 
#------------------------------------------------------------------------------
set ipModName "IpTxHandler"
set ipName    "iptx"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : Ready Logic Barrier
#------------------------------------------------------------------------------
set ipModName "ReadyLogicBarrier"
set ipName    "rlb"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : TCP Offload Engine
#------------------------------------------------------------------------------
set ipModName "TcpOffloadEngine"
set ipName    "toe"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : UDP Offload Engine
#------------------------------------------------------------------------------
set ipModName "UdpOffloadEngine"
set ipName    "uoe"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# IBM-HSL-IP : FPGA Managememt Core
#------------------------------------------------------------------------------
set ipModName "FpgaManagementCore"
set ipName    "fmc"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list CONFIG.Component_Name {SMC} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



