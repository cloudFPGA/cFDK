Network Abstraction Layer (NAL)
==================================

This document describes the design of the **Network Abstraction Layer (NAL)** used by the *cloudFPGA* platform.

## Overview

A block diagram of the **`NAL`** is depicted in Figure 1. 

It features the *Processing of the AXI4 Lite control connection*, a *Port Open & Close logic*, a *TCP Agency*, an *Message Routing Table (MRT) Agency*, a *Cache Invalidation logic*, and *Status & Event Processing logic*, and *Sub-Systems for TCP (TSS) and UDP (USS)*.

<img title="" src="file:///home/ngl/gitrepos/cloudFPGA/cFDK/DOC/NAL/images/Fig-NAL-Structure.png" alt="loading-ag-265" data-align="center">

<p align="center"><b>Figure-1: Block diagram of the of the Network Abstraction Layer</b></p>

## Context

The purpose of the NAL is to separate privileged management traffic from the unprivileged Role traffic, to ensure the traffic to and from the Role is/stays within the users' subnet, and to abstract the details of port opening, connection handling and IP Address translation.

The Role Interface for UDP / TCP traffic is defined by the **[Themisto Shell](./../Themisto.md)** and can be summarized as follows:

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

Currently, only the **[Themisto Shell](./../Themisto.md)** contains the NAL. 

## Main Functionality

The NAL is controlled by the *FPGA Management Core* [*FMC*](../FMC/FMC.md) via an AXI4Lite bus. Through this connection, the NAL gets the list of allowed IP addresses for this Role, the mapping of IP addresses to *`NodeId`s*, and reports status as well as some debug information.

The split between management TCP traffic and "normal" user traffic is done by the **TSS**. For the TCP traffic, the FMC is connected via FIFOs (instantiated in `Shell.v`, so external to the NAL). 

To map *`NodeId`s* or *`rank`s* to IPv4 addresses, the NAL maintains a **Message Routing Table (MRT)**. This table is written via the AXI4 Lite controll link. The *MRT Agency* enables the access to this table via `request/reply` stream-pairs.

The *TCP Agency* keeps track of valid *TCP sessions* and translates them to IPv4 addresses and NodeIds or vice-versa, as requested by other cores via `request/reply` stream-pairs.

To handle the input of listen port request vectors (`po_rx_ports`, see above; `piTcpRxPorts` and `piUdpRxPorts` in Figure 1) for UDP and TCP, the *Port Open & Close* logic translates them in absolute opening or closing requests for the relevant UDP or TCP processes.

To reduce the latency of the `NodeId <--> IpAddress (<--> TCP session Id)` mapping, the USS and TSS are using *Caches*. The *Cache Invalidation Logic* detects conditions when those caches must be invalidated and signals this to all concerning processes (e.g. after a partial reconfiguration of the role, or the update of the MRT).

For debugging and monitoring, the USS and TSS systems create *events* (e.g. about a successful packet transmission, the meta data of the last connection, or an unauthorized access attempt) and send them to the *Status & Event Processing*. This process merges all those event notification, maps them to the correct status address in the AXI4 Lite address space, and forwards these updates to the AXI4Lite Processing Logic. (The events are handled with a best-effort methodology, because the NAL won't block any data stream if the events processing lags behind.)

The details of the TSS and USS can be found [here (TCP)](./TSS.md) and [here (UDP)](./USS.md). 

## HLS Coding Style and Naming Conventions

The design of **`NTS`** uses some specific HDL and HLS naming rules to ease the description and the understanding of
 its architecture. Please consider reading the two documents [**HDL coding style and naming conventions**](../hdl-naming-conventions.md)
  and [**HLS coding style and naming conventions**](./hls-naming-conventions.md) before diving into the code or starting
  to contribute to this part of the cloudFPGA project.

## Type definitions

The **`NTS`** is predominantly written in C/C++ with a few pieces coded in HDL (see Figure-1). The top-level that 
assembles and interconnects all the components, is the Verilog file [**nts_TcpIp.v**](../../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp.v). 
To facilitate the development and the interfacing with other C/C++ cores, we also provide the header file
 [**nts.hpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp).

For the components specified in C/C++, we use the Xilinx HLS flow to synthesize and export them as a standalone IP cores.
 With respect to Figure-1, the white boxes represent the sub-components specified in C/C++, the yellow boxes the ones
 specified in HDL, while blue boxes represent interfaces to neighbouring IP blocks.

## List of Components

The following table lists the sub-components of **`NTS`** and provides a link to their documentation as well as their
architecture body. 

| Entity              | Description                                      | Architecture                                                      |
|:-------------------:|:------------------------------------------------ |:----------------------------------------------------------------- |
| **[ARS](ARS.md)**   | Address Resolution Server (ARS).                 | [arp_server](../../SRA/LIB/SHELL/LIB/hls/NTS/arp/src/arp.cpp)     |
| **[ICMP](ICMP.md)** | Internet Control Message Protocol (ICMP) server. | [icmp_server](../../SRA/LIB/SHELL/LIB/hls/NTS/icmp/src/icmp.cpp)  |
| **[IPRX](IPRX.md)** | IP Receive frame handler (IPRX).                 | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp) |
| **[IPTX](IPTX.md)** | IP Transmit frame handler (IPTX).                | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iptx/src/iptx.cpp) |
| **[TOE](./TOE.md)** | TCP Offload Engine.                              | [toe](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.cpp)            |
| **[UOE](./UOE.md)** | UDP Offload Engine.                              | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)            |

