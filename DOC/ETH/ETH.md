# 10 Gigabit Ethernet (ETH)
This document describes the design of the **10 Gigabit Ethernet (ETH)** sub-system used by the _cloudFPGA_ platform.

## Overview
A block diagram of **`ETH`** is depicted in Figure [TODO-Under construction]. It features a ...

From an _Open Systems Interconnection (OSI)_ model point of view, the **`ETH`** module implements the _Physical layer 
 (L1)_ and the _Data Link layer (L2)_ of the **OSI** model. These two layers are also referred to as the _Network 
 Access_ layer in the **TCP/IP** model.     

## HLS Coding Style and Naming Conventions
The design of **`ETH`** uses some specific HDL and HLS naming rules to ease the description and the understanding of
 its architecture. Please consider reading the two documents [**HDL coding style and naming conventions**](../hdl-naming-conventions.md)
 and [**HLS coding style and naming conventions**](./hls-naming-conventions.md) before diving into the code or starting
 to contribute to this part of the cloudFPGA project.

## List of Components
The following table lists the sub-components of **`ETH`** and provides a link to their documentation as well as their
architecture body. 

| Entity          | Description                      | Architecture
|:--------------- |:---------------------------------|:--------------
| **META**        | Synchronization block            | TenGigEth_SyncBlock 
| **CORE**        | 10GbE core                       | TenGigEth_Core
| **ALGC**        | Clock source                     | TenGigEth_AxiLiteClk 


 

 