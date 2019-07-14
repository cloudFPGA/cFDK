# HDL Coding Style and Conventions
This document describes the conventions used by the **cloudFPGA Development Kit (cFDK)** to name HLD signals, HDL processes and HDL variables. 

## Preliminary
The following naming rules aim at improving the readability of the HDL code and at facilitating the understanding of the various components instantiated by the *cFDK*, while restricting the documentation to a minimum.  

For example, one of the general guidelines is to always explicitly indicate the source and/or the destination of a signal into the name of the signal itself. 
As you will read, some of these rules may preclude the generic re-use of a process or a variable because their names are too specific or too much spelled out (.e.g a signal name may carry both the name  of its source and its destination). 
This drawback is acknowledged and is accepted since the high-level components of the *cFDK* are mostly unique and are barely replicated. Instead, these high-level components come with large number of I/O ports and interfaces which are easier to understand and to interconnect if they use explicit and/or descriptive names.
Notice however that these rules are intended to vanish and to be replaced by more traditional HDL naming conventions as the design becomes more generic during the top-down specification. This transition typically occurs when instantiating specific IP cores or generic template blocks such as FIFOs or RAMs.

## General Naming Conventions
The following general rules apply to the **identifiers** used to name objects when coding in VHDL or Verilog. Object items which can be named are: constants, variables, types, entities, architectures, modules, instances, configuration, signals, ports, processes, procedures, functions, libraries and packages.

The cFDK applies the following naming conventions to name its identifiers:  
- All identifiers use mixed casing (i.e. *CamelCase* style) except instantiated entities and IP blocks which only use upper-case letters.
- If applicable, an identifier starts with a lower-cased <[_**prefix**_](#prefix)> that indicates the type of the named object (e.g. '_s_' for signal or '_pi_” for Port-In).
    
- If applicable, an identifier must be followed by a <[_**suffix**_](#suffix)> indicating special properties of the named object (e.g. “__n_” for active low or “_En_” for Enable).
    
## Instance Names
An instance is a VHDL component or a Verilog module.
- An instance name is an abbreviation (*e.g.* **TOE**) formed with initial or significant letters of the instantiated component or module (*i.e.* *TcpOffloadEngine*). 
- An instance name is two to six characters long. 
- An instance name is always written in **UPPER-CASE**. 

### Main Instances Names

| Abbreviation    | Description
|:----------------|:-----------------------------------------
| **CAM**         | Content Addressable Memory
| **ICMP**        | Internet Control Message Protocol server                  
| **IPRX**        | Internet Protocol packet Receiver
| **IPTX**        | Internet Protocol packet Transmitter
| **L2MUX**       | Layer-2 Multiplexer                                       
| **L3MUX**       | Layer-3 Multiplexer                      
| **MEM**         | Memory sub-system
| **TOE**         | TCP Offload Engine  
| **TRIF**        | TCP Role Interface (alias APP)                       



========================================


- If applicable, an identifier contains a string <_**\_FROM\_**_> (in upper-case) indicating the object it belongs to, or it is sourced from. This is typically the name of an instantiated entity if the name relates to a signal (e.g. “_sETH_ShlClk_”), or eventually the name of a package if the name relates to a type or a constant (e.g. “_c__EMIF___AddrWidth_”).
    
-   If applicable, an identifier contains one <_**Interface**____ > and zero or plus <_**SubInterface_**_> (all in in CamelCase) indicating the interface and sub-interface names (s) of the object it belongs to, or it is sourced from (e.g “_sNTS___Eth_AxisWrite___tdata_”). The idea of the interface and sub-interface(s) is to group a set of signals under common interface and sub-interface names, and is to be thought here as a replacement for the VHDL record construct which is not supported by many synthesis tools.






<FONT COLOR="#3333ff">This BLUE</FONT> and this BLACK
<FONT COLOR="#009900">This is GREEN
<FONT COLOR="#ff3333">This is RED




