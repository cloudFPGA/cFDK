# Internet Control Message Protocol server (ICMP)
This document describes the design of the **Internet Control Message Protocol server (ICMP)** used by the network transport stack of the *cloudFPGA* platform.

## Overview
A block diagram of the *ICMP* is depicted in Figure 1. It features: 
  - an *ICMP header Checksum accumulator and Checker (ICc)* that handles the incoming data stream from the IPRX. It assesses the validity of the ICMP checksum while forwarding the data stream to the InvalidPacketDropper (IPd). If the incoming packet is an ICMP_REQUEST, the packet forwarded to [IPd] is turned into an ICMP_REPLY and the corresponding checksum is forwarded to the IcmpChecksumInserter (ICi).
  - a *Control Message Builder (CMb)* that assembles the ICMP error messages.
  - an *IP Header Appender (IHa)* that encapsulates the ICMP error message into an IP packet. 
  - an *IP Checksum Inserter (ICi)* that inserts both the IP header checksum and the ICMP checksum in the outgoing IP packet.


![Block diagram of the ICMP](./images/Fig-ICMP-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the ICMP Server</b></p>
<br>

## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **IPRX**                                          | IP Receive frame handler (IPRX)                       | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/iprx_handler/src/iprx_handler.cpp)
| **IPTX**                                          | IP Transmit frame handler (IPTX).                     | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **UDP**                                           | UDP engine (UDP).                                     | [udp](../../SRA/LIB/SHELL/LIB/hls/udp/src/udp.hpp)


## List of HLS Components

| Acronym       | Description                 | Filename
|:--------------|:----------------------------|:--------------
| **CMb**       | Control Message Builder     | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)
| **ICc**       | ICMP Checksum Checker       | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)
| **ICi**       | IP Checksum Inserter        | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)
| **IHa**       | IP Header Appender          | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)
| **IPd**       | Invalid Packet Dropper      | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/icmp_server/src/icmp_server.cpp)

