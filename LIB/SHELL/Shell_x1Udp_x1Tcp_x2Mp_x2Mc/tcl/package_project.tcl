# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Mar 3 2018
# * Authors : Francois Abel
# * 
# * Description : A Tcl script that packages the current SHELL project as an IP.
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
# *  - UG1118 / Ch.1 / Creating and Packaging Custom IP.
# *  - UG1119 / Lab1 / Packaging a Project.
# *  - UG994  / Ch.5 / Propagating Parameters in IP Integrator.
# *-----------------------------------------------------------------------------
# * Modification History:
# *  Fab: Feb-16-2018 Created.
# ******************************************************************************

package require cmdline

# Set the Global Settings used by the SHELL Project
#-------------------------------------------------------------------------------
if { ! [ info exists topName ] && ! [ info exists topFile ] } {
    # Expect to be in the TCL directory and source the TCL settings file 
    source xpr_settings.tcl
}

# Set the Local Settings used by this Script
#-------------------------------------------------------------------------------
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


#-------------------------------------------------------------------------------
# Parsing of the Command Line
#  Note: All the strings after the '-tclargs' option are considered as TCL
#        arguments to this script and are passed on to TCL 'argc' and 'argv'.
#-------------------------------------------------------------------------------
if { $argc > 0 } {
    my_dbg_trace "There is/are [ llength $argv ] TCL argument(s) to this script." ${dbgLvl_1}
    set i 0
    foreach arg $argv {
        my_dbg_trace "  argv\[$i\] = $arg " ${dbgLvl_2}
        set i [ expr { ${i} + 1 } ]
    }

    # Step-1: Processing of '$argv' using the 'cmdline' package
    set options {
        { clean "Removes the current IP from managed IP directory." }
        { show  "Display the names of the existing IP modules."     }
    }
    set usage "\nUSAGE: Vivado -mode batch -source ${argv0} -notrace -tclargs \[OPTIONS] \nOPTIONS:"
    
    array set kvList [ cmdline::getoptions argv ${options} ${usage} ]
    
    # Process the arguments
    foreach { key value } [ array get kvList ] {
        my_dbg_trace "KEY = ${key} | VALUE = ${value}" ${dbgLvl_2}
        if { ${key} eq "clean" &&  ${value} != 0 } {
            set targetIpCore ${topName}
            my_warn_puts "\[TODO\] Delete the previous \'${targetIpCore}\' IP directory !!! \n"
            break;
        } 
        if { ${key} eq "show" &&  ${value} != 0 } {
            my_puts "Current list of SRA-managed IPs:"
            if { [ file exists ${ipSraDir} ] == 1 } {
                set ipList [ glob ${ipSraDir}/* ]
                foreach ip ${ipList} {
                    puts "\t${ip}"
                }
            } else {
                my_puts "\t is empty..."
            }
            return ${::OK}
        } 
    }
}


my_puts "################################################################################"
my_puts "##"
my_puts "##  PACKAGING THE CURRENT PROJECT AS AN SHELL-ROLE-ARCHITECTURE IP "
my_puts "##"
my_puts "################################################################################"
my_puts "Start at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

# Always start from the root directory
#--------------------------------------------------------------------------------
catch { cd ${rootDir} }

# Create a managed IP directory for the Shell-Role-Architecture(SRA)
#--------------------------------------------------------------------------------
if { [ file exists ${ipSraDir} ] != 1 } {
    my_warn_puts "Could not find the managed IP directory for the Shell-Role-Architecture(SRA)."  
    my_puts "Creating the SRA-managed IP directory: \'${ipSraDir}\'. "
    file mkdir ${ipSraDir}
} else {
    my_puts "The SRA-managed IP directory is set to: \'${ipSraDir}\'."
}

# Create a project for the SRA-managed IPs (if does not exist)
#--------------------------------------------------------------------------------
if { [ file exists ${ipSraXprDir} ] != 1 } {
    my_warn_puts "Could not find the directory of the SRA-managed IP project."  
    my_puts "Creating a directory for the SRA-managed IP project: \'${ipSraXprDir}\'. "
    file mkdir ${ipSraXprDir}
}
if { [ file exists ${ipSraXprDir}/${ipSraXprName}.xpr ] != 1 } {
    my_warn_puts "Could not find the SRA-managed IP project file."  
    my_puts "Creating the managed IP project: \'${ipSraXprName}.xpr\' "
    create_project ${ipSraXprName} ${ipSraXprDir} -part ${xilPartName} -ip -force
} else {
    my_puts "The SRA-managed IP project is set to  : \'${ipSraXprName}.xpr\'."
}

# Open the current project (the one to be packaged)
#------------------------------------------------------------------------------
if { [ catch { current_project } rc ] } {
    my_puts "Opening the current project: ${xprDir}/${xprName}"
    open_project  ${xprDir}/${xprName}
} else {
    my_warn "Project \'${rc}\' is already opened!"
}


#================================================================================
#
# Packaging the currrent project as an IP
#
#================================================================================
set ipModName  "Shell"
set ipName     "Shell"
set ipVendor   "ZRL"
set ipLibrary  "cloudFPGA"
set ipTaxonomy "/${ipLibrary}/${ipModName}"
set ipVersion  "1.0"

my_puts ""

my_dbg_trace "Package the project" ${dbgLvl_2}
ipx::package_project -root_dir ${ipSraDir}/${ipModName} -vendor ${ipVendor} -library ${ipLibrary} -taxonomy ${ipTaxonomy} -generated_files -set_current false -force -force_update_compile_order -import_files true

my_dbg_trace "Unload the core" ${dbgLvl_2}
ipx::unload_core ${ipSraDir}/${ipModName}/component.xml

my_dbg_trace "Edit IP in project" ${dbgLvl_2}
ipx::edit_ip_in_project -upgrade true -name tmp_edit_project -directory ${ipSraDir}/${ipModName} ${ipSraDir}/${ipModName}/component.xml

my_dbg_trace "Set core description property" ${dbgLvl_2}
set today "[ clock format [clock seconds] -format {%b-%d-%Y} ]"
set ipDescription "IP Shell for the FMKU60 (Packaged on ${today} with the imported files option)."
set_property description ${ipDescription} [ipx::current_core]

my_dbg_trace "Set core display name property" ${dbgLvl_2}
set_property display_name ${ipModName} [ipx::current_core]

my_dbg_trace "Update compile order" ${dbgLvl_2}
update_compile_order -fileset sources_1

my_dbg_trace "Set core vendor property" ${dbgLvl_2}
set_property vendor_display_name {IBM Research Zurich} [ipx::current_core]

my_dbg_trace "Set core URL property" ${dbgLvl_2}
set_property company_url https://www.zurich.ibm.com/ [ipx::current_core]

my_dbg_trace "Set core revision property" ${dbgLvl_2}
set_property core_revision 2 [ipx::current_core]

my_dbg_trace "Create XGUI files" ${dbgLvl_2}
ipx::create_xgui_files [ipx::current_core]

my_dbg_trace "Update core checksum" ${dbgLvl_2}
ipx::update_checksums [ipx::current_core]

my_dbg_trace "Save core" ${dbgLvl_2}
ipx::save_core [ipx::current_core]

my_dbg_trace "Check core integrity" ${dbgLvl_2}
ipx::check_integrity -quiet [ipx::current_core]

my_dbg_trace "Archive core" ${dbgLvl_2}
ipx::archive_core ${ipSraDir}/${ipModName}/${ipVendor}_${ipLibrary}_${ipModName}_${ipVersion}.zip [ipx::current_core]


puts    ""
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
my_puts "################################################################################"
my_puts "##  DONE WITH CREATING AND PACKAGING THE PROJECT AS AN IP."
my_puts "##                                                        "
my_puts "##             (Return Code = ${nrErrors}) "
my_puts "################################################################################\n"

# Close project
#-------------------------------------------------
close_project

return ${nrErrors}




#BURKHARDS NOTE TODO
##0
#ipx::package_project -root_dir /dataL/ngl/cloudFPGA/SRA/IP/FMKU60/Shell -vendor ZRL -library cloudFPGA -taxonomy /cloudFPGA/Shell -set_current false
## -generated_files  seems to have three states true/false/not set 
##
##1
#ipx::edit_ip_in_project /dataL/ngl/cloudFPGA/SRA/IP/FMKU60/Shell/component.xml
#set_property description {Default version of the shell for the FMKU60} [ipx::current_core]
#set_property vendor_display_name {IBM Research} [ipx::current_core]
#set_property company_url http://www.zurich.ibm.com [ipx::current_core]
#set_property core_revision 2 [ipx::current_core]
#set_property version 1.4 [ipx::current_core]
#set_property display_name Shell_v1_4 [ipx::current_core]
#set_property description Shell_v1_4 [ipx::current_core]
##2
#ipx::create_xgui_files [ipx::current_core]
##3
#ipx::update_checksums [ipx::current_core]
##4
#ipx::save_core [ipx::current_core]
##5
#ipx::check_integrity -quiet [ipx::current_core]
##6
#ipx::archive_core /dataL/ngl/cloudFPGA/SRA/SHELL/FMKU60/Shell/ZRL_cloudFPGA_Shell_1.0.zip [ipx::current_core]
##7
#close_project 
#rm -rf edit_ip.*
#







