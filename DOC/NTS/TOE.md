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
  * [**toe.cpp**](../../SRA/LIB/SHELL/LIB/hls/toe/src/toe.cpp)
  * [**toe.hpp**](../../SRA/LIB/SHELL/LIB/hls/toe/src/toe.hpp)


## List of Interfaces

| Acronym                                           | Description                                           | Filename
|:--------------------------------------------------|:------------------------------------------------------|:--------------
| **CAM**                                           | Content Addessable Memory interface                   | [ToeCam](../../SRA/LIB/SHELL/LIB/hdl/nts/ToeCam/ToeCam.v)
| **IPRX**                                          | IP RX interface                                       | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/iprx_handler/src/iprx_handler.cpp)
| **L3MUX**                                         | Layer-3 MUltipleXer interface                         | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **MEM**                                           | MEMory sub-system (data-mover to DDR4)                | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **[TRIF](#markdown-header-tcp-role-interface)**   | Tcp Role InterFace (alias APP)                        | tcp_role_interface

## List of HLS Components

| Acronym                   | Description                | Filename
|:--------------------------|:---------------------------|:--------------
| **[AKd](./AKd.md)**       | AcK delayer                | [ack_delay](../../SRA/LIB/SHELL/LIB/hls/toe/src/ack_delay/ack_delay.cpp)
| **[EVe](./EVe.md)**       | EVent engine               | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
| **[PRt](./PRt.md)**       | PoRt table                 | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
| **[RAi](./RAi.md)**       | Rx Application interface   | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
| **[RSt](./RSt.md)**       | Rx Sar table               | [rx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_sar_table/rx_sar_table.cpp)
| **[RXe](./RXe.md)**       | RX engine                  | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
| **[SLc](./SLc.md)**       | Session Lookup controller  | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **[STt](./STt.md)**       | STate table                | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)
| **[TAi](./TAi.md)**       | Tx Application interface   | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **[TIm](./TIm.md)**       | TImers                     | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
| **[TSt](./TSt.md)**       | Tx Sar table               | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_sar_table/tx_sar_table.cpp)
| **[TXe](./TXe.md)**       | TX engine                  | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)

## Description of the interfaces [TODO]

### TCP Role Interface
The _TCP Role Interface_ (TRIF) connects the TOE to the user's role and its application (APP).
The interface consists of the following signals (TODO).  
 

