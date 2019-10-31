# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Sep 2019
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that compiles, simulates or synthesizes the 
# *               current C function.
# * 
# * Synopsis:
# *   make         : Runs the default build for this C function.
# *   make help    : Shows a helper message. 
# *
# ******************************************************************************

# Name of the C function to build
cFunc = tx_engine

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f | grep -v "\ ")

.PHONY: all clean help csim csynth cosim project

${cFunc}_prj/solution1/impl/ip: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@touch $@
	@touch ../../.ip_guard

all: ## Runs CSIM, CSYNTH and COSIM
	export hlsCSim=1; export hlsCSynth=1; export hlsCoSim=1; vivado_hls -f run_hls.tcl

clean: ## Cleans the current project
	${RM} -rf ${cFunc}_prj
	${RM} vivado*.log

csim: ## Runs the HLS C simulation
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

csynth: ## Runs the HLS C synthesis
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

cosim: ## Runs the HLS C/RTL cosimulation
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

project: ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

rtlSyn: ## Executes logic synthesis to obtain more accurate timing and and utilization numbers
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=2; vivado_hls -f run_hls.tcl

rtlImpl: ## Performs both RTL synthesis and implementation including detailed palce and route
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=3; vivado_hls -f run_hls.tcl

test: ## Runs HLS csim, csynth and cosim in sequence
	export hlsCSim=1; export hlsCSynth=1; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

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

