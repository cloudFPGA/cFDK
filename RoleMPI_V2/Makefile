# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie *this* ROLE
# *
# ******************************************************************************

.PHONY: all clean

all: Role_MPIv0_x2Mp_OOC.dcp

Role_MPIv0_x2Mp_OOC.dcp: ./hdl ./xdc ./tcl 
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force


clean: 
	rm -rf ./xpr/ Role*_OOC.dcp


