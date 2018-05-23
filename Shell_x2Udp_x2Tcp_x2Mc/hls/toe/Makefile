DEPS := $(shell find ./src/ -type f | grep -v "\ ")

.PHONY: all
all: toe_prj

toe_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf toe_prj
	${RM} vivado*.log
