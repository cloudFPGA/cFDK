DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: tcp_role_if_prj

tcp_role_if_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf tcp_role_if_prj
	${RM} vivado*.log
