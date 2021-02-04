# /*******************************************************************************
#  * Copyright 2016 -- 2020 IBM Corporation
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

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: Jun 2018
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile for a SPECIFIC HLS Core
#  *

DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view
all: fmc_prj/solution1/impl/ip

fmc_prj/solution1/impl/ip: $(DEPS) 
	rm -rf $@
	@#cp ./src/fmc_hw_flags.hpp ./src/cfdk_dynamic.hpp
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@#print the report
	@cat fmc_prj/solution1/syn/report/fmc_csynth.rpt
	@touch $@
	@touch ../../.ip_guard
	@#rm -f ./src/cfdk_dynamic.hpp


csim: clean
	@#cp ./src/fmc_sim_flags.hpp ./src/cfdk_dynamic.hpp
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@#rm -f ./src/cfdk_dynamic.hpp


cosim: 
	@#cp ./src/fmc_sim_flags.hpp ./src/cfdk_dynamic.hpp
	@#/bin/echo -e "This CoSim runs with smaller test sizes than the synthezised design!\nSo it must be rebuild again for the synthesis.\n"
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl
	@#touch ./src/fmc_hw_flags.hpp
	@#rm -f ./src/cfdk_dynamic.hpp
	@#/bin/echo -e "\nchecking wave database for correctness..."
	@#diff fmc_prj/solution1/sim/verilog/fmc.wdb ./ref/fmc.wdb
	@#/bin/echo -e "Wave database verified.\n"

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database fmc.wdb\n" > ./fmc_prj/solution1/sim/verilog/open_wave.tcl
	cd ./fmc_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf fmc_prj
	${RM} vivado*.log
