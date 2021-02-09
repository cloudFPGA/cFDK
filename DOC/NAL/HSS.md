Housekeeping Sub-System (HSS)
==============================

This document describes the design of the **Housekeeping Sub-System (HSS)** of the [NAL](./NAL.md) used by the *cloudFPGA* platform.

## Overview

The HSS module contains all processes that are necessary to keep the administrative data, as well as debug and status information up to date. 

The architecture is written in C++/HLS in [hss.cpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp)/[hss.hpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.hpp).

## Data structures

### Message Routing Table (MRT)

The MRT maps `NodeId`s or `rank`s to IPv4 addresses, as sketched in the following:

| `rank`, i.e. line number of the MRT | IP address |
|:-----------------------------------:|:----------:|
| 0                                   | 0x0a0b0c01 |
| 1                                   | 0x0a0b0c42 |
| ....                                | ....       |
| 63                                  | 0x0ac7a302 |

`Rank` is a term from the MPI world, but for the cloudFPGA platform without virtualization (i.e. one Role per FPGA), it is synonymous to`NodeId`.

The **maximum cluster size is 64** (currently). The IP addresses are represented as a big-endian 32bit integer. 

The MRT is implemented by an array stored in a 2-port BRAM. 

### TCP Triple-Session Table

If a new chunk of a TCP stream did arrive at the FPGA, the TOE only communicates the corresponding `SessionId` of this new data. Similar, if the Role want's to write to an existing TCP stream, it needs the corresponding SessionId. 
At the other side, the abstractions provided by the Themisto Shell, hides those details from the user. So that the user can read and write TCP or UDP by using `NodeId` and port numbers. 

Consequently, the `NAL` needs to map `NodeId`s with their ports to TCP `SessionIds`. This is done using so called **TCP Triple**s. 

A TCP *triple contains an IPv4 address, the destination port, and the source port* (and the IP address of the FPGA is the obvious part in the TCP connection quadruple). This triple is then a binary string that represents this connection and the same input leads to the same binary representation. As example, the triple `0xbe1c0a0b0c010a9e` represents the TCP connection that comes from the remote host and port 10.11.12.01:48668 (`0x0a0b0c01:0xbe1c`) to the local port 2718 (`0x0a9e`). 

So, after mapping `NodeId`s to `IPv4-address`es (or vice versa) using the `MRT`, the TCP triple can be created and with this the corresponding `SessionId` can be looked up, using the following table:

| `SessionId` (`ap_uint<16>`) | NAL TCP Triple (`ap_uint<64>`) | entry valid (`ap_uint<1>`) | privileged connection (`ap_uint<1>`) | to be deleted (`ap_uint<1>`) |
|:---------------------------:|:------------------------------:|:--------------------------:|:------------------------------------:|:----------------------------:|
| 2                           | (0xbe1c, 0x0a0b0c01, 0x0a9e)   | 1                          | 1                                    | 0                            |
| 11                          | (0x0a9e, 0x0a0b0c42, 0x0a9e)   | 1                          | 0                                    | 0                            |
| 3                           | (0xbe3b, 0x0a0b0c01, 0x0a9e)   | 0                          | 1                                    | 0                            |
| ...                         | ...                            | ...                        | ...                                  | ...                          |

The column `privileged connection` indicates if this TCP connection came form an authenticated management node (this leads to a different treatment of this connection during partial reconfiguration and on port changes). 

The column `to be deleted` marks sessions of an old Role (i.e. previously to a partial reconfiguration) that must be closed by `pTcpCls`. 

Currently, the NAL supports *up to 8 parallel sessions* (as does the TOE). 

## Functionality of the processes

In the following, the functionality of each process is briefly described:

- `axi4liteProcessing`: This process contains the Axi4 Lite secondary endpoint and reads the MRT and configuration values from it as well as writes the status values. It notifies all other concerned processes on MRT or configuration updates and is notified on status updates.  

- `pMrtAgency`: This process can access the BRAM that contains the MRT and replies to lookup requests between 1 and 80+ cycles. It also creates a status event if the MRT version changes (e.g. after a partial reconfiguration or a cluster update).

- `pPortLogic`: This logic translates the one-hot encoded open-port vectors from the Role (i.e. `piUdpRxPorts` and `piTcpRxPorts`) to absolute port numbers. If the input vector changes, or during a reset of the Role, the necessary open or close requests are send to `pUdpLsn`, `pUdpCls`,  `pTcpLsn`, and `pTcpCls`. 

- `pCacheInvalDetection`: This logic detects if the caches of the USS and TSS have to be invalidated and signals this to the concerned processes. E.g. after a partial reconfiguration, a cluster extend or reduce, or after closing of TCP connections, the  look-up caches must be invalidated. 

- `pTcpAgency`: This process contains the SessionId-Triple CAM as described above. It can reply to requests within 3 cycles. 

To see how this processes are connected with the remaining parts of the NAL, please refer to Figure 1 of the [NAL documentation](./NAL.md).