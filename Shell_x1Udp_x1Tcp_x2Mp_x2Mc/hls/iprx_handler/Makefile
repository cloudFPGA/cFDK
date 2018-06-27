# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Jan 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that synthesizes and packages the current HLS core
# *     as an IP.
# * 
# * Synopsis:
# *   make clean   : Cleanup of the current directory.
# *   make         : Runs the default build for this core.
# *
# ******************************************************************************

# Name of the HLS core to build
ipName = iprx_handler

# Dependencies
SRC_DEPS := $(shell find ./src/ -type f)

.PHONY: all

all: ${ipName}_prj

${ipName}_prj: $(SRC_DEPS)
	$(MAKE) clean
	vivado_hls -f run_hls.tcl
	@touch $@

clean:
	${RM} -rf ${ipName}_prj
	${RM} vivado*.log

# A little make receipt to print a variable (usage example: make print-SRC_DEPS)
print-%: ; @echo $* = $($*)
