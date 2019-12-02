# Tx Sar Table (TSt)
Tx segmentation and re-assembly table for the **TCP Offload engibe (TOE)** used by the *cloudFPGA* platform. 

## Overview
The table stores the data structures for managing the TCP Tx buffer and the Tx sliding window.
The TxSarTable is a single process concurrently accessed by the RxEngine (RXe), the TxApplicationInterface (TAi) and the TxEngine (TXe).
![Block diagram of the TOE/TSt](./images/Fig-TOE-TSt-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Tx SAR Table</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **TAi**        | Tx Application interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
|  **TXe**        | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)


<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **TSt**         | Tx SAR Table processs                                 | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_sar_table/tx_sar_table.cpp)

<br>
