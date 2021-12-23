# Event Engine (EVe)
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/EVe.md)

Event engine for the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform.

## Overview
The event engine arbitrates the incoming events and forwards them to the Tx Engine (TXe).
The engine is a single process concurrently accessed by the RxEngine (RXe), the Timers (TIm), the TxApplicationInterface (TAi) and the AckDelayer (AKd).
![Block diagram of the TOE/EVe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-TOE-EVe-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Tx SAR Table</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **AKd**        | AcK delayer                                           | [ack_delay](../../SRA/LIB/SHELL/LIB/hls/toe/src/ack_delay/ack_delay.cpp)
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **TAi**        | Tx Application interface                              | [tx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_app_interface/tx_app_interface.cpp)
|  **TIm**        | Timers interface                                      | [TODO]
|  **TXe**        | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)


<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **EVe**         | EVent engine processs                                 | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)

<br>
