# Session Lookup Controller (SLc)
This document describes the process which provides a front-end interfaces to the Content Addessable Memory (CAM) of the **[TCP Offload engine (TOE)](./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
Every TCP connection is identified by a SessionId and an associated 4-tuple {IP_DA,TCP_DP,IP_SA,TCP_SP} structure. These data are stored in the CAM which content is accessed and managed by the SLc. A block diagram of the **SLc** is depicted in Figure 1.
![Block diagram of the TOE/SLc](./images/Fig-TOE-SLc-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Session Lookup Controller</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **CAM**        | Content Addessable Memory interface                   | [ToeCam](../../SRA/LIB/SHELL/LIB/hdl/nts/ToeCam/ToeCam.v)
|  **PRt**        | Port Table interface                                  | [port_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)  
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **STt**        | STate table                                           | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)
|  **TAi**        | Tx Application interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
|  **TXe**        | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Lrh**         | Lookup Reply Handler processs                         | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **Rlt**         | Reverse Lookup Table processs                         | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **Sim**         | Session Id Manager processs                           | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **Urh**         | Update RequestHandler processs                        | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **Urs**         | Update Request Sender processs                        | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)


<br>
