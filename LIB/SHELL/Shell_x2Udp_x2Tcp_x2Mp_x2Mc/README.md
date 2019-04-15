#  **ABOUT: ~/FMKU60/SHELL/Shell\_x2Udp\_x2Tcp\_x2Mc**

## Overview

This directory contains all the files for generating the a version of the  **SHELL** with two TCP ports, two UDP ports and two Memory Channels. This shell is then referred to as 'Shell_x2Udp_x2Tcp_x2Mc'.

## Directories

        - doc is for documentation about the current SHELL.  

        - hls  contains the Vivado High-Level Synthesis (HLS) code that implements the network-transport-session (NTS) part of the SHELL.
    
        - hdl contains the code in Hardware Description Language (Vhdl and Verilog) used by the SHELL.
    
        - tcl contains scripts for creating the Vivado project, the Vivado-based IPs and the user defined IPs.

        - xdc folder contains the physical and timing constraints files for synthesis, plaeement and routing of the SHELL.

## HOW-TO

Currently the SHELL is instantiated by a TOP with its complete sources, so the Makefile in TOP knows best, what to run. 

However, use the following commands if you need to run a local standalone synthesis of the SHELL: 

        - make clean
            to start a with a clean new environment.
            
        - make full_src
            to build anything that is necesarry to import the SHELL in TOP. 





