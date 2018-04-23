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

.PHONY: all clean src_based ip_based

all: src_based #OR ip_based, whatever is preferred as default

src_based:
	$(MAKE) -C $(SHELL_DIR) full_src
	$(MAKE) -C ./tcl/ full_src


ip_based: 
	$(error NOT YET IMPLEMENTED)


clean: 
	$(MAKE) -C ./tcl/ clean


