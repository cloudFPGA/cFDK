DEPS := $(shell find ./src/ -type f)

.PHONY: all

all: RoleEchoStoreAndForward_prj

RoleEchoStoreAndForward_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf RoleEchoStoreAndForward_prj
	${RM} vivado*.log
