# Network Transport Stack (NTS)
This document describes the design of the **TCP/IP Network and Transport Stack (NTS)** used by the *cloudFPGA* platfrom.  

## Preliminary
This code was initialy developed by *Xilinx Dublin Labs, Ireland* who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

## Overview
A block diagram of the *NTS* is depicted in Figure 1. It features a *User Datagram Protocol (UDP)* engine , a *Transmission Control Protocol Offload Engine (TOE)*, an *Internet Control Message Protocol (ICMP)* server and an *Address Resolution Protocol (ARP) Server*.
![Block diagram of the NTS](../internal/shell/images/Fig-NTS0-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the of the Network Transport Stack</b></p>  
<br>

## HLS Coding Style and Naming Conventions
Please consider reading the following HLS naming conventions document before diving or contributing to this part of the cloudFPGA project: [TODO - Link to a MD file] 



<br>

## List of HLS Components

| Name         |  Acronym       | Description                                                       |
|:------------ |:--------------:|:------------------------------------------------------------------|
| [arp_server](./arp_server)    | **ARS**  | Address Resolution Protocol (ARP) Server.              |
| [dhcp_client](./dhcp_client)  | **DHCP**  | Dynamic Host Configuration Protocol (DHCP) client.    |
| [icmp_server](./icmp_server)  | **ICMP**  | Internet Control Message Protocol (ICMP) server.      |
| [iprx_handler](./iprx_handler)| **IPRX**  | IP Receiver frame handler (IPRX).                     |
| [iptx_handler](./iptx_handler)| **IPTX**  | IP Transmit frame handler (IPTX).                     |
| [mpe](./mpe)                  | **MPE**   |                                                       |
| [smc](./smc)                  | **SMC**   |                                                       |
| [tcp_role_if](./tcp_role_if)  | **TRIF**  | TCP Role Interface.                                   |
| [toe](./toe)                  | **TOE**   | TCP Offload Engine.                                   |
| [udp](./udp)                  | **UDP**   | UDP engine.                                           |
| [udp_mux](./udp_mux)          | **UDMX**  | UDP Multiplexer.                                      |
| [udp_role_if](./udp_role_if)  | **URIF**  | UDP Role Interface.                                   | 

## HowTo 





