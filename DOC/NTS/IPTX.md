# IP Tx Handler (IPTX)
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/IPTX.md)

This document describes the design of the **IP Tx Handler (IPTX)** used by the network transport stack of the *cloudFPGA* platform.

## Overview
A block diagram of the *IPTX* is depicted in Figure 1. It features: 
  - a *Header Checksum Accelerator (HCa)* that computes the IPv4 header checksum,
  - an *IP Checksum Inserter (ICi)* which inserts that checksum into the IP header of the incoming packet,
  - an *IP Address Extractor (IAe)* that extracts the IP destination address from the incoming packet and forwards it 
  to the **Address Resolution Server (ARP)** in order to look up the corresponding MAC address.
  - a *MAC Address Inserter (MAi)* to insert the Ethernet MAC address corresponding to the IPv4 destination address that
   was looked-up by the ARP server.

![Block diagram of the IPTX](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-IPTX-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the IP Tx Handler</b></p>
<br>

## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **ARP**                                           | Address Resolution Server                             | [ARP](../../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp_Arp.vhd)
| **L2MUX**                                         | Layer-2 MUltipleXer interface (2-to-1)                | [AXI4-Stream Interconnect](https://www.xilinx.com/products/intellectual-property/axi4-stream_interconnect.html)
| **L3MUX**                                         | Layer-3 MUltipleXer interface                         | [AXI4-Stream Interconnect](https://www.xilinx.com/products/intellectual-property/axi4-stream_interconnect.html)
| **MMIO**                                          | Memory Mapped IOs                                     | [mmioClient_A8_D8.v](../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)

## List of HLS Components

| Acronym       | Description                | Filename
|:--------------|:---------------------------|:--------------
| **HCa**       | Header Checksum Accumulator| [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **IAe**       | IP Address Extractor       | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **ICi**       | IP Checksum Inserter       | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **MAi**       | MAC Address Inserter       | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
 

