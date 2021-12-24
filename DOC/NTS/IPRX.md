# IP Rx Handler (IPRX)

This document describes the design of the **IP Rx Handler (IPRX)** used by the network transport stack of 
the *cloudFPGA* platform.

## Overview
A block diagram of the *IPRX* is depicted in Figure 1. It features:
  - an *Input Buffer (IBuf)* that enqueues the incoming data traffic into a FiFo,
  - a *Mac Protocol Detector (MPd)* that parses the Ethernet header to detect ARP vs IPv4 frames and forward 
  them accordingly,
  - an *IP Length Checker (ILc)* to check the IPv4/TotalLength field,
  - an *IP Checksum Accumulator (ICa)* to compute  the header checksum of the incoming IPv4 packet.
  - an *IP Checksum Checker (ICc)* to assess the header checksum of the incoming IPv4 packet.
  - an *IP Invalid Dropper (IId)* that drops the incoming IP packet when its version is not '4', 
  when it is fragmented, or when its IP header checksum is not valid,
  - an *IP Cut Length (ICl)* to intercept and drop packets which are longer than the announced 'Ip4TotalLenght' 
  field,
  - an *IP Packet Router (IPr)* to route the IPv4 packets to one of the ICMP, UDP or TCP engines.
 
![Block diagram of the IPRX](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./images/Fig-IPRX-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the IP Rx Handler</b></p>
<br>

## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **ARP**                                           | Address Resolution Server                             | [nts_TcpIp_Arp](../../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp_Arp.vhd)
| **ETH**                                           | 10 Gigabit Ethernet Subsystem                         | [tenGigEth](../../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **ICMP**                                          | Internet Control Message Protocol Server                                                      | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.hpp)
| **TOE**                                           | TCP Offload Engine                                    | [toe](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.hpp)
| **UDP**                                           | UDP Offload Engine                                    | [udp](../../SRA/LIB/SHELL/LIB/hls/NTS/udp/src/udp.hpp)

## List of HLS Components

| Acronym       | Description                | Filename
|:--------------|:---------------------------|:--------------
| **IBuff**     | Input Buffer               | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **ICa**       | IP Checksum Accumulator    | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)       
| **ICc**       | IP Checksum Checker        | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **ICl**       | IP Cut Length              | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **IId**       | IP Invalid Dropper         | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **ILc**       | IP Length Checker          | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **IPr**       | IP Packet Router           | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **MPd**       | Mac Protocol Detector      | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
