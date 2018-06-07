DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: smc_prj

smc_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf smc_prj
	${RM} vivado*.log
