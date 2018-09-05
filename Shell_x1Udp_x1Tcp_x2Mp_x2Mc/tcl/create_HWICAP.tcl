

create_ip -name axi_hwicap -vendor xilinx.com -library ip -version 3.0 -module_name HWICAPC
set_property -dict [list CONFIG.C_WRITE_FIFO_DEPTH {1024} CONFIG.Component_Name {HWICAP} CONFIG.C_ICAP_EXTERNAL {0}] [get_ips HWICAPC]

generate_target all [get_ips HWICAPC]

create_ip_run [get_ips HWICAPC]

#launch_runs -jobs 8 HWICAPC_synth_1


#update_compile_order -fileset sources_1

#WARNING: [Vivado 12-3523] Attempt to change 'Component_Name' from 'HWICAP' to 'HWICAP' is not allowed and is ignored.
#generate_target {instantiation_template} [get_files /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/HWICAP/HWICAP.xci]
#INFO: [IP_Flow 19-1686] Generating 'Instantiation Template' target for IP 'HWICAP'...
#update_compile_order -fileset sources_1
#generate_target all [get_files  /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/HWICAP/HWICAP.xci]
#INFO: [IP_Flow 19-1686] Generating 'Synthesis' target for IP 'HWICAP'...
#INFO: [IP_Flow 19-1686] Generating 'Simulation' target for IP 'HWICAP'...
#INFO: [IP_Flow 19-1686] Generating 'Change Log' target for IP 'HWICAP'...
#catch { config_ip_cache -export [get_ips -all HWICAP] }
#export_ip_user_files -of_objects [get_files /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/HWICAP/HWICAP.xci] -no_script -sync -force -quiet
#create_ip_run [get_files -of_objects [get_fileset sources_1] /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/HWICAP/HWICAP.xci]
#launch_runs -jobs 8 HWICAP_synth_1
#[Thu May 31 08:46:24 2018] Launched HWICAP_synth_1...
#Run output will be captured here: /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.runs/HWICAP_synth_1/runme.log
#export_simulation -of_objects [get_files /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.srcs/sources_1/ip/HWICAP/HWICAP.xci] -directory /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.ip_user_files/sim_scripts -ip_user_files_dir /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.ip_user_files -ipstatic_source_dir /home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.ip_user_files/ipstatic -lib_map_path [list {modelsim=/home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.cache/compile_simlib/modelsim} {questa=/home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.cache/compile_simlib/questa} {ies=/home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.cache/compile_simlib/ies} {vcs=/home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.cache/compile_simlib/vcs} {riviera=/home/ngl/gitrepo/cloudFPGA/SRA/FMKU60/TOP/TopFlash/xpr/topFMKU60_Flash.cache/compile_simlib/riviera}] -use_ip_compiled_libs -force -quiet
#
#
