# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jan 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that synthesizes and packages the current HLS core
# *     as an IP.
# * 
# * Synopsis:
# *   make         : Runs the default build for this core.
# *   make help    : Shows a helper message. 
# *
# ******************************************************************************

# Name of the HLS core to build
ipName = toe

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f | grep -v "\ ")

.PHONY: all clean help csim csynth cosim project

all: ${ipName}_prj

${ipName}_prj: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl
	@touch $@


csim: ## Runs the HLS C simulation
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

csynth: ## Runs the HLS C synthesis
	export hlsCSim=1; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

cosim: ## Runs the HLS C/RTL cosimulation
	export hlsCSim=1; export hlsCSynth=1; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

project: ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

clean: ## Cleans the current project
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log

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

