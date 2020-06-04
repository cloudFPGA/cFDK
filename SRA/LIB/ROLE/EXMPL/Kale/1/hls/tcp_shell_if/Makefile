# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jan 2018
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
ipName = tcp_shell_if

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)

.PHONY: all clean help csim csynth cosim project regression

all: ${ipName}_prj/solution1/impl/ip

${ipName}_prj/solution1/impl/ip: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=1; vivado_hls -f run_hls.tcl
	@touch $@
	@touch ../../.ip_guard

.synth_guard: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@touch $@

#-- Targets --------------------------------------------------------------------

clean:                    ## Cleans the current project
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log
	@${RM} .synth_guard

csim:                     ## Runs the HLS C simulation
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

csynth: .synth_guard      ## Runs the HLS C synthesis

cosim: .synth_guard       ## Runs the HLS C/RTL cosimulation
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

project:                  ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

rtlSyn:                   ## Executes logic synthesis to obtain more accurate timing and and utilization numbers
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=2; vivado_hls -f run_hls.tcl

rtlImpl:                  ## Performs both RTL synthesis and implementation including detailed palce and route
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=3; vivado_hls -f run_hls.tcl

regression: .synth_guard  ## Runs HLS csim, csynth and cosim in sequence
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

help:                     ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | sed -e 's/^/    /' 
	@echo
