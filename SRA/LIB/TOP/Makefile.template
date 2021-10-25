# /*******************************************************************************
#  * Copyright 2016 -- 2020 IBM Corporation
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
#  *       Makefile to build a cloudFPGA-project (cFp)
#  *
#  *     Synopsis:
#  *       make [target]  (use 'make help' to list the targets)
#  *

# NO HEADING OR TRAILING SPACES!!
SHELL_DIR =$(cFpRootDir)/cFDK/SRA/LIB/SHELL/$(cFpSRAtype)/
ROLE_DIR =$(usedRoleDir)
ROLE2_DIR =$(usedRole2Dir)

CLEAN_TYPES = *.log *.jou *.str *.time


.PHONY: all clean pr Role Role2 RoleIp RoleIp2 ShellSrc pr_full pr2 monolithic ensureNotMonolithic full_clean ensureMonolithic monolithic_incr save_mono_incr pr_verify assert_env
.PHONY: pr_debug pr_full_debug pr_only pr2_only ensureDebugNets monolithic_debug monolithic_incr_debug monolithic_proj help

all: pr


# assert ENVIRONMENT
assert_env: 
	./cFDK/SRA/LIB/bash/assert_envs.sh

# next line until including (end of cFa placeholder) MUST STAY UNCHANGED
#cFa addtional targets


# (end of cFa placeholder)

# --- Middleware ---
MIDLW_DIR =$(cFpRootDir)/cFDK/SRA/LIB/MIDLW/$(cFpSRAtype)/ #TODO
.PHONY: MidlwSrc MidlwPr MidlwDummy MidlwSrcTrue MidlwPrTrue

# conditional switch
ifdef cFpMidlwIpDir
MidlwSrc: MidlwSrcTrue

MidlwPr: MidlwPrTrue

else 
MidlwSrc: MidlwDummy

MidlwPr: MidlwDummy
endif


MidlwDummy:
	@echo "No Middleware configured"

MidlwSrcTrue: assert_env
	$(MAKE) -C $(MIDLW_DIR)

MidlwPrTrue: assert_env
	$(MAKE) -C $(MIDLW_DIR) MIDLW_$(cFpSRAtype)_OOC.dcp


# --- default Makefile ---

Role: assert_env
	$(MAKE) -C $(ROLE_DIR)

Role2: assert_env
	$(MAKE) -C $(ROLE2_DIR)

xpr: 
	mkdir -p $(cFpXprDir)

RoleIp: assert_env
	$(MAKE) -C $(ROLE_DIR)/ ip

RoleIp2: assert_env
	$(MAKE) -C $(ROLE2_DIR)/ ip


ShellSrc: assert_env
	$(MAKE) -C $(SHELL_DIR) 

pr: ensureNotMonolithic ShellSrc MidlwPr Role  | xpr  ## Builds Shell (if necessary) and first Role only using PR flow (default)
	$(MAKE) -C ./TOP/tcl/ full_src_pr

pr2: ensureNotMonolithic ShellSrc MidlwPr Role2 | xpr ## Builds Shell (if necessary) and second Role only using PR flow
	$(MAKE) -C ./TOP/tcl/ full_src_pr_2

pr_full: ensureNotMonolithic ShellSrc MidlwPr Role Role2 | xpr ## Builds Shell (if necessary) and both Roles using PR flow
	$(MAKE) -C ./TOP/tcl/ full_src_pr_all

pr_flash: ensureNotMonolithic ShellSrc MidlwPr Role  | xpr  ## Builds Shell (if necessary) and first Role using PR flow and generate flash file
	$(MAKE) -C ./TOP/tcl/ full_src_pr_flash

#pr_debug: ensureNotMonolithic ensureDebugNets ShellSrc Role  | xpr  # Builds Shell (if necessary) and first Role only using PR flow including debug probes for the Shell
#	$(MAKE) -C ./TOP/tcl/ full_src_pr_debug
pr_debug: ## Builds Shell (if necessary) and first Role only using PR flow including debug probes for the Shell (currently disabled)
	@/bin/echo -e "Debuging within a PR flow is currently only possible using the Vivado GUI.\nPlease refer also to UG947 - Lab 4.\n"
	@exit 1

#pr_full_debug: ensureNotMonolithic ensureDebugNets ShellSrc Role Role2 | xpr # Builds Shell (if necessary) and both Roles using PR flow including debug probes for the Shell
#	$(MAKE) -C ./TOP/tcl/ full_src_pr_all_debug
pr_full_debug: ## Builds Shell (if necessary) and both Roles using PR flow including debug probes for the Shell (currently disabled)
	@/bin/echo -e "Debuging within a PR flow is currently only possible using the Vivado GUI.\nPlease refer also to UG947 - Lab 4.\n"
	@exit 1

#pr_incr: ensureNotMonolithic ShellSrc Role  | xpr
#	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./TOP/tcl/ full_src_pr_incr

# incremental compile seems not to work with second run
#pr2_incr: ensureNotMonolithic ShellSrc Role2 | xpr
#	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./TOP/tcl/ full_src_pr_2_incr

