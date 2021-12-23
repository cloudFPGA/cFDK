# TCP Offload Engine (TOE)

This document describes the design of the **TCP Offload Engine (TOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initially developed by **Xilinx Dublin Labs, Ireland** who kindly accepted to share it with the *cloudFPGA
project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018,
the code was ported and adapted for the needs and specificity of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be
 found [here](https://github.com/fpgasystems/fpga-network-stack).    

## Overview
A block diagram of **`TOE`** is depicted in Figure 1. It features an *Rx Engine (RXe)* to handle the incoming data path
from the IP layer, a *Tx Engine (TXe)* to assemble outgoing data packets for the IP layer, and a set of TCP state- and
data-keeping engines.

![Block diagram of the TOE](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-TOE-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the TCP Offload Engine</b></p>

The TCP offload engine is entirely written in C/C++, and the Xilinx HLS flow is used here to synthesize and export **`TOE`** 
as a standalone IP core. With respect to Figure-1, every white box represents a sub-component of **`TOE`**, while gray boxes
represent interfaces to neighbouring IP blocks.

The top-level file [**toe.cpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.cpp) defines the architecture body of **`TOE`**
while its header counterpart [**toe.hpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.hpp) defines the entity interfaces
of **`TOE`**.
 
## HLS Coding Style and Naming Conventions
The HLS design of **`TOE`** uses some specific naming rules to ease the description and the understanding of its architecture. 
Please consider reading the document [**HLS coding style and naming conventions**](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/ ./hls-naming-conventions.md) before 
diving or contributing to this part of the cloudFPGA project.

## List of Components

The following table lists the sub-components of **`TOE`** and provides a link to their documentation as well as their
architecture body. 

| Entity                  | Description                | Architecture
|:------------------------|:---------------------------|:--------------
| **[AKd](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./AKd.md)**     | AcK delayer                | [ack_delay](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/ack_delay/ack_delay.cpp)
| **[EVe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./EVe.md)**     | EVent engine               | [event_engine](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/event_engine/event_engine.cpp)
| **[PRt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./PRt.md)**     | PoRt table                 | [port_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/      ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/port_table/port_table.cpp)
| **[RAi](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./RAi.md)**     | Rx Application interface   | [tx_app_interface](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_app_interface/rx_app_interface.cpp)
| **[RSt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./RSt.md)**     | Rx Sar table               | [rx_sar_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_sar_table/rx_sar_table.cpp)
| **[RXe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./RXe.md)**     | RX engine                  | [rx_engine](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **[SLc](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./SLc.md)**     | Session Lookup controller  | [session_lookup_controller](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **[STt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./STt.md)**     | STate table                | [state_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/     ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/state_table/state_table.cpp)
| **[TAi](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TAi.md)**     | Tx Application interface   | [tx_app_interface](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_app_interface/tx_app_interface.cpp)
| **[TIm](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TIm.md)**     | TImers                     | [timers](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/          ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/timers/timers.cpp)
| **[TSt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TSt.md)**     | Tx Sar table               | [tx_sar_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_sar_table/tx_sar_table.cpp)
| **[TXe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TXe.md)**     | TX engine                  | [tx_engine](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_engine/src/tx_engine.cpp)

## Description of the Interfaces

The entity declaration of **`TOE`** is specified as follows. It consists of 6 groups of interfaces referred to as:
 * the [Memory Mapped IO Interface](#memory-mapped-io-interface) (MMIO), 
 * the [IP Network Layer-3 Interface](#ip-network-layer-3-interface) (IPRX and IPTX),
 * the [TCP Application Layer Interface](#tcp-application-layer-interface) (TAIF),
 * the [Memory System Interface](#memory-system-interface) (MEM),
 * the [Content Addressable Memory Interface](#content-addressable-memory-interface) (CAM),
 * the [Implementation Dependent Block Interface](#implementation-dependent-block-interface).

```C
void toe(
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------
    Ip4Addr                                  piMMIO_IpAddr,
    StsBit                                  &poNTS_Ready,
    //------------------------------------------------------
    //-- IPRX / IP Rx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &siIPRX_Data,
    //------------------------------------------------------
    //-- IPTX / IP Tx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &soIPTX_Data,
    //------------------------------------------------------
    //-- TAIF / Receive Data Interfaces
    //------------------------------------------------------
    stream<TcpAppNotif>                     &soTAIF_Notif,
    stream<TcpAppRdReq>                     &siTAIF_DReq,
    stream<TcpAppData>                      &soTAIF_Data,
    stream<TcpAppMeta>                      &soTAIF_Meta,
    //------------------------------------------------------
    //-- TAIF / Listen Interfaces
    //------------------------------------------------------
    stream<TcpAppLsnReq>                    &siTAIF_LsnReq,
    stream<TcpAppLsnRep>                    &soTAIF_LsnRep,
    //------------------------------------------------------
    //-- TAIF / Send Data Interfaces
    //------------------------------------------------------
    stream<TcpAppData>                      &siTAIF_Data,
    stream<TcpAppSndReq>                    &siTAIF_SndReq,
    stream<TcpAppSndRep>                    &soTAIF_SndRep,
    //------------------------------------------------------
    //-- TAIF / Open Connection Interfaces
    //------------------------------------------------------
    stream<TcpAppOpnReq>                    &siTAIF_OpnReq,
    stream<TcpAppOpnRep>                    &soTAIF_OpnRep,
    //------------------------------------------------------
    //-- TAIF / Close Interfaces
    //------------------------------------------------------
    stream<TcpAppClsReq>                    &siTAIF_ClsReq,
    //-- Not Used                           &soTAIF_ClsSts,
    //------------------------------------------------------
    //-- MEM / Rx PATH / S2MM Interface
    //------------------------------------------------------
    //-- Not Used                           &siMEM_RxP_RdSts,
    stream<DmCmd>                           &soMEM_RxP_RdCmd,
    stream<AxisApp>                         &siMEM_RxP_Data,
    stream<DmSts>                           &siMEM_RxP_WrSts,
    stream<DmCmd>                           &soMEM_RxP_WrCmd,
    stream<AxisApp>                         &soMEM_RxP_Data,
    //------------------------------------------------------
    //-- MEM / Tx PATH / S2MM Interface
    //------------------------------------------------------
    //-- Not Used                           &siMEM_TxP_RdSts,
    stream<DmCmd>                           &soMEM_TxP_RdCmd,
    stream<AxisApp>                         &siMEM_TxP_Data,
    stream<DmSts>                           &siMEM_TxP_WrSts,
    stream<DmCmd>                           &soMEM_TxP_WrCmd,
    stream<AxisApp>                         &soMEM_TxP_Data,
    //------------------------------------------------------
    //-- CAM / Session Lookup & Update Interfaces
    //------------------------------------------------------
    stream<CamSessionLookupRequest>         &soCAM_SssLkpReq,
    stream<CamSessionLookupReply>           &siCAM_SssLkpRpl,
    stream<CamSessionUpdateRequest>         &soCAM_SssUpdReq,
    stream<CamSessionUpdateReply>           &siCAM_SssUpdRpl
);
``` 

Note that the entity declaration of **`TOE`** does not contain any _clock_ or _reset_ port. These ports are created by
the Vivado HLS synthesis tool and are described in the [Implementation Dependent Block Interfaces](#implementation-dependent-bock-interfaces) section.    

### Memory Mapped IO Interface
The memory mapped IO (MMIO) interface consists of a set of status and configuration IO signals.
```
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------
    Ip4Addr                                  piMMIO_IpAddr,
    StsBit                                  &poNTS_Ready,
```
* `piMMIO_IpAddr` is used to configure the IPv4 address of **`TOE`**. It implements a *scalar input port* of type
    \<[Ip4Addr](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp)\> which encodes the IPv4 address in network byte oder
    (i.e. in big-endian order).
* `poNTS_Ready` is used to indicate the readiness of **`TOE`**. It implements a *scalar output port* of type
    \<[StsBit](../../SRA/LIB/SHELL/LIB/hls/NTS/nts_types.hpp)\> which is set _high_ when **`TOE`** is initialised and
    ready for operations on all of its interfaces.

### IP Network Layer-3 Interface
The IP network layer-3 interface consists of a receive (IPRX) and a transmit (IPTX) interface. 
```
    //------------------------------------------------------
    //-- IPRX / IP Rx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &siIPRX_Data,
    //------------------------------------------------------
    //-- IPTX / IP Tx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &soIPTX_Data,
```

* `siIPRX_Data` is used to receive IPv4 packets from the IP network layer-3. It implements an *AXI4-Stream
  interface* of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp)\>. 
* `soIPTX_Data` is used to send IPv4 packets to the IP network layer-3. It implements an *AXI4-Stream
  interface* of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp)\>. 

### TCP Application Layer Interface
The TCP application layer interface (TAIF) connects **`TOE`** to a network presentation layer such as a *TLS process*, 
a network application layer such as an *HTTP server*, or directly to the user application itself. It consists of a 
receive and a transmit path, each path further containing a data and a control sub-interface.
* [RxAppData](#rx-application-data-interface) implements the receive data path between **`TOE`** and the application
  layer (i.e., the incoming network traffic direction) and [RxAppCtrl](#rx-application-control-interface) manages the
  receive control path.
* [TxAppData](#tx-application-data-interface) implements the transmit data path between the application layer and
  **`TOE`** (i.e., the outgoing network traffic direction) and [TxAppCtrl](#tx-application-control-interface) manages
   the control path.
   
#### Rx Application Control Interface
In order for the application (APP) to receive data over a connection initiated by a remote node, the **`APP`** must first 
request **`TOE`** to open a TCP port in passive listening mode. Once a port is set in passive open mode, **`TOE`** can
accept a remote connection request and can create a session for it. The same session can latter be used by **`APP`**
to send data back to the remote node.
```
    //------------------------------------------------------
    //-- TAIF / Listen Interfaces
    //------------------------------------------------------
    stream<TcpAppLsnReq>                    &siTAIF_LsnReq,
    stream<TcpAppLsnRep>                    &soTAIF_LsnRep,
```
* `siTAIF_LsnReq` is used by the **`APP`** to request a TCP port number to be opened in passive listening mode.
  It implements an *AXI4-Stream interface* of type \<[TcpPort](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisTcp.hpp)\>.
* `soTAIF_LsnRep` is a status reply returned by **`TOE`** upon a listen port request. It implements an *AXI4-Stream 
   interface* of type \<[StsBool](../../SRA/LIB/SHELL/LIB/hls/NTS/nts_types.hpp)\>.
  * Warning: This reply stream is operated in non-blocking mode to avoid any stalling of the listen interface. This
    means that **`APP`** must provision sufficient buffering to store the listen reply returned by **`TOE`**.

#### Rx Application Data Interface
The data transfer between **`TOE`** and **`APP`** is synchronized by the following handshake protocol. First, **`APP`**
issues a request to retrieve data from the Rx buffer of a TCP session. Next, **`TOE`** sends the requested number of
bytes to **`APP`** along with some metadata information about the retrieved data. In parallel, **`TOE`** notifies 
**`APP`** whenever new data arrived into the Rx buffer of a TCP session; this is somehow similar to an interrupt.
```
    //------------------------------------------------------
    //-- TAIF / Receive Data Interfaces
    //------------------------------------------------------
    stream<TcpAppNotif>                     &soTAIF_Notif,
    stream<TcpAppRdReq>                     &siTAIF_DReq,
    stream<TcpAppData>                      &soTAIF_Data,
    stream<TcpAppMeta>                      &soTAIF_Meta,
```
* `soTAIF_Notif` notifies **`APP`** of the arrival of new data in the Rx buffer of a TCP session. It implements an
    *AXI4-Stream interface* of type \<[TcpAppNotif](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the 
    id-number of the session, the number of received bytes, the TCP destination port, the IP source address, the TCP
    source port and the TCP state of the connection.  
* `siTAIF_DReq` is used by **`APP`** to request data from the TCP Rx buffer. It implements an *AXI4-Stream interface*
    of type \<[TcpAppRdReq](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the number of bytes and the 
    id-number of the session to read from. 
* `soTAIF_Data` is the data stream forwarded by **`TOE`** upon a data read request. It implements an *AXI4-Stream
    interface* of type \<[TcpAppData](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisApp.hpp)\> and its length equals the one
    request by **`APP`**.
* `soTAIF_Meta` is the metadata of the data stream forwarded by **`TOE`** upon a data read request. It implements an
    *AXI4-Stream interface* of type \<[TcpAppMeta](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the
    id-number or the forwarded data stream.
  
Warning:
  To avoid any stalling of the receive data path, the outgoing streams `soTAIF_Notif`, `soTAIF_Data` and `soTAIF_Meta`
  are operated in non-blocking mode. This implies that the user application process connected to these streams must
  provision enough buffering to store the corresponding bytes exchanged on these interfaces.

#### Tx Application Control Interface
In order for **`APP`** to initiate a connection with a remote node, the **`APP`** must first request **`TOE`** to open
an active socket. Once the connection is established by **`TOE`**, an id-number that identifies the session is
returned to **`APP`** for use in further sending operations.
```
    //------------------------------------------------------
    //-- TAIF / Open Connection Interfaces
    //------------------------------------------------------
    stream<TcpAppOpnReq>                    &siTAIF_OpnReq,
    stream<TcpAppOpnRep>                    &soTAIF_OpnRep,
```
* `siTAIF_OpnReq` is used by **`APP`** to request the opening of an active socket. It implements an *AXI4-Stream
    interface* of type \<[SockAddr](../../SRA/LIB/SHELL/LIB/hls/NTS/nts_types.hpp)\> which specifies the remote IP
    address and remote TCP port to open.
* `soTAIF_OpnRep` is a status reply returned by **`TOE`** upon an open socket request. It implements an *AXI4-Stream 
   interface* of type \<[TcpAppOpnRep](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the id-number and the
   TCP state of the newly opened connection.
  * Warning: This reply stream is operated in non-blocking mode to avoid any stalling of the open connection interface.
    This means that **`APP`** must provision sufficient buffering to store the listen reply returned by **`TOE`**.

#### Tx Application Data Interface
The data transfer between **`APP`** and **`TOE`** is synchronized by the following handshake protocol. First, **`APP`**
issues a request to send data for a specific TCP session. Second, **`TOE`** replies with the number of bytes that is 
willing to accept from this request. Third, **`APP`** forwards a stream of bytes which length corresponds to the number
of bytes that were granted during the handshaking process.
```
    //------------------------------------------------------
    //-- TAIF / Send Data Interfaces
    //------------------------------------------------------
    stream<TcpAppSndReq>                    &siTAIF_SndReq,
    stream<TcpAppSndRep>                    &soTAIF_SndRep,
    stream<TcpAppData>                      &siTAIF_Data,
```
* `siTAIF_SndReq` is used by **`APP`** for requesting to send data to **`TOE`**. It implements an *AXI4-Stream interface*
    of type \<[TcpAppSndReq](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the id-number of the session and
    the length of the data stream to transmit.  
* `soTAIF_SndRep` is a status reply returned by **`TOE`** upon a data send request. It implements an *AXI4-Stream 
    interface* of type \<[TcpAppSndRep](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the id-number of the
    session, the number of bytes that were requested to be sent by **`APP`**, the number of bytes that **`TOE`** is 
    willing to accept based on the space left in the Tx memory buffer of this session, and a handshake return code.
    * Warning: This reply stream is operated in non-blocking mode. Therefore, **`APP`** must always provision enough
       buffering to store such a reply.
* `siTAIF_Data` is the data stream forwarded by **`APP`** following the transmission handshake process. It implements an
    *AXI4-Stream interface* of type \<[TcpAppData](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisApp.hpp)\> and its length equals
    the one negotiated by the handshaking process.

### Memory System Interface
  [TODO - Under construction]
  
### Content Addressable Memory Interface
  [TODO - Under construction]

### Implementation Dependent Block Interface
This section describes the vendor dependent implementations details. Xilinx Vivado HLS typically creates three types of
 ports on the RTL design: an input clock, an input reset and some optional block-level handshake signals.
 * `aclk` is a 156.25MHz input clock for the operation of **`TOE`**.
 * `aresetn` is an active _low_ synchronous (w/ `aclk`) reset signal.
 
 Warning: 
 * Note that the design of **`TOE`** is specified to use the block-level IO protocol referred to as `ap_ctrl_none`.
    This mode prevents the RTL to create and implement block-level handshake signals such as `ap_start`, `ap_idle`,
    `ap_ready` and  `ap_done` which are typically used to specify when the design can start to perform an operation,
     when the operation ends and when the design is idle and ready for new inputs.
    
## HowTo HLS Design Flow
This section summarizes the compilation, simulation, synthesize and packaging steps of **`TOE`** with the Xilinx Vivado
 HLS tools.
 
The source files of **`TOE`** are located under [cFDK/LIB/SRA/LIB/Shell/hls/NTS/toe](../../SRA/LIB/SHELL/LIB/hls/NTS/toe).

A makefile provides the following commands:
* `make project` to create an HLS project for **`TOE`**. 
* `make csim` to run a C simulation.
* `make csynth` to run the HLS C synthesis.
* `make cosim` to run the HLS C/RTL co-simulation.
* `make rtlSyn`  to run an RTL synthesis to obtain more accurate timing and and utilization numbers.
* `make rtlImpl` to run perform both RTL synthesis and implementation including detailed place and route.
* `make regression` to run csim, csynth and cosim in sequence.
* `make help` to print this list of commands.
