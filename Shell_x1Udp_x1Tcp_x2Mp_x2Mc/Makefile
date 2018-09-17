# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein, Francois Abel
# *
# * Description : A makefile to build and synthezie the SHELL.
# *
# * Modifications:
# *  Fab-Jun06 - Added dependency 'ip_user_files' for target 'ip'.
# * 
# ******************************************************************************

XCI_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)

.PHONY: all hls_cores clean full_src shell_synth pkg_shell 

# [TODO-Flow for pkg_shell]

all: full_src

full_src: ip 

full_src_mpi: ip_mpi

hls_cores: 
	@$(MAKE) -C ./hls/ 

# INFO: hls itself must not be Phony (otherwise dependend targets are rebuild every time);
# But hls should check dependencis recursive every time, so it must depend on a Phony 
# target itself
hls: hls_cores

# directory as dependency: not the best solution but here the most practical one 
# INFO: hls must be before $(XCI_DEPS)! otherwise make does not see the updated *_prj 
ip: hls ./tcl/create_ip_cores.tcl $(XCI_DEPS) ./ip/ip_user_files
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 

ip_mpi: hls ./tcl/create_ip_cores.tcl $(XCI_DEPS) ./ip/ip_user_files
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log -tclargs -useMPI
	@touch $@

# Create IP directory if it does not exist
./ip/ip_user_files:
	mkdir -p $@

xpr: ip
	cd ./tcl/ ; vivado -mode batch -source create_project.tcl -notrace -log create_project.log -tclargs -force

shell_synth: xpr # NOT TESTED
	@#cd ./tcl/ ;  vivado -mode batch -source synth_ip_cores.tcl -notrace -log synth_ip_cores.log
	$(error NOT YET IMPLEMENTED)

pkg_shell: xpr shell_synth # NOT TESTED
	@#cd ./tcl/ ; vivado -mode batch -source package_project.tcl -notrace
	$(error NOT YET IMPLEMENTED)


clean: 
	rm -rf ./ip ./xpr 
	$(MAKE) -C ./hls clean


