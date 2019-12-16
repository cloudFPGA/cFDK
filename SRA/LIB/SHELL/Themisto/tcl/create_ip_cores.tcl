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
# VIVADO-IP : Decouple IP 
#------------------------------------------------------------------------------ 
#get current port Descriptions 
# TODO: update Decoupling if interface is fixed
#source ./decouple_ip_type.tcl 
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
# VIVADO-IP : AXI Register Slice 
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
set ipModName "AxisRegisterSlice_80"
set ipName    "axis_register_slice"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.1"
set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {10} \
                      CONFIG.HAS_TKEEP {1} \
                      CONFIG.HAS_TLAST {1} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

#------------------------------------------------------------------------------  
# IBM-HSL-IP : NRC IP
#------------------------------------------------------------------------------
set ipModName "NetworkRoutingCore"
set ipName    "nrc_main"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list CONFIG.Component_Name {NRC} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



#now, execute the finish
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


