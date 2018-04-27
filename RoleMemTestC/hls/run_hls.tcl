## from the automatic gernerated script.tcl

open_project memtest_prj
set_top memtest_app
add_files memtest.cpp
add_files memtest.h
open_solution "solution1"
set_part {xcku060-ffva1156-2-i} -tool vivado
create_clock -period 6.4 -name default

# for optional tuning:`
#source "./memtest_prj/solution1/directives.tcl"

#csim_design
csynth_design
#cosim_design

export_design -rtl vhdl -format ip_catalog -display_name "ROLE Memory Test" -vendor "IBM" -version "0.1"

exit

