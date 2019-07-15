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

#source ../../../tcl/xpr_constants.tcl
source ../../../tcl/xpr_settings.tcl

set savedDir [pwd]
# execute the LIB version
source ../../LIB/tcl/create_ip_cores.tcl

cd $savedDir 

# seems not to be necessary currently...(only HLS cores in LIB)
#set_property      ip_repo_paths [list ${ip_repo_paths} ../hls/ ] [ current_fileset ]
#update_ip_catalog


#------------------------------------------------------------------------------  
# IBM-HSL-IP : UDP Role Interface
#------------------------------------------------------------------------------
set ipModName "UdpRoleInterface"
set ipName    "udp_role_if"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

#OBSOLETE-20190712 #------------------------------------------------------------------------------  
#OBSOLETE-20190712 # VIVADO-IP : Partial Reconfiguration Decoupler IP 
#OBSOLETE-20190712 #------------------------------------------------------------------------------ 

#OBSOLETE-20190712 # Define the ports of the PR-Decoupler IP
#OBSOLETE-20190712 source ./decouple_ip_type.tcl 

#OBSOLETE-20190712 set ipModName "Partial Reconfiguration Decoupler"
#OBSOLETE-20190712 set ipName    "pr_decoupler"
#OBSOLETE-20190712 set ipVendor  "xilinx.com"
#OBSOLETE-20190712 set ipLibrary "ip"
#OBSOLETE-20190712 set ipVersion "1.0"
#OBSOLETE-20190712 set ipCfgList ${DecouplerType}

#OBSOLETE-20190712 set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

#OBSOLETE-20190712 if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


# Now, execute the finish
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


