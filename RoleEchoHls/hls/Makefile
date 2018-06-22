DEPS := $(shell find ./src/ -type f)

.PHONY: all

all: EchoStoreAndForward_prj

EchoStoreAndForward_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf EchoStoreAndForward_prj
	${RM} vivado*.log
