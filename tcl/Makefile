
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


.PHONY: all clean full_src
all: $(XPR_PROJ)


clean:
	rm -f  ./*.log
	rm -f  ./*.jou


$(XPR_PROJ): $(IP_SHELL)
	rm -f run.log
	rm -f vivado.jou
	vivado -mode batch -source run.tcl -notrace -log run.log -tclargs -force 


full_src: 
	vivado -mode batch -source run.tcl -notrace -log run.log -tclargs -full_src -force


