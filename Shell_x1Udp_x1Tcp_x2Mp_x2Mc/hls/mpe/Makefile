DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim cosim_view
all: mpe_prj

# TODO clean before rebuild seems to be necessary...
mpe_prj: $(DEPS) 
	rm -rf $@
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@touch $@

sim: 
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim: 
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database mpe_main.wdb\n" > ./mpe_prj/solution1/sim/verilog/open_wave.tcl
	cd ./mpe_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf mpe_prj
	${RM} vivado*.log
