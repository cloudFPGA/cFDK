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

source ../../../tcl/xpr_constants.tcl


# execute the LIB version
source ../../LIB/tcl/create_ip_cores.tcl


#------------------------------------------------------------------------------  
# VIVADO-IP : Decouple IP 
#------------------------------------------------------------------------------ 
#get current port Descriptions 
source ${tclDir}/decouple_ip_type.tcl 

set ipModName "Decoupler"
set ipName    "pr_decoupler"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.0"
set ipCfgList ${DecouplerType}

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#now, execute the finish
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


