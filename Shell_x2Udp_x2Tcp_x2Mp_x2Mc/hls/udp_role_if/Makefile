DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: udp_role_if_prj

udp_role_if_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf udp_role_if_prj
	${RM} vivado*.log
