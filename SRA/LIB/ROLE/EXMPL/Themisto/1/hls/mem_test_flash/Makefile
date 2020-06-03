# Name of the HLS core to build
ipName =mem_test_flash

DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view
all: ${ipName}_prj/solution1/impl/ip

${ipName}_prj/solution1/impl/ip: $(DEPS)
	@#rm -rf $@
	rm -rf ${ipName}_prj
	@cp ./src/memory_size_real.hpp ./src/dynamic.hpp
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@#touch ${ipName}_prj
	@rm -f ./src/dynamic.hpp
	@touch $@
	@touch ../../.ip_guard

sim: 
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@rm -f ./src/dynamic.hpp


cosim: 
	@#rm -rf ${ipName}_prj
	@/bin/echo -e "This CoSim runs with smaller test sizes than the synthezised design!\nSo it must be rebuild again for the synthesis.\n"
	@cp ./src/memory_size_debug.hpp ./src/dynamic.hpp
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl
	@rm -rf ${ipName}_prj/solution1/impl/ip
	@rm -f ./src/dynamic.hpp

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database $(ipName)_main.wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	rm -rf ${ipName}_prj
	rm vivado*.log


