# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
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


.PHONY: all hls_cores clean ip enc_ips


all: ip


hls_cores: 
	@$(MAKE) -C ../LIB/hls/

../LIB/hls: hls_cores

ip: $(IP_DIR)

# directory as dependency: not the best solution but here the most practical one 
# We need ./.ip_guard to be touched by the hls ip core makefiles, because XCI_DEPS doesn't work. 
# Dependencies as environment variables are evaluated BEFORE the hls target get's executed, 
# so if a hls core doesn't exist completly (e.g. after a clean) the create_ip_cores.tcl will not be started. 
# We must depend on a directory NOT a PHONY in order to avoid a re-build every time.
$(IP_DIR): ../LIB/hls ./tcl/create_ip_cores.tcl ./tcl/decouple_ip_type.tcl ../LIB/tcl/create_ip_cores.tcl $(IP_DIR)/ip_user_files ../LIB/.ip_guard
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 
	@touch $@


../LIB/.ip_guard: 
	@touch $@


# Create IP directory if it does not exist
$(IP_DIR)/ip_user_files:
	mkdir -p $@

enc_ips: $(IP_DIR)/ip_user_files
	cd ./tcl/ ; vivado -mode batch -source create_only_enc_ip_cores.tcl -notrace -log create_only_enc_ip_cores.log 


clean: 
	rm -rf $(IP_DIR)
	$(MAKE) -C ../LIB clean


