DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: iprx_handler_prj

iprx_handler_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf iprx_handler_prj
	${RM} vivado*.log
