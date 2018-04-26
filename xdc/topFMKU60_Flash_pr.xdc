#=====================================================================
# Constraints for partial reconfiguration
#=====================================================================

create_pblock pblock_ROLE
add_cells_to_pblock [get_pblocks pblock_ROLE] [get_cells -quiet [list ROLE]]
resize_pblock [get_pblocks pblock_ROLE] -add {SLICE_X6Y10:SLICE_X59Y294}
resize_pblock [get_pblocks pblock_ROLE] -add {DSP48E2_X1Y4:DSP48E2_X10Y117}
resize_pblock [get_pblocks pblock_ROLE] -add {RAMB18_X1Y4:RAMB18_X7Y117}
resize_pblock [get_pblocks pblock_ROLE] -add {RAMB36_X1Y2:RAMB36_X7Y58}
set_property SNAPPING_MODE ON [get_pblocks pblock_ROLE]
set_property HD.RECONFIGURABLE true [get_cells ROLE]


