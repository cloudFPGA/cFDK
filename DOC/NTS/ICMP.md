# Internet Control Message Protocol server (ICMP)

This document describes the design of the **Internet Control Message Protocol server (ICMP)** used by the network transport stack of the *cloudFPGA* platform.

## Overview
A block diagram of the *ICMP* is depicted in Figure 1. It features: 
  - an *ICMP header Checksum accumulator and Checker (ICc)* that handles the incoming data stream from the IPRX. It assesses the validity of the ICMP checksum while forwarding the data stream to the InvalidPacketDropper (IPd). If the incoming packet is an ICMP_REQUEST, the packet forwarded to [IPd] is turned into an ICMP_REPLY and the corresponding checksum is forwarded to the IcmpChecksumInserter (ICi).
  - a *Control Message Builder (CMb)* that assembles the ICMP error messages.
  - an *IP Header Appender (IHa)* that encapsulates the ICMP error message into an IP packet. 
  - an *IP Checksum Inserter (ICi)* that inserts both the IP header checksum and the ICMP checksum in the outgoing IP packet.


![Block diagram of the ICMP](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./images/Fig-ICMP-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the ICMP Server</b></p>
<br>

## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **IPRX**                                          | IP Receive frame handler (IPRX)                       | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp)
| **IPTX**                                          | IP Transmit frame handler (IPTX).                     | [iptx](../../SRA/LIB/SHELL/LIB/hls/NTS/iptx/src/iptx.cpp)
| **UDP**                                           | UDP engine (UDP).                                     | [udp](../../SRA/LIB/SHELL/LIB/hls/NTS/udp/src/udp.hpp)


## List of HLS Components

| Acronym       | Description                 | Filename
|:--------------|:----------------------------|:--------------
| **CMb**       | Control Message Builder     | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)
| **ICc**       | ICMP Checksum Checker       | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)
| **ICi**       | IP Checksum Inserter        | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)
| **IHa**       | IP Header Appender          | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)
| **IPd**       | Invalid Packet Dropper      | [icmp](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)

