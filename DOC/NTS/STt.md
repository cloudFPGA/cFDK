# State Table (STt)
TCP state table management for the **TCP Offload engibe (TOE)** used by the *cloudFPGA* platform. 

## Overview
The state table of the **TCP Offload engibe (TOE)** stores the connection state of each session during its lifetime. 
These states are defined in RFC793 as: LISTEN, SYN-SENT, SYN-RECEIVED, ESTABLISHED, FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING, LAST-ACK, TIME-WAIT and CLOSED.
The StateTable is a single process accessed by the RxEngine (RXe), the TxAppInterface (TAi) and by the TxEngine (TXe). 
The process also receives session close commands from the Timers (TIm) and sends session release commands to the SessionLookupController (SLc).
![Block diagram of the TOE/STt](./images/Fig-TOE-STt-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the State Table</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **SLc**        | Session Lookup Controller interface                   | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
|  **TAi**        | Tx Application interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
|  **TIm**        | Timers                                                | [TODO]


<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **STt**         | State Table processs                                  | [state_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/state_table/state_table.cpp)

<br>
