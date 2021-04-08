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
#  *     Created: May 2018
#  *     Authors: Burkhard Ringlein
#  *
#  *     Description:
#  *        Constraints for partial reconfiguration
#  *

create_pblock pblock_ROLE
add_cells_to_pblock [get_pblocks pblock_ROLE] [get_cells -quiet [list ROLE]]

### Position 0
#resize_pblock [get_pblocks pblock_ROLE] -add {SLICE_X6Y10:SLICE_X59Y294}
#resize_pblock [get_pblocks pblock_ROLE] -add {DSP48E2_X1Y4:DSP48E2_X10Y117}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB18_X1Y4:RAMB18_X7Y117}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB36_X1Y2:RAMB36_X7Y58}

### Position 1
#resize_pblock [get_pblocks pblock_ROLE] -add {SLICE_X53Y60:SLICE_X90Y239}
#resize_pblock [get_pblocks pblock_ROLE] -add {DSP48E2_X9Y24:DSP48E2_X16Y95}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB18_X7Y24:RAMB18_X10Y95}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB36_X7Y12:RAMB36_X10Y47}

### Position 2 (working)
#resize_pblock [get_pblocks pblock_ROLE] -add {SLICE_X2Y198:SLICE_X91Y278}
#resize_pblock [get_pblocks pblock_ROLE] -add {DSP48E2_X0Y80:DSP48E2_X16Y109}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB18_X0Y80:RAMB18_X10Y109}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB36_X0Y40:RAMB36_X10Y54}

### Position 3 (to big?)
#resize_pblock [get_pblocks pblock_ROLE] -add {SLICE_X2Y155:SLICE_X91Y278}
#resize_pblock [get_pblocks pblock_ROLE] -add {DSP48E2_X0Y62:DSP48E2_X16Y109}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB18_X0Y62:RAMB18_X10Y109}
#resize_pblock [get_pblocks pblock_ROLE] -add {RAMB36_X0Y31:RAMB36_X10Y54}

### Position 4 (L-Shape)
#resize_pblock pblock_ROLE -from SLICE_X2Y192:SLICE_X91Y291 -to SLICE_X2Y194:SLICE_X91Y293 -from LAGUNA_X0Y120:LAGUNA_X13Y223 -to LAGUNA_X0Y120:LAGUNA_X13Y227 -locs keep_all
#resize_pblock pblock_1 -from SLICE_X2Y110:SLICE_X29Y192 -to SLICE_X2Y111:SLICE_X29Y193 -from DSP48E2_X0Y44:DSP48E2_X5Y75 -to DSP48E2_X0Y46:DSP48E2_X5Y75 -from RAMB18_X0Y44:RAMB18_X3Y75 -to RAMB18_X0Y46:RAMB18_X3Y75 -from RAMB36_X0Y22:RAMB36_X3Y37 -to RAMB36_X0Y23:RAMB36_X3Y37 -locs keep_all
#resize_pblock pblock_ROLE -from SLICE_X2Y198:SLICE_X91Y297 -to SLICE_X1Y193:SLICE_X90Y292 -from DSP48E2_X0Y80:DSP48E2_X16Y117 -to DSP48E2_X0Y78:DSP48E2_X16Y115 -from RAMB18_X0Y80:RAMB18_X10Y117 -to RAMB18_X0Y78:RAMB18_X10Y115 -from RAMB36_X0Y40:RAMB36_X10Y58 -to RAMB36_X0Y39:RAMB36_X10Y57 -locs keep_none
#
#resize_pblock pblock_ROLE -add {SLICE_X2Y194:SLICE_X91Y293 SLICE_X2Y111:SLICE_X29Y193 DSP48E2_X0Y46:DSP48E2_X5Y75 RAMB18_X0Y46:RAMB18_X3Y75 RAMB36_X0Y23:RAMB36_X3Y37}

#resize_pblock pblock_ROLE -from SLICE_X29Y193:SLICE_X89Y297 -to SLICE_X29Y190:SLICE_X89Y294 -from DSP48E2_X5Y78:DSP48E2_X15Y117 -to DSP48E2_X5Y76:DSP48E2_X15Y117 -from RAMB18_X4Y78:RAMB18_X10Y117 -to RAMB18_X4Y76:RAMB18_X10Y117 -from RAMB36_X4Y39:RAMB36_X10Y58 -to RAMB36_X4Y38:RAMB36_X10Y58 -from SLICE_X2Y111:SLICE_X28Y297 -to SLICE_X2Y108:SLICE_X28Y294 -from DSP48E2_X0Y46:DSP48E2_X4Y117 -to DSP48E2_X0Y44:DSP48E2_X4Y117 -from RAMB18_X0Y46:RAMB18_X3Y117 -to RAMB18_X0Y44:RAMB18_X3Y117 -from RAMB36_X0Y23:RAMB36_X3Y58 -to RAMB36_X0Y22:RAMB36_X3Y58 -locs keep_all'

#resize_pblock pblock_ROLE -add {SLICE_X29Y190:SLICE_X89Y294 DSP48E2_X5Y76:DSP48E2_X15Y117 RAMB18_X4Y76:RAMB18_X10Y117 RAMB36_X4Y38:RAMB36_X10Y58 SLICE_X2Y108:SLICE_X28Y294 DSP48E2_X0Y44:DSP48E2_X4Y117 RAMB18_X0Y44:RAMB18_X3Y117 RAMB36_X0Y22:RAMB36_X3Y58}


#resize_pblock pblock_ROLE -from SLICE_X31Y185:SLICE_X90Y289 -to SLICE_X31Y192:SLICE_X90Y296 -from DSP48E2_X6Y74:DSP48E2_X16Y115 -to DSP48E2_X6Y78:DSP48E2_X16Y117 -from RAMB18_X4Y74:RAMB18_X10Y115 -to RAMB18_X4Y78:RAMB18_X10Y117 -from RAMB36_X4Y37:RAMB36_X10Y57 -to RAMB36_X4Y39:RAMB36_X10Y58 -from SLICE_X2Y113:SLICE_X30Y289 -to SLICE_X2Y120:SLICE_X30Y296 -from DSP48E2_X0Y46:DSP48E2_X5Y115 -to DSP48E2_X0Y48:DSP48E2_X5Y117 -from RAMB18_X0Y46:RAMB18_X3Y115 -to RAMB18_X0Y48:RAMB18_X3Y117 -from RAMB36_X0Y23:RAMB36_X3Y57 -to RAMB36_X0Y24:RAMB36_X3Y58 -locs keep_all

resize_pblock pblock_ROLE -add {SLICE_X31Y192:SLICE_X90Y296 DSP48E2_X6Y78:DSP48E2_X16Y117 RAMB18_X4Y78:RAMB18_X10Y117 RAMB36_X4Y39:RAMB36_X10Y58 SLICE_X2Y120:SLICE_X30Y296 DSP48E2_X0Y48:DSP48E2_X5Y117 RAMB18_X0Y48:RAMB18_X3Y117 RAMB36_X0Y24:RAMB36_X3Y58}



set_property SNAPPING_MODE ON [get_pblocks pblock_ROLE]
set_property DONT_TOUCH true [get_cells ROLE]
set_property HD.RECONFIGURABLE true [get_cells ROLE]



