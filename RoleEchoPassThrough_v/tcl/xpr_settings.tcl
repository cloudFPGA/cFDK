# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : May 31 2018
# * Authors : Francois Abel
# * 
# * Description : Source this TCL script to set the global settings of the 
# *   the current Vivado project.
# *   The "user defined" section shall be modified to adapt to your project. 
# * 
# * Synopsis : source <this_file>
# * 
# ******************************************************************************

#-------------------------------------------------------------------------------
# User Defined Settings (Can be edited)
#-------------------------------------------------------------------------------
set xprName      "roleEchoPassThrough" 
set xilPartName  "xcku060-ffva1156-2-i"
set brdPartName  "FMKU60"

# [INFO] SHELL and ROLE Naming Convention:
#  x1Udp stands for one UDP interface
#  x1Tcp stands fro one TCP interface
#  x2Mp  stands for two Memory Port interfaces
#  x2Mc  stands for two Memory Channel interfaces
#-------------------------------------------------------------- 
set topName      "Role_x1Udp_x1Tcp_x2Mp_x2Mc"
set topFile      "roleEchoHdl.v"


#-------------------------------------------------------------------------------
# This Xilinx Project Settings (Do not edit)  
#-------------------------------------------------------------------------------
set currDir      [pwd]
set rootDir      [file dirname [pwd]] 
set hdlDir       ${rootDir}/hdl
set hlsDir       ${rootDir}/hls
set ipDir        ${rootDir}/ip
set tclDir       ${rootDir}/tcl
set xdcDir       ${rootDir}/xdc
set xprDir       ${rootDir}/xpr

#-- IPs Managed by this Xilinx (Xpr) Project
set ipXprName    "managed_ip_project"
set ipXprDir     ${ipDir}/${ipXprName}
set ipXprFile    [file join ${ipXprDir} ${ipXprName}.xpr ]

#-- IPs Managed by the Shell-Role-Architecture (Sra) Project  
set ipSraDir     ${rootDir}/../../IP
set ipSraXprName "managed_ip_project"
set ipSraXprDir  ${ipSraDir}/${ipSraXprName}
set ipSraXprFile [ file join ${ipSraXprDir} ${ipSraXprName}.xpr ]


#-------------------------------------------------------------------------------
# TRACE, DEBUG and OTHER Settings and Variables  
#-------------------------------------------------------------------------------
set OK         0
set KO         1

set gDbgLvl    2

set OS         [string toupper [lindex $tcl_platform(os) 0]]

set BLACK      0 
set RED        1 
set GREEN      2 
set YELLOW     3
set BLUE       4 
set MAGENTA    5 
set CYAN       6 
set WHITE      7


#-------------------------------------------------------------------------------
# proc my_puts { text {fg -1} }
#  A procedure that adds a specific prompt and that changes the color of the 
#  text to display by calling the 'tput utility of Linux. 
#  :param text the text to display as a string.
#  :param fg   the foreground color to set (0=Black, 1=Red, 2=Green, 3=Yellow, 
#                                          4=Blue, 5=Magenta, 6=Cyan, 7=White).
#  :return nothing
#
#  See https://linux.101hacks.com/ps1-examples/prompt-color-using-tput/
#-------------------------------------------------------------------------------
proc my_puts { text {fg -1} } {
    set prompt "<cloudFPGA> "
    if { ${fg} != -1  && ${fg} >= 0 && ${fg} <= 7 } {
        set color ${fg}
    } else {
        set color ${::BLUE}
    }
    if { ${::OS} eq "WINDOWS" } {
        set coloredText "${prompt}${text}"
    } else {
        set coloredText [exec tput setaf ${color}]${prompt}${text}[exec tput sgr0]
    }
    puts ${coloredText}
}

#-------------------------------------------------------------------------------
# proc my_err_puts { text }
#  A procedure to display an error message colored in "RED". 
#  :param text the error message to display as a string.
#  :return nothing
#-------------------------------------------------------------------------------
proc my_err_puts { text } {
    my_puts "ERROR: ${text}" ${::RED} 
}

