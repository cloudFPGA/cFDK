# *******************************************************************************
# * Copyright 2016 -- 2021 IBM Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *******************************************************************************

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


