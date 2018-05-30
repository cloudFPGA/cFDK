
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Francois Abel
# *
# * Description : A makefile that creates a Vivado XPR project for the Shell.
# *
# * Usage:
# *  make		: Creates, syhthesizes and packages a new "Shell" IP core.
# *  make clean : Removes the previous XPR directory and starts from scratch.  
# *
# ******************************************************************************


IP_SHELL = ../../../../IP/FMKU60/Shell/component.xml

HDL_FILES = [TODO]
XDC_FILES = [TODO]


all: $(IP_SHELL)


clean:
	rm -f  ./*.log
	rm -f  ./*.jou
	rm -rf ../xpr
#	vivado -mode batch -source create_project.tcl -notrace -log create_project.log -tclargs -clean


$(IP_SHELL): $(HDL_FILES) $(XDC_FILES)
	@echo '**** WARNING: his makefile does not yet work yet ! ****'
	exit

	rm -f *.log
	rm -f *.jou
	### STEP-1: Synthesisze the HLS-based IP cores.
	../hls
	make
	cd -
	### STEP-2: Create the IP cores used by the SHELL.
	vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log
	### STEP-3: Create a new Vivado project for the SHELL.
	vivado -mode batch -source create_project.tcl -notrace -log create_project.log  
	### STEP-4: [Optional] Synthesize the IP cores in OOC mode.
	vivado -mode batch -source synth_ip_cores.tcl -notrace -log synth_ip_cores.log
	### STEP-5: Package the project as a new IP.
	vivado -mode batch -source package_project.tcl -notrace -log package_project.log