#-------------------------------------------------------------------------------
# proc my_warn_puts { text }
#  A procedure to display a warning message colored in "MAGENTA". 
#  :param text the warning message to display as a string.
#  :return nothing
#-------------------------------------------------------------------------------
proc my_warn_puts { text } {
    my_puts "WARNING: ${text}" ${::MAGENTA} 
}

#-------------------------------------------------------------------------------
# proc my_info_puts { text }
#  A procedure to display an information message colored in "GREEN". 
#  :param text the info message to display as a string.
#  :return nothing
#-------------------------------------------------------------------------------
proc my_info_puts { text } {
    my_puts "INFO: ${text}" ${::MAGENTA} 
}

#-------------------------------------------------------------------------------
# proc my_dbg_trace { text dbgLvl }
#  A procedure to display a debug message colored as a function of the global 
#  debug level set in this file and the debug level requested by the caller. 
#  :param text   the text to display as a string.
#  :param dbgLvl the current debug level used by the caller.
#  :return nothing
#-------------------------------------------------------------------------------
proc my_dbg_trace { text dbgLvl } {
    if { ${::gDbgLvl} == 0 } { 
        return 
    } else {
        switch ${dbgLvl} {
            1 { if  { ${::gDbgLvl} >= 1 } { my_puts "<D1> ${text}" ${::CYAN}   } }
            2 { if  { ${::gDbgLvl} >= 2 } { my_puts "<D2> ${text}" ${::GREEN}  } }
            3 { if  { ${::gDbgLvl} >= 3 } { my_puts "<D3> ${text}" ${::YELLOW} } }
            default { my_puts "<UNKNOWN DEBUG LEVEL> ${text}"                  }
        }
    }
}



#OBSOLETE-20180221 #-------------------------------------------------------------------------------
#OBSOLETE-20180221 # proc my_dbg_trace { text dbgLvl }
#OBSOLETE-20180221 #  A procedure to display a debug message colored as a function of the current
#OBSOLETE-20180221 #  debug level set by the caller. 
#OBSOLETE-20180221 #  :param text   the text to display as a string.
#OBSOLETE-20180221 #  :param dbgLvl the current debug level used by the caller.
#OBSOLETE-20180221 #  :return nothing
#OBSOLETE-20180221 #-------------------------------------------------------------------------------
#OBSOLETE-20180221 proc my_dbg_trace { text dbgLvl } {
#OBSOLETE-20180221     switch ${dbgLvl} {
#OBSOLETE-20180221         0 {
#OBSOLETE-20180221             return
#OBSOLETE-20180221         }
#OBSOLETE-20180221         1 {
#OBSOLETE-20180221             my_puts "<D1> ${text}" ${::CYAN}
#OBSOLETE-20180221         }
#OBSOLETE-20180221         2 {
#OBSOLETE-20180221             my_puts "<D2> ${text}" ${::GREEN}
#OBSOLETE-20180221         }
#OBSOLETE-20180221         3 {
#OBSOLETE-20180221             my_puts "<D3> ${text}" ${::YELLOW}
#OBSOLETE-20180221         }
#OBSOLETE-20180221         default {
#OBSOLETE-20180221             my_puts "<UNKNOWN DEBUG LEVEL> ${text}"
#OBSOLETE-20180221         }
#OBSOLETE-20180221     }
#OBSOLETE-20180221 }

#OBSOLETE-20180221 #-------------------------------------------------------------------------------
#OBSOLETE-20180221 # proc my_dbg_info_2 { text dbgLvl}
#OBSOLETE-20180221 #  A procedure to display a debug message of level #2 colored in "GREEN". 
#OBSOLETE-20180221 #  :param text the text to display as a string.
#OBSOLETE-20180221 #  :return nothing
#OBSOLETE-20180221 #-------------------------------------------------------------------------------
#OBSOLETE-20180221 proc my_dbg_info_2 { text dbgLvl } {
#OBSOLETE-20180221     if { ${dbgLvl} >= 2 } {
#OBSOLETE-20180221         my_puts "<D2> ${text}" ${::GREEN} 
#OBSOLETE-20180221     } else {
#OBSOLETE-20180221         return
#OBSOLETE-20180221     }
#OBSOLETE-20180221 }
