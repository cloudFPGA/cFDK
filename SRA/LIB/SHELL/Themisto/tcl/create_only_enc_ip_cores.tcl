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
#  *        Create TCL script for a SPECIFIC SHELL version
#  *


package require cmdline

source ../../../tcl/xpr_settings.tcl

set savedDir [pwd]
# execute the LIB version
source ../../LIB/tcl/create_only_enc_ip_cores.tcl

cd $savedDir 

# seems not to be necessary currently...(only HLS cores in LIB)
#set_property      ip_repo_paths [list ${ip_repo_paths} ../hls/ ] [ current_fileset ]
#update_ip_catalog


#now, execute the finish
source ../../LIB/tcl/create_ip_cores_running_finish.tcl


