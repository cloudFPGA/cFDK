# Name of the HLS core to build
ipName = jacobi2dv0


DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view
all: ${ipName}_prj/solution1/impl/ip

${ipName}_prj/solution1/impl/ip: $(DEPS)
	rm -rf $@
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@touch $@

sim: 
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim: 
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database $(ipName).wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log


