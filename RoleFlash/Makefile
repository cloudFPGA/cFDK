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

all: Role_OOC.dcp

Role_OOC.dcp: ./hdl/ ./xdc/ ./tcl/ 
	cd ./tcl/; vivado -mode batch -source run.tcl -notrace -log run.log -tclargs -force


clean: 
	rm -rf ./xpr/ Role_OOC.dcp


