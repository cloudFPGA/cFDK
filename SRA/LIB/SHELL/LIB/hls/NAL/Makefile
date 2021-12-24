# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

# *****************************************************************************
# *                            cloudFPGA
# *----------------------------------------------------------------------------
# * Created : July 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that synthesizes and packages the current HLS core
# *     as an IP.
# * 
# * Synopsis:
# *   make clean   : Cleanup of the current directory.
# *   make         : Runs the default build for this core.
# *
# ******************************************************************************


# Name of the HLS core to build
ipName = nal
top_suffix =_main
# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)
SRC_DEPS += ../network_utils.hpp ../network_utils.cpp ../NTS/nts_config.hpp ../NTS/nts_types.hpp ../NTS/nts.hpp

REF_DEPS = $(shell find ./ref/ -type f)

.PHONY: all clean csim cosim cosim_view

all: ${ipName}_prj/solution1/impl/ip

${ipName}_prj/solution1/impl/ip: $(SRC_DEPS) $(REF_DEPS)
	$(MAKE) clean
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl | tee csynth.log
	@#print the report
	@cat ${ipName}_prj/solution1/syn/report/${ipName}${top_suffix}_csynth.rpt
	@#the following is for checking if the AXI4Lite Addresses had changed 
	@#diff ./${ipName}_prj/solution1/impl/ip/drivers/${ipName}${top_suffix}_v1_0/src/x${ipName}${top_suffix}_hw.h ./ref/x${ipName}${top_suffix}_hw.h
	@sed "s/[ \t]*\/\/.*$$//"  ./${ipName}_prj/solution1/impl/ip/drivers/${ipName}${top_suffix}_v1_0/src/x${ipName}${top_suffix}_hw.h | grep -v "^$$" > ./${ipName}_prj/x${ipName}${top_suffix}_hw.h_stripped.tmp
	@diff ./${ipName}_prj/x${ipName}${top_suffix}_hw.h_stripped.tmp ./ref/x${ipName}${top_suffix}_hw.h_stripped
	@/bin/echo -e "AXI4Lite Addresses checked: OK\n"
	@touch $@
	@touch ../../.ip_guard



csim: clean
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl | tee csim.log


cosim: 
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl | tee cosim.log

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database $(ipName)$(top_suffix).wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log

