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

### Position 4 (L-Shape) (working)

#resize_pblock pblock_ROLE -add {SLICE_X31Y192:SLICE_X90Y296 DSP48E2_X6Y78:DSP48E2_X16Y117 RAMB18_X4Y78:RAMB18_X10Y117 RAMB36_X4Y39:RAMB36_X10Y58 SLICE_X2Y120:SLICE_X30Y296 DSP48E2_X0Y48:DSP48E2_X5Y117 RAMB18_X0Y48:RAMB18_X3Y117 RAMB36_X0Y24:RAMB36_X3Y58}



### Position 5 (U-Shape) (working)

#resize_pblock pblock_ROLE -add {SLICE_X72Y130:SLICE_X90Y295 DSP48E2_X14Y52:DSP48E2_X16Y117 RAMB18_X9Y52:RAMB18_X10Y117 RAMB36_X9Y26:RAMB36_X10Y58 SLICE_X33Y191:SLICE_X71Y295 DSP48E2_X6Y78:DSP48E2_X13Y117 RAMB18_X4Y78:RAMB18_X8Y117 RAMB36_X4Y39:RAMB36_X8Y58 SLICE_X3Y120:SLICE_X31Y295 DSP48E2_X0Y48:DSP48E2_X5Y117 RAMB18_X0Y48:RAMB18_X3Y117 RAMB36_X0Y24:RAMB36_X3Y58}


### Position 6 (U-Shape extended) (working)

resize_pblock pblock_ROLE -add {SLICE_X72Y130:SLICE_X90Y295 DSP48E2_X14Y52:DSP48E2_X16Y117 RAMB18_X9Y52:RAMB18_X10Y117 RAMB36_X9Y26:RAMB36_X10Y58 SLICE_X33Y191:SLICE_X71Y295 DSP48E2_X6Y78:DSP48E2_X13Y117 RAMB18_X4Y78:RAMB18_X8Y117 RAMB36_X4Y39:RAMB36_X8Y58 SLICE_X3Y120:SLICE_X31Y295 DSP48E2_X0Y48:DSP48E2_X5Y117 RAMB18_X0Y48:RAMB18_X3Y117 RAMB36_X0Y24:RAMB36_X3Y58 SLICE_X3Y15:SLICE_X17Y119 DSP48E2_X0Y6:DSP48E2_X3Y47 RAMB18_X0Y6:RAMB18_X1Y47 RAMB36_X0Y3:RAMB36_X1Y23}


set_property SNAPPING_MODE ON [get_pblocks pblock_ROLE]
set_property DONT_TOUCH true [get_cells ROLE]
set_property HD.RECONFIGURABLE true [get_cells ROLE]



