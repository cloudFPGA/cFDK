# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie
# *         this ROLE.
# *
# * Synopsis:
# *     make         : Runs the default build for this ROLE.
# *     make clean   : Makes a cleanup of the current directory.
# *     make project : Creates a toplevel project of this ROLE for Vivado.
# *
# ******************************************************************************


HLS_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)

#OBSOLETE HLS_DEPS := $(shell find ./hls/*/*_prj -maxdepth 0 -type d)

.PHONY: all clean hls_cores project


all: Role_x1Udp_x1Tcp_x2Mp_OOC.dcp

Role_x1Udp_x1Tcp_x2Mp_OOC.dcp: ./hdl ./ip ./xdc ./tcl
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force

hls_cores:
	@$(MAKE) -C ./hls/

hls: hls_cores

# We need ./.ip_guard to be touched by the hls ip core makefiles, because HLS_DEPS doesn't work. i
# HLS_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist completly (e.g. after a clean)
# the create_ip_cores.tcl will not be started. 
# TODO: $(HLS_DEPS) obsolete?
ip: hls ./tcl/create_ip_cores.tcl $(HLS_DEPS) ./ip/ip_user_files ./.ip_guard
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 
	@echo ------- DONE ------------------------------------- 

.ip_guard: 
	@touch $@

# Create IP directory if it does not exist
./ip/ip_user_files:
	@echo -- Creating ip/ip_user_files directory -----------
	mkdir -p $@


clean:
	rm -rf ./ip ./xpr
	$(MAKE) -C ./hls clean
	rm -rf ./xpr/ Role*_OOC.dcp


project:
	cd ./tcl/; vivado -mode batch -source handle_vivado.tcl -notrace -log create_project.log -tclargs -create

# A little make receipt to print a variable (usage example: make print-HLS_DEPS)
print-%: ; @echo $* = $($*)
