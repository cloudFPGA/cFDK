# DDR4 Memory Sub-System (MEM)
This document describes the design of the **DDR4 Memory (MEM)** sub-system used by the _cloudFPGA_ platform.

## Overview
A block diagram of **`MEM`** is depicted in Figure [TODO-Under construction]. It features a ...
   

## HLS Coding Style and Naming Conventions
The design of **`MEM`** uses some specific HDL and HLS naming rules to ease the description and the understanding of
 its architecture. Please consider reading the two documents [**HDL coding style and naming conventions**](../hdl-naming-conventions.md)
 and [**HLS coding style and naming conventions**](./hls-naming-conventions.md) before diving into the code or starting
 to contribute to this part of the cloudFPGA project.

## List of Components
The following table lists the sub-components of **`MEM`** and provides a link to their documentation as well as their
architecture body. 

| Entity          | Description                 | Architecture
|:--------------- |:----------------------------|:--------------
| **TODO**        | Under construction          | Todo 
| **TODO**        | Under construction          | Todo 
| **TODO**        | Under construction          | Todo 


## Things to know and document later

### Burst sizes

- AXI4 protocol has two restrictions:
  - maximum 4096 Bytes
  - maximum 256 transactions
  - so the actual burst limt depends on the data width

- MC1 (connected to the Role)
  - use `memChan_DualPort_Hybrid` (for Kale & Themisto)
  - Streaming mode using AXI data mover: **64** max burst length (data size 64Bytes)
  - Memory mapped mode (directly connected to Axi interconnect): also 64 (since data size is also 512)
  - config parameters `READ_ACCEPTANCE` and `WRITE_ACCEPTANCE`: means the number of outstanding transfers, which must be equal to `2^ID_WIDTH`. 

- MC0 (connected to the Shell):
  - use `memChan_DualPort`
  - burst size is limited to **16** ?
  - depending on `gUserDataChanWidth`

### Adressing

- addresses for AXI are always **line based**
- so if one line has the width of 512 bits = 64 Bytes, then the address field of AXI is based on that
- e.g. address 0x3 in `ARADDR` means reading bytes 128 -- 191. 
- this is true for memory-mapped and streaming interfaces

### HLS pragmas

for memory-mapped:
```
void main(
...
int32_t boFdram[MEM_SIZE],
...
);
{
...
#pragma HLS INTERFACE m_axi port=boFdram bundle=boAPP_DRAM offset=direct latency=52 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=256 max_write_burst_length=256
//so for int32, max burst size for memory-mapped AXI4 is 1KiB, since the length is limited to 256
//num_outstanding comes from AXI Interconnect config
//latency is based on measurements
//offset direct means that there will be a address port in HDL that is the basis for the physical addresses

//access bytes 4 -- 7
ap_uint<32> tmp = boFdram[1];

...
}

```




