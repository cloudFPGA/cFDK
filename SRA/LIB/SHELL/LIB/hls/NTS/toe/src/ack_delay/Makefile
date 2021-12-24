# *
# * Copyright 2016 -- 2021 IBM Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *

# *****************************************************************************
# *
# * Description : A makefile to simulate, synthesize and package the current
# *   HLS core as an IP. 
# *
# * Synopsis:
# *   make help
# *
# ******************************************************************************

# Name of the C function to build
cFunc = ack_delay

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f | grep -v "\ ")

.PHONY: all clean csim cosim csynth help project regression rtlSyn rtlImpl

${cFunc}_prj/solution1/impl/ip: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@touch $@

.synth_guard: $(SRC_DEPS)
	$(MAKE) clean
	export hlsCSim=0; export hlsCSynth=1; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl
	@touch $@

#-- Targets --------------------------------------------------------------------

all: ${cFunc}_prj/solution1/impl/ip

clean:                   ## Cleans the current project
	${RM} -rf ${cFunc}_prj
	${RM} vivado*.log
	@${RM} .synth_guard

csim:                    ## Runs the HLS C simulation
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

csynth: .synth_guard     ## Runs the HLS C synthesis

cosim: .synth_guard      ## Runs the HLS C/RTL cosimulation
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

project:                 ## Creates the HLS project
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=0; vivado_hls -f run_hls.tcl

regression:             ## Runs HLS csim, csynth and cosim in sequence
	export hlsCSim=1; export hlsCSynth=0; export hlsCoSim=1; export hlsRtl=0; vivado_hls -f run_hls.tcl

rtlSyn: .synth_guard    ## Executes logic synthesis to obtain more accurate timing and and utilization numbers
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=2; vivado_hls -f run_hls.tcl

rtlImpl: .synth_guard   ## Performs both RTL synthesis and implementation including detailed place and route
	export hlsCSim=0; export hlsCSynth=0; export hlsCoSim=0; export hlsRtl=3; vivado_hls -f run_hls.tcl

help:                    ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | sed -e 's/^/    /' 
	@echo

