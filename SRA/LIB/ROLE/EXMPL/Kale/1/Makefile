# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein, Francois Abel
# *
# * Description : A makefile that invokes all necesarry steps to synthesize
# *         this ROLE.
# *
# * Synopsis:
# *     make         : Runs the default build for this ROLE.
# *     make clean   : Makes a cleanup of the current directory.
# *     make project : Creates a toplevel project for this ROLE for Vivado.
# *
# ******************************************************************************

HLS_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)

CLEAN_TYPES = *.log *.jou *.str *.time


.PHONY: all clean hls_cores assert_env help project


#OBSOLETE all: Role_x1Udp_x1Tcp_x2Mp_OOC.dcp
all: assert_env Role_$(cFpSRAtype)_OOC.dcp

assert_env:
	@../cFDK/SRA/LIB/bash/assert_envs.sh

#OBSOLETE Role_x1Udp_x1Tcp_x2Mp_OOC.dcp: ./hdl ./ip ./xdc ./tcl
#OBSOLETE 	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force

Role_$(cFpSRAtype)_OOC.dcp: ./hdl ./ip ./tcl  ## Builds the Out-Of-Contect (OOC) Design-Checkpoint (dcp) for this Role (default)
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force

hls_cores:
	@$(MAKE) -C ./hls/

hls: assert_env hls_cores

# We need ./.ip_guard to be touched by the hls ip core makefiles, because HLS_DEPS doesn't work.
# HLS_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist
#  completly (e.g. after a clean) the create_ip_cores.tcl will not be started. 
# TODO: $(HLS_DEPS) obsolete?
ip: hls ./tcl/create_ip_cores.tcl $(HLS_DEPS) ./ip/ip_user_files ./.ip_guard  ## Export ROLE HLS cores to Vivado IP direcotry
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 
	@echo ------- DONE ------------------------------------- 

.ip_guard: 
	@touch $@

# Create IP directory if it does not exist
./ip/ip_user_files:
	@echo -- Creating ip/ip_user_files directory -----------
	mkdir -p $@


#-- Targets --------------------------------------------------------------------

clean: ## Cleans the current project
	rm -rf ./ip ./xpr
	rm -rf $(CLEAN_TYPES)
	rm -rf ./tcl/$(CLEAN_TYPES)
	$(MAKE) -C ./hls clean
	rm -rf ./xpr/
	rm -rf Role*_OOC.dcp

project: ## Creates a toplevel project for this ROLE
	cd ./tcl/; vivado -mode batch -source handle_vivado.tcl -notrace -log create_project.log -tclargs -create

print-%: ## A little make receipt to print a variable (usage example --> 'make print-HLS_DEPS')
	@echo $* = $($*)

help:    ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | sed -e 's/^/    /' 
	@echo
