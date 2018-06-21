# **HLS VERSION OF THE ECHO APPLICATION IN STORE-AND-FORWARD MODE**

## Overview

This directory is provided as an example to demonstrate the interactions and interconnections of the Shell-Role Architecture. 

The _RoleEchoHls_ application is a Vivado High-Level Synthesis (HLS) version of a role that implements an echo application made of a UDP loopback and a TCP loopback connections. The role is said to be operating in "_store-and-forward_" mode because every received packet is first stored in the DDR4 before being read from that memory and being sent back.     

## About

   - **hls/** contains the Vivado HLS code of the _RoleEchoHls_ application.
   - **tcl/** contains scripts for creating a Vivado project of the _RoleEchoHls_ application.

   - **Makefile** handles the implementation dependencies of this application. 

## How To Build

   - the current role design
   
      --> make

## How To Configure

  - the settings of the Xilinx project
  
      --> edit the script './tcl/xpr_settings.tcl' and set the variables of the 'User Defined Settings' section at the beginning of the file.
