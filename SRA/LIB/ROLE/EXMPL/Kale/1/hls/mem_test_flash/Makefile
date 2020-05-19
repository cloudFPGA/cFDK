# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jan 2019
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that synthesizes and packages the current HLS core
# *               as an IP.
# * 
# * Synopsis:
# *   make help    : Displays a help message.
# *   make clean   : Cleanup of the current directory.
# *   make         : Runs the default build for this core.
# *
# ******************************************************************************

# Name of the HLS core to build
ipName =mem_test_flash

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view

all: ${ipName}_prj/solution1/impl/ip

${ipName}_prj/solution1/impl/ip: $(SRC_DEPS)
	@#rm -rf $@
	rm -rf ${ipName}_prj
	@cp ./src/memory_size_real.hpp ./src/dynamic.hpp
	#OBSOLETE-20190906	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl
	@#touch ${ipName}_prj
	@rm -f ./src/dynamic.hpp
	@touch $@
	@touch ../../.ip_guard

clean: ## Cleans the current project
	rm -rf ${ipName}_prj
	rm vivado*.log

csim: ## Runs the HLS C simulation
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	#OBSOLETE-20190906	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@rm -f ./src/dynamic.hpp

cosim: ## Runs the HLS C/RTL cosimulation
	@#rm -rf ${ipName}_prj
	@/bin/echo -e "This CoSim runs with smaller test sizes than the synthezised design!\nSo it must be rebuild again for the synthesis.\n"
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	#OBSOLETE-20190906	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@rm -rf ${ipName}_prj/solution1/impl/ip
	@rm -f ./src/dynamic.hpp

cosim_view: ## Opens the wave viewer
	@/bin/echo -e "current_fileset\nopen_wave_database $(ipName)_main.wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

export: ## Exports the RTL as IP
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl

project: ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl	

help: ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ ##\ (.+)' ${MAKEFILE_LIST} | column -t -c 2 -s ':#' | sed -e 's/^/    /' 
	@echo

