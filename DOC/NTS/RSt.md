# Rx Sar Table (RSt)

Rx segmentation and re-assembly table for the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
The table stores the data structures for managing the TCP Rx buffer and the Rx sliding window.
The RxSarTable is a single process concurrently accessed by the RxEngine (RXe), the RxApplicationInterface (RAi) and the TxEngine (TXe).
![Block diagram of the TOE/RSt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-TOE-RSt-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Rx SAR Table</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **RAi**        | Rx Application interface                              | [rx_app_if](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_if/rx_app_if.cpp)
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **TXe**        | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)


<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **RSt**         | Rx SAR Table processs                                 | [rx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_sar_table/rx_sar_table.cpp)

<br>
