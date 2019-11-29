# Tx Engine (TXe)
This document describes the transmit data path of the **TCP Offload engibe (TOE)** used by the *cloudFPGA* platform.
 
## Overview  
The Tx Engine (TXe) is responsible for the generation and transmission of IPv4 packets with embedded TCP payload.
A block diagram of the **TXe** is depicted in Figure 1.
Upon reception of a transmission event, a state machine loads and generates the necessary metadata for constructing an IPv4 packet. 
If that packet contains any payload, the data are retrieved from the DDR4 memory and are embedded as a TCP segment into the IPv4 packet. 
The complete packet is then streamed out over the **L3MUX** interface of the TOE.
![Block diagram of the TOE/TXe](./images/Fig-TOE-TXe-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the Tx Engine</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **AKd**         | Ack Delayer interface                                 | [ack_delay](../../SRA/LIB/SHELL/LIB/hls/toe/src/ack_delay/ack_delay.cpp)  
| **EVe**         | Event Engine interface                                | [event_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/event_engine/event_engine.cpp)
| **L3MUX**       | IP Tx Interface                                       | [iptx_handler](../../SRA/LIB/SHELL/LIB/hls/iptx_handler/src/iptx_handler.cpp)
| **MEM**         | Memory sub-system (data-mover to DDR4)                | [memSubSys](../../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **RSt**         | Rx SAR Table interface                                | [rx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/rx_sar_table/rx_sar_table.cpp)
| **SLc**         | Session Lookup Controller interface                   | [session_lookup_controller](../../SRA/LIB/SHELL/LIB/hls/toe/src/session_lookup_controller/session_lookup_controller.cpp)
| **TIm**         | Timers interface                                      | [TODO]
| **TSt**         | Tx SAR Table interface                                | [tx_sar_table](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_sar_table/tx_sar_table.cpp)

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Ihc**         | IP Header Constructor process                         | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)
| **Ips**         | IP Packet Stitcher process                            | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)
| **Mdl**         | Meta Data Loader process                              | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp) 
| **Mrd**         | Memory Reader process                                 | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)
| **Phc**         | PseudoHeaderConstructor process                       | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp) 
| **Sca**         | Sub Checksum Accumulator process                      | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp) 
| **Sps**         | Socket Pair Splitter process                          | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp) 
| **Tca**         | TCP Checksum Accumulator process                      | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp)
| **Tss**         | TCP Segment Stitcher                                  | [tx_engine](../../SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine/src/tx_engine.cpp) 

<br>
