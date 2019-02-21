#  cloudFPGA / SHELL / hls*

This directory contains the High-Level Synthesis (HLS) code used by the Network Transport and Session (NTS) stack of the cloudFPGA platform. 

## Preliminary
This codes was initialy developed by the Xilinx Dublin Labs who kindly acepted to share it with the cloudFPGA project via the GitHub repository: https://github.com/XilinxDublinLabs/arches . In 2018, the code was ported and adapted for the needs and specificities of the cloudFPGA platform.

## HLS Coding Style and Naming Conventions
Please consider reading the following HLS naming conventions document before diving or contributing to this part of the cloudFPGA project: [TODO - Link to a MD file] 


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





