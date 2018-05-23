#  **ABOUT: _~/FMKU60/Shell_**

## Overview

This directory contains all the files for generating the a version of the  **SHELL** with two TCP ports, two UDP ports and two Memory Channels. This shell is then referred to as 'Shell_x2Udp_x2Tcp_x2Mc'. 

## About: _~/FMKU60/Shell/directories_

        1. **doc** is for documentation about the current SHELL.  

        2. **hls** contains the Vivado High-Level Synthesis (HLS) code that implements the network-transport-session (NTS) part of the SHELL.
    
        3. **hdl** contains the code in Hardware Description Language (Vhdl and Verilog) used by the SHELL.

        4. **tcl** contains scripts for creating the Vivado project, the Vivado-based IPs and the user defined IPs.

        5. **xdc** folder contains the physical and timing constraints files for synthesis, plaeement and routing of the SHELL.

## HOWTO: 

Currently the SHELL is instantiated by a TOP with its complete sources, so the Makefile in TOP knows best, what to run. 

Use the following commands in case you need to run a local standalone synthesis of the SHELL: 

    * **make full_src**
	Bilds anything that is necesarry to import the SHELL in TOP. 





