### The shell Kale
This document describes the design of the shell **_Kale_** of the cloudFPGA platform.

#### Overview
This version of the cloudFPGA (cF) shell was developed for the testing and the bring-up of the FMKU2595 module when it is equipped with a XCKU060. As such, the shell **_Kale_** only provides the minimal features required to access the hardware components of the FPGA card, and it cannnot be used to deploy partial bit streams. Instead, a JTAG interface is required to download and configure the FPGA with a fully static bit stream.
As shown in Figure 1, the shell **_Kale_** implements the following IP cores:
  - one 10G Ethernet subsystem (ETH) as described in PG157,
  - a dual 8GB DDR4 memory subsystem (MEM) as described in PG150,
  - one network and transport stack (NTS) core based on the TCP-UDP/IP protocols,
  - one register file core with memory mapped IOs (MMIO).

![Block diagram of Kale](./imgs/Fig-SHELL-Kale.png#center)
<p align="center"><b>Figure-1: Block diagram of the shell Kale</b></p>
<br>

#### HDL Coding Style and Naming Conventions
Please consider reading the [**HDL Naming Conventions**](./hdl-naming-conventions.md) document if you intend to deploy this shell or you want to contribute to this part of the cloudFPGA project.
<br>

#### List of Interfaces
The shell **_Kale_** exposes the following interfaces:

| Acronym         | Description                                           | File
|:----------------|:------------------------------------------------------|:--------------
| **CLKT**        | CLocK Tree interface                                  | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **DDR4**        | Double Data Rate 4 memory interface                   | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ECON**        | Edge CONnnector interface                             | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **PSOC**        | Programmable System-On-Chip interface                 | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ROLE**        | Test and bring-up application interface               | [ROLE (a.k.a APP)](#description-of-the-role-interface)
<br>

#### List of HDL Components
Details about the **_Kale_** internal components and IP cores can be found by following the links of the below table:

| Acronym                                          | Description                | File
|:-------------------------------------------------|:---------------------------|:--------------
| **[ETH](../SRA/LIB/SHELL/LIB/hdl/eth/ETH.md)**   | 10G ETHernet subsystem     | [tenGigEth.v](../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **[MEM](../SRA/LIB/SHELL/LIB/hdl/eth/MEM.md)**   | DDR4 MEMory subsystem      | [memSubSys.v](../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **[MMIO](../SRA/LIB/SHELL/LIB/hdl/mmio/MMIO.md)**| Memory Mapped IOs          | [mmioClient_A8_D8.v](../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)
| **[NTS](./NTS/README.md)**                       | Network Transport Stack    | [nts_TcpIp.v](../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp.v)
<br>

##### Description of the ROLE interface
**WARNING** FYI, the **ROLE I/F** is often referred to as **APP I/F** for _Application Interface_.




---
**Trivia**: The [moon Kale](https://en.wikipedia.org/wiki/Kale_(moon))
