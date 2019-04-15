DEPS := $(shell find ./src/ -type f)

.PHONY: all
all: dhcp_client_prj

dhcp_client_prj: $(DEPS)
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf dhcp_client_prj
	${RM} vivado*.log
