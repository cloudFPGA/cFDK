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
SHELL_DIR =../../SHELL/Shell
ROLE_DIR =../../ROLE
USED_ROLE =RoleFlash

.PHONY: all clean src_based ip_based RoleFlash pr Role ShellSrc

all: pr
#all: src_based
#OR ip_based, whatever is preferred as default

Role: $(USED_ROLE)

RoleFlash:
	$(MAKE) -C $(ROLE_DIR)/$@
	
ShellSrc:
	$(MAKE) -C $(SHELL_DIR) full_src

src_based: ShellSrc
	$(MAKE) -C ./tcl/ full_src

pr: ShellSrc Role
	export usedRole=$(USED_ROLE); $(MAKE) -C ./tcl/ full_src_pr

ip_based: 
	$(error NOT YET IMPLEMENTED)


clean: 
	$(MAKE) -C ./tcl/ clean
	rm -rf ./xpr/ 


