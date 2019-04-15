#  **ABOUT: _~/FMKU60/Shell/tcl_**

## **Overview**

This directory contains the Tcl scripts for invoking the Vivado tools in batch mode.
Use these scripts to create a TCL-based project design flow of the current SHELL. 

##########################################################
##  **HOW TO CREATE THE SHELL IN VIVADO TCL PROJECT MODE**  ##
##########################################################

### STEP-1: Synthesisze the HLS-based IP cores.

        -> cd ../hls
        -> make
    
### STEP-2: Create the IP cores used by the SHELL.

        -> cd . 
        -> vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log
        
### STEP-3: Create a new Vivado project for the SHELL.

        -> cd . 
        -> vivado -mode batch -source create_project.tcl -notrace -log create_project.log  

### STEP-4: [Optional] Synthesize the IP cores in OOC mode.

        -> cd . 
        -> vivado -mode batch -source synth_ip_cores.tcl -notrace -log synth_ip_cores.log
           
           -> Or alternativelly, open the project with Vivado IDE
              and start the synthesis of the IP cores from there.         

### STEP-5: Package the project as a new IP.

        -> cd .
        -> vivado -mode batch -source package_project.tcl -notrace -log package_project.log

           -> Or alternativelly, open the project with Vivado IDE
              and execute Tool->Create and package new IP from there.
