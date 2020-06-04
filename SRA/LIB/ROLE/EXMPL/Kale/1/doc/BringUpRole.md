# cFp_BringUp / ROLE


## Preliminary
In cloudFPGA (cF), the user application is referred to as a **_ROLE_** and is integrated along
with a **_SHELL_** that abstracts the hardware components of the FPGA module.

## Overview
A block diagram of the ROLE is depicted in Figure 1. It features a set of functions that were
developped as IP cores to support the test and the bring-up of the TCP-, UDP- and DDR4 components
of the cF module FMKU2595 when equipped with a XCKU060.
![Block diagram of the cFp_BringUp ROLE](./imgs/Fig-ROLE-Structure.png#center)
<p align="center"><b>Figure-1: Block diagram of cFp_BringUp ROLE</b></p>
<br>

## List of Interfaces

| Acronym         | Description                                           | Filename
|:----------------|:------------------------------------------------------|:--------------
| **SHELL**       | SHELL interface                                       | [Role.vhdl](../hdl/Role.vhdl)
<br>

## List of HLS Components

| Acronym                     | Description                | Filename
|:----------------------------|:---------------------------|:--------------
| **[TSIF](./TSIF.md)**       | TCP Shell InterFace        | [tcp_shell_if](../hls/tcp_shell_if/src/tcp_shell_if.cpp)
| **[USIF](./USIF.md)**       | UDP Shell InterFace        | [udp_shell_if](../hls/udp_shell_if/src/udp_shell_if.cpp)
| **[TAF](./TAF.md)**         | TCP Application Flash      | [tcp_app_flash](../hls/tcp_app_flash/src/tcp_app_flash.cpp)
| **[UAF](./UAF.md)**         | UDP Application Flash      | [udp_app_flash](../hls/udp_app_flash/src/udp_app_flash.cpp)
<br>




