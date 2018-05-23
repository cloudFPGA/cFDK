DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: arp_server_prj

arp_server_prj: $(DEPS)
	vivado_hls -f run_hls.tcl

clean:
	${RM} -rf arp_server_prj
	${RM} vivado*.log
