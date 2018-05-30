DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: icmp_server_prj

icmp_server_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf icmp_server_prj
	${RM} vivado*.log
