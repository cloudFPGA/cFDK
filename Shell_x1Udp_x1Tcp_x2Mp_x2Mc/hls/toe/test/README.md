# cloudFPGA / SHELL / hls / toe / test

**This directory is dedicated to the testbench of the TCP Offload Engine (TOE).** 
 
## How To Run a HLS C Simulation
- Option-1: The simplest way it to run all the tests at once. To do that, you start from the upper directory (*cd ..*) and you run the following command: **make csim**.
- Option-2: Run a single test from the Vivado HLS IDE. To do that, open the menu **Project->Project Settings->Simulations** and add the arguments to be used by the testbench called 'test_toe.cpp'.
    - As an exemple, the following two input arguments *'1 ../../../../test/testVectors/appRx_OneSeg.dat'* will run the testbench in TX_MODE (1) while using the test vector file 'appRx_OneSeg.dat'. Refer to '../run_hls.tcl' for more exemples.
  

![Structure of the testbench](https://github.ibm.com/cloudFPGA/SRA/blob/fab_nts/FMKU60/SHELL/Shell_x1Udp_x1Tcp_x2Mp_x2Mc/hls/toe/test/images/Fig-TestToe-Structure.bmp)

