# Tx Application Interface (TAi)
This document describes the front-end interface process to the TCP Role Interface (TRIF) of the **[TCP Offload engine (TOE)](./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
This process transmits the data streams of established connections. Before transmitting, it is also used to create a new active connection. 
Active connections are the ones opened by the FPGA as client and they make use of dynamically assigned or ephemeral ports in the range 32,768 to
65,535. A block diagram of the **TAi** is depicted in Figure 1.

The basic handshake between the [TAi] and the application is as follows:
- After data and metadata are received from [TRIF], the state of the connection is checked and the segment memory pointer is loaded into
  the APP pointer table.
- Next, data are written into the DDR4 buffer memory and the application is notified about the successful or failing 
write to the buffer memory.
 
![Block diagram of the TOE/TAi](./images/Fig-TOE-TAi-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Tx Application Interface</b></p>
<br>

## List of Interfaces

| Acronym                    | Description                             | Filename
|:---------------------------|:----------------------------------------|:--------------
|  **[EVe](./EVe.md)**       | Event Engine interface                  | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
|  **MEM**                   | MEMory sub-system (data-mover to DDR4)  | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
|  **[PRt](./PRt.md)**       | PoRt table                              | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
|  **[RXe](./RXe.md)**       | RX engine                               | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **[SLc](./SLc.md)**       | Session Lookup Controller interface     | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
|  **[STt](./STt.md)**       | State Table interface                   | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)  
|  **[TSt](./TSt.md)**       | Tx Sar table                            | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_sar_table/tx_sar_table.cpp)
|  **TRIF**                  | Tcp Role InterFace (alias APP)          | 

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Emx**         | Event multiplexer processs                            | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Mwr**         | Memory writer processs                                | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Slg**         | Stream length generator processs                      | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Sml**         | Stream metadata loader processs                       | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Taa**         | Tx application accept processs                        | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Tash**        | Tx application status handler process                 | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
| **Tat**         | Tx application table process                          | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)

<br>
