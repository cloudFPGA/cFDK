open_project echo_app_prj

set_top echo_app

add_files echo_app.cpp
add_files -tb echo_app.cpp

open_solution "solution1"
#set_part {xc7vx690tffg1761-2}
set_part {xcku060-ffva1156-2-e}
create_clock -period 6.4 -name default


#csim_design -clean -setup
csynth_design
export_design -format ip_catalog -display_name "Echo Application" -description "Echo Application for Testing" -vendor "IBM" -version "1.00"
exit
