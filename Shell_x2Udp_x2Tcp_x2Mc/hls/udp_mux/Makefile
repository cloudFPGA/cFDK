DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: udp_mux_prj

udp_mux_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf udp_mux_prj
	${RM} vivado*.log
