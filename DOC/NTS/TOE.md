# TCP Offload Engine (TOE)
This document describes the design of the **TCP Offload Engine (TOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initialy developed by **Xilinx Dublin Labs, Ireland** who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be found [here](https://github.com/fpgasystems/fpga-network-stack).    

## Overview
A block diagram of the *TOE* is depicted in Figure 1. It features an *Rx Engine (RXe)* to handle the incoming data path from the IP layer, a *Tx Engine (TXe)* to assemble outgoing data packets for the IP layer, and a set of TCP state- and data-keeping engines.


![Block diagram of the TOE](./images/Fig-TOE-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the TCP Offload Engine</b></p>
<br>

## HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../hdl-naming-conventions.md), 
  2) [**HLS Naming Conventions**](./hls-naming-conventions.md)
<br>

## List of Files
The two entry files to consider for the TCP Offload Engine are:
  * [**toe.cpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.cpp)
  * [**toe.hpp**](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.hpp)

## List of Interfaces

| Acronym                                           | Description                                 | Filename
|:--------------------------------------------------|:--------------------------------------------|:--------------
| **[CAM](#content-addressable-memory-interface)**  | Content Addressable Memory interface        | [ToeCam](      ../../SRA/LIB/SHELL/LIB/hdl/nts/ToeCam/ToeCam.v)
| **[IPRX](#ip-receive-layer-interface)**           | IP Receive layer interface                  | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx_handler.hpp)
| **[IPTX](#ip-transmit-layer-interface)**          | IP Transmit layer interface                 | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/NTS/iptx/src/iptx_handler.hpp)
| **[MEM](#memory-system-interface)**               | Memory system interface                     | [memSubSys](   ../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **[MMIO](#memory-mapped-io-interface)**           | Memory mapped IO interface                  | [mmioClient](  ../../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)
| **[TAIF](#tcp-application-layer-interface)**      | TCP Application layer interface             | [toe](         ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/toe.hpp)

## List of HLS Components

| Acronym                   | Description                | Filename
|:--------------------------|:---------------------------|:--------------
| **[AKd](./AKd.md)**       | AcK delayer                | [ack_delay](       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/ack_delay/ack_delay.cpp)
| **[EVe](./EVe.md)**       | EVent engine               | [event_engine](    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/event_engine/event_engine.cpp)
| **[PRt](./PRt.md)**       | PoRt table                 | [port_table](      ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/port_table/port_table.cpp)
| **[RAi](./RAi.md)**       | Rx Application interface   | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_app_interface/rx_app_interface.cpp)
| **[RSt](./RSt.md)**       | Rx Sar table               | [rx_sar_table](    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_sar_table/rx_sar_table.cpp)
| **[RXe](./RXe.md)**       | RX engine                  | [rx_engine](       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **[SLc](./SLc.md)**       | Session Lookup controller  | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **[STt](./STt.md)**       | STate table                | [state_table](     ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/state_table/state_table.cpp)
| **[TAi](./TAi.md)**       | Tx Application interface   | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_app_interface/tx_app_interface.cpp)
| **[TIm](./TIm.md)**       | TImers                     | [timers](          ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/timers/timers.cpp)
| **[TSt](./TSt.md)**       | Tx Sar table               | [tx_sar_table](    ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_sar_table/tx_sar_table.cpp)
| **[TXe](./TXe.md)**       | TX engine                  | [tx_engine](       ../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_engine/src/tx_engine.cpp)

## Description of the Interfaces
The entity declaration of the TCP Offload Engine (TOE) is specified as follows:
```C
void toe(
    //------------------------------------------------------
    //-- MMIO Interfaces
    //------------------------------------------------------
    Ip4Addr                                  piMMIO_IpAddr,
    //------------------------------------------------------
    //-- NTS Interfaces
    //------------------------------------------------------
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


### IP Receive Layer Interface
```C
    //------------------------------------------------------
    //-- IPRX / IP Rx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &siIPRX_Data,
```
The IP receive layer interface receives IPv4 packets from the IPRX core of the NTS. 
It consists of an **AXI4-Stream** interface of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp))\>.

### IP Transmit Interface
```C
    //------------------------------------------------------
    //-- IPTX / IP Tx / Data Interface
    //------------------------------------------------------
    stream<AxisIp4>                         &soIPTX_Data,
```
The IP transmit layer interface sends IPv4 packets to the IPTX core of the NTS. 
It consists of an **AXI4-Stream** interface of type \<[AxisIp4](../../SRA/LIB/SHELL/LIB/hls/NTS/AxisIp4.hpp))\>

### TCP Application Layer Interface
The _TCP Role Interface_ (TRIF) connects the TOE to the user's role and its application (APP).
The interface consists of the following signals (TODO).  



### Memory System Interface  [TODO]
(data-mover to DDR4) 

### Content Addressable Memory Interface  [TODO]


