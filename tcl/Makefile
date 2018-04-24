
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Francois Abel & Burkhard Ringlein 
# *
# * Description : A makefile to create a Vivado XPR project for the TopFlash 
# *               and to run the synthesis and implemenation steps.
# *
# * Usage:
# *  make	: Create, synthesize and implement the "topFMKU_Flash" design.
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
	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -clean


$(XPR_PROJ): $(IP_SHELL)
	rm -f run_project.log
	rm -f vivado.jou
	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -force 


full_src: 
	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -full_src -force


