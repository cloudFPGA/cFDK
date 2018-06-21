# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie 
# *		this specific ROLE.
# * 
# * Synopsis:
# *     make            : Runs the default build for this ROLE.
# *     make clean      : Makes a cleanup of the current directory.
# *	make project	: Creates a toplevel project for Vivado and stops.
# *   
# ******************************************************************************

.PHONY: all clean project

#all: Role_Udp_Tcp_McDp_4BEmif_OOC.dcp
all: Role_x1Udp_x1Tcp_x2Mp_OOC.dcp

#Role_Udp_Tcp_McDp_4BEmif_OOC.dcp: ./hdl ./xdc ./tcl 
Role_x1Udp_x1Tcp_x2Mp_OOC.dcp: ./hdl ./xdc ./tcl 
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force


clean: 
	rm -rf ./xpr/ Role*_OOC.dcp


project:
	cd ./tcl/; vivado -mode batch -source handle_vivado.tcl -notrace -log create_project.log -tclargs -create
