# TCP Offload Engine (TOE)
This document describes the design of the **TCP Offload Engine (TOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initialy developed by **Xilinx Dublin Labs, Ireland** who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be found [here](https://github.com/fpgasystems/fpga-network-stack).    

## Overview
A block diagram of the *TOE* is depicted in Figure 1. It features an *Rx Engine (RXe)* for handling the incoming data path from the IP layer, a *Tx Engine (TXe)* for handling the outgoing data path to the IP layer, and a set of TCP state- and data-keeping engines. 
![Block diagram of the TOE](./images/Fig-TOE-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the TCP Offload Engine</b></p>
<br>

## HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../hdl-naming-conventions.md), 
  2) [**HLS Naming Conventions**](./hls-naming-conventions.md)
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **CAM**         | Content Addessable Memory interface                   | [ToeCam](../../SRA/LIB/SHELL/LIB/hdl/nts/ToeCam/ToeCam.v)
| **IPRX**        | IP RX interface                                       | [iprx_handler](../../SRA/LIB/SHELL/LIB/hls/iprx_handler/src/iprx_handler.cpp)
| **L3MUX**       | Layer-3 MUltipleXer interface                         | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **MEM**         | MEMory sub-system (data-mover to DDR4)                | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **TRIF**        | Tcp Role InterFace (alias APP)                        | tcp_role_interface

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **[RXe](./RXe.md)**   | RX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
| **[TXe](./TXe.md)**   | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)
| **RAi**         | Rx Application interface                              | rx_app_if
| **TAi**         | Tx Application interface                              | tx_app_interface
| **RSt**         | Rx Sar table                                          | rx_sar_table
| **TSt**         | Tx Sar table                                          | tx_sar_table
| **EVe**         | EVent engine                                          | event_engine
| **AKd(Evd)**    | AcK delayer                                           | ack_delay
| **TIm**         | TImers                                                | 
| **STt**         | STate table                                           | state_table
| **[PRt](./PRt.md)**       | PoRt table                                            | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
| **[SLc](./SLc.md)**       | Session Lookup controller                             | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)

<br>






