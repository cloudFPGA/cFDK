# UDP Offload Engine (UOE)
This document describes the design of the **UDP Offload Engine (UOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initialy developed by **Xilinx Dublin Labs, Ireland** who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

## Overview
The *UOE* consists of an RxEngine (RXe) depicted in Figure-1 and a TxEngine (TXe) depicted in Figure-2. 

The *RXe* features a [TODO] ...
![Block diagram of the RXe](./images/Fig-UOE-RXe-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Rx Engine of the UDP Offload Engine</b></p>

The *TXe* features a [TODO] ...
![Block diagram of the TXe](./images/Fig-UOE-TXe-Structure.bmp#center)
<p align="center"><b>Figure-2: Block diagram of the Tx Engine of the UDP Offload Engine</b></p>

<br>

## HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../hdl-naming-conventions.md), 
  2) [**HLS Naming Conventions**](./hls-naming-conventions.md)
<br>

## List of Files
  * [**uoe.cpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
  * [**uoe.hpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.hpp)


## List of Interfaces

| Acronym                                           | Description                                      | Filename
|:--------------------------------------------------|:-------------------------------------------------|:--------------
| **[ICMP](ICMP.md)**                               | Internet Control Message Protocol (ICMP) server. | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.hpp)
| **[IPRX](IPRX.md)**                               | IP RX interface                                  | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx_handler/src/iprx_handler.cpp)
| **[IPTX]()IPTX.md)**                              | IP TX interface                                  | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iptx_handler/src/iptx_handler.cpp)
| **[UAIF](#markdown-header-udp-app-interface)**    | UDP Application InterFace                        | tcp_role_interface

## List of HLS Components

| Acronym       | Description                | Filename
|:--------------|:---------------------------|:--------------
| **Ihs**       | Ip Header Strippe          | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Iha**       | Ip Header Adder            | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Rph**       | Rx Packet Handler          | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Tai**       | Tx Application Interface   | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Tdh**       | Tx Datagram Handler        | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Uca**       | Udp Cheksum Accumulator    | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Ucc**       | Udp Cheksum Checker        | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Uha**       | Udp Header Adder           | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **Upt**       | Udp Port Table             | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)


## Description of the interfaces [TODO]

### UDP App Interface
The _UDP Application Interface_ (UAIF) connects the UOE to the user's role and its application (APP).
The interface consists of the following signals (TODO).  
 

