#  **ABOUT: _~/FMKU60/RoleFlash/tcl_**

## **Overview**

This directory contains the Tcl scripts for invoking the Vivado tools in batch mode.
Use these scripts to create a TCL-based project design flow of the current ROLE. 

## **HOW TO CREATE THE ROLE VIVADO TCL PROJECT MODE**

### STEP-1: Create a new Vivado project for the ROLE.

        -> cd . 
        -> vivado -mode batch -source create_project.tcl -notrace        

### STEP-2: Synthesize the projet in OOC mode.

        -> cd . 
        -> vivado -mode batch -source [TODO] -notrace
           
           -> Or alternativelly, open the project with Vivado IDE
              and start the synthesis from there.         

### STEP-3: [TODO] Simulate and test...

### STEP-4: [TODO] Assemble a SHELL and your ROLE into a TOP design.

