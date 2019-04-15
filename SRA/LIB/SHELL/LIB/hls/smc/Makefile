DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view
all: smc_prj/solution1/impl

# TODO clean before rebuild seems to be necessary...
smc_prj/solution1/impl: $(DEPS) 
	rm -rf $@
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@touch $@
	@touch ../../.ip_guard


sim: clean
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim: 
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database smc_main.wdb\n" > ./smc_prj/solution1/sim/verilog/open_wave.tcl
	cd ./smc_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf smc_prj
	${RM} vivado*.log
