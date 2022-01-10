# /*******************************************************************************
#  * Copyright 2016 -- 2022 IBM Corporation
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
#  *        The end of th create-ip.tcl script chain for
#  *        all SHELLs.
#  *

puts    ""
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
my_puts "################################################################################"
my_puts "##"
my_puts "##  DONE WITH THE CREATION OF \[ ${nrGenIPs} \] IP CORE(S) FROM THE MANAGED IP REPOSITORY "
my_puts "##"
if { ${nrErrors} != 0 } {
  my_puts "##"
  my_err_puts "##                   Warning: Failed to generate ${nrErrors} IP core(s) ! "
  my_puts "##"
  exit ${KO}
}
my_puts "################################################################################\n"


# Close project
#-------------------------------------------------
close_project

exit ${nrErrors}

