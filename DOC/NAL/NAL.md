Network Abstraction Layer (NAL)

==================================

This document describes the design of the **Network Abstraction Layer (NAL)** used by the *cloudFPGA* platform.

## Overview

A block diagram of the **`NAL`** is depicted in Figure 1. 

It features the *Processing of the AXI4 Lite control connection*, a *Port Open & Close logic*, a *TCP Agency*, an *Message Routing Table (MRT) Agency*, a *Cache Invalidation logic*, and *Status & Event Processing logic*, and *Sub-Systems for TCP (TSS) and UDP (USS)*.

![Block Diagram of the NAL](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./images/Fig-NAL-Structure.png?raw=true#center)

<p align="center"><b>Figure-1: Block diagram of the of the Network Abstraction Layer</b></p>

## Context

The purpose of the NAL is to separate privileged management traffic from the unprivileged Role traffic, to ensure the traffic to and from the Role is/stays within the users' subnet, and to abstract the details of port opening, connection handling and IP Address translation.

The Role Interface for UDP / TCP traffic is defined by the **[Themisto Shell](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./../Themisto.md)** and can be summarized as follows:

```cpp
hls::ap_uint<32>                 *po_rx_ports,    //Vector indicating Listening Ports 
hls::stream<NetworkWord>         &siNetwork_data, //Input data (UDP or TCP)
hls::stream<NetworkMetaStream>   &siNetwork_meta, //Input meta data
hls::stream<NetworkWord>         &soNetwork_data, //Output data (UDP or TCP)
hls::stream<NetworkMetaStream>   &soNetwork_meta  //Output meta data
```

The `po_rx_ports` defines the ports a Role wants to open. The ports are one-hot encoded starting at `NAL_RX_MIN_PORT` (for definition, see [network.hpp](../../SRA/LIB/hls/network.hpp)). So a `*po_rx_ports = 0xa` would open the ports `NAL_RX_MIN_PORT+0`, `NAL_RXMIN_PORT+1`, and `NAL_RX_MIN_PORT+3` for listening. 

The network data is an *AXI4 Stream with 64 bit width*. 

The meta data is a quintuple consisting of the following:

```cpp
struct NetworkMeta {
  NodeId  dst_rank;   // The node-id where the data goest to (the FPGA itself if receiving)
  NrcPort dst_port;   // The receiving port
  NodeId  src_rank;   // The origin node-id (the FPGA itself if sending)
  NrcPort src_port;   // The origin port
  NetworkDataLength len;    // The length of this data chunk/packet (optional)

};
```

Please note, *this interface is the **same** for UDP and TCP*. All lower-level details are abstracted away by the NAL. To decide if a Role IP Core should be connected via UDP, TCP, or both, the signals of the Shell Role interface in the `Role.v{hdl}` should be connected as desired.

## Supported Shells

Currently, only the **[Themisto Shell](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./../Themisto.md)** contains the NAL. 

## Main Functionality

The NAL is controlled by the *FPGA Management Core* [*FMC*](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../FMC/FMC.md) via an AXI4Lite bus. Through this connection, the NAL gets the list of allowed IP addresses for this Role, the mapping of IP addresses to *`NodeId`s*, and reports status as well as some debug information.

The split between management TCP traffic and "normal" user traffic is done by the **TSS**. For the TCP traffic, the FMC is connected via FIFOs (instantiated in `Shell.v`, so external to the NAL). 

To map *`NodeId`s* or *`rank`s* to IPv4 addresses, the NAL maintains a **Message Routing Table (MRT)**. This table is written via the AXI4 Lite controll link. The *MRT Agency* enables the access to this table via `request/reply` stream-pairs.

The *TCP Agency* keeps track of valid *TCP sessions* and translates them to IPv4 addresses and NodeIds or vice-versa, as requested by other cores via `request/reply` stream-pairs.

To handle the input of listen port request vectors (`po_rx_ports`, see above; `piTcpRxPorts` and `piUdpRxPorts` in Figure 1) for UDP and TCP, the *Port Open & Close* logic translates them in absolute opening or closing requests for the relevant UDP or TCP processes.

To reduce the latency of the `NodeId <--> IpAddress (<--> TCP session Id)` mapping, the USS and TSS are using *Caches*. The *Cache Invalidation Logic* detects conditions when those caches must be invalidated and signals this to all concerning processes (e.g. after a partial reconfiguration of the role, or the update of the MRT).

For debugging and monitoring, the USS and TSS systems create *events* (e.g. about a successful packet transmission, the meta data of the last connection, or an unauthorized access attempt) and send them to the *Status & Event Processing*. This process merges all those event notification, maps them to the correct status address in the AXI4 Lite address space, and forwards these updates to the AXI4Lite Processing Logic. 

The details of the TSS and USS can be found [here (TSS)](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./TSS.md) and [here (USS)](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./USS.md). The HSS is futher documented [here](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md). 

## HLS Coding Style and Naming Conventions

For the cloudFPGA specific HDL and HLS naming rules and conventions, please see the [`NTS` documentation](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../NTS/NTS.md).

## List of Components

The following table lists the sub-components of **`NAL`** and provides a link to their documentation as well as their architecture body. 

| Entity                                                      | Description                                                          | Architecture                                           |
|:-----------------------------------------------------------:|:-------------------------------------------------------------------- |:------------------------------------------------------ |
| **[Axi4Lite Processing](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md)**                         | Processing of Axi4Lite control bus.                                  | [hss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp) |
| **[MRT Agency](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md)**                                  | Providing stream-based access to the Message Routing Table (MRT).    | [hss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp) |
| **[TCP Agency](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md)**                                  | Providing stream-based access to the TCP session-to-triple table.    | [hss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp) |
| **[Port Open & Close](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./HSS.md)**                           | Handling of the user open & close port vectors.                      | [hss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp) |
| **[Status & Event Processing](#status-%26-event-handling)** | Processing of the Event notification and maintains the status table. | [nal.cpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/nal.cpp) |
| **[USS](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./USS.md)**                                         | UDP Sub-System                                                       | [uss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/uss.cpp) |
| **[TSS](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/./TSS.md)**                                         | TCP Sub-System                                                       | [tss.cpp](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NAL/../../SRA/LIB/SHELL/LIB/hls/NAL/src/tss.cpp) |

## Description of the Interfaces

The HLS entity declaration of the **`NAL`** is specified as follows:

```cpp
void nal_main(
    // ----- link to FMC -----
    ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    //state of the FPGA
    ap_uint<1>                     *layer_4_enabled,
    ap_uint<1>                     *layer_7_enabled,
    ap_uint<1>                     *role_decoupled,
    // ready signal from NTS
    ap_uint<1>                  *piNTS_ready,
    // ----- link to MMIO ----
    ap_uint<16>                 *piMMIO_FmcLsnPort,
    ap_uint<32>                 *piMMIO_CfrmIp4Addr,
    // -- my IP address 
    ap_uint<32>                 *myIpAddress,

    //-- ROLE UDP connection
    ap_uint<32>                 *pi_udp_rx_ports,
    stream<NetworkWord>         &siUdp_data,
    stream<NetworkWord>         &soUdp_data,
    stream<NetworkMetaStream>   &siUdp_meta,
    stream<NetworkMetaStream>   &soUdp_meta,

    // -- ROLE TCP connection
    ap_uint<32>                 *pi_tcp_rx_ports,
    stream<NetworkWord>         &siTcp_data,
    stream<NetworkMetaStream>   &siTcp_meta,
    stream<NetworkWord>         &soTcp_data,
    stream<NetworkMetaStream>   &soTcp_meta,

    // -- FMC TCP connection
    stream<TcpAppData>          &siFMC_data,
    stream<TcpAppMeta>          &siFMC_SessId,
    stream<TcpAppData>          &soFMC_data,
    stream<TcpAppMeta>          &soFMC_SessId,

    //-- UOE / Control Port Interfaces
    stream<UdpPort>             &soUOE_LsnReq,
    stream<StsBool>             &siUOE_LsnRep,
    stream<UdpPort>             &soUOE_ClsReq,
    stream<StsBool>             &siUOE_ClsRep,
    //-- UOE / Rx Data Interfaces
    stream<UdpAppData>          &siUOE_Data,
    stream<UdpAppMeta>          &siUOE_Meta,
    //-- UOE / Tx Data Interfaces
    stream<UdpAppData>          &soUOE_Data,
    stream<UdpAppMeta>          &soUOE_Meta,
    stream<UdpAppDLen>          &soUOE_DLen,

    //-- TOE / Rx Data Interfaces
    stream<TcpAppNotif>            &siTOE_Notif,
    stream<TcpAppRdReq>            &soTOE_DReq,
    stream<TcpAppData>             &siTOE_Data,
    stream<TcpAppMeta>             &siTOE_SessId,
    //-- TOE / Listen Interfaces
    stream<TcpAppLsnReq>           &soTOE_LsnReq,
    stream<TcpAppLsnRep>           &siTOE_LsnRep,
    //-- TOE / Tx Data Interfaces
    stream<TcpAppData>          &soTOE_Data,
    stream<TcpAppSndReq>        &soTOE_SndReq,
    stream<TcpAppSndRep>        &siTOE_SndRep,
    //-- TOE / Open Interfaces
    stream<TcpAppOpnReq>          &soTOE_OpnReq,
    stream<TcpAppOpnRep>           &siTOE_OpnRep,
    //-- TOE / Close Interfaces
    stream<TcpAppClsReq>           &soTOE_ClsReq
);
```

## Status & Event Handling

Most components of the NAL send internal events of type `NalEventNotif` (see below) in individual streams to a process that merges these streams (`eventFifoMerge` in `nal.cpp`) and forwards it to the event processing in the process `pStatusMemory`.

```cpp
enum NalCntIncType {NID_MISS_RX = 0, NID_MISS_TX, PCOR_TX, TCP_CON_FAIL, LAST_RX_PORT, \
  LAST_RX_NID, LAST_TX_PORT, LAST_TX_NID, PACKET_RX, PACKET_TX, UNAUTH_ACCESS, \
    AUTH_ACCESS, FMC_TCP_BYTES};

struct NalEventNotif {
  NalCntIncType type;
  ap_uint<32>   update_value;
  //in case of LAST_* types, the update_value is the new value
  //on other cases, it is an increment value
  NalEventNotif() {}
  NalEventNotif(NalCntIncType nt, ap_uint<32> uv): type(nt), update_value(uv) {}
};
```

These events are then mapped to 16 32-bit status lines of the Axi4Lite address space. If one line changes, the updated line is streamed to the Axi4Lite processing engine (`axi4liteProcessing` in `hss.cpp`). 
The mapping of events to address offsets is defined by the `NAL_STATUS_*` definitions in `nal.hpp`. 

The events are handled with a best-effort methodology, because the NAL won't block any data stream if the events processing lags behind. It could be that if a lot of small packets with high bandwidth were processed, some event updates are dropped. 

### Status registers

The NAL maintains a set of registers that reports some events and internal states for debugging reasons. The registers are all 32bit wide, but some of them are devided to report multiple events, as shown below:

| `ctrlLink` address | bit address | description                                                                                    |
|:------------------:|:-----------:|:---------------------------------------------------------------------------------------------- |
| 0x90               | 31 -- 0     | Current version of the MRT (counter since power-on)                                            |
| 0x91               | 31 -- 0     | Currently opened UDP ports (vector encoded, as described above)                                |
| 0x92               | 31 -- 0     | Currently opened TCP ports (for the Role, vector encoded)                                      |
| 0x93               | 31 -- 0     | Currently opened privileged TCP port (for the FMC, absolute number)                            |
| 0x94               | 31 -- 0     | FMC unauthorized access count                                                                  |
| 0x95               | 31 -- 0     | FMC authorized access count                                                                    |
| 0x96               | 31 -- 0     | Unused                                                                                         |
| 0x97               | 31 -- 0     | Unused                                                                                         |
| 0x98               | 31 -- 0     | Number of *bytes* received for the FMC in total                                                |
| 0x99               | 15 -- 0     | Last RX NodeId                                                                                 |
| 0x99               | 31 -- 16    | Last RX Port (absolute number)                                                                 |
| 0x9a               | 15 -- 0     | Number of packets received with an invalid/unkown ip address (i.e. not part of the cluster)    |
| 0x9a               | 31 -- 15    | Number of packets tried to send to an invalid port                                             |
| 0x9b               | 15 -- 0     | Last TX NodeId                                                                                 |
| 0x9b               | 31 -- 16    | Last TX Port (absolute number)                                                                 |
| 0x9c               | 15 -- 0     | Number of packets tried to send to an invalid/unkown ip address (i.e. not part of the cluster) |
| 0x9c               | 31 -- 16    | Number of TCP connection failures/timeouts (TX side)                                           |
| 0x9d               | 31 -- 0     | NodeId of this NAL                                                                             |
| 0x9e               | 31 -- 0     | Total number of packets received (for TCP: based on `tlast` of seqments)                       |
| 0x9f               | 31 -- 0     | Total number of packets send (for TCP: based on `tlast` of seqments)                           |

The FMC of the Themisto Shell requests this status registers regularly and includes them in the corresponding HTTP responses. Some of the debugging registers are also reported to the user, via the `flight recorder data`-functionality of the CFRM.

## Latencies of the components

Vivado HLS estimates the latencies of the NAL as follows: 

```
Latency (clock cycles): 
    * Summary: 
    +-----+-----+-----+-----+----------+
    |  Latency  |  Interval | Pipeline |
    | min | max | min | max |   Type   |
    +-----+-----+-----+-----+----------+
    |   15|  194|    2|  100| dataflow |
    +-----+-----+-----+-----+----------+

    + Detail: 
        * Instance: 
        +-------------------------+----------------------+-----+-----+-----+-----+----------+
        |                         |                      |  Latency  |  Interval | Pipeline |
        |         Instance        |        Module        | min | max | min | max |   Type   |
        +-------------------------+----------------------+-----+-----+-----+-----+----------+
        |pTcpAgency_U0            |pTcpAgency            |    2|    2|    1|    1| function |
        |pTcpRRh_U0               |pTcpRRh               |    1|    1|    1|    1| function |
        |pStatusMemory_U0         |pStatusMemory         |    5|    5|    1|    1| function |
        |pPortLogic_U0            |pPortLogic            |    1|    1|    1|    1| function |
        |pTcpWRp_U0               |pTcpWRp               |    1|    1|    1|    1| function |
        |axi4liteProcessing_U0    |axi4liteProcessing    |    1|   99|    1|   99|   none   |
        |pMrtAgency_U0            |pMrtAgency            |    1|   82|    1|   82|   none   |
        |pUdpTX_U0                |pUdpTX                |    1|    1|    1|    1| function |
        |pTcpRDp_U0               |pTcpRDp               |    1|    1|    1|    1| function |
        |pTcpWBu_U0               |pTcpWBu               |    0|    0|    1|    1| function |
        |pRoleTcpRxDeq_U0         |pRoleTcpRxDeq         |    1|    1|    1|    1| function |
        |pFmcTcpRxDeq_U0          |pFmcTcpRxDeq          |    0|    0|    1|    1| function |
        |pUdpRx_U0                |pUdpRx                |    1|    1|    1|    1| function |
        |pTcpCOn_U0               |pTcpCOn               |    0|    0|    1|    1| function |
        |pCacheInvalDetection_U0  |pCacheInvalDetection  |    0|    0|    1|    1| function |
        |pUdpLsn_U0               |pUdpLsn               |    0|    0|    1|    1| function |
        |pTcpLsn_U0               |pTcpLsn               |    0|    0|    1|    1| function |
        |pRoleUdpRxDeq_U0         |pRoleUdpRxDeq         |    1|    1|    1|    1| function |
        |pUoeUdpTxDeq_U0          |pUoeUdpTxDeq          |    1|    1|    1|    1| function |
        |eventFifoMerge_U0        |eventFifoMerge        |    0|    0|    1|    1| function |
        |pTcpRxNotifEnq_U0        |pTcpRxNotifEnq        |    0|    0|    1|    1| function |
        |pTcpCls_U0               |pTcpCls               |    0|    0|    1|    1| function |
        |nal_main_entry1155_U0    |nal_main_entry1155    |    0|    0|    0|    0|   none   |
        |pUdpCls_U0               |pUdpCls               |    0|    0|    1|    1| function |
        |nal_main_entry3_U0       |nal_main_entry3       |    0|    0|    0|    0|   none   |
        +-------------------------+----------------------+-----+-----+-----+-----+----------+
```
