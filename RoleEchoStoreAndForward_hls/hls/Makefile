DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: role_echo_store_and_forward_prj

role_echo_store_and_forward_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf role_echo_store_and_forward_prj
	${RM} vivado*.log
