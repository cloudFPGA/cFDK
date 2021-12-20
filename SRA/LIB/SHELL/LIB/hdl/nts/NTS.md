# ABOUT: _cloudFPGA/SHELL/FMKU60/src/nts_
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master/SRA/LIB/SHELL/LIB/hdl/nts/NTS.md)


## Overview:

This directory contains the toplevel design of the networking subsystem that is instantiated by the shell of the current target platform for transferring data sequences between the user application layer and the underlaying Ethernet media layers.

From an Open Systems Interconnection (OSI) model point of view, this subsystem module implements the Network layer (L3), the Transport layer (L4) and the Session layer (L5) of the OSI model.

We use the generic terminology of "Network-Transport-Session" (NTS) to refer to the different implementations of this networking stack. For example, the TCP/IP model makes use of the Internet Protocol (IP) for the network layer, the Transmission Control Protocol (TCP) and/or the User Datagram Protocol (UDP) for the transport layer.

## NTS Structure: TCP/IP-based
  
 * [NTS]
  * [TcpIp]
  * [TODO]
  
  
  
