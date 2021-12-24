# Port Table (PRt)

TCP port table management for the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
The port table of the **TCP Offload engibe (TOE)** keeps track of the TCP port numbers which are in use. It maintains two port ranges based on two tables of 32768 x 1-bit:
  - One for static ports (0 to 32,767) which are used for listening ports,
  - One for dynamically assigned or ephemeral ports (32,768 to 65,535) which are used for active connections.

![Block diagram of the TOE/PRt](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./images/Fig-TOE-PRt-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Port Table</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **RAi**        | Rx Application interface                              | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **SLc**        | Session Lookup Controller interface                   | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
|  **TAi**        | Tx Application interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
|  **TOE**        | TCP Offload Engine                                    | [toe](../../SRA/LIB/SHELL/LIB/hls/toe/src/toe.cpp)
<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Fpt**         | Free Port Table processs                              | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
| **Irr**         | Input Request Router processs                         | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
| **Lpt**         | Listening Port Table processs                         | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
| **Orm**         | Output Reply Multiplexer processs                     | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)

<br>
