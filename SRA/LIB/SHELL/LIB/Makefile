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

#XCI_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)

#we need both types of directories to detect all possible reasons to re-run create_ip_cores.tcl
XCI_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip ./hls/*/src/ -maxdepth 0 -type d)


.PHONY: all hls_cores clean full_src 


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
# We need ./.ip_guard to be touched by the hls ip core makefiles, because XCI_DEPS doesn't work. i
# HLS_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist completly (e.g. after a clean)
# the create_ip_cores.tcl will not be started. 
# TODO: $(XCI_DEPS) obsolete?
ip: hls ./tcl/create_ip_cores.tcl $(XCI_DEPS) ./ip/ip_user_files ./.ip_guard
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 

#ip_mpi: hls ./tcl/create_ip_cores.tcl $(XCI_DEPS) ./ip/ip_user_files ./.ip_guard
#	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log -tclargs -useMPI
#	@touch $@

.ip_guard: 
	@touch $@


# Create IP directory if it does not exist
./ip/ip_user_files:
	mkdir -p $@


clean: 
	rm -rf ./ip
	$(MAKE) -C ./hls clean


