TCP Sub-System (TSS)
======================

This document describes the design of the **TCP Sub-System (TSS)** of the [NAL](./NAL.md) used by the *cloudFPGA* platform.

## Overview

A block diagram of the **`TSS`** is depicted in Figure 1. 

It features processes for the opening of TCP *ports*, opening and closing of TCP *connections* and two processes for each of the RX Path (`pTcpRRh` and `pTcpRDp`) as well as the TX Path (`pTcpWRp` and `pTcpWBu`).

![Block Diagram of the NAL](./images/Fig-TSS-Structure.png#center)

<p align="center"><b>Figure-1: Block diagram of the of the TCP Sub-System</b></p>

The architecture is written in C++/HLS in  [tss.cpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/tss.cpp)/[tss.hpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/tss.hpp).

## Interfaces

The details of the TCP Offload Engine (TOE) and its interface are described [here](../NTS/TOE.md). 

The details of the Role TCP interface for the Themisto Shell are described [here](../Themisto.md).

The connections to other processes of the NAL are as follows: 

- `sCacheInvalidation`: A Stream of type `bool` to signalize when the RX and TX paths have to invalidate the cached MRT <-> IPv4 address mapping.
- `sInternalEventFifo`: Fifos for each process to write event updates to the Event Processing Engine of the NAL.
- `sConfigUpdate`: Stream to signalize when the "own" rank/NodeId of the FPGA did change (with the updated value).
- `sGetNidReq`: A Stream to request the NodeId of a given IPv4 address from the MRT Agency.
- `sGetNidRep`: The reply of the MRT Agency containing the NodeId or an invalid value (so that the packet will be dropped). 
- `sGetTripleFromSid_Req`: A Stream to request the **TCP Triple** (i.e. the corresponding Destination Port, Source Port, and Remote IP address) from a given TCP SessionId from the TCP agency.
- `sGetTripleFromSid_Rep`: The answer of the TCP agency. 
- `sMarkTripleAsPrivileged`: Notify the TCP agency that the corresponding SessionId (and TCP Triple) belongs to a *privileged TCP connection* (i.e. a connection for the FMC).
- `sDeleteEntryBySid`: Notifies the TCP Agency that this Session was closed (by the remote node).
- `sAddNewTriple`: Add a new TCP Triple with its SessionId to the table inside the TCP Agency.
- `sGetSidFromTriple_Req`: Get the SessionId for a given TCP Triple from the TCP Agency.
- `sGetSidFromTriple_Rep`: The answer of the TCP Agency. 

The details of the mentioned TCP and MRT Agencies can be found [here](./HSS.md). 

## Functionality of the processes

In the following, the functionality of each process is briefly described:

- `pTcpLsn`: Asks the TOE to open a new TCP port for listening, based on the request of the Role or FMC.

- `pTcpCls`: Asks the TOE to close existing TCP *connections*, i.e. during or after a partial reconfiguration of a new Role, and after the reset of the Role. 

- `pTcpRRh`: This process maintains an internal [cam8](../../SRA/LIB/SHELL/LIB/hls/NAL/src/cam8.hpp) to keep track of how many bytes for each active TCP session that is waiting in the TOE. It always reacts at new`siTOE_Notif`  and either directly requests the data to be delivered, or accumulate the waiting sizes. The counter feedback of `pRoleTcpRxDeq` and `pFmcRxDeq` ensures that `pTcpRRh` will only request data from the TOE, that could be read without disruption. When data is requested from the TOE via `soTOE_Dreq`, a notification (`sRDp_ReqNotif`) is also send to `pTCPRDp`. 

- `pTcpRDp`: This process reads the metadata and data from TOE, and decides in the beginning if this packet belongs to a valid Node in the cluster (so it is forwarded to the Role) or if it is a management command from an authorized source (so it is forwarded to the FMC). In all other cases the TCP packet will be dropped. To speed up the process of decision, all relevant `MRT <-> Ipv4Adress <-> TCP Session` mapping data are kept in a cache. 

- `pFmcRxDeq` & `pRoleTcpRxDeq`: These processes dequeue the data of TCP packets stored in the internal FIFOs and writes them to the Role or FMC. They keep track of how many bytes are written and write this number on each `tlast == 1` to the `pTcpRRh`. 

- `pTcpWRp`: This process reacts on data input from Role or FMC. Based on the provided meta data, it asks the TCP agency if a valid session exists for the given triple. If not, it requests `pTcpCOn` to open the missing connection to the remote host. Once the right SessionId is known, the data and metadata are written into the internal FIFO. If the length is not provided by the Role or FMC, the number of bytes are counted and written afterwards to the corresponding FIFO. If the length is larger then the maximum payload size, the data will be split. To speed up the process of decision, all relevant `MRT <-> Ipv4Adress <-> TCP Session` mapping data are kept in a cache.

- `pTcpCOn`: This process asks the TOE to open a new TCP connection, based on the requests from `pTcpWRp`. If the connection is acknowledged, the new SessionId is replied. In case of a timeout, an invalid session will be replied so that `pTcpWRp` can drop the packet.

- `pTcpWBu`: This process waits for the *length* of the current TCP packet, as provided by `TcpWRp`. Once the length is known, it asks the TOE if there is enough space to write this packet to the TX buffer. Based on the reply from TOE, either the full packet or parts of it are written to the TOE TX interface. If a packet couldn't be written completely, the a new request is send to the TOE for the remaining bytes.