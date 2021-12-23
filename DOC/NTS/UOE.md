# UDP Offload Engine (UOE)

This document describes the design of the **UDP Offload Engine (UOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initially developed by **Xilinx Dublin Labs, Ireland** who kindly accepted to share it with the *cloudFPGA
 project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, 
 the code was ported and adapted for the needs and specificity of the *cloudFPGA* platform.

## Overview
The **`UOE`** consists of a *RxEngine (RXe)* to handle the incoming data packets from the IP layer, and a *TxEngine
 (TXe)* to assemble outgoing data packets for the IP layer. **`RXe`** is depicted in Figure-1 and **`TXe`** is depicted 
 in Figure-2. 
![Block diagram of the RXe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-UOE-RXe-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Rx Engine of the UDP Offload Engine</b></p>

![Block diagram of the TXe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-UOE-TXe-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-2: Block diagram of the Tx Engine of the UDP Offload Engine</b></p>

The UDP offload engine is entirely written in C/C++, and the Xilinx HLS flow is used here to synthesize and export 
 **`UOE`** as a standalone IP core. With respect to Figure-1 and Figure-2, every white box represents a sub-process 
 of **`RXe`** respectively **`TXe`**, while gray boxes represent interfaces to neighbouring IP blocks.

The top-level file [**uoe.cpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp) defines the architecture body of 
 **`UOE`** while its header counterpart [**uoe.hpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.hpp) defines the 
 entity interfaces of **`UOE`**.

## HLS Coding Style and Naming Conventions
The HLS design of **`UOE`** uses some specific naming rules to ease the description and the understanding of its 
 architecture. Please consider reading the document [**HLS coding style and naming conventions**](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/ ./hls-naming-conventions.md) before 
 diving or contributing to this part of the cloudFPGA project.

## List of Processes
The following table lists the sub-processes of the components **`UOE/RXe`** and **`UOE/TXe`**.

|Component| Process | Description                | File
|:-------:|:-------:|:---------------------------|:--------------
| **RXe** |  *Ihs*  | Ip Header Stripper         | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **RXe** |  *Rph*  | Rx Packet Handler          | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **RXe** |  *Ucc*  | Udp Checksum Checker       | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **RXe** |  *Upt*  | Udp Port Table             | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
|         |         |                            |
| **TXe** |  *Tai*  | Tx Application Interface   | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **TXe** |  *Tdh*  | Tx Datagram Handler        | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **TXe** |  *Uca*  | Udp Checksum Accumulator   | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **TXe** |  *Uha*  | Udp Header Adder           | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)
| **TXe** |  *Iha*  | Ip Header Adder            | [uoe](../../SRA/LIB/SHELL/LIB/hls/NTS/uoe/src/uoe.cpp)

## Description of the Interfaces
The entity declaration of **`UOE`** is specified as follows. It consists of 4 groups of interfaces referred to as:
 * the [Memory Mapped IO Interface](#memory-mapped-io-interface) (MMIO), 
 * the [IP Network Layer-3 Interface](#ip-network-layer-3-interface) (IPRX and IPTX),
 * the [UDP Application Layer Interface](#udp-application-layer-interface) (UAIF),
 * the [Implementation Dependent Block Interface](#implementation-dependent-block-interface).

```C++
void uoe(
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    CmdBit                           piMMIO_En,
    stream<StsBool>                 &soMMIO_Ready,
    //------------------------------------------------------
    //-- IPRX / IP Rx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                 &siIPRX_Data,
    //------------------------------------------------------
    //-- IPTX / IP Tx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                 &soIPTX_Data,
    //------------------------------------------------------
    //-- UAIF / Control Port Interfaces
    //------------------------------------------------------
    stream<UdpAppLsnReq>            &siUAIF_LsnReq,
    stream<UdpAppLsnRep>            &soUAIF_LsnRep,
    stream<UdpAppClsReq>            &siUAIF_ClsReq,
    stream<UdpAppClsRep>            &soUAIF_ClsRep,
    //------------------------------------------------------
    //-- UAIF / Rx Data Interfaces
    //------------------------------------------------------
    stream<UdpAppData>              &soUAIF_Data,
    stream<UdpAppMeta>              &soUAIF_Meta,
    stream<UdpAppDLen>              &soUAIF_DLen,
    //------------------------------------------------------
    //-- UAIF / Tx Data Interfaces
    //------------------------------------------------------
    stream<UdpAppData>              &siUAIF_Data,
    stream<UdpAppMeta>              &siUAIF_Meta,
    stream<UdpAppDLen>              &siUAIF_DLen,
    //------------------------------------------------------
    //-- ICMP / Message Data Interface (Port Unreachable)
    //------------------------------------------------------
    stream<AxisIcmp>                &soICMP_Data
);
```

Note that the entity declaration of **`UOE`** does not contain any _clock_ or _reset_ port. These ports are created by
 the Vivado HLS synthesis tool and are described in the [Implementation Dependent Block Interfaces](#implementation-dependent-bock-interfaces) 
 section.    

### Memory Mapped IO Interface
The memory mapped IO (MMIO) interface consists of a set of status and configuration IO signals.
```
    //------------------------------------------------------
    //-- MMIO Interface
    //------------------------------------------------------
    CmdBit                           piMMIO_En,
    stream<StsBool>                 &soMMIO_Ready,
```
* `piMMIO_En` is an active *high* enable signal used to delay the start of **`UOE`** after a reset. It implements a 
    *scalar input port* of type \<[CmdBit](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/nts_types.hpp)\>.
* `soMMIO_Ready` is used to indicate the readiness of **`UOE`**. It implements an *AXI4-Stream interface* of type 
    \<[StsBool](../../SRA/LIB/SHELL/LIB/hls/nts_types.hpp)\>.

### IP Network Layer-3 Interface
The IP network layer-3 interface consists of a receive (IPRX) and a transmit (IPTX) interface. 
```
    //------------------------------------------------------
    //-- IPRX / IP Rx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                 &siIPRX_Data,
    //------------------------------------------------------
    //-- IPTX / IP Tx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                 &soIPTX_Data,
```
* `siIPRX_Data` is used to receive IPv4 packets from the IP network layer-3. It implements an *AXI4-Stream
  interface* of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp)\>. 
* `soIPTX_Data` is used to send IPv4 packets to the IP network layer-3. It implements an *AXI4-Stream
  interface* of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp)\>. 
  
### UDP Application Layer Interface 
The UDP application layer interface (UAIF) connects **`UOE`** to a network application layer or directly to a user 
 application. It consists of a receive data path, a transmit data path and a control interface.
* [RxAppData](#rx-application-data-interface) implements the receive data path between **`UOE`** and the application
  layer (i.e., the incoming network traffic direction).
* [TxAppData](#tx-application-data-interface) implements the transmit data path between the application layer and
  **`UOE`** (i.e., the outgoing network traffic direction).
* [AppCtrl](#application-control-interface) manages the control path.
  
#### Application Control Interface
In order for the application (APP) to receive data from a remote node, the **`APP`** must first request **`UOE`** to 
 open a UDP port in listening mode. Only when a port is set in open mode, will **`UOE`** start accepting data for it. 
 Otherwise, if a port is closed, all data traffic destined to it is dropped.
```
    //------------------------------------------------------
    //-- UAIF / Control Port Interfaces
    //------------------------------------------------------
    stream<UdpAppLsnReq>            &siUAIF_LsnReq,
    stream<UdpAppLsnRep>            &soUAIF_LsnRep,
    stream<UdpAppClsReq>            &siUAIF_ClsReq,
    stream<UdpAppClsRep>            &soUAIF_ClsRep,
```
* `siUAIF_LsnReq` is used by the **`APP`** to request a UDP port number to be opened in listening mode. It implements 
    an *AXI4-Stream interface* of type \<[UdpPort](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisUdp.hpp)\>.
* `soUAIF_LsnRep` is a status reply returned by **`UOE`** upon a listen port request. It implements an *AXI4-Stream 
   interface* of type \<[StsBool](../../SRA/LIB/SHELL/LIB/hls/NTS/nts_types.hpp)\>.
* `siUAIF_ClsReq` is used by the **`APP`** to request a UDP port number to be closed. It implements an *AXI4-Stream 
    interface* of type \<[UdpPort](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisUdp.hpp)\>.
* `soUAIF_ClsRep` is a status reply returned by **`UOE`** upon a close port request. It implements an *AXI4-Stream 
   interface* of type \<[StsBool](../../SRA/LIB/SHELL/LIB/hls/NTS/nts_types.hpp)\>.
  
#### Rx Application Data Interface
The data transfer between **`UOE`** and **`APP`** is flow-controlled by the stream based operation of the interface, 
 but the data transfer between the Ethernet network and the **`UOE`** is not. Therefore, since **`UOE`** comes with 
 limited input buffering (~16KB), it is mandatory for the **`APP`** to read the incoming data provided by **`UOE`** 
 as soon possible. Otherwise, the input buffer of **`UOE`** will fill up and once full, every new incoming IP packet
 will be dropped. 
```
    //------------------------------------------------------
    //-- UAIF / Rx Data Interfaces
    //------------------------------------------------------
    stream<UdpAppData>              &soUAIF_Data,
    stream<UdpAppMeta>              &soUAIF_Meta,
    stream<UdpAppDLen>              &soUAIF_DLen,
``` 
* `soUAIF_Data` is the data stream forwarded by **`UOE`** upon data arrival from the network. It implements an 
    *AXI4-Stream interface* of type \<[UdpAppData](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisApp.hpp)\> and its maximum 
    length equals the *UDP Maximum Datagram Size* where *UDP_MDS = (MTU_ZYC2-IP4_HEADER_LEN-UDP_HEADER_LEN) & ~0x7* = 
    1416 bytes.
* `soUAIF_Meta` is the metadata of the data stream forwarded by **`UOE`** upon data arrival. It implements an
    *AXI4-Stream interface* of type \<[UdpAppMeta](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the
    socket-pair associated with this data stream.
* `soUAIF_DLen` specifies the length of the data stream forwarded by **`UOE`**. It implements an 
    *AXI4-Stream interface* of type \<[UdpAppDLen](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\>. 

#### Tx Application Data Interface
For **`UOE`** to send data on the network, the **`APP`** must provide the data as a stream of chunks, and every stream 
must be accompanied by a metadata and payload length information. The metadata specifies the socket-pair that the 
stream belongs to, while the payload length specifies its length.
```
    //------------------------------------------------------
    //-- UAIF / Tx Data Interfaces
    //------------------------------------------------------
    stream<UdpAppData>              &siUAIF_Data,
    stream<UdpAppMeta>              &siUAIF_Meta,
    stream<UdpAppDLen>              &siUAIF_DLen,
```
* `siUAIF_Data` is the data stream forwarded by **`APP`** to **`UOE`** for transmission on the Ethernet network. It 
    implements an *AXI4-Stream interface* of type \<[UdpAppData](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisApp.hpp)\>. 
* `siUAIF_Meta` is the metadata associated with the forwarded data stream. It implements an *AXI4-Stream interface*
    of type \<[UdpAppMeta](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> which specifies the socket-pair associated with 
    this data stream. 
* `siUAIF_DLen` specifies the length of the data stream that **`APP`** is requesting to send. It implements an 
    *AXI4-Stream interface* of type \<[UdpAppDLen](../../SRA/LIB/SHELL/LIB/hls/NTS/nts.hpp)\> and the value loaded into
    it defines one of the two following modes of operation: 
    1) *DATAGRAM_MODE*: If the 'DLen' stream is loaded with a length != 0, this length is used as reference for handling 
        the corresponding data stream. If the length is larger than UDP_MDS bytes (.i.e, MTU_ZYC2-IP_HEADER_LEN-UDP_HEADER_LEN),
        this process will split the incoming datagram and will generate as many sub-datagrams as required to transport 
        all 'DLen' bytes over multiple Ethernet frames.
        - Warning: While in *DATAGRAM_MODE* mode, the setting of the '*TLAST*' bit of the data stream is not required
            but is highly recommended.
    2) *STREAMING_MODE*: If the 'DLen' stream is loaded with a length == 0, the corresponding data stream will keep
        on being forwarded based on the current metadata information until the '*TLAST*' bit of the data stream is set. 
        In this mode, **`UOE`** will wait for the reception of UDP_MDS bytes before generating a new UDP-over-IPv4 
        packet, unless the '*TLAST*' bit of the data stream is set.
        - Warning: While in *STREAMING_MODE*, it is the responsibility of the application to set the '*TLAST*' bit of 
            the data stream back to '1' from time to time, otherwise that single stream will monopolize the **`UOE`** 
            indefinitely.
