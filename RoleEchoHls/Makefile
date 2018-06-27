# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : June 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie 
# *		this specific ROLE.
# * 
# * Synopsis (and typical flow):
# *   make clean   : Makes a cleanup of the current directory.
# *   make         : Runs the default build for this ROLE.
# *   make project : Creates a toplevel project of this ROLE for Vivado.
# *
# ******************************************************************************

HLS_DEPS := $(shell find ./hls/*_prj -maxdepth 0 -type d)

.PHONY: all clean hls_cores project

all: RoleEchoHls 

hls_cores:
	@$(MAKE) -C ./hls/

hls: hls_cores

ip: hls ./tcl/create_ip_cores.tcl $(HLS_DEPS) ./ip/ip_user_files
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 

# Create IP directory if it does not exist
./ip/ip_user_files:
	mkdir -p $@

RoleEchoHls: ip

clean: 
	rm -rf ./xpr ./ip
	$(MAKE) -C ./hls clean

project:
	cd ./tcl/; vivado -mode batch -source handle_vivado.tcl -notrace -log create_project.log -tclargs -create

# A little make receipt to print a variable (usage example: make print-HLS_DEPS)
print-%: ; @echo $* = $($*)
