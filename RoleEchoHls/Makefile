# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : June 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie 
# *		this specific ROLE.
# * 
# * Synopsis:
# *   make         : Runs the default build for this ROLE.
# *   make clean   : Makes a cleanup of the current directory.
# *	  make project :  Creates a toplevel project for Vivado and stops.
# *   
# ******************************************************************************

.PHONY: all clean project

all: RoleEchoHls 

RoleEchoHls:
	@$(MAKE) -C ./hls/

clean: 
	rm -rf ./xpr/

project:
	cd ./tcl/; vivado -mode batch -source handle_vivado.tcl -notrace -log create_project.log -tclargs -create
