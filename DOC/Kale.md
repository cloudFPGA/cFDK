#### The shell Kale
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

| Acronym         | Description                                           | Interface(s)
|:----------------|:------------------------------------------------------|:--------------
| **CLKT**        | CLocK Tree interface                                  | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **DDR4**        | Double Data Rate 4 memory interface                   | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ECON**        | Edge CONnnector interface                             | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **PSOC**        | Programmable System-On-Chip interface                 | [Kale/Shell.v](../SRA/LIB/SHELL/Kale/Shell.v)
| **ROLE**        | Test and bring-up application interface               | [ROLE-UDP](#role-udp-interface)
|                 |                                                       | [ROLE-TCP](#role-tcp-interface)
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

#### Description of the ROLE interfaces
**WARNING** - The *ROLE I/F* is also referred to as *APP I/F* which stands for _Application Interface_.

<a name="role-udp-interface"></a> <p align="center"><b>Table 1 - ROLE / UDP Interface </b></p>

| Interface             | Dir | Size   | Signal Name          | Type  | Description
|:----------------------|:---:| ------:|----------------------|:-----:|:------------
| Tx Data Interface     | in  | [63:0] | siROL_Nts_Udp_Data   | AXI4S | UDP data stream to be transmitted by the shell.
|  (.i.e Role->Shell)   | in  | [95:0] | siROL_Nts_Udp_Meta   | AXI4S | Metadata of the UDP data stream to be sent by the shell. This information specifies a socket-pair association with the following format:
|                       |     |        |                      |       | - [31:00] IPv4  Source Address
|                       |     |        |                      |       | - [47:32] UDP   Source Port
|                       |     |        |                      |       | - [79:48] IPv4  Destination Address
|                       |     |        |                      |       | - [95:80] UDP   Destination Port
|                       | in  | [15:0] | siROL_Nts_Udp_DLen   | AXI4S | The length of the UDP data stream to send. If this field is configured with a length of '0', the data stream will be forwarded until the 'TLAST' bit is set. In this mode, the UDP Offload Engine of the shell will wait for the reception of 1472 bytes before generating a new UDP-over-IPv4 packet, unless the 'TLAST' bit of the data stream is set.
| Rx Data Interface     | out | [63:0] | soROL_Nts_Udp_Data   | AXI4S | UDP datagram received by the shell.
|  (.i.e Shell->Role)   | out | [95:0] | soROL_Nts_Udp_Meta   | AXI4S | Metadata of the received datagram. This information specifies the socket-pair association of the received datagram. The format is as follows:
|                       |     |        |                      |       | - [31:00] IPv4  Source Address
|                       |     |        |                      |       | - [47:32] UDP   Source Port
|                       |     |        |                      |       | - [79:48] IPv4  Destination Address
|                       |     |        |                      |       | - [95:80] UDP   Destination Port     
| Rx Ctrl Interface     | in  | [15:0] | siROL_Nts_Udp_LsnReq | AXI4S | UDP listen port request.
|  (.i.e Shell-->Role)  | out | [ 7:0] | soROL_Nts_Udp_LsnRep | AXI4S | UDP listen port reply (0=closed/1=opened).  
|                       | in  | [15:0] | siROL_Nts_Udp_ClsReq | AXI4S | UDP close  port request.
|                       | out | [ 7:0] | soROL_Nts_Udp_ClsRep | AXI4S | UDP close  port reply (0=closed/1=opened).            
<br>

a name="role-tcp-interface"></a> <p align="center"><b>Table 2 - ROLE / TCP Interface </b></p>

| Interface             | Dir | Size   | Signal Name          | Type  | Description
|:----------------------|:---:| ------:|----------------------|:-----:|:------------
| Tx Data Interface     | in  | [63:0] | siROL_Nts_Tcp_Data   | AXI4S | TCP data stream to be transmitted by the shell.
|  (.i.e Role->Shell)   | in  | [15:0] | siROL_Nts_Tcp_Meta   | AXI4S | Metadata of the TCP data stream to be sent by the shell. This information specifies the session-id of the connection that the data stream belongs to. The shell is notified of new data to send as soon as this field is updated.
|                       | out | [23:0] | soROL_Nts_Tcp_DSts   | AXI4S | TCP data send status returned by the shell at the end of the data transmission. The format is as follows:
|                       |     |        |                      |       | - [ 15:00] The # of bytes transmitted or an error code if status==0.
|                       |     |        |                      |       | - [ 23:16] Status of the transfer (0=ko/1=ok)
| Rx Data Interface     | out | [63:0] | soROL_Nts_Tcp_Data   | AXI4S | TCP segment received by the shell.
|  (.i.e Shell->Role)   | out | [15:0] | soROL_Nts_Tcp_Meta   | AXI4S | Metadata of the received segment. This information specifies the session-id of the connection that the segment belongs to.
|                       | out |[103:0] | soROL_Nts_Tcp_Notif  | AXI4S | TCP Rx data notification. Signals the reception of new segment to the application. The format is as follows:
|                       |     |        |                      |       | - [ 15:00] The session id of the connection.
|                       |     |        |                      |       | - [ 31:16] The length of the received segment.
|                       |     |        |                      |       | - [ 63:32] The IPv4 source address.
|                       |     |        |                      |       | - [ 79:64] The TCP source port.   
|                       |     |        |                      |       | - [ 95:80] The TCP destination port.
|                       |     |        |                      |       | - [103:96] A session closed indication.
|                       | in  | [31:0] | siROL_Nts_Tcp_DReq   | AXI4S | TCP data request. Used by the application to read a segment from the Rx buffer. The format is as follows:
|                       |     |        |                      |       | - [ 15:00] The session id of the connection to read from. 
|                       |     |        |                      |       | - [ 31:16] The number of bytes to read from the TCP Rx buffer.   
| Tx Ctrl Interface     | in  | [47:0] | siROL_Nts_Tcp_OpnReq | AXI4S | TCP open port request. Used by the application to open an active connection. This information specifies a socket address as follows:
|                       |     |        |                      |       | - [ 31:00] The IPv4 destination address.
|                       |     |        |                      |       | - [ 47:32] The TCP destination port. 
|  (.i.e Role->Shell)   | out | [23:0] | soROL_Nts_Tcp_OpnRep | AXI4S | TCP open port reply. Indicates the status of the active port opening and the session id that was assigned to it. 
|                       |     |        |                      |       | - [ 15:00] The session id of the opened connection.
|                       |     |        |                      |       | - [ 23:16] The status of the port opening.
|                       | in  | [15:0] | siROL_Nts_Tcp_ClsReq | AXI4S | TCP close  session request.
|                       | out | [ 7:0] | soROL_Nts_Tcp_ClsRep | AXI4S | TCP close  session reply (0=closed/1=opened). 
| Rx Ctrl Interface     | in  | [15:0] | siROL_Nts_Tcp_LsnReq | AXI4S | TCP listen port request.
|  (.i.e Shell-->Role)  | out | [ 7:0] | soROL_Nts_Tcp_LsnRep | AXI4S | TCP listen port reply (0=closed/1=opened).  



---
**Trivia**: The [moon Kale](https://en.wikipedia.org/wiki/Kale_(moon))
