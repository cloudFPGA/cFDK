# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
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
# * Created : Jun 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A Tcl script for the HLS batch syhthesis of the FMC core
# *   used by the SHELL of a cloudFPGA module.
# *   project.
# *
# ******************************************************************************

# User defined settings
#-------------------------------------------------
set projectName    "fmc"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"

set ipName         ${projectName}
set ipDisplayName  "FPGA Management Core for cloudFPGA"
set ipDescription  "FPGA module for the management stack, includes a.o. the RESTful interface"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set tbDir        ${currDir}/tb
#set implDir      ${currDir}/${projectName}_prj/${solutionName}/impl/ip
#set repoDir      ${currDir}/../../ip


# Get targets out of env
#-------------------------------------------------

set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
set_top       ${projectName}


add_files     ${srcDir}/${projectName}.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/${projectName}.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/http.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/http.hpp -cflags "-DCOSIM"
if { $hlsCoSim} {
  #to disable asserts etc.
  add_files -tb tb/tb_fmc.cpp -cflags "-DCOSIM"
} else {
  add_files -tb tb/tb_fmc.cpp
}


open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

# Run C Simulation and Synthesis
#-------------------------------------------------

if { $hlsSim} {
  csim_design -compiler gcc -clean
  #csim_design -compiler clang -clean
} else {

  #config_rtl -reset all -reset_async -reset_level low #seems to be ignored, due to the presence of AXI interfaces...
  csynth_design

  if { $hlsCoSim} {
    #cosim_design -compiler gcc -trace_level all -rtl vhdl
    #cosim_design -compiler clang -trace_level all
    cosim_design -compiler gcc -trace_level all
} else {

  # Export RTL
  #-------------------------------------------------
  export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
}
}

exit
