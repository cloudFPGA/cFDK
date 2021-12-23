#  ~cFDK/SRA/LIB/SHELL/LIB/hls/NTS

This directory contains the **High-Level Synthesis (HLS)** code used to generate the **Network Transport Stack (NTS)** used by the various shells of the *cloudFPGA* platform. 

## Preliminary
Please consider taking a quick look at the [documentation of the NTS](../../../../../../DOC/NTS) the **NTS** before diving or contributing to this part of the *cloudFPGA* project.


### Name & Description of the HLS components

| Name                          |  Acronym   | Description                                                       |
|:----------------------------- |:----------:|:------------------------------------------------------------------|
| [arp_server](./arp_server)    | **ARS**   | Address Resolution Protocol (ARP) Server.          
| [icmp_server](./icmp_server)  | **ICMP**  | Internet Control Message Protocol (ICMP) server.      
| [iprx_handler](./iprx_handler)| **IPRX**  | IP Receiver frame handler (IPRX).                     
| [iptx_handler](./iptx_handler)| **IPTX**  | IP Transmit frame handler (IPTX).                    
| [tcp_role_if](./tcp_role_if)  | **TRIF**  | TCP Role Interface.                                   
| [toe](./toe)                  | **TOE**   | TCP Offload Engine.                                   
| [udp](./udp)                  | **UDP**   | UDP engine.                                           
| [udp_mux](./udp_mux)          | **UDMX**  | UDP Multiplexer.                                      
| [udp_role_if](./udp_role_if)  | **URIF**  | UDP Role Interface.                                    




<br>
| [dhcp_client](./dhcp_client)  | **DHCP**  | Dynamic Host Configuration Protocol (DHCP) client.                
