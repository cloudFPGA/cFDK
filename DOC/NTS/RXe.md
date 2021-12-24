# Rx Engine (RXe)

This document describes the receive data path of the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform.
 
## Overview
A block diagram of the **RXe** is depicted in Figure 1.
New Ipv4 packets enter the engine via the **IPRX** interface. 
Their IPv4 and TCP headers are parsed and their TCP checksum are checked.
Next, if the TCP destination port of the incoming TCP segment is opened and if that segment contains valid payload, 
it is stored in external DDR4 memory and the *Rx Application Interface* (**RAi**) is notified about this arrival of new data.
![Block diagram of the TOE/RXe](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./images/Fig-TOE-RXe-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Rx Engine</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **EVe**         | Event Engine interface                                | [event_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/event_engine/event_engine.cpp)
| **IPRX**        | IP RX Interface                                       | [iprx](../../SRA/LIB/SHELL/LIB/hls/NTS/iprx/src/iprx.cpp) 
| **MEM**         | MEMory sub-system (data-mover to DDR4)                | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **PRt**         | Port Table interface.                                 | [port_table](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/port_table/port_table.cpp)  
| **RAi**         | Rx Application interface                              | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_app_interface/rx_app_interface.cpp)
| **RSt**         | Rx SAR Table interface                                | [rx_sar_table](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_sar_table/rx_sar_table.cpp)
| **SLc**         | Session Lookup Controller interface                   | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **STt**         | State Table interface                                 | [state_table](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/state_table/state_table.cpp)  
| **TIm**         | Timers interface                                      | [TODO]
| **TSt**         | Tx SAR Table interface                                | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/tx_sar_table/tx_sar_table.cpp)

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Csa**         | Check-Sum Accumulator process.                        | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Evm**         | Event Muxer process.                                  | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Fsm**         | Finite State Machine process.                         | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Iph**         | Insert Pseudo Header process.                         | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Mdh**         | Meta Data Handler process.                            | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Mwr**         | Memory WRiter process.                                | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Ran**         | Rx Application Notifier process.                      | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Tid**         | Tcp Invalid Dropper process.                          | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Tle**         | Tcp Length Extractor process.                         | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)
| **Tsd**         | Tcp Segment Dropper process.                          | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/NTS/toe/src/rx_engine/src/rx_engine.cpp)

<br>
