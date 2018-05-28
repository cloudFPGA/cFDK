# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Francois Abel, Jagath Weerasinghe  
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the application
# *   RoleEchoStoreAndForward.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "role_echo_store_and_forward"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "Echo application operated in store-and-forwarding mode."
set ipDescription  "Stores every received UDP and TCP packets in DDR4 before reading and and echoing them back to the network."
set ipVendor       "USER"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set testDir      ${currDir}/test
set implDir      ${currDir}/${projectName}_prj/${solutionName}/impl/ip 
# [FIXME] set repoDir      ${currDir}/../../ip

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}

add_files     ${srcDir}/${projectName}.cpp
add_files -tb ${testDir}/test_${projectName}.cpp

open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Synthesis
#-------------------------------------------------
csim_design -clean -setup
csynth_design

# Export RTL
#-------------------------------------------------
export_design -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}

exit

