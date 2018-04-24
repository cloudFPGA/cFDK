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

SHELL_DIR = ../../SHELL/Shell 
ROLE_DIR = ../../ROLE

.PHONY: all clean src_based ip_based RoleFlash pr Role

all: src_based
#OR ip_based, whatever is preferred as default

Role: RoleFlash

RoleFlash: 
	$(MAKE) -C $(ROLE_DIR)/$@

src_based:
	$(MAKE) -C $(SHELL_DIR) full_src
	$(MAKE) -C ./tcl/ full_src

pr: Role
	$(MAKE) -C ./tcl/ full_src_pr

ip_based: 
	$(error NOT YET IMPLEMENTED)


clean: 
	$(MAKE) -C ./tcl/ clean
	rm -rf ./xpr/ 


