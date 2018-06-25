# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie a TOP
# *
# ******************************************************************************

# NO HEADING OR TRAILING SPACES!!
SHELL_DIR =../../SHELL/Shell_x1Udp_x1Tcp_x2Mp_x2Mc
ROLE_DIR =../../ROLE
USED_ROLE =RoleFlash
USED_ROLE_2 =RoleFlash_V2

CLEAN_TYPES = *.log *.jou *.str *.time


.PHONY: all clean src_based ip_based RoleFlash pr Role ShellSrc pr_full pr2 monolithic ensureNotMonolithic full_clean ensureMonolithic monolithic_incr save_mono_incr save_pr_incr pr_verify

all: pr
#all: src_based
#OR ip_based, whatever is preferred as default

Role: $(USED_ROLE)

Role2: $(USED_ROLE_2)


xpr: 
	mkdir -p ./xpr/ 

RoleFlash:
	$(MAKE) -C $(ROLE_DIR)/$@

RoleFlash_V2:
	$(MAKE) -C $(ROLE_DIR)/$@

ShellSrc:
	$(MAKE) -C $(SHELL_DIR) full_src

src_based: ensureNotMonolithic ShellSrc Role | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src

pr: ensureNotMonolithic ShellSrc Role  | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr

pr2: ensureNotMonolithic ShellSrc Role2 | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_2

pr_full: ensureNotMonolithic ShellSrc Role Role2 | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_all

pr_incr: ensureNotMonolithic ShellSrc Role  | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_incr

pr2_incr: ensureNotMonolithic ShellSrc Role2 | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_2_incr


pr_only: ensureNotMonolithic Role  | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_only

pr2_only: ensureNotMonolithic Role2 | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_2_only

pr_incr_only: ensureNotMonolithic Role  | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_incr_only

pr2_incr_only: ensureNotMonolithic Role2 | xpr
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_2_incr_only


ip_based: 
	$(error NOT YET IMPLEMENTED)

#no ROLE, because Role is synthezied with sources!
monolithic: ensureMonolithic ShellSrc | xpr 
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	@#export usedRole=$(USED_ROLE); cd tcl; vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -forceWithoutBB -role -create -synth -impl -bitgen
	export usedRole=$(USED_ROLE); $(MAKE) -C ./tcl/ monolithic

#no ROLE, because Role is synthezied with sources!
monolithic_incr: ensureMonolithic ShellSrc | xpr 
	@echo "this project was startet without Black Box flow => until you clean up, there is no other flow possible" > ./xpr/.project_monolithic.lock
	export usedRole=$(USED_ROLE); $(MAKE) -C ./tcl/ monolithic_incr 

save_mono_incr: ensureMonolithic 
	export usedRole=$(USED_ROLE); $(MAKE) -C ./tcl/ save_mono_incr

save_pr_incr: 
	$(error THIS IS DONE AUTOMATICALLY DURING THE FLOW)

pr_verify: 
	$(MAKE) -C ./tcl/ pr_verify


ensureNotMonolithic: | xpr 
	@test ! -f ./xpr/.project_monolithic.lock || (cat ./xpr/.project_monolithic.lock && exit 1)

ensureMonolithic:
	@test  -f ./xpr/.project_monolithic.lock || test ! -d ./xpr/ || (echo "This project was startet with Black Box flow => please clean up first" && exit 1)

clean: 
	$(MAKE) -C ./tcl/ clean 
	rm -rf $(CLEAN_TYPES)
	rm -rf ./xpr/ ./hd_visual/
	rm -rf ./dcps/


full_clean: clean 
	$(MAKE) -C $(SHELL_DIR) clean 
	$(MAKE) -C $(ROLE_DIR)/$(USED_ROLE) clean 
	$(MAKE) -C $(ROLE_DIR)/$(USED_ROLE_2) clean



