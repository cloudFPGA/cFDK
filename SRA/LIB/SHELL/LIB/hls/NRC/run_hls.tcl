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


# *****************************************************************************
# *                            cloudFPGA
# *----------------------------------------------------------------------------
# * Created : Dec 2017
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A Tcl script for the HLS batch syhthesis of one HLS core.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "nrc"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "Network Routing Core"
set ipDescription  "The Network Routing Core for the Shell"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set testDir      ${currDir}/test
set implDir      ${currDir}/${projectName}_prj/${solutionName}/impl/ip 
#set repoDir      ${currDir}/../../ip

# Get targets out of env  
#-------------------------------------------------
set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)


# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}_main

add_files     ${srcDir}/${projectName}.cpp
add_files     ${srcDir}/${projectName}.hpp
add_files     ${srcDir}/../../../../../hls/network.hpp
add_files     ${srcDir}/../../network_utils.hpp
add_files     ${srcDir}/../../network_utils.cpp
add_files     ${srcDir}/../../simulation_utils.hpp
add_files     ${srcDir}/../../simulation_utils.cpp
add_files     ${srcDir}/../../memory_utils.hpp

add_files -tb ${testDir}/tb_${projectName}.cpp

# Create a solution
#-------------------------------------------------
open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

if { $hlsSim} { 
  csim_design -compiler gcc -clean
} else {

  csynth_design
  
  if { $hlsCoSim} {
    cosim_design -compiler gcc -trace_level all 
  } else {
  
  # Export RTL
  #-------------------------------------------------
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
  }
}

# Exit Vivado HLS
#--------------------------------------------------
exit
