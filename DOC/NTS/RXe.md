# Rx Engine (RXe)
This document describes the receive data path of the **TCP Offload engibe (TOE)** used by the *cloudFPGA* platform.
 
## Overview
A block diagram of the **RXe** is depicted in Figure 1.
New Ipv4 packets enter the engine via the **IPRX** interface. 
Their IPv4 and TCP headers are parsed and their TCP checksum are checked.
Next, if the TCP destination port of the incoming TCP segment is opened and if that segment contains valid payload, 
it is stored in external DDR4 memory and the *Rx Application Interface* (**RAi**) is notified about this arrival of new data.

![Block diagram of the TOE/RXe](../internal/shell/images/Fig-TOE-RXe-Structure.bmp#center)
<p align="center"><b>Figure-1: Block diagram of the of the Rx Engine</b></p>
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