## Description of the Interfaces

The entity declaration of **`NTS`** is specified as follows. It consists of 5 groups of interfaces referred to as:

* the [Memory Mapped IO Interface](#memory-mapped-io-interface) (MMIO), 
* the [Ethernet Data Link Layer-2 Interface](#ethernet-data-link-layer-2-interface) (ETH),
* the [TCP Application Layer Interface](#tcp-application-layer-interface) (TAIF),
* the [UDP Application Layer Interface](#udp-application-layer-interface) (UAIF),
* the [Memory System Interface](#memory-system-interface) (MEM),

```
module NetworkTransportStack_TcpIp (

  //------------------------------------------------------
  //-- Global Clock used by the entire SHELL
  //--   (This is typically 'sETH0_ShlClk' and we use it all over the place)
  //------------------------------------------------------ 
  input          piShlClk,
  //------------------------------------------------------
  //-- Global Reset used by the entire SHELL
  //--  This is typically 'sETH0_ShlRst'. If the module is created by HLS,
  //--   we use it as the default startup reset of the module.)
  //------------------------------------------------------ 
  input          piShlRst,
  //------------------------------------------------------
  //-- ETH / Ethernet Layer-2 Interfaces
  //------------------------------------------------------
  //--  Axi4-Stream Ethernet Rx Data --------
  input [ 63:0]  siETH_Data_tdata, 
  input [  7:0]  siETH_Data_tkeep,
  input          siETH_Data_tlast,
  input          siETH_Data_tvalid,
  output         siETH_Data_tready,
  //-- Axi4-Stream Ethernet Tx Data --------
  output [ 63:0] soETH_Data_tdata,
  output [  7:0] soETH_Data_tkeep,
  output         soETH_Data_tlast,
  output         soETH_Data_tvalid,
  input          soETH_Data_tready,
  //------------------------------------------------------
  //-- MEM / TxP Interfaces
  //------------------------------------------------------
  //-- FPGA Transmit Path / S2MM-AXIS --------------------
  //---- Axi4-Stream Read Command -----------
  output[ 79:0]  soMEM_TxP_RdCmd_tdata,
  output         soMEM_TxP_RdCmd_tvalid,
  input          soMEM_TxP_RdCmd_tready,
  //---- Axi4-Stream Read Status ------------
  input [  7:0]  siMEM_TxP_RdSts_tdata,
  input          siMEM_TxP_RdSts_tvalid,
  output         siMEM_TxP_RdSts_tready,
  //---- Axi4-Stream Data Input Channel ----
  input [ 63:0]  siMEM_TxP_Data_tdata,
  input [  7:0]  siMEM_TxP_Data_tkeep,
  input          siMEM_TxP_Data_tlast,
  input          siMEM_TxP_Data_tvalid,
  output         siMEM_TxP_Data_tready,
  //---- Axi4-Stream Write Command ----------
  output [ 79:0] soMEM_TxP_WrCmd_tdata,
  output         soMEM_TxP_WrCmd_tvalid,
  input          soMEM_TxP_WrCmd_tready,
  //---- Axi4-Stream Write Status -----------
  input [  7:0]  siMEM_TxP_WrSts_tdata,
  input          siMEM_TxP_WrSts_tvalid,
  output         siMEM_TxP_WrSts_tready,
  //---- Axi4-Stream Data Output Channel ----
  output [ 63:0] soMEM_TxP_Data_tdata,
  output [  7:0] soMEM_TxP_Data_tkeep,
  output         soMEM_TxP_Data_tlast,
  output         soMEM_TxP_Data_tvalid,
  input          soMEM_TxP_Data_tready,
  //------------------------------------------------------
  //-- MEM / RxP Interfaces
  //------------------------------------------------------
  //-- FPGA Receive Path / S2MM-AXIS -------------
  //---- Axi4-Stream Read Command -----------
  output [ 79:0] soMEM_RxP_RdCmd_tdata,
  output         soMEM_RxP_RdCmd_tvalid,
  input          soMEM_RxP_RdCmd_tready,
  //---- Axi4-Stream Read Status ------------
  input [   7:0] siMEM_RxP_RdSts_tdata,
  input          siMEM_RxP_RdSts_tvalid,
  output         siMEM_RxP_RdSts_tready,
  //---- Axi4-Stream Data Input Channel -----
  input [ 63:0]  siMEM_RxP_Data_tdata,
  input [  7:0]  siMEM_RxP_Data_tkeep,
  input          siMEM_RxP_Data_tlast,
  input          siMEM_RxP_Data_tvalid,
  output         siMEM_RxP_Data_tready,
  //---- Axi4-Stream Write Command ----------
  output[ 79:0]  soMEM_RxP_WrCmd_tdata,
  output         soMEM_RxP_WrCmd_tvalid,
  input          soMEM_RxP_WrCmd_tready,
  //---- Axi4-Stream Write Status -----------
  input [  7:0]  siMEM_RxP_WrSts_tdata,
  input          siMEM_RxP_WrSts_tvalid,
  output         siMEM_RxP_WrSts_tready,
  //---- Axi4-Stream Data Input Channel -----
  output [ 63:0] soMEM_RxP_Data_tdata,
  output [  7:0] soMEM_RxP_Data_tkeep,
  output         soMEM_RxP_Data_tlast,
  output         soMEM_RxP_Data_tvalid,
  input          soMEM_RxP_Data_tready,
  //------------------------------------------------------
  //-- UAIF / UDP Tx Data Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ---------------
  input   [63:0] siAPP_Udp_Data_tdata,
  input   [ 7:0] siAPP_Udp_Data_tkeep,
  input          siAPP_Udp_Data_tlast,
  input          siAPP_Udp_Data_tvalid,
  output         siAPP_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata -----------
  input   [95:0] siAPP_Udp_Meta_tdata,
  input          siAPP_Udp_Meta_tvalid,
  output         siAPP_Udp_Meta_tready,
  //---- Axi4-Stream UDP Data Length ---------
  input   [15:0] siAPP_Udp_DLen_tdata,
  input          siAPP_Udp_DLen_tvalid,
  output         siAPP_Udp_DLen_tready,
  //------------------------------------------------------
  //-- UAIF / Rx Data Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Data ---------------
  output  [63:0] soAPP_Udp_Data_tdata,
  output  [ 7:0] soAPP_Udp_Data_tkeep,
  output         soAPP_Udp_Data_tlast,
  output         soAPP_Udp_Data_tvalid,
  input          soAPP_Udp_Data_tready,
  //---- Axi4-Stream UDP Metadata -----------
  output  [95:0] soAPP_Udp_Meta_tdata,
  output         soAPP_Udp_Meta_tvalid,
  input          soAPP_Udp_Meta_tready,
  //------------------------------------------------------
  //-- UAIF / UDP Rx Ctrl Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //---- Axi4-Stream UDP Listen Request -----
  input   [15:0] siAPP_Udp_LsnReq_tdata,
  input          siAPP_Udp_LsnReq_tvalid,
  output         siAPP_Udp_LsnReq_tready,
  //---- Axi4-Stream UDP Listen Reply -------
  output  [ 7:0] soAPP_Udp_LsnRep_tdata,
  output         soAPP_Udp_LsnRep_tvalid,
  input          soAPP_Udp_LsnRep_tready,
  //---- Axi4-Stream UDP Close Request ------
  input   [15:0] siAPP_Udp_ClsReq_tdata,
  input          siAPP_Udp_ClsReq_tvalid,
  output         siAPP_Udp_ClsReq_tready,
  //---- Axi4-Stream UDP Close Reply --------
  output  [ 7:0] soAPP_Udp_ClsRep_tdata,
  output         soAPP_Udp_ClsRep_tvalid,
  input          soAPP_Udp_ClsRep_tready,
  //------------------------------------------------------
  //-- TAIF / Tx Data Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream APP Data ---------------
  input [ 63:0]  siAPP_Tcp_Data_tdata,
  input [  7:0]  siAPP_Tcp_Data_tkeep,
  input          siAPP_Tcp_Data_tvalid,
  input          siAPP_Tcp_Data_tlast,
  output         siAPP_Tcp_Data_tready,
  //---- Axi4-Stream APP Send Request -------
  input [ 31:0]  siAPP_Tcp_SndReq_tdata,
  input          siAPP_Tcp_SndReq_tvalid,
  output         siAPP_Tcp_SndReq_tready,
  //---- Axi4-Stream APP Send Reply ---------
  output [ 55:0] soAPP_Tcp_SndRep_tdata,
  output         soAPP_Tcp_SndRep_tvalid,
  input          soAPP_Tcp_SndRep_tready,
  //------------------------------------------------------
  //-- TAIF / Rx Data Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //-- Axi4-Stream TCP Data -----------------
  output [ 63:0] soAPP_Tcp_Data_tdata,
  output [  7:0] soAPP_Tcp_Data_tkeep,
  output         soAPP_Tcp_Data_tvalid,
  output         soAPP_Tcp_Data_tlast,
  input          soAPP_Tcp_Data_tready,
  //--  Axi4-Stream TCP Metadata ------------
  output [ 15:0] soAPP_Tcp_Meta_tdata,
  output         soAPP_Tcp_Meta_tvalid,
  input          soAPP_Tcp_Meta_tready,
  //--  Axi4-Stream TCP Data Notification ---
  output [103:0] soAPP_Tcp_Notif_tdata,
  output         soAPP_Tcp_Notif_tvalid,
  input          soAPP_Tcp_Notif_tready,
  //--  Axi4-Stream TCP Data Request --------
  input  [ 31:0] siAPP_Tcp_DReq_tdata,
  input          siAPP_Tcp_DReq_tvalid,
  output         siAPP_Tcp_DReq_tready,
  //------------------------------------------------------
  //-- TAIF / Tx Ctlr Interfaces (.i.e APP-->NTS)
  //------------------------------------------------------
  //---- Axi4-Stream TCP Open Session Request
  input [ 47:0]  siAPP_Tcp_OpnReq_tdata,
  input          siAPP_Tcp_OpnReq_tvalid,
  output         siAPP_Tcp_OpnReq_tready,
  //---- Axi4-Stream TCP Open Session Reply
  output [ 23:0] soAPP_Tcp_OpnRep_tdata,
  output         soAPP_Tcp_OpnRep_tvalid,
  input          soAPP_Tcp_OpnRep_tready,
  //---- Axi4-Stream TCP Close Request ------
  input [ 15:0]  siAPP_Tcp_ClsReq_tdata,
  input          siAPP_Tcp_ClsReq_tvalid,
  output         siAPP_Tcp_ClsReq_tready,
  //------------------------------------------------------
  //-- TAIF / Rx Ctlr Interfaces (.i.e NTS-->APP)
  //------------------------------------------------------
  //----  Axi4-Stream TCP Listen Request ----
  input [ 15:0]  siAPP_Tcp_LsnReq_tdata,
  input          siAPP_Tcp_LsnReq_tvalid,
  output         siAPP_Tcp_LsnReq_tready,
  //----  Axi4-Stream TCP Listen Ack --------
  output [  7:0] soAPP_Tcp_LsnRep_tdata,  // RepBit stream must be 8-bits boundary
  output         soAPP_Tcp_LsnRep_tvalid,
  input          soAPP_Tcp_LsnRep_tready,
  //------------------------------------------------------
  //-- MMIO / Interfaces
  //------------------------------------------------------
  input          piMMIO_Layer2Rst,
  input          piMMIO_Layer3Rst,
  input          piMMIO_Layer4Rst,
  input          piMMIO_Layer4En,
  input  [ 47:0] piMMIO_MacAddress,
  input  [ 31:0] piMMIO_Ip4Address,
  input  [ 31:0] piMMIO_SubNetMask,
  input  [ 31:0] piMMIO_GatewayAddr,
  output         poMMIO_CamReady,
  output         poMMIO_NtsReady

);
```

### Memory Mapped IO Interface

The memory mapped IO (MMIO) interface consists of a set of status and configuration IO signals.

```
    //------------------------------------------------------
    //-- MMIO / Interfaces
    //------------------------------------------------------
    input          piMMIO_Layer2Rst,
    input          piMMIO_Layer3Rst,
    input          piMMIO_Layer4Rst,
    input          piMMIO_Layer4En,
    input  [ 47:0] piMMIO_MacAddress,
    input  [ 31:0] piMMIO_Ip4Address,
    input  [ 31:0] piMMIO_SubNetMask,
    input  [ 31:0] piMMIO_GatewayAddr,
    output         poMMIO_CamReady,
    output         poMMIO_NtsReady
```

* `piMMIO_Layer2Rst`, `piMMIO_Layer3Rst` and `piMMIO_Layer4Rst` are active *high* reset signals. The layer number
    corresponds to one of the seven layers of the conceptual OSI network model.
* `piMMIO_Layer4En` is an active *high* enable signal. It can be used to delay the start of the transport layer #4.
* `piMMIO_MacAddress`, `piMMIO_Ip4Address`, `piMMIO_SubNetMask` and `piMMIO_GatewayAddr` are used to configure the *MAC
    address, the *IPv4 address*, the *IP subnet mask* and the *default gateway address* of **`NTS`**, respectively. All
    the addresses must be encoded in network byte oder (i.e. in big-endian order).
* `poMMIO_CamReady` and `poMMIO_NtsReady` are active *high* ready signals. They indicate the readiness of **`NTS`**.

### Ethernet Data Link Layer-2 Interface

The Ethernet data link layer-2 interface (ETH) connects **`NTS`** to a 10 Gigabit Ethernet Media Access Controller
 (10GEMAC). The interface consists of a receive and a transmit path, each of them being implemented as an *AXI4-Stream
  interface* of 64 bits.

* `siETH_Data_****` is used to receive Ethernet MAC frames from the data link layer-2.
* `soETH_Data_****` is used to send Ethernet MAC frames to the data link layer-2.

### TCP Application Layer Interface

The TCP application layer interface (TAIF) connects **`NTS`** to a network presentation layer such as a TLS process,
 a network application layer such as an HTTP server, or directly to a user application. It consists of a set of
 receive and transmit interfaces, each of them being implemented as an *AXI4-Stream interface*.

* `siAPP_Tcp_****` signals implement the transmit data and control paths between the application layer and **`NTS`** (i.e., 
   the outgoing network traffic direction). 
* `soAPP_Tcp_****` signals implement the receive data and control paths between **`NTS`** and the application layer (i.e., 
   the incoming network traffic direction).

Refer to the [TCP Application Layer Interface of TOE](./TOE.md#tcp-application-layer-interface) for a detailed description
of these interfaces. 

### UDP Application Layer Interface

The UDP application layer interface (UAIF) connects **`NTS`** to a network application layer or directly to a user
 application. It consists of a set of receive and transmit interfaces, each of them being implemented as an *AXI4-Stream
 interface*.

* `siAPP_Udp_****` signals implement the transmit data and control paths between the application layer and **`NTS`** (i.e., 
    the outgoing network traffic direction). 
  * `soAPP_Udp_****` signals implement the receive data and control paths between **`NTS`** and the application layer (i.e., 
    the incoming network traffic direction).

Refer to the [UDP Application Layer Interface of UOE](./UOE.md#udp-application-layer-interface) for a detailed description
of these interfaces. 

### Memory System Interface

[TODO - Under construction]
