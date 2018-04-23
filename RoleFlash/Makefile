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

Role_OOC.dcp:
	cd ./tcl/; vivado -mode batch -source create_project.tcl -notrace -log create_project.log


clean: 
	rm -rf ./xpr/ Role_OOC.dcp


