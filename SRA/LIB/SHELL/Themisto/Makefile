#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile for a SPECIFIC SHELL
#  *

ifndef cFpIpDir
	echo "ERROR: cFpIpDir MUST BE DEFINED as environment! variable"
	exit 1
else
IP_DIR = $(cFpIpDir)
endif


#SEEMS to be obsolete (was dependency for `ip`)
#XCI_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)
#we need both types of directories to detect all possible reasons to re-run create_ip_cores.tcl
#XCI_DEPS := $(shell find ../LIB/hls/*/*_prj/solution1/impl/ip ./hls/*/src/ -maxdepth 0 -type d)


.PHONY: all hls_cores clean ip 


all: ip


hls_cores: 
	@$(MAKE) -C ../LIB/hls/ 

../LIB/hls: hls_cores

ip: $(IP_DIR)

# directory as dependency: not the best solution but here the most practical one 
# We need ./.ip_guard to be touched by the hls ip core makefiles, because XCI_DEPS doesn't work. 
# XCI_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist completly (e.g. after a clean)
# the create_ip_cores.tcl will not be started. 
# We must depend on a directory NOT a PHONY in order to avoid a re-build every time.
$(IP_DIR): ../LIB/hls ./tcl/create_ip_cores.tcl ../LIB/tcl/create_ip_cores.tcl $(IP_DIR)/ip_user_files ../LIB/.ip_guard
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 


../LIB/.ip_guard: 
	@touch $@


# Create IP directory if it does not exist
$(IP_DIR)/ip_user_files:
	mkdir -p $@


clean: 
	rm -rf $(IP_DIR)
	$(MAKE) -C ../LIB clean

