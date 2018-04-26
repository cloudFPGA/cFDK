
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Francois Abel
# *
# * Description : A makefile that creates a Vivado XPR project for the TopFlash.
# *
# * Usage:
# *  make		: Creates a new "topFMKU_Flash.xpr" project.
# *  make clean : Removes the previous XPR directory and starts a new project.  
# *
# ******************************************************************************


IP_SHELL = ../../../IP/Shell/component.xml

XPR_PROJ = ../xpr/topFMKU60_Flash.xpr

BIT_FILE = ../xpr/topFMKU60_Flash.runs/impl_1/topFlash.bit


.PHONY: all clean full_src full_src_pr
all: $(XPR_PROJ)


clean:
	rm -f  ./*.log
	rm -f  ./*.jou


$(XPR_PROJ): $(IP_SHELL)
	rm -f create_project.log
	rm -f vivado.jou
	vivado -mode batch -source create_project.tcl -notrace -log create_project.log -tclargs -force 


full_src: 
	vivado -mode batch -source create_project.tcl -notrace -log create_project.log -tclargs -full_src -force

full_src_pr:
	vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -full_src -force -role

