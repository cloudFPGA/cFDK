# 10 Gigabit Ethernet (ETH)

This directory contains the toplevel design of the 10 Gigabit Ethernet (ETH) subsystem 
 used by the _cloudFPGA_ platform.

## Overview
  
  From an _Open Systems Interconnection (OSI)_ model point of view, this **`ETH`**
  module implements the _Physical layer (L1)_ and the _Data Link layer (L2)_ of the
  OSI model. These two layers are also referred to as the _Network Access_ layer in
  the _TCP/IP_ model.     

## Structure
  
```
[ETH] --> TenGigEth
  +-- [META] --> TenGigEth_SyncBlock 
  +-- [CORE] --> TenGigEth_Core
  |    +-- [IP  ] --> TenGigEthSubSys_X1Y8_R
  |    +-- [FIFO] --> TenGigEth_MacFifo
  +-- [ALGC] --> TenGigEth_AxiLiteClk
```
 
