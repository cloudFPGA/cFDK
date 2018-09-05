

create_ip -name smc_main -vendor IBM -library hls -version 1.0 -module_name SMC
set_property -dict [list CONFIG.Component_Name {SMC}] [get_ips SMC]

generate_target all [get_ips SMC]

create_ip_run [get_ips SMC]

#launch_runs -jobs 8 SMC_synth_1


#update_compile_order -fileset sources_1

