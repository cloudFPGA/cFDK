DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: udp_prj

udp_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf udp_prj
	${RM} vivado*.log
