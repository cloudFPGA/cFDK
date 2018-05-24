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

.PHONY: all clean src_based ip_based RoleFlash pr Role ShellSrc pr_full

all: pr
#all: src_based
#OR ip_based, whatever is preferred as default

Role: $(USED_ROLE)

Role2: $(USED_ROLE_2)

RoleFlash:
	$(MAKE) -C $(ROLE_DIR)/$@

RoleFlash_V2:
	$(MAKE) -C $(ROLE_DIR)/$@

ShellSrc:
	$(MAKE) -C $(SHELL_DIR) full_src

src_based: ShellSrc Role
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src

pr: ShellSrc Role 
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr

pr2: ShellSrc Role2
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_2

pr_full: ShellSrc Role Role2
	export usedRole=$(USED_ROLE); export usedRole2=$(USED_ROLE_2); $(MAKE) -C ./tcl/ full_src_pr_all

ip_based: 
	$(error NOT YET IMPLEMENTED)


clean: 
	$(MAKE) -C ./tcl/ clean
	rm -rf ./xpr/ 
	#TODO discuss if delete dcps
	rm -rf ./dcps/

