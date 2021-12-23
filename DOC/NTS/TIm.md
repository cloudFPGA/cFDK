# Timers (TIm)

Timer processes for the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
This is a container for all the timer-based processes of the **TCP Offload engibe (TOE)**.
![Block diagram of the TOE/TIm](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-TOE-TIm-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Timers</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **EVe**        | EVent Engine interface                                | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
|  **RAi**        | Rx Application Interface                              | [rx_app_if](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_if/rx_app_if.cpp)
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **STt**        | State Table interface                                 | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)
|  **TAi**        | Tx Application Interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Clt**         | Close timer processs                                  | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
| **Pbt**         | Probe timer processs                                  | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
| **Rtt**         | Retransmit timer processs                             | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
| **Emx**         | Event multiplexer process                             | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
| **Smx**         | State multiplexer process                             | [timers](../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)

<br>