#pr_full_incr: ensureNotMonolithic ShellSrc Role Role2 | xpr
#	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./TOP/tcl/ full_src_pr_all_incr

pr_only: ensureNotMonolithic Role  | xpr ## Building partial bitifle for Role 1 (if Shell.dcp is present)
	$(MAKE) -C ./TOP/tcl/ full_src_pr_only

pr2_only: ensureNotMonolithic Role2 | xpr ## Building partial bitifle for Role 2 (if Shell.dcp is present)
	$(MAKE) -C ./TOP/tcl/ full_src_pr_2_only

#pr_incr_only: ensureNotMonolithic Role  | xpr
#	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./TOP/tcl/ full_src_pr_incr_only

# incremental compile seems not to work with second run
#pr2_incr_only: ensureNotMonolithic Role2 | xpr
#	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./TOP/tcl/ full_src_pr_2_incr_only

#no ROLE, because Role is synthezied with sources!
monolithic: ensureMonolithic ShellSrc MidlwSrc RoleIp | xpr  ## Default build when PR is not used.
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	@#export usedRole=$(USED_ROLE); cd tcl; vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -forceWithoutBB -role -create -synth -impl -bitgen
	$(MAKE) -C ./TOP/tcl/ monolithic

#no ROLE, because Role is synthezied with sources!
monolithic_incr: ensureMonolithic ShellSrc MidlwSrc RoleIp | xpr  ## Default incremental build.
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	$(MAKE) -C ./TOP/tcl/ monolithic_incr 

monolithic_debug: ensureMonolithic ensureDebugNets ShellSrc MidlwSrc RoleIp | xpr   ## Default build to include a debug probe.
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	$(MAKE) -C ./TOP/tcl/ monolithic_debug

monolithic_incr_debug: ensureMonolithic ensureDebugNets ShellSrc MidlwSrc RoleIp | xpr   ## Default incremental build to include a debug probe..
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	$(MAKE) -C ./TOP/tcl/ monolithic_incr_debug

monolithic_proj: ensureMonolithic | xpr ## Only creates a new Vivado project (only works after a clean; does not take dependencies into account)
	$(MAKE) -C ./TOP/tcl/ monolithic_proj

save_mono_incr: ensureMonolithic  ## Saves the current monolithic for use in further incremental builds.
	export usedRole=$(USED_ROLE); $(MAKE) -C ./TOP/tcl/ save_mono_incr

#save_pr_incr: 
#	$(error THIS IS DONE AUTOMATICALLY DURING THE FLOW)

pr_verify: assert_env
	$(MAKE) -C ./TOP/tcl/ pr_verify


ensureNotMonolithic: assert_env | xpr 
	@test ! -f ./xpr/.project_monolithic.lock || (cat ./xpr/.project_monolithic.lock && exit 1)

ensureMonolithic: assert_env
	@test  -f ./xpr/.project_monolithic.lock || test ! -d ./xpr/ || (echo "This project was startet with Black Box flow => please clean up first" && exit 1)

ensureDebugNets: assert_env
	@test -f ./TOP/xdc/.DEBUG_SWITCH || /bin/echo -e "This file is a guard for handle_vivado.tcl and should prevent the failure of a build process\nbecuase the make target monolithic_debug is called but the debug nets aren't defined in\n<Top>/TOP/xdc/debug.xdc .\nThis must be done once manually in the Gui and then write "YES" in the last line of this file.\n(It must stay the last line...)\n\nHave you defined the debug nets etc. in <Top>/xdc/debug.xdc?\nNO" > ./TOP/xdc/.DEBUG_SWITCH
	@test `tail -n 1 ./TOP/xdc/.DEBUG_SWITCH` = YES || (echo "Please define debug nets in ./TOP/xdc/debug.xdc and answer the question in ./TOP/xdc/.DEBUG_SWITCH" && exit 1)


clean: ## Cleans the current cFp project (.i.e this TOP)
	$(MAKE) -C ./TOP/tcl/ clean 
	rm -rf $(CLEAN_TYPES)
	rm -rf ./xpr/ ./hd_visual/
	rm -rf ./dcps/
	# rm -rf ./xdc/.DEBUG_SWITCH

full_clean: clean ## Cleans the entire cFp structure (.i.e TOP, SHELL and ROLE).
	$(MAKE) -C $(SHELL_DIR) clean 
	$(MAKE) -C $(ROLE_DIR) clean 
	$(MAKE) -C $(ROLE2_DIR) clean
	rm -rf ./TOP/xdc/.DEBUG_SWITCH


print-%: ## A little make receipt to print a variable (usage example --> 'make print-SHELL_DIR')
	@echo $* = $($*)

help: ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@cat ${MAKEFILE_LIST}  | egrep '^(.+)\:(.+)##\ (.+)' |  column -s ':#' |  sed 's/:.*#/||/' |  column -s '||' -t | sed -e 's/^/    /' 
	@echo
