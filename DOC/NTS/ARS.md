# Address Resolution Server (ARS)

This document describes the design of the **Address Resolution Server (ARS)** used by the network transport stack of the *cloudFPGA* platform.

## Overview
A block diagram of the *ARS* is depicted in Figure 1. It features: 
  - an *ARP Packet Receiver (APr)* that parses the incoming ARP packet and extracts the relevant ARP fields as a metadata structure. 
  - an *ARP Packet Sender (APs)* that builds an ARP-REPLY packet upon request from the APr process, or an ARP-REQUEST packet upon request from the ACc process.
  - an *ARP CAM Controller (ACc)* that interfaces the RTL Content-Addressable Memory (CAM). This process serves the MAC lookup requests from the IpTxHandler (IPTX) and the MAC update requests from APr.

![Block diagram of the ARS](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-ARS-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the ARP Resolution Server</b></p>
<br>

## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **CAM**                                           | Content-Addressable Memory                            | [ArpCam](../../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp_ArpCam.vhd)
| **ETH**                                           | 10G Ethernet interface (via L2MUX)                    | [tenGigEth](../../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **IPRX**                                          | IP Receive frame handler (IPRX)                       | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/iprx_handler/src/iprx_handler.cpp)
| **IPTX**                                          | IP Transmit frame handler (IPTX).                     | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **MMIO**                                          | Memory Mapped IOs                                     | [mmioClient_A8_D8.v](../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)

## List of HLS Components

| Acronym       | Description                | Filename
|:--------------|:---------------------------|:--------------
| **ACc**       | ARP CAM Controller         | [arp_server](../../SRA/LIB/SHELL/LIB/hls/arp_server/src/arp_server.cpp)
| **APr**       | ARP Packet Receiver        | [arp_server](../../SRA/LIB/SHELL/LIB/hls/arp_server/src/arp_server.cpp)
| **APs**       | ARP CAM Controller         | [arp_server](../../SRA/LIB/SHELL/LIB/hls/arp_server/src/arp_server.cpp)

