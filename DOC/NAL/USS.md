UDP Sub-System (USS)
======================

This document describes the design of the **UDP Sub-System (USS)** of the [NAL](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./NAL.md) used by the *cloudFPGA* platform.

## Overview

A block diagram of the **`USS`** is depicted in Figure 1. 

It features processes for the opening and closing of UDP ports and for the RX Path (`pUdpRX`) as well as the TX Path (`pUdpTX`).

![Block Diagram of the NAL](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./images/Fig-USS-Structure.png?raw=true#center)

<p align="center"><b>Figure-1: Block diagram of the of the UDP Sub-System</b></p>

The architecture is written in C++/HLS in  [uss.cpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/uss.cpp)/[uss.hpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/uss.hpp).

## Interfaces

The details of the UDP Offload Engine (UOE) and its interface are described [here](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../NTS/UOE.md). 

The details of the Role UDP interface for the Themisto Shell are described [here](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../Themisto.md).

The connections to other processes of the NAL are as follows: 

- `sCacheInvalidation`: A Stream of type `bool` to signalize when the RX and TX paths have to invalidate the cached MRT <-> IPv4 address mapping.
- `sInternalEventFifo`: FIFOs for each process to write event updates to the Event Processing Engine of the NAL.
- `sConfigUpdate`: Stream to signalize when the "own" rank/NodeId of the FPGA did change (with the updated value).
- `sGetNidReq`: A Stream to request the NodeId of a given IPv4 address from the MRT Agency.
- `sGetNidRep`: The reply of the MRT Agency containing the NodeId or an invalid value (so that the packet will be dropped). 
- `piIpAddr`: The own IPv4 address, supplied by the MMIO registers.

The details of the mentioned MRT Agency can be found [here](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md).

## Functionality of the processes

In the following, the functionality of each process is briefly described:

- `pUdpLsn`: Asks the UOE to open a new UDP ports for listening, based on the request of the Role.

- `pUdpCls`: Asks the UOE to close an open UDP port. This happens either based on requests of the Role, during a partial reconfiguration, or after a reset of the Role. 

- `pUdpRX`: This process waits for the data and metadata of an incoming UDP packet and decides in the beginning if this packet belongs to a valid Node in the cluster (so it is forwarded to the Role). If not, the UDP packet will be dropped. To speed up the process of decision, all relevant `MRT <-> Ipv4Adress <-> TCP Session` mapping data are kept in a cache. UDP packets are provided in a *"take it or leave it"* way-of-thinking, so there are no buffers. If the Role can't read an incoming UDP packet, it will be dropped. 

- `pUdpTx`: This process waits for the data and metadata of an incoming UDP packet from the Role and decides in the beginning if this packet is addressed to a valid node in the cluster. If not, the UDP packet will be dropped. To speed up the process of decision, all relevant `MRT <-> Ipv4Adress <-> TCP Session` mapping data are kept in a cache. If the length of the packet is not known, the value `0` will be written as length to the UOE to activate the streaming mode. 
