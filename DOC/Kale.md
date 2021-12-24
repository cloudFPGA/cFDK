#### The shell Kale

This document describes the design of the **`SHELL`** **_Kale_** of the cloudFPGA platform.

#### Overview
This version of the cloudFPGA (cF) shell was developed for the testing and the bring-up of the FMKU2595 module when it 
 is equipped with a XCKU060. As such, the shell **_Kale_** only provides the minimal features required to access the 
 hardware components of the FPGA card, and it cannot be used to deploy partial bit streams. Instead, a JTAG interface is 
 required to download and configure the FPGA with a fully static bit stream.

A block diagram of the shell **_Kale_** is shown in Figure 1. It implements the following IP cores:
  - one 10G Ethernet subsystem (ETH) as described in PG157,
  - a dual 8GB DDR4 memory subsystem (MEM) as described in PG150,
  - one network and transport stack (NTS) core based on the TCP-UDP/IP protocols,
  - one register file core with memory mapped IOs (MMIO).

![Block diagram of Kale](./imgs/Fig-SHELL-Kale.png?raw=true#center)
<p align="center"><b>Figure-1: Block diagram of the shell Kale</b></p>

The **`SHELL`** is predominantly written in C/C++ with a few pieces coded in HDL, such as the top-level Verilog file 
[**Shell.v**](https://github.com/cloudFPGA/cFDK/tree/main/SRA/LIB/SHELL/Kale/Shell.v) which assembles and interconnects the major components. 

For the components specified in C/C++, we use the Xilinx HLS flow to synthesize and export them as a standalone IP cores.
 With respect to Figure-1, the white boxes represent the sub-components specified in C/C++ or HDL, and the gray boxes 
 represent interfaces to neighbouring FPGA's IP blocks or to external physical components on the FPGA card.

#### HDL Coding Style and Naming Conventions
The design of the **`SHELL`** uses some specific HDL and HLS naming rules to ease the description and the understanding 
 of its architecture. Please consider reading the two documents [**HDL coding style and naming conventions**](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./hdl-naming-conventions.md)
 and [**HLS coding style and naming conventions**](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./NTS/hls-naming-conventions.md) before diving into the code or starting
 to contribute to this part of the cloudFPGA project.

#### List of Components
The following table lists the sub-components of the **`SHELL`** and provides a link to their documentation as well as 
their architecture body. 

| Entity                            | Description                | Architecture
|:----------------------------------|:---------------------------|:--------------
| **[ETH](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./ETH/ETH.md)**           | 10G Ethernet subsystem     | [tenGigEth.v](https://github.com/cloudFPGA/cFDK/tree/main/DOC/../SRA/LIB/SHELL/LIB/hdl/eth/tenGigEth.v)
| **[MEM](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./MEM/MEM.md)**           | DDR4 Memory subsystem      | [memSubSys.v](https://github.com/cloudFPGA/cFDK/tree/main/DOC/../SRA/LIB/SHELL/LIB/hdl/mem/memSubSys.v)
| **[MMIO](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./MMIO/MMIO.md)**        | Memory Mapped IOs          | [mmioClient_A8_D8.v](https://github.com/cloudFPGA/cFDK/tree/main/DOC/../SRA/LIB/SHELL/LIB/hdl/mmio/mmioClient_A8_D8.v)
| **[NTS](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./NTS/NTS.md)**           | Network Transport Stack    | [nts_TcpIp.v](https://github.com/cloudFPGA/cFDK/tree/main/DOC/../SRA/LIB/SHELL/LIB/hdl/nts/nts_TcpIp.v)

#### List of Interfaces
The **`SHELL`** _Kale_ exposes the following five interfaces:

| Interface   | Description                                           | Implementation
|:------------|:------------------------------------------------------|:--------------
| **CLKT**    | CLocK Tree interface                                  | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **DDR4**    | Double Data Rate 4 memory interface                   | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ECON**    | Edge CONnnector interface                             | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **PSOC**    | Programmable System-On-Chip interface                 | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ROLE**    | User application interface                            | [ROLE](#description-of-the-shell-role-interfaces)

**[INFO]** - The *ROLE I/F* is also referred to as *APP I/F* which stands for _Application Interface_.

#### Description of the SHELL-ROLE Interfaces
The interface between the **`SHELL`** _Kale_ and a user application implemented in the **`ROLE`** consists of four
 sub-interfaces:
- One [UDP Application Interface](#udp-application-interface) implemented as a set of AXI4-Stream interfaces,
- One [TCP Application Interface](#tcp-application-interface) implemented as a set of AXI4 stream interfaces,
- One [Memory Stream Interface](#memory-stream-interface) implemented as a set of AXI4 stream interfaces,
- One [Memory Mapped Interface](#memory-mapped-interface) based on an AXI4-Full interface. 

##### UDP Application Interface
The UDP application interface (UAIF) connects the UDP offload engine of the **`SHELL`** to a UDP 
 network application layer or directly to a UDP user application. It consists of a set of receive and transmit 
 interfaces, each of them being implemented as an *AXI4-Stream interface*.
* `siROL_Nts_Udp_****` signals implement the transmit data and control paths between the application layer of the 
    **`ROLE`** and the **`SHELL`** (i.e., the outgoing network traffic direction). 
 * `soROL_Nts_Udp_****` signals implement the receive data and control paths between the **`SHELL`** and the application
    layer of the **`ROLE`** (i.e., the incoming network traffic direction).

Refer to the [UDP Application Layer Interface of UOE](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./NTS/UOE.md#udp-application-layer-interface) for a detailed 
 description of these interfaces. 

##### TCP Application Interface
The TCP application interface (TAIF) connects the TCP offload engine of the **`SHELL`** to a network presentation 
 layer such as a TLS process, a network application layer such as an HTTP server, or directly to a TCP user application.
 It consists of a set of receive and transmit interfaces, each of them being implemented as an *AXI4-Stream interface*.
 * `siROL_Nts_Tcp_****` signals implement the transmit data and control paths between the application layer of the 
    **`ROLE`** and the **`SHELL`** (i.e., the outgoing network traffic direction). 
 * `soROL_Nts__Tcp_****` signals implement the receive data and control paths between the **`SHELL`** and the 
    application layer of the **`ROLE`** (i.e., the incoming network traffic direction).

Refer to the [TCP Application Layer Interface of TOE](https://github.com/cloudFPGA/cFDK/tree/main/DOC/./NTS/TOE.md#tcp-application-layer-interface) for a detailed
 description of these interfaces.

##### Memory Stream Interface
The Memory Stream Interface (Mp0) exposes 8GB of DDR4 memory to the user application in the **`ROLE`**. The interface is
 said to be stream-based because it consists of the six *AXI4-Stream interfaces* listed in Table-1.  

a name="mp0-app-interface"></a> <p align="center"><b>Table 1 - SHELL / Role / Mp0 Interface </b></p>

| Interface             | Dir | Size   | Signal Name          | Type  | Description
|:----------------------|:---:| ------:|----------------------|:-----:|:------------
| Read Command          | in  | [79:0] | siROL_Mem_Mp0_RdCmd  | AXI4S | Request to read from DDR4 memory.
| Read Status           | out | [ 7:0] | soROL_Mem_Mp0_RdSts  | AXI4S | Status of the memory read operation.
| Read Data Channel     | out |[511:0] | soROL_Mem_Mp0_Read   | AXI4S | Data stream read from the DDR4 memory.
| Write Command         | in  | [79:0] | siROL_Mem_Mp0_WrCmd  | AXI4S | Request to write to DDR4 memory.
| Write Status          | out | [ 7:0] | soROL_Mem_Mp0_WrSts  | AXI4S | Status of the memory write operation.
| Write Data Channel    | in  |[511:0] | siROL_Mem_Mp0_Write  | AXI4S | Data stream to write into the DDR4 memory.

##### Memory Mapped Interface
The Memory Mapped Interface (MP1) exposes 8GB of DDR4 memory to the user application in the **`ROLE`**. It consists of
 the _AXI4-Full interface_ listed in Table-2.

a name="mp1-app-interface"></a> <p align="center"><b>Table 3 - SHELL / Role / Mp1 Interface </b></p>

| Interface             | Dir | Size   | Signal Name          | Type  | Description
|:----------------------|:---:| ------:|----------------------|:-----:|:------------
| Write Address         | in  | [32:0] | miROL_Mem_Mp1_Addr   | AXI4F | DDR4 memory write address.
| Write Data Bus        | in  |[511:0] | miROL_Mem_Mp1_Data   | AXI4F | DDR4 memory write data.
| Write Response        | out | [ 1:0] | moROL_Mem_Mp1_Resp   | AXI4F | DDR4 memory write response.
| Read  Address         | in  | [32:0] | miROL_Mem_Mp1_Addr   | AXI4F | DDR4 memory read address.
| Read  Data Bus        | out |[511:0] | moROL_Mem_Mp1_Data   | AXI4F | DDR4 memory write data.

#### Description of the SHELL-CLKT Interface
The clocK tree interface (CLKT) is under construction [TODO].
     
#### Description of the SHELL-DDR4 Interface
The double data rate 4 memory interface (DDR4) is under construction [TODO].

#### Description of the SHELL-ECON Interfaces
The edge connector interface (ECON) is under construction [TODO].

#### Description of the SHELL-PSOC Interfaces
The programmable system-on-chip interface is under construction [TODO].   



---
**Trivia**: The [moon Kale](https://en.wikipedia.org/wiki/Kale_(moon))
