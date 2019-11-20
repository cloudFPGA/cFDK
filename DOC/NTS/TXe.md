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
| **EVe**         | Event Engine interface.                               | event_engine
| **IPRX**        | IP Rx Interface.                                      | iprx_handler
| **MEM**         | Memory sub-system (data-mover to DDR4).               | [TODO]
| **PRt**         | Port Table interface.                                 | port_table
| **RSt**         | Rx SAR Table interface.                               | rx_sar_table
| **SLc**         | Session Lookup Controller interface.                  | session_lookup_controller
| **STt**         | State Table interface.                                | state_table
| **TRIF**        | TCP Role Interface (alias APP).                       | tcp_role_interface
| **TIm**         | Timers interface.                                     | [TODO]
| **TSt**         | Tx SAR Table interface.                               | tx_sar_table

<br>

## List of HLS Components

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **Csa**         | Check-Sum Accumulator process.                        | rx_engine
| **Evm**         | Event Muxer process.                                  | rx_engine
| **Fsm**         | Finite State Machine process.                         | rx_engine
| **Iph**         | Insert Pseudo Header process.                         | rx_engine
| **Mdh**         | Meta Data Handler process.                            | rx_engine
| **Mwr**         | Memory WRiter process.                                | rx_engine
| **Ran**         | Rx Application Notifier process.                      | rx_engine
| **Tid**         | Tcp Invalid Dropper process.                          | rx_engine
| **Tle**         | Tcp Length Extractor process.                         | rx_engine
| **Tsd**         | Tcp Segment Dropper process.                          | rx_engine

<br>
