# Tx Engine (TXe)
This document describes the transmit data path of the **TCP Offload engibe (TOE)** used by the *cloudFPGA* platform.
 
## Overview  
The Tx Engine (TXe) is responsible for the generation and transmission of IPv4 packets with embedded TCP payload.
A block diagram of the **TXe** is depicted in Figure 1.
Upon reception of a transmission event, a state machine loads and generates the necessary metadata for constructing an IPv4 packet. 
If that packet contains any payload, the data are retrieved from the DDR4 memory and are embedded as a TCP segment into the IPv4 packet. 
The complete packet is then streamed out over the **L3MUX** interface of the TOE.


![Block diagram of the TOE/TXe](./images/Fig-TOE-TXe-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the of the Tx Engine</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **AKd**         | Ack Delayer interface                                 | ack_delay
| **EVe**         | Event Engine interface                                | event_engine
| **L3MUX**       | IP Tx Interface                                       | iptx_handler
| **MEM**         | Memory sub-system (data-mover to DDR4)                | memSubSys
| **RSt**         | Rx SAR Table interface                                | rx_sar_table
| **SLc**         | Session Lookup Controller interface                   | session_lookup_controller
| **TIm**         | Timers interface                                      | [TODO]
| **TSt**         | Tx SAR Table interface                                | tx_sar_table

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Ihc**         | IP Header Constructor process                         | tx_engine
| **Ips**         | IP Packet Stitcher process                            | tx_engine
| **Mdl**         | Meta Data Loader process                              | tx_engine
| **Mrd**         | Memory Reader process                                 | tx_engine
| **Phc**         | PseudoHeaderConstructor process                       | tx_engine
| **Sca**         | Sub Checksum Accumulator process                      | tx_engine
| **Sps**         | Socket Pair Splitter process                          | tx_engine
| **Tca**         | TCP Checksum Accumulator process                      | tx_engine
| **Tss**         | TCP Segment Stitcher                                  | tx_engine

<br>
