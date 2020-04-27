### Network Transport Stack (NTS)
This document describes the design of the **TCP/IP Network and Transport Stack (NTS)** used by the *cloudFPGA* platform.  

#### Preliminary and Acknowledgments
This code was initialy developed by **Xilinx Dublin Labs, Ireland** who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be found [here](https://github.com/fpgasystems/fpga-network-stack).    

#### Overview
A block diagram of the *NTS* is depicted in Figure 1. It features a *User Datagram Protocol (UDP)* engine , a *Transmission Control Protocol Offload Engine (TOE)*, an *Internet Control Message Protocol (ICMP)* server and an *Address Resolution Protocol (ARP) Server*.
![Block diagram of the NTS](./images/Fig-NTS-Structure.bmp)
<p align="center"><b>Figure-1: Block diagram of the of the Network Transport Stack</b></p>  
<br>

#### HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../hdl-naming-conventions.md),
  2) [**HLS Naming Conventions**](./hls-naming-conventions.md)
<br>

#### List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **ETH**         | 10G Ethernet interface.                               | [tenGigEth](../../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **MEM**         | Memory sub-system (data-mover to DDR4).               | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **ROLE**        | Role Interface (alias APP).                           | [TODO]

#### List of HLS Components

| Acronym             | Description                                       | Filename
|:--------------------|:--------------------------------------------------|:--------------
| **[ARS](ARS.md)**   | Address Resolution Server (ARS).                  | [arp_server](../../SRA/LIB/SHELL/LIB/hls/arp_server/src/arp_server.cpp)
| **[ICMP](ICMP.md)** | Internet Control Message Protocol (ICMP) server.  | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)
| **[IPRX](IPRX.md)** | IP Receive frame handler (IPRX).                  | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/iprx_handler/src/iprx_handler.cpp)
| **[IPTX](IPTX.md)** | IP Transmit frame handler (IPTX).                 | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **[TOE](./TOE.md)** | TCP Offload Engine.                               | [toe](../../SRA/LIB/SHELL/LIB/hls/toe/src/toe.cpp)
| **UDP**             | UDP engine.                                       | udp

<br>
<br>
