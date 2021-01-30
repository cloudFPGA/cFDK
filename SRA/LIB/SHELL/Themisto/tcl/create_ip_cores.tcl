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
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Create TCL script for a SPECIFIC SHELL version
#  *


package require cmdline

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
source ./decouple_ip_type.tcl 

set ipModName "Decoupler"
set ipName    "pr_decoupler"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "1.0"
set ipCfgList ${DecouplerType}

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

##------------------------------------------------------------------------------  
## VIVADO-IP : AXI Register Slice 
##------------------------------------------------------------------------------
##  Signal Properties
##    [Yes] : Enable TREADY
##    [8]   : TDATA Width (bytes)
##    [No]  : Enable TSTRB
##    [Yes] : Enable TKEEP
##    [Yes] : Enable TLAST
##    [0]   : TID Width (bits)
##    [0]   : TDEST Width (bits)
##    [0]   : TUSER Width (bits)
##    [No]  : Enable ACLKEN
##------------------------------------------------------------------------------
#set ipModName "AxisRegisterSlice_80"
#set ipName    "axis_register_slice"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "1.1"
#set ipCfgList  [ list CONFIG.TDATA_NUM_BYTES {10} \
#                      CONFIG.HAS_TKEEP {1} \
#                      CONFIG.HAS_TLAST {1} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


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



#------------------------------------------------------------------------------  
# VIVADO-IP : FIFO Generator
#------------------------------------------------------------------------------
set ipModName "FifoNetwork_Data"
set ipName    "fifo_generator"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "13.2"
set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {64} CONFIG.Output_Data_Width {64} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Empty_Threshold_Assert_Value {4} CONFIG.Empty_Threshold_Negate_Value {5} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : FIFO Generator
#------------------------------------------------------------------------------
set ipModName "FifoNetwork_Keep"
set ipName    "fifo_generator"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "13.2"
set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {8} CONFIG.Output_Data_Width {8} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Empty_Threshold_Assert_Value {4} CONFIG.Empty_Threshold_Negate_Value {5} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : FIFO Generator
#------------------------------------------------------------------------------
set ipModName "FifoNetwork_Last"
set ipName    "fifo_generator"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "13.2"
set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Empty_Threshold_Assert_Value {4} CONFIG.Empty_Threshold_Negate_Value {5} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


#------------------------------------------------------------------------------  
# VIVADO-IP : FIFO Generator
#------------------------------------------------------------------------------
set ipModName "FifoSession_Data"
set ipName    "fifo_generator"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "13.2"
set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {16} CONFIG.Output_Data_Width {16} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Empty_Threshold_Assert_Value {4} CONFIG.Empty_Threshold_Negate_Value {5} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


##------------------------------------------------------------------------------  
## VIVADO-IP : FIFO Generator
##------------------------------------------------------------------------------
#set ipModName "FifoNetwork_Data_Large"
#set ipName    "fifo_generator"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "13.2"
#set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {64} CONFIG.Output_Data_Width {64} \
#                CONFIG.Input_Depth {8192} CONFIG.Output_Depth {8192} CONFIG.Data_Count_Width {13} CONFIG.Write_Data_Count_Width {13} CONFIG.Read_Data_Count_Width {13} \
#                CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} CONFIG.Full_Threshold_Assert_Value {8063} CONFIG.Full_Threshold_Negate_Value {8062} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }
#
#
##------------------------------------------------------------------------------  
## VIVADO-IP : FIFO Generator
##------------------------------------------------------------------------------
#set ipModName "FifoNetwork_Keep_Large"
#set ipName    "fifo_generator"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "13.2"
#set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {8} CONFIG.Output_Data_Width {8} \
#                CONFIG.Input_Depth {8192} CONFIG.Output_Depth {8192} CONFIG.Data_Count_Width {13} CONFIG.Write_Data_Count_Width {13} CONFIG.Read_Data_Count_Width {13} \
#                CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} CONFIG.Full_Threshold_Assert_Value {8063} CONFIG.Full_Threshold_Negate_Value {8062} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }
#
#
##------------------------------------------------------------------------------  
## VIVADO-IP : FIFO Generator
##------------------------------------------------------------------------------
#set ipModName "FifoNetwork_Last_Large"
#set ipName    "fifo_generator"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "13.2"
#set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} \
#                CONFIG.Input_Depth {8192} CONFIG.Output_Depth {8192} CONFIG.Data_Count_Width {13} CONFIG.Write_Data_Count_Width {13} CONFIG.Read_Data_Count_Width {13} \
#                CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} CONFIG.Full_Threshold_Assert_Value {8063} CONFIG.Full_Threshold_Negate_Value {8062} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }
#
#
##------------------------------------------------------------------------------  
## VIVADO-IP : FIFO Generator
##------------------------------------------------------------------------------
#set ipModName "FifoSession_Data_Large"
#set ipName    "fifo_generator"
#set ipVendor  "xilinx.com"
#set ipLibrary "ip"
#set ipVersion "13.2"
#set ipCfgList [ list CONFIG.Performance_Options {First_Word_Fall_Through} CONFIG.Input_Data_Width {16} CONFIG.Output_Data_Width {16} \
#                CONFIG.Input_Depth {8192} CONFIG.Output_Depth {8192} CONFIG.Data_Count_Width {13} CONFIG.Write_Data_Count_Width {13} CONFIG.Read_Data_Count_Width {13} \
#                CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} CONFIG.Full_Threshold_Assert_Value {8063} CONFIG.Full_Threshold_Negate_Value {8062} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }


##------------------------------------------------------------------------------  
## IBM-HSL-IP : NRC IP
##------------------------------------------------------------------------------
#set ipModName "NetworkRoutingCore"
#set ipName    "nrc_main"
#set ipVendor  "IBM"
#set ipLibrary "hls"
#set ipVersion "1.0"
#set ipCfgList  [ list CONFIG.Component_Name {NRC} ]
#
#set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
#
#if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }

#------------------------------------------------------------------------------  
# IBM-HSL-IP : NAL IP
#------------------------------------------------------------------------------
set ipModName "NetworkAbstractionLayer"
set ipName    "nal_main"
set ipVendor  "IBM"
set ipLibrary "hls"
set ipVersion "1.0"
set ipCfgList  [ list CONFIG.Component_Name {NAL} ]

set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]

if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



#now, execute the finish
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


