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

## This file contains tcl commands, that must be sourced for partial reconfiguration bitstream generation.

set_property UNAVAILABLE_DURING_CALIBRATION TRUE [get_ports piCLKT_Usr1Clk_p]


set_property BITSTREAM.GENERAL.COMPRESS False [current_design]
set_property bitstream.general.perFrameCRC yes [current_design]

# Request to embed a timestamp into the bitstream
set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [current_design]

