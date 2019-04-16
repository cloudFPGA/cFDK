## This file contains tcl commands, that must be sourced for partial reconfiguration bitGen. 

set_property UNAVAILABLE_DURING_CALIBRATION TRUE [get_ports piCLKT_Usr1Clk_p]


set_property BITSTREAM.GENERAL.COMPRESS False [current_design]
set_property bitstream.general.perFrameCRC yes [current_design]

# Request to embed a timestamp into the bitstream
set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [current_design]

