# ACK Delay (AKd)

Acknowlegment delayer for the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform.

## Overview
Upon reception of an ACK event from the Event Engine (EVe), the counter associated to the corresponding session is initialized to (100ms/MAX_SESSIONS). Next, this counter is decremented every (MAX_SESSIONS) until it reaches zero. At that time, a request to generate an ACK for that session is forwarded to the TxEngine (TXe).
The ACK delayer is a single process that is only accessed by the Event Engine (EVe).
![Block diagram of the TOE/AKd](https://github.com/cloudFPGA/cFDK/blob/main/DOC/NTS/./images/Fig-TOE-AKd-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the ACK Delayer</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
|  **EVe**        | EVent engine                                          | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
|  **RXe**        | RX engine                                             | [rx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **TXe**        | TX engine                                             | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)


<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **AKd**         | ACK delayer processs                                  | [ack_delay](../../SRA/LIB/SHELL/LIB/hls/toe/src/ack_delay/ack_delay.cpp)

<br>
