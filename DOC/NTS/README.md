### Network Transport Stack (NTS)
This document describes the design of the **TCP/IP Network and Transport Stack (NTS)** used by the *cloudFPGA* platform.  

#### Preliminary and Acknowledgments
This code was initially developed by **Xilinx Dublin Labs, Ireland** who kindly accepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specialities of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be found [here](https://github.com/fpgasystems/fpga-network-stack).    

#### HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../hdl-naming-conventions.md),
  2) [**HLS Naming Conventions**](./hls-naming-conventions.md)
<br>

#### Overview
A block diagram of the **NTS** is depicted in Figure 1. It features a *User Datagram Protocol (UDP)* engine , a *Transmission Control Protocol Offload Engine (TOE)*, an *Internet Control Message Protocol (ICMP)* server and an *Address Resolution Protocol (ARP) Server*.
![Block diagram of the NTS](./images/Fig-NTS-Structure.bmp)
<p align="center"><b>Figure-1: Block diagram of the of the Network Transport Stack</b></p>  
<br>

The **NTS** is predominantly written in C/C++ with a few pieces coded in HDL (see Figure-1). The top-level that assembles 
and interconnects all the components, is the Verilog file [nts_TcpIp.v](../../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp.v). 
To facilitate the development and the interfacing with other C/C++ cores, we also provide the header file [nts.hpp](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp). 

#### List of Interfaces
The four main interfaces of the **NTS** are color-coded in *blue* on the above picture. They consist of:

| Acronym         | Description                                           | File
|:---------------:|:------------------------------------------------------|:--------------:
| **ETH**         | 10G Ethernet interface.                               | [tenGigEth](../../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **MEM**         | Memory sub-system (data-mover to DDR4).               | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **TAIF**        | TCP Application InterFace (alias TRIF).                | [TODO] 
| **UAIF**        | UDP Application InterFace (alias URIF).                | [TODO] 

#### List of HLS Components

| Acronym             | Description                                       | File
|:-------------------:|:--------------------------------------------------|:--------------:
| **[ARS](ARS.md)**   | Address Resolution Server (ARS).                  | [arp_server](../../SRA/LIB/SHELL/LIB/hls/NTS/arp/src/arp_server.hpp)
| **[ICMP](ICMP.md)** | Internet Control Message Protocol (ICMP) server.  | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.hpp)
| **[IPRX](IPRX.md)** | IP Receive frame handler (IPRX).                  | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.hpp)
| **[IPTX](IPTX.md)** | IP Transmit frame handler (IPTX).                 | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iptx/src/iptx.hpp)
| **[TOE](./TOE.md)** | TCP Offload Engine.                               | [toe](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.hpp)
| **[UOE](./UOE.md)** | UDP Offload Engine.                               | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.hpp)

<br>
<br>
