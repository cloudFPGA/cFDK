DEPS := $(shell find ./src/ -type f)

.PHONY: all sim cosim
all: smc_prj

smc_prj: $(DEPS)
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@touch $@

sim: 
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim: 
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

clean:
	${RM} -rf smc_prj
	${RM} vivado*.log
