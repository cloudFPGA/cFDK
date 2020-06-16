#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Create TCL script for a SPECIFIC SHELL version
#  *


package require cmdline

source ../../../tcl/xpr_settings.tcl

set savedDir [pwd]

# STEP-1: Execute the SHELL/LIB version. 
#   This will create the cores which are common to all shells. 
source ../../LIB/tcl/create_ip_cores.tcl

# STEP-2: Create the IP cores which are specific to this shell. 
cd $savedDir 

# Uncomment the following lines to add a specific IP to your shell.

#------------------------------------------------------------------------------  
# KALE-HSL-IP : A short description of this IP
#------------------------------------------------------------------------------
# set ipModName "TheIpModule"
# set ipName    "TheIpName"
# set ipVendor  "IBM"
# set ipLibrary "hls"
# set ipVersion "1.0"
# set ipCfgList  [ list ]
#
# set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
# if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


# STEP-3: Now, execute the finish script
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


