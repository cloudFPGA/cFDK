# TCP Offload Engine (TOE)
This document describes the design of the **TCP Offload Engine (TOE)** used by the *cloudFPGA* platform.

## Preliminary and Acknowledgments
This code was initialy developed by **Xilinx Dublin Labs, Ireland** who kindly acepted to share it with the *cloudFPGA project* via the *GitHub* repository: https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip. In 2018, the code was ported and adapted for the needs and specificities of the *cloudFPGA* platform.

FYI - An enhanced branch of the initial *Xilinx* code is maintained by the **Systems Group @ ETH Zurich** and can be found [here](https://github.com/fpgasystems/fpga-network-stack).    

## Overview
A block diagram of the *TOE* is depicted in Figure 1. It features a *Rx Engine (RXe)*, a *Tx Engine (TXe)*, and a ... [TODO] ....
![Block diagram of the TOE](./images/Fig-TOE-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the of the TCP Offload Engine</b></p>
<br>

## HLS Coding Style and Naming Conventions
Please consider reading the following two documents before diving or contributing to this part of the cloudFPGA project.
  1) [**HDL Naming Conventions**](../../hdl-naming-conventions.md), 
  2) [**HLS Naming Conventions**](../hls-naming-conventions.md)
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **CAM**         | Content Addessable Memory interface.                  | [TODO]
| **IPRX**        | IP Rx Interface.                                      | [TODO]
| **L3MUX**       | Layer-3 Multiplexer interface.                        | [TODO]
| **MEM**         | Memory sub-system (data-mover to DDR4).               | [TODO]
| **TRIF**        | TCP Role Interface (alias APP).                       | tcp_role_interface

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **[RXe](./RXe.md)**   | Rx Engine.                                            | rx_engine
| **[TXe](./TXe.md)**   | Tx Engine.                                            | tx_engine
| **RAi**         | Rx Application Interface.                             | rx_app_if
| **TAi**         | Tx Application Interface.                             | tx_app_interface
| **RSt**         | Rx Sar Table.                                         | rx_sar_table
| **TSt**         | Tx Sar Table.                                         | tx_sar_table
| **EVe**         | Event Engine.                                         | event_engine
| **AKd(Evd)**    | Ack Delayer(Event Delayer).                           | ack_delay
| **TIm**         | Timers.                                               | 
| **STt**         | State Table.                                          | state_table
| **[PRt](#prt)**       | Port Table.                                           | port_table
| **[SLc](#slc)**       | Session Lookup Controller.                            | session_lookup_controller

<br>

### <a name="prt"></a>Port Table (PRt)

The structure of the PRt is as follows:

**PRt**
- **pTodo1** | Process 1
- **pTodo2** | Process 2

### <a name="slc"></a>Session Lookup Controller (SLc)

The structure of the SLc is as follows:

- **SLc** | Session Lookup Controller
  - **Sim** | Session Id Manager
  - **Lrh** | Lookup Reply Handler
  - **Urs** | Update Request Sender
  - **Urh** | Update Reply Handler
  - **Rlt** | Reverse Lookup Table

