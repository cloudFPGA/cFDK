# Rx Application Interface (RAi)
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/RAi.md)

This document describes the front-end interface process to the TCP Role Interface (TRIF) of the **[TCP Offload engine (TOE)](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TOE.md)** used by the *cloudFPGA* platform. 

## Overview
This process provides the application (APP a.k.a ROLE) with the TCP data streams of the established connections. A block diagram of the **RAi** is depicted in Figure 1.


The basic handshake between the [RAi] and the application is as follows:
- The _NotificationMux_ (Nmx) sends a notification to [TRIF] whenever a valid TCP segment is received and is written into the Rx Memory (MEM).
- Upon reception of a data request from [TRIF], 
  - the _RxAppStream_ (Ras) process generates a memory read command that is sent to [MEM] via the _RxMemoryAccess_ [Rma] process, 
  - a request to update the Rx application pointer of the current session is forwarded to the _RxSarTable_ [RSt] process,
  - a meta-data (.i.e the session-id) is sent back to [TRIF] to signal that the request has been processed.
- The data issued by [MEM] are handled by the process _MemoryReader_ (Mrd) and are forwarded to the application via [TRIF].

Finally, aqn application can only receive data if its TCP receive port has previously been opened in '_listen_' mode. Such a connection opening is handled by the _ListenAppInterface_ process .
 
![Block diagram of the TOE/RAi](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./images/Fig-TOE-RAi-Structure.bmp?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the Rx Application Interface</b></p>
<br>

## List of Interfaces

| Acronym                    | Description                             | Filename
|:---------------------------|:----------------------------------------|:--------------
|  **MEM**                   | MEMory sub-system (data-mover to DDR4)  | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
|  **[PRt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./PRt.md)**       | PoRt table                              | [port_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/toe/src/port_table/port_table.cpp)
|  **[RSt](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./RSt.md)**       | Rx Sar table                            | [rx_sar_table](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_sar_table/rx_sar_table.cpp)
|  **[RXe](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./RXe.md)**       | RX engine                               | [rx_engine](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine/src/rx_engine.cpp)
|  **[TIm](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/./TIm.md)**       | TImers                                  | [timers](https://github.com/cloudFPGA/cFDK/blob/master/DOC/NTS/../../SRA/LIB/SHELL/LIB/hls/toe/src/timers/timers.cpp)
|  **TRIF**                  | Tcp Role InterFace (alias APP)          | 

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Lai**         | Listen Application Interface process                  | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
| **Nmx**         | Notification Multiplexer processs                     | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
| **Mrd**         | Memory Reader processs                                | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
| **Ras**         | Rx Application Stream processs                        | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)
| **Rma**         | Rx Memory Access processs                             | [rx_app_interface](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_app_interface/rx_app_interface.cpp)

<br>
